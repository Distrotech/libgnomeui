/* GnomeIconList widget - scrollable icon list
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

/* Much of this file is shamelessly ripped from the GtkCList widget by Jay Painter */

#include <gdk_imlib.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkhscrollbar.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkvscrollbar.h>
#include "gnome-icon-list.h"
#include <string.h>

#define DEFAULT_ROW_SPACING  6
#define DEFAULT_COL_SPACING  6
#define DEFAULT_TEXT_SPACING 4
#define DEFAULT_ICON_BORDER  4
#define DEFAULT_WIDTH        70
#define SCROLLBAR_SPACING    5 /* this is the same value as in GtkCList */

#define ILIST_WIDTH(ilist) (ilist->icon_cols * (ilist->max_icon_width + ilist->col_spacing) - ilist->col_spacing)
#define ILIST_HEIGHT(ilist) (ilist->icon_rows * (ilist->max_icon_height + ilist->row_spacing) - ilist->row_spacing)


typedef struct {
	GdkImlibImage *im;
	char *text;

	GdkPixmap *pixmap;
	GdkBitmap *mask;
	struct gnome_icon_text_info *ti;

	GtkStateType state;

	gpointer data;
	GtkDestroyNotify destroy;

	GdkColor foreground;
	GdkColor background;

	int fg_set : 1;
	int bg_set : 1;
} Icon;

typedef enum {
	SYNC_INSERT,
	SYNC_REMOVE
} SyncType;


enum {
	SELECT_ICON,
	UNSELECT_ICON,
	LAST_SIGNAL
};

typedef void (* GnomeIconListSignal1) (GtkObject *object,
				       gint       arg1,
				       GdkEvent  *event,
				       gpointer   data);


static void gnome_icon_list_class_init     (GnomeIconListClass *class);
static void gnome_icon_list_init           (GnomeIconList      *ilist);
static void gnome_icon_list_destroy        (GtkObject          *object);
static void gnome_icon_list_realize        (GtkWidget          *widget);
static void gnome_icon_list_unrealize      (GtkWidget          *widget);
static void gnome_icon_list_map            (GtkWidget          *widget);
static void gnome_icon_list_unmap          (GtkWidget          *widget);
static void gnome_icon_list_size_request   (GtkWidget          *widget,
					    GtkRequisition     *requisition);
static void gnome_icon_list_size_allocate  (GtkWidget          *widget,
					    GtkAllocation      *allocation);
static void gnome_icon_list_draw           (GtkWidget          *widget,
					    GdkRectangle       *area);
static gint gnome_icon_list_button_press   (GtkWidget          *widget,
					    GdkEventButton     *event);
static gint gnome_icon_list_button_release (GtkWidget          *widget,
					    GdkEventButton     *event);
static gint gnome_icon_list_expose         (GtkWidget          *widget,
					    GdkEventExpose     *event);
static void gnome_icon_list_foreach        (GtkContainer       *container,
					    GtkCallback         callback,
					    gpointer            callback_data);

static void real_select_icon (GnomeIconList *ilist, gint num, GdkEvent *event);
static void real_unselect_icon (GnomeIconList *ilist, gint num, GdkEvent *event);

static void gnome_icon_list_marshal_signal_1 (GtkObject     *object,
					      GtkSignalFunc  func,
					      gpointer       func_data,
					      GtkArg        *args);


static GtkContainerClass *parent_class;

static ilist_signals[LAST_SIGNAL] = { 0 };


guint
gnome_icon_list_get_type (void)
{
	static guint icon_list_type = 0;

	if (!icon_list_type) {
		GtkTypeInfo icon_list_info = {
			"GnomeIconList",
			sizeof (GnomeIconList),
			sizeof (GnomeIconListClass),
			(GtkClassInitFunc) gnome_icon_list_class_init,
			(GtkObjectInitFunc) gnome_icon_list_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		icon_list_type = gtk_type_unique (gtk_container_get_type (), &icon_list_info);
	}

	return icon_list_type;
}

static void
gnome_icon_list_class_init (GnomeIconListClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkContainerClass *container_class;

	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;
	container_class = (GtkContainerClass *) class;

	parent_class = gtk_type_class (gtk_container_get_type ());

	ilist_signals[SELECT_ICON] =
		gtk_signal_new ("select_icon",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeIconListClass, select_icon),
				gnome_icon_list_marshal_signal_1,
				GTK_TYPE_NONE, 2,
				GTK_TYPE_INT,
				GTK_TYPE_POINTER);

	ilist_signals[UNSELECT_ICON] =
		gtk_signal_new ("unselect_icon",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeIconListClass, unselect_icon),
				gnome_icon_list_marshal_signal_1,
				GTK_TYPE_NONE, 2,
				GTK_TYPE_INT,
				GTK_TYPE_POINTER);

	gtk_object_class_add_signals (object_class, ilist_signals, LAST_SIGNAL);

	object_class->destroy = gnome_icon_list_destroy;

	widget_class->realize = gnome_icon_list_realize;
	widget_class->unrealize = gnome_icon_list_unrealize;
	widget_class->map = gnome_icon_list_map;
	widget_class->unmap = gnome_icon_list_unmap;
	widget_class->size_request = gnome_icon_list_size_request;
	widget_class->size_allocate = gnome_icon_list_size_allocate;
	widget_class->draw = gnome_icon_list_draw;
	widget_class->button_press_event = gnome_icon_list_button_press;
	widget_class->button_release_event = gnome_icon_list_button_release;
	widget_class->expose_event = gnome_icon_list_expose;

	container_class->foreach = gnome_icon_list_foreach;

	class->select_icon = real_select_icon;
	class->unselect_icon = real_unselect_icon;
}

static void
get_icon_num_from_xy (GnomeIconList *ilist, int x, int y, int *num, int *on_spacing)
{
	int row, col;
	int width, height;

	if (ilist->icons == 0) {
		*num = -1;
		*on_spacing = TRUE;
	}

	x += ilist->x_offset;
	y += ilist->y_offset;

	if ((x > ILIST_WIDTH (ilist)) || (y > ILIST_HEIGHT (ilist))) {
		*num = -1;
		*on_spacing = TRUE;

		return;
	}

	width = ilist->max_icon_width + ilist->col_spacing;
	height = ilist->max_icon_height + ilist->row_spacing;

	col = x / width;
	row = y / height;

	if ((row == (ilist->icon_rows - 1))
	    && ((ilist->icons % ilist->icon_cols) != 0)
	    && (col >= (ilist->icons % ilist->icon_cols))) {
		*num = -1;
		*on_spacing = TRUE;
	}

	*num = row * ilist->icon_cols + col;

	if ((x >= (col * width + ilist->max_icon_width))
	    || (y >= (row * height + ilist->max_icon_height)))
		*on_spacing = TRUE;
	else
		*on_spacing = FALSE;
}

static void
get_icon_xy_from_num (GnomeIconList *ilist, int pos, int *x, int *y)
{
	int row, col;

	row = pos / ilist->icon_cols;
	col = pos % ilist->icon_cols;

	*x = col * (ilist->max_icon_width + ilist->col_spacing) - ilist->x_offset;
	*y = row * (ilist->max_icon_height + ilist->row_spacing) - ilist->y_offset;
}

