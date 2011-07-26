#include "dryos.h"
#include "bmp.h"
#include "menu.h"
#include "property.h"
#include "config.h"
#include "gui.h"
#include "af_patterns.h"

type_PATTERN_MAP_ITEM pattern_map[] = {
		{AF_PATTERN_CENTER,         AF_PATTERN_SQUARE, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},
		{AF_PATTERN_SQUARE,         AF_PATTERN_HLINE,  AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

		{AF_PATTERN_TOP,            AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_TOPLEFT,       AF_PATTERN_TOPRIGHT},
		{AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_TOPDIAMOND,     AF_PATTERN_LEFTTRIANGLE,  AF_PATTERN_RIGHTTRIANGLE},
		{AF_PATTERN_TOPDIAMOND,     AF_PATTERN_CENTER, AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_TOPHALF,        AF_PATTERN_LEFTDIAMOND,   AF_PATTERN_RIGHTDIAMOND},
		{AF_PATTERN_TOPHALF,        AF_PATTERN_CENTER, AF_PATTERN_TOPDIAMOND,     AF_PATTERN_HLINE,          AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

		{AF_PATTERN_BOTTOM,         AF_PATTERN_CENTER, AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_BOTTOM,         AF_PATTERN_BOTTOMLEFT,    AF_PATTERN_BOTTOMRIGHT},
		{AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_CENTER, AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_BOTTOM,         AF_PATTERN_LEFTTRIANGLE,  AF_PATTERN_RIGHTTRIANGLE},
		{AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_CENTER, AF_PATTERN_BOTTOMHALF,     AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_LEFTDIAMOND,   AF_PATTERN_RIGHTDIAMOND},
		{AF_PATTERN_BOTTOMHALF,     AF_PATTERN_CENTER, AF_PATTERN_HLINE,          AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

		{AF_PATTERN_TOPLEFT,        AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_BOTTOMLEFT,     AF_PATTERN_LEFT,          AF_PATTERN_TOPRIGHT},
		{AF_PATTERN_TOPRIGHT,       AF_PATTERN_CENTER, AF_PATTERN_TOP,            AF_PATTERN_BOTTOMRIGHT,    AF_PATTERN_TOPLEFT,       AF_PATTERN_RIGHT},
		{AF_PATTERN_BOTTOMLEFT,     AF_PATTERN_CENTER, AF_PATTERN_TOPLEFT,        AF_PATTERN_BOTTOM,         AF_PATTERN_LEFT,          AF_PATTERN_BOTTOMRIGHT},
		{AF_PATTERN_BOTTOMRIGHT,    AF_PATTERN_CENTER, AF_PATTERN_TOPRIGHT,       AF_PATTERN_BOTTOM,         AF_PATTERN_BOTTOMLEFT,    AF_PATTERN_RIGHT},

		{AF_PATTERN_LEFT,           AF_PATTERN_CENTER, AF_PATTERN_TOPLEFT,        AF_PATTERN_BOTTOMLEFT,     AF_PATTERN_LEFT,          AF_PATTERN_LEFTTRIANGLE},
		{AF_PATTERN_LEFTTRIANGLE,   AF_PATTERN_CENTER, AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_LEFT,          AF_PATTERN_LEFTDIAMOND},
		{AF_PATTERN_LEFTDIAMOND,    AF_PATTERN_CENTER, AF_PATTERN_TOPDIAMOND,     AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_LEFTTRIANGLE,  AF_PATTERN_LEFTHALF},
		{AF_PATTERN_LEFTHALF,       AF_PATTERN_CENTER, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTDIAMOND,   AF_PATTERN_VLINE},

		{AF_PATTERN_RIGHT,          AF_PATTERN_CENTER, AF_PATTERN_TOPRIGHT,       AF_PATTERN_BOTTOMRIGHT,    AF_PATTERN_RIGHTTRIANGLE, AF_PATTERN_RIGHT},
		{AF_PATTERN_RIGHTTRIANGLE,  AF_PATTERN_CENTER, AF_PATTERN_TOPTRIANGLE,    AF_PATTERN_BOTTOMTRIANGLE, AF_PATTERN_RIGHTDIAMOND,  AF_PATTERN_RIGHT},
		{AF_PATTERN_RIGHTDIAMOND,   AF_PATTERN_CENTER, AF_PATTERN_TOPDIAMOND,     AF_PATTERN_BOTTOMDIAMOND,  AF_PATTERN_RIGHTHALF,     AF_PATTERN_RIGHTTRIANGLE},
		{AF_PATTERN_RIGHTHALF,      AF_PATTERN_CENTER, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_VLINE,         AF_PATTERN_RIGHTDIAMOND},

		{AF_PATTERN_HLINE,          AF_PATTERN_VLINE,  AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},
		{AF_PATTERN_VLINE,          AF_PATTERN_ALL,    AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

		{AF_PATTERN_ALL,            AF_PATTERN_CENTER, AF_PATTERN_TOPHALF,        AF_PATTERN_BOTTOMHALF,     AF_PATTERN_LEFTHALF,      AF_PATTERN_RIGHTHALF},

		END_OF_LIST
};

int afp_transformer (int pattern, type_DIRECTION direction);

PROP_INT(PROP_AFPOINT, af_point);

void afp_show_in_viewfinder()
{
	msleep(50);
	assign_af_button_to_halfshutter();
	msleep(50);
	SW1(1,0);
	msleep(50);
	SW1(0,0);
	msleep(50);
	restore_af_button_assignment();
}

void set_af_point(int afpoint)
{
	prop_request_change(PROP_AFPOINT, &afpoint, 4);
	task_create("afp_tmp", 0x18, 0, afp_show_in_viewfinder, 0);
}

int afpoint_for_key_guess = 0;

void afp_center () {
	set_af_point(afp_transformer(af_point, DIRECTION_CENTER));
}

void afp_top () {
	set_af_point(afp_transformer(af_point, DIRECTION_UP));
}

void afp_bottom () {
	set_af_point(afp_transformer(af_point, DIRECTION_DOWN));
}

void afp_left () {
	set_af_point(afp_transformer(af_point, DIRECTION_LEFT));
}

void afp_right () {
	set_af_point(afp_transformer(af_point, DIRECTION_RIGHT));
}

int afp_transformer (int pattern, type_DIRECTION direction) {
	type_PATTERN_MAP_ITEM *item;

	// Loop over all items in the pattern map
	for (item = pattern_map; ! IS_EOL(item); item++) {

		// When we find an item matching the current pattern...
		if (item->pattern == pattern) {

			// ... we return the next pattern, according to the direction indicated
			switch (direction) {
			case DIRECTION_CENTER:
				return item->next_center;
			case DIRECTION_UP:
				return item->next_top;
			case DIRECTION_DOWN:
				return item->next_bottom;
			case DIRECTION_LEFT:
				return item->next_left;
			case DIRECTION_RIGHT:
				return item->next_right;
			}
		}
	}

	// Just in case something goes wrong
	return AF_PATTERN_CENTER;
}

CONFIG_INT("focus.patterns", af_patterns, 0);

static void
afp_display(
	void *			priv,
	int			x,
	int			y,
	int			selected
)
{
	bmp_printf(
		selected ? MENU_FONT_SEL : MENU_FONT,
		x, y,
		"Focus Patterns: %s",
		af_patterns ? "ON" : "OFF"
	);
	if (lv && af_patterns) menu_draw_icon(x, y, MNI_WARNING, 0);
}


static struct menu_entry afp_menu[] = {
	{
		.name = "Focus Patterns",
		.priv = &af_patterns,
		.display	= afp_display,
		.select = menu_binary_toggle,
		.help = "Custom AF patterns (photo mode only; ported from 400plus)"
	},
};

static void
afp_init( void* unused )
{
	menu_add( "Focus", afp_menu, COUNT(afp_menu) );
}

INIT_FUNC( __FILE__, afp_init );
