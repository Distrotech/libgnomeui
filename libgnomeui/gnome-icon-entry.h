/* GnomeIconEntry widget - Combo box with "Browse" button for files and
 *			   A pick button which can display a list of icons
 *			   in a current directory, the browse button displays
 *			   same dialog as pixmap-entry
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: George Lebl <jirka@5z.com>
 * icon selection based on original dentry-edit code which was:
 *	Written by: Havoc Pennington, based on code by John Ellis.
 */

#ifndef GNOME_ICON_ENTRY_H
#define GNOME_ICON_ENTRY_H


#include <gtk/gtkvbox.h>
#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-file-entry.h>


BEGIN_GNOME_DECLS


#define GNOME_TYPE_ICON_ENTRY            (gnome_icon_entry_get_type ())
#define GNOME_ICON_ENTRY(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ICON_ENTRY, GnomeIconEntry))
#define GNOME_ICON_ENTRY_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ICON_ENTRY, GnomeIconEntryClass))
#define GNOME_IS_ICON_ENTRY(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ICON_ENTRY))
#define GNOME_IS_ICON_ENTRY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ICON_ENTRY))


typedef struct _GnomeIconEntry      GnomeIconEntry;
typedef struct _GnomeIconEntryClass GnomeIconEntryClass;

struct _GnomeIconEntry {
	GtkVBox vbox;
	
	GtkWidget *fentry;

	GtkWidget *pickbutton;
	
	GtkWidget *pick_dialog;
	gchar *pick_dialog_dir;
};

struct _GnomeIconEntryClass {
	GtkVBoxClass parent_class;
};


guint      gnome_icon_entry_get_type    (void);
GtkWidget *gnome_icon_entry_new         (const gchar *history_id,
					 const gchar *browse_dialog_title);

/*by default gnome_pixmap entry sets the default directory to the
  gnome pixmap directory, this will set it to a subdirectory of that,
  or one would use the file_entry functions for any other path*/
void       gnome_icon_entry_set_pixmap_subdir(GnomeIconEntry *ientry,
					      const gchar *subdir);
void       gnome_icon_entry_set_icon(GnomeIconEntry *ientry,
				     const gchar *filename);
GtkWidget *gnome_icon_entry_gnome_file_entry(GnomeIconEntry *ientry);
GtkWidget *gnome_icon_entry_gnome_entry (GnomeIconEntry *ientry);
GtkWidget *gnome_icon_entry_gtk_entry   (GnomeIconEntry *ientry);

/*only return a file if it was possible to load it with imlib*/
gchar      *gnome_icon_entry_get_filename(GnomeIconEntry *ientry);

END_GNOME_DECLS

#endif
