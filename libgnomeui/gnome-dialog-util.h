/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#ifndef GNOME_DIALOG_UTIL_H
#define GNOME_DIALOG_UTIL_H

#ifndef GNOME_DISABLE_DEPRECATED

/**** 
  Sugar functions to pop up dialogs in a hurry. These are probably
  too sugary, but they're used in gnome-app-util anyway so they may as
  well be here for others to use when there's no GnomeApp.
  The gnome-app-util functions are preferred if there's a GnomeApp
  to use them with, because they allow configurable statusbar messages
  instead of a dialog.
  ****/


#include "gnome-types.h"

G_BEGIN_DECLS

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

/* Dialog containing a prompt and a text entry field for a response */
GtkWidget * gnome_request_dialog (gboolean password,
                                  const gchar * prompt,
                                  const gchar * default_text,
                                  const guint16 max_length,
                                  GnomeStringCallback callback,
                                  gpointer data,
                                  GtkWindow * parent);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif

