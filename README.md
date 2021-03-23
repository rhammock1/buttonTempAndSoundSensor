# buttonTempAndSoundSensor
Arduino sketch which runs a small strip of WS2812B leds with different programs switched between with a button

Currently this sketch has 3 different programs. 
The first reads data from a ds1820b temperature sensor, displays the temperatuer on a 4 unit 7 segment, and fades leds in and out in a color based on the temperature.
The second reaeds data from a sound detection sensor and blinks leds when it detects sound,
The third generates random RGB values and illuminates the leds with the random color.
