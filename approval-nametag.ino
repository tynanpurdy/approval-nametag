// tynan purdy
// id 3051 interactive studio - fall 2021
// approval nametag

// ------------------------------------------------------------------------------
// SETUP ------------------------------------------------------------------------
// ------------------------------------------------------------------------------

// imports ----------------------------------------------------------------------
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>
// ------------------------------------------------------------------------------

// ir setup (IR was cancelled) --------------------------------------------------
//#define IR_IN   A2
//#define IR_OUT   2
// ------------------------------------------------------------------------------

// mic + led setup (Adafruit AmpliTie) ------------------------------------------
#define N_PIXELS   8  // Number of pixels in strand
#define MIC_PIN   A0  // Microphone is attached to this analog pin
#define LED_PIN   21  // NeoPixel LED strand is connected to this pin
#define SAMPLE_WINDOW   10  // Sample window for average level
#define PEAK_HANG 24 // Time of pause before peak dot falls
#define PEAK_FALL 4 // Rate of falling peak dot
#define INPUT_FLOOR 50 // Lower range of analogRead input
#define INPUT_CEILING 500 // Max range of analogRead input, 
                           // the lower the value the more sensitive (1023 = max)

byte peak = 16;      // Peak level of column; used for falling dots
unsigned int sample;

byte dotCount = 0;  //Frame counter for peak dot
byte dotHangCount = 0; //Frame counter for holding peak dot

Adafruit_NeoPixel
    strip = Adafruit_NeoPixel(N_PIXELS, LED_PIN);
// ------------------------------------------------------------------------------

// oled featherwing -------------------------------------------------------------
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
#define BUTTON_A 15
#define BUTTON_B 32
#define BUTTON_C 14
// ------------------------------------------------------------------------------

// global variables -------------------------------------------------------------
int score = 0;         // num points on screen
int limit1 = 200;      // level 1 sound threshold
int limit2 = 500;     // level 2 sound threshold
int limit3 = 500;     // level 3 sound threshold
int sound = 0;         // mic reading container
int lastSound = 0;
int soundState;
int lastSoundState = 0;
int lastSoundDebounceTime = 0;
bool operating = true; // on / off state
// ------------------------------------------------------------------------------

// Arduino Debounce page --------------------------------------------------------
// Variables will change:
int ledState = HIGH;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = 1;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

// ------------------------------------------------------------------------------

// initialize -------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    
//    pinMode(IR_IN, INPUT);
//    pinMode(IR_OUT, OUTPUT);
    pinMode(MIC_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_B, INPUT);
    
    strip.begin();
    strip.show();
    
    display.begin(0x3C, true);
    display.display();
    delay(1000);
    display.clearDisplay();
    display.display();
    display.setRotation(-1);
    display.setTextSize(9);
    display.setTextColor(SH110X_WHITE);
    newScreen(0);
}
// -------------------------------------------------------------------------------

// -------------------------------------------------------------------------------
// MAIN LOOP ---------------------------------------------------------------------
// -------------------------------------------------------------------------------

void loop() {
    if (isOn()) {
        Serial.println(analogRead(MIC_PIN));
        ampLEDtude();
        updateScore(isHighFive());
    }
}

// --------------------------------------------------------------------------------
// HELPER FUNCTIONS ---------------------------------------------------------------
// --------------------------------------------------------------------------------

// manages on/off state and resets score ------------------------------------------
bool isOn() {
    // Arduino Debounce tutorial --------------------------------------------------
    // read the state of the switch into a local variable:
    int reading = digitalRead(BUTTON_B);

    // check to see if you just pressed the button
    // (i.e. the input went from LOW to HIGH), and you've waited long enough
    // since the last press to ignore any noise:

    // If the switch changed, due to noise or pressing:
    if (reading != lastButtonState) {
        // reset the debouncing timer
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        // whatever the reading is at, it's been there for longer than the debounce
        // delay, so take it as the actual current state:

        // if the button state has changed:
        if (reading != buttonState) {
            buttonState = reading;
  
            // only toggle the LED if the new button state is HIGH
            if (buttonState == 0) {
                operating = !operating;
            }
        }
    }

    // set the LED:
    // digitalWrite(ledPin, ledState);
    if (!operating) {
        score = 0;
    }
    newScreen(score);

    // save the reading. Next time through the loop, it'll be the lastButtonState:
    lastButtonState = reading;
    // -----------------------------------------------------------------------------
    return operating;
}
// ---------------------------------------------------------------------------------

