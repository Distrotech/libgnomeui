/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * gnome-window-icon.h: convenience functions for window mini-icons
 *
 * Copyright 2000, 2001 Ximian, Inc.
 *
 * Author:  Jacob Berkman  <jacob@ximian.com>
 */

/*
 * These functions are a convenience wrapper for the
 * gtk_window_set_icon_list() function.  They allow setting a default
 * icon, which is used by many top level windows in libgnomeui, such
 * as GnomeApp and GnomeDialog windows.
 *
 * They also handle drawing the icon on the iconified window's icon in
 * window managers such as TWM and Window Maker.
 *
 */

#ifndef GNOME_WINDOW_ICON_H
#define GNOME_WINDOW_ICON_H

#ifndef GNOME_DISABLE_DEPRECATED

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* set an icon on a window */
void gnome_window_icon_set_from_default   (GtkWindow *w);
void gnome_window_icon_set_from_file      (GtkWindow *w, const char  *filename);
void gnome_window_icon_set_from_file_list (GtkWindow *w, const char **filenames);

/* set the default icon used */
void gnome_window_icon_set_default_from_file      (const char  *filename);
void gnome_window_icon_set_default_from_file_list (const char **filenames);

/* check for the GNOME_DESKTOP_ICON environment variable */
void gnome_window_icon_init (void);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* GNOME_WINDOW_ICON_H */

