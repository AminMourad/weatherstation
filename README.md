# weatherstation

A basic weather station capable of pulling weather data from the internet, measuring CO2, temperature, and humidity data, and displaying them on a 2.42" OLED display. It uses an ESP32-S2 mini dev board as a microcontroller with connectivity, a Sensirion SCD40 sensor for CO2 measurements, and a DHT22 temperature and humidity sensor. While the SCD40 has integrated temperature and humidity sensors, it is inaccurate due to the sensor heating up during measurements. It also makes for a better learning experience to have different ways of interfacing with sensors.

The project was created to teach students without previous knowledge in CS about the basics of microcontroller programming. To help with getting started, an Arduino-specific [blockly](https://developers.google.com/blockly?hl=de) environment with custom blocks was used. It is based on [plamprecht/Ardublockly-ESP](https://github.com/plamprecht/Ardublockly-ESP), which is based on [carlosperate/ardublockly](https://github.com/carlosperate/ardublockly). Currently, it is too much of a mess to open-source, but the created blocks can be found in this repo, and the environment is online on [blockly.mourad.pw](https://blockly.mourad.pw/ardublockly/index.html). Some blocks might not work entirely as expected.

Weather state icons were taken from [erikflowers/weather-icons](https://github.com/erikflowers/weather-icons) and converted to 16x16 bitmaps.

A custom PCB was made to connect the used ESP32 dev board with the sensors and display in a nice modular package. The files for the PCB and the laser-cut housing can be found here.

![Housing of the programmed weatherstation in front of a workshop participants Tablet.](https://github.com/AminMourad/weatherstation/blob/main/media/weatherstation_housing.png?raw=true)

![PCB of the weatherstation, some parts of the housing in the background](https://github.com/AminMourad/weatherstation/blob/main/media/weatherstation_PCB.png?raw=true)

