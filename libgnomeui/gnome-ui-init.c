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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <libgnome/libgnome.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include "gnome-i18nP.h"
#include "gnome-client.h"
#include "gnome-preferences.h"
#include "gnome-init.h"
#include "gnome-winhints.h"
#include "gnome-gconf.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gnome-pixmap.h"

#include <libgnomeuiP.h>

const char libgnomeui_param_crash_dialog[]="B:libgnomeui/show_crash_dialog";
const char libgnomeui_param_display[]="S:libgnomeui/display";
const char libgnomeui_param_default_icon[]="S:libgnomeui/default_icon";

/******************* libgnomeui module ***********************/
static void libgnomeui_arg_callback(poptContext con,
                                    enum poptCallbackReason reason,
                                    const struct poptOption * opt,
                                    const char * arg, void * data);
static void libgnomeui_pre_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info);
static void libgnomeui_post_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info);
static void libgnomeui_rc_parse (gchar *command);
static GdkPixmap *libgnomeui_pixbuf_image_loader(GdkWindow   *window,
                                                 GdkColormap *colormap,
                                                 GdkBitmap  **mask,
                                                 GdkColor    *transparent_color,
                                                 const char *filename);
static void libgnomeui_segv_setup(gboolean post_arg_parse);

static GnomeModuleRequirement libgnomeui_requirements[] = {
        {VERSION, &libgnome_module_info},
        {VERSION, &libgnomecanvas_module_info},
        {NULL, NULL}
};

enum { ARG_DISABLE_CRASH_DIALOG=1, ARG_DISPLAY };

static struct poptOption libgnomeui_options[] = {
        {NULL, '\0', POPT_ARG_INTL_DOMAIN, PACKAGE, 0, NULL, NULL},
	{NULL, '\0', POPT_ARG_CALLBACK|POPT_CBFLAG_PRE|POPT_CBFLAG_POST,
	 &libgnomeui_arg_callback, 0, NULL, NULL},
	{"disable-crash-dialog", '\0', POPT_ARG_NONE, NULL, ARG_DISABLE_CRASH_DIALOG},
        {"display", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, NULL, ARG_DISPLAY,
         N_("X display to use"), N_("DISPLAY")},
	{NULL, '\0', 0, NULL, 0}
};

GnomeModuleInfo libgnomeui_module_info = {
        "libgnomeui", VERSION, "GNOME GUI Library",
        libgnomeui_requirements,
        libgnomeui_pre_args_parse, libgnomeui_post_args_parse,
        libgnomeui_options
};

static void
libgnomeui_pre_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info)
{
        gboolean ctype_set;
        char *ctype, *old_ctype = NULL;
        gboolean do_crash_dialog = TRUE;
        const char *envar;

        envar = g_getenv("GNOME_DISABLE_CRASH_DIALOG");
        if(envar)
                do_crash_dialog = atoi(envar)?FALSE:TRUE;
        g_object_set (G_OBJECT(app), LIBGNOMEUI_PARAM_CRASH_DIALOG,
                      do_crash_dialog, LIBGNOMEUI_PARAM_DISPLAY,
                      g_getenv("DISPLAY"), NULL);

        if(do_crash_dialog)
                libgnomeui_segv_setup(FALSE);

        /* Begin hack to propogate an en_US locale into Gtk+ if LC_CTYPE=C, so that non-ASCII
           characters will display for as many people as possible. Related to bug #1979 */
        ctype = setlocale (LC_CTYPE, NULL);

        if (!strcmp(ctype, "C")) {
                old_ctype = g_strdup (g_getenv ("LC_CTYPE"));
                putenv ("LC_CTYPE=en_US");
                ctype_set = TRUE;
        } else
                ctype_set = FALSE;

        gtk_set_locale ();

        if (ctype_set) {
                char *setme;

                if (old_ctype) {
                        setme = g_strconcat ("LC_CTYPE=", old_ctype, NULL);
                        g_free(old_ctype);
                } else
                        setme = "LC_CTYPE";

                putenv (setme);
        }
        /* End hack */
}

static void
libgnomeui_post_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info)
{
        gnome_type_init();
        gtk_rc_set_image_loader(libgnomeui_pixbuf_image_loader);
        libgnomeui_rc_parse(program_invocation_name);
        gnome_preferences_load();

        libgnomeui_segv_setup(TRUE);
}

