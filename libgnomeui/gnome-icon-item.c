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
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkrgb.h>
#include <gdk/gdkx.h>
#include <libart_lgpl/art_rgb_affine.h>
#include <libart_lgpl/art_rgb_rgba_affine.h>
#include <string.h>

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include "gnome-icon-item.h"

/* Margins used to display the information */
#define MARGIN_X 2
#define MARGIN_Y 2

#define ROUND(n) (floor ((n) + .5))

/* Private part of the GnomeIconTextItem structure */
struct _GnomeIconTextItemPrivate {

	/* Our layout */
	PangoLayout *layout;
	int layout_width, layout_height;
	GtkWidget *entry_top;
	GtkWidget *entry;

	guint selection_start;
	guint min_width;
	guint min_height;

	GdkGC *cursor_gc;

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

	/* Whether selection is occuring */
	guint selecting         : 1;
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

static void
send_focus_event (GnomeIconTextItem *iti, gboolean in)
{
	GnomeIconTextItemPrivate *priv;
	GtkWidget *widget;
	gboolean has_focus;
	GdkEvent fake_event;
	
	g_return_if_fail (in == FALSE || in == TRUE);

	priv = iti->_priv;
	if (priv->entry == NULL) {
		g_assert (!in);
		return;
	}

	widget = GTK_WIDGET (priv->entry);
	has_focus = GTK_WIDGET_HAS_FOCUS (widget);
	if (has_focus == in) {
		return;
	}
	
	memset (&fake_event, 0, sizeof (fake_event));
	fake_event.focus_change.type = GDK_FOCUS_CHANGE;
	fake_event.focus_change.window = widget->window;
	fake_event.focus_change.in = in;
	gtk_widget_event (widget, &fake_event);
	g_return_if_fail (GTK_WIDGET_HAS_FOCUS (widget) == in);
}

/* Updates the pango layout width */
static void
update_pango_layout (GnomeIconTextItem *iti)
{
	GnomeIconTextItemPrivate *priv;
	PangoRectangle bounds;
	const char *text;
	
	priv = iti->_priv;	

	if (iti->editing) {
		text = gtk_entry_get_text (GTK_ENTRY (priv->entry));
	} else {
		text = iti->text;
	}
		
	pango_layout_set_text (priv->layout, text, strlen (text));

	pango_layout_set_width (priv->layout, iti->width * PANGO_SCALE);

	pango_layout_get_pixel_extents (iti->_priv->layout, NULL, &bounds);

	priv->layout_width = bounds.width;
	priv->layout_height = bounds.height;
}

/* Stops the editing state of an icon text item */
static void
iti_stop_editing (GnomeIconTextItem *iti)
{
	GnomeIconTextItemPrivate *priv;
	
	priv = iti->_priv;
	iti->editing = FALSE;
	send_focus_event (iti, FALSE);
	update_pango_layout (iti);
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
	g_signal_emit (iti, iti_signals[EDITING_STOPPED], 0);
}


/* Accepts the text in the off-screen entry of an icon text item */
static void
iti_edition_accept (GnomeIconTextItem *iti)
{
	GnomeIconTextItemPrivate *priv;
	gboolean accept;

	priv = iti->_priv;
	accept = TRUE;

	g_signal_emit (iti, iti_signals [TEXT_CHANGED], 0, &accept);

	if (iti->editing){
		if (accept) {
			if (iti->is_text_allocated)
				g_free (iti->text);

			iti->text = g_strdup (gtk_entry_get_text (GTK_ENTRY(priv->entry)));
			iti->is_text_allocated = 1;
		}

		iti_stop_editing (iti);
	}
	update_pango_layout (iti);
	priv->need_text_update = TRUE;

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}

/* Ensure the item gets focused (both globally, and local to Gtk) */
static void
iti_ensure_focus (GnomeCanvasItem *item)
{
	GtkWidget *toplevel;

        /* gnome_canvas_item_grab_focus still generates focus out/in
         * events when focused_item == item
         */
        if (GNOME_CANVAS_ITEM (item)->canvas->focused_item != item) {
        	gnome_canvas_item_grab_focus (GNOME_CANVAS_ITEM (item));
        }

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (item->canvas));
	if (toplevel != NULL && GTK_WIDGET_REALIZED (toplevel)) {
		gdk_window_focus (toplevel->window, GDK_CURRENT_TIME);
	}
}

