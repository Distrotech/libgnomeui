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

/* GnomeFileEntry widget - Combo box with "Browse" button for files
 *
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_FILE_ENTRY_H
#define GNOME_FILE_ENTRY_H


#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-entry.h>


BEGIN_GNOME_DECLS


#define GNOME_TYPE_FILE_ENTRY            (gnome_file_entry_get_type ())
#define GNOME_FILE_ENTRY(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_FILE_ENTRY, GnomeFileEntry))
#define GNOME_FILE_ENTRY_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_FILE_ENTRY, GnomeFileEntryClass))
#define GNOME_IS_FILE_ENTRY(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_FILE_ENTRY))
#define GNOME_IS_FILE_ENTRY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_FILE_ENTRY))


typedef struct _GnomeFileEntry      GnomeFileEntry;
typedef struct _GnomeFileEntryClass GnomeFileEntryClass;

struct _GnomeFileEntry {
	GtkHBox hbox;

	char *browse_dialog_title;
	char *default_path;

	/*the file dialog widget*/
	GtkWidget *fsw;

	GtkWidget *gentry;

	gboolean is_modal : 1;

	gboolean directory_entry : 1; /*optional flag to only do directories*/
};

struct _GnomeFileEntryClass {
	GtkHBoxClass parent_class;

	/*if you want to modify the browse dialog, bind this with
	  connect_after and modify object->fsw, or you could just
	  create your own and set it to object->fsw in a normally
	  connected handler, it has to be a gtk_file_selection though*/
	void (* browse_clicked)(GnomeFileEntry *fentry);
};


guint      gnome_file_entry_get_type    (void);
GtkWidget *gnome_file_entry_new         (const char *history_id,
					 const char *browse_dialog_title);
void       gnome_file_entry_construct  (GnomeFileEntry *fentry,
					const char *history_id,
					const char *browse_dialog_title);

GtkWidget *gnome_file_entry_gnome_entry (GnomeFileEntry *fentry);
GtkWidget *gnome_file_entry_gtk_entry   (GnomeFileEntry *fentry);
void       gnome_file_entry_set_title   (GnomeFileEntry *fentry,
					 const char *browse_dialog_title);

/*set default path for the browse dialog*/
void	   gnome_file_entry_set_default_path(GnomeFileEntry *fentry,
					     const char *path);

/*sets up the file entry to be a directory picker rather then a file picker*/
void	   gnome_file_entry_set_directory(GnomeFileEntry *fentry,
					  gboolean directory_entry);

/*returns a filename which is a full path with WD or the default
  directory prepended if it's not an absolute path, returns
  NULL on empty entry or if the file doesn't exist and that was
  a requirement*/
char      *gnome_file_entry_get_full_path(GnomeFileEntry *fentry,
					  gboolean file_must_exist);

/* set the filename to something, this is like setting the internal
 * GtkEntry */
void       gnome_file_entry_set_filename(GnomeFileEntry *fentry,
					 const char *filename);

/*set modality of the file browse dialog, only applies for the
  next time a dialog is created*/
void       gnome_file_entry_set_modal	(GnomeFileEntry *fentry,
					 gboolean is_modal);

END_GNOME_DECLS

#endif
