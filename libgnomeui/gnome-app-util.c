/* GNOME GUI Library: gnome-app-util.c
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

#include "gnome-app-util.h"
#include "libgnome/gnome-i18nP.h"
#include <gtk/gtk.h>
#include "gnome-stock.h"
#include "gnome-dialog-util.h"
#include "gnome-uidefs.h"

#define NOT_IMPLEMENTED g_warning("Status bar support not implemented.\n")

/* ================================================================== */

void gnome_app_set_status (GnomeApp * app, const gchar * status)
{
  NOT_IMPLEMENTED;
}

/* =================================================================== */


static void gnome_app_message_bar (GnomeApp * app, const gchar * message)
{
  NOT_IMPLEMENTED;
}

void gnome_app_message (GnomeApp * app, const gchar * message)
{
  /* if (user wants dialog, not statusbar) */
  gnome_message_dialog(message);
  /* else use statusbar. */
}

void gnome_app_flash (GnomeApp * app, const gchar * flash)
{
  /* This is a no-op for dialogs, since these messages aren't 
     important enough to pester users. */
  NOT_IMPLEMENTED;
}

static gnome_app_error_bar(GnomeApp * app, const gchar * error)
{
  NOT_IMPLEMENTED;
}

void gnome_app_error (GnomeApp * app, const gchar * error)
{
  gnome_error_dialog (error);
}

static void gnome_app_warning_dialog (const gchar * warning)
{
  show_ok_box(warning, GNOME_MESSAGE_BOX_WARNING);
}

static void gnome_app_warning_bar (GnomeApp * app, const gchar * warning)
{
  NOT_IMPLEMENTED;
}

void gnome_app_warning (GnomeApp * app, const gchar * warning)
{
  gnome_warning_dialog(warning);
}

/* ========================================================== */

static void gnome_app_reply_bar(GnomeApp * app, const gchar * question,
				GnomeReplyCallback callback, gpointer data,
				gboolean yes_or_ok, gboolean modal)
{
  NOT_IMPLEMENTED;
}

void gnome_app_question (GnomeApp * app, const gchar * question,
			 GnomeReplyCallback callback, gpointer data)
{
  gnome_question_dialog(question, callback, data);
}

void gnome_app_question_modal (GnomeApp * app, const gchar * question,
			       GnomeReplyCallback callback, gpointer data)
{
  gnome_question_dialog_modal(question, callback, data);
}

void gnome_app_ok_cancel (GnomeApp * app, const gchar * message,
			  GnomeReplyCallback callback, gpointer data)
{
  gnome_ok_cancel_dialog(message, callback, data);
}

void gnome_app_ok_cancel_modal (GnomeApp * app, const gchar * message,
				GnomeReplyCallback callback, gpointer data)
{
  gnome_ok_cancel_dialog_modal(message, callback, data);
}

static void gnome_app_request_bar  (GnomeApp * app, const gchar * prompt,
				    GnomeStringCallback callback, 
				    gpointer data, gboolean password)
{
  NOT_IMPLEMENTED;
}

void gnome_app_request_string (GnomeApp * app, const gchar * prompt,
			       GnomeStringCallback callback, gpointer data)
{ 
  gnome_request_string_dialog(prompt, callback, data);
}


void gnome_app_request_password (GnomeApp * app, const gchar * prompt,
				 GnomeStringCallback callback, gpointer data)
{
  gnome_request_password_dialog(prompt, callback, data);
}

/* ================================================== */

typedef struct {
  GtkWidget * bar; /* Progress bar, if any */
  GtkWidget * widget; /* dialog or status bar */
  guint timeout_tag;
  GnomeApp * app;
  GnomeAppProgressFunc percentage_cb;
  GnomeAppProgressCancelFunc cancel_cb;
  gpointer data;
} ProgressKeyReal;

/* Invalid value, I hope. FIXME */
#define INVALID_TIMEOUT 0

/* Works for statusbar and dialog both. */
static gint progress_timeout_cb (ProgressKeyReal * key)
{
  gfloat percent;
  percent = (* key->percentage_cb)(key->data);
  gnome_app_progress_update (key, percent);
  return TRUE;
}


