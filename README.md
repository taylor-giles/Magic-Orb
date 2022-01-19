# Magic Crystal Orb
Collaborators/Wizards - 
[Taylor Giles](https://taylorgiles.me), [Anthony Piccinone](https://anthony.piccinone.com), [Joseph Romano](https://josephromanodesign.com/)

## Summary
A set of three ESP8266-powered crystal orbs that light up together whenever a fellow "wizard" activates one. Each orb in the set is assigned a color - either red, green, or blue - and when an orb is activated, its color is added to the mixture, resulting in a mystical white when all three wizards are present at the same time.

## Usage
### Connect to the Internet
The ESP8266 boards will remember the last WiFi network they were connected to, so they can connect automatically. However, if connection fails (or the network is no longer present), then the device will enter a configuration mode.
The light will flash, indicating that the local AP and configuration page have been activated. Connect to the generated WiFi network using another device, and navigate
to ``192.168.4.1``. Enter the credentials for the desired WiFi network, and the ESP8266 will reboot to connect to the chosen network.

### Activate the Orb
To activate a connected orb, simply place your hand on it, and the magic crystal will add your color to all the other connected orbs. When any of the connected orbs
are activated, your orb will change color to match.

## Build Instructions
### Materials
* ESP8266 NodeMCU
* RGB LED
* PIR Motion Sensor
* Frosted glass/acrylic orb
* Base and support (3D-printed: see ``Models`` folder)

### Instructions
1. Connect the hardware according to the given schematic
1. Secure the PIR Sensor to the top of the support and secure the support to the base
(Note: The sensor should just touch the inside of the orb when it is placed into the base)
1. For each orb, select a color. Connect NodeMCU pin ``D5`` to ``GND`` to select green; ``D6`` to ``GND`` for blue; leave both ungrounded for red
1. Make a [Firebase](https://firebase.google.com/) project with a Realtime Database
1. Install the [Arduino IDE](https://www.arduino.cc/en/software)
1. [Install the ESP8266 addon for Arduino IDE](https://randomnerdtutorials.com/how-to-install-esp8266-board-arduino-ide/)
1. Install the following libraries: [WiFiManager](https://github.com/tzapu/WiFiManager) & [Firebase ESP Client](https://github.com/mobizt/Firebase-ESP-Client)
1. Open ``Magic-Orb.ino`` and fill in the ``WM_SSID``, ``WM_PASSWORD``, ``API_KEY``, and ``DATABASE_URL`` (note that the last two are from Firebase)
1. Upload the sketch to the ESP8266 board
1. Refer to "Usage" and enjoy :)

