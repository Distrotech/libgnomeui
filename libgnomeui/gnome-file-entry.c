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

/* GnomeFileEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-file-selector.h>
#include "gnome-macros.h"
#include "gnome-file-entry.h"

struct _GnomeFileEntryPrivate {
};
	

static void   gnome_file_entry_class_init   (GnomeFileEntryClass *class);
static void   gnome_file_entry_init         (GnomeFileEntry      *gentry);
static void   gnome_file_entry_finalize     (GObject         *object);

static GnomeEntryClass *parent_class;

GType
gnome_file_entry_get_type (void)
{
	static GType entry_type = 0;

	if (!entry_type) {
		GtkTypeInfo entry_info = {
			"GnomeFileEntry",
			sizeof (GnomeFileEntry),
			sizeof (GnomeFileEntryClass),
			(GtkClassInitFunc) gnome_file_entry_class_init,
			(GtkObjectInitFunc) gnome_file_entry_init,
			NULL,
			NULL,
			NULL
		};

		entry_type = gtk_type_unique (gnome_entry_get_type (), &entry_info);
	}

	return entry_type;
}

static void
gnome_file_entry_class_init (GnomeFileEntryClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gnome_entry_get_type ());

	gobject_class->finalize = gnome_file_entry_finalize;
}

static void
gnome_file_entry_init (GnomeFileEntry *gentry)
{
	gentry->_priv = g_new0 (GnomeFileEntryPrivate, 1);
}

GtkWidget *
gnome_file_entry_construct (GnomeFileEntry     *fentry,
			    GNOME_Selector      corba_selector,
			    Bonobo_UIContainer  uic)
{
	g_return_val_if_fail (fentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), NULL);
	g_return_val_if_fail (corba_selector != CORBA_OBJECT_NIL, NULL);

	return (GtkWidget *) gnome_entry_construct
		(GNOME_ENTRY (fentry), corba_selector, uic);
}


/**
 * gnome_file_entry_new
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Description: Creates a new GnomeFileEntry widget.  If  @history_id is
 * not %NULL, then the history list will be saved and restored between
 * uses under the given id.
 *
 * Returns: Newly-created GnomeFileEntry widget.
 */
GtkWidget *
gnome_file_entry_new (const gchar *history_id)
{
	GnomeSelector *selector;

	selector = g_object_new (gnome_file_selector_get_type (),
				 "history_id", history_id,
				 NULL);

	return gnome_entry_new_full (selector, CORBA_OBJECT_NIL, history_id);
}

GtkWidget *
gnome_file_entry_new_from_selector (GNOME_Selector     corba_selector,
				    Bonobo_UIContainer uic)
{
	GnomeFileEntry *fentry;

	g_return_val_if_fail (corba_selector != CORBA_OBJECT_NIL, NULL);

	fentry = g_object_new (gnome_file_entry_get_type (), NULL);

	return gnome_file_entry_construct (fentry, corba_selector, uic);
}

static void
gnome_file_entry_finalize (GObject *object)
{
	GnomeFileEntry *gentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	gentry = GNOME_FILE_ENTRY (object);

	g_free (gentry->_priv);
	gentry->_priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}
