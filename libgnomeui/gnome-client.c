/* gnome-client.c - */

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>

#include "libgnome/libgnome.h"
#include "gnome-client.h"
#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <X11/Xatom.h>

extern char *program_invocation_name;

#ifndef HAVE_LIBSM
typedef gpointer SmPointer;
#endif /* !HAVE_LIBSM */

enum {
  SAVE_YOURSELF,
  DIE,
  SAVE_COMPLETE,
  SHUTDOWN_CANCELLED,
  CONNECT,
  DISCONNECT,
  LAST_SIGNAL
};

typedef gint (*GnomeClientSignal1) (GnomeClient        *client,
				    gint                phase,
				    GnomeRestartStyle   save_style,
				    gint                shutdown,
				    GnomeInteractStyle  interact_style,
				    gint                fast,
				    gpointer            client_data);
typedef void (*GnomeClientSignal2) (GnomeClient        *client,
				    gint                restarted,
				    gpointer            client_data);

static void gnome_client_marshal_signal_1 (GtkObject     *object,
					   GtkSignalFunc  func,
					   gpointer       func_data,
					   GtkArg        *args);
static void gnome_client_marshal_signal_2 (GtkObject     *object,
					   GtkSignalFunc  func,
					   gpointer       func_data,
					   GtkArg        *args);

static void gnome_client_class_init              (GnomeClientClass *klass);
static void gnome_client_init                    (GnomeClient      *client);

static void gnome_real_client_destroy            (GtkObject        *object);
static void gnome_real_client_save_complete      (GnomeClient      *client);
static void gnome_real_client_shutdown_cancelled (GnomeClient      *client);
static void gnome_real_client_connect            (GnomeClient      *client,
						  gint              restarted);
static void gnome_real_client_disconnect         (GnomeClient      *client);

#ifdef HAVE_LIBSM

static void client_save_yourself_callback      (SmcConn   smc_conn,
						SmPointer client_data,
						int       save_type,
						Bool      shutdown,
						int       interact,
						Bool      fast);
static void client_die_callback                (SmcConn   smc_conn,
						SmPointer client_data);
static void client_save_complete_callback      (SmcConn   smc_conn,
						SmPointer client_data);
static void client_shutdown_cancelled_callback (SmcConn   smc_conn,
						SmPointer client_data);
static void client_interact_callback           (SmcConn   smc_conn,
						SmPointer client_data);
static void client_save_phase_2_callback       (SmcConn   smc_conn,
						SmPointer client_data);

static void gnome_process_ice_messages (gpointer client_data, 
					gint source,
					GdkInputCondition condition);

static void   client_set_prop_from_string   (GnomeClient *client,
					     gchar       *prop_name,
					     gchar       *value);
static void   client_set_prop_from_gchar    (GnomeClient *client,
					     gchar       *prop_name,
					     gchar        value);
static void   client_set_prop_from_array    (GnomeClient *client,
					     gchar       *prop_name,
					     gchar       *array[]);
static void   client_unset_prop             (GnomeClient *client,
					     gchar       *prop_name);

#endif /* HAVE_LIBSM */

#define array_free(array) gnome_string_array_free((array))
static gchar** array_init_from_arg           (gint argc, 
					      gchar *argv[]);
static gchar** array_copy                    (gchar **source);
static void    array_insert_sm_client_id_arg (gchar ***array, 
					      gchar *client_id);

/* 'GnomeInteractData' stuff */

typedef struct _GnomeInteractData GnomeInteractData;

struct _GnomeInteractData
{
  gint                   tag;
  GnomeClient           *client;
  GnomeDialogType        dialog_type;
  gint                   in_use;
  gint                   interp;
  GnomeInteractFunction  function;
  gpointer               data;
  GtkDestroyNotify       destroy;
};

static void gnome_interaction_remove (GnomeInteractData *interactf);

static GList *interact_functions = NULL;

static GtkObjectClass *parent_class = NULL;
static gint client_signals[LAST_SIGNAL] = { 0 };

static const char *sm_client_id_arg_name = "--sm-client-id";
static const char *sm_client_id_prop="SM_CLIENT_ID";

/*****************************************************************************/
/* Managing the default client */

/* The default client.  */
static GnomeClient *default_client= NULL;

/* Forward declaration for our parsing function.  */
static error_t client_parse_func (int key, char *arg,
				  struct argp_state *state);

