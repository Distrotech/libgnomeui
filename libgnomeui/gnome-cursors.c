/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-cursors.c - Stock GNOME cursors
   
   Copyright (C) 1999 Iain Holmes
   All rights reserved.
   Contains code by Miguel de Icaza
   
   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
   
   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */
/*
  @NOTATION@
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gnome-gconf.h"

#include <gconf/gconf-client.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkx.h>

#ifdef NEED_GNOMESUPPORT_H
#  include "gnomesupport.h"
#endif

#include "gnome-cursors.h"
#include <unistd.h>
#include <ctype.h>

static GHashTable *cursortable = NULL;

/*
 * XPM data
 */


static const char * blank_xpm[] = {
	"8 1 1 1",
	"     c None",
	"        "
};

static char * default_arrow_xpm[] = {
"8 14 3 2",
"  	c None",
". 	c #000000",
"+ 	c #FFFFFF",
"..      ",
".+.     ",
".++.    ",
".+++.   ",
".++++.  ",
".+++++. ",
".++++...",
".++++.  ",
".+..+.  ",
".+. .+. ",
"..  .+. ",
"     .+.",
"     .+.",
"      . "};

static const char * egg_timer1_xpm[] = {
"16 16 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"   ..........   ",
"   .        .   ",
"   .    .   .   ",
"    ....+.+.    ",
"    .+..+.+.    ",
"     ..+.+.     ",
"     .+....     ",
"      .+ .      ",
"      . +.      ",
"     . .  .     ",
"     .  + .     ",
"    . ++.  .    ",
"    .  +   .    ",
"   .  +.++  .   ",
"   . +++.+  .   ",
"   ..........   "};

static const char * pointing_hand_xpm[] = {
"16 19 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"    ..          ",
"   .++.         ",
"   .++.         ",
"   .++.         ",
"   .++...       ",
"   .++.++.      ",
"   .++.++...    ",
" . .++.++.++... ",
".+..++.++.++.++.",
".++.++.++.++.++.",
".++.++.++.++.++.",
".++.++.++.++.++.",
" .+++++++++++++.",
" .+++++++++++++.",
"  .++++++++++++.",
"  .++++++++++++.",
"  .+++++++++++. ",
"   .+++++++++.  ",
"    .........   "};

static const char * cursor_zoom_in_xpm[] = {
"32 32 2 1",
"       c None",
".      c #000000",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"        ......                  ",
"       ..     ..                ",
"      ..       ..               ",
"      .         .               ",
"     .    ...    .              ",
"     .    ...    .              ",
"     .  .......  .              ",
"     .  .......  .              ",
"     .    ...    .              ",
"      .   ...   ..              ",
"      ..       . .              ",
"       ..     . . .             ",
"        ........ . .            ",
"         .....  . . .           ",
"                 . . .          ",
"                  . . .         ",
"                   . . .        ",
"                    . . .       ",
"                     .   .      ",
"                      .  .      ",
"                       ..       ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                "};

static const char * cursor_zoom_out_xpm[] = {
"32 32 2 1",
"       c None",
".      c #000000",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"        ......                  ",
"       ..     ..                ",
"      ..       ..               ",
"      .         .               ",
"     .           .              ",
"     .           .              ",
"     .  .......  .              ",
"     .  .......  .              ",
"     .           .              ",
"      .         ..              ",
"      ..       . .              ",
"       ..     . . .             ",
"        ........ . .            ",
"         .....  . . .           ",
"                 . . .          ",
"                  . . .         ",
"                   . . .        ",
"                    . . .       ",
"                     .   .      ",
"                      .  .      ",
"                       ..       ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                "};


#define hand_closed_data_width 20
#define hand_closed_data_height 20
static const char hand_closed_data_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x80, 0x3f, 0x00,
   0x80, 0xff, 0x00, 0x80, 0xff, 0x00, 0xb0, 0xff, 0x00, 0xf0, 0xff, 0x00,
   0xe0, 0xff, 0x00, 0xe0, 0x7f, 0x00, 0xc0, 0x7f, 0x00, 0x80, 0x3f, 0x00,
   0x00, 0x3f, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* Made with Gimp */
