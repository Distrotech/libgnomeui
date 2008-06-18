/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * gnome-window.h: wrappers for setting window properties
 *
 * Author:  Chema Celorio <chema@celorio.com>
 */

/*
 * These functions are a convenience wrapper for gtk_window_set_title
 * This allows all the gnome-apps to have a consitent way of setting
 * the window & dialogs titles. We could also add a configurable way
 * of setting the windows titles in the future..
 */

#ifndef GNOME_WINDOW_H
#define GNOME_WINDOW_H

#ifndef GNOME_DISABLE_DEPRECATED

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* set the window title */
void gnome_window_toplevel_set_title (GtkWindow *window,
				      const gchar *doc_name,
				      const gchar *app_name,
				      const gchar *extension);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* GNOME_WINDOW_H */

