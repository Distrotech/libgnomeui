/* gnome-client.h - GNOME session management client support
 *
 * Copyright (C) 1998 Carsten Schaar
 *
 * Author: Carsten Schaar <nhadcasc@fs-maphy.uni-hannover.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#ifndef GNOME_CLIENT_H
#define GNOME_CLIENT_H

#include <unistd.h>
#include <sys/types.h>
#include <gtk/gtkobject.h>
#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-dialog.h>

BEGIN_GNOME_DECLS

#define GNOME_CLIENT(obj)           GTK_CHECK_CAST (obj, gnome_client_get_type (), GnomeClient)
#define GNOME_CLIENT_CLASS(klass)   GTK_CHECK_CLASS_CAST (klass, gnome_client_get_type (), GnomeClientClass)
#define GNOME_IS_CLIENT(obj)        GTK_CHECK_TYPE (obj, gnome_client_get_type ())

#define GNOME_CLIENT_CONNECTED(obj) (GNOME_CLIENT (obj)->smc_conn)

typedef struct _GnomeClient      GnomeClient;
typedef struct _GnomeClientClass GnomeClientClass;


typedef enum
{
  GNOME_INTERACT_NONE,
  GNOME_INTERACT_ERRORS,
  GNOME_INTERACT_ANY
} GnomeInteractStyle;

typedef enum
{
  GNOME_DIALOG_ERROR,
  GNOME_DIALOG_NORMAL
} GnomeDialogType;

typedef enum
{
  GNOME_SAVE_GLOBAL,
  GNOME_SAVE_LOCAL,
  GNOME_SAVE_BOTH
} GnomeSaveStyle;

typedef enum
{
  GNOME_RESTART_IF_RUNNING,
  GNOME_RESTART_ANYWAY,
  GNOME_RESTART_IMMEDIATELY,
  GNOME_RESTART_NEVER
} GnomeRestartStyle;

typedef enum
{
  GNOME_CLIENT_IDLE,
  GNOME_CLIENT_SAVING_PHASE_1,
  GNOME_CLIENT_WAITING_FOR_PHASE_2,
  GNOME_CLIENT_SAVING_PHASE_2,
  GNOME_CLIENT_FROZEN,
  GNOME_CLIENT_DISCONNECTED
} GnomeClientState;

typedef void (*GnomeInteractFunction) (GnomeClient     *client,
				       gint             key,
				       GnomeDialogType  dialog_type,
				       gpointer         data);

struct _GnomeClient
{
  GtkObject           object;

  /* general information about the connection to the session manager */
  gpointer            smc_conn;
  gint                input_id;

  /* client id of this client */
  gchar              *client_id;

  /* Previous client id of this client.  */
  gchar		     *previous_id;

  /* Prefix for per client configuration files.  */
  gchar              *config_prefix;

  /* Prefix for configuration files.  */
  gchar              *global_config_prefix;

  /* Static command line options.  */
  GList              *static_args;

  /* The following properties are predefined in the X session
     management protocol.  The entries marked with a 'x' are required
     by the session management protocol.  The entries marked with a
     's' are set automatically when creating a new gnome client.  */
  gchar             **clone_command;        /*[xs]*/
  gchar              *current_directory;    /*[  ]*/
  gchar             **discard_command;      /*[  ]*/
  GHashTable         *environment;          /*[  ]*/
  pid_t               process_id;           /*[ s]*/
  gchar              *program;              /*[xs]*/
  gchar             **resign_command;       /*[  ]*/
  gchar             **restart_command;      /*[xs]*/
  GnomeRestartStyle   restart_style;        /*[  ]*/
  gchar             **shutdown_command;     /*[  ]*/
  gchar              *user_id;              /*[xs]*/

  /* values sent with the last SaveYourself message */
  GnomeSaveStyle      save_style;
  GnomeInteractStyle  interact_style;
  gboolean            shutdown;
  gboolean            fast;

  /* other internal state information */
  GnomeClientState    state;
  gboolean            save_phase_2_requested;
  gboolean            save_successfull;
  gboolean            save_yourself_emitted;

  GSList             *interaction_keys;
};


struct _GnomeClientClass
{
  GtkObjectClass parent_class;

