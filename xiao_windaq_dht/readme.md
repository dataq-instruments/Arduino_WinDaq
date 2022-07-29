Interface with DHT11, an environmental temperature and humitity sensor

Environmental temperature and humidity sensor (DHT11), which can be [purchased online](https://www.seeedstudio.com/Grove-Temperature-Humidity-Sensor-DHT11.html):

    - The source can be found in [xiao_windaq-dht](https://github.com/dataq-instruments/Arduino_WinDaq/tree/main/xiao_windaq_dht)
    - Visit https://github.com/adafruit/DHT-sensor-library
    - Push Code button and select Download ZIP
    - From Arduino IED, follow ArduinoIDE->Sketch->Include Library->Add ZIP Library... to add the downloaded library from above step
    - DHT11 module should be connected to the DIG port of DI-188

Note: due to the strict timing issue from DHC sensor, the max sample rate needs to be lowered to 10 Hz
