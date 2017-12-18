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
uint32_t easterEggRollActivation = 30;      // Activates rainbowEasterEggroll after this many consecutive color changes

////
// End User Variables
////

double lastColorUpdate = 0;                 // Epoch of last color update (local or remote)
String colorFromID;                         // String, Tracks who sent the color (for debug)
uint16_t colorRecieved;                     // 0 - 255, Tracks the color received from another lamp
bool lampOn = 0;                            // Tracks if the lamp is lit
uint8_t activeColor = 0;                    // 0 - 255, Tracks what color is currently active (start at red)
uint8_t activeR = 255;                      // 0 - 255, Red component of activeColor;
uint8_t activeG = 0;                        // 0 - 255, Green component of activeColor;
uint8_t activeB = 0;                        // 0 - 255, Blue component of activeColor;
double lastDecayDelay = 0;                  // Time Tracker for decayDelay
uint16_t lampBrightness = 0;                // 0 - 255, Tracks current lamp brightness
uint16_t maxBrightness = maxDayBrightness;  // Assigned the current max brightness
uint8_t dayTrack = 0;                       // Track day/dusk/night condition
uint16_t activePixels = 0;                      // Tracks number of active pixels, 0 is first pixel
uint8_t lastDay = 0;                        // Used to track if onceADay() has run yet today
uint8_t lastHour = 0;                       // Used to track if onceAnHour() has run yet this hour

// Variables for special effects
uint32_t consecutiveChanges = 0;            // Track how many times the color has been changed before turning off
uint16_t fadeColor = 0;                     // Track color for special events
uint16_t fadePixelTracker = 0;              // Track last touched pixel for some idle sequences
float redStates[PIXEL_COUNT];               // Fireworks Variable
float blueStates[PIXEL_COUNT];              // Fireworks Variable
float greenStates[PIXEL_COUNT];             // Fireworks Variable
float fadeRate = 0.99;                      // Fireworks Variable: 0.01-0.99, controls decay speed
uint16_t heartbeatDirector = 0;             // Heartbeat Tracking
uint16_t heartbeatColor = 0;                // Heartbeat Tracking
uint16_t easterEggrollColor = 0;            // Track color for rainbowEasterEggroll()
uint16_t easterMonth;                       // Stores this year's easter month.
uint16_t easterDay;                         // Stores this year's easter day.
uint16_t mode = 0;
double modeTracking = 0;
bool familampOverride = 0;                   // Track if the familamp override is enabled

