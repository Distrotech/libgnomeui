#ifndef GNOME_APP_UTIL_H
#define GNOME_APP_UTIL_H
/****
  Convenience UI functions for use with GnomeApp 
  ****/

/* Take me out after testing. */
#include <gnome.h>

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

/* What to show in the status bar while not showing anything else.
   There can only be one of these at a time. You might use it 
   to display the currently selected item, for example. */
void gnome_app_set_status (GnomeApp * app, const gchar * status);


/* =============================================
  Simple messages and questions to the user; use dialogs for now, but
   ultimately can use either dialogs or statusbar. Supposed to be a
   "semantic markup" kind of thing. */

/* A simple message, in an OK dialog or the status bar. Requires
   confirmation from the user before it goes away. */
void gnome_app_message (GnomeApp * app, const gchar * message);

/* Flash the message in the statusbar for a few moments; if no
   statusbar, do nothing (?). For trivial little status messages,
   e.g. "Auto saving..." */
void gnome_app_flash (GnomeApp * app, const gchar * flash);

/* An important fatal error; if it appears in the statusbar, 
   it might gdk_beep() and require acknowledgement. */
void gnome_app_error (GnomeApp * app, const gchar * error);

/* A not-so-important error, but still marked better than a flash */
void gnome_app_warning (GnomeApp * app, const gchar * warning);

/* =============================================================== */

/* For these, the user can cancel without ever calling the callback,
   e.g. by clicking the dialog's "close" button. So don't count on the
   callback ever being called. */

/* reply is one of the above defines. */
typedef void (* GnomeAppReplyFunc)(gint reply, gpointer data);

/* Ask a yes or no question, and call the callback when it's answered. */
void gnome_app_question (GnomeApp * app, const gchar * question,
			 GnomeAppReplyFunc callback, gpointer data);

void gnome_app_question_modal (GnomeApp * app, const gchar * question,
			       GnomeAppReplyFunc callback, gpointer data);

/* OK-Cancel question. */
void gnome_app_ok_cancel (GnomeApp * app, const gchar * message,
			  GnomeAppReplyFunc callback, gpointer data);

void gnome_app_ok_cancel_modal (GnomeApp * app, const gchar * message,
				GnomeAppReplyFunc callback, gpointer data);

/* string is a g_malloc'd string which should be freed, or NULL if the
   user cancelled. */
typedef void (* GnomeAppStringFunc)(gchar * string, gpointer data); 

/* Get a string. */
void gnome_app_request_string (GnomeApp * app, const gchar * prompt,
			       GnomeAppStringFunc callback, gpointer data);

/* Request a string, but don't echo to the screen. */
void gnome_app_request_password (GnomeApp * app, const gchar * prompt,
				 GnomeAppStringFunc callback, gpointer data);


/* ========================================================== */

/* Returns the fraction of the task done at a given time. */
typedef gfloat (* GnomeAppProgressFunc) (gpointer data);
/* What to call if the operation is canceled. */
typedef void (* GnomeAppProgressCancelFunc) (gpointer data);
/* A key used to refer to the callback. DO NOT FREE.
   It's freed by _done or when the progress is canceled. */
typedef gpointer GnomeAppProgressKey;

/* These will be a progress bar dialog or maybe dots in the statusbar. */

/* Call percentage_cb every interval to set the progress indicator.
   Both callbacks get the data arg. */
GnomeAppProgressKey 
gnome_app_progress_timeout (GnomeApp * app, 
			    const gchar * description,
			    guint32 interval, 
			    GnomeAppProgressFunc percentage_cb,
			    GnomeAppProgressCancelFunc cancel_cb,
			    gpointer data);

/* Just create a callback key; it then has to be updated 
   with _update() */
GnomeAppProgressKey 
gnome_app_progress_manual (GnomeApp * app, 
			   const gchar * description,
			   GnomeAppProgressCancelFunc cancel_cb,
			   gpointer data);

/* Only makes sense with manual. */
void gnome_app_progress_update (GnomeAppProgressKey key, gfloat percent);

/* Call this when the progress meter should go away. Automatically 
   called if progress is cancelled. */
void gnome_app_progress_done (GnomeAppProgressKey key);



END_GNOME_DECLS

#endif
