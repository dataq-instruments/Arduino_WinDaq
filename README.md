# Arduino_To_WinDaq
This is an open source project that can make any **Arduino®** (Arduino Zero is used here) or **Arduino®-compatible** module (Seeed Studio's Seeeduino XIAO is used here) to stream analog waveforms to [WinDaq](https://www.dataq.com/products/windaq/). It supports up to 16 analog channels (4 analog channels is demonstrated in the source codes) to accept readings from on-board ADC or sensors connected to grove connectors. **It supports WinDaq addons like [XY plot](https://www.dataq.com/products/windaq/add_ons/index.htm), [Excel link](https://www.dataq.com/products/windaq/windaqxl/product.html), [FFT, Spetrgram, Smarter Meters, Gauge, trigger and more](http://www.ultimaserial.com/wdspectrum.html)**

This is an on-going project, final Windaq support has not been released yet

The major difference between Seeeduino XIAO and Arduino ZERO is their Analog input pins

<img src="http://cdn.shopify.com/s/files/1/0506/1689/3647/products/ABX00003_01.iso_d6dab5cd-56ad-4eb2-8381-bc1ea6de29fb_866x686.jpg" width="300" height="300">  <img src="https://www.chip1stop.com/img/product/SEED/seeeduino-xiao-preview_1.jpg" width="300" height="300">

Warning: the input voltage for **Arduino®** modules is only 0-3.3V! 
