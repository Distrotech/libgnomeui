/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-cursors.h - Stock GNOME cursors

   Copyright (C) 1999 Iain Holmes
   All rights reserved.

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
   Boston, MA 02111-1307, USA.
*/
/*
  @NOTATION@
*/

#ifndef GNOME_CURSORS_H
#define GNOME_CURSORS_H

#include <gdk/gdk.h>

typedef enum  {
	/* Update bitfield size in struct when adding enums */
	GNOME_CURSOR_FILE,
	GNOME_CURSOR_XBM,
	GNOME_CURSOR_XPM,
	GNOME_CURSOR_GDK_PIXBUF,
	GNOME_CURSOR_STANDARD
} GnomeStockCursorType;

typedef struct _GnomeStockCursor GnomeStockCursor;

struct _GnomeStockCursor { /* The new one */
	const char *cursorname;
	gconstpointer cursor_data;
	gchar *xbm_mask_bits;
	GdkBitmap *pmap, *mask;

	GdkColor foreground, background;
	gint16 hotspot_x, hotspot_y;
	gint16 width, height;
	guchar alpha_threshhold;
	GnomeStockCursorType type : 4;
};

void       gnome_stock_cursor_register   (GnomeStockCursor *cursor);

/* This returns a pointer to the internal GnomeStockCursor,
   which should not be modified. */
GnomeStockCursor*
           gnome_stock_cursor_lookup_entry(const char *srcname);
                                          
void       gnome_stock_cursor_unregister  (const char *cursorname);

/* Use gdk_cursor_destroy() to destroy the returned value */
GdkCursor *gnome_stock_cursor_new         (const char *cursorname);


/* Set the cursor on a window with gdk_window_set_cursor(),
 * tnen destroy it with gdk_cursor_destroy()
 */

#define GNOME_STOCK_CURSOR_DEFAULT "gnome-default-arrow"
#define GNOME_STOCK_CURSOR_BLANK "gnome-blank"
#define GNOME_STOCK_CURSOR_HAND_OPEN "gnome-hand-open"
#define GNOME_STOCK_CURSOR_HAND_CLOSE "gnome-hand-close"
#define GNOME_STOCK_CURSOR_ZOOM_IN "gnome-zoom-in"
#define GNOME_STOCK_CURSOR_ZOOM_OUT "gnome-zoom-out"
#define GNOME_STOCK_CURSOR_EGG_1 "gnome-egg-timer-1"
#define GNOME_STOCK_CURSOR_POINTING_HAND "gnome-pointing-hand"
#define GNOME_STOCK_CURSOR_HORIZONTAL "gnome-horizontal-arrow"
#define GNOME_STOCK_CURSOR_VERTICAL "gnome-vertical-arrow"
#define GNOME_STOCK_CURSOR_NE_SW "gnome-northeast-southwest"
#define GNOME_STOCK_CURSOR_NW_SE "gnome-northwest-southeast"
#define GNOME_STOCK_CURSOR_FLEUR "gnome-fleur"
#define GNOME_STOCK_CURSOR_CORNERS "gnome-corner-arrow"
#define GNOME_STOCK_CURSOR_WHATISIT "gnome-what-is-it"
#define GNOME_STOCK_CURSOR_XTERM "gnome-xterm"

#endif /* GNOME_CURSORS_H */
