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

/* #define PAGELAYOUT_TEST */

#define DEFAULT_PAPER "A4"
#define DEFAULT_UNIT "centimeter"

#include <string.h>

#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-uidefs.h>

#include "gnome-paper-selector.h"
#include "gnome-unit-spinner.h"

#include "portrait.xpm"
#include "landscape.xpm"

struct _GnomePaperSelectorPrivate {
  const GnomeUnit *unit;
  const GnomePaper *paper;

  GtkWidget *paper_size, *paper_label;
  GtkWidget *orient_portrait, *orient_landscape;
  GtkWidget *tmargin, *bmargin, *lmargin, *rmargin;
  GtkWidget *scaling;
  GtkWidget *fittopage;

  GtkWidget *darea;

  GdkGC *gc;

  /* position of paper preview */
  gint16 x, y, width, height;

  gboolean block_changed : 1;
};

static const GnomeUnit *default_unit = NULL;
static const GnomeUnit *point_unit = NULL;
static const GnomePaper *default_paper = NULL;

enum {
  CHANGED,
  FITTOPAGE,
  LAST_SIGNAL
};

static guint ps_signals[LAST_SIGNAL] = { 0 };
static GtkTableClass *parent_class;

static void gnome_paper_selector_class_init(GnomePaperSelectorClass *class);
static void gnome_paper_selector_init(GnomePaperSelector *self);
static void gnome_paper_selector_destroy(GtkObject *object);
static void gnome_paper_selector_finalize(GObject *object);

GtkType
gnome_paper_selector_get_type(void)
{
  static GtkType ps_type = 0;

  if (!ps_type) {
    GtkTypeInfo ps_info = {
      "GnomePaperSelector",
      sizeof(GnomePaperSelector),
      sizeof(GnomePaperSelectorClass),
      (GtkClassInitFunc) gnome_paper_selector_class_init,
      (GtkObjectInitFunc) gnome_paper_selector_init,
      NULL,
      NULL,
      NULL
    };
    ps_type = gtk_type_unique(gtk_table_get_type(), &ps_info);
  }
  return ps_type;
}

static void
gnome_paper_selector_class_init(GnomePaperSelectorClass *class)
{
  GtkObjectClass *object_class;
  GObjectClass *gobject_class;
  
  object_class = (GtkObjectClass*) class;
  gobject_class = (GObjectClass*) class;
  parent_class = gtk_type_class(gtk_table_get_type());

  ps_signals[CHANGED] =
    gtk_signal_new("changed",
		   GTK_RUN_FIRST,
		   GTK_CLASS_TYPE(object_class),
		   GTK_SIGNAL_OFFSET(GnomePaperSelectorClass, changed),
		   gtk_signal_default_marshaller,
		   GTK_TYPE_NONE, 0);
  ps_signals[FITTOPAGE] =
    gtk_signal_new("fittopage",
		   GTK_RUN_FIRST,
		   GTK_CLASS_TYPE(object_class),
		   GTK_SIGNAL_OFFSET(GnomePaperSelectorClass, fittopage),
		   gtk_signal_default_marshaller,
		   GTK_TYPE_NONE, 0);

  object_class->destroy = gnome_paper_selector_destroy;
  gobject_class->finalize = gnome_paper_selector_finalize;
}

static void fittopage_pressed(GnomePaperSelector *self);
static void darea_size_allocate(GnomePaperSelector *self,GtkAllocation *alloc);
static gint darea_expose_event(GnomePaperSelector *self, GdkEventExpose *ev);
static void paper_size_change(GtkMenuItem *item, GnomePaperSelector *self);
static void orient_changed(GnomePaperSelector *self);
static void margin_changed(GnomePaperSelector *self);
static void scale_changed(GnomePaperSelector *self);