/* Command-line arguments understood by this module.  */
static struct argp_option arguments[] =
{
  { "sm-client-id", -1, N_("ID"), OPTION_HIDDEN,
    N_("Specify session management id"), -2 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Definition of our command-line parser.  */
static struct argp parser =
{
  arguments,			/* Argument vector.  */
  client_parse_func,		/* Parsing function.  */
  NULL,				/* Docs.  */
  NULL,				/* More docs.  */
  NULL,				/* Children.  */
  NULL,				/* Help filter.  */
  PACKAGE			/* Translation domain.  */
};


/* Parse command-line arguments we recognize.  */
static error_t
client_parse_func (int key, char *arg, struct argp_state *state)
{
  if (key == -1)
    {
      /* Found our argument.  Set the session id.  */
      g_assert (default_client != NULL);
      gnome_client_set_id (default_client, arg);
      g_free (default_client->previous_id);
      default_client->previous_id = g_strdup (arg);
    }
  else if (key == ARGP_KEY_SUCCESS)
    {
      /* We're done, we think.  */
      g_assert (default_client != NULL);
      gnome_client_connect (default_client);
    }
  else
    return ARGP_ERR_UNKNOWN;

  return 0;
}

static void
default_client_connect (GnomeClient *client,
			gint         restarted,
			gpointer     client_data)
{
  char *client_id= gnome_client_get_id (client);
  
  /* FIXME */
  XChangeProperty (gdk_display, gdk_leader_window,
		   XInternAtom(gdk_display, sm_client_id_prop, False),
		   XA_STRING, 8, GDK_PROP_MODE_REPLACE,
		   (unsigned char *) client_id,
		   strlen(client_id));               
}

static void
default_client_disconnect (GnomeClient *client,
			   gpointer client_data)
{
  XDeleteProperty(gdk_display, gdk_leader_window,
		  XInternAtom(gdk_display, sm_client_id_prop, False));
}

static void
default_client_init (void)
{
  default_client= gnome_client_new_without_connection ();

  if (default_client)
    {
      gtk_signal_connect (GTK_OBJECT (default_client), "connect",
			  GTK_SIGNAL_FUNC (default_client_connect), NULL);
      gtk_signal_connect (GTK_OBJECT (default_client), "disconnect",
			  GTK_SIGNAL_FUNC (default_client_disconnect), NULL);
      gnome_parse_register_arguments (&parser);
    }
}

GnomeClient *
gnome_client_new_default (void)
{
  if (! default_client)
    default_client_init ();
  return default_client;
}


/*****************************************************************************/
/* GTK-class managing functions */

guint
gnome_client_get_type (void)
{
  static guint client_type = 0;
  
  if (!client_type)
    {
      GtkTypeInfo client_info =
      {
	"GnomeClient",
	sizeof (GnomeClient),
	sizeof (GnomeClientClass),
	(GtkClassInitFunc) gnome_client_class_init,
	(GtkObjectInitFunc) gnome_client_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL
      };

      client_type = gtk_type_unique (gtk_object_get_type (), &client_info);
    }
  
  return client_type;
}

static void
gnome_client_class_init (GnomeClientClass *klass)
{
  GtkObjectClass *object_class = (GtkObjectClass*) klass;
  
  parent_class = gtk_type_class (gtk_object_get_type ());
  
  
  client_signals[SAVE_YOURSELF] =
    gtk_signal_new ("save_yourself",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, save_yourself),
		    gnome_client_marshal_signal_1,
		    GTK_TYPE_BOOL, 5,
		    GTK_TYPE_INT,
		    GTK_TYPE_ENUM,
		    GTK_TYPE_BOOL,
		    GTK_TYPE_ENUM,
		    GTK_TYPE_BOOL);
  client_signals[DIE] =
    gtk_signal_new ("die",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, die),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  client_signals[SAVE_COMPLETE] =
    gtk_signal_new ("save_complete",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, save_complete),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  client_signals[SHUTDOWN_CANCELLED] =
    gtk_signal_new ("shutdown_cancelled",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, shutdown_cancelled),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  client_signals[CONNECT] =
    gtk_signal_new ("connect",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, connect),
		    gnome_client_marshal_signal_2,
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_BOOL);
  client_signals[DISCONNECT] =
    gtk_signal_new ("disconnect",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, disconnect),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  
  gtk_object_class_add_signals (object_class, client_signals, LAST_SIGNAL);
  
  object_class->destroy = gnome_real_client_destroy;
  
  klass->save_yourself      = NULL;
  klass->die                = gnome_client_disconnect;
  klass->save_complete      = gnome_real_client_save_complete;
  klass->shutdown_cancelled = gnome_real_client_shutdown_cancelled;
  klass->connect            = gnome_real_client_connect;
  klass->disconnect         = gnome_real_client_disconnect;
}

static void
gnome_client_init (GnomeClient *client)
{
  client->smc_conn          = NULL;
  client->client_id         = NULL;
  client->previous_id       = NULL;
  client->input_id          = 0;

  /* Preset some default values.  */
  client->clone_command     = NULL;
  client->current_directory = NULL;
  client->discard_command   = NULL;
  client->environment       = NULL;
  
  client->process_id        = getpid ();

  client->program           = NULL;
  client->resign_command    = NULL;
  client->restart_command   = NULL;

  client->restart_style     = -1;
  
  client->shutdown_command  = NULL;
  
  {
    gchar *user_id;
    
    user_id = getenv ("USER");
    if (user_id)
      client->user_id= g_strdup (user_id);
    else
      {
	struct passwd *pwd;
	
	pwd = getpwuid (getuid ());
	if (pwd)
	  {
	    client->user_id= g_strdup (pwd->pw_name);
	    
	    /* FIXME: 'pwd' should be freed */
	  }
	else
	  client->user_id= "";
      }
  }

  client->state                       = GNOME_CLIENT_IDLE;
  client->number_of_interact_requests = 0;
}

