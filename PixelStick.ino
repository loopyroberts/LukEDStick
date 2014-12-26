#include <SPI.h>
#include <Adafruit_ILI9341/Adafruit_ILI9341.h>
#include <Adafruit_GFX/Adafruit_GFX.h>
#include <Sd/Sd.h>
#include <touch/ads7843.h>
#include <FastLED/FastLED.h>

#define CLOCK_PIN 13
#define DATA_PIN 15
#define NUM_LEDS 288

#define MAINMENU 0
#define PAINT 1
#define SETUPMENU 2
#define COLOURMENU 3
#define SIZEMENU 4

#define FILEMODE 0
#define RAINMODE 1
#define FULLRAINMODE 2
#define PAINTMODE 3

#define COLSMAX 3


//SdFat SD;
// Define the array of leds
CRGB leds[NUM_LEDS];

double _x0, _y0, _x1, _y1, _x1n, _y1n;
int k = 0;

uint8_t COLSMODE = 0;

uint16_t scale = 311;

int colour;

bool play = false;
bool playonce = false;

//int paintField[270][60];

int brightness = 32;
int xt, yt;
Point p;
volatile long intrTime;

int mode = 0;
int drawmode = 0;

unsigned char r, g, b;

byte cs = 12;
byte dc = 10;
byte mosi = 9;
byte sclk = 8;
byte rst = 11;
byte miso = 6;

String filename;// = String("poke.txt");

Adafruit_ILI9341 tft = Adafruit_ILI9341(cs, dc, mosi, sclk, rst, miso);

ADS7843 ts = ADS7843(4, 5, 3, 2, 1);

File imfile, root;

String names[20];
uint8_t namesIndx = 0;

uint8_t flag, speed, namesIndxMax;


uint32_t x, y, v_time, hue_time, hxy;

// Play with the values of the variables below and see what kinds of effects they
// have!  More octaves will make things slower.

// how many octaves to use for the brightness and hue functions
uint8_t octaves = 3;
uint8_t hue_octaves = 3;

// the 'distance' between points on the x and y axis
int xscale = 7771;
int yscale = 57771;

// the 'distance' between x/y points for the hue noise
int hue_scale = 10;

// how fast we move through time & hue noise
int time_speed = 1111;
int hue_speed = 31;

// adjust these values to move along the x or y axis between frames
int x_speed = 931;
int y_speed = 1711;


void setup()
{
	Serial.begin(9600);
	//while (!Serial) {
	//; // wait for serial port to connect. Needed for Leonardo only
	//}

	hxy = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
	x = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
	y = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
	v_time = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
	hue_time = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
	
	FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, GBR, DATA_RATE_MHZ(8)>(leds, NUM_LEDS);
	FastLED.setBrightness(brightness);
	speed = 5;
	tft.begin();
	tft.fillScreen(0);

	Serial.print("Initializing SD card...");

	pinMode(17, OUTPUT);
	pinMode(18, OUTPUT);
	pinMode(19, INPUT);
	pinMode(20, OUTPUT);

	if (!SD.begin(17, 18, 19, 20)) {
	//if(!SD.begin(17)) {
		Serial.println("initialization failed!");
		return;
	}
	Serial.println("initialization done.");
	
	root = SD.open("/");
	
	printDirectory(root, 0);
	filename = names[0];
	//for (uint8_t i = 0; i < 20; i++)
	//{
	//	Serial.print(i);
	//	Serial.println(names[i]);
	//}
	Serial.println("done!");
	ts.begin();
	
	drawMenu();
	
	colour = ILI9341_GREEN;
	
	attachInterrupt(1, processTouch, FALLING);
}

