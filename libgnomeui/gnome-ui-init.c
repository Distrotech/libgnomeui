#include <config.h>

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <argp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <gtk/gtk.h>
#include <gdk_imlib.h>

#include "libgnome/libgnomeP.h"
#include "gnome-preferences.h"
#include "libgnomeui/gnome-client.h"

extern char *program_invocation_name;
extern char *program_invocation_short_name;

extern void  gnome_client_init (void);
extern void  gnome_type_init (void);

static void gnome_rc_parse(gchar *command);
#ifdef USE_SEGV_HANDLE
static void gnome_segv_handle(int signum);
#endif
static GdkPixmap *imlib_image_loader(GdkWindow   *window,
				     GdkColormap *colormap,
				     GdkBitmap  **mask,
				     GdkColor    *transparent_color,
				     const gchar *filename);



/* This describes all the arguments understood by Gtk.  We parse them
 * out and put them into our own private argv, which we then feed to
 * Gtk once the command line has been successfully parsed.  Yes, this
 * is a total hack.  FIXME: should hidden options really be visible?
 *
 * Note that the key numbers must start at -1 and decrease by 1.  The
 * key number is used to look up the argument name we supply to Gtk.
 */
static struct argp_option our_gtk_options[] =
{
	{ N_("\nGtk+ toolkit options"), -1, NULL, OPTION_DOC,   NULL, -3 },
	{ "gdk-debug",         -1, N_("FLAGS"),   0,      N_("Gdk debuging flags to set"), 0 },
	{ "gdk-no-debug",      -2, N_("FLAGS"),   0,      N_("Gdk debuging flags to unset"), 0 },
	{ "display",           -3, N_("DISPLAY"), 0,	  N_("X display to use"), 0 },
	{ "sync",              -4, NULL,          OPTION_HIDDEN, NULL, 0 },
	{ "no-xshm",           -5, NULL,          0,	  N_("Don't use X shared memory extension"), 0 },
	{ "name",              -6, N_("NAME"),    0,	  N_("Program name as used by the window manager"), 0 },
	{ "class",             -7, N_("CLASS"),   0,	  N_("Program class as used by the window manager"), 0 },
	{ "gxid_host",         -8, N_("HOST"),    0,	  N_("FIXME"), 0 },
	{ "gxid_port",         -9, N_("PORT"),    0,	  N_("FIXME"), 0 },
	{ "xim-preedit",      -10, N_("STYLE"),   0,	  N_("FIXME"), 0 },
	{ "xim-status",       -11, N_("STYLE"),   0,	  N_("FIXME"), 0 },
	{ "gtk-debug",        -12, N_("FLAGS"),   0,      N_("Gtk+ debuging flags to set"), 0 },
	{ "gtk-no-debug",     -13, N_("FLAGS"),   0,      N_("Gtk+ debuging flags to unset"), 0 },
	{ "g-fatal-warnings", -14, NULL,          0,      N_("Make all warnings fatal"), 0 },
	{ "gtk-module",       -15, N_("MODULE"),  0,      N_("Load additional Gtk modules"), 0 },
	{ NULL, 0, NULL, 0, NULL, 0 }
};

/* Index of next free slot in the argument vector we construct for
 * Gtk.
 */
static int our_argc;

/* Argument vector we construct.  */
static char **our_argv;

/* The master client.  */
static GnomeClient *client;

