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

#include <gtk/gtk.h>

#include <libgnome/gnome-util.h>
#include <libgnome/gnome-program.h>
#include <libgnome/gnomelib-init.h>

#include "gnome-i18nP.h"
#include "gnome-client.h"
#include "gnome-preferences.h"
#include "gnome-init.h"
#include "gnome-winhints.h"
#include "gnome-gconf.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gnome-pixmap.h"

#include "libgnomeuiP.h"

const char libgnomeui_param_crash_dialog[]="B:libgnomeui/show_crash_dialog";
const char libgnomeui_param_display[]="S:libgnomeui/display";
const char libgnomeui_param_default_icon[]="S:libgnomeui/default_icon";

/******************** gtk proxy module *******************/

static void
gnome_add_gtk_arg_callback(poptContext con,
			   enum poptCallbackReason reason,
			   const struct poptOption * opt,
			   const char * arg, void * data);
static void gtk_pre_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info);
static void gtk_post_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info);

static struct poptOption gtk_options [] = {
        { NULL, '\0', POPT_ARG_INTL_DOMAIN, PACKAGE, 0, NULL, NULL},

	{ NULL, '\0', POPT_ARG_CALLBACK|POPT_CBFLAG_PRE,
	  &gnome_add_gtk_arg_callback, 0, NULL},

