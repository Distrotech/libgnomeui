/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * gnome-window.c: wrappers for setting window properties
 *
 * Copyright 2000, Chema Celorio
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 *
 * Authors:  Chema Celorio <chema@celorio.com>
 */


#include <config.h>
#include <glib.h>
#include <string.h>
#include <libgnome/gnome-program.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtksignal.h>
#include <gdk/gdkx.h>

#include "gnome-window.h"
#include "gnome-init.h"

/**
 * gnome_window_toplevel_set_title:
 * @window: A pointer to the toplevel window
 * @doc_name: the document name with extension (if any)
 * @app_name: the application name
 * @extension: the default extension that the application uses.
 *             NULL if there isn't a default extension.
 * 
 * Set the title of a toplevel window. Acording to gnome policy or
 * (if implemented) to a gnome setting.
 *
 **/
void
gnome_window_toplevel_set_title (GtkWindow *window, const gchar *doc_name,
				 const gchar *app_name, const gchar *extension)
{
    gchar *full_title;
    gchar *doc_name_clean = NULL;
	
    g_return_if_fail (GTK_IS_WINDOW (window));
    g_return_if_fail (doc_name != NULL);
    g_return_if_fail (app_name != NULL);

    /* Remove the extension from the doc_name*/
    if (extension != NULL) {
	gchar * pos = strstr (doc_name, extension);
	if (pos != NULL)
	    doc_name_clean = g_strndup (doc_name, pos - doc_name);
    }

    if (!doc_name_clean)
	doc_name_clean = g_strdup (doc_name);
	
    full_title = g_strdup_printf ("%s : %s", doc_name_clean, app_name);
    gtk_window_set_title (window, full_title);

    g_free (doc_name_clean);
    g_free (full_title);
}

typedef struct {
    GdkPixmap *icon_pixmap;
    GdkBitmap *icon_mask;
    GdkWindow *icon_window;
} IconInfo;

static void
gnome_window_realized (GtkWindow *window, IconInfo *pbi)
{
    gdk_window_set_icon (GTK_WIDGET (window)->window, pbi->icon_window,
			 pbi->icon_pixmap, pbi->icon_mask);
}

static void
gnome_window_destroyed (GtkWindow *window, IconInfo *pbi)
{
    gdk_pixmap_unref (pbi->icon_pixmap);
    gdk_bitmap_unref (pbi->icon_mask);
    gdk_window_unref (pbi->icon_window);
    g_free (pbi);
}

void
gnome_window_set_icon (GtkWindow *window, GdkPixbuf *pixbuf, gboolean overwrite)
{
    gboolean skip_connect = FALSE;
    IconInfo *pbi;

    pbi = gtk_object_get_data (GTK_OBJECT (window), "WM_HINTS.icon_info");
    if (pbi && !overwrite)
	return;
    if(pbi) {
	skip_connect = TRUE;
	gdk_pixmap_unref (pbi->icon_pixmap);
	gdk_pixmap_unref (pbi->icon_mask);
	gdk_window_unref (pbi->icon_window);
    } else
	pbi = g_new (IconInfo, 1);

    {
	GdkWindowAttr wa;
	wa.visual = gdk_rgb_get_visual ();
	wa.colormap = gdk_rgb_get_cmap ();
	pbi->icon_window = gdk_window_new (GDK_ROOT_PARENT (), &wa, GDK_WA_VISUAL|GDK_WA_COLORMAP);
    }
    gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pbi->icon_pixmap, &pbi->icon_mask, 128);

    if (!skip_connect) {
	gtk_object_set_data (GTK_OBJECT (window), "WM_HINTS.icon_info", pbi);
	gtk_signal_connect_after (GTK_OBJECT (window), "realize",
				  GTK_SIGNAL_FUNC (gnome_window_realized), pbi);
	gtk_signal_connect (GTK_OBJECT (window), "destroy",
			   GTK_SIGNAL_FUNC (gnome_window_destroyed), pbi);
    }

    if (GTK_WIDGET_REALIZED (window))
	gnome_window_realized (window, pbi);
}

void
gnome_window_set_icon_from_file (GtkWindow *window, const char *filename, gboolean overwrite)
{
    GdkPixbuf *pb;
    GError *error;

    error = NULL;
    pb = gdk_pixbuf_new_from_file (filename, &error);
    if (error != NULL) {
	g_warning (error->message);
	g_error_free (error);
    }
    if(!pb) {
	error = NULL;
	filename = gnome_program_locate_file (gnome_program_get (),
					      GNOME_FILE_DOMAIN_PIXMAP,
					      filename, TRUE, NULL);
		
	pb = gdk_pixbuf_new_from_file (filename, &error);
	if (error != NULL) {
	    g_warning (error->message);
	    g_error_free (error);
	}
	g_free ((gpointer)filename);
    }
    if(!pb)
	return;

    gnome_window_set_icon (window, pb, overwrite);
    gdk_pixbuf_unref (pb);
}

/**
 * gnome_window_icon_set_from_default:
 * @w: the #GtkWidget to set the icon on
 *
 * Description: Makes the #GtkWindow @w use the default icon.
 */
void
gnome_window_set_icon_from_default (GtkWindow *w)
{
	GnomeProgram *program;
	const gchar *default_icon = NULL;

	program = gnome_program_get ();
	g_object_get (G_OBJECT (program), LIBGNOMEUI_PARAM_DEFAULT_ICON, &default_icon);
	if (default_icon)
		gnome_window_set_icon_from_file (w, default_icon, FALSE);
}

