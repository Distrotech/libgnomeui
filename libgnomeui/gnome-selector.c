/*
 * Copyright (C) 2000 SuSE GmbH
 * Author: Martin Baulig <baulig@suse.de>
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

/* GnomeSelector widget - pure virtual widget.
 *
 *   Use the Gnome{File,Icon,Pixmap}Selector subclasses.
 *
 * Author: Martin Baulig <baulig@suse.de>
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-selector.h"
#include "gnome-entry.h"

struct _GnomeSelectorPrivate {
	GtkWidget   *gentry;

	guint32      changed : 1;
};
	

static void gnome_selector_class_init (GnomeSelectorClass *class);
static void gnome_selector_init       (GnomeSelector      *selector);
static void gnome_selector_destroy    (GtkObject       *object);
static void gnome_selector_finalize   (GObject         *object);

static void selector_set_arg          (GtkObject       *object,
				       GtkArg          *arg,
				       guint           arg_id);
static void selector_get_arg          (GtkObject       *object,
				       GtkArg          *arg,
				       guint           arg_id);

static GtkVBoxClass *parent_class;

enum {
	ARG_0,
};

guint
gnome_selector_get_type (void)
{
	static guint selector_type = 0;

	if (!selector_type) {
		GtkTypeInfo selector_info = {
			"GnomeSelector",
			sizeof (GnomeSelector),
			sizeof (GnomeSelectorClass),
			(GtkClassInitFunc) gnome_selector_class_init,
			(GtkObjectInitFunc) gnome_selector_init,
			NULL,
			NULL,
			NULL
		};

		selector_type = gtk_type_unique (gtk_vbox_get_type (),
						 &selector_info);
	}

	return selector_type;
}

static void
gnome_selector_class_init (GnomeSelectorClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gtk_combo_get_type ());

	object_class->destroy = gnome_selector_destroy;
	gobject_class->finalize = gnome_selector_finalize;
	object_class->get_arg = selector_get_arg;
	object_class->set_arg = selector_set_arg;
}

static void
selector_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeSelector *self;

	self = GNOME_SELECTOR (object);

	switch (arg_id) {
	default:
		break;
	}
}

static void
selector_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeSelector *self;

	self = GNOME_SELECTOR (object);

	switch (arg_id) {
	default:
		break;
	}
}

static void
gnome_selector_init (GnomeSelector *selector)
{
	selector->_priv = g_new0 (GnomeSelectorPrivate, 1);

	selector->_priv->changed      = FALSE;
}

/**
 * gnome_selector_construct:
 * @selector: Pointer to GnomeSelector object.
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Constructs a #GnomeSelector object, for language bindings or subclassing
 * use #gnome_selector_new from C
 *
 * Returns: 
 */
void
gnome_selector_construct (GnomeSelector *selector, 
			  const gchar *history_id)
{
	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (selector));

	selector->_priv->gentry		= gnome_entry_new (history_id);
}


static void
gnome_selector_destroy (GtkObject *object)
{
	GnomeSelector *selector;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (object));

	selector = GNOME_SELECTOR (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_selector_finalize (GObject *object)
{
	GnomeSelector *selector;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (object));

	selector = GNOME_SELECTOR (object);

	g_free (selector->_priv);
	selector->_priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/**
 * gnome_selector_get_gnome_entry
 * @selector: Pointer to GnomeSelector object.
 *
 * Description: Obtain pointer to GnomeSelector's GnomeEntry widget
 *
 * Returns: Pointer to GnomeEntry widget.
 */
GtkWidget *
gnome_selector_get_gnome_entry (GnomeSelector *selector)
{
	g_return_val_if_fail (selector != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

	return selector->_priv->gentry;
}