void loop()
{
	switch (drawmode)
	{
	case FILEMODE:
		if (!(play || playonce))
		{
			FastLED.clear();
			FastLED.show();
		}
		else
		{
			playonce = false;
			uint8_t *buf;
			char namebuf[filename.length() + 1];
			//Serial.println(filename);
			filename.toCharArray(namebuf, sizeof(namebuf));

			//Serial.println(namebuf);
			imfile = SD.open(namebuf, FILE_READ);

			if (imfile)
			{
				while (imfile.available())
				{
					for (int i = 0; i < NUM_LEDS; i++)
					{
						leds[i].red = imfile.read();
						leds[i].green = imfile.read();
						leds[i].blue = imfile.read();
						//Serial.print(leds[i].red);
						//Serial.print(leds[i].green);
						//Serial.print(leds[i].blue);
					}
					//Serial.println(';');
					FastLED.show();
					FastLED.show();
					delay(speed);
				}
			}
			imfile.close();
			//Serial.print("X: ");
			//Serial.println(ts.getpos(&flag).x);
		}
		break;
	case RAINMODE:
		//fill_rainbow(leds, NUM_LEDS, k++,3);
		//fillnoise8();
		/*for (int i = 0; i < NUM_LEDS; i++)
		{
			hsv2rgb(k++%360, 255, 255, &r, &g, &b, 255);
			leds[i] = CRGB(r, g, b);
		}

		*/
		switch (COLSMODE)
		{
		case 0:
			fill_rainbow(leds, NUM_LEDS, k++, 3);
			break;
		case 1:
			fill_solid(leds, NUM_LEDS, CHSV(k++, 255, 255));
			break;
		case 2:
			fill_2dnoise16(leds, NUM_LEDS, 1, false,
				octaves, x, xscale, y, yscale, v_time,
				hue_octaves, hxy, hue_scale, hxy, hue_scale, hue_time, false);

			x += x_speed;
			y += y_speed;
			v_time += time_speed;
			hue_time += hue_speed;
			break;
		}
		//Fire2012();
		FastLED.show();
		delay(speed);
		break;
		
	case FULLRAINMODE:
		/*hsv2rgb(k++%360, 255, 255, &r, &g, &b, 255);
		for (int i = 0; i < NUM_LEDS; i++)
		{
			
			leds[i] = CRGB(r, g, b);
		}
		*/
		fill_solid(leds, NUM_LEDS, CHSV(k++, 255, 255));
		//fill_rainbow(leds, NUM_LEDS, k++);
		FastLED.show();
		delay(speed);
		break;
	}
}
void hsv2rgb(int hue, int sat, int val, byte * r, byte * g, byte * b, byte maxBrightness) {
	unsigned int H_accent = hue / 60;
	unsigned int bottom = ((255 - sat) * val) >> 8;
	unsigned int top = val;
	unsigned char rising = ((top - bottom)  *(hue % 60)) / 60 + bottom;
	unsigned char falling = ((top - bottom)  *(60 - hue % 60)) / 60 + bottom;

	switch (H_accent) {
	case 0:
		*r = top;
		*g = rising;
		*b = bottom;
		break;

	case 1:
		*r = falling;
		*g = top;
		*b = bottom;
		break;

	case 2:
		*r = bottom;
		*g = top;
		*b = rising;
		break;

	case 3:
		*r = bottom;
		*g = falling;
		*b = top;
		break;

	case 4:
		*r = rising;
		*g = bottom;
		*b = top;
		break;

	case 5:
		*r = top;
		*g = bottom;
		*b = falling;
		break;
	}
	// Scale values to maxBrightness
	*r = *r * maxBrightness / 255;
	*g = *g * maxBrightness / 255;
	*b = *b * maxBrightness / 255;
}

void printDirectory(File dir, int numTabs) {
	// Begin at the start of the directory
	dir.rewindDirectory();
	uint8_t i = 0;
	while (true) {
		File entry = dir.openNextFile();
		if (!entry) {
			// no more files
			//Serial.println("**nomorefiles**");
			break;
		}
		// Print the 8.3 name
		//Serial.print(entry.name());
		// Recurse for directories, otherwise print the file size
		if (!entry.isDirectory()) {
			String ei = entry.name();
			if (ei.endsWith(".PIX")){
				names[i] = entry.name();
				i++;
			}
		}

		entry.close();
	}
	namesIndxMax = i;
}