static void
gnome_real_client_destroy (GtkObject *object)
{
  GnomeClient *client;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (object));
  
  client = GNOME_CLIENT (object);
  
  if (GNOME_CLIENT_CONNECTED (client))
    gnome_client_disconnect (client);

  g_free (client->client_id);
  g_free (client->previous_id);

  array_free (client->clone_command);
  g_free     (client->current_directory);
  array_free (client->discard_command);
  array_free (client->environment);
  g_free     (client->program);
  array_free (client->resign_command);
  array_free (client->restart_command);
  array_free (client->shutdown_command);
  g_free     (client->user_id);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/*****************************************************************************/
/* 'gnome_client' public member functions */

GnomeClient *
gnome_client_new (void)
{
  GnomeClient *client;
  
  client= gnome_client_new_without_connection ();
  
  gnome_client_connect (client);

  return client;
}

GnomeClient *
gnome_client_new_without_connection (void)
{
  GnomeClient *client;
  gchar       *client_id;
  
  client= gtk_type_new (gnome_client_get_type ());

  /* Preset the CloneCommand, RestartCommand and Program properties.
     FIXME: having a default would be cool, but it is probably hard to
     get this to interact correctly with the distributed command-line
     parsing.  */
  client->clone_command   = NULL;
  client->restart_command = NULL;
  
  client->program = g_strdup (program_invocation_name);

  return client;
}

void
gnome_client_flush (GnomeClient *client)
{
#ifdef HAVE_LIBSM
  IceConn conn;
#endif

  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_CLIENT_CONNECTED (client));

#ifdef HAVE_LIBSM
  conn = SmcGetIceConnection (client->smc_conn);
  IceFlush (conn);
#endif
}

/*****************************************************************************/

#define ERROR_STRING_LENGTH 256

void
gnome_client_connect (GnomeClient *client)
{
#ifdef HAVE_LIBSM
  static SmPointer  context;
  SmcCallbacks      callbacks;
  gchar            *client_id;
#endif /* HAVE_LIBSM */

  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

#ifdef HAVE_LIBSM
  if (GNOME_CLIENT_CONNECTED (client))
    return;

  callbacks.save_yourself.callback      = client_save_yourself_callback;
  callbacks.die.callback                = client_die_callback;
  callbacks.save_complete.callback      = client_save_complete_callback;
  callbacks.shutdown_cancelled.callback = client_shutdown_cancelled_callback;

  callbacks.save_yourself.client_data = 
    callbacks.die.client_data =
    callbacks.save_complete.client_data =
    callbacks.shutdown_cancelled.client_data = (SmPointer) client;

  if (getenv ("SESSION_MANAGER"))
    {
      gchar error_string_ret[ERROR_STRING_LENGTH] = "";
      
      client->smc_conn = 
	SmcOpenConnection (NULL, &context, 
			   SmProtoMajor, SmProtoMinor,
			   SmcSaveYourselfProcMask | SmcDieProcMask |
			   SmcSaveCompleteProcMask | 
			   SmcShutdownCancelledProcMask,
			   &callbacks,
			   client->client_id, &client_id,
			   ERROR_STRING_LENGTH, error_string_ret);
      
      if (error_string_ret[0])
	g_warning ("While connecting to session manager:\n%s.",
		   error_string_ret);
    }

  if (GNOME_CLIENT_CONNECTED (client))
    {
      IceConn ice_conn;
      gint    restarted = FALSE;
      
      if (client->client_id == NULL)
	{
	  client->client_id = client_id;
	}
      else if (strcmp (client->client_id, client_id) != 0)
	{
	  g_free (client->client_id);
	  client->client_id = client_id;
	}
      else
	{
	  g_free (client_id);
	  restarted = TRUE;
	}
      
      /* Lets call 'gnome_process_ice_messages' if new data arrives.  */
      ice_conn = SmcGetIceConnection (client->smc_conn);
      client->input_id= gdk_input_add (IceConnectionNumber (ice_conn),
				       GDK_INPUT_READ, 
				       gnome_process_ice_messages,
				       (gpointer) client);

      /* Let all the world know, that we have a connection to a
         session manager.  */
      gtk_signal_emit (GTK_OBJECT (client), client_signals[CONNECT], 
		       restarted);
    }
#endif /* HAVE_LIBSM */
}


void
gnome_client_disconnect (GnomeClient *client)
{
  g_return_if_fail (client != NULL);


  gnome_client_flush (client);
  if (GNOME_CLIENT_CONNECTED (client))
    gtk_signal_emit (GTK_OBJECT (client), client_signals[DISCONNECT]);
}

/*****************************************************************************/

void 
gnome_client_set_clone_command (GnomeClient *client, 
				gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  /* Usually the CloneCommand property is required.  If you unset the
     CloneCommand property, we will use the RestartCommand instead.  */
  if (argv == NULL)
    {
      g_return_if_fail (argc == 0);

      array_free (client->clone_command);
      client->clone_command = array_copy (client->restart_command);
    }
  else
    {
      array_free (client->clone_command);
      client->clone_command = array_init_from_arg (argc, argv);
    }

#ifdef HAVE_LIBSM
  client_set_prop_from_array (client, SmCloneCommand, 
			      client->clone_command);
#endif /* HAVE_LIBSM */
}


