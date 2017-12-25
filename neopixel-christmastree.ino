// This #include statement was automatically added by the Particle IDE.
#include <neopixel.h>

#include "application.h"

SYSTEM_MODE(AUTOMATIC);

#define PIXEL_COUNT 400
#define PIXEL_PIN D0

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN);

////
// User Variables
////

uint32_t decayTime = 2835;                  // Start extinguishing light after elapsed seconds
uint32_t decayDelay = 15;                    // Seconds between decay fade-out steps
uint8_t nightHours[2] = {6,      21};       // Night mode starts at nightHours[1], ends at nightHours[0]
uint8_t duskHours[2] =  {  7,  19  };       // Dusk mode starts at duskHours[1], ends at duskHours[0].  Needs to be inside nightHours' times.
                                            // Day mode starts at duskHours[0], ends at duskHours[1]
uint16_t maxDayBrightness = 120;            // 0 - 255, lamp will not exceed this during the day
uint16_t maxDuskBrightness = 100;            // 0 - 255, lamp will not exceed this during dusk
uint16_t maxNightBrightness = 80;            // 0 - 255, lamp will not exceed this during the night
uint32_t secondsPerMode = 120;               // Activates rainbowEasterEggroll after this many consecutive color changes

////
// End User Variables
////

double lastColorUpdate = 0;                 // Epoch of last color update (local or remote)
String colorFromID;                         // String, Tracks who sent the color (for debug)
uint16_t colorRecieved;                     // 0 - 255, Tracks the color received from another lamp
uint8_t activeColor = 0;                    // 0 - 255, Tracks what color is currently active (start at red)
uint8_t activeR = 255;                      // 0 - 255, Red component of activeColor;
uint8_t activeG = 0;                        // 0 - 255, Green component of activeColor;
uint8_t activeB = 0;                        // 0 - 255, Blue component of activeColor;
double lastDecayDelay = 0;                  // Time Tracker for decayDelay
uint16_t lampBrightness = maxDayBrightness; // 0 - 255, Tracks current lamp brightness
//uint16_t maxBrightness = maxDayBrightness;  // Assigned the current max brightness
uint8_t dayTrack = 0;                       // Track day/dusk/night condition
uint16_t activePixels = 0;                  // Tracks number of active pixels, 0 is first pixel

// Variables for special effects
uint32_t consecutiveChanges = 0;            // Track how many times the color has been changed before turning off
uint16_t fadeColor = 0;                     // Track color for special events
uint16_t fadePixelTracker = 0;              // Track last touched pixel for some idle sequences
uint16_t easterEggrollColor = 0;            // Track color for rainbowEasterEggroll()
uint16_t mode = 0;
double modeTracking = 0;
bool familampOverride = 0;                   // Track if the familamp override is enabled

void setup() {
    strip.begin();
    strip.show();
    Time.zone(-5);
    Time.setDSTOffset(1);
    Time.beginDST();
    //Listen for other lamps to send a particle.publish()
    Particle.subscribe("FamiLamp_Update", gotColorUpdate, MY_DEVICES);
    modeTracking = Time.now();
    rainbowFull(5, 0); // 5ms Delay, 0 is fade in
    rainbowFull(5, 2); // 5ms Delay, 2 is fade out
}

void loop() {
    if (familampOverride == 1 ) {
        idleColorFlicker(colorRecieved);
    } else {
        christmasTreeLoop();
    }
    
	if (familampOverride == 1 && (Time.now() - lastColorUpdate) > 300 ) {
	    familampOverride = 0;
	}
	
}

void christmasTreeLoop() {
    if (Time.now() - modeTracking >= secondsPerMode) {
            mode++;
            mode%=9;
            modeTracking = Time.now();
    }
    switch (mode) {
    	case 0:
            // Easter Egg 1
            rainbowEasterEggroll(3);
            break;
        case 1:
            shimmer(0);
            break;
        case 2:
            // Red and green
            alternatingColors(lampBrightness,0,0,0,lampBrightness,0);
            break;
        case 3:
            // Birthdays
            idleDisco();
            break;
        case 4:
            // Red and green cycle
            idleColorFader(0,85);
            break;
        case 5:
            // Shimmer Blue/purple
            shimmer(1);
            break;
        case 6:
            // Red and white (candy Cane)
            alternatingColors(lampBrightness*0.8,lampBrightness*0.8,lampBrightness*0.8,0,lampBrightness,0);
            break;
        case 7:
            rainbowEasterEggroll(2);
            break;
        case 8:
            shimmer(2);
            break;
        default:
            mode = 0;
            break;
    }
}