static void
draw_icon (GnomeIconList *ilist, Icon *icon, int x, int y, GdkRectangle *area)
{
	int xpix, ypix;
	int xtext, ytext;
	GdkGC *fg_gc, *bg_gc;
	GtkStyle *style;
	GdkRectangle iarea, warea, aarea;
	int width, height;

	/* See if we should paint */

	iarea.x = x;
	iarea.y = y;
	iarea.width = ilist->max_icon_width;
	iarea.height = ilist->max_icon_height;

	if (area)
		warea = *area;
	else {
		warea.x = 0;
		warea.y = 0;
		warea.width = ilist->ilist_window_width;
		warea.height = ilist->ilist_window_height;
	}

	if (!gdk_rectangle_intersect (&iarea, &warea, &aarea))
		return;

	/* Set colors */

	style = GTK_WIDGET (ilist)->style;

	if (icon->state == GTK_STATE_SELECTED) {
		fg_gc = style->fg_gc[GTK_STATE_SELECTED];
		bg_gc = style->bg_gc[GTK_STATE_SELECTED];
	} else {
		if (icon->fg_set) {
			gdk_gc_set_foreground (ilist->fg_gc, &icon->foreground);
			fg_gc = ilist->fg_gc;
		} else
			fg_gc = style->fg_gc[GTK_STATE_PRELIGHT];

		if (icon->bg_set) {
			gdk_gc_set_foreground (ilist->bg_gc, &icon->background);
			bg_gc = ilist->bg_gc;
		} else
			bg_gc = style->bg_gc[GTK_STATE_PRELIGHT];
	}

	/* Background */

	gdk_draw_rectangle (ilist->ilist_window,
			    bg_gc,
			    TRUE,
			    aarea.x, aarea.y,
			    aarea.width, aarea.height);

	/* Calculate positions */

	width = icon->im ? icon->im->rgb_width : 0;
	height = icon->im ? icon->im->rgb_height : 0;

	switch (ilist->mode) {
	case GNOME_ICON_LIST_ICONS:
		xpix = (ilist->max_icon_width - width) / 2;
		ypix = (ilist->max_icon_height - height) / 2;
		xtext = 0;
		ytext = 0;
		break;

	case GNOME_ICON_LIST_TEXT_BELOW:
		xpix = (ilist->max_icon_width - width) / 2;
		ypix = ilist->icon_border + (ilist->max_pixmap_height - height) / 2;
		xtext = 0;
		ytext = ilist->icon_border + ilist->max_pixmap_height + ilist->text_spacing;

		break;

	case GNOME_ICON_LIST_TEXT_RIGHT:
		xpix = ilist->icon_border + (ilist->max_pixmap_width - width) / 2;
		ypix = (ilist->max_icon_height - height) / 2;
		xtext = ilist->icon_border + ilist->max_pixmap_width + ilist->text_spacing;
		ytext = (ilist->max_icon_height - (style->font->ascent + style->font->descent)) / 2 + style->font->ascent;

		break;

	default:
		g_assert_not_reached ();
	}

	xpix += x;
	ypix += y;
	xtext += x;
	ytext += y;

	/* Pixmap */

	if (icon->pixmap) {
		if (icon->mask) {
			gdk_gc_set_clip_mask (ilist->fg_gc, icon->mask);
			gdk_gc_set_clip_origin (ilist->fg_gc, xpix, ypix);
		}

		gdk_draw_pixmap (ilist->ilist_window,
				 ilist->fg_gc,
				 icon->pixmap,
				 0, 0,
				 xpix, ypix,
				 width,
				 height);

		if (icon->mask) {
			gdk_gc_set_clip_origin (ilist->fg_gc, 0, 0);
			gdk_gc_set_clip_mask (ilist->fg_gc, NULL);
		}
	}

	/* Text */

	if (ilist->mode != GNOME_ICON_LIST_ICONS)
		gnome_icon_paint_text (icon->ti, ilist->ilist_window, fg_gc, xtext, ytext, ilist->max_icon_width);
}

static void
draw_icon_by_num (GnomeIconList *ilist, Icon *icon, int num)
{
	int x, y;
	GdkRectangle area;

	get_icon_xy_from_num (ilist, num, &x, &y);

	area.x = x;
	area.y = y;
	area.width = ilist->max_icon_width;
	area.height = ilist->max_icon_height;

	draw_icon (ilist, icon, x, y, NULL);
}

static void
draw_icons_area (GnomeIconList *ilist, GdkRectangle *area)
{
	GdkRectangle iarea;
	int num, on_spacing;
	GtkStyle *style;
	GList *start_list, *list;
	int startx, starty;
	int x, y, n;

	style = GTK_WIDGET (ilist)->style;

	if (area)
		iarea = *area;
	else {
		iarea.x = 0;
		iarea.y = 0;
		iarea.width = ilist->ilist_window_width;
		iarea.height = ilist->ilist_window_height;
	}

	if ((iarea.width == 0) || (iarea.height == 0))
		return;

	gdk_window_clear_area (ilist->ilist_window, iarea.x, iarea.y, iarea.width, iarea.height);

	/* Calc initial icon */

	get_icon_num_from_xy (ilist, iarea.x, iarea.y, &num, &on_spacing);

	if (num == -1)
		return; /* No icons to draw */

	/* FIXME: this lookup is expensive */

	start_list = g_list_nth (ilist->icon_list, num);

	get_icon_xy_from_num (ilist, num, &startx, &starty);

	for (y = starty; (y < (iarea.y + iarea.height)) && start_list;
	     y += ilist->max_icon_height + ilist->row_spacing) {
		/* Draw row of icons */

		list = start_list;

		x = startx;

		for (n = (num % ilist->icon_cols);
		     (n < ilist->icon_cols) && list && (x < (iarea.x + iarea.width));
		     n++) {
			draw_icon (ilist, list->data, x, y, &iarea);
			list = list->next;

			x += ilist->max_icon_width + ilist->col_spacing;
		}

		/* Skip to next row of icons */

		start_list = g_list_nth (start_list, ilist->icon_cols);
	}
}

static void
check_exposures (GnomeIconList *ilist)
{
	GdkEvent *event;

	while ((event = gdk_event_get_graphics_expose (ilist->ilist_window)) != NULL) {
		gtk_widget_event (GTK_WIDGET (ilist), event);

		if (event->expose.count == 0) {
			gdk_event_free (event);
			break;
		}

		gdk_event_free (event);
	}
}

static void
vadjustment_value_changed (GtkAdjustment *adj, gpointer data)
{
	GnomeIconList *ilist;
	int value, diff;
	GdkRectangle area;

	ilist = GNOME_ICON_LIST (data);

	if (!GTK_WIDGET_DRAWABLE (ilist))
		return;

	value = adj->value;

	if (value > ilist->y_offset) {
		/* scroll down */

		diff = value - ilist->y_offset;

		if (diff >= ilist->ilist_window_height) {
			/* paint everything */

			ilist->y_offset = value;
			draw_icons_area (ilist, NULL);
			return;
		}

		if (diff != 0)
			gdk_window_copy_area (ilist->ilist_window,
					      ilist->fg_gc,
					      0, 0,
					      ilist->ilist_window,
					      0, diff,
					      ilist->ilist_window_width,
					      ilist->ilist_window_height - diff);
		
		area.x = 0;
		area.y = ilist->ilist_window_height - diff;
		area.width = ilist->ilist_window_width;
		area.height = diff;
	} else {
		/* scroll up */

		diff = ilist->y_offset - value;

		if (diff >= ilist->ilist_window_height) {
			/* paint everything */

			ilist->y_offset = value;
			draw_icons_area (ilist, NULL);
			return;
		}

		if (diff != 0)
			gdk_window_copy_area (ilist->ilist_window,
					      ilist->fg_gc,
					      0, diff,
					      ilist->ilist_window,
					      0, 0,
					      ilist->ilist_window_width,
					      ilist->ilist_window_height - diff);

		area.x = 0;
		area.y = 0;
		area.width = ilist->ilist_window_width;
		area.height = diff;
	}

	ilist->y_offset = value;

	if ((diff != 0) && (diff != ilist->ilist_window_height))
		check_exposures (ilist);

	draw_icons_area (ilist, &area);
}

static void
hadjustment_value_changed (GtkAdjustment *adj, gpointer data)
{
	GnomeIconList *ilist;
	int value, diff;
	GdkRectangle area;

	ilist = GNOME_ICON_LIST (data);

	if (!GTK_WIDGET_DRAWABLE (ilist))
		return;

	value = adj->value;

	if (value > ilist->x_offset) {
		/* scroll right */

		diff = value - ilist->x_offset;

		if (diff >= ilist->ilist_window_width) {
			/* paint everything */

			ilist->x_offset = value;
			draw_icons_area (ilist, NULL);
			return;
		}

		if (diff != 0)
			gdk_window_copy_area (ilist->ilist_window,
					      ilist->fg_gc,
					      0, 0,
					      ilist->ilist_window,
					      diff, 0,
					      ilist->ilist_window_width - diff,
					      ilist->ilist_window_height);
		
		area.x = ilist->ilist_window_width - diff;
		area.y = 0;
		area.width = diff;
		area.height = ilist->ilist_window_height;
	} else {
		/* scroll left */

		diff = ilist->x_offset - value;

		if (diff >= ilist->ilist_window_width) {
			/* paint everything */

			ilist->x_offset = value;
			draw_icons_area (ilist, NULL);
			return;
		}

		if (diff != 0)
			gdk_window_copy_area (ilist->ilist_window,
					      ilist->fg_gc,
					      diff, 0,
					      ilist->ilist_window,
					      0, 0,
					      ilist->ilist_window_width - diff,
					      ilist->ilist_window_height);

		area.x = 0;
		area.y = 0;
		area.width = diff;
		area.height = ilist->ilist_window_height;
	}

	ilist->x_offset = value;

	if ((diff != 0) && (diff != ilist->ilist_window_width))
		check_exposures (ilist);

	draw_icons_area (ilist, &area);
}