void 
gnome_client_set_current_directory (GnomeClient *client,
				    const gchar *dir)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  
  g_free (client->current_directory);
  
  if (dir)
    {
      client->current_directory= g_strdup (dir);
#ifdef HAVE_LIBSM
      client_set_prop_from_string (client, SmCurrentDirectory, 
				  client->current_directory);
#endif /* HAVE_LIBSM */
    }
  else
    {
      client->current_directory= NULL;
#ifdef HAVE_LIBSM
      client_unset_prop (client, SmCurrentDirectory);
#endif /* HAVE_LIBSM */
    }
}


void 
gnome_client_set_discard_command (GnomeClient *client,
				  gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  if (argv == NULL)
    {
      g_return_if_fail (argc == 0);
      
      array_free (client->discard_command);
      client->discard_command= NULL;
#ifdef HAVE_LIBSM
      client_unset_prop (client, SmDiscardCommand);
#endif /* HAVE_LIBSM */
    }
  else
    {
      array_free (client->discard_command);
      client->discard_command = array_init_from_arg (argc, argv);
      
#ifdef HAVE_LIBSM
      client_set_prop_from_array (client, SmDiscardCommand, 
				  client->discard_command);
#endif /* HAVE_LIBSM */
    }
}


void 
gnome_client_set_environment (GnomeClient *client,
			     gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  
  if (argv == NULL)
    {
      g_return_if_fail (argc == 0);
      
      array_free (client->environment);
      client->environment= NULL;
#ifdef HAVE_LIBSM
      client_unset_prop (client, SmEnvironment);
#endif /* HAVE_LIBSM */
    }
  else
    {
      array_free (client->environment);
      client->environment = array_init_from_arg (argc, argv);
      
#ifdef HAVE_LIBSM
      client_set_prop_from_array (client, SmEnvironment, 
				  client->environment);
#endif /* HAVE_LIBSM */
    }
}


void 
gnome_client_set_process_id (GnomeClient *client, pid_t pid)
{
  gchar str_pid[32];
  
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  client->process_id = pid;
  
  sprintf (str_pid, "%d", client->process_id);
#ifdef HAVE_LIBSM
  client_set_prop_from_string (client, SmProcessID, str_pid);
#endif /* HAVE_LIBSM */
}


void 
gnome_client_set_program (GnomeClient *client, 
			  const gchar *program)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  /* The Program property is required, so you must not unset it.  */
  g_return_if_fail (program != NULL);
  
  g_free (client->program);
  
  client->program= g_strdup (program);
#ifdef HAVE_LIBSM
  client_set_prop_from_string (client, SmProgram, client->program);
#endif /* HAVE_LIBSM */
}


void 
gnome_client_set_resign_command (GnomeClient *client,
				 gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  if (argv == NULL)
    {
      g_return_if_fail (argc == 0);
      
      array_free (client->resign_command);
      client->resign_command= NULL;
#ifdef HAVE_LIBSM
      client_unset_prop (client, SmResignCommand);
#endif /* HAVE_LIBSM */
    }
  else
    {
      array_free (client->resign_command);
      client->resign_command = array_init_from_arg (argc, argv);
  
#ifdef HAVE_LIBSM
      client_set_prop_from_array (client, SmResignCommand, 
				  client->resign_command);
#endif /* HAVE_LIBSM */
    }
}


void 
gnome_client_set_restart_command (GnomeClient *client,
				  gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  
  /* The RestartCommand property is required, so you must not unset
     it.  */
  g_return_if_fail (argc != 0);
  g_return_if_fail (argv != NULL);

  array_free (client->restart_command);
  client->restart_command = array_init_from_arg (argc, argv);

  /* Set the '--sm-client-id' option, if we are connected.  We set it
     in a copy, though, and not the original.  This lets us re-use the
     original for the clone command if we must.  */
  if (GNOME_CLIENT_CONNECTED (client) && client->restart_command)
    {
      gchar **dup = array_copy (client->restart_command);
      array_insert_sm_client_id_arg (&dup, client->client_id);

#ifdef HAVE_LIBSM
      client_set_prop_from_array (client, SmRestartCommand, dup);
#endif /* HAVE_LIBSM */
    }
}


void 
gnome_client_set_restart_style (GnomeClient *client,
				GnomeRestartStyle style)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  client->restart_style= style;

#ifdef HAVE_LIBSM
  client_set_prop_from_gchar (client, SmRestartStyleHint, 
			      (gchar) client->restart_style);
#endif /* HAVE_LIBSM */
}


void 
gnome_client_set_shutdown_command (GnomeClient *client,
				   gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  
  if (argv == NULL)
    {
      g_return_if_fail (argc == 0);
      
      array_free (client->shutdown_command);
      client->shutdown_command = NULL;
#ifdef HAVE_LIBSM
      client_unset_prop (client, SmShutdownCommand);
#endif /* HAVE_LIBSM */
    }
  else
    {
      array_free (client->shutdown_command);
      client->shutdown_command = array_init_from_arg (argc, argv);

#ifdef HAVE_LIBSM
      client_set_prop_from_array (client, SmShutdownCommand, 
				  client->shutdown_command);
#endif /* HAVE_LIBSM */
    }
}


