# esLPWeather

ESP8266 based board using a TLP5111 timer as wake-up source and a BME280 for _weather_ monitoring.

## Wake up

- Either use the "Wake" button or use the bottom left header to connect a switch
(eg: a door reed switch)
- The board connects to the Wifi networks, sends the POST request and goes back to sleep


## Configuration

- Press and Hold the "Wake" button for 5-10s, the LED should stay lit
- If no Network is configured, the ESP creates a Wifi network, connect to it and configure it
using the Web panel
- If a Network is configured, the web panel is available using through the device IP address
- Change and save your settings
- Reboot the board using the button on the web panel or reset the power to the board.


## Serial Flash

- Serial pinout is (top to bottom):
  1. Tx
  2. Rx
  3. Gnd

- Press and hold the Flash button (right next the LED)
- Press the Wake button (under the ESP12)
- Should be good !
- Flash the code using PlatformIO
- Reboot (unplug power and reconnect)


### Notes

- Don't forget to uplaod both the code and the SPIFFS data
- Using the battery is not recommended for development purposes due to the need to reset the power
to go back in normal/run mode
- External wake (using a reed switch for example) doesn't work really well on external power but
works fine on battery
- DHCP requires at least 15s to complete a normal WakeUp. Static IP only uses 4s

### Acknowledgements

Original idea (and some schematics parts) cmes from the
[trigBoard](https://www.kevindarrah.com/wiki/index.php?title=TrigBoard) project