/** An RGB color. */
typedef struct Color {
  unsigned char red, green, blue;

  Color(int r, int g, int b) : red(r), green(g), blue(b) {}
  Color() : red(0), green(0), blue(0) {}
} Color;

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
    if (Time.now() - modeTracking >= 120) {
            mode++;
            mode%=6;
            modeTracking = Time.now();
    }
    switch (mode) {
    	case 0:
            // Easter Egg 1
            rainbowEasterEggroll(3);
            break;
        case 1:
            // Birthdays
            idleDisco();
            break;
        case 2:
            // Red and green cycle
            idleColorFader(0,85);
            break;
        case 3:
            // Shimmer
            shimmer();
            break;
        case 4:
            rainbowEasterEggroll(2);
            break;
        case 5:
            candyCane();
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
    lampBrightness = maxBrightness;
    lampOn = 1;
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

void setColorFade(byte c) {
    familampOverride = 1;
    uint16_t currR, currG, currB, endR, endG, endB;
    uint32_t color = wheelColor(c, lampBrightness);
    endR = (uint16_t)((color >> 16) & 0xff); // Splits out new color into separate R, G, B
    endG = (uint16_t)((color >> 8) & 0xff);
    endB = (uint16_t)(color & 0xff);
    for (uint16_t j = 0; j < strip.numPixels(); j++) {
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
    }
    strip.show();
    delay(20);
}

void setColorDither(byte c) { // c is color.  This function does a "random dither" to set the new color
    // April Fool's day: ignore the color you picked and replace it with something random
    if (Time.day() == 1 && Time.month() == 4) {
        c = random(0,255);
    }
    // Determine highest bit needed to represent pixel index
    uint32_t color = wheelColor(c, lampBrightness);
    int hiBit = 0;
    int n = strip.numPixels() - 1;
    for(int bit=1; bit < 0x8000; bit <<= 1) {
        if(n & bit) hiBit = bit;
    }
    
    int bit, reverse;
    for(int i=0; i<(hiBit << 1); i++) {
        // Reverse the bits in i to create ordered dither:
        reverse = 0;
        for(bit=1; bit <= hiBit; bit <<= 1) {
            reverse <<= 1;
            if(i & bit) reverse |= 1;
        }
        if ( ((Time.month() * Time.day()) % 256) == c) {
            easterEggrollColor = 0;
            color = wheelColor((reverse * 256 / strip.numPixels()) & 255, lampBrightness);
        } else if (consecutiveChanges != 0 && consecutiveChanges % easterEggRollActivation == 0) {
            easterEggrollColor = 0;
            color = wheelColor((reverse * 256 / 6) & 255, lampBrightness);
        }
        if (reverse > activePixels) {
            strip.setPixelColor(reverse, 0, 0, 0);
        } else {
            strip.setPixelColor(reverse, color);
        }
        strip.show();
        delay(20);
    }
    activeColor = c;
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
        thisColor = wheelColor(((i * 400 / strip.numPixels()) + j) & 255, k);
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
    lampBrightness = 140;
    if ( maxBrightness < lampBrightness ) {
        lampBrightness = maxBrightness;
    }
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

void idleFireworks(uint8_t w) {
    // Emulates fireworks bursting inside lamp.  Single LEDs flash
    // in the specified color pattern:
    // `w = 0` for mulitcolor, `w = 1` for all white flashes
    lampBrightness = 140;
    uint16_t minColor = 20;
    if ( maxBrightness < lampBrightness ) {
        lampBrightness = maxBrightness;
    }
    //if (random(5) == 1) {
        uint16_t i = random(strip.numPixels());
        if (redStates[i] < minColor && greenStates[i] < minColor && blueStates[i] < minColor) {
            if (w == 0){
                redStates[i] = random(minColor,lampBrightness);
                greenStates[i] = random(minColor,lampBrightness);
                blueStates[i] = random(minColor,lampBrightness);
            } else {
                redStates[i] = lampBrightness;
                greenStates[i] = lampBrightness;
                blueStates[i] = lampBrightness;
            }
        }
    //}
    for(uint16_t l = 0; l < strip.numPixels(); l++) {
        if (redStates[l] > minColor || greenStates[l] > minColor || blueStates[l] > minColor) {
            strip.setPixelColor(l, redStates[l], greenStates[l], blueStates[l]);
            if (redStates[l] > minColor) {
                redStates[l] = redStates[l] - 1;
            } else {
                redStates[l] = 0;
            }
        
            if (greenStates[l] > minColor) {
                greenStates[l] = greenStates[l] - 1;
            } else {
                greenStates[l] = 0;
            }
        
            if (blueStates[l] > minColor) {
                blueStates[l] = blueStates[l] - 1;
            } else {
                blueStates[l] = 0;
            }
        
        } else {
            strip.setPixelColor(l, 0, 0, 0);
        }
    }
    strip.show();
}
void idleDisco() {
    // Recreation of 70s Disco floor.  Each cycle 60 random LEDs are updated
    // to 60 random colors and brightnesses.
    lampBrightness = 120;
    if ( maxBrightness < lampBrightness ) {
        lampBrightness = maxBrightness;
    }
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
void idleHeartbeat() {
    lampBrightness = 120;
    if ( maxBrightness < lampBrightness ) {
        lampBrightness = maxBrightness;
    }
    uint16_t endColor = 0;
    
    if( heartbeatDirector == 0 ) {
        endColor = lampBrightness * 0.6;
    }else if( heartbeatDirector == 1 ) {
        endColor = lampBrightness * 0.2;
    }else if( heartbeatDirector == 2 ) {
        endColor = lampBrightness;
    }else if( heartbeatDirector == 3 ) {
        endColor = lampBrightness * 0.12;
    } else {
        //do nothing, this will delay
    }

    if( heartbeatColor < endColor ) {
        for(int j=heartbeatColor; j<endColor; j+=4) {
            for(int i=25; i<35; i++) {
                strip.setPixelColor(i, j, 0, 0);
                
            }
        strip.show();
        delay(15);
        }
    } else if ( heartbeatColor > endColor ) {
        for(int j=heartbeatColor; j>endColor; j--) {
            for(int i=25; i<35; i++) {
                strip.setPixelColor(i, j, 0, 0);
                
            }
        strip.show();
        delay(30);
        }
    } else {
        delay(15);
        delay(15);
        delay(15);
        delay(15);
        delay(15);
        delay(15);
        delay(15);
    }
    
    heartbeatColor = endColor;
    
    heartbeatDirector++;
    heartbeatDirector%=4;
}

void idleEaster() {
    lampBrightness = 120;
    if ( maxBrightness < lampBrightness ) {
        lampBrightness = maxBrightness;
    }
    rainbowEasterEggroll(2);
}

Color getColorFromInteger(uint32_t col) {
    Color retVal;
    
    
    return retVal;
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

void shimmer() {
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
    strip.setPixelColor(i, strip.Color(currG, currR, currB));
    // every odd LED gets set to a quarter the brighness of the preceding even LED
    if (i + 1 < strip.numPixels())
    {
        strip.setPixelColor(i + 1, strip.Color(currR >> 2, currG >> 2, currB >> 2));
    }
  }
  strip.show();
  delay(80);
}

void candyCane() {
    // Fades to a candy cane stripe
    int currR, currG, currB;
    //white
    for (uint16_t i = 0; i < strip.numPixels(); i+=2) {
        long startRGB = strip.getPixelColor(i); // Get pixel's current color
        currR = (uint16_t)((startRGB >> 16) & 0xff); // Splits out current color into separate R, G, B
        currG = (uint16_t)((startRGB >> 8) & 0xff);
        currB = (uint16_t)(startRGB & 0xff);
        if ( currR > lampBrightness ) {
            currR--;
        } else if ( currR < lampBrightness ) {
            currR++;
        } else {
            currR = lampBrightness;
        }
        if ( currG > lampBrightness ) {
            currG--;
        } else if ( currG < lampBrightness ) {
            currG++;
        } else {
            currG = lampBrightness;
        }
        if ( currB > lampBrightness ) {
            currB--;
        } else if ( currB < lampBrightness ) {
            currB++;
        } else {
            currB = lampBrightness;
        }
        
        //Catch overflows
        currR %= 255;
        currG %= 255;
        currB %= 255;
        
        strip.setPixelColor(i, currR, currG, currB);
    }
    //red
    for (uint16_t i = 1; i < strip.numPixels(); i+=2) {
        long startRGB = strip.getPixelColor(i); // Get pixel's current color
        currG = (uint16_t)((startRGB >> 16) & 0xff); // Splits out current color into separate R, G, B
        currR = (uint16_t)((startRGB >> 8) & 0xff);
        currB = (uint16_t)(startRGB & 0xff);
        if ( currR > lampBrightness ) {
            currR--;
        } else if ( currR < lampBrightness ) {
            currR++;
        } else {
            currR = lampBrightness;
        }
        if ( currG > 0 ) {
            currG--;
        } else if ( currG < 0 ) {
            currG++;
        } else {
            currG = 0;
        }
        if ( currB > 0 ) {
            currB--;
        } else if ( currB < 0 ) {
            currB++;
        } else {
            currB = 0;
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
