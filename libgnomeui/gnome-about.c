/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-about.c - An about box widget for gnome.

   Copyright (C) 2001 CodeFactory AB
   Copyright (C) Anders Carlsson <andersca@codefactory.se>

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Anders Carlsson <andersca@codefactory.se>
*/

#include "gnome-about.h"

GtkWidget *gnome_about_new (const gchar  *name,
			    const gchar  *version,
			    const gchar  *copyright,
			    const gchar  *comments,
			    const gchar **authors,
			    const gchar **documenters,
			    const gchar  *translator_credits,
			    GdkPixbuf    *logo_pixbuf)
{
	GtkWidget *about;

	/* Totally unusable stub for now */
	about = gtk_dialog_new ();

	return about;
}
