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

/* GnomeImageSelector widget - combo box with auto-saved history
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
#include "gnome-image-selector.h"

struct _GnomeImageSelectorPrivate {
};
	

static void   gnome_image_selector_class_init   (GnomeImageSelectorClass *class);
static void   gnome_image_selector_init         (GnomeImageSelector      *gselector);
static void   gnome_image_selector_finalize     (GObject                 *object);

static GObject*
gnome_image_selector_constructor (GType                  type,
				  guint                  n_construct_properties,
				  GObjectConstructParam *construct_properties);

static GnomeComponentWidgetClass *parent_class;

GType
gnome_image_selector_get_type (void)
{
    static GType selector_type = 0;

    if (!selector_type) {
	GtkTypeInfo selector_info = {
	    "GnomeImageSelector",
	    sizeof (GnomeImageSelector),
	    sizeof (GnomeImageSelectorClass),
	    (GtkClassInitFunc) gnome_image_selector_class_init,
	    (GtkObjectInitFunc) gnome_image_selector_init,
	    NULL,
	    NULL,
	    NULL
	};

	selector_type = gtk_type_unique (gnome_component_widget_get_type (), &selector_info);
    }

    return selector_type;
}

static void
gnome_image_selector_class_init (GnomeImageSelectorClass *class)
{
    GObjectClass *object_class;

    object_class = (GObjectClass *) class;

    parent_class = gtk_type_class (gnome_component_widget_get_type ());

    object_class->constructor = gnome_image_selector_constructor;
    object_class->finalize = gnome_image_selector_finalize;
}

static void
gnome_image_selector_init (GnomeImageSelector *gselector)
{
    gselector->_priv = g_new0 (GnomeImageSelectorPrivate, 1);
}

extern GnomeComponentWidget *gnome_component_widget_do_construct (GnomeComponentWidget *);

static GObject*
gnome_image_selector_constructor (GType                  type,
				  guint                  n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
    GObject *object = G_OBJECT_CLASS (parent_class)->constructor
	(type, n_construct_properties, construct_properties);
    GnomeImageSelector *iselector = GNOME_IMAGE_SELECTOR (object);
    Bonobo_Unknown corba_objref = CORBA_OBJECT_NIL;
    gchar *moniker = NULL;
    GValue value = { 0, };

    if (type != GNOME_TYPE_IMAGE_SELECTOR)
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

    moniker = g_strdup_printf ("OAFIID:GNOME_UI_Component_IconSelector");

    g_object_set (object, "moniker", moniker, NULL);

 out:
    g_free (moniker);
    if (!gnome_component_widget_do_construct (GNOME_COMPONENT_WIDGET (iselector)))
	return NULL;

    return object;
}

GtkWidget *
gnome_image_selector_new (void)
{
    return g_object_new (gnome_image_selector_get_type (), NULL);
}

static void
gnome_image_selector_finalize (GObject *object)
{
    GnomeImageSelector *iselector;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_IMAGE_SELECTOR (object));

    iselector = GNOME_IMAGE_SELECTOR (object);

    g_free (iselector->_priv);
    iselector->_priv = NULL;

    if (G_OBJECT_CLASS (parent_class)->finalize)
	(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}
