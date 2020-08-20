# lorahambeacon
A LoRa Beacon and messaging tool for Amateur Radio with the bsfrance LoRa32u4II 915 MHz LoRa Arduino board

2020 Matthew Asham <ve7udp@yahoo.com> 

## hardware
 * Lora32u4ii Lora Development Board, JST-PH2.0MM-2P Connector, IPEX Antenna, ATmega32u4 SX1276 HPD13A 915MHZ, for Arduino UNO Mega 2560 Nano - https://www.amazon.ca/gp/product/B07HD17Z3H - ~$27 CAD
 
## status

The software has been validated against the Dragino LG02 LoRa gateway.  

I've run this for a while locally and haven't encountered any new obvious bugs.  The code could use some optimization, and the command parser needs to be refactored to handle commands from multiple inputs, and should use lookup tables.  At the moment ~2K of RAM is left free.

## building the sketch

...

## operation

Upon execution of the compiled sketch, lorahambeacon will read the configuration structure from eeprom.  If a valid configuration is not found, the unit will default to some basic operating parameters.

Beacons, telemetry (soon) and text messages are transmitted with your configured callsign, eg:  VE7UDP: hello world

### configuration

  /callsign VE7???
  /btext Some beacon text you want to transmit every so often
  /binval Interval of beacon (in minutes)
  /save Save configuration
  /reboot Reboot

### texting

Simply type text.  Any line of input that does not start with a '/' will be treated as a message and transmitted verbatim to the current frequency.  

### advanced

  /dfl reset configuration to "default"
  /reg dump registers
  /scan loop through the channel list and display any received packets

 
