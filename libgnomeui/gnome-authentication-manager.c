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

#undef GTK_DISABLE_DEPRECATED

#include <config.h>

#include <glib/gi18n-lib.h>

#include "gnome-authentication-manager.h"
#include "gnome-authentication-manager-private.h"

#include <gnome.h>
#include "gnome-password-dialog.h"
#include <libgnomevfs/gnome-vfs-module-callback.h>
#include <libgnomevfs/gnome-vfs-standard-callbacks.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <gnome-keyring.h>


#if 0
#define DEBUG_MSG(x) printf x
#else 
#define DEBUG_MSG(x)
#endif

static int gnome_authentication_manager_dialog_visible = 0;

/**
 * gnome_authentication_manager_dialog_is_visible:
 *
 * This function checks whether there are any references on the authentication dialog and returns a 
 * a gboolean value.
 *
 * Returns: TRUE if there are any references on the authentication dialog, FALSE otherwise.
 *
 * Since: 2.8
 */
gboolean
gnome_authentication_manager_dialog_is_visible (void)
{
	return gnome_authentication_manager_dialog_visible > 0;
}

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
	DEBUG_MSG (("+%s button: %d\n", G_STRFUNC, button_number));

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
authentication_dialog_destroyed (GtkDialog *dialog, CallbackInfo *info)
{
	DEBUG_MSG (("+%s\n", G_STRFUNC));

	gnome_authentication_manager_dialog_visible--;
	mark_callback_completed (info);	
}

static gint /* GtkFunction */
present_authentication_dialog_nonblocking (CallbackInfo *info)
{
	GnomePasswordDialog *dialog;
	GtkWidget *toplevel, *current_grab;

	g_return_val_if_fail (info != NULL, 0);

	dialog = construct_password_dialog (info->is_proxy_authentication, info->in_args);

	toplevel = NULL;
	current_grab = gtk_grab_get_current ();
	if (current_grab) {
		toplevel = gtk_widget_get_toplevel (current_grab);
	}
	if (toplevel && GTK_WIDGET_TOPLEVEL (toplevel)) {
		/* There is a modal window, so we need to be modal too in order to
		 * get input. We set the other modal dialog as parent, which
		 * hopefully gives us better window management.
		 */
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (toplevel));
	} else {
		gtk_window_set_modal (GTK_WINDOW (dialog), FALSE);
	}

	g_signal_connect (dialog, "response", 
			  G_CALLBACK (authentication_dialog_button_clicked), info);

	g_signal_connect (dialog, "destroy", 
			  G_CALLBACK (authentication_dialog_destroyed), info);

	gtk_widget_show (GTK_WIDGET (dialog));

	gnome_authentication_manager_dialog_visible++;
	
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

	GDK_THREADS_ENTER ();
	
	in_real = (GnomeVFSModuleCallbackAuthenticationIn *)in;
	out_real = (GnomeVFSModuleCallbackAuthenticationOut *)out;

	is_proxy_authentication = (user_data == GINT_TO_POINTER (1));

	DEBUG_MSG (("+%s uri:'%s' is_proxy_auth: %u\n", G_STRFUNC, in_real->uri, (unsigned) is_proxy_authentication));

	info = g_new (CallbackInfo, 1);

	info->is_proxy_authentication = is_proxy_authentication;
	info->in_args = in_real;
	info->out_args = out_real;
	info->response = response;
	info->response_data = response_data;

	present_authentication_dialog_nonblocking (info);

	DEBUG_MSG (("-%s\n", G_STRFUNC));

	GDK_THREADS_LEAVE ();
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

	GDK_THREADS_ENTER ();
	
	in_real = (GnomeVFSModuleCallbackAuthenticationIn *)in;
	out_real = (GnomeVFSModuleCallbackAuthenticationOut *)out;

	is_proxy_authentication = (user_data == GINT_TO_POINTER (1));

	DEBUG_MSG (("+%s uri:'%s' is_proxy_auth: %u\n", G_STRFUNC, in_real->uri, (unsigned) is_proxy_authentication));

	present_authentication_dialog_blocking (is_proxy_authentication, in_real, out_real);

	DEBUG_MSG (("-%s\n", G_STRFUNC));

	GDK_THREADS_LEAVE ();
}

