#include <FastLED.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <LiquidCrystal_I2C.h>

#define LCD_CHARS   20
#define LCD_LINES   4
#define NUM_LEDS	147		
#define DATA_PIN	2
#define LED_TYPE    WS2812B

#define LCDPin	12
#define LightsPin 10

CRGB leds[NUM_LEDS];

ClickEncoder *encoder;	// pointer to encoder
uint8_t increment;		// encoder output stored here every loop

// set the LCD address to 0x27 for a 20 chars 4 line display
// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

#define numberOfModes 5
uint8_t numOptionsInMode[] = { 4, 3, 3 , 1, 2 };
uint8_t currentOption;
uint8_t currentMode = 4;
long timeOut = -1;
uint8_t dimBy = 0;
bool lightWasOn;

uint8_t lcdON;
uint8_t lightsOn;

const uint8_t MODE = 0;
const uint8_t HUE = 1;
const uint8_t SAT = 2;
const uint8_t BRIGHT = 3;
const uint8_t DELAY = 4;
const uint8_t DELTA = 5;

// settings 
uint8_t setting = currentMode;
// 0-mode, 1-hue, 2-saturation, 3-brightness, 4-interval, 5-optional
uint8_t settings[][6] =
{
	{ 0, 87, 255, 255, 26, 0 },	// confetti
	{ 1, 0, 255, 255, 30, 0 },	// solid
	{ 2, 0, 255, 180, 40, 2 },	// rainbow
	{ 3, 0, 255, 255, 49, 0 },	// pulse
	{ 0, 240, 255, 255, 26, 0 },	// confetti - red
	{ 0, 170, 255, 255, 26, 0 }	// confetti blue
};

void timerIsr() {
	encoder->service();
}

void setup()
{
	// initialize LEDS
	LEDS.setBrightness(255);
	FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

	pinMode(LCDPin, INPUT);
	pinMode(LightsPin, INPUT);

	// initialize encoder & timer
	encoder = new ClickEncoder(A1, A0, A2, 4);
	encoder->setAccelerationEnabled(true);
	Timer1.initialize(1000);
	Timer1.attachInterrupt(timerIsr);
	timeOut = millis(); // initilaze timout

	// initialize display
	lcd.begin(LCD_CHARS, LCD_LINES);
	lcd.backlight();
	lightWasOn = false;
}

void loop()
{
	lcdON = digitalRead(LCDPin);
	lightsOn = digitalRead(LightsPin);

	updateLcdPower(lcdON);

	updateBrightness(lightsOn);

	if (lcdON == HIGH) {
		checkEncoderInput();
	}

	ledUpdate(settings[setting]);

}

void updateLcdPower(int state)
{
	if (state == LOW) {
		lcd.off();
	}
	else {
		lcd.on();
	}
}

void updateBrightness(int lightState)
{
	if (lightState == LOW) {
		if (lightWasOn) {
			lightWasOn = false;
			dimBy = settings[setting][BRIGHT]; // needs more work
		}
		if (settings[setting][BRIGHT] - dimBy > 0) {
			dimBy += 1;
		}
	}
	else {
		if (dimBy > 0) {
			dimBy -= 1;
		}
	}
}

void checkEncoderInput()
{
	increment = encoder->getValue();

	if (increment != 0) {
		settings[currentMode][currentOption] += increment;
		updateDisplay();
	}

	ClickEncoder::Button b = encoder->getButton();

	if (b != ClickEncoder::Open) {
		switch (b) {
		case ClickEncoder::Clicked:
			currentOption = (currentOption + 1) % numOptionsInMode[currentMode];
			updateDisplay();
			break;
		case ClickEncoder::DoubleClicked:
			uint8_t previousBrightness = settings[setting][BRIGHT];
			currentOption = 0;
			currentMode = (currentMode + 1) % numberOfModes;
			setting = currentMode;
			if (lightsOn == HIGH) {
				dimBy = previousBrightness - settings[setting][BRIGHT];
			}
			else {
				dimBy = settings[setting][BRIGHT];
			}
			updateDisplay();
			break;
		case ClickEncoder::Held:
			break;
		}
	}
}

void updateDisplay()
{

}

void ledUpdate(uint8_t current[])
{
	switch (current[0]) {
		case 0:
			fadeToBlackBy(leds, NUM_LEDS, 2);//long strip used fives
			leds[random16(NUM_LEDS)] +=
				CHSV(current[HUE] + random8(64), current[SAT], current[BRIGHT] - dimBy);
			break;
		case 1:
			fill_solid(leds, NUM_LEDS, CHSV(current[HUE], current[SAT], current[BRIGHT] - dimBy));
			break;
		case 2:
			fill_rainbow(leds, NUM_LEDS, current[HUE], current[DELTA]);
			FastLED.setBrightness(current[BRIGHT] - dimBy);
			current[HUE] += 1;
			break;
		case 3:
			fill_solid(leds, NUM_LEDS, CHSV(current[HUE], current[SAT], current[BRIGHT] - dimBy));
			current[HUE] += 1;
			break;
	}
}

void adjustHue(uint8_t hue)
{
	lcd.setCursor(9, 2);
	lcd.print("COLOR     ");
	lcd.setCursor(9, 3);
	lcd.print("[");
	lcd.print(hue);
	lcd.print("]    ");
}

void adjustSaturation(uint8_t sat)
{
	lcd.setCursor(9, 2);
	lcd.print("SATURATION   ");
	lcd.setCursor(9, 3);
	lcd.print("[");
	lcd.print(sat);
	lcd.print("]    ");
}

void adjustBrightness(uint8_t bright)
{
	lcd.setCursor(9, 2);
	lcd.print("BRIGHTNESS     ");
	lcd.setCursor(9, 3);
	lcd.print("[");
	lcd.print(bright);
	lcd.print("]    ");
}

void adjustInterval(uint8_t interval)
{
	lcd.setCursor(9, 2);
	lcd.print("SPEED       ");
	lcd.setCursor(9, 3);
	lcd.print("[");
	lcd.print(interval);
	lcd.print("]    ");
}

void adjustDeltaHue(uint8_t deltaHue)
{
	lcd.setCursor(9, 2);
	lcd.print("DELTAHUE      ");
	lcd.setCursor(9, 3);
	lcd.print("[");
	lcd.print(deltaHue);
	lcd.print("]    ");
}