/* Starts the selection state in the icon text item */
static void
iti_start_selecting (GnomeIconTextItem *iti, int idx, guint32 event_time)
{
	GnomeIconTextItemPrivate *priv;
	GtkEditable *e;
	GdkCursor *ibeam;

	priv = iti->_priv;
	e = GTK_EDITABLE (priv->entry);

	gtk_editable_select_region (e, idx, idx);
	gtk_editable_set_position (e, idx);
	ibeam = gdk_cursor_new (GDK_XTERM);
	gnome_canvas_item_grab (GNOME_CANVAS_ITEM (iti),
				GDK_BUTTON_RELEASE_MASK |
				GDK_POINTER_MOTION_MASK,
				ibeam, event_time);
	gdk_cursor_unref (ibeam);

	gtk_editable_select_region (e, idx, idx);
	priv->selecting = TRUE;
	priv->selection_start = idx;

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));

	g_signal_emit (iti, iti_signals[SELECTION_STARTED], 0);
}

/* Stops the selection state in the icon text item */
static void
iti_stop_selecting (GnomeIconTextItem *iti, guint32 event_time)
{
	GnomeIconTextItemPrivate *priv;
	GnomeCanvasItem *item;
	GtkEditable *e;

	priv = iti->_priv;
	item = GNOME_CANVAS_ITEM (iti);
	e = GTK_EDITABLE (priv->entry);

	gnome_canvas_item_ungrab (item, event_time);
	priv->selecting = FALSE;

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
	g_signal_emit (iti, iti_signals[SELECTION_STOPPED], 0);
}

/* Handles selection range changes on the icon text item */
static void
iti_selection_motion (GnomeIconTextItem *iti, int idx)
{
	GnomeIconTextItemPrivate *priv;
	GtkEditable *e;
	g_assert (idx >= 0);
	
	priv = iti->_priv;
	e = GTK_EDITABLE (priv->entry);

	if (idx < (int) priv->selection_start) {
		gtk_editable_select_region (e, idx, priv->selection_start);
	} else {
		gtk_editable_select_region (e, priv->selection_start, idx);
	}

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}

static void
iti_entry_text_changed_by_clipboard (GtkObject *widget, gpointer data)
{
	GnomeIconTextItem *iti;
	GnomeCanvasItem *item;

        /* Update text item to reflect changes */
        iti = GNOME_ICON_TEXT_ITEM (data);
	
	update_pango_layout (iti);
	iti->_priv->need_text_update = TRUE;
	
        item = GNOME_CANVAS_ITEM (iti);
        gnome_canvas_item_request_update (item);
}


/* Position insertion point based on arrow key event */
static void
iti_handle_arrow_key_event (GnomeIconTextItem *iti, GdkEvent *event)
{
	/* FIXME: Implement this */
}

static int
get_layout_index (GnomeIconTextItem *iti, int x, int y)
{
	int index;
	int trailing;
	const char *cluster;
	const char *cluster_end;

	GnomeIconTextItemPrivate *priv;
	PangoRectangle extents;

	trailing = 0;
	index = 0;
	priv = iti->_priv;

	pango_layout_get_extents (priv->layout, NULL, &extents);

	x = (x * PANGO_SCALE) + extents.x;
	y = (y * PANGO_SCALE) + extents.y;
	
	pango_layout_xy_to_index (priv->layout, x, y, &index, &trailing);

	cluster = gtk_entry_get_text (GTK_ENTRY (priv->entry)) + index;
	cluster_end = cluster;
	while (trailing) {
		cluster_end = g_utf8_next_char (cluster_end);
		--trailing;
	}
	index += (cluster_end - cluster);

	return index;
}

static void
iti_entry_activate (GtkObject *widget, gpointer data)
{
	iti_edition_accept (GNOME_ICON_TEXT_ITEM (data));
}

