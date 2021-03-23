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

- Set up all feeds using the written wrapper
- Add an MQTT based configuration feed.
- Add callback for subscription type feeds
- Add server based OTA
- Fix janky double-reset detector code maybe? It always resets on firmware flash :(