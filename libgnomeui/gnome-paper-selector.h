/* GNOME Paper Selector
 * Copyright (C) 1999 James Henstridge <james@daa.com.au>
 * All rights reserved.
 *
 * This replaces the paper selector by Dirk Luetjens.
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

#ifndef GNOME_PAPER_SELECTOR_H
#define GNOME_PAPER_SELECTOR_H

#include <gtk/gtk.h>

#include <libgnome/gnome-paper.h>

G_BEGIN_DECLS

#define OLD_GNOME_SELECTOR_API

#define GNOME_TYPE_PAPER_SELECTOR            (gnome_paper_selector_get_type ())
#define GNOME_PAPER_SELECTOR(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_PAPER_SELECTOR, GnomePaperSelector))
#define GNOME_PAPER_SELECTOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_PAPER_SELECTOR, GnomePaperSelectorClass))
#define GNOME_IS_PAPER_SELECTOR(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_PAPER_SELECTOR))
#define GNOME_IS_PAPER_SELECTOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_PAPER_SELECTOR))
#define GNOME_PAPER_SELECTOR_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_PAPER_SELECTOR, GnomePaperSelectorClass))

#define DIA_IS_PAGE_LAYOUT(obj) GTK_CHECK_TYPE(obj, gnome_paper_selector_get_type())

typedef struct _GnomePaperSelector        GnomePaperSelector;
typedef struct _GnomePaperSelectorPrivate GnomePaperSelectorPrivate;
typedef struct _GnomePaperSelectorClass   GnomePaperSelectorClass;

typedef enum {
  GNOME_PAPER_ORIENT_PORTRAIT,
  GNOME_PAPER_ORIENT_LANDSCAPE
} GnomePaperOrient;

struct _GnomePaperSelector {
  GtkTable parent;

  /*<private>*/
  GnomePaperSelectorPrivate *_priv;
};

struct _GnomePaperSelectorClass {
  GtkTableClass parent_class;

  void (*changed)(GnomePaperSelector *pl);
  void (*fittopage)(GnomePaperSelector *pl);
};

GtkType      gnome_paper_selector_get_type    (void) G_GNUC_CONST;
GtkWidget   *gnome_paper_selector_new(void);
GtkWidget   *gnome_paper_selector_new_with_unit (const GnomeUnit *unit);

const GnomePaper *gnome_paper_selector_get_paper (GnomePaperSelector *pl);
void         gnome_paper_selector_set_paper   (GnomePaperSelector *pl,
					       const gchar *paper);
void         gnome_paper_selector_get_margins (GnomePaperSelector *pl,
					       const GnomeUnit *unit,
					       gfloat *tmargin,
					       gfloat *bmargin,
					       gfloat *lmargin,
					       gfloat *rmargin);
void         gnome_paper_selector_set_margins (GnomePaperSelector *pl,
					       const GnomeUnit *unit,
					       gfloat tmargin, gfloat bmargin,
					       gfloat lmargin, gfloat rmargin);
GnomePaperOrient gnome_paper_selector_get_orientation
					      (GnomePaperSelector *pl);
void         gnome_paper_selector_set_orientation (GnomePaperSelector *pl,
						   GnomePaperOrient orient);
gfloat       gnome_paper_selector_get_scaling (GnomePaperSelector *self);
void         gnome_paper_selector_set_scaling (GnomePaperSelector *self,
					       gfloat scaling);

void         gnome_paper_selector_get_effective_area (GnomePaperSelector *self,
						      const GnomeUnit *unit,
						      gfloat *width,
						      gfloat *height);

/* compatibility with old interface ... */
#ifdef OLD_PAGE_SELECTOR_API
gfloat gnome_paper_selector_get_width  (GnomePaperSelector *gspaper);
gfloat gnome_paper_selector_get_height (GnomePaperSelector *gspaper);
gfloat gnome_paper_selector_get_left_margin   (GnomePaperSelector *gspaper);
gfloat gnome_paper_selector_get_right_margin  (GnomePaperSelector *gspaper);
gfloat gnome_paper_selector_get_top_margin    (GnomePaperSelector *gspaper);
gfloat gnome_paper_selector_get_bottom_margin (GnomePaperSelector *gspaper);
#endif

G_END_DECLS

#endif