static void
gnome_icon_list_init (GnomeIconList *ilist)
{
	GtkAdjustment *adj;

	GTK_WIDGET_UNSET_FLAGS (ilist, GTK_NO_WINDOW);

	ilist->icons = 0;
	ilist->icon_list = NULL;
	ilist->icon_list_end = NULL;

	ilist->row_spacing = DEFAULT_ROW_SPACING;
	ilist->col_spacing = DEFAULT_COL_SPACING;
	ilist->text_spacing = DEFAULT_TEXT_SPACING;
	ilist->icon_border = DEFAULT_ICON_BORDER;

	ilist->x_offset = 0;
	ilist->y_offset = 0;

	ilist->max_icon_width = 0;
	ilist->max_icon_height = 0;
	ilist->max_pixmap_width = 0;
	ilist->max_pixmap_height = 0;

	ilist->icon_rows = 0;
	ilist->icon_cols = 0;

	ilist->separators = g_strdup (" ");

	ilist->mode = GNOME_ICON_LIST_TEXT_BELOW;
	ilist->frozen = TRUE; /* starts frozen! */
	ilist->dirty  = TRUE;
	
	ilist->ilist_window = NULL;
	ilist->ilist_window_width = 0;
	ilist->ilist_window_height = 0;

	ilist->fg_gc = NULL;
	ilist->bg_gc = NULL;

	ilist->border_type = GTK_SHADOW_IN;
	ilist->selection_mode = GTK_SELECTION_SINGLE;

	ilist->selection = NULL;
	ilist->last_selected = 0;
	
	/* Create scrollbars */

	ilist->vscrollbar = gtk_vscrollbar_new (NULL);
	adj = gtk_range_get_adjustment (GTK_RANGE (ilist->vscrollbar));
	gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			    (GtkSignalFunc) vadjustment_value_changed,
			    ilist);
	gtk_signal_connect (GTK_OBJECT (adj), "changed",
			    (GtkSignalFunc) vadjustment_value_changed,
			    ilist);
	gtk_widget_set_parent (ilist->vscrollbar, GTK_WIDGET (ilist));
	gtk_widget_show (ilist->vscrollbar);

	ilist->hscrollbar = gtk_hscrollbar_new (NULL);
	adj = gtk_range_get_adjustment (GTK_RANGE (ilist->hscrollbar));
	gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
			    (GtkSignalFunc) hadjustment_value_changed,
			    ilist);
	gtk_signal_connect (GTK_OBJECT (adj), "changed",
			    (GtkSignalFunc) hadjustment_value_changed,
			    ilist);
	gtk_widget_set_parent (ilist->hscrollbar, GTK_WIDGET (ilist));
	gtk_widget_show (ilist->hscrollbar);

	ilist->vscrollbar_policy = GTK_POLICY_ALWAYS;
	ilist->hscrollbar_policy = GTK_POLICY_ALWAYS;
}

static void
gnome_icon_list_destroy (GtkObject *object)
{
	GnomeIconList *ilist;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (object));

	ilist = GNOME_ICON_LIST (object);

	g_free (ilist->separators);

	ilist->frozen = TRUE;
	ilist->dirty  = TRUE;
	gnome_icon_list_clear (ilist);

	if (ilist->vscrollbar) {
		gtk_widget_unparent (ilist->vscrollbar);
		ilist->vscrollbar = NULL;
	}

	if (ilist->hscrollbar) {
		gtk_widget_unparent (ilist->hscrollbar);
		ilist->hscrollbar = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

GtkWidget *
gnome_icon_list_new (void)
{
	return GTK_WIDGET (gtk_type_new (gnome_icon_list_get_type ()));
}

static void
calc_icon_size (GnomeIconList *ilist, Icon *icon, int *width, int *height)
{
	GdkFont *font;
	int pixmap_w, pixmap_h;
	int text_w, text_h;
	static int desired_size;

	if (!GTK_WIDGET_REALIZED (ilist))
		g_warning ("calc_icon_size: oops, ilist not realized");

	font = GTK_WIDGET (ilist)->style->font;
	if (!desired_size){
		desired_size = gdk_string_width (font, "XXXXXXXXXX");
		if (desired_size == 0){
			desired_size = 80;
		}
	}

	icon->ti = gnome_icon_layout_text (font, icon->text, ilist->separators, desired_size, TRUE);

	pixmap_w = icon->im ? icon->im->rgb_width : 0;
	pixmap_h = icon->im ? icon->im->rgb_height : 0;
	text_w = icon->ti->width;
	text_h = icon->ti->height;

	switch (ilist->mode) {
	case GNOME_ICON_LIST_ICONS:
		*width = pixmap_w;
		*height = pixmap_h;
		break;

	case GNOME_ICON_LIST_TEXT_BELOW:
		*width = MAX (pixmap_w, text_w);
		*height = pixmap_h + ilist->text_spacing + text_h;
		break;

	case GNOME_ICON_LIST_TEXT_RIGHT:
		*width = pixmap_w + ilist->text_spacing + text_w;
		*height = MAX (pixmap_h, text_h);
		break;

	default:
		g_assert_not_reached ();
	}

	*width += 2 * ilist->icon_border;
	*height += 2 * ilist->icon_border;
}

static void
recalc_max_icon_size_1 (GnomeIconList *ilist, Icon *icon)
{
	int width, height;
	int w, h;

	calc_icon_size (ilist, icon, &width, &height);

	if (width > ilist->max_icon_width)
		ilist->max_icon_width = width;

	if (height > ilist->max_icon_height)
		ilist->max_icon_height = height;

	w = icon->im ? icon->im->rgb_width : 0;
	h = icon->im ? icon->im->rgb_height : 0;

	if (w > ilist->max_pixmap_width)
		ilist->max_pixmap_width = w;

	if (h > ilist->max_pixmap_height)
		ilist->max_pixmap_height = h;
}

static void
recalc_max_icon_size (GnomeIconList *ilist)
{
	GList *list;

	ilist->max_icon_width = 0;
	ilist->max_icon_height = 0;

	for (list = ilist->icon_list; list; list = list->next)
		recalc_max_icon_size_1 (ilist, list->data);
}

static void
gnome_icon_list_realize (GtkWidget *widget)
{
	GnomeIconList *ilist;
	int border_width;
	GdkWindowAttr attributes;
	gint attributes_mask;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (widget));

	ilist = GNOME_ICON_LIST (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	border_width = GTK_CONTAINER (widget)->border_width;

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x + border_width;
	attributes.y = widget->allocation.y + border_width;
	attributes.width = widget->allocation.width - 2 * border_width;
	attributes.height = widget->allocation.height - 2 * border_width;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gdk_imlib_get_visual ();
	attributes.colormap = gdk_imlib_get_colormap ();
	attributes.event_mask = (gtk_widget_get_events (widget)
				 | GDK_EXPOSURE_MASK);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	/* Main window */

	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, ilist);

	widget->style = gtk_style_attach (widget->style, widget->window);

	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

	/* List window */

	attributes.x += widget->style->klass->xthickness;
	attributes.y += widget->style->klass->ythickness;
	attributes.width -= (2 * widget->style->klass->xthickness
			     + (GTK_WIDGET_VISIBLE (ilist->vscrollbar) ? ilist->vscrollbar_width : 0));
	attributes.height -= (2 * widget->style->klass->ythickness
			     + (GTK_WIDGET_VISIBLE (ilist->hscrollbar) ? ilist->hscrollbar_height : 0));
	attributes.event_mask |= GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK;

	ilist->ilist_window = gdk_window_new (widget->window, &attributes, attributes_mask);
	gdk_window_set_user_data (ilist->ilist_window, ilist);

	gdk_window_set_background (ilist->ilist_window, &widget->style->bg[GTK_STATE_PRELIGHT]);

	/* GCs */

	ilist->fg_gc = gdk_gc_new (ilist->ilist_window);
	ilist->bg_gc = gdk_gc_new (ilist->ilist_window);

	gdk_gc_set_exposures (ilist->fg_gc, TRUE);

	/* FIXME: is this the proper place to do this? */

	recalc_max_icon_size (ilist);
}

