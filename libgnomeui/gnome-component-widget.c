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
#include <bonobo/bonobo-moniker-util.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-async.h>
#include <bonobo/bonobo-main.h>

struct _GnomeSelectorClientPrivate {
    GNOME_Selector selector;
    Bonobo_EventSource_ListenerId listener_id;
    GnomeAsyncContext *async_ctx;

    gchar *browse_dialog_moniker;

    GNOME_Tristate  want_entry_widget;
    GNOME_Tristate  want_preview_widget;
    GNOME_Tristate  want_selector_widget;
    GNOME_Tristate  want_browse_dialog;
    GNOME_Tristate  want_browse_button;
    GNOME_Tristate  want_clear_button;
    GNOME_Tristate  want_default_button;

    guint32 constructed : 1;

    BonoboPropertyBag *pbag;

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

static GNOME_AsyncID last_async_id G_GNUC_UNUSED = 0;

enum {
    PROP_0,

    /* Construction properties */
    PROP_WANT_ENTRY_WIDGET,
    PROP_WANT_PREVIEW_WIDGET,
    PROP_WANT_SELECTOR_WIDGET,
    PROP_WANT_BROWSE_DIALOG,
    PROP_WANT_BROWSE_BUTTON,
    PROP_WANT_CLEAR_BUTTON,
    PROP_WANT_DEFAULT_BUTTON,
    PROP_BROWSE_DIALOG_MONIKER
};

static void
gnome_selector_client_finalize (GObject *object)
{
    GnomeSelectorClient *client = GNOME_SELECTOR_CLIENT (object);

    g_free (client->_priv);
    client->_priv = NULL;

    G_OBJECT_CLASS (gnome_selector_client_parent_class)->finalize (object);
}

static void
set_arg_tristate (BonoboArg *arg, GNOME_Tristate value, CORBA_Environment *ev)
{
    switch (value) {
    case GNOME_TRISTATE_YES:
	BONOBO_ARG_SET_BOOLEAN (arg, TRUE);
	break;
    case GNOME_TRISTATE_NO:
	BONOBO_ARG_SET_BOOLEAN (arg, FALSE);
	break;
    default:
	bonobo_exception_set (ev, ex_Bonobo_PropertyBag_NotFound);
	break;
    }
}

static void
gnome_selector_client_pbag_get_property (BonoboPropertyBag *bag, BonoboArg *arg,
					 guint arg_id, CORBA_Environment *ev,
					 gpointer user_data)
{
    GnomeSelectorClient *client;

    g_return_if_fail (user_data != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (user_data));

    client = GNOME_SELECTOR_CLIENT (user_data);

    switch (arg_id) {
    case PROP_WANT_ENTRY_WIDGET:
	set_arg_tristate (arg, client->_priv->want_entry_widget, ev);
	break;
    case PROP_WANT_PREVIEW_WIDGET:
	set_arg_tristate (arg, client->_priv->want_preview_widget, ev);
	break;
    case PROP_WANT_SELECTOR_WIDGET:
	set_arg_tristate (arg, client->_priv->want_selector_widget, ev);
	break;
    case PROP_WANT_BROWSE_DIALOG:
	set_arg_tristate (arg, client->_priv->want_browse_dialog, ev);
	break;
    case PROP_WANT_BROWSE_BUTTON:
	set_arg_tristate (arg, client->_priv->want_browse_button, ev);
	break;
    case PROP_WANT_CLEAR_BUTTON:
	set_arg_tristate (arg, client->_priv->want_clear_button, ev);
	break;
    case PROP_WANT_DEFAULT_BUTTON:
	set_arg_tristate (arg, client->_priv->want_default_button, ev);
	break;
    case PROP_BROWSE_DIALOG_MONIKER:
	BONOBO_ARG_SET_STRING (arg, client->_priv->browse_dialog_moniker);
	break;
    default:
	g_warning (G_STRLOC ": unknown property id %d", arg_id);
	break;
    }
}

static void
gnome_selector_client_set_property (GObject *object, guint param_id,
				    const GValue *value, GParamSpec *pspec)
{
    GnomeSelectorClient *client;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (object));

    client = GNOME_SELECTOR_CLIENT (object);

