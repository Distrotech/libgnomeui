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

/* GnomeImageEntry widget - combo box with auto-saved history
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
#include <bonobo/bonobo-moniker-util.h>
#include <bonobo/bonobo-exception.h>
#include "gnome-image-entry.h"

#define ICON_SIZE 48

struct _GnomeImageEntryPrivate {
    gboolean constructed;

    gboolean is_pixmap_entry;
    guint preview_x, preview_y;
};

enum {
    PROP_0,

    /* Construction properties */
    PROP_IS_PIXMAP_ENTRY,

    /* Normal properties */
    PROP_PREVIEW_X,
    PROP_PREVIEW_Y
};

static void   gnome_image_entry_class_init   (GnomeImageEntryClass *class);
static void   gnome_image_entry_init         (GnomeImageEntry      *gentry);
static void   gnome_image_entry_finalize     (GObject              *object);

static GObject*
gnome_image_entry_constructor (GType                  type,
			       guint                  n_construct_properties,
			       GObjectConstructParam *construct_properties);

static GnomeSelectorWidgetClass *parent_class;

GType
gnome_image_entry_get_type (void)
{
    static GType entry_type = 0;

    if (!entry_type) {
	GtkTypeInfo entry_info = {
	    "GnomeImageEntry",
	    sizeof (GnomeImageEntry),
	    sizeof (GnomeImageEntryClass),
	    (GtkClassInitFunc) gnome_image_entry_class_init,
	    (GtkObjectInitFunc) gnome_image_entry_init,
	    NULL,
	    NULL,
	    NULL
	};

	entry_type = gtk_type_unique (gnome_selector_widget_get_type (), &entry_info);
    }

    return entry_type;
}

static void
gnome_image_entry_set_property (GObject *object, guint param_id,
				const GValue *value, GParamSpec *pspec)
{
    GnomeImageEntry *ientry;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_IMAGE_ENTRY (object));

    ientry = GNOME_IMAGE_ENTRY (object);

    switch (param_id) {
    case PROP_IS_PIXMAP_ENTRY:
	g_assert (!ientry->_priv->constructed);
	ientry->_priv->is_pixmap_entry = g_value_get_boolean (value);
	break;
    case PROP_PREVIEW_X:
	ientry->_priv->preview_x = g_value_get_uint (value);
	break;
    case PROP_PREVIEW_Y:
	ientry->_priv->preview_y = g_value_get_uint (value);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
gnome_image_entry_get_property (GObject *object, guint param_id, GValue *value,
				GParamSpec *pspec)
{
    GnomeImageEntry *ientry;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_IMAGE_ENTRY (object));

    ientry = GNOME_IMAGE_ENTRY (object);

    switch (param_id) {
    case PROP_IS_PIXMAP_ENTRY:
	g_value_set_boolean (value, ientry->_priv->is_pixmap_entry);
	break;
    case PROP_PREVIEW_X:
	g_value_set_uint (value, ientry->_priv->preview_x);
	break;
    case PROP_PREVIEW_Y:
	g_value_set_uint (value, ientry->_priv->preview_y);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
gnome_image_entry_class_init (GnomeImageEntryClass *class)
{
    GObjectClass *object_class;

    object_class = (GObjectClass *) class;

    parent_class = gtk_type_class (bonobo_widget_get_type ());

    object_class->constructor = gnome_image_entry_constructor;
    object_class->finalize = gnome_image_entry_finalize;

    object_class->get_property = gnome_image_entry_get_property;
    object_class->set_property = gnome_image_entry_set_property;

    /* Construction properties */
    g_object_class_install_property
	(object_class,
	 PROP_IS_PIXMAP_ENTRY,
	 g_param_spec_boolean ("is-pixmap-entry", NULL, NULL,
			       FALSE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));

    /* Normal properties */
    g_object_class_install_property
	(object_class,
	 PROP_PREVIEW_X,
	 g_param_spec_uint ("preview-x", NULL, NULL,
			    0, G_MAXINT, ICON_SIZE,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE |
			     G_PARAM_CONSTRUCT)));
    g_object_class_install_property
	(object_class,
	 PROP_PREVIEW_Y,
	 g_param_spec_uint ("preview-y", NULL, NULL,
			    0, G_MAXINT, ICON_SIZE,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE |
			     G_PARAM_CONSTRUCT)));
}

static void
gnome_image_entry_init (GnomeImageEntry *gentry)
{
    gentry->_priv = g_new0 (GnomeImageEntryPrivate, 1);
}

extern GnomeSelectorWidget *gnome_selector_widget_do_construct (GnomeSelectorWidget *);

static GObject*
gnome_image_entry_constructor (GType                  type,
			       guint                  n_construct_properties,
			       GObjectConstructParam *construct_properties)
{
    GObject *object = G_OBJECT_CLASS (parent_class)->constructor
	(type, n_construct_properties, construct_properties);
    GnomeImageEntry *ientry = GNOME_IMAGE_ENTRY (object);
    GnomeImageEntryPrivate *priv = ientry->_priv;
    Bonobo_Unknown corba_objref = CORBA_OBJECT_NIL;
    gchar *moniker = NULL;
    GValue value = { 0, };

    if (type != GNOME_TYPE_IMAGE_ENTRY)
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

    if (priv->is_pixmap_entry)
	moniker = g_strdup_printf ("%s!type=pixmap;preview-x=%d;preview-y=%d",
				   "OAFIID:GNOME_UI_Component_ImageEntry_Item",
				   priv->preview_x, priv->preview_y);
    else
	moniker = g_strdup_printf ("%s!type=icon", "OAFIID:GNOME_UI_Component_ImageEntry_Item");

    g_object_set (object, "moniker", moniker, NULL);

 out:
    g_free (moniker);
    if (!gnome_selector_widget_do_construct (GNOME_SELECTOR_WIDGET (ientry)))
	return NULL;

    return object;
}

GtkWidget *
gnome_image_entry_new_icon_entry (void)
{
    return g_object_new (gnome_image_entry_get_type (),
			 "is-pixmap-entry", FALSE, NULL);
}

GtkWidget *
gnome_image_entry_new_pixmap_entry (guint preview_x, guint preview_y)
{
    return g_object_new (gnome_image_entry_get_type (),
			 "preview-x", preview_x, "preview-y", preview_y,
			 "is-pixmap-entry", TRUE, NULL);
}

static void
gnome_image_entry_finalize (GObject *object)
{
    GnomeImageEntry *ientry;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_IMAGE_ENTRY (object));

    ientry = GNOME_IMAGE_ENTRY (object);

    g_free (ientry->_priv);
    ientry->_priv = NULL;

    if (G_OBJECT_CLASS (parent_class)->finalize)
	(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}