	{ "gdk-debug", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Gdk debugging flags to set"), N_("FLAGS")},

	{ "gdk-no-debug", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Gdk debugging flags to unset"), N_("FLAGS")},

	{ "display", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("X display to use"), N_("DISPLAY")},

	{ "sync", '\0', POPT_ARG_NONE, NULL, 0,
	  N_("Make X calls synchronous"), NULL},

	{ "no-xshm", '\0', POPT_ARG_NONE, NULL, 0,
	  N_("Don't use X shared memory extension"), NULL},

	{ "name", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Program name as used by the window manager"), N_("NAME")},

	{ "class", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Program class as used by the window manager"), N_("CLASS")},

	{ "gxid_host", '\0', POPT_ARG_STRING, NULL, 0,
	  NULL, N_("HOST")},

	{ "gxid_port", '\0', POPT_ARG_STRING, NULL, 0,
	  NULL, N_("PORT")},

	{ "xim-preedit", '\0', POPT_ARG_STRING, NULL, 0,
	  NULL, N_("STYLE")},

	{ "xim-status", '\0', POPT_ARG_STRING, NULL, 0,
	  NULL, N_("STYLE")},

	{ "gtk-debug", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Gtk+ debugging flags to set"), N_("FLAGS")},

	{ "gtk-no-debug", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Gtk+ debugging flags to unset"), N_("FLAGS")},

	{ "g-fatal-warnings", '\0', POPT_ARG_NONE, NULL, 0,
	  N_("Make all warnings fatal"), NULL},

	{ "gtk-module", '\0', POPT_ARG_STRING, NULL, 0,
	  N_("Load an additional Gtk module"), N_("MODULE")},

	{ NULL, '\0', 0, NULL, 0}
};

static GnomeModuleRequirement gtk_requirements[] = {
	/* We require libgnome setup to be run first as it
	 * sets some strings for us, and inits user
	 * directories */
        {VERSION, &libgnome_module_info},
        {NULL, NULL}
};

GnomeModuleInfo gtk_module_info = {
        "gtk", "1.2.5" /* aargh broken! */, "GTK",
        gtk_requirements,
        gtk_pre_args_parse, gtk_post_args_parse,
        gtk_options
};

struct {
        GPtrArray *gtk_args;
} gnome_gtk_init_info = {0};

static void
gtk_pre_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info)
{
#if 0
	if (g_getenv ("GTK_DEBUG_OBJECTS"))
		gtk_debug_flags |= GTK_DEBUG_OBJECTS;
#endif
		
        gnome_gtk_init_info.gtk_args = g_ptr_array_new();
}

static void
gtk_post_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info)
{
        int i;
        int final_argc;
        char **final_argv;
        char *accels_rc_filename;

        g_ptr_array_add(gnome_gtk_init_info.gtk_args, NULL);

        final_argc = gnome_gtk_init_info.gtk_args->len - 1;
        final_argv = g_memdup(gnome_gtk_init_info.gtk_args->pdata, sizeof(char *) * gnome_gtk_init_info.gtk_args->len);
        gtk_init(&final_argc, &final_argv);

        gdk_rgb_init();
        
        accels_rc_filename = g_concat_dir_and_file (gnome_user_accels_dir, gnome_program_get_name(gnome_program_get()));
        gtk_item_factory_parse_rc(accels_rc_filename);
        g_free(accels_rc_filename);

        g_free(final_argv);
        for(i = 0; i < gnome_gtk_init_info.gtk_args->len; i++)
                g_free(g_ptr_array_index(gnome_gtk_init_info.gtk_args, i));
        g_ptr_array_free(gnome_gtk_init_info.gtk_args, TRUE); gnome_gtk_init_info.gtk_args = NULL;
}

static void
gnome_add_gtk_arg_callback(poptContext con,
			   enum poptCallbackReason reason,
			   const struct poptOption * opt,
			   const char * arg, void * data)
{
        char *newstr;

	switch(reason) {
	case POPT_CALLBACK_REASON_PRE:
		/* Note that the value of argv[0] passed to main() may be
		 * different from the value that this passes to gtk
		 */
		g_ptr_array_add(gnome_gtk_init_info.gtk_args,
				(char *)g_strdup(poptGetInvocationName(con)));
		break;
		
	case POPT_CALLBACK_REASON_OPTION:
	        switch(opt->argInfo) {
		case POPT_ARG_STRING:
		case POPT_ARG_INT:
		case POPT_ARG_LONG:
		  newstr = g_strconcat("--", opt->longName, "=", arg, NULL);
		  break;
		default:
		  newstr = g_strconcat("--", opt->longName, NULL);
		  break;
		}

		g_ptr_array_add(gnome_gtk_init_info.gtk_args, newstr);
                /* XXX gnome-client tie-in */
		break;
        default:
                break;
	}
}

/******************* libgnomeui module ***********************/
static void libgnomeui_arg_callback(poptContext con,
                                    enum poptCallbackReason reason,
                                    const struct poptOption * opt,
                                    const char * arg, void * data);
static void libgnomeui_pre_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info);
static void libgnomeui_post_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info);
static void libgnomeui_rc_parse (gchar *command);
static GdkPixmap *libgnomeui_pixbuf_image_loader(GdkWindow   *window,
                                                 GdkColormap *colormap,
                                                 GdkBitmap  **mask,
                                                 GdkColor    *transparent_color,
                                                 const char *filename);
static void libgnomeui_segv_setup(gboolean post_arg_parse);

static GnomeModuleRequirement libgnomeui_requirements[] = {
        {VERSION, &libgnome_module_info},
        {"1.2.5", &gtk_module_info},
        {VERSION, &gnome_client_module_info},
        {VERSION, &gnome_gconf_module_info},
        {NULL, NULL}
};

enum { ARG_DISABLE_CRASH_DIALOG=1, ARG_DISPLAY };

static struct poptOption libgnomeui_options[] = {
        { NULL, '\0', POPT_ARG_INTL_DOMAIN, PACKAGE, 0, NULL, NULL},
	{NULL, '\0', POPT_ARG_CALLBACK|POPT_CBFLAG_PRE|POPT_CBFLAG_POST,
	 &libgnomeui_arg_callback, 0, NULL, NULL},
	{"disable-crash-dialog", '\0', POPT_ARG_NONE, NULL, ARG_DISABLE_CRASH_DIALOG},
        {"display", '\0', POPT_ARG_STRING|POPT_ARGFLAG_DOC_HIDDEN, NULL, ARG_DISPLAY, N_("X display to use"), N_("DISPLAY")},
	{NULL, '\0', 0, NULL, 0}
};

GnomeModuleInfo libgnomeui_module_info = {
        "libgnomeui", VERSION, "GNOME GUI Library",
        libgnomeui_requirements,
        libgnomeui_pre_args_parse, libgnomeui_post_args_parse,
        libgnomeui_options
};

static void
libgnomeui_pre_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info)
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
libgnomeui_post_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info)
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
		char buf[32];

		g_snprintf(buf, sizeof(buf), "%d", signum);

		/* Child process */
		execl(GNOMEUIBINDIR "/gnome_segv", GNOMEUIBINDIR "/gnome_segv",
		      program_invocation_name, buf, gnome_app_version, NULL);

                execlp("gnome_segv", "gnome_segv", program_invocation_name, buf, gnome_app_version, NULL);

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
        gnome_program_init (app_id, app_version, argc, argv, LIBGNOMEUI_INIT,
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


/****************************************************************************************/

#ifdef BUILD_UNUSED_LEGACY_CODE

static void initialize_gtk_signal_relay(void);
static gboolean
relay_gtk_signal(GtkObject *object,
		 guint signal_id,
		 guint n_params,
		 GtkArg *params,
		 gchar *signame);

static void gnome_rc_parse(gchar *command);

static GdkPixmap *imlib_image_loader(GdkWindow   *window,
				     GdkColormap *colormap,
				     GdkBitmap  **mask,
				     GdkColor    *transparent_color,
				     const char *filename);

/* This isn't conditionally compiled based on USE_SEGV_HANDLE
   because the --disable-crash-dialog option is always accepted,
   even if it has no effect
*/
gboolean disable_crash_dialog = FALSE;

/* The master client.  */
static GnomeClient *client = NULL;


static void
gnome_init_cb(poptContext ctx, enum poptCallbackReason reason,
	      const struct poptOption *opt)
{
	if(gnome_initialized)
		return;
	
	switch(reason) {
	case POPT_CALLBACK_REASON_PRE:
                {
                        char *ctype, *old_ctype = NULL;
                        gboolean ctype_set;

                        gnome_segv_setup (FALSE);
                        client = gnome_master_client();
                }
		break;
	case POPT_CALLBACK_REASON_POST:
		gdk_imlib_init();
		gnome_type_init();
		gtk_rc_set_image_loader(imlib_image_loader);
		gnome_rc_parse(program_invocation_name);
		gnome_preferences_load();
		/*
		 * No, this is not the most evil thing you have
		 * seen raster.  Imlib is the most evil thing
	   	 * we have seen.  And until the cache in imlib
	 	 * is smarter, this stays.
		 *
		 * Last I checked, there was no imlib-capplet
		 */
		if (gnome_preferences_get_disable_imlib_cache ()){
			int pixmaps, images;
			
			/*
			 * If cache info has been set to -1, -1, it
			 * means something initialized before us and
			 * is requesting the cache to not be touched
			 */
			gdk_imlib_get_cache_info (&pixmaps, &images);

			if (!(pixmaps == -1 || images == -1))
				gdk_imlib_set_cache_info (0, images);
		}

                gnome_segv_setup(TRUE);
		
		/* lame trigger stuff. This really needs to be sorted
		   out better so that the trigger API is exported more
		   - perhaps there's something hidden in glib that
		   already does this :) */

		initialize_gtk_signal_relay();
		
		gnome_initialized = TRUE;
		break;
	case POPT_CALLBACK_REASON_OPTION:
                if (opt->val == -2) {
                        disable_crash_dialog = TRUE;
                }
	default:
		break;
	}
}

static const struct poptOption gnome_options[] = {
};

static void
gnome_register_options(void)
{
	gnomelib_register_popt_table(gtk_options, N_("GTK options"));
	gnomelib_register_popt_table(gnome_options, N_("GNOME GUI options"));
}

/* #define ALLOC_DEBUGGING_HOOKS */
#ifdef ALLOC_DEBUGGING_HOOKS
#include <malloc.h>
#include <sys/time.h>

static struct mallinfo mi1, mi2;

#define AM() mi1 = mallinfo();
#define PM(x) mi2 = mallinfo(); printf(x ": used %d, now %d\n", \
mi2.uordblks - mi1.uordblks, mi2.uordblks);

