/* Blame Elliot for the poptimization of this file */

#include <config.h>

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <alloca.h>

#include <gtk/gtk.h>
#include <gdk_imlib.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#ifdef HAVE_ESD
#include <esd.h>
#endif

#include "libgnome/libgnomeP.h"
#include "libgnome/gnome-popt.h"
#include "gnome-preferences.h"
#include "libgnomeui/gnome-client.h"
#include "libgnomeui/gnome-init.h"

static gboolean
relay_gtk_signal(GtkObject *object,
		 guint signal_id,
		 guint n_params,
		 GtkArg *params,
		 GnomeTriggerList *t);

extern char *program_invocation_name;
extern char *program_invocation_short_name;

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



/* The master client.  */
static GnomeClient *client = NULL;

/* handeling of automatic syncing of gnome_config stuff */
static int config_was_changed = FALSE;
static int sync_timeout = -1;
static gboolean gnome_initialized = FALSE;

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

static void
gnome_add_gtk_arg_callback(poptContext con,
			   enum poptCallbackReason reason,
			   const struct poptOption * opt,
			   const char * arg, void * data)
{
  static GPtrArray *gtk_args = NULL;
  char *newstr, *newarg;
  int final_argc;
  char **final_argv;

  if(gnome_initialized) {
    /* gnome has already been initialized, so app might be making a
       second pass over the args - just ignore */
    return;
  }

  switch(reason) {
  case POPT_CALLBACK_REASON_PRE:
    gtk_args = g_ptr_array_new();
    g_ptr_array_add(gtk_args,
		    g_strdup(poptGetInvocationName(con)));
    break;

  case POPT_CALLBACK_REASON_OPTION:
    newstr = g_copy_strings("--", opt->longName, NULL);
    g_ptr_array_add(gtk_args, newstr);
    gnome_client_add_static_arg(client, newstr, NULL);

    if(opt->argInfo == POPT_ARG_STRING) {
      g_ptr_array_add(gtk_args, (char *)arg);
      gnome_client_add_static_arg(client, arg, NULL);
    }
    break;

  case POPT_CALLBACK_REASON_POST:
    g_ptr_array_add(gtk_args, NULL);
    final_argc = gtk_args->len - 1;
    final_argv = g_memdup(gtk_args->pdata, sizeof(char *) * gtk_args->len);

    gtk_init(&final_argc, &final_argv);

    g_free(final_argv);
#ifdef GNOME_CLIENT_WILL_COPY_ARGS
    g_strfreev(gtk_args->pdata); /* this weirdness is here to eliminate
					  memory leaks if gtk_init() modifies
					  argv[] */

    g_ptr_array_free(gtk_args, FALSE);
#else
    g_ptr_array_free(gtk_args, TRUE);
#endif
    gtk_args = NULL;
    break;
  }
}

