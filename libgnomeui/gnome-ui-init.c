/* -*- Mode: C; tab-width: 8; c-basic-offset: 8; indent-tabs-mode: nil -*- */
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

/* Blame Elliot for most of this stuff*/

#include <config.h>

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <glib.h>
#ifndef G_OS_WIN32
#include <sys/wait.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

/* Must be before all other gnome includes!! */
#include <glib/gi18n-lib.h>

#include <gdkconfig.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#include <libgnome/libgnome.h>
#include <bonobo/bonobo-ui-main.h>
#include <gconf/gconf-client.h>

#include "gnome-client.h"
#include "gnome-gconf-ui.h"
#include "gnome-ui-init.h"
#include "gnome-stock-icons.h"
#include "gnome-url.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgnomeuiP.h"

#include <gtk/gtk.h>

/*****************************************************************************
 * libgnomeui
 *****************************************************************************/

static void libgnomeui_arg_callback 	(poptContext con,
					 enum poptCallbackReason reason,
					 const struct poptOption * opt,
					 const char * arg,
					 void * data);
static gboolean libgnomeui_goption_disable_crash_dialog (const gchar *option_name,
							 const gchar *value,
							 gpointer data,
							 GError **error);
static void libgnomeui_init_pass	(const GnomeModuleInfo *mod_info);
static void libgnomeui_class_init	(GnomeProgramClass *klass,
					 const GnomeModuleInfo *mod_info);
static void libgnomeui_instance_init	(GnomeProgram *program,
					 GnomeModuleInfo *mod_info);
static void libgnomeui_post_args_parse	(GnomeProgram *app,
					 GnomeModuleInfo *mod_info);
static void libgnomeui_rc_parse		(GnomeProgram *program);

/* Prototype for a private gnome_stock function */
G_GNUC_INTERNAL void _gnome_stock_icons_init (void);

/* Whether to make noises when the user clicks a button, etc.  We cache it
 * in a boolean rather than querying GConf every time.
 */
static gboolean use_event_sounds;

/* GConf client for monitoring the event sounds option */
static GConfClient *gconf_client = NULL;

enum { ARG_DISABLE_CRASH_DIALOG=1, ARG_DISPLAY };

G_GNUC_INTERNAL void _gnome_ui_gettext_init (gboolean);

void G_GNUC_INTERNAL
_gnome_ui_gettext_init (gboolean bind_codeset)
{
	static gboolean initialized = FALSE;
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET	
	static gboolean codeset_bound = FALSE;
#endif

	if (!initialized)
	{
		bindtextdomain (GETTEXT_PACKAGE, GNOMEUILOCALEDIR);
		initialized = TRUE;
	}
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET	
	if (!codeset_bound && bind_codeset)
	{
		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
		codeset_bound = TRUE;
	}
#endif
}

static GOptionGroup *
libgnomeui_get_goption_group (void)
{
	const GOptionEntry libgnomeui_goptions[] = {
		{ "disable-crash-dialog", '\0', G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN,
		  G_OPTION_ARG_CALLBACK, libgnomeui_goption_disable_crash_dialog,
		  N_("Disable Crash Dialog"), NULL },
		  /* --display is already handled by gtk+ */
		{ NULL }
	};
	GOptionGroup *option_group;

	_gnome_ui_gettext_init (TRUE);

	option_group = g_option_group_new ("gnome-ui",
					   N_("GNOME GUI Library:"),
					   N_("Show GNOME GUI options"),
					   NULL, NULL);
	g_option_group_set_translation_domain (option_group, GETTEXT_PACKAGE);
	g_option_group_add_entries(option_group, libgnomeui_goptions);

	return option_group;
}

