#ifndef GNOME_DIALOG_UTIL_H
#define GNOME_DIALOG_UTIL_H

/**** 
  Sugar functions to pop up dialogs in a hurry. These are probably
  too sugary, but they're used in gnome-app-util anyway so they may as
  well be here for others to use when there's no GnomeApp.
  The gnome-app-util functions are preferred if there's a GnomeApp
  to use them with, because they allow configurable statusbar messages
  instead of a dialog.
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
GtkWidget * gnome_ok_dialog             (const gchar * message);
GtkWidget * gnome_ok_dialog_parented    (const gchar * message,
					 GtkWindow * parent);

/* Operation failed fatally. In an OK dialog. */
GtkWidget * gnome_error_dialog          (const gchar * error);
GtkWidget * gnome_error_dialog_parented (const gchar * error,
					 GtkWindow * parent);

/* Just a warning. */
GtkWidget * gnome_warning_dialog           (const gchar * warning);
GtkWidget * gnome_warning_dialog_parented  (const gchar * warning,
					    GtkWindow * parent);

/* Look in gnome-types.h for the callback types. */

/* Ask a yes or no question, and call the callback when it's answered. */
GtkWidget * gnome_question_dialog                 (const gchar * question,
						   GnomeReplyCallback callback,
						   gpointer data);

GtkWidget * gnome_question_dialog_parented        (const gchar * question,
						   GnomeReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent);

GtkWidget * gnome_question_dialog_modal           (const gchar * question,
						   GnomeReplyCallback callback,
						   gpointer data);

GtkWidget * gnome_question_dialog_modal_parented  (const gchar * question,
						   GnomeReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent);


/* OK-Cancel question. */
GtkWidget * gnome_ok_cancel_dialog                (const gchar * message,
						   GnomeReplyCallback callback,
						   gpointer data);

GtkWidget * gnome_ok_cancel_dialog_parented       (const gchar * message,
						   GnomeReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent);

GtkWidget * gnome_ok_cancel_dialog_modal          (const gchar * message,
						   GnomeReplyCallback callback,
						   gpointer data);

GtkWidget * gnome_ok_cancel_dialog_modal_parented (const gchar * message,
						   GnomeReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent);

#ifndef GNOME_EXCLUDE_DEPRECATED
/* This function is deprecated; use gnome_request_dialog() instead. */
GtkWidget * gnome_request_string_dialog           (const gchar * prompt,
						   GnomeStringCallback callback,
						   gpointer data);

/* This function is deprecated; use gnome_request_dialog() instead. */
GtkWidget * gnome_request_string_dialog_parented  (const gchar * prompt,
						   GnomeStringCallback callback,
						   gpointer data,
						   GtkWindow * parent);

/* This function is deprecated; use gnome_request_dialog() instead. */
GtkWidget * gnome_request_password_dialog         (const gchar * prompt,
						   GnomeStringCallback callback,
						   gpointer data);

/* This function is deprecated; use gnome_request_dialog() instead. */
GtkWidget * gnome_request_password_dialog_parented(const gchar * prompt,
						   GnomeStringCallback callback,
						   gpointer data,
						   GtkWindow * parent);
#endif

/* Dialog containing a prompt and a text entry field for a response */
GtkWidget * gnome_request_dialog (gboolean password,
                                  const gchar * prompt,
                                  const gchar * default_text,
                                  const guint16 max_length,
                                  GnomeStringCallback callback,
                                  gpointer data,
                                  GtkWindow * parent);

END_GNOME_DECLS

#endif
