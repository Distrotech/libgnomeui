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

#define GNOME_TYPE_CLIENT            (gnome_client_get_type ())
#define GNOME_CLIENT(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CLIENT, GnomeClient))
#define GNOME_CLIENT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CLIENT, GnomeClientClass))
#define GNOME_IS_CLIENT(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CLIENT))
#define GNOME_IS_CLIENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE (((klass), GNOME_TYPE_CLIENT))

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
  GNOME_CLIENT_DISCONNECTED,
  GNOME_CLIENT_REGISTERING
} GnomeClientState;

typedef enum
{
  GNOME_CLIENT_IS_CONNECTED= 1 << 0,
  GNOME_CLIENT_RESTARTED   = 1 << 1,
  GNOME_CLIENT_RESTORED    = 1 << 2
} GnomeClientFlags;


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

  /* Prefix for per save configuration files.  */
  gchar              *config_prefix;

  /* Prefix for app configuration files.  */
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

  gboolean (* save_yourself)  (GnomeClient        *client,
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

/* For internal use by the gnome-libs: */
guint        gnome_client_get_type (void);
void         gnome_client_init(void); 

/* Get the master session management client.  This master client gets
   a client id, that may be specified by the '--sm-client-id' command
   line option.  A master client will be generated by 'gnome-init'.
   If possible the master client will contact the session manager
   after command-line parsing is finished (unless
   'gnome_client_disable_master_connection' was called).  The master
   client will also set the SM_CLIENT_ID property on the client leader
   window of your application.  

   Additionally, the master client gets some static arguments set
   automatically (see 'gnome_client_add_static_arg' for static
   arguments): 'gnome_init' passes all the command line options which 
   are recognised by gtk as static arguments to the master client. */
GnomeClient *gnome_master_client 	         (void);

/* Get the config prefix for a client. This config prefix provides a
   suitable place to store any details about the state of the client 
   which can not be described using the app's command line arguments (as
   set in the restart command). You may push the returned value using
   'gnome_config_push_prefix' and read or write any values you require. */
gchar*       gnome_client_get_config_prefix        (GnomeClient *client);

/* Get the config prefix that will be returned by the previous function
   for clients which have NOT been restarted or cloned (i.e. for clients 
   started by the user without `--sm-' options). This config prefix may be
   used to write the user's preferred config for these "new" clients.

   You could also use this prefix as a place to store and retrieve config
   details that you wish to apply to ALL instances of the app. However, 
   this practice limits the users freedom to configure each instance in
   a different way so it should be used with caution. */
gchar*       gnome_client_get_global_config_prefix (GnomeClient *client);

/* Set the value used for the global config prefix. The config prefixes 
   returned by gnome_client_get_config_prefix are formed by extending
   this prefix with an unique identifier.
   
   The global config prefix defaults to a name based on the name of
   the executable. This function allows you to set it to a different
   value. It should be called BEFORE retrieving the config prefix for
   the first time. Later calls will be ignored.

   For example, setting a global config prefix of "/app.d/session/"
   would ensure that all your session save files or directories would
   be gathered together into the app.d directory. */
void         gnome_client_set_global_config_prefix (GnomeClient *client,
						    const gchar* prefix);

/* Returns some flags, that give additional information about this
   client.  Right now, the following flags are supported:
  
   - GNOME_CLIENT_IS_CONNECTED: The client is connected to a session
     manager (It's the same information like using
     GNOME_CLIENT_CONNECTED).
   
   - GNOME_CLIENT_RESTARTED: The client has been restarted, i. e. it
     has been running with the same client id before.
     
   - GNOME_CLIENT_RESTORED: This flag is only used for the master
     client.  It indicates, that there may be a configuraion file from
     which the clients state should be restored (using the
     gnome_client_get_config_prefix call).  */
   
GnomeClientFlags gnome_client_get_flags            (GnomeClient *client);

/* The following functions are used to set or unset the session
   management properties that are used by the session manager to determine 
   how to handle the app. If you want to unset an array property, you 
   have to specify a NULL argv, if you want to unset a string property 
   you have to specify NULL as parameter.

   The `--sm-' options are automatically added as the first arguments
   to the restart and clone commands and you should not try to set them. */

/* The session manager usually only restarts clients which were running
   when the session was last saved. You can set the restart style to make
   the manager restart the client:
   -  at the start of every session (GNOME_RESTART_ANYWAY) or 
   -  whenever the client dies (GNOME_RESTART_IMMEDIATELY) or 
   -  never (GNOME_RESTART_NEVER). */
void         gnome_client_set_restart_style      (GnomeClient *client,
						  GnomeRestartStyle style);

/* The gnome-session manager includes an extension to the protocol which
   allows the order in which clients are started up to be organised into
   a number of run levels. This function may be used to inform the
   gnome-session manager of where this client should appear in this
   run level ordering. The priority runs from 0 (started first) to 99
   (started last) and defaults to 50. Users may override the value
   that is suggested to gnome-session by calling this function. */
void         gnome_client_set_priority      (GnomeClient *client,
					     guint priority);

/* Executing the restart command on the local host should reproduce
   the state of the client at the time of the session save as closely
   as possible. Saving config info under the gnome_client_get_config_prefix
   is generally useful. */
void         gnome_client_set_restart_command    (GnomeClient *client,
						  gint argc, gchar *argv[]);

/* This function provides an alternative way of adding new arguments
   to the restart command. The arguments are placed before the arguments
   specified by 'gnome_client_set_restart_command' and after the arguments
   recognised by gtk specified by the user on the original command line. */
void         gnome_client_add_static_arg (GnomeClient *client, ...);

/* Executing the discard command on the local host should delete the
   information saved as part of the session save that was in progress
   when the discard command was set. For example:

     gchar *prefix = gnome_client_get_config_prefix (client);
     gchar *argv[] = { "rm", "-r", NULL };
     argv[2] = gnome_config_get_real_path (prefix);
     gnome_client_set_discard_command (client, 3, argv); */
void         gnome_client_set_discard_command    (GnomeClient *client,
						  gint argc, gchar *argv[]);

/* These two commands are used by clients that use the GNOME_RESTART_ANYWAY
   restart style to to undo their effects (these clients usually perform 
   initialisation functions and leave effects behind after they die).
   The shutdown command simply undoes the effects of the client. It is 
   executed during a normal logout. The resign command combines the effects 
   of a shutdown command and a discard command. It is executed when the user
   decides that the client should cease to be restarted. */
void         gnome_client_set_resign_command     (GnomeClient *client,
						  gint argc, gchar *argv[]);
void         gnome_client_set_shutdown_command   (GnomeClient *client,
						  gint argc, gchar *argv[]);

/* All the preceeding session manager commands are executed in the directory 
   and environment set up by these two commands: */
void         gnome_client_set_current_directory  (GnomeClient *client,
						  const gchar *dir);
void         gnome_client_set_environment        (GnomeClient *client,
						  const gchar *name,
						  const gchar *value);

/* These four values are set automatically to the values required by the 
   session manager and you should not need to change them. The clone
   command is directly copied from the restart command. */
void         gnome_client_set_clone_command      (GnomeClient *client, 
						  gint argc, gchar *argv[]);
void         gnome_client_set_process_id         (GnomeClient *client, 
						  pid_t pid);
void         gnome_client_set_program            (GnomeClient *client, 
						  const gchar *program);
void         gnome_client_set_user_id            (GnomeClient *client,
						  const gchar *user_id);

/* The following function may be called during a "save_youself" handler
   to request that a (modal) dialog is presented to the user. The session 
   manager decides when the dialog is shown and it will not be shown
   unless the interact_style == GNOME_INTERACT_ANY. A "Cancel Logout"
   button will be added during a shutdown. */
void         gnome_client_save_any_dialog       (GnomeClient *client,
					         GnomeDialog *dialog);

/* The following function may be called during a "save_youself" handler
   when an error has occured during the save. The session manager decides 
   when the dialog is shown and it will not be shown when the interact_style
   == GNOME_INTERACT_NONE.  A "Cancel Logout" button will be added 
   during a shutdown. */
void         gnome_client_save_error_dialog      (GnomeClient *client,
					          GnomeDialog *dialog);

/* Request the session managaer to emit the "save_yourself" signal for 
   a second time after all the clients in the session have ceased 
   interacting with the user and entered an idle state. This might be 
   useful if your app manages other apps and requires that they are in 
   an idle state before saving its final data. */
void         gnome_client_request_phase_2        (GnomeClient *client);

/* Request the session manager to save the session in some way. The
   arguments correspond with the arguments passed to the "save_yourself"
   signal handler.

   The save_style indicates whether the save should affect data accessible 
   to other users (GNOME_SAVE_GLOBAL) or only the state visible to 
   the current user (GNOME_SAVE_LOCAL) or both. Setting shutdown to 
   TRUE will initiate a logout. The interact_style specifies which kinds 
   of interaction will be available. Setting fast to TRUE will limit the 
   save to setting the session manager properties plus any essential data. 
   Setting the value of global to TRUE will request that all the other 
   apps in the session do a save as well. A global save is mandatory when 
   doing a shutdown. */
void	     gnome_client_request_save (GnomeClient	       *client,
				        GnomeSaveStyle		save_style,
				        gboolean		shutdown,
				        GnomeInteractStyle	interact_style,
				        gboolean		fast,
				        gboolean		global);

/* Flush the underlying connection to the session manager.  This is 
   useful if you have some pending changes that you want to make sure 
   get committed.  */
void         gnome_client_flush (GnomeClient *client);

/* Normally the master client is connected to the session manager
   automatically, when calling 'gnome_init'.  One can disable this
   automatic connect by calling this function. Using this function
   should definitely be an exception.  */
void         gnome_client_disable_master_connection (void);

/* Create a new session management client and try to connect to a
   session manager. This is useful if you are acting as a proxy for
   other executables that do not communicate with session manager.  */
GnomeClient *gnome_client_new                    (void);

/* Create a new session management client.  */
GnomeClient *gnome_client_new_without_connection (void);

/* Try to connect to a session manager.  If the client was created
   with a valid session management client id, we will try to connect
   to the manager with this old id.  If the connection was successfull,
   the "connect" signal will be emitted, after some default properties
   have been sent to the session manager.  */
void         gnome_client_connect                (GnomeClient *client);

/* Disconnect from the session manager.  After disconnecting, the
   "disconnect" signal will be emitted. */
void         gnome_client_disconnect             (GnomeClient *client);

/* Set the client id.  This is only possible, if the client is not
   connected to a session manager.  */
void         gnome_client_set_id                 (GnomeClient *client,
						  const gchar *client_id);

/* Get the client id of a session management client object.  If this
   object has never been connected to a session manager and a client
   id hasn't been set, this function return 'NULL'.  */
gchar*       gnome_client_get_id                 (GnomeClient *client);

/* Get the client id from the last session.  If this client was not
   recreated from a previous session, returns NULL. The session 
   manager tries to maintain the same id from one session to another. */
gchar*       gnome_client_get_previous_id        (GnomeClient *client);

/* Get the cloned session management client. This function is included 
   for backwards compatibility and always returns NULL. */
GnomeClient *gnome_cloned_client 	         (void);

/* Use the following functions, if you want to interact with the user
   during a "save_yourself" handler without being restricted to using 
   the dialog based commands gnome_client_save_[any/error]_dialog.  
   If and when the session manager decides that it's the app's turn to 
   interact then 'func' will be called with the specified arguments and 
   a unique 'GnomeInteractionKey'. The session manager will block other 
   clients from interacting until this key is returned with 
   'gnome_interaction_key_return'.  */
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

END_GNOME_DECLS

#endif /* GNOME_CLIENT_H */