#define hand_closed_mask_width 20
#define hand_closed_mask_height 20
static const char hand_closed_mask_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x80, 0x3f, 0x00, 0xc0, 0xff, 0x00,
   0xc0, 0xff, 0x01, 0xf0, 0xff, 0x01, 0xf8, 0xff, 0x01, 0xf8, 0xff, 0x01,
   0xf0, 0xff, 0x01, 0xf0, 0xff, 0x00, 0xe0, 0xff, 0x00, 0xc0, 0x7f, 0x00,
   0x80, 0x7f, 0x00, 0x80, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define hand_open_data_width 20
#define hand_open_data_height 20
static const char hand_open_data_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
   0x60, 0x36, 0x00, 0x60, 0x36, 0x00, 0xc0, 0x36, 0x01, 0xc0, 0xb6, 0x01,
   0x80, 0xbf, 0x01, 0x98, 0xff, 0x01, 0xb8, 0xff, 0x00, 0xf0, 0xff, 0x00,
   0xe0, 0xff, 0x00, 0xe0, 0x7f, 0x00, 0xc0, 0x7f, 0x00, 0x80, 0x3f, 0x00,
   0x00, 0x3f, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define hand_open_mask_width 20
#define hand_open_mask_height 20
static const char hand_open_mask_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x60, 0x3f, 0x00,
   0xf0, 0x7f, 0x00, 0xf0, 0x7f, 0x01, 0xe0, 0xff, 0x03, 0xe0, 0xff, 0x03,
   0xd8, 0xff, 0x03, 0xfc, 0xff, 0x03, 0xfc, 0xff, 0x01, 0xf8, 0xff, 0x01,
   0xf0, 0xff, 0x01, 0xf0, 0xff, 0x00, 0xe0, 0xff, 0x00, 0xc0, 0x7f, 0x00,
   0x80, 0x7f, 0x00, 0x80, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static char * horiz_xpm[] = {
"16 8 3 2",
"  	c None",
". 	c #000000",
"+ 	c #FFFFFF",
"   ...  ...     ",
"  .+.    .+.    ",
" .++......++.   ",
".++++++++++++.  ",
".++++++++++++.  ",
" .++......++.   ",
"  .+.    .+.    ",
"   ...  ...     "};

static char * vert_xpm[] = {
"8 14 3 2",
"  	c None",
". 	c #000000",
"+ 	c #FFFFFF",
"   ..   ",
"  .++.  ",
" .++++. ",
".++++++.",
"...++...",
". .++. .",
"  .++.  ",
"  .++.  ",
". .++. .",
"...++...",
".++++++.",
" .++++. ",
"  .++.  ",
"   ..   "};

static char * ne_sw_xpm[] = {
"16 12 3 2",
"  	c None",
". 	c #000000",
"+ 	c #FFFFFF",
"       ....     ",
"      .++++.    ",
"     . .+++.    ",
"      .++++.    ",
"     .+++.+.    ",
"  . .+++. .     ",
" . .+++. .      ",
".+.+++.         ",
".++++.          ",
".+++. .         ",
".++++.          ",
" ....           "};

static char * nw_se_xpm[] = {
"16 12 3 2",
"  	c None",
". 	c #000000",
"+ 	c #FFFFFF",
" ....           ",
".++++.          ",
".+++. .         ",
".++++.          ",
".+.+++.         ",
" . .+++. .      ",
"  . .+++. .     ",
"     .+++.+.    ",
"      .++++.    ",
"     . .+++.    ",
"      .++++.    ",
"       ....     "};

static char * nsew_xpm[] = {
"24 18 3 2",
"  	c None",
". 	c #000000",
"+ 	c #FFFFFF",
"        ..              ",
"       .++.             ",
"      .++++.            ",
"     .++++++.           ",
"     .+.++.+.           ",
"   .. .++++. ..         ",
"  .++. .++. .++.        ",
" .++.+.++++.+.++.       ",
".++++++++++++++++.      ",
".++++++++++++++++.      ",
" .++.+.++++.+.++.       ",
"  .++. .++. .++.        ",
"   .. .++++. ..         ",
"     .+.++.+.           ",
"     .++++++.           ",
"      .++++.            ",
"       .++.             ",
"        ..              "};