static GnomePasswordDialog *
construct_full_password_dialog (const GnomeVFSModuleCallbackFullAuthenticationIn *in_args)
{
	char *message;
	GnomePasswordDialog *dialog;
	GString *name;
	
	/* Generic case: */
	name = g_string_new (NULL);
	if (in_args->username != NULL) {
		g_string_append_printf (name, "%s@", in_args->username);
	}
	if (in_args->server != NULL) {
		g_string_append (name, in_args->server);
	}
	if (in_args->port != 0) {
		g_string_append_printf (name, ":%d", in_args->port);
	}
	if (in_args->object != NULL) {
		g_string_append_printf (name, "/%s", in_args->object);
	}
	if (in_args->domain != NULL) {
		/* Translators: "You must log in to acces user@server.com/share domain MYWINDOWSDOMAIN." */
		message = g_strdup_printf (_("You must log in to access %s domain %s\n"), name->str, in_args->domain);
	} else {
		message = g_strdup_printf (_("You must log in to access %s\n"), name->str);
	}

	g_string_free (name, TRUE);
		
	dialog = GNOME_PASSWORD_DIALOG (gnome_password_dialog_new (
								   _("Authentication Required"),
								   message,
								   in_args->default_user,
								   "",
								   FALSE));
	g_free (message);

	gnome_password_dialog_set_domain (dialog, in_args->default_domain);
	gnome_password_dialog_set_show_userpass_buttons (dialog,
						in_args->flags & GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_ANON_SUPPORTED);
	gnome_password_dialog_set_show_username (dialog, 
						 in_args->flags & GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME);
	gnome_password_dialog_set_show_domain (dialog, 
					       in_args->flags & GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_DOMAIN);
	gnome_password_dialog_set_show_password (dialog, 
						 in_args->flags & GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_PASSWORD);
	gnome_password_dialog_set_show_remember (dialog, gnome_keyring_is_available () && 
							 in_args->flags & GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_SAVING_SUPPORTED);

	return dialog;
}


static void
full_auth_get_result_from_dialog (GnomePasswordDialog *dialog,
				  GnomeVFSModuleCallbackFullAuthenticationOut *out_args,
				  gboolean result)
{
	GnomePasswordDialogRemember remember;
	if (result) {
		out_args->abort_auth = FALSE;
		out_args->out_flags = 0;
		if (gnome_password_dialog_anon_selected (dialog)) {
			out_args->out_flags |= GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_OUT_ANON_SELECTED;
		}
		else {
			out_args->username = gnome_password_dialog_get_username (dialog);
			out_args->domain = gnome_password_dialog_get_domain (dialog);
			out_args->password = gnome_password_dialog_get_password (dialog);
		}

		remember = gnome_password_dialog_get_remember (dialog);
		if (remember == GNOME_PASSWORD_DIALOG_REMEMBER_SESSION) {
			out_args->save_password = TRUE;
			out_args->keyring = g_strdup ("session");
		} else if (remember == GNOME_PASSWORD_DIALOG_REMEMBER_FOREVER) {
			out_args->save_password = TRUE;
			out_args->keyring = NULL;
		} else {
			out_args->save_password = FALSE;
			out_args->keyring = NULL;
		}
	} else {
		out_args->abort_auth = TRUE;
	}
}