static void
realize_cursor_gc (GnomeIconTextItem *iti)
{
	GdkColor *cursor_color;
	GdkColor red = { 0, 0xffff, 0x0000, 0x0000 };
	
	if (iti->_priv->cursor_gc) {
		g_object_unref (iti->_priv->cursor_gc);
	}
	
	iti->_priv->cursor_gc = gdk_gc_new (GTK_WIDGET (GNOME_CANVAS_ITEM (iti)->canvas)->window);
	
	gtk_widget_style_get (GTK_WIDGET (iti->_priv->entry), "cursor_color",
			      &cursor_color, NULL);
	if (cursor_color) {
		gdk_gc_set_rgb_fg_color (iti->_priv->cursor_gc, cursor_color);
	} else {
		gdk_gc_set_rgb_fg_color (iti->_priv->cursor_gc, &red);
	}
}


static void
gnome_icon_text_item_realize (GnomeCanvasItem *item)
{
	GnomeIconTextItem *iti = GNOME_ICON_TEXT_ITEM (item);

	if (iti->_priv->entry) {
		realize_cursor_gc (iti);
	}
	
	GNOME_CALL_PARENT (GNOME_CANVAS_ITEM_CLASS, realize, (item));
}

static void
gnome_icon_text_item_unrealize (GnomeCanvasItem *item)
{
	GnomeIconTextItem *iti = GNOME_ICON_TEXT_ITEM (item);
	
	if (iti->_priv->cursor_gc) {
		g_object_unref (iti->_priv->cursor_gc);
		iti->_priv->cursor_gc = NULL;
	}

	GNOME_CALL_PARENT (GNOME_CANVAS_ITEM_CLASS, unrealize, (item));
}

/* Starts the editing state of an icon text item */
static void
iti_start_editing (GnomeIconTextItem *iti)
{
	GnomeIconTextItemPrivate *priv;

	priv = iti->_priv;

	if (iti->editing)
		return;

	/* We place an entry offscreen to handle events and selections.  */
	if (priv->entry_top == NULL) {
		priv->entry = gtk_entry_new ();
		g_signal_connect (priv->entry, "activate",
				  G_CALLBACK (iti_entry_activate), iti);


		if (GTK_WIDGET_REALIZED (GTK_WIDGET (GNOME_CANVAS_ITEM (iti)->canvas))) {
			realize_cursor_gc (iti);
		}
		
		/* Make clipboard functions cause an update, since clipboard
		 * functions will change the offscreen entry */
		gtk_signal_connect_after (GTK_OBJECT (priv->entry), "changed",
					  G_CALLBACK (iti_entry_text_changed_by_clipboard), iti);
		priv->entry_top = gtk_window_new (GTK_WINDOW_POPUP);
		gtk_container_add (GTK_CONTAINER (priv->entry_top), 
				   GTK_WIDGET (priv->entry));
		gtk_widget_set_uposition (priv->entry_top, 20000, 20000);
		gtk_widget_show_all (priv->entry_top);
	}

	gtk_entry_set_text (GTK_ENTRY (priv->entry), iti->text);
	gtk_editable_select_region (GTK_EDITABLE (priv->entry), 0, -1);

	iti->editing = TRUE;
	priv->need_state_update = TRUE;

	send_focus_event (iti, TRUE);
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));

	gtk_signal_emit (GTK_OBJECT (iti), iti_signals[EDITING_STARTED]);
}

