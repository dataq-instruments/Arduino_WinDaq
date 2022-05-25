# Arduino_To_WinDaq
This is an open source project that can make any **Arduino®** (Arduino Zero is used here) or **Arduino®-compatible** module (Seeed Studio's Seeeduino XIAO is used here) to stream analog waveforms to WinDaq. It supports up to 16 analog channels (4 analog channels is demonstrated in the source codes) to accept readings from on-board ADC or sensors connected to grove connectors. **It supports WinDaq addons like XY plot, Excel link, FFT, gauge, trigger and more**

This is an on-going project, final Windaq support has not been released yet

Make sure to use the latest DICOM100.EXE

DICOMMNT.EXE needs to set RTS and DTE true to make serial class under Arduino® work 

The major difference between Seeeduino XIAO and Arduino ZERO is their Analog input pins
