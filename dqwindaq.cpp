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
bool dqScanning=false;

static int state=0;
static int cmdStrindex=0;
dqCal_type dqCal;

int dqEEPROMInit(void)
{
  int i;
  dqCal.structrev=DQSTRUCT_REV;  
  dqCal.hardwarerev=0;
  sprintf(dqCal.key, "0123456789ABCDEF"); //required by Windaq
  sprintf(dqCal.serial_n, "8888888888");  //required by Windaq
  sprintf(dqCal.lastCalDate, "6214F19E"); //2022/2/22: 2:22:22

  for (i=0; i<8; i++){
    dqCal.adc_scale[i]=DQDEFAUL_SCALE;
    dqCal.adc_offset[i]=DQDEFAUL_OFFSET;
  }
}

/*This is to be used to find out the true scale/offset*/
int dqDropCal(void)
{
  int i;
  uint8_t *pc =(uint8_t *)&dqCal;

  for (int i=0; i<sizeof(dqCal); i++) {
    pc[i]=EEPROM.read(i);
  }

  for (i=0; i<8; i++){
    dqCal.adc_scale[i]=DQBASE_SCALE;
    dqCal.adc_offset[i]=0;
  }
}

int dqReceiveChar (int c)
{
  int r=0;
    if ((c==0)||(c=='_')){
      state=10;
    }
    else {
      if (!dqScanning)
        SerialUSB.write(c);
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
    if (cc!='S') dqWindaq=true;
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
    if (dqCmd == DQSTR_INFO) command = DQCMD_INFO;
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
		else if (dqCmd ==DQSTR_RANGE) command = DQCMD_RANGE;
		else if (dqCmd ==DQSTR_EOL) command = DQCMD_EOL;
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