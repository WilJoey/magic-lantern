/** \file
 * Onscreen audio meters
 */
#include "dryos.h"
#include "bmp.h"

/** Read the raw level from the audio device.
 *
 * Expected values are signed 16-bit?
 */
static inline int16_t
audio_read_level( int channel )
{
	uint32_t *audio_level = (uint32_t*)( 0xC0920000 + 0x110 );
	return (int16_t) audio_level[channel];
}

struct audio_level
{
	int		last;
	int		avg;
	int		peak;
};


struct audio_level audio_levels[2] = { {}, {} };



/** Returns a dB translated from the raw level
 *
 * Range is -40 to 0 dB
 */
static int
audio_level_to_db(
	int			raw_level
)
{
	int db;

	for( db = 40 ; db ; db-- )
	{
		if( audio_thresholds[db] > raw_level )
			return -db;
	}

	return 0;
}




#ifdef OSCOPE_METERS
void draw_meters(void)
{
#define MAX_SAMPLES 720
	static int16_t levels[ MAX_SAMPLES ];
	static uint32_t index;
	levels[ index++ ] = audio_read_level();
	if( index >= MAX_SAMPLES )
		index = 0;


	struct vram_info * vram = &vram_info[ vram_get_number(2) ];
	//thunk audio_dev_compute_average_level = (void*) 0xFF9725C4;
	//audio_dev_compute_average_level();

	// The level goes from -40 to 0
	uint32_t x;
	for( x=0 ; x<MAX_SAMPLES && x<vram->width ; x++ )
	{
		uint16_t y = 256 + levels[ x ] / 128;
		vram->vram[ y * vram->pitch + x ] = 0xFFFF;
	}

	uint32_t y;
	for( y=0 ; y<128 ; y++ )
	{
		vram->vram[ y * vram->pitch + index ] = 0x888F;
	}

}
#else


static uint8_t
db_to_color(
	int			db
)
{
	if( db < -35 )
		return 0x2F; // white
	if( db < -20 )
		return 0x06; // dark green
	if( db < -15 )
		return 0x0F; // yellow
	return 0x0c; // dull red
}

static uint8_t
db_peak_to_color(
	int			db
)
{
	if( db < -35 )
		return 0x7f; // dark blue
	if( db < -20 )
		return 0x07; // bright green
	if( db < -15 )
		return 0xAE; // bright yellow
	return 0x08; // bright red
}

// Transparent black
static const uint8_t bg_color = 0x03;


static void
draw_meter(
	int			y,
	struct audio_level *	level
)
{
	const uint32_t width = bmp_width();
	const uint32_t pitch = bmp_pitch();
	uint32_t * row = (uint32_t*) bmp_vram();
	row += (pitch/4) * y;

	const int db_avg = audio_level_to_db( level->avg );
	const int db_peak = audio_level_to_db( level->peak );

	// levels go from -40 to 0, so -40 * 16 == 640
	const uint32_t x_db_avg = (width + db_avg * 16) / 4;
	const uint32_t x_db_peak = (width + db_peak * 16) / 4;

	const uint8_t bar_color = db_to_color( db_avg );
	const uint8_t peak_color = db_peak_to_color( db_peak );
	const int meter_height = 12;

	const uint32_t bar_color_word = 0
		| (bar_color << 24)
		| (bar_color << 16)
		| (bar_color <<  8)
		| (bar_color <<  0);

	const uint32_t peak_color_word = 0
		| (peak_color << 24)
		| (peak_color << 16)
		| (peak_color <<  8)
		| (peak_color <<  0);

	const uint32_t bg_color_word = 0
		| (bg_color << 24)
		| (bg_color << 16)
		| (bg_color <<  8)
		| (bg_color <<  0);

	// Write the meter an entire scan line at a time
	for( y=0 ; y<meter_height ; y++, row += pitch/4 )
	{
		uint32_t x;
		for( x=0 ; x<width/4 ; x++ )
		{
			if( x < x_db_avg )
				row[x] = bar_color_word;
			else
			if( x < x_db_peak )
				row[x] = bg_color_word;
			else
			if( x < x_db_peak + 10 )
				row[x] = peak_color_word;
			else
				row[x] = bg_color_word;
		}
	}
}


static void
draw_ticks(
	int		y,
	int		tick_height
)
{
	const uint32_t width = bmp_width();
	const uint32_t pitch = bmp_pitch();
	uint32_t * row = (uint32_t*) bmp_vram();
	row += (pitch/4) * y;

	for( ; tick_height > 0 ; tick_height--, row += pitch/4 )
	{
		int db;
		for( db=-40 * 8; db<= 0 ; db+=5*8 )
		{
			const uint32_t x_db = width + db * 2;
			row[x_db/4] = 0x01010101;
		}
	}
}