void 
gnome_client_set_user_id (      GnomeClient *client,
			  const gchar       *id)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  /* The UserID property is required, so you must not unset it.  */
  g_return_if_fail (id != NULL);
  
  g_free (client->user_id);
  
  client->user_id= g_strdup (id);
#ifdef HAVE_LIBSM
  client_set_prop_from_string (client, SmUserID, client->user_id);
#endif /* HAVE_LIBSM */
}

void
gnome_client_set_id (GnomeClient *client, const gchar *id)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  g_return_if_fail (!GNOME_CLIENT_CONNECTED (client));

  g_return_if_fail (id != NULL);

  g_free (client->client_id);
  client->client_id= g_strdup (id);
}


gchar *
gnome_client_get_id (GnomeClient *client)
{
  g_return_val_if_fail (client != NULL, NULL);
  g_return_val_if_fail (GNOME_IS_CLIENT (client), NULL);
  
  return client->client_id;
}

gchar *
gnome_client_get_previous_id (GnomeClient *client)
{
  g_return_val_if_fail (client != NULL, NULL);
  return client->previous_id;
}

/*****************************************************************************/

static void
gnome_real_client_save_complete (GnomeClient *client)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  
  client->state = GNOME_CLIENT_IDLE;
}

/* The following function is executed, after receiving a SHUTDOWN
 * CANCELLED message from the session manager.  It is executed before
 * the function connected to the 'shutdown_cancelled' signal are
 * called */

static void
gnome_real_client_shutdown_cancelled (GnomeClient *client)
{
  GList             *temp_list;
  GnomeInteractData *interactf;
  
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

#ifdef HAVE_LIBSM
  if (client->state == GNOME_CLIENT_SAVING)
    SmcSaveYourselfDone (client->smc_conn, False);
  
  client->state = GNOME_CLIENT_IDLE;

  /* Free all interation keys but the one in use.  */
  temp_list = interact_functions;
  while (temp_list)
    {
      interactf = temp_list->data;
      temp_list = temp_list->next;
      
      if (interactf->client == client)
	gnome_interaction_remove (interactf);
    }
#endif /* HAVE_LIBSM */
}

static void
gnome_real_client_connect (GnomeClient *client,
			   gint         restarted)
{  
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  
  if (client->clone_command == 0)
    client->clone_command = array_copy (client->restart_command);

#ifdef HAVE_LIBSM
  /* We now set all non empty properties.  */
  client_set_prop_from_array (client, SmCloneCommand,
			      client->clone_command);
  client_set_prop_from_string (client, SmCurrentDirectory,
			       client->current_directory);
  client_set_prop_from_array (client, SmDiscardCommand,
			      client->discard_command);
  client_set_prop_from_array (client, SmEnvironment,
			      client->environment);
  {
    gchar str_pid[32];
    
    sprintf (str_pid, "%d", client->process_id);
    client_set_prop_from_string (client, SmProcessID, str_pid);
  }
  client_set_prop_from_string (client, SmProgram,
			       client->program);
  client_set_prop_from_array (client, SmResignCommand,
			      client->resign_command);

  if (client->restart_command != NULL)
    {
      gchar **dup = array_copy (client->restart_command);
      array_insert_sm_client_id_arg (&dup, client->client_id);
      client_set_prop_from_array (client, SmRestartCommand, dup);
    }
  else
    client_set_prop_from_array (client, SmRestartCommand, NULL);

  if (client->restart_style != -1)
    client_set_prop_from_gchar (client, SmRestartStyleHint,
				(gchar)client->restart_style);
  client_set_prop_from_array (client, SmShutdownCommand,
			      client->shutdown_command);
  client_set_prop_from_string (client, SmUserID,
			       client->user_id);
#endif /* HAVE_LIBSM */
}


static void
gnome_real_client_disconnect (GnomeClient *client)
{
  GList             *temp_list;
  GnomeInteractData *interactf;
  
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  
#ifdef HAVE_LIBSM
  if (GNOME_CLIENT_CONNECTED (client))
    {
      SmcCloseConnection (client->smc_conn, 0, NULL);
      client->smc_conn = NULL;
    }
  
  if (client->input_id)
    {
      gdk_input_remove (client->input_id);
      client->input_id = 0;
    }
#endif /* HAVE_LIBSM */
  
  client->state = GNOME_CLIENT_IDLE;
  
  /* Free all interation keys but the one in use.  */
  temp_list = interact_functions;
  while (temp_list)
    {
      interactf = temp_list->data;
      temp_list = temp_list->next;
      
      if (interactf->client == client)
	gnome_interaction_remove (interactf);
    }
}

/*****************************************************************************/

static void
gnome_client_request_interaction_internal (GnomeClient           *client,
					   GnomeDialogType        dialog_type,
					   gint                   interp,
					   GnomeInteractFunction  function,
					   gpointer               data,
					   GtkDestroyNotify       destroy) 
{
  static gint interaction_tag = 1;
  GnomeInteractData *interactf;
  
#ifdef HAVE_LIBSM

  interactf = g_new (GnomeInteractData, 1);
  
  g_return_if_fail (interactf != NULL);
  
  interactf->tag         = interaction_tag++;
  interactf->client      = client;
  interactf->dialog_type = dialog_type;
  interactf->in_use      = FALSE;
  interactf->interp      = interp;
  interactf->function    = function;
  interactf->data        = data;
  interactf->destroy     = destroy;

  interact_functions = g_list_append (interact_functions, interactf);
  client->number_of_interact_requests++;

  SmcInteractRequest (client->smc_conn, 
		      dialog_type, 
		      client_interact_callback, 
		      (SmPointer) client);

#endif /* HAVE_LIBSM */
}

