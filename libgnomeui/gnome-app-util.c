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
#include "libgnome/gnome-util.h"

#include "gnome-stock.h"
#include "gnome-dialog-util.h"
#include "gnome-dialog.h"
#include "gnome-uidefs.h"
#include "gnome-preferences.h"
#include "gnome-appbar.h"

#include <gtk/gtk.h>

static gboolean 
gnome_app_has_appbar_progress(GnomeApp * app)
{
  return ( (app->statusbar != NULL) &&
	   (GNOME_APPBAR_HAS_PROGRESS(app->statusbar)) );
}

static gboolean 
gnome_app_has_appbar_status(GnomeApp * app)
{
  return ( (app->statusbar != NULL) &&
	   (GNOME_APPBAR_HAS_STATUS(app->statusbar)) );
}

static gboolean
gnome_app_interactive_statusbar(GnomeApp * app)
{
 return ( gnome_app_has_appbar_status (app) && 
	  gnome_preferences_get_statusbar_dialog() &&
	  gnome_preferences_get_statusbar_interactive() );
}

/* ================================================================== */

/* =================================================================== */
/* Simple messages */

static void
ack_cb(GnomeAppBar * bar, gpointer data)
{
  gnome_appbar_clear_prompt(bar);
}

static void
ack_clear_prompt_cb(GnomeAppBar * bar, gpointer data)
{
#ifdef GNOME_ENABLE_DEBUG
  g_print("Clearing prompt (ack)\n");
#endif

  gtk_signal_disconnect_by_func(GTK_OBJECT(bar), 
				GTK_SIGNAL_FUNC(ack_cb),
				data);
  gtk_signal_disconnect_by_func(GTK_OBJECT(bar), 
				GTK_SIGNAL_FUNC(ack_clear_prompt_cb),
				data);
}

static void gnome_app_message_bar (GnomeApp * app, const gchar * message)
{
  gchar * prompt = g_copy_strings(message, _(" (press return)"), NULL);
  gnome_appbar_set_prompt(GNOME_APPBAR(app->statusbar), prompt, FALSE);
  g_free(prompt);
  gtk_signal_connect(GTK_OBJECT(app->statusbar), "user_response",
		     GTK_SIGNAL_FUNC(ack_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(app->statusbar), "clear_prompt",
		     GTK_SIGNAL_FUNC(ack_clear_prompt_cb), NULL);
}

GtkWidget *
gnome_app_message (GnomeApp * app, const gchar * message)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APP(app), NULL);

  if ( gnome_preferences_get_statusbar_dialog() &&
       gnome_app_interactive_statusbar(app) ) {
    gnome_app_message_bar ( app, message );
    return NULL;
  }
  else {
    return gnome_ok_dialog(message);
  }
}

static void 
gnome_app_error_bar(GnomeApp * app, const gchar * error)
{
  gchar * s = g_copy_strings(_("ERROR: "), error, NULL);
  gdk_beep();
  gnome_app_message_bar(app, s);
  g_free(s);
}

GtkWidget * 
gnome_app_error (GnomeApp * app, const gchar * error)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APP(app), NULL);
  g_return_val_if_fail(error != NULL, NULL);

  if ( gnome_app_interactive_statusbar(app) ) {
    gnome_app_error_bar(app, error);
    return NULL;
  }
  else return gnome_error_dialog (error);
}

static void gnome_app_warning_bar (GnomeApp * app, const gchar * warning)
{
  gchar * s = g_copy_strings(_("Warning: "), warning, NULL);
  gdk_beep();
  gnome_app_flash(app, s);
  g_free(s);
}

GtkWidget * 
gnome_app_warning (GnomeApp * app, const gchar * warning)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APP(app), NULL);
  g_return_val_if_fail(warning != NULL, NULL);

  if ( gnome_app_has_appbar_status(app) && 
       gnome_preferences_get_statusbar_dialog() ) {
    gnome_app_warning_bar(app, warning);
    return NULL;
  }
  else {
    return gnome_warning_dialog(warning);
  }
}

/* =================================================== */
/* Flash */

struct _MessageInfo {
  GnomeApp * app;
  guint timeoutid;
  guint handlerid;
};