const GnomeModuleInfo *
libgnomeui_module_info_get (void)
{
	static struct poptOption libgnomeui_options[] = {
		{ NULL, '\0', POPT_ARG_INTL_DOMAIN, GETTEXT_PACKAGE, 0, NULL, NULL },
		{ NULL, '\0', POPT_ARG_CALLBACK|POPT_CBFLAG_PRE|POPT_CBFLAG_POST,
		  &libgnomeui_arg_callback, 0, NULL, NULL },
		{ "disable-crash-dialog", '\0', POPT_ARG_NONE, NULL, ARG_DISABLE_CRASH_DIALOG,
		  N_("Disable Crash Dialog"), NULL },
		{ "display", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, NULL, ARG_DISPLAY,
		  N_("X display to use"), N_("DISPLAY") },
		{ NULL, '\0', 0, NULL, 0, NULL, NULL }
	};

	static GnomeModuleInfo module_info = {
		"libgnomeui", VERSION, N_("GNOME GUI Library"),
		NULL, libgnomeui_instance_init,
		NULL, libgnomeui_post_args_parse,
		libgnomeui_options,
		libgnomeui_init_pass, libgnomeui_class_init,
		NULL, NULL
	};

	module_info.get_goption_group_func = libgnomeui_get_goption_group;

	if (module_info.requirements == NULL) {
		static GnomeModuleRequirement req[6];

		_gnome_ui_gettext_init (FALSE);

		req[0].required_version = "1.101.2";
		req[0].module_info = LIBBONOBOUI_MODULE;

		req[1].required_version = VERSION;
		req[1].module_info = gnome_client_module_info_get ();

		req[2].required_version = "1.1.1";
		req[2].module_info = gnome_gconf_ui_module_info_get ();

		req[3].required_version = NULL;
		req[3].module_info = NULL;

		module_info.requirements = req;
	}

	return &module_info;
}

typedef struct {
        guint crash_dialog_id;
        guint display_id;
        guint default_icon_id;
} GnomeProgramClass_libgnomeui;

typedef struct {
	gchar	*default_icon;
	guint constructed : 1;
	guint show_crash_dialog : 1;
} GnomeProgramPrivate_libgnomeui;

static GQuark quark_gnome_program_private_libgnomeui = 0;
static GQuark quark_gnome_program_class_libgnomeui = 0;

