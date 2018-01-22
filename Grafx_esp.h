// https://github.com/PaulStoffregen/Grafx_esp
// http://forum.pjrc.com/threads/26305-Highly-optimized-ILI9341-(320x240-TFT-color-display)-library
// Warning this is Kurt's hacked up version using my SPIN library to allow different SPI busses
// Also allows use on only one valid CS pin on that buss, which must be used for DC

/***************************************************
  This is our library for the Adafruit  ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// <SoftEgg>

//Additional graphics routines by Tim Trzepacz, SoftEgg LLC added December 2015
//(And then accidentally deleted and rewritten March 2016. Oops!)
//Gradient support
//----------------
//		fillRectVGradient	- fills area with vertical gradient
//		fillRectHGradient	- fills area with horizontal gradient
//		fillScreenVGradient - fills screen with vertical gradient
// 	fillScreenHGradient - fills screen with horizontal gradient

//Additional Color Support
//------------------------
//		color565toRGB		- converts 565 format 16 bit color to RGB
//		color565toRGB14		- converts 16 bit 565 format color to 14 bit RGB (2 bits clear for math and sign)
//		RGB14tocolor565		- converts 14 bit RGB back to 16 bit 565 format color

//Low Memory Bitmap Support
//-------------------------
// writeRect8BPP - 	write 8 bit per pixel paletted bitmap
// writeRect4BPP - 	write 4 bit per pixel paletted bitmap
// writeRect2BPP - 	write 2 bit per pixel paletted bitmap
// writeRect1BPP - 	write 1 bit per pixel paletted bitmap

//String Pixel Length support
//---------------------------
//		strPixelLen			- gets pixel length of given ASCII string

// <\SoftEgg>
// Also some of this comes from the DMA version of the library...

/* Grafx_t3DMA library code is placed under the MIT license
 * Copyright (c) 2016 Frank Bösing
 *
*/

#ifndef _Grafx_esp_H_
#define _Grafx_esp_H_
#define  Grafx_USE_DMAMEM


#define ENABLE_Grafx_FRAMEBUFFER

// Allow way to override using SPI

#ifdef __cplusplus
#include "Arduino.h"
#include <SPI.h>
#endif

#include "Fonts/fonts.h"

#define Grafx_TFTWIDTH  240
#define Grafx_TFTHEIGHT 320

#define Grafx_NOP     0x00
#define Grafx_SWRESET 0x01
#define Grafx_RDDID   0x04
#define Grafx_RDDST   0x09

#define Grafx_SLPIN   0x10
#define Grafx_SLPOUT  0x11
#define Grafx_PTLON   0x12
#define Grafx_NORON   0x13

#define Grafx_RDMODE  0x0A
#define Grafx_RDMADCTL  0x0B
#define Grafx_RDPIXFMT  0x0C
#define Grafx_RDIMGFMT  0x0D
#define Grafx_RDSELFDIAG  0x0F

#define Grafx_INVOFF  0x20
#define Grafx_INVON   0x21
#define Grafx_GAMMASET 0x26
#define Grafx_DISPOFF 0x28
#define Grafx_DISPON  0x29

#define Grafx_CASET   0x2A
#define Grafx_PASET   0x2B
#define Grafx_RAMWR   0x2C
#define Grafx_RAMRD   0x2E

#define Grafx_PTLAR    0x30
#define Grafx_MADCTL   0x36
#define Grafx_VSCRSADD 0x37
#define Grafx_PIXFMT   0x3A

#define Grafx_FRMCTR1 0xB1
#define Grafx_FRMCTR2 0xB2
#define Grafx_FRMCTR3 0xB3
#define Grafx_INVCTR  0xB4
#define Grafx_DFUNCTR 0xB6

#define Grafx_PWCTR1  0xC0
#define Grafx_PWCTR2  0xC1
#define Grafx_PWCTR3  0xC2
#define Grafx_PWCTR4  0xC3
#define Grafx_PWCTR5  0xC4
#define Grafx_VMCTR1  0xC5
#define Grafx_VMCTR2  0xC7

#define Grafx_RDID1   0xDA
#define Grafx_RDID2   0xDB
#define Grafx_RDID3   0xDC
#define Grafx_RDID4   0xDD

#define Grafx_GMCTRP1 0xE0
#define Grafx_GMCTRN1 0xE1
/*
#define Grafx_PWCTR6  0xFC

*/

