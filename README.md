# Arduino_To_WinDaq Chart Recorder
Originated from our DI-188, this open source project turns any **Arduino®** (Arduino Zero is used here) or **Arduino®-compatible** module (Seeeduino XIAO is used here) to a [WinDaq](https://www.dataq.com/products/windaq/)-based data acquisition chart recorder. It supports up to 16 analog channels (4 analog channels is demonstrated in the source codes) to accept readings from on-board ADC or sensors connected to grove connectors. **It supports WinDaq addons like [FFT, Spectrogram, Smart Meters, Gauge display, various trigger modes, calculated channel](http://www.ultimaserial.com/wdspectrum.html), [XY plot](https://www.dataq.com/products/windaq/add_ons/index.htm), and [Excel link](https://www.dataq.com/products/windaq/windaqxl/product.html),**

To use the project:
1) Installing Arduino IDE 2.0. Please follow instruction from https://www.arduino.cc/en/software#experimental-software 
2) If you don't use XIAO module, jump to step 3)
    -  following instruction in https://wiki.seeedstudio.com/Seeeduino-XIAO/#:~:text=Install%20it.&text=After%20installing%20the%20board%2C%20click,the%20Tools%20%7C%20Serial%20Port%20menu. 
    - Restart Arduino IDE to allow XIAO to be added to the search engine
    - From ArduinoIDE->Tools->Boards->Board Manager, type in XIAO or Arduino Zero and let IDE download all libraries for your module
       - When you upgrade Arduino IDE, you may need to upgrade the libraries as well
    - Select ArduinoIDE->Tools->Boards->Seeed SAMD boards->Seeeduino XIAO  
    - You may also need to specify the COM port for the module
3) Install Flash library
    - Visit https://github.com/cmaglie/FlashStorage
    - Push Code button and select Download ZIP
    - From Arduino IDE, follow ArduinoIDE->Sketch->Include Library->Add ZIP Library... to add the downloaded library from above step
4) Restart Arduino IDE 
5) Now you can compile and upload the firmware
6) To download WinDaq, please click [www.dataq.com](https://www.dataq.com/products/windaq/). WinDaq is a proprietary commercial software from DATAQ, and it allows free personal home use only under this Arduino project

Projects to interface with various sensors
1) 3-axis digital accelerometer LIS3DHTR (See demo screen shot below)
    - The source is included in the base project
    - Visit https://github.com/Seeed-Studio/Seeed_Arduino_LIS3DHTR
    - Push Code button and select Download ZIP
    - From Arduino IED, follow ArduinoIDE->Sketch->Include Library->Add ZIP Library... to add the downloaded library from above step
    - In the sketch, uncomment INCLUDE_3DACC to use it.   
2) Environmental temperature and humidity sensor (DHT11)
    - The source can be found in [xiao_windaq-dht](https://github.com/dataq-instruments/Arduino_WinDaq/tree/main/xiao_windaq_dht)
    - Visit https://github.com/adafruit/DHT-sensor-library
    - Push Code button and select Download ZIP
    - From Arduino IED, follow ArduinoIDE->Sketch->Include Library->Add ZIP Library... to add the downloaded library from above step

**Warning: the input voltage for plain Arduino® modules is limited to only 0-3V!**

**Need industry-grade front end of +/-10V range (30V Max) with 1MΩ input impedance? Please consider our XIAO-based starter kit [DI-188](https://www.dataq.com/products/di-188/)**

<img src="https://www.dataq.com/resources/images/di-188-arduino-daq2.png" width="300" height="300">  <img src="http://cdn.shopify.com/s/files/1/0506/1689/3647/products/ABX00003_01.iso_d6dab5cd-56ad-4eb2-8381-bc1ea6de29fb_866x686.jpg" width="300" height="300"> 

 ![alt text](https://www.dataq.com/resources/repository/arduino_3d.gif "Arduino Data logger: ScreenCapture by LICECap")


