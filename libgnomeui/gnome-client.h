/* gnome-client.h - Session management support for Gnome apps. */
/* Carsten Schaar <nhadcasc@fs-maphy.uni-hannover.de */

#ifndef GNOME_CLIENT_H
#define GNOME_CLIENT_H

#include <unistd.h>

/* If we don't have libSM, then we just define bogus values for the
   things we need from SMlib.h.  The values don't matter because this
   whole library is just stubbed out in this case.  */
#ifdef HAVE_LIBSM
#include <X11/SM/SMlib.h>
#else /* HAVE_LIBSM */
#define SmInteractStyleNone     0
#define SmInteractStyleErrors   1
#define SmInteractStyleAny      2
#define SmDialogError           0
#define SmDialogNormal          1
#define SmSaveGlobal            0
#define SmSaveLocal             1
#define SmSaveBoth              2
#define SmRestartIfRunning      0
#define SmRestartAnyway         1
#define SmRestartImmediately    2
#define SmRestartNever          3
#endif /* !HAVE_LIBSM */

BEGIN_GNOME_DECLS

#ifndef HAVE_LIBSM
typedef struct _SmcConn *SmcConn;
#endif /* !HAVE_LIBSM */


#define GNOME_CLIENT(obj)           GTK_CHECK_CAST (obj, gnome_client_get_type (), GnomeClient)
#define GNOME_CLIENT_CLASS(klass)   GTK_CHECK_CLASS_CAST (klass, gnome_client_get_type (), GnomeClientClass)
#define GNOME_IS_CLIENT(obj)        GTK_CHECK_TYPE (obj, gnome_client_get_type ())

#define GNOME_CLIENT_CONNECTED(obj) (GNOME_CLIENT (obj)->smc_conn)

typedef struct _GnomeClient      GnomeClient;
typedef struct _GnomeClientClass GnomeClientClass;

/* Some redefinitions so we can use familiar names.  */
typedef enum
{
  GNOME_INTERACT_NONE = SmInteractStyleNone,
  GNOME_INTERACT_ERRORS = SmInteractStyleErrors,
  GNOME_INTERACT_ANY = SmInteractStyleAny
} GnomeInteractStyle;

typedef enum
{
  GNOME_DIALOG_ERROR = SmDialogError,
  GNOME_DIALOG_NORMAL = SmDialogNormal
} GnomeDialogType;

typedef enum
{
  GNOME_SAVE_GLOBAL = SmSaveGlobal,
  GNOME_SAVE_LOCAL = SmSaveLocal,
  GNOME_SAVE_BOTH = SmSaveBoth
} GnomeSaveStyle;

typedef enum
{
  GNOME_RESTART_IF_RUNNING = SmRestartIfRunning,
  GNOME_RESTART_ANYWAY = SmRestartAnyway,
  GNOME_RESTART_IMMEDIATELY = SmRestartImmediately,
  GNOME_RESTART_NEVER = SmRestartNever
} GnomeRestartStyle;

typedef enum
{
  GNOME_CLIENT_IDLE,
  GNOME_CLIENT_SAVING,
  GNOME_CLIENT_WAITING
} GnomeClientState;

typedef void (*GnomeInteractFunction) (GnomeClient     *client,
				       gint             key,
				       GnomeDialogType  dialog_type,
				       gpointer         data);

struct _GnomeClient
{
  GtkObject           object;

  /* general information about the connection to the session manager */
  SmcConn             smc_conn;
  gint                input_id;

  /* client id of this client */
  gchar              *client_id;

  /* Previous client id of this client.  */
  gchar		     *previous_id;

  /* The following properties are predefined in the X session
     management protocol.  The entries marked with a 'x' are required
     by the session management protocol.  The entries marked with a
     's' are set automatically when creating a new gnome client.  */
  gchar             **clone_command;        /*[xs]*/
  gchar              *current_directory;    /*[  ]*/
  gchar             **discard_command;      /*[  ]*/
  gchar             **environment;          /*[  ]*/
  pid_t               process_id;           /*[ s]*/
  gchar              *program;              /*[xs]*/
  gchar             **resign_command;       /*[  ]*/
  gchar             **restart_command;      /*[xs]*/
  GnomeRestartStyle   restart_style;        /*[  ]*/
  gchar             **shutdown_command;     /*[  ]*/
  gchar              *user_id;              /*[xs]*/

  /* values sent with the last SaveYourself message */
  GnomeSaveStyle      save_type;
  gint                shutdown;
  GnomeInteractStyle  interact_style;
  gint                fast;
  gint                phase;

  /* other internal state information */
  GnomeClientState    state;
  gint                save_phase_2_requested;
  gint                save_successfull;
  gint                number_of_save_signals;

  gint                number_of_interact_requests;
};


struct _GnomeClientClass
{
  GtkObjectClass parent_class;

