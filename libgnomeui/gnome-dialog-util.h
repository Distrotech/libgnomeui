#ifndef GNOME_DIALOG_UTIL_H
#define GNOME_DIALOG_UTIL_H

/**** 
  Sugar functions to pop up dialogs in a hurry. These are probably
  too sugary, but they're used in gnome-app-util anyway so they may as
  well be here for others to use when there's no GnomeApp.
  The gnome-app-util functions are preferred if there's a GnomeApp
  to use them with.
  ****/

#include <libgnome/gnome-defs.h>
#include "gnome-types.h"

BEGIN_GNOME_DECLS

/* A little OK box */
void gnome_ok_dialog      (const gchar * message);

/* Operation failed fatally. In an OK dialog. */
void gnome_error_dialog   (const gchar * error);

/* Just a warning. */
void gnome_warning_dialog (const gchar * warning);

/* Look in gnome-types.h for the callback types. */

/* Ask a yes or no question, and call the callback when it's answered. */
void gnome_question_dialog        (const gchar * question,
			           GnomeReplyCallback callback, gpointer data);

void gnome_question_dialog_modal  (const gchar * question,
			           GnomeReplyCallback callback, gpointer data);

/* OK-Cancel question. */
void gnome_ok_cancel_dialog       (const gchar * message,
			           GnomeReplyCallback callback, gpointer data);

void gnome_ok_cancel_dialog_modal (const gchar * message,
				   GnomeReplyCallback callback, gpointer data);

/* Get a string. */
void gnome_request_string_dialog  (const gchar * prompt,
			           GnomeStringCallback callback, 
				   gpointer data);

/* Request a string, but don't echo to the screen. */
void gnome_request_password_dialog (const gchar * prompt,
				    GnomeStringCallback callback, 
				    gpointer data);


END_GNOME_DECLS

#endif