typedef struct _MessageInfo MessageInfo;

static gint
remove_message_timeout (MessageInfo * mi) 
{
  gnome_appbar_refresh(GNOME_APPBAR(mi->app->statusbar));
  gtk_signal_disconnect(GTK_OBJECT(mi->app), mi->handlerid);
  g_free ( mi );
  return FALSE; /* removes the timeout */
}

/* Called if the app is destroyed before the timeout occurs. */
static void
remove_timeout_cb ( GtkWidget * app, MessageInfo * mi ) 
{
  gtk_timeout_remove(mi->timeoutid);
  g_free(mi);
}

static const guint32 flash_length = 3000; /* 3 seconds, I hope */

void 
gnome_app_flash (GnomeApp * app, const gchar * flash)
{
  /* This is a no-op for dialogs, since these messages aren't 
     important enough to pester users. */
  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));
  g_return_if_fail(flash != NULL);

  /* OK to call app_flash without a statusbar */
  if ( gnome_app_has_appbar_status(app) ) {
    MessageInfo * mi;

    g_return_if_fail(GNOME_IS_APPBAR(app->statusbar));
    
    gnome_appbar_set_status(GNOME_APPBAR(app->statusbar), flash);

    mi = g_new(MessageInfo, 1);

    mi->timeoutid = 
      gtk_timeout_add ( flash_length,
			(GtkFunction) remove_message_timeout,
			mi );
    
    mi->handlerid = 
      gtk_signal_connect ( GTK_OBJECT(app),
			   "destroy",
			   GTK_SIGNAL_FUNC(remove_timeout_cb),
			   mi );

    mi->app       = app;
  }   
}

/* ========================================================== */

typedef struct {
  GnomeReplyCallback callback;
  gpointer data;
} ReplyInfo;

static void
bar_reply_cb(GnomeAppBar * ab, ReplyInfo * ri)
{
  gchar * response;

  response = gnome_appbar_get_response(ab);
  g_return_if_fail(response != NULL);

#ifdef GNOME_ENABLE_DEBUG
  g_print("Got reply: \"%s\"\n", response);
#endif

  if ( (g_strcasecmp(_("y"), response) == 0) || 
       (g_strcasecmp(_("yes"), response) == 0) ) {
    (* (ri->callback)) (GNOME_YES, ri->data);
  }
  else if ( (g_strcasecmp(_("n"), response) == 0) || 
	    (g_strcasecmp(_("no"), response) == 0) ) {
    (* (ri->callback)) (GNOME_NO, ri->data);
  }
  else {
    g_free(response);
    gdk_beep(); /* Kind of lame; better to give a helpful message */
    return;
  }
  g_free(response);
  gnome_appbar_clear_prompt(ab);
  return;
}

/* FIXME this can all be done as a special case of request_string
   where we check the string for yes/no afterward. */

static void
reply_clear_prompt_cb(GnomeAppBar * ab, ReplyInfo * ri)
{
#ifdef GNOME_ENABLE_DEBUG
  g_print("Clear prompt callback (reply)\n");
#endif
  gtk_signal_disconnect_by_func(GTK_OBJECT(ab), 
				GTK_SIGNAL_FUNC(bar_reply_cb),
				ri);
  gtk_signal_disconnect_by_func(GTK_OBJECT(ab), 
				GTK_SIGNAL_FUNC(reply_clear_prompt_cb),
				ri);
  g_free(ri);
}

static void 
gnome_app_reply_bar(GnomeApp * app, const gchar * question,
		    GnomeReplyCallback callback, gpointer data,
		    gboolean yes_or_ok, gboolean modal)
{
  gchar * prompt;
  ReplyInfo * ri;

  prompt = g_copy_strings(question, yes_or_ok ? _(" (yes or no)") : 
			  _("  - OK? (yes or no)"), NULL);
  gnome_appbar_set_prompt(GNOME_APPBAR(app->statusbar), prompt, modal);
  g_free(prompt);
  
  ri = g_new(ReplyInfo, 1);
  ri->callback = callback;
  ri->data     = data;

  gtk_signal_connect(GTK_OBJECT(app->statusbar), "user_response",
		     GTK_SIGNAL_FUNC(bar_reply_cb), ri);
  gtk_signal_connect(GTK_OBJECT(app->statusbar), "clear_prompt",
		     GTK_SIGNAL_FUNC(reply_clear_prompt_cb), ri);
}