/* dialog only */
static void progress_clicked_cb (GnomeDialog * d, gint button,
				 ProgressKeyReal * key)
{
  if (key->cancel_cb) {
    (* key->cancel_cb)(key->data);
  }
  key->widget = NULL; /* The click closed the dialog */
  gnome_app_progress_done(key);
}

static void 
progress_dialog (const gchar * description, ProgressKeyReal * key)
{
  GtkWidget * d, * pb, * label;

  d = gnome_dialog_new ( _("Progress"), 
			 GNOME_STOCK_BUTTON_CANCEL,
			 NULL );
  gnome_dialog_set_close (GNOME_DIALOG(d), TRUE);
  gtk_signal_connect(GTK_OBJECT(d), "clicked", 
		     GTK_SIGNAL_FUNC(progress_clicked_cb),
		     key);
			       
  label = gtk_label_new( description );

  pb = gtk_progress_bar_new();

  gtk_box_pack_start ( GTK_BOX(GNOME_DIALOG(d)->vbox), 
		       label, TRUE, TRUE, GNOME_PAD );
  gtk_box_pack_start ( GTK_BOX(GNOME_DIALOG(d)->vbox),
		       pb, TRUE, TRUE, GNOME_PAD );

  key->bar = pb;
  key->widget = d;

  gtk_widget_show_all (d);
}

GnomeAppProgressKey 
gnome_app_progress_timeout (GnomeApp * app, 
			    const gchar * description,
			    guint32 interval, 
			    GnomeAppProgressFunc percentage_cb,
			    GnomeAppProgressCancelFunc cancel_cb,
			    gpointer data)
{
  ProgressKeyReal * key;

  g_return_val_if_fail (app != NULL, NULL);
  g_return_val_if_fail (GNOME_IS_APP(app), NULL);
  g_return_val_if_fail (description != NULL, NULL);
  g_return_val_if_fail (percentage_cb != NULL, NULL);

  key = g_new (ProgressKeyReal, 1);

  key->percentage_cb = percentage_cb;
  key->cancel_cb = cancel_cb;
  key->data = data;

  /* If dialog, */
  progress_dialog (description, key);
  /* else if statusbar, do something different. */

  key->timeout_tag = gtk_timeout_add ( interval, 
				       (GtkFunction) progress_timeout_cb,
				       key );

  return key;
}

GnomeAppProgressKey 
gnome_app_progress_manual (GnomeApp * app, 
			   const gchar * description,
			   GnomeAppProgressCancelFunc cancel_cb,
			   gpointer data)
{
  ProgressKeyReal * key;

  g_return_val_if_fail (app != NULL, NULL);
  g_return_val_if_fail (GNOME_IS_APP(app), NULL);
  g_return_val_if_fail (description != NULL, NULL);

  key = g_new (ProgressKeyReal, 1);

  key->cancel_cb = cancel_cb;
  key->data = data;
  key->timeout_tag = INVALID_TIMEOUT;  

  progress_dialog (description, key);

  return key;
}

void gnome_app_progress_update (GnomeAppProgressKey key, gfloat percent)
{
  ProgressKeyReal * real_key = (GnomeAppProgressKey) key;

  g_return_if_fail ( key != NULL );  

  gtk_progress_bar_update ( GTK_PROGRESS_BAR(real_key->bar), percent );
}

static void progress_timeout_remove(ProgressKeyReal * key)
{
  if (key->timeout_tag != INVALID_TIMEOUT) {
    gtk_timeout_remove(key->timeout_tag);
    key->timeout_tag = INVALID_TIMEOUT;
  }
}

void gnome_app_progress_done (GnomeAppProgressKey key)
{
  ProgressKeyReal * real_key = (GnomeAppProgressKey) key;

  g_return_if_fail ( key != NULL );

  progress_timeout_remove((ProgressKeyReal *)key);

  if (real_key->widget) gnome_dialog_close(GNOME_DIALOG(real_key->widget));
  g_free(key);
}




