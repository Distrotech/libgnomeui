#include <config.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk_imlib.h>
#include "libgnome/libgnome.h"
#include <argp.h>

extern char *program_invocation_name;
extern char *program_invocation_short_name;

static void gnome_rc_parse(gchar *command);



/* This describes all the arguments understood by Gtk.  We parse them
   out and put them into our own private argv, which we then feed to
   Gtk once the command line has been successfully parsed.  Yes, this
   is a total hack.  FIXME: should hidden options really be visible?

   Note that the key numbers must start at -1 and decrease by 1.  The
   key number is used to look up the argument name we supply to Gtk.  */
static struct argp_option our_gtk_options[] =
{
	{ "gdk-debug",    -1, NULL,          OPTION_HIDDEN,    NULL, -2 },
	{ "gdk-no-debug", -2, NULL,          OPTION_HIDDEN, NULL, 0 },
	{ "display",      -3, N_("DISPLAY"), 0,	  N_("X display to use"), 0 },
	{ "sync",         -4, NULL,          OPTION_HIDDEN, 	  NULL, 0 },
	{ "no-xshm",      -5, NULL,          0,	  N_("Don't use X shared memory extension"), 0 },
	{ "name",         -6, N_("NAME"),    0,	  N_("FIXME"), 0 },
	{ "class",        -7, N_("CLASS"),   0,	  N_("FIXME"), 0 },
	{ "gxid_host",    -8, N_("HOST"),    0,	  N_("FIXME"), 0 },
	{ "gxid_port",    -9, N_("PORT"),    0,	  N_("FIXME"), 0 },
	{ "xim-preedit", -10, N_("STYLE"),   0,	  N_("FIXME"), 0 },
	{ "xim-status",  -11, N_("STYLE"),   0,	  N_("FIXME"), 0 },
	{ NULL, 0, NULL, 0, NULL, 0 }
};

/* Index of next free slot in the argument vector we construct for
   Gtk.  */
static int our_argc;

/* Argument vector we construct.  */
static char **our_argv;

/* Called during argument parsing to handle various details.  */
static error_t
our_gtk_parse_func (int key, char *arg, struct argp_state *state)
{
	if (key < 0)
	{
		/* This is some argument we defined.  We handle it by pushing
		   the flag, and possibly the argument, onto our saved argument
		   vector.  Later this is passed to gtk_init.  */
		our_argv[our_argc++] = g_strconcat ("--",
						    our_gtk_options[- key - 1].name,
						    NULL);
		if (arg)
			our_argv[our_argc++] = g_strdup (arg);
	}
	else if (key == ARGP_KEY_INIT)
	{
		/* We use twice the amount of space you'd think we need,
		   because the user might write all options as
		   `--foo=bar', but we always split into two arguments,
		   like `-foo bar'.  */
		our_argv = (char **) malloc (2 * (state->argc + 1) * sizeof (char *));
		our_argc = 0;
		our_argv[our_argc++] = g_strdup (state->argv[0]);
	}
	else if (key == ARGP_KEY_SUCCESS)
	{
		int i, copy_ac = our_argc;
		char **copy = (char **) g_malloc (our_argc * sizeof (char *));

		memcpy (copy, our_argv, our_argc * sizeof (char *));
		our_argv[our_argc] = NULL;

		gtk_init (&our_argc, &our_argv);
		gdk_imlib_init ();
		gnome_rc_parse (program_invocation_name);

		for (i = 0; i < copy_ac; ++i)
			g_free (copy[i]);
		g_free (copy);
		g_free (our_argv);
	}
	else
		return ARGP_ERR_UNKNOWN;

	return 0;
}

static struct argp our_gtk_parser =
{
	our_gtk_options,		/* Options.  */
	our_gtk_parse_func,		/* The parser function.  */
	NULL,				/* Description of other args.  */
	NULL,				/* Extra text for long help.  */
	NULL,				/* Child arguments.  */
	NULL,				/* Help filter.  */
	PACKAGE			/* Translation domain.  */
};

void
gnomeui_register_arguments (void)
{
	gnome_parse_register_arguments (&our_gtk_parser);
}



/* The default version hook tells everybody that this is a free Gnome
   program.  You must override if that is incorrect.  */
static void
default_version_func (FILE *stream, struct argp_state *state)
{
	fprintf (stream, "Gnome %s %s\n", gnome_app_id,
		 argp_program_version ? argp_program_version : "");
	/* FIXME: define a way to set copyright date so we can print it
	   here?  */
}

error_t
gnome_init (char *app_id, struct argp *app_args,
	    int argc, char **argv,
	    unsigned int flags, int *arg_index)
{
	/* now we replace gtk_init() with gnome_init() in our apps */
	gtk_set_locale();

	gnomelib_register_arguments ();
	gnomeui_register_arguments ();

	/* On non-glibc systems, this is not set up for us.  */
	if (!program_invocation_name) {
		char *arg;
	  
		program_invocation_name = argv[0];
		arg = strrchr (argv[0], '/');
		program_invocation_short_name =
		  arg ? (arg + 1) : program_invocation_name;
	}
	
	gnomelib_init (app_id);

	if (! argp_program_version_hook)
		argp_program_version_hook = default_version_func;

	/* Now parse command-line arguments.  */
	return gnome_parse_arguments (app_args, argc, argv, flags, arg_index);
}

/* perhaps this belongs in libgnome.. move it if you like. */

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
gnome_rc_parse (gchar *command)
{
	gint i;
	gint buf_len;
	gint found = 0;
	gchar *buf = NULL;
	gchar *file;
	gchar *apprc;
	
	buf_len = strlen(command);
	
	for (i = 0; i < buf_len; i++) {
		if (command[buf_len - i] == '/') {
			buf = g_strdup (&command[buf_len - i + 1]);
			found = TRUE;
			break;
		}
	}
	
	if (!found)
		buf = g_strdup (command);
	
	apprc = g_malloc (strlen(buf) + 3);
	sprintf(apprc, "%src", buf);
	
	g_free(buf);
	
	
	/* <gnomedatadir>/gtkrc */
	file = gnome_datadir_file("gtkrc");
	if (file){
		gtk_rc_parse (file);
		g_free (file);
	}

	/* <gnomedatadir>/<progname> */
	file = gnome_datadir_file(apprc);
	if (file){
		gtk_rc_parse (file);
		g_free (file);
	}
	
	/* ~/.gnome/gtkrc */
	file = gnome_util_home_file("gtkrc");
	if (file){
		gtk_rc_parse (file);
		g_free (file);
	}
	
	/* ~/.gnome/<progname> */
	file = gnome_util_home_file(apprc);
	if (file){
		gtk_rc_parse (file);
		g_free (file);
	}
	
	g_free (apprc);
}
