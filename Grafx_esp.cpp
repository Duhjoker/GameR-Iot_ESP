// https://github.com/PaulStoffregen/Grafx_esp
// http://forum.pjrc.com/threads/26305-Highly-optimized-ILI9341-(320x240-TFT-color-display)-library

/***************************************************
  This is our library for the Adafruit ILI9341 Breakout and Shield
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
// 		writeRect8BPP - 	write 8 bit per pixel paletted bitmap
// 		writeRect4BPP - 	write 4 bit per pixel paletted bitmap
// 		writeRect2BPP - 	write 2 bit per pixel paletted bitmap
// 		writeRect1BPP - 	write 1 bit per pixel paletted bitmap

//TODO: transparent bitmap writing routines for sprites

//String Pixel Length support
//---------------------------
//		strPixelLen			- gets pixel length of given ASCII string

// <\SoftEgg>

#include "Grafx_esp.h"
#include <SPI.h>  

// Teensy 3.1 can only generate 30 MHz SPI when running at 120 MHz (overclock)

#define WIDTH  Grafx_TFTWIDTH
#define HEIGHT Grafx_TFTHEIGHT

static const uint8_t isinTable8[] = {
	0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44,
	49, 53, 57, 62, 66, 70, 75, 79, 83, 87,
	91, 96, 100, 104, 108, 112, 116, 120, 124, 128,
	131, 135, 139, 143, 146, 150, 153, 157, 160, 164,
	167, 171, 174, 177, 180, 183, 186, 190, 192, 195,
	198, 201, 204, 206, 209, 211, 214, 216, 219, 221,
	223, 225, 227, 229, 231, 233, 235, 236, 238, 240,
	241, 243, 244, 245, 246, 247, 248, 249, 250, 251,
	252, 253, 253, 254, 254, 254, 255, 255, 255, 255,
};

// Constructor when using hardware ILI9241_KINETISK__pspi->  Faster, but must use SPI pins
// specific to each board type (e.g. 11,13 for Uno, 51,52 for Mega, etc.)

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//////////////-----------------------Constructor----------------------//////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
Grafx_esp::Grafx_esp(uint8_t cs, uint8_t dc, uint8_t rst, 
	uint8_t mosi, uint8_t sclk, uint8_t miso )
{
	_cs   = cs;
	_dc   = dc;
	_rst  = rst;
	_mosi = mosi;
	_sclk = sclk;
	_miso = miso;
	_width    = WIDTH;
	_height   = HEIGHT;

	rotation  = 0;
	cursor_y  = cursor_x    = 0;
	textsize  = 1;
	textcolor = textbgcolor = 0xFFFF;
	wrap      = true;
	font      = NULL;
	setClipRect();
	setOrigin();

	// Added to see how much impact actually using non hardware CS pin might be
    _cspinmask = 0;
    _csport = NULL;

	#ifdef ENABLE_Grafx_FRAMEBUFFER
    _pfbtft = NULL;	
    _pfbtft2 = NULL;	
    _use_fbtft = 0;						// Are we in frame buffer mode?
    #endif

}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////--------------------initialisation--------------------//////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
static const uint8_t init_commands[] = {
	4, 0xEF, 0x03, 0x80, 0x02,
	4, 0xCF, 0x00, 0XC1, 0X30,
	5, 0xED, 0x64, 0x03, 0X12, 0X81,
	4, 0xE8, 0x85, 0x00, 0x78,
	6, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02,
	2, 0xF7, 0x20,
	3, 0xEA, 0x00, 0x00,
	2, Grafx_PWCTR1, 0x23, // Power control
	2, Grafx_PWCTR2, 0x10, // Power control
	3, Grafx_VMCTR1, 0x3e, 0x28, // VCM control
	2, Grafx_VMCTR2, 0x86, // VCM control2
	2, Grafx_MADCTL, 0x48, // Memory Access Control
	2, Grafx_PIXFMT, 0x55,
	3, Grafx_FRMCTR1, 0x00, 0x18,
	4, Grafx_DFUNCTR, 0x08, 0x82, 0x27, // Display Function Control
	2, 0xF2, 0x00, // Gamma Function Disable
	2, Grafx_GAMMASET, 0x01, // Gamma curve selected
	16, Grafx_GMCTRP1, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08,
	0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00, // Set Gamma
	16, Grafx_GMCTRN1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07,
	0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F, // Set Gamma
	0
};

void Grafx_esp::begin(void)
{
	// verify SPI pins are valid;
	// allow user to say use current ones...
	// Currently we are just using defaults

	SPI.begin();

	// TLC
	pcs_data = 0;
	pcs_command = 0;
	pinMode(_cs, OUTPUT);
	_csport = portOutputRegister(digitalPinToPort(_cs));
	_cspinmask = digitalPinToBitMask(_cs);
	*_csport |= _cspinmask;
	pinMode(_dc, OUTPUT);
	_dcport = portOutputRegister(digitalPinToPort(_dc));
	_dcpinmask = digitalPinToBitMask(_dc);
	*_dcport |= _dcpinmask;
	_dcpinAsserted = 0;

	// toggle RST low to reset
	if (_rst < 255) {
		pinMode(_rst, OUTPUT);
		digitalWrite(_rst, HIGH);
		delay(5);
		digitalWrite(_rst, LOW);
		delay(20);
		digitalWrite(_rst, HIGH);
		delay(150);
	}
	/*
	uint8_t x = readcommand8(Grafx_RDMODE);
	Serial.print("\nDisplay Power Mode: 0x"); Serial.println(x, HEX);
	x = readcommand8(Grafx_RDMADCTL);
	Serial.print("\nMADCTL Mode: 0x"); Serial.println(x, HEX);
	x = readcommand8(Grafx_RDPIXFMT);
	Serial.print("\nPixel Format: 0x"); Serial.println(x, HEX);
	x = readcommand8(Grafx_RDIMGFMT);
	Serial.print("\nImage Format: 0x"); Serial.println(x, HEX);
	x = readcommand8(Grafx_RDSELFDIAG);
	Serial.print("\nSelf Diagnostic: 0x"); Serial.println(x, HEX);
	*/
	beginSPITransaction();
	const uint8_t *addr = init_commands;
	while (1) {
		uint8_t count = *addr++;
		if (count-- == 0) break;
		writecommand_cont(*addr++);
		while (count-- > 0) {
			writedata8_cont(*addr++);
		}
	}
	writecommand_last(Grafx_SLPOUT);    // Exit Sleep
	endSPITransaction();
	delay(120);
	beginSPITransaction();
	writecommand_last(Grafx_DISPON);    // Display on
	endSPITransaction();
}

void Grafx_esp::sleep(bool enable) {
	beginSPITransaction();
	if (enable) {
		writecommand_cont(Grafx_DISPOFF);
		writecommand_last(Grafx_SLPIN);
		endSPITransaction();
	}
	else {
		writecommand_cont(Grafx_DISPON);
		writecommand_last(Grafx_SLPOUT);
		endSPITransaction();
		delay(5);
	}
}

void Grafx_esp::pushColor(uint16_t color)
{
	beginSPITransaction();
	writedata16_last(color);
	endSPITransaction();
}

//void Grafx_esp::fillScreen(uint16_t color)
//{
//	fillRect(0, 0, _width, _height, color);
//}

void Grafx_esp::fillScreen(uint16_t color)
{
	#ifdef ENABLE_GrafxT3_FRAMEBUFFER
	if (_use_fbtft && _standard) {
		// Speed up lifted from Franks DMA code... _standard is if no offsets and rects..
		uint32_t color32 = (color << 16) | color;

		uint32_t *pfbPixel = (uint32_t *)_pfbtft;
		uint32_t *pfbtft_end = (uint32_t *)((uint16_t *)&_pfbtft[(GrafxT3_TFTHEIGHT * GrafxT3_TFTWIDTH)]); // setup
		while (pfbPixel < pfbtft_end) {
			*pfbPixel++ = color32; *pfbPixel++ = color32; *pfbPixel++ = color32;*pfbPixel++ = color32;
			*pfbPixel++ = color32; *pfbPixel++ = color32; *pfbPixel++ = color32;*pfbPixel++ = color32;
			*pfbPixel++ = color32; *pfbPixel++ = color32; *pfbPixel++ = color32;*pfbPixel++ = color32;
			*pfbPixel++ = color32; *pfbPixel++ = color32; *pfbPixel++ = color32;*pfbPixel++ = color32;
			*pfbPixel++ = color32; *pfbPixel++ = color32; *pfbPixel++ = color32;*pfbPixel++ = color32;
			*pfbPixel++ = color32; *pfbPixel++ = color32; *pfbPixel++ = color32;*pfbPixel++ = color32;
			*pfbPixel++ = color32; *pfbPixel++ = color32; *pfbPixel++ = color32;*pfbPixel++ = color32;
			*pfbPixel++ = color32; *pfbPixel++ = color32; *pfbPixel++ = color32;*pfbPixel++ = color32;
		}

	} else 
	#endif
	{
		fillRect(0, 0, _width, _height, color);
	}
}

void Grafx_esp::drawPixel(int16_t x, int16_t y, uint16_t color) {
	x += _originx;
	y += _originy;
	if ((x < _displayclipx1) || (x >= _displayclipx2) || (y < _displayclipy1) || (y >= _displayclipy2)) return;

#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (_use_fbtft) {
		*(mapYtoFBPtr(y) + x) = FBmapColor(color);

	}
	else
#endif
	{
		beginSPITransaction();
		setAddr(x, y, x, y);
		writecommand_cont(Grafx_RAMWR);
		writedata16_last(color);
		endSPITransaction();
	}
}