/* Normal VU meter */
static void draw_meters(void)
{
	// The db values are multiplied by 8 to make them
	// smoother.
	draw_meter( 0, &audio_levels[0] );
	draw_ticks( 12, 4 );
	draw_meter( 16, &audio_levels[1] );
}

#endif


/** Draw transparent crop marks
 *  And draw the 16:9 crop marks for full time
 * The screen is 480 vertical lines, but we only want to
 * show 720 * 9 / 16 == 405 of them.  If we use this number,
 * however, things don't line up right.
 */
void
draw_matte( void )
{
	const uint32_t width	= 720;

	bmp_fill( 0x01, 0, 32, width, 1 );
	bmp_fill( 0x01, 0, 390, width, 1 );
	//bmp_fill( bg_color, 0, 0, width, 32 );
	//bmp_fill( bg_color, 0, 390, width, 430 - 390 );
}


static void
draw_zebra( void )
{
	struct vram_info * vram = &vram_info[ vram_get_number(2) ];

/*
	static int written;
	if( !written )
		write_debug_file( "vram.yuv", vram->vram, vram->height * vram->pitch * 2 );
	written = 1;
*/

	uint32_t x,y;

	const uint8_t zebra_color_0 = 0x6F; // bright read
	const uint8_t zebra_color_1 = 0x5F; // dark red

	// For unused contrast detection algorithm
	//const uint8_t contrast_color = 0x0D; // blue
	//const uint16_t threshold = 0xF000;

	// skip the audio meter at the top and the bar at the bottom
	// hardcoded; should use a constant based on the type of display
	for( y=33 ; y < 390; y++ )
	{
		uint32_t * const v_row = (uint32_t*)( vram->vram + y * vram->pitch );
		uint16_t * const b_row = (uint16_t*)( bmp_vram() + y * bmp_pitch() );

		for( x=0 ; x < vram->width ; x+=2 )
		{
			uint32_t pixels = v_row[x/2];
#if 0
			uint16_t pixel0 = (pixels >> 16) & 0xFFFF;
			uint16_t pixel1 = (pixels >>  0) & 0xFFFF;

			// Check for contrast
			// This doesn't work very well, so I have it
			// compiled out for now.
			if( (pixel0 > pixel1 && pixel0 - pixel1 > 0x4000 )
			||  (pixel0 < pixel1 && pixel1 - pixel0 > 0x4000 )
			)
			{
				b_row[x/2] = (contrast_color << 8) | contrast_color;
				continue;
			}
#endif

			// If neither pixel is overexposed, ignore it
			if( (pixels & 0xF000F000) != 0xF000F000 )
			{
				b_row[x/2] = 0;
				continue;
			}

			// Determine if we are a zig or a zag line
			uint32_t zag = ((y >> 3) ^ (x >> 3)) & 1;

			// Build the 16-bit word to write both pixels
			// simultaneously into the BMP VRAM
			uint16_t zebra_color_word = zag
				? (zebra_color_0<<8) | (zebra_color_0<<0)
				: (zebra_color_1<<8) | (zebra_color_1<<0);

			b_row[x/2] = zebra_color_word;
		}
	}
}


static int
my_gui_task(
	void *			UNUSED( arg ),
	gui_event_t		UNUSED( event ),
	int			UNUSED( arg2 ),
	int			UNUSED( arg3 )
)
{
	draw_zebra();
	draw_matte();
	draw_meters();
	return 1;
}


extern struct event gui_events[];
extern int gui_events_index;

#if 0
static void
draw_events( void )
{
	bmp_printf( 0, 10, "A/V jack: %s",
		camera_engine.av_jack & 1 ? "No " : "Yes"
	);

	bmp_hexdump( 0, 30, &camera_engine, 32 );

	unsigned i;
	for( i=0 ; i<16 ; i++ )
	{
		struct event * event = &gui_events[ i ];
		bmp_printf( 0, 200 + font_height * i,
			"%sEvent %x: %x %08x %08x %08x\n",
			i == gui_events_index ? "->" : "  ",
			i,
			(unsigned) event->type,
			(unsigned) event->param,
			(unsigned) event->obj,
			(unsigned) event->arg
		);
	}
}
#endif

