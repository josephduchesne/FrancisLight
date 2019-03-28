/* A mashup of the fastled and bmp180 demos */
#include <FastLED.h>

#include <Wire.h>

#include <BMP180MI.h>

#define I2C_ADDRESS 0x77

//create an BMP180 object using the I2C interface
BMP180I2C bmp180(I2C_ADDRESS);

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    D4
//#define CLK_PIN   4
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    9
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          255
#define FRAMES_PER_SECOND  60

#define SAMPLES 20
float readings[SAMPLES];
int sample_index = -1;

#define CHANGE_DEBOUNCE 500
unsigned long int last_change = 0;


void setup() {
  delay(3000); // 3 second delay for recovery

  // put your setup code here, to run once:
  Serial.begin(115200);

  //wait for serial connection to open (only necessary on some boards)
  while (!Serial);

  Wire.begin();

  //begin() initializes the interface, checks the sensor ID and reads the calibration parameters.  
  if (!bmp180.begin())
  {
    Serial.println("begin() failed. check your BMP180 Interface and I2C Address.");
    while (1);
  }

  //reset sensor to default parameters.
  bmp180.resetToDefaults();

  //enable ultra high resolution mode for pressure measurements
  bmp180.setSamplingMode(BMP180MI::MODE_UHR);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, confetti, sinelon, bpm, off };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void loop()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();
  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  
  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) {
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  if ( bmp180.hasValue() ){
    Serial.print("Pressure: "); 
    float pressure = bmp180.getPressure()/1000;
    Serial.print(pressure);
    Serial.print(" kPa baseline: ");
  
    float baseline;
    if (sample_index == -1) {
      for(int i=0; i<SAMPLES; i++) readings[i]=pressure;
      sample_index = 0;
      baseline = pressure;
    } else {
      readings[sample_index] = pressure;
      baseline = readings[0];
      if (sample_index>=SAMPLES) sample_index=0;
      for(int i=1; i<SAMPLES; i++) baseline = min(readings[i], baseline);
    }
    Serial.print(baseline);
    Serial.print(" delta: ");
    Serial.print(pressure-baseline);
    Serial.println("kPa");
    
  
    if (pressure-baseline > 0.5 && millis()-last_change>CHANGE_DEBOUNCE) {
      last_change = millis();
      nextPattern();
    }
  }
  bmp180.measurePressure();
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void off()
{
  FastLED.clear();
}

void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS - 1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

