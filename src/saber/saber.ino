/*
  Copyright (c) 2016 Lee Thomason, Grinning Lizard Software

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
  of the Software, and to permit persons to whom the Software is furnished to do
  so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

// Arduino Libraries
#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SPIFlash.h>
#include <OLED_SSD1306.h>

#include <Audio.h>
#include <Button.h>
#include <Adafruit_LIS3DH.h>
#include <NXPMotionSense.h>
#include <Grinliz_Arduino_Util.h>
#include <DotStar.h>

// Includes
// -- Must be first. Has configuration. -- //
#include "pins.h"
#include "electrical.h"

#ifdef SABER_SOUND_ON
#include "sfx.h"
#include "AudioPlayer.h"
#endif
#include "saberdb.h"
#include "cmdparser.h"
#include "blade.h"
#include "sketcher.h"
#include "renderer.h"
#include "saberUtil.h"
#include "tester.h"

static const uint8_t  BLADE_BLACK[NCHANNELS]  = {0};
static const uint32_t FLASH_TIME              = 120;
static const uint32_t VBAT_TIME_INTERVAL      = 500;

static const uint32_t INDICATOR_TIME          = 500;
static const uint32_t INDICATOR_CYCLE         = INDICATOR_TIME * 2;
static const float    GFORCE_RANGE            = 4.0f;
static const float    GFORCE_RANGE_INV        = 1.0f / GFORCE_RANGE;

bool     paletteChange  = false; // used to prevent sound fx on palette changes
uint32_t reflashTime    = 0;
bool     flashOnClash   = false;
float    maxGForce2     = 0.0f;

BladeState  bladeState;
ButtonCB    buttonA(PIN_SWITCH_A, Button::PULL_UP);
LEDManager  ledA(PIN_LED_A, false);
LEDManager  ledB(PIN_LED_B, false);  // Still have LEDB support, even if 1 button.

UIRenderData uiRenderData;

#ifdef SABER_TWO_BUTTON
ButtonCB    buttonB(PIN_SWITCH_B, Button::PULL_UP);
#else
ButtonMode  buttonMode;
#endif
SaberDB     saberDB;
AveragePower averagePower;

#if SABER_CRYSTAL == SABER_DOTSTAR
DotStar::RGB leds[SABER_NUM_LEDS];
DotStar dotstar(PIN_DOTSTAR_EN);
#  ifdef SABER_DOTSTAR_UI
DotStarUI dotstarUI;
#  endif
#endif

#if SABER_ACCELEROMETER == SABER_ACCELEROMETER_LIS3DH
Adafruit_LIS3DH accel;
#elif SABER_ACCELEROMETER == SABER_ACCELEROMETER_NXP
NXPMotionSense accel;
#endif

#ifdef SABER_DISPLAY
static const int OLED_WIDTH = 128;
static const int OLED_HEIGHT = 32;
uint8_t oledBuffer[OLED_WIDTH * OLED_HEIGHT / 8] = {0};

OLED_SSD1306 display(PIN_OLED_DC, PIN_OLED_RESET, PIN_OLED_CS);
Timer displayTimer(100);
Sketcher sketcher;
Renderer renderer;
#endif
#ifdef SABER_SOUND_ON
AudioPlayer audioPlayer;
SFX sfx(&audioPlayer);
#endif

CMDParser cmdParser(&saberDB);
Blade blade;
Timer vbatTimer(VBAT_TIME_INTERVAL);
Timer gforceDataTimer(110);
File logFile;

Tester tester;

void setupSD(int logCount) 
{
  SPI.setMOSI(PIN_SABER_MOSI);
  SPI.setSCK(PIN_SABER_CLOCK);
#ifdef SABER_SOUND_ON
  if (!(SD.begin(PIN_SDCARD_CS))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  SD.mkdir("logs");
  char path[] = "logs/log00.txt";
  path[8] = ((logCount / 10) % 10) + '0';
  path[9] = (logCount % 10) + '0';
  logFile = SD.open(path, FILE_WRITE);
  logFile.print("Log open. Instance="); logFile.println(logCount);
#endif
}

void setup() {
  pinMode(PIN_AMP_EN, OUTPUT);
  digitalWrite(PIN_AMP_EN, LOW);

  Serial.begin(19200);  // still need to turn it on in case a command line is connected.
#if SERIAL_DEBUG == 1
  while (!Serial) {
    delay(100);
  }
#endif
  //TCNT1 = 0x7FFF; // set blue & green channels out of phase
  // Database is the "source of truth".
  // Set it up first.
  saberDB.readData();

#if SERIAL_DEBUG == 1
  Log.attachSerial(&Serial);
#endif
#ifdef SABER_SOUND_ON
  setupSD(saberDB.numSetupCalls());
  Log.attachLog(&logFile);
#endif

  Log.p("setup()").eol();

#if SABER_ACCELEROMETER == SABER_ACCELEROMETER_LIS3DH
  Log.p("LIS3DH Accelerometer starting.").eol();
  if (!accel.begin()) {
    Log.p("Accelerometer ERROR.").eol();
  }
  else {
    Log.p("Accelerometer open.").eol();
    accel.setRange(LIS3DH_RANGE_4_G);
    accel.setDataRate(LIS3DH_DATARATE_100_HZ);
  }
#elif SABER_ACCELEROMETER == SABER_ACCELEROMETER_NXP
  Log.p("NXP Accelerometer starting.").eol();
  if (accel.begin()) {
    Log.p("Accelerometer open.").eol();
  }
  else {
    Log.p("Accelerometer ERROR.").eol();
  }
#endif

#ifdef SABER_VOLTMETER
  analogReference(INTERNAL);  // 1.1 volts
  analogRead(PIN_VMETER);     // warm up the ADC to avoid spurious initial value.
  Log.p("Voltmeter open.").eol();
#endif

  blade.setRGB(BLADE_BLACK);

  buttonA.holdHandler(buttonAHoldHandler);
  buttonA.clickHandler(buttonAClickHandler);
  buttonA.releaseHandler(buttonAReleaseHandler);

#ifdef SABER_TWO_BUTTON
  buttonB.clickHandler(buttonBClickHandler);
  buttonB.releaseHandler(buttonBReleaseHandler);
  buttonB.holdHandler(buttonBHoldHandler);
  buttonB.pressHandler(buttonBPressHandler);
#endif

#ifdef SABER_TWO_BUTTON
  tester.attach(&buttonA, &buttonB);
#else
  tester.attach(&buttonA, 0);
#endif

#ifdef SABER_SOUND_ON
  sfx.init();
  Log.p("sfx initialized.").eol();
#endif

  blade.setVoltage(averagePower.power());

#ifdef SABER_DISPLAY
  display.begin(OLED_WIDTH, OLED_HEIGHT, SSD1306_SWITCHCAPVCC);
  renderer.Attach(OLED_WIDTH, OLED_HEIGHT, oledBuffer);
  renderer.Fill(0);
  display.display(oledBuffer);

  Log.p("OLED display connected.").eol();
#endif

#if SABER_CRYSTAL == SABER_RGB_CRYSTAL
  pinMode(PIN_CRYSTAL_R, OUTPUT);
  pinMode(PIN_CRYSTAL_G, OUTPUT);
  pinMode(PIN_CRYSTAL_B, OUTPUT);
#elif SABER_CRYSTAL == SABER_DOTSTAR
  dotstar.setBrightness(12);
  dotstar.attachLEDs(leds, SABER_NUM_LEDS);
  for(int i=0; i<SABER_NUM_LEDS; ++i) {
    leds[i].set(0x010101);
  }
  dotstar.display();
  dotstar.display();
#endif

  syncToDB();
  ledA.set(true); // "power on" light
  Log.event("[saber start]");
}

uint32_t calcReflashTime() {
  return millis() + random(500) + 200;
}

int vbatToPowerLevel(int32_t vbat)
{
  int level = 0;
  if      (vbat > 3950) level = 4;
  else if (vbat > 3800) level = 3;
  else if (vbat > 3650) level = 2;
  else if (vbat > LOW_VOLTAGE) level = 1;
  return level;
}

/*
   The saberDB is the source of true. (The Model.)
   Bring other things in sync when it changes.
*/
void syncToDB()
{
#ifdef SABER_SOUND_ON
  sfx.setFont(saberDB.soundFont());
  sfx.mute(!saberDB.soundOn());
  sfx.setVolume204(saberDB.volume());
#endif

  uiRenderData.volume = saberDB.volume4();
  for (int i = 0; i < 3; ++i) {
    uiRenderData.color[i] = saberDB.bladeColor()[i];
  }
  uiRenderData.palette = saberDB.paletteIndex();
  uiRenderData.power = vbatToPowerLevel(averagePower.power());
#ifdef SABER_SOUND_ON
  uiRenderData.fontName = sfx.currentFontName();
#endif

  if (!ledB.blinking()) {   // If blinking, then the LED is being used as UI.
    ledB.set(saberDB.soundOn());
  }
}