    switch (param_id) {
    case PROP_WANT_ENTRY_WIDGET:
	g_assert (!client->_priv->constructed);
	client->_priv->want_entry_widget = g_value_get_enum (value);
	break;
    case PROP_WANT_PREVIEW_WIDGET:
	g_assert (!client->_priv->constructed);
	client->_priv->want_preview_widget = g_value_get_enum (value);
	break;
    case PROP_WANT_SELECTOR_WIDGET:
	g_assert (!client->_priv->constructed);
	client->_priv->want_selector_widget = g_value_get_enum (value);
	break;
    case PROP_WANT_BROWSE_DIALOG:
	g_assert (!client->_priv->constructed);
	client->_priv->want_browse_button = g_value_get_enum (value);
	break;
    case PROP_WANT_BROWSE_BUTTON:
	g_assert (!client->_priv->constructed);
	client->_priv->want_browse_button = g_value_get_enum (value);
	break;
    case PROP_WANT_CLEAR_BUTTON:
	g_assert (!client->_priv->constructed);
	client->_priv->want_clear_button = g_value_get_enum (value);
	break;
    case PROP_WANT_DEFAULT_BUTTON:
	g_assert (!client->_priv->constructed);
	client->_priv->want_default_button = g_value_get_enum (value);
	break;
    case PROP_BROWSE_DIALOG_MONIKER:
	g_assert (!client->_priv->constructed);
	client->_priv->browse_dialog_moniker = g_value_dup_string (value);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
gnome_selector_client_get_property (GObject *object, guint param_id, GValue *value,
				    GParamSpec *pspec)
{
    GnomeSelectorClient *client;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (object));

    client = GNOME_SELECTOR_CLIENT (object);

    switch (param_id) {
    case PROP_WANT_ENTRY_WIDGET:
	g_value_set_enum (value, client->_priv->want_entry_widget);
	break;
    case PROP_WANT_PREVIEW_WIDGET:
	g_value_set_enum (value, client->_priv->want_preview_widget);
	break;
    case PROP_WANT_SELECTOR_WIDGET:
	g_value_set_enum (value, client->_priv->want_selector_widget);
	break;
    case PROP_WANT_BROWSE_DIALOG:
	g_value_set_enum (value, client->_priv->want_browse_button);
	break;
    case PROP_WANT_BROWSE_BUTTON:
	g_value_set_enum (value, client->_priv->want_browse_button);
	break;
    case PROP_WANT_CLEAR_BUTTON:
	g_value_set_enum (value, client->_priv->want_clear_button);
	break;
    case PROP_WANT_DEFAULT_BUTTON:
	g_value_set_enum (value, client->_priv->want_default_button);
	break;
    case PROP_BROWSE_DIALOG_MONIKER:
	g_value_set_string (value, client->_priv->browse_dialog_moniker);
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
gnome_selector_client_class_init (GnomeSelectorClientClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;

    gnome_selector_client_parent_class = g_type_class_peek_parent (klass);

    object_class->finalize = gnome_selector_client_finalize;
    object_class->set_property = gnome_selector_client_set_property;
    object_class->get_property = gnome_selector_client_get_property;

    /* Construction properties */
    g_object_class_install_property
	(object_class,
	 PROP_WANT_ENTRY_WIDGET,
	 g_param_spec_enum ("want-entry-widget", NULL, NULL,
			    GNOME_TYPE_TRISTATE, GNOME_TRISTATE_DEFAULT,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE |
			     G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(object_class,
	 PROP_WANT_PREVIEW_WIDGET,
	 g_param_spec_enum ("want-preview-widget", NULL, NULL,
			    GNOME_TYPE_TRISTATE, GNOME_TRISTATE_DEFAULT,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE |
			     G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(object_class,
	 PROP_WANT_SELECTOR_WIDGET,
	 g_param_spec_enum ("want-selector-widget", NULL, NULL,	
			    GNOME_TYPE_TRISTATE, GNOME_TRISTATE_DEFAULT,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE |
			     G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(object_class,
	 PROP_WANT_BROWSE_DIALOG,
	 g_param_spec_enum ("want-browse-dialog", NULL, NULL,
			    GNOME_TYPE_TRISTATE, GNOME_TRISTATE_DEFAULT,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE |
			     G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(object_class,
	 PROP_WANT_BROWSE_BUTTON,
	 g_param_spec_enum ("want-browse-button", NULL, NULL,
			    GNOME_TYPE_TRISTATE, GNOME_TRISTATE_DEFAULT,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE |
			     G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(object_class,
	 PROP_WANT_CLEAR_BUTTON,
	 g_param_spec_enum ("want-clear-button", NULL, NULL,
			    GNOME_TYPE_TRISTATE, GNOME_TRISTATE_DEFAULT,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE |
			     G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(object_class,
	 PROP_WANT_DEFAULT_BUTTON,
	 g_param_spec_enum ("want-default-button", NULL, NULL,
			    GNOME_TYPE_TRISTATE, GNOME_TRISTATE_DEFAULT,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE |
			     G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(object_class,
	 PROP_BROWSE_DIALOG_MONIKER,
	 g_param_spec_string ("browse-dialog-moniker", NULL, NULL,
			      NULL,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));
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
gnome_selector_client_construct (GnomeSelectorClient *client, const gchar *moniker,
				 Bonobo_UIContainer uic)
{
    GNOME_SelectorFactory factory;
    GNOME_Selector selector;
    CORBA_Environment ev;

    g_return_val_if_fail (client != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR_CLIENT (client), NULL);
    g_return_val_if_fail (moniker != NULL, NULL);
    g_return_val_if_fail (!client->_priv->constructed, NULL);

    CORBA_exception_init (&ev);

    g_message (G_STRLOC);

    client->_priv->pbag = bonobo_property_bag_new (gnome_selector_client_pbag_get_property,
						   NULL, client);

    bonobo_property_bag_add (client->_priv->pbag,
			     "browse-dialog-moniker", PROP_BROWSE_DIALOG_MONIKER,
			     BONOBO_ARG_STRING, NULL, NULL, BONOBO_PROPERTY_READABLE);
    bonobo_property_bag_add (client->_priv->pbag,
			     "want-entry-widget", PROP_WANT_ENTRY_WIDGET,
			     BONOBO_ARG_BOOLEAN, NULL, NULL, BONOBO_PROPERTY_READABLE);
    bonobo_property_bag_add (client->_priv->pbag,
			     "want-preview-widget", PROP_WANT_PREVIEW_WIDGET,
			     BONOBO_ARG_BOOLEAN, NULL, NULL, BONOBO_PROPERTY_READABLE);
    bonobo_property_bag_add (client->_priv->pbag,
			     "want-browse-dialog", PROP_WANT_BROWSE_DIALOG,
			     BONOBO_ARG_BOOLEAN, NULL, NULL, BONOBO_PROPERTY_READABLE);
    bonobo_property_bag_add (client->_priv->pbag,
			     "want-selector-widget", PROP_WANT_SELECTOR_WIDGET,
			     BONOBO_ARG_BOOLEAN, NULL, NULL, BONOBO_PROPERTY_READABLE);
    bonobo_property_bag_add (client->_priv->pbag,
			     "want-browse-button", PROP_WANT_BROWSE_BUTTON,
			     BONOBO_ARG_BOOLEAN, NULL, NULL, BONOBO_PROPERTY_READABLE);
    bonobo_property_bag_add (client->_priv->pbag,
			     "want-default-button", PROP_WANT_DEFAULT_BUTTON,
			     BONOBO_ARG_BOOLEAN, NULL, NULL, BONOBO_PROPERTY_READABLE);
    bonobo_property_bag_add (client->_priv->pbag,
			     "want-clear-button", PROP_WANT_CLEAR_BUTTON,
			     BONOBO_ARG_BOOLEAN, NULL, NULL, BONOBO_PROPERTY_READABLE);

    bonobo_property_bag_add_gtk_args (client->_priv->pbag, G_OBJECT (client));

    factory = bonobo_get_object (moniker, "GNOME/SelectorFactory", &ev);
    if (BONOBO_EX (&ev) || (factory == CORBA_OBJECT_NIL)) {
	g_object_unref (G_OBJECT (client));
	CORBA_exception_free (&ev);
	return NULL;
    }

    selector = GNOME_SelectorFactory_createSelector (factory, BONOBO_OBJREF (client->_priv->pbag), &ev);

    if (BONOBO_EX (&ev) || (selector == NULL)) {
	g_object_unref (G_OBJECT (client));
	CORBA_exception_free (&ev);
	return NULL;
    }

    CORBA_exception_free (&ev);

    return gnome_selector_client_construct_from_objref (client, selector, uic);
}

GnomeSelectorClient *
gnome_selector_client_construct_from_objref (GnomeSelectorClient *client,
					     GNOME_Selector corba_selector,
					     Bonobo_UIContainer uic)
{
    Bonobo_Control corba_control;
    Bonobo_EventSource event_source;
    CORBA_Environment ev;

    g_return_val_if_fail (client != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR_CLIENT (client), NULL);
    g_return_val_if_fail (corba_selector != NULL, NULL);
    g_return_val_if_fail (!client->_priv->constructed, NULL);

    client->_priv->constructed = TRUE;

    CORBA_exception_init (&ev);

    client->_priv->async_ctx = gnome_async_context_new ();

    client->_priv->selector = bonobo_object_dup_ref (corba_selector, &ev);
    if (BONOBO_EX (&ev)) {
	g_object_unref (G_OBJECT (client));
	CORBA_exception_free (&ev);
	return NULL;
    }

    corba_control = GNOME_Selector_getControl (client->_priv->selector, &ev);
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
gnome_selector_client_new (const gchar *moniker, Bonobo_UIContainer uic)
{
    GnomeSelectorClient *client;

    g_return_val_if_fail (moniker != NULL, NULL);

    client = g_object_new (gnome_selector_client_get_type (), NULL);

    return gnome_selector_client_construct (client, moniker, uic);
}

GnomeSelectorClient *
gnome_selector_client_new_from_objref (GNOME_Selector corba_selector, Bonobo_UIContainer uic)
{
    GnomeSelectorClient *client;

    g_return_val_if_fail (corba_selector != CORBA_OBJECT_NIL, NULL);

    client = g_object_new (gnome_selector_client_get_type (), NULL);

    return gnome_selector_client_construct_from_objref (client, corba_selector, uic);
}

static GNOME_Selector_AsyncData *
gnome_selector_client_create_async_data (GnomeSelectorClient *client, const gchar *uri,
					 GnomeAsyncHandle *async_handle)
{
    GNOME_Selector_AsyncData *async_data;

    g_return_val_if_fail (client != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR_CLIENT (client), NULL);

    async_data = GNOME_Selector_AsyncData__alloc ();
    async_data->client_id = client->_priv->client_id;
    async_data->async_id = ++last_async_id;
    async_data->user_data._type = TC_null;

    g_hash_table_insert (client->_priv->async_ops, GINT_TO_POINTER (async_data->async_id), async_handle);

    return async_data;
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

static void
gnome_selector_client_event_cb (BonoboListener *listener, char *event_name, 
				CORBA_any *any, CORBA_Environment *ev, gpointer user_data)
{
    GnomeSelectorClient *client;
    GNOME_Selector_AsyncReply *async_reply;
    GnomeAsyncHandle *async_handle;

    g_return_if_fail (user_data != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (user_data));
    g_return_if_fail (any != NULL);
    g_return_if_fail (CORBA_TypeCode_equal (any->_type, TC_GNOME_Selector_AsyncReply, ev));
    g_return_if_fail (!BONOBO_EX (ev));

    client = GNOME_SELECTOR_CLIENT (user_data);
    async_reply = any->_value;

    /* Is the event for us ? */
    if (async_reply->async_data.client_id != client->_priv->client_id)
	return;

    async_handle = g_hash_table_lookup (client->_priv->async_ops,
					GINT_TO_POINTER (async_reply->async_data.async_id));

    g_message (G_STRLOC ": %p - `%s' - (%d,%d) - `%s' - %d - %p", client, event_name,
	       async_reply->async_data.client_id, async_reply->async_data.async_id,
	       async_reply->uri, async_reply->success, async_handle);

    if (!async_handle)
	return;

    gnome_async_handle_completed (async_handle, async_reply->success);
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
gnome_selector_client_check_uri (GnomeSelectorClient  *client,
				 GnomeAsyncHandle    **async_handle_return,
				 const gchar          *uri,
				 gboolean              directory_ok,
				 guint                 timeout_msec,
				 GnomeAsyncFunc        async_func,
				 gpointer              user_data)
{
    CORBA_Environment ev;
    GnomeAsyncHandle *async_handle;
    GNOME_Selector_AsyncData *async_data;

    g_return_if_fail (client != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (client));

    async_handle = gnome_async_context_get (client->_priv->async_ctx, GNOME_ASYNC_TYPE_CHECK_URI, 
					    async_func, G_OBJECT (client), uri, user_data,
					    NULL);
    if (async_handle_return) {
	*async_handle_return = async_handle;
	gnome_async_handle_ref (async_handle);
    }

    async_data = gnome_selector_client_create_async_data (client, uri, async_handle);

    CORBA_exception_init (&ev);
    GNOME_Selector_checkURI (client->_priv->selector, uri, directory_ok, async_data, &ev);
    CORBA_exception_free (&ev);
}

void
gnome_selector_client_scan_directory (GnomeSelectorClient  *client,
				      GnomeAsyncHandle    **async_handle_return,
				      const gchar          *uri,
				      guint                 timeout_msec,
				      GnomeAsyncFunc        async_func,
				      gpointer              user_data)
{
    CORBA_Environment ev;
    GnomeAsyncHandle *async_handle;
    GNOME_Selector_AsyncData *async_data;

    g_return_if_fail (client != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (client));

    async_handle = gnome_async_context_get (client->_priv->async_ctx, GNOME_ASYNC_TYPE_SCAN_DIRECTORY, 
					    async_func, G_OBJECT (client), uri, user_data,
					    NULL);
    if (async_handle_return) {
	*async_handle_return = async_handle;
	gnome_async_handle_ref (async_handle);
    }

    async_data = gnome_selector_client_create_async_data (client, uri, async_handle);

    CORBA_exception_init (&ev);
    GNOME_Selector_scanDirectory (client->_priv->selector, uri, async_data, &ev);
    CORBA_exception_free (&ev);
}

void
gnome_selector_client_set_uri (GnomeSelectorClient  *client,
			       GnomeAsyncHandle    **async_handle_return,
                               const gchar          *uri,
                               guint                 timeout_msec,
                               GnomeAsyncFunc        async_func,
			       gpointer              user_data)
{
    CORBA_Environment ev;
    GnomeAsyncHandle *async_handle;
    GNOME_Selector_AsyncData *async_data;

    g_return_if_fail (client != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (client));

    async_handle = gnome_async_context_get (client->_priv->async_ctx, GNOME_ASYNC_TYPE_SET_URI, 
					    async_func, G_OBJECT (client), uri, user_data,
					    NULL);
    if (async_handle_return) {
	*async_handle_return = async_handle;
	gnome_async_handle_ref (async_handle);
    }

    async_data = gnome_selector_client_create_async_data (client, uri, async_handle);

    CORBA_exception_init (&ev);
    GNOME_Selector_setURI (client->_priv->selector, uri, async_data, &ev);
    CORBA_exception_free (&ev);
}

void
gnome_selector_client_add_uri (GnomeSelectorClient  *client,
			       GnomeAsyncHandle    **async_handle_return,
                               const gchar          *uri,
			       glong                 position,
			       guint                 list_id,
                               guint                 timeout_msec,
                               GnomeAsyncFunc        async_func,
			       gpointer              user_data)
{
    CORBA_Environment ev;
    GnomeAsyncHandle *async_handle;
    GNOME_Selector_AsyncData *async_data;

    g_return_if_fail (client != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR_CLIENT (client));

    async_handle = gnome_async_context_get (client->_priv->async_ctx, GNOME_ASYNC_TYPE_ADD_URI, 
					    async_func, G_OBJECT (client), uri, user_data,
					    NULL);
    if (async_handle_return) {
	*async_handle_return = async_handle;
	gnome_async_handle_ref (async_handle);
    }

    async_data = gnome_selector_client_create_async_data (client, uri, async_handle);

    CORBA_exception_init (&ev);
    GNOME_Selector_addURI (client->_priv->selector, uri, position, list_id, async_data, &ev);
    CORBA_exception_free (&ev);
}

GNOME_Selector
gnome_selector_client_get_selector (GnomeSelectorClient *client)
{
    g_return_val_if_fail (client != NULL, CORBA_OBJECT_NIL);
    g_return_val_if_fail (GNOME_IS_SELECTOR_CLIENT (client), CORBA_OBJECT_NIL);

    return client->_priv->selector;
}
