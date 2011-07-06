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
#include "glcd.h"
#include "font5x7.h"
#include "fonttable.h"

#ifdef SEVENCHRON

extern volatile uint8_t time_s, time_m, time_h;
extern volatile uint8_t old_m, old_h;
extern volatile uint8_t date_m, date_d, date_y;
extern volatile uint8_t alarming, alarm_h, alarm_m;
extern volatile uint8_t time_format;
extern volatile uint8_t region;
extern volatile uint8_t score_mode;
extern volatile uint8_t RotateFlag;

extern volatile uint8_t second_changed, minute_changed, hour_changed;

#ifdef OPTION_DOW_DATELONG
extern const uint8_t DOWText[]; 
extern const uint8_t MonthText[]; 
#endif

uint8_t last_score_mode_sev = 0;

//Prototypes:
//Called from dispatcher:
void initamin_sev(void);
void initdisplay_sev(uint8_t inverted);
void drawdisplay_sev(uint8_t inverted);
void step(void);
//Support:
void drawdot_sev(uint8_t x, uint8_t y, uint8_t inverted);
void draw7seg_sev(uint8_t x, uint8_t y, uint8_t segs, uint8_t inverted);
void drawdigit_sev(uint8_t x, uint8_t y, uint8_t d, uint8_t inverted);
void drawsegment_sev(uint8_t s, uint8_t x, uint8_t y, uint8_t inverted);
void drawvseg_sev(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t inverted);
void drawhseg_sev(uint8_t x, uint8_t y, uint8_t inverted);

void initanim_sev(void)
{
    #ifdef DEBUGF
    DEBUG(putstring("screen width: "));
    DEBUG(uart_putw_dec(GLCD_XPIXELS));
    DEBUG(putstring("\n\rscreen height: "));
    DEBUG(uart_putw_dec(GLCD_YPIXELS));
    DEBUG(putstring_nl(""));
    #endif

    initdisplay_sev(0);
}

void initdisplay_sev(uint8_t inverted)
{
    glcdFillRectangle(0, 0, GLCD_XPIXELS, GLCD_YPIXELS, inverted);
}

void printnumber_sev(uint8_t x, uint8_t num, uint8_t inverted)
{
    uint8_t temp=num&0x7F;
    if(num & 0x80)
        drawdigit_sev(DISPLAY_H10_X_SEV, DISPLAY_TIME_Y_SEV, 8, !inverted);
    else
        drawdigit_sev(x,DISPLAY_TIME_Y_SEV,temp/10,inverted);
    drawdigit_sev(
        x + DISPLAY_H1_X_SEV - DISPLAY_H10_X_SEV,
        DISPLAY_TIME_Y_SEV,
        temp % 10,
        inverted);
}

void drawdisplay_sev(uint8_t inverted) {

    if((score_mode != SCORE_MODE_TIME) && (score_mode != SCORE_MODE_ALARM))
    {
        drawdot_sev(GLCD_XPIXELS / 2, GLCD_YPIXELS * 1 / 3, !inverted);
        drawdot_sev(GLCD_XPIXELS / 2, GLCD_YPIXELS * 2 / 3, !inverted);
        drawdot_sev(GLCD_XPIXELS / 2, GLCD_YPIXELS * 1 / 10, !inverted);
    }

    if(score_mode == SCORE_MODE_YEAR)
    {
        printnumber_sev(DISPLAY_H10_X_SEV, 20, inverted);
        printnumber_sev(DISPLAY_M10_X_SEV, date_y % 100, inverted);
    }
    else if (score_mode == SCORE_MODE_DATE)
    {
        uint8_t left, right;
        if (region == REGION_US)
        {
            left = date_m;
            right = date_d;
        }
        else
        {
            left = date_d;
            right = date_m;
        }
        printnumber_sev(DISPLAY_H10_X_SEV, left, inverted);
        printnumber_sev(DISPLAY_M10_X_SEV, right, inverted);
    } 
#ifdef OPTION_DOW_DATELONG
    else if (score_mode == SCORE_MODE_DOW)
    {
        uint8_t dow = dotw(date_m, date_d, date_y);
        draw7seg_sev(DISPLAY_H10_X_SEV, DISPLAY_TIME_Y_SEV, 0x00 , inverted);
        drawdigit_sev(
            DISPLAY_H1_X_SEV,
            DISPLAY_TIME_Y_SEV,
            sdotw(dow, 0),
            inverted);
        drawdigit_sev(
            DISPLAY_M10_X_SEV,
            DISPLAY_TIME_Y_SEV,
            sdotw(dow, 1),
            inverted);
        drawdigit_sev(
            DISPLAY_M1_X_SEV,
            DISPLAY_TIME_Y_SEV,
            sdotw(dow, 2),
            inverted);
    }
    else if (score_mode == SCORE_MODE_DATELONG_MON)
    {
        draw7seg_sev(DISPLAY_H10_X_SEV, DISPLAY_TIME_Y_SEV, 0x00 , inverted);
        drawdigit_sev(
            DISPLAY_H1_X_SEV,
            DISPLAY_TIME_Y_SEV,
            smon(date_m, 0),
            inverted);
        drawdigit_sev(
            DISPLAY_M10_X_SEV,
            DISPLAY_TIME_Y_SEV,
            smon(date_m, 1),
            inverted);
        drawdigit_sev(
            DISPLAY_M1_X_SEV,
            DISPLAY_TIME_Y_SEV,
            smon(date_m, 2), inverted);
    }
    else if (score_mode == SCORE_MODE_DATELONG_DAY)
    {
        draw7seg_sev(DISPLAY_H10_X_SEV, DISPLAY_TIME_Y_SEV, 0x00, inverted);
        draw7seg_sev(DISPLAY_H1_X_SEV, DISPLAY_TIME_Y_SEV, 0x00, inverted);
        drawdigit_sev(
            DISPLAY_M10_X_SEV,
            DISPLAY_TIME_Y_SEV,
            date_d / 10,
            inverted);
        drawdigit_sev(
            DISPLAY_M1_X_SEV,
            DISPLAY_TIME_Y_SEV,
            date_d % 10,
            inverted);
    } 
#endif
    else if ((score_mode == SCORE_MODE_TIME) ||
             (score_mode == SCORE_MODE_ALARM))
    {
        // draw time or alarm
        uint8_t left, right;
        if(score_mode == SCORE_MODE_ALARM)
        {
            left = alarm_h;
            right = alarm_m;
        }
        else
        {
            left = time_h;
            right = time_m;
        }
        uint8_t am = (left < 12);
        if(time_format == TIME_12H)
        {
            left = hours(left);
            if(am)
                drawdot_sev(GLCD_XPIXELS / 2, GLCD_YPIXELS * 1 / 10, inverted);
            else
                drawdot_sev(GLCD_XPIXELS / 2, GLCD_YPIXELS * 1 / 10, !inverted);
        }
        else
            drawdot_sev(GLCD_XPIXELS / 2, GLCD_YPIXELS * 1 / 10, !inverted);
      

        // draw hours
        printnumber_sev(
            DISPLAY_H10_X_SEV,
            left | ((left < 10) ? 0x80 : 0),
            inverted);
        printnumber_sev(DISPLAY_M10_X_SEV, right, inverted);
    
        if(second_changed)
        {
            second_changed = time_s%2;
            drawdot_sev(GLCD_XPIXELS / 2, GLCD_YPIXELS * 1 / 3, second_changed);
            drawdot_sev(GLCD_XPIXELS / 2, GLCD_YPIXELS * 2 / 3, second_changed);
            second_changed = 0;
        }
    }
}


