/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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

/* GnomeEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_ENTRY_H
#define GNOME_ENTRY_H

#ifndef GNOME_DISABLE_DEPRECATED

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GNOME_TYPE_ENTRY            (gnome_entry_get_type ())
#define GNOME_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_ENTRY, GnomeEntry))
#define GNOME_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ENTRY, GnomeEntryClass))
#define GNOME_IS_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_ENTRY))
#define GNOME_IS_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ENTRY))
#define GNOME_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_ENTRY, GnomeEntryClass))

/* This also supports the GtkEditable interface so
 * to get text use the gtk_editable_get_chars method
 * on this object */

typedef struct _GnomeEntry        GnomeEntry;
typedef struct _GnomeEntryPrivate GnomeEntryPrivate;
typedef struct _GnomeEntryClass   GnomeEntryClass;

struct _GnomeEntry {
	GtkCombo combo;

	/*< private >*/
	GnomeEntryPrivate *_priv;
};

struct _GnomeEntryClass {
	GtkComboClass parent_class;

	/* Like the GtkEntry signals */
	void (* activate) (GnomeEntry *entry);

	gpointer reserved1, reserved2; /* Reserved for future use,
					  we'll need to proxy insert_text
					  and delete_text signals */
};


GType        gnome_entry_get_type         (void) G_GNUC_CONST;
GtkWidget   *gnome_entry_new              (const gchar *history_id);

/* for language bindings and subclassing, use gnome_entry_new */

GtkWidget   *gnome_entry_gtk_entry        (GnomeEntry  *gentry);

const gchar *gnome_entry_get_history_id   (GnomeEntry  *gentry);

void         gnome_entry_set_history_id   (GnomeEntry  *gentry,
					   const gchar *history_id);

void         gnome_entry_set_max_saved    (GnomeEntry  *gentry,
					   guint        max_saved);
guint        gnome_entry_get_max_saved    (GnomeEntry  *gentry);

void         gnome_entry_prepend_history  (GnomeEntry  *gentry,
					   gboolean    save,
					   const gchar *text);
void         gnome_entry_append_history   (GnomeEntry  *gentry,
					   gboolean     save,
					   const gchar *text);
void         gnome_entry_clear_history    (GnomeEntry  *gentry);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* GNOME_ENTRY_H */

