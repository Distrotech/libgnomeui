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
#include <libbonobo.h>

static BonoboObject *
libgnomeui_components_factory (BonoboGenericFactory *this, 
			       const char           *object_id,
			       void                 *closure)
{
    static gboolean initialized = FALSE;

    if (!initialized) {
	initialized = TRUE;             
    }

    g_warning ("Failing to manufacture a '%s'", object_id);
        
    return NULL;
}

BONOBO_OAF_SHLIB_FACTORY_MULTI ("OAFIID:GNOME_UI_Components_Factory",
				"GNOME UI Components",
				libgnomeui_components_factory,
				NULL);