void
gnome_client_request_interaction (GnomeClient *client,
				  GnomeDialogType dialog_type,
				  GnomeInteractFunction function,
				  gpointer data)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  g_return_if_fail (client->state == GNOME_CLIENT_SAVING);

  g_return_if_fail ((client->interact_style != GNOME_INTERACT_NONE) &&
		    ((client->interact_style == GNOME_INTERACT_ANY) ||
		     (dialog_type == GNOME_DIALOG_ERROR)));
  
  gnome_client_request_interaction_internal (client, dialog_type, 
					     FALSE, function, data, NULL);  
}

void
gnome_client_request_interaction_interp (GnomeClient *client,
					 GnomeDialogType dialog_type,
					 GtkCallbackMarshal function,
					 gpointer data,
					 GtkDestroyNotify destroy)
{
#if 0
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  g_return_if_fail (client->state == GNOME_CLIENT_SAVING);

  g_return_if_fail ((client->interact_style != GNOME_INTERACT_NONE) &&
		    ((client->interact_style == GNOME_INTERACT_ANY) ||
		     (dialog_type == GNOME_DIALOG_ERROR)));
  
  gnome_client_request_interaction_internal (client, dialog_type, 
					     TRUE,
					     (GnomeInteractFunction)function, 
					     data, destroy);
#else
  /* Not yet implemented */
#endif
}

static void
gnome_invoke_interact_function (GnomeInteractData *interactf)
{
  interactf->in_use= TRUE;

  if (!interactf->interp)
    interactf->function (interactf->client, interactf->tag,
			 interactf->dialog_type, interactf->data);
  else
    {
      GtkArg args[4];

      args[0].name = NULL;
      args[0].type = GTK_TYPE_NONE;

      args[1].name = NULL;
      args[1].type = GTK_TYPE_OBJECT;
      GTK_RETLOC_OBJECT(args[1]) = &interactf->client;
      
      args[2].name = NULL;
      args[2].type = GTK_TYPE_INT;
      GTK_RETLOC_INT(args[2]) = &interactf->tag;

      args[3].name = "GnomeDialogType";
      args[3].type = GTK_TYPE_ENUM;
      GTK_RETLOC_ENUM(args[3]) = &interactf->dialog_type;

      ((GtkCallbackMarshal)interactf->function) (NULL,
						 interactf->data,
						 3, args);
    }
}

static void
gnome_interaction_destroy (GnomeInteractData *interactf)
{
  if (interactf->destroy)
    (interactf->destroy) (interactf->data);
  g_free (interactf);
}

static void
gnome_interaction_remove (GnomeInteractData *interactf)
{
  if (interactf->client)
    interactf->client->number_of_interact_requests--;
  
  if (interactf->in_use)
    interactf->client = NULL;
  else
    {
      interact_functions = g_list_remove (interact_functions, interactf);
      gnome_interaction_destroy (interactf);
    }
}

/*****************************************************************************/

void
gnome_client_request_phase_2 (GnomeClient *client)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  /* Check if we are in save phase one */
  g_return_if_fail (client->state == GNOME_CLIENT_SAVING);
  g_return_if_fail (client->phase == 1);

  client->save_phase_2_requested= TRUE;
}

void
gnome_client_request_save (GnomeClient        *client,
			   GnomeSaveStyle      save_style,
			   int /* bool */      shutdown,
			   GnomeInteractStyle  interact_style,
			   int /* bool */      fast,
			   int /* bool */      global)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  
#ifdef HAVE_LIBSM
  if (GNOME_CLIENT_CONNECTED (client))
    {
      SmcRequestSaveYourself (client->smc_conn, save_style,
			      shutdown, interact_style,
			      fast, global);            
    }
#endif HAVE_LIBSM
}

/*****************************************************************************/
/* 'GnomeInteractData' stuff */

void
gnome_interaction_key_return (gint key,
			      gint cancel_shutdown)
{
#ifdef HAVE_LIBSM
  GList             *temp_list;
  GnomeInteractData *interactf = NULL;
  GnomeClient       *client = NULL;

  /* Find the first interact function belonging to this key.  */

  temp_list = interact_functions;
  while (temp_list)
    {
      interactf = temp_list->data;
      if (interactf->tag == key)
	break;
      
      temp_list = temp_list->next;
    }
  
  g_return_if_fail (interactf);

  client = interactf->client;
  
  /* This interact function is not used anymore */  
  interactf->in_use = FALSE;
  gnome_interaction_remove (interactf);

  /* The case that 'client != NULL' should only occur, if the
     connection to the session manager was closed, while we where
     interacting or we received a SHUTDOWN_CANCELLED while
     interacting.  */
  if (client == NULL)
    return;
  
  if (cancel_shutdown && !client->shutdown)
    cancel_shutdown= FALSE;
  
  SmcInteractDone (client->smc_conn, cancel_shutdown);
  
  if (client->number_of_interact_requests == 0)
    {
      if (client->save_phase_2_requested && (client->phase == 1))
	{
	  SmcRequestSaveYourselfPhase2 (client->smc_conn,
					client_save_phase_2_callback,
					(SmPointer) client);
	}
      else
	{
	  SmcSaveYourselfDone (client->smc_conn, 
			       client->save_successfull);

	  client->state =
	    (client->shutdown) ? GNOME_CLIENT_WAITING : GNOME_CLIENT_IDLE;
	}
    } 
#endif /* HAVE_LIBSM */
}

