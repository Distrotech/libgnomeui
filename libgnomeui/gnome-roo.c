/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-roo.c

   Copyright (C) 2000 Jaka Mocnik

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

   Author: Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
*/

#include <config.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>


#include <libgnome/gnome-i18n.h>
#include "gnome-roo.h"
#include "gnome-pouch.h"
#include "gnome-pouchP.h"

#define ROO_CLOSE_HILIT    (1L << 0)
#define ROO_ICONIFY_HILIT  (1L << 1)
#define ROO_MAXIMIZE_HILIT (1L << 2)
#define ROO_ICONIFIED      (1L << 3)
#define ROO_MAXIMIZED      (1L << 4)
#define ROO_SELECTED       (1L << 5)
#define ROO_IN_MOVE        (1L << 6)
#define ROO_IN_RESIZE      (1L << 7)
#define ROO_ICON_PARKED    (1L << 8)
#define ROO_WAS_ICONIFIED  (1L << 9)
#define ROO_UNPOSITIONED   (1L << 10)

enum
{
	CLOSE,
	ICONIFY,
	UNICONIFY,
    MAXIMIZE,
    UNMAXIMIZE,
	SELECT,
	DESELECT,
	LAST_SIGNAL
};

typedef enum
{
	RESIZE_TOP_LEFT,
	RESIZE_TOP,
    RESIZE_TOP_RIGHT,
    RESIZE_RIGHT,
	RESIZE_BOTTOM_RIGHT,
    RESIZE_BOTTOM,
    RESIZE_BOTTOM_LEFT,
    RESIZE_LEFT,
	RESIZE_NONE,
} GnomeRooResizeType;

static void gnome_roo_class_init(GnomeRooClass *klass);
static void gnome_roo_init(GnomeRoo *roo);
static void gnome_roo_finalize(GObject *object);
static void gnome_roo_size_request(GtkWidget *w, GtkRequisition *req);
static void gnome_roo_realize(GtkWidget *w);
static void gnome_roo_unrealize(GtkWidget *w);
static void gnome_roo_map(GtkWidget *w);
static void gnome_roo_unmap(GtkWidget *w);
static void gnome_roo_size_allocate(GtkWidget *w, GtkAllocation *allocation);
static void gnome_roo_paint(GtkWidget *w, GdkRectangle *area);
static gboolean gnome_roo_expose(GtkWidget *w, GdkEventExpose *e);
static gboolean gnome_roo_button_press(GtkWidget *w, GdkEventButton *e);
static gboolean gnome_roo_button_release(GtkWidget *w, GdkEventButton *e);
static gboolean gnome_roo_motion_notify(GtkWidget *w, GdkEventMotion *e);
static void gnome_roo_maximize(GnomeRoo *roo);
static void gnome_roo_unmaximize(GnomeRoo *roo);
static void gnome_roo_iconify(GnomeRoo *roo);
static void gnome_roo_uniconify(GnomeRoo *roo);
static void gnome_roo_select(GnomeRoo *roo);
static void gnome_roo_deselect(GnomeRoo *roo);
static void gnome_roo_parent_set(GtkWidget *widget, GtkWidget *old_parent);

static GnomeRooResizeType get_resize_type(GnomeRoo *roo, gint x, gint y);
static void calculate_size(GnomeRoo *roo, guint *rw, guint *rh);
static void calculate_title_size(GnomeRoo *roo);
static gboolean in_title_bar(GnomeRoo *roo, guint x, guint y);
static gboolean in_close_button(GnomeRoo *roo, guint x, guint y);
static gboolean in_maximize_button(GnomeRoo *roo, guint x, guint y);
static gboolean in_iconify_button(GnomeRoo *roo, guint x, guint y);
static void draw_shadow(GtkWidget *widget, GtkStateType state, gboolean out, gint x, gint y, gint w, gint h);
static void draw_cross(GtkWidget *widget, GdkGC *gc, gint x, gint y, gint s);
static void paint_close_button(GnomeRoo *roo, gboolean hilight);
static void paint_iconify_button(GnomeRoo *roo, gboolean hilight);
static void paint_maximize_button(GnomeRoo *roo, gboolean hilight);

static GdkCursor *resize_cursor[8] = { NULL, NULL, NULL, NULL,
									   NULL, NULL, NULL, NULL };
static GdkCursor *move_cursor = NULL;
static GdkCursorType resize_cursor_type[8] = {
	GDK_TOP_LEFT_CORNER,
	GDK_TOP_SIDE,
	GDK_TOP_RIGHT_CORNER,
	GDK_RIGHT_SIDE,
	GDK_BOTTOM_RIGHT_CORNER,
	GDK_BOTTOM_SIDE,
	GDK_BOTTOM_LEFT_CORNER,
	GDK_LEFT_SIDE,
};

static gint roo_signals[LAST_SIGNAL];

static GtkObjectClass *parent_class;

guint
gnome_roo_get_type()
{
	static guint roo_type = 0;
	
	if (!roo_type) {
		GtkTypeInfo roo_info = {
			"GnomeRoo",
			sizeof(GnomeRoo),
			sizeof(GnomeRooClass),
			(GtkClassInitFunc) gnome_roo_class_init,
			(GtkObjectInitFunc) gnome_roo_init,
			NULL,
			NULL,
			NULL
		};
		roo_type = gtk_type_unique(gtk_bin_get_type(), &roo_info);
	}
	return roo_type;
}

