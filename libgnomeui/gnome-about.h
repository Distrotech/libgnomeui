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

/* 
 * "About..." Widget
 *
 * AUTHOR:
 * Cesar Miquel <miquel@df.uba.ar>
 * REWRITTEN BY:
 * Gergõ Érdi <cactus@cactus.rulez.org>
 *
 * DESCRIPTION:
 *	A very specialized widget to display "About this program"-like
 * boxes.
 */

#ifndef __GNOME_ABOUT_H__
#define __GNOME_ABOUT_H__


#include <gtk/gtkdialog.h>

G_BEGIN_DECLS

#define GNOME_TYPE_ABOUT            (gnome_about_get_type ())
#define GNOME_ABOUT(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ABOUT, GnomeAbout))
#define GNOME_ABOUT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ABOUT, GnomeAboutClass))
#define GNOME_IS_ABOUT(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ABOUT))
#define GNOME_IS_ABOUT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ABOUT))
#define GNOME_ABOUT_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_ABOUT, GnomeAboutClass))


typedef struct _GnomeAbout        GnomeAbout;
typedef struct _GnomeAboutPrivate GnomeAboutPrivate;
typedef struct _GnomeAboutClass   GnomeAboutClass;

struct _GnomeAbout
{
  GtkDialog dialog;

  /*< private >*/
  GnomeAboutPrivate *_priv;
};

struct _GnomeAboutClass
{
  GtkDialogClass parent_class;
};


GtkType    gnome_about_get_type       (void) G_GNUC_CONST;
/* Main routine that creates the widget 
 *
 * USAGE:
 *
 *	const gchar *authors[] = {"author1", "author2", ..., NULL};
 *        (note: not having the 'const' will cause a warning during compile.)
 * 
 *	GtkWidget *about = gnome_about_new ( _("GnoApp"), "1.2b",
 *				_("Copyright FSF (C) 1998"),
 *				authors,
 *				"Comment line 1\nLine 2",
 *				"/usr/local/share/pixmaps/gnoapp-logo.xpm");
 *	gtk_widget_show (about);		
 */

GtkWidget* gnome_about_new	(const gchar	*title, /* Name of the application. */
				 const gchar	*version, /* Version. */
				 const gchar	*copyright, /* Copyright notice (one
						       line.) */
				 const gchar	**authors, /* NULL terminated list of
						      authors. */
				 const gchar	*comments, /* Other comments. */
				 const gchar	*logo /* A logo pixmap file. */
				 );

GtkWidget* gnome_about_new_with_url	(const gchar *title, /* Name of the application. */
					 const gchar *version, /* Version. */
					 const gchar *url, /* Application URL */
					 const gchar *copyright, /* Copyright notice (one line.) */
					 const gchar **authors, /* NULL terminated list of authors. */
					 const gchar **author_urls, /* NULL terminated list of author URLs */
					 const gchar *comments, /* Other comments. */
					 const gchar *logo /* A logo pixmap file. */
					 );

/* Only for use by bindings to languages other than C; don't use
   in applications. */
void
gnome_about_construct (GnomeAbout *about,
		       const gchar *title,
		       const gchar *version,
		       const gchar *url,
		       const gchar *copyright,
		       const gchar **authors,
		       const gchar **author_urls,
		       const gchar *comments,
		       const gchar *logo);
G_END_DECLS

#endif /* __GNOME_ABOUT_H__ */


