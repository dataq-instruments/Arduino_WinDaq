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

#include <Arduino.h>
#include "dqwindaq.h"
#include <FlashAsEEPROM.h>

String dqCmd;
String dqPar1;
String dqPar2;
String dqPar3;
String dqPar4;
bool dqWindaq=false;
char dqCmdStr[64];
volatile bool dqScanning=false;
int dqStream=1;
int dqMode=1;
int dqTotalChannel=4;
int dqGainGroup=0x5555;
uint32_t samd21_serno[4];

/*The following applies to DATAQ's DI-188 module. For Arduino, please change the range to 0, 3 Volt*/
const String dqChannel[8]={
  "Volt, -10, 10",
  "Volt, -10, 10",
  "Volt, -10, 10",
  "Volt, -10, 10",
  "G, -16, 16",
  "G, -16, 16",
  "G, -16, 16",
  "Deg, -275, 1450"
};

/*GainFlags: {1,2,4,5,8,10,16,20,32,40,50,64,80,100,128,200,256,400,500,512,800,1000,1024,2000,2048,4000,5000,10000,20000,50000,100000,1000000}*/
const int dqGain[8]={
  1,1,1,1,7,7,7,1
};

static int state=0;
static int cmdStrindex=0;
dqCal_type dqCal;
char dqeol[4] = "\r";

void get_samd21_serno(uint32_t s[4]) {
  s[0] = *SERNO0;
  s[1] = SERNO1t3[0];
  s[2] = SERNO1t3[1];
  s[3] = SERNO1t3[2];
}

int dqEEPROMInit(void)
{
  int i, j;
  dqCal.structrev=DQSTRUCT_REV;  
  dqCal.hardwarerev=0;
  get_samd21_serno(samd21_serno);
  sprintf(dqCal.key, "0123456789ABCDEF"); 
  sprintf(dqCal.serial_n, "%08X",  samd21_serno[0]+(samd21_serno[3]&0xfffff));  //Create an unique serial number
  sprintf(dqCal.lastCalDate, "6214F19E"); //2022/2/22: 2:22:22

  for (i=0; i<8; i++){
    for (j=0; j<4; j++){
      dqCal.adc_scale[i][j]=DQDEFAUL_SCALE;
      dqCal.adc_offset[i][j]=DQDEFAUL_OFFSET;
    }
  }
}

int dqReceiveChar (int c)
{
  int r=0;
    if ((c==0)||(c=='_')){
      state=10;
    }
    else {
      if (!dqScanning){
        if (c!=0xD) SerialUSB.write(c);
        //else SerialUSB.write(" ");
      }
      else if (c=='S')  {
        dqScanning=false;
        SerialUSB.write(c);
      }
    }

    switch(state){
      case 0:
        if (c==0xA)break; //Ignore LF
        dqCmdStr[cmdStrindex++]=c;
        dqCmdStr[cmdStrindex]='\0';
        if (c==0xD){
          cmdStrindex=0;
          r=DQCMD_AVAILABLE;
          state=0;
        }
        break;
      case 10:
        state=11; //00 is ignored
        cmdStrindex=0;
        break;
      case 11:  /*The first character of legacy command*/
        dqCmdStr[cmdStrindex++]=c;
        state=12;
        break;
      case 12:  /*The second character of legacy command*/
        dqCmdStr[cmdStrindex++]=c;
        dqCmdStr[cmdStrindex]='\0';
        cmdStrindex=0;
        state=0;
        r=DQCMD_AVAILABLE;
        break;
      default:
        cmdStrindex=0;
        break;
    }
    return r;
}

void dqParseCommand(char cmdt[]){
  int ind1, ind2, ind3, ind4;
  char cc;

  String cmdtemp=dqGetString(cmdt);

  dqPar1="";
  dqPar2="";
  dqPar3="";

  cc=cmdtemp.charAt(0);

  if(((cc >= 'A') && (cc <= 'E')) //Legacy commands for Windaq
    || (cc == 'L')
    || (cc == 'M')
    || (cc == 'N')
    || (cc == 'S')
    || (cc == 'R')
    || (cc == 'H')){
    dqCmd =cmdtemp.substring(0, 1); 
    dqPar1= cmdtemp.substring(1); 
    if (cc!='S') { 
      dqeol[0] = '\r';	//eol = carriage return
			dqeol[1] = '\0';
      dqWindaq=true;
    }
    return;
  }

  ind1 = cmdtemp.indexOf(' ');  //finds location of first ,
  if (ind1<0){
    ind1=cmdtemp.length();
    dqCmd = cmdtemp.substring(0, ind1);   //captures first data String
  }
  else{
    dqCmd = cmdtemp.substring(0, ind1);   //captures first data String
    ind2 = cmdtemp.indexOf(' ', ind1+1 );   //finds location of second 
    if (ind2<0){
      ind2=cmdtemp.length();
      dqPar1 = cmdtemp.substring(ind1+1, ind2);   //captures first data String
    }
    else{
      dqPar1 = cmdtemp.substring(ind1+1, ind2);   //captures first data String
      ind3 = cmdtemp.indexOf(' ', ind2+1 );
      if (ind3<0){
        ind3=cmdtemp.length();
        dqPar2 = cmdtemp.substring(ind2+1, ind3);   //captures first data String
      }
      else{
        dqPar2 = cmdtemp.substring(ind2+1, ind3);   //captures first data String
        dqPar3 = cmdtemp.substring(ind3+1);   //captures first data String
      }
    }
  }
}

