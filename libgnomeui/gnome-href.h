/* WARNING ____ IMMATURE API ____ liable to change */

/* gnome-href.h
 * Copyright (C) 1998, James Henstridge <james@daa.com.au>
 * All rights reserved.
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
/*
  @NOTATION@
*/

#ifndef GNOME_HREF_H
#define GNOME_HREF_H

#include <glib.h>
#include <gtk/gtkbutton.h>


G_BEGIN_DECLS

#define GNOME_TYPE_HREF            (gnome_href_get_type ())
#define GNOME_HREF(obj)            (GTK_CHECK_CAST((obj), GNOME_TYPE_HREF, GnomeHRef))
#define GNOME_HREF_CLASS(klass)    (GTK_CHECK_CLASS_CAST((klass), GNOME_TYPE_HREF, GnomeHRefClass))
#define GNOME_IS_HREF(obj)         (GTK_CHECK_TYPE((obj), GNOME_TYPE_HREF))
#define GNOME_IS_HREF_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_HREF))
#define GNOME_HREF_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_HREF, GnomeHRefClass))

typedef struct _GnomeHRef        GnomeHRef;
typedef struct _GnomeHRefPrivate GnomeHRefPrivate;
typedef struct _GnomeHRefClass   GnomeHRefClass;


struct _GnomeHRef {
  GtkButton button;

  /*< private >*/
  GnomeHRefPrivate *_priv;
};

struct _GnomeHRefClass {
  GtkButtonClass parent_class;
};

/*
 * GNOME href class methods
 */

GtkType gnome_href_get_type(void) G_GNUC_CONST;
GtkWidget *gnome_href_new(const gchar *url, const gchar *text);

/* for bindings and subclassing, use the gnome_href_new from C */
void gnome_href_construct(GnomeHRef *href,
			  const gchar *url,
			  const gchar *text);

void gnome_href_set_url(GnomeHRef *href, const gchar *url);
const gchar *gnome_href_get_url(GnomeHRef *href);

void gnome_href_set_text(GnomeHRef *href, const gchar *text);
const gchar *gnome_href_get_text(GnomeHRef *href);

#ifndef GNOME_DISABLE_DEPRECATED
/* DEPRECATED!, use set/get_text */
void gnome_href_set_label(GnomeHRef *href, const gchar *label);
const gchar *gnome_href_get_label(GnomeHRef *href);
#endif /* GNOME_DISABLE_DEPRECATED */

G_END_DECLS

#endif

