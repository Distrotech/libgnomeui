/* DEPRECATED */
/* WARNING ____ IMMATURE API ____ liable to change */

/* gnome-procbar.h - Gnome Process Bar.

   Copyright (C) 1998 Martin Baulig
   All rights reserved.

   Based on the orignal gtop/procbar.c from Radek Doulik.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.
*/
/*
  @NOTATION@
*/

#ifndef __GNOME_PROCBAR_H__
#define __GNOME_PROCBAR_H__

#include <libgnome/gnome-defs.h>
#include <gtk/gtkhbox.h>

BEGIN_GNOME_DECLS

#define GNOME_TYPE_PROC_BAR            (gnome_proc_bar_get_type ())
#define GNOME_PROC_BAR(obj)	       (GTK_CHECK_CAST ((obj), GNOME_TYPE_PROC_BAR, GnomeProcBar))
#define GNOME_PROC_BAR__CLASS(klass)   (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_PROC_BAR, GnomeProcBarClass))
#define GNOME_IS_PROC_BAR(obj)	       (GTK_CHECK_TYPE ((obj), GNOME_TYPE_PROC_BAR))
#define GNOME_IS_PROC_BAR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_PROC_BAR))
#define GNOME_PROC_BAR_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_PROC_BAR, GnomeProcBarClass))

typedef struct _GnomeProcBar        GnomeProcBar;
typedef struct _GnomeProcBarPrivate GnomeProcBarPrivate;
typedef struct _GnomeProcBarClass   GnomeProcBarClass;

struct _GnomeProcBar {
	GtkHBox hbox;

	/*< private >*/
	GnomeProcBarPrivate *_priv;
};

struct _GnomeProcBarClass {
	GtkHBoxClass parent_class;
};

guint       gnome_proc_bar_get_type        (void);
GtkWidget * gnome_proc_bar_new             (GtkWidget *label,
					    gint n, const GdkColor *colors,
					    gboolean (*cb)(gpointer));
void        gnome_proc_bar_construct       (GnomeProcBar *pb,
					    GtkWidget *label,
					    gint n, const GdkColor *colors,
					    gboolean (*cb)(gpointer));
void        gnome_proc_bar_set_values      (GnomeProcBar *pb, const guint val []);
void        gnome_proc_bar_set_orient      (GnomeProcBar *pb, gboolean vertical);
void        gnome_proc_bar_start           (GnomeProcBar *pb, gint gtime, gpointer data);
void        gnome_proc_bar_stop            (GnomeProcBar *pb);
void        gnome_proc_bar_update          (GnomeProcBar *pb, const GdkColor *colors);

END_GNOME_DECLS

#endif
