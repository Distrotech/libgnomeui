/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-authn-manager.c - machinary for handling authentication to URI's

   Copyright (C) 2001 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Michael Fleming <mikef@praxis.etla.net>
*/

/*
 * Questions:
 *  -- Can we center the authentication dialog over the window where the operation
 *     is occuring?  (which doesn't even make sense in a drag between two windows)
 *  -- Can we provide a CORBA interface for components to access this info?
 *
 * dispatch stuff needs to go in utils
 *
 */

#include <config.h>
#include "gnome-authentication-manager.h"

#include <gnome.h>
#include <libgnome/gnome-i18n.h>
#include "gnome-password-dialog.h"
#include <libgnomevfs/gnome-vfs-module-callback.h>
#include <libgnomevfs/gnome-vfs-standard-callbacks.h>
#include <libgnomevfs/gnome-vfs-utils.h>


#if 0
#define DEBUG_MSG(x) printf x
#else 
#define DEBUG_MSG(x)
#endif

static GnomePasswordDialog *
construct_password_dialog (gboolean is_proxy_authentication, const GnomeVFSModuleCallbackAuthenticationIn *in_args)
{
	char *message;
	GnomePasswordDialog *dialog;

	message = g_strdup_printf (
		is_proxy_authentication
			? _("Your HTTP Proxy requires you to log in.\n")
			: _("You must log in to access \"%s\".\n%s"), 
		in_args->uri, 
		in_args->auth_type == AuthTypeBasic
			? _("Your password will be transmitted unencrypted.") 
			: _("Your password will be transmitted encrypted."));

	dialog = GNOME_PASSWORD_DIALOG (gnome_password_dialog_new (
			_("Authentication Required"),
			message,
			"",
			"",
			FALSE));

	g_free (message);

	return dialog;
}

