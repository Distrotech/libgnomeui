/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME GUI Library - gnome-gconf-ui.c
 * Copyright (C) 2000  Red Hat Inc.,
 * All rights reserved.
 *
 * Author: Jonathan Blandford  <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/


#include <config.h>
#include <stdlib.h>

#define GCONF_ENABLE_INTERNALS
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "gnome-i18nP.h"

#include <libgnome/libgnome.h>
#include <gtk/gtk.h>

#include "gnome-gconf-ui.h"

/*
 * Our global GConfClient, and module stuff
 */
static void gnome_default_gconf_client_error_handler (GConfClient                  *client,
                                                      GError                       *error);


static void
gnome_gconf_ui_pre_args_parse (GnomeProgram *app, GnomeModuleInfo *mod_info)
{
        gconf_client_set_global_default_error_handler
		(gnome_default_gconf_client_error_handler);
}

const GnomeModuleInfo *
_gnome_gconf_ui_module_info_get (void)
{
	static GnomeModuleInfo module_info = {
		"gnome-gconf-ui",
		gconf_version,
		N_("GNOME GConf UI Support"),
		NULL,
		NULL /* instance init */,
		gnome_gconf_ui_pre_args_parse,
		NULL /* post_args_parse */,
		NULL /* options */,
		NULL /* init_pass */,
		NULL /* class_init */,
		NULL, NULL /* expansions */
	};

	return &module_info;
}

typedef struct {
        GConfClient *client;
} ErrorIdleData;

static guint error_handler_idle = 0;
static GSList *pending_errors = NULL;
static ErrorIdleData eid = { NULL };

static gboolean
error_idle_func (gpointer data)
{
        GtkWidget *dialog;
        GSList *iter;
        gboolean have_overridden = FALSE;
        const gchar* fmt = NULL;
        
        error_handler_idle = 0;

        g_return_val_if_fail(eid.client != NULL, FALSE);
        g_return_val_if_fail(pending_errors != NULL, FALSE);
        
        iter = pending_errors;
        while (iter != NULL) {
                GError *error = iter->data;

                if (g_error_matches (error, GCONF_ERROR, GCONF_ERROR_OVERRIDDEN))
                        have_overridden = TRUE;
                
                iter = g_slist_next(iter);
        }
        
        if (have_overridden) {
                fmt = _("You attempted to change an aspect of your "
			"configuration that your system administrator "
			"or operating system vendor does not allow you to "
			"change. Some of the settings you have selected may "
			"not take effect, or may not be restored next time "
			"you use this application (%s).");
                
        } else {
                fmt = _("An error occurred while loading or saving "
			"configuration information for %s. Some of your "
			"configuration settings may not work properly.");
        }

        dialog = gtk_message_dialog_new (NULL /* parent */,
					 0 /* flags */,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 fmt,
					 gnome_program_get_human_readable_name(gnome_program_get()));
	gtk_signal_connect_object (GTK_OBJECT (dialog), "response",
				   GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   GTK_OBJECT (dialog));
        gtk_widget_show_all (dialog);


        /* FIXME put this in a "Technical Details" optional part of the dialog
           that can be opened up if users are interested */
        iter = pending_errors;
        while (iter != NULL) {
                GError *error = iter->data;
                iter->data = NULL;

                fprintf(stderr, _("GConf error details: %s\n"), error->message);

                g_error_free(error);
                
                iter = g_slist_next(iter);
        }

        g_slist_free(pending_errors);

        pending_errors = NULL;
        
        g_object_unref(G_OBJECT(eid.client));
        eid.client = NULL;

        return FALSE;
}

static void
gnome_default_gconf_client_error_handler (GConfClient                  *client,
                                          GError                       *error)
{
        g_object_ref(G_OBJECT(client));
        
        if (eid.client) {
                g_object_unref(G_OBJECT(eid.client));
        }
        
        eid.client = client;
        
        pending_errors = g_slist_append(pending_errors, g_error_copy(error));

        if (error_handler_idle == 0) {
                error_handler_idle = gtk_idle_add (error_idle_func, NULL);
        }
}

/**
 * gnomeui_gconf_lazy_init:
 *
 * Description:  Internal libgnome/ui routine.  You never have
 * to do this from your code.  But all places in libgnome/ui
 * that need gconf should call this before calling any gconf
 * calls.
 **/
void
_gnomeui_gconf_lazy_init (void)
{
	/* Note this is the same as in libgnome/libgnome/gnome-gconf.c, keep
	 * this in sync (it's called gnome_gconf_lazy_init) */
	char *argv [] = { "dummy", NULL };
        gchar *settings_dir;
	GConfClient* client = NULL;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	gconf_init (1, argv, NULL);

        client = gconf_client_get_default ();

        gconf_client_add_dir (client,
			      "/desktop/gnome",
			      GCONF_CLIENT_PRELOAD_NONE, NULL);

        settings_dir = gnome_gconf_get_gnome_libs_settings_relative ("");

        gconf_client_add_dir (client,
			      settings_dir,
			      /* Possibly we should turn preload on for this */
			      GCONF_CLIENT_PRELOAD_NONE,
			      NULL);
        g_free (settings_dir);
}
