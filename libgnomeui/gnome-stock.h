/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Stock icons, buttons, and menu items.
   Copyright (C) 1997, 1998 Free Software Foundation
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

   Author: Eckehard Berns
*/
/*
  @NOTATION@
*/

#ifndef __GNOME_STOCK_H__
#define __GNOME_STOCK_H__

#include <libgnome/gnome-defs.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>
#include "gnome-pixmap.h"
#include "gnome-stock-ids.h"

BEGIN_GNOME_DECLS

/*
 * The GnomeStock widget
 */


#define GNOME_TYPE_STOCK            (gnome_stock_get_type ())
#define GNOME_STOCK(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_STOCK, GnomeStock))
#define GNOME_STOCK_CLASS(klass)    (GTK_CHECK_CAST_CLASS ((klass), GNOME_TYPE_STOCK, GnomeStock))
#define GNOME_IS_STOCK(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_STOCK))
#define GNOME_IS_STOCK_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_STOCK))

typedef struct _GnomeStock       GnomeStock;
typedef struct _GnomeStockClass  GnomeStockClass;

struct _GnomeStock {
	GnomePixmap pixmap;

        /*< private >*/
        
        char *icon;
};

struct _GnomeStockClass {
	GnomePixmapClass pixmap_class;
};

guint         gnome_stock_get_type (void);
GtkWidget    *gnome_stock_new (void);
GtkWidget    *gnome_stock_new_with_icon (const char *icon);
GtkWidget    *gnome_stock_new_with_icon_at_size (const char *icon, int width, int height);
void          gnome_stock_set_icon (GnomeStock *stock, const char *icon);
void          gnome_stock_set_icon_at_size (GnomeStock *stock, const char *icon, int width, int height);

/*
 * The stock pixmap hash table
 */

/* forward declaration for opaque datatype. */
typedef union  _GnomeStockPixmapEntry        GnomeStockPixmapEntry;

/*
 * Hash table manipulation
 */

/* register a pixmap. returns non-zero, if successful */
void                   gnome_stock_pixmap_register (const char *icon,
                                                    GtkStateType state,
                                                    GnomeStockPixmapEntry *entry);

/* change an existing entry. returns non-zero on success */
void                   gnome_stock_pixmap_change   (const char *icon,
                                                    GtkStateType state,
                                                    GnomeStockPixmapEntry *entry);

/* check for the existance of an entry. returns the entry if it
   exists, or NULL otherwise */
GnomeStockPixmapEntry *gnome_stock_pixmap_lookup (const char *icon,
                                                  GtkStateType state);

/* Return a GdkPixmap and GdkMask for a stock pixmap (or NULL if no such icon) */
void gnome_stock_pixmap_gdk (const char *icon,
                             GtkStateType state,
			     GdkPixmap **pixmap,
			     GdkPixmap **mask);

/*
 * Hash entry datatype operations
 */

void gnome_stock_pixmap_entry_ref (GnomeStockPixmapEntry* entry);
void gnome_stock_pixmap_entry_unref (GnomeStockPixmapEntry* entry);

GnomeStockPixmapEntry *gnome_stock_pixmap_entry_new_from_gdk_pixbuf (GdkPixbuf *pixbuf,
                                                                     const gchar* label);

GnomeStockPixmapEntry *gnome_stock_pixmap_entry_new_from_gdk_pixbuf_at_size (GdkPixbuf *pixbuf,
                                                                             const gchar* label,
                                                                             gint width, gint height);

GnomeStockPixmapEntry *gnome_stock_pixmap_entry_new_from_filename (const gchar* filename,
                                                                   const gchar* label);

GnomeStockPixmapEntry *gnome_stock_pixmap_entry_new_from_pathname (const gchar* pathname,
                                                                   const gchar* label);

GnomeStockPixmapEntry *gnome_stock_pixmap_entry_new_from_xpm_data (const gchar** xpm_data,
                                                                   const gchar* label);


GdkPixbuf *gnome_stock_pixmap_entry_get_gdk_pixbuf(GnomeStockPixmapEntry *entry);

/*
 * Utility functions to retrieve buttons
 */

/* this function returns a button with a pixmap (if ButtonUseIcons is enabled)
 * and the provided text */

GtkWidget            *gnome_pixmap_button         (GtkWidget *pixmap,
						   const char *text);


/* returns a default button widget for dialogs */
GtkWidget             *gnome_stock_button          (const char *type);

/* Returns a button widget.  If the TYPE argument matches a
   GNOME_STOCK_BUTTON_* define, then a stock button is created.
   Otherwise, an ordinary button is created, and TYPE is given as the
   label.  */
GtkWidget             *gnome_stock_or_ordinary_button (const char *type);






/*
 * Menu item utility function
 */

/* returns a GtkMenuItem with an stock icon and text */
GtkWidget             *gnome_stock_menu_item       (const char *type,
						    const char *text);





/*
 * Stock menu accelerators
 */

/* To customize the accelerators add a file ~/.gnome/GnomeStock, wich looks
 * like that:
 *
 * [Accelerators]
 * Menu_New=Shft+Ctl+N
 * Menu_About=Ctl+A
 * Menu_Save As=Ctl+Shft+S
 * Menu_Quit=Alt+X
 */

/* this function returns the stock menu accelerators for the menu type in key
 * and mod */
gboolean	       gnome_stock_menu_accel      (const char *type,
						    guchar *key,
						    guint8 *mod);

/* apps can call this function at startup to add per app accelerator
 * redefinitions. section should be something like "/filename/section/" with
 * both the leading and trailing `/' */
void                   gnome_stock_menu_accel_parse(const char *section);





/* Should this be deprecated? Is anyone using it? what is it for? -hp */

/*
 * Creates a toplevel window with a shaped mask.  Useful for making the DnD
 * windows
 */
GtkWidget *gnome_stock_transparent_window (const char *icon, GtkStateType state);




END_GNOME_DECLS

#endif /* GNOME_STOCK_H */