void processTouch()
{
	switch (mode)
	{
	case MAINMENU:
		mainmenu();
		break;
	case PAINT:
		painter();
		break;
	case SETUPMENU:
		setupmenu();
		break;
	case COLOURMENU:
		colourmenu();
		break;
	}
}
void mainmenu()
{
	if (millis() - intrTime > 100)
	{
		intrTime = millis();
		p = ts.getpos(&flag);
		xt = 240 - (int)(p.x*0.076 - 25);
		yt = (int)(p.y*0.095 - 40);
		if (flag)
		{
			tft.setTextSize(3);
			//tft.fillRect(15, 150, 100, 30, 0);
			//tft.setCursor(15, 150);
			//tft.print(p.x);
			//tft.fillRect(15, 250, 100, 30, 0);
			//tft.setCursor(15, 250);
			//tft.print(p.y);
			if (xt > 120 && yt < 103)
			{
				mode = PAINT;
				drawMenu();
				
			}
			if (xt<120 && yt<103)
			{
				mode = SETUPMENU;
				drawMenu();
			}
			else if (xt<120 && yt>103 && yt<206)
			{
				play = false;
				tft.fillRect(121, 240, 118, 112, 0);
				tft.setCursor(135, 250);
				tft.println("Play");
				COLSMODE++;
				if (COLSMODE >= COLSMAX)
				{
					COLSMODE = 0;
				}
				drawmode = RAINMODE;
			}
			else if (xt<120 && yt>206)
			{
				play = false;
				tft.fillRect(121, 240, 118, 112, 0);
				tft.setCursor(135, 250);
				tft.println("Play");
				COLSMODE--;
				if (COLSMODE == 255)
				{
					COLSMODE = COLSMAX - 1;
				}
				drawmode = RAINMODE;
			}
			else if (xt>120 && yt>103 && yt<206) // Play once
			{
				drawmode = FILEMODE;
				playonce = true;
			}
			else if (xt > 120 && yt > 206)	// Play/Stop
			{
				drawmode = FILEMODE;
				if (play)
				{
					play = false;
					tft.fillRect(121, 240, 118, 112, 0);
					tft.setCursor(135, 250);
					tft.println("Play");
				}
				else
				{
					play = true;
					tft.fillRect(121, 240, 118, 112, 0);
					tft.setCursor(135, 250);
					tft.println("Stop");
				}
			}
		}
	}
}
void painter()
{
	intrTime = millis();
	p = ts.getpos(&flag);
	if (flag)
	{
		if (yt > 25)
		{
			//tft.drawPixel(xt, yt, ILI9341_GREEN);
			tft.fillRect(xt, yt, 2, 2, colour);
		}
		if (yt < 25 && xt>120)
		{
			mode = MAINMENU;
			drawMenu();
			//Write data to LEDs
		}
		if (yt > 295 && xt<120)
		{
			mode = COLOURMENU;
			drawMenu();
			//Write data to LEDs
		}
		if (yt > 295 && xt>120)
		{
			mode = SIZEMENU;
			drawMenu();
			//Write data to LEDs
		}
	}
}
void setupmenu()
{
	if (millis() - intrTime > 100)
	{
		intrTime = millis();
		p = ts.getpos(&flag);
		xt = 240 - (int)(p.x*0.076 - 25);
		yt = (int)(p.y*0.095 - 40);
		if (flag)
		{
			if (yt < 30 && xt>120)
			{
				mode = MAINMENU;
				drawMenu();
				//Write data to LEDs
			}
			if (xt>120 && yt<206 && yt>106)
			{
				if (speed<255) speed++;
				tft.fillRect(100, 160, 40, 25, 0);
				tft.setTextSize(3);
				tft.setCursor(100, 160);
				tft.println(speed);
			}
			else if (xt<120 && yt<206&&yt>106)
			{
				if (speed>0) speed--;
				tft.fillRect(100, 160, 40, 25, 0);
				tft.setTextSize(3);
				tft.setCursor(100, 160);
				tft.println(speed);
			}
			if (xt<120 && yt>206)
			{
				brightness += 32;
				//brightness = FastLED.getBrightness();
				if (brightness >= 255)
				{
					FastLED.setBrightness(255);
					brightness = 256;
				}
				else
				{
					FastLED.setBrightness(brightness);
				}
				tft.fillRect(90, 260, 70, 25, 0);
				tft.setTextSize(3);
				tft.setCursor(90, 260);
				tft.println(brightness);
			}
			else if (xt>120 && yt>206)
			{
				brightness -= 32;
				//brightness = FastLED.getBrightness();
				if (brightness <= 0)
				{
					FastLED.setBrightness(0);
					brightness = 0;
				}
				else
				{
					FastLED.setBrightness(brightness);
				}
				tft.fillRect(90, 260, 70, 25, 0);
				tft.setTextSize(3);
				tft.setCursor(90, 260);
				tft.println(brightness);
			}
			if (xt<120 && yt<103 && yt>25)
			{
				namesIndx--;
				if (namesIndx == 255)
				{
					namesIndx = namesIndxMax - 1;
				}
				//Serial.println(filename);
				filename = names[namesIndx];
				//Serial.println(filename);
				tft.fillRect(80, 93, 80, 10, 0);
				tft.setTextSize(1);
				tft.setCursor(80, 93);
				tft.println(filename);
				
				//drawMenu();
			}
			else if (xt>120 && yt<103 && yt>30)
			{
				namesIndx++;
				if (namesIndx >= namesIndxMax)
				{
					namesIndx = 0;
				}
				//Serial.println(filename);
				filename = names[namesIndx];
				//Serial.println(filename);
				
				tft.fillRect(80, 93, 80, 10, 0);
				tft.setTextSize(1);
				tft.setCursor(80, 93);
				tft.println(filename);
				//drawMenu();
			}
		}
	}
}




