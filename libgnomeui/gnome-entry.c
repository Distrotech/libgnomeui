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
	BonoboControl *control;

	GtkWidget *combo;
	GtkWidget *entry;
};
	

static void   gnome_entry_class_init   (GnomeEntryClass *class);
static void   gnome_entry_init         (GnomeEntry      *gentry);
static void   gnome_entry_finalize     (GObject         *object);

static gchar *get_entry_text_handler   (GnomeSelector   *selector,
					GnomeEntry      *gentry);
static void   set_entry_text_handler   (GnomeSelector   *selector,
                                        const gchar     *text,
					GnomeEntry      *gentry);
static void   activate_entry_handler   (GnomeSelector   *selector,
					GnomeEntry      *gentry);
static void   history_changed_handler  (GnomeSelector   *selector,
					GnomeEntry      *gentry);
static void   entry_activated_cb       (GtkWidget       *widget,
					GnomeSelector   *selector);


static GnomeSelectorClientClass *parent_class;

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
gnome_entry_class_init (GnomeEntryClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gnome_selector_client_get_type ());

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

	return (GtkWidget *) gnome_selector_client_construct
		(GNOME_SELECTOR_CLIENT (gentry), corba_selector, uic);
}

GtkWidget *
gnome_entry_new_full (GnomeSelector      *selector,
		      Bonobo_UIContainer  uic)
{
	BonoboEventSource *event_source;
	GtkWidget *entry_widget;
	GnomeEntry *gentry;

	entry_widget = gtk_combo_new ();

	gentry = g_object_new (gnome_entry_get_type (), NULL);

	gentry->_priv->combo = entry_widget;
	gentry->_priv->entry = GTK_COMBO (entry_widget)->entry;

	gtk_combo_disable_activate (GTK_COMBO (entry_widget));
	gtk_combo_set_case_sensitive (GTK_COMBO (entry_widget), TRUE);

	g_signal_connect_data (gentry->_priv->entry, "activate",
			       G_CALLBACK (entry_activated_cb),
			       selector, NULL, FALSE, FALSE);

	gtk_widget_show_all (entry_widget);

	gentry->_priv->control = bonobo_control_new (entry_widget);

	event_source = bonobo_event_source_new ();

	gnome_selector_construct (selector, event_source);

	gnome_selector_bind_to_control (selector,
					BONOBO_OBJECT (gentry->_priv->control));

	g_signal_connect_data (selector, "get_entry_text",
			       G_CALLBACK (get_entry_text_handler),
			       gentry, NULL, FALSE, FALSE);
	g_signal_connect_data (selector, "set_entry_text",
			       G_CALLBACK (set_entry_text_handler),
			       gentry, NULL, FALSE, FALSE);
	g_signal_connect_data (selector, "activate_entry",
			       G_CALLBACK (activate_entry_handler),
			       gentry, NULL, FALSE, FALSE);
	g_signal_connect_data (selector, "history_changed",
			       G_CALLBACK (history_changed_handler),
			       gentry, NULL, FALSE, FALSE);

	return gnome_entry_construct (gentry, BONOBO_OBJREF (selector),
				      CORBA_OBJECT_NIL);
}

GtkWidget *
gnome_entry_new (void)
{
	GnomeSelector *selector;

	selector = g_object_new (gnome_selector_get_type (), NULL);

	return gnome_entry_new_full (selector, CORBA_OBJECT_NIL);
}

GtkWidget *
gnome_entry_new_from_selector (GNOME_Selector corba_selector,
			       Bonobo_UIContainer uic)
{
	GnomeEntry *gentry;

	g_return_val_if_fail (corba_selector != CORBA_OBJECT_NIL, NULL);

	gentry = g_object_new (gnome_entry_get_type (), NULL);
	
	return gnome_entry_construct (gentry, corba_selector, uic);
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

static gchar *
get_entry_text_handler (GnomeSelector *selector, GnomeEntry *gentry)
{
	const char *text;

	g_return_val_if_fail (selector != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);
	g_return_val_if_fail (gentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ENTRY (gentry), NULL);

	text = gtk_entry_get_text (GTK_ENTRY (gentry->_priv->entry));
	return g_strdup (text);
}

static void
set_entry_text_handler (GnomeSelector *selector, const gchar *text,
			GnomeEntry *gentry)
{
	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (selector));
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (gentry->_priv->entry)
		gtk_entry_set_text (GTK_ENTRY (gentry->_priv->entry), text);
}

static void
activate_entry_handler (GnomeSelector *selector, GnomeEntry *gentry)
{
	const char *text;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (selector));
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	text = gtk_entry_get_text (GTK_ENTRY (gentry->_priv->entry));
	gnome_selector_prepend_history (selector, TRUE, text);
}

static void
history_changed_handler (GnomeSelector *selector, GnomeEntry *gentry)
{
	GtkWidget *list_widget;
	GList *items = NULL;
	GSList *history_list, *c;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (selector));
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	list_widget = GTK_COMBO (gentry->_priv->combo)->list;

	gtk_list_clear_items (GTK_LIST (list_widget), 0, -1);

	history_list = gnome_selector_get_history (selector);

	for (c = history_list; c; c = c->next) {
		GtkWidget *item;

		item = gtk_list_item_new_with_label (c->data);
		items = g_list_prepend (items, item);
		gtk_widget_show_all (item);
	}

	items = g_list_reverse (items);

	gtk_list_prepend_items (GTK_LIST (list_widget), items);

	g_slist_foreach (history_list, (GFunc) g_free, NULL);
	g_slist_free (history_list);
}

static void
entry_activated_cb (GtkWidget *widget, GnomeSelector *selector)
{
	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (selector));

	g_signal_emit_by_name (selector, "activate_entry");
}

gchar *
gnome_entry_get_text (GnomeEntry *gentry)
{
	g_return_val_if_fail (gentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ENTRY (gentry), NULL);

	return gnome_selector_client_get_entry_text (GNOME_SELECTOR_CLIENT (gentry));
}

void
gnome_entry_set_text (GnomeEntry *gentry, const gchar *text)
{
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	gnome_selector_client_set_entry_text (GNOME_SELECTOR_CLIENT (gentry),
					      text);
}