static void gnome_roo_class_init(GnomeRooClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	parent_class = GTK_OBJECT_CLASS(gtk_type_class(gtk_bin_get_type()));
	widget_class = GTK_WIDGET_CLASS(klass);
	object_class = GTK_OBJECT_CLASS(klass);
	gobject_class = G_OBJECT_CLASS(klass);

	roo_signals[CLOSE] =
		gtk_signal_new ("close",
				GTK_RUN_LAST,
				GTK_CLASS_TYPE (object_class),
				GTK_SIGNAL_OFFSET (GnomeRooClass, close),
				gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	roo_signals[ICONIFY] =
		gtk_signal_new ("iconify",
				GTK_RUN_LAST,
				GTK_CLASS_TYPE (object_class),
				GTK_SIGNAL_OFFSET (GnomeRooClass, iconify),
				gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	roo_signals[UNICONIFY] =
		gtk_signal_new ("uniconify",
				GTK_RUN_LAST,
				GTK_CLASS_TYPE (object_class),
				GTK_SIGNAL_OFFSET (GnomeRooClass, uniconify),
				gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	roo_signals[MAXIMIZE] =
		gtk_signal_new ("maximize",
				GTK_RUN_LAST,
				GTK_CLASS_TYPE (object_class),
				GTK_SIGNAL_OFFSET (GnomeRooClass, maximize),
				gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	roo_signals[UNMAXIMIZE] =
		gtk_signal_new ("unmaximize",
				GTK_RUN_LAST,
				GTK_CLASS_TYPE (object_class),
				GTK_SIGNAL_OFFSET (GnomeRooClass, unmaximize),
				gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	roo_signals[SELECT] =
		gtk_signal_new ("select",
				GTK_RUN_LAST,
				GTK_CLASS_TYPE (object_class),
				GTK_SIGNAL_OFFSET (GnomeRooClass, select),
				gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	roo_signals[DESELECT] =
		gtk_signal_new ("deselect",
				GTK_RUN_LAST,
				GTK_CLASS_TYPE (object_class),
				GTK_SIGNAL_OFFSET (GnomeRooClass, deselect),
				gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);


	gobject_class->finalize = gnome_roo_finalize;

	widget_class->realize = gnome_roo_realize;
	widget_class->unrealize = gnome_roo_unrealize;
	widget_class->map = gnome_roo_map;
	widget_class->unmap = gnome_roo_unmap;
	widget_class->size_request = gnome_roo_size_request;
	widget_class->size_allocate = gnome_roo_size_allocate;
	widget_class->expose_event = gnome_roo_expose;
	widget_class->button_press_event = gnome_roo_button_press;
	widget_class->button_release_event = gnome_roo_button_release;
	widget_class->motion_notify_event = gnome_roo_motion_notify;
	widget_class->parent_set = gnome_roo_parent_set;

	klass->close = NULL;
	klass->iconify = gnome_roo_iconify;
	klass->uniconify = gnome_roo_uniconify;
	klass->maximize = gnome_roo_maximize;
	klass->unmaximize = gnome_roo_unmaximize;
	klass->select = gnome_roo_select;
	klass->deselect = gnome_roo_deselect;
}

static void
gnome_roo_init(GnomeRoo *roo)
{ 
	GTK_WIDGET_UNSET_FLAGS(roo, GTK_NO_WINDOW);

	roo->priv = g_new0(GnomeRooPrivate, 1);

	roo->priv->flags = 0;
	roo->priv->title = NULL;
	roo->priv->vis_title = NULL;
	roo->priv->cover = NULL;
	roo->priv->icon_allocation.width = roo->priv->icon_allocation.height = -1;
	roo->priv->user_allocation.width = roo->priv->user_allocation.height = -1;

	GTK_CONTAINER(roo)->border_width = 2;
}

static void gnome_roo_finalize(GObject *object)
{
	GnomeRoo *roo = GNOME_ROO(object);

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeRoo: finalization");
#endif

	if(roo->priv->title)
		g_free(roo->priv->title);
	roo->priv->title = NULL;

	if(G_OBJECT_CLASS(parent_class)->finalize)
		G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
gnome_roo_parent_set(GtkWidget *widget, GtkWidget *old_parent)
{
	if(widget->parent && !GNOME_IS_POUCH(widget->parent))
		g_warning("GnomeRoo: parent is not a GnomePouch");

	if(GTK_WIDGET_CLASS(parent_class)->parent_set)
		(*GTK_WIDGET_CLASS(parent_class)->parent_set)(widget, old_parent);
}

static
gboolean in_title_bar(GnomeRoo *roo, guint x, guint y)
{
	if(y < roo->priv->title_bar_height - 1 + GTK_WIDGET(roo)->style->ythickness)
		return TRUE;

	return FALSE;
}

static
gboolean in_close_button(GnomeRoo *roo, guint x, guint y)
{
	if(x < 2 + roo->priv->title_bar_height && x >= 2 && in_title_bar(roo, x, y))
		return TRUE;

	return FALSE;
}

static
gboolean in_maximize_button(GnomeRoo *roo, guint x, guint y)
{
	GtkWidget *w = GTK_WIDGET(roo);

	if(x < w->allocation.width - 2 &&
	   x >= w->allocation.width - roo->priv->title_bar_height - 2 &&
	   in_title_bar(roo, x, y))
		return TRUE;

	return FALSE;
}

static
gboolean in_iconify_button(GnomeRoo *roo, guint x, guint y)
{
	GtkWidget *w = GTK_WIDGET(roo);

	if(x < w->allocation.width - 2 - roo->priv->title_bar_height - 1 &&
	   x >= w->allocation.width - 2 - 2*roo->priv->title_bar_height &&
	   in_title_bar(roo, x, y))
		return TRUE;

	return FALSE;
}

static void
draw_shadow(GtkWidget *widget, GtkStateType state, gboolean out, gint x, gint y, gint w, gint h)
{
	GdkGC *tl_gc, *br_gc;

	if(out) {
		tl_gc = widget->style->light_gc[state];
		br_gc = widget->style->dark_gc[state];
	}
	else {
		tl_gc = widget->style->dark_gc[state];
		br_gc = widget->style->light_gc[state];
	}

	gdk_draw_line(widget->window, tl_gc,
				  x, y, x + w - 2, y);
	gdk_draw_line(widget->window, tl_gc,
				  x, y, x, y + h - 1);
	gdk_draw_line(widget->window, br_gc,
				  x + w - 1, y, x + w - 1, y + h - 1);
	gdk_draw_line(widget->window, br_gc,
				  x + 1, y + h - 1, x + w - 1, y + h - 1);
}

static void
draw_cross(GtkWidget *widget, GdkGC *gc, gint x, gint y, gint s)
{
	gdk_draw_line(widget->window, gc,
				  x + s/4, y + s/4, x + s - s/4, y + s - s/4);
	gdk_draw_line(widget->window, gc,
				  x + s/4, y + s - s/4, x + s - s/4, y + s/4);
}

static void
paint_close_button(GnomeRoo *roo, gboolean hilight)
{
	GtkWidget *w = GTK_WIDGET(roo);
	GtkStateType state;

	if(roo->priv->flags & ROO_SELECTED)
		state = GTK_STATE_SELECTED;
	else
		state = GTK_STATE_NORMAL;

	if(hilight) {
		roo->priv->flags |= ROO_CLOSE_HILIT;
	}
	else {
		roo->priv->flags &= ~ROO_CLOSE_HILIT;
	}

	/* draw buttons */
	draw_shadow(w, state, !hilight,
				2, 2,
				roo->priv->title_bar_height, roo->priv->title_bar_height);
	draw_cross(w, (hilight?w->style->white_gc:w->style->black_gc),
			   2, 2, roo->priv->title_bar_height);
}

static void
paint_iconify_button(GnomeRoo *roo, gboolean hilight)
{
	GtkWidget *w = GTK_WIDGET(roo);
	GtkStateType state;

	if(roo->priv->flags & ROO_SELECTED)
		state = GTK_STATE_SELECTED;
	else
		state = GTK_STATE_NORMAL;

	if(hilight) {
		roo->priv->flags |= ROO_ICONIFY_HILIT;
	}
	else {
		roo->priv->flags &= ~ROO_ICONIFY_HILIT;
	}

	draw_shadow(w, state, !hilight,
				w->allocation.width - 2 - 2*roo->priv->title_bar_height - 1, 2,
				roo->priv->title_bar_height, roo->priv->title_bar_height);
	gdk_draw_rectangle(w->window, (hilight?w->style->white_gc:w->style->black_gc), TRUE,
					   w->allocation.width - 2*roo->priv->title_bar_height - 1, roo->priv->title_bar_height - 2,
					   roo->priv->title_bar_height - 4, 2);
}

static void
paint_maximize_button(GnomeRoo *roo, gboolean hilight)
{
	GtkWidget *w = GTK_WIDGET(roo);
	GtkStateType state;

	if(roo->priv->flags & ROO_SELECTED)
		state = GTK_STATE_SELECTED;
	else
		state = GTK_STATE_NORMAL;

	if(hilight) {
		roo->priv->flags |= ROO_MAXIMIZE_HILIT;
	}
	else {
		roo->priv->flags &= ~ROO_MAXIMIZE_HILIT;
	}

	draw_shadow(w, state, !hilight,
				w->allocation.width - 2 - roo->priv->title_bar_height, 2,
				roo->priv->title_bar_height, roo->priv->title_bar_height);
	gdk_draw_rectangle(w->window, (hilight?w->style->white_gc:w->style->black_gc), FALSE,
					   w->allocation.width - roo->priv->title_bar_height, 4,
					   roo->priv->title_bar_height - 5, roo->priv->title_bar_height - 5);
}

static void
calculate_size(GnomeRoo *roo, guint *rw, guint *rh)
{
	GtkWidget *w = GTK_WIDGET(roo);

	guint16 width, height, bw;
	GtkRequisition child_req;

	width = 4;
	height = 4;
	if(!(roo->priv->flags & ROO_ICONIFIED)) {
		width += 2*GTK_CONTAINER(w)->border_width + 2;
		height += GTK_CONTAINER(w)->border_width + 2;
	}
	roo->priv->title_bar_height = w->style->font->ascent + 2*w->style->font->descent;
	height += roo->priv->title_bar_height;

	bw = 3*roo->priv->title_bar_height;

	if(roo->priv->flags & ROO_ICONIFIED && roo->priv->title)
		bw += gdk_string_width(w->style->font, roo->priv->title);
	else
		bw += gdk_string_width(w->style->font, "Abc...") + 4;

	if(GTK_BIN(w)->child && !(roo->priv->flags & ROO_ICONIFIED)) {
		gtk_widget_size_request(GTK_BIN(w)->child, &child_req);
		bw = MAX(bw, child_req.width);
		height += child_req.height;
	}

	width += bw;

	roo->priv->min_width = width;
	roo->priv->min_height = height;
	*rw = width;
	*rh = height;
}

static void
gnome_roo_size_request(GtkWidget *w, GtkRequisition *req)
{
	guint width = 0, height = 0;

	calculate_size(GNOME_ROO(w), &width, &height);

	req->width = width;
	req->height = height;
}

static void
calculate_title_size(GnomeRoo *roo)
{
	GtkWidget *w = GTK_WIDGET(roo);
	gint title_len, title_space, vis_space;

	if(roo->priv->vis_title)
		g_free(roo->priv->vis_title);

	title_space = w->allocation.width;
	title_space -= 4 + 4 + 1 + 3*roo->priv->title_bar_height;
	vis_space = gdk_string_width(w->style->font, roo->priv->title);
	if(vis_space < title_space) {
		roo->priv->vis_title = g_strdup(roo->priv->title);
		return;
	}
	title_space -= gdk_string_width(w->style->font, "...");
	title_len = 0;
	do {
		title_len++;
		vis_space = gdk_text_width(w->style->font, roo->priv->title, title_len);
	} while(roo->priv->title[title_len] != '\0' && title_space > vis_space);
	if(title_space < vis_space)
		title_len--;
	roo->priv->vis_title = g_new(gchar, title_len + 4);
	strcpy(roo->priv->vis_title + title_len, "...");
   	while(--title_len >= 0)
		roo->priv->vis_title[title_len] = roo->priv->title[title_len];
}

static void
gnome_roo_size_allocate(GtkWidget *w, GtkAllocation *allocation)
{
	GnomeRoo *roo = GNOME_ROO(w);
	GtkAllocation child_alloc;
	GtkRequisition child_req;

	w->allocation = *allocation;

	if(GTK_WIDGET_REALIZED(w)) {
		gdk_window_move_resize(w->window,
							   allocation->x, allocation->y,
							   allocation->width, allocation->height);
		gdk_window_move_resize(roo->priv->cover,
							   allocation->x, allocation->y,
							   allocation->width, allocation->height);
	}

	if(GTK_BIN(w)->child && GTK_WIDGET_VISIBLE(GTK_BIN(w)->child)) {
		gtk_widget_get_child_requisition(GTK_BIN(w)->child, &child_req);
		child_alloc.y = roo->priv->title_bar_height + 4;
		child_alloc.height = w->allocation.height - child_alloc.y;
		if(roo->priv->flags & ROO_MAXIMIZED)
			child_alloc.x = 0;
		else {
			child_alloc.x = 2 + GTK_CONTAINER(w)->border_width;
			child_alloc.height -= 2 + GTK_CONTAINER(w)->border_width;
		}
		child_alloc.width = w->allocation.width - 2*child_alloc.x;
		gtk_widget_size_allocate(GTK_BIN(w)->child, &child_alloc);
	}

	if(!((roo->priv->flags & ROO_MAXIMIZED) || (roo->priv->flags & ROO_ICONIFIED))) {
#ifdef GNOME_ENABLE_DEBUG
		g_message("GnomeRoo: user allocation %dx%d at (%d,%d)",
				  allocation->width, allocation->height,
				  allocation->x, allocation->y);
#endif
		roo->priv->user_allocation = *allocation;
	}

	calculate_title_size(roo);
}

static void
gnome_roo_realize(GtkWidget *w)
{
	GnomeRoo *roo = GNOME_ROO(w);
	GdkWindowAttr attributes;
	gint attributes_mask;

	GTK_WIDGET_SET_FLAGS(w, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = w->allocation.x;
	attributes.y = w->allocation.y;
	attributes.width = w->allocation.width;
	attributes.height = w->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual(w);
	attributes.colormap = gtk_widget_get_colormap(w);
	attributes.event_mask = gtk_widget_get_events(w) |
		                    GDK_POINTER_MOTION_MASK |
		                    GDK_BUTTON_PRESS_MASK |
		                    GDK_BUTTON_RELEASE_MASK |
		                    GDK_EXPOSURE_MASK;
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	w->window = gdk_window_new(gtk_widget_get_parent_window(w), &attributes, attributes_mask);
	gdk_window_set_user_data (w->window, w);

	attributes.wclass = GDK_INPUT_ONLY;
	attributes.event_mask = GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK;
	roo->priv->cover = gdk_window_new(gtk_widget_get_parent_window(w), &attributes, attributes_mask);
	gdk_window_set_user_data (roo->priv->cover, w);

	w->style = gtk_style_attach(w->style, w->window);
	gtk_style_set_background(w->style, w->window, GTK_STATE_NORMAL);
}

static void gnome_roo_unrealize(GtkWidget *w)
{
	GnomeRoo *roo = GNOME_ROO(w);

	if(roo->priv->cover) {
		gdk_window_destroy(roo->priv->cover);
		roo->priv->cover = NULL;
	}

	if(GTK_WIDGET_CLASS(parent_class)->unrealize)
		(*GTK_WIDGET_CLASS(parent_class)->unrealize)(w);
}

static void
gnome_roo_map(GtkWidget *w)
{
	GnomeRoo *roo = GNOME_ROO(w);

	if(GTK_WIDGET_CLASS(parent_class)->map)
		(*GTK_WIDGET_CLASS(parent_class)->map)(w);

	if(!gnome_roo_is_selected(roo)) {
#ifdef GNOME_ENABLE_DEBUG
		g_message("GnomeRoo: showing cover");
#endif
		gdk_window_show(roo->priv->cover);
	}
}

static void gnome_roo_unmap(GtkWidget *w)
{
	GnomeRoo *roo = GNOME_ROO(w);

	if(GTK_WIDGET_MAPPED(w) && !gnome_roo_is_selected(roo)) {
#ifdef GNOME_ENABLE_DEBUG
		g_message("GnomeRoo: hiding cover");
#endif
		gdk_window_hide(roo->priv->cover);
	}

	if(GTK_WIDGET_CLASS(parent_class)->unmap)
		(*GTK_WIDGET_CLASS(parent_class)->unmap)(w);
}

static GnomeRooResizeType get_resize_type(GnomeRoo *roo, gint x, gint y)
{
	GnomeRooResizeType resize;
	GtkWidget *w = GTK_WIDGET(roo);
	guint border_width = GTK_CONTAINER(roo)->border_width;
	gint ww, wh, wx, wy, wd;

	if(roo->priv->flags & (ROO_ICONIFIED | ROO_MAXIMIZED))
		return RESIZE_NONE;

	gdk_window_get_geometry(w->window, &wx, &wy, &ww, &wh, &wd);

	resize = RESIZE_NONE;
	if(x >= w->parent->allocation.width - wx || x < -wx ||
	   y >= w->parent->allocation.height - wy || y < -wy)
		;
	else if(x < border_width) {
		if(y < border_width)
			resize = RESIZE_TOP_LEFT;
		else if(y >= wh - border_width)
			resize = RESIZE_BOTTOM_LEFT;
		else
			resize = RESIZE_LEFT;
	}
	else if(x >= ww - border_width) {
		if(y < border_width)
			resize = RESIZE_TOP_RIGHT;
		else if(y >= wh - border_width)
			resize = RESIZE_BOTTOM_RIGHT;
		else
			resize = RESIZE_RIGHT;
	}
	else if(y < border_width)
		resize = RESIZE_TOP;
	else if(y >= wh - border_width)
		resize = RESIZE_BOTTOM;

	return resize;
}

static gboolean
gnome_roo_button_press(GtkWidget *w, GdkEventButton *e)
{
	GnomeRoo *roo = GNOME_ROO(w);
	GnomeRooResizeType resize;
	GdkCursor *cursor = NULL;

	if(e->window == roo->priv->cover) {
#ifdef GNOME_ENABLE_DEBUG
		g_message("GnomeRoo: got button press on cover");
#endif
		gtk_signal_emit(GTK_OBJECT(roo), roo_signals[SELECT], NULL);
	}

	if(e->button != 1)
		return FALSE;

	resize = get_resize_type(roo, e->x, e->y);
	if(resize != RESIZE_NONE) {
		if(!resize_cursor[resize])
			resize_cursor[resize] = gdk_cursor_new(resize_cursor_type[resize]);

		cursor = resize_cursor[resize];
		roo->priv->flags |= ROO_IN_RESIZE;
		roo->priv->resize_type = resize;
	}
	else if(in_title_bar(roo, e->x, e->y)) {
		/* do we want to highlight any title bar button? */
		if(in_close_button(roo, e->x, e->y))
			paint_close_button(roo, TRUE);
		else if(in_iconify_button(roo, e->x, e->y))
			paint_iconify_button(roo, TRUE);
		else if(in_maximize_button(roo, e->x, e->y))
			paint_maximize_button(roo, TRUE);
		else if(!(roo->priv->flags & ROO_MAXIMIZED)) {
			roo->priv->flags |= ROO_IN_MOVE;
			if(!move_cursor)
				move_cursor = gdk_cursor_new(GDK_FLEUR);
			cursor = move_cursor;
		}
	}

	if(roo->priv->flags &  (ROO_IN_MOVE | ROO_IN_RESIZE)) {
		roo->priv->grab_x = e->x;
		roo->priv->grab_y = e->y;

		gtk_grab_add(w);
		gdk_pointer_grab(w->window, FALSE,
						 GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
						 NULL, cursor, GDK_CURRENT_TIME);
	}

	return TRUE;
}

static gboolean
gnome_roo_button_release(GtkWidget *w, GdkEventButton *e)
{
	GnomeRoo *roo = GNOME_ROO(w);
	gint wx, wy, ww, wh, wd;

	if(e->button != 1)
		return FALSE;

	if(roo->priv->flags & ROO_IN_MOVE) {
		gdk_window_get_position(w->window, &wx, &wy);
		if(w->allocation.x != wx || w->allocation.y != wy) {
			if(roo->priv->flags & ROO_ICONIFIED)
				gnome_roo_park(roo, wx, wy);
			else
				gnome_pouch_move(GNOME_POUCH(w->parent), roo, wx, wy);
		}
		roo->priv->flags &= ~ROO_IN_MOVE;
	}
	else if(roo->priv->flags & ROO_IN_RESIZE) {
		gdk_window_get_geometry(w->window, &wx, &wy, &ww, &wh, &wd);
		if(w->allocation.x != wx || w->allocation.y != wy)
			gnome_pouch_move(GNOME_POUCH(w->parent), roo, wx, wy);
		if(w->allocation.width != ww || w->allocation.height != wh)
			gtk_widget_set_usize(w, ww, wh);
		roo->priv->flags &= ~ROO_IN_RESIZE;
	}
	else {
		if(roo->priv->flags & ROO_CLOSE_HILIT)
			paint_close_button(roo, FALSE);
		else if(roo->priv->flags & ROO_ICONIFY_HILIT)
			paint_iconify_button(roo, FALSE);
		else if(roo->priv->flags & ROO_MAXIMIZE_HILIT)
			paint_maximize_button(roo, FALSE);

		if(in_close_button(roo, e->x, e->y)) {
			gtk_signal_emit_by_name(GTK_OBJECT(w->parent), "close-child", roo, NULL);
		}
		else if(in_maximize_button(roo, e->x, e->y)) {
			gnome_roo_set_maximized(roo, !gnome_roo_is_maximized(roo));
		}
		else if(in_iconify_button(roo, e->x, e->y)) {
			if(gnome_roo_is_maximized(roo)) {
				gnome_roo_set_maximized(roo, FALSE);
				if(!(roo->priv->flags & ROO_ICONIFIED))
					gnome_roo_set_iconified(roo, TRUE);
			}
			else
				gnome_roo_set_iconified(roo, !(roo->priv->flags & ROO_ICONIFIED));
		}
	}

	gdk_pointer_ungrab(GDK_CURRENT_TIME);
	gtk_grab_remove(w);

	return TRUE;
}

static gboolean
gnome_roo_motion_notify(GtkWidget *w, GdkEventMotion *e)
{
	GnomeRoo *roo = GNOME_ROO(w);
	GnomeRooResizeType resize;
	gint dx, dy, wx, wy;

	if(roo->priv->flags & ROO_IN_MOVE) {
		dx = e->x - roo->priv->grab_x;
		dy = e->y - roo->priv->grab_y;
		gdk_window_get_position(w->window, &wx, &wy);
		wx += dx;
		wy += dy;
		wx = MAX(wx, -w->allocation.width + 3*roo->priv->title_bar_height);
		wx = MIN(wx, w->parent->allocation.width - 2*roo->priv->title_bar_height);
		wy = MAX(wy, 0);
		wy = MIN(wy, w->parent->allocation.height - roo->priv->title_bar_height);
		gdk_window_move(w->window, wx, wy);
		gdk_window_move(roo->priv->cover, wx, wy);

		return TRUE;
	}
	else if(!(roo->priv->flags & ROO_IN_RESIZE)) {
		resize = get_resize_type(roo, e->x, e->y);

		/* are we in a border area that indicates resizing? */
		if(resize != roo->priv->resize_type) {
			if(resize != RESIZE_NONE) {
				if(!resize_cursor[resize])
					resize_cursor[resize] = gdk_cursor_new(resize_cursor_type[resize]);
				gdk_window_set_cursor(w->window, resize_cursor[resize]);
				gdk_window_set_cursor(roo->priv->cover, resize_cursor[resize]);

			}
			else {
				gdk_window_set_cursor(w->window, NULL);
				gdk_window_set_cursor(roo->priv->cover, NULL);
			}
			roo->priv->resize_type = (guint)resize;
		}
	}
	else { /* roo->priv->flags & ROO_IN_RESIZE */
		gint move_x = 0, move_y = 0, resize_x = 0, resize_y = 0;
		gint new_x, new_y, new_width, new_height;
		gint max_x, max_y, max_w, max_h;
		gint pw, ph, ww, wh, wd;

		gdk_window_get_geometry(w->window, &wx, &wy, &ww, &wh, &wd);
		gdk_window_get_size(w->parent->window, &pw, &ph);

		max_x = wx + ww - roo->priv->min_width;
		max_y = wy + wh - roo->priv->min_height;

		switch(roo->priv->resize_type) {
		case RESIZE_TOP_LEFT:
			move_x = e->x;
			move_y = e->y;
			resize_x = -e->x;
			resize_y = -e->y;
			break;
		case RESIZE_TOP:
			move_y = e->y;
			resize_y = -e->y;
			break;
		case RESIZE_TOP_RIGHT:
			move_y = e->y;
			resize_x = e->x - ww;
			resize_y = -e->y;
			break;
		case RESIZE_RIGHT:
			resize_x = e->x - ww;
			break;
		case RESIZE_BOTTOM_RIGHT:
			resize_x = e->x - ww;
			resize_y = e->y - wh;
			break;
		case RESIZE_BOTTOM:
			resize_y = e->y - wh;
			break;
		case RESIZE_BOTTOM_LEFT:
			move_x = e->x;
			resize_x = -e->x;
			resize_y = e->y - wh;
			break;
		case RESIZE_LEFT:
			move_x = e->x;
			resize_x = -e->x;
			break;
		default:
			break;
		}

		new_x = wx + move_x;
		new_y = wy + move_y;
		if(new_x < 0) {
			resize_x += new_x;
			new_x = 0;
		}
		if(new_y < 0) {
			resize_y += new_y;
			new_y = 0;
		}
		new_x = MIN(new_x, max_x);
		new_y = MIN(new_y, max_y);

		new_width = ww + resize_x;
		new_height = wh + resize_y;
		new_width = MAX(roo->priv->min_width, new_width);
		new_height = MAX(roo->priv->min_height, new_height);
		max_w = pw - wx;
		max_h = ph - wy;
		new_width = MIN(new_width, max_w);
		new_height = MIN(new_height, max_h);

		if(wx != new_x || wy != new_y)
			gnome_pouch_move(GNOME_POUCH(w->parent), roo, new_x, new_y);
		if(ww != new_width || wh != new_height)
			gtk_widget_set_usize(w, new_width, new_height);
	}

	return FALSE;
}

static void
gnome_roo_maximize(GnomeRoo *roo)
{
	GtkWidget *w = GTK_WIDGET(roo);

#ifdef GNOME_ENABLE_DEBUG
		g_message("GnomeRoo: maximize");
#endif

	if(roo->priv->flags & ROO_MAXIMIZED)
		return;

	roo->priv->flags |= ROO_MAXIMIZED;

	if(roo->priv->flags & ROO_ICONIFIED) {
		roo->priv->flags |= ROO_WAS_ICONIFIED;
		gtk_signal_emit(GTK_OBJECT(roo), roo_signals[UNICONIFY], NULL);
	}

	if(w->parent)
		gtk_signal_emit_by_name(GTK_OBJECT(w->parent), "maximize-child", roo, NULL);
}

static void gnome_roo_unmaximize(GnomeRoo *roo)
{
	GtkWidget *w = GTK_WIDGET(roo);

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeRoo: unmaximize");
#endif

	if(!(roo->priv->flags & ROO_MAXIMIZED))
		return;

	roo->priv->flags &= ~ROO_MAXIMIZED;

	if(roo->priv->flags & ROO_WAS_ICONIFIED) {
		gtk_signal_emit(GTK_OBJECT(roo), roo_signals[ICONIFY], NULL);
		roo->priv->flags &= ~ROO_WAS_ICONIFIED;
	}

	if(w->parent)
		gtk_signal_emit_by_name(GTK_OBJECT(w->parent), "unmaximize-child", roo, NULL);
}

static void
gnome_roo_iconify(GnomeRoo *roo)
{
	GtkWidget *w = GTK_WIDGET(roo);
	guint icon_width = 0, icon_height = 0;

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeRoo: iconify");
#endif

	if(roo->priv->flags & ROO_MAXIMIZED)
		gtk_signal_emit(GTK_OBJECT(roo), roo_signals[UNMAXIMIZE], NULL);

	roo->priv->flags |= ROO_ICONIFIED;

	if(GTK_BIN(roo)->child && GTK_WIDGET_VISIBLE(GTK_BIN(roo)->child))
		gtk_widget_hide(GTK_BIN(roo)->child);
	calculate_size(roo, &icon_width, &icon_height);
	roo->priv->icon_allocation.width = icon_width;
	roo->priv->icon_allocation.height = icon_height;
	gtk_widget_set_usize(w, icon_width, icon_height);

	if(w->parent)
		gtk_signal_emit_by_name(GTK_OBJECT(w->parent), "iconify-child", roo, NULL);
}

static void gnome_roo_uniconify(GnomeRoo *roo)
{
	GtkWidget *w = GTK_WIDGET(roo);

#ifdef GNOME_ENABLE_DEBUG
		g_message("GnomeRoo: uniconify");
#endif

	if(!(roo->priv->flags & ROO_ICONIFIED))
		return;

	roo->priv->flags &= ~ROO_ICONIFIED;

	if(GTK_BIN(roo)->child && !GTK_WIDGET_VISIBLE(GTK_BIN(roo)->child)) {
		/* recall last size and position */
		if(w->parent) {
			gnome_pouch_move(GNOME_POUCH(w->parent), roo,
							 roo->priv->user_allocation.x,
							 roo->priv->user_allocation.y);
			gtk_widget_set_usize(w,
								 roo->priv->user_allocation.width,
								 roo->priv->user_allocation.height);
		}
		gtk_widget_show(GTK_BIN(roo)->child);
	}

	gtk_widget_queue_resize(w);

	if(w->parent)
		gtk_signal_emit_by_name(GTK_OBJECT(w->parent), "uniconify-child", roo, NULL);
}

static void
gnome_roo_select(GnomeRoo *roo)
{
	GtkWidget *w = GTK_WIDGET(roo);

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeRoo: select");
#endif
	roo->priv->flags |= ROO_SELECTED;

	if(GTK_WIDGET_MAPPED(roo)) {
		gdk_window_hide(roo->priv->cover);
		gtk_widget_queue_draw(w);
	}

	if(w->parent)
		gtk_signal_emit_by_name(GTK_OBJECT(w->parent), "select-child", roo, NULL);
}

static void gnome_roo_deselect(GnomeRoo *roo)
{
	GtkWidget *w = GTK_WIDGET(roo);

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeRoo: deselect");
#endif
	roo->priv->flags &= ~ROO_SELECTED;

	if(GTK_WIDGET_MAPPED(roo)) {
		gdk_window_show(roo->priv->cover);
		gtk_widget_queue_draw(w);
	}

	if(w->parent)
		gtk_signal_emit_by_name(GTK_OBJECT(w->parent), "unselect-child", roo, NULL);
}

static void
gnome_roo_paint(GtkWidget *w, GdkRectangle *area)
{
	GnomeRoo *roo = GNOME_ROO(w);
	guint border_width = GTK_CONTAINER(w)->border_width;
	GtkStateType state;

	if(roo->priv->flags & ROO_SELECTED)
		state = GTK_STATE_SELECTED;
	else
		state = GTK_STATE_NORMAL;

	gdk_gc_set_clip_rectangle(w->style->mid_gc[state], area);

	/* draw borders */
	gdk_draw_rectangle(w->window, w->style->mid_gc[state], TRUE,
					   1, 1,
					   w->allocation.width - 2, roo->priv->title_bar_height + 2);
	if(!(roo->priv->flags & ROO_MAXIMIZED)) {
		gdk_draw_rectangle(w->window, w->style->mid_gc[state], TRUE,
						   1, roo->priv->title_bar_height + 3,
						   border_width, w->allocation.height - roo->priv->title_bar_height - 2);
		gdk_draw_rectangle(w->window, w->style->mid_gc[state], TRUE,
						   w->allocation.width - border_width - 1, roo->priv->title_bar_height + 3,
						   border_width, w->allocation.height - roo->priv->title_bar_height - 2);
		gdk_draw_rectangle(w->window, w->style->mid_gc[state], TRUE,
						   1, w->allocation.height - border_width - 1,
						   w->allocation.width - 2, border_width);
	}

	/* draw border bevels */
	if(roo->priv->flags & ROO_MAXIMIZED)
		draw_shadow(w, state, TRUE, 0, 0, w->allocation.width, roo->priv->title_bar_height + 4);
	else {
		draw_shadow(w, state, TRUE, 0, 0, w->allocation.width, w->allocation.height);
		if(!(roo->priv->flags & ROO_ICONIFIED))
			draw_shadow(w, state, FALSE,
						border_width + 1, roo->priv->title_bar_height + 3,
						w->allocation.width - 2*(1 + border_width),
						w->allocation.height - border_width - 1 - roo->priv->title_bar_height - 3);
	}

	/* paint buttons */
	paint_close_button(roo, roo->priv->flags & ROO_CLOSE_HILIT);
	paint_iconify_button(roo, roo->priv->flags & ROO_ICONIFY_HILIT);
	paint_maximize_button(roo, roo->priv->flags & ROO_MAXIMIZE_HILIT);

	/* TODO: properly draw title */
	if(roo->priv->title)
		gtk_paint_string(w->style, w->window,
						 state, area,
						 w, "gnome-roo",
						 roo->priv->title_bar_height + 4, w->style->font->ascent + w->style->font->descent,
						 roo->priv->vis_title);
}

static gboolean gnome_roo_expose(GtkWidget *w, GdkEventExpose *e)
{
	if(GTK_WIDGET_DRAWABLE(w)) {
		gnome_roo_paint(w, &e->area);
	}

	if(GTK_WIDGET_CLASS(parent_class)->expose_event)
		(*GTK_WIDGET_CLASS(parent_class)->expose_event)(w, e);

	return FALSE;
}

/**
 * gnome_roo_set_title:
 * @roo: A pointer to a GnomeRoo widget.
 * @title: The new title.
 * 
 * Description:
 * Changes the title of a @roo to @title.
 **/
void
gnome_roo_set_title(GnomeRoo *roo, const gchar *title)
{
	g_return_if_fail(GNOME_IS_ROO(roo));

	if(roo->priv->title)
		g_free(roo->priv->title);
	roo->priv->title = g_strdup(title);
	gtk_widget_queue_draw(GTK_WIDGET(roo));
}

/**
 * gnome_roo_get_title:
 * @roo: A pointer to a GnomeRoo widget.
 * 
 * Description:
 * Retrieves the title of a @roo.
 *
 * Return value:
 * @roo's title.
 **/
const gchar *
gnome_roo_get_title(GnomeRoo *roo)
{
	g_return_val_if_fail(roo != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_ROO(roo), NULL);

	return roo->priv->title;
}

/**
 * gnome_roo_set_iconified:
 * @roo: A pointer to a GnomeRoo widget.
 * @iconified: A boolean value indicating if the roo should be
 * iconified or not.
 * 
 * Description:
 * Iconifies or uniconifies the @roo according to the value of @iconified.
 **/
void
gnome_roo_set_iconified(GnomeRoo *roo, gboolean iconified)
{
	g_return_if_fail(GNOME_IS_ROO(roo));

	if(iconified && !gnome_roo_is_iconified(roo))
		gtk_signal_emit(GTK_OBJECT(roo), roo_signals[ICONIFY], NULL);
	else if(!iconified && gnome_roo_is_iconified(roo))
		gtk_signal_emit(GTK_OBJECT(roo), roo_signals[UNICONIFY], NULL);
}

/**
 * gnome_roo_set_maximized:
 * @roo: A pointer to a GnomeRoo widget.
 * @maximized: A boolean value indicating if the view should be
 * maximized or not.
 * 
 * Description:
 * Maximizes or unmaximizes the @roo according to the value of @maximized.
 **/
void
gnome_roo_set_maximized(GnomeRoo *roo, gboolean maximized)
{
	g_return_if_fail(GNOME_IS_ROO(roo));

	if(maximized && !gnome_roo_is_maximized(roo))
		gtk_signal_emit(GTK_OBJECT(roo), roo_signals[MAXIMIZE], NULL);
	else if(!maximized && gnome_roo_is_maximized(roo))
		gtk_signal_emit(GTK_OBJECT(roo), roo_signals[UNMAXIMIZE], NULL);
}

/**
 * gnome_roo_is_iconified:
 * @roo: A pointer to a GnomeRoo widget.
 * 
 * Description:
 * Returns %TRUE if the @roo is iconified and %FALSE otherwise.
 *
 * Return value:
 * A gboolean indicating if the @roo is iconified.
 **/
gboolean
gnome_roo_is_iconified(GnomeRoo *roo)
{
	g_return_val_if_fail(GNOME_IS_ROO(roo), FALSE);

	return roo->priv->flags & ROO_ICONIFIED;
}

/**
 * gnome_roo_is_maximized:
 * @roo: A pointer to a GnomeRoo widget.
 * 
 * Description:
 * Returns %TRUE if the @roo is maximized and %FALSE otherwise.
 *
 * Return value:
 * A gboolean indicating if the @roo is maximized.
 **/
gboolean
gnome_roo_is_maximized(GnomeRoo *roo)
{
	g_return_val_if_fail(GNOME_IS_ROO(roo), FALSE);

	return roo->priv->flags & ROO_MAXIMIZED;
}

/**
 * gnome_roo_is_selected:
 * @roo: A pointer to a GnomeRoo widget.
 * 
 * Description:
 * Returns %TRUE if the @roo is selected and %FALSE otherwise.
 *
 * Return value:
 * A gboolean indicating if the @roo is selected.
 **/
gboolean
gnome_roo_is_selected(GnomeRoo *roo)
{
	g_return_val_if_fail(GNOME_IS_ROO(roo), FALSE);

	return roo->priv->flags & ROO_SELECTED;
}

/**
 * gnome_roo_park:
 * @roo: A pointer to a GnomeRoo widget.
 * @x: X coordinate of the parking space
 * @y: Y coordinate of the parking space
 * 
 * Description:
 * At each next iconification, @roo will be moved to (@x, @y). If the @roo
 * is already iconified, it is moved there immediately.
 **/
void
gnome_roo_park(GnomeRoo *roo, gint x, gint y)
{
	GtkWidget *w;

	g_return_if_fail(GNOME_IS_ROO(roo));

	w = GTK_WIDGET(roo);

	roo->priv->flags |= ROO_ICON_PARKED;
	if(gnome_roo_is_iconified(roo) && w->parent)
		gnome_pouch_move(GNOME_POUCH(w->parent), roo, x, y);
	else {
		roo->priv->icon_allocation.x = x;
		roo->priv->icon_allocation.y = y;
	}

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeRoo: parked at %d,%d\n",
			  roo->priv->icon_allocation.x, roo->priv->icon_allocation.y);
#endif
}

/**
 * gnome_roo_unpark:
 * @roo: A pointer to a GnomeRoo widget.
 * 
 * Description:
 * As of the next iconification, the @roo's icon will not be moved to
 * the position set with a previous call to gnome_roo_park() anymore.
 **/
void gnome_roo_unpark(GnomeRoo *roo)
{
	g_return_if_fail(GNOME_IS_ROO(roo));

	roo->priv->flags &= ~ROO_ICON_PARKED;
}

/**
 * gnome_roo_is_parked:
 * @roo: A pointer to a GnomeRoo widget.
 * 
 * Description:
 * Returns %TRUE if the @roo has been parked. The return value is valid
 * even if the @roo is currently not iconified and therefore not parked.
 *
 * Return value:
 * A gboolean indicating if the @roo has been parked. 
 **/
gboolean
gnome_roo_is_parked(GnomeRoo *roo)
{
	return roo->priv->flags & ROO_ICON_PARKED;
}

/**
 * gnome_roo_new:
 * 
 * Description:
 * Creates a new GnomeRoo widget.
 *
 * Return value:
 * A pointer to a GnomeRoo widget.
 **/
GtkWidget *
gnome_roo_new()
{
	return gtk_type_new(gnome_roo_get_type());
}
