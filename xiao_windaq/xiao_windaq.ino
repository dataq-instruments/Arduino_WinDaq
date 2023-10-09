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

//#define HISPEED
//#define HIRES
//#define INCLUDE_3DACC

#ifdef INCLUDE_3DACC
#undef HISPEED
/*Add accelerometer to data stream, see https://github.com/Seeed-Studio/Seeed_Arduino_LIS3DHTR
With this option on, you will not be able to run normal analog channel at full speed*/
#include "LIS3DHTR.h"
#include <Wire.h>
LIS3DHTR<TwoWire> LIS; //IIC
float I2CMax=16;
#define WIRE Wire

volatile int I2Cdata[16];
#endif

/*BURSTRATE should be derivble from 1M clock*/
#ifdef HISPEED
#define BURSTRATE 20000
#undef HIRES
#else
#ifdef HIRES
#define BURSTRATE 100
#else
#ifdef INCLUDE_3DACC
#define BURSTRATE 400
#else
#define BURSTRATE 8000
#endif
#endif
#endif


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

volatile long adcAve[8];
long adcDec;
long adcDec1;
volatile long adcDecCounter[8];

char ADCdata[ADC_BUFFER];

volatile int ImdADCdata[32];
int wADCdataIdx=0;
int rADCdataIdx=0;
bool SendS0=false;
int ch=0x1000;
int HaveLIS=1;

void adcPacer_isr();
float findTrueSampleRate (float f);
void InitADC(void);
void LoadConfiguration (void);

void setup() {
  InitADC();

  dqLoadConfiguration();

  SerialUSB.begin(115200);

  NVIC_SetPriority(DMAC_IRQn, 2);
  NVIC_SetPriority(USB_IRQn, 2);

  while (!SerialUSB) {;} // wait for SerialUSB port to connect. 

#ifdef INCLUDE_3DACC
  LIS.begin(WIRE, LIS3DHTR_ADDRESS_UPDATED); //IIC init
  delay(100);
  LIS.setOutputDataRate(LIS3DHTR_DATARATE_100HZ);
  dqGainGroup=0x75; //Four independent gain chanels, three group gain channels
#endif
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
#ifdef INCLUDE_3DACC    
    if (HaveLIS)
    {
      I2Cdata[0]=LIS.getAccelerationX()*(32767./I2CMax);
      I2Cdata[1]=LIS.getAccelerationY()*(32767./I2CMax);
      I2Cdata[2]=LIS.getAccelerationZ()*(32767./I2CMax);
    }
#endif    
  }
  else {
    if (SendS0){
      SerialUSB.print("S0");
      SendS0=false;
    }
  }
}