static void
draw_audio_regs( void )
{
	int y = 100, reg;
	for( reg=0x20 ; reg < 0x3F ; reg += 8, y += font_height )
	{
		//DebugMsg( DM_MAGIC, 3,
		bmp_printf( 100, y,
			"audio reg %02x: %02x %02x %02x %02x  %02x %02x %02x %02x",
			reg,
			audio_ic_read( (reg+0) << 8 ),
			audio_ic_read( (reg+1) << 8 ),
			audio_ic_read( (reg+2) << 8 ),
			audio_ic_read( (reg+3) << 8 ),
			audio_ic_read( (reg+4) << 8 ),
			audio_ic_read( (reg+5) << 8 ),
			audio_ic_read( (reg+6) << 8 ),
			audio_ic_read( (reg+7) << 8 )
		);
	}

	//bmp_printf( 100, y, "Volume: %08
	bmp_hexdump( 100, y, (uint32_t*)( 0xC0920000 + 0x110 ), 16 );
}



static void
compute_audio_levels(
	int ch
)
{
	struct audio_level * const level = &audio_levels[ch];

	int raw = audio_read_level( ch );
	if( raw < 0 )
		raw = -raw;

	level->last	= raw;
	level->avg	= (level->avg * 3 + raw) / 4;
	if( raw > level->peak )
		level->peak = raw;

	// Decay the peak to the average
	level->peak = ( level->peak + level->avg ) / 2;
}


/** Task to monitor the audio levels.
 *
 * Compute the average and peak level, periodically calling
 * the draw_meters() function to display the results on screen.
 */
static void
my_task( void )
{
	// Overwrite the PTPCOM message
	dm_names[ DM_MAGIC ] = "[MAGIC] ";
	DebugMsg( DM_MAGIC, 3, "!!!!! User task is running" );
	dm_set_store_level( DM_MOVR, 7 );
	uint32_t * movw_struct = (void*) 0x1ed4;
	uint8_t mode = movw_struct[3] & 0xFF;
	dm_set_store_level( mode, 7 );
	DebugMsg( DM_MAGIC, 3, "store dm %d", mode );

	msleep( 4000 );

	//sounddev_active_in(0,0);
	//sounddev_active_out(0,0);

	//FA_StartLiveView();
	call( "FA_StartLiveView" );

	//gui_task_create( my_gui_task, 0x9999 );
	//msleep( 500 );
	//dialog_create( 0, 0, my_gui_task, 0xC0, 0 );

	//msleep( 500 );
	//call( "checkdraftvram" );
	//msleep( 500 );
	//dumpf();

	int do_disp_check = 0;

	while(1)
	{
		msleep( 120 );
		compute_audio_levels( 0 );
		compute_audio_levels( 1 );

		if( gui_events[ gui_events_index ].type == 0
		&&  gui_events[ gui_events_index ].param == 0x13
		)
			do_disp_check++;


		//winsys_take_semaphore();
		//take_semaphore( hdmi_config.bmpddev_sem, 0 );

		//bmp_hexdump( 10, 200, bmp_vram_info, 64 );
		//uint32_t x, y, w, h;
		//vram_image_pos_and_size( &x, &y, &w, &h );
		//bmp_printf( 100, 200, "vram_info: %dx%d %dx%d", x, y, w, h );

		//bmp_hexdump( 1, 40, (void*) 0x2580, 64 );

		//bmp_hexdump( 1, 40, &winsys_struct, 32 );
		//bmp_hexdump( 1, 200, winsys_struct.vram_object, 32 );

		//give_semaphore( hdmi_config.bmpddev_sem );

		if( do_disp_check == 1 )
		{
			dumpf();
		}

		//draw_zebra();
		draw_meters();
		draw_audio_regs();
		//draw_text_state();
	}
}

/** Replace the sound dev task with our own to disable AGC.
 *
 * This task disables the AGC when the sound device is activated.
 */
void
my_sounddev_task( void )
{
	//void * file = FIO_CreateFile( "A:/snddev.log" );
	//FIO_WriteFile( file, sounddev, sizeof(*sounddev) );
	//FIO_CloseFile( file );

	DebugMsg( DM_AUDIO, 3, "!!!!! %s started sem=%x", __func__, (uint32_t) sounddev.sem_alc );

	sounddev.sem_alc = create_named_semaphore( 0, 0 );

	sounddev_active_in(0,0);

	while(1)
	{
		msleep( 1000 );

		audio_ic_write( AUDIO_IC_PM1 | 0x6D ); // power up ADC and DAC
		audio_ic_write( AUDIO_IC_SIG1 | 0x14 ); // power up, no gain
		audio_ic_write( AUDIO_IC_SIG2 | 0x04 ); // external, no gain
		audio_ic_write( AUDIO_IC_PM3 | 0x07 ); // external input
		audio_ic_write( AUDIO_IC_ALC1 | 0x00 ); // disable all ALC
		//audio_ic_write( AUDIO_IC_ALC1 | 0x24 ); // enable recording ALC

		// Set manual low gain; +30dB == 0xE1
		// gain == (byte - 145) * 0.375
		audio_ic_write( AUDIO_IC_IVL | 0xB1 );
		audio_ic_write( AUDIO_IC_IVR | 0xB1 );

		// Disable HPF
		//audio_ic_write( AUDIO_IC_HPF0 | 0x00 );
		//audio_ic_write( AUDIO_IC_HPF1 | 0x00 );
		//audio_ic_write( AUDIO_IC_HPF2 | 0x00 );
		//audio_ic_write( AUDIO_IC_HPF3 | 0x00 );

		// Disable LPF
		//audio_ic_write( AUDIO_IC_LPF0 | 0x00 );
		//audio_ic_write( AUDIO_IC_LPF1 | 0x00 );
		//audio_ic_write( AUDIO_IC_LPF2 | 0x00 );
		//audio_ic_write( AUDIO_IC_LPF3 | 0x00 );

		// Enable loop mode
		uint32_t mode3 = audio_ic_read( AUDIO_IC_MODE3 );
		mode3 |= (1<<6);
		audio_ic_write( AUDIO_IC_MODE3 | mode3 );

		//draw_audio_regs();
	}

	DebugMsg( DM_AUDIO, 3, "!!!!! %s task exited????", __func__ );
}

