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

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-selector.h>
#include "gnome-macros.h"
#include "gnome-entry.h"

struct _GnomeEntryPrivate {
	gboolean constructed;

	gboolean is_file_entry;
};
	

static void   gnome_entry_class_init   (GnomeEntryClass *class);
static void   gnome_entry_init         (GnomeEntry      *gentry);
static void   gnome_entry_finalize     (GObject         *object);

static GnomeSelectorClientClass *parent_class;

enum {
    PROP_0,

    /* Construction properties */
    PROP_IS_FILE_ENTRY,
};

GType
gnome_entry_get_type (void)
{
	static GType entry_type = 0;

	if (!entry_type) {
		GtkTypeInfo entry_info = {
			"GnomeEntry",
			sizeof (GnomeEntry),
			sizeof (GnomeEntryClass),
			(GtkClassInitFunc) gnome_entry_class_init,
			(GtkObjectInitFunc) gnome_entry_init,
			NULL,
			NULL,
			NULL
		};

		entry_type = gtk_type_unique (gnome_selector_client_get_type (), &entry_info);
	}

	return entry_type;
}

static void
gnome_entry_set_property (GObject *object, guint param_id,
			  const GValue *value, GParamSpec *pspec)
{
	GnomeEntry *entry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	entry = GNOME_ENTRY (object);

	switch (param_id) {
	case PROP_IS_FILE_ENTRY:
		g_assert (!entry->_priv->constructed);
		entry->_priv->is_file_entry = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_entry_get_property (GObject *object, guint param_id, GValue *value,
			  GParamSpec *pspec)
{
	GnomeEntry *entry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	entry = GNOME_ENTRY (object);

	switch (param_id) {
	case PROP_IS_FILE_ENTRY:
		g_value_set_boolean (value, entry->_priv->is_file_entry);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_entry_class_init (GnomeEntryClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gnome_selector_client_get_type ());

	gobject_class->get_property = gnome_entry_get_property;
	gobject_class->set_property = gnome_entry_set_property;

	/* Construction properties */
	g_object_class_install_property
		(gobject_class,
		 PROP_IS_FILE_ENTRY,
		 g_param_spec_boolean ("is-file-entry", NULL, NULL,
				       FALSE,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE |
					G_PARAM_CONSTRUCT_ONLY)));

	gobject_class->finalize = gnome_entry_finalize;
}

static void
gnome_entry_init (GnomeEntry *gentry)
{
	gentry->_priv = g_new0 (GnomeEntryPrivate, 1);
}

GtkWidget *
gnome_entry_construct (GnomeEntry         *gentry,
		       GNOME_Selector      corba_selector,
		       Bonobo_UIContainer  uic)
{
	g_return_val_if_fail (gentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ENTRY (gentry), NULL);
	g_return_val_if_fail (corba_selector != CORBA_OBJECT_NIL, NULL);

	return (GtkWidget *) gnome_selector_client_construct_from_objref
		(GNOME_SELECTOR_CLIENT (gentry), corba_selector, uic);
}

GtkWidget *
gnome_entry_new (void)
{
	GnomeEntry *entry;

	entry = g_object_new (gnome_entry_get_type (),
			      "is-file-entry", FALSE,
			      NULL);

	return (GtkWidget *) gnome_selector_client_construct
		(GNOME_SELECTOR_CLIENT (entry),
		 "OAFIID:GNOME_UI_Component_Entry",
		 CORBA_OBJECT_NIL);
}

GtkWidget *
gnome_file_entry_new (void)
{
	GnomeEntry *entry;

	entry = g_object_new (gnome_entry_get_type (),
			      "is-file-entry", TRUE,
			      NULL);

	return (GtkWidget *) gnome_selector_client_construct
		(GNOME_SELECTOR_CLIENT (entry),
		 "OAFIID:GNOME_UI_Component_Entry",
		 CORBA_OBJECT_NIL);
}

GtkWidget *
gnome_entry_new_from_selector (GNOME_Selector     corba_selector,
			       Bonobo_UIContainer uic)
{
	GnomeEntry *entry;

	g_return_val_if_fail (corba_selector != CORBA_OBJECT_NIL, NULL);

	entry = g_object_new (gnome_entry_get_type (), NULL);

	return (GtkWidget *) gnome_selector_client_construct_from_objref
		(GNOME_SELECTOR_CLIENT (entry), corba_selector, uic);
}

static void
gnome_entry_finalize (GObject *object)
{
	GnomeEntry *gentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	gentry = GNOME_ENTRY (object);

	g_free (gentry->_priv);
	gentry->_priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

gchar *
gnome_entry_get_text (GnomeEntry *gentry)
{
	g_return_val_if_fail (gentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ENTRY (gentry), NULL);

#ifdef FIXME
	return gnome_selector_client_get_entry_text (GNOME_SELECTOR_CLIENT (gentry));
#else
	return NULL;
#endif
}

void
gnome_entry_set_text (GnomeEntry *gentry, const gchar *text)
{
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

#ifdef FIXME
	gnome_selector_client_set_entry_text (GNOME_SELECTOR_CLIENT (gentry),
					      text);
#endif
}