static void
gnome_paper_selector_init(GnomePaperSelector *self)
{
  GtkWidget *frame, *box, *table, *menu, *menuitem, *vbox, *wid;
  GdkPixmap *pix;
  GdkBitmap *mask;
  const GList *papers;

  self->_priv = g_new0(GnomePaperSelectorPrivate, 1);

  if (default_unit == NULL)
    default_unit = gnome_unit_with_name(DEFAULT_UNIT);
  if (point_unit == NULL)
    point_unit = gnome_unit_with_name("point");
  if (default_paper == NULL)
    default_paper = gnome_paper_with_name(DEFAULT_PAPER);

  gtk_table_resize(GTK_TABLE(self), 3, 2);
  gtk_table_set_row_spacings(GTK_TABLE(self), 5);
  gtk_table_set_col_spacings(GTK_TABLE(self), 5);

  /* paper size */
  frame = gtk_frame_new(_("Paper Size"));
  gtk_table_attach(GTK_TABLE(self), frame, 0,1, 0,1,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  box = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  self->_priv->paper_size = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(box), self->_priv->paper_size, TRUE, FALSE, 0);

  menu = gtk_menu_new();
  for (papers = gnome_paper_name_list(); papers; papers = papers->next) {
    const GnomePaper *paper=gnome_paper_with_name((const gchar *)papers->data);

    menuitem = gtk_menu_item_new_with_label((const gchar *)papers->data);
    gtk_object_set_user_data(GTK_OBJECT(menuitem), (gpointer) paper);
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
		       GTK_SIGNAL_FUNC(paper_size_change), self);
    gtk_container_add(GTK_CONTAINER(menu), menuitem);
    gtk_widget_show(menuitem);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(self->_priv->paper_size), menu);
  gtk_widget_show(self->_priv->paper_size);

  self->_priv->paper_label = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(box), self->_priv->paper_label, TRUE, TRUE, 0);
  gtk_widget_show(self->_priv->paper_label);

  /* orientation */
  frame = gtk_frame_new(_("Orientation"));
  gtk_table_attach(GTK_TABLE(self), frame, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  box = gtk_hbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  self->_priv->orient_portrait = gtk_radio_button_new(NULL);
  vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
  gtk_container_add(GTK_CONTAINER(self->_priv->orient_portrait), vbox);
  gtk_widget_show(vbox);
  pix = gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(GTK_WIDGET(self)), &mask, NULL,
		portrait_xpm);
  wid = gtk_pixmap_new(pix, mask);
  gdk_pixmap_unref(pix);
  gdk_bitmap_unref(mask);
  gtk_box_pack_start(GTK_BOX(vbox), wid, TRUE, TRUE, 0);
  gtk_widget_show(wid);
  wid = gtk_label_new(_("Portrait"));
  gtk_box_pack_start(GTK_BOX(vbox), wid, TRUE, TRUE, 0);
  gtk_widget_show(wid);

  gtk_box_pack_start(GTK_BOX(box), self->_priv->orient_portrait, TRUE, TRUE, 0);
  gtk_widget_show(self->_priv->orient_portrait);

  self->_priv->orient_landscape = gtk_radio_button_new(
	gtk_radio_button_group(GTK_RADIO_BUTTON(self->_priv->orient_portrait)));
  vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
  gtk_container_add(GTK_CONTAINER(self->_priv->orient_landscape), vbox);
  gtk_widget_show(vbox);
  pix = gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(GTK_WIDGET(self)), &mask, NULL,
		landscape_xpm);
  wid = gtk_pixmap_new(pix, mask);
  gdk_pixmap_unref(pix);
  gdk_bitmap_unref(mask);
  gtk_box_pack_start(GTK_BOX(vbox), wid, TRUE, TRUE, 0);
  gtk_widget_show(wid);
  wid = gtk_label_new(_("Landscape"));
  gtk_box_pack_start(GTK_BOX(vbox), wid, TRUE, TRUE, 0);
  gtk_widget_show(wid);

  gtk_box_pack_start(GTK_BOX(box), self->_priv->orient_landscape, TRUE, TRUE, 0);
  gtk_widget_show(self->_priv->orient_landscape);

  /* margins */
  frame = gtk_frame_new(_("Margins"));
  gtk_table_attach(GTK_TABLE(self), frame, 0,1, 1,2,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  table = gtk_table_new(4, 2, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_widget_show(table);

  wid = gtk_label_new(_("Top:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 0,1,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->_priv->tmargin = gnome_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, default_unit);
  gtk_table_attach(GTK_TABLE(table), self->_priv->tmargin, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->_priv->tmargin);

  wid = gtk_label_new(_("Bottom:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 1,2,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->_priv->bmargin = gnome_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, default_unit);
  gtk_table_attach(GTK_TABLE(table), self->_priv->bmargin, 1,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->_priv->bmargin);

  wid = gtk_label_new(_("Left:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 2,3,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->_priv->lmargin = gnome_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, default_unit);
  gtk_table_attach(GTK_TABLE(table), self->_priv->lmargin, 1,2, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->_priv->lmargin);

  wid = gtk_label_new(_("Right:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 3,4,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->_priv->rmargin = gnome_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, default_unit);
  gtk_table_attach(GTK_TABLE(table), self->_priv->rmargin, 1,2, 3,4,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->_priv->rmargin);

  /* Scaling */
  frame = gtk_frame_new(_("Scaling"));
  gtk_table_attach(GTK_TABLE(self), frame, 0,1, 2,3,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  box = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  self->_priv->scaling = gtk_spin_button_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(100,1,10000, 1,10,10)), 1, 1);
  gtk_box_pack_start(GTK_BOX(box), self->_priv->scaling, TRUE, FALSE, 0);
  gtk_widget_show(self->_priv->scaling);

  self->_priv->fittopage = gtk_button_new_with_label(_("Fit to Page"));
  gtk_box_pack_start(GTK_BOX(box), self->_priv->fittopage, TRUE, FALSE, 0);
  gtk_widget_show(self->_priv->fittopage);

  /* the drawing area */
  self->_priv->darea = gtk_drawing_area_new();
  gtk_table_attach(GTK_TABLE(self), self->_priv->darea, 1,2, 1,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->_priv->darea);

  /* connect the signal handlers */
  gtk_signal_connect_object(GTK_OBJECT(self->_priv->orient_portrait), "toggled",
			    GTK_SIGNAL_FUNC(orient_changed), GTK_OBJECT(self));

  gtk_signal_connect_object(GTK_OBJECT(self->_priv->tmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->_priv->bmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->_priv->lmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->_priv->rmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));

  gtk_signal_connect_object(GTK_OBJECT(self->_priv->scaling), "changed",
			    GTK_SIGNAL_FUNC(scale_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->_priv->fittopage), "pressed",
			    GTK_SIGNAL_FUNC(fittopage_pressed),
			    GTK_OBJECT(self));

  gtk_signal_connect_object(GTK_OBJECT(self->_priv->darea), "size_allocate",
			    GTK_SIGNAL_FUNC(darea_size_allocate),
			    GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->_priv->darea), "expose_event",
			    GTK_SIGNAL_FUNC(darea_expose_event),
			    GTK_OBJECT(self));

  self->_priv->unit = default_unit;
  self->_priv->paper = default_paper;

  self->_priv->gc = NULL;
  self->_priv->block_changed = FALSE;
}

/**
 * gnome_paper_selector_new:
 * 
 * createa GnomePaperSelector, using centimeters as the display unit for
 * the spin buttons.
 * 
 * Return value: the GnomePaperSelector
 **/
GtkWidget *
gnome_paper_selector_new(void)
{
  return gnome_paper_selector_new_with_unit(default_unit);
}

/**
 * gnome_paper_selector_new_with_unit:
 * @unit: the display units for the spin buttons
 * 
 * creates a new GnomePaperSelector
 * 
 * Return value: the GnomePaperSelector
 **/
GtkWidget *
gnome_paper_selector_new_with_unit(const GnomeUnit *unit)
{
  GnomePaperSelector *self = gtk_type_new(gnome_paper_selector_get_type());

  if (unit) {
    self->_priv->unit = unit;
    gnome_unit_spinner_change_units(GNOME_UNIT_SPINNER(self->_priv->tmargin), unit);
    gnome_unit_spinner_change_units(GNOME_UNIT_SPINNER(self->_priv->bmargin), unit);
    gnome_unit_spinner_change_units(GNOME_UNIT_SPINNER(self->_priv->lmargin), unit);
    gnome_unit_spinner_change_units(GNOME_UNIT_SPINNER(self->_priv->rmargin), unit);
  }
  gnome_paper_selector_set_paper(self, DEFAULT_PAPER);
  return GTK_WIDGET(self);
}

/**
 * gnome_paper_selector_get_paper:
 * @self: the GnomePaperSelector
 * 
 * get the currently selected paper.
 * 
 * Return value: the GnomePaper representing the currently selected paper.
 **/
const GnomePaper *
gnome_paper_selector_get_paper(GnomePaperSelector *self)
{
  g_return_val_if_fail(self != NULL, NULL);

  return self->_priv->paper;
}

/**
 * gnome_paper_selector_set_paper:
 * @self: the GnomePaperSelector
 * @paper: the name of the paper.
 * 
 * change the currently selected paper.  This will also reset the margins
 * for the new paper.
 **/
void
gnome_paper_selector_set_paper(GnomePaperSelector *self, const gchar *paper)
{
  const GList *l;
  gint i;
  const GnomePaper *p = NULL;

  g_return_if_fail(self != NULL);

  if (paper)
    p = gnome_paper_with_name(paper);
  if (!p)
    p = default_paper;
  paper = gnome_paper_name(p);

  for (l = gnome_paper_name_list(), i = 0; l; l = l->next, i++)
    if (!strcmp((const gchar *)l->data, paper))
      break;
  gtk_option_menu_set_history(GTK_OPTION_MENU(self->_priv->paper_size), i);
  gtk_menu_item_activate(
	GTK_MENU_ITEM(GTK_OPTION_MENU(self->_priv->paper_size)->menu_item));
}

/**
 * gnome_paper_selector_get_margins:
 * @self: the GnomePaperSelector
 * @unit: the units to return the margin widths in.
 * @tmargin: a variable for the top margin, or NULL
 * @bmargin: a variable for the bottom margin, or NULL
 * @lmargin: a variable for the left margin, or NULL
 * @rmargin: a variable for the right margin or NULL
 * 
 * Get the currently set margins for the paper.  If the unit argument
 * is NULL, then the paper selector's default unit is used.
 **/
void
gnome_paper_selector_get_margins(GnomePaperSelector *self,
				 const GnomeUnit *unit,
				 gfloat *tmargin, gfloat *bmargin,
				 gfloat *lmargin, gfloat *rmargin)
{
  g_return_if_fail(self != NULL);

  if (!unit)
    unit = self->_priv->unit;

  if (tmargin)
    *tmargin = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->tmargin),
					    unit);
  if (bmargin)
    *bmargin = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->bmargin),
					    unit);
  if (lmargin)
    *lmargin = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->lmargin),
					    unit);
  if (rmargin)
    *rmargin = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->rmargin),
					    unit);
}

