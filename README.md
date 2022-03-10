# Arduino_WinDaq
Use Seeed Studion's XIAO module in WinDaq. 

This is an on-going project, no true Windaq support yet

External lib is needed to build the sketch, download the zip file and import to Arduino IDE

https://github.com/cmaglie/FlashStorage

Make sure to use the latest DICOM100.EXE
DICOMMNT.EXE  needs to set RTS and DTE true to make serial class under arduino work 

Here are some helpful info if you want to know more about codes

C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\hardware\samd\1.8.2\libraries\TimerTC3
C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\tools\CMSIS-Atmel\1.2.1\CMSIS-Atmel\CMSIS\Device\ATMEL\samr21\include\component\adc.h
C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\hardware\samd\1.8.2\cores\arduino\WVariant.h
C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\hardware\samd\1.8.2\cores\arduino\wiring_analog.c

C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\tools\CMSIS-Atmel\1.2.1\CMSIS-Atmel\CMSIS\Device\ATMEL\samr21\include\component\adc.h
C:\Users\youraccount\AppData\Local\Arduino15\packages\Seeeduino\hardware\samd\1.8.2\variants\XIAO_m0 contains variant.cpp that has the definition of g_APinDescription

