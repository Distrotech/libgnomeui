/*
 * Copyright (C) 1999, 2000 Red Hat, Inc.
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

#include <config.h>
#include <gobject/genums.h>
#include <gobject/gboxed.h>
#include <gtk/gtk.h>

#include <libgnomecanvas/libgnomecanvas.h>
#include <libgnome/libgnome.h>

static void
add_gtk_arg_callback (poptContext con, enum poptCallbackReason reason,
		      const struct poptOption * opt,
		      const char * arg, void * data);

static void
gtk_pre_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info);

static void
gtk_post_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info);

static struct poptOption gtk_options [] = {
	{ NULL, '\0', POPT_ARG_CALLBACK|POPT_CBFLAG_PRE,
	  &add_gtk_arg_callback, 0, NULL, NULL },

	{ NULL, '\0', POPT_ARG_INTL_DOMAIN, PACKAGE, 0, NULL, NULL },

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

static GnomeModuleRequirement gtk_requirements [] = {
	/* We require libgnome setup to be run first as it
	 * initializes the type system and some other stuff. */
	{ VERSION, &libgnome_module_info } ,
	{ NULL, NULL }
};

GnomeModuleInfo gtk_module_info = {
	"gtk", GTK_VERSION, "GTK+",
	gtk_requirements,
	gtk_pre_args_parse, gtk_post_args_parse, NULL,
	NULL,
	NULL, NULL, NULL
};

static void
libgnomecanvas_pre_args_parse (GnomeProgram *program,
			       GnomeModuleInfo *mod_info);

static GnomeModuleRequirement libgnomecanvas_requirements [] = {
	{ GTK_VERSION, &gtk_module_info },
	{ NULL, NULL }
};

GnomeModuleInfo libgnomecanvas_module_info = {
	"libgnomecanvas", VERSION, "GNOME Canvas",
	libgnomecanvas_requirements,
	libgnomecanvas_pre_args_parse, NULL, NULL,
	NULL,
	NULL, NULL, NULL
};

typedef struct {
	GPtrArray *gtk_args;
} gnome_gtk_init_info;

static void
gtk_pre_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
	struct poptOption *options, *ptr;
	gnome_gtk_init_info *init_info = g_new0 (gnome_gtk_init_info, 1);
	guint count = 1;

	for (ptr = gtk_options;
	     (ptr->argInfo != POPT_ARG_NONE) || (ptr->descrip != NULL);
	     ptr++)
		count++;

	options = g_memdup (gtk_options, sizeof (struct poptOption) * count);
	options->descrip = (const char *) init_info;

	init_info->gtk_args = g_ptr_array_new ();

	mod_info->options = options;
}

static void
gtk_post_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
	gnome_gtk_init_info *init_info;
	int final_argc;
	char **final_argv;

	g_message (G_STRLOC);

	init_info = (gnome_gtk_init_info *) mod_info->options [0].descrip;

	g_ptr_array_add (init_info->gtk_args, NULL);

	final_argc = init_info->gtk_args->len - 1;
	final_argv = g_memdup (init_info->gtk_args->pdata,
			       sizeof (char *) * init_info->gtk_args->len);

	gtk_init (&final_argc, &final_argv);

	gdk_rgb_init();

	g_free (mod_info->options);
	mod_info->options = NULL;

	g_ptr_array_free (init_info->gtk_args, TRUE);
	g_free (init_info);
}

static void
add_gtk_arg_callback (poptContext con, enum poptCallbackReason reason,
		      const struct poptOption * opt,
		      const char * arg, void * data)
{
	gnome_gtk_init_info *init_info = data;
	char *newstr;

	g_message (G_STRLOC ": %p", data);

	switch (reason) {
	case POPT_CALLBACK_REASON_PRE:
		/* Note that the value of argv[0] passed to main() may be
		 * different from the value that this passes to gtk
		 */
		g_ptr_array_add (init_info->gtk_args,
				 (char *) g_strdup (poptGetInvocationName (con)));
		break;
		
	case POPT_CALLBACK_REASON_OPTION:
		switch (opt->argInfo) {
		case POPT_ARG_STRING:
		case POPT_ARG_INT:
		case POPT_ARG_LONG:
			newstr = g_strconcat ("--", opt->longName, "=", arg, NULL);
			break;
		default:
			newstr = g_strconcat ("--", opt->longName, NULL);
			break;
		}

		g_ptr_array_add (init_info->gtk_args, newstr);
		/* XXX gnome-client tie-in */
		break;
	default:
		break;
	}
}

static void
libgnomecanvas_pre_args_parse (GnomeProgram *program,
			       GnomeModuleInfo *mod_info)
{
	libgnomecanvas_init ();
}