static void
gnome_icon_list_unrealize (GtkWidget *widget)
{
	GnomeIconList *ilist;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (widget));

	ilist = GNOME_ICON_LIST (widget);

	ilist->frozen = TRUE;
	ilist->dirty  = TRUE;
	gdk_gc_destroy (ilist->fg_gc);
	gdk_gc_destroy (ilist->bg_gc);

	gdk_window_set_user_data (ilist->ilist_window, NULL);
	gdk_window_destroy (ilist->ilist_window);

	ilist->ilist_window = NULL;
	ilist->fg_gc = NULL;
	ilist->bg_gc = NULL;

	if (GTK_WIDGET_CLASS (parent_class)->unrealize)
		(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static void
gnome_icon_list_map (GtkWidget *widget)
{
	GnomeIconList *ilist;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (widget));

	if (GTK_WIDGET_MAPPED (widget))
		return;

	ilist = GNOME_ICON_LIST (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

	gdk_window_show (widget->window);
	gdk_window_show (ilist->ilist_window);

	if (GTK_WIDGET_VISIBLE (ilist->vscrollbar) && !GTK_WIDGET_MAPPED (ilist->vscrollbar))
		gtk_widget_map (ilist->vscrollbar);

	if (GTK_WIDGET_VISIBLE (ilist->hscrollbar) && !GTK_WIDGET_MAPPED (ilist->hscrollbar))
		gtk_widget_map (ilist->hscrollbar);

	ilist->frozen = FALSE; /* unfreeze now that we can paint */
	
}

static void
gnome_icon_list_unmap (GtkWidget *widget)
{
	GnomeIconList *ilist;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (widget));

	if (!GTK_WIDGET_MAPPED (widget))
		return;

	ilist = GNOME_ICON_LIST (widget);

	GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

	gdk_window_hide (ilist->ilist_window);
	gdk_window_hide (widget->window);

	if (GTK_WIDGET_MAPPED (ilist->vscrollbar))
		gtk_widget_unmap (ilist->vscrollbar);

	if (GTK_WIDGET_MAPPED (ilist->hscrollbar))
		gtk_widget_unmap (ilist->hscrollbar);

	ilist->frozen = TRUE; /* freeze it; we cannot paint */
}

static void
gnome_icon_list_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GnomeIconList *ilist;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (widget));
	g_return_if_fail (requisition != NULL);
	ilist = GNOME_ICON_LIST (widget);

	/* Start with frame border */

	requisition->width = 2 * widget->style->klass->xthickness;
	requisition->height = 2 * widget->style->klass->ythickness;

	/* Vscrollbar */

	if ((ilist->vscrollbar_policy == GTK_POLICY_AUTOMATIC)
	    || GTK_WIDGET_VISIBLE (ilist->vscrollbar)) {
		gtk_widget_size_request (ilist->vscrollbar, &ilist->vscrollbar->requisition);

		ilist->vscrollbar_width = ilist->vscrollbar->requisition.width + SCROLLBAR_SPACING;
		requisition->width += ilist->vscrollbar_width;
		requisition->height = MAX (requisition->height, ilist->vscrollbar->requisition.height);
	} else
		ilist->vscrollbar_width = 0;

	/* Hscrollbar */

	if ((ilist->hscrollbar_policy == GTK_POLICY_AUTOMATIC)
	    || GTK_WIDGET_VISIBLE (ilist->hscrollbar)) {
		gtk_widget_size_request (ilist->hscrollbar, &ilist->hscrollbar->requisition);

		requisition->width = MAX (ilist->hscrollbar->requisition.width,
					  requisition->width - ilist->vscrollbar->requisition.width);
		ilist->hscrollbar_height = ilist->hscrollbar->requisition.height + SCROLLBAR_SPACING;
		requisition->height += ilist->hscrollbar_height;
	} else
		ilist->hscrollbar_height = 0;

	/* Container border */

	requisition->width += 2 * GTK_CONTAINER (widget)->border_width;
	requisition->height += 2 * GTK_CONTAINER (widget)->border_width;
}

static void
adjust_scrollbars (GnomeIconList *ilist)
{
	GtkAdjustment *adj;
	int ilist_height, ilist_width;

	/* Recalculate icon rows and columns */

	ilist->icon_cols = ((ilist->ilist_window_width + ilist->col_spacing)
			    / (ilist->max_icon_width + ilist->col_spacing));

	if (ilist->icon_cols > 0)
		ilist->icon_rows = (ilist->icons + ilist->icon_cols - 1) / ilist->icon_cols;
	else
		ilist->icon_rows = 0;

	ilist_height = ILIST_HEIGHT (ilist);
	ilist_width = ILIST_WIDTH (ilist);

	/* Vertical adjustment */

	adj = GTK_RANGE (ilist->vscrollbar)->adjustment;

	adj->page_size      = ilist->ilist_window_height;
	adj->page_increment = ilist->ilist_window_height / 2;
	adj->step_increment = (ilist->max_icon_height + ilist->row_spacing) / 2;
	adj->lower          = 0;
	adj->upper          = ilist_height;

	if ((ilist->ilist_window_height - ilist->y_offset) > ilist_height) {
		adj->value = ilist_height - ilist->ilist_window_height;
		gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
	};

	/* Horizontal adjustment */

	adj = GTK_RANGE (ilist->hscrollbar)->adjustment;

	adj->page_size      = ilist->ilist_window_width;
	adj->page_increment = ilist->ilist_window_width / 2;
	adj->step_increment = (ilist->max_icon_width + ilist->col_spacing) / 2;
	adj->lower          = 0;
	adj->upper          = ilist_width;

	if ((ilist->ilist_window_width - ilist->x_offset) > ilist_width) {
		adj->value = ilist_width - ilist->ilist_window_width;
		gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
	}

	/* Adjust vertical scrollbar */

	if ((ilist_height <= ilist->ilist_window_height)
	    && (ilist->vscrollbar_policy == GTK_POLICY_AUTOMATIC)) {
		if (GTK_WIDGET_VISIBLE (ilist->vscrollbar)) {
			gtk_widget_hide (ilist->vscrollbar);
			gtk_widget_queue_resize (GTK_WIDGET (ilist));
		}
	} else
		if (!GTK_WIDGET_VISIBLE (ilist->vscrollbar)) {
			gtk_widget_show (ilist->vscrollbar);
			gtk_widget_queue_resize (GTK_WIDGET (ilist));
		}

	/* Adjust horizontal scrollbar */

	if ((ilist_width <= ilist->ilist_window_width)
	    && (ilist->hscrollbar_policy == GTK_POLICY_AUTOMATIC)) {
		if (GTK_WIDGET_VISIBLE (ilist->hscrollbar)) {
			gtk_widget_hide (ilist->hscrollbar);
			gtk_widget_queue_resize (GTK_WIDGET (ilist));
		}
	} else
		if (!GTK_WIDGET_VISIBLE (ilist->hscrollbar)) {
			gtk_widget_show (ilist->hscrollbar);
			gtk_widget_queue_resize (GTK_WIDGET (ilist));
		}

	/* Shout at the world */

	gtk_signal_emit_by_name (GTK_OBJECT (GTK_RANGE (ilist->vscrollbar)->adjustment), "changed");
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_RANGE (ilist->hscrollbar)->adjustment), "changed");
}

