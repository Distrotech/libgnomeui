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

#ifndef GNOME_APP_UTIL_H
#define GNOME_APP_UTIL_H
/****
  Convenience UI functions for use with GnomeApp 
  ****/

#ifndef GNOME_DISABLE_DEPRECATED

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gnome-app.h"
#include "gnome-types.h"

G_BEGIN_DECLS

/* =============================================
  Simple messages and questions to the user; use dialogs for now, but
   ultimately can use either dialogs or statusbar. Supposed to be a
   "semantic markup" kind of thing. */
/* If the function returns GtkWidget *, it will return any dialog
   created, or NULL if none */

/* A simple message, in an OK dialog or the status bar. Requires
   confirmation from the user before it goes away. */
GtkWidget * 
gnome_app_message (GnomeApp * app, const gchar * message);

/* Flash the message in the statusbar for a few moments; if no
   statusbar, do nothing (?). For trivial little status messages,
   e.g. "Auto saving..." */
void 
gnome_app_flash (GnomeApp * app, const gchar * flash);

/* An important fatal error; if it appears in the statusbar, 
   it might gdk_beep() and require acknowledgement. */
GtkWidget * 
gnome_app_error (GnomeApp * app, const gchar * error);

/* A not-so-important error, but still marked better than a flash */
GtkWidget * 
gnome_app_warning (GnomeApp * app, const gchar * warning);

/* =============================================================== */

/* For these, the user can cancel without ever calling the callback,
   e.g. by clicking the dialog's "close" button. So don't count on the
   callback ever being called. */

/* Ask a yes or no question, and call the callback when it's answered. */
GtkWidget * 
gnome_app_question (GnomeApp * app, const gchar * question,
		    GnomeReplyCallback callback, gpointer data);

GtkWidget * 
gnome_app_question_modal (GnomeApp * app, const gchar * question,
			  GnomeReplyCallback callback, gpointer data);

/* OK-Cancel question. */
GtkWidget *
gnome_app_ok_cancel (GnomeApp * app, const gchar * message,
		     GnomeReplyCallback callback, gpointer data);

GtkWidget * 
gnome_app_ok_cancel_modal (GnomeApp * app, const gchar * message,
			   GnomeReplyCallback callback, gpointer data);

/* Get a string. */
GtkWidget * 
gnome_app_request_string (GnomeApp * app, const gchar * prompt,
			  GnomeStringCallback callback, gpointer data);

/* Request a string, but don't echo to the screen. */
GtkWidget * 
gnome_app_request_password (GnomeApp * app, const gchar * prompt,
			    GnomeStringCallback callback, gpointer data);


/* ========================================================== */

/* Returns the fraction of the task done at a given time. */
typedef gdouble (* GnomeAppProgressFunc) (gpointer data);

/* What to call if the operation is canceled. */
typedef void (* GnomeAppProgressCancelFunc) (gpointer data);

/* A key used to refer to the callback. DO NOT FREE.
   It's freed by _done or when the progress is canceled. */
typedef gpointer GnomeAppProgressKey;

/* These will be a progress bar dialog or the progress bar in the AppBar. */

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
void 
gnome_app_set_progress (GnomeAppProgressKey key, gdouble percent);

/* Call this when the progress meter should go away. Automatically 
   called if progress is cancelled. */
void 
gnome_app_progress_done (GnomeAppProgressKey key);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED_SOURCE */

#endif
