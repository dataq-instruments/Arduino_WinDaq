# Arduino_WinDaq
This is an open source project to use any Arduino module (Seeed Studio's Seeeduino XIAO is used as basic example) to stream analog waveforms to WinDaq. It supports up to 11 channels, with 4 analog input channels and 4 reserved channels to accept readings from sensors connected to grove connectors

This is an on-going project, final Windaq support has not been released yet

External library is needed to build the sketch, download the zip file and import to Arduino IDE

https://github.com/cmaglie/FlashStorage

Make sure to use the latest DICOM100.EXE

DICOMMNT.EXE needs to set RTS and DTE true to make serial class under arduino work 

Here are some helpful info if you want to know more about codes

C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\hardware\samd\1.8.2\libraries\TimerTC3
C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\tools\CMSIS-Atmel\1.2.1\CMSIS-Atmel\CMSIS\Device\ATMEL\samr21\include\component\adc.h
C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\hardware\samd\1.8.2\cores\arduino\WVariant.h
C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\hardware\samd\1.8.2\cores\arduino\wiring_analog.c

C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\tools\CMSIS-Atmel\1.2.1\CMSIS-Atmel\CMSIS\Device\ATMEL\samr21\include\component\adc.h
C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\hardware\samd\1.8.2\variants\XIAO_m0 contains variant.cpp that has the definition of g_APinDescription

