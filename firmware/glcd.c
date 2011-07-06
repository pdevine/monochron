/*! \file glcd.c \brief Graphic LCD API functions. */
//*****************************************************************************
//
// File Name    : 'glcd.c'
// Title        : Graphic LCD API functions
// Author        : Pascal Stang - Copyright (C) 2002
// Date            : 5/30/2002
// Revised        : 5/30/2002
// Version        : 0.5
// Target MCU    : Atmel AVR
// Editor Tabs    : 4
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// This code is distributed under the GNU Public License
//        which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef WIN32
// AVR specific includes
    #include <avr/io.h>
    #include <avr/pgmspace.h>
    #include <avr/eeprom.h>
#endif

#include "glcd.h"

// include hardware support
#include "ks0108.h"
// include fonts
#include "font5x7.h"
#include "fontgr.h"

#include "util.h"




// graphic routines

// set dot
void glcdSetDot(u08 x, u08 y)
{
    unsigned char temp;

    //putstring("->addr "); uart_putw_dec(x);
    //putstring(", "); uart_putw_dec(y/8);
    //putstring_nl(")");

    glcdSetAddress(x, y/8);
    temp = glcdDataRead();    // dummy read
    temp = glcdDataRead();    // read back current value
    glcdSetAddress(x, y/8);
    glcdDataWrite(temp | (1 << (y % 8)));
    glcdStartLine(0);
}

// draw line
void glcdLine(u08 x1, u08 y1, u08 x2, u08 y2, u08 color)
{
    unsigned char temp;

    int err;
    int err2;
    int sx = -1;
    int sy = -1;

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    if(x1 < x2)
        sx = 1;

    if(y1 < y2)
        sy = 1;

    err = dx - dy;

    while(1)
    {
        glcdSetAddress(x1, y1 / 8);
        temp = glcdDataRead();    // dummy read
        temp = glcdDataRead();    // read back current value
        glcdSetAddress(x1, y1 / 8);

        if (color == ON)
            glcdDataWrite(temp | (1 << (y1 % 8)));
        else
            glcdDataWrite(temp & (1 << (y1 % 8)));

        if(x1 == x2 && y1 == y2)
            break;

        err2 = 2 * err;
        if(err2 > -dy)
        {
            err = err - dy;
            x1 = x1 + sx;
        }

        if(err2 < dx)
        {
            err = err + dx;
            y1 = y1 + sy;
        }
    }

    glcdStartLine(0);
}

// draw filled rectangle
void glcdFillRectangle(u08 x, u08 y, u08 a, u08 b, u08 color)
{
    unsigned char i, j, temp;
    signed char k;

    if (y % 8) {
        for (i = 0; i < a; i++) {
            glcdSetAddress(x + i, y / 8);
            temp = glcdDataRead();    // dummy read
            temp = glcdDataRead();    // read back current value
            // not on a perfect boundary
            for (k = (y % 8); k < (y % 8) + b && (k < 8); k++) {
                if (color == ON)
                    temp |= _BV(k);
                else
                    temp &= ~_BV(k);
            }
            glcdSetAddress(x + i, y / 8);
            glcdDataWrite(temp);
        } 
        // we did top section so remove it

        if (b > 8 - (y % 8))
            b -= 8 - (y % 8);
        else 
            b = 0;
        y -= (y % 8);
        y += 8;
    }
    // skip to next section
    for (j = (y / 8); j < (y + b) / 8; j++) {
        glcdSetAddress(x, j);
        for (i = 0; i < a; i++) {
            if (color == ON)
                glcdDataWrite(0xFF);
            else
                glcdDataWrite(0x00);
        }
    }
    b = b % 8;
    // do remainder
    if (b) {
        for (i = 0; i < a; i++) {
            glcdSetAddress(x + i, j);
            temp = glcdDataRead();    // dummy read
            temp = glcdDataRead();    // read back current value
            // not on a perfect boundary
            for (k = 0; k < b; k++) {
                if (color == ON)
                    temp |= _BV(k);
                else
                    temp &= ~_BV(k);
            }
            glcdSetAddress(x + i, j);
            glcdDataWrite(temp);
        }
    }
    glcdStartLine(0);
}


// draw circle
void glcdFillCircle(u08 xcenter, u08 ycenter, u08 radius, u08 color)
{
    int tswitch, y, x = 0;
    unsigned char d;

    d = ycenter - xcenter;
    y = radius;
    tswitch = 3 - 2 * radius;
    while (x <= y) {
        glcdFillRectangle(xcenter + x, ycenter - y, 1, y*2, color);
        glcdFillRectangle(xcenter - x, ycenter - y, 1, y*2, color);
        glcdFillRectangle(ycenter + y - d, ycenter - x, 1, x*2, color);
        glcdFillRectangle(ycenter - y - d, ycenter - x, 1, x*2, color);   

        if (tswitch < 0)
            tswitch += (4 * x + 6);
        else {
            tswitch += (4 * (x - y) + 10);
            y--;
        }

        x++;
    }
}

// text routines

// write a character at the current position
void glcdWriteChar(unsigned char c, uint8_t inverted)
{
    u08 i = 0, j;

    for(i = 0; i < 5; i++)
    {
        j = get_font(((c - 0x20) * 5) + i);
        if (inverted)
            j = ~j;
        glcdDataWrite(j);
    }

    // write a spacer line
    if (inverted) 
        glcdDataWrite(0xFF);
    else 
        glcdDataWrite(0x00);

    glcdStartLine(0);
}

void glcdWriteCharGr(u08 grCharIdx, uint8_t inverted)
{
    u08 idx;
    u08 grLength;
    u08 grStartIdx = 0;
    u08 line;

    // get starting index of graphic bitmap
    for(idx = 0; idx < grCharIdx; idx++)
    {
        // add this graphic's length to the startIdx
        // to get the startIdx of the next one
        // 2010-03-03 BUG Dataman/CRJONES There's a bug here: 
        //   Have to add 1 for the byte-cout.
        // grStartIdx += pgm_read_byte(FontGr+grStartIdx);
        grStartIdx += pgm_read_byte(&FontGr[grStartIdx])+1;
    }
    grLength = pgm_read_byte(&FontGr[grStartIdx]);

    // write the lines of the desired graphic to the display
    for(idx = 0; idx < grLength; idx++)
    {
        // write the line
        line = pgm_read_byte(&FontGr[(grStartIdx + 1) + idx]);
        if (inverted == INVERTED)
            line = 255 - line;
        glcdDataWrite(line);
    }
}

void glcdPutStr_ram(char *data, uint8_t inverted)
{
    while (*data) {
        glcdWriteChar(*data, inverted);
        data++;
    }
}

void glcdPutStr_rom(const char *data, uint8_t inverted)
{
    uint8_t i;

    for (i = 0; pgm_read_byte(&data[i]); i++) {
        glcdWriteChar(pgm_read_byte(&data[i]), inverted);
    }
}

uint8_t get_font(uint16_t addr)
{
    return eeprom_read_byte(&Font5x7[addr]);
}
