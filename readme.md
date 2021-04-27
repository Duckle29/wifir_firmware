# WiFIR firmware

This is the firmware for my [WiFIR](https://git.io/WiFIR) hardware. It: 

- Connects to [AdafruitIO](io.adafruit.com) over MQTTS using a root certificate store. No need to worry about expiring fingerprints or insecure SSL.
- Samples the onboard AHT20 temperature and relative humidity sensor
    - Applies a constant temperature offset (regulator gets warm :/) and calculates a corrected
    relative humidity based on this
- Samples the onboard SGP30 TVOC sensor (and co2 estimation)
- Publishes the gathered temperature, relative humidity, TVOC measurements  and estimated CO2 over MQTTS
- Listens for air-condition IR messages, and publishes the state over MQTTS.
- Listens for new states pushed via MQTT and blasts the state over it's IR emitters.

Its intended use is as a remote control of a heat-pump / air-condition, with environmental measurements.
The IR receiver is to keep track of state if someone uses the remote on-location. 


## ToDo

- ~~Set up all feeds using the written wrapper~~ Decided against using the wrapper, but kept the Feed struct
- ~~Add an MQTT based configuration feed.~~ Currently supports "ota" "reboot" and "caltemp:xx.x" to
immediately check for OTA updates, reboot, and calibrate the temperature. Caltemp expeects the actual current temp, and calculates the offset from there. 
- ~~Add callback for subscription type feeds~~ Done
- ~~Add server based OTA~~ Implemented... Kinda(*) Meant to work with [ESP Update Server](http://kstobbe.dk/2019/03/20/web-server-for-esp32-and-esp8266-software-update-over-the-air/) ([git](https://github.com/kstobbe/esp-update-server))
- ~~Fix janky double-reset detector code maybe? It always resets on firmware flash :(~~ Made my own simple "Multiple reset detector" implementation
- ~~Implement MFLN support~~ Implemented. Assumes 512B support for the OTA server (see [docs for nginx](https://nginx.org/en/docs/http/ngx_http_ssl_module.html#ssl_buffer_size) and set it to 512) and sets RX buffers based on MQTTS MFLN results
- ~~Add the ability to remotely read the debug/info messages.~~ Have an MQTT log feed now. 


## \*star\* 
~~The OTA implementation relies on replacing the included ESP ESP8266httpUpdate library with [the latest version](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266httpUpdate).~~
The latest version of this library is now included from /lib/. It takes precedence over the system libs.
This is needed as the ESP update server really shouldn't be run exposed to the internet as it has no protection.  
I've decided on using basic http auth (.htpasswd file), which means the updater has to set some headers.  
Current packaged version doesn't have the function for setting these headers yet.