// Color definitions

#define TRANSPARENT_INDEX 0

#define BLANK         0x00   
#define BLACK         0x0000      /*   0,   0,   0 */
#define WHITE         0xFFFF      /* 255, 255, 255 */
#define NAVY          0x000F      /*   0,   0, 128 */
#define BROWN         0xA145
#define LIGHTBROWN    0XF7BB
#define BEIGE         0xF52C
#define DARKGREEN     0x03E0      /*   0, 128,   0 */
#define DARKCYAN      0x03EF      /*   0, 128, 128 */
#define MAROON        0x7800      /* 128,   0,   0 */
#define PURPLE        0x780F      /* 128,   0, 128 */
#define OLIVE         0x7BE0      /* 128, 128,   0 */
#define GREY          0x8410
#define LIGHTGREY     0xC618      /* 192, 192, 192 */
#define DARKGREY      0x7BEF      /* 128, 128, 128 */
#define BLUE          0x001F      /*   0,   0, 255 */
#define GREEN         0x07E0      /*   0, 255,   0 */
#define CYAN          0x07FF      /*   0, 255, 255 */
#define RED           0xF800      /* 255,   0,   0 */
#define MAGENTA       0xF81F      /* 255,   0, 255 */
#define YELLOW        0xFFE0      /* 255, 255,   0 */
#define ORANGE        0xFD20      /* 255, 165,   0 */
#define GREENYELLOW   0xAFE5      /* 173, 255,  47 */
#define PINK          0xFFD6D6    /* 255, 214, 214 */    

#define CL(_r,_g,_b) ((((_r)&0xF8)<<8)|(((_g)&0xFC)<<3)|((_b)>>3))

#define sint16_t int16_t

typedef struct {
	const unsigned char *index;
	const unsigned char *unicode;
	const unsigned char *data;
	unsigned char version;
	unsigned char reserved;
	unsigned char index1_first;
	unsigned char index1_last;
	unsigned char index2_first;
	unsigned char index2_last;
	unsigned char bits_index;
	unsigned char bits_width;
	unsigned char bits_height;
	unsigned char bits_xoffset;
	unsigned char bits_yoffset;
	unsigned char bits_delta;
	unsigned char line_space;
	unsigned char cap_height;
} ILI9341_t3_font_t;

struct Rect{
	int16_t x;
	int16_t y;
	uint8_t width;
	uint8_t height;
};

#define Grafx_DMA_INIT	0x01 	// We have init the Dma settings
#define Grafx_DMA_CONT	0x02 	// continuous mode
#define Grafx_DMA_ACTIVE  0x80    // Is currently active

#ifdef __cplusplus
// At all other speeds, ILI9241_KINETISK__pspi->beginTransaction() will use the fastest available clock
#define Grafx_SPICLOCK 30000000
#define Grafx_SPICLOCK_READ 20000000

//#define swap(a, b) { int16_t t = a; a = b; b = t; }
//#define swap(a, b) { typeof(a) t = a; a = b; b = t; }
inline void swap(int16_t &a, int16_t &b) { int16_t t = a; a = b; b = t; }

//#include "glcdfont.c"

class Grafx_esp : public Print
{
  public:

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
//////////////-----------------------constructor-----------------------------//////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

	Grafx_esp(uint8_t _CS, uint8_t _DC, uint8_t _RST = 255, uint8_t _MOSI=11, uint8_t _SCLK=13, uint8_t _MISO=12);
	
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
//////////////------------------------Initialisation-------------------------//////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

	void        begin(void);
  	void        sleep(bool enable);		
	void        pushColor(uint16_t color);
	void        fillScreen(uint16_t color);
    void        drawPixel(int16_t x, int16_t y, uint16_t color);
	void        drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
	void        invertDisplay(boolean i);
	void        writePixels(uint16_t * colors, uint32_t len);
	void        drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
	void        drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
	void        fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
	void        drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
	void        fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2);
	void        fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2);
	void        fillScreenVGradient(uint16_t color1, uint16_t color2);
	void        fillScreenHGradient(uint16_t color1, uint16_t color2);

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
//////////////----------------------------Geometry----------------------------//////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
	void        drawArc(int16_t x, int16_t y, int16_t r, int16_t rs, int16_t re, uint16_t color);//new
	void        ellipse(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);//new
	void        drawBezier(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);//new
	void        drawQuad(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color);
	void        fillQuad(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color);
	void        drawPolygon(int16_t cx, int16_t cy, uint8_t sides, int16_t diameter, float rot, uint16_t color);
    void        drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
	void        drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
	void        fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
	void        fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);
	void        drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
	void        fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
	void        drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
	void        fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);


