/* Stock icons, buttons, and menu items.
   Copyright (C) 1997, 1998 Free Software Foundation

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

   Author: Eckehard Berns  */

#ifndef __GNOME_STOCK_H__
#define __GNOME_STOCK_H__

#include <libgnome/gnome-defs.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>
#include "gnome-pixmap.h"

BEGIN_GNOME_DECLS

/*
 * The GnomeStock widget
 */


#define GNOME_STOCK(obj)         GTK_CHECK_CAST(obj, gnome_stock_get_type(), GnomeStock)
#define GNOME_STOCK_CLASS(klass) GTK_CHECK_CAST_CLASS(obj, gnome_stock_get_type(), GnomeStock)
#define GNOME_IS_STOCK(obj)      GTK_CHECK_TYPE(obj, gnome_stock_get_type())

typedef struct _GnomeStock       GnomeStock;
typedef struct _GnomeStockClass  GnomeStockClass;

struct _GnomeStock {
	GnomePixmap pixmap;
        char *icon;
};

struct _GnomeStockClass {
	GnomePixmapClass pixmap_class;
};

guint         gnome_stock_get_type(void);
GtkWidget    *gnome_stock_new(void);
GtkWidget    *gnome_stock_new_with_icon(const char *icon);
GtkWidget    *gnome_stock_new_with_icon_at_size(const char *icon, int width, int height);
gboolean      gnome_stock_set_icon(GnomeStock *stock, const char *icon);
gboolean      gnome_stock_set_icon_at_size(GnomeStock *stock, const char *icon, int width, int height);





/* Structures for the internal stock image hash table entries */

typedef struct _GnomeStockPixmapEntryAny     GnomeStockPixmapEntryAny;
typedef struct _GnomeStockPixmapEntryData    GnomeStockPixmapEntryData;
typedef struct _GnomeStockPixmapEntryFile    GnomeStockPixmapEntryFile;
typedef struct _GnomeStockPixmapEntryPath    GnomeStockPixmapEntryPath;
typedef struct _GnomeStockPixmapEntryWidget  GnomeStockPixmapEntryWidget;
typedef struct _GnomeStockPixmapEntryGPixmap GnomeStockPixmapEntryGPixmap;
typedef union  _GnomeStockPixmapEntry        GnomeStockPixmapEntry;

typedef enum {
        GNOME_STOCK_PIXMAP_TYPE_NONE,
        GNOME_STOCK_PIXMAP_TYPE_DATA,
        GNOME_STOCK_PIXMAP_TYPE_FILE,
        GNOME_STOCK_PIXMAP_TYPE_PATH,
        GNOME_STOCK_PIXMAP_TYPE_WIDGET,
	GNOME_STOCK_PIXMAP_TYPE_GPIXMAP
} GnomeStockPixmapType;


/* a data entry holds a hardcoded pixmap */
struct _GnomeStockPixmapEntryData {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        const gchar **xpm_data;
};

/* a file entry holds a filename (no path) to the pixamp. this pixmap
   will be seached for using gnome_pixmap_file */
struct _GnomeStockPixmapEntryFile {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        gchar *filename;
};

/* a path entry holds the complete (absolut) path to the pixmap file */
struct _GnomeStockPixmapEntryPath {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        gchar *pathname;
};

/* a widget entry holds a GnomeStockPixmapWidget. This kind of icon can be
 * used by a theme to completely change the handling of a stock icon. */
struct _GnomeStockPixmapEntryWidget {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        GtkWidget *widget;
};

/* a GnomePixmap */
struct _GnomeStockPixmapEntryGPixmap {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        GnomePixmap *pixmap;
};

struct _GnomeStockPixmapEntryAny {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
};

union _GnomeStockPixmapEntry {
        GnomeStockPixmapType type;
        GnomeStockPixmapEntryAny any;
        GnomeStockPixmapEntryData data;
        GnomeStockPixmapEntryFile file;
        GnomeStockPixmapEntryPath path;
        GnomeStockPixmapEntryWidget widget;
        GnomeStockPixmapEntryGPixmap gpixmap;
};




/*
 * The stock pixmap hash table
 */

/* register a pixmap. returns non-zero, if successful */
gint                   gnome_stock_pixmap_register (const char *icon,
						    const char *subtype,
                                                    GnomeStockPixmapEntry *entry);

/* change an existing entry. returns non-zero on success */
gint                   gnome_stock_pixmap_change   (const char *icon,
						    const char *subtype,
                                                    GnomeStockPixmapEntry *entry);

/* check for the existance of an entry. returns the entry if it
   exists, or NULL otherwise */
GnomeStockPixmapEntry *gnome_stock_pixmap_checkfor (const char *icon,
						    const char *subtype);

/* Return a GdkPixmap and GdkMask for a stock pixmap */
void gnome_stock_pixmap_gdk (const char *icon,
			     const char *subtype,
			     GdkPixmap **pixmap,
			     GdkPixmap **mask);






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





/* Should this be deprecated? Is anyone using it? -hp */

/*
 * Creates a toplevel window with a shaped mask.  Useful for making the DnD
 * windows
 */
GtkWidget *gnome_stock_transparent_window (const char *icon, const char *subtype);




END_GNOME_DECLS

#endif /* GNOME_STOCK_H */