static void
gnome_icon_list_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GnomeIconList *ilist;
	int border_width;
	GtkAllocation sb_alloc;
	int l_width, l_height;
	int icon_cols, icon_rows;
	int show_vsb, show_hsb;
	int need_x, need_y;
	int i;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (widget));
	g_return_if_fail (allocation != NULL);

	ilist = GNOME_ICON_LIST (widget);

	widget->allocation = *allocation;

	if (GTK_WIDGET_REALIZED (widget)) {
		border_width = GTK_CONTAINER (widget)->border_width;

		/* Decide whether to show scrollbars, and calculate list window size -- done in two passes */

		show_vsb = FALSE;
		show_hsb = FALSE;

		l_width = allocation->width - (2 * border_width + 2 * widget->style->klass->xthickness);
		l_height = allocation->height - (2 * border_width + 2 * widget->style->klass->ythickness);

		for (i = 0; i < 2; i++) {
			/* Calculate needed height */

			if (ilist->icons > 0) {
				icon_cols = (l_width + ilist->col_spacing) / (ilist->max_icon_width + ilist->col_spacing);

				if (icon_cols < 1)
					icon_cols = 1;

				icon_rows = (ilist->icons + icon_cols - 1) / icon_cols;

				need_y = icon_rows * (ilist->max_icon_height + ilist->row_spacing) - ilist->row_spacing;
			} else {
				need_y = 0;
				icon_cols = 0;
			}

			/* Set scrollbars */

			if ((need_y <= l_height) && (ilist->vscrollbar_policy == GTK_POLICY_AUTOMATIC))
				show_vsb = FALSE;
			else
				if (!show_vsb) {
					show_vsb = TRUE;
					l_width -= ilist->vscrollbar_width;
				}

			need_x = icon_cols * (ilist->max_icon_width + ilist->col_spacing) - ilist->col_spacing;

			if ((need_x <= l_width) && (ilist->hscrollbar_policy == GTK_POLICY_AUTOMATIC))
				show_hsb = FALSE;
			else
				if (!show_hsb) {
					show_hsb = TRUE;
					l_height -= ilist->hscrollbar_height;
				}
		}

		/* Allocate main window */

		gdk_window_move_resize (widget->window,
					allocation->x + border_width,
					allocation->y + border_width,
					allocation->width - 2 * border_width,
					allocation->height - 2 * border_width);

		/* Allocate list window */

		gdk_window_move_resize (ilist->ilist_window,
					widget->style->klass->xthickness,
					widget->style->klass->ythickness,
					l_width,
					l_height);

		ilist->ilist_window_width = l_width;
		ilist->ilist_window_height = l_height;

		/* Allocate scrollbars */

		adjust_scrollbars (ilist);

		if (show_vsb) {
			if (!GTK_WIDGET_VISIBLE (ilist->vscrollbar))
				gtk_widget_show (ilist->vscrollbar);

			sb_alloc.x = allocation->width - (2 * border_width + ilist->vscrollbar->requisition.width);
			sb_alloc.y = 0;
			sb_alloc.width = ilist->vscrollbar->requisition.width;
			sb_alloc.height = ((allocation->height - 2 * border_width)
					   - (show_hsb ? ilist->hscrollbar_height : 0));

			gtk_widget_size_allocate (ilist->vscrollbar, &sb_alloc);
		} else if (GTK_WIDGET_VISIBLE (ilist->vscrollbar))
			gtk_widget_hide (ilist->vscrollbar);

		if (show_hsb) {
			if (!GTK_WIDGET_VISIBLE (ilist->hscrollbar))
				gtk_widget_show (ilist->hscrollbar);

			sb_alloc.x = 0;
			sb_alloc.y = allocation->height - (2 * border_width + ilist->hscrollbar->requisition.height);
			sb_alloc.width = ((allocation->width - 2 * border_width)
					  - (show_vsb ? ilist->vscrollbar_width : 0));
			sb_alloc.height = ilist->hscrollbar->requisition.height;

			gtk_widget_size_allocate (ilist->hscrollbar, &sb_alloc);
		} else if (GTK_WIDGET_VISIBLE (ilist->hscrollbar))
			gtk_widget_hide (ilist->hscrollbar);
	}

	adjust_scrollbars (ilist);
}

static void
gnome_icon_list_draw (GtkWidget *widget, GdkRectangle *area)
{
	GnomeIconList *ilist;
	int border_width;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (widget));
	g_return_if_fail (area != NULL);

	if (!GTK_WIDGET_DRAWABLE (widget))
		return;

	ilist = GNOME_ICON_LIST (widget);
	border_width = GTK_CONTAINER (widget)->border_width;

	gdk_window_clear_area (widget->window,
			       area->x - border_width,
			       area->y - border_width,
			       area->width, area->height);

	gtk_draw_shadow (widget->style, widget->window,
			 GTK_STATE_NORMAL, ilist->border_type,
			 0, 0,
			 ilist->ilist_window_width + 2 * widget->style->klass->xthickness,
			 ilist->ilist_window_height + 2 * widget->style->klass->ythickness);

	area->x -= border_width + widget->style->klass->xthickness;
	area->y -= border_width + widget->style->klass->ythickness;

	draw_icons_area (ilist, area);

	/* FIXME: need to draw scrollbars? */
}

void
gnome_icon_list_unselect_all (GnomeIconList *ilist, GdkEvent *event, void *keep)
{
	int i;
	GList *icon_list = ilist->icon_list;
	Icon *icon;
	
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	for (i = 0; icon_list ; icon_list = icon_list->next, i++){
		icon = icon_list->data;
		
		if (icon != keep && icon->state == GTK_STATE_SELECTED)
			gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[UNSELECT_ICON], i, event);
	}
}

static void
toggle_icon (GnomeIconList *ilist, int num, GdkEvent *event)
{
	gint i, count;
	GList *list;
	Icon *icon, *selected_icon;

	i = 0;
	selected_icon = NULL;

	switch (ilist->selection_mode) {
	case GTK_SELECTION_SINGLE:
		for (list = ilist->icon_list; list; list = list->next) {
			icon = list->data;

			if (i == num)
				selected_icon = icon;
			else if ( (icon->state == GTK_STATE_SELECTED) &&
				  (event->button.button == 1) )
				gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[UNSELECT_ICON],
						 i, event);

			i++;
		}

		if ( selected_icon && (selected_icon->state == GTK_STATE_SELECTED) && (event->button.button == 1) )
			gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[UNSELECT_ICON],
					 num, event);
		else
			gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[SELECT_ICON],
					 num, event);

		break;

	case GTK_SELECTION_BROWSE:
		for (list = ilist->icon_list; list; list = list->next) {
			icon = list->data;

			if ((i != num) && (icon->state == GTK_STATE_SELECTED) && (event->button.button == 1) )
				gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[UNSELECT_ICON],
						 i, event);

			i++;
		}

		gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[SELECT_ICON],
				 num, event);

		break;

	case GTK_SELECTION_MULTIPLE:
		/* If neither the shift or control keys are pressed, clear the selection */
		if (!(event->button.state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK))){
			Icon *clicked_icon;

			clicked_icon = g_list_nth (ilist->icon_list, num)->data;

			if( (event->button.button == 1 ) || (clicked_icon->state != GTK_STATE_SELECTED) )
				gnome_icon_list_unselect_all (ilist, event, clicked_icon);

			gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[SELECT_ICON], num, event);
			ilist->last_selected = num;
			break;
		}
		if ((event->button.state & GDK_SHIFT_MASK) && (event->button.button == 1)) {
			int first, last, pos;

			if (ilist->last_selected < num){
				first = ilist->last_selected;
				last  = num;
			} else {
				first = num;
				last  = ilist->last_selected;
			}

			count = last - first + 1;
			list = g_list_nth (ilist->icon_list, first);
			pos = first;
			while (count--){
				icon = list->data;

				if (icon->state != GTK_STATE_SELECTED)
					gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[SELECT_ICON], pos, event);

				list = list->next;
				pos++;
			}
		} else if ((event->button.state & GDK_CONTROL_MASK) && (event->button.button == 1)) {
			int signal;
			
			icon = g_list_nth (ilist->icon_list, num)->data;
			signal = (icon->state == GTK_STATE_SELECTED) ? UNSELECT_ICON : SELECT_ICON;
			gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[signal], num, event);
		}
		ilist->last_selected = num;
		break;

	case GTK_SELECTION_EXTENDED:
		break;

	default:
		break;
	}
}