void step_sev(void)
{
    if(!RotateFlag || (minute_changed == 2) || (hour_changed == 2))
        minute_changed = hour_changed = 0;
}

void drawdot_sev(uint8_t x, uint8_t y, uint8_t inverted)
{
    glcdFillCircle(x, y, DOTRADIUS, !inverted);
}

void draw7seg_sev(uint8_t x, uint8_t y, uint8_t segs, uint8_t inverted)
{
    for(uint8_t i=0;i<7;i++)
    {
        if(segs & (1 << (7 - i)))
            drawsegment_sev(i, x, y, inverted);
        else
            drawsegment_sev(i, x, y, !inverted);
    }
}

void drawdigit_sev(uint8_t x, uint8_t y, uint8_t d, uint8_t inverted)
{
    if(d < 10)
        draw7seg_sev(x, y, eeprom_read_byte(&numbertable[d]), inverted);
    else if ((d >= 'a') || (d <= 'z'))
        draw7seg_sev(x, y, eeprom_read_byte(&alphatable[(d - 'a')]), inverted);
    else
        draw7seg_sev(x, y, 0x00, inverted);
}

#define SEGMENT_HORIZONTAL_SEV 0
#define SEGMENT_VERTICAL_SEV 1
uint8_t seg_location_sev[7][3] = {
    {SEGMENT_HORIZONTAL_SEV, VSEGMENT_W/2+1,0},
    {SEGMENT_VERTICAL_SEV, HSEGMENT_W + 2, HSEGMENT_H / 2 + 2},
    {SEGMENT_VERTICAL_SEV, HSEGMENT_W + 2, GLCD_YPIXELS / 2 + 2},
    {SEGMENT_HORIZONTAL_SEV, VSEGMENT_W / 2 + 1, GLCD_YPIXELS - HSEGMENT_H},
    {SEGMENT_VERTICAL_SEV, 0, GLCD_YPIXELS / 2 + 2},
    {SEGMENT_VERTICAL_SEV, 0, HSEGMENT_H / 2 + 2},
    {SEGMENT_HORIZONTAL_SEV,
     VSEGMENT_W / 2 + 1,
     (GLCD_YPIXELS - HSEGMENT_H) / 2},
};

void drawsegment_sev(uint8_t s, uint8_t x, uint8_t y, uint8_t inverted)
{
    if(!seg_location_sev[s][0])
        drawvseg_sev(
            x + seg_location_sev[s][1],
            y + seg_location_sev[s][2],
            HSEGMENT_W,
            HSEGMENT_H,
            inverted);
    else
        drawvseg_sev(
            x + seg_location_sev[s][1],
            y + seg_location_sev[s][2],
            VSEGMENT_W,
            VSEGMENT_H,
            inverted);
}


void drawvseg_sev(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t inverted) {
    uint8_t i;
    for(i = 0; i < 3; i++)
    {
        glcdFillRectangle(
            x + i,
            y + 2 - i,
            w - (i * 2),
            h - 4 + (i * 2),
            !inverted);
    }
}

#endif
