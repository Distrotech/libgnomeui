/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* GnomePixmapEntry widget - Combo box with "Browse" button for files and
 *			     preview space for pixmaps and a file picker in
 *			     electric eyes style (well kind of)
 *
 *
 * Author: George Lebl <jirka@5z.com>
 */

#ifndef GNOME_PIXMAP_ENTRY_H
#define GNOME_PIXMAP_ENTRY_H


#include <gtk/gtkvbox.h>
#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-file-entry.h>


BEGIN_GNOME_DECLS


#define GNOME_TYPE_PIXMAP_ENTRY            (gnome_pixmap_entry_get_type ())
#define GNOME_PIXMAP_ENTRY(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_PIXMAP_ENTRY, GnomePixmapEntry))
#define GNOME_PIXMAP_ENTRY_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_PIXMAP_ENTRY, GnomePixmapEntryClass))
#define GNOME_IS_PIXMAP_ENTRY(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_PIXMAP_ENTRY))
#define GNOME_IS_PIXMAP_ENTRY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_PIXMAP_ENTRY))
#define GNOME_PIXMAP_ENTRY_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_PIXMAP_ENTRY, GnomePixmapEntryClass))


typedef struct _GnomePixmapEntry        GnomePixmapEntry;
typedef struct _GnomePixmapEntryPrivate GnomePixmapEntryPrivate;
typedef struct _GnomePixmapEntryClass   GnomePixmapEntryClass;

struct _GnomePixmapEntry {
	GtkVBox vbox;

	/*< private >*/
	GnomePixmapEntryPrivate *_priv;
};

struct _GnomePixmapEntryClass {
	GtkVBoxClass parent_class;
};


guint      gnome_pixmap_entry_get_type    (void);
GtkWidget *gnome_pixmap_entry_new         (const gchar *history_id,
					   const gchar *browse_dialog_title,
					   gboolean do_preview);
void       gnome_pixmap_entry_construct   (GnomePixmapEntry *gentry,
					   const gchar *history_id,
					   const gchar *browse_dialog_title,
					   gboolean do_preview);

/*by default gnome_pixmap entry sets the default directory to the
  gnome pixmap directory, this will set it to a subdirectory of that,
  or one would use the file_entry functions for any other path*/
void       gnome_pixmap_entry_set_pixmap_subdir(GnomePixmapEntry *pentry,
						const gchar *subdir);

/* entry widgets */
GtkWidget *gnome_pixmap_entry_gnome_file_entry(GnomePixmapEntry *pentry);
GtkWidget *gnome_pixmap_entry_gnome_entry (GnomePixmapEntry *pentry);
GtkWidget *gnome_pixmap_entry_gtk_entry   (GnomePixmapEntry *pentry);

/* preview widgets */
GtkWidget  *gnome_pixmap_entry_scrolled_window(GnomePixmapEntry *pentry);
GtkWidget  *gnome_pixmap_entry_preview_widget(GnomePixmapEntry *pentry);


/*set the preview parameters, if preview is off then the preview frame
  will be hidden*/
void       gnome_pixmap_entry_set_preview (GnomePixmapEntry *pentry,
					   gboolean do_preview);
void	   gnome_pixmap_entry_set_preview_size(GnomePixmapEntry *pentry,
					       gint preview_w,
					       gint preview_h);

/*only return a file if it was possible to load it with gdk-pixbuf*/
gchar      *gnome_pixmap_entry_get_filename(GnomePixmapEntry *pentry);

END_GNOME_DECLS

#endif