void drawMenu()
{
	switch (mode)
	{
	case MAINMENU:
		tft.fillScreen(0);
		tft.drawLine(120, 0, 120, 320, 65535);
		tft.drawLine(0, 106, 240, 106, 65535);
		tft.drawLine(0, 212, 240, 212, 65535);
		tft.setTextSize(3);
		tft.setCursor(15, 40);
		tft.println("Setup");
		tft.setCursor(135, 40);
		tft.println("Paint");
		tft.setCursor(5, 150);
		tft.println("Cols +");
		tft.setCursor(5, 250);
		tft.println("Cols -");
		tft.setCursor(135, 140);
		tft.println("Play");
		tft.setCursor(135, 170);
		tft.println("Once");
		if (!play)
		{
			tft.setCursor(135, 250);
			tft.println("Play");
		}
		else
		{
			tft.setCursor(135, 250);
			tft.println("Stop");
		}
		break;

	case PAINT:
		tft.fillScreen(0);
		tft.setTextSize(2);
		tft.setCursor(5, 5);
		tft.print("Paint");
		tft.setCursor(155, 5);
		tft.print("Done");
		tft.drawLine(0, 25, 240, 25, 65535);
		tft.drawLine(120, 0, 120, 25, 65535);
		tft.drawLine(0, 295, 240, 295, 65535);
		tft.setCursor(5, 300);
		tft.print("Colour");
		tft.setCursor(155, 300);
		tft.print("Size");
		break;

	case COLOURMENU:
		tft.fillScreen(0);
		tft.setTextSize(2);
		tft.setCursor(5, 5);
		tft.print("Colour");
		tft.setCursor(155, 5);
		tft.print("Done");
		tft.drawLine(0, 24, 240, 24, 65535);
		tft.drawLine(120, 0, 120, 24, 65535);

		for (int i = 0; i < 240; i++)
		{
			//for (int j = 0; j < 270; j++)
			//{
				hsv2rgb((int)i * 360 / 240.0, 255, 255, &r, &g, &b, 255);
				tft.drawLine(i, 60, i, 180, tft.color565(r, g, b));
			//}
		}
		tft.fillRoundRect(15, 220, 210, 60, 10, colour);
		break;
	case SETUPMENU:
		tft.fillScreen(0);
		tft.setTextSize(2);
		tft.setCursor(155, 5);
		tft.print("Done");
		tft.setCursor(25, 5);
		tft.print("Setup");
		tft.drawLine(0, 25, 240, 25, 65535);
		tft.drawLine(120, 0, 120, 25, 65535);
		//tft.drawLine(0, 295, 240, 295, 65535);
		tft.setTextSize(1);
		//tft.setCursor(155, 110);
		//tft.println("Brightness");
		tft.setCursor(90, 235);
		tft.println("Brightness");
		tft.setTextSize(5);
		tft.setCursor(40, 250);
		tft.println("+");
		tft.setCursor(170, 250);
		tft.println("-");
		tft.setTextSize(3);
		tft.setCursor(90, 260);
		tft.println(brightness);
		tft.setTextSize(1);
		tft.setCursor(100, 138);
		tft.println("Speed");
		tft.setTextSize(5);
		tft.setCursor(40, 150);
		tft.println("+");
		tft.setCursor(170, 150);
		tft.println("-");
		tft.setTextSize(3);
		tft.setCursor(100, 160);
		tft.println(speed);
		tft.setTextSize(1);
		tft.setCursor(100, 50);
		tft.println("File");
		tft.setCursor(80, 93);
		tft.println(filename);
		tft.setTextSize(5);
		tft.setCursor(40, 80);
		tft.println("<");
		tft.setCursor(170, 80);
		tft.println(">");

		break;
	}
}
void colourmenu()
{
	if (millis() - intrTime > 10)
	{
		intrTime = millis();
		p = ts.getpos(&flag);
		if (flag)
		{
			xt = 240 - (int)(p.x*0.075 - 30);
			yt = (int)(p.y*0.1 - 40);
			if (yt > 60 && yt < 180)
			{
				hsv2rgb((byte)xt * 360 / 240.0, 255, 255, &r, &g, &b, 255);
				colour = tft.color565(r, g, b);
				tft.fillRoundRect(15, 220, 210, 60, 10, colour);
			}
			if (xt>120 && yt < 25)
			{
				mode = PAINT;
				drawMenu();
			}
		}
	}
}
void Fire2012()
{
	// Array of temperature readings at each simulation cell
	static byte heat[NUM_LEDS];

	// Step 1.  Cool down every cell a little
	for (int i = 0; i < NUM_LEDS; i++) {
		heat[i] = qsub8(heat[i], random8(0, ((55 * 10) / NUM_LEDS) + 2));
	}

	// Step 2.  Heat from each cell drifts 'up' and diffuses a little
	for (int k = NUM_LEDS - 1; k >= 2; k--) {
		heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
	}

	// Step 3.  Randomly ignite new 'sparks' of heat near the bottom
	if (random8() < 120) {
		int y = random8(7);
		heat[y] = qadd8(heat[y], random8(160, 255));
	}

	// Step 4.  Map from heat cells to LED colors
	for (int j = 0; j < NUM_LEDS; j++) {
		leds[j] = HeatColor(heat[j]);
	}
}