void InitADC(void){
  int i;
  ADC->INPUTCTRL.bit.MUXPOS = g_APinDescription[channellist[0]].ulADCChannelNumber;                   // Set the analog input to A1, because plusPin =1?
  ADC->INPUTCTRL.bit.MUXNEG = ADC_INPUTCTRL_MUXNEG_GND_Val;
  ADC->REFCTRL.bit.REFSEL=ADC_REFCTRL_REFSEL_INTVCC1_Val; //ADC_REFCTRL_REFSEL_INT1V_Val    
    
  ADC->INPUTCTRL.bit.GAIN =ADC_INPUTCTRL_GAIN_DIV2_Val; //default is 1/2 gain
  ADC->INPUTCTRL.bit.INPUTOFFSET =0; //Watch out, this is NOT the offset adjustment for ADC reading!

  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization
  
  ADC->SAMPCTRL.bit.SAMPLEN = 0x04;                  // Set max Sampling Time Length (6 bit integer) to half divided ADC clock pulse (2.66us), sampling time = (SAMPLEEN+1)*CLK_ADC/2), see pag 797
  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization

  ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV16 |       // Divide Clock ADC GCLK (48Mhz) by a prescaler of 16 (48MHz/16 = 3 MHz), 
                   ADC_CTRLB_RESSEL_16BIT;           // Set the ADC resolution to 16 bits
  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization  

#ifdef HISPEED
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_4 |      //256 for 1 khz 128 for 2khz, 16 for 16K, lower number reduces the amplitude (see page 787 of data sheet about accumulation)
                     ADC_AVGCTRL_ADJRES(8);
#else
#ifdef HIRES
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1024 |      //256 for 1 khz 128 for 2khz, 16 for 16K, lower number reduces the amplitude (see page 787 of data sheet about accumulation)
                     ADC_AVGCTRL_ADJRES(8);
#else
#ifdef INCLUDE_3DACC
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_256 |      //256 for 1 khz 128 for 2khz, 16 for 16K, lower number reduces the amplitude (see page 787 of data sheet about accumulation)
                     ADC_AVGCTRL_ADJRES(8);
#else
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_32 |      //256 for 1 khz 128 for 2khz, 16 for 16K, lower number reduces the amplitude (see page 787 of data sheet about accumulation)
                     ADC_AVGCTRL_ADJRES(8);
#endif                     
#endif                     
#endif                     
  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization  
  
  NVIC_SetPriority(ADC_IRQn, 1);    // Set the Nested Vector Interrupt Controller (NVIC) priority for the ADC to 1 (0 is the highest) 
  NVIC_EnableIRQ(ADC_IRQn);         // Connect the ADC to Nested Vector Interrupt Controller (NVIC)
  ADC->INTENSET.reg = ADC_INTENSET_RESRDY;           // Generate interrupt on result ready (RESRDY)
  ADC->CTRLA.bit.ENABLE = 1;                         // Enable the ADC
  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization

  ADCPacer_Timer.initialize(1000000/BURSTRATE);    // The base burst throughput rate is 40000s/s and this is the skew between channels
  ADCPacer_Timer.attachInterrupt(adcPacer_isr);

  findTrueSampleRate(RequestedSampleRate);

  NVIC_SetPriority(TC3_IRQn, 0);    // Set the Nested Vector Interrupt Controller (NVIC) priority for the TIMER3 to 0 (highest) 

  pinMode(9, OUTPUT); //Use A9 as digital ouptut

#ifdef INCLUDE_3DACC
  dqTotalChannel=7;
#else
  dqTotalChannel=4;
#endif

  for (i=0; i<16; i++) ImdADCdata[i]=0;
}

void adcPacer_isr() { 
  if  (dqScanning)
    ADC->SWTRIG.bit.START = 1;                         // Initiate a software trigger to start an ADC conversion
}

