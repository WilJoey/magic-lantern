#ifndef _lens_h_
#define _lens_h_
/** \file
 * Lens and camera control
 *
 * These are Magic Lantern specific structures that control the camera's
 * shutter speed, ISO and lens aperture.  It also records the focal length,
 * distance and other parameters by hooking the different lens related
 * properties.
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "property.h"

int get_htp();

struct lens_info
{
volatile	void *			token;
volatile	char 			name[ 32 ];
volatile	unsigned		focal_len; // in mm
volatile	unsigned		focus_dist; // in cm
volatile	unsigned		aperture;
volatile	int				ae;        // exposure compensation, in 1/8 EV steps, signed
volatile	unsigned		shutter;
volatile	unsigned		iso;
volatile	unsigned		iso_auto;
volatile	unsigned		hyperfocal; // in mm
volatile	unsigned		dof_near; // in mm
volatile	unsigned		dof_far; // in mm
volatile	unsigned		job_state; // see PROP_LAST_JOB_STATE

volatile	unsigned		wb_mode;  // see property.h for possible values
volatile	unsigned		kelvin;   // wb temperature; only used when wb_mode = WB_KELVIN
volatile	int8_t		wbs_gm;
volatile	int8_t		wbs_ba;

volatile	unsigned		picstyle; // 1 ... 9: std, portrait, landscape, neutral, faithful, monochrome, user 1, user 2, user 3
/*	int32_t 		contrast;   // -4..4
	uint32_t		sharpness;  // 0..7
	uint32_t		saturation; // -4..4
	uint32_t		color_tone; // -4..4 */

	// Store the raw values before the lookup tables
volatile	uint8_t			raw_aperture;
volatile	uint8_t			raw_shutter;
volatile	uint8_t			raw_iso;
volatile	uint8_t			raw_iso_auto;
volatile	uint8_t			raw_picstyle;

volatile	float 			lens_rotation;
volatile	float			lens_step;
};

extern struct lens_info lens_info;


struct prop_lv_lens
{
	uint32_t		lens_rotation; // float in little-endian actually
	uint32_t		lens_step; // float in little-endian actually
	uint32_t		off_0x08;
	uint32_t		off_0x0c;
	uint32_t		off_0x10;
	uint32_t		off_0x14;
	uint32_t		off_0x18;
	uint32_t		off_0x1c;
	uint32_t		off_0x20;
	uint32_t		off_0x24;
	uint32_t		off_0x28;
	uint16_t		focal_len;	// off_0x2c;
	uint16_t		focus_dist;	// off_0x2e;
	uint32_t		off_0x30;
	uint32_t		off_0x34;
	uint16_t		off_0x38;
} __attribute__((packed));

SIZE_CHECK_STRUCT( prop_lv_lens, 58 );


struct prop_focus
{
	uint8_t			active;		// off_0x00, must be 1
	uint8_t			step_hi;	// off_0x01
	uint8_t			step_lo;	// off_0x02
	uint8_t			mode;		// off_0x03 unknown, usually 7?
	uint8_t			unk;
} __attribute__((packed));

SIZE_CHECK_STRUCT( prop_focus, 5 );

struct prop_picstyle_settings
{
	int32_t 	contrast;   // -4..4
	uint32_t	sharpness;  // 0..7
	int32_t		saturation; // -4..4
	int32_t		color_tone; // -4..4
	uint32_t	off_0x10;   // deadbeaf?!
	uint32_t	off_0x14;   // deadbeaf?!
} __attribute__((aligned,packed));  

SIZE_CHECK_STRUCT( prop_picstyle_settings, 0x18 );

void lens_wait_readytotakepic(int wait);

// return true on success
extern bool lens_set_rawaperture( int aperture);
extern bool lens_set_rawiso( int iso );
extern bool lens_set_rawshutter( int shutter );
extern void lens_set_ae( int ae );
extern void lens_set_drivemode( int dm );
extern void lens_set_wbs_gm(int value);
extern void lens_set_wbs_ba(int value);

extern void bv_update();
extern bool bv_set_rawshutter(unsigned shutter);
extern bool bv_set_rawaperture(unsigned aperture);
extern bool bv_set_rawiso(unsigned iso);

