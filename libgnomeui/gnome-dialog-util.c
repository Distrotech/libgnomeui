/* GNOME GUI Library: gnome-dialog-util.c
 * Copyright (C) 1998 Free Software Foundation
 * Author: Havoc Pennington
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <config.h>
#include "gnome-stock.h"
#include "gnome-messagebox.h"
#include "gnome-types.h"
#include "gnome-uidefs.h"
#include "gnome-dialog-util.h"
#include <gtk/gtk.h>

static GtkWidget * show_ok_box(const gchar * message, const gchar * type,
			       GtkWindow * parent)
{  
  GtkWidget * mbox;

  mbox = gnome_message_box_new (message, type,
				GNOME_STOCK_BUTTON_OK, NULL);
  
  if (parent != NULL) {
    gnome_dialog_set_parent(GNOME_DIALOG(mbox),parent);
  }

  gtk_widget_show (mbox);
  return mbox;
}


/* A little OK box */
GtkWidget * gnome_ok_dialog      (const gchar * message)
{
  return show_ok_box (message, GNOME_MESSAGE_BOX_INFO, NULL);
}

GtkWidget * gnome_ok_dialog_parented    (const gchar * message,
					 GtkWindow * parent)
{
  return show_ok_box (message, GNOME_MESSAGE_BOX_INFO, parent);
}

/* Operation failed fatally. In an OK dialog. */
GtkWidget * gnome_error_dialog   (const gchar * error)
{
  return show_ok_box(error, GNOME_MESSAGE_BOX_ERROR, NULL);
}

GtkWidget * gnome_error_dialog_parented (const gchar * error,
					 GtkWindow * parent)
{
  return show_ok_box(error, GNOME_MESSAGE_BOX_ERROR, parent);
}

/* Just a warning. */
GtkWidget * gnome_warning_dialog (const gchar * warning)
{
  return show_ok_box(warning, GNOME_MESSAGE_BOX_WARNING, NULL);
}

GtkWidget * gnome_warning_dialog_parented  (const gchar * warning,
					    GtkWindow * parent)
{
  return show_ok_box(warning, GNOME_MESSAGE_BOX_WARNING, parent);
}


typedef struct {
  gpointer function;
  gpointer data;
  GtkEntry * entry;
} callback_info;

static void dialog_reply_callback (GnomeMessageBox * mbox, 
				   gint button, callback_info* data)
{
  GnomeReplyCallback func = (GnomeReplyCallback) data->function;
  (* func)(button, data->data);
  g_free(data);
}
 
static GtkWidget * reply_dialog (const gchar * question,
				 GnomeReplyCallback callback, 
				 gpointer data,
				 gboolean yes_or_ok,
				 gboolean modal,
				 GtkWindow * parent)
{
  GtkWidget * mbox;
  callback_info * info;

  if (yes_or_ok) {
    mbox = gnome_message_box_new(question, GNOME_MESSAGE_BOX_QUESTION,
				 GNOME_STOCK_BUTTON_YES, 
				 GNOME_STOCK_BUTTON_NO, NULL);
  }
  else {
    mbox = gnome_message_box_new(question, GNOME_MESSAGE_BOX_QUESTION,
				 GNOME_STOCK_BUTTON_OK, 
				 GNOME_STOCK_BUTTON_CANCEL, NULL);
  }

  info = g_new(callback_info, 1);

  info->function = callback;
  info->data = data;

  gtk_signal_connect(GTK_OBJECT(mbox), "clicked",
		     GTK_SIGNAL_FUNC(dialog_reply_callback),
		     info);

  if (modal) {
    gtk_window_set_modal(GTK_WINDOW(mbox),TRUE);
  }

  if (parent != NULL) {
    gnome_dialog_set_parent(GNOME_DIALOG(mbox),parent);
  }

  gtk_widget_show(mbox);
  return mbox;
}


/* Ask a yes or no question, and call the callback when it's answered. */
GtkWidget * gnome_question_dialog        (const gchar * question,
					  GnomeReplyCallback callback, 
					  gpointer data)
{
  return reply_dialog(question, callback, data, TRUE, FALSE, NULL);
}

GtkWidget * gnome_question_dialog_parented        (const gchar * question,
						   GnomeReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent)
{
  return reply_dialog(question, callback, data, TRUE, FALSE, parent);
}



GtkWidget * gnome_question_dialog_modal  (const gchar * question,
					  GnomeReplyCallback callback, 
					  gpointer data)
{
  return reply_dialog(question, callback, data, TRUE, TRUE, NULL);
}

GtkWidget * gnome_question_dialog_modal_parented  (const gchar * question,
						   GnomeReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent)
{
  return reply_dialog(question, callback, data, TRUE, TRUE, parent);
}


/* OK-Cancel question. */
GtkWidget * gnome_ok_cancel_dialog       (const gchar * message,
					  GnomeReplyCallback callback, 
					  gpointer data)
{
  return reply_dialog(message, callback, data, FALSE, FALSE, NULL);
}

GtkWidget * gnome_ok_cancel_dialog_parented       (const gchar * message,
						   GnomeReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent)
{
  return reply_dialog(message, callback, data, FALSE, FALSE, parent);
}

