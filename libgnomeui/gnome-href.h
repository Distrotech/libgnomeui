/* WARNING ____ IMMATURE API ____ liable to change */

/* gnome-href.h
 * Copyright (C) 1998, James Henstridge <james@daa.com.au>
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
 */

#ifndef GNOME_HREF_H
#define GNOME_HREF_H

#include <glib.h>
#include <gtk/gtkbutton.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define GNOME_TYPE_HREF            (gnome_href_get_type ())
#define GNOME_HREF(obj)            (GTK_CHECK_CAST((obj), GNOME_TYPE_HREF, GnomeHRef))
#define GNOME_HREF_CLASS(klass)    (GTK_CHECK_CLASS_CAST((klass), GNOME_TYPE_HREF, GnomeHRefClass))
#define GNOME_IS_HREF(obj)         (GTK_CHECK_TYPE((obj), GNOME_TYPE_HREF))
#define GNOME_IS_HREF_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_HREF))

typedef struct _GnomeHRef GnomeHRef;
typedef struct _GnomeHRefClass GnomeHRefClass;


struct _GnomeHRef {
  GtkButton button;

  gchar *url;
  GtkWidget *label;
};

struct _GnomeHRefClass {
  GtkButtonClass parent_class;
};

/*
 * GNOME href class methods
 */

guint gnome_href_get_type(void);
GtkWidget *gnome_href_new(const gchar *url, const gchar *label);

void gnome_href_set_url(GnomeHRef *href, const gchar *url);
gchar *gnome_href_get_url(GnomeHRef *href);

void gnome_href_set_label(GnomeHRef *href, const gchar *label);
gchar *gnome_href_get_label(GnomeHRef *href);

/* the label can be accessed with gtk_label_get and gtk_label_get */

END_GNOME_DECLS
#endif

