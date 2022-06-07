/*
  Copyright (c) 2022 DATAQ Instruments, 241 Springside Drive, Akron, OH 44333, USA.  All right reserved.
 
  This project is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
  as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.
  This project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
  See the GNU Lesser General Public License for more details. You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <TimerTC3.h>
#include "dqwindaq.h"
#include <FlashAsEEPROM.h>

int cmd_type;
int ADCChannelCount=1;
int ADCChannelIdx=0;
int channellist[32]={0,1,2,3};
int ActualChannelCount=1;

char tchar[32];
float RequestedSampleRate=1.0;
float TrueSampleRate=1.0;
int number;
int cnn=0;

long adcAve[8];
long adcDec;
long adcDec1;
long adcDecCounter[8];

char ADCdata[ADC_BUFFER];

int ImdADCdata[32];
int wADCdataIdx=0;
int rADCdataIdx=0;
bool SendS0=false;
int ch=0x1000;

void adcPacer_isr();
float findTrueSampleRate (float f);
void InitADC(void);
void LoadConfiguration (void);

void setup() {
  InitADC();

  LoadConfiguration();

  SerialUSB.begin(115200);

  while (!SerialUSB) {;} // wait for SerialUSB port to connect. 
}

void loop() {
  int i;
  uint8_t *pc =(uint8_t *)&dqCal;

  while (SerialUSB.available()){
    if (dqReceiveChar (SerialUSB.read())==DQCMD_AVAILABLE){
      dqParseCommand(dqCmdStr);
      cmd_type= dqMatchCommand(dqCmd);
      if (cmd_type>=DQCMD_NOP) 
        execCommand(cmd_type);
      else{
        if  (dqScanning==0) {
          SerialUSB.print("Unknown command: ");
          SerialUSB.print(dqCmdStr);
        }
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

void InitADC(void){
  ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[channellist[0]].ulADCChannelNumber;                   // Set the analog input to A1, because plusPin =1?
  ADC->INPUTCTRL.bit.MUXNEG = ADC_INPUTCTRL_MUXNEG_GND_Val;
  ADC->REFCTRL.bit.REFSEL=ADC_REFCTRL_REFSEL_INTVCC1_Val;
    
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

  ADCPacer_Timer.initialize(125);    // The base burst throughput rate is 8000s/s and this is the skew between channels
  ADCPacer_Timer.attachInterrupt(adcPacer_isr);

  findTrueSampleRate(RequestedSampleRate);
 
  NVIC_SetPriority(TC3_IRQn, 0);    // Set the Nested Vector Interrupt Controller (NVIC) priority for the TIMER3 to 0 (highest) 

  pinMode(9, OUTPUT); //Use A9 as digital ouptut
}

void adcPacer_isr() { 
  if  (dqScanning)
    ADC->SWTRIG.bit.START = 1;                         // Initiate a software trigger to start an ADC conversion
}

void ADC_Handler()
{
  int adc_reg ;                       // Results counter
  int i, j;

  if (ADC->INTFLAG.bit.RESRDY)                       // Check if the result ready (RESRDY) flag has been set
  { 
    long fadc_reg =(long)ADC->RESULT.reg;     
    int adc_reg;

    /*digital calibration*/
    fadc_reg = fadc_reg-DQ_MIDPOINT+(long)dqCal.adc_offset[channellist[ch]];
    fadc_reg = fadc_reg*(long)dqCal.adc_scale[channellist[ch]];
    fadc_reg=fadc_reg/DQBASE_SCALE;

    if (fadc_reg>DQ_CEILING)fadc_reg=DQ_CEILING;
    else if (fadc_reg<DQ_FLOOR)fadc_reg=DQ_FLOOR;

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

          switch (dqMode){
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
              sprintf(tchar, "%s%d", dqeol, adc_reg);
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
              if ((dqMode&0x7f)==0){
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

  if (dqLegacyCommand(cmd)==DQCMD_HANDLED) return;

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
    case DQCMD_SCALE:  
      if ((dqPar1.length ()==0)||(dqPar2.length ()==0)){
        break;
      }
      i=dqPar1.toInt();
      if ((i>=0)&&(i<8))
        dqCal.adc_scale[i]=dqPar2.toInt();;
      break;    
    case DQCMD_OFFSET:
      if ((dqPar1.length ()==0)||(dqPar2.length ()==0)){
        break;
      }
      i=dqPar1.toInt();
      if ((i>=0)&&(i<8))
        dqCal.adc_offset[i]=dqPar2.toInt();;
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
    default:
      SerialUSB.print("Unsupported command:"+dqCmd);
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

    dqScanning=false;
    ADCPacer_Timer.stop();
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

  long temp=(long)(8000./(f*(float)ActualChannelCount));

  if (temp<1)temp=1;
  if (temp>60000)temp=60000;

  r=8000/(((float)temp*(float)ActualChannelCount));

  adcDec1=temp;
  temp=temp-1;
  for (i=0; i<MAXADCHANNEL; i++){ 
    adcAve[i]=0;
    adcDecCounter[i]=temp;
  }
  adcDec=temp;

  return (float)r;
}

void LoadConfiguration (void)
{
  int i;
  uint8_t *pc =(uint8_t *)&dqCal;

  if (!EEPROM.isValid()) {
    dqEEPROMInit();
    for (int i=0; i<sizeof(dqCal); i++) {
      EEPROM.write(i, pc[i]);
    }
    
    EEPROM.commit();
    EEPROM.isValid();
  } 
  else{
    for (int i=0; i<sizeof(dqCal); i++) {
      pc[i]=EEPROM.read(i);
    }
  }
}