static void
libgnomeui_arg_callback(poptContext con,
                        enum poptCallbackReason reason,
                        const struct poptOption * opt,
                        const char * arg, void * data)
{
        GnomeProgram *program = gnome_program_get ();

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
 * If you don't like it.. give me a good reason.  Symlin
 */
static void
libgnomeui_rc_parse (gchar *command)
{
	gint i;
	gint buf_len;
	gchar *buf = NULL;
	gchar *file;
	gchar *apprc;
	
	buf_len = strlen(command);
	
	for (i = 0; i < buf_len; i++) {
		if (command[buf_len - i] == '/') {
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
                                          "gtkrc", TRUE, NULL);
  	if (file) {
  		gtk_rc_add_default_file (file); 
		g_free (file);
	}

	/* <gnomedatadir>/<progname> */
        file = gnome_program_locate_file (gnome_program_get (),
                                          GNOME_FILE_DOMAIN_DATADIR,
                                          apprc, TRUE, NULL);
	if (file) {
                gtk_rc_add_default_file (file);
                g_free (file);
        }
	
	/* ~/.gnome/gtkrc */
	file = gnome_util_home_file("gtkrc");
	if (file) {
		gtk_rc_add_default_file (file);
		g_free (file);
	}
	
	/* ~/.gnome/<progname> */
	file = gnome_util_home_file(apprc);
	if (file) {
		gtk_rc_add_default_file (file);
		g_free (file);
	}

	gtk_rc_init ();
}

static GdkPixmap *
libgnomeui_pixbuf_image_loader(GdkWindow   *window,
                               GdkColormap *colormap,
                               GdkBitmap  **maskp,
                               GdkColor    *transparent_color,
                               const char *filename)
{
	GdkPixmap *retval = NULL;
        GdkBitmap *mask = NULL;
        GdkPixbuf *pixbuf;
        GError *error;

        /* FIXME we are ignoring colormap and transparent color */
        
        error = NULL;
        pixbuf = gdk_pixbuf_new_from_file(filename, &error);
        if (error != NULL) {
                g_warning (G_STRLOC ": cannot load %s: %s", filename,
                           error->message);
                g_error_free (error);
        }

        if (pixbuf == NULL)
                return NULL;

        gdk_pixbuf_render_pixmap_and_mask(pixbuf, &retval, &mask, 128);

        gdk_pixbuf_unref(pixbuf);
        
        if (maskp)
                *maskp = mask;
        else
                gdk_bitmap_unref(mask);

        return retval;
}

/* crash handler */
static void libgnomeui_segv_handle(int signum);

static void
libgnomeui_segv_setup(gboolean post_arg_parse)
{
        static struct sigaction *setptr;
        struct sigaction sa;
        gboolean do_crash_dialog = TRUE;
        GValue value = { 0, };

        g_value_init (&value, G_TYPE_BOOLEAN);
        g_object_get_property (G_OBJECT (gnome_program_get()),
                               LIBGNOMEUI_PARAM_CRASH_DIALOG, &value);
        do_crash_dialog = g_value_get_boolean (&value);
        g_value_unset (&value);

        memset(&sa, 0, sizeof(sa));

        setptr = &sa;
        if(do_crash_dialog) {
                sa.sa_handler = (gpointer)libgnomeui_segv_handle;
        } else {
                sa.sa_handler = SIG_DFL;
        }

        sigaction(SIGSEGV, setptr, NULL);
        sigaction(SIGFPE, setptr, NULL);
        sigaction(SIGBUS, setptr, NULL);
}

static void libgnomeui_segv_handle(int signum)
{
	static int in_segv = 0;
	pid_t pid;
	
	in_segv++;

        if (in_segv > 2) {
                /* The fprintf() was segfaulting, we are just totally hosed */
                _exit(1);
        } else if (in_segv > 1) {
                /* dialog display isn't working out */
                fprintf(stderr, _("Multiple segmentation faults occurred; can't display error dialog\n"));
                _exit(1);
        }

        /* Make sure we release grabs */
        gdk_pointer_ungrab(GDK_CURRENT_TIME);
        gdk_keyboard_ungrab(GDK_CURRENT_TIME);

        gdk_flush();
        
	if ((pid = fork())) {
                /* Wait for user to see the dialog, then exit. */
                /* Why wait at all? Because we want to allow people to attach to the
		   process */
		int estatus;
		pid_t eret;

		eret = waitpid(pid, &estatus, 0);

                /* Don't use app attributes here - a lot of things are probably hosed */
		if(g_getenv("GNOME_DUMP_CORE"))
	                abort ();

		_exit(1);
	} else {
                GnomeProgram *program;
		char buf[32];

		g_snprintf(buf, sizeof(buf), "%d", signum);

                program = gnome_program_get();

		/* Child process */
		execl(GNOMEUIBINDIR "/gnome_segv", GNOMEUIBINDIR "/gnome_segv",
		      gnome_program_get_name(program), buf, gnome_program_get_version(program), NULL);

                execlp("gnome_segv", "gnome_segv", gnome_program_get_name(program), buf,
                       gnome_program_get_version(program), NULL);

                _exit(99);
	}

	in_segv--;
}

/* #warning "Solve the sound events situation" */

/* backwards compat */
int gnome_init_with_popt_table(const char *app_id,
			       const char *app_version,
			       int argc, char **argv,
			       const struct poptOption *options,
			       int flags,
			       poptContext *return_ctx)
{
        gnome_program_init(app_id, app_version, argc, argv, LIBGNOMEUI_INIT,
                           GNOME_PARAM_POPT_TABLE, options,
                           GNOME_PARAM_POPT_FLAGS, flags,
                           NULL);

        if(return_ctx) {
                GValue value = { 0, };

                g_value_init (&value, G_TYPE_POINTER);
                g_object_get_property (G_OBJECT (gnome_program_get()),
                                       GNOME_PARAM_POPT_CONTEXT, &value);
                *return_ctx = g_value_peek_pointer (&value);
                g_value_unset (&value);
        }

        return 0;
}


