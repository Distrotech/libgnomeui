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
#include <gtk/gtkwindow.h>


#include "gnome-window.h"

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

