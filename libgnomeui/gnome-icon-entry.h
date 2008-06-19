/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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

/* GnomeIconEntry widget - Combo box with "Browse" button for files and
 *			   A pick button which can display a list of icons
 *			   in a current directory, the browse button displays
 *			   same dialog as pixmap-entry
 *
 * Author: George Lebl <jirka@5z.com>
 * icon selection based on original dentry-edit code which was:
 *	Written by: Havoc Pennington, based on code by John Ellis.
 */

#ifndef GNOME_ICON_ENTRY_H
#define GNOME_ICON_ENTRY_H


#include <glib.h>
#include <gtk/gtk.h>
#include <libgnomeui/gnome-file-entry.h>


G_BEGIN_DECLS


#define GNOME_TYPE_ICON_ENTRY            (gnome_icon_entry_get_type ())
#define GNOME_ICON_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_ICON_ENTRY, GnomeIconEntry))
#define GNOME_ICON_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ICON_ENTRY, GnomeIconEntryClass))
#define GNOME_IS_ICON_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_ICON_ENTRY))
#define GNOME_IS_ICON_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ICON_ENTRY))
#define GNOME_ICON_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_ICON_ENTRY, GnomeIconEntryClass))


typedef struct _GnomeIconEntry         GnomeIconEntry;
typedef struct _GnomeIconEntryPrivate  GnomeIconEntryPrivate;
typedef struct _GnomeIconEntryClass    GnomeIconEntryClass;

struct _GnomeIconEntry {
	GtkVBox vbox;
	
	/*< private >*/
	GnomeIconEntryPrivate *_priv;
};

struct _GnomeIconEntryClass {
	GtkVBoxClass parent_class;

	void (*changed) (GnomeIconEntry *ientry);
	void (*browse) (GnomeIconEntry *ientry);

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};


GType      gnome_icon_entry_get_type    (void) G_GNUC_CONST;
GtkWidget *gnome_icon_entry_new         (const gchar *history_id,
					 const gchar *browse_dialog_title);

/* for language bindings and subclassing, use gnome_icon_entry_new from C */
void       gnome_icon_entry_construct   (GnomeIconEntry *ientry,
					 const gchar *history_id,
					 const gchar *browse_dialog_title);

/*by default gnome_pixmap entry sets the default directory to the
  gnome pixmap directory, this will set it to a subdirectory of that,
  or one would use the file_entry functions for any other path*/
void       gnome_icon_entry_set_pixmap_subdir(GnomeIconEntry *ientry,
					      const gchar *subdir);

/*only return a file if it was possible to load it with gdk-pixbuf*/
gchar      *gnome_icon_entry_get_filename(GnomeIconEntry *ientry);

/* set the icon to something, returns TRUE on success */
gboolean   gnome_icon_entry_set_filename(GnomeIconEntry *ientry,
					 const gchar *filename);

void       gnome_icon_entry_set_browse_dialog_title(GnomeIconEntry *ientry,
						    const gchar *browse_dialog_title);
void       gnome_icon_entry_set_history_id(GnomeIconEntry *ientry,
					   const gchar *history_id);
void       gnome_icon_entry_set_max_saved (GnomeIconEntry *ientry,
					   guint max_saved);

GtkWidget *gnome_icon_entry_pick_dialog	(GnomeIconEntry *ientry);

#ifndef GNOME_EXCLUDE_DEPRECATED
/* DEPRECATED routines left for compatibility only, will disapear in
 * some very distant future */
/* this is deprecated in favour of the above */
void       gnome_icon_entry_set_icon(GnomeIconEntry *ientry,
				     const gchar *filename);
GtkWidget *gnome_icon_entry_gnome_file_entry(GnomeIconEntry *ientry);
GtkWidget *gnome_icon_entry_gnome_entry (GnomeIconEntry *ientry);
GtkWidget *gnome_icon_entry_gtk_entry   (GnomeIconEntry *ientry);
#endif


G_END_DECLS

#endif
