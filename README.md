# Arduino_To_WinDaq Chart Recorder
Originated from our DI-188, this open source project turns any **Arduino®** (Arduino Zero is used here) or **Arduino®-compatible** module such as DI-188 to a [WinDaq](https://www.dataq.com/products/windaq/)-based data acquisition chart recorder. It supports up to 16 analog channels (4 analog channels is demonstrated in the source codes) to accept readings from on-board ADC or sensors connected to grove connectors. 

**To use the project:**
1) Please [follow this instruction](https://www.arduino.cc/en/software#experimental-software) to install Arduino IDE 2.0.  
2) If you don't use DI-188, which is based on Seeeduino XIAO, jump to step 3)
    -  [Follow this instruction](https://wiki.seeedstudio.com/Seeeduino-XIAO/#:~:text=Install%20it.&text=After%20installing%20the%20board%2C%20click,the%20Tools%20%7C%20Serial%20Port%20menu. ) to allow XIAO to be added to Arduino IDE search engine
    - Restart Arduino IDE to allow XIAO to be added to the search engine
    - From ArduinoIDE->Tools->Boards->Board Manager, type in XIAO or Arduino Zero and let IDE download all libraries for your module
       - When you upgrade Arduino IDE, you may need to upgrade the libraries as well
    - Select ArduinoIDE->Tools->Boards->Seeed SAMD boards->Seeeduino XIAO  
    - You may also need to specify the COM port for the module
3) Install Flash library
    - Visit https://github.com/cmaglie/FlashStorage
    - Push Code button and select Download ZIP
    - From Arduino IDE, follow ArduinoIDE->Sketch->Include Library->Add ZIP Library... to add the downloaded library from above step
4) Download the source from above (press the button <> Code and download ZIP file
5) Copy the file folders inside the source ZIP file to your arduino IDE's file folder (default folder is ThisPC->Documents->Arduino)
6) Restart Arduino IDE
7) From Arduino IDE, use File->Open... to open a project
8) Make sure Arduino IDE->Tools->Boards points to Seeed SAM boards->Seeeduino XIAO
9) Now you can compile and upload the firmware 
10) To download WinDaq, please click [www.dataq.com](https://www.dataq.com/products/windaq/). WinDaq is a proprietary commercial software from DATAQ, and it allows free personal home use only under this Arduino project. 
11) Only the [XIAO_WINDAQ base project](https://github.com/dataq-instruments/Arduino_WinDaq/tree/main/xiao_windaq) is maintained regulary. If you have trouble using the other sketches, please compare it with base

**DI-188 Projects to interface with various sensors and other applications**
1) 3-axis digital accelerometer LIS3DHTR (See demo screen shot below)
    - Range: +/-2,4,8,16G
    - The source is included in the base project
    - Visit https://github.com/Seeed-Studio/Seeed_Arduino_LIS3DHTR
    - Push Code button and select Download ZIP
    - From Arduino IED, follow ArduinoIDE->Sketch->Include Library->Add ZIP Library... to add the downloaded library from above step
    - In the sketch, uncomment INCLUDE_3DACC to use it.  
    - LIS3DHTR module should be conneced to a I2C port of DI-188
2) Environmental temperature and humidity sensor (DHT11)
    - Range: 0 to 50C +/-2C, 20 to 90%RH+/-5%RH
    - The source can be found in [xiao_windaq-dht](https://github.com/dataq-instruments/Arduino_WinDaq/tree/main/xiao_windaq_dht)
    - Visit https://github.com/adafruit/DHT-sensor-library
    - Push Code button and select Download ZIP
    - From Arduino IED, follow ArduinoIDE->Sketch->Include Library->Add ZIP Library... to add the downloaded library from above step
    - **DHT11 module should be connected to the DIG port of DI-188**
3) Environmental temperature and Air Pressure Sensor (DSP310)
    - Range: -40 to 85 C +/-0.5C, 300 to 1200 hPa +/-1hPa 
    - The source can be found in [xiao_windaq-dsp310](https://github.com/dataq-instruments/Arduino_WinDaq/tree/main/xiao_windaq_dps310)
    - Visit https://github.com/adafruit/Adafruit_DPS310
    - Push Code button and select Download ZIP
    - From Arduino IED, follow ArduinoIDE->Sketch->Include Library->Add ZIP Library... to add the downloaded library from above step
    - DPS module should be connected to a I2C port of DI-188
4) [DI-188](https://www.dataq.com/products/di-188/), along with this open source project, is now supported by a turn-key software package to facilitate the study of seismological concepts in middle school through introductory undergraduate classrooms
    - Download and run the software from https://www.iris.edu/hq/inclass/software-web-app/jamaseis
5) Higher speed version
    - The source is included in the base project
    - Uncomment #define HISPEED
    - The burst rate will be raised to 20K s/s, but resolution will be lowered to 9 bit due to noise in ADC
6) Higher resolution version
    - The source is included in the base project
    - Uncomment #define HIRES
    - The resolution will be increased to 14 bit, but the max throughput rate will be lowered to 100s/s

**Calibration of DI-188**
DI-188 is shipped with nominal calibration. If you want a better reading, please check out this [calibration utility](https://github.com/dataq-instruments/Arduino_WinDaq/tree/main/calibration)

**Warning: the input voltage for plain Arduino® modules is limited to only 0-3V!** Need industry-grade front end of +/-10V range (30V Max) with 1MΩ input impedance? **Please consider our XIAO-based starter kit [DI-188](https://www.dataq.com/products/di-188/)**

<img src="https://www.dataq.com/resources/images/di-188-arduino-daq2.png" width="300" height="300"> 

 ![alt text](https://www.dataq.com/resources/repository/arduino_3d.gif "Arduino Data logger: ScreenCapture by LICECap")


