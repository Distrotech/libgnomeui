/*
 * Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation
 * Copyright (C) 2001 Anders Carlsson
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
 *
 * Author: Anders Carlsson <andersca@gnu.org>
 *
 * Based on the GNOME 1.0 icon item by Miguel de Icaza and Federico Mena.
 */

#include <config.h>
#include <math.h>
#include <libgnome/gnome-macros.h>
#include <gtk/gtkentry.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include "gnome-icon-item.h"

/* Margins used to display the information */
#define MARGIN_X 2
#define MARGIN_Y 2

/* Private part of the GnomeIconTextItem structure */
struct _GnomeIconTextItemPrivate {

	/* Our layout */
	PangoLayout *layout;
	int layout_width, layout_height;

	/* Whether the user pressed the mouse while the item was unselected */
	guint unselected_click : 1;

	/* Whether we need to update the position */
	guint need_pos_update : 1;

	/* Whether we need to update the font */
	guint need_font_update : 1;

	/* Whether we need to update the text */
	guint need_text_update : 1;

	/* Whether we need to update because the editing/selected state changed */
	guint need_state_update : 1;

};

enum {
	TEXT_CHANGED,
	HEIGHT_CHANGED,
	WIDTH_CHANGED,
	EDITING_STARTED,
	EDITING_STOPPED,
	SELECTION_STARTED,
	SELECTION_STOPPED,
	LAST_SIGNAL
};

static guint iti_signals [LAST_SIGNAL] = { 0 };

GNOME_CLASS_BOILERPLATE (GnomeIconTextItem, gnome_icon_text_item,
			 GnomeCanvasItem, GNOME_TYPE_CANVAS_ITEM);

/* Updates the pango layout width */
static void
update_pango_layout (GnomeIconTextItem *iti)
{
	GnomeIconTextItemPrivate *priv;
	PangoRectangle bounds;
	
	priv = iti->_priv;

	if (pango_layout_get_width (priv->layout) / PANGO_SCALE == iti->width)
		return;

	pango_layout_set_width (priv->layout, iti->width * PANGO_SCALE);

	pango_layout_get_pixel_extents (iti->_priv->layout, NULL, &bounds);

	priv->layout_width = bounds.width;
	priv->layout_height = bounds.height;
}

/* Stops the editing state of an icon text item */
static void
iti_stop_editing (GnomeIconTextItem *iti)
{
	g_print ("yes, stopping the edit!\n");
}

/* Starts the editing state of an icon text item */
static void
iti_start_editing (GnomeIconTextItem *iti)
{
	GnomeIconTextItemPrivate *priv;

	priv = iti->_priv;

	if (iti->editing)
		return;

	iti->editing = TRUE;

	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));

	gtk_signal_emit (GTK_OBJECT (iti), iti_signals[EDITING_STARTED]);

	g_print ("yes, starting the edit!\n");
}

/* Accepts the text in the off-screen entry of an icon text item */
static void
iti_edition_accept (GnomeIconTextItem *iti)
{
	g_print ("yes, accepting the input!\n");
}

/* Recomputes the bounding box of an icon text item */
static void
recompute_bounding_box (GnomeIconTextItem *iti)
{
	GnomeCanvasItem *item;
	double affine[6];
	ArtPoint p, q;
	int x1, y1, x2, y2;
	int width, height;

	item = GNOME_CANVAS_ITEM (iti);

	width = iti->_priv->layout_width + 2 * MARGIN_X;
	height = iti->_priv->layout_height + 2 * MARGIN_Y;

	x1 = iti->x + (iti->width - width) / 2;
	y1 = iti->y;
	x2 = x1 + width;
	y2 = y1 + height;

	/* Translate to world coordinates */

	gnome_canvas_item_i2w_affine (item, affine);

	p.x = x1;
	p.y = y1;
	art_affine_point (&q, &p, affine);
	item->x1 = q.x;
	item->y1 = q.y;

	p.x = x2;
	p.y = y2;
	art_affine_point (&q, &p, affine);
	item->x2 = q.x;
	item->y2 = q.y;
}

