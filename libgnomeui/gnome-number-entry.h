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

/* GnomeNumberEntry widget - Combo box with "Calculator" button for setting the number
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

	/*< private >*/
	gchar *calc_dialog_title;
	
	GtkWidget *calc_dlg;

	GtkWidget *gentry;
};

struct _GnomeNumberEntryClass {
	GtkHBoxClass parent_class;
};


guint      gnome_number_entry_get_type    (void);
GtkWidget *gnome_number_entry_new         (const gchar *history_id,
					   const gchar *calc_dialog_title);

/* for language bindings and subclassing use gnome_number_entry_new from C */
void       gnome_number_entry_construct   (GnomeNumberEntry *nentry,
					   const gchar *history_id,
					   const gchar *calc_dialog_title);

GtkWidget *gnome_number_entry_gnome_entry (GnomeNumberEntry *nentry);
GtkWidget *gnome_number_entry_gtk_entry   (GnomeNumberEntry *nentry);
void       gnome_number_entry_set_title   (GnomeNumberEntry *nentry,
					   const gchar *calc_dialog_title);

gdouble    gnome_number_entry_get_number  (GnomeNumberEntry *nentry);
void       gnome_number_entry_set_number  (GnomeNumberEntry *nentry,
					   double number);


END_GNOME_DECLS

#endif
