/* GnomePaperSelector widget 
 * Copyright (C) 1998 the Free Software Foundation
 *
 * Author: Dirk Luetjens <dirk@luedi.oche.de>
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

#ifndef GNOME_PAPER_SELECTOR_H
#define GNOME_PAPER_SELECTOR_H

#include <gtk/gtk.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define GNOME_TYPE_PAPER_SELECTOR            (gnome_paper_selector_get_type ())
#define GNOME_PAPER_SELECTOR(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_PAPER_SELECTOR, GnomePaperSelector))
#define GNOME_PAPER_SELECTOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_PAPER_SELECTOR, GnomePaperSelectorClass))
#define GNOME_IS_PAPER_SELECTOR(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_PAPER_SELECTOR))
#define GNOME_IS_PAPER_SELECTOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_PAPER_SELECTOR))


typedef struct _GnomePaperSelector      GnomePaperSelector;
typedef struct _GnomePaperSelectorClass GnomePaperSelectorClass;

struct _GnomePaperSelector {
  GtkVBox vbox;
  
  GtkWidget *paper;
  GtkWidget *width;
  GtkWidget *height;
  GtkWidget *unit;
  GtkWidget *unit_label;

  gint paper_id;
  gint width_id;
  gint height_id;
};

struct _GnomePaperSelectorClass {
  GtkVBoxClass parent_class;
};

guint		gnome_paper_selector_get_type	(void);
GtkWidget	*gnome_paper_selector_new	(void);

gchar		*gnome_paper_selector_get_name	(GnomePaperSelector *gspaper);
gfloat		gnome_paper_selector_get_width	(GnomePaperSelector *gspaper);
gfloat		gnome_paper_selector_get_height	(GnomePaperSelector *gspaper);
gfloat          gnome_paper_selector_get_left_margin   (GnomePaperSelector *gspaper);
gfloat          gnome_paper_selector_get_right_margin  (GnomePaperSelector *gspaper);
gfloat          gnome_paper_selector_get_top_margin    (GnomePaperSelector *gspaper);
gfloat          gnome_paper_selector_get_bottom_margin (GnomePaperSelector *gspaper);

  /* These are still only stubs. */
void		gnome_paper_selector_set_name	(GnomePaperSelector *gspaper,
						 const gchar *name);
void		gnome_paper_selector_set_width	(GnomePaperSelector *gspaper,
						 gfloat width);
void		gnome_paper_selector_set_height	(GnomePaperSelector *gspaper,
						 gfloat height);

END_GNOME_DECLS

#endif 