typedef struct {
  gulong magic;
  gulong bsize;
  struct timeval tv;
} BlkInfo;

static void (*old_free_hook)(__malloc_ptr_t __ptr);
static __malloc_ptr_t (*old_malloc_hook) (size_t __size);
static __malloc_ptr_t (*old_realloc_hook) (__malloc_ptr_t __ptr,
					   size_t __size);

static void my_free_hook(__malloc_ptr_t ptr)
{
  char buf[512];
  BlkInfo *tv;
  struct timeval now;

  tv = (BlkInfo *)(((char *)ptr) - sizeof(BlkInfo));

  __free_hook = old_free_hook;
  if(tv->magic != 0xadaedead) {
    goto out;
  }

  gettimeofday(&now, NULL);
  sprintf(buf, "-%p %ld %ld\n", ptr, tv->bsize,
	  (now.tv_sec*1000000 + now.tv_usec)
	  - (tv->tv.tv_sec*1000000 + tv->tv.tv_usec));
  write(1, buf, strlen(buf));

  free(tv);
 out:
  __free_hook = my_free_hook;
}

static __malloc_ptr_t my_malloc_hook(size_t bsize)
{
  BlkInfo *newblk;
  __malloc_ptr_t retval;
  char buf[512];

  if(!bsize) return NULL;

  __malloc_hook = old_malloc_hook;

  newblk = malloc(bsize+sizeof(BlkInfo));

  if(newblk) {
    newblk->magic = 0xadaedead;
    newblk->bsize = bsize;
    gettimeofday(&newblk->tv, NULL);
  }


  retval = newblk;
  retval += sizeof(BlkInfo);

  sprintf(buf, "+%p %ld\n", retval, bsize);
  write(1, buf, strlen(buf));

  __malloc_hook = my_malloc_hook;
  return retval;
}

