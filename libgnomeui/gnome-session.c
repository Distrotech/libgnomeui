/* gnome-session.c - Gnome session client library.  */

#include <config.h>

#include <assert.h>

#ifdef HAVE_LIBSM
#include <X11/ICE/ICElib.h>
#endif /* HAVE_LIBSM */

#include "gnome.h"
#include "gnome-session.h"

#define PAD(n,P)  ((((n) % (P)) == 0) ? (n) : ((n) + (P) - ((n) % (P))))

#ifdef HAVE_LIBSM

typedef enum
{
  c_idle,
  c_save_yourself,
  c_shutdown_cancelled,
  c_interact_request,
  c_interact,
  c_die,
} client_state;

/* A struct of this type holds information about the client.  */
struct client_info
{
  client_state state;
  GnomeSaveFunction *saver;
  gpointer saver_client_data;
  GnomeDeathFunction *death;
  gpointer death_client_data;
  SmcConn connection;
};

/* The state for this client.  */
static struct client_info *info;

/* True if we've started listening to ICE.  */
static int ice_init = 0;

/* ICE connection tag as known by GDK event loop.  */
static guint ice_tag;


/* This is called when data is available on an ICE connection.  */
static void
process_ice_messages (gpointer client_data, gint source,
		      GdkInputCondition condition)
{
  IceProcessMessagesStatus status;
  IceConn connection = (IceConn) client_data;

  status = IceProcessMessages (connection, NULL, NULL);
  /* FIXME: handle case when status==closed.  */
}

/* This is called when a new ICE connection is made.  It arranges for
   the ICE connection to be handled via the event loop.  */
static void
new_ice_connection (IceConn connection, IcePointer client_data, Bool opening,
		    IcePointer *watch_data)
{
  if (opening)
    ice_tag = gdk_input_add (IceConnectionNumber (connection),
			     GDK_INPUT_READ, process_ice_messages,
			     (gpointer) connection);
  else
    gdk_input_remove (ice_tag);
}

static void
save_yourself (SmcConn connection, SmPointer client_data, int save_type,
	       Bool shutdown, int interact_style, Bool fast)
{
  int status;

  info->state = c_save_yourself;
  if (info->saver)
    status = info->saver (info->saver_client_data, save_type, shutdown,
			  interact_style, fast);
  else
    status = True;
  SmcSaveYourselfDone (connection, status);

  /* State might have changed while calling SAVER.  Assume that if it
     has changed, then we are ok.  */
  /* FIXME: Make it impossible for the user to interact.  */
  while (info->state == c_save_yourself
	 && ! gtk_main_iteration ())
    ;

  info->state = c_idle;
}

static void
die (SmcConn connection, SmPointer client_data)
{
  info->state = c_die;
  SmcCloseConnection (connection, 0, NULL);
  if (info->death)
    info->death (info->death_client_data);
  /* FIXME.  */
  exit (0);
}

static void
save_complete (SmcConn connection, SmPointer client_data)
{
  assert (info->state == c_save_yourself);
  info->state = c_idle;
}

static void
shutdown_cancelled (SmcConn connection, SmPointer client_data)
{
  assert (info->state == c_save_yourself || info->state == c_interact_request);
  info->state = c_shutdown_cancelled;
}

#endif /* HAVE_LIBSM */

char *
gnome_session_init (GnomeSaveFunction saver,
		    gpointer saver_client_data,
		    GnomeDeathFunction death,
		    gpointer death_client_data,
		    char *previous)
{
#ifdef HAVE_LIBSM
  SmcConn connection;
  unsigned long mask;
  SmcCallbacks callbacks;
  char *client_id;
  char buf[256];

  assert (! info);

  if (! ice_init)
    {
      IceAddConnectionWatch (new_ice_connection, NULL);
      ice_init = 1;
    }

  info = g_new (struct client_info, 1);
  info->state = c_idle;
  info->saver = saver;
  info->saver_client_data = saver_client_data;
  info->death = death;
  info->death_client_data = death_client_data;

  callbacks.save_yourself.callback = save_yourself;
  callbacks.save_yourself.client_data = NULL;
  callbacks.die.callback = die;
  callbacks.die.client_data = NULL;
  callbacks.save_complete.callback = save_complete;
  callbacks.save_complete.client_data = NULL;
  callbacks.shutdown_cancelled.callback = shutdown_cancelled;
  callbacks.shutdown_cancelled.client_data = NULL;

  mask = (SmcSaveYourselfProcMask | SmcDieProcMask
	  | SmcSaveCompleteProcMask | SmcShutdownCancelledProcMask);

  info->connection = SmcOpenConnection (NULL, NULL, SmProtoMajor, SmProtoMinor,
					mask, &callbacks, previous, &client_id,
					sizeof buf, buf);
  if (! info->connection)
    {
      free (info);
      info = NULL;
      return NULL;
    }

  return client_id;
#else
  return NULL;
#endif /* HAVE_LIBSM */
}

