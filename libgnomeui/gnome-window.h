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
 *
 * These functions were added with the 1.2.5 release of the GNOME libraries
 * in Oct, 2000.  This means that not all users will have this functionality
 * in the GNOME libraries, and should only be used accordingly.  The header file
 * must be explicitely included, also (#include <libgnomeui/gnome-window.h>).
 */

#ifndef GNOME_WINDOW_H
#define GNOME_WINDOW_H

#include <gtk/gtkwindow.h>

G_BEGIN_DECLS

/* set the window title */
void gnome_window_toplevel_set_title (GtkWindow *w,
				      const gchar *doc_name,
				      const gchar *app_name,
				      const gchar *extension);

void gnome_window_set_icon (GtkWindow *window,
			    GdkPixbuf *pixbuf,
			    gboolean overwrite);

void gnome_window_set_icon_from_file (GtkWindow *window,
				      const char *filename,
				      gboolean overwrite);

G_END_DECLS

#endif /* GNOME_WINDOW_H */