int dqMatchCommand(String dqCmd){
    int command;
    if (dqCmd == DQSTR_READ) command = DQCMD_READ;
    else if (dqCmd == DQSTR_INFO) command = DQCMD_INFO;
    else if (dqCmd ==DQSTR_START      ) command = DQCMD_START;
    else if (dqCmd ==DQSTR_STOP       ) command = DQCMD_STOP;
    else if (dqCmd ==DQSTR_SLIST      ) command = DQCMD_SLIST;
    else if (dqCmd ==DQSTR_SRATE      ) command = DQCMD_SAMPLERATE;
    else if (dqCmd ==DQSTR_RRATE      ) command = DQCMD_RSAMPLERATE;
    else if (dqCmd ==DQSTR_FILTER     ) command = DQCMD_FILTER;
    else if (dqCmd ==DQSTR_SAMPLERATE) command = DQCMD_SAMPLERATE;
    else if (dqCmd ==DQSTR_RSAMPLERATE) command = DQCMD_RSAMPLERATE;
    else if (dqCmd ==DQSTR_FILTER) command = DQCMD_FILTER;
    else if (dqCmd ==DQSTR_DI145A) command = DQCMD_DI145A;
    else if (dqCmd ==DQSTR_DI145B) command = DQCMD_DI145B;
    else if (dqCmd ==DQSTR_DI145C) command = DQCMD_DI145C;
    else if (dqCmd ==DQSTR_DI145D) command = DQCMD_DI145D;
    else if (dqCmd ==DQSTR_DI145E) command = DQCMD_DI145E;
    else if (dqCmd ==DQSTR_DI145L) command = DQCMD_DI145L;
    else if (dqCmd ==DQSTR_DI145M) command = DQCMD_DI145M;
    else if (dqCmd ==DQSTR_DI145N) command = DQCMD_DI145N;
    else if (dqCmd ==DQSTR_DI145S) command = DQCMD_DI145S;
    else if (dqCmd ==DQSTR_DI145R) command = DQCMD_DI145R;
    else if (dqCmd ==DQSTR_DI145H) command = DQCMD_DI145H;
    else if (dqCmd ==DQSTR_CHN) command = DQCMD_SLIST;
    else if (dqCmd ==DQSTR_DIN) command = DQCMD_DIN;
    else if (dqCmd ==DQSTR_DOUT) command = DQCMD_DOUT;
    else if (dqCmd ==DQSTR_RESET) command = DQCMD_RESET;
    else if (dqCmd ==DQSTR_ENDO) command = DQCMD_ENDO;
    else if (dqCmd ==DQSTR_FFL) command = DQCMD_FFL;
    else if (dqCmd ==DQSTR_LED) command = DQCMD_LED;
    else if (dqCmd ==DQSTR_ENCODE) command = DQCMD_ENCODE;
    else if (dqCmd ==DQSTR_PGA) command = DQCMD_PGA;
    else if (dqCmd ==DQSTR_DEBUG) command = DQCMD_DEBUG;
    else if (dqCmd ==DQSTR_NOP) command = DQCMD_NOP;
    else if (dqCmd ==DQSTR_NOP2) command = DQCMD_NOP;
    else if (dqCmd ==DQSTR_RANGE) command = DQCMD_RANGE;
    else if (dqCmd ==DQSTR_EOL) command = DQCMD_EOL;
    else if (dqCmd ==DQSTR_WFLASH) command = DQCMD_WFLASH;
    else if (dqCmd ==DQSTR_RFLASH) command = DQCMD_RFLASH;
    else if (dqCmd ==DQSTR_STREAM) command = DQCMD_STREAM;
    else if (dqCmd ==DQSTR_SCALE) command = DQCMD_SCALE;
    else if (dqCmd ==DQSTR_OFFSET) command = DQCMD_OFFSET;
    else if (dqCmd ==DQSTR_RCHN) command = DQCMD_RCHN;
    else if (dqCmd ==DQSTR_RGAIN) command = DQCMD_RGAIN;
    else if (dqCmd ==DQSTR_GGRP) command = DQCMD_GGRP;
    else command=DQCMD_INVALID;
     
    return command;
}