////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
//////////////-----------------------------Bitmap-----------------------------//////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
    void        drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
    void        drawBitmap1(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
	void        drawBitmap2(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
	void        drawBitmap3(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
	void        drawBitmap4(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
	
    void        drawBitmapTM(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *bitmap, uint16_t dx, uint16_t dy, uint16_t dw, uint16_t dh,  uint16_t color);
	boolean     getBitmapPixel(const uint8_t* bitmap, uint16_t x, uint16_t y);

	void        drawTilemap(uint16_t x, uint16_t y, const uint16_t *tilemap, const uint16_t **spritesheet, const uint16_t * palette);
	void        drawTilemap(uint16_t x, uint16_t y, const uint16_t *tilemap, const uint16_t **spritesheet, uint16_t dx, uint16_t dy, uint16_t dw, uint16_t dh, const uint16_t * palette);

	typedef struct {       //line 171 "Public Variables   - ADD by Summoner123
		uint16_t x;                    //X coordinate                 - ADD by Summoner123
		uint16_t y;                    //Y coordinate                 - ADD by Summoner123
		const uint16_t *spritecol;    //Sprite of object             - ADD by Summoner123
	}object;
	object solid[400];         // Matriz were saved a Sprite, X and Y cordinates of all tiles on the screen - ADD by Summoner123

	uint16_t numcolision = 4;     //count of solid objects indacat how many tiles drawed on the screen - ADD by Summoner123

	bool flagcollision = true;
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
//////////////-----------------------------Collision--------------------------//////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

	boolean collidePointRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t w, int16_t h);
	boolean collideRectRect(int16_t x1, int16_t y1, int16_t w1, int16_t h1, int16_t x2, int16_t y2, int16_t w2, int16_t h2);
	boolean collideBitmapBitmap(int16_t x1, int16_t y1, const uint8_t* b1, int16_t x2, int16_t y2, const uint8_t* b2);


////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
//////////////---------------------------Text---------------------------------//////////////
////////////////////////////////////////////////////////////////////////////////////////////
	void        drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
	void        setTextColor(uint16_t c);
	void        setTextColor(uint16_t c, uint16_t bg);
	void        setTextSize(uint8_t s);
	uint8_t     getTextSize();
	void        setTextWrap(boolean w);
	boolean     getTextWrap();
    void        setRotation(uint8_t r);
	void        setFont(uint8_t f);
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////POPUP/TITLESCREEN///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
    void Popup(const __FlashStringHelper* text, uint8_t s, int16_t x, int16_t y);
    void Popup2(const __FlashStringHelper* text, uint8_t s, int16_t x, int16_t y);
    void Popup3(const __FlashStringHelper* text, uint8_t s, int16_t x, int16_t y);
    const __FlashStringHelper* popupText;
    uint8_t popupTimeLeft;
    
//    void titleScreen(const __FlashStringHelper* name, const uint8_t *logo);
//    void titleScreen(const __FlashStringHelper* name);
//	  void titleScreen(const uint8_t* logo);
//    void titleScreen();

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////-------------------------Cursor----------------------------------//////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
	void        setScroll(uint16_t offset);
	void        setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void        setCursor(int16_t x, int16_t y);
	void        getCursor(int16_t *x, int16_t *y);

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////------------------------------Color-------------------------------//////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 8-bit (each) R,G,B, get back 16-bit packed color
	static      uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);}

	//color565toRGB		- converts 565 format 16 bit color to RGB
	static void color565toRGB(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
		r = (color>>8)&0x00F8;
		g = (color>>3)&0x00FC;
		b = (color<<3)&0x00F8;
	}
	
	//color565toRGB14		- converts 16 bit 565 format color to 14 bit RGB (2 bits clear for math and sign)
	//returns 00rrrrr000000000,00gggggg00000000,00bbbbb000000000
	//thus not overloading sign, and allowing up to double for additions for fixed point delta
	static void color565toRGB14(uint16_t color, int16_t &r, int16_t &g, int16_t &b) {
		r = (color>>2)&0x3E00;
		g = (color<<3)&0x3F00;
		b = (color<<9)&0x3E00;
	}
	
	//RGB14tocolor565		- converts 14 bit RGB back to 16 bit 565 format color
	static uint16_t RGB14tocolor565(int16_t r, int16_t g, int16_t b)
	{
		return (((r & 0x3E00) << 2) | ((g & 0x3F00) >>3) | ((b & 0x3E00) >> 9));
	}
	
	//uint8_t readdata(void);
	uint8_t readcommand8(uint8_t reg, uint8_t index = 0);

	// Added functions to read pixel data...
	uint16_t readPixel(int16_t x, int16_t y);
	void readRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pcolors);
	void writeRect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors);

	// writeRect8BPP - 	write 8 bit per pixel paletted bitmap
	//					bitmap data in array at pixels, one byte per pixel
	//					color palette data in array at palette
	void writeRect8BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels, const uint16_t * palette );

	// writeRect4BPP - 	write 4 bit per pixel paletted bitmap
	//					bitmap data in array at pixels, 4 bits per pixel
	//					color palette data in array at palette
	//					width must be at least 2 pixels
	void writeRect4BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels, const uint16_t * palette );
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
    void writeRect4BPPtm(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels, uint16_t dx, uint16_t dy, uint16_t dw, uint16_t dh, const uint16_t * palette );

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
	// writeRect2BPP - 	write 2 bit per pixel paletted bitmap
	//					bitmap data in array at pixels, 4 bits per pixel
	//					color palette data in array at palette
	//					width must be at least 4 pixels
	void writeRect2BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels, const uint16_t * palette );
	
	// writeRect1BPP - 	write 1 bit per pixel paletted bitmap
	//					bitmap data in array at pixels, 4 bits per pixel
	//					color palette data in array at palette
	//					width must be at least 8 pixels
	void writeRect1BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pixels, const uint16_t * palette );

	// writeRectNBPP - 	write N(1, 2, 4, 8) bit per pixel paletted bitmap
	//					bitmap data in array at pixels
	//  Currently writeRect1BPP, writeRect2BPP, writeRect4BPP use this to do all of the work. 
	// 
	void writeRectNBPP(int16_t x, int16_t y, int16_t w, int16_t h,  uint8_t bits_per_pixel, 
		const uint16_t *pixels, const uint16_t * palette );

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
//////////////---------------------Frame buffer helpers-----------------------//////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

	// setOrigin sets an offset in display pixels where drawing to (0,0) will appear
	// for example: setOrigin(10,10); drawPixel(5,5); will cause a pixel to be drawn at hardware pixel (15,15)
	void setOrigin(int16_t x = 0, int16_t y = 0) { 
		_originx = x; _originy = y; 
		//if (Serial) Serial.printf("Set Origin %d %d\n", x, y);
		updateDisplayClip();
	}
	void getOrigin(int16_t* x, int16_t* y) { *x = _originx; *y = _originy; }

	// setClipRect() sets a clipping rectangle (relative to any set origin) for drawing to be limited to.
	// Drawing is also restricted to the bounds of the display

	void setClipRect(int16_t x1, int16_t y1, int16_t w, int16_t h) 
		{ _clipx1 = x1; _clipy1 = y1; _clipx2 = x1+w; _clipy2 = y1+h; 
			//if (Serial) Serial.printf("Set clip Rect %d %d %d %d\n", x1, y1, w, h);
			updateDisplayClip();
		}
	void setClipRect() {
			 _clipx1 = 0; _clipy1 = 0; _clipx2 = _width; _clipy2 = _height; 
			//if (Serial) Serial.printf("clear clip Rect\n");
			 updateDisplayClip(); 
		}

	virtual   size_t write(uint8_t);
	int16_t   width(void)  { return _width; }
	int16_t   height(void) { return _height; }
	uint8_t   getRotation(void);
	int16_t   getCursorX(void) const { return cursor_x; }
	int16_t   getCursorY(void) const { return cursor_y; }
	void      setFont(const ILI9341_t3_font_t &f) { font = &f; }
	void      setFont() { font = NULL; }
	void      setFontAdafruit(void) { font = NULL; }
	void      drawFontChar(unsigned int c);
	int16_t   strPixelLen(char * str);

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//////////////-------------------------------Buffer------------------------------//////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

	// added support to use optional Frame buffer
	uint8_t useFrameBuffer(boolean b);		// use the frame buffer?  First call will allocate
	void	freeFrameBuffer(void);			// explicit call to release the buffer
	void	updateScreen(void);				// call to say update the screen now. 
 
	boolean persistence; //disable clean() at each frame if true //Duhjoker
	boolean updateAll();
	uint8_t startMenuTimer;
	uint32_t frameCount;
	void setFrameRate(uint8_t fps);

	uint8_t getCpuLoad();
	uint16_t getFreeRam();
	uint16_t frameDurationMicros;
	uint32_t frameStartMicros, frameEndMicros;

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
//////////////------------------------------Misc----------------------------------//////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