static void
gnome_icon_text_item_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeIconTextItem *iti;
	GnomeIconTextItemPrivate *priv;

	iti = GNOME_ICON_TEXT_ITEM (item);
	priv = iti->_priv;
	
	GNOME_CALL_PARENT (GNOME_CANVAS_ITEM_CLASS, update, (item, affine, clip_path, flags));

	/* If necessary, queue a redraw of the old bounding box */
	if ((flags & GNOME_CANVAS_UPDATE_VISIBILITY)
	    || (flags & GNOME_CANVAS_UPDATE_AFFINE)
	    || priv->need_pos_update
	    || priv->need_font_update
	    || priv->need_text_update)
		gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	/* Compute new bounds */
	if (priv->need_pos_update
	    || priv->need_font_update
	    || priv->need_text_update)
		recompute_bounding_box (iti);

	/* Queue redraw */
	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	priv->need_pos_update = FALSE;
	priv->need_font_update = FALSE;
	priv->need_text_update = FALSE;
	priv->need_state_update = FALSE;
}

/* Draw method handler for the icon text item */
static void
gnome_icon_text_item_draw (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height)
{
	GtkStyle *style;
	GdkGC *gc, *bgc;
	int xofs, yofs;
	int w, h;
	GnomeIconTextItem *iti;
	GnomeIconTextItemPrivate *priv;

	iti = GNOME_ICON_TEXT_ITEM (item);
	priv = iti->_priv;
	
	style = GTK_WIDGET (GNOME_CANVAS_ITEM (iti)->canvas)->style;
	
	gc = style->fg_gc [GTK_STATE_NORMAL];
	bgc = style->bg_gc [GTK_STATE_NORMAL];

	w = priv->layout_width + 2 * MARGIN_X;
	h = priv->layout_height + 2 * MARGIN_Y;
	
	xofs = item->x1 - x;
	yofs = item->y1 - y;

	if (iti->selected && !iti->editing)
		gdk_draw_rectangle (drawable,
				    style->bg_gc[GTK_STATE_SELECTED],
				    TRUE,
				    xofs + 1, yofs + 1,
				    w - 2, h - 2);
	
	if (iti->focused && ! iti->editing)
		gtk_draw_focus (style,
				drawable,
				xofs, yofs,
				w - 1, h - 1);
				
	gdk_draw_layout (drawable,
			 style->fg_gc[(iti->selected
				       ? GTK_STATE_SELECTED
				       : GTK_STATE_NORMAL)],
			 xofs - (iti->width - priv->layout_width - 2 * MARGIN_X) / 2,
			 yofs + 1,
			 priv->layout);
}

/* Bounds method handler for the icon text item */
static void
gnome_icon_text_item_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GnomeIconTextItem *iti;
	GnomeIconTextItemPrivate *priv;
	int width, height;

	iti = GNOME_ICON_TEXT_ITEM (item);
	priv = iti->_priv;
	
	width = priv->layout_width + 2 * MARGIN_X;
	height = priv->layout_height + 2 * MARGIN_Y;

	*x1 = iti->x + (iti->width - width) / 2;
	*y1 = iti->y;
	*x2 = *x1 + width;
	*y2 = *y1 + height;
}

/* Point method handler for the icon text item */
static double
gnome_icon_text_item_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	double dx, dy;
	
	*actual_item = item;

	if (cx < item->x1)
		dx = item->x1 - cx;
	else if (cx > item->x2)
		dx = cx - item->x2;
	else
		dx = 0.0;

	if (cy < item->y1)
		dy = item->y1 - cy;
	else if (cy > item->y2)
		dy = cy - item->y2;
	else
		dy = 0.0;

	return sqrt (dx * dx + dy * dy);
}

static gboolean
gnome_icon_text_item_event (GnomeCanvasItem *item, GdkEvent *event)
{
	return TRUE;
}

