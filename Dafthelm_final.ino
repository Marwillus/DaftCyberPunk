//librarys
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include <QTRSensors.h>
#include <OneButton.h>

//inputs
const int led = 9;
#define DATA_PIN_MOUTH   5
#define DATA_PIN_EYES   6
#define MIC_PIN A5

//LEDstrip
#define NUM_LEDS_MOUTH 14         // number of LEDs in mouth-strip
#define NUM_LEDS_EYES 8           // number of LEDs in eye-strip
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB ledsMouth[NUM_LEDS_MOUTH];       
CRGB ledsEyes[NUM_LEDS_EYES];
#define BRIGHTNESS  30
uint8_t gHue = 0;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS_MOUTH, DATA_PIN_MOUTH, NEO_GRB + NEO_KHZ800);

//Button & Sensor
OneButton button1(8, true);
#define RIGHT_EYE = A0;
#define LEFT_EYE = A1;     
QTRSensors qtr;
const uint8_t SensorCount = 2;
uint16_t sensorValues[SensorCount];

//VU Variables
#define SAMPLE_WINDOW   5  // Sample window for average level
#define PEAK_HANG 24 //Time of pause before peak dot falls
#define PEAK_FALL 4 //Rate of falling peak dot
#define INPUT_FLOOR 250 //Lower range of analogRead input
#define INPUT_CEILING 900 //Max range of analogRead input, the lower the value the more sensitive (1023 = max)
#define SAMPLES   60  // Length of buffer for dynamic level adjustment
byte peak = 16;      // Peak level of column; used for falling dots
unsigned int sample;

byte dotCount = 0;  //Frame counter for peak dot
byte dotHangCount = 0; //Frame counter for holding peak dot

///L/////R///
//0123//4567//
int eyeOpen[] = {1, 2, 5, 6};
int eyeToLeft[4] = {0, 1, 4, 5};
int eyeToRight[4] = {2, 3, 6, 7};
int eyeLeft[2] = {0,4};
int eyeRight[2]= {3,7};
int blinkLeft[2]= {6,5};
int blinkRight[2]= {1,2};
int eyeMotion[7][4]=
{{0,4,4,4},{0, 1, 4, 5},{1, 2, 5, 6},{2, 3, 6, 7},{3,7,7,7}};

int sensorL, initL;
int sensorR, initR;
int span= 0;
int mappedSpan;


///////////////////setup///////////////////////////////////////
void setup() {

  Serial.begin(9600);
  pinMode (led, OUTPUT);


  //LEDs
  //FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(ledsMouth, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<NEOPIXEL, DATA_PIN_EYES>(ledsEyes, 0, NUM_LEDS_MOUTH);
  FastLED.addLeds<NEOPIXEL, DATA_PIN_MOUTH>(ledsMouth, 0, DATA_PIN_EYES);
  FastLED.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show(); // all pixels to 'off'

  //eye-sensor
  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]) {
    A0, A1
  }, SensorCount);
  initL = sensorValues[0];
  initR = sensorValues[1];

  Serial.begin(9600);

  button1.attachClick(click1);
  button1.attachDuringLongPress(longPress1);
}


////////////////////loop//////////////////////////////////////
void loop() {
  vu();
  earFade();
  eyeLed();
  FastLED.show();
  FastLED.clear();
}



/////////////////////////////////////////////// button functions ////////////////////
void click1(){

   Serial.print("span: ");
   Serial.print(span);
   Serial.print("   ");
   Serial.print("mapped: ");
   Serial.print(mappedSpan);
   Serial.println();
}

void longPress1() {

//  Serial.print("left: ");
//  Serial.print(sensorL );
//  Serial.print("  ");
//  Serial.print("right: ");
//  Serial.print(sensorR);
//  Serial.print("  ");
  Serial.print("span: ");
   Serial.print(span);
  Serial.println();
  delay(10);
}


////////////////////////////////////////////////// ear LEDs ////////////////////////////
void earFade() {
  int fade = beatsin16( 10, 3, 200 );
  analogWrite(led, fade);

}