extern int
lens_take_picture(
	int		wait, 
	int allow_af
);


/** Will block if it is not safe to send the focus command */
extern int
lens_focus(
	int num_steps, 
	int stepsize, 
	int wait,
	int extra_delay
);

/** Wait for the last command to complete */
extern void
lens_focus_wait( void );

/** Start the lens focus task */
extern void
lens_focus_start( int dir );

/** Stop the lens focus task */
extern void
lens_focus_stop( void );

/** Format a distance in mm into something useful */
extern const char *
lens_format_dist(
	unsigned		mm
);

#define KELVIN_MIN 1500
#define KELVIN_MAX 15000
#define KELVIN_STEP 100

// exact ISO values would break the feature of coloring ISO's :)
// sprintf("%d,", round(12800 ./ 2.^([56:-1:0]./8)))
                               //~ 100,109,119,130,141,154,168,183,200,218,238,259,283,308,336,367,400,436,476,519,566,617,673,734,800,872,951,1037,1131,1234,1345,1467,1600,1745,1903,2075,2263,2468,2691,2934,3200,3490,3805,4150,4525,4935,5382,5869,6400,6979,7611,12800,25600};
static const int values_iso[] = {0,100,110,115,125,140,160,170,185,200,220,235,250,280,320,350,380,400,435,470,500,580,640,700,750,800,860,930,1000,1100,1250,1400,1500,1600,1750,1900,2000,2250,2500,2750,3000,3200,3500,3750,4000,4500,5000,5500,6000,6400,12800,25600};
static const int codes_iso[]  = {0, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,  128,  136}; 

// assumming 1/4000 is native, and 1/8EV steps =>
// octave> sprintf("%4d,", round(4000 ./ 2.^([59:-1:-8]./8)))
// octave> sprintf("%4d,", (1:68) + 160 - 68)
// exact values:
//~ static const int values_shutter[] = {0, 24,  26,  29,  31,  34,  37,  41,  44,  48,  53,  57,  63,  68,  74,  81,  88,  96, 105, 115, 125, 136, 149, 162, 177, 193, 210, 229, 250, 273, 297, 324, 354, 386, 420, 459, 500, 545, 595, 648, 707, 771, 841, 917,1000,1091,1189,1297,1414,1542,1682,1834,2000,2181,2378,2594,2828,3084,3364,3668,4000,4362,4757,5187,5657,6169,6727,7336,8000};
// rounded to 2 digits:
static const int values_shutter[] = {0, 25,  26,  29,  31,  34,  37,  41,  44,  48,  53,  57,  63,  68,  74,  81,  88,  96, 110, 120, 130, 140, 150, 160, 180, 190, 210, 230, 250, 270, 300, 320, 350, 390, 420, 460, 500, 560, 600, 650, 710, 770, 840, 920,1000,1100,1200,1300,1400,1500,1700,1800,2000,2200,2300,2600,2800,3000,3400,3700,4000,4400,4800,5200,5600,6200,6800,7300,8000};
static const int codes_shutter[]  = {0, 93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160};

// aperture*10
static const int values_aperture[] = {0,  10,  11,  11,  12,  12,  13,  14,  14,  15,  16,  16,  17,  18,  19,  20,  20,  21,  22,  23,  24,  25,  27,  28,  29,  30,  32,  33,  35,  36,  38,  40,  41,  43,  45,  47,  49,  51,  54,  56,  59,  61,  64,  67,  70,  73,  76,  80,  83,  87,  91,  95,  99, 103, 108, 113, 118, 123, 128, 134, 140, 146, 153, 160, 167, 174, 182, 190, 198, 207, 216, 226, 236, 246, 257, 269, 281, 293, 306, 320, 334, 348, 364, 380, 397, 414, 433, 452};
static const int codes_aperture[] =  {0,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96};

#define SWAP_ENDIAN(x) (((x)>>24) | (((x)<<8) & 0x00FF0000) | (((x)>>8) & 0x0000FF00) | ((x)<<24))

void draw_ml_topbar();
void draw_ml_bottombar(int double_buffering, int clear);

#endif
