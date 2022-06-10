# Arduino_To_WinDaq
This is an open source project that can make any **Arduino®** (Arduino Zero is used here) or **Arduino®-compatible** module (Seeed Studio's Seeeduino XIAO is used here) to stream analog waveforms to [WinDaq](https://www.dataq.com/products/windaq/). It supports up to 16 analog channels (4 analog channels is demonstrated in the source codes) to accept readings from on-board ADC or sensors connected to grove connectors. **It supports WinDaq addons like [XY plot](https://www.dataq.com/products/windaq/add_ons/index.htm), [Excel link](https://www.dataq.com/products/windaq/windaqxl/product.html), [FFT, Spetrgram, Smarter Meters, Gauge, trigger and more](http://www.ultimaserial.com/wdspectrum.html)**

To use the project:
1) Installing Arduino IDE 2.0rc from https://www.arduino.cc/en/software#experimental-software
2) If you use XIAO module, following instruction in https://wiki.seeedstudio.com/Seeeduino-XIAO/#:~:text=Install%20it.&text=After%20installing%20the%20board%2C%20click,the%20Tools%20%7C%20Serial%20Port%20menu. Otherwise, jump to step 5)
3) You may need to restart Arduino IDE to allow XIAO to be added to the search engine
4) Optional (See demo screen shot below): Install 3-Axis Digital Accelerometer library by download the project ZIP file from https://github.com/Seeed-Studio/Seeed_Arduino_LIS3DHTR, ArduinoIDE->Sketch->Include Library->Add ZIP Library... (In the sketch, please uncomment INCLUDE_3DACC to use it. **Warning: the input voltage for Arduino® modules is only 0-3.3V!** )
5) From ArduinoIDE->Tools->Boards->Board Manager, type in XIAO or Arduino Zero and let IDE download all libraries for your module
6) Select ArduinoIDE->Tools->Boards->Seeed SAMD boards->Seeeduino XIAO if you use XIAO, or Arduino Zero from the similar path. 
7) You may also need to specify the COM port for the module
8) Install Flash library by download the project ZIP file from https://github.com/cmaglie/FlashStorage, ArduinoIDE->Sketch->Include Library->Add ZIP Library...

This is an on-going project, final Windaq support has not been released yet

The major difference between Seeeduino XIAO and Arduino ZERO is their Analog input ports

<img src="http://cdn.shopify.com/s/files/1/0506/1689/3647/products/ABX00003_01.iso_d6dab5cd-56ad-4eb2-8381-bc1ea6de29fb_866x686.jpg" width="300" height="300">  <img src="https://www.chip1stop.com/img/product/SEED/seeeduino-xiao-preview_1.jpg" width="300" height="300">  

 ![alt text](https://www.dataq.com/resources/repository/arduino_3d.gif "Arduino Data logger: ScreenCapture by LICECap")


