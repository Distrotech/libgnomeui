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
#include <libgnome/gnome-defs.h>
#include "gnome-file-selector.h"



BEGIN_GNOME_DECLS


#define GNOME_TYPE_ICON_ENTRY            (gnome_icon_entry_get_type ())
#define GNOME_ICON_ENTRY(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ICON_ENTRY, GnomeIconEntry))
#define GNOME_ICON_ENTRY_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ICON_ENTRY, GnomeIconEntryClass))
#define GNOME_IS_ICON_ENTRY(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ICON_ENTRY))
#define GNOME_IS_ICON_ENTRY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ICON_ENTRY))
#define GNOME_ICON_ENTRY_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_ICON_ENTRY, GnomeIconEntryClass))


typedef struct _GnomeIconEntry         GnomeIconEntry;
typedef struct _GnomeIconEntryPrivate  GnomeIconEntryPrivate;
typedef struct _GnomeIconEntryClass    GnomeIconEntryClass;

struct _GnomeIconEntry {
	GnomeFileSelector selector;
	
	/*< private >*/
	GnomeIconEntryPrivate *_priv;
};

struct _GnomeIconEntryClass {
	GnomeFileSelectorClass parent_class;
};


guint      gnome_icon_entry_get_type    (void) G_GNUC_CONST;
GtkWidget *gnome_icon_entry_new         (const gchar *history_id,
					 const gchar *browse_dialog_title);

/* for language bindings and subclassing, use gnome_icon_entry_new from C */
void       gnome_icon_entry_construct   (GnomeIconEntry *ientry,
					 const gchar *history_id,
					 const gchar *browse_dialog_title);

void       gnome_icon_entry_construct_full (GnomeIconEntry *ientry,
                                            const gchar *history_id,
                                            const gchar *dialog_title,
                                            GtkWidget *entry_widget,
                                            GtkWidget *selector_widget,
                                            GtkWidget *browse_dialog,
                                            guint32 flags);

/* returns the GnomeIconSelector widget of the browse dialog. */
GtkWidget *gnome_icon_entry_get_icon_selector (GnomeIconEntry *ientry);

void	   gnome_icon_entry_set_preview_size  (GnomeIconEntry *ientry,
					       guint preview_x,
					       guint preview_y);

END_GNOME_DECLS

#endif