/* Recomputes the bounding box of an icon text item */
static void
recompute_bounding_box (GnomeIconTextItem *iti)
{

	GnomeCanvasItem *item;
	double x1, y1, x2, y2;
	int width_c, height_c;
	int width_w, height_w;
	int max_width_w;

	item = GNOME_CANVAS_ITEM (iti);
	
	width_c = iti->_priv->layout_width + 2 * MARGIN_X;
	height_c = iti->_priv->layout_height + 2 * MARGIN_Y;
	width_w = ROUND (width_c / item->canvas->pixels_per_unit);
	height_w = ROUND (height_c / item->canvas->pixels_per_unit);
	max_width_w = ROUND (iti->width / item->canvas->pixels_per_unit);

	x1 = iti->x;
	y1 = iti->y;

	gnome_canvas_item_i2w (item, &x1, &y1);

	x1 += ROUND ((max_width_w - width_w) / 2);
	x2 = x1 + width_w;
	y2 = y1 + height_w;

	gnome_canvas_w2c_d (item->canvas, x1, y1, &item->x1, &item->y1);
	gnome_canvas_w2c_d (item->canvas, x2, y2, &item->x2, &item->y2);
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

static void
iti_draw_cursor (GnomeIconTextItem *iti, GdkDrawable *drawable, 
		 int x, int y)
{
	int stem_width;
	int i;
	PangoRectangle pos;

	g_return_if_fail (iti->_priv->cursor_gc != NULL);

	pango_layout_get_cursor_pos (iti->_priv->layout, 
				     gtk_editable_get_position (GTK_EDITABLE (iti->_priv->entry)), 
				     &pos, NULL);
	stem_width = PANGO_PIXELS (pos.height) / 30 + 1;
	for (i = 0; i < stem_width; i++) {
		gdk_draw_line (drawable, iti->_priv->cursor_gc,
			       x + PANGO_PIXELS (pos.x) + i - stem_width / 2, 
			       y + PANGO_PIXELS (pos.y),
			       x + PANGO_PIXELS (pos.x) + i - stem_width / 2, 
			       y + PANGO_PIXELS (pos.y) + PANGO_PIXELS (pos.height));	
	}
}

/* Draw method handler for the icon text item */
static void
gnome_icon_text_item_draw (GnomeCanvasItem *item, GdkDrawable *drawable, 
			   int x, int y, int width, int height)
{
	GtkWidget *widget;
	GtkStyle *style;
	GdkGC *gc, *bgc;
	int xofs, yofs;
	int text_xofs, text_yofs;
	int w, h;
	GnomeIconTextItem *iti;
	GnomeIconTextItemPrivate *priv;

	widget = GTK_WIDGET (item->canvas);
	iti = GNOME_ICON_TEXT_ITEM (item);
	priv = iti->_priv;
	
	style = GTK_WIDGET (GNOME_CANVAS_ITEM (iti)->canvas)->style;
	
	gc = style->fg_gc [GTK_STATE_NORMAL];
	bgc = style->bg_gc [GTK_STATE_NORMAL];

	w = priv->layout_width + 2 * MARGIN_X;
	h = priv->layout_height + 2 * MARGIN_Y;
	
	xofs = item->x1 - x;
	yofs = item->y1 - y;

	text_xofs = xofs - (iti->width - priv->layout_width - 2 * MARGIN_X) / 2;
	text_yofs = yofs + MARGIN_Y;

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
	
	if (iti->editing) {
		/* FIXME: are these the right graphics contexts? */
		gdk_draw_rectangle (drawable,
				    style->base_gc[GTK_STATE_NORMAL],
				    TRUE, 
				    xofs, yofs, w - 1, h - 1);
		gdk_draw_rectangle (drawable,
				    style->fg_gc[GTK_STATE_NORMAL],
				    FALSE,
				    xofs, yofs, w - 1, h - 1);
	}	

	gdk_draw_layout (drawable,
			 style->text_gc[(iti->selected
				       ? GTK_STATE_SELECTED
				       : GTK_STATE_NORMAL)],
			 text_xofs, 
			 text_yofs,
			 priv->layout);

	if (iti->editing) {
		int range[2];
		if (gtk_editable_get_selection_bounds (GTK_EDITABLE (priv->entry), &range[0], &range[1])) {
			GdkRegion *clip_region;
			GdkColor *selection_color, *text_color;
			guint8 state;
		
			state = GTK_WIDGET_HAS_FOCUS (widget) ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE;
			selection_color = &widget->style->base[state];
			text_color = &widget->style->text[state];
			clip_region = gdk_pango_layout_get_clip_region
				(priv->layout,
				 text_xofs,
				 text_yofs,
				 range, 1);
			gdk_gc_set_clip_region (widget->style->black_gc, 
						clip_region);
			gdk_draw_layout_with_colors (drawable,
						     widget->style->black_gc,
						     text_xofs, 
						     text_yofs,
						     priv->layout,
						     text_color, 
						     selection_color);
			gdk_gc_set_clip_region (widget->style->black_gc, NULL);
			gdk_region_destroy (clip_region);
		} else {
			iti_draw_cursor (iti, drawable, text_xofs, text_yofs);
		}
	}
}

static void
draw_pixbuf_aa (GdkPixbuf *pixbuf, GnomeCanvasBuf *buf, double affine[6], int x_offset, int y_offset)
{
	void (* affine_function)
		(art_u8 *dst, int x0, int y0, int x1, int y1, int dst_rowstride,
		 const art_u8 *src, int src_width, int src_height, int src_rowstride,
		 const double affine[6],
		 ArtFilterLevel level,
		 ArtAlphaGamma *alpha_gamma);

	affine[4] += x_offset;
	affine[5] += y_offset;

	affine_function = gdk_pixbuf_get_has_alpha (pixbuf)
		? art_rgb_rgba_affine
		: art_rgb_affine;
	
	(* affine_function)
		(buf->buf,
		 buf->rect.x0, buf->rect.y0,
		 buf->rect.x1, buf->rect.y1,
		 buf->buf_rowstride,
		 gdk_pixbuf_get_pixels (pixbuf),
		 gdk_pixbuf_get_width (pixbuf),
		 gdk_pixbuf_get_height (pixbuf),
		 gdk_pixbuf_get_rowstride (pixbuf),
		 affine,
		 ART_FILTER_NEAREST,
		 NULL);

	affine[4] -= x_offset;
	affine[5] -= y_offset;
}

static void
gnome_icon_text_item_render (GnomeCanvasItem *item, GnomeCanvasBuf *buffer)
{
	GdkVisual *visual;
	GdkPixmap *pixmap;
	GdkPixbuf *text_pixbuf;
	double affine[6];
	int width, height;

	visual = gdk_rgb_get_visual ();
	art_affine_identity(affine);
	width  = ROUND (item->x2 - item->x1);
	height = ROUND (item->y2 - item->y1);	

	pixmap = gdk_pixmap_new (NULL, width, height, visual->depth);

	gdk_draw_rectangle (pixmap, GTK_WIDGET (item->canvas)->style->white_gc,
			    TRUE,
			    0, 0,
			    width,
			    height);
	
	/* use a common routine to draw the label into the pixmap */
	gnome_icon_text_item_draw (item, pixmap, 
				   ROUND (item->x1), ROUND (item->y1), 
				   width, height);
	
	/* turn it into a pixbuf */
	text_pixbuf = gdk_pixbuf_get_from_drawable
		(NULL, pixmap, gdk_rgb_get_colormap (),
		 0, 0,
		 0, 0, 
		 width, 
		 height);
	
	/* draw the pixbuf containing the label */
	draw_pixbuf_aa (text_pixbuf, buffer, affine, ROUND (item->x1), ROUND (item->y1));
	g_object_unref (text_pixbuf);

	buffer->is_bg = FALSE;
	buffer->is_buf = TRUE;
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
	GnomeIconTextItem *iti;
	GnomeIconTextItemPrivate *priv;
	int idx;
	double cx, cy;
	
	iti = GNOME_ICON_TEXT_ITEM (item);
	priv = iti->_priv;

	switch (event->type) {
	case GDK_KEY_PRESS:
		if (!iti->editing) {
			break;
		}
		
		switch(event->key.keyval) {
		
		/* Pass these events back to parent */		
		case GDK_Escape:
		case GDK_Return:
		case GDK_KP_Enter:
			return FALSE;

		/* Handle up and down arrow keys.  GdkEntry does not know 
		 * how to handle multi line items */
		case GDK_Up:		
		case GDK_Down:
			iti_handle_arrow_key_event(iti, event);
			break;
			
		default:			
			/* Check for control key operations */
			if (event->key.state & GDK_CONTROL_MASK) {
				return FALSE;
			}

			/* Handle any events that reach us */
			gtk_widget_event (GTK_WIDGET (priv->entry), event);
			break;
		}
		
		/* Update text item to reflect changes */
		update_pango_layout (iti);
		priv->need_text_update = TRUE;
		gnome_canvas_item_request_update (item);
		return TRUE;

	case GDK_BUTTON_PRESS:
		if (!iti->editing) {
			break;
		}

		if (event->button.button == 1) {
			gnome_canvas_w2c_d (item->canvas, event->button.x, event->button.y, &cx, &cy);
			idx = get_layout_index (iti, 
						(cx - item->x1) + MARGIN_X,
						(cy - item->y1) + MARGIN_Y);
			iti_start_selecting (iti, idx, event->button.time);
		}
		return TRUE;
	case GDK_MOTION_NOTIFY:
		if (!priv->selecting)
			break;

		gtk_widget_event (GTK_WIDGET (priv->entry), event);
		gnome_canvas_w2c_d (item->canvas, event->button.x, event->button.y, &cx, &cy);			
		idx = get_layout_index (iti, 
					floor ((cx - (item->x1 + MARGIN_X)) + .5),
					floor ((cy - (item->y1 + MARGIN_Y)) + .5));
		
		iti_selection_motion (iti, idx);
		return TRUE;

	case GDK_BUTTON_RELEASE:
		if (priv->selecting && event->button.button == 1)
			iti_stop_selecting (iti, event->button.time);
		else
			break;
		return TRUE;
	default:
		break;
	}

	return FALSE;
}

static void 
gnome_icon_text_item_destroy (GtkObject *object)
{
	GnomeIconTextItem *iti = GNOME_ICON_TEXT_ITEM (object);
	GnomeCanvasItem *item = GNOME_CANVAS_ITEM (object);
	GnomeIconTextItemPrivate *priv = iti->_priv;
	
	gnome_canvas_request_redraw (item->canvas, 
				     ROUND (item->x1),
				     ROUND (item->y1),
				     ROUND (item->x2),
				     ROUND (item->y2));
	
	if (iti->text && iti->is_text_allocated) {
		g_free (iti->text);
		iti->text = NULL;
	}

	if (priv->layout) {
		g_object_unref (priv->layout);	
		priv->layout = NULL;
	}
	
	if (priv->entry_top) {
		gtk_widget_destroy (priv->entry_top);
		priv->entry_top = NULL;
	}
	
	if (priv->cursor_gc) {
		g_object_unref (priv->cursor_gc);
		priv->cursor_gc = NULL;
	}
	
	GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
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

	object_class->destroy = gnome_icon_text_item_destroy;

	canvas_item_class->update = gnome_icon_text_item_update;
	canvas_item_class->draw = gnome_icon_text_item_draw;
	canvas_item_class->render = gnome_icon_text_item_render;
	canvas_item_class->bounds = gnome_icon_text_item_bounds;
	canvas_item_class->point = gnome_icon_text_item_point;
	canvas_item_class->event = gnome_icon_text_item_event;
	canvas_item_class->realize = gnome_icon_text_item_realize;
	canvas_item_class->unrealize = gnome_icon_text_item_unrealize;
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

	if (iti->editing)
		iti_stop_editing (iti);

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
const char *
gnome_icon_text_item_get_text (GnomeIconTextItem *iti)
{
	GnomeIconTextItemPrivate *priv;

	g_return_val_if_fail (GNOME_IS_ICON_TEXT_ITEM (iti), NULL);

	priv = iti->_priv;

	if (iti->editing) {
		return gtk_entry_get_text (GTK_ENTRY(priv->entry));
	} else {
		return iti->text;
	}
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
	iti_ensure_focus (GNOME_CANVAS_ITEM (iti));
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

GtkEditable *
gnome_icon_text_item_get_editable  (GnomeIconTextItem *iti)
{
	GnomeIconTextItemPrivate *priv;

	priv = iti->_priv;
	return priv->entry ? GTK_EDITABLE (priv->entry) : NULL;
}
