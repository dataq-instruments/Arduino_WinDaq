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

#include <TimerTC3.h>
#include "dqwindaq.h"

#define ADC_BUFFER 6000   
#define ADCPacer_Timer TimerTc3 
#define MAXADCHANNEL 8

int cmd_type;
int ADCChannelCount=1;
int ADCChannelIdx=0;
int channellist[32]={0,1,2,3};
int ActualChannelCount=1;
int mode=1;
char tchar[16];
float RequestedSampleRate;
float TrueSampleRate=0.0;
int number;

char ADCdata[ADC_BUFFER];
int wADCdataIdx=0;
int rADCdataIdx=0;
bool SendS0=false;
int ch=0;

void setup() {
  int i;

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

  ADCPacer_Timer.initialize(1000);    // The default interval is 1000000 microseconds
  ADCPacer_Timer.attachInterrupt(adcPacer_isr);
 
  NVIC_SetPriority(TC3_IRQn, 0);    // Set the Nested Vector Interrupt Controller (NVIC) priority for the TIMER3 to 0 (highest) 
  
  SerialUSB.begin(115200);

  while (!SerialUSB) {
    ; // wait for SerialUSB port to connect. 
  }
}

void adcPacer_isr() { 
  if  (dqScanning)
    ADC->SWTRIG.bit.START = 1;                         // Initiate a software trigger to start an ADC conversion
}

void loop() {
  int i;

  while (SerialUSB.available()){
    if (dqReceiveChar (SerialUSB.read())==DQCMD_AVAILABLE){
      dqParseCommand(dqCmdStr);
      cmd_type= dqMatchCommand(dqCmd);
      if (cmd_type>DQCMD_NOP) execCommand(cmd_type);
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
    adc_reg =ADC->RESULT.reg;     
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
          sprintf(tchar, "\n%d ", adc_reg);
        }
        else{
          sprintf(tchar, "%d ", adc_reg);
        }
        i=strlen(tchar);
        for (j =0; j<i; j++){
          ADCdata[wADCdataIdx++]=tchar[j];
          if (wADCdataIdx>=ADC_BUFFER) wADCdataIdx=0;
        }
        break;
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

    ADC->INTFLAG.bit.RESRDY = 0;                     // Clear the RESRDY flag
    while(ADC->STATUS.bit.SYNCBUSY);                 // Wait for read synchronization
  }
}


void execCommand(int cmd)
{
  int i;
  switch (cmd)
  {
    case DQCMD_DI145A:
      switch (dqPar1.toInt()){
        case 4:
          SerialUSB.print("0123456789ABCDEF");
          break;
        case 3:
          SerialUSB.print("00000000");
          break;
        case 2:
          SerialUSB.print("73");
          break;
        case 1:
          SerialUSB.print("1880");
          break;
        case 7:
          SerialUSB.print("604F443A");
          break;
        default:
          break;
      }
      break;
    case DQCMD_DI145N:
      SerialUSB.print("5BBCB20303");
      break;
    case DQCMD_START:
      start_stop(1);
      break;
    case DQCMD_STOP:
      start_stop(0);
      break;      
    case DQCMD_DI145S:
      start_stop (dqPar1.toInt());
      break;
    case DQCMD_SAMPLERATE:
      if (dqPar1.length ()==0){
        SerialUSB.print(dqCmd+" "+ String(TrueSampleRate, 6));
      }
      else{
        RequestedSampleRate=dqPar1.toFloat();
        TrueSampleRate=findTrueSampleRate(RequestedSampleRate);
      }
      break;  
    case DQCMD_SLIST:
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
    default:
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
    dqScanning=false;
    dqWindaq=false;
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
    SendS0=true;
  }
}

float findTrueSampleRate (float f)
{
  /*We set the max throughput rate at 10K, higher than this we should adjust ADC configuration*/
  float r=0.0;
  if (f<0.01)f=0.01;
  int MyAvg=ADC_AVGCTRL_SAMPLENUM_16;
  long temp=(long)(1000000./(f*(float)ADCChannelCount));

  if (temp<80.)temp=80;
  if (temp>100000000.)temp=100000000;

  ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV16 |       // Divide Clock ADC GCLK (48Mhz) by a prescaler of 16 (48MHz/16 = 3 MHz), 
                  ADC_CTRLB_RESSEL_16BIT;           // Set the ADC resolution to 16 bits
  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization  

  if (temp<=100) MyAvg=ADC_AVGCTRL_SAMPLENUM_16;
  else if (temp<200) MyAvg=ADC_AVGCTRL_SAMPLENUM_32;
  else if (temp<400) MyAvg=ADC_AVGCTRL_SAMPLENUM_64;
  else if (temp<800) MyAvg=ADC_AVGCTRL_SAMPLENUM_128;
  else if (temp<1600) MyAvg=ADC_AVGCTRL_SAMPLENUM_256;  
  else if (temp<3200) MyAvg=ADC_AVGCTRL_SAMPLENUM_512;
  else MyAvg=ADC_AVGCTRL_SAMPLENUM_1024;

  ADC->AVGCTRL.reg = MyAvg |      
                    ADC_AVGCTRL_ADJRES(8);
  while(ADC->STATUS.bit.SYNCBUSY);                   // Wait for synchronization  
  
  //ADCPacer_Timer.initialize(100);    // The default interval is 1000000 microseconds
  ADCPacer_Timer.initialize(temp);    // The default interval is 1000000 microseconds
  r=1000000./((float)temp*(float)ADCChannelCount);

  return (float)r;
}