/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 4; tab-width: 8 -*- */
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

#include <config.h>
#include <libgnomeui/gnome-entry.h>
#include <bonobo/bonobo-exception.h>

struct _GnomeSelectorClientPrivate {
    GNOME_Selector selector;
};

static BonoboWidgetClass *gnome_selector_client_parent_class;

static void
gnome_selector_client_finalize (GObject *object)
{
    GnomeSelectorClient *client = GNOME_SELECTOR_CLIENT (object);

    g_free (client->_priv);
    client->_priv = NULL;

    G_OBJECT_CLASS (gnome_selector_client_parent_class)->finalize (object);
}

static void
gnome_selector_client_class_init (GnomeSelectorClientClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;

    gnome_selector_client_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize = gnome_selector_client_finalize;
}

static void
gnome_selector_client_init (GnomeSelectorClient *client)
{
    client->_priv = g_new0 (GnomeSelectorClientPrivate, 1);
}

GtkType
gnome_selector_client_get_type (void)
{
    static GtkType type = 0;

    if (! type) {
	static const GtkTypeInfo info = {
	    "GnomeSelectorClient",
	    sizeof (GnomeSelectorClient),
	    sizeof (GnomeSelectorClientClass),
	    (GtkClassInitFunc) gnome_selector_client_class_init,
	    (GtkObjectInitFunc) gnome_selector_client_init,
	    NULL, /* reserved_1 */
	    NULL, /* reserved_2 */
	    (GtkClassInitFunc) NULL
	};

	type = gtk_type_unique (bonobo_widget_get_type (), &info);
    }

    return type;
}

GnomeSelectorClient *
gnome_selector_client_construct (GnomeSelectorClient *client, GNOME_Selector corba_selector,
				 Bonobo_UIContainer uic)
{
    Bonobo_Control corba_control;
    CORBA_Environment ev;

    g_return_val_if_fail (client != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR_CLIENT (client), NULL);
    g_return_val_if_fail (corba_selector != CORBA_OBJECT_NIL, NULL);

    CORBA_exception_init (&ev);

    corba_control = GNOME_Selector_getControl (corba_selector, &ev);
    if (BONOBO_EX (&ev) || (corba_control == CORBA_OBJECT_NIL)) {
	g_object_unref (G_OBJECT (client));
	CORBA_exception_free (&ev);
	return NULL;
    }

    if (!bonobo_widget_construct_control_from_objref (BONOBO_WIDGET (client), corba_control, uic)) {
	g_object_unref (G_OBJECT (client));
	CORBA_exception_free (&ev);
	return NULL;
    }

    client->_priv->selector = bonobo_object_dup_ref (corba_selector, &ev);
    if (BONOBO_EX (&ev)) {
	g_object_unref (G_OBJECT (client));
	CORBA_exception_free (&ev);
	return NULL;
    }

    CORBA_exception_free (&ev);

    return client;
}

GnomeSelectorClient *
gnome_selector_client_new (GNOME_Selector corba_selector, Bonobo_UIContainer uic)
{
    GnomeSelectorClient *client;

    g_return_val_if_fail (corba_selector != CORBA_OBJECT_NIL, NULL);

    client = g_object_new (gnome_selector_client_get_type (), NULL);

    return gnome_selector_client_construct (client, corba_selector, uic);
}

gchar *
gnome_selector_client_get_entry_text (GnomeSelectorClient *client)
{
    gchar *retval = NULL;
    CORBA_Environment ev;

    g_return_val_if_fail (client != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR_CLIENT (client), NULL);

    g_assert (client->_priv->selector != CORBA_OBJECT_NIL);

    CORBA_exception_init (&ev);
    retval = GNOME_Selector_getEntryText (client->_priv->selector, &ev);
    CORBA_exception_free (&ev);

    return retval;
}

void
gnome_selector_client_set_entry_text (GnomeSelectorClient *client,
				      const gchar *text)
{
    CORBA_Environment ev;

    g_return_if_fail (client != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (client));

    g_assert (client->_priv->selector != CORBA_OBJECT_NIL);

    CORBA_exception_init (&ev);
    GNOME_Selector_setEntryText (client->_priv->selector, text, &ev);
    CORBA_exception_free (&ev);


}

void
gnome_selector_client_activate_entry (GnomeSelectorClient *client)
{
    CORBA_Environment ev;

    g_return_if_fail (client != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (client));

    g_assert (client->_priv->selector != CORBA_OBJECT_NIL);

    CORBA_exception_init (&ev);
    GNOME_Selector_activateEntry (client->_priv->selector, &ev);
    CORBA_exception_free (&ev);
}