GtkWidget * 
gnome_app_question (GnomeApp * app, const gchar * question,
		    GnomeReplyCallback callback, gpointer data)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APP(app), NULL);
  g_return_val_if_fail(question != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  if ( gnome_app_interactive_statusbar(app) ) {
    gnome_app_reply_bar(app, question, callback, data, TRUE, FALSE);
    return NULL;
  }
  else {
    return gnome_question_dialog(question, callback, data);
  }
}

GtkWidget * 
gnome_app_question_modal (GnomeApp * app, const gchar * question,
			  GnomeReplyCallback callback, gpointer data)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APP(app), NULL);
  g_return_val_if_fail(question != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  if ( gnome_app_interactive_statusbar(app) ) {
    gnome_app_reply_bar(app, question, callback, data, TRUE, TRUE);
    return NULL;
  }
  else {
    return gnome_question_dialog_modal(question, callback, data);
  }
}

GtkWidget * 
gnome_app_ok_cancel (GnomeApp * app, const gchar * message,
		     GnomeReplyCallback callback, gpointer data)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APP(app), NULL);
  g_return_val_if_fail(message != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  if ( gnome_app_interactive_statusbar(app) ) {
    gnome_app_reply_bar(app, message, callback, data, FALSE, FALSE);
    return NULL;
  }
  else {
    return gnome_ok_cancel_dialog(message, callback, data);
  }
}

GtkWidget * 
gnome_app_ok_cancel_modal (GnomeApp * app, const gchar * message,
			   GnomeReplyCallback callback, gpointer data)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APP(app), NULL);
  g_return_val_if_fail(message != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  if ( gnome_app_interactive_statusbar(app) ) {
    gnome_app_reply_bar(app, message, callback, data, FALSE, TRUE);
    return NULL;
  }
  else {
    return gnome_ok_cancel_dialog_modal(message, callback, data);
  }
}

typedef struct {
  GnomeStringCallback callback;
  gpointer data;
} RequestInfo;


static void 
bar_request_cb(GnomeAppBar * ab, RequestInfo * ri)
{
  gchar * response;

  response = gnome_appbar_get_response(ab);
  g_return_if_fail(response != NULL);

#ifdef GNOME_ENABLE_DEBUG
  g_print("Got string: \"%s\"\n", response);
#endif

  /* response isn't freed because the callback expects an 
     allocated string. */
  (* (ri->callback)) (response, ri->data);

  gnome_appbar_clear_prompt(ab);
  return;
}

static void
request_clear_prompt_cb(GnomeAppBar * ab, RequestInfo * ri)
{
#ifdef GNOME_ENABLE_DEBUG
  g_print("Clearing prompt (request)\n");
#endif


  gtk_signal_disconnect_by_func(GTK_OBJECT(ab), 
				GTK_SIGNAL_FUNC(bar_request_cb),
				ri);
  gtk_signal_disconnect_by_func(GTK_OBJECT(ab), 
				GTK_SIGNAL_FUNC(request_clear_prompt_cb),
				ri);
  g_free(ri);
}

static void 
gnome_app_request_bar  (GnomeApp * app, const gchar * prompt,
			GnomeStringCallback callback, 
			gpointer data, gboolean password)
{
  if (password == TRUE) {
    g_warning("Password support not implemented for appbar");
  }
  else {
    RequestInfo * ri;
    
    gnome_appbar_set_prompt(GNOME_APPBAR(app->statusbar), prompt, FALSE);
    
    ri = g_new(RequestInfo, 1);
    ri->callback = callback;
    ri->data     = data;
    
    gtk_signal_connect(GTK_OBJECT(app->statusbar), "user_response",
		       GTK_SIGNAL_FUNC(bar_request_cb), ri);   
    gtk_signal_connect(GTK_OBJECT(app->statusbar), "clear_prompt",
		       GTK_SIGNAL_FUNC(request_clear_prompt_cb), ri);     
  }
}