String dqGetString(char arr[])
{
    int len = 0;
    do
    {
        len++;
    } while (arr[len] > '\r');

    String s = arr;

    return s.substring(0,len);
}

int dqLegacyCommand(int cmd)
{
  int i, j;
  char imdstr[64];

  uint8_t *pc =(uint8_t *)&dqCal;
  switch (cmd)
  {
    case DQCMD_NOP:
      break;
    case DQCMD_DI145A: //Required by Windaq
      switch (dqPar1.toInt()){
        case 1:
          SerialUSB.print(MODELNUMBER);
          SerialUSB.print("0"); //Legacy reason
          break;
        case 2:
          SerialUSB.print(FIRMWARE_REV);
          break;
        case 3:
          SerialUSB.print("00000000"); //Legacy reason
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
      cmd=DQCMD_HANDLED;
      break;
    case DQCMD_DI145N: //Required by Windaq
      SerialUSB.print(dqCal.serial_n);
      SerialUSB.print("88"); //required by Windaq for legacy reason,
      cmd=DQCMD_HANDLED;
      break;
    case DQCMD_ENCODE:
      SerialUSB.print(" ");
      if (dqPar1.length ()==0){
        break;
      }    
      dqMode=dqPar1.toInt()&0xff;
      SerialUSB.print(dqeol);
      cmd=DQCMD_HANDLED;
      break;
    case DQCMD_RCHN:
      SerialUSB.print(" ");
      if (dqPar1.length ()==0){
        SerialUSB.print(dqTotalChannel);
      }
      else if (dqPar2.length ()==0){
        SerialUSB.print(dqChannel[dqPar1.toInt()&0x7]);  
      }
      SerialUSB.print(dqeol);
      cmd=DQCMD_HANDLED;
      break;    
    case DQCMD_GGRP:
      SerialUSB.print(" ");
      SerialUSB.print(dqGainGroup);  
      SerialUSB.print(dqeol);
      cmd=DQCMD_HANDLED;
      break;    
    case DQCMD_RGAIN:
      SerialUSB.print(" ");
      for (i=0; i<dqTotalChannel; i++){
        SerialUSB.print(dqGain[i]);
        SerialUSB.print(" ");
      }
      SerialUSB.print(dqeol);    
      cmd=DQCMD_HANDLED;
      break;
    case DQCMD_INFO:
      SerialUSB.print(" ");
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
          SerialUSB.print(MODELNUMBER);
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
        case 4: //Key
          break;  
        case 6: //Required by Windaq
          SerialUSB.print(dqCal.serial_n); 
          SerialUSB.print(dqeol);
          break;
        case 10: //hardware ID
          break;
        default:
          SerialUSB.print("Invalid parameter");
          SerialUSB.print(dqeol);
          break;
      }
      cmd=DQCMD_HANDLED;
      break;
    case DQCMD_DI145E: //Required by Windaq
      cmd=DQCMD_HANDLED;
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
      cmd=DQCMD_HANDLED;
      SerialUSB.print(dqeol);
      break;
    case DQCMD_SCALE:  
      if ((dqPar1.length ()==0)||(dqPar2.length ()==0)||(dqPar3.length ()==0)){
        cmd=DQCMD_HANDLED;
        break;
      }
      i=dqPar1.toInt();
      j=dqPar2.toInt();
      if (((i>=0)&&(i<8))&&((j>=0)&&(j<4)))
        dqCal.adc_scale[i][j]=dqPar3.toInt();
      cmd=DQCMD_HANDLED;       
      SerialUSB.print(dqeol);
      break;    
    case DQCMD_OFFSET:
      if ((dqPar1.length ()==0)||(dqPar2.length ()==0)||(dqPar3.length ()==0)){
        cmd=DQCMD_HANDLED;
        break;
      }
      i=dqPar1.toInt();
      j=dqPar2.toInt();
      if (((i>=0)&&(i<8))&&((j>=0)&&(j<4)))
        dqCal.adc_offset[i][j]=dqPar3.toInt();
      cmd=DQCMD_HANDLED;
      SerialUSB.print(dqeol);
      break;
    case DQCMD_WFLASH:  
      SerialUSB.print(" ");
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
        }
      }
      SerialUSB.print(dqeol);
      cmd=DQCMD_HANDLED;
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
      cmd=DQCMD_HANDLED;    
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
      SerialUSB.print(dqeol);
      cmd=DQCMD_HANDLED;
      break; 
    case DQCMD_STREAM:
      dqStream=dqPar1.toInt();
      cmd=DQCMD_HANDLED;
      break; 
    case DQCMD_CJCDELTA:
      SerialUSB.print(" ");
      SerialUSB.print("NA");
      SerialUSB.print(dqeol);
      cmd=DQCMD_HANDLED;
      break;
    default:
      break;
  }
  return cmd;
}

void dqLoadConfiguration (void)
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

