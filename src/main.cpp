#define numRounds       10    // 
//#define numArrows       3
#define secGetToLine    10    // time to get to the shootingline before the shooting starts
#define secShooting     90    // time for shooting
#define numGroups       4     // max 4 groups
#define maxBrightness   255   // max 0-255 (0 = off, 255 = full brightness)


unsigned int colorOfGroups[4] = {0, 21845, 10922, 43681};  // group 1/A: red, group 2/B: green, group 3/C: yellow, group 4/D: blue
int colorOfTimer              = 21845;  // green //enter color in HSV format (0-65535);
int colorOfGetToLine          = 43681;  // blue  //enter color in HSV format (0-65535);
#define NUM_PIXELS              18
float numLedGroupIndication =   3;
#define numLedGap               1
int numLedTimer = NUM_PIXELS - numLedGroupIndication - numLedGap;
bool HoldState = false;
bool FFWState =  false;

unsigned long timerOne = 0;


#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define PIN 6
#define FFWbutton 2
#define Holdbutton 3


Adafruit_NeoPixel pixels(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

//--------------------------------------------------------------------------------

void checkButtons(){
  int debounceDuration = 50;

  if (!digitalRead(FFWbutton)) {
    while (!digitalRead(FFWbutton)) {
      delay(debounceDuration);
    }
    if (FFWState == false){
      FFWState = true;
    } else {
      FFWState = false;
    }

    //Serial.print("FFWbutton pressed, FFWState: ");
    //Serial.println(FFWState);
  }

  if (!digitalRead(Holdbutton)) {
    while (!digitalRead(Holdbutton)) {
      delay(debounceDuration);
    }
    if (HoldState == false){
      HoldState = true;
    } else {
      HoldState = false;
    }

    //Serial.print("Holdbutton pressed, HoldState: ");
    //Serial.println(HoldState);
  }

}

void countDown(float firstPixel, float lastPixel, int color /*HSV*/ ,unsigned long duration){

  int iterations = log(maxBrightness) / log(1.05);
  float numPixels = lastPixel - firstPixel + 1;
  float fadeDelay = (duration*1000 / iterations)/numPixels; // delay for each brightness level; 1/numLedGroupIndication = 0 ... WHY?
  
  for (int i = firstPixel; i <= lastPixel; i++) {
    pixels.setPixelColor(i, pixels.ColorHSV(color, 255, maxBrightness));
  }
  pixels.show();


  for (int i = lastPixel; i >= firstPixel; i--) {
    for (float brightness = maxBrightness; brightness > 1; brightness = brightness / 1.05) {
      pixels.setPixelColor(i, pixels.ColorHSV(color, 255, brightness));
      pixels.show();
      delay(fadeDelay);
    
      checkButtons();
      while (HoldState==true)
      {
        checkButtons();
      }

      if (FFWState==true)
      {
        FFWState = false;
        return;
      }

  }
    pixels.setPixelColor(i, pixels.Color(0, 0, 0)); // set LED color to off
    pixels.show();
 
  }  
}

void fade(int color){
  pixels.clear();                                                           // clear all registers, in theory not necessary
  for (int i = 0; i < 3; i++) {                                             // repeat the fade 3 times
    for (int j = 0; j < NUM_PIXELS; j++) {                                  // fade in
      pixels.setPixelColor(j, pixels.ColorHSV(color, 255, maxBrightness));  // set LED color to maxBrightness
    }
    pixels.show();
    delay(500);                                                             // duration of maxBrightness

    for (int j = 0; j < NUM_PIXELS; j++) {                                  // fade out           
      pixels.setPixelColor(j, pixels.ColorHSV(color, 255, 0));              // set LED color to off
    }
    pixels.show();
    delay(500);                                                            // duration of off
  }
}

// void fade(int color){
//   int fadeDuration = 1000;      // fade duration in milliseconds
//   int fadeDelay = 10;           // delay between each fade step in milliseconds

//   for (int i = 0; i < 3; i++) { // repeat the fade 3 times
//     for (int brightness = 0; brightness <= maxBrightness; brightness++) { // fade in
//       for (int j = 0; j < NUM_PIXELS; j++) {
//         pixels.setPixelColor(j, pixels.ColorHSV(color, 255, brightness));
//       }
//       pixels.show();
//       delay(fadeDelay);
//     }

//     for (int brightness = maxBrightness; brightness >= 0; brightness--) { // fade out
//       for (int j = 0; j < NUM_PIXELS; j++) {
//         pixels.setPixelColor(j, pixels.ColorHSV(color, 255, brightness));
//       }
//       pixels.show();
//       delay(fadeDelay);
//     }
//   }
// }

void hold(){
  HoldState = true;
  while (HoldState==true && FFWState==false)
  {
    checkButtons();
  }
  FFWState = false;
  HoldState = false;
}



//--------------------------------------------------------------------------------

void setup() {
  //delay(3000); // 3 second delay for recovery
  pixels.begin();        // initialize NeoPixel strip object
  Serial.begin(9600);    // initialize serial communication at 9600 bits per second

  pinMode(FFWbutton, INPUT_PULLUP);   // both buttons are connected to ground and pulled up with internal pullup resistors
  pinMode(Holdbutton, INPUT_PULLUP);  
}

void loop() {
  //checkButtons();
  countDown(0,numLedTimer, colorOfGetToLine, secGetToLine); // get to the line
  countDown(0,numLedTimer, colorOfTimer, secShooting);      // shooting
  fade(21845);                                              // indicates the end of shooting 
  hold(); // holds every thing until the hold- or continue-button is pressed, time to collect the arrows and write down the scores

  

}