////////////////////////////////////////////////// eye effect///////////////////////
void eyeLed() {

  qtr.read(sensorValues);
  sensorR = sensorValues[0];
  sensorL = sensorValues[1];
  button1.tick();
  
  span= sensorR-sensorL;
  mappedSpan=constrain(map(span,-10,60,0,4),0,4);

  unsigned long startMillis = millis();
  
  while (millis() - startMillis < SAMPLE_WINDOW){
    if (span<-40){
      initializeEye(blinkLeft,2);
    } 
    else if(span>100){
      initializeEye(blinkRight,2);
    }
    else if(sensorR<800&&sensorL<800){
      FastLED.clear();
    }
    else if(span>15&&span<35){
      initializeEye(eyeOpen,4);
    }
    else{
  initializeEye(eyeMotion[mappedSpan],4);
    }
  }
}

void initializeEye(int arr[],int arrLength){
  for (int i=0;i<arrLength;i++){
    ledsEyes[arr[i]] = CRGB::Red;
  }
}



////////////////////////////////////////////////// mouth effects////////////////////
void rainbow() {
  EVERY_N_MILLISECONDS( 20 ) {
    gHue++;
  }
  fill_rainbow( ledsMouth, NUM_LEDS_MOUTH / 2, gHue, 12 );
}

void kitt() {
  fadeToBlackBy( ledsMouth, NUM_LEDS_MOUTH, 50);
  int pos = beatsin16( 14, 0, NUM_LEDS_MOUTH - 1 );
  // ledsMouth[pos] += CHSV( gHue, 0, 0);
  ledsMouth[pos] = CRGB::Red;
}

//////////////////////////////////////////////////// VU /////////////////////////////
void vu()
{
  unsigned long startMillis = millis(); // Start of sample window
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

  // Serial.println(peakToPeak);


///////////////////////////set strip color
  for (int i = 0; i <= NUM_LEDS_MOUTH - 1; i++) {
//    strip.setPixelColor(i, Wheel(map(i, 0, NUM_LEDS - 1, 30, 150))); //rainbow gradient
  strip.setPixelColor(i,200,0,0); //red
  }


  /////////////////////////Scale the input logarithmically instead of linearly
  c = fscale(INPUT_FLOOR, INPUT_CEILING, NUM_LEDS_MOUTH,0, peakToPeak, 2);

  if (c < peak) {
    peak = c;        // Keep dot on top
    dotHangCount = 0;    // make the dot hang before falling
  }
  if (c <= NUM_LEDS_MOUTH) { // Fill partial column with off pixels
    //    drawLine(NUM_LEDS, NUM_LEDS - c, strip.Color(0, 0, 0)); <-orginal
    drawLine(NUM_LEDS_MOUTH, NUM_LEDS_MOUTH - c/2, strip.Color(0, 0, 0));
    drawLineReverse(0, 0 +c/2 , strip.Color(0, 0, 0));
  }

///////////////////////////// Set peak dot to match the color
//  y = NUM_LEDS - peak;
//  strip.setPixelColor(y - 1, Wheel(map(y, 0, NUM_LEDS - 1, 30, 150)));
  
  strip.show();

  // Frame based peak dot animation
  if (dotHangCount > PEAK_HANG) { //Peak pause length
    if (++dotCount >= PEAK_FALL) { //Fall rate
      peak++;
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
  for (int i = from; i <= to; i++) {
    strip.setPixelColor(i, c);
  }
}
void drawLineReverse(uint8_t from, uint8_t to, uint32_t c) {
  uint8_t fromTemp;
  if (from < to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  for (int i = from; i >= to; i--) {
    strip.setPixelColor(i, c);
  }
}

float fscale( float originalMin, float originalMax, float newBegin, float
              newEnd, float inputValue, float curve) {

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

  /*
    Serial.println(curve * 100, DEC);   // multply by 100 to preserve resolution
    Serial.println();
  */

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin) {
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

  if (invFlag == 0) {
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
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
