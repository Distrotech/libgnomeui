/* GnomeNumberEntry widget - Combo box with "Calculator" button for setting the number
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: George Lebl <jirka@5z.com>,
 *	   Federico Mena <federico@nuclecu.unam.mx> (the file entry which was a base for this)
 */

#ifndef GNOME_NUMBER_ENTRY_H
#define GNOME_NUMBER_ENTRY_H


#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-entry.h>


BEGIN_GNOME_DECLS


#define GNOME_NUMBER_ENTRY(obj)         GTK_CHECK_CAST (obj, gnome_number_entry_get_type (), GnomeNumberEntry)
#define GNOME_NUMBER_ENTRY_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_number_entry_get_type (), GnomeNumberEntryClass)
#define GNOME_IS_NUMBER_ENTRY(obj)      GTK_CHECK_TYPE (obj, gnome_number_entry_get_type ())


typedef struct _GnomeNumberEntry      GnomeNumberEntry;
typedef struct _GnomeNumberEntryClass GnomeNumberEntryClass;

struct _GnomeNumberEntry {
	GtkHBox hbox;

	char *calc_dialog_title;
	
	GtkWidget *calc_dlg;

	GtkWidget *gentry;
};

struct _GnomeNumberEntryClass {
	GtkHBoxClass parent_class;
};


guint      gnome_number_entry_get_type    (void);
GtkWidget *gnome_number_entry_new         (char *history_id,
					   char *calc_dialog_title);

GtkWidget *gnome_number_entry_gnome_entry (GnomeNumberEntry *nentry);
GtkWidget *gnome_number_entry_gtk_entry   (GnomeNumberEntry *nentry);
void       gnome_number_entry_set_title   (GnomeNumberEntry *nentry,
					   char *calc_dialog_title);

gdouble    gnome_number_entry_get_number  (GnomeNumberEntry *nentry);


END_GNOME_DECLS

#endif
