//librarys
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include <QTRSensors.h>
#include <OneButton.h>

//inputs
const int led = 9;
#define DATA_PIN_MOUTH   5
#define DATA_PIN_EYES   6

#define EYE_L_PIN A2
#define EYE_R_PIN A3
OneButton button1(3, true);
#define MIC_PIN A0

//LEDstrip
#define NUM_LEDS_EYES 8           // number of LEDs in eye-strip
#define NUM_LEDS_MOUTH 14         // number of LEDs in mouth-strip
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB ledsEyes[NUM_LEDS_EYES];
CRGB ledsMouth[NUM_LEDS_MOUTH];
int brightness = 30;
uint8_t gHue = 0;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS_MOUTH, DATA_PIN_MOUTH, NEO_GRB + NEO_KHZ800);

//Button & Sensor
int btnCount = 3;
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

//Eye-"Matrix"
///L/////R///
//0123//4567//
int eyeOpen[] = {1, 2, 5, 6};
int eyeToLeft[4] = {0, 1, 4, 5};
int eyeToRight[4] = {2, 3, 6, 7};
int eyeLeft[2] = {0, 4};
int eyeRight[2] = {3, 7};
int blinkLeft[2] = {6, 5};
int blinkRight[2] = {1, 2};
int eyeMotion[7][4] =
{{0, 4, 4, 4}, {0, 1, 4, 5}, {1, 2, 5, 6}, {2, 3, 6, 7}, {3, 7, 7, 7}};

int sensorL, initL;
int sensorR, initR;
int span = 0;
int mappedSpan;


///////////////////setup///////////////////////////////////////
void setup() {

  Serial.begin(9600);
  pinMode (led, OUTPUT);

  //LEDs
  FastLED.addLeds<NEOPIXEL, DATA_PIN_EYES>(ledsEyes, 0, NUM_LEDS_EYES);
  FastLED.addLeds<NEOPIXEL, DATA_PIN_MOUTH>(ledsMouth, 0, NUM_LEDS_MOUTH);
  FastLED.setBrightness(brightness);
  strip.begin();
  strip.show(); // all pixels to 'off'

  //eye-sensor
  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]) {
    EYE_L_PIN, EYE_R_PIN
  }, SensorCount);
  initL = sensorValues[0];
  initR = sensorValues[1];

  //button init
  button1.attachClick(click1);
  button1.attachDuringLongPress(longPress1);
}


////////////////////loop//////////////////////////////////////
void loop() {
  button1.tick();
  switch (btnCount) {
    case 0:
      vu();
      earFade();
      eyeLed();
      FastLED.show();
      FastLED.clear();
      break;

    case 1:
      //      earFade();
      rainbowWithGlitter();

      FastLED.show();
      FastLED.clear();
      break;

    case 2:
      //      earFade();
      kitt();
      FastLED.show();

      break;
    case 3:
      eyeLed();
      FastLED.show();
      FastLED.clear();
Serial.println(span);
delay(40);
//      Serial.print("left: ");
//      Serial.print(sensorL);
//      Serial.print("  ");
//      Serial.print("right: ");
//      Serial.print(sensorR);
//      Serial.println("  ");
      break;
  }
}