TASK_OVERRIDE( sounddev_task, my_sounddev_task );


/** Replace the audio level task with our own.
 *
 * This task runs when the sound device is activated to keep track of
 * the average audio level and translate it to dB.  Nothing ever seems
 * to activate it, so it is commented out for now.
 */
static void
my_audio_level_task( void )
{
	//const uint32_t * const thresholds = (void*) 0xFFC60ABC;
	DebugMsg( DM_AUDIO, 3, "!!!!! %s: Replaced Canon task %x", __func__, audio_level_task );

	audio_in.gain		= -40;
	audio_in.sample_count	= 0;
	audio_in.max_sample	= 0;
	audio_in.sem_interval	= create_named_semaphore( 0, 1 );
	audio_in.sem_task	= create_named_semaphore( 0, 0 );

	while(1)
	{
		DebugMsg( DM_AUDIO, 3, "%s: sleeping init=%d\n", __func__, audio_in.initialized );
		if( take_semaphore( audio_in.sem_interval, 0 ) & 1 )
		{
			//DebugAssert( "!IS_ERROR", "SoundDevice sem_interval", 0x82 );
			break;
		}

		if( take_semaphore( audio_in.sem_task, 0 ) & 1 )
		{
			//DebugAssert( "!IS_ERROR", SoundDevice", 0x83 );
			break;
		}

		DebugMsg( DM_AUDIO, 3, "%s: awake init=%d\n", __func__, audio_in.initialized );

		if( !audio_in.initialized )
		{
			DebugMsg( DM_AUDIO, 3, "**** %s: agc=%d/%d wind=%d volume=%d",
				__func__,
				audio_in.agc_on,
				audio_in.last_agc_on,
				audio_in.windcut,
				audio_in.volume
			);

			audio_set_filter_off();

			if( audio_in.last_agc_on == 1
			&&  audio_in.agc_on == 0
			)
				audio_set_alc_off();
			
			audio_in.last_agc_on = audio_in.agc_on;
			audio_set_windcut( audio_in.windcut );

			audio_set_sampling_param( 44100, 0x10, 1 );
			audio_set_volume_in(
				audio_in.agc_on,
				audio_in.volume
			);

			if( audio_in.agc_on == 1 )
				audio_set_alc_on();

			audio_in.initialized	= 1;
			audio_in.gain		= -39;
			audio_in.sample_count	= 0;

		}

		if( audio_in.asif_started == 0 )
		{
			DebugMsg( DM_AUDIO, 3, "%s: Starting asif observer", __func__ );
			audio_start_asif_observer();
			audio_in.asif_started = 1;
		}

		//uint32_t level = audio_read_level(0);
		give_semaphore( audio_in.sem_task );

		// Never adjust it!
		//set_audio_agc();
		//if( file != (void*) 0xFFFFFFFF )
			//FIO_WriteFile( file, &level, sizeof(level) );

		// audio_interval_wakeup will unlock our semaphore
		oneshot_timer( 0x200, audio_interval_unlock, audio_interval_unlock, 0 );
	}

	DebugMsg( DM_AUDIO, 3, "!!!!! %s task exited????", __func__ );
}


TASK_OVERRIDE( audio_level_task, my_audio_level_task );

static void
dump_task( void )
{
	msleep( 10000 );
	DebugMsg( DM_MAGIC, 3, "Calling dumpf" );
	dumpf();
}


void
create_audio_task(void)
{
	dmstart();

	task_create(
		"user_task",
		0x1f,
		0x1000,
		my_task,
		0
	);

	//task_create( "dump_task", 0x1f, 0, dump_task, 0 );
}