/*****************************************************************************/
/* 'gnome_client' private member functions */

#ifdef HAVE_LIBSM

void
client_set_prop_from_string (GnomeClient *client,
			     gchar *prop_name,
			     gchar *value)
{
  SmProp prop, *proplist[1];
  SmPropValue val;

  g_return_if_fail (prop_name  != NULL);

  if (!GNOME_CLIENT_CONNECTED (client) || (value == NULL))
    return;  

  prop.name     = prop_name;
  prop.type     = SmARRAY8;
  prop.num_vals = 1;
  prop.vals     = &val;
  val.length = strlen (value) + 1;
  val.value  = value;
  proplist[0] = &prop;
  SmcSetProperties (client->smc_conn, 1, proplist);
}

void
client_set_prop_from_gchar (GnomeClient *client,
			    gchar *prop_name,
			    gchar value)
{
  SmProp prop, *proplist[1];
  SmPropValue val;

  g_return_if_fail (prop_name  != NULL);

  if (!GNOME_CLIENT_CONNECTED (client))
    return;  

  prop.name     = prop_name;
  prop.type     = SmCARD8;
  prop.num_vals = 1;
  prop.vals     = &val;
  val.length = 1;
  val.value  = &value;
  proplist[0] = &prop;
  SmcSetProperties (client->smc_conn, 1, proplist);
}

void
client_set_prop_from_array (GnomeClient *client,
			    gchar *prop_name,
			    gchar *array[])
{
  gint    argc;
  gchar **ptr;
  gint    i;

  SmProp prop, *proplist[1];
  SmPropValue *vals;
  
  g_return_if_fail (prop_name != NULL);

  if (!GNOME_CLIENT_CONNECTED (client) || (array == NULL))
    return;

  /* We count the number of elements in our array.  */
  for (ptr = array, argc = 0; *ptr ; ptr++, argc++) /* LOOP */;

  /* Now initialize the 'vals' array.  */
  vals = g_new (SmPropValue, argc);
  for (ptr = array, i = 0 ; i < argc ; ptr++, i++)
    {
      vals[i].length = strlen (*ptr) + 1;
      vals[i].value  = *ptr;
    }

  prop.name     = prop_name;
  prop.type     = SmLISTofARRAY8;
  prop.num_vals = argc;
  prop.vals     = vals;
  proplist[0]   = &prop;
  SmcSetProperties (client->smc_conn, 1, proplist);

  g_free (vals);
}		   

void
client_unset_prop (GnomeClient *client, gchar *prop_name)
{
  g_return_if_fail (prop_name  != NULL);

  if (GNOME_CLIENT_CONNECTED (client))
    SmcDeleteProperties (client->smc_conn, 1, &prop_name);
}

#endif /* HAVE_LIBSM */

/*****************************************************************************/

#ifdef HAVE_LIBSM

static void 
gnome_process_ice_messages (gpointer client_data, 
			    gint source,
			    GdkInputCondition condition)
{
  IceProcessMessagesStatus  status;
  GnomeClient              *client = (GnomeClient*) client_data;
  
  status = IceProcessMessages (SmcGetIceConnection (client->smc_conn),
			       NULL, NULL);
  
  if (status == IceProcessMessagesIOError)
    {
      gnome_client_disconnect (client);
      /* FIXME: sent error messages */
    }
}

static void
client_save_yourself_callback (SmcConn   smc_conn,
			       SmPointer client_data,
			       int       save_style,
			       Bool      shutdown,
			       int       interact_style,
			       Bool      fast)
{
  GnomeClient *client = (GnomeClient*) client_data;

  client->save_type          = (GnomeSaveStyle)     save_style;
  client->shutdown           = (gint)               shutdown;
  client->interact_style     = (GnomeInteractStyle) interact_style;
  client->fast               = (gint)               fast;
  client->phase              = 1;

  client->state                     = GNOME_CLIENT_SAVING;
  client->save_phase_2_requested    = FALSE;
  client->save_successfull          = TRUE;
  client->number_of_save_signals    = 0;

  gtk_signal_emit (GTK_OBJECT (client), 
		   client_signals[SAVE_YOURSELF],
		   1, save_style, shutdown, interact_style, fast);

#ifdef HAVE_LIBSM
  if (client->number_of_interact_requests == 0)
    {
      if (client->save_phase_2_requested)
	{
	  SmcRequestSaveYourselfPhase2 (client->smc_conn,
					client_save_phase_2_callback,
					(SmPointer) client);
	}
      else 
	{
	  if (client->number_of_save_signals == 0)
	    client->save_successfull = FALSE;

	  if (client->state == GNOME_CLIENT_SAVING)
	    SmcSaveYourselfDone (client->smc_conn, client->save_successfull);

	  client->state = 
	    (client->shutdown) ? GNOME_CLIENT_WAITING : GNOME_CLIENT_IDLE;
	}
    }
#endif HAVE_LIBSM
}

