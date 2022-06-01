/*
  Copyright (c) 2022 DATAQ Instruments.  All right reserved.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "TimerTC3.h"
#include "dqwindaq.h"
#include <FlashAsEEPROM.h>

#define ADC_BUFFER 5000   
#define ADCPacer_Timer TimerTc3 
#define MAXADCHANNEL 8
#define SOFTWARE_REV 0
#define HARDWARE_REV 0

#define ALLOW_IMD    1

//#define RELEASE

// See SAMD21 datasheet, section "10.3.3 Serial Number"
#define SERNO0 ((uint32_t *)0x0080a00c)
#define SERNO1t3 ((uint32_t *)0x0080a040)

uint32_t samd21_serno[4];

void get_samd21_serno(uint32_t s[4]) {
  s[0] = *SERNO0;
  s[1] = SERNO1t3[0];
  s[2] = SERNO1t3[1];
  s[3] = SERNO1t3[2];
}

int cmd_type;
int ADCChannelCount=1;
int ADCChannelIdx=0;
int channellist[32]={0,1,2,3};
int ActualChannelCount=4;
int mode=1;
char tchar[32];
char eol[4] = "\r";
float RequestedSampleRate=1.0;
float TrueSampleRate=1.0;
int number;
int cnn=0;

long adcAve[8];
long adcDec;
long adcDec1;
long adcDecCounter[8];

char ADCdata[ADC_BUFFER];

//#ifdef ALLOW_IMD  
int ImdADCdata[32];
//#endif 
int wADCdataIdx=0;
int rADCdataIdx=0;
bool SendS0=false;
int ch=0x1000;

void adcPacer_isr();
float findTrueSampleRate (float f);

void setup() {
  int i;
  uint8_t *pc =(uint8_t *)&dqCal;

  ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[channellist[0]].ulADCChannelNumber;                   // Set the analog input to A1, because plusPin =1?
  ADC->INPUTCTRL.bit.MUXNEG = ADC_INPUTCTRL_MUXNEG_GND_Val;
  ADC->REFCTRL.bit.REFSEL=ADC_REFCTRL_REFSEL_INTVCC1_Val;
  //ADC->REFCTRL.bit.REFSEL=ADC_REFCTRL_REFSEL_INT1V_Val;
  
  ADC->INPUTCTRL.bit.GAIN =ADC_INPUTCTRL_GAIN_DIV2_Val; //default is 1/2 gain
  ADC->INPUTCTRL.bit.INPUTOFFSET =0; //Watch out, this is NOT the offset adjustment for ADC reading!

  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization
  
  ADC->SAMPCTRL.bit.SAMPLEN = 0x04;                  // Set max Sampling Time Length (6 bit integer) to half divided ADC clock pulse (2.66us), sampling time = (SAMPLEEN+1)*CLK_ADC/2), see pag 797
  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization

  ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV16 |       // Divide Clock ADC GCLK (48Mhz) by a prescaler of 16 (48MHz/16 = 3 MHz), 
                   ADC_CTRLB_RESSEL_16BIT;           // Set the ADC resolution to 16 bits
  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization  

  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_16 |      //256 for 1 khz 128 for 2khz, 16 for 16K, lower number reduces the amplitude (see page 787 of data sheet about accumulation)
                     ADC_AVGCTRL_ADJRES(8);
  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization  
  
  NVIC_SetPriority(ADC_IRQn, 1);    // Set the Nested Vector Interrupt Controller (NVIC) priority for the ADC to 0 (highest) 
  NVIC_EnableIRQ(ADC_IRQn);         // Connect the ADC to Nested Vector Interrupt Controller (NVIC)
  ADC->INTENSET.reg = ADC_INTENSET_RESRDY;           // Generate interrupt on result ready (RESRDY)
  ADC->CTRLA.bit.ENABLE = 1;                         // Enable the ADC
  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization

  if (!EEPROM.isValid()) {
    dqEEPROMInit();
    for (int i=0; i<sizeof(dqCal); i++) {
      EEPROM.write(i, pc[i]);
    }
    //#ifdef RELEASE
    EEPROM.commit();
    //#endif
            // commit() saves all the changes to EEPROM, it must be called
            // every time the content of virtual EEPROM is changed to make
            // the change permanent.
            // This operation burns Flash write cycles and should not be
            // done too often. See readme for details:
            // https://github.com/cmaglie/FlashStorage#limited-number-of-writes

    EEPROM.isValid();
  } 
  else{
    for (int i=0; i<sizeof(dqCal); i++) {
      pc[i]=EEPROM.read(i);
    }
  }

  findTrueSampleRate(4000);
  ADCPacer_Timer.initialize(125);    // The base burst throughput rate is 8000s/s and this is the skew between channels
  ADCPacer_Timer.attachInterrupt(adcPacer_isr);

  findTrueSampleRate(RequestedSampleRate);
 
  NVIC_SetPriority(TC3_IRQn, 0);    // Set the Nested Vector Interrupt Controller (NVIC) priority for the TIMER3 to 0 (highest) 

  pinMode(9, OUTPUT); //Use A9 as digital ouptut
  
  SerialUSB.begin(115200);

  while (!SerialUSB) {
    ; // wait for SerialUSB port to connect. 
  }
}

void adcPacer_isr() { 
  //#ifndef ALLOW_IMD
  if  (dqScanning)
  //#endif
    ADC->SWTRIG.bit.START = 1;                         // Initiate a software trigger to start an ADC conversion
}

void loop() {
  int i;
  uint8_t *pc =(uint8_t *)&dqCal;

  while (SerialUSB.available()){
    if (dqReceiveChar (SerialUSB.read())==DQCMD_AVAILABLE){
      dqParseCommand(dqCmdStr);
      cmd_type= dqMatchCommand(dqCmd);
      if (cmd_type>DQCMD_NOP) 
        execCommand(cmd_type);
      else{
        if  (dqScanning==0) SerialUSB.print("Unknown command");
      }
    }
  }

  if (dqScanning){
    if (wADCdataIdx!=rADCdataIdx){
      if (SerialUSB.availableForWrite()>0){
        if (wADCdataIdx>rADCdataIdx){
          i=wADCdataIdx-rADCdataIdx;
          if (SerialUSB.availableForWrite()<i) i=SerialUSB.availableForWrite();
          SerialUSB.write(&ADCdata[rADCdataIdx], i);
          rADCdataIdx=rADCdataIdx+i;
        }
        else{ /*send all until the end of buffer*/
          i=ADC_BUFFER-rADCdataIdx;
          if (SerialUSB.availableForWrite()<i) i=SerialUSB.availableForWrite();
          SerialUSB.write(&ADCdata[rADCdataIdx],i);
          rADCdataIdx=rADCdataIdx+i;
          if (rADCdataIdx==ADC_BUFFER)rADCdataIdx=0;
        }
      }
    }
  }
  else {
    if (SendS0){
      SerialUSB.print("S0");
      SendS0=false;
    }
  }
}