static void
present_full_authentication_dialog_blocking (const GnomeVFSModuleCallbackFullAuthenticationIn * in_args,
					     GnomeVFSModuleCallbackFullAuthenticationOut *out_args)
{
	GnomePasswordDialog *dialog;
	gboolean dialog_result;
	
	dialog = construct_full_password_dialog (in_args);

	dialog_result = gnome_password_dialog_run_and_block (dialog);

	full_auth_get_result_from_dialog (dialog, out_args, dialog_result);

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

typedef struct {
	const GnomeVFSModuleCallbackFullAuthenticationIn	*in_args;
	GnomeVFSModuleCallbackFullAuthenticationOut	*out_args;

	GnomeVFSModuleCallbackResponse response;
	gpointer response_data;
} FullCallbackInfo;

static void
full_authentication_dialog_button_clicked (GtkDialog *dialog, 
					   gint button_number, 
					   FullCallbackInfo *info)
{
	DEBUG_MSG (("+%s button: %d\n", G_STRFUNC, button_number));

	full_auth_get_result_from_dialog (GNOME_PASSWORD_DIALOG (dialog),
					  info->out_args,
					  button_number == GTK_RESPONSE_OK);

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
full_authentication_dialog_destroyed (GtkDialog *dialog, FullCallbackInfo *info)
{
	DEBUG_MSG (("+%s\n", G_STRFUNC));

	info->response (info->response_data);
	g_free (info);
	gnome_authentication_manager_dialog_visible--;
}

static gint /* GtkFunction */
present_full_authentication_dialog_nonblocking (FullCallbackInfo *info)
{
	GnomePasswordDialog *dialog;
	GtkWidget *toplevel, *current_grab;

	g_return_val_if_fail (info != NULL, 0);

	dialog = construct_full_password_dialog (info->in_args);

	toplevel = NULL;
	current_grab = gtk_grab_get_current ();
	if (current_grab) {
		toplevel = gtk_widget_get_toplevel (current_grab);
	}
	if (toplevel && GTK_WIDGET_TOPLEVEL (toplevel)) {
		/* There is a modal window, so we need to be modal too in order to
		 * get input. We set the other modal dialog as parent, which
		 * hopefully gives us better window management.
		 */
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (toplevel));
	} else {
		gtk_window_set_modal (GTK_WINDOW (dialog), FALSE);
	}

	g_signal_connect (dialog, "response", 
			  G_CALLBACK (full_authentication_dialog_button_clicked), info);

	g_signal_connect (dialog, "destroy", 
			  G_CALLBACK (full_authentication_dialog_destroyed), info);

	gtk_widget_show (GTK_WIDGET (dialog));

	gnome_authentication_manager_dialog_visible++;
	
	return 0;
}

static void /* GnomeVFSAsyncModuleCallback */
vfs_async_full_authentication_callback (gconstpointer in, size_t in_size, 
					gpointer out, size_t out_size, 
					gpointer user_data,
					GnomeVFSModuleCallbackResponse response,
					gpointer response_data)
{
	GnomeVFSModuleCallbackFullAuthenticationIn *in_real;
	GnomeVFSModuleCallbackFullAuthenticationOut *out_real;
	FullCallbackInfo *info;
	
	g_return_if_fail (sizeof (GnomeVFSModuleCallbackFullAuthenticationIn) == in_size
		&& sizeof (GnomeVFSModuleCallbackFullAuthenticationOut) == out_size);

	g_return_if_fail (in != NULL);
	g_return_if_fail (out != NULL);

	GDK_THREADS_ENTER ();
	
	in_real = (GnomeVFSModuleCallbackFullAuthenticationIn *)in;
	out_real = (GnomeVFSModuleCallbackFullAuthenticationOut *)out;

	DEBUG_MSG (("+%s uri:'%s' \n", G_STRFUNC, in_real->uri));

	info = g_new (FullCallbackInfo, 1);

	info->in_args = in_real;
	info->out_args = out_real;
	info->response = response;
	info->response_data = response_data;

	present_full_authentication_dialog_nonblocking (info);

	DEBUG_MSG (("-%s\n", G_STRFUNC));

	GDK_THREADS_LEAVE ();
}

static void /* GnomeVFSModuleCallback */
vfs_full_authentication_callback (gconstpointer in, size_t in_size, 
				  gpointer out, size_t out_size, 
				  gpointer user_data)
{
	GnomeVFSModuleCallbackFullAuthenticationIn *in_real;
	GnomeVFSModuleCallbackFullAuthenticationOut *out_real;
	
	g_return_if_fail (sizeof (GnomeVFSModuleCallbackFullAuthenticationIn) == in_size
		&& sizeof (GnomeVFSModuleCallbackFullAuthenticationOut) == out_size);

	g_return_if_fail (in != NULL);
	g_return_if_fail (out != NULL);

	GDK_THREADS_ENTER ();
	
	in_real = (GnomeVFSModuleCallbackFullAuthenticationIn *)in;
	out_real = (GnomeVFSModuleCallbackFullAuthenticationOut *)out;

	DEBUG_MSG (("+%s uri:'%s'\n", G_STRFUNC, in_real->uri));

	present_full_authentication_dialog_blocking (in_real, out_real);

	DEBUG_MSG (("-%s\n", G_STRFUNC));

	GDK_THREADS_LEAVE ();
}


typedef struct {
	const GnomeVFSModuleCallbackFillAuthenticationIn	*in_args;
	GnomeVFSModuleCallbackFillAuthenticationOut	*out_args;

	GnomeVFSModuleCallbackResponse response;
	gpointer response_data;
} FillCallbackInfo;


static void
fill_auth_callback (GnomeKeyringResult result,
		    GList             *list,
		    gpointer           data)
{
	FillCallbackInfo *info;
	GnomeKeyringNetworkPasswordData *pwd_data;;
	
	info = data;

	if (result != GNOME_KEYRING_RESULT_OK ||
	    list == NULL) {
		info->out_args->valid = FALSE;
	} else {
		/* We use the first result, which is the least specific match */
		pwd_data = list->data;
		info->out_args->valid = TRUE;
		info->out_args->username = g_strdup (pwd_data->user);
		info->out_args->domain = g_strdup (pwd_data->domain);
		info->out_args->password = g_strdup (pwd_data->password);
	}
	info->response (info->response_data);
}

static void /* GnomeVFSAsyncModuleCallback */
vfs_async_fill_authentication_callback (gconstpointer in, size_t in_size, 
					gpointer out, size_t out_size, 
					gpointer user_data,
					GnomeVFSModuleCallbackResponse response,
					gpointer response_data)
{
	GnomeVFSModuleCallbackFillAuthenticationIn *in_real;
	GnomeVFSModuleCallbackFillAuthenticationOut *out_real;
	gpointer request;
	FillCallbackInfo *info;
	
	g_return_if_fail (sizeof (GnomeVFSModuleCallbackFillAuthenticationIn) == in_size
		&& sizeof (GnomeVFSModuleCallbackFillAuthenticationOut) == out_size);

	g_return_if_fail (in != NULL);
	g_return_if_fail (out != NULL);

	in_real = (GnomeVFSModuleCallbackFillAuthenticationIn *)in;
	out_real = (GnomeVFSModuleCallbackFillAuthenticationOut *)out;

	DEBUG_MSG (("+%s uri:'%s' \n", G_STRFUNC, in_real->uri));

	info = g_new (FillCallbackInfo, 1);

	info->in_args = in_real;
	info->out_args = out_real;
	info->response = response;
	info->response_data = response_data;
	/* Check this return? */	
	request = gnome_keyring_find_network_password (in_real->username,
						       in_real->domain,
						       in_real->server,
						       in_real->object,
						       in_real->protocol,
						       in_real->authtype,
						       in_real->port,
						       fill_auth_callback,
						       info, g_free);

	DEBUG_MSG (("-%s\n", G_STRFUNC));
}

static void /* GnomeVFSModuleCallback */
vfs_fill_authentication_callback (gconstpointer in, size_t in_size, 
				  gpointer out, size_t out_size, 
				  gpointer user_data)
{
	GnomeVFSModuleCallbackFillAuthenticationIn *in_real;
	GnomeVFSModuleCallbackFillAuthenticationOut *out_real;
	GnomeKeyringNetworkPasswordData *pwd_data;
	GList *list;
	GnomeKeyringResult result;
	
	g_return_if_fail (sizeof (GnomeVFSModuleCallbackFillAuthenticationIn) == in_size
		&& sizeof (GnomeVFSModuleCallbackFillAuthenticationOut) == out_size);

	g_return_if_fail (in != NULL);
	g_return_if_fail (out != NULL);

	in_real = (GnomeVFSModuleCallbackFillAuthenticationIn *)in;
	out_real = (GnomeVFSModuleCallbackFillAuthenticationOut *)out;

	DEBUG_MSG (("+%s uri:'%s' \n", G_STRFUNC, in_real->uri));

	result = gnome_keyring_find_network_password_sync (in_real->username,
							   in_real->domain,
							   in_real->server,
							   in_real->object,
							   in_real->protocol,
							   in_real->authtype,
							   in_real->port,
							   &list);
	
	if (result != GNOME_KEYRING_RESULT_OK ||
	    list == NULL) {
		out_real->valid = FALSE;
	} else {
		/* We use the first result, which is the least specific match */
		pwd_data = list->data;

		out_real->valid = TRUE;
		out_real->username = g_strdup (pwd_data->user);
		out_real->domain = g_strdup (pwd_data->domain);
		out_real->password = g_strdup (pwd_data->password);
		
		gnome_keyring_network_password_list_free (list);
	}

	DEBUG_MSG (("-%s\n", G_STRFUNC));
}

typedef struct {
	const GnomeVFSModuleCallbackSaveAuthenticationIn	*in_args;
	GnomeVFSModuleCallbackSaveAuthenticationOut	*out_args;

	GnomeVFSModuleCallbackResponse response;
	gpointer response_data;
} SaveCallbackInfo;


static void
save_auth_callback (GnomeKeyringResult result,
		    guint32            item_id,
		    gpointer           data)
{
	SaveCallbackInfo *info;
	
	info = data;

	info->response (info->response_data);
}

static void /* GnomeVFSAsyncModuleCallback */
vfs_async_save_authentication_callback (gconstpointer in, size_t in_size, 
					gpointer out, size_t out_size, 
					gpointer user_data,
					GnomeVFSModuleCallbackResponse response,
					gpointer response_data)
{
	GnomeVFSModuleCallbackSaveAuthenticationIn *in_real;
	GnomeVFSModuleCallbackSaveAuthenticationOut *out_real;
	gpointer request;
	SaveCallbackInfo *info;
	
	g_return_if_fail (sizeof (GnomeVFSModuleCallbackSaveAuthenticationIn) == in_size
		&& sizeof (GnomeVFSModuleCallbackSaveAuthenticationOut) == out_size);

	g_return_if_fail (in != NULL);
	g_return_if_fail (out != NULL);

	in_real = (GnomeVFSModuleCallbackSaveAuthenticationIn *)in;
	out_real = (GnomeVFSModuleCallbackSaveAuthenticationOut *)out;

	DEBUG_MSG (("+%s uri:'%s' \n", G_STRFUNC, in_real->uri));

	info = g_new (SaveCallbackInfo, 1);

	info->in_args = in_real;
	info->out_args = out_real;
	info->response = response;
	info->response_data = response_data;
	/* Check this return? */	
	request = gnome_keyring_set_network_password (in_real->keyring,
						      in_real->username,
						      in_real->domain,
						      in_real->server,
						      in_real->object,
						      in_real->protocol,
						      in_real->authtype,
						      in_real->port,
						      in_real->password,
						      save_auth_callback,
						      info, g_free);

	DEBUG_MSG (("-%s\n", G_STRFUNC));
}

static void /* GnomeVFSModuleCallback */
vfs_save_authentication_callback (gconstpointer in, size_t in_size, 
				  gpointer out, size_t out_size, 
				  gpointer user_data)
{
	GnomeVFSModuleCallbackSaveAuthenticationIn *in_real;
	GnomeKeyringResult result;
	guint32 item;
	
	g_return_if_fail (sizeof (GnomeVFSModuleCallbackSaveAuthenticationIn) == in_size
		&& sizeof (GnomeVFSModuleCallbackSaveAuthenticationOut) == out_size);

	g_return_if_fail (in != NULL);
	g_return_if_fail (out != NULL);

	in_real = (GnomeVFSModuleCallbackSaveAuthenticationIn *)in;

	DEBUG_MSG (("+%s uri:'%s' \n", G_STRFUNC, in_real->uri));
	/* Check this return? */
	result = gnome_keyring_set_network_password_sync (in_real->keyring,
							  in_real->username,
							  in_real->domain,
							  in_real->server,
							  in_real->object,
							  in_real->protocol,
							  in_real->authtype,
							  in_real->port,
							  in_real->password,
							  &item);

	DEBUG_MSG (("-%s\n", G_STRFUNC));
}

typedef struct {
	const GnomeVFSModuleCallbackQuestionIn	*in_args;
	GnomeVFSModuleCallbackQuestionOut		*out_args;

	GnomeVFSModuleCallbackResponse response;
	gpointer response_data;
} QuestionCallbackInfo;

static void
question_dialog_button_clicked (GtkDialog *dialog, 
				      gint button_number, 
				      QuestionCallbackInfo *info)
{
	info->out_args->answer = button_number;
	gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
question_dialog_destroyed (GtkDialog *dialog, QuestionCallbackInfo *info)
{
	info->response (info->response_data);
	g_free (info);
	gnome_authentication_manager_dialog_visible--;
}

static void
question_dialog_closed (GtkDialog *dialog, QuestionCallbackInfo *info)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static GtkWidget *
create_question_dialog (char *prim_msg, char *sec_msg, char **choices) 
{
	GtkWidget *dialog;
	int cnt, len;
	
	dialog = gtk_message_dialog_new_with_markup (NULL, 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",prim_msg, sec_msg);
	
	if (choices) {
		/* First count the items in the list then 
		 * add the buttons in reverse order */
		for (len=0; choices[len] != NULL; len++); 
		for (cnt=len-1; cnt >= 0; cnt--) {
			/* Maybe we should define some gnome-vfs stockbuttons and 
			 * then replace them here with gtk-stockbuttons */
			gtk_dialog_add_button (GTK_DIALOG(dialog), choices[cnt], cnt);
		}
	}
	return (dialog);
}

static gint /* GtkFunction */
present_question_dialog_nonblocking (QuestionCallbackInfo *info)
{
	GtkWidget *dialog;
	GtkWidget *toplevel, *current_grab;

	g_return_val_if_fail (info != NULL, 0);

	dialog = create_question_dialog (info->in_args->primary_message, info->in_args->secondary_message, info->in_args->choices);
	
	toplevel = NULL;
	current_grab = gtk_grab_get_current ();
	if (current_grab) {
		toplevel = gtk_widget_get_toplevel (current_grab);
	}
	if (toplevel && GTK_WIDGET_TOPLEVEL (toplevel)) {
		/* There is a modal window, so we need to be modal too in order to
		 * get input. We set the other modal dialog as parent, which
		 * hopefully gives us better window management.
		 */
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (toplevel));
	}

	g_signal_connect (GTK_OBJECT(dialog), "response", 
			  G_CALLBACK (question_dialog_button_clicked), info);
	g_signal_connect (dialog, "close", 
			  G_CALLBACK (question_dialog_closed), info);
	g_signal_connect (dialog, "destroy", 
			  G_CALLBACK (question_dialog_destroyed), info);
	
	gtk_widget_show (GTK_WIDGET (dialog));

	gnome_authentication_manager_dialog_visible++;
	return 0;
}


static void /* GnomeVFSAsyncModuleCallback */
vfs_async_question_callback (gconstpointer in, size_t in_size, 
			     gpointer out, size_t out_size, 
			     gpointer user_data,
			     GnomeVFSModuleCallbackResponse response,
			     gpointer response_data)
{
	GnomeVFSModuleCallbackQuestionIn *in_real;
	GnomeVFSModuleCallbackQuestionOut *out_real;
	QuestionCallbackInfo *info;
	
	g_return_if_fail (sizeof (GnomeVFSModuleCallbackQuestionIn) == in_size
		&& sizeof (GnomeVFSModuleCallbackQuestionOut) == out_size);

	g_return_if_fail (in != NULL);
	g_return_if_fail (out != NULL);

	in_real = (GnomeVFSModuleCallbackQuestionIn *)in;
	out_real = (GnomeVFSModuleCallbackQuestionOut *)out;
	
	out_real->answer = -1; /* Set a default value */

	info = g_new (QuestionCallbackInfo, 1);
	
	info->in_args = in_real;
	info->out_args = out_real;
	info->response = response;
	info->response_data = response_data;

	GDK_THREADS_ENTER ();

	present_question_dialog_nonblocking (info);

	GDK_THREADS_LEAVE ();
}

static void /* GnomeVFSModuleCallback */
vfs_question_callback (gconstpointer in, size_t in_size, 
		       gpointer out, size_t out_size, 
		       gpointer user_data)
{
	GnomeVFSModuleCallbackQuestionIn *in_real;
	GnomeVFSModuleCallbackQuestionOut *out_real;
	GtkWidget *dialog;
	
	g_return_if_fail (sizeof (GnomeVFSModuleCallbackQuestionIn) == in_size
		&& sizeof (GnomeVFSModuleCallbackQuestionOut) == out_size);

	g_return_if_fail (in != NULL);
	g_return_if_fail (out != NULL);

	in_real = (GnomeVFSModuleCallbackQuestionIn *)in;
	out_real = (GnomeVFSModuleCallbackQuestionOut *)out;
	
	GDK_THREADS_ENTER ();

	out_real->answer = -1; /* Set a default value */
	dialog = create_question_dialog (in_real->primary_message, in_real->secondary_message, in_real->choices);
	out_real->answer = gtk_dialog_run (GTK_DIALOG(dialog));

	gtk_widget_destroy (GTK_WIDGET (dialog));

	GDK_THREADS_LEAVE ();
}

/** 
 * gnome_authentication_manager_init:
 * 
 * This function checks for thread support and does a thread initialisation, if the support 
 * is available. Also sets the default sync and async gnome-vfs callbacks for various types
 * of authentication.
 * 
 * Note: If you call this, and you use threads with gtk+, you must never
 * hold the gdk lock while doing synchronous gnome-vfs calls. Otherwise
 * an authentication callback presenting a dialog could try to grab the
 * already held gdk lock, causing a deadlock.
 *
 * Since: 2.4
 */
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

	gnome_vfs_async_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
						     vfs_async_fill_authentication_callback, 
						     GINT_TO_POINTER (0),
						     NULL);
	gnome_vfs_async_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
						     vfs_async_full_authentication_callback, 
						     GINT_TO_POINTER (0),
						     NULL);
	gnome_vfs_async_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
						     vfs_async_save_authentication_callback, 
						     GINT_TO_POINTER (0),
						     NULL);
	gnome_vfs_async_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_QUESTION,
						     vfs_async_question_callback, 
						     GINT_TO_POINTER (0),
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

	gnome_vfs_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
					       vfs_fill_authentication_callback, 
					       GINT_TO_POINTER (0),
					       NULL);
	gnome_vfs_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
					       vfs_full_authentication_callback, 
					       GINT_TO_POINTER (0),
					       NULL);
	gnome_vfs_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
					       vfs_save_authentication_callback, 
					       GINT_TO_POINTER (0),
					       NULL);
	gnome_vfs_module_callback_set_default (GNOME_VFS_MODULE_CALLBACK_QUESTION,
					       vfs_question_callback, 
					       GINT_TO_POINTER (0),
					       NULL);

}