void gotColorUpdate(const char *name, const char *data) {
    String str = String(data);
    char strBuffer[40] = "";
    str.toCharArray(strBuffer, 40);
    colorFromID = strtok(strBuffer, "~");
    colorRecieved = atof(strtok(NULL, "~"));
    consecutiveChanges++;
    activePixels = strip.numPixels();
    familampOverride = 1;
    setColorFlash(colorRecieved);
	lastColorUpdate = Time.now();
}

void setColorFlash(byte c) {
    uint16_t endR, endG, endB, partR, partG, partB;
    uint32_t color = wheelColor(c, lampBrightness);
    endR = (uint16_t)((color >> 16) & 0xff); // Splits out new color into separate R, G, B
    endG = (uint16_t)((color >> 8) & 0xff);
    endB = (uint16_t)(color & 0xff);
    partR = (lampBrightness - endR) / 12;
    partG = (lampBrightness - endG) / 12;
    partB = (lampBrightness - endB) / 12;
    
    for(int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor((strip.numPixels() - 1) - i - 14, lampBrightness, lampBrightness, lampBrightness);
        strip.setPixelColor((strip.numPixels() - 1) - i - 13, lampBrightness, lampBrightness, lampBrightness);
        strip.setPixelColor((strip.numPixels() - 1) - i - 12, lampBrightness, lampBrightness, lampBrightness);
        strip.setPixelColor((strip.numPixels() - 1) - i - 11, lampBrightness - ( partG * 1 ), lampBrightness - ( partR * 1 ), lampBrightness - ( partB * 1 ));
        strip.setPixelColor((strip.numPixels() - 1) - i - 10, lampBrightness - ( partG * 2 ), lampBrightness - ( partR * 2 ), lampBrightness - ( partB * 2 ));
        strip.setPixelColor((strip.numPixels() - 1) - i - 9, lampBrightness - ( partG * 3 ), lampBrightness - ( partR * 3 ), lampBrightness - ( partB * 3 ));
        strip.setPixelColor((strip.numPixels() - 1) - i - 8, lampBrightness - ( partG * 4 ), lampBrightness - ( partR * 4 ), lampBrightness - ( partB * 4 ));
        strip.setPixelColor((strip.numPixels() - 1) - i - 7, lampBrightness - ( partG * 5 ), lampBrightness - ( partR * 5 ), lampBrightness - ( partB * 5 ));
        strip.setPixelColor((strip.numPixels() - 1) - i - 6, lampBrightness - ( partG * 6 ), lampBrightness - ( partR * 6 ), lampBrightness - ( partB * 6 ));
        strip.setPixelColor((strip.numPixels() - 1) - i - 5, lampBrightness - ( partG * 7 ), lampBrightness - ( partR * 7 ), lampBrightness - ( partB * 7 ));
        strip.setPixelColor((strip.numPixels() - 1) - i - 4, lampBrightness - ( partG * 8 ), lampBrightness - ( partR * 8 ), lampBrightness - ( partB * 8 ));
        strip.setPixelColor((strip.numPixels() - 1) - i - 3, lampBrightness - ( partG * 9 ), lampBrightness - ( partR * 9 ), lampBrightness - ( partB * 9 ));
        strip.setPixelColor((strip.numPixels() - 1) - i - 2, lampBrightness - ( partG * 10 ), lampBrightness - ( partR * 10 ), lampBrightness - ( partB * 10 ));
        strip.setPixelColor((strip.numPixels() - 1) - i - 1, lampBrightness - ( partG * 11 ), lampBrightness - ( partR * 11 ), lampBrightness - ( partB * 11 ));
        strip.setPixelColor((strip.numPixels() - 1) - i, endG, endR, endB);
        strip.show();
        delay(5);
    }
    
}

uint32_t wheelColor(uint16_t WheelPos, uint16_t iBrightness) {
	float R, G, B;
	float brightness = iBrightness / 255.0;

	if (WheelPos < 85) {
		R = WheelPos * 3;
		G = 255 - WheelPos * 3;
		B = 0;
	} else if (WheelPos < 170) {
		WheelPos -= 85;
		R = 255 - WheelPos * 3;
		G = 0;
		B = WheelPos * 3;
	} else {
		WheelPos -= 170;
		R = 0;
		G = WheelPos * 3;
		B = 255 - WheelPos * 3;
	}
	activeR = R * brightness;// + .5;
	activeG = G * brightness;// + .5;
	activeB = B * brightness;// + .5;
	return strip.Color((byte) activeR,(byte) activeG,(byte) activeB);
}

