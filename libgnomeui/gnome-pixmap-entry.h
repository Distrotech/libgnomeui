/* GnomePixmapEntry widget - Combo box with "Browse" button for files and
 *			     preview space for pixmaps and a file picker in
 *			     electric eyes style
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Authors: George Lebl <jirka@5z.com>
 *	    Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_PIXMAP_ENTRY_H
#define GNOME_PIXMAP_ENTRY_H


#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-entry.h>


BEGIN_GNOME_DECLS


#define GNOME_PIXMAP_ENTRY(obj)         GTK_CHECK_CAST (obj, gnome_pixmap_entry_get_type (), GnomePixmapEntry)
#define GNOME_PIXMAP_ENTRY_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_pixmap_entry_get_type (), GnomePixmapEntryClass)
#define GNOME_IS_PIXMAP_ENTRY(obj)      GTK_CHECK_TYPE (obj, gnome_pixmap_entry_get_type ())


typedef struct _GnomePixmapEntry      GnomePixmapEntry;
typedef struct _GnomePixmapEntryClass GnomePixmapEntryClass;

struct _GnomePixmapEntry {
	GtkHBox hbox;

	char *browse_dialog_title;
	int do_preview; /*put a preview frame with the pixmap next to
			  the entry*/
	int preview_w,preview_h;  /*preview width and height, if the image
				    is larger, it will be scaled*/
	
	GtkWidget *gentry;
	GtkWidget *preview;
};

struct _GnomePixmapEntryClass {
	GtkHBoxClass parent_class;
};


guint      gnome_pixmap_entry_get_type    (void);
GtkWidget *gnome_pixmap_entry_new         (char *history_id,
					   char *browse_dialog_title,
					   int do_preview,
					   int preview_w,
					   int preview_h);

GtkWidget *gnome_pixmap_entry_gnome_entry (GnomePixmapEntry *pentry);
GtkWidget *gnome_pixmap_entry_gtk_entry   (GnomePixmapEntry *pentry);
void       gnome_pixmap_entry_set_title   (GnomePixmapEntry *pentry,
					   char *browse_dialog_title);
void       gnome_pixmap_entry_set_preview (GnomePixmapEntry *pentry,
					   int do_preview,
					   int preview_w,
					   int preview_h);


END_GNOME_DECLS

#endif