static char * corners_xpm[] = {
"16 12 3 2",
"  	c None",
". 	c #000000",
"+ 	c #FFFFFF",
" ....  ....     ",
".++++..++++.    ",
".+++....+++.    ",
".++++..++++.    ",
".+.++++++.+.    ",
" ...++++...     ",
" ...++++...     ",
".+.++++++.+.    ",
".++++..++++.    ",
".+++....+++.    ",
".++++..++++.    ",
" ....  ....     "};

static char * bar_xpm[] = {
"8 16 3 2",
"  	c None",
". 	c #000000",
"+ 	c #FFFFFF",
" .. ..  ",
".++.++. ",
" .+++.  ",
"  .+.   ",
"  .+.   ",
"  .+.   ",
"  .+.   ",
"  .+.   ",
"  .+.   ",
"  .+.   ",
"  .+.   ",
"  .+.   ",
"  .+.   ",
" .+++.  ",
".++.++. ",
" .. ..  "};

static char * question_xpm[] = {
"24 24 3 2",
"  	c None",
". 	c #000000",
"+ 	c #FFFFFF",
"..          ......      ",
".+.        .++++++.     ",
".++.      .++....++.    ",
".+++.    .++.    .++.   ",
".++++.   .+.      .+.   ",
".+++++.  .+.      .+.   ",
".++++... .+.      .+.   ",
".++++.    .       .+.   ",
".+..+.            .+.   ",
".+. .+.          .++.   ",
"..  .+.      ....++.    ",
"     .+.    .+++++.     ",
"     .+.    .++...      ",
"      .     .+.         ",
"            .+.         ",
"            .+.         ",
"             .          ",
"                        ",
"             .          ",
"            .+.         ",
"            .+.         ",
"             .          ",
"                        ",
"                        "};

#define WHITE { 0x0000, 0xffff, 0xffff, 0xffff }
#define BLACK { 0x0000, 0x0000, 0x0000, 0x0000 }