GtkWidget * gnome_ok_cancel_dialog_modal (const gchar * message,
					  GnomeReplyCallback callback, 
					  gpointer data)
{
  return reply_dialog(message, callback, data, FALSE, TRUE, NULL);
}

GtkWidget * gnome_ok_cancel_dialog_modal_parented (const gchar * message,
						   GnomeReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent)
{
  return reply_dialog(message, callback, data, FALSE, TRUE, parent);
}

static void dialog_string_callback (GnomeMessageBox * mbox, gint button, 
				    callback_info * data)
{
  gchar * s = NULL;
  gchar * tmp;
  GnomeStringCallback func = (GnomeStringCallback)data->function;

  if (button == 0) {
    tmp = gtk_entry_get_text (data->entry);
    if (tmp) s = g_strdup(tmp);
  }

  (* func)(s, data->data);

  g_free(data);
}

static GtkWidget * request_dialog (const gchar * request, 
				   const gchar * default_text,
				   const guint16 max_length,
				   GnomeStringCallback callback,
				   gpointer data, gboolean password,
				   GtkWindow * parent)
{
  GtkWidget * mbox;
  callback_info * info;
  GtkWidget * entry;

  mbox = gnome_message_box_new ( request, GNOME_MESSAGE_BOX_QUESTION,
				 GNOME_STOCK_BUTTON_OK, 
				 GNOME_STOCK_BUTTON_CANCEL,
				 NULL );
  gnome_dialog_set_default ( GNOME_DIALOG(mbox), 0 );

  /* set up text entry widget */
  entry = gtk_entry_new();
  if (password) gtk_entry_set_visibility (GTK_ENTRY(entry), FALSE);
  if ((default_text != NULL) && (*default_text))
    gtk_entry_set_text(GTK_ENTRY(entry), default_text);
  if (max_length > 0)
    gtk_entry_set_max_length(GTK_ENTRY(entry), max_length);

  gtk_box_pack_end ( GTK_BOX(GNOME_DIALOG(mbox)->vbox), 
		     entry, FALSE, FALSE, GNOME_PAD_SMALL );

  /* If Return is pressed in the text entry, propagate to the buttons */
  gnome_dialog_editable_enters(GNOME_DIALOG(mbox), GTK_EDITABLE(entry));

  info = g_new(callback_info, 1);

  info->function = callback;
  info->data = data;
  info->entry = GTK_ENTRY(entry);

  gtk_signal_connect (GTK_OBJECT(mbox), "clicked", 
		      GTK_SIGNAL_FUNC(dialog_string_callback), 
		      info);

  if (parent != NULL) {
    gnome_dialog_set_parent(GNOME_DIALOG(mbox),parent);
  }

  gtk_widget_show (entry);
  gtk_widget_show (mbox);
  return mbox;
}


/* Get a string. */
GtkWidget * gnome_request_string_dialog  (const gchar * prompt,
					  GnomeStringCallback callback, 
					  gpointer data)
{
  g_message("gnome_request_string_dialog is deprecated, use gnome_request_dialog instead.");
  return request_dialog (prompt, NULL, 0, callback, data, FALSE, NULL);
}

GtkWidget * gnome_request_string_dialog_parented  (const gchar * prompt,
						   GnomeStringCallback callback,
						   gpointer data,
						   GtkWindow * parent)
{
  g_message("gnome_request_string_dialog_parented is deprecated, use gnome_request_dialog instead.");
  return request_dialog (prompt, NULL, 0, callback, data, FALSE, parent);
}

/* Request a string, but don't echo to the screen. */
GtkWidget * gnome_request_password_dialog (const gchar * prompt,
					   GnomeStringCallback callback, 
					   gpointer data)
{
  g_message("gnome_request_password_dialog is deprecated, use gnome_request_dialog instead.");
  return request_dialog (prompt, NULL, 0, callback, data, TRUE, NULL);
}

GtkWidget * gnome_request_password_dialog_parented(const gchar * prompt,
						   GnomeStringCallback callback,
						   gpointer data,
						   GtkWindow * parent)
{
  g_message("gnome_request_password_dialog_parented is deprecated, use gnome_request_dialog instead.");
  return request_dialog (prompt, NULL, 0, callback, data, TRUE, parent);
}

/**
 * gnome_request_dialog
 * @password: %TRUE if on-screen text input is masked
 * @prompt: Text of the prompt to be displayed
 * @default_text: Default text in entry widget, %NULL if none
 * @max_length: Maximum input chars allowed
 * @callback: Callback function for handling dialog results
 * @parent: Parent window, or %NULL for no parent.
 *
 * Description:  Creates a GNOME text entry request dialog.  @callback
 * is called when the dialog closes, passing the text entry input or
 * %NULL if the user cancelled.  @callback is defined as
 *
 * void (* GnomeStringCallback)(gchar * string, gpointer data); 
 *
 * Returns:  Pointer to new GNOME dialog object.
 **/

GtkWidget * gnome_request_dialog (gboolean password,
				  const gchar * prompt,
				  const gchar * default_text,
				  const guint16 max_length,
				  GnomeStringCallback callback, 
				  gpointer data,
				  GtkWindow * parent)
{
  return request_dialog (prompt, default_text, max_length,
  			 callback, data, password, parent);
}