void
gnome_session_set_restart_style (GnomeRestartStyle style)
{
#ifdef HAVE_LIBSM
  SmProp prop, *proplist[1];
  SmPropValue val;
  char c = (char) style;

  if (! info)
    return;

  prop.name = SmRestartStyleHint;
  prop.type = SmCARD8;
  prop.num_vals = 1;
  prop.vals = &val;
  val.length = 1;
  val.value = &style;
  proplist[0] = &prop;
  SmcSetProperties (info->connection, 1, proplist);
#endif /* HAVE_LIBSM */
}

void
gnome_session_set_current_directory (char *dir)
{
#ifdef HAVE_LIBSM
  SmProp prop, *proplist[1];
  SmPropValue val;
  int len;

  if (! info)
    return;

  prop.name = SmCurrentDirectory;
  prop.type = SmARRAY8;
  prop.num_vals = 1;
  prop.vals = &val;
  val.length = strlen (dir) + 1;
  val.value = dir;
  proplist[0] = &prop;
  SmcSetProperties (info->connection, 1, proplist);
#endif /* HAVE_LIBSM */
}

void
gnome_session_set_program (char *name)
{
#ifdef HAVE_LIBSM
  SmProp prop, *proplist[1];
  SmPropValue val;
  int len;

  if (! info)
    return;

  prop.name = SmProgram;
  prop.type = SmARRAY8;
  prop.num_vals = 1;
  prop.vals = &val;
  val.length = strlen (name) + 1;
  val.value = name;
  proplist[0] = &prop;
  SmcSetProperties (info->connection, 1, proplist);
#endif /* HAVE_LIBSM */
}

/* Helper.  */
static void
set_prop_from_argv (char *property, int argc, char *argv[])
{
#ifdef HAVE_LIBSM
  SmProp prop, *proplist[1];
  SmPropValue *vals;
  int i;

  if (! info)
    return;

  prop.name = property;
  prop.type = SmLISTofARRAY8;

  vals = (SmPropValue *) malloc (argc * sizeof (SmPropValue));
  for (i = 0; i < argc; ++i)
    {
      vals[i].length = strlen (argv[i]) + 1;
      vals[i].value = argv[i];
    }

  prop.num_vals = argc;
  prop.vals = vals;
  proplist[0] = &prop;

  SmcSetProperties (info->connection, 1, proplist);

  free (vals);
#endif /* HAVE_LIBSM */
}

void
gnome_session_set_discard_command (int argc, char *argv[])
{
  set_prop_from_argv (SmDiscardCommand, argc, argv);
}

void
gnome_session_set_restart_command (int argc, char *argv[])
{
  set_prop_from_argv (SmRestartCommand, argc, argv);
}

void
gnome_session_set_clone_command (int argc, char *argv[])
{
  set_prop_from_argv (SmCloneCommand, argc, argv);
}

void
gnome_session_set_initialization_command (int argc, char *argv[])
{
  set_prop_from_argv (GNOME_SM_INIT_COMMAND, argc, argv);
}

#ifdef HAVE_LIBSM

static void
interact (SmcConn connection, SmPointer client_data)
{
  assert (info->state == c_interact_request);
  info->state = c_interact;
}

#endif /* HAVE_LIBSM */

int
gnome_session_request_interaction (GnomeDialogType type)
{
#ifdef HAVE_LIBSM
  int status;

  assert (info->state == c_save_yourself);
  info->state = c_interact_request;
  status = SmcInteractRequest (info->connection, type,
			       interact, NULL);
  if (! status)
    return 0;

  /* FIXME: Make it impossible for the user to interact.  */
  while (info->state == c_interact_request
	 && ! gtk_main_iteration ())
    ;

  return (info->state == c_interact);
#else
  return 1;
#endif /* HAVE_LIBSM */
}

void
gnome_session_interaction_done (int shutdown)
{
#ifdef HAVE_LIBSM
  assert (info->state == c_interact);
  info->state = c_save_yourself;
  SmcInteractDone (info->connection, shutdown);
#endif /* HAVE_LIBSM */
}