static GnomeStockCursor default_cursors[] = {
        {
                GNOME_STOCK_CURSOR_DEFAULT, /* name */
                default_arrow_xpm, /* cursor_data */
                NULL,     /* xbm mask */
                NULL, NULL, /* bitmap/mask */
                WHITE, /* foreground */
                BLACK, /* background */
                0, 0,     /* hotspot */
                -1, -1,   /* w x h */
                0,        /* alpha */
                GNOME_CURSOR_XPM /* type */
        },
        {
                GNOME_STOCK_CURSOR_BLANK, /* name */
                blank_xpm, /* cursor_data */
                NULL, /* xbm mask */
                NULL, NULL, /* bitmap/mask */
                WHITE, /* foreground */
                BLACK, /* background */
                0, 0,     /* hotspot */
                -1, -1,   /* w x h */
                0,        /* alpha */
                GNOME_CURSOR_XPM /* type */
        },
        {
                GNOME_STOCK_CURSOR_POINTING_HAND, /* name */
                pointing_hand_xpm, /* cursor_data */
                NULL, /* xbm mask */
                NULL, NULL, /* bitmap/mask */
                WHITE, /* foreground */
                BLACK, /* background */
                5, 0,     /* hotspot */
                -1, -1,   /* w x h */
                0,        /* alpha */
                GNOME_CURSOR_XPM /* type */
        },
        {
                GNOME_STOCK_CURSOR_HAND_OPEN, /* name */
                (gchar*)hand_open_data_bits, /* cursor_data */
                (gchar*)hand_open_mask_bits,
                NULL, NULL, /* bitmap/mask */
                WHITE, /* foreground */
                BLACK, /* background */
                10, 10,     /* hotspot */
                hand_open_data_width, hand_open_data_height,   /* w x h */
                0,        /* alpha */
                GNOME_CURSOR_XBM /* type */
        },
        {
                GNOME_STOCK_CURSOR_HAND_CLOSE, /* name */
                (gchar*)hand_closed_data_bits, /* cursor_data */
                (gchar*)hand_closed_mask_bits,
                NULL, NULL, /* bitmap/mask */
                WHITE, /* foreground */
                BLACK, /* background */
                10, 10,     /* hotspot */
                hand_closed_data_width, hand_closed_data_height,   /* w x h */
                0,        /* alpha */
                GNOME_CURSOR_XBM /* type */
        },
        {
                GNOME_STOCK_CURSOR_ZOOM_IN, /* name */
                cursor_zoom_in_xpm, /* cursor_data */
                NULL, /* xbm mask */
                NULL, NULL, /* bitmap/mask */
                BLACK, /* foreground */
                WHITE, /* background */
                10, 10,     /* hotspot */
                -1, -1,   /* w x h */
                0,        /* alpha */
                GNOME_CURSOR_XPM /* type */
        },
        {
                GNOME_STOCK_CURSOR_ZOOM_OUT, /* name */
                cursor_zoom_out_xpm, /* cursor_data */
                NULL, /* xbm mask */
                NULL, NULL, /* bitmap/mask */
                BLACK, /* foreground */
                WHITE, /* background */
                10, 10,     /* hotspot */
                -1, -1,   /* w x h */
                0,        /* alpha */
                GNOME_CURSOR_XPM /* type */
        },
        {
                GNOME_STOCK_CURSOR_EGG_1, /* name */
                egg_timer1_xpm, /* cursor_data */
                NULL, /* xbm mask */
                NULL, NULL, /* bitmap/mask */
                {0x0, 0xFFFF, 0xFFFF, 0x0}, /* foreground */
                BLACK, /* background */
                8, 8,     /* hotspot */
                -1, -1,   /* w x h */
                0,        /* alpha */
                GNOME_CURSOR_XPM /* type */
        },
	{
		GNOME_STOCK_CURSOR_HORIZONTAL, /* name */
		horiz_xpm, /* cursor_data */
		NULL, /* xbm_mask */
		NULL, NULL, /* bitmap/mask */
		WHITE, /* foreground */
		BLACK, /* background */
		6, 3,  /* hotspot */
		-1, -1,/* w x h */
		0,     /* alpha */
		GNOME_CURSOR_XPM /* type */
	},
	{
		GNOME_STOCK_CURSOR_VERTICAL, /* name */
		vert_xpm, /* cursor_data */
		NULL, /* xbm_mask */
		NULL, NULL, /* bitmap/mask */
		WHITE, /* foreground */
		BLACK, /* background */
		3, 6,  /* hotspot */
		-1, -1,/* w x h */
		0,     /* alpha */
		GNOME_CURSOR_XPM /* type */
	},
	{
		GNOME_STOCK_CURSOR_NE_SW, /* name */
		ne_sw_xpm, /* cursor_data */
		NULL, /* xbm_mask */
		NULL, NULL, /* bitmap/mask */
		WHITE, /* foreground */
		BLACK, /* background */
		5, 5,  /* hotspot */
		-1, -1,/* w x h */
		0,     /* alpha */
		GNOME_CURSOR_XPM /* type */
	},
	{
		GNOME_STOCK_CURSOR_NW_SE, /* name */
		nw_se_xpm, /* cursor_data */
		NULL, /* xbm_mask */
		NULL, NULL, /* bitmap/mask */
		WHITE, /* foreground */
		BLACK, /* background */
		5, 5,  /* hotspot */
		-1, -1,/* w x h */
		0,     /* alpha */
		GNOME_CURSOR_XPM /* type */
	},
	{
		GNOME_STOCK_CURSOR_FLEUR, /* name */
		nsew_xpm, /* cursor_data */
		NULL, /* xbm_mask */
		NULL, NULL, /* bitmap/mask */
		WHITE, /* foreground */
		BLACK, /* background */
		8, 8,  /* hotspot */
		-1, -1,/* w x h */
		0,     /* alpha */
		GNOME_CURSOR_XPM /* type */
	},
	{
		GNOME_STOCK_CURSOR_CORNERS, /* name */
		corners_xpm, /* cursor_data */
		NULL, /* xbm_mask */
		NULL, NULL, /* bitmap/mask */
		WHITE, /* foreground */
		BLACK, /* background */
		5, 5,  /* hotspot */
		-1, -1,/* w x h */
		0,     /* alpha */
		GNOME_CURSOR_XPM /* type */
	},
	{
		GNOME_STOCK_CURSOR_WHATISIT, /* name */
		question_xpm, /* cursor_data */
		NULL, /* xbm_mask */
		NULL, NULL, /* bitmap/mask */
		WHITE, /* foreground */
		BLACK, /* background */
		0, 0,  /* hotspot */
		-1, -1, /* w x h */
		0,     /* alpha */
		GNOME_CURSOR_XPM /* type */
	},
	{
		GNOME_STOCK_CURSOR_XTERM, /* name */
		bar_xpm, /* cursor_data */
		NULL, /* xbm_mask */
		NULL, NULL, /* bitmap/mask */
		WHITE, /* foreground */
		BLACK, /* background */
		3, 7,  /* hotspot */
		-1, -1,/* w x h */
		0,     /* alpha */
		GNOME_CURSOR_XPM /* type */
	}
};

