/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-cursors.h - Stock GNOME cursors

   Copyright (C) 1999 Iain Holmes

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

#ifndef GNOME_CURSORS_H
#define GNOME_CURSORS_H

typedef enum _GnomeStockCursorType GnomeStockCursorType;

enum _GnomeStockCursorType {
	/* update structure when adding enums */
	GNOME_CURSOR_FILE,
	GNOME_CURSOR_XBM,
	GNOME_CURSOR_XPM,
	GNOME_CURSOR_GDK_PIXBUF,
	GNOME_CURSOR_STANDARD
};

typedef struct _GnomeStockCursor GnomeStockCursor;

struct _GnomeStockCursor {
        gchar *cursorname;

        /* Only needed for _XBM */
        gchar *xbm_mask_bits;
        /* Filled in by the library */
        GdkBitmap *pmap;
        GdkBitmap *mask;

        /*
          GNOME_CURSOR_FILE: filename
          GNOME_CURSOR_XBM:  XBM data bits
          GNOME_CURSOR_XPM:  XPM data
          GNOME_CURSOR_GDK_PIXBUF: GdkPixbuf*
          GNOME_CURSOR_STANDARD: a #define from gdkcursors.h
        */
        gpointer cursor_data;

        /* Must initialize in all cases */
        GdkColor foreground;
        GdkColor background;

        /* Not needed for standard X cursors */
        gint16 hotspot_x;
        gint16 hotspot_y;

        /* Only needed for XBM */
        gint16 width;
        gint16 height;

        /* Only needed for _GDK_PIXBUF and _FILE */
        guchar alpha_threshold;

        GnomeStockCursorType type : 3;
};

void       gnome_stock_cursor_register   (GnomeStockCursor *cursor);

/* This returns a pointer to the internal GnomeStockCursor,
   which should not be modified. */
GnomeStockCursor*
           gnome_stock_cursor_lookup_entry(const gchar *srcname);
                                          
void       gnome_stock_cursor_unregister  (const gchar *cursorname);

GdkCursor *gnome_stock_cursor_new         (const gchar *cursorname);


/* Set the cursor on a window with gdk_window_set_cursor(),
 * tnen destroy it with gdk_cursor_destroy()
 */

#define GNOME_STOCK_CURSOR_DEFAULT "default-arrow"
#define GNOME_STOCK_CURSOR_BLANK "blank"
#define GNOME_STOCK_CURSOR_HAND_OPEN "hand-open"
#define GNOME_STOCK_CURSOR_HAND_CLOSE "hand-close"
#define GNOME_STOCK_CURSOR_ZOOM_IN "zoom-in"
#define GNOME_STOCK_CURSOR_ZOOM_OUT "zoom-out"
#define GNOME_STOCK_CURSOR_EGG_1 "egg-timer-1"
#define GNOME_STOCK_CURSOR_POINTING_HAND "pointing-hand"


#endif /* GNOME_CURSORS_H */