/**
 * gnome_paper_selector_set_margins:
 * @self: the GnomePaperSelector
 * @unit: the unit the margin widths are in terms of, or NULL
 * @tmargin: the new top margin
 * @bmargin: the new bottom margin
 * @lmargin: the new left margin
 * @rmargin: the new right margin
 * 
 * Set the margins for the paper selector.  If the unit argument is NULL,
 * the paper selector's default unit is used.
 **/
void
gnome_paper_selector_set_margins(GnomePaperSelector *self,
				 const GnomeUnit *unit,
				 gfloat tmargin, gfloat bmargin,
				 gfloat lmargin, gfloat rmargin)
{
  g_return_if_fail(self != NULL);

  if ( ! unit)
	  unit = self->_priv->unit;

  self->_priv->block_changed = TRUE;
  gnome_unit_spinner_set_value(GNOME_UNIT_SPINNER(self->_priv->tmargin), tmargin,
			       unit);
  gnome_unit_spinner_set_value(GNOME_UNIT_SPINNER(self->_priv->bmargin), bmargin,
			       unit);
  gnome_unit_spinner_set_value(GNOME_UNIT_SPINNER(self->_priv->lmargin), lmargin,
			       unit);
  gnome_unit_spinner_set_value(GNOME_UNIT_SPINNER(self->_priv->rmargin), rmargin,
			       unit);
  self->_priv->block_changed = FALSE;

  gtk_signal_emit(GTK_OBJECT(self), ps_signals[CHANGED]);
}