static const gint num_default_cursors = sizeof(default_cursors)/sizeof(GnomeStockCursor);


static void
build_cursor_table (void)
{
	int i;
	GError *err = NULL;
	GConfClient *client;

        g_assert (cursortable == NULL);

	/* Get the main gnome gconf client */
        client = gconf_client_get_default (); 

	cursortable = g_hash_table_new (g_str_hash, g_str_equal);
	
	for (i = 0; i < num_default_cursors; i++){
		const char *name = default_cursors[i].cursorname;
		char *key, *cursorstr;

		/* Build key */
		key = g_strconcat ("/desktop/gnome/cursors/", name, NULL);
		cursorstr = gconf_client_get_string (client, key, &err);
		if (cursorstr) {
			gnome_stock_cursor_register (&default_cursors[i]);
		} else {
			gnome_stock_cursor_register (&default_cursors[i]);
		}

		if (err) {
			g_warning (err->message);
			g_error_free (err);
		}
		
		g_free (key);
		g_free (cursorstr);
	}
}


/**
 * create_bitmap_and_mask_from_xpm:
 * @bitmap: Bitmap to hold the cursor bitmap.
 * @mask: Bitmap to hold mask.
 * @xpm: XPM data.
 *
 * Originally written by Miguel de Icaza.
 * Modified by Iain Holmes.
 */
static void
create_bitmap_and_mask_from_xpm (GdkBitmap **bitmap,
				 GdkBitmap **mask,
				 const char **xpm)
{
	int height, width, colors;
	char *pixmap_buffer;
	char *mask_buffer;
	int x, y, pix, yofs;
	int transparent_color, black_color;
	
	sscanf (xpm [0], "%d %d %d %d", &width, &height, &colors, &pix);
	
 	g_assert (width%8 == 0);
	pixmap_buffer = (char*) g_malloc (sizeof (char) * (width * height/8));
	mask_buffer = (char*) g_malloc (sizeof (char) * (width * height/8));
	
	if (!pixmap_buffer || !mask_buffer)
		return;
	
	g_assert (colors <= 3);
	
	transparent_color = ' ';
	black_color = '.';
	
	yofs = colors + 1;
	for (y = 0; y < height; y++){
		for (x = 0; x < width;){
			char value = 0, maskv = 0;
			
			for (pix = 0; pix < 8; pix++, x++){
				if (xpm [y + yofs][x] != transparent_color){
					maskv |= 1 << pix;
					
					if (xpm [y + yofs][x] != black_color){
						value |= 1 << pix;
					}
				}
			}
			pixmap_buffer [(y * (width/8) + x/8)-1] = value;
			mask_buffer [(y * (width/8) + x/8)-1] = maskv;
		}
	}
	*bitmap = gdk_bitmap_create_from_data (NULL, pixmap_buffer, width, height);
	*mask   = gdk_bitmap_create_from_data (NULL, mask_buffer, width, height);

	g_free(pixmap_buffer);
	g_free(mask_buffer);
}


