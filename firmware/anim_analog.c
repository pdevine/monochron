/* ***************************************************************************
// anim.c - the main animation and drawing code for MONOCHRON
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
**************************************************************************** */

#include <avr/io.h>      // this contains all the IO port definitions
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
 
#include "util.h"
#include "ratt.h"
#include "ks0108.h"
#include "ks0108conf.h"
#include "glcd.h"

#ifdef ANALOGCHRON

// Routines called by dispatcher
void initanim_analog(void);                  // Initialzes Animation
void initdisplay_analog(uint8_t);            // Intializes Display
void step_analog(void);                      // Update
void drawdisplay_analog(uint8_t);            // Draw
void settime_analog(uint8_t);                // Updates Time

// Local Routines
void WriteTime_analog(uint8_t);
int sin60(int angle);
int cos60(int angle);

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t old_m, old_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarming, alarm_h, alarm_m;
extern volatile uint8_t time_format;
extern volatile uint8_t region;
extern volatile uint8_t score_mode;
extern volatile uint8_t second_changed, minute_changed, hour_changed;

//uint8_t digitsmutex_int = 0;
uint8_t wasalarming = 0; // flag to indicate resetting bases from reverse video is required

uint16_t sin60_table[] =
    {    0,   107,   213,   316,   416,   512,
       602,   685,   761,   828,   887,   935,
       974,  1002,  1018,  1024,  1018,  1002,
       974,   935,   887,   828,   761,   685,
       602,   512,   416,   316,   213,   107,
    };

int sin60(int angle)
{
    if(!angle)
        return 0;

    if(angle >= 180)
        return -sin60(angle - 180);
    if(angle < 0)
        return -sin60(abs(angle));

    return sin60_table[(int) angle / 6];
}

int cos60(int angle)
{
    return sin60(angle + 90);
}



void initanim_analog(void) {

    #ifdef DEBUGF
        DEBUG(putstring("screen width: "));
        DEBUG(uart_putw_dec(GLCD_XPIXELS));
        DEBUG(putstring("\n\rscreen height: "));
        DEBUG(uart_putw_dec(GLCD_YPIXELS));
        DEBUG(putstring_nl(""));
    #endif

    initdisplay_analog(0);
 }

void initdisplay_analog(uint8_t inverted)
{
    // clear screen
    glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted);

    // get time & display
    WriteTime_analog(inverted);
}

void WriteTime_analog(uint8_t inverted)
{
 	 glcdSetAddress(3,0);
	 printnumber(time_m, inverted);
     glcdSetAddress(102,0);
	 printnumber(time_s, inverted);
}

void setscore_analog(uint8_t inverted)
{
    WriteTime_analog(inverted);
}

void step_analog(void)
{
}

void drawdisplay_analog(uint8_t inverted)
{
    static uint8_t last_s;
    int a;

    // clear the second hand
    if(last_s != time_s)
    {
        glcdClearScreen();

        for(a = 0; a <= 360; a += 90)
        {
            glcdLine(
                cos60(a) * 25 / 1024 + (GLCD_XPIXELS / 2),
                sin60(a) * 25 / 1024 + (GLCD_YPIXELS / 2),
                cos60(a) * 30 / 1024 + (GLCD_XPIXELS / 2),
                sin60(a) * 30 / 1024 + (GLCD_YPIXELS / 2),
                !inverted);
        }

        glcdLine(
            GLCD_XPIXELS / 2,
            GLCD_YPIXELS / 2,
            cos60((time_s - 15) * 6) * 28 / 1024 + (GLCD_XPIXELS / 2),
            sin60((time_s - 15) * 6) * 28 / 1024 + (GLCD_YPIXELS / 2),
            !inverted);

        glcdLine(
            GLCD_XPIXELS / 2,
            GLCD_YPIXELS / 2,
            cos60((time_m - 15) * 6) * 28 / 1024 + (GLCD_XPIXELS / 2),
            sin60((time_m - 15) * 6) * 28 / 1024 + (GLCD_YPIXELS / 2),
            !inverted);

        // hours are calculated by the hour plus the fraction of the minutes
        glcdLine(
            GLCD_XPIXELS / 2,
            GLCD_YPIXELS / 2,
            cos60((time_h - 15) * 30 + 
                   (time_m * 6 / 10)) * 14 / 1024 + (GLCD_XPIXELS / 2),
            sin60((time_h - 15) * 30 +
                   (time_m * 6 / 10)) * 14 / 1024 + (GLCD_YPIXELS / 2),
            !inverted);

        last_s = time_s;
    }


}

#endif
