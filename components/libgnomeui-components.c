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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bonobo/bonobo-ui-main.h>
#include <bonobo/bonobo-shlib-factory.h>
#include <bonobo/bonobo-item-handler.h>
#include <libgnome/gnome-selector-factory.h>
#include "gnome-icon-selector-component.h"
#include "gnome-image-entry-component.h"
#include "gnome-entry-component.h"

static Bonobo_Unknown
image_entry_get_object_fn (BonoboItemHandler *h, const char *item_name,
			   gboolean only_if_exists, gpointer data,
			   CORBA_Environment *ev)
{
    GSList *options, *c;
    gboolean is_pixmap_entry = FALSE;
    guint preview_x = 0, preview_y = 0;
    GnomeSelector *selector = NULL;

    options = bonobo_item_option_parse (item_name);

    for (c = options; c; c = c->next) {
	BonoboItemOption *option = c->data;

	if (!strcmp (option->key, "type")) {
	    is_pixmap_entry = !strcmp (option->value, "pixmap");
	} else if (!strcmp (option->key, "preview_x")) {
	    preview_x = atoi (option->value);
	} else if (!strcmp (option->key, "preview_y")) {
	    preview_y = atoi (option->value);
	} else {
	    g_warning (G_STRLOC ": unknown option `%s'", option->key);
	}
    }

    if (is_pixmap_entry) {
	if ((preview_x > 0) && (preview_y) > 0)
	    selector = g_object_new (gnome_image_entry_component_get_type (),
				     "is_pixmap_entry", TRUE,
				     "preview_x", preview_x,
				     "preview_y", preview_y,
				     NULL);
	else
	    selector = g_object_new (gnome_image_entry_component_get_type (),
				     "is_pixmap_entry", TRUE,
				     NULL);
    } else {
	selector = g_object_new (gnome_image_entry_component_get_type (),
				 "is_pixmap_entry", FALSE,
				 NULL);
    }

    bonobo_item_options_free (options);

    return BONOBO_OBJREF (selector);
}

static BonoboObject *
libgnomeui_components_factory (BonoboGenericFactory *this, 
			       const char           *object_id,
			       void                 *closure)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
	initialized = TRUE;             
    }

    if (!strcmp (object_id, "OAFIID:GNOME_UI_Component_IconSelector")) {
	GnomeSelectorFactory *factory;

	factory = gnome_selector_factory_new (gnome_icon_selector_component_get_type ());

	return BONOBO_OBJECT (factory);
    } else if (!strcmp (object_id, "OAFIID:GNOME_UI_Component_ImageEntry")) {
	GnomeSelectorFactory *factory;

	factory = gnome_selector_factory_new (gnome_image_entry_component_get_type ());

	return BONOBO_OBJECT (factory);
    } else if (!strcmp (object_id, "OAFIID:GNOME_UI_Component_ImageEntry_Item")) {
	BonoboItemHandler *item_handler;

	item_handler = bonobo_item_handler_new (NULL, image_entry_get_object_fn, NULL);

	return BONOBO_OBJECT (item_handler);
    } else if (!strcmp (object_id, "OAFIID:GNOME_UI_Component_Entry")) {
	GnomeSelectorFactory *factory;

	factory = gnome_selector_factory_new (gnome_entry_component_get_type ());

	return BONOBO_OBJECT (factory);
    } else
	g_warning ("Failing to manufacture a '%s'", object_id);
        
    return NULL;
}

BONOBO_OAF_SHLIB_FACTORY_MULTI ("OAFIID:GNOME_UI_Components_Factory",
				"GNOME UI Components",
				libgnomeui_components_factory,
				NULL);