static gint
gnome_icon_list_button_release (GtkWidget *widget, GdkEventButton *event)
{
	GnomeIconList *ilist;
	
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_ICON_LIST (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	ilist = GNOME_ICON_LIST (widget);

	if (ilist->last_clicked == -1)
		return FALSE;
			
	if (event->window != ilist->ilist_window)
		return FALSE;

	toggle_icon (ilist, ilist->last_clicked, (GdkEvent *) event);
	return FALSE;
}

static gint
gnome_icon_list_button_press (GtkWidget *widget, GdkEventButton *event)
{
	GnomeIconList *ilist;
	int num, on_spacing;
	Icon *icon;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_ICON_LIST (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	ilist = GNOME_ICON_LIST (widget);

	if (event->window != ilist->ilist_window)
		return FALSE;

	get_icon_num_from_xy (ilist, event->x, event->y, &num, &on_spacing);

	if (on_spacing)
		return FALSE;

	icon = g_list_nth (ilist->icon_list, num)->data;
	if (icon->state == GTK_STATE_SELECTED && event->type != GDK_2BUTTON_PRESS && (event->button == 1) ){
		ilist->last_clicked = num;
		return FALSE;
	} else {
		ilist->last_clicked = -1;
		toggle_icon (ilist, num, (GdkEvent *) event);
	}
	
	return FALSE;
}

static gint
gnome_icon_list_expose (GtkWidget *widget, GdkEventExpose *event)
{
	GnomeIconList *ilist;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_ICON_LIST (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (!GTK_WIDGET_DRAWABLE (widget))
		return FALSE;

	ilist = GNOME_ICON_LIST (widget);

	if (event->window == widget->window) {
		gtk_draw_shadow (widget->style, widget->window,
				 GTK_STATE_NORMAL, ilist->border_type,
				 0, 0,
				 ilist->ilist_window_width + 2 * widget->style->klass->xthickness,
				 ilist->ilist_window_height + 2 * widget->style->klass->ythickness);
	} else if (event->window == ilist->ilist_window)
		draw_icons_area (ilist, &event->area);

	return FALSE;
}

static void
gnome_icon_list_foreach (GtkContainer *container, GtkCallback callback, gpointer callback_data)
{
	GnomeIconList *ilist;

	g_return_if_fail (container != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (container));
	g_return_if_fail (callback != NULL);

	ilist = GNOME_ICON_LIST (container);

	if (ilist->vscrollbar)
		(*callback) (ilist->vscrollbar, callback_data);

	if (ilist->hscrollbar)
		(*callback) (ilist->hscrollbar, callback_data);
}

void
gnome_icon_list_set_selection_mode (GnomeIconList *ilist, GtkSelectionMode mode)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	ilist->selection_mode = mode;
	ilist->last_selected = 0;
}

void
gnome_icon_list_set_policy (GnomeIconList *ilist,
			    GtkPolicyType vscrollbar_policy,
			    GtkPolicyType hscrollbar_policy)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if (ilist->vscrollbar_policy != vscrollbar_policy) {
		ilist->vscrollbar_policy = vscrollbar_policy;

		if (GTK_WIDGET (ilist)->parent)
			gtk_widget_queue_resize (GTK_WIDGET (ilist));
	}

	if (ilist->hscrollbar_policy != hscrollbar_policy) {
		ilist->hscrollbar_policy = hscrollbar_policy;

		if (GTK_WIDGET (ilist)->parent)
			gtk_widget_queue_resize (GTK_WIDGET (ilist));
	}
}


static Icon *
icon_new_from_imlib (GnomeIconList *ilist, GdkImlibImage *im, char *text)
{
	Icon *icon;

	icon = g_new (Icon, 1);

	icon->im = im;

	if (icon->im) {
		gdk_imlib_render (icon->im, icon->im->rgb_width, icon->im->rgb_height);
		icon->pixmap = gdk_imlib_move_image (icon->im);
		icon->mask = gdk_imlib_move_mask (icon->im);
	} else {
		icon->pixmap = NULL;
		icon->mask = NULL;
	}

	icon->text = g_strdup (text);

    	icon->state = GTK_STATE_NORMAL;

	icon->data = NULL;
	icon->destroy = NULL;

	icon->fg_set = FALSE;
	icon->bg_set = FALSE;

	if (GTK_WIDGET_REALIZED (ilist))
		recalc_max_icon_size_1 (ilist, icon);

	return icon;
}

static Icon *
icon_new (GnomeIconList *ilist, char *icon_filename, char *text)
{
	GdkImlibImage *im;

	if (icon_filename)
		im = gdk_imlib_load_image (icon_filename);
	else
		im = NULL;
	return icon_new_from_imlib (ilist, im, text);
}

static void
icon_destroy (GnomeIconList *ilist, Icon *icon)
{
	if (icon->destroy)
		(*icon->destroy) (icon->data);

	if (icon->pixmap)
		gdk_imlib_free_pixmap (icon->pixmap);

	if (icon->mask)
		gdk_imlib_free_bitmap (icon->mask);

	if (icon->im)
		gdk_imlib_destroy_image (icon->im);

	if (icon->text)
		g_free (icon->text);

	if (icon->ti)
		gnome_icon_text_info_free (icon->ti);
	
	g_free (icon);
}

static int
icon_list_append (GnomeIconList *ilist, Icon *icon)
{
	ilist->icons++;
	if (!ilist->icon_list) {
		ilist->icon_list = g_list_append (ilist->icon_list, icon);
		ilist->icon_list_end = ilist->icon_list;

		switch (ilist->selection_mode) {
		case GTK_SELECTION_BROWSE:
			gnome_icon_list_select_icon (ilist, 0);
			break;

		default:
			break;
		}
	} else
		ilist->icon_list_end = g_list_append (ilist->icon_list_end, icon)->next;

	if (!ilist->frozen) {
		adjust_scrollbars (ilist);
		draw_icons_area (ilist, NULL);
	} else
		ilist->dirty = TRUE;

	return ilist->icons - 1;
}

int
gnome_icon_list_append_imlib (GnomeIconList *ilist, GdkImlibImage *im, char *text)
{
	Icon *icon;

	g_return_val_if_fail (ilist != NULL, -1);
	g_return_val_if_fail (GNOME_IS_ICON_LIST (ilist), -1);

	icon = icon_new_from_imlib (ilist, im, text);
	return icon_list_append (ilist, icon);
}

int
gnome_icon_list_append (GnomeIconList *ilist, char *icon_filename, char *text)
{
	Icon *icon;

	g_return_val_if_fail (ilist != NULL, -1);
	g_return_val_if_fail (GNOME_IS_ICON_LIST (ilist), -1);

	icon = icon_new (ilist, icon_filename, text);
	return icon_list_append (ilist, icon);
}

static void
sync_selection (GnomeIconList *ilist, int pos, SyncType type)
{
	GList *list;

	for (list = ilist->icon_list; list; list = list->next) {
		if (GPOINTER_TO_INT (list->data) >= pos)
			switch (type) {
			case SYNC_INSERT:
				list->data = GINT_TO_POINTER (list->data) + 1;
				break;

			case SYNC_REMOVE:
				list->data = GINT_TO_POINTER (list->data) - 1;
				break;

			default:
				g_assert_not_reached ();
			}
	}
}

void
gnome_icon_list_insert (GnomeIconList *ilist, int pos, char *icon_filename, char *text)
{
	Icon *icon;

	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if ((pos < 0) || (pos > ilist->icons))
		return;

	if (ilist->icons == 0)
		gnome_icon_list_append (ilist, icon_filename, text);
	else {
		icon = icon_new (ilist, icon_filename, text);

		if (pos == ilist->icons)
			ilist->icon_list_end = g_list_append (ilist->icon_list_end, icon)->next;
		else
			ilist->icon_list = g_list_insert (ilist->icon_list, icon, pos);

		ilist->icons++;

		sync_selection (ilist, pos, SYNC_INSERT);
	}

	if (!ilist->frozen) {
		adjust_scrollbars (ilist);
		draw_icons_area (ilist, NULL);
	} else
		ilist->dirty = TRUE;
}

void
gnome_icon_list_remove (GnomeIconList *ilist, int pos)
{
	int was_selected;
	GList *list;
	Icon *icon;

	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if ((pos < 0) || (pos >= ilist->icons))
		return;

	was_selected = FALSE;

	list = g_list_nth (ilist->icon_list, pos);
	icon = list->data;

	if (icon->state == GTK_STATE_SELECTED) {
		was_selected = TRUE;

		switch (ilist->selection_mode) {
		case GTK_SELECTION_SINGLE:
		case GTK_SELECTION_BROWSE:
		case GTK_SELECTION_MULTIPLE:
			gnome_icon_list_unselect_icon (ilist, pos);
			break;

		default:
			break;
		}
	}

	if (pos == (ilist->icons - 1))
		ilist->icon_list_end = list->prev;

	ilist->icon_list = g_list_remove_link (ilist->icon_list, list);
	ilist->icons--;

	sync_selection (ilist, pos, SYNC_REMOVE);

	if (was_selected) {
		switch (ilist->selection_mode) {
		case GTK_SELECTION_BROWSE:
			if (pos == ilist->icons)
				gnome_icon_list_select_icon (ilist, pos - 1);
			else
				gnome_icon_list_select_icon (ilist, pos);

			break;

		default:
			break;
		}
	}

	icon_destroy (ilist, icon);

	if (GTK_WIDGET_REALIZED (ilist))
		recalc_max_icon_size (ilist);

	if (!ilist->frozen) {
		adjust_scrollbars (ilist);
		draw_icons_area (ilist, NULL);
	} else
		ilist->dirty = TRUE;
}

void
gnome_icon_list_clear (GnomeIconList *ilist)
{
	GList *list;

	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	for (list = ilist->icon_list; list; list = list->next)
		icon_destroy (ilist, list->data);

	g_list_free (ilist->icon_list);
	g_list_free (ilist->selection);

	ilist->icon_list = NULL;
	ilist->icon_list_end = NULL;
	ilist->selection = NULL;
	ilist->last_selected = 0;
	ilist->icons = 0;

	if (GTK_WIDGET_REALIZED (ilist))
		recalc_max_icon_size (ilist);

	if (ilist->vscrollbar) {
		GTK_RANGE (ilist->vscrollbar)->adjustment->value = 0.0;
		gtk_signal_emit_by_name (GTK_OBJECT (GTK_RANGE (ilist->vscrollbar)->adjustment), "changed");

		if (!ilist->frozen) {
			adjust_scrollbars (ilist);
			draw_icons_area (ilist, NULL);
		} else
			ilist->dirty = TRUE;
	}
}

void
gnome_icon_list_set_icon_data (GnomeIconList *ilist, int pos, gpointer data)
{
	gnome_icon_list_set_icon_data_full (ilist, pos, data, NULL);
}

void
gnome_icon_list_set_icon_data_full (GnomeIconList *ilist, int pos, gpointer data, GtkDestroyNotify destroy)
{
	Icon *icon;

	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if ((pos < 0) || (pos >= ilist->icons))
		return;

	icon = g_list_nth (ilist->icon_list, pos)->data;
	icon->data = data;
	icon->destroy = destroy;

	/* This is the same comment that appears in GtkCList - Federico */
	/* re-send the selected signal if data is changed/added
	 * so the application can respond to the new data -- 
	 * this could be questionable behavior */

	if (icon->state == GTK_STATE_SELECTED)
		gnome_icon_list_select_icon (ilist, pos);
}

gpointer
gnome_icon_list_get_icon_data (GnomeIconList *ilist, int pos)
{
	Icon *icon;

	g_return_val_if_fail (ilist != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_LIST (ilist), NULL);

	if ((pos < 0) || (pos >= ilist->icons))
		return NULL;

	icon = g_list_nth (ilist->icon_list, pos)->data;
	return icon->data;
}

int
gnome_icon_list_find_icon_from_data (GnomeIconList *ilist, gpointer data)
{
	GList *list;
	int n;
	Icon *icon;

	g_return_val_if_fail (ilist != NULL, -1);
	g_return_val_if_fail (GNOME_IS_ICON_LIST (ilist), -1);

	for (n = 0, list = ilist->icon_list; list; n++, list = list->next) {
		icon = list->data;
		if (icon->data == data)
			return n;
	}

	return -1;
}

static void
select_icon (GnomeIconList *ilist, int pos, GdkEvent *event)
{
	gint i;
	GList *list;
	Icon *icon;

	switch (ilist->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
		i = 0;

		for (list = ilist->icon_list; list; list = list->next) {
			icon = list->data;

			if ((i != pos) && (icon->state == GTK_STATE_SELECTED))
				gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[UNSELECT_ICON],
						 i, event);

			i++;
		}

		gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[SELECT_ICON],
				 pos, event);

		break;

	case GTK_SELECTION_MULTIPLE:
		gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[SELECT_ICON],
				 pos, event);
		break;

	case GTK_SELECTION_EXTENDED:
		break;

	default:
		break;
	}
}

