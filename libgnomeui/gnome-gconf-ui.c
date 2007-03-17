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

#include <glib/gi18n-lib.h>

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

static void
dialog_add_details (GtkDialog  *dialog,
                    GtkWidget  *details,
                    int         extra_hsize,
                    int         extra_vsize)
{
        GtkWidget *hbox;
        GtkWidget *button;
        GtkRequisition req;
        GtkWidget *align;
  
        hbox = gtk_hbox_new (FALSE, 0);

        gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  
        gtk_box_pack_start (GTK_BOX (dialog->vbox),
                            hbox,
                            TRUE, TRUE, 0);

        align = gtk_alignment_new (1.0, 1.0, 0.0, 0.0);
  
        button = gtk_button_new_with_mnemonic (_("_Details"));

        gtk_container_add (GTK_CONTAINER (align), button);
  
        gtk_box_pack_end (GTK_BOX (hbox), align,
                          FALSE, FALSE, 0);  
  
        gtk_box_pack_start (GTK_BOX (hbox), details,
                            TRUE, TRUE, 0);

        /* show the details on click */
        g_signal_connect_swapped (G_OBJECT (button),
                                  "clicked",
                                  G_CALLBACK (gtk_widget_show),
                                  details);
  
        /* second callback destroys the button (note disconnects first callback) */
        g_signal_connect (G_OBJECT (button), "clicked",
                          G_CALLBACK (gtk_widget_destroy),
                          NULL);

        /* Set default dialog size to size with the details,
   * and without the button, but then rehide the details
   */
        gtk_widget_show_all (hbox);

        gtk_widget_size_request (GTK_WIDGET (dialog), &req);

        gtk_window_set_default_size (GTK_WINDOW (dialog),
                                     req.width + extra_hsize,
                                     req.height + extra_vsize);
  
        gtk_widget_hide (details);
}

typedef struct {
        GConfClient *client;
} ErrorIdleData;

static guint error_handler_idle = 0;
static GSList *pending_errors = NULL;
static ErrorIdleData eid = { NULL };
static GtkWidget *current_dialog = NULL;
static GtkWidget *current_details = NULL;

static gboolean
error_idle_func (gpointer data)
{
        GSList *iter;

        error_handler_idle = 0;

        g_return_val_if_fail(eid.client != NULL, FALSE);
        g_return_val_if_fail(pending_errors != NULL, FALSE);

        GDK_THREADS_ENTER();

        if (current_dialog == NULL) {
                GtkWidget *dialog;
                gboolean have_overridden = FALSE;
                const char* fmt = NULL;
                GtkWidget *sw;

                iter = pending_errors;
                while (iter != NULL) {
                        GError *error = iter->data;

                        if (g_error_matches (error, GCONF_ERROR, GCONF_ERROR_OVERRIDDEN) ||
                            g_error_matches (error, GCONF_ERROR, GCONF_ERROR_NO_WRITABLE_DATABASE))
                                have_overridden = TRUE;

                        iter = g_slist_next (iter);
                }

                if (have_overridden) {
                        fmt = _("The application \"%s\" attempted to change an "
                                "aspect of your configuration that your system "
                                "administrator or operating system vendor does not "
                                "allow you to change. Some of the settings you have "
                                "selected may not take effect, or may not be "
                                "restored next time you use the application.");
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
                gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
                g_signal_connect (dialog, "response",
                                  G_CALLBACK (gtk_widget_destroy),
                                  NULL);

                sw = gtk_scrolled_window_new (NULL, NULL);
                gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                                GTK_POLICY_NEVER,
                                                GTK_POLICY_AUTOMATIC);
                gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                                     GTK_SHADOW_IN);
                current_details = gtk_text_view_new ();
                gtk_text_view_set_editable (GTK_TEXT_VIEW (current_details),
                                            FALSE);
                gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (current_details),
                                             GTK_WRAP_WORD);
                gtk_container_add (GTK_CONTAINER (sw), current_details);
                gtk_widget_show (current_details);
                
                dialog_add_details (GTK_DIALOG (dialog),
                                    sw,
                                    0, 70);
                
                current_dialog = dialog;
                g_object_add_weak_pointer (G_OBJECT (current_dialog),
                                           (void**) &current_dialog);

                g_object_add_weak_pointer (G_OBJECT (current_details),
                                           (void**) &current_details);
        }

        g_assert (current_dialog);
        g_assert (current_details);
        
        iter = pending_errors;
        while (iter != NULL) {
                GError *error = iter->data;
                GtkTextIter end;
                GtkTextBuffer *buffer;

                buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (current_details));

                gtk_text_buffer_get_end_iter (buffer, &end);

                gtk_text_buffer_insert (buffer, &end, error->message, -1);
                gtk_text_buffer_insert (buffer, &end, "\n", -1);

                g_error_free (error);

                iter = g_slist_next (iter);
        }

        g_slist_free (pending_errors);

        pending_errors = NULL;

        g_object_unref (G_OBJECT (eid.client));
        eid.client = NULL;

        gtk_window_present (GTK_WINDOW (current_dialog));
        
        GDK_THREADS_LEAVE();

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
        gchar *settings_dir;
	GConfClient* client = NULL;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

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

        /* Leak the GConfClient reference, we want to keep
         * the client alive forever.
         */
}

gboolean
_gnome_gconf_get_bool (const char *key)
{
	GConfClient *client;
	gboolean ret;

	gnomeui_gconf_lazy_init ();

	client = gconf_client_get_default ();

	ret = gconf_client_get_bool (client,
				     key,
                                     NULL);

	g_object_unref (G_OBJECT (client));

	return ret;
}