void ADC_Handler()
{
  int adc_reg ;                       // Results counter
  int i, j;

  if (ADC->INTFLAG.bit.RESRDY)                       // Check if the result ready (RESRDY) flag has been set
  { 
    long fadc_reg =(unsigned long)ADC->RESULT.reg;     
    int adc_reg;

    /*digital calibration*/
    fadc_reg = fadc_reg-DQ_MIDPOINT+(long)dqCal.adc_offset[channellist[ch]];
    fadc_reg = fadc_reg*(long)dqCal.adc_scale[channellist[ch]];
    fadc_reg=fadc_reg/DQBASE_SCALE;

    if (fadc_reg>DQ_CEILINGP)fadc_reg=DQ_CEILINGP;
    else if (fadc_reg<DQ_CEILINGN)fadc_reg=DQ_CEILINGN;

    adcAve[ch]+=fadc_reg;
    
    if (adcDecCounter[ch]!=0){
      adcDecCounter[ch]--;
      ch++;
      if (ch==ActualChannelCount)ch=0;
      ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[channellist[ch]].ulADCChannelNumber;  
    }
    else{
      /*We have the data now*/
      adcDecCounter[ch]=adcDec;
      
      fadc_reg=adcAve[ch]/(adcDec1);

      adcAve[ch]=0;

      ImdADCdata[ch]=fadc_reg;

      if (dqWindaq){
        adc_reg = (int)fadc_reg;
        adc_reg=adc_reg^DQ_INVERTSIGN;
      }
      else{
        adc_reg = (int)fadc_reg;
      }

      if (dqWindaq) {
        if (ADCChannelIdx==0){ 
          ADCdata[wADCdataIdx++]=(adc_reg>>1)&0xFE;
          ADCdata[wADCdataIdx++]=(adc_reg>>8)|1;
        }
        else{
          ADCdata[wADCdataIdx++]=(adc_reg>>1)|1;
          ADCdata[wADCdataIdx++]=(adc_reg>>8)|1;
        }
        if (wADCdataIdx>=ADC_BUFFER) wADCdataIdx=0;
      }
      else{
        if (dqStream){

          switch (mode){
          case 0:
            ADCdata[wADCdataIdx++]=adc_reg&0xFF;
            ADCdata[wADCdataIdx++]=adc_reg>>8;
            if (wADCdataIdx>=ADC_BUFFER) wADCdataIdx=0;
            break;
          case 0x80:
            ADCdata[wADCdataIdx++]=adc_reg>>8;
            ADCdata[wADCdataIdx++]=adc_reg&0xFF;
            if (wADCdataIdx>=ADC_BUFFER) wADCdataIdx=0;
            break;
          default:
            if (ADCChannelIdx==0){ 
              sprintf(tchar, "%s%d", eol, adc_reg);
            }
            else{
              sprintf(tchar, " %d", adc_reg);
            }
            i=strlen(tchar);
            for (j =0; j<i; j++){
              ADCdata[wADCdataIdx++]=tchar[j];
              if (wADCdataIdx>=ADC_BUFFER) wADCdataIdx=0;
            }
            break;
          }
        }
      }

      ch++;
      if (ch==ActualChannelCount)ch=0;

      ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[channellist[ch]].ulADCChannelNumber;  //

      ADCChannelIdx++;

      if (ADCChannelIdx==ADCChannelCount)
        ADCChannelIdx=0;
      else{
        if (ADCChannelIdx==ActualChannelCount){
          while (ADCChannelIdx<ADCChannelCount){
            /*Here you can add other channel to the stream*/
            if (dqWindaq){ /*Patch blank data*/
              ADCdata[wADCdataIdx++]=1;
              ADCdata[wADCdataIdx++]=1;
              if (wADCdataIdx>=ADC_BUFFER) wADCdataIdx=0;
            }
            else{
              if ((mode&0x7f)==0){
                ADCdata[wADCdataIdx++]=0;
                ADCdata[wADCdataIdx++]=0;
                if (wADCdataIdx>=ADC_BUFFER) wADCdataIdx=0;
              }
              else{ /*No need to patch for ASCII format*/  
              }
            }

            ADCChannelIdx++;
          }
          ADCChannelIdx=0;
        }
      }      
    }
    
    ADC->INTFLAG.bit.RESRDY = 0;                     // Clear the RESRDY flag
    while(ADC->STATUS.bit.SYNCBUSY);                 // Wait for read synchronization
  }
}