static void
present_authentication_dialog_blocking (gboolean is_proxy_authentication,
			    const GnomeVFSModuleCallbackAuthenticationIn * in_args,
			    GnomeVFSModuleCallbackAuthenticationOut *out_args)
{
	GnomePasswordDialog *dialog;
	gboolean dialog_result;

	dialog = construct_password_dialog (is_proxy_authentication, in_args);

	dialog_result = gnome_password_dialog_run_and_block (dialog);

	if (dialog_result) {
		out_args->username = gnome_password_dialog_get_username (dialog);
		out_args->password = gnome_password_dialog_get_password (dialog);
	} else {
		out_args->username = NULL;
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

typedef struct {
	const GnomeVFSModuleCallbackAuthenticationIn	*in_args;
	GnomeVFSModuleCallbackAuthenticationOut		*out_args;
	gboolean				 is_proxy_authentication;

	GnomeVFSModuleCallbackResponse response;
	gpointer response_data;
} CallbackInfo;

static void
mark_callback_completed (CallbackInfo *info)
{
	info->response (info->response_data);
	g_free (info);
}

static void
authentication_dialog_button_clicked (GtkDialog *dialog, 
				      gint button_number, 
				      CallbackInfo *info)
{
	DEBUG_MSG (("+%s button: %d\n", G_GNUC_FUNCTION, button_number));

	if (button_number == GTK_RESPONSE_OK) {
		info->out_args->username 
			= gnome_password_dialog_get_username (GNOME_PASSWORD_DIALOG (dialog));
		info->out_args->password
			= gnome_password_dialog_get_password (GNOME_PASSWORD_DIALOG (dialog));
	}

	/* a NULL in the username field indicates "no credentials" to the caller */

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
authentication_dialog_closed (GtkDialog *dialog, CallbackInfo *info)
{
	DEBUG_MSG (("+%s\n", G_GNUC_FUNCTION));

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
authentication_dialog_destroyed (GtkDialog *dialog, CallbackInfo *info)
{
	DEBUG_MSG (("+%s\n", G_GNUC_FUNCTION));

	mark_callback_completed (info);	
}

static gint /* GtkFunction */
present_authentication_dialog_nonblocking (CallbackInfo *info)
{
	GnomePasswordDialog *dialog;

	g_return_val_if_fail (info != NULL, 0);

	dialog = construct_password_dialog (info->is_proxy_authentication, info->in_args);

	gtk_window_set_modal (GTK_WINDOW (dialog), FALSE);

	g_signal_connect (dialog, "response", 
			  G_CALLBACK (authentication_dialog_button_clicked), info);

	g_signal_connect (dialog, "close", 
			  G_CALLBACK (authentication_dialog_closed), info);

	g_signal_connect (dialog, "destroy", 
			  G_CALLBACK (authentication_dialog_destroyed), info);

	gtk_widget_show_all (GTK_WIDGET (dialog));

	return 0;
}

static void /* GnomeVFSAsyncModuleCallback */
vfs_async_authentication_callback (gconstpointer in, size_t in_size, 
				   gpointer out, size_t out_size, 
				   gpointer user_data,
				   GnomeVFSModuleCallbackResponse response,
				   gpointer response_data)
{
	GnomeVFSModuleCallbackAuthenticationIn *in_real;
	GnomeVFSModuleCallbackAuthenticationOut *out_real;
	gboolean is_proxy_authentication;
	CallbackInfo *info;
	
	g_return_if_fail (sizeof (GnomeVFSModuleCallbackAuthenticationIn) == in_size
		&& sizeof (GnomeVFSModuleCallbackAuthenticationOut) == out_size);

	g_return_if_fail (in != NULL);
	g_return_if_fail (out != NULL);

	in_real = (GnomeVFSModuleCallbackAuthenticationIn *)in;
	out_real = (GnomeVFSModuleCallbackAuthenticationOut *)out;

	is_proxy_authentication = (user_data == GINT_TO_POINTER (1));

	DEBUG_MSG (("+%s uri:'%s' is_proxy_auth: %u\n", G_GNUC_FUNCTION, in_real->uri, (unsigned) is_proxy_authentication));

	info = g_new (CallbackInfo, 1);

	info->is_proxy_authentication = is_proxy_authentication;
	info->in_args = in_real;
	info->out_args = out_real;
	info->response = response;
	info->response_data = response_data;

	present_authentication_dialog_nonblocking (info);

	DEBUG_MSG (("-%s\n", G_GNUC_FUNCTION));
}

static void /* GnomeVFSModuleCallback */
vfs_authentication_callback (gconstpointer in, size_t in_size, 
			     gpointer out, size_t out_size, 
			     gpointer user_data)
{
	GnomeVFSModuleCallbackAuthenticationIn *in_real;
	GnomeVFSModuleCallbackAuthenticationOut *out_real;
	gboolean is_proxy_authentication;
	
	g_return_if_fail (sizeof (GnomeVFSModuleCallbackAuthenticationIn) == in_size
		&& sizeof (GnomeVFSModuleCallbackAuthenticationOut) == out_size);

	g_return_if_fail (in != NULL);
	g_return_if_fail (out != NULL);

	in_real = (GnomeVFSModuleCallbackAuthenticationIn *)in;
	out_real = (GnomeVFSModuleCallbackAuthenticationOut *)out;

	is_proxy_authentication = (user_data == GINT_TO_POINTER (1));

	DEBUG_MSG (("+%s uri:'%s' is_proxy_auth: %u\n", G_GNUC_FUNCTION, in_real->uri, (unsigned) is_proxy_authentication));

	present_authentication_dialog_blocking (is_proxy_authentication, in_real, out_real);

	DEBUG_MSG (("-%s\n", G_GNUC_FUNCTION));
}

void
gnome_authentication_manager_init (void)
{
	if (!g_thread_supported ()) {
		g_thread_init (NULL);
	}

	gnome_vfs_async_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
						     vfs_async_authentication_callback, 
						     GINT_TO_POINTER (0),
						     NULL);
	gnome_vfs_async_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION, 
						     vfs_async_authentication_callback, 
						     GINT_TO_POINTER (1),
						     NULL);

	/* These are in case someone makes a synchronous http call for
	 * some reason. 
	 */

	gnome_vfs_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
					       vfs_authentication_callback, 
					       GINT_TO_POINTER (0),
					       NULL);
	gnome_vfs_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION, 
					       vfs_authentication_callback, 
					       GINT_TO_POINTER (1),
					       NULL);
}