static const struct poptOption gtk_options[] = {
  {NULL, '\0', POPT_ARG_CALLBACK|POPT_CBFLAG_PRE|POPT_CBFLAG_POST,
   &gnome_add_gtk_arg_callback, 0, NULL},
  {"gdk-debug", '\0', POPT_ARG_STRING, NULL, 0,
   N_("Gdk debugging flags to set"), N_("FLAGS")},
  {"gdk-no-debug", '\0', POPT_ARG_STRING, NULL, 0,
   N_("Gdk debugging flags to unset"), N_("FLAGS")},
  {"display", '\0', POPT_ARG_STRING, NULL, 0,
   N_("X display to use"), N_("DISPLAY")},
  {"sync", '\0', POPT_ARG_NONE, NULL, 0,
   N_("Make X calls synchronous"), NULL},
  {"no-xshm", '\0', POPT_ARG_NONE, NULL, 0,
   N_("Don't use X shared memory extension"), NULL},
  {"name", '\0', POPT_ARG_STRING, NULL, 0,
   N_("Program name as used by the window manager"), N_("NAME")},
  {"class", '\0', POPT_ARG_STRING, NULL, 0,
   N_("Program class as used by the window manager"), N_("CLASS")},
  {"gxid_host", '\0', POPT_ARG_STRING, NULL, 0,
   NULL, N_("HOST")},
  {"gxid_port", '\0', POPT_ARG_STRING, NULL, 0,
   NULL, N_("PORT")},
  {"xim-preedit", '\0', POPT_ARG_STRING, NULL, 0,
   NULL, N_("STYLE")},
  {"xim-status", '\0', POPT_ARG_STRING, NULL, 0,
   NULL, N_("STYLE")},
  {"gtk-debug", '\0', POPT_ARG_STRING, NULL, 0,
   N_("Gtk+ debugging flags to set"), N_("FLAGS")},
  {"gtk-no-debug", '\0', POPT_ARG_STRING, NULL, 0,
   N_("Gtk+ debugging flags to unset"), N_("FLAGS")},
  {"g-fatal-warnings", '\0', POPT_ARG_NONE, NULL, 0,
   N_("Make all warnings fatal"), NULL},
  {"gtk-module", '\0', POPT_ARG_STRING, NULL, 0,
   N_("Load an additional Gtk module"), N_("MODULE")},
  {NULL, '\0', 0, NULL, 0}
};

static void
gnome_init_cb(poptContext ctx, enum poptCallbackReason reason,
	      const struct poptOption *opt)
{
#ifdef USE_SEGV_HANDLE
  struct sigaction sa;
#endif

  if(gnome_initialized)
    return;

  switch(reason) {
  case POPT_CALLBACK_REASON_PRE:

#ifdef USE_SEGV_HANDLE
    /* 
	 * Yes, we do this twice, so if an error occurs before init,
	 * it will be caught, and if it happens after init, we'll override
	 * gtk's handler
	 */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = (gpointer)gnome_segv_handle;
    sigaction(SIGSEGV, &sa, NULL);
#endif
    gtk_set_locale();
    client = gnome_master_client();

    break;
  case POPT_CALLBACK_REASON_POST:
    gdk_imlib_init();
    gnome_type_init();
    gtk_rc_set_image_loader(imlib_image_loader);
    gnome_rc_parse(program_invocation_name);
    gnome_preferences_load();
    gnome_config_set_set_handler(set_handler,NULL);
    gnome_config_set_sync_handler(sync_handler,NULL);
    g_atexit(atexit_handler);
    
#ifdef USE_SEGV_HANDLE
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = (gpointer)gnome_segv_handle;
    sigaction(SIGSEGV, &sa, NULL);
#endif

    /* lame trigger stuff. This really needs to be sorted out better
       so that the trigger API is exported more - perhaps there's something
       hidden in glib that already does this :) */
       
    if(gnome_config_get_bool("/sound/system/settings/start_esd=true")
       && gnome_config_get_bool("/sound/system/settings/event_sounds=true")
       && gnome_triggerlist_topnode) {
      int n;

      for(n = 0; n < gnome_triggerlist_topnode->numsubtrees; n++) {
	if(!strcmp(gnome_triggerlist_topnode->subtrees[n]->nodename,
		   "gtk-events"))
	  break;
      }

      if(n < gnome_triggerlist_topnode->numsubtrees) {
	GnomeTriggerList* gtk_events_node;

	gtk_events_node = gnome_triggerlist_topnode->subtrees[n];
	g_assert(gtk_events_node != NULL);

	for(n = 0; n < gtk_events_node->numsubtrees; n++) {
	  int signums[5];
	  int nsigs, i;
	  char *signame;
	  gpointer hookdata = gtk_events_node->subtrees[n];

	  g_assert(hookdata != NULL);

	  signame = gtk_events_node->subtrees[n]->nodename;
	  g_assert(signame != NULL);

	  /* XXX this is an incredible hack based on a compile-time knowledge of
	     what gtk widgets do what, rather than */
	  if(!strcmp(signame, "activate")) {
	    gtk_type_class(gtk_menu_item_get_type());
	    signums[0] = gtk_signal_lookup(signame, gtk_menu_item_get_type());

	    gtk_type_class(gtk_editable_get_type());
	    signums[1] = gtk_signal_lookup(signame, gtk_editable_get_type());
	    nsigs = 2;
	  } else if(!strcmp(signame, "toggled")) {
	    gtk_type_class(gtk_toggle_button_get_type());
	    signums[0] = gtk_signal_lookup(signame,
					   gtk_toggle_button_get_type());

	    gtk_type_class(gtk_check_menu_item_get_type());
	    signums[1] = gtk_signal_lookup(signame,
					   gtk_check_menu_item_get_type());
	    nsigs = 2;
	  } else if(!strcmp(signame, "clicked")) {
	    gtk_type_class(gtk_button_get_type());
	    signums[0] = gtk_signal_lookup(signame, gtk_button_get_type());
	    nsigs = 1;
	  } else {
	    gtk_type_class(gtk_widget_get_type());
	    signums[0] = gtk_signal_lookup(signame, gtk_widget_get_type());
	    nsigs = 1;
	  }
	  
	  for(i = 0; i < nsigs; i++)
	    if(signums[i] > 0)
	      gtk_signal_add_emission_hook(signums[i],
					   (GtkEmissionHook)relay_gtk_signal,
					   hookdata);
	}
      }
    }

    gnome_initialized = TRUE;
    break;
  case POPT_CALLBACK_REASON_OPTION:
    if(opt->val == -1) {
      g_print ("Gnome %s %s\n", gnome_app_id, gnome_app_version);
      exit(0);
      _exit(1);
    }
    break;
  }
}