#define MAXIMUM(x,y) ((x) > (y) ? (x) : (y))
#define MINIMUM(x,y) ((x) < (y) ? (x) : (y))

/* Take RGB and desaturate it. */
static unsigned int
threshold (unsigned int r,
	   unsigned int g,
	   unsigned int b)
{
	int max, min;
	
	max = MAXIMUM(r, g);
	max = MAXIMUM(max, b);
	min = MINIMUM(b, g);
	min = MINIMUM(min, r);
	
	return (max + min) / 2;
}


/**
 * gnome_cursor_create_from_pixbuf:
 * @pixbuf: #GdkPixbuf containing the image.
 * @bitmap: Return for the image bitmap.
 * @mask: Return for the image mask.
 * @width: Return for the image width.
 * @height: Return for the image height.
 *
 * Creates a bitmap and mask from a #GdkPixbuf.
 */
static void
gnome_cursor_create_from_pixbuf (GdkPixbuf *pixbuf,
                                 guchar alpha_threshold,
				 GdkBitmap **bitmap,
				 GdkBitmap **mask)
{
	int w, h, has_alpha;
	int x, y;
	unsigned char *pixel;
	unsigned int r, g, b, a = 255;
	unsigned char *pmap, *bmap;
	
	has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
	pixel = gdk_pixbuf_get_pixels (pixbuf);

        w = gdk_pixbuf_get_width(pixbuf);
        h = gdk_pixbuf_get_height(pixbuf);
        
	g_assert (w%8 == 0);
	pmap = (char*) g_malloc (sizeof (char) * (w * h / 8));
	bmap = (char*) g_malloc (sizeof (char) * (w * h / 8));
	
	for (y = 0; y < h; y++) {
                for (x = 0; x < w;) {
                        int pix;
                        unsigned char value = 0, maskv = 0;
			
                        for (pix = 0; pix < 8; pix++, x++) {

                                r = *(pixel++);
                                g = *(pixel++);
                                b = *(pixel++);
                                if (has_alpha)
                                        a = *(pixel++);
				
                                if (a >= 127) {
                                        maskv |= 1 << pix;
					
                                        /* threshold should be a #define */
                                        if (threshold (r, g, b) >= alpha_threshold) {
                                                value |= 1 << pix;
                                        }
                                }
                        }
                        pmap[(y * (w / 8) + x / 8 - 1)] = value;
                        bmap[(y * (w / 8) + x / 8 - 1)] = maskv;
                }
	}
	
	*bitmap = gdk_bitmap_create_from_data (NULL, pmap, w, h);
	*mask = gdk_bitmap_create_from_data (NULL, bmap, w, h);

	g_free(pmap);
	g_free(bmap);
}


/**
 * gnome_stock_cursor_new:
 * @cursorname: The name of the cursor to be set
 *
 * Returns a #GdkCursor of the requested cursor or %NULL on error.
 * @cursorname is the name of a gnome stock cursor; either one you've
 * registered or one of the stock definitions
 * (GNOME_STOCK_CURSOR_DEFAULT, etc.)
 *
 * Returns: A new #GdkCursor or %NULL on failure.  */
