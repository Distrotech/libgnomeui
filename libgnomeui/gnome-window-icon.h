/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * gnome-window-icon.h: convenience functions for window mini-icons
 *
 * Copyright 2000 Helix Code, Inc.
 *
 * Author:  Jacob Berkman  <jacob@helixcode.com>
 */

/*
 * These functions are a convenience wrapper for the gdk_window_set_icon()
 * function.  They allow setting a default icon, which is used by many
 * top level windows in libgnomeui, such as GnomeApp and GnomeDialog
 * windows.
 *
 * They also handle drawing the icon on the iconified window's icon in
 * window managers such as TWM and Window Maker.
 *
 * These functions were added with the 1.2.0 release of the GNOME libraries
 * in May, 2000.  This means that not all users will have this functionality
 * in the GNOME libraries, and should only be used accordingly.  The header file
 * must be explicitely included, also (#include <libgnomeui/gnome-window-icon.h>).
 */

#ifndef GNOME_WINDOW_ICON_H
#define GNOME_WINDOW_ICON_H

#include <gdk_imlib.h>
#include <gtk/gtkwindow.h>

BEGIN_GNOME_DECLS

/* set an icon on a window */
void gnome_window_icon_set_from_default (GtkWindow *w);
void gnome_window_icon_set_from_file    (GtkWindow *w, const char    *filename);
void gnome_window_icon_set_from_imlib   (GtkWindow *w, GdkImlibImage *im);

/* set the default icon used */
void gnome_window_icon_set_default_from_file  (const char    *filename);
void gnome_window_icon_set_default_from_imlib (GdkImlibImage *im);

/* check for the GNOME_DESKTOP_ICON environment variable */
void gnome_window_icon_init (void);

END_GNOME_DECLS

#endif /* GNOME_WINDOW_ICON_H */