/**
 * gnome_paper_selector_get_orientation:
 * @self: the GnomePaperSelector
 * 
 * get the currently set orientation of the paper.
 * 
 * Return value: the paper orientation.
 **/
GnomePaperOrient
gnome_paper_selector_get_orientation(GnomePaperSelector *self)
{
  g_return_val_if_fail(self != NULL, GNOME_PAPER_ORIENT_PORTRAIT);

  if (GTK_TOGGLE_BUTTON(self->_priv->orient_portrait)->active)
    return GNOME_PAPER_ORIENT_PORTRAIT;
  else
    return GNOME_PAPER_ORIENT_LANDSCAPE;
}

/**
 * gnome_paper_selector_set_orientation:
 * @self: the GnomePaperSelector
 * @orient: the new paper orientation.
 * 
 * Set the paper orientation for the paper selector.
 **/
void
gnome_paper_selector_set_orientation(GnomePaperSelector *self,
				     GnomePaperOrient orient)
{
  g_return_if_fail(self != NULL);

  switch (orient) {
  case GNOME_PAPER_ORIENT_PORTRAIT:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->_priv->orient_portrait),
				 TRUE);
    break;
  case GNOME_PAPER_ORIENT_LANDSCAPE:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->_priv->orient_landscape),
				 TRUE);
    break;
  }
}

