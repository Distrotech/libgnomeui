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
#include <libgnomeui/gnome-image-entry-component.h>
#include <bonobo/bonobo-moniker-util.h>
#include "gnome-image-entry.h"

struct _GnomeImageEntryPrivate {
};
	

static void   gnome_image_entry_class_init   (GnomeImageEntryClass *class);
static void   gnome_image_entry_init         (GnomeImageEntry      *gentry);
static void   gnome_image_entry_finalize     (GObject         *object);

static GnomeSelectorClientClass *parent_class;

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

		entry_type = gtk_type_unique (gnome_selector_client_get_type (), &entry_info);
	}

	return entry_type;
}

static void
gnome_image_entry_class_init (GnomeImageEntryClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gnome_selector_client_get_type ());

	gobject_class->finalize = gnome_image_entry_finalize;
}

static void
gnome_image_entry_init (GnomeImageEntry *gentry)
{
	gentry->_priv = g_new0 (GnomeImageEntryPrivate, 1);
}

GtkWidget *
gnome_image_entry_construct (GnomeImageEntry     *ientry,
			     GNOME_Selector       corba_selector,
			     Bonobo_UIContainer   uic)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_IMAGE_ENTRY (ientry), NULL);
	g_return_val_if_fail (corba_selector != CORBA_OBJECT_NIL, NULL);

	return (GtkWidget *) gnome_selector_client_construct
		(GNOME_SELECTOR_CLIENT (ientry), corba_selector, uic);
}

GtkWidget *
gnome_image_entry_new_icon_entry (void)
{
	GNOME_Selector selector;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	selector = bonobo_get_object ("OAFIID:GNOME_UI_Component_ImageEntry!type=icon",
				      "GNOME/Selector", &ev);
	CORBA_exception_free (&ev);

	return gnome_image_entry_new_from_selector (selector, CORBA_OBJECT_NIL);
}

GtkWidget *
gnome_image_entry_new_pixmap_entry (guint preview_x, guint preview_y)
{
	GNOME_Selector selector;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);
	selector = bonobo_get_object ("OAFIID:GNOME_UI_Component_ImageEntry!type=pixmap",
				      "GNOME/Selector", &ev);
	CORBA_exception_free (&ev);

	return gnome_image_entry_new_from_selector (selector, CORBA_OBJECT_NIL);
}

GtkWidget *
gnome_image_entry_new_from_selector (GNOME_Selector     corba_selector,
				     Bonobo_UIContainer uic)
{
	GnomeImageEntry *ientry;

	g_return_val_if_fail (corba_selector != CORBA_OBJECT_NIL, NULL);

	ientry = g_object_new (gnome_image_entry_get_type (), NULL);

	return gnome_image_entry_construct (ientry, corba_selector, uic);
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