static void
client_die_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client = (GnomeClient*) client_data;

  gtk_signal_emit (GTK_OBJECT (client), client_signals[DIE]);
}

static void
client_save_complete_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client = (GnomeClient*) client_data;

  gtk_signal_emit (GTK_OBJECT (client), client_signals[SAVE_COMPLETE]);
}

static void
client_shutdown_cancelled_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client = (GnomeClient*) client_data;

  gtk_signal_emit (GTK_OBJECT (client), client_signals[SHUTDOWN_CANCELLED]);
}

static void 
client_interact_callback (SmcConn smc_conn, SmPointer client_data)
{
  GList             *temp_list;
  GnomeInteractData *interactf = NULL;

  /* Find the first interact function belonging to this client.  */

  temp_list = interact_functions;
  while (temp_list)
    {
      interactf = temp_list->data;
      if (interactf->client == (GnomeClient*) client_data)
	break;
      
      temp_list = temp_list->next;
    }
  
  g_assert (interactf);
  
  gnome_invoke_interact_function (interactf);
}

static void 
client_save_phase_2_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client = (GnomeClient*) client_data;

  client->phase= 2;
 
  gtk_signal_emit (GTK_OBJECT (client), client_signals[SAVE_YOURSELF],
		   2,
		   client->save_type,
		   client->shutdown,
		   client->interact_style,
		   client->fast);

  if (client->number_of_interact_requests == 0)
    {
      if (client->state == GNOME_CLIENT_SAVING)
	SmcSaveYourselfDone (client->smc_conn, 
			     client->save_successfull);

      client->state =
	(client->shutdown) ? GNOME_CLIENT_WAITING : GNOME_CLIENT_IDLE;
    }
}

/*****************************************************************************/

#endif /* HAVE_LIBSM */

static void
gnome_client_marshal_signal_1 (GtkObject     *object,
			       GtkSignalFunc  func,
			       gpointer       func_data,
			       GtkArg        *args)
{
  GnomeClient        *client;
  GnomeClientSignal1  rfunc;
  
  client = (GnomeClient*)object;
  rfunc  = (GnomeClientSignal1) func;
  
  client->save_successfull = (* rfunc) (client,
					GTK_VALUE_INT (args[0]),
					GTK_VALUE_ENUM (args[1]),
					GTK_VALUE_BOOL (args[2]),
					GTK_VALUE_ENUM (args[3]),
					GTK_VALUE_BOOL (args[4]),
					func_data);
  client->number_of_save_signals++;
}

static void
gnome_client_marshal_signal_2 (GtkObject     *object,
			       GtkSignalFunc  func,
			       gpointer       func_data,
			       GtkArg        *args)
{
  GnomeClientSignal2  rfunc;
  
  rfunc = (GnomeClientSignal2) func;
  
  (* rfunc) ((GnomeClient*)object,
	     GTK_VALUE_BOOL (args[0]),
	     func_data);
}

/*****************************************************************************/
/* array helping functions - these function should be replaced by g_lists */

static void
array_insert_sm_client_id_arg (gchar ***array, gchar *client_id)
{
  gchar **new_array;
  gchar **ptr1;
  gchar **ptr2;
  gint    argc;

  g_return_if_fail (client_id != NULL);

  for (ptr1 = *array, argc = 0; *ptr1 ; ptr1++ , argc++) /* LOOP */;

  ptr2 = new_array = g_new (gchar*, argc + 3);
  ptr1 = *array;

  *ptr2 = *ptr1;
  ptr1++; ptr2++;
  *ptr2 = g_strdup (sm_client_id_arg_name);
  ptr2++;
  *ptr2 = g_strdup (client_id);
  ptr2++;
  for (; *ptr1; ptr1++, ptr2++)
    *ptr2 = *ptr1;
  *ptr2= NULL;

  g_free (*array);
  *array = new_array;
}

/*****************************************************************************/

static gchar **
array_init_from_arg (gint argc, gchar *argv[])
{
  gchar **array;
  gchar **ptr;

  if (argv == NULL)
    {
      g_return_val_if_fail (argc == 0, NULL);
      
      return NULL;
    }
  else
    {
      /* Now initialize the array.  */
      ptr = array = g_new (gchar *, argc + 1);
      
      for (; argc > 0 ; ptr++, argv++, argc--)
	*ptr= g_strdup (*argv);
      
      *ptr= NULL;
    }

  return array;
}

static gchar **
array_copy (gchar **source)
{
  gchar **array;
  gchar **ptr;
  gint    argc;
  
  if (source == NULL)
    return NULL;
  
  /* Count number of elements in source array. */
  for (ptr = source, argc = 0; *ptr; ptr++, argc++) /* LOOP */;
  
  /* Allocate memory for array. */
  array = g_new (gchar *, argc + 1);

  /* Copy the elements. */
  for (ptr = array; argc > 0 ; ptr++, source++, argc--)
    *ptr = g_strdup (*source);
  
  *ptr = NULL;
  
  return array;
}