void buttonAReleaseHandler(const Button&)
{
  ledA.blink(0, 0);
}

#ifdef SABER_TWO_BUTTON
void blinkVolumeHandler(const LEDManager& manager)
{
  saberDB.setVolume4(manager.numBlinks());
  syncToDB();
}

void buttonAClickHandler(const Button&)
{
  Log.p("buttonAClickHandler...");
  // Special case: color switch.
  if (bladeState.state() == BLADE_ON && buttonB.isDown()) {
    Log.p("palette change.").eol();
    saberDB.nextPalette();
    paletteChange = true;
    syncToDB();
  }
  else {
    int32_t vbat = averagePower.power();
    Log.event("[Vbat]", vbat);
    ledA.blink(vbatToPowerLevel(vbat), INDICATOR_CYCLE, 0, LEDManager::BLINK_TRAILING);
  }
}


void buttonAHoldHandler(const Button&) {
  Log.p("buttonAHoldHandler").eol();
  if (bladeState.state() == BLADE_OFF) {
    bladeState.change(BLADE_IGNITE);
#   ifdef SABER_SOUND_ON
    sfx.playSound(SFX_POWER_ON, SFX_OVERRIDE);
#   endif
  }
  else if (bladeState.state() != BLADE_RETRACT) {
    bladeState.change(BLADE_RETRACT);
#   ifdef SABER_SOUND_ON
    sfx.playSound(SFX_POWER_OFF, SFX_OVERRIDE);
#   endif
  }
}