static void
show_url (GtkWidget *parent,
	  const char *url)
{
	GdkScreen *screen;
	GError *error = NULL;

	screen = gtk_widget_get_screen (parent);

	if (!gnome_url_show_on_screen (url, screen, &error))
	{
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 "%s", _("Could not open link"));
		gtk_message_dialog_format_secondary_text
			(GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
		g_error_free (error);

		g_signal_connect (dialog, "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);
		gtk_widget_show (dialog);
	}
}

static void
about_url_hook (GtkAboutDialog *about,
		const char *link,
		gpointer data)
{
	show_url (GTK_WIDGET (about), link);
}

static void
about_email_hook (GtkAboutDialog *about,
		  const char *email,
		  gpointer data)
{
	char *address;

	/* FIXME: escaping? */
	address = g_strdup_printf ("mailto:%s", email);
	show_url (GTK_WIDGET (about), address);
	g_free (address);
}

static void
libgnomeui_private_free (GnomeProgramPrivate_libgnomeui *priv)
{
	g_free (priv->default_icon);
	g_free (priv);
}

static void
libgnomeui_get_property (GObject *object, guint param_id, GValue *value,
                         GParamSpec *pspec)
{
        GnomeProgramClass_libgnomeui *cdata;
        GnomeProgramPrivate_libgnomeui *priv;
        GnomeProgram *program;

        g_return_if_fail(object != NULL);
        g_return_if_fail(GNOME_IS_PROGRAM (object));

        program = GNOME_PROGRAM(object);

        cdata = g_type_get_qdata(G_OBJECT_TYPE(program), quark_gnome_program_class_libgnomeui);
        priv = g_object_get_qdata(G_OBJECT(program), quark_gnome_program_private_libgnomeui);

	if (param_id == cdata->default_icon_id)
		g_value_set_string (value, priv->default_icon);
	else if (param_id == cdata->crash_dialog_id)
		g_value_set_boolean (value, priv->show_crash_dialog);
	else if (param_id == cdata->display_id)
		g_value_set_string (value, gdk_get_display_arg_name ());
	else 
        	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
}

static void
libgnomeui_set_property (GObject *object, guint param_id,
                         const GValue *value, GParamSpec *pspec)
{
        GnomeProgramClass_libgnomeui *cdata;
        GnomeProgramPrivate_libgnomeui *priv;
        GnomeProgram *program;

        g_return_if_fail(object != NULL);
        g_return_if_fail(GNOME_IS_PROGRAM (object));

        program = GNOME_PROGRAM(object);

        cdata = g_type_get_qdata(G_OBJECT_TYPE(program), quark_gnome_program_class_libgnomeui);
        priv = g_object_get_qdata(G_OBJECT(program), quark_gnome_program_private_libgnomeui);

	if (param_id == cdata->default_icon_id)
		priv->default_icon = g_value_dup_string (value);
	else if (param_id == cdata->crash_dialog_id)
		priv->show_crash_dialog = g_value_get_boolean (value) != FALSE;
	else if (param_id == cdata->display_id)
		; /* We don't need to store it, we get it from gdk when we need it */
	else
                G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
}

static void
libgnomeui_init_pass (const GnomeModuleInfo *mod_info)
{
    if (!quark_gnome_program_private_libgnomeui)
	quark_gnome_program_private_libgnomeui = g_quark_from_static_string
	    ("gnome-program-private:libgnomeui");

    if (!quark_gnome_program_class_libgnomeui)
	quark_gnome_program_class_libgnomeui = g_quark_from_static_string
	    ("gnome-program-class:libgnomeui");
}

static void
libgnomeui_class_init (GnomeProgramClass *klass, const GnomeModuleInfo *mod_info)
{
        GnomeProgramClass_libgnomeui *cdata = g_new0 (GnomeProgramClass_libgnomeui, 1);

        g_type_set_qdata (G_OBJECT_CLASS_TYPE (klass), quark_gnome_program_class_libgnomeui, cdata);

        cdata->crash_dialog_id = gnome_program_install_property (
                klass,
                libgnomeui_get_property,
                libgnomeui_set_property,
                g_param_spec_boolean (LIBGNOMEUI_PARAM_CRASH_DIALOG, NULL, NULL,
                                      TRUE, (G_PARAM_READABLE | G_PARAM_WRITABLE |
                                      G_PARAM_CONSTRUCT)));
        cdata->display_id = gnome_program_install_property (
                klass,
                libgnomeui_get_property,
                libgnomeui_set_property,
                g_param_spec_string (LIBGNOMEUI_PARAM_DISPLAY, NULL, NULL, 
				     g_getenv ("DISPLAY"),
                                     (G_PARAM_READABLE | G_PARAM_WRITABLE |
                                      G_PARAM_CONSTRUCT_ONLY)));

        cdata->default_icon_id = gnome_program_install_property (
                klass,
                libgnomeui_get_property,
                libgnomeui_set_property,
                g_param_spec_string (LIBGNOMEUI_PARAM_DEFAULT_ICON, NULL, NULL, NULL,
                                     (G_PARAM_READABLE | G_PARAM_WRITABLE |
                                      G_PARAM_CONSTRUCT_ONLY)));

	gtk_about_dialog_set_url_hook (about_url_hook, NULL, NULL);
	gtk_about_dialog_set_email_hook (about_email_hook, NULL, NULL);
}

static void
libgnomeui_instance_init (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
    GnomeProgramPrivate_libgnomeui *priv = g_new0 (GnomeProgramPrivate_libgnomeui, 1);

    g_object_set_qdata_full (G_OBJECT (program),
			     quark_gnome_program_private_libgnomeui,
			     priv, (GDestroyNotify) libgnomeui_private_free);
}

static gboolean
relay_gnome_signal (GSignalInvocationHint *hint,
              	     guint n_param_values,
                    const GValue *param_values,
                    gchar *signame)
{
        char *pieces[3] = {"gnome-2", NULL, NULL};
        static GQuark disable_sound_quark = 0;
        GObject *object = g_value_get_object (&param_values[0]);

        if (!use_event_sounds)
                return TRUE;

        if (!disable_sound_quark)
                disable_sound_quark = g_quark_from_static_string ("gnome_disable_sound_events");

        if (g_object_get_qdata (object, disable_sound_quark))
                return TRUE;

        if (GTK_IS_WIDGET (object)) {

                if (!GTK_WIDGET_DRAWABLE (object))
                        return TRUE;

                if (GTK_IS_MESSAGE_DIALOG (object)) {
                        GtkMessageType message_type;
                        g_object_get (object, "message_type", &message_type, NULL);
                        switch (message_type)
                        {
                                case GTK_MESSAGE_INFO:
                                        pieces[1] = "info";
                                        break;

                                case GTK_MESSAGE_QUESTION:
                                        pieces[1] = "question";
                                        break;

                                case GTK_MESSAGE_WARNING:
                                        pieces[1] = "warning";
                                        break;

                                case GTK_MESSAGE_ERROR:
                                        pieces[1] = "error";
                                        break;

                                default:
                                        pieces[1] = NULL;
                        }
                }
        }

        if (gnome_sound_connection_get () < 0)
                return TRUE;

        gnome_triggers_vdo ("", NULL, (const char **) pieces);

        return TRUE;
}

static void
initialize_gnome_signal_relay (void)
{
        static gboolean initialized = FALSE;
        int signum;

        if (initialized)
                return;

        initialized = TRUE;

        signum = g_signal_lookup ("show", GTK_TYPE_MESSAGE_DIALOG);
        g_signal_add_emission_hook (signum, 0,
                                    (GSignalEmissionHook) relay_gnome_signal,
                                    NULL, NULL);

}

static gboolean
relay_gtk_signal(GSignalInvocationHint *hint,
		 guint n_param_values,
		 const GValue *param_values,
		 gchar *signame)
{
  char *pieces[3] = {"gtk-events-2", NULL, NULL};
  static GQuark disable_sound_quark = 0;
  GObject *object = g_value_get_object (&param_values[0]);

  pieces[1] = signame;

  if (!use_event_sounds)
    return TRUE;

  if(!disable_sound_quark)
    disable_sound_quark = g_quark_from_static_string("gnome_disable_sound_events");

  if(g_object_get_qdata (object, disable_sound_quark))
    return TRUE;

  if(GTK_IS_WIDGET(object)) {
    if(!GTK_WIDGET_DRAWABLE(object))
      return TRUE;

    if(GTK_IS_MENU_ITEM(object) && GTK_MENU_ITEM(object)->submenu)
      return TRUE;
  }

  if (gnome_sound_connection_get() < 0)
    return TRUE;

  gnome_triggers_vdo("", NULL, (const char **)pieces);

  return TRUE;
}

static void
initialize_gtk_signal_relay (void)
{
	gpointer iter_signames;
	char *signame;
	char *ctmp, *ctmp2;
        static gboolean initialized = FALSE;

        if (initialized)
                return;

        initialized = TRUE;
	
	ctmp = gnome_config_file ("/sound/events/gtk-events-2.soundlist");
	ctmp2 = g_strconcat ("=", ctmp, "=", NULL);
	g_free (ctmp);
	iter_signames = gnome_config_init_iterator_sections (ctmp2);
	gnome_config_push_prefix (ctmp2);
	g_free (ctmp2);
	
	while ((iter_signames = gnome_config_iterator_next (iter_signames,
							    &signame, NULL))) {
		int signums [5];
		int nsigs, i;
		gboolean used_signame;

		/*
		 * XXX this is an incredible hack based on a compile-time
		 * knowledge of what gtk widgets do what, rather than
		 * anything based on the info available at runtime.
		 */
		if (!strcmp (signame, "activate")) {
			g_type_class_unref (g_type_class_ref (gtk_menu_item_get_type ()));
			signums [0] = g_signal_lookup (signame, gtk_menu_item_get_type ());
			
			g_type_class_unref (g_type_class_ref (gtk_entry_get_type ()));
			signums [1] = g_signal_lookup (signame, gtk_entry_get_type ());
			nsigs = 2;
		} else if (!strcmp(signame, "toggled")) {
			g_type_class_unref (g_type_class_ref (gtk_toggle_button_get_type ()));
			signums [0] = g_signal_lookup (signame,
							 gtk_toggle_button_get_type ());
			
			g_type_class_unref (g_type_class_ref (gtk_check_menu_item_get_type ()));
			signums [1] = g_signal_lookup (signame,
							 gtk_check_menu_item_get_type ());
			nsigs = 2;
		} else if (!strcmp (signame, "clicked")) {
			g_type_class_unref (g_type_class_ref (gtk_button_get_type ()));
			signums [0] = g_signal_lookup (signame, gtk_button_get_type ());
			nsigs = 1;
		} else {
			g_type_class_unref (g_type_class_ref (gtk_widget_get_type ()));
			signums [0] = g_signal_lookup (signame, gtk_widget_get_type ());
			nsigs = 1;
		}

		used_signame = FALSE;
		for (i = 0; i < nsigs; i++) {
			if (signums [i] > 0) {
				g_signal_add_emission_hook (
                                        signums [i], 0,
                                        (GSignalEmissionHook) relay_gtk_signal,
                                        signame, NULL);
                                used_signame = TRUE;
                        }
                }

                if (!used_signame)
                        g_free (signame);
	}
	gnome_config_pop_prefix ();
}

/* Callback used when the GConf event_sounds key's value changes */
static void
event_sounds_changed_cb (GConfClient* client, guint cnxn_id, GConfEntry *entry, gpointer data)
{
        gboolean new_use_event_sounds;

        new_use_event_sounds = (gnome_gconf_get_bool ("/desktop/gnome/sound/enable_esd") &&
                                gnome_gconf_get_bool ("/desktop/gnome/sound/event_sounds"));

        if (new_use_event_sounds && !use_event_sounds) {
                GDK_THREADS_ENTER();
                initialize_gtk_signal_relay ();
                initialize_gnome_signal_relay ();
                GDK_THREADS_LEAVE();
	}

        use_event_sounds = new_use_event_sounds;
}

static void
setup_event_listener (void)
{
        gconf_client = gconf_client_get_default ();
        if (!gconf_client)
                return;

        gconf_client_add_dir (gconf_client, "/desktop/gnome/sound",
                              GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

        gconf_client_notify_add (gconf_client, "/desktop/gnome/sound/event_sounds",
                                 event_sounds_changed_cb,
                                 NULL, NULL, NULL);
        gconf_client_notify_add (gconf_client, "/desktop/gnome/sound/enable_esd",
                                 event_sounds_changed_cb,
                                 NULL, NULL, NULL);
	
        use_event_sounds = (gnome_gconf_get_bool ("/desktop/gnome/sound/enable_esd") &&
                            gnome_gconf_get_bool ("/desktop/gnome/sound/event_sounds"));

        if (use_event_sounds) {
                initialize_gtk_signal_relay ();
                initialize_gnome_signal_relay ();
	 }
}

static void
libgnomeui_post_args_parse(GnomeProgram *program, GnomeModuleInfo *mod_info)
{
        GnomeProgramPrivate_libgnomeui *priv;
        gchar *filename;

        gnome_type_init();
        libgnomeui_rc_parse (program);
        priv = g_object_get_qdata(G_OBJECT(program), quark_gnome_program_private_libgnomeui);

        priv->constructed = TRUE;

        /* load the accelerators */
        filename = g_build_filename (gnome_user_accels_dir_get (),
                                     gnome_program_get_app_id (program),
                                     NULL);
        gtk_accel_map_load (filename);
        g_free (filename);

	_gnome_ui_gettext_init (TRUE);

        _gnome_stock_icons_init ();

        setup_event_listener ();
}

static void
libgnomeui_arg_callback(poptContext con,
                        enum poptCallbackReason reason,
                        const struct poptOption * opt,
                        const char * arg, void * data)
{
        GnomeProgram *program = g_dataset_get_data (con, "GnomeProgram");
	g_assert (program != NULL);

        switch(reason) {
        case POPT_CALLBACK_REASON_OPTION:
                switch(opt->val) {
                case ARG_DISABLE_CRASH_DIALOG:
                        g_object_set (G_OBJECT (program),
                                      LIBGNOMEUI_PARAM_CRASH_DIALOG,
                                      FALSE, NULL);
                        break;
                case ARG_DISPLAY:
                        g_object_set (G_OBJECT (program),
                                      LIBGNOMEUI_PARAM_DISPLAY,
                                      arg, NULL);
                        break;
                }
                break;
        default:
                break;
        }
}

static gboolean
libgnomeui_goption_disable_crash_dialog (const gchar *option_name,
					 const gchar *value,
					 gpointer data,
					 GError **error)
{ 
	g_object_set (G_OBJECT (gnome_program_get ()),
		      LIBGNOMEUI_PARAM_CRASH_DIALOG, FALSE, NULL);

	return TRUE;
}

/* automagically parse all the gtkrc files for us.
 * 
 * Parse:
 * $gnomedatadir/gtkrc
 * $gnomedatadir/$apprc
 * ~/.gnome/gtkrc
 * ~/.gnome/$apprc
 *
 * appname is derived from argv[0].  IMHO this is a great solution.
 * It provides good consistancy (you always know the rc file will be
 * the same name as the executable), and it's easy for the programmer.
 * 
 * If you don't like it.. give me a good reason.
 */
static void
libgnomeui_rc_parse (GnomeProgram *program)
{
        gchar *command = g_get_prgname ();
	gint i;
	gint buf_len;
	const gchar *buf = NULL;
	gchar *file;
	gchar *apprc;
	
	buf_len = strlen(command);
	
	for (i = 0; i < buf_len; i++) {
		if (G_IS_DIR_SEPARATOR (command[buf_len - i])) {
			buf = &command[buf_len - i + 1];
			break;
		}
	}
	
	if (!buf)
                buf = command;

        apprc = g_alloca (strlen(buf) + 4);
	sprintf(apprc, "%src", buf);
	
	/* <gnomedatadir>/gtkrc */
        file = gnome_program_locate_file (gnome_program_get (),
                                          GNOME_FILE_DOMAIN_DATADIR,
                                          "gtkrc-2.0", TRUE, NULL);
  	if (file) {
  		gtk_rc_parse (file); 
		g_free (file);
	}

	/* <gnomedatadir>/<progname> */
        file = gnome_program_locate_file (gnome_program_get (),
                                          GNOME_FILE_DOMAIN_DATADIR,
                                          apprc, TRUE, NULL);
	if (file) {
                gtk_rc_parse (file);
                g_free (file);
        }
	
	/* ~/.gnome/gtkrc */
	file = gnome_util_home_file("gtkrc-2.0");
	if (file) {
		gtk_rc_parse (file);
		g_free (file);
	}
	
	/* ~/.gnome/<progname> */
	file = gnome_util_home_file(apprc);
	if (file) {
		gtk_rc_parse (file);
		g_free (file);
	}
}

/* #warning "Solve the sound events situation" */

/* backwards compat */
/**
 * gnome_init_with_popt_table:
 * @app_id: Application id.
 * @app_version: Application version.
 * @argc: argument count (for example argc as received by main)
 * @argv: argument vector (for example argv as received by main)
 * @options: poptOption table with options to parse
 * @flags: popt flags.
 * @return_ctx: if non-NULL, the popt context is returned here.
 *
 * Initializes the application.  This sets up all of the GNOME
 * internals and prepares them (imlib, gdk, session-management, triggers,
 * sound, user preferences).
 *
 * Unlike #gnome_init, with #gnome_init_with_popt_table you can provide
 * a table of popt options (popt is the command line argument parsing
 * library).
 *
 * Deprecated, use #gnome_program_init with the LIBGNOMEUI_MODULE.
 *
 * Returns: 0 (always)
 */
int
gnome_init_with_popt_table (const char *app_id,
			    const char *app_version,
			    int argc, char **argv,
			    const struct poptOption *options,
			    int flags,
			    poptContext *return_ctx)
{
	GnomeProgram *program;
        program = gnome_program_init (app_id, app_version,
				      LIBGNOMEUI_MODULE,
				      argc, argv,
				      GNOME_PARAM_POPT_TABLE, options,
				      GNOME_PARAM_POPT_FLAGS, flags,
				      NULL);

        if(return_ctx) {
                poptContext context;
                g_object_get (program, GNOME_PARAM_POPT_CONTEXT, &context, NULL);
                *return_ctx = context;
        }

        return 0;
}

const GnomeModuleInfo *
gnome_gtk_module_info_get (void)
{
        return bonobo_ui_gtk_module_info_get ();
}