static __malloc_ptr_t my_realloc_hook(__malloc_ptr_t __ptr,
				      size_t __size)
{
  BlkInfo *oldblk, *newblk;
  __malloc_ptr_t rv;

  __realloc_hook = old_realloc_hook;
  oldblk = (((char *)__ptr) - sizeof(BlkInfo));

  if(__ptr && oldblk->magic == 0xadaedead) {
    newblk = realloc(oldblk, __size + sizeof(BlkInfo));
    rv = newblk + 1;
    newblk->bsize = __size;
  } else
    rv = realloc(__ptr, __size);

  __realloc_hook = my_realloc_hook;

  return rv;
}
#endif

/**
 * gnome_init_with_popt_table:
 * @app_id: Application id.
 * @app_version: Application version.
 * @argc: pointer to argc (for example, as received by main)
 * @argv: pointer to argc (for example, as received by main)
 * @options: poptOption table with options to parse
 * @flags: popt flags.
 * @return_ctx: if non-NULL, the popt context is returned here.
 *
 * Initializes the application.  This sets up all of the GNOME
 * internals and prepares them (imlib, gdk, session-management, triggers,
 * sound, user preferences)
 *
 * Unlike gnome_init, with gnome_init_with_popt_table you can provide
 * a table of popt options (popt is the command line argument parsing
 * library).
 */

int
gnome_init_with_popt_table(const char *app_id,
			   const char *app_version,
			   int argc, char **argv,
			   const struct poptOption *options,
			   int flags,
			   poptContext *return_ctx)
{
	poptContext ctx;
	char *appdesc;

#ifdef ALLOC_DEBUGGING_HOOKS
	appdesc = malloc(10);
	appdesc = realloc(appdesc, 20);
	free(appdesc);
	
	old_free_hook = __free_hook;
	old_malloc_hook = __malloc_hook;
	old_realloc_hook = __realloc_hook;
	__free_hook = my_free_hook;
	__malloc_hook = my_malloc_hook;
	__realloc_hook = my_realloc_hook;
#endif
	g_return_val_if_fail(gnome_initialized == FALSE, -1);
	
	gnomelib_init (app_id, app_version);
	
	gnome_register_options();

	gnome_client_init();
	
	if(options) {
		appdesc = g_strdup_printf(_("%s options"), app_id);
		gnomelib_register_popt_table(options, appdesc);
	}
	
	ctx = gnomelib_parse_args(argc, argv, flags);
	
	if(return_ctx)
		*return_ctx = ctx;
	else
		poptFreeContext(ctx);

	return 0;
}