/**
 * gnome_paper_selector_get_scaling:
 * @self: the GnomePaperSelector
 * 
 * Get the scaling factor for the paper selector
 * 
 * Return value: the scaling factor.
 **/
gfloat
gnome_paper_selector_get_scaling(GnomePaperSelector *self)
{
  g_return_val_if_fail(self != NULL, 0.0);

  return GTK_SPIN_BUTTON(self->_priv->scaling)->adjustment->value / 100.0;
}

/**
 * gnome_paper_selector_set_scaling:
 * @self: the GnomePaperSelector
 * @scaling: the new scaling factor
 * 
 * Set the scaling factor.
 **/
void
gnome_paper_selector_set_scaling(GnomePaperSelector *self, gfloat scaling)
{
  g_return_if_fail(self != NULL);

  GTK_SPIN_BUTTON(self->_priv->scaling)->adjustment->value = scaling * 100.0;
  gtk_adjustment_value_changed(GTK_SPIN_BUTTON(self->_priv->scaling)->adjustment);
}

/**
 * gnome_paper_selector_get_effective_area:
 * @self: the GnomePaperSelector
 * @unit: the unit the width/height should be in, or NULL
 * @width: a variable to return the width in
 * @height: a variable to return the height in 
 * 
 * Returns the effective area of the paper.  This takes the paper size,
 * margin widths and scaling factor into account.
 **/
void
gnome_paper_selector_get_effective_area(GnomePaperSelector *self,
					const GnomeUnit *unit,
					gfloat *width, gfloat *height)
{
  gfloat h, w, scaling;

  g_return_if_fail(self != NULL);

  if ( ! unit)
	  unit = self->_priv->unit;

  if (GTK_TOGGLE_BUTTON(self->_priv->orient_portrait)->active) {
    w = gnome_paper_pswidth(self->_priv->paper);
    h = gnome_paper_psheight(self->_priv->paper);
  } else {
    h = gnome_paper_pswidth(self->_priv->paper);
    w = gnome_paper_psheight(self->_priv->paper);
  }
  w = gnome_unit_convert(w, point_unit, unit);
  h = gnome_unit_convert(h, point_unit, unit);

  h -= gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->tmargin), unit);
  h -= gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->bmargin), unit);
  w -= gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->lmargin), unit);
  w -= gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->rmargin), unit);
  scaling = GTK_SPIN_BUTTON(self->_priv->scaling)->adjustment->value / 100.0;
  h /= scaling;
  w /= scaling;

  if (width)  *width = w;
  if (height) *height = h;
}

static void
fittopage_pressed(GnomePaperSelector *self)
{
  gtk_signal_emit(GTK_OBJECT(self), ps_signals[FITTOPAGE]);
}

static void size_page(GnomePaperSelector *self, GtkAllocation *a)
{
  self->_priv->width = a->width - 3;
  self->_priv->height = a->height - 3;

  /* change to correct metrics */
  if (GTK_TOGGLE_BUTTON(self->_priv->orient_portrait)->active) {
    if (self->_priv->width * gnome_paper_psheight(self->_priv->paper) >
	self->_priv->height * gnome_paper_pswidth(self->_priv->paper))
      self->_priv->width = self->_priv->height * gnome_paper_pswidth(self->_priv->paper) /
	gnome_paper_psheight(self->_priv->paper);
    else
      self->_priv->height = self->_priv->width * gnome_paper_psheight(self->_priv->paper) /
	gnome_paper_pswidth(self->_priv->paper);
  } else {
    if (self->_priv->width * gnome_paper_pswidth(self->_priv->paper) >
	self->_priv->height * gnome_paper_psheight(self->_priv->paper))
      self->_priv->width = self->_priv->height * gnome_paper_psheight(self->_priv->paper) /
	gnome_paper_pswidth(self->_priv->paper);
    else
      self->_priv->height = self->_priv->width * gnome_paper_pswidth(self->_priv->paper) /
	gnome_paper_psheight(self->_priv->paper);
  }

  self->_priv->x = (a->width - self->_priv->width - 3) / 2;
  self->_priv->y = (a->height - self->_priv->height - 3) / 2;
}

static void
darea_size_allocate(GnomePaperSelector *self, GtkAllocation *allocation)
{
  size_page(self, allocation);
}