void buttonBPressHandler(const Button&) {
  paletteChange = false;
}

void buttonBHoldHandler(const Button&) {
  Log.p("buttonBHoldHandler").eol();
  if (bladeState.state() != BLADE_OFF) {
    if (!paletteChange) {
      bladeState.change(BLADE_FLASH);
#ifdef SABER_SOUND_ON
      sfx.playSound(SFX_USER_HOLD, SFX_OVERRIDE);
#endif
      flashOnClash = true;
      reflashTime = calcReflashTime();
    }
  }
  else if (bladeState.state() == BLADE_OFF) {
    if (saberDB.soundOn()) {
      saberDB.setSoundOn(false);
      syncToDB();
    }
    else {
      ledB.blink(4, INDICATOR_CYCLE, blinkVolumeHandler);
    }
  }
}

void buttonBReleaseHandler(const Button& b) {
  if (flashOnClash && bladeState.state() != BLADE_OFF) {
#ifdef SABER_SOUND_ON
    sfx.playSound(SFX_IDLE, SFX_OVERRIDE);
#endif
  }
  flashOnClash = false;
  if (ledB.blinking()) {
    ledB.blink(0, 0);
  }
  syncToDB();
}

void buttonBClickHandler(const Button&) {
  Log.p("buttonBClickHandler").eol();
  if (bladeState.state() == BLADE_ON) {
    if (!paletteChange) {
      bladeState.change(BLADE_FLASH);
#ifdef SABER_SOUND_ON
      sfx.playSound(SFX_USER_TAP, SFX_GREATER_OR_EQUAL);
#endif
    }
  }
}

#else

void blinkVolumeHandler(const LEDManager& manager)
{
  saberDB.setVolume4(manager.numBlinks() - 1);
  syncToDB();
}

void blinkPaletteHandler(const LEDManager& manager)
{
  saberDB.setPalette(manager.numBlinks() - 1);
  syncToDB();
}

// One button case.
void buttonAClickHandler(const Button&)
{
  if (bladeState.bladeOff()) {
    buttonMode.nextMode();
  }
  else if (bladeState.state() == BLADE_ON) {
    bladeState.change(BLADE_FLASH);
#ifdef SABER_SOUND_ON
    sfx.playSound(SFX_USER_TAP, SFX_GREATER_OR_EQUAL);
#endif
  }
}

