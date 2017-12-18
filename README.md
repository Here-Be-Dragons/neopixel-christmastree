# Neopixel-driven Christmas Tree

### Overview
---------------------
Networked Christmas Tree lights using a Particle Photon, and WB2812B LEDs (aka NeoPixels).  Also syncs to my Familamp project to color the tree for a few minutes the same color as the lamps any time an update is sent. (https://github.com/Here-Be-Dragons/familamp)

### Modes
---------------------
- Looping Rainbow
- Individual Random Colors
- Candy Cane Stripes
- Red/Green fade
- Blue/Purple Shimmer
- Familamp Color Override (with sparkle)

### Hardware
---------------------
-   Particle Photon
-   WB2812B string lights.  I used 400 (8 strands).
-   A power supply (I used a 10A 5V bench supply).
-   A method to inject additional power along the strand.  I injected 5 times.  The thin gauge of the wire of the string lights drops voltage very quickly, so it has to be injected frequently.
-   Other supporting bits generic for NeoPixels:
	-   1 resistor of value between 300 and 500 Ohms
  -   1 500mF Capacitor to pad voltage during color changes

### Software
---------------------
-   Particle's Cloud
-   NeoPixel Library: https://github.com/adafruit/Adafruit_NeoPixel (Already ported to Particle's Libraries)