static gint
darea_expose_event(GnomePaperSelector *self, GdkEventExpose *event)
{
  GdkWindow *window= self->_priv->darea->window;
  gfloat val;
  gint num;
  GdkGC *black_gc = gtk_widget_get_style(GTK_WIDGET(self))->black_gc;
  GdkGC *white_gc = gtk_widget_get_style(GTK_WIDGET(self))->white_gc;

  if (!window)
    return FALSE;

  /* setup gc ... */
  if (!self->_priv->gc) {
    GdkColor blue;

    self->_priv->gc = gdk_gc_new(window);
    blue.red = 0;
    blue.green = 0;
    blue.blue = 0x7fff;
    gdk_color_alloc(gtk_widget_get_colormap(GTK_WIDGET(self)), &blue);
    gdk_gc_set_foreground(self->_priv->gc, &blue);
  }

  gdk_window_clear_area (window,
                         0, 0,
                         self->_priv->darea->allocation.width,
                         self->_priv->darea->allocation.height);

  /* draw the page image */
  gdk_draw_rectangle(window, black_gc, TRUE, self->_priv->x+3, self->_priv->y+3,
		     self->_priv->width, self->_priv->height);
  gdk_draw_rectangle(window, white_gc, TRUE, self->_priv->x, self->_priv->y,
		     self->_priv->width, self->_priv->height);
  gdk_draw_rectangle(window, black_gc, FALSE, self->_priv->x, self->_priv->y,
		     self->_priv->width-1, self->_priv->height-1);

  /* draw margins */
  if (GTK_TOGGLE_BUTTON(self->_priv->orient_portrait)->active) {
    val = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->tmargin),
				       point_unit);
    num = self->_priv->y + val * self->_priv->height /gnome_paper_psheight(self->_priv->paper);
    gdk_draw_line(window, self->_priv->gc, self->_priv->x+1, num, self->_priv->x+self->_priv->width-2,num);

    val = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->bmargin),
				       point_unit);
    num = self->_priv->y + self->_priv->height -
      val * self->_priv->height / gnome_paper_psheight(self->_priv->paper);
    gdk_draw_line(window, self->_priv->gc, self->_priv->x+1, num, self->_priv->x+self->_priv->width-2,num);

    val = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->lmargin),
				       point_unit);
    num = self->_priv->x + val * self->_priv->width / gnome_paper_pswidth(self->_priv->paper);
    gdk_draw_line(window, self->_priv->gc, num, self->_priv->y+1,num,self->_priv->y+self->_priv->height-2);

    val = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->rmargin),
				       point_unit);
    num = self->_priv->x + self->_priv->width -
      val * self->_priv->width / gnome_paper_pswidth(self->_priv->paper);
    gdk_draw_line(window, self->_priv->gc, num, self->_priv->y+1,num,self->_priv->y+self->_priv->height-2);
  } else {
    val = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->tmargin),
				       point_unit);
    num = self->_priv->y + val * self->_priv->height /gnome_paper_pswidth(self->_priv->paper);
    gdk_draw_line(window, self->_priv->gc, self->_priv->x+1, num, self->_priv->x+self->_priv->width-2,num);

    val = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->bmargin),
				       point_unit);
    num = self->_priv->y + self->_priv->height -
      val * self->_priv->height / gnome_paper_pswidth(self->_priv->paper);
    gdk_draw_line(window, self->_priv->gc, self->_priv->x+1, num, self->_priv->x+self->_priv->width-2,num);

    val = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->lmargin),
				       point_unit);
    num = self->_priv->x + val * self->_priv->width / gnome_paper_psheight(self->_priv->paper);
    gdk_draw_line(window, self->_priv->gc, num, self->_priv->y+1,num,self->_priv->y+self->_priv->height-2);

    val = gnome_unit_spinner_get_value(GNOME_UNIT_SPINNER(self->_priv->rmargin),
				       point_unit);
    num = self->_priv->x + self->_priv->width -
      val * self->_priv->width / gnome_paper_psheight(self->_priv->paper);
    gdk_draw_line(window, self->_priv->gc, num, self->_priv->y+1,num,self->_priv->y+self->_priv->height-2);
  }

  return FALSE;
}