/**
 * gnome_init:
 * @app_id: Application id.
 * @app_version: Application version.
 * @argc: argc (for example, as received by main)
 * @argv: argv (for example, as received by main)
 *
 * Initializes the application.  This sets up all of the GNOME
 * internals and prepares them (imlib, gdk, session-management, triggers,
 * sound, user preferences)
 */
int
gnome_init(const char *app_id,
	   const char *app_version,
	   int argc, char **argv)
{
  g_return_val_if_fail(gnome_initialized == FALSE, -1);

  gnome_init_with_popt_table(app_id, app_version,
			     argc, argv, NULL, 0, NULL);

  return 0;
}

/* perhaps this belongs in libgnome.. move it if you like. */


static gboolean
relay_gtk_signal(GtkObject *object,
		 guint signal_id,
		 guint n_params,
		 GtkArg *params,
		 gchar *signame)
{
#ifdef HAVE_ESD
  char *pieces[3] = {"gtk-events", NULL, NULL};
  static GQuark disable_sound_quark = 0;

  pieces[1] = signame;

  if(!disable_sound_quark)
    disable_sound_quark = g_quark_from_static_string("gnome_disable_sound_events");

  if(gtk_object_get_data_by_id(object, disable_sound_quark))
    return TRUE;

  if(GTK_IS_WIDGET(object)) {
    if(!GTK_WIDGET_DRAWABLE(object))
      return TRUE;

    if(GTK_IS_MENU_ITEM(object) && GTK_MENU_ITEM(object)->submenu)
      return TRUE;
  }


  gnome_triggers_vdo("", NULL, (const char **)pieces);

  return TRUE;
#else
  return FALSE; /* this shouldn't even happen... */
#endif
}

static void
initialize_gtk_signal_relay (void)
{
#ifdef HAVE_ESD
	gpointer iter_signames;
	char *signame;
	char *ctmp, *ctmp2;
	
	if (gnome_sound_connection < 0)
		return;
	
	if (!gnome_config_get_bool ("/sound/system/settings/event_sounds=true"))
		return;
	
	ctmp = gnome_config_file ("/sound/events/gtk-events.soundlist");
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
		if(!strcmp (signame, "activate")){
			gtk_type_class (gtk_menu_item_get_type ());
			signums [0] = gtk_signal_lookup (signame, gtk_menu_item_get_type ());
			
			gtk_type_class (gtk_editable_get_type ());
			signums [1] = gtk_signal_lookup (signame, gtk_editable_get_type ());
			nsigs = 2;
		} else if(!strcmp(signame, "toggled")){
			gtk_type_class (gtk_toggle_button_get_type ());
			signums [0] = gtk_signal_lookup (signame,
							 gtk_toggle_button_get_type ());
			
			gtk_type_class (gtk_check_menu_item_get_type ());
			signums [1] = gtk_signal_lookup (signame,
							 gtk_check_menu_item_get_type ());
			nsigs = 2;
		} else if (!strcmp (signame, "clicked")){
			gtk_type_class (gtk_button_get_type ());
			signums [0] = gtk_signal_lookup (signame, gtk_button_get_type ());
			nsigs = 1;
		} else {
			gtk_type_class (gtk_widget_get_type ());
			signums [0] = gtk_signal_lookup (signame, gtk_widget_get_type ());
			nsigs = 1;
		}

		used_signame = FALSE;
		for(i = 0; i < nsigs; i++) {
			if (signums [i] > 0) {
				gtk_signal_add_emission_hook (signums [i],
							      (GtkEmissionHook)relay_gtk_signal,
							      signame);
                                used_signame = TRUE;
                        }
                }
                if(!used_signame)
                        g_free(signame);
	}
	gnome_config_pop_prefix ();
	
#endif
}

#endif /* BUILD_UNUSED_LEGACY_CODE */