/**
 * gnome_authentication_manager_push_async:
 *
 * This function calls gnome_vfs_async_module_callback_push() to set temporary async handlers for 
 * the various types of authentication.
 *  
 * Since: 2.6
 */
void
gnome_authentication_manager_push_async (void)
{
	gnome_vfs_async_module_callback_push (GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
					      vfs_async_authentication_callback, 
					      GINT_TO_POINTER (0),
					      NULL);
	gnome_vfs_async_module_callback_push (GNOME_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION, 
					      vfs_async_authentication_callback, 
					      GINT_TO_POINTER (1),
					      NULL);
	
	gnome_vfs_async_module_callback_push (GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
					      vfs_async_fill_authentication_callback, 
					      GINT_TO_POINTER (0),
					      NULL);
	gnome_vfs_async_module_callback_push (GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
					      vfs_async_full_authentication_callback, 
					      GINT_TO_POINTER (0),
					      NULL);
	gnome_vfs_async_module_callback_push (GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
					      vfs_async_save_authentication_callback, 
					      GINT_TO_POINTER (0),
					      NULL);
	gnome_vfs_async_module_callback_push (GNOME_VFS_MODULE_CALLBACK_QUESTION,
					      vfs_async_question_callback, 
					      GINT_TO_POINTER (0),
					      NULL);
}