void execCommand(int cmd)
{
  int i, j;
  char imdstr[64];

  uint8_t *pc =(uint8_t *)&dqCal;
  switch (cmd)
  {
    case DQCMD_READ:
      if (dqScanning){
        imdstr[0]=0;
        for (i=0;i<ActualChannelCount;i++){
          sprintf(&imdstr[strlen(imdstr)], "%d, ", ImdADCdata[i]);
        }
        sprintf(&imdstr[strlen(imdstr)-2], "\r");
        SerialUSB.print(imdstr);
      }
      break;
    case DQCMD_STREAM:
      dqStream=dqPar1.toInt();
      break;
    case DQCMD_DI145A: //Required by Windaq
      switch (dqPar1.toInt()){
        case 1:
          SerialUSB.print("1880");
          break;
        case 2:
          SerialUSB.print("73");
          break;
        case 3:
          SerialUSB.print("00000000");
          break;
        case 4:
          SerialUSB.print(dqCal.key);
          break;
        case 7:
          SerialUSB.print(dqCal.lastCalDate);
          break;
        default:
          SerialUSB.print("Invalid parameter");
          break;
      }
      break;
    case DQCMD_DI145N: //Required by Windaq
      SerialUSB.print(dqCal.serial_n);
      SerialUSB.print("88"); //required by Windaq for legacy reason,
      break;
    case DQCMD_START: //Required by Windaq
      start_stop(1);
      break;
    case DQCMD_STOP: //Required by Windaq
      start_stop(0);
      SerialUSB.print(dqCmd);
      break;      
    case DQCMD_DI145S: //Required by Windaq
      start_stop (dqPar1.toInt());
      break;
    case DQCMD_RSAMPLERATE:   //Required by Windaq
    case DQCMD_SAMPLERATE: 
      if (dqPar1.length ()==0){
        SerialUSB.print(String(TrueSampleRate, 6));
      }
      else{
        RequestedSampleRate=dqPar1.toFloat();
        TrueSampleRate=findTrueSampleRate(RequestedSampleRate);
      }
      break;  
    case DQCMD_SLIST: //Required by Windaq
      if (dqPar1.length ()==0){
        SerialUSB.print(ADCChannelCount);
        break;
      }
      i=dqPar1.toInt();
      if (i==0){
        ADCChannelCount=1;
        ActualChannelCount=1;
        channellist[i]=dqPar2.toInt()&0xf;
        for (i=1;i<32;i++) channellist[i]=0; 
      }
      else {
        ADCChannelCount++;
        if (i<MAXADCHANNEL){
            if ((dqPar2.toInt()&0xf)<MAXADCHANNEL){
              ActualChannelCount++; //Only the first MAXADCHANNEL channels, in sequential order 
            }
        }
        if (ActualChannelCount>MAXADCHANNEL)ActualChannelCount=MAXADCHANNEL;
        channellist[i]=dqPar2.toInt()&0xf;
      }
      TrueSampleRate=findTrueSampleRate(RequestedSampleRate); //Changing number of channel may affect the true sample rate
      break;
    case DQCMD_ENCODE:
      if (dqPar1.length ()==0){
        break;
      }    
      mode=dqPar1.toInt()&0xff;
      break;
    case DQCMD_DOUT:
      if (dqPar1.length ()==0){
        break;
      }
      i=dqPar1.toInt();   
      if (i==0) 
          digitalWrite(9, LOW);
      else
          digitalWrite(9, HIGH);
      break;  
    case DQCMD_INFO:
      if (dqPar1.length ()==0){
        break;
      }
      i=dqPar1.toInt();
      switch(i){
        case 0: //Required by Windaq
          SerialUSB.print("DATAQ");
          SerialUSB.print(dqeol);
          break;
        case 1: //Required by Windaq 
          SerialUSB.print("1888");
          SerialUSB.print(dqeol);
          break;
        case 2: //Required by Windaq
          SerialUSB.print(SOFTWARE_REV);
          SerialUSB.print(dqeol);
          break;
        case 3: //Required by Windaq
          SerialUSB.print(HARDWARE_REV);
          SerialUSB.print(dqeol);
          break;
        case 6: //Required by Windaq
          SerialUSB.print(dqCal.serial_n); 
          //get_samd21_serno(samd21_serno);
          //sprintf(imdstr, "SAMD21 Serial Number: 0x%08lx%05lx\n", samd21_serno[0], samd21_serno[3]&0xfffff);    /*It seems the two center long words hardly changes, so we won't use them*/
          //Serial.print(imdstr);      
          SerialUSB.print(dqeol);
          break;
        default:
          SerialUSB.print("Invalid parameter");
          SerialUSB.print(dqeol);
          break;
      }
      break;
    case DQCMD_DI145E: //Required by Windaq
      break;
    case DQCMD_WFLASH:  
      if (dqPar1.length ()>0){
        i=dqPar1.toInt();
        if ((i>=2)&&(i<sizeof(dqCal))){ /*The first two bytes are structure rev*/
          if(dqPar2.length ()>0){
            pc[i]=(uint8_t)dqPar2.toInt()&0xff;
          }
        }
        else if (i==-1){
          SerialUSB.print("Flash updating...");
          for (i=0; i<sizeof(dqCal); i++) {
            EEPROM.write(i, pc[i]);
          }
          EEPROM.commit();
          SerialUSB.print("Done");
          SerialUSB.print(dqeol);
        }
      }
      break;
    case DQCMD_RFLASH:
      if (dqPar1.length ()>0){
        if (dqPar1=="init"){
          for (int i=0; i<sizeof(dqCal); i++) {
            pc[i]=EEPROM.read(i);
          }
        }
      }
      for (int i=0; i<sizeof(dqCal); i++) {
        SerialUSB.print(" ");
        SerialUSB.print(pc[i]);
      }
      SerialUSB.print("\r");
      break;
    case DQCMD_EOL:
			switch(dqPar1[0])
			{
			case '0':
				dqeol[0] = '\r';	//eol = carriage return
				dqeol[1] = '\0';
				break;
			case '1':
				dqeol[0] = '\n';	//eol = line feed
				dqeol[1] = '\0';
				break;
			case '2':
				dqeol[0] = '\r';	//eol = carriage return
				dqeol[1] = '\n';	//eol = line feed
				dqeol[2] = '\0';
			  break;
			default:
        break;
			}    
      break;
    default:
      SerialUSB.print("Unsupported command");
      SerialUSB.print(dqeol);
      break;
  }
}