// Bresenham's algorithm - thx wikpedia
void Grafx_esp::drawLine(int16_t x0, int16_t y0,
	int16_t x1, int16_t y1, uint16_t color)
{
	if (y0 == y1) {
		if (x1 > x0) {
			drawFastHLine(x0, y0, x1 - x0 + 1, color);
		}
		else if (x1 < x0) {
			drawFastHLine(x1, y0, x0 - x1 + 1, color);
		}
		else {
			drawPixel(x0, y0, color);
		}
		return;
	}
	else if (x0 == x1) {
		if (y1 > y0) {
			drawFastVLine(x0, y0, y1 - y0 + 1, color);
		}
		else {
			drawFastVLine(x0, y1, y0 - y1 + 1, color);
		}
		return;
	}

	bool steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}
	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	}
	else {
		ystep = -1;
	}

	beginSPITransaction();
	int16_t xbegin = x0;
	if (steep) {
		for (; x0 <= x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					VLine(y0, xbegin, len + 1, color);
				}
				else {
					Pixel(y0, x0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			VLine(y0, xbegin, x0 - xbegin, color);
		}

	}
	else {
		for (; x0 <= x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					HLine(xbegin, y0, len + 1, color);
				}
				else {
					Pixel(x0, y0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			HLine(xbegin, y0, x0 - xbegin, color);
		}
	}
	writecommand_last(Grafx_NOP);
	endSPITransaction();
}

void Grafx_esp::invertDisplay(boolean i)
{
	beginSPITransaction();
	writecommand_last(i ? Grafx_INVON : Grafx_INVOFF);
	endSPITransaction();
}

void Grafx_esp::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	x += _originx;
	y += _originy;
	// Rectangular clipping
	if ((x < _displayclipx1) || (x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (y < _displayclipy1) { h = h - (_displayclipy1 - y); y = _displayclipy1; }
	if ((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;
	if (h<1) return;

#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (_use_fbtft) {
		color = FBmapColor(color);
		while (h--) {
			*(mapYtoFBPtr(y) + x) = color;
			y++;
		}
	}
	else
#endif
	{
		beginSPITransaction();
		setAddr(x, y, x, y + h - 1);
		writecommand_cont(Grafx_RAMWR);
		while (h-- > 1) {
			writedata16_cont(color);
		}
		writedata16_last(color);
		endSPITransaction();
	}
}

void Grafx_esp::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	x += _originx;
	y += _originy;

	// Rectangular clipping
	if ((y < _displayclipy1) || (x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (x<_displayclipx1) { w = w - (_displayclipx1 - x); x = _displayclipx1; }
	if ((x + w - 1) >= _displayclipx2)  w = _displayclipx2 - x;
	if (w<1) return;

#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (_use_fbtft) {
		color = FBmapColor(color);
		uint16_t * pfbPixel = mapYtoFBPtr(y) + x;
		while (w--) {
			*pfbPixel++ = color;
		}
	}
	else
#endif
	{
		beginSPITransaction();
		setAddr(x, y, x + w - 1, y);
		writecommand_cont(Grafx_RAMWR);
		while (w-- > 1) {
			writedata16_cont(color);
		}
		writedata16_last(color);
		endSPITransaction();
	}
}

// fill a rectangle
void Grafx_esp::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	x += _originx;
	y += _originy;

	// Rectangular clipping (drawChar w/big text requires this)
	if ((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (((x + w) <= _displayclipx1) || ((y + h) <= _displayclipy1)) return;
	if (x < _displayclipx1) { w -= (_displayclipx1 - x); x = _displayclipx1; }
	if (y < _displayclipy1) { h -= (_displayclipy1 - y); y = _displayclipy1; }
	if ((x + w - 1) >= _displayclipx2)  w = _displayclipx2 - x;
	if ((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (_use_fbtft) {
		color = FBmapColor(color);

		for (; h>0; h--) {
			uint16_t * pfbPixel = mapYtoFBPtr(y) + x;
			for (int i = 0; i < w; i++) {
				*pfbPixel++ = color;
			}
			y++;
		}
	}
	else
#endif
	{

		// TODO: this can result in a very long transaction time
		// should break this into multiple transactions, even though
		// it'll cost more overhead, so we don't stall other SPI libs
		beginSPITransaction();
		setAddr(x, y, x + w - 1, y + h - 1);
		writecommand_cont(Grafx_RAMWR);
		for (y = h; y>0; y--) {
			for (x = w; x>1; x--) {
				writedata16_cont(color);
			}
			writedata16_last(color);
			if (y > 1 && (y & 1)) {
				endSPITransaction();
				beginSPITransaction();
			}
		}
		endSPITransaction();
	}
}

// Draw a rectangle
void Grafx_esp::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (_use_fbtft) {
		drawFastHLine(x, y, w, color);
		drawFastHLine(x, y + h - 1, w, color);
		drawFastVLine(x, y, h, color);
		drawFastVLine(x + w - 1, y, h, color);
	}
	else
#endif
	{
		beginSPITransaction();
		HLine(x, y, w, color);
		HLine(x, y + h - 1, w, color);
		VLine(x, y, h, color);
		VLine(x + w - 1, y, h, color);
		writecommand_last(Grafx_NOP);
		endSPITransaction();
	}
}

// fillRectVGradient	- fills area with vertical gradient
void Grafx_esp::fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2)
{
	x += _originx;
	y += _originy;

	// Rectangular clipping 
	if ((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (x < _displayclipx1) { w -= (_displayclipx1 - x); x = _displayclipx1; }
	if (y < _displayclipy1) { h -= (_displayclipy1 - y); y = _displayclipy1; }
	if ((x + w - 1) >= _displayclipx2)  w = _displayclipx2 - x;
	if ((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

	int16_t r1, g1, b1, r2, g2, b2, dr, dg, db, r, g, b;
	color565toRGB14(color1, r1, g1, b1);
	color565toRGB14(color2, r2, g2, b2);
	dr = (r2 - r1) / h; dg = (g2 - g1) / h; db = (b2 - b1) / h;
	r = r1; g = g1; b = b1;

#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (_use_fbtft) {
		for (; h>0; h--) {
			uint16_t color = RGB14tocolor565(r, g, b);
			color = FBmapColor(color);

			uint16_t * pfbPixel = mapYtoFBPtr(y) + x;
			for (int i = 0; i < w; i++) {
				*pfbPixel++ = color;
			}
			r += dr; g += dg; b += db;
			y++;
		}
	}
	else
#endif
	{
		beginSPITransaction();
		setAddr(x, y, x + w - 1, y + h - 1);
		writecommand_cont(Grafx_RAMWR);
		for (y = h; y>0; y--) {
			uint16_t color = RGB14tocolor565(r, g, b);

			for (x = w; x>1; x--) {
				writedata16_cont(color);
			}
			writedata16_last(color);
			if (y > 1 && (y & 1)) {
				endSPITransaction();
				beginSPITransaction();
			}
			r += dr; g += dg; b += db;
		}
		endSPITransaction();
	}
}

// fillRectHGradient	- fills area with horizontal gradient
void Grafx_esp::fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color1, uint16_t color2)
{
	x += _originx;
	y += _originy;

	// Rectangular clipping 
	if ((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (x < _displayclipx1) { w -= (_displayclipx1 - x); x = _displayclipx1; }
	if (y < _displayclipy1) { h -= (_displayclipy1 - y); y = _displayclipy1; }
	if ((x + w - 1) >= _displayclipx2)  w = _displayclipx2 - x;
	if ((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

	int16_t r1, g1, b1, r2, g2, b2, dr, dg, db, r, g, b;
	uint16_t color;
	color565toRGB14(color1, r1, g1, b1);
	color565toRGB14(color2, r2, g2, b2);
	dr = (r2 - r1) / w; dg = (g2 - g1) / w; db = (b2 - b1) / w;
	r = r1; g = g1; b = b1;
#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (_use_fbtft) {
		for (; h>0; h--) {
			uint16_t * pfbPixel = mapYtoFBPtr(y) + x;
			for (int i = 0; i < w; i++) {
				*pfbPixel++ = FBmapColor(RGB14tocolor565(r, g, b));
				r += dr; g += dg; b += db;
			}
			y++;
			r = r1; g = g1; b = b1;
		}
	}
	else
#endif
	{
		beginSPITransaction();
		setAddr(x, y, x + w - 1, y + h - 1);
		writecommand_cont(Grafx_RAMWR);
		for (y = h; y>0; y--) {
			for (x = w; x>1; x--) {
				color = RGB14tocolor565(r, g, b);
				writedata16_cont(color);
				r += dr; g += dg; b += db;
			}
			color = RGB14tocolor565(r, g, b);
			writedata16_last(color);
			if (y > 1 && (y & 1)) {
				endSPITransaction();
				beginSPITransaction();
			}
			r = r1; g = g1; b = b1;
		}
		endSPITransaction();
	}
}

// fillScreenVGradient - fills screen with vertical gradient
void Grafx_esp::fillScreenVGradient(uint16_t color1, uint16_t color2)
{
	fillRectVGradient(0, 0, _width, _height, color1, color2);
}

// fillScreenHGradient - fills screen with horizontal gradient
void Grafx_esp::fillScreenHGradient(uint16_t color1, uint16_t color2)
{
	fillRectHGradient(0, 0, _width, _height, color1, color2);
}

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////////////----------------------Geometry------------------------------//////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/


/*
x,y	center of the arc/circle
r		radius in pixel
rs	start angle (counter clock direction, zero at the left)
re			end angle

Notes:
1) A circle will be drawn, if rs == re
2) angle: 128 = Pi
*/
void Grafx_esp::drawArc(int16_t x, int16_t y, int16_t r, int16_t rs, int16_t re, uint16_t color) {
	int16_t l, i, w;//int16_t
	int16_t x1, y1, x2, y2;
	unsigned short dw;
	if (re > rs)
		dw = re - rs;
	else
		dw = 256 - rs + re;

	if (dw == 0) dw = 256;
	l = (uint8_t)(((((unsigned short)r * dw) >> 7) * (unsigned short)201) >> 8);
	//l = (uint8_t)(((((uint16_t)r * dw) >> 7) * (uint16_t)201)>>7);
	//l = (uint8_t)(((((uint16_t)r * dw) >> 7) * (uint16_t)256)>>7);
	x1 = x + (((int16_t)r * (int16_t)i_cos(rs)) >> 7);
	y1 = y + (((int16_t)r * (int16_t)i_sin(rs)) >> 7);
	for (i = 1; i <= l; i++){
		w = ((unsigned short)dw*(unsigned short)i) / (unsigned short)l + rs;
		//w = ((uint16_t)dw * (uint16_t)i) / (uint16_t)l + rs;
		x2 = x + (((int16_t)r * (int16_t)i_cos(w)) >> 7);
		y2 = y + (((int16_t)r * (int16_t)i_sin(w)) >> 7);
		if ((x1 < _width && x2 < _width) && (y1 < _height && y2 < _height)) drawLine(x1, y1, x2, y2, color);
		x1 = x2;
		y1 = y2;
	}
}

void Grafx_esp::ellipse(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
	long a = abs(x1 - x0), b = abs(y1 - y0), b1 = b & 1; /* values of diameter */
	long dx = 4 * (1 - a) * b * b, dy = 4 * (b1 + 1) * a * a; /* error increment */
	long err = dx + dy + b1 * a * a, e2; /* error of 1.step */

	if (x0 > x1) { x0 = x1; x1 += a; } /* if called with swapped points */
	if (y0 > y1) y0 = y1; /* .. exchange them */
	y0 += (b + 1) / 2; /* starting pixel */
	y1 = y0 - b1;
	a *= 8 * a;
	b1 = 8 * b * b;

	do {
		drawPixel(x1, y0, color); /*   I. Quadrant */
		drawPixel(x0, y0, color); /*  II. Quadrant */
		drawPixel(x0, y1, color); /* III. Quadrant */
		drawPixel(x1, y1, color); /*  IV. Quadrant */
		e2 = 2 * err;
		if (e2 >= dx) { x0++; x1--; err += dx += b1; } /* x step */
		if (e2 <= dy) { y0++; y1--; err += dy += a; }  /* y step */
	} while (x0 <= x1);

	while (y0 - y1 < b) {  /* too early stop of flat ellipses a=1 */
		drawPixel(x0 - 1, ++y0, color); /* -> complete tip of ellipse */
		drawPixel(x0 - 1, --y1, color);
	}
}

void Grafx_esp::drawBezier(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color){
	int sx = x0 < x2 ? 1 : -1, sy = y0 < y2 ? 1 : -1; /* step direction */
	int cur = sx * sy * ((x0 - x1) * (y2 - y1) - (x2 - x1) * (y0 - y1)); /* curvature */
	int x = x0 - 2 * x1 + x2, y = y0 - 2 * y1 + y2, xy = 2 * x * y * sx * sy;
	/* compute error increments of P0 */
	long dx = (1 - 2 * abs(x0 - x1)) * y * y + abs(y0 - y1) * xy - 2 * cur * abs(y0 - y2);
	long dy = (1 - 2 * abs(y0 - y1)) * x * x + abs(x0 - x1) * xy + 2 * cur * abs(x0 - x2);
	/* compute error increments of P2 */
	long ex = (1 - 2 * abs(x2 - x1)) * y * y + abs(y2 - y1) * xy + 2 * cur * abs(y0 - y2);
	long ey = (1 - 2 * abs(y2 - y1)) * x * x + abs(x2 - x1) * xy - 2 * cur * abs(x0 - x2);

	if (cur == 0) { drawLine(x0, y0, x2, y2, color); return; } /* straight line */

	x *= 2 * x; y *= 2 * y;
	if (cur < 0) { /* negated curvature */
		x = -x; dx = -dx; ex = -ex; xy = -xy;
		y = -y; dy = -dy; ey = -ey;
	}
	/* algorithm fails for almost straight line, check error values */
	if (dx >= -y || dy <= -x || ex <= -y || ey >= -x) {
		drawLine(x0, y0, x1, y1, color); /* simple approximation */
		drawLine(x1, y1, x2, y2, color);
		return;
	}
	dx -= xy; ex = dx + dy; dy -= xy; /* error of 1.step */

	for (;;) { /* plot curve */
		drawPixel(y0, x0, color);
		ey = 2 * ex - dy; /* save value for test of y step */
		if (2 * ex >= dx) { /* x step */
			if (x0 == x2) break;
			x0 += sx; dy -= xy; ex += dx += y;
		}
		if (ey <= 0) { /* y step */
			if (y0 == y2) break;
			y0 += sy; dx -= xy; ex += dy += x;
		}
	}
}

void Grafx_esp::drawQuad(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color)
{
	drawLine(x0, y0, x1, y1, color);//low 1
	drawLine(x1, y1, x2, y2, color);//high 1
	drawLine(x2, y2, x3, y3, color);//high 2
	drawLine(x3, y3, x0, y0, color);//low 2
}

void Grafx_esp::fillQuad(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color)
{
	fillTriangle(x0, y0, x1, y1, x2, y2, color);
	fillTriangle(x0, y0, x2, y2, x3, y3, color);
}

//from triangle to whatever...
void Grafx_esp::drawPolygon(int16_t cx, int16_t cy, uint8_t sides, int16_t diameter, float rot, uint16_t color)
{
	sides = (sides > 2 ? sides : 3);
	float dtr = (PI / 180.0) + PI;
	float rads = 360.0 / sides;//points spacd equally
	uint8_t i;
	for (i = 0; i < sides; i++) {
		drawLine(
			cx + (sin((i*rads + rot) * dtr) * diameter),
			cy + (cos((i*rads + rot) * dtr) * diameter),
			cx + (sin(((i + 1)*rads + rot) * dtr) * diameter),
			cy + (cos(((i + 1)*rads + rot) * dtr) * diameter),
			color);
	}
}

//#include "glcdfont.c"
//extern "C" const unsigned char glcdfont[];

// Draw a circle outline
void Grafx_esp::drawCircle(int16_t x0, int16_t y0, int16_t r,
	uint16_t color) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	drawPixel(x0, y0 + r, color);
	drawPixel(x0, y0 - r, color);
	drawPixel(x0 + r, y0, color);
	drawPixel(x0 - r, y0, color);

	while (x<y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		drawPixel(x0 + x, y0 + y, color);
		drawPixel(x0 - x, y0 + y, color);
		drawPixel(x0 + x, y0 - y, color);
		drawPixel(x0 - x, y0 - y, color);
		drawPixel(x0 + y, y0 + x, color);
		drawPixel(x0 - y, y0 + x, color);
		drawPixel(x0 + y, y0 - x, color);
		drawPixel(x0 - y, y0 - x, color);
	}
}

void Grafx_esp::drawCircleHelper(int16_t x0, int16_t y0,
	int16_t r, uint8_t cornername, uint16_t color) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	while (x<y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		if (cornername & 0x4) {
			drawPixel(x0 + x, y0 + y, color);
			drawPixel(x0 + y, y0 + x, color);
		}
		if (cornername & 0x2) {
			drawPixel(x0 + x, y0 - y, color);
			drawPixel(x0 + y, y0 - x, color);
		}
		if (cornername & 0x8) {
			drawPixel(x0 - y, y0 + x, color);
			drawPixel(x0 - x, y0 + y, color);
		}
		if (cornername & 0x1) {
			drawPixel(x0 - y, y0 - x, color);
			drawPixel(x0 - x, y0 - y, color);
		}
	}
}

void Grafx_esp::fillCircle(int16_t x0, int16_t y0, int16_t r,
	uint16_t color) {
	drawFastVLine(x0, y0 - r, 2 * r + 1, color);
	fillCircleHelper(x0, y0, r, 3, 0, color);
}

// Used to do circles and roundrects
void Grafx_esp::fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
	uint8_t cornername, int16_t delta, uint16_t color) {

	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	while (x<y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		if (cornername & 0x1) {
			drawFastVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
			drawFastVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
		}
		if (cornername & 0x2) {
			drawFastVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
			drawFastVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
		}
	}
}

// Draw a triangle
void Grafx_esp::drawTriangle(int16_t x0, int16_t y0,
	int16_t x1, int16_t y1,
	int16_t x2, int16_t y2, uint16_t color) {
	drawLine(x0, y0, x1, y1, color);
	drawLine(x1, y1, x2, y2, color);
	drawLine(x2, y2, x0, y0, color);
}

// Fill a triangle
void Grafx_esp::fillTriangle(int16_t x0, int16_t y0,
	int16_t x1, int16_t y1,
	int16_t x2, int16_t y2, uint16_t color) {

	int16_t a, b, y, last;

	// Sort coordinates by Y order (y2 >= y1 >= y0)
	if (y0 > y1) {
		swap(y0, y1); swap(x0, x1);
	}
	if (y1 > y2) {
		swap(y2, y1); swap(x2, x1);
	}
	if (y0 > y1) {
		swap(y0, y1); swap(x0, x1);
	}

	if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
		a = b = x0;
		if (x1 < a)      a = x1;
		else if (x1 > b) b = x1;
		if (x2 < a)      a = x2;
		else if (x2 > b) b = x2;
		drawFastHLine(a, y0, b - a + 1, color);
		return;
	}

	int16_t
		dx01 = x1 - x0,
		dy01 = y1 - y0,
		dx02 = x2 - x0,
		dy02 = y2 - y0,
		dx12 = x2 - x1,
		dy12 = y2 - y1,
		sa = 0,
		sb = 0;

	// For upper part of triangle, find scanline crossings for segments
	// 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	// is included here (and second loop will be skipped, avoiding a /0
	// error there), otherwise scanline y1 is skipped here and handled
	// in the second loop...which also avoids a /0 error here if y0=y1
	// (flat-topped triangle).
	if (y1 == y2) last = y1;   // Include y1 scanline
	else         last = y1 - 1; // Skip it

	for (y = y0; y <= last; y++) {
		a = x0 + sa / dy01;
		b = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;
		/* longhand:
		a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
		b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		*/
		if (a > b) swap(a, b);
		drawFastHLine(a, y, b - a + 1, color);
	}

	// For lower part of triangle, find scanline crossings for segments
	// 0-2 and 1-2.  This loop is skipped if y1=y2.
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for (; y <= y2; y++) {
		a = x1 + sa / dy12;
		b = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;
		/* longhand:
		a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
		b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		*/
		if (a > b) swap(a, b);
		drawFastHLine(a, y, b - a + 1, color);
	}
}

// Draw a rounded rectangle
void Grafx_esp::drawRoundRect(int16_t x, int16_t y, int16_t w,
	int16_t h, int16_t r, uint16_t color) {
	// smarter version
	drawFastHLine(x + r, y, w - 2 * r, color); // Top
	drawFastHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
	drawFastVLine(x, y + r, h - 2 * r, color); // Left
	drawFastVLine(x + w - 1, y + r, h - 2 * r, color); // Right
	// draw four corners
	drawCircleHelper(x + r, y + r, r, 1, color);
	drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
	drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
	drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

// Fill a rounded rectangle
void Grafx_esp::fillRoundRect(int16_t x, int16_t y, int16_t w,
	int16_t h, int16_t r, uint16_t color) {
	// smarter version
	fillRect(x + r, y, w - 2 * r, h, color);

	// draw four corners
	fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
	fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//////////////---------------------------Bitmap-----------------------//////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
// Draw a 1 - bit image(bitmap) at the specified(x, y) position from the
// provided bitmap buffer (must be PROGMEM memory) using the specified
// foreground color (unset bits are transparent).
void Grafx_esp::drawBitmap1(int16_t x, int16_t y,
const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {

	int16_t i, j, byteWidth = (w + 7) / 8;
	uint16_t byte;

	for (j = 0; j<h; j++) {
		for (i = 0; i<w; i++) {
			if (i & 7) byte <<= 1;
			else      byte = pgm_read_byte(bitmap + j * byteWidth + i / 8);
			if (byte & 0x80) drawPixel(x + i, y + j, color);
		}
	}
}

// Draw a 1-bit image (bitmap) at the specified (x,y) position from the
// provided bitmap buffer (must be PROGMEM memory) using the specified
// foreground (for set bits) and background (for clear bits) colors.
void Grafx_esp::drawBitmap2(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {

	int16_t i, j, byteWidth = (w + 7) / 8;
	uint16_t byte;

	for (j = 0; j<h; j++) {
		for (i = 0; i<w; i++) {
			if (i & 7) byte <<= 1;
			else      byte = pgm_read_byte(bitmap + j * byteWidth + i / 8);
			if (byte & 0x80) drawPixel(x + i, y + j, color);
			else            drawPixel(x + i, y + j, bg);
		}
	}
}

// drawBitmap() variant for RAM-resident (not PROGMEM) bitmaps.
void Grafx_esp::drawBitmap3(int16_t x, int16_t y,
	uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {

	int16_t i, j, byteWidth = (w + 7) / 8;
	uint16_t byte;

	for (j = 0; j<h; j++) {
		for (i = 0; i<w; i++) {
			if (i & 7) byte <<= 1;
			else      byte = bitmap[j * byteWidth + i / 8];
			if (byte & 0x80) drawPixel(x + i, y + j, color);
		}
	}
}

// drawBitmap() variant w/background for RAM-resident (not PROGMEM) bitmaps.
void Grafx_esp::drawBitmap4(int16_t x, int16_t y,
	uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {

	int16_t i, j, byteWidth = (w + 7) / 8;
	uint16_t byte;

	for (j = 0; j<h; j++) {
		for (i = 0; i<w; i++) {
			if (i & 7) byte <<= 1;
			else      byte = bitmap[j * byteWidth + i / 8];
			if (byte & 0x80) drawPixel(x + i, y + j, color);
			else            drawPixel(x + i, y + j, bg);
		}
	}
}

void Grafx_esp::drawBitmapTM(int8_t x, int8_t y, int8_t w, int8_t h, const uint8_t *bitmap, uint8_t dx, uint8_t dy, uint8_t dw, uint8_t dh, uint16_t color) {
	int8_t i, j, byteWidth = (w + 7) / 8;
	dw += dx;
	dh += dy;
//	int8_t largest = 0;
//	int8_t largesty = 0;
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (B10000000 >> (i % 8))) {
				int8_t drawX = x + i;
				int8_t drawY = y + j;

				if (drawX >= dx && drawX < dw && drawY >= dy && drawY < dh){
					drawPixel(drawX, drawY, color);
				}
			}
		}
	}
}

boolean Grafx_esp::getBitmapPixel(const uint8_t* bitmap, uint16_t x, uint16_t y){
	return pgm_read_byte(bitmap + 2 + y * ((pgm_read_byte(bitmap) + 7) / 8) + (x >> 3)) & (B10000000 >> (x % 8));
}

void Grafx_esp::drawTilemap(int x, int y, const uint16_t *tilemap, const uint8_t **spritesheet, uint16_t * palette){
	drawTilemap(x, y, tilemap, spritesheet, 0, 0, Grafx_TFTHEIGHT, Grafx_TFTWIDTH, palette);
}
void Grafx_esp::drawTilemap(int x, int y, const uint16_t *tilemap, const uint8_t **spritesheet, uint16_t dx, uint16_t dy, uint16_t dw, uint16_t dh, uint16_t * palette){
	uint8_t tilemap_width = pgm_read_byte(tilemap);
	uint8_t tilemap_height = pgm_read_byte(tilemap + 1);
	uint8_t tile_width = pgm_read_byte(tilemap + 2);
	uint8_t tile_height = pgm_read_byte(tilemap + 3);
	tilemap += 4; // now the first tiyleis at tilemap

	uint16_t ddw = dw + dx;
	uint16_t ddh = dh + dy;
	uint16_t maxDdx = (dw - x + tile_width - 1) / tile_width;
	uint16_t maxDdy = (dh - y + tile_height - 1) / tile_height;
	if (tilemap_width < maxDdx){
		maxDdx = tilemap_width;
	}
	if (tilemap_height < maxDdy){
		maxDdy = tilemap_height;
	}
	int16_t startDdx = (-x) / tile_width;
	int16_t startDdy = (-y) / tile_height;
	if (startDdx < 0){
		startDdx = 0;
	}
	if (startDdy < 0){
		startDdy = 0;
	}
	if (flagcollision)numcolision = 0;                                 //Line 735 - clear numcolision - ADD by Summoner123

	for (uint16_t ddy = startDdy; ddy < maxDdy; ddy++){
		for (uint16_t ddx = startDdx; ddx < maxDdx; ddx++){
			int16_t drawX = ddx*tile_width + x + dx;
			int16_t drawY = ddy*tile_height + y + dy;
			uint16_t tile = pgm_read_byte(tilemap + ddy*tilemap_width + ddx);
			if (drawX >= dx && drawY >= dy && drawX <= (ddw - tile_width) && drawY <= (ddh - tile_height)){
				writeRectNBPP(drawX, drawY,tile_width, tile_height, 4, spritesheet[tile], palette );
				if (flagcollision){
					solid[numcolision].x = drawX;                     //Save X coordinate      - ADD by Summoner123
					solid[numcolision].y = drawY;                     //Save Y coordinate      - ADD by Summoner123
					solid[numcolision].spritecol = spritesheet[tile]; //Save Sprite of tile    - ADD by Summoner123
					numcolision++;                                    //Increment numcolision  - ADD by Summoner123
				}
			}
			else{ // we need to draw a partial bitmap
				writeRect4BPPtm(drawX, drawY, tile_width, tile_height, spritesheet[tile], dx, dy, dw, dh, palette);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//////////////-------------------------Collision----------------------//////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
boolean Grafx_esp::collidePointRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t w, int16_t h){
	if ((x1 >= x2) && (x1<x2 + w))
		if ((y1 >= y2) && (y1<y2 + h))
			return true;
	return false;
}

boolean Grafx_esp::collideRectRect(int16_t x1, int16_t y1, int16_t w1, int16_t h1, int16_t x2, int16_t y2, int16_t w2, int16_t h2){
	return !(x2 >= x1 + w1 ||
		x2 + w2 <= x1 ||
		y2 >= y1 + h1 ||
		y2 + h2 <= y1);
}

boolean Grafx_esp::collideBitmapBitmap(int16_t x1, int16_t y1, const uint8_t* b1, int16_t x2, int16_t y2, const uint8_t* b2){
	int16_t w1 = pgm_read_byte(b1);
	int16_t h1 = pgm_read_byte(b1 + 1);
	int16_t w2 = pgm_read_byte(b2);
	int16_t h2 = pgm_read_byte(b2 + 1);

	if (collideRectRect(x1, y1, w1, h1, x2, y2, w2, h2) == false){
		return false;
	}

	int16_t xmin = (x1 >= x2) ? 0 : x2 - x1;
	int16_t ymin = (y1 >= y2) ? 0 : y2 - y1;
	int16_t xmax = (x1 + w1 >= x2 + w2) ? x2 + w2 - x1 : w1;
	int16_t ymax = (y1 + h1 >= y2 + h2) ? y2 + h2 - y1 : h1;
	for (uint8_t y = ymin; y < ymax; y++){
		for (uint8_t x = xmin; x < xmax; x++){
			if (getBitmapPixel(b1, x, y) && getBitmapPixel(b2, x1 + x - x2, y1 + y - y2)){
				return true;
			}
		}
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//////////////---------------------------text------------------------//////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// Draw a character
void Grafx_esp::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size) {
	if ((x >= _width) || // Clip right
		(y >= _height) || // Clip bottom
		((x + 6 * size - 1) < 0) || // Clip left
		((y + 8 * size - 1) < 0))   // Clip top
		return;
	if (c < fontStart || c > fontStart + fontLength) {
		c = 0;
	}
	else {
		c -= fontStart;
	}

	uint16_t bitCount = 0;
	uint16_t line = 0;
	uint16_t i, j;
	int fontIndex = (c*(fontWidth*fontHeight) / 8) + 4;
	for (i = 0; i<fontHeight; i++) {
		//uint8_t line;
		for (j = 0; j<fontWidth; j++) {
			if (bitCount++ % 8 == 0) {
				line = pgm_read_byte(fontData + fontIndex++);
			}
			if (line & 0x80) {
				if (size > 1) {//big
					fillRect(x + (j*size), y + (i*size), size, size, color);
				}
				else {  // default size
					drawPixel(x + j, y + i, color);
				}
			}
			else if (bg != color) {
				if (size > 1) {// big
					fillRect(x + (j*size), y + (i*size), size, size, bg);
				}
				else {  // def size
					drawPixel(x + j, y + i, bg);
				}
			}
			line <<= 1;
		}
	}
}


// Draw a character
/*void Grafx_esp::drawChar(int16_t x, int16_t y, unsigned char c,
uint16_t fgcolor, uint16_t bgcolor, uint8_t size)
{
	if ((x >= _width) || // Clip right
		(y >= _height) || // Clip bottom
		((x + 6 * size - 1) < 0) || // Clip left  TODO: is this correct?
		((y + 8 * size - 1) < 0))   // Clip top   TODO: is this correct?
		return;

	if (fgcolor == bgcolor) {
		// This transparent approach is only about 20% faster
		if (size == 1) {
			uint8_t mask = 0x01;
			int16_t xoff, yoff;
			for (yoff = 0; yoff < 8; yoff++) {
				uint8_t line = 0;
				for (xoff = 0; xoff < 5; xoff++) {
					if (glcdfont[c * 5 + xoff] & mask) line |= 1;
					line <<= 1;
				}
				line >>= 1;
				xoff = 0;
				while (line) {
					if (line == 0x1F) {
						drawFastHLine(x + xoff, y + yoff, 5, fgcolor);
						break;
					}
					else if (line == 0x1E) {
						drawFastHLine(x + xoff, y + yoff, 4, fgcolor);
						break;
					}
					else if ((line & 0x1C) == 0x1C) {
						drawFastHLine(x + xoff, y + yoff, 3, fgcolor);
						line <<= 4;
						xoff += 4;
					}
					else if ((line & 0x18) == 0x18) {
						drawFastHLine(x + xoff, y + yoff, 2, fgcolor);
						line <<= 3;
						xoff += 3;
					}
					else if ((line & 0x10) == 0x10) {
						drawPixel(x + xoff, y + yoff, fgcolor);
						line <<= 2;
						xoff += 2;
					}
					else {
						line <<= 1;
						xoff += 1;
					}
				}
				mask = mask << 1;
			}
		}
		else {
			uint8_t mask = 0x01;
			int16_t xoff, yoff;
			for (yoff = 0; yoff < 8; yoff++) {
				uint8_t line = 0;
				for (xoff = 0; xoff < 5; xoff++) {
					if (glcdfont[c * 5 + xoff] & mask) line |= 1;
					line <<= 1;
				}
				line >>= 1;
				xoff = 0;
				while (line) {
					if (line == 0x1F) {
						fillRect(x + xoff * size, y + yoff * size,
							5 * size, size, fgcolor);
						break;
					}
					else if (line == 0x1E) {
						fillRect(x + xoff * size, y + yoff * size,
							4 * size, size, fgcolor);
						break;
					}
					else if ((line & 0x1C) == 0x1C) {
						fillRect(x + xoff * size, y + yoff * size,
							3 * size, size, fgcolor);
						line <<= 4;
						xoff += 4;
					}
					else if ((line & 0x18) == 0x18) {
						fillRect(x + xoff * size, y + yoff * size,
							2 * size, size, fgcolor);
						line <<= 3;
						xoff += 3;
					}
					else if ((line & 0x10) == 0x10) {
						fillRect(x + xoff * size, y + yoff * size,
							size, size, fgcolor);
						line <<= 2;
						xoff += 2;
					}
					else {
						line <<= 1;
						xoff += 1;
					}
				}
				mask = mask << 1;
			}
		}
	}
	else {
		// This solid background approach is about 5 time faster
		uint8_t xc, yc;
		uint8_t xr, yr;
		uint8_t mask = 0x01;
		uint16_t color;

		// We need to offset by the origin.
		x += _originx;
		y += _originy;
		int16_t x_char_start = x;  // remember our X where we start outputting...

		if ((x >= _displayclipx2) || // Clip right
			(y >= _displayclipy2) || // Clip bottom
			((x + 6 * size - 1) < _displayclipx1) || // Clip left  TODO: this is not correct
			((y + 8 * size - 1) < _displayclipy1))   // Clip top   TODO: this is not correct
			return;


#ifdef ENABLE_Grafx_FRAMEBUFFER
		if (_use_fbtft) {
			fgcolor = FBmapColor(fgcolor);
			bgcolor = FBmapColor(bgcolor);

			uint16_t * pfbPixel_row = &_pfbtft[y*_width + x];
			for (yc = 0; (yc < 8) && (y < _displayclipy2); yc++) {
				for (yr = 0; (yr < size) && (y < _displayclipy2); yr++) {
					x = x_char_start; 		// get our first x position...
					if (y >= _displayclipy1) {
						uint16_t * pfbPixel = pfbPixel_row;
						for (xc = 0; xc < 5; xc++) {
							if (glcdfont[c * 5 + xc] & mask) {
								color = fgcolor;
							}
							else {
								color = bgcolor;
							}
							for (xr = 0; xr < size; xr++) {
								if ((x >= _displayclipx1) && (x < _displayclipx2)) {
									*pfbPixel = color;
								}
								pfbPixel++;
								x++;
							}
						}
						for (xr = 0; xr < size; xr++) {
							if ((x >= _displayclipx1) && (x < _displayclipx2)) {
								*pfbPixel = bgcolor;
							}
							pfbPixel++;
							x++;
						}
					}
					pfbPixel_row += _width; // setup pointer to 
					y++;
				}
				mask = mask << 1;
			}

		}
		else
#endif
		{
			// need to build actual pixel rectangle we will output into.
			int16_t y_char_top = y;	// remember the y
			int16_t w = 6 * size;
			int16_t h = 8 * size;

			if (x < _displayclipx1) { w -= (_displayclipx1 - x); x = _displayclipx1; }
			if ((x + w - 1) >= _displayclipx2)  w = _displayclipx2 - x;
			if (y < _displayclipy1) { h -= (_displayclipy1 - y); y = _displayclipy1; }
			if ((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

			beginSPITransaction();
			setAddr(x, y, x + w - 1, y + h - 1);

			y = y_char_top;	// restore the actual y.
			writecommand_cont(Grafx_RAMWR);
			for (yc = 0; (yc < 8) && (y < _displayclipy2); yc++) {
				for (yr = 0; (yr < size) && (y < _displayclipy2); yr++) {
					x = x_char_start; 		// get our first x position...
					if (y >= _displayclipy1) {
						for (xc = 0; xc < 5; xc++) {
							if (glcdfont[c * 5 + xc] & mask) {
								color = fgcolor;
							}
							else {
								color = bgcolor;
							}
							for (xr = 0; xr < size; xr++) {
								if ((x >= _displayclipx1) && (x < _displayclipx2)) {
									writedata16_cont(color);
								}
								x++;
							}
						}
						for (xr = 0; xr < size; xr++) {
							if ((x >= _displayclipx1) && (x < _displayclipx2)) {
								writedata16_cont(bgcolor);
							}
							x++;
						}
					}
					y++;
				}
				mask = mask << 1;
			}
			writecommand_last(Grafx_NOP);
			endSPITransaction();
		}
	}
}*/

void Grafx_esp::setTextColor(uint16_t c) {
	// For 'transparent' background, we'll set the bg
	// to the same as fg instead of using a flag
	textcolor = textbgcolor = c;
}

void Grafx_esp::setTextColor(uint16_t c, uint16_t b) {
	textcolor = c;
	textbgcolor = b;
}

void Grafx_esp::setTextSize(uint8_t s) {
	textsize = (s > 0) ? s : 1;
}

uint8_t Grafx_esp::getTextSize() {
	return textsize;
}

void Grafx_esp::setTextWrap(boolean w) {
	wrap = w;
}

boolean Grafx_esp::getTextWrap()
{
	return wrap;
}

void Grafx_esp::setRotation(uint8_t m)
{
	rotation = m % 4; // can't be higher than 3
	beginSPITransaction();
	writecommand_cont(Grafx_MADCTL);
	switch (rotation) {
	case 0:
		writedata8_last(MADCTL_MX | MADCTL_BGR);
		_width = Grafx_TFTWIDTH;
		_height = Grafx_TFTHEIGHT;
		break;
	case 1:
		writedata8_last(MADCTL_MV | MADCTL_BGR);
		_width = Grafx_TFTHEIGHT;
		_height = Grafx_TFTWIDTH;
		break;
	case 2:
		writedata8_last(MADCTL_MY | MADCTL_BGR);
		_width = Grafx_TFTWIDTH;
		_height = Grafx_TFTHEIGHT;
		break;
	case 3:
		writedata8_last(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
		_width = Grafx_TFTHEIGHT;
		_height = Grafx_TFTWIDTH;
		break;
	}
	endSPITransaction();
	setClipRect();
	setOrigin();

	cursor_x = 0;
	cursor_y = 0;
}

uint8_t Grafx_esp::getRotation(void) {
	return rotation;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
//////////////-----------------------Cursor------------------------------//////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

void Grafx_esp::setScroll(uint16_t offset)
{
	beginSPITransaction();
	writecommand_cont(Grafx_VSCRSADD);
	writedata16_last(offset);
	endSPITransaction();
}

void Grafx_esp::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	beginSPITransaction();
	setAddr(x0, y0, x1, y1);
	writecommand_last(Grafx_RAMWR); // write to RAM
	endSPITransaction();
}

void Grafx_esp::setCursor(int16_t x, int16_t y) {
	if (x < 0) x = 0;
	else if (x >= _width) x = _width - 1;
	cursor_x = x;
	if (y < 0) y = 0;
	else if (y >= _height) y = _height - 1;
	cursor_y = y;
}
void Grafx_esp::getCursor(int16_t *x, int16_t *y) {
	*x = cursor_x;
	*y = cursor_y;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////------------------------------Color-------------------------------//////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

// Read Pixel at x,y and get back 16-bit packed color
#define READ_PIXEL_PUSH_BYTE 0x3f
uint16_t Grafx_esp::readPixel(int16_t x, int16_t y)
{
	// Kinetisk
	uint16_t colors;
	readRect(x, y, 1, 1, &colors);
	return colors;
}

// Now lets see if we can read in multiple pixels
// Teensy LC version
void Grafx_esp::readRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pcolors)
{
#if 0
	// Use our Origin. 
	x += _originx;
	y += _originy;
	//BUGBUG:: Should add some validation of X and Y

	if (_miso == 0xff) return;		// bail if not valid miso

	uint8_t rgb[3];               // RGB bytes received from the display
	uint8_t rgbIdx = 0;
	uint32_t txCount = w * h * 3; // number of bytes we will transmit to the display
	uint32_t rxCount = txCount;   // number of bytes we will receive back from the display

	beginSPITransaction(Grafx_SPICLOCK_READ);

	setAddr(x, y, x + w - 1, y + h - 1);
	writecommand_cont(Grafx_RAMRD); // read from RAM

	// transmit a DUMMY byte before the color bytes
	writedata8_cont(0);

	// Wait until that one returns, Could do a little better and double buffer but this is easer for now.
	waitTransmitComplete();

	// Since double buffer setup lets try keeping read/write in sync
#define RRECT_TIMEOUT 0xffff	
#undef 	READ_PIXEL_PUSH_BYTE
#define READ_PIXEL_PUSH_BYTE 0 // try with zero to see... 	
	uint16_t timeout_countdown = RRECT_TIMEOUT;
	uint16_t dl_in;
	// Write out first byte:

	while (!(_pkinetisl_spi->S & SPI_S_SPTEF)); // Not worried that this can completely hang?
	_pkinetisl_spi->DL = READ_PIXEL_PUSH_BYTE;

	while (rxCount && timeout_countdown) {
		// Now wait until we can output something
		dl_in = 0xffff;
		if (rxCount > 1) {
			while (!(_pkinetisl_spi->S & SPI_S_SPTEF)); // Not worried that this can completely hang?
			if (_pkinetisl_spi->S & SPI_S_SPRF)
				dl_in = _pkinetisl_spi->DL;
			_pkinetisl_spi->DL = READ_PIXEL_PUSH_BYTE;
		}

		// Now wait until there is a byte available to receive
		while ((dl_in != 0xffff) && !(_pkinetisl_spi->S & SPI_S_SPRF) && --timeout_countdown);
		if (timeout_countdown) {   // Make sure we did not get here because of timeout 
			rxCount--;
			rgb[rgbIdx] = (dl_in != 0xffff) ? dl_in : _pkinetisl_spi->DL;
			rgbIdx++;
			if (rgbIdx == 3) {
				rgbIdx = 0;
				*pcolors++ = color565(rgb[0], rgb[1], rgb[2]);
			}
			timeout_countdown = timeout_countdown;
		}
	}

	// Debug code. 
	/*	if (timeout_countdown == 0) {
	Serial.print("RRect Timeout ");
	Serial.println(rxCount, DEC);
	} */
	endSPITransaction();
#endif	
}

// Now lets see if we can writemultiple pixels
void Grafx_esp::writeRect(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *pcolors)
{

	x += _originx;
	y += _originy;
	uint16_t x_clip_left = 0;  // How many entries at start of colors to skip at start of row
	uint16_t x_clip_right = 0;    // how many color entries to skip at end of row for clipping
	// Rectangular clipping 

	// See if the whole thing out of bounds...
	if ((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (((x + w) <= _displayclipx1) || ((y + h) <= _displayclipy1)) return;

	// In these cases you can not do simple clipping, as we need to synchronize the colors array with the
	// We can clip the height as when we get to the last visible we don't have to go any farther. 
	// also maybe starting y as we will advance the color array. 
	if (y < _displayclipy1) {
		int dy = (_displayclipy1 - y);
		h -= dy;
		pcolors += (dy*w); // Advance color array to 
		y = _displayclipy1;
	}

	if ((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

	// For X see how many items in color array to skip at start of row and likewise end of row 
	if (x < _displayclipx1) {
		x_clip_left = _displayclipx1 - x;
		w -= x_clip_left;
		x = _displayclipx1;
	}
	if ((x + w - 1) >= _displayclipx2) {
		x_clip_right = w;
		w = _displayclipx2 - x;
		x_clip_right -= w;
	}

#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (_use_fbtft) {
		for (; h>0; h--) {
			uint16_t * pfbPixel = mapYtoFBPtr(y) + x;
			pcolors += x_clip_left;
			for (int i = 0; i < w; i++) {
				*pfbPixel++ = FBmapColor(*pcolors++);
			}
			y++;
			pcolors += x_clip_right;

		}
		return;
	}
#endif

	beginSPITransaction();
	setAddr(x, y, x + w - 1, y + h - 1);
	writecommand_cont(Grafx_RAMWR);
	for (y = h; y>0; y--) {
		pcolors += x_clip_left;
		for (x = w; x>1; x--) {
			writedata16_cont(*pcolors++);
		}
		writedata16_last(*pcolors++);
		pcolors += x_clip_right;
	}
	endSPITransaction();
}

// writeRect8BPP - 	write 8 bit per pixel paletted bitmap
//					bitmap data in array at pixels, one byte per pixel
//					color palette data in array at palette
void Grafx_esp::writeRect8BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette)
{
	//Serial.printf("\nWR8: %d %d %d %d %x\n", x, y, w, h, (uint32_t)pixels);
	x += _originx;
	y += _originy;

	uint16_t x_clip_left = 0;  // How many entries at start of colors to skip at start of row
	uint16_t x_clip_right = 0;    // how many color entries to skip at end of row for clipping
	// Rectangular clipping 

	// See if the whole thing out of bounds...
	if ((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (((x + w) <= _displayclipx1) || ((y + h) <= _displayclipy1)) return;

	// In these cases you can not do simple clipping, as we need to synchronize the colors array with the
	// We can clip the height as when we get to the last visible we don't have to go any farther. 
	// also maybe starting y as we will advance the color array. 
	if (y < _displayclipy1) {
		int dy = (_displayclipy1 - y);
		h -= dy;
		pixels += (dy*w); // Advance color array to 
		y = _displayclipy1;
	}

	if ((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

	// For X see how many items in color array to skip at start of row and likewise end of row 
	if (x < _displayclipx1) {
		x_clip_left = _displayclipx1 - x;
		w -= x_clip_left;
		x = _displayclipx1;
	}
	if ((x + w - 1) >= _displayclipx2) {
		x_clip_right = w;
		w = _displayclipx2 - x;
		x_clip_right -= w;
	}
	//Serial.printf("WR8C: %d %d %d %d %x- %d %d\n", x, y, w, h, (uint32_t)pixels, x_clip_right, x_clip_left);
#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (_use_fbtft) {
		for (; h>0; h--) {
			pixels += x_clip_left;
			uint16_t * pfbPixel = mapYtoFBPtr(y) + x;
			for (int i = 0; i < w; i++) {
				*pfbPixel++ = FBmapColor(palette[*pixels++]);
			}
			pixels += x_clip_right;
			y++;
		}
		return;
	}
#endif

	beginSPITransaction();
	setAddr(x, y, x + w - 1, y + h - 1);
	writecommand_cont(Grafx_RAMWR);
	for (y = h; y>0; y--) {
		pixels += x_clip_left;
		//Serial.printf("%x: ", (uint32_t)pixels);
		for (x = w; x>1; x--) {
			//Serial.print(*pixels, DEC);
			writedata16_cont(palette[*pixels++]);
		}
		//Serial.println(*pixels, DEC);
		writedata16_last(palette[*pixels++]);
		pixels += x_clip_right;
	}
	endSPITransaction();
}

// writeRect4BPP - 	write 4 bit per pixel paletted bitmap
//					bitmap data in array at pixels, 4 bits per pixel
//					color palette data in array at palette
//					width must be at least 2 pixels
void Grafx_esp::writeRect4BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette)
{
	// Simply call through our helper
	writeRectNBPP(x, y, w, h, 4, pixels, palette);
}

///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void Grafx_esp::writeRect4BPPtm(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, uint16_t dx, uint16_t dy, uint16_t dw, uint16_t dh, const uint16_t * palette )
{
	dw += dx;
	dh += dy;{
//	int16_t largest = 0;
//	int16_t largesty = 0;
		int16_t drawX = x;
				int16_t drawY = y;

				if (drawX >= dx && drawX < dw && drawY >= dy && drawY < dh){
	// Simply call through our helper
	writeRectNBPP(x, y, w, h,  4, pixels, palette );
        }
    }
}
// writeRect2BPP - 	write 2 bit per pixel paletted bitmap
//					bitmap data in array at pixels, 4 bits per pixel
//					color palette data in array at palette
//					width must be at least 4 pixels
void Grafx_esp::writeRect2BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette)
{
	// Simply call through our helper
	writeRectNBPP(x, y, w, h, 2, pixels, palette);

}

///============================================================================
// writeRect1BPP - 	write 1 bit per pixel paletted bitmap
//					bitmap data in array at pixels, 4 bits per pixel
//					color palette data in array at palette
//					width must be at least 8 pixels
void Grafx_esp::writeRect1BPP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, const uint16_t * palette)
{
	// Simply call through our helper
	writeRectNBPP(x, y, w, h, 1, pixels, palette);
}



///============================================================================
// writeRectNBPP - 	write N(1, 2, 4, 8) bit per pixel paletted bitmap
//					bitmap data in array at pixels
//  Currently writeRect1BPP, writeRect2BPP, writeRect4BPP use this to do all of the work. 
void Grafx_esp::writeRectNBPP(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t bits_per_pixel,
	const uint8_t *pixels, const uint16_t * palette)
{
	//Serial.printf("\nWR8: %d %d %d %d %x\n", x, y, w, h, (uint32_t)pixels);
	x += _originx;
	y += _originy;
	uint8_t pixels_per_byte = 8 / bits_per_pixel;
	uint16_t count_of_bytes_per_row = (w + pixels_per_byte - 1) / pixels_per_byte;		// Round up to handle non multiples
	uint8_t row_shift_init = 8 - bits_per_pixel;				// We shift down 6 bits by default 
	uint8_t pixel_bit_mask = (1 << bits_per_pixel) - 1; 		// get mask to use below
	// Rectangular clipping 

	// See if the whole thing out of bounds...
	if ((x >= _displayclipx2) || (y >= _displayclipy2)) return;
	if (((x + w) <= _displayclipx1) || ((y + h) <= _displayclipy1)) return;

	// In these cases you can not do simple clipping, as we need to synchronize the colors array with the
	// We can clip the height as when we get to the last visible we don't have to go any farther. 
	// also maybe starting y as we will advance the color array. 
	// Again assume multiple of 8 for width
	if (y < _displayclipy1) {
		int dy = (_displayclipy1 - y);
		h -= dy;
		pixels += dy * count_of_bytes_per_row;
		y = _displayclipy1;
	}

	if ((y + h - 1) >= _displayclipy2) h = _displayclipy2 - y;

	// For X see how many items in color array to skip at start of row and likewise end of row 
	if (x < _displayclipx1) {
		uint16_t x_clip_left = _displayclipx1 - x;
		w -= x_clip_left;
		x = _displayclipx1;
		// Now lets update pixels to the rigth offset and mask
		uint8_t x_clip_left_bytes_incr = x_clip_left / pixels_per_byte;
		pixels += x_clip_left_bytes_incr;
		row_shift_init = 8 - (x_clip_left - (x_clip_left_bytes_incr * pixels_per_byte) + 1) * bits_per_pixel;
	}

	if ((x + w - 1) >= _displayclipx2) {
		w = _displayclipx2 - x;
	}

	const uint8_t * pixels_row_start = pixels;  // remember our starting position offset into row

#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (_use_fbtft) {
//		uint16_t * pfbPixel_row = &_pfbtft[ y*_width + x];
		for (; h>0; h--) {
			uint16_t * pfbPixel = mapYtoFBPtr(y) + x;
			pixels = pixels_row_start;				// setup for this row
			uint8_t pixel_shift = row_shift_init;			// Setup mask
                    int16_t x_out = x; 
			for (int i = 0; i < w; i++) {
///				*pfbPixel++ = FBmapColor(palette[((*pixels) >> pixel_shift) & pixel_bit_mask]);
				uint8_t palette_index = ((*pixels)>>pixel_shift) & pixel_bit_mask;
                if (palette_index != TRANSPARENT_INDEX)
                	Pixel(x_out, y, palette[palette_index]); // *pfbPixel++;
 //               *pfbPixel = palette[palette_index];

           
				if (!pixel_shift) {
					pixel_shift = 8 - bits_per_pixel;	//setup next mask
					pixels++;
				}
				else {
					pixel_shift -= bits_per_pixel;
				}
				x_out++;
			}
			pixels_row_start += count_of_bytes_per_row;
			y++;
		}
		return;

	}
#endif
/*
	beginSPITransaction();
	setAddr(x, y, x + w - 1, y + h - 1);
	writecommand_cont(Grafx_RAMWR);
	for (; h>0; h--) {
		pixels = pixels_row_start;				// setup for this row
		uint8_t pixel_shift = row_shift_init;			// Setup mask

		for (int i = 0; i < w; i++) {
			writedata16_cont(palette[((*pixels) >> pixel_shift) & pixel_bit_mask]);
			if (!pixel_shift) {
				pixel_shift = 8 - bits_per_pixel;	//setup next mask
				pixels++;
			}
			else {
				pixel_shift -= bits_per_pixel;
			}
		}
		pixels_row_start += count_of_bytes_per_row;
	}
	writecommand_last(Grafx_NOP);
	endSPITransaction();
}*/
////////////////////////////////////////////////////////////////////
///for transparent pixels
	beginSPITransaction();
	for (;h>0; h--) {
		pixels = pixels_row_start;				// setup for this row
		uint8_t pixel_shift = row_shift_init;			// Setup mask
                int16_t x_out = x; 
		for (int i = 0 ;i < w; i++) {
			uint8_t palette_index = ((*pixels)>>pixel_shift) & pixel_bit_mask;
			if (palette_index != TRANSPARENT_INDEX)
				Pixel(x_out, y, palette[palette_index]);
			if (!pixel_shift) {
				pixel_shift = 8 - bits_per_pixel;	//setup next mask
				pixels++;
			} else {
				pixel_shift -= bits_per_pixel;
			}
			x_out++;
		}
		pixels_row_start += count_of_bytes_per_row;
		y++;
	}
	endSPITransaction();
}
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//////////////-------------------------Frame Buffer------------------------//////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

//=======================================================================
// Add optinal support for using frame buffer to speed up complex outputs
//=======================================================================
#define CBALLOC (Grafx_TFTHEIGHT*Grafx_TFTWIDTH*2)
uint8_t Grafx_esp::useFrameBuffer(boolean b)		// use the frame buffer?  First call will allocate
{
#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (b) {
		// First see if we need to allocate buffer
		if (_pfbtft == NULL) {
			Serial.println("Allocate Frame buffer");
			_pfbtft = (uint16_t *)malloc(CBALLOC);
			// Try allocating the whole thing
			if (_pfbtft == NULL) {
				Serial.println("Malloc failed");  Serial.flush();
				// See if we can allocate in two parts.
				_pfbtft = (uint16_t *)malloc(CBALLOC / 2);
				if (_pfbtft == NULL) {
					Serial.println("half size Malloc failed");  Serial.flush();
					return 0;	// failed 
				}
				_pfbtft2 = (uint16_t *)malloc(CBALLOC / 2);
				if (_pfbtft2 == NULL) {
					Serial.println("2nd half size Malloc failed");  Serial.flush();
					free(_pfbtft);
					_pfbtft = NULL;
					return 0;	// failed 
				}
				_fbtft1_lines = _height / 2;
				memset(_pfbtft, 0, CBALLOC / 2);
				memset(_pfbtft2, 0, CBALLOC / 2);
			}
			else {
				_fbtft1_lines = _height;
				memset(_pfbtft, 0, CBALLOC);
				_pfbtft2 = NULL;
			}
		}

		_use_fbtft = 1;
	}
	else
		_use_fbtft = 0;

	return _use_fbtft;
#else
	return 0;
#endif
}

void Grafx_esp::freeFrameBuffer(void)						// explicit call to release the buffer
{
#ifdef ENABLE_Grafx_FRAMEBUFFER
	if (_pfbtft != NULL) {
		free(_pfbtft);
		_pfbtft = NULL;
		_use_fbtft = 0;	// make sure the use is turned off
		if (_pfbtft2 != NULL) {
			free(_pfbtft2);
			_pfbtft2 = NULL;
		}
	}
#endif
}
void Grafx_esp::updateScreen(void)					// call to say update the screen now.
{
	// Not sure if better here to check flag or check existence of buffer.
	// Will go by buffer as maybe can do interesting things?
	//	Serial.println("UpdateScreen Called");
	if (_use_fbtft) {
		//		Serial.println("Using Frame buffer");
		beginSPITransaction();
		if (_standard) {
			// Doing full window. 
			setAddr(0, 0, _width - 1, _height - 1);
			writecommand_cont(Grafx_RAMWR);
			//			Serial.print(_fbtft1_lines*_width*2, DEC);
			setDataMode();
			SPI.transferBytes((uint8_t*)_pfbtft, NULL, _fbtft1_lines*_width * 2);
			if (_fbtft1_lines < _height)
				SPI.transferBytes((uint8_t*)_pfbtft2, NULL, _fbtft1_lines*_width * 2);
		}
		else {
			// setup just to output the clip rectangle area. 
			setAddr(_displayclipx1, _displayclipy1, _displayclipx2 - 1, _displayclipy2 - 1);
			writecommand_cont(Grafx_RAMWR);
			setDataMode();
			uint16_t count_bytes_output_per_row = (_displayclipx2 - _displayclipx1) * 2; // Need to verify count
			for (uint16_t y = _displayclipy1; y < _displayclipy2; y++) {
				SPI.transferBytes((uint8_t*)(mapYtoFBPtr(y) + _displayclipx1), NULL, count_bytes_output_per_row);
			}
		}
		endSPITransaction();
	}

}

boolean Grafx_esp::updateAll() {
	if (((nextFrameMillis - millis()) > timePerFrame) && frameEndMicros) { //if time to render a new frame is reached and the frame end has ran once
		nextFrameMillis = millis() + timePerFrame;
		frameCount++;

		frameEndMicros = 0;
		frameStartMicros = micros();


//		buttons.update();

		return true;

	}
	else {
		if (!frameEndMicros) { //runs once at the end of the frame
//			sound.updateTrack();
//			sound.updatePattern();
//			sound.updateNote();
//			updatePopup();
//			displayBattery();

			updateScreen(); //send the buffer to the screen
			if (!persistence)
				freeFrameBuffer(); //clear the buffer

			frameEndMicros = micros(); //measure the frame's end time
			frameDurationMicros = frameEndMicros - frameStartMicros;

			//            display.setTextColor(BLACK);
			//            display.setCursor(0, 40);
			//            display.print(frameDurationMicros / timePerFrame);
			//            display.print(" ");
			//            display.print(2048 - freeRam());

			//            display.setCursor(0, 32);
			//            display.print("CPU:");
			//            display.print(frameDurationMicros / timePerFrame);
			//            display.println("/1000");
			//            display.print("RAM:");
			//            display.print(2048 - freeRam());
			//            display.println("/2048");
		}
		return false;
	}
}

void Grafx_esp::setFrameRate(uint8_t fps) {
	timePerFrame = 1000 / fps;
//	sound.prescaler = fps / 20;
//	sound.prescaler = max(1, sound.prescaler);
}

uint8_t Grafx_esp::getCpuLoad(){
	return(frameDurationMicros / (10 * timePerFrame));
}

uint16_t Grafx_esp::getFreeRam() {
	//from http://www.controllerprojects.com/2011/05/23/determining-sram-usage-on-arduino/
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//////////////---------------------------Misc------------------------------//////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
size_t Grafx_esp::write(uint8_t c)
{
	if (font) {
		if (c == '\n') {
			cursor_y += font->line_space; // Fix linefeed. Added by T.T., SoftEgg
			cursor_x = 0;
		}
		else {
			drawFontChar(c);
		}
	}
	else {
		if (c == '\n') {
			cursor_y += textsize * 8;
			cursor_x = 0;
		}
		else if (c == '\r') {
			// skip em
		}
		else {
			drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
			cursor_x += textsize * 6;
			if (wrap && (cursor_x > (_width - textsize * 6))) {
				cursor_y += textsize * 8;
				cursor_x = 0;
			}
		}
	}
	return 1;
}

void Grafx_esp::plot8points(uint16_t cx, uint16_t cy, uint16_t x, uint16_t y, uint16_t color){
	plot4points(cx, cy, x, y, color);
	if (x != y) plot4points(cx, cy, y, x, color);
}


void Grafx_esp::plot4points(uint16_t cx, uint16_t cy, uint16_t x, uint16_t y, uint16_t color){
	drawPixel(cx + x, cy + y, color);
	if (x != 0) drawPixel(cx - x, cy + y, color);
	if (y != 0) drawPixel(cx + x, cy - y, color);
	if (x != 0 && y != 0) drawPixel(cx - x, cy - y, color);
}

int Grafx_esp::i_sin(int x){
	boolean pos = true;  // positive - keeps an eye on the sign.
	uint8_t idx;
	//remove next 6 lines for fastestl!
	/*     if (x < 0) {
	x = -x;
	pos = !pos;
	}
	if (x >= 360) x %= 360;   */
	if (x > 180)
	{
		idx = x - 180;
		pos = !pos;
	}
	else idx = x;
	if (idx > 90) idx = 180 - idx;
	if (pos) return isinTable8[idx] / 2;
	return -(isinTable8[idx] / 2);
}

int Grafx_esp::i_cos(int x){
	return i_sin(x + 90);
}

// Return the size of the display(per current rotation)
//int16_t Grafx_esp::_width(void) const {
//	return _width;
//}

//int16_t Grafx_esp::_height(void) const {
//	return _height;
//}

/*
uint8_t Grafx_esp::readdata(void)
{
  uint8_t r;
       // Try to work directly with SPI registers...
       // First wait until output queue is empty
        uint16_t wTimeout = 0xffff;
        while (((_pkinetisk_spi->SR) & (15 << 12)) && (--wTimeout)) ; // wait until empty
        
//       	_pkinetisk_spi->MCR |= SPI_MCR_CLR_RXF; // discard any received data
//		_pkinetisk_spi->SR = SPI_SR_TCF;
        
        // Transfer a 0 out... 
        writedata8_cont(0);   
        
        // Now wait until completed. 
        wTimeout = 0xffff;
        while (((_pkinetisk_spi->SR) & (15 << 12)) && (--wTimeout)) ; // wait until empty
        r = _pkinetisk_spi->POPR;  // get the received byte... should check for it first...
    return r;
}*/
 
uint8_t Grafx_esp::readcommand8(uint8_t c, uint8_t index)
{
    // Bail if not valid miso
    if (_miso == 0xff) return 0;

	beginSPITransaction();
	writecommand_cont(0xD9);
	writedata8_cont(0x10 + index);

	writecommand_cont(c);
	setDataMode();
	uint8_t r = SPI.transfer(0);
	endSPITransaction();
	return r;
}

 static uint32_t fetchbit(const uint8_t *p, uint32_t index)
{
	if (p[index >> 3] & (1 << (7 - (index & 7)))) return 1;
	return 0;
}

static uint32_t fetchbits_unsigned(const uint8_t *p, uint32_t index, uint32_t required)
{
	uint32_t val = 0;
	do {
		uint8_t b = p[index >> 3];
		uint32_t avail = 8 - (index & 7);
		if (avail <= required) {
			val <<= avail;
			val |= b & ((1 << avail) - 1);
			index += avail;
			required -= avail;
		} else {
			b >>= avail - required;
			val <<= required;
			val |= b & ((1 << required) - 1);
			break;
		}
	} while (required);
	return val;
}

static uint32_t fetchbits_signed(const uint8_t *p, uint32_t index, uint32_t required)
{
	uint32_t val = fetchbits_unsigned(p, index, required);
	if (val & (1 << (required - 1))) {
		return (int32_t)val - (1 << required);
	}
	return (int32_t)val;
}

void Grafx_esp::drawFontChar(unsigned int c)
{
	uint32_t bitoffset;
	const uint8_t *data;

	//Serial.printf("drawFontChar(%c) %d\n", c, c);

	if (c >= font->index1_first && c <= font->index1_last) {
		bitoffset = c - font->index1_first;
		bitoffset *= font->bits_index;
	} else if (c >= font->index2_first && c <= font->index2_last) {
		bitoffset = c - font->index2_first + font->index1_last - font->index1_first + 1;
		bitoffset *= font->bits_index;
	} else if (font->unicode) {
		return; // TODO: implement sparse unicode
	} else {
		return;
	}
	//Serial.printf("  index =  %d\n", fetchbits_unsigned(font->index, bitoffset, font->bits_index));
	data = font->data + fetchbits_unsigned(font->index, bitoffset, font->bits_index);

	uint32_t encoding = fetchbits_unsigned(data, 0, 3);
	if (encoding != 0) return;
	uint32_t width = fetchbits_unsigned(data, 3, font->bits_width);
	bitoffset = font->bits_width + 3;
	uint32_t height = fetchbits_unsigned(data, bitoffset, font->bits_height);
	bitoffset += font->bits_height;
	//Serial.printf("  size =   %d,%d\n", width, height);
	//Serial.printf("  line space = %d\n", font->line_space);

	int32_t xoffset = fetchbits_signed(data, bitoffset, font->bits_xoffset);
	bitoffset += font->bits_xoffset;
	int32_t yoffset = fetchbits_signed(data, bitoffset, font->bits_yoffset);
	bitoffset += font->bits_yoffset;
	//Serial.printf("  offset = %d,%d\n", xoffset, yoffset);

	uint32_t delta = fetchbits_unsigned(data, bitoffset, font->bits_delta);
	bitoffset += font->bits_delta;
	//Serial.printf("  delta =  %d\n", delta);

	//Serial.printf("  cursor = %d,%d\n", cursor_x, cursor_y);

	 //horizontally, we draw every pixel, or none at all
	if (cursor_x < 0) cursor_x = 0;
	int32_t origin_x = cursor_x + xoffset;
	if (origin_x < 0) {
		cursor_x -= xoffset;
		origin_x = 0;
	}
	if (origin_x + (int)width > _width) {
		if (!wrap) return;
		origin_x = 0;
		if (xoffset >= 0) {
			cursor_x = 0;
		} else {
			cursor_x = -xoffset;
		}
		cursor_y += font->line_space;
	}
	if (cursor_y >= _height) return;

	// vertically, the top and/or bottom can be clipped
	int32_t origin_y = cursor_y + font->cap_height - height - yoffset;
	//Serial.printf("  origin = %d,%d\n", origin_x, origin_y);

	// TODO: compute top skip and number of lines
	int32_t linecount = height;
	//uint32_t loopcount = 0;
	int32_t y = origin_y;
	bool opaque = (textbgcolor != textcolor);


	// Going to try a fast Opaque method which works similar to drawChar, which is near the speed of writerect
	if (!opaque) {
		while (linecount > 0) {
			//Serial.printf("    linecount = %d\n", linecount);
			uint32_t n = 1;
			if (fetchbit(data, bitoffset++) != 0) {
				n = fetchbits_unsigned(data, bitoffset, 3) + 2;
				bitoffset += 3;
			}
			uint32_t x = 0;
			do {
				int32_t xsize = width - x;
				if (xsize > 32) xsize = 32;
				uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
				//Serial.printf("    multi line %d %d %x\n", n, x, bits);
				drawFontBits(opaque, bits, xsize, origin_x + x, y, n);
				bitoffset += xsize;
				x += xsize;
			} while (x < width);


			y += n;
			linecount -= n;
			//if (++loopcount > 100) {
				//Serial.println("     abort draw loop");
				//break;
			//}
		}
	} else {
		// Now opaque mode... 
		// Now write out background color for the number of rows above the above the character
		// figure out bounding rectangle... 
		// In this mode we need to update to use the offset and bounding rectangles as we are doing it it direct.
		// also update the Origin 
		int cursor_x_origin = cursor_x + _originx;
		int cursor_y_origin = cursor_y + _originy;
		origin_x += _originx;
		origin_y += _originy;



		int start_x = (origin_x < cursor_x_origin) ? origin_x : cursor_x_origin; 	
		if (start_x < 0) start_x = 0;
		
		int start_y = (origin_y < cursor_y_origin) ? origin_y : cursor_y_origin; 
		if (start_y < 0) start_y = 0;
		int end_x = cursor_x_origin + delta; 
		if ((origin_x + (int)width) > end_x)
			end_x = origin_x + (int)width;
		if (end_x >= _displayclipx2)  end_x = _displayclipx2;	
		int end_y = cursor_y_origin + font->line_space; 
		if ((origin_y + (int)height) > end_y)
			end_y = origin_y + (int)height;
		if (end_y >= _displayclipy2) end_y = _displayclipy2;	
		end_x--;	// setup to last one we draw
		end_y--;
		int start_x_min = (start_x >= _displayclipx1) ? start_x : _displayclipx1;
		int start_y_min = (start_y >= _displayclipy1) ? start_y : _displayclipy1;

		// See if anything is in the display area.
		if((end_x < _displayclipx1) ||(start_x >= _displayclipx2) || (end_y < _displayclipy1) || (start_y >= _displayclipy2)) {
			cursor_x += delta;	// could use goto or another indent level...
		 	return;
		}
/*
		Serial.printf("drawFontChar(%c) %d\n", c, c);
		Serial.printf("  size =   %d,%d\n", width, height);
		Serial.printf("  line space = %d\n", font->line_space);
		Serial.printf("  offset = %d,%d\n", xoffset, yoffset);
		Serial.printf("  delta =  %d\n", delta);
		Serial.printf("  cursor = %d,%d\n", cursor_x, cursor_y);
		Serial.printf("  origin = %d,%d\n", origin_x, origin_y);

		Serial.printf("  Bounding: (%d, %d)-(%d, %d)\n", start_x, start_y, end_x, end_y);
		Serial.printf("  mins (%d %d),\n", start_x_min, start_y_min);
*/
		#ifdef ENABLE_Grafx_FRAMEBUFFER
		if (_use_fbtft) {
			uint16_t fgcolor = FBmapColor(textcolor);
			uint16_t bgcolor = FBmapColor(textbgcolor);

			uint16_t * pfbPixel_row = &_pfbtft[ start_y*_width + start_x];
			uint16_t * pfbPixel;
			int screen_y = start_y;
			int screen_x;

			while (screen_y < origin_y) {
				pfbPixel = pfbPixel_row;
				// only output if this line is within the clipping region.
				if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
					for (screen_x = start_x; screen_x <= end_x; screen_x++) {
						if (screen_x >= _displayclipx1) {
							*pfbPixel = bgcolor;
						}
						pfbPixel++;
					}
				}
				screen_y++;
				pfbPixel_row += _width;
			}

			// Now lets process each of the data lines. 
			screen_y = origin_y;

			while (linecount > 0) {
				//Serial.printf("    linecount = %d\n", linecount);
				uint32_t b = fetchbit(data, bitoffset++);
				uint32_t n;
				if (b == 0) {
					//Serial.println("Single");
					n = 1;
				} else {
					//Serial.println("Multi");
					n = fetchbits_unsigned(data, bitoffset, 3) + 2;
					bitoffset += 3;
				}
				uint32_t bitoffset_row_start = bitoffset;
				while (n--) {
					pfbPixel = pfbPixel_row;
					if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
						bitoffset = bitoffset_row_start;	// we will work through these bits maybe multiple times

						for (screen_x = start_x; screen_x < origin_x; screen_x++) {
							if (screen_x >= _displayclipx1) {
								*pfbPixel = bgcolor;
							} // make sure not clipped
							pfbPixel++;
						}
					}

					screen_x = origin_x;
					uint32_t x = 0;
					do {
						uint32_t xsize = width - x;
						if (xsize > 32) xsize = 32;
						uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
						uint32_t bit_mask = 1 << (xsize-1);
						//Serial.printf(" %d %d %x %x\n", x, xsize, bits, bit_mask);
						if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
							while (bit_mask && (screen_x <= end_x)) {
								if ((screen_x >= _displayclipx1) && (screen_x < _displayclipx2)) {
									*pfbPixel = (bits & bit_mask) ? fgcolor : bgcolor;
								}
								pfbPixel++;	
								bit_mask = bit_mask >> 1;
								screen_x++;	// increment our pixel position. 
							}
						}
							bitoffset += xsize;
						x += xsize;
					} while (x < width);
					if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
						// output bg color and right hand side
						while (screen_x++ <= end_x) {
							*pfbPixel++ = bgcolor;
						}
					}			 
		 			screen_y++;
					pfbPixel_row += _width;
					linecount--;
				}
			}

			// clear below character
	 		while (screen_y++ <= end_y) {
				if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
					pfbPixel = pfbPixel_row;
					for (screen_x = start_x; screen_x <= end_x; screen_x++) {
						if (screen_x >= _displayclipx1) {
							*pfbPixel = bgcolor;
						}
						pfbPixel++;
					}
				}
				pfbPixel_row += _width;
			}

		} else 
		#endif
		{
			beginSPITransaction();
			//Serial.printf("SetAddr %d %d %d %d\n", start_x_min, start_y_min, end_x, end_y);
			// output rectangle we are updating... We have already clipped end_x/y, but not yet start_x/y
			setAddr( start_x_min, start_y_min, end_x, end_y);
			writecommand_cont(Grafx_RAMWR);
			int screen_y = start_y_min;
			int screen_x;
			while (screen_y < origin_y) {
				for (screen_x = start_x_min; screen_x <= end_x; screen_x++) {
					writedata16_cont(textbgcolor);
				}
				screen_y++;
			}

			// Now lets process each of the data lines. 
			screen_y = origin_y;
			while (linecount > 0) {
				//Serial.printf("    linecount = %d\n", linecount);
				uint32_t b = fetchbit(data, bitoffset++);
				uint32_t n;
				if (b == 0) {
					//Serial.println("    Single");
					n = 1;
				} else {
					//Serial.println("    Multi");
					n = fetchbits_unsigned(data, bitoffset, 3) + 2;
					bitoffset += 3;
				}
				uint32_t bitoffset_row_start = bitoffset;
				while (n--) {
					// do some clipping here. 
					bitoffset = bitoffset_row_start;	// we will work through these bits maybe multiple times
					// We need to handle case where some of the bits may not be visible, but we still need to
					// read through them
					//Serial.printf("y:%d  %d %d %d %d\n", screen_y, start_x, origin_x, _displayclipx1, _displayclipx2);
					if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
						for (screen_x = start_x; screen_x < origin_x; screen_x++) {
							if ((screen_x >= _displayclipx1) && (screen_x < _displayclipx2)) {
								//Serial.write('-');
								writedata16_cont(textbgcolor);
							}
						}
					}	
					uint32_t x = 0;
					screen_x = origin_x;
					do {
						uint32_t xsize = width - x;
						if (xsize > 32) xsize = 32;
						uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
						uint32_t bit_mask = 1 << (xsize-1);
						//Serial.printf("     %d %d %x %x - ", x, xsize, bits, bit_mask);
						if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
							while (bit_mask) {
								if ((screen_x >= _displayclipx1) && (screen_x < _displayclipx2)) {
									writedata16_cont((bits & bit_mask) ? textcolor : textbgcolor);
									//Serial.write((bits & bit_mask) ? '*' : '.');
								}
								bit_mask = bit_mask >> 1;
								screen_x++ ; // Current actual screen X
							}
							//Serial.println();
							bitoffset += xsize;
						}
						x += xsize;
					} while (x < width) ;
					if ((screen_y >= _displayclipy1) && (screen_y < _displayclipy2)) {
						// output bg color and right hand side
						while (screen_x++ <= end_x) {
							writedata16_cont(textbgcolor);
							//Serial.write('+');
						}
						//Serial.println();
					}
		 			screen_y++;
					linecount--;
				}
			}

			// clear below character - note reusing xcreen_x for this
			screen_x = (end_y + 1 - screen_y) * (end_x + 1 - start_x_min); // How many bytes we need to still output
			//Serial.printf("Clear Below: %d\n", screen_x);
			while (screen_x-- > 1) {
				writedata16_cont(textbgcolor);
			}
			writedata16_last(textbgcolor);
			endSPITransaction();
		}

	}
	// Increment to setup for the next character.
	cursor_x += delta;

}

//strPixelLen			- gets pixel length of given ASCII string
int16_t Grafx_esp::strPixelLen(char * str)
{
//	//Serial.printf("strPixelLen %s\n", str);
	if (!str) return(0);
	uint16_t len=0, maxlen=0;
	while (*str)
	{
		if (*str=='\n')
		{
			if ( len > maxlen )
			{
				maxlen=len;
				len=0;
			}
		}
		else
		{
			if (!font)
			{
				len+=textsize*6;
			}
			else
			{

				uint32_t bitoffset;
				const uint8_t *data;
				uint16_t c = *str;

//				//Serial.printf("char %c(%d)\n", c,c);

				if (c >= font->index1_first && c <= font->index1_last) {
					bitoffset = c - font->index1_first;
					bitoffset *= font->bits_index;
				} else if (c >= font->index2_first && c <= font->index2_last) {
					bitoffset = c - font->index2_first + font->index1_last - font->index1_first + 1;
					bitoffset *= font->bits_index;
				} else if (font->unicode) {
					continue;
				} else {
					continue;
				}
				//Serial.printf("  index =  %d\n", fetchbits_unsigned(font->index, bitoffset, font->bits_index));
				data = font->data + fetchbits_unsigned(font->index, bitoffset, font->bits_index);

				uint32_t encoding = fetchbits_unsigned(data, 0, 3);
				if (encoding != 0) continue;
//				uint32_t width = fetchbits_unsigned(data, 3, font->bits_width);
//				//Serial.printf("  width =  %d\n", width);
				bitoffset = font->bits_width + 3;
				bitoffset += font->bits_height;

//				int32_t xoffset = fetchbits_signed(data, bitoffset, font->bits_xoffset);
//				//Serial.printf("  xoffset =  %d\n", xoffset);
				bitoffset += font->bits_xoffset;
				bitoffset += font->bits_yoffset;

				uint32_t delta = fetchbits_unsigned(data, bitoffset, font->bits_delta);
				bitoffset += font->bits_delta;
//				//Serial.printf("  delta =  %d\n", delta);

				len += delta;//+width-xoffset;
//				//Serial.printf("  len =  %d\n", len);
				if ( len > maxlen )
				{
					maxlen=len;
//					//Serial.printf("  maxlen =  %d\n", maxlen);
				}
			
			}
		}
		str++;
	}
//	//Serial.printf("Return  maxlen =  %d\n", maxlen);
	return( maxlen );
}

void Grafx_esp::drawFontBits(bool opaque, uint32_t bits, uint32_t numbits, int32_t x, int32_t y, uint32_t repeat)
{
	if (bits == 0) {
		if (opaque) {
			fillRect(x, y, numbits, repeat, textbgcolor);
		}
	} else {
		int32_t x1 = x;
		uint32_t n = numbits;
		int w;
		int bgw;

		w = 0;
		bgw = 0;

		do {
			n--;
			if (bits & (1 << n)) {
				if (bgw>0) {
					if (opaque) {
						fillRect(x1 - bgw, y, bgw, repeat, textbgcolor);
					}
					bgw=0;
				}
				w++;
			} else {
				if (w>0) {
					fillRect(x1 - w, y, w, repeat, textcolor);
					w = 0;
				}
				bgw++;
			}
			x1++;
		} while (n > 0);

		if (w > 0) {
			fillRect(x1 - w, y, w, repeat, textcolor);
		}

		if (bgw > 0) {
			if (opaque) {
				fillRect(x1 - bgw, y, bgw, repeat, textbgcolor);
			}
		}
	}
}