GtkWidget * 
gnome_app_request_string (GnomeApp * app, const gchar * prompt,
			  GnomeStringCallback callback, gpointer data)
{ 
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APP(app), NULL);
  g_return_val_if_fail(prompt != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  if ( gnome_app_interactive_statusbar(app) ) {
    gnome_app_request_bar(app, prompt, callback, data, FALSE);
    return NULL;
  }
  else {
    return gnome_request_string_dialog(prompt, callback, data);   
  }
}


GtkWidget * 
gnome_app_request_password (GnomeApp * app, const gchar * prompt,
			    GnomeStringCallback callback, gpointer data)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APP(app), NULL);
  g_return_val_if_fail(prompt != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  /* FIXME implement for AppBar */

  return gnome_request_password_dialog(prompt, callback, data);
}

/* ================================================== */

typedef struct {
  GtkWidget * bar; /* Progress bar, for dialog; NULL for AppBar */
  GtkWidget * widget; /* dialog or AppBar */
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
  gdouble percent;
  percent = (* key->percentage_cb)(key->data);
  gnome_app_set_progress (key, percent);
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
  gtk_signal_connect (GTK_OBJECT(d), "clicked", 
		      GTK_SIGNAL_FUNC(progress_clicked_cb),
		      key);
			       
  label = gtk_label_new (description);

  pb = gtk_progress_bar_new();

  gtk_box_pack_start ( GTK_BOX(GNOME_DIALOG(d)->vbox), 
		       label, TRUE, TRUE, GNOME_PAD );
  gtk_box_pack_start ( GTK_BOX(GNOME_DIALOG(d)->vbox),
		       pb, TRUE, TRUE, GNOME_PAD );

  key->bar = pb;
  key->widget = d;

  gtk_widget_show_all (d);
}

static void 
progress_bar (GnomeApp * app, const gchar * description, ProgressKeyReal * key)
{
  key->bar = NULL;
  key->widget = app->statusbar;
  gnome_appbar_set_status(GNOME_APPBAR(key->widget), description);
}

static void
stop_progress_cb(GnomeApp * app, GnomeAppProgressKey key)
{
  gnome_app_progress_done(key);
}

/* FIXME share code between manual and timeout */

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

  if ( gnome_app_has_appbar_progress(app) &&
       gnome_preferences_get_statusbar_dialog() ) {
    progress_bar    (app, description, key);
  }
  else {
    progress_dialog (description, key);
  }

  key->timeout_tag = gtk_timeout_add ( interval, 
				       (GtkFunction) progress_timeout_cb,
				       key );

  /* Make sure progress stops if the app is destroyed. */
  gtk_signal_connect(GTK_OBJECT(app), "destroy",
		     GTK_SIGNAL_FUNC(stop_progress_cb),
		     key);

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

  if ( gnome_app_has_appbar_progress(app) &&
       gnome_preferences_get_statusbar_dialog() ) {
    progress_bar    (app, description, key);
  }
  else {
    progress_dialog (description, key);
  }

  /* Make sure progress stops if the app is destroyed. */
  gtk_signal_connect(GTK_OBJECT(app), "destroy",
		     GTK_SIGNAL_FUNC(stop_progress_cb),
		     key);

  return key;
}

void gnome_app_set_progress (GnomeAppProgressKey key, gdouble percent)
{
  ProgressKeyReal * real_key = (GnomeAppProgressKey) key;

  g_return_if_fail ( key != NULL );  

  if (real_key->bar) {
    gtk_progress_bar_update ( GTK_PROGRESS_BAR(real_key->bar), percent );
  }
  else {
    gnome_appbar_set_progress ( GNOME_APPBAR(real_key->widget), percent );
  }
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

  if (real_key->bar) { /* It's a dialog */
    if (real_key->widget) gnome_dialog_close(GNOME_DIALOG(real_key->widget));
  }
  else {
    /* Reset the bar */
    gnome_appbar_set_progress ( GNOME_APPBAR(real_key->widget), 0.0 );
  }
  g_free(key);
}




