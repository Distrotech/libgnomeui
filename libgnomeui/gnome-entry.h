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


#include <glib.h>
#include <libgnomebase/gnome-defs.h>
#include "gnome-selector.h"


BEGIN_GNOME_DECLS


#define GNOME_TYPE_ENTRY            (gnome_entry_get_type ())
#define GNOME_ENTRY(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ENTRY, GnomeEntry))
#define GNOME_ENTRY_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ENTRY, GnomeEntryClass))
#define GNOME_IS_ENTRY(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ENTRY))
#define GNOME_IS_ENTRY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ENTRY))
#define GNOME_ENTRY_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_ENTRY, GnomeEntryClass))


typedef struct _GnomeEntry        GnomeEntry;
typedef struct _GnomeEntryPrivate GnomeEntryPrivate;
typedef struct _GnomeEntryClass   GnomeEntryClass;

struct _GnomeEntry {
	GnomeSelector selector;

	/*< private >*/
	GnomeEntryPrivate *_priv;
};

struct _GnomeEntryClass {
	GnomeSelectorClass parent_class;
};


guint        gnome_entry_get_type         (void) G_GNUC_CONST;
GtkWidget   *gnome_entry_new              (const gchar *history_id);

gchar       *gnome_entry_get_text         (GnomeEntry  *gentry);

void         gnome_entry_set_text         (GnomeEntry  *gentry,
					   const gchar *text);

END_GNOME_DECLS

#endif