static const struct poptOption gnome_options[] = {
  {NULL, '\0', POPT_ARG_CALLBACK|POPT_CBFLAG_PRE|POPT_CBFLAG_POST,
   &gnome_init_cb, 0, NULL, NULL},
  {"version", 'V', POPT_ARG_NONE, NULL, -1},
  POPT_AUTOHELP
  {NULL, '\0', 0, NULL, 0}
};

static void
gnome_register_options(void)
{
  gnomelib_register_popt_table(gtk_options, "GTK options");

  gnomelib_register_popt_table(gnome_options, "GNOME GUI options");
}

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
  g_return_val_if_fail(gnome_initialized == FALSE, -1);

  gnomelib_init (app_id, app_version);

  gnome_register_options();

  gnome_client_init();

  if(options) {
    appdesc = alloca(sizeof(" options") + strlen(app_id));
    strcpy(appdesc, app_id); strcat(appdesc, " options");
    gnomelib_register_popt_table(options, appdesc);
  }

  ctx = gnomelib_parse_args(argc, argv, flags);

  if(return_ctx)
    *return_ctx = ctx;
  else
    poptFreeContext(ctx);

  return 0;
}

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
	g_snprintf(apprc, strlen(buf) + 3, "%src", buf);
	
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
		g_snprintf(buf, sizeof(buf), "%ld", eip);
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

static gboolean
relay_gtk_signal(GtkObject *object,
		 guint signal_id,
		 guint n_params,
		 GtkArg *params,
		 GnomeTriggerList *t)
{
#ifdef HAVE_ESD
  /* Yes, this short circuits the rest of the triggers mechanism. It's
     easy to fix if we need to, though. */
  if(gnome_sound_connection < 0) return TRUE;

  if(t->actions[0]->u.media.cache_id == -1) {
    char *buf = alloca(strlen(t->nodename) + sizeof("gtk-events/"));

    t->actions[0]->u.media.cache_id =
      gnome_sound_sample_load(buf, t->actions[0]->u.media.file);

    if(t->actions[0]->u.media.cache_id < 0)
      t->actions[0]->u.media.cache_id = -2; /* don't even bother trying
					     to play this sound, since
					     loading failed */
  }

  if(t->actions[0]->u.media.cache_id >= 0)
    esd_sample_play(gnome_sound_connection, t->actions[0]->u.media.cache_id);

  return TRUE;
#endif
}