void rainbowFull(byte wait, byte fade) {
  uint16_t i, j, k;
  uint16_t thisColor;
  if(fade == 0) k = 0;
  else k = lampBrightness;

  for(j = 0; j <= 255; j++) {
    for(i = 0; i < strip.numPixels(); i++) {
        thisColor = wheelColor(((i * 60 / strip.numPixels()) + j) & 255, k);
        strip.setPixelColor((strip.numPixels() - 1) - i, thisColor);
    }
    strip.show();
    delay(wait);
    if(fade == 0 && k < lampBrightness) {
        k++;
    }
    if(fade == 2 && k > 0) {
        k--;
    }
  }
}

void rainbowEasterEggroll(byte type) {
    // displays full rainbow and rolls the color each time called
    uint16_t magicNumber;
    if (type == 1) {
        magicNumber = 24;
    } else if (type == 2) {
        magicNumber = 100;
    } else if (type == 3) {
        magicNumber = 200;
    } else {
        magicNumber = strip.numPixels();
    }
    for(uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, wheelColor(((i * 256 / magicNumber) + easterEggrollColor) & 255, lampBrightness));
    }
    strip.show();
    delay(40);
    easterEggrollColor++;
}

void idleColorFader(uint8_t c1, uint8_t c2) {
    // Slow fade between the two specified colors, `c1` and `c2`
    uint16_t currR, currG, currB, endR, endG, endB;
    uint32_t color = wheelColor(fadeColor, lampBrightness);
    endR = (uint16_t)((color >> 16) & 0xff); // Splits out new color into separate R, G, B
    endG = (uint16_t)((color >> 8) & 0xff);
    endB = (uint16_t)(color & 0xff);
    for (uint16_t j = 0; j < fadePixelTracker; j++) {
        long startRGB = strip.getPixelColor(j); // Get pixel's current color
        currR = (uint16_t)((startRGB >> 16) & 0xff); // Splits out current color into separate R, G, B
        currG = (uint16_t)((startRGB >> 8) & 0xff);
        currB = (uint16_t)(startRGB & 0xff);
        if ( currR > endR ) {
            currR = currR - 1;
        } else if ( currR < endR ) {
            currR = currR + 1;
        } else {
            currR = endR;
        }
        if ( currG > endG ) {
            currG = currG - 1;
        } else if ( currG < endG ) {
            currG = currG + 1;
        } else {
            currG = endG;
        }
        if ( currB > endB ) {
            currB = currB - 1;
        } else if ( currB < endB ) {
            currB = currB + 1;
        } else {
            currB = endB;
        }
        
        //Catch overflows
        currR %= 255;
        currG %= 255;
        currB %= 255;

        strip.setPixelColor(j, currR, currG, currB);
        if ( j >= strip.numPixels() - 1 && endR == currR && endG == currG && endB == currB) {
            if ( fadeColor == c1 ) {
                fadeColor = c2;
            } else {
                fadeColor = c1;
            }
            fadePixelTracker = 0;
        }
    }
    strip.show();
    if ( fadePixelTracker < strip.numPixels() ) fadePixelTracker++;
    delay(10);
}

void idleDisco() {
    // Recreation of 70s Disco floor.  Each cycle 60 random LEDs are updated
    // to 60 random colors and brightnesses.
    for(int i=0; i<60; i++) {
        int randr = random(0,lampBrightness);
        int randg = random(0,lampBrightness); 
        int randb = random(0,lampBrightness);
        int randi = random(0,strip.numPixels());
        strip.setPixelColor(randi, randr, randg, randb);
        strip.show();
        delay(5);
    }
}
void idleColorFlicker(uint8_t c) {
    // Similar to idleDisco, but only uses a single color and randomly
    // varies brightness between `lampBrightness` and `lampBrightness - 10`
    uint32_t color = wheelColor(c, lampBrightness);
    for(uint16_t i=0; i<strip.numPixels(); i++) {    
        uint16_t j = random(0,strip.numPixels()-1);
        int8_t flicker = random(-8,8);
        int flickerR = (uint16_t)((color >> 16) & 0xff) + flicker; // Splits out new color into separate R, G, B
        int flickerG = (uint16_t)((color >> 8) & 0xff) + flicker;
        int flickerB = (uint16_t)(color & 0xff) + flicker;
        if(flickerR<0) flickerR=0;
        if(flickerG<0) flickerG=0;
        if(flickerB<0) flickerB=0;
        if(flickerR>255) flickerR=255;
        if(flickerG>255) flickerB=255;
        if(flickerB>255) flickerG=255;
        strip.setPixelColor(j, flickerG, flickerR, flickerB);
        
    }
    strip.show();
    delay(20);
}
int randomWalk(int val, int maxVal, unsigned char changeAmount, unsigned char directions)
{
  unsigned char walk = random(directions);  // direction of random walk
  if (walk == 0)
  {
    // decrease val by changeAmount down to a min of 0
    if (val >= changeAmount)
    {
      val -= changeAmount;
    }
    else
    {
      val = 0;
    }
  }
  else if (walk == 1)
  {
    // increase val by changeAmount up to a max of maxVal
    if (val <= maxVal - changeAmount)
    {
      val += changeAmount;
    }
    else
    {
      val = maxVal;
    }
  }
  return val;
}

