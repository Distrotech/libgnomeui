/* GnomeFileEntry widget - Combo box with "Browse" button for files
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_FILE_ENTRY_H
#define GNOME_FILE_ENTRY_H


#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-entry.h>


BEGIN_GNOME_DECLS


#define GNOME_FILE_ENTRY(obj)         GTK_CHECK_CAST (obj, gnome_file_entry_get_type (), GnomeFileEntry)
#define GNOME_FILE_ENTRY_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_file_entry_get_type (), GnomeFileEntryClass)
#define GNOME_IS_FILE_ENTRY(obj)      GTK_CHECK_TYPE (obj, gnome_file_entry_get_type ())


typedef struct _GnomeFileEntry      GnomeFileEntry;
typedef struct _GnomeFileEntryClass GnomeFileEntryClass;

struct _GnomeFileEntry {
	GtkHBox hbox;

	char *browse_dialog_title;

	GtkWidget *gentry;
};

struct _GnomeFileEntryClass {
	GtkHBoxClass parent_class;
};


guint      gnome_file_entry_get_type    (void);
GtkWidget *gnome_file_entry_new         (char *history_id, char *browse_dialog_title);

GtkWidget *gnome_file_entry_gnome_entry (GnomeFileEntry *fentry);
GtkWidget *gnome_file_entry_gtk_entry   (GnomeFileEntry *fentry);
void       gnome_file_entry_set_title   (GnomeFileEntry *fentry, char *browse_dialog_title);


END_GNOME_DECLS

#endif
