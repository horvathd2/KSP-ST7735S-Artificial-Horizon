#ifndef __HORIZON_H_
#define __HORIZON_H_

#include "st7735s.h"

#define TABLE_SIZE          630
#define MAX_NAVBALL_POINTS  500

#define COLOR565_GRAY   0x8080
#define COLOR565_BLACK  0x0000

#define cx      64
#define cy      80
#define radius  60

#define FB_WIDTH   128
#define FB_HEIGHT  160

#define DEG_TO_RAD  0.017453292519943295
#define PI		    3.141592653589793238
#define STEP_RAD    (2 * PI / TABLE_SIZE)

extern const float sin_table[TABLE_SIZE];
extern const float cos_table[TABLE_SIZE];

float fsin(float rad);

float fcos(float rad);

void fb_clear(uint16_t color);

uint16_t* horizon_get_framebuffer(void);

void draw_navball(float pitch_deg, float roll_deg, float yaw_deg);

void framebuffer_draw_circle(uint8_t rad, 
                             uint16_t X0, uint16_t Y0, 
                             uint16_t color);

/*

int project(float x, float y, 
            float z, int *sx, 
            int *sy);

void rotate(float *x, float *y, 
            float *z, float pitch, 
            float roll, float yaw);

void drawNavball(st7735s_t *lcd, 
                 float pitchDeg, 
                 float rollDeg, 
                 float yawDeg);

*/

#endif