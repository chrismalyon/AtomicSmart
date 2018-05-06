# AtomicSmart
Multi Purpose SmartHome Switch Wemos

Before flashing the Wemos D1 with this firmware pre-flash it with the EEPROM clear sketch to ensure your dont have anything random in the EEPROM.

Once flashed you can join to the SSID being broadcast as AtomicSmart*** the *** will be replaced with the last 3 digits of the ESP ChipID
You can then browse to http://192.168.4.1 and enter your SSID, WEP key and either MQTT broker details or enable the Alexa support.

This firmware is designed to work with the Wemos Relay shield. You can also add a button to pin D2 for a local trigger of the relay. 

This firmware is still in development and is currently designed to integrate with OpenHab. The MQTT topic is currenly hardcoded however can easily be updated in code as you need. Future versions will allow you to define the topic via the web interface.