// detect another nametag (IR cancelled) -------------------------------------------
//bool handshake() {
//    if (digitalRead(IR_IN) > 100) {
//        Serial.println("handshake confirmed");
////        strip.setPixelColor(0, green);
//        strip.show();
//        return true;
//    } else {
//        Serial.println("no handshake");
//        strip.clear();
//        strip.show();
//        return false;
//    }
//}
// ---------------------------------------------------------------------------------

// detect a high-five sound --------------------------------------------------------
int isHighFive() {
    sound = analogRead(MIC_PIN);

    // debounce - if the sound is past it's peak, push it to 0
    if (sound < lastSoundState) {
        sound = 0;
    }
    
    lastSoundState = sound;

    if (sound < limit1) {
        return 0;
    } else if (sound < limit2) {
        return 1;
    } else if (sound < limit3) {
        return 2;
    } else {
        return 5;
    }
}
// ---------------------------------------------------------------------------------

// update the score displayed on the screen ----------------------------------------
void updateScore(int change) {
    score += change;
    // floor score at -9
    if (score <= -9) {
        score = -9;  
    }
    if (score >= 50) {
        score = 0;  
    }
    newScreen(score);
}
// ---------------------------------------------------------------------------------

// update the display with a new score ---------------------------------------------
void newScreen(int score_in) {
    display.clearDisplay();
    display.setCursor(0,0);
    if (operating) {
        display.println(score_in);
    }
    display.display();
}
// ---------------------------------------------------------------------------------

// Adafruit VU Meter Baseball Hat --------------------------------------------------
void ampLEDtude() {
    unsigned long startMillis= millis();  // Start of sample window
    float peakToPeak = 0;   // peak-to-peak level

    unsigned int signalMax = 0;
    unsigned int signalMin = 1023;
    unsigned int c, y;

    // collect data for length of sample window (in mS)
    while (millis() - startMillis < SAMPLE_WINDOW)
    {
        sample = analogRead(MIC_PIN);
        if (sample < 1024)  // toss out spurious readings
        {
            if (sample > signalMax)
            {
                signalMax = sample;  // save just the max levels
            }
            else if (sample < signalMin)
            {
                signalMin = sample;  // save just the min levels
            }
        }
    }
    peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
   
    //Fill the strip with rainbow gradient
    for (int i=strip.numPixels();i>=0;i--) {
        strip.setPixelColor(i,Wheel(map(i,0,strip.numPixels(),30,150)));
    }
  
  
    //Scale the input logarithmically instead of linearly
    c = fscale(INPUT_FLOOR, INPUT_CEILING, strip.numPixels(), 0, peakToPeak, 2);
  
    if(c < peak) {
        peak = c;        // Keep dot on top
        dotHangCount = 0;    // make the dot hang before falling
    }
    if (c <= strip.numPixels()) { // Fill partial column with off pixels
        drawLine(0, -c, strip.Color(0, 0, 0));
    }
  
    // Set the peak dot to match the rainbow gradient
    y = strip.numPixels() - peak;
    
    strip.setPixelColor(y-1,Wheel(map(y,0,strip.numPixels(),30,150)));
  
    strip.show();
  
    // Frame based peak dot animation
    if(dotHangCount > PEAK_HANG) { //Peak pause length
        if(++dotCount >= PEAK_FALL) { //Fall rate 
          peak--;
          dotCount = 0;
        }
    } 
    else {
        dotHangCount++; 
    }
}

//Used to draw a line between two points of a given color
void drawLine(uint8_t from, uint8_t to, uint32_t c) {
  uint8_t fromTemp;
  if (from > to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  for(int i=from; i<=to; i++){
    strip.setPixelColor(i, c);
  }
}


float fscale( float originalMin, float originalMax, float newBegin, float
newEnd, float inputValue, float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){ 
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {   
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }

  return rangedValue;
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
// ---------------------------------------------------------------------------------
