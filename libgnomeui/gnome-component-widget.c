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
#include <bonobo/bonobo-async.h>

struct _GnomeSelectorClientPrivate {
    GNOME_Selector selector;
    Bonobo_EventSource_ListenerId listener_id;

    GNOME_Selector_ClientID client_id;

    GHashTable *async_ops;
};

static void
gnome_selector_client_event_cb (BonoboListener    *listener,
				char              *event_name, 
				CORBA_any         *any,
				CORBA_Environment *ev,
				gpointer           user_data);

static BonoboWidgetClass *gnome_selector_client_parent_class;

static GNOME_Selector_AsyncID last_async_id = 0;

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
    Bonobo_EventSource event_source;
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

    client->_priv->client_id = GNOME_Selector_getClientID (client->_priv->selector, &ev);
    if (BONOBO_EX (&ev)) {
	g_object_unref (G_OBJECT (client));
	CORBA_exception_free (&ev);
	return NULL;
    }

    event_source = GNOME_Selector_getEventSource (client->_priv->selector, &ev);
    if (BONOBO_EX (&ev)) {
	g_object_unref (G_OBJECT (client));
	CORBA_exception_free (&ev);
	return NULL;
    }

    client->_priv->listener_id = bonobo_event_source_client_add_listener
	(event_source, gnome_selector_client_event_cb, "GNOME/Selector:async", &ev, client);
    if (BONOBO_EX (&ev)) {
	g_object_unref (G_OBJECT (client));
	CORBA_exception_free (&ev);
	return NULL;
    }

    client->_priv->async_ops = g_hash_table_new (NULL, NULL);

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

struct _GnomeSelectorClientAsyncHandle {
    GNOME_Selector_AsyncContext  *async_ctx;
    GnomeSelectorClient          *client;
    guint                         timeout_msec;
    GnomeSelectorClientAsyncFunc  async_func;
    gpointer                      user_data;
    GDestroyNotify                destroy_fn;
};

static GnomeSelectorClientAsyncHandle *
_gnome_selector_client_async_handle_get (GnomeSelectorClient *client,
					 GnomeSelectorClientAsyncFunc async_func,
					 gpointer user_data, GDestroyNotify destroy_fn)
{
    GnomeSelectorClientAsyncHandle *async_handle;

    g_return_val_if_fail (client != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR_CLIENT (client), NULL);

    async_handle = g_new0 (GnomeSelectorClientAsyncHandle, 1);
    async_handle->client = client;
    async_handle->async_func = async_func;
    async_handle->user_data = user_data;
    async_handle->destroy_fn = destroy_fn;

    async_handle->async_ctx = GNOME_Selector_AsyncContext__alloc ();
    async_handle->async_ctx->client_id = client->_priv->client_id;
    async_handle->async_ctx->async_id = ++last_async_id;
    async_handle->async_ctx->user_data._type = TC_null;

    g_object_ref (G_OBJECT (async_handle->client));

    g_hash_table_insert (client->_priv->async_ops, GINT_TO_POINTER (last_async_id), async_handle);

    return async_handle;
}

static void
_gnome_selector_client_async_handle_free (GnomeSelectorClientAsyncHandle *async_handle)
{
    g_return_if_fail (async_handle != NULL);
    g_return_if_fail (async_handle->client != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (async_handle->client));

    g_hash_table_remove (async_handle->client->_priv->async_ops,
			 GINT_TO_POINTER (async_handle->async_ctx->async_id));

    if (async_handle->destroy_fn)
	async_handle->destroy_fn (async_handle->user_data);

    g_object_unref (G_OBJECT (async_handle->client));
    CORBA_free (async_handle->async_ctx);
    g_free (async_handle);
}

static void
gnome_selector_client_event_cb (BonoboListener *listener, char *event_name, 
				CORBA_any *any, CORBA_Environment *ev, gpointer user_data)
{
    GnomeSelectorClient *client;
    GNOME_Selector_AsyncReply *async_reply;
    GnomeSelectorClientAsyncHandle *async_handle;

    g_return_if_fail (user_data != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (user_data));
    g_return_if_fail (any != NULL);
    g_return_if_fail (CORBA_TypeCode_equal (any->_type, TC_GNOME_Selector_AsyncReply, ev));
    g_return_if_fail (!BONOBO_EX (ev));

    client = GNOME_SELECTOR_CLIENT (user_data);
    async_reply = any->_value;

    /* Is the event for us ? */
    if (async_reply->ctx.client_id != client->_priv->client_id)
	return;

    async_handle = g_hash_table_lookup (client->_priv->async_ops,
					GINT_TO_POINTER (async_reply->ctx.async_id));

    g_message (G_STRLOC ": %p - `%s' - (%d,%d) - `%s' - %d - %p", client, event_name,
	       async_reply->ctx.client_id, async_reply->ctx.async_id, async_reply->uri,
	       async_reply->success, async_handle);

    if (!async_handle)
	return;

    if (async_handle->async_func)
	async_handle->async_func (client, async_handle, async_reply->type,
				  async_reply->uri, async_reply->error, async_reply->success,
				  async_handle->user_data);

    _gnome_selector_client_async_handle_free (async_handle);
}

gchar *
gnome_selector_client_get_uri (GnomeSelectorClient *client)
{
    gchar *retval = NULL;
    CORBA_Environment ev;

    g_return_val_if_fail (client != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR_CLIENT (client), NULL);

    g_assert (client->_priv->selector != CORBA_OBJECT_NIL);

    CORBA_exception_init (&ev);
    retval = GNOME_Selector_getURI (client->_priv->selector, &ev);
    CORBA_exception_free (&ev);

    return retval;
}

void
gnome_selector_client_set_uri (GnomeSelectorClient             *client,
			       GnomeSelectorClientAsyncHandle **async_handle_return,
                               const gchar                     *uri,
                               guint                            timeout_msec,
                               GnomeSelectorClientAsyncFunc     async_func,
			       gpointer                         user_data,
			       GDestroyNotify                   destroy_fn)
{
    CORBA_Environment ev;
    GnomeSelectorClientAsyncHandle *async_handle;

    g_return_if_fail (client != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (client));

    async_handle = _gnome_selector_client_async_handle_get (client, async_func, user_data, destroy_fn);
    if (async_handle_return)
	*async_handle_return = async_handle;

    CORBA_exception_init (&ev);
    GNOME_Selector_setURI (client->_priv->selector, uri, async_handle->async_ctx, &ev);
    CORBA_exception_free (&ev);
}
