/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
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
#include <bonobo/bonobo-exception.h>
#include "gnome-macros.h"
#include "gnome-entry.h"

struct _GnomeEntryPrivate {
    Bonobo_PropertyBag pbag;

    gboolean constructed;

    gboolean is_file_entry;
};
	

static void   gnome_entry_class_init   (GnomeEntryClass *class);
static void   gnome_entry_init         (GnomeEntry      *gentry);
static void   gnome_entry_finalize     (GObject         *object);

static GObject*
gnome_entry_constructor (GType                  type,
			 guint                  n_construct_properties,
			 GObjectConstructParam *construct_properties);

static GnomeSelectorWidget *parent_class;

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

	entry_type = gtk_type_unique (gnome_selector_widget_get_type (), &entry_info);
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

    parent_class = gtk_type_class (gnome_selector_widget_get_type ());

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

    gobject_class->constructor = gnome_entry_constructor;
    gobject_class->finalize = gnome_entry_finalize;
}

static void
gnome_entry_init (GnomeEntry *gentry)
{
    gentry->_priv = g_new0 (GnomeEntryPrivate, 1);
}

extern GnomeSelectorWidget *gnome_selector_widget_do_construct (GnomeSelectorWidget *);

static GObject*
gnome_entry_constructor (GType                  type,
			 guint                  n_construct_properties,
			 GObjectConstructParam *construct_properties)
{
    GObject *object = G_OBJECT_CLASS (parent_class)->constructor
	(type, n_construct_properties, construct_properties);
    GnomeEntry *gentry = GNOME_ENTRY (object);
    GnomeEntryPrivate *priv = gentry->_priv;
    Bonobo_Unknown corba_objref = CORBA_OBJECT_NIL;
    gchar *moniker = NULL;
    GValue value = { 0, };
    CORBA_Environment ev;

    if (type != GNOME_TYPE_ENTRY)
	return object;

    g_value_init (&value, G_TYPE_POINTER);
    g_object_get_property (object, "corba-objref", &value);
    corba_objref = g_value_get_pointer (&value);
    g_value_unset (&value);

    if (corba_objref != CORBA_OBJECT_NIL)
	goto out;

    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (object, "moniker", &value);
    moniker = g_value_dup_string (&value);
    g_value_unset (&value);

    if (moniker != NULL)
	goto out;

    if (priv->is_file_entry)
	g_object_set (object, "moniker", "OAFIID:GNOME_UI_Component_FileEntry", NULL);
    else
	g_object_set (object, "moniker", "OAFIID:GNOME_UI_Component_Entry", NULL);

 out:
    g_free (moniker);
    if (!gnome_selector_widget_do_construct (GNOME_SELECTOR_WIDGET (gentry)))
	return NULL;

    corba_objref = bonobo_widget_get_objref (BONOBO_WIDGET (gentry));
    if (corba_objref == CORBA_OBJECT_NIL) {
	g_object_unref (object);
	return NULL;
    }

    CORBA_exception_init (&ev);
    priv->pbag = Bonobo_Unknown_queryInterface (corba_objref, "IDL:Bonobo/PropertyBag:1.0", &ev);
    CORBA_exception_free (&ev);

    return object;
}

GtkWidget *
gnome_entry_new (void)
{
    return g_object_new (gnome_entry_get_type (), "is-file-entry", FALSE, NULL);
}

GtkWidget *
gnome_file_entry_new (void)
{
    return g_object_new (gnome_entry_get_type (), "is-file-entry", TRUE, NULL);
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
    g_return_val_if_fail (gentry->_priv->pbag != CORBA_OBJECT_NIL, NULL);

    return bonobo_pbclient_get_string (gentry->_priv->pbag, "entry-text", NULL);
}

void
gnome_entry_set_text (GnomeEntry *gentry, const gchar *text)
{
    g_return_if_fail (gentry != NULL);
    g_return_if_fail (GNOME_IS_ENTRY (gentry));
    g_return_if_fail (gentry->_priv->pbag != CORBA_OBJECT_NIL);

    bonobo_pbclient_set_string (gentry->_priv->pbag, "entry-text", text, NULL);
}

