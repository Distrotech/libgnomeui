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

static GnomeSelectorClientClass *parent_class;

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

	selector_type = gtk_type_unique (gnome_selector_client_get_type (), &selector_info);
    }

    return selector_type;
}

static void
gnome_image_selector_class_init (GnomeImageSelectorClass *class)
{
    GtkObjectClass *object_class;
    GObjectClass *gobject_class;

    object_class = (GtkObjectClass *) class;
    gobject_class = (GObjectClass *) class;

    parent_class = gtk_type_class (gnome_selector_client_get_type ());

    gobject_class->finalize = gnome_image_selector_finalize;
}

static void
gnome_image_selector_init (GnomeImageSelector *gselector)
{
    gselector->_priv = g_new0 (GnomeImageSelectorPrivate, 1);
}

GtkWidget *
gnome_image_selector_new (void)
{
    GnomeImageSelector *iselector;

    iselector = g_object_new (gnome_image_selector_get_type (),
			      NULL);

    return (GtkWidget *) gnome_selector_client_construct
	(GNOME_SELECTOR_CLIENT (iselector),
	 "OAFIID:GNOME_UI_Component_IconSelector",
	 CORBA_OBJECT_NIL);
}

GtkWidget *
gnome_image_selector_new_from_selector (GNOME_Selector     corba_selector,
					Bonobo_UIContainer uic)
{
    GnomeImageSelector *iselector;

    g_return_val_if_fail (corba_selector != CORBA_OBJECT_NIL, NULL);

    iselector = g_object_new (gnome_image_selector_get_type (), NULL);

    return (GtkWidget *) gnome_selector_client_construct_from_objref
	(GNOME_SELECTOR_CLIENT (iselector), corba_selector, uic);
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