  gint (* save_yourself)      (GnomeClient        *client,
			       gint                phase,
			       GnomeRestartStyle   save_style,
			       gint                shutdown,
			       GnomeInteractStyle  interact_style,
			       gint                fast);
  void (* die)                (GnomeClient        *client);
  void (* save_complete)      (GnomeClient        *client);
  void (* shutdown_cancelled) (GnomeClient        *client);

  void (* connect)            (GnomeClient        *client,
			       gint                restarted);
  void (* disconnect)         (GnomeClient        *client);
};


guint        gnome_client_get_type (void);

/* Create a new session management client and try to connect to a
   session manager.  */
GnomeClient *gnome_client_new                    (gint argc, gchar *argv[]);


/* Create a new session management client.  */
GnomeClient *gnome_client_new_without_connection (gint argc, gchar *argv[]);


/* Try to connect to a session manager.  If the client was created
   with a valid session management client id, we will try to connect
   to the manager with this old id.  If the connection was successfull,
   the 'connect' signal will be emitted, after some default properties
   have been sent to the session manager.  */
void         gnome_client_connect                (GnomeClient *client);


/* Disconnect from the session manager.  After disconnecting, the
   'disconnect' signal will be emitted.  */
void         gnome_client_disconnect             (GnomeClient *client);


/* Set the client id.  This is only possible, if the client is not
   connected to a session manager.  */
void         gnome_client_set_id                 (GnomeClient *client,
						  const gchar *client_id);

/* Get the client id of a session management client object.  If this
   object has never been connected to a session manager and a client
   id hasn't been set, this function return 'NULL'.  */
gchar       *gnome_client_get_id                 (GnomeClient *client);

/* Get the client id from the last session.  If this object was not
   recreated from a previous session, returns NULL.  This is useful
   because a client might choose to store configuration information
   in a location based on the session id; this old session id must be
   recalled later to find the information.  */
gchar       *gnome_client_get_previous_id        (GnomeClient *client);


/* The follwing functions are used to set or unset some session
   management properties.  

   If you want to unset an array property, you have to specify a NULL
   argv, if you want to unset a string property you have to specify
   NULL as parameter.  You are not allowed to unset the properties
   marked as required (see above).  There is one exception to this
   rule: If you unset the clone command, the clone command will be set
   to the value of the restart command but without a '--sm-client-id'
   command line option.

   If you set the restart command without specifing a '--sm-client-id'
   option, this option will be inserted automatically, if the client
   is connected to a session manager.  */
void         gnome_client_set_clone_command      (GnomeClient *client, 
						  gint argc, gchar *argv[]);
void         gnome_client_set_current_directory  (GnomeClient *client,
						  const gchar *dir);
void         gnome_client_set_discard_command    (GnomeClient *client,
						  gint argc, gchar *argv[]);
void         gnome_client_set_environment        (GnomeClient *client,
						  gint argc, gchar *argv[]);
void         gnome_client_set_process_id         (GnomeClient *client, 
						  pid_t pid);
void         gnome_client_set_program            (GnomeClient *client, 
						  const gchar *program);
void         gnome_client_set_restart_command    (GnomeClient *client,
						  gint argc, gchar *argv[]);
void         gnome_client_set_resign_command     (GnomeClient *client,
						  gint argc, gchar *argv[]);
void         gnome_client_set_restart_style      (GnomeClient *client,
						  GnomeRestartStyle style);
void         gnome_client_set_shutdown_command   (GnomeClient *client,
						  gint argc, gchar *argv[]);
void         gnome_client_set_user_id            (GnomeClient *client,
						  const gchar *user_id);


/* The following function can be used, if you want that the
   'save_yourself' signal is emitted again, after all clients finished
   saveing.  */
void         gnome_client_request_phase_2        (GnomeClient *client);


/* Use the following functions, if you want to interact with the user
   will saveing his data.  If the session manager decides that it's
   out turn to interact, 'func' will be called with the specified
   arguments and a special 'GnomeInteractionKey'.  The session manager
   will block other clients from interacting until this key is
   returned with 'gnome_interaction_key_return'.  */

void         gnome_client_request_interaction    (GnomeClient *client,
						  GnomeDialogType dialog,
						  GnomeInteractFunction func,
						  gpointer client_data);

void         gnome_client_request_interaction_interp (GnomeClient *client,
						      GnomeDialogType dialog,
						      GtkCallbackMarshal func,
						      gpointer data,
						      GtkDestroyNotify destroy);

/* 'gnome_interaction_key_return' is used to tell gnome, that you are
   finished with interaction */
void         gnome_interaction_key_return        (gint key,
						  gint cancel_shutdown);

/* Request the session manager to save the session in some way.  This
   can also be used to request a logout.  If IS_GLOBAL is true, then
   all clients are saved; otherwise only this client is saved.  */
void         gnome_client_request_save (GnomeClient        *client,
					GnomeSaveStyle      save_style,
					int /* bool */      shutdown,
					GnomeInteractStyle  interact_style,
					int /* bool */      fast,
					int /* bool */      global);

END_GNOME_DECLS

#endif /* GNOME_CLIENT_H */