static void
gnome_icon_text_item_class_init (GnomeIconTextItemClass *klass)
{
	GnomeCanvasItemClass *canvas_item_class;
	GtkObjectClass *object_class;
	
	object_class = (GtkObjectClass *)klass;
	canvas_item_class = (GnomeCanvasItemClass *)klass;
	
	iti_signals [TEXT_CHANGED] =
		gtk_signal_new (
			"text_changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, text_changed),
			gtk_marshal_BOOL__NONE,
			GTK_TYPE_BOOL, 0);

	iti_signals [HEIGHT_CHANGED] =
		gtk_signal_new (
			"height_changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, height_changed),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	iti_signals [WIDTH_CHANGED] =
		gtk_signal_new (
			"width_changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, width_changed),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	iti_signals[EDITING_STARTED] =
		gtk_signal_new (
			"editing_started",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, editing_started),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	iti_signals[EDITING_STOPPED] =
		gtk_signal_new (
			"editing_stopped",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, editing_stopped),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	iti_signals[SELECTION_STARTED] =
		gtk_signal_new (
			"selection_started",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, selection_started),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	iti_signals[SELECTION_STOPPED] =
		gtk_signal_new (
			"selection_stopped",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, selection_stopped),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	canvas_item_class->update = gnome_icon_text_item_update;
	canvas_item_class->draw = gnome_icon_text_item_draw;
	canvas_item_class->bounds = gnome_icon_text_item_bounds;
	canvas_item_class->point = gnome_icon_text_item_point;
	canvas_item_class->event = gnome_icon_text_item_event;
}

static void
gnome_icon_text_item_instance_init (GnomeIconTextItem *item)
{
	item->_priv = g_new0 (GnomeIconTextItemPrivate, 1);
}

/**
 * gnome_icon_text_item_configure:
 * @iti: An icon text item.
 * @x: X position in which to place the item.
 * @y: Y position in which to place the item.
 * @width: Maximum width allowed for this item, to be used for word wrapping.
 * @fontname: Name of the fontset that should be used to display the text.
 * @text: Text that is going to be displayed.
 * @is_editable: Deprecated.
 * @is_static: Whether @text points to a static string or not.
 *
 * This routine is used to configure a &GnomeIconTextItem.
 *
 * @x and @y specify the cordinates where the item is placed inside the canvas.
 * The @x coordinate should be the leftmost position that the icon text item can
 * assume at any one time, that is, the left margin of the column in which the
 * icon is to be placed.  The @y coordinate specifies the top of the icon text
 * item.
 *
 * @width is the maximum width allowed for this icon text item.  The coordinates
 * define the upper-left corner of an icon text item with maximum width; this may
 * actually be outside the bounding box of the item if the text is narrower than
 * the maximum width.
 *
 * If @is_static is true, it means that there is no need for the item to
 * allocate memory for the string (it is a guarantee that the text is allocated
 * by the caller and it will not be deallocated during the lifetime of this
 * item).  This is an optimization to reduce memory usage for large icon sets.
 */
void
gnome_icon_text_item_configure (GnomeIconTextItem *iti, int x, int y,
				int width, const char *fontname,
				const char *text,
				gboolean is_editable, gboolean is_static)
{
	GnomeIconTextItemPrivate *priv = iti->_priv;

	g_return_if_fail (GNOME_IS_ICON_TEXT_ITEM (iti));
	g_return_if_fail (width > 2 * MARGIN_X);
	g_return_if_fail (text != NULL);
	
	iti->x = x;
	iti->y = y;
	iti->width = width;
	iti->is_editable = is_editable != FALSE;

	if (iti->text && iti->is_text_allocated)
		g_free (iti->text);

	iti->is_text_allocated = !is_static;
	
	/* This cast is to shut up the compiler */
	if (is_static)
		iti->text = (char *) text;
	else
		iti->text = g_strdup (text);
	
	if (iti->fontname)
		g_free (iti->fontname);

	iti->fontname = fontname ? g_strdup (fontname) : NULL;

	/* Create our new PangoLayout */
	if (priv->layout != NULL)
		g_object_unref (priv->layout);
	priv->layout = gtk_widget_create_pango_layout (GTK_WIDGET (GNOME_CANVAS_ITEM (iti)->canvas), iti->text);

	pango_layout_set_alignment (priv->layout, PANGO_ALIGN_CENTER);
	update_pango_layout (iti);

	/* FIXME: Should we update font stuff here? */

	priv->need_pos_update = TRUE;
	priv->need_font_update = TRUE;
	priv->need_text_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}

/**
 * gnome_icon_text_item_setxy:
 * @iti:  An icon text item.
 * @x: X position.
 * @y: Y position.
 *
 * Sets the coordinates at which the icon text item should be placed.
 *
 * See also: gnome_icon_text_item_configure().
 */
void
gnome_icon_text_item_setxy (GnomeIconTextItem *iti, int x, int y)
{
	GnomeIconTextItemPrivate *priv;

	g_return_if_fail (GNOME_IS_ICON_TEXT_ITEM (iti));

	priv = iti->_priv;

	iti->x = x;
	iti->y = y;

	priv->need_pos_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}

void
gnome_icon_text_item_focus (GnomeIconTextItem *iti, int focused)
{
	GnomeIconTextItemPrivate *priv;

	g_return_if_fail (GNOME_IS_ICON_TEXT_ITEM (iti));

	priv = iti->_priv;

	if (!iti->focused == !focused)
		return;
	
	iti->focused = focused ? TRUE : FALSE;

	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}

/**
 * gnome_icon_text_item_select:
 * @iti: An icon text item
 * @sel: Whether the icon text item should be displayed as selected.
 *
 * This function is used to control whether an icon text item is displayed as
 * selected or not.  Mouse events are ignored by the item when it is unselected;
 * when the user clicks on a selected icon text item, it will start the text
 * editing process.
 */
void
gnome_icon_text_item_select (GnomeIconTextItem *iti, int sel)
{
	GnomeIconTextItemPrivate *priv;

	g_return_if_fail (GNOME_IS_ICON_TEXT_ITEM (iti));

	priv = iti->_priv;

	if (!iti->selected == !sel)
		return;

	iti->selected = sel ? TRUE : FALSE;

#ifdef FIXME
	if (!iti->selected && iti->editing)
		iti_edition_accept (iti);
#endif
	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}


/**
 * gnome_icon_text_item_get_text:
 * @iti: An icon text item.
 *
 * Returns the current text string in an icon text item.  The client should not
 * free this string, as it is internal to the icon text item.
 */
char *
gnome_icon_text_item_get_text (GnomeIconTextItem *iti)
{
	GnomeIconTextItemPrivate *priv;

	g_return_val_if_fail (GNOME_IS_ICON_TEXT_ITEM (iti), NULL);

	priv = iti->_priv;

	return iti->text;
}


/**
 * gnome_icon_text_item_start_editing:
 * @iti: An icon text item.
 *
 * Starts the editing state of an icon text item.
 **/
void
gnome_icon_text_item_start_editing (GnomeIconTextItem *iti)
{
	g_return_if_fail (GNOME_IS_ICON_TEXT_ITEM (iti));

	if (iti->editing)
		return;

	iti->selected = TRUE; /* Ensure that we are selected */
	gnome_canvas_item_grab_focus (GNOME_CANVAS_ITEM (iti));
	iti_start_editing (iti);
}

/**
 * gnome_icon_text_item_stop_editing:
 * @iti: An icon text item.
 * @accept: Whether to accept the current text or to discard it.
 *
 * Terminates the editing state of an icon text item.  The @accept argument
 * controls whether the item's current text should be accepted or discarded.  If
 * it is discarded, then the icon's original text will be restored.
 **/
void
gnome_icon_text_item_stop_editing (GnomeIconTextItem *iti,
				   gboolean accept)
{
	g_return_if_fail (GNOME_IS_ICON_TEXT_ITEM (iti));

	if (!iti->editing)
		return;

	if (accept)
		iti_edition_accept (iti);
	else
		iti_stop_editing (iti);
}
