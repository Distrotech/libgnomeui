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
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-selectorP.h"
#include "gnome-uidefs.h"


static void gnome_selector_class_init (GnomeSelectorClass *class);
static void gnome_selector_init       (GnomeSelector      *selector);
static void gnome_selector_destroy    (GtkObject          *object);
static void gnome_selector_finalize   (GObject            *object);

static void selector_set_arg          (GtkObject          *object,
				       GtkArg             *arg,
				       guint               arg_id);
static void selector_get_arg          (GtkObject          *object,
				       GtkArg             *arg,
				       guint               arg_id);

static void browse_clicked            (GnomeSelector   *selector);

static GtkVBoxClass *parent_class;

enum {
	ARG_0,
};

enum {
	BROWSE_SIGNAL,
	LAST_SIGNAL
};

static int gnome_selector_signals [LAST_SIGNAL] = {0};

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

	parent_class = gtk_type_class (gtk_vbox_get_type ());

	gnome_selector_signals [BROWSE_SIGNAL] =
		gtk_signal_new("browse",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE (object_class),
			       GTK_SIGNAL_OFFSET (GnomeSelectorClass,
						  browse),
			       gtk_signal_default_marshaller,
			       GTK_TYPE_NONE,
			       0);
	gtk_object_class_add_signals (object_class,
				      gnome_selector_signals,
				      LAST_SIGNAL);

	object_class->destroy = gnome_selector_destroy;
	gobject_class->finalize = gnome_selector_finalize;
	object_class->get_arg = selector_get_arg;
	object_class->set_arg = selector_set_arg;

	class->browse = browse_clicked;
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

	selector->_priv->changed = FALSE;

	selector->_priv->selector_widget = NULL;
	selector->_priv->is_popup = FALSE;
}

static void
browse_clicked (GnomeSelector *selector)
{
	GnomeSelectorPrivate *priv = selector->_priv;

	/*if it already exists make sure it's shown and raised*/
	if (priv->selector_widget) {
		gtk_widget_show (priv->selector_widget);
		if (priv->selector_widget->window)
			gdk_window_raise (priv->selector_widget->window);
	}
}

static void
browse_signal (GtkWidget *widget, gpointer data)
{
	gtk_signal_emit (GTK_OBJECT(data),
			 gnome_selector_signals [BROWSE_SIGNAL]);
}

/**
 * gnome_selector_construct:
 * @selector: Pointer to GnomeSelector object.
 * @history_id: If not %NULL, the text id under which history data is stored
 * @selector_widget: Widget which should be used as selector.
 * @is_popup: Flag indicating whether @selector_widget is a popop dialog
 * or not.
 *
 * Constructs a #GnomeSelector object, for language bindings or subclassing
 * use #gnome_selector_new from C
 *
 * Returns: 
 */
void
gnome_selector_construct (GnomeSelector *selector, 
			  const gchar *history_id,
			  const gchar *dialog_title,
			  GtkWidget *selector_widget,
			  gboolean is_popup)
{
	GnomeSelectorPrivate *priv;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (selector));

	priv = selector->_priv;

	priv->gentry = gnome_entry_new (history_id);

	priv->dialog_title = g_strdup (dialog_title);

	priv->selector_widget = selector_widget;
	gtk_widget_ref (priv->selector_widget);
	priv->is_popup = is_popup;

	if (priv->is_popup) {
		priv->entry_hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
		priv->browse_button = gtk_button_new_with_label
			(_("Browse..."));

		gtk_signal_connect (GTK_OBJECT (priv->browse_button),
				    "clicked", browse_signal, selector);

		gtk_box_pack_start (GTK_BOX (priv->entry_hbox),
				    priv->gentry, TRUE, TRUE, 0);
		gtk_box_pack_start (GTK_BOX (priv->entry_hbox),
				    priv->browse_button, FALSE, FALSE, 0);

		gtk_widget_show_all (priv->entry_hbox);

		gtk_box_pack_start (GTK_BOX (selector),
				    priv->entry_hbox,
				    FALSE, FALSE, 0);
	} else {
		gtk_widget_show (priv->gentry);

		gtk_box_pack_start (GTK_BOX (selector),
				    priv->gentry,
				    FALSE, FALSE, GNOME_PAD_SMALL);

		gtk_box_pack_start (GTK_BOX (selector),
				    priv->selector_widget,
				    TRUE, TRUE, GNOME_PAD_SMALL);

		gtk_widget_show (priv->selector_widget);
	}
}


static void
gnome_selector_destroy (GtkObject *object)
{
	GnomeSelector *selector;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (object));

	selector = GNOME_SELECTOR (object);

	if (selector->_priv->selector_widget) {
		gtk_widget_unref (selector->_priv->selector_widget);
		selector->_priv->selector_widget = NULL;
		selector->_priv->is_popup = FALSE;
	}

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


/**
 * gnome_selector_get_dialog_title
 * @selector: Pointer to GnomeSelector object.
 *
 * Description: Returns the titel of the popup dialog.
 *
 * Returns: Titel of the popup dialog.
 */
const gchar *
gnome_selector_get_dialog_title (GnomeSelector *selector)
{
	g_return_val_if_fail (selector != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

	return selector->_priv->dialog_title;
}


/**
 * gnome_selector_set_dialog_title
 * @selector: Pointer to GnomeSelector object.
 * @dialog_title: New title for the popup dialog.
 *
 * Description: Sets the titel of the popup dialog.
 *
 * This is only used when the widget uses a popup dialog for
 * the actual selector.
 *
 * Returns:
 */
void
gnome_selector_set_dialog_title (GnomeSelector *selector,
				 const gchar *dialog_title)
{
	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (selector));

	if (selector->_priv->dialog_title) {
		g_free (selector->_priv->dialog_title);
		selector->_priv->dialog_title = NULL;
	}

	/* this is NULL safe. */
	selector->_priv->dialog_title = g_strdup (dialog_title);
}