GdkCursor*
gnome_stock_cursor_new (const char *cursorname)
{
	GnomeStockCursor *cursor = NULL;
	GdkCursor *gdk_cursor;
        GdkBitmap *pmap = NULL;
        GdkBitmap *mask = NULL;

        /* FIXME if GdkCursor had a refcount we could cache that
           instead of the pixmap/bitmap */
        
	if (!cursortable)
		build_cursor_table ();
	
	cursor = g_hash_table_lookup (cursortable, cursorname);
	
	if (cursor == NULL) {
		g_warning ("Unknown cursor %s", cursorname);
		return NULL;
	}

        if (cursor->type != GNOME_CURSOR_STANDARD && cursor->pmap == NULL)
	{
                switch (cursor->type) {
                case GNOME_CURSOR_XBM:
                        pmap = gdk_bitmap_create_from_data (NULL,
                                                            cursor->cursor_data,
                                                            cursor->width,
                                                            cursor->height);
                        mask = gdk_bitmap_create_from_data (NULL,
                                                            cursor->xbm_mask_bits,
                                                            cursor->width,
                                                            cursor->height);
                        break;
                        
                case GNOME_CURSOR_XPM:
                        create_bitmap_and_mask_from_xpm (&pmap, &mask, 
                                                         (const char **)cursor->cursor_data);
                        break;
                
                case GNOME_CURSOR_FILE:
                {
                        GdkPixbuf *pixbuf;
			GError *error;

			error = NULL;
			pixbuf = gdk_pixbuf_new_from_file (cursor->cursor_data,
							   &error);
			if (error != NULL) {
				g_warning (G_STRLOC ": can't load %s: %s",
					   (gchar *) cursor->cursor_data,
					   error->message);
				g_error_free (error);
				return NULL;
			}

                        gnome_cursor_create_from_pixbuf (pixbuf,
                                                         cursor->alpha_threshhold,
                                                         &pmap, &mask);
                        gdk_pixbuf_unref (pixbuf);
                }
                break;
              
                case GNOME_CURSOR_GDK_PIXBUF:
                        gnome_cursor_create_from_pixbuf ((GdkPixbuf*) cursor->cursor_data,
                                                         cursor->alpha_threshhold,
                                                         &pmap, &mask);
                        break;
                
                default:
                        g_warning ("Invalid GnomeCursorType");
                        return NULL;
                }
        }

	if (pmap && mask)
	{
		cursor->pmap = pmap;
		cursor->mask = mask;
	}
	

        if (cursor->type != GNOME_CURSOR_STANDARD) {
                if (!cursor->pmap)
                        return NULL;
                else
		{
                        gdk_cursor = gdk_cursor_new_from_pixmap (cursor->pmap, 
                                                                 cursor->mask,
                                                                 &cursor->foreground,
                                                                 &cursor->background,
                                                                 cursor->hotspot_x, 
                                                                 cursor->hotspot_y);
                }
        } else {
                return gdk_cursor_new(GPOINTER_TO_INT(cursor->cursor_data));
        }
	
	return gdk_cursor;
}


/**
 * gnome_stock_cursor_register:
 * @cursor: #GnomeStockCursor describing the cursor to register.
 *
 * The #GnomeStockCursor is not copied and must not be freed.  */
void
gnome_stock_cursor_register (GnomeStockCursor *cursor)
{
	GnomeStockCursor *old_cursor;
        
	if (!cursortable)
		build_cursor_table ();

        g_return_if_fail (cursor != NULL);
        g_return_if_fail (cursor->pmap == NULL);
        g_return_if_fail (cursor->mask == NULL);
        
	/* Check if the cursorname is unique */
	old_cursor = g_hash_table_lookup (cursortable, cursor->cursorname);
        
	if (old_cursor) {
		g_warning ("Cursor name %s is already registered.", 
			   cursor->cursorname);
		return;
	}

	g_hash_table_insert (cursortable, (gpointer)cursor->cursorname, cursor);
}

/**
 * gnome_stock_cursor_lookup_entry:
 * @cursorname: cursor name to look up in the list of registered cursors.
 *
 * The returned #GnomeStockCursor is not copied and must not be freed.  */
GnomeStockCursor*
gnome_stock_cursor_lookup_entry(const char *cursorname)
{
	if (!cursortable)
		build_cursor_table ();
	
	return g_hash_table_lookup (cursortable, cursorname);
}


/**
 * gnome_stock_cursor_unregister:
 * @cursorname: Name of cursor to unregister.
 *
 * Unregisters a cursor and frees all memory associated with it.
 */
void
gnome_stock_cursor_unregister (const char *cursorname)
{
	GnomeStockCursor *cursor;

        g_return_if_fail (cursortable != NULL);

	cursor = g_hash_table_lookup (cursortable, cursorname);
	g_return_if_fail (cursor != NULL);
	
	g_hash_table_remove (cursortable, cursorname);

        gdk_bitmap_unref(cursor->pmap);
        gdk_bitmap_unref(cursor->mask);
}



/*
 * The original idea was to have a stack of cursors for each GdkWindow,
 * with push/pop. However this can't be sanely implemented
 * without reference counting on the cursors.
 */