protected:

    void      plot8points(uint16_t cx, uint16_t cy, uint16_t x, uint16_t y, uint16_t color);
    void      plot4points(uint16_t cx, uint16_t cy, uint16_t x, uint16_t y, uint16_t color);
    int       i_sin(int x);
    int       i_cos(int x);
    int8_t    fontKern;
    const unsigned char *fontData;

	uint8_t timePerFrame;
	uint32_t nextFrameMillis;

	int16_t   _width, _height; // Display w/h as modified by current rotation
	int16_t   cursor_x, cursor_y;
	uint8_t   fontWidth, fontHeight, fontStart, fontLength;
	int16_t   _clipx1, _clipy1, _clipx2, _clipy2;
	int16_t   _originx, _originy;
	int16_t   _displayclipx1, _displayclipy1, _displayclipx2, _displayclipy2;
	bool      _invisible = false; 
	bool      _standard = true; // no bounding rectangle or origin set. 

	inline void updateDisplayClip() {
		_displayclipx1 = max(0,min(_clipx1+_originx,width()));
		_displayclipx2 = max(0,min(_clipx2+_originx,width()));

		_displayclipy1 = max(0,min(_clipy1+_originy,height()));
		_displayclipy2 = max(0,min(_clipy2+_originy,height()));
		_invisible = (_displayclipx1 == _displayclipx2 || _displayclipy1 == _displayclipy2);
		_standard =  (_displayclipx1 == 0) && (_displayclipx2 == _width) && (_displayclipy1 == 0) && (_displayclipy2 == _height);
		if (Serial) {
			//Serial.printf("UDC (%d %d)-(%d %d) %d %d\n", _displayclipx1, _displayclipy1, _displayclipx2, 
			//	_displayclipy2, _invisible, _standard);

		}
	}

	uint16_t textcolor, textbgcolor;
	uint8_t textsize, rotation;
	boolean wrap; // If set, 'wrap' text at right edge of display
	const ILI9341_t3_font_t *font;

  	uint8_t  _rst;
  	uint8_t _cs, _dc;
	uint8_t pcs_data, pcs_command;
	uint8_t _miso, _mosi, _sclk;
	// add support to allow only one hardware CS (used for dc)
    uint32_t _cspinmask;
    volatile uint32_t *_csport;
    volatile uint32_t  *_dcport;
    uint32_t _dcpinmask;
	#ifdef ENABLE_Grafx_FRAMEBUFFER
    // Add support for optional frame buffer
    uint16_t	*_pfbtft;						// Optional Frame buffer 
    uint16_t	_fbtft1_lines;					// How many lines of the data is held in first allocation
    uint16_t	*_pfbtft2;						// ON ESP32, may not be able to allocate memory in one chunk.
    uint8_t		_use_fbtft;						// Are we in frame buffer mode?

    #endif

	void setAddr(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
	  __attribute__((always_inline)) {
		writecommand_cont(Grafx_CASET); // Column addr set
		writedata16_cont(x0);   // XSTART
		writedata16_cont(x1);   // XEND
		writecommand_cont(Grafx_PASET); // Row addr set
		writedata16_cont(y0);   // YSTART
		writedata16_cont(y1);   // YEND
	}
	void beginSPITransaction(uint32_t clock = Grafx_SPICLOCK) __attribute__((always_inline)) {
		SPI.beginTransaction(SPISettings(clock, MSBFIRST, SPI_MODE0));
		if (_csport)
			*_csport  &= ~_cspinmask;
	}
	void endSPITransaction() __attribute__((always_inline)) {
		if (_csport)
			*_csport |= _cspinmask;
		SPI.endTransaction();
	}

/// ESP STUFF
	uint16_t *mapYtoFBPtr(uint16_t y) __attribute__((always_inline)) {
		if (y < _fbtft1_lines)
			return &_pfbtft[y*_width];
		else 
			return &_pfbtft2[(y-_fbtft1_lines)*_width];
	}
	uint16_t FBmapColor(uint16_t color) __attribute__((always_inline)) {
		return ((color & 0xff) << 8) | ((color >> 8) & 0xff) ;
	}

	uint16_t FBmapColor32(uint16_t color) __attribute__((always_inline)) {
		color = FBmapColor(color);
		return ((uint32_t)color << 16) | color;
	}

	uint8_t _dcpinAsserted;

	void setCommandMode() __attribute__((always_inline)) {
		if (!_dcpinAsserted) {
			*_dcport  &= ~_dcpinmask;
			_dcpinAsserted = 1;
		}
	}

	void setDataMode() __attribute__((always_inline)) {
		if (_dcpinAsserted) {
			*_dcport  |= _dcpinmask;
			_dcpinAsserted = 0;
		}
	}

	void writecommand_cont(uint8_t c)  {
		setCommandMode();
		SPI.transfer(c);
	}
	void writedata8_cont(uint8_t c) {
		setDataMode();
		SPI.transfer(c);
	}

	void writedata16_cont(uint16_t c)  {
		setDataMode();
		SPI.transfer16(c);
	}

	void writecommand_last(uint8_t c)  {
		setCommandMode();
		SPI.transfer(c);
	}
	void writedata8_last(uint8_t c)  {
		setDataMode();
		SPI.transfer(c);
	}
	void writedata16_last(uint16_t c) {
		setDataMode();
		SPI.transfer16(c);
	}

// END ESP stuff


	void HLine(int16_t x, int16_t y, int16_t w, uint16_t color)
	  __attribute__((always_inline)) {
		#ifdef ENABLE_Grafx_FRAMEBUFFER
	  	if (_use_fbtft) {
	  		drawFastHLine(x, y, w, color);
	  		return;
	  	}
	  	#endif
	    x+=_originx;
	    y+=_originy;

	    // Rectangular clipping
	    if((y < _displayclipy1) || (x >= _displayclipx2) || (y >= _displayclipy2)) return;
	    if(x<_displayclipx1) { w = w - (_displayclipx1 - x); x = _displayclipx1; }
	    if((x+w-1) >= _displayclipx2)  w = _displayclipx2-x;
	    if (w<1) return;

		setAddr(x, y, x+w-1, y);
		writecommand_cont(Grafx_RAMWR);
		do { writedata16_cont(color); } while (--w > 0);
	}
	void VLine(int16_t x, int16_t y, int16_t h, uint16_t color)
	  __attribute__((always_inline)) {
		#ifdef ENABLE_Grafx_FRAMEBUFFER
	  	if (_use_fbtft) {
	  		drawFastVLine(x, y, h, color);
	  		return;
	  	}
	  	#endif
		x+=_originx;
	    y+=_originy;

	    // Rectangular clipping
	    if((x < _displayclipx1) || (x >= _displayclipx2) || (y >= _displayclipy2)) return;
	    if(y < _displayclipy1) { h = h - (_displayclipy1 - y); y = _displayclipy1;}
	    if((y+h-1) >= _displayclipy2) h = _displayclipy2-y;
	    if(h<1) return;

		setAddr(x, y, x, y+h-1);
		writecommand_cont(Grafx_RAMWR);
		do { writedata16_cont(color); } while (--h > 0);
	}
	void Pixel(int16_t x, int16_t y, uint16_t color)
	  __attribute__((always_inline)) {
	    x+=_originx;
	    y+=_originy;

	  	if((x < _displayclipx1) ||(x >= _displayclipx2) || (y < _displayclipy1) || (y >= _displayclipy2)) return;

		#ifdef ENABLE_Grafx_FRAMEBUFFER
	  	if (_use_fbtft) {
	  		*(mapYtoFBPtr(y) + x) = FBmapColor( color );
	  		return;
	  	}
	  	#endif
		setAddr(x, y, x, y);
		writecommand_cont(Grafx_RAMWR);
		writedata16_cont(color);
	}
	void drawFontBits(bool opaque, uint32_t bits, uint32_t numbits, int32_t x, int32_t y, uint32_t repeat);
};

#ifndef swap
#define swap(a, b) { typeof(a) t = a; a = b; b = t; }
#endif

#endif // __cplusplus

#endif