static void
paper_size_change(GtkMenuItem *item, GnomePaperSelector *self)
{
  gchar buf[512];

  self->_priv->paper = (const GnomePaper *)gtk_object_get_user_data(GTK_OBJECT(item));
  size_page(self, &self->_priv->darea->allocation);
  gtk_widget_queue_draw(self->_priv->darea);

  self->_priv->block_changed = TRUE;
  gnome_unit_spinner_set_value(GNOME_UNIT_SPINNER(self->_priv->tmargin),
			     gnome_paper_tmargin(self->_priv->paper), point_unit);
  gnome_unit_spinner_set_value(GNOME_UNIT_SPINNER(self->_priv->bmargin),
			     gnome_paper_bmargin(self->_priv->paper), point_unit);
  gnome_unit_spinner_set_value(GNOME_UNIT_SPINNER(self->_priv->lmargin),
			     gnome_paper_lmargin(self->_priv->paper), point_unit);
  gnome_unit_spinner_set_value(GNOME_UNIT_SPINNER(self->_priv->rmargin),
			     gnome_paper_rmargin(self->_priv->paper), point_unit);

  if (GTK_TOGGLE_BUTTON(self->_priv->orient_portrait)->active) {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->tmargin))->upper =
      gnome_paper_psheight(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->bmargin))->upper =
      gnome_paper_psheight(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->lmargin))->upper =
      gnome_paper_pswidth(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->rmargin))->upper =
      gnome_paper_pswidth(self->_priv->paper);
  } else {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->tmargin))->upper =
      gnome_paper_pswidth(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->bmargin))->upper =
      gnome_paper_pswidth(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->lmargin))->upper =
      gnome_paper_psheight(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->rmargin))->upper =
      gnome_paper_psheight(self->_priv->paper);
  }
  self->_priv->block_changed = FALSE;

  g_snprintf(buf, sizeof(buf), _("%0.3g%s x %0.3g%s"),
	     gnome_unit_convert(gnome_paper_pswidth(self->_priv->paper),
				point_unit, self->_priv->unit),
	     gnome_unit_abbrev(self->_priv->unit),
	     gnome_unit_convert(gnome_paper_psheight(self->_priv->paper),
				point_unit, self->_priv->unit),
	     gnome_unit_abbrev(self->_priv->unit));
  gtk_label_set(GTK_LABEL(self->_priv->paper_label), buf);

  gtk_signal_emit(GTK_OBJECT(self), ps_signals[CHANGED]);
}

static void
orient_changed(GnomePaperSelector *self)
{
  size_page(self, &self->_priv->darea->allocation);
  gtk_widget_queue_draw(self->_priv->darea);

  if (GTK_TOGGLE_BUTTON(self->_priv->orient_portrait)->active) {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->tmargin))->upper =
      gnome_paper_psheight(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->bmargin))->upper =
      gnome_paper_psheight(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->lmargin))->upper =
      gnome_paper_pswidth(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->rmargin))->upper =
      gnome_paper_pswidth(self->_priv->paper);
  } else {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->tmargin))->upper =
      gnome_paper_pswidth(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->bmargin))->upper =
      gnome_paper_pswidth(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->lmargin))->upper =
      gnome_paper_psheight(self->_priv->paper);
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->_priv->rmargin))->upper =
      gnome_paper_psheight(self->_priv->paper);
  }

  if (!self->_priv->block_changed)
    gtk_signal_emit(GTK_OBJECT(self), ps_signals[CHANGED]);
}

static void
margin_changed(GnomePaperSelector *self)
{
  gtk_widget_queue_draw(self->_priv->darea);
  if (!self->_priv->block_changed)
    gtk_signal_emit(GTK_OBJECT(self), ps_signals[CHANGED]);
}

static void
scale_changed(GnomePaperSelector *self)
{
  if (!self->_priv->block_changed)
    gtk_signal_emit(GTK_OBJECT(self), ps_signals[CHANGED]);
}