/**
 * gnome_authentication_manager_pop_async:
 *
 * This function calls gnome_vfs_async_module_callback_pop() to remove all the temporary async 
 * gnome-vfs callbacks associated with various types of authentication.
 *
 * Since: 2.6
 */
void
gnome_authentication_manager_pop_async (void)
{
	gnome_vfs_async_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION);
	gnome_vfs_async_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION);
	gnome_vfs_async_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION);
	gnome_vfs_async_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION);
	gnome_vfs_async_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION);
	gnome_vfs_async_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_QUESTION);
}

/**
 * gnome_authentication_manager_push_sync:
 *
 * This function calls gnome_vfs_module_callback_push() to set temperory handlers for 
 * various types of authentication.
 *
 * Since: 2.6
 */
void
gnome_authentication_manager_push_sync (void)
{
	
	gnome_vfs_module_callback_push (GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
					vfs_authentication_callback, 
					GINT_TO_POINTER (0),
					NULL);
	gnome_vfs_module_callback_push (GNOME_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION, 
					vfs_authentication_callback, 
					GINT_TO_POINTER (1),
					NULL);
	
	gnome_vfs_module_callback_push (GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
					vfs_fill_authentication_callback, 
					GINT_TO_POINTER (0),
					NULL);
	gnome_vfs_module_callback_push (GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
					vfs_full_authentication_callback, 
					GINT_TO_POINTER (0),
					NULL);
	gnome_vfs_module_callback_push (GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
					vfs_save_authentication_callback, 
					GINT_TO_POINTER (0),
					NULL);
	gnome_vfs_module_callback_push (GNOME_VFS_MODULE_CALLBACK_QUESTION,
					vfs_question_callback, 
					GINT_TO_POINTER (0),
					NULL);
}

/**
 * gnome_authentication_manager_pop_sync:
 * 
 * This function calls gnome_vfs_module_callback_pop() to remove all the gnome-vfs sync handlers associated
 * with various types of authentication.
 *
 * Since: 2.6
 */

void
gnome_authentication_manager_pop_sync (void)
{
	gnome_vfs_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION);
	gnome_vfs_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION);
	gnome_vfs_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION);
	gnome_vfs_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION);
	gnome_vfs_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION);
	gnome_vfs_module_callback_pop (GNOME_VFS_MODULE_CALLBACK_QUESTION);
}