  gint (* save_yourself)      (GnomeClient        *client,
			       gint                phase,
			       GnomeSaveStyle      save_style,
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

/* Normally the master client is connected to the session manager
   automatically, when calling 'gnome_init'.  One can disable this
   automatic connect by calling this function. Using this function
   should definitely be an exception.  */
void         gnome_client_disable_master_connection (void);

/* Get the master session management client.  This master client gets
   a client id, that may be specified by the '--sm-client-id' command
   line option.  A master client will be generated by 'gnome-init'.
   If possible the master client will contact the session manager
   after command-line parsing is finished (except
   'gnome_client_disable_master_connection' was called).  The master
   client will also set the SM_CLIENT_ID property on the main window
   of your application.  

   Additional the master client gets some static arguments set
   automatically (see 'gnome_client_add_static_arg' for static
   arguments): 'gnome_init' sets all command line options, that are
   understood be 'gnome_init' as static arguments to the master
   client. */
GnomeClient *gnome_master_client 	         (void);

/* Get the cloned session management client.  This client gets a
   client id, that may be specified by the '--sm-cloned-id' command
   line option.  If no '--sm-cloned-id' command line was given, the
   '--sm-client-id' option will be evaluated.  This client is
   intentend to be used in combination with the
   'gnome_client_get_[global_]config_prefix' commands, in order to get
   the configuration of a cloned client.  This client must not be
   connected to a session-manager.*/
GnomeClient *gnome_cloned_client 	         (void);


/* Create a new session management client and try to connect to a
   session manager.  */
GnomeClient *gnome_client_new                    (void);

/* Create a new session management client.  */
GnomeClient *gnome_client_new_without_connection (void);


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

/* Get the config prefix for a client.  This config prefix depend on
   the program name and the client id of a client.  This function is
   useful, if your store configuration information of this client not
   only in the command line, but in a config file.  You may use the
   returned value as a prefix using the 'gnome_config_push_prefix'
   function.  */

gchar       *gnome_client_get_config_prefix        (GnomeClient *client);

/* Get the config prefix for a class of clients.  The returned value
   is more or less the same as the value returned by
   'gnome_client_get_config_prefix', but it does not include the
   client id.  */

gchar       *gnome_client_get_global_config_prefix (GnomeClient *client);


/* The follwing functions are used to set or unset some session
   management properties.  

   If you want to unset an array property, you have to specify a NULL
   argv, if you want to unset a string property you have to specify
   NULL as parameter.  You are not allowed to unset the properties
   marked as required (see above).  There is one exception to this
   rule: If you unset the clone command, the clone command will be set
   to the value of the restart command.

   The magic `--sm-client-id' option is automatically appended to the
   restart command.  Do not include this option in the restart command
   that you set.  */
void         gnome_client_set_clone_command      (GnomeClient *client, 
						  gint argc, gchar *argv[]);
void         gnome_client_set_current_directory  (GnomeClient *client,
						  const gchar *dir);
void         gnome_client_set_discard_command    (GnomeClient *client,
						  gint argc, gchar *argv[]);
void         gnome_client_set_environment        (GnomeClient *client,
						  const gchar *name,
						  const gchar *value);
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

/* The following function may be used, to add static arguments to the
   clone and restart command.  This function can be called more than
   once.  Every call appends the new arguments to the older ones.
   These arguments are inserted ahead the arguments set with the
   'gnome_client_set[clone|restart]_command'.  This list of arguments,
   given to this function must be end with 'NULL'.  */
void         gnome_client_add_static_arg (GnomeClient *client, ...);


/* The following function can be used, if you want that the
   'save_yourself' signal is emitted again, after all clients finished
   saveing.  */
void         gnome_client_request_phase_2        (GnomeClient *client);

/* The following function may be called during a "save_youself" handler
 * to request that a (modal) dialog is presented to the user. The session 
 * manager decides when the dialog is shown and it will not be shown
 * unless the interact_style == GNOME_INTERACT_ANY. A "Cancel Logout"
 * button will be added during a shutdown. */
void         gnome_client_save_any_dialog       (GnomeClient *client,
					         GnomeDialog *dialog);

/* The following function may be called during a "save_youself" handler
 * when an error has occured during the save. The session manager decides 
 * when the dialog is shown and it will not be shown when the interact_style
 * == GNOME_INTERACT_NONE.  A "Cancel Logout" button will be added 
 * during a shutdown. */
void         gnome_client_save_error_dialog      (GnomeClient *client,
					          GnomeDialog *dialog);

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
void         gnome_interaction_key_return        (gint     key,
						  gboolean cancel_shutdown);

/* Request the session manager to save the session in some way.  This
   can also be used to request a logout.  If IS_GLOBAL is true, then
   all clients are saved; otherwise only this client is saved.  */
void	     gnome_client_request_save (GnomeClient	       *client,
				        GnomeSaveStyle		save_style,
				        gboolean		shutdown,
				        GnomeInteractStyle	interact_style,
				        gboolean		fast,
				        gboolean		global);

/* This will force the underlying connection to the session manager to
   be flushed.  This is useful if you have some pending changes that
   you want to make sure get committed.  */
void         gnome_client_flush (GnomeClient *client);

void         gnome_client_init(void); /* For use by gnome-init upon
					 program startup */

END_GNOME_DECLS

#endif /* GNOME_CLIENT_H */
