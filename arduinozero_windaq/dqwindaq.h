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

#define ADC_BUFFER 5000   
#define ADCPacer_Timer TimerTc3 
#define MAXADCHANNEL 4
#define SOFTWARE_REV 0
#define HARDWARE_REV 0

#define ALLOW_IMD    1
//#define RELEASE


// See SAMD21 datasheet, section "10.3.3 Serial Number"
#define SERNO0 ((uint32_t *)0x0080a00c)
#define SERNO1t3 ((uint32_t *)0x0080a040)


#define DQCMD_NOP                0x00
#define DQCMD_HANDLED            0x00   
#define DQCMD_INFO               0x01
#define DQCMD_START              0x02
#define DQCMD_STOP               0x03
#define DQCMD_SLIST              0x04
#define DQCMD_SR                 0x05
#define DQCMD_DEC                0x09
#define DQCMD_FILTER             0x0A
#define DQCMD_RSAMPLERATE        0x25  
#define DQCMD_SAMPLERATE         0x26  
#define DQCMD_DI145A             0x27  
#define DQCMD_DI145B             0x28  
#define DQCMD_DI145C             0x29  
#define DQCMD_DI145D             0x2A  
#define DQCMD_DI145E             0x24  
#define DQCMD_DI145L             0x2B	
#define DQCMD_DI145M             0x2C  
#define DQCMD_DI145N             0x2D  
#define DQCMD_DI145S             0x2E  
#define DQCMD_DI145R             0x2F  
#define DQCMD_DI145H             0x30  
#define DQCMD_CHN                0x31  
#define DQCMD_DIN                0x32  
#define DQCMD_DOUT               0x33  
#define DQCMD_KEY                0x34  
#define DQCMD_RESET              0x35  
#define DQCMD_ENDO               0x36  
#define DQCMD_FFL                0x37  
#define DQCMD_LED                0x38 
#define DQCMD_CHNS               0x39 
#define DQCMD_ENCODE             0x3A 
#define DQCMD_PGA                0x3B  
#define DQCMD_RFLASH	  	       0x3C  
#define DQCMD_WFLASH	  	       0x3D  
#define DQCMD_DEBUG              0x51
#define DQCMD_YMD                0x52
#define DQCMD_HMS                0x53
#define DQCMD_MS                 0x64	
#define DQCMD_RANGE              0x66	
#define DQCMD_EOL                0x67	
#define DQCMD_DECA               0x68	
#define DQCMD_ADV_CMD            0x6F
#define DQCMD_DAC                0x73
#define DQCMD_READ               0x74
#define DQCMD_STREAM             0x75
#define DQCMD_OFFSET             0x76
#define DQCMD_SCALE              0x77
#define DQCMD_INVALID            -1
#define DQCMD_AVAILABLE          1000

#define DQSTR_NOP                "nop"
#define DQSTR_NOP2               "\r"
#define DQSTR_INFO               "info"
#define DQSTR_START              "start"
#define DQSTR_STOP               "stop"
#define DQSTR_SLIST              "slist"
#define DQSTR_SRATE              "srate"
#define DQSTR_RRATE              "rrate"
#define DQSTR_DEC                "dec"
#define DQSTR_FILTER             "filter"
#define DQSTR_HELP               "help"
#define DQSTR_SAMPLERATE         "sr"
#define DQSTR_RSAMPLERATE        "rr"
#define DQSTR_DI145A             "A"
#define DQSTR_DI145B             "B"
#define DQSTR_DI145C             "C"
#define DQSTR_DI145D             "D"
#define DQSTR_DI145E             "E"
#define DQSTR_DI145L             "L"
#define DQSTR_DI145M             "M"
#define DQSTR_DI145N             "N"
#define DQSTR_DI145S             "S"
#define DQSTR_DI145R             "R"
#define DQSTR_DI145H             "H"
#define DQSTR_CHN                "chn"
#define DQSTR_DIN                "din"
#define DQSTR_DOUT               "dout"
#define DQSTR_KEY                "k"
#define DQSTR_RESET              "reset"
#define DQSTR_ENDO               "endo"
#define DQSTR_FFL                "ffl"
#define DQSTR_LED                "led"
#define DQSTR_CHNS               "chns"
#define DQSTR_ENCODE             "encode"
#define DQSTR_PGA                "pga"
#define DQSTR_DEBUG              "debug"
#define DQSTR_RANGE              "range"
#define DQSTR_EOL                "eol"
#define DQSTR_DECA               "deca"
#define DQSTR_MS                 "ms"
#define DQSTR_WFLASH   	  	     "writeflash"	
#define DQSTR_RFLASH   	  	     "readflash"	
#define DQSTR_READ   	  	       "read"	
#define DQSTR_STREAM 	  	       "stream"	
#define DQSTR_OFFSET 	  	       "offset"	
#define DQSTR_SCALE 	  	       "scale"	

#define DQSTRUCT_REV              1

/*Norminal digital calibration constants*/
#define DQDEFAUL_SCALE            18876
#define DQDEFAUL_OFFSET           15

/*Calibration constants*/
#define DQBASE_SCALE              16384
#define DQ_MIDPOINT               32768
#define DQ_CEILING                32767
#define DQ_FLOOR                  -32768
#define DQ_INVERTSIGN             0x8000


struct dqCal_type{
  unsigned short structrev; 
  unsigned short hardwarerev;
  char key [32];
  char serial_n[16];
  char lastCalDate[16];
  short adc_scale[8];
  short adc_offset[8];
};

extern int dqMatchCommand (String cmdstr);
extern String dqGetString(char arr[]);
extern void dqParseCommand(char cmdt[]);
extern int dqReceiveChar(int c);
extern int dqEEPROMInit(void);
extern int dqDropCal(void);
extern int dqDefaultCal(void);
extern int dqLegacyCommand(int cmd);

extern String dqCmd;
extern String dqPar1;
extern String dqPar2;
extern String dqPar3;
extern String dqPar4;
extern bool dqWindaq;
extern char dqCmdStr[];
extern bool dqScanning;
extern dqCal_type dqCal;
extern int dqStream;
extern char dqeol[];
extern int dqMode;