void buttonAHoldHandler(const Button&)
{
  if (bladeState.state() == BLADE_OFF) {

    if (buttonMode.mode() == BUTTON_MODE_NORMAL) {
      bladeState.change(BLADE_IGNITE);
#ifdef SABER_SOUND_ON
      sfx.playSound(SFX_POWER_ON, SFX_OVERRIDE);
#endif
    }
    else if (buttonMode.mode() == BUTTON_MODE_PALETTE) {
      saberDB.setPalette(0);
      ledA.blink(SaberDB::NUM_PALETTES, INDICATOR_CYCLE, blinkPaletteHandler);
    }
    else if (buttonMode.mode() == BUTTON_MODE_VOLUME) {
      ledA.blink(5, INDICATOR_CYCLE, blinkVolumeHandler);
    }

  }
  else if (bladeState.state() != BLADE_RETRACT) {
    bladeState.change(BLADE_RETRACT);
#ifdef SABER_SOUND_ON
    sfx.playSound(SFX_POWER_OFF, SFX_OVERRIDE);
#endif
  }
}


#endif

void processBladeState()
{
  switch (bladeState.state()) {
    case BLADE_OFF:
      break;

    case BLADE_ON:
      blade.setRGB(saberDB.bladeColor());
      break;

    case BLADE_IGNITE:
      {
        uint32_t igniteTime = 1000;
#ifdef SABER_SOUND_ON
        igniteTime = sfx.getIgniteTime();
#endif
        bool done = blade.setInterp(millis() - bladeState.startTime(), igniteTime, BLADE_BLACK, saberDB.bladeColor());
        if (done) {
          bladeState.change(BLADE_ON);
        }
      }
      break;

    case BLADE_RETRACT:
      {
        uint32_t retractTime = 1000;
#ifdef SABER_SOUND_ON
        retractTime = sfx.getRetractTime();
#endif
        bool done = blade.setInterp(millis() - bladeState.startTime(), retractTime, saberDB.bladeColor(), BLADE_BLACK);
        if (done) {
          bladeState.change(BLADE_OFF);
        }
      }
      break;

    case BLADE_FLASH:
      // interpolate?
      {
        uint32_t delta = millis() - bladeState.startTime();
        if (delta > FLASH_TIME) {
          blade.setRGB(saberDB.bladeColor());
          bladeState.change(BLADE_ON);
        }
        else {
          blade.setRGB(saberDB.impactColor());
        }
      }
      break;

    default:
      ASSERT(false);
      break;
  }
}

void serialEvent() {
  bool processed = false;
  uint8_t color[NCHANNELS] = {0};

  while (Serial.available()) {
    int c = Serial.read();
    if (c == '\n') {
#if 0
      Serial.print("event "); Serial.println(cmdParser.getBuffer());
#endif
      processed = cmdParser.processCMD(color);
    }
    else {
      cmdParser.push(c);
    }
  }
  if (processed) {
    syncToDB();
  }
}