void
gnome_icon_list_select_icon (GnomeIconList *ilist, int pos)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if ((pos < 0) || (pos >= ilist->icons))
		return;

	select_icon (ilist, pos, NULL);
}

static void
unselect_icon (GnomeIconList *ilist, int pos, GdkEvent *event)
{
	switch (ilist->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
	case GTK_SELECTION_MULTIPLE:
		gtk_signal_emit (GTK_OBJECT (ilist), ilist_signals[UNSELECT_ICON],
				 pos, event);
		break;

	case GTK_SELECTION_EXTENDED:
		break;

	default:
		break;
	}
}

void
gnome_icon_list_unselect_icon (GnomeIconList *ilist, int pos)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if ((pos < 0) || (pos >= ilist->icons))
		return;

	unselect_icon (ilist, pos, NULL);
}

void
gnome_icon_list_freeze (GnomeIconList *ilist)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	ilist->frozen = TRUE;
}

void
gnome_icon_list_thaw (GnomeIconList *ilist)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	ilist->frozen = FALSE;

	if (ilist->dirty){
		adjust_scrollbars (ilist);
		draw_icons_area (ilist, NULL);
		ilist->dirty = FALSE;
	}
}

void
gnome_icon_list_moveto (GnomeIconList *ilist, int pos, double yalign)
{
	int xpos, ypos;
	int y;
	GtkAdjustment *adj;

	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if ((pos < 0) || (pos >= ilist->icons))
		return;

	yalign = CLAMP (yalign, 0.0, 1.0);

	get_icon_xy_from_num (ilist, pos, &xpos, &ypos);

	ypos += ilist->y_offset;

	y = ypos - yalign * (ilist->ilist_window_height - ilist->max_icon_height);

	adj = GTK_RANGE (ilist->vscrollbar)->adjustment;

	if (y < 0)
		adj->value = 0.0;
	else if (y > (ILIST_HEIGHT (ilist) - ilist->ilist_window_height))
		adj->value = ILIST_HEIGHT (ilist) - ilist->ilist_window_height;
	else
		adj->value = y;

	gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
}

GtkVisibility
gnome_icon_list_icon_is_visible (GnomeIconList *ilist, int pos)
{
	int x, y;
	GdkRectangle iarea, warea, darea;

	g_return_val_if_fail (ilist != NULL, GTK_VISIBILITY_NONE);
	g_return_val_if_fail (GNOME_IS_ICON_LIST (ilist), GTK_VISIBILITY_NONE);

	get_icon_xy_from_num (ilist, pos, &x, &y);

	iarea.x = x;
	iarea.y = y;
	iarea.width = ilist->max_icon_width;
	iarea.height = ilist->max_icon_height;

	warea.x = 0;
	warea.y = 0;
	warea.width = ilist->ilist_window_width;
	warea.height = ilist->ilist_window_height;

	if (!gdk_rectangle_intersect (&iarea, &warea, &darea))
		return GTK_VISIBILITY_NONE;
	else if ((darea.width != ilist->max_icon_width) || (darea.height != ilist->max_icon_height))
		return GTK_VISIBILITY_PARTIAL;
	else
		return GTK_VISIBILITY_FULL;
}

void
gnome_icon_list_set_foreground (GnomeIconList *ilist, int pos, GdkColor *color)
{
	Icon *icon;

	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if ((pos < 0) || (pos >= ilist->icons))
		return;

	icon = g_list_nth (ilist->icon_list, pos)->data;

	if (color) {
		icon->foreground = *color;
		icon->fg_set = TRUE;
	} else
		icon->fg_set = FALSE;

	if (!ilist->frozen)
		draw_icon_by_num (ilist, icon, pos);
	else
		ilist->dirty = 1;
}

void
gnome_icon_list_set_background (GnomeIconList *ilist, int pos, GdkColor *color)
{
	Icon *icon;

	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if ((pos < 0) || (pos >= ilist->icons))
		return;

	icon = g_list_nth (ilist->icon_list, pos)->data;

	if (color) {
		icon->background = *color;
		icon->bg_set = TRUE;
	} else
		icon->bg_set = FALSE;

	if (!ilist->frozen)
		draw_icon_by_num (ilist, icon, pos);
	else
		ilist->dirty = 1;
}

void
gnome_icon_list_set_row_spacing (GnomeIconList *ilist, int spacing)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));
	g_return_if_fail (spacing >= 0);

	ilist->row_spacing = spacing;

	if (!ilist->frozen) {
		adjust_scrollbars (ilist);
		draw_icons_area (ilist, NULL);
	} else
		ilist->dirty = TRUE;
}