void ADC_Handler()
{
  int adc_reg ;                       // Results counter
  int i, j;
  long fadc_reg;

  if (ADC->INTFLAG.bit.RESRDY)                       // Check if the result ready (RESRDY) flag has been set
  { 
    fadc_reg=(long)ADC->RESULT.reg;  
#ifdef HISPEED
    fadc_reg=fadc_reg<<2;
#endif

    /*digital calibration*/
    fadc_reg = fadc_reg-DQ_MIDPOINT+(long)dqCal.adc_offset[channellist[ch]][0];
    fadc_reg = fadc_reg*(long)dqCal.adc_scale[channellist[ch]][0];
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
#ifndef HIRES      
      fadc_reg=adcAve[ch]/(adcDec1);
#endif
      adcAve[ch]=0;

      #ifdef INCLUDE_3DACC
      if ((channellist[ch]&0xF)>=MAXADCHANNEL){
         /*We are dealing with artificial channels*/
         fadc_reg=I2Cdata[(channellist[ch]&0x7)-MAXADCHANNEL];
      }
      #endif
      
      ImdADCdata[ch]=fadc_reg;
      
      if (dqWindaq){
#ifdef HISPEED
        fadc_reg=fadc_reg&0XFF80; //only 9-bit resolution for 40K
#else        
#ifndef HIRES
        fadc_reg=fadc_reg&0XFFF0;
#endif        
#endif        
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
            #ifdef INCLUDE_3DACC   
              adc_reg=I2Cdata[channellist[(ADCChannelIdx&0xF)]-MAXADCHANNEL]^DQ_INVERTSIGN;
            #else
              adc_reg=0;
            #endif   
              ADCdata[wADCdataIdx++]=(adc_reg>>1)|1;
              ADCdata[wADCdataIdx++]=(adc_reg>>8)|1;
              if (wADCdataIdx>=ADC_BUFFER) wADCdataIdx=0;
            }
            else{
              #ifdef INCLUDE_3DACC   
                adc_reg=I2Cdata[channellist[(ADCChannelIdx&0xF)]-MAXADCHANNEL];
              #else
                adc_reg=0;
              #endif  
              if ((dqMode&0x7f)==0){
                ADCdata[wADCdataIdx++]=adc_reg;
                ADCdata[wADCdataIdx++]=adc_reg>>8;
                if (wADCdataIdx>=ADC_BUFFER) wADCdataIdx=0;
              }
              else{ /*ASCII mode, if you wish to implement*/
              }
            }
            ImdADCdata[ADCChannelIdx]=adc_reg;

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
      SerialUSB.print(" ");
      if (dqScanning){
        imdstr[0]=0;
        for (i=0;i<ADCChannelCount;i++){
          sprintf(&imdstr[strlen(imdstr)], "%d, ", ImdADCdata[i]);
        }
        sprintf(&imdstr[strlen(imdstr)-2], dqeol);
        SerialUSB.print(imdstr);
      }
      else SerialUSB.print(dqeol);
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
        SerialUSB.print(" ");
        SerialUSB.print(String(TrueSampleRate, 6));
      }
      else{
        RequestedSampleRate=dqPar1.toFloat();
        TrueSampleRate=findTrueSampleRate(RequestedSampleRate);
      }
      SerialUSB.print(dqeol);
      break;  
    case DQCMD_SLIST: //Required by Windaq
      /*Actual ADC channels are always in front, and it should not duplicated*/
      if (dqPar1.length ()==0){
        SerialUSB.print(" ");
        SerialUSB.print(ADCChannelCount);
        SerialUSB.print(dqeol);
        break;
      }
      i=dqPar1.toInt();
      if (i==0){
        ADCChannelCount=1;
        ActualChannelCount=0;
        channellist[0]=dqPar2.toInt()&0xf;
        if ((channellist[0]&0xf)<MAXADCHANNEL){
          ActualChannelCount=1; //Only the first MAXADCHANNEL channels, in sequential order 
        }        
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
      #ifdef INCLUDE_3DACC
        if ((dqPar2.toInt()&0xf)>=MAXADCHANNEL){ /*Check for non-normal ADC channels*/
          switch ((dqPar2.toInt()>>8)&0x3){
            case 1:
              LIS.setFullScaleRange(LIS3DHTR_RANGE_8G); //Gain of 
              I2CMax=8;
              break;
            case 2:
              LIS.setFullScaleRange(LIS3DHTR_RANGE_4G); //Gain of 
              I2CMax=4;
              break;
            case 3:
              LIS.setFullScaleRange(LIS3DHTR_RANGE_2G); //Gain of 
              I2CMax=2;
              break;
            default:
              LIS.setFullScaleRange(LIS3DHTR_RANGE_16G); //Gain of 
              I2CMax=16;
              break;
          }
        }
      #endif
      SerialUSB.print(dqeol);
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
  float r=0.0;
  int i;
  if (f<0.2)f=0.2;

  if (ActualChannelCount==0)ActualChannelCount=1;
  long temp=(long)(BURSTRATE/(f*(float)ActualChannelCount));

  if (temp<1)temp=1;
  if (temp>60000)temp=60000;

  r=BURSTRATE/(((float)temp*(float)ActualChannelCount));

  adcDec1=temp;
  temp=temp-1;
  for (i=0; i<MAXADCHANNEL; i++){ 
    adcAve[i]=0;
    adcDecCounter[i]=temp;
  }
  adcDec=temp;

  return (float)r;
}
