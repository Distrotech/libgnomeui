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

/* The GtkWidget * return values were added in retrospect; sometimes
   you might want to connect to the "close" signal of the dialog, or
   something, the return value makes the functions more
   flexible. However, there is nothing especially guaranteed about
   these dialogs except that they will be dialogs, so don't count on
   anything. */


/* A little OK box */
GtkWidget * gnome_ok_dialog      (const gchar * message);

/* Operation failed fatally. In an OK dialog. */
GtkWidget * gnome_error_dialog   (const gchar * error);

/* Just a warning. */
GtkWidget * gnome_warning_dialog (const gchar * warning);

/* Look in gnome-types.h for the callback types. */

/* Ask a yes or no question, and call the callback when it's answered. */
GtkWidget * gnome_question_dialog        (const gchar * question,
					  GnomeReplyCallback callback, 
					  gpointer data);

GtkWidget * gnome_question_dialog_modal  (const gchar * question,
					  GnomeReplyCallback callback, 
					  gpointer data);

/* OK-Cancel question. */
GtkWidget * gnome_ok_cancel_dialog       (const gchar * message,
					  GnomeReplyCallback callback, 
					  gpointer data);

GtkWidget * gnome_ok_cancel_dialog_modal (const gchar * message,
					  GnomeReplyCallback callback, 
					  gpointer data);

/* Get a string. */
GtkWidget * gnome_request_string_dialog  (const gchar * prompt,
					  GnomeStringCallback callback, 
					  gpointer data);

/* Request a string, but don't echo to the screen. */
GtkWidget * gnome_request_password_dialog (const gchar * prompt,
					   GnomeStringCallback callback, 
					   gpointer data);


END_GNOME_DECLS

#endif