void start_stop(int i){
  if (i!=0){
    dqScanning=true;
    ADCPacer_Timer.start();
  }
  else  {
    ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[channellist[0]].ulADCChannelNumber;   
    if (dqWindaq) SendS0=true;        //required by WinDaq
    dqWindaq=false;
/*#ifdef ALLOW_IMD 
    ADCPacer_Timer.stop();
    findTrueSampleRate(4000);
    ADCPacer_Timer.start();
#else    */
    dqScanning=false;
    ADCPacer_Timer.stop();
/*#endif    */
  }
  ch=0;
  wADCdataIdx=0;
  rADCdataIdx=0;
  ADCChannelIdx=0;  

  if (dqScanning){
    NVIC_EnableIRQ(ADC_IRQn);         // Connect the ADC to Nested Vector Interrupt Controller (NVIC)
  }
  else{
    NVIC_DisableIRQ(ADC_IRQn);         // Connect the ADC to Nested Vector Interrupt Controller (NVIC)
  }
}

float findTrueSampleRate (float f)
{
  /*Use software AVE instead of buildin hardware AVE to minimize skew between channels, but it is a little noisier*/
  float r=0.0;
  int i;
  if (f<0.2)f=0.2;

  long temp=(long)(8000./(f*(float)ADCChannelCount));

  if (temp<1)temp=1;
  if (temp>60000)temp=60000;

  r=8000/(((float)temp*(float)ADCChannelCount));

  adcDec1=temp;
  temp=temp-1;
  for (i=0; i<MAXADCHANNEL; i++){ 
    adcAve[i]=0;
    adcDecCounter[i]=temp;
  }
  adcDec=temp;

  return (float)r;
}