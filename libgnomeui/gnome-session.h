/* gnome-session.h - Session management support for Gnome apps.  */

#ifndef GNOME_SESSION_H
#define GNOME_SESSION_H

BEGIN_GNOME_DECLS

/* If we don't have libSM, then we just define bogus values for the
   things we need from SMlib.h.  The values don't matter because this
   whole library is just stubbed out in this case.  */
#ifdef HAVE_LIBSM
#include <X11/SM/SMlib.h>
#else /* HAVE_LIBSM */
#define SmInteractStyleNone	0
#define SmInteractStyleErrors	1
#define SmInteractStyleAny	2
#define SmDialogError		0
#define SmDialogNormal		1
#define SmSaveGlobal	0
#define SmSaveLocal	1
#define SmSaveBoth	2
#define SmRestartIfRunning	0
#define SmRestartAnyway		1
#define SmRestartImmediately	2
#define SmRestartNever		3
#endif /* HAVE_LIBSM */

/* Name of the initialization command property.  */
#define GNOME_SM_INIT_COMMAND "InitializationCommand"


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


/* A function of this type is called when the state should be saved.
   The function should return 1 if the save was successful, 0
   otherwise.  */
typedef int GnomeSaveFunction (gpointer client_data,
			       GnomeSaveStyle save_style,
			       int /*bool*/ is_shutdown,
			       GnomeInteractStyle interact_style,
			       int /*bool*/ is_fast);

/* A function of this type is called when the session manager requests
   us to exit.  State will have already been saved.  If this function
   returns, the handler will exit with status 0.  */
typedef void GnomeDeathFunction (gpointer client_data);



/* Initialize.  This returns the current client id (which should be
   saved for restarting), or NULL on error.  If this client has been
   restarted from a saved session, the old client id should be passed
   as PREVIOUS_ID.  Otherwise NULL should be used.  The return value
   is malloced and should be freed by the caller when appropriate.  */
char *gnome_session_init (GnomeSaveFunction saver,
			  gpointer saver_client_data,
			  GnomeDeathFunction death,
			  gpointer death_client_data,
			  char *previous_id);

/* Set the restart style.  Default is GNOME_RESTART_IF_RUNNING.  */
void gnome_session_set_restart_style (GnomeRestartStyle style);

/* Set the current directory property.  */
void gnome_session_set_current_directory (char *dir);

/* Set the discard command.  This is a command that can clean up
   after a local save.  */
void gnome_session_set_discard_command (int argc, char *argv[]);

/* Set the restart command.  */
void gnome_session_set_restart_command (int argc, char *argv[]);

/* Set the clone command.  This is like the restart command but
   doesn't preserve session id info.  */
void gnome_session_set_clone_command (int argc, char *argv[]);

/* Set the initialization command.  This is a Gnome-specific
   extension.  When a session starts, each initialization command gets
   run exactly once (duplicates are trimmed).  */
void gnome_session_set_initialization_command (int argc, char *argv[]);

/* Set the program name.  The argument should just be ARGV[0].  */
void gnome_session_set_program (char *name);

/* Request the interaction token.  This will return 1 when this client
   has the interaction token.  If it returns 0, then the shutdown has
   been cancelled, and so the interaction should not take place.  */
int gnome_session_request_interaction (GnomeDialogType dialog_type);

/* Release the interaction token.  If SHUTDOWN is true, then the
   shutdown will be cancelled.  */
void gnome_session_interaction_done (int shutdown);


END_GNOME_DECLS

#endif /* GNOME_SESSION_H */