static void
gnome_paper_selector_destroy(GtkObject *object)
{
  GnomePaperSelector *self;

  g_return_if_fail(object != NULL);

  /* remember, destroy can be run multiple times! */

  self = GNOME_PAPER_SELECTOR(object);
  if (self->_priv->gc)
    gdk_gc_unref(self->_priv->gc);
  self->_priv->gc = NULL;

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

static void
gnome_paper_selector_finalize(GObject *object)
{
  GnomePaperSelector *self;

  g_return_if_fail(object != NULL);

  self = GNOME_PAPER_SELECTOR(object);

  g_free(self->_priv);
  self->_priv = NULL;

  if (G_OBJECT_CLASS(parent_class)->finalize)
    (* G_OBJECT_CLASS(parent_class)->finalize)(object);
}



#ifdef OLD_PAGE_SELECTOR_API
/**
 * gnome_paper_selector_get_width:
 * @gspaper: the GnomePaperSelector
 * 
 * Get the paper width in points.  This function is provided for backward
 * compatibility.
 * 
 * Return value: the paper width in points.
 **/
gfloat
gnome_paper_selector_get_width(GnomePaperSelector *gspaper)
{
  g_return_val_if_fail(gspaper != NULL, 0);

  return gnome_paper_pswidth(gnome_paper_selector_get_paper(gspaper));
}
/**
 * gnome_paper_selector_get_height:
 * @gspaper: the GnomePaperSelector
 * 
 * Get the paper height in points.  This function is provided for backward
 * compatibility.
 * 
 * Return value: the paper height in points.
 **/
gfloat
gnome_paper_selector_get_height(GnomePaperSelector *gspaper);
{
  g_return_val_if_fail(gspaper != NULL, 0);

  return gnome_paper_psheight(gnome_paper_selector_get_paper(gspaper));
}
/**
 * gnome_paper_selector_get_left_margin:
 * @gspaper: the GnomePaperSelector
 * 
 * Get the left margin width in points.  This function is provided for
 * backward compatibility.
 * 
 * Return value: the left margin width in points.
 **/
gfloat
gnome_paper_selector_get_left_margin(GnomePaperSelector *gspaper);
{
  gfloat margin;

  g_return_val_if_fail(gspaper != NULL, 0);

  gnome_paper_selector_get_margins(gspaper, point_unit,
				   NULL, NULL, &margin, NULL);
  return margin;
}
/**
 * gnome_paper_selector_get_right_margin:
 * @gspaper: the GnomePaperSelector
 * 
 * Get the right margin width in points.  This function is provided for
 * backward compatibility.
 * 
 * Return value: the right margin width in points.
 **/
gfloat
gnome_paper_selector_get_right_margin(GnomePaperSelector *gspaper);
{
  gfloat margin;

  g_return_val_if_fail(gspaper != NULL, 0);

  gnome_paper_selector_get_margins(gspaper, point_unit,
				   NULL, NULL, NULL, &margin);
  return margin;
}
/**
 * gnome_paper_selector_get_top_margin:
 * @gspaper: the GnomePaperSelector
 * 
 * Get the top margin width in points.  This function is provided for
 * backward compatibility.
 * 
 * Return value: the top margin width in points.
 **/
gfloat
gnome_paper_selector_get_top_margin(GnomePaperSelector *gspaper);
{
  gfloat margin;

  g_return_val_if_fail(gspaper != NULL, 0);

  gnome_paper_selector_get_margins(gspaper, point_unit,
				   &margin, NULL, NULL, NULL);
  return margin;
}
/**
 * gnome_paper_selector_get_bottom_margin:
 * @gspaper: the GnomePaperSelector
 * 
 * Get the bottom margin width in points.  This function is provided for
 * backward compatibility.
 * 
 * Return value: the bottom margin width in points.
 **/
gfloat
gnome_paper_selector_get_bottom_margin(GnomePaperSelector *gspaper);
{
  gfloat margin;

  g_return_val_if_fail(gspaper != NULL, 0);

  gnome_paper_selector_get_margins(gspaper, point_unit,
				   NULL, &margin, NULL, NULL);
  return margin;
}
#endif


#ifdef PAGELAYOUT_TEST

void
changed_signal(GnomePaperSelector *self)
{
  g_message("changed");
}
void
fittopage_signal(GnomePaperSelector *self)
{
  g_message("fit to page");
}

void
main(int argc, char **argv)
{
  GtkWidget *win, *ps;

  gtk_init(&argc, &argv);

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win), _("Page Setup"));
  gtk_signal_connect(GTK_OBJECT(win), "destroy",
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  ps = gnome_paper_selector_new();
  gtk_container_set_border_width(GTK_CONTAINER(ps), 5);
  gtk_container_add(GTK_CONTAINER(win), ps);
  gtk_widget_show(ps);

  gtk_signal_connect(GTK_OBJECT(ps), "changed",
		     GTK_SIGNAL_FUNC(changed_signal), NULL);
  gtk_signal_connect(GTK_OBJECT(ps), "fittopage",
		     GTK_SIGNAL_FUNC(fittopage_signal), NULL);

  gtk_widget_show(win);
  gtk_main();
}

#endif