/* Called during argument parsing to handle various details.  */
static error_t
our_gtk_parse_func (int key, char *arg, struct argp_state *state)
{
	if (key < 0)
	{
		/* This is some argument we defined.  We handle it by pushing
		 * the flag, and possibly the argument, onto our saved argument
		 * vector.  Later this is passed to gtk_init.
		 *
		 * Add our arguments as static argument to the master
		 * client.
		 */
		our_argv[our_argc] = g_strconcat ("--",
						    our_gtk_options[- key].name,
						    NULL);
		gnome_client_add_static_arg (client, our_argv [our_argc++], NULL);
		
		if (arg)
		  {
		    our_argv[our_argc] = g_strdup (arg);
		    gnome_client_add_static_arg (client, our_argv[our_argc++], NULL);
		  }
	}
	else if (key == ARGP_KEY_INIT)
	{
		/* We use twice the amount of space you'd think we need,
		 * because the user might write all options as
		 * `--foo=bar', but we always split into two arguments,
		 * like `-foo bar'.
		 */
		our_argv = (char **) g_malloc (2 * (state->argc + 1) * sizeof (char *));
		our_argc = 0;
		our_argv[our_argc++] = g_strdup (state->argv[0]);

		/* Get the master client.  It must be defined, when
		 * processing 'ARGP_KEY_INIT'.
		 */
		client= gnome_master_client ();
	}
	else if (key == ARGP_KEY_SUCCESS || key == ARGP_KEY_ERROR)
	{
		int i, copy_ac = our_argc;
		char **copy = (char **) g_malloc (our_argc * sizeof (char *));

		memcpy (copy, our_argv, our_argc * sizeof (char *));
		our_argv[our_argc] = NULL;

		gtk_init (&our_argc, &our_argv);
		gdk_imlib_init ();
		gnome_type_init();

		gtk_rc_set_image_loader(imlib_image_loader);
		gnome_rc_parse (program_invocation_name);

		for (i = 0; i < copy_ac; ++i)
			g_free (copy[i]);
		g_free (copy);
		g_free (our_argv); our_argv = NULL;
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
 * program.  You must override if that is incorrect.
 */
static void
default_version_func (FILE *stream, struct argp_state *state)
{
	fprintf (stream, "Gnome %s %s\n", gnome_app_id,
		 argp_program_version ? argp_program_version : "");
	
	/* FIXME: define a way to set copyright date so we can print it
	 * here?
	 */
}

/* handeling of automatic syncing of gnome_config stuff */
static int config_was_changed = FALSE;
static int sync_timeout = -1;

static int
sync_timeout_f (gpointer data)
{
	sync_timeout = -1;
	gnome_config_sync ();
	return FALSE;
}

/*
 * gnome_config set handler, when we use a _set_* function this is
 * called and it schedhules a sync in 10 seconds
 */
static void
set_handler(gpointer data)
{
	config_was_changed = TRUE;

	/* timeout already running */
	if(sync_timeout!=-1)
		return;
	/* set a new sync timeout for 10 seconds from now */
	sync_timeout = gtk_timeout_add (10*1000,sync_timeout_f,NULL);
}

/* called on every gnome_config_sync */
static void
sync_handler(gpointer data)
{
	config_was_changed = FALSE;
	if (sync_timeout > -1)
		gtk_timeout_remove (sync_timeout);
	sync_timeout = -1;
}

/*
 * is called as an atexit handler and synces the config if it was changed
 * but not yet synced
 */
static void
atexit_handler(void)
{
	/*avoid any further sync_handler calls*/
	gnome_config_set_sync_handler(NULL,NULL);
	if(config_was_changed)
		gnome_config_sync();
}

error_t
gnome_init_with_data (char *app_id, struct argp *app_args,
		      int argc, char **argv,
		      unsigned int flags, int *arg_index,
		      void *user_data)
{
	error_t retval;

#ifdef USE_SEGV_HANDLE
	struct sigaction sa;

	/* Debugging segv when you have a pointer grab is less than
	 * useful, Sop instead of removing the above #if 0, fix the DND
	 *
	 * Yes, we do this twice, so if an error occurs before init,
	 * it will be caught, and if it happens after init, we'll override
	 * gtk's handler
	 */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = (gpointer)gnome_segv_handle;
	sigaction(SIGSEGV, &sa, NULL);
#endif
	/* now we replace gtk_init() with gnome_init() in our apps */
	gtk_set_locale();

	gnome_client_init ();
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

	/* Load UI preferences. Should this go here? */
	gnome_preferences_load();

	if (! argp_program_version_hook)
		argp_program_version_hook = default_version_func;

	/* Now parse command-line arguments.  */
	retval = gnome_parse_arguments_with_data (app_args, argc, argv,
						  flags, arg_index, user_data);
	
	/*now set up the handeling of automatic config syncing*/
	gnome_config_set_set_handler(set_handler,NULL);
	gnome_config_set_sync_handler(sync_handler,NULL);
	atexit(atexit_handler);

#ifdef USE_SEGV_HANDLE
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = (gpointer)gnome_segv_handle;
	sigaction(SIGSEGV, &sa, NULL);
#endif
	return retval;
}

error_t
gnome_init (char *app_id, struct argp *app_args,
	    int argc, char **argv,
	    unsigned int flags, int *arg_index)
{
	return gnome_init_with_data (app_id, app_args, argc, argv,
				     flags, arg_index, NULL);
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

#ifdef USE_SEGV_HANDLE
static void gnome_segv_handle(int signum)
{
	static int in_segv = 0;
	void *eip = NULL;
	pid_t pid;
	
#if 0
	/* Is there any way to do this portably? */
#ifdef linux
#if defined(__i386__)
	eip = sc->eip;
#elif defined(__ppc__)
	eip = sc->nc_ip;
#elif defined(__sparc__)
	eip = sc->sigc_pc;
#endif
#endif
	
#endif /* #if 0 */
	
	if(in_segv)
		return;
	
	in_segv++;
	
	pid = fork();
	
	if (pid){
		int estatus;
		pid_t eret;
		
		eret = waitpid(pid, &estatus, 0);
		
		/* We only do a zap-em if the user specifically requested it
		 * - if an error happens getting user decision, we carry on.
		 * "Failsafe" is the idea
		 */
		if (WIFEXITED(estatus) && WEXITSTATUS(estatus) == 0)
			exit(0);
		else if (WIFEXITED(estatus) && WEXITSTATUS(estatus) == 99){
			
			/* We don't decrement the in_segv lock
			 * - basically the user wants to ignore all future segv's
			 */
			return;
		}
	} else {
		char buf[32];
		snprintf(buf, sizeof(buf), "%ld", eip);
		/* Child process */
		execl(GNOMEBINDIR "/gnome_segv", GNOMEBINDIR "/gnome_segv",
		      program_invocation_name, buf, NULL);
	}
	in_segv--;
}
#endif /* USE_SEGV_HANDLE */

static GdkPixmap *
imlib_image_loader(GdkWindow   *window,
		   GdkColormap *colormap,
		   GdkBitmap  **mask,
		   GdkColor    *transparent_color,
		   const gchar *filename)
{
	GdkPixmap *retval;
	
	if (gdk_imlib_load_file_to_pixmap ((char *) filename, &retval, mask))
		return retval;
	else
		return NULL;
}