void
gnome_icon_list_set_col_spacing (GnomeIconList *ilist, int spacing)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));
	g_return_if_fail (spacing >= 0);

	ilist->col_spacing = spacing;

	if (!ilist->frozen) {
		adjust_scrollbars (ilist);
		draw_icons_area (ilist, NULL);
	} else
		ilist->dirty = TRUE;
}

void
gnome_icon_list_set_text_spacing (GnomeIconList *ilist, int spacing)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));
	g_return_if_fail (spacing >= 0);

	ilist->text_spacing = spacing;

	if (!ilist->frozen) {
		recalc_max_icon_size (ilist);
		adjust_scrollbars (ilist);
		draw_icons_area (ilist, NULL);
	} else
		ilist->dirty = TRUE;
}

void
gnome_icon_list_set_icon_border (GnomeIconList *ilist, int spacing)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));
	g_return_if_fail (spacing >= 0);

	ilist->icon_border = spacing;

	if (!ilist->frozen) {
		recalc_max_icon_size (ilist);
		adjust_scrollbars (ilist);	
		draw_icons_area (ilist, NULL);
	} else
		ilist->dirty = TRUE;
}

void
gnome_icon_list_set_separators (GnomeIconList *ilist, char *separators)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	g_free (ilist->separators);

	if (separators)
		ilist->separators = g_strdup (separators);
	else
		ilist->separators = g_strdup (" ");

	if (!ilist->frozen) {
		recalc_max_icon_size (ilist);
		adjust_scrollbars (ilist);	
		draw_icons_area (ilist, NULL);
	} else
		ilist->dirty = TRUE;
}

void
gnome_icon_list_set_mode (GnomeIconList *ilist, GnomeIconListMode mode)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if (ilist->mode == mode)
		return;

	ilist->mode = mode;

	if (!ilist->frozen) {
		recalc_max_icon_size (ilist);
		adjust_scrollbars (ilist);
		draw_icons_area (ilist, NULL);
	} else
		ilist->dirty = 1;
}

void
gnome_icon_list_set_border (GnomeIconList *ilist, GtkShadowType border)
{
	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	ilist->border_type = border;

	if (GTK_WIDGET_VISIBLE (ilist))
		gtk_widget_queue_resize (GTK_WIDGET (ilist));
}

int
gnome_icon_list_get_icon_at (GnomeIconList *ilist, int x, int y)
{
	int num, on_spacing;

	g_return_val_if_fail (ilist != NULL, -1);
	g_return_val_if_fail (GNOME_IS_ICON_LIST (ilist), -1);

	if ((x < 0) || (y < 0) || (x >= ilist->ilist_window_width) || (y >= ilist->ilist_window_height))
		return -1;

	get_icon_num_from_xy (ilist, x, y, &num, &on_spacing);

	if ((num == -1) || on_spacing)
		return -1;

	return num;
}

static void
real_select_icon (GnomeIconList *ilist, gint num, GdkEvent *event)
{
	Icon *icon;

	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if ((num < 0) || (num >= ilist->icons))
		return;

	icon = g_list_nth (ilist->icon_list, num)->data;

	if (icon->state == GTK_STATE_NORMAL) {
		icon->state = GTK_STATE_SELECTED;
		ilist->selection = g_list_append (ilist->selection, GINT_TO_POINTER (num));

		if (!ilist->frozen
		    && (gnome_icon_list_icon_is_visible (ilist, num) != GTK_VISIBILITY_NONE))
			draw_icon_by_num (ilist, icon, num);
		else
			ilist->dirty = 1;
	}
}

static void
real_unselect_icon (GnomeIconList *ilist, gint num, GdkEvent *event)
{
	Icon *icon;

	g_return_if_fail (ilist != NULL);
	g_return_if_fail (GNOME_IS_ICON_LIST (ilist));

	if ((num < 0) || (num >= ilist->icons))
		return;

	icon = g_list_nth (ilist->icon_list, num)->data;

	if (icon->state == GTK_STATE_SELECTED) {
		icon->state = GTK_STATE_NORMAL;
		ilist->selection = g_list_remove (ilist->selection, GINT_TO_POINTER (num));

		if (!ilist->frozen
		    && (gnome_icon_list_icon_is_visible (ilist, num) != GTK_VISIBILITY_NONE))
			draw_icon_by_num (ilist, icon, num);
		else
			ilist->dirty = 1;
	}
}

static void
gnome_icon_list_marshal_signal_1 (GtkObject *object, GtkSignalFunc func, gpointer func_data, GtkArg *args)
{
	GnomeIconListSignal1 rfunc;

	rfunc = (GnomeIconListSignal1) func;

	(* rfunc) (object, GTK_VALUE_INT (args[0]), GTK_VALUE_POINTER (args[1]), func_data);
}

static void
free_row (gpointer data, gpointer user_data)
{
	struct gnome_icon_text_info_row *row;

	if (data) {
		row = data;
		g_free (row->text);
		g_free (row);
	}
}

void
gnome_icon_text_info_free (struct gnome_icon_text_info *ti)
{
	g_list_foreach (ti->rows, free_row, NULL);
	g_list_free (ti->rows);
	g_free (ti);
}

struct gnome_icon_text_info *
gnome_icon_layout_text (GdkFont *font, char *text, char *separators, int max_width, int confine)
{
	struct gnome_icon_text_info *ti;
	struct gnome_icon_text_info_row *row;
	char *row_end, *row_text, *break_pos;
	int i, row_width, window_width;
	int len, rlen;
	int sep_pos;

	if (!separators)
		separators = " ";

	ti = g_new (struct gnome_icon_text_info, 1);

	ti->rows = NULL;
	ti->font = font;
	ti->width = 0;
	ti->height = 0;

	ti->baseline_skip = ti->font->ascent + ti->font->descent;

	window_width = 0;

	while (*text) {
		row_end = strchr (text, '\n');
		if (!row_end)
			row_end = strchr (text, '\0');

		len = row_end - text + 1;
		row_text = g_new (char, len);
		memcpy (row_text, text, len - 1);
		row_text[len - 1] = '\0';

		/* Adjust the window's width or shorten the row until
		 * it fits in the window.
		 */

		while (1) {
			row_width = gdk_string_width (ti->font, row_text);
			if (!window_width) {
				/* make an initial guess at window's width */

				if (row_width < max_width)
					window_width = row_width;
				else
					window_width = max_width;
			}

			if (row_width <= window_width)
				break;

			rlen = strlen (row_text);

			sep_pos = strcspn (row_text, separators);

			if (sep_pos != rlen) {
				/* the row is currently too wide, but we have separators in the row
				 * so we can break it into smaller pieces
				 */

				int avg_width = row_width / rlen;

				i = window_width;

				if (avg_width != 0)
					i /= avg_width;

				if (i >= len)
					i = len - 1;

				break_pos = strtok (row_text + i, separators);
				if (!break_pos) {
					break_pos = row_text + i;
					while (strchr (separators, *--break_pos));
				}

				*break_pos = '\0';
			} else {
				/* We can't break this row into any smaller
				 * pieces, so we have no choice but to widen
				 * the window
				 *
				 * For MC, we may want to modify the code above
				 * so that it can also split the string on the
				 * slahes of filenames.
				 */

				window_width = row_width;
				break;
			}
		}

		if (row_width > ti->width)
			ti->width = row_width;

		row = g_new (struct gnome_icon_text_info_row, 1);
		row->text = row_text;
		row->width = gdk_string_width (ti->font, row_text);

		ti->rows = g_list_append (ti->rows, row);
		ti->height += ti->baseline_skip;

		text += strlen (row_text);
		if (!*text)
			break;

		if (text[0] == '\n' && text[1]) {
			/* end of paragraph and there is more text to come */
			ti->rows = g_list_append (ti->rows, NULL);
			ti->height += ti->baseline_skip / 2;
		}

		text++; /* skip blank or newline */
	}

	return ti;
}

void
gnome_icon_paint_text (struct gnome_icon_text_info *ti, GdkDrawable *drawable, GdkGC *gc,
		       int x_ofs, int y_ofs, int width)
{
	int y;
	GList *item;
	struct gnome_icon_text_info_row *row;

	y = y_ofs + ti->font->ascent;

	for (item = ti->rows; item; item = item->next) {
		if (item->data) {
			row = item->data;
			gdk_draw_string (drawable, ti->font, gc, x_ofs + (width - row->width) / 2, y, row->text);
			y += ti->baseline_skip;
		} else
			y += ti->baseline_skip / 2;
	}
}