void shimmer(int shimmerMode) {
  const unsigned char changeAmount = 5;   // size of random walk step

  for (int i = 0; i < strip.numPixels(); i += 2) {
    int currR = (uint16_t)((strip.getPixelColor(i) >> 16) & 0xff); // Splits out current color into separate R, G, B
    int currG = (uint16_t)((strip.getPixelColor(i) >> 8) & 0xff);
    int currB = (uint16_t)(strip.getPixelColor(i) & 0xff);
    
    // randomly walk the brightness of every even LED
    currB = randomWalk(currB, lampBrightness, changeAmount, 2);
    
    // warm white: red = x, green = 0.8x, blue = 0.125x
    currG = currB*4/5;  // green = 80% of red
    currR = currB >> 3;  // blue = red/8
    // Bright Leds
    switch (shimmerMode) {
    	case 0:
            // Purple
            strip.setPixelColor(i, strip.Color(currR, currG, currB));
            break;
        case 1:
            // Green
            strip.setPixelColor(i, strip.Color(currB, currR, currG));
            break;
        default:
            // Blue/purple
            strip.setPixelColor(i, strip.Color(currB, currB, currB));
            break;
            
        /*
            // Light Blue
            strip.setPixelColor(i, strip.Color(currG, currR, currB));
            // Dim Yellow
            //strip.setPixelColor(i, strip.Color(currG, currB, currR));
            // Green
            //strip.setPixelColor(i, strip.Color(currB, currG, currR));
            // Aquaish green
            //strip.setPixelColor(i, strip.Color(currB, currR, currG));
        */
    }
    // every odd LED gets set to a quarter the brighness of the preceding even LED
    if (i + 1 < strip.numPixels())
    {
        switch (shimmerMode) {
    	case 0:
            // light blue
            strip.setPixelColor(i + 1, strip.Color(currG >> 2, currR >> 2, currB >> 2));
            break;
        case 1:
            // Aqua
            strip.setPixelColor(i + 1, strip.Color(currB >> 2, currR >> 2, currG >> 2));
            break;
        default:
            // Purpe
            strip.setPixelColor(i + 1, strip.Color(currR >> 2, currG >> 2, currB >> 2));
            break;
        }
    }
  }
  strip.show();
  delay(80);
}

void alternatingColors(int oneG, int oneR, int oneB, int twoG, int twoR, int twoB) {
    // Fades to two colors alternating
    int currR, currG, currB;
    for (uint16_t i = 0; i < strip.numPixels(); i+=2) {
        long startRGB = strip.getPixelColor(i); // Get pixel's current color
        currG = (uint16_t)((startRGB >> 16) & 0xff); // Splits out current color into separate R, G, B
        currR = (uint16_t)((startRGB >> 8) & 0xff);
        currB = (uint16_t)(startRGB & 0xff);
        if ( currR > oneR ) {
            currR--;
        } else if ( currR < oneR ) {
            currR++;
        } else {
            currR = oneR;
        }
        if ( currG > oneG ) {
            currG--;
        } else if ( currG < oneG ) {
            currG++;
        } else {
            currG = oneG;
        }
        if ( currB > oneB ) {
            currB--;
        } else if ( currB < oneB ) {
            currB++;
        } else {
            currB = oneB;
        }
        
        //Catch overflows
        currR %= 255;
        currG %= 255;
        currB %= 255;
        
        strip.setPixelColor(i, currG, currR, currB);
    }
    for (uint16_t i = 1; i < strip.numPixels(); i+=2) {
        long startRGB = strip.getPixelColor(i); // Get pixel's current color
        currG = (uint16_t)((startRGB >> 16) & 0xff); // Splits out current color into separate R, G, B
        currR = (uint16_t)((startRGB >> 8) & 0xff);
        currB = (uint16_t)(startRGB & 0xff);
        if ( currR > twoR ) {
            currR--;
        } else if ( currR < twoR ) {
            currR++;
        } else {
            currR = twoR;
        }
        if ( currG > twoG ) {
            currG--;
        } else if ( currG < twoG ) {
            currG++;
        } else {
            currG = twoG;
        }
        if ( currB > twoB ) {
            currB--;
        } else if ( currB < twoB ) {
            currB++;
        } else {
            currB = twoB;
        }
        
        //Catch overflows
        currR %= 255;
        currG %= 255;
        currB %= 255;
        
        strip.setPixelColor(i, currG, currR, currB);
    }
    strip.show();
    delay(100);
}