void loop() {
  const uint32_t msec = millis();
  tester.process();

  buttonA.process();
  ledA.process();
#ifdef SABER_TWO_BUTTON
  buttonB.process();
  ledB.process();
#endif

  tester.process();
  if (bladeState.state() == BLADE_ON) {
    float g2Normal = 1.0f;
    float g2 = 1.0f;
#if SABER_ACCELEROMETER == SABER_ACCELEROMETER_LIS3DH
    accel.read();

    STATIC_ASSERT(BLADE_AXIS != NORMAL_AXIS_A);
    STATIC_ASSERT(BLADE_AXIS != NORMAL_AXIS_B);

    float bladeAxis = (&accel.x_g)[BLADE_AXIS];
    float normalA = (&accel.x_g)[NORMAL_AXIS_A];
    float normalB = (&accel.x_g)[NORMAL_AXIS_B];

    g2Normal = normalA * normalA + normalB * normalB;
    g2 = g2Normal + bladeAxis * bladeAxis;

#elif SABER_ACCELEROMETER == SABER_ACCELEROMETER_NXP
    // Using the prop shield: x is in the blade direction,
    // y & z are normal.

    float ax=0, ay=0, az=0, gx=0, gy=0, gz=0;
    accel.readMotionSensor(ax, ay, az, gx, gy, gz);
    g2Normal = ay * ay + az * az;
    g2 = g2Normal + ax * ax;
    //Log.p("g2Normal ").p(g2Normal).p(" g2 ").p(g2).eol();
    //delay(50);
#endif

    maxGForce2 = max(maxGForce2, g2);
    float motion = saberDB.motion();
    float impact = saberDB.impact();

    if ((g2Normal >= impact * impact)) {
      bool sound = false;
#ifdef SABER_SOUND_ON
      sound = sfx.playSound(SFX_IMPACT, SFX_GREATER_OR_EQUAL);
#endif

      if (sound) {
        Log.p("Impact. g=").p(sqrt(g2)).eol();
        bladeState.change(BLADE_FLASH);
      }
    }
    else if ( g2 >= motion * motion) {
      bool sound = false;
#ifdef SABER_SOUND_ON
      sound = sfx.playSound(SFX_MOTION, SFX_GREATER);
#endif
      if (sound) {
        Log.p("Motion. g=").p(sqrt(g2)).eol();
      }
    }
  }
  else {
    maxGForce2 = 1;
  }

  processBladeState();
  tester.process();
#ifdef SABER_SOUND_ON
  sfx.process();
#endif
  tester.process();

  if (vbatTimer.tick()) {
    averagePower.push(readVbat());
    blade.setVoltage(averagePower.power());
  }

  if (gforceDataTimer.tick()) {
#ifdef SABER_DISPLAY
    maxGForce2 = constrain(maxGForce2, 0.1, 16);
    static const float MULT = 256.0f / GFORCE_RANGE;  // g=1 translates to uint8 64
    const uint8_t gForce = constrain(sqrtf(maxGForce2) * MULT, 0, 255);
    sketcher.Push(gForce);
#endif
    maxGForce2 = 0;
  }

  if (reflashTime && msec >= reflashTime) {
    reflashTime = 0;
    if (flashOnClash && bladeState.state() == BLADE_ON) {
      Log.event("[FlashOnClash]");
      bladeState.change(BLADE_FLASH);
      reflashTime = calcReflashTime();
    }
  }

  if (displayTimer.tick()) {
    int sketcherMode = Sketcher::BLADE_ON_MODE;
# ifdef SABER_TWO_BUTTON
    if (bladeState.state() == BLADE_OFF) {
      sketcherMode = Sketcher::REST_MODE;
    }
# else
    if (bladeState.state() == BLADE_OFF) {
      switch (buttonMode.mode()) {
        case BUTTON_MODE_PALETTE: sketcherMode = Sketcher::PALETTE_MODE; break;
        case BUTTON_MODE_VOLUME:  sketcherMode = Sketcher::VOLUME_MODE;  break;
        default:                  sketcherMode = Sketcher::REST_MODE;    break;
      }
    }
# endif
# ifdef SABER_DISPLAY
    sketcher.Draw(&renderer, displayTimer.period(), sketcherMode, &uiRenderData);
    display.display(oledBuffer);
# endif 
# if SABER_CRYSTAL == SABER_DOTSTAR
#   ifdef SABER_DOTSTAR_UI
      dotstarUI.Draw(leds + SABER_DOTSTAR_UI, sketcherMode, &uiRenderData);
      dotstar.display();
#   endif    
# endif
  }

#if SABER_CRYSTAL == SABER_RGB_CRYSTAL
  const uint8_t* rgb = saberDB.bladeColor();
  analogWrite(PIN_CRYSTAL_R, rgb[0]);
  analogWrite(PIN_CRYSTAL_G, rgb[1]);
  analogWrite(PIN_CRYSTAL_B, rgb[2]);
#elif SABER_CRYSTAL == SABER_DOTSTAR
  #ifdef SABER_DOTSTAR_CRYSTAL
  {
    const uint8_t* rgb = saberDB.bladeColor();
    leds[SABER_DOTSTAR_CRYSTAL].red   = rgb[0];
    leds[SABER_DOTSTAR_CRYSTAL].green = rgb[1];
    leds[SABER_DOTSTAR_CRYSTAL].blue  = rgb[2];
    dotstar.display();
  }
  #endif
#endif
}

int32_t readVbat() {
#ifdef SABER_VOLTMETER
  int32_t analog = analogRead(PIN_VMETER);
  int32_t mV = analog * UVOLT_MULT / int32_t(1000);
  return mV;
#else
  return NOMINAL_VOLTAGE;
#endif
}

