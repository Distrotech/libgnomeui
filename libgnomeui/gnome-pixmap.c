/* GNOME GUI Library
 * Copyright (C) 1997, 1998 the Free Software Foundation
 *
 * Authors: Miguel de Icaza
 *          Federico Mena
 */

#include <config.h>

#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gdk_imlib.h>
#include "libgnome/gnome-defs.h"
#include <gtk/gtk.h>                  /* These two includes should be remove once everyting */
#include "libgnome/libgnomeP.h"        /* is switched to use the GnomePixmap widget.         */
#include "gnome-pixmap.h"


static void gnome_pixmap_class_init    (GnomePixmapClass *class);
static void gnome_pixmap_init          (GnomePixmap      *gpixmap);
static void gnome_pixmap_destroy       (GtkObject        *object);
static void gnome_pixmap_realize       (GtkWidget        *widget);
static void gnome_pixmap_size_request  (GtkWidget        *widget,
					GtkRequisition   *requisition);
static void gnome_pixmap_size_allocate (GtkWidget        *widget,
					GtkAllocation    *allocation);
static void gnome_pixmap_draw          (GtkWidget        *widget,
					GdkRectangle     *area);
static gint gnome_pixmap_expose        (GtkWidget        *widget,
					GdkEventExpose   *event);
static void setup_window_and_style     (GnomePixmap *gpixmap);


static GtkWidgetClass *parent_class;

/**
 * gnome_pixmap_get_type:
 *
 * Returns: the GtkType for the GnomePixmap object
 */
guint
gnome_pixmap_get_type (void)
{
	static guint pixmap_type = 0;

	if (!pixmap_type) {
		GtkTypeInfo pixmap_info = {
			"GnomePixmap",
			sizeof (GnomePixmap),
			sizeof (GnomePixmapClass),
			(GtkClassInitFunc) gnome_pixmap_class_init,
			(GtkObjectInitFunc) gnome_pixmap_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		pixmap_type = gtk_type_unique (gtk_widget_get_type (), &pixmap_info);
	}

	return pixmap_type;
}

static void
gnome_pixmap_init (GnomePixmap *gpixmap)
{
	gpixmap->pixmap = NULL;
	gpixmap->mask = NULL;
}

static void
gnome_pixmap_class_init (GnomePixmapClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	parent_class = gtk_type_class (gtk_widget_get_type ());

	widget_class->realize = gnome_pixmap_realize;
	widget_class->size_request = gnome_pixmap_size_request;
	widget_class->size_allocate = gnome_pixmap_size_allocate;
	widget_class->draw = gnome_pixmap_draw;
	widget_class->expose_event = gnome_pixmap_expose;

	object_class->destroy = gnome_pixmap_destroy;
}

static void
free_pixmap_and_mask (GnomePixmap *gpixmap)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));

	if (gpixmap->pixmap) {
		gdk_pixmap_unref (gpixmap->pixmap);
		gpixmap->pixmap = NULL;
	}

	if (gpixmap->mask) {
		gdk_pixmap_unref (gpixmap->mask);
		gpixmap->mask = NULL;
	}
}

static void
gnome_pixmap_destroy (GtkObject *object)
{
	GnomePixmap *gpixmap;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (object));

	gpixmap = GNOME_PIXMAP (object);

	free_pixmap_and_mask (gpixmap);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/**
 * gnome_pixmap_new_from_file:
 * @filename: The name of a file containing a graphics image
 *
 * Description: Returns a widget that contains the image, or %NULL
 * if it fails to load the image.
 *
 * Returns: A new #GnomePixmap widget or %NULL
 */
GtkWidget *
gnome_pixmap_new_from_file (const char *filename)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail(filename != NULL, NULL);

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_file (gpixmap, filename);

	return GTK_WIDGET (gpixmap);
}

/**
 * gnome_pixmap_new_from_file_at_size:
 * @filename: The name of a file containing a graphics image
 * @width: desired width
 * @height: desired height.
 *
 * Description: Returns a widget that contains the image scaled to
 * @width by @height pixels, or %NULL if it fails to load the image.
 *
 * Returns: A new #GnomePixmap widget or %NULL
 */
GtkWidget *
gnome_pixmap_new_from_file_at_size (const char *filename, int width, int height)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail(filename != NULL, NULL);

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_file_at_size (gpixmap, filename, width, height);

	return GTK_WIDGET (gpixmap);
}

/**
 * gnome_pixmap_new_from_xpm_d:
 * @xpm_data: A pointer to an inlined xpm image.
 *
 * Description: Returns a widget that contains the image, or %NULL
 * if it fails to load the image.
 *
 * Returns: A new #GnomePixmap widget or %NULL
 */
GtkWidget *
gnome_pixmap_new_from_xpm_d (char **xpm_data)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail(xpm_data != NULL, NULL);

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_xpm_d (gpixmap, xpm_data);

	return GTK_WIDGET (gpixmap);
}

/**
 * gnome_pixmap_new_from_xpm_d_at_size:
 * @xpm_data: A pointer to an inlined xpm image.
 * @width: desired widht
 * @height: desired height.
 *
 * Description: Returns a widget that contains the image scaled to
 * @width by @height pixels, or %NULL if it fails to load the image.
 *
 * Returns: A new #GnomePixmap widget or %NULL
 */
GtkWidget *
gnome_pixmap_new_from_xpm_d_at_size (char **xpm_data, int width, int height)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail(xpm_data != NULL, NULL);

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_xpm_d_at_size (gpixmap, xpm_data, width, height);

	return GTK_WIDGET (gpixmap);
}

/**
 * gnome_pixmap_new_from_rgb_d:
 * @data: A pointer to an inlined rgb image.
 *
 * Description: Returns a widget that contains the image, or %NULL
 * if it fails to load the image.
 *
 * Returns: A new #GnomePixmap widget or %NULL
 */
GtkWidget *
gnome_pixmap_new_from_rgb_d (unsigned char *data, unsigned char *alpha,
			     int rgb_width, int rgb_height)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail(data != NULL, NULL);

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_rgb_d (gpixmap, data, alpha,
				 rgb_width, rgb_height);

	return GTK_WIDGET (gpixmap);
}

/**
 * gnome_pixmap_new_from_rgb_d_shaped:
 * @data: A pointer to an inlined rgb image
 * @alpha: pointer to the alpha channel.
 * @rgb_width: width of the rgb data
 * @rgb_height: height of the rgb data.
 * @shape_color: which color encodes the transparency
 *
 * Description: Returns a widget that contains the image, or %NULL
 * if it fails to load the image.
 *
 * Returns: A new #GnomePixmap widget or %NULL
 */
GtkWidget *
gnome_pixmap_new_from_rgb_d_shaped (unsigned char *data, unsigned char *alpha,
				    int rgb_width, int rgb_height,
				    GdkImlibColor *shape_color)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail(data != NULL, NULL);

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_rgb_d_shaped (gpixmap, data, alpha,
					rgb_width, rgb_height,
					shape_color);

	return GTK_WIDGET (gpixmap);
}

/**
 * gnome_pixmap_new_from_rgb_d_at_size:
 * @data: A pointer to an inlined rgb image.
 * @alpha:
 * @rgb_width: the width of the rgb image.
 * @rgb_height: the height of the rgb image.
 * @width: desired width.
 * @height: desired height.
 *
 * Description: Returns a widget that contains the image scaled to
 * @width by @height pixels, or %NULL if it fails to load the image.
 *
 * Returns: A new #GnomePixmap widget or %NULL
 */
GtkWidget *
gnome_pixmap_new_from_rgb_d_at_size (unsigned char *data, unsigned char *alpha,
				     int rgb_width, int rgb_height,
				     int width, int height)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail(data != NULL, NULL);

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_rgb_d_at_size (gpixmap, data, alpha,
					rgb_width, rgb_height,
					width, height);

	return GTK_WIDGET (gpixmap);
}

/**
 * gnome_pixmap_new_from_rgb_d_shaped_at_size:
 * @data: A pointer to an inlined rgb image
 * @alpha: pointer to the alpha channel.
 * @rgb_width: width of the rgb data
 * @rgb_height: height of the rgb data.
 * @shape_color: which color encodes the transparency
 * @width: desired width.
 * @height: desired height.
 *
 * Description: Returns a widget that contains the image scaled to
 * @width by @height pixels, or %NULL if it fails to load the image.
 *
 * Returns: A new #GnomePixmap widget or %NULL
 */
GtkWidget *
gnome_pixmap_new_from_rgb_d_shaped_at_size (unsigned char *data,
					    unsigned char *alpha,
					    int rgb_width, int rgb_height,
					    int width, int height,
					    GdkImlibColor *shape_color)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail(data != NULL, NULL);

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_rgb_d_shaped_at_size (gpixmap, data, alpha,
						rgb_width, rgb_height,
						width, height,
						shape_color);

	return GTK_WIDGET (gpixmap);
}

/**
 * gnome_pixmap_new_from_imlib:
 * @im: A pointer to GdkImlibImage data
 *
 * Description: Returns a widget that contains the image, or %NULL
 * if it fails to load the image. Note that @im will not be
 * rendered after this call.
 *
 * Returns: A new #GnomePixmap widget or %NULL
 */
GtkWidget *
gnome_pixmap_new_from_imlib (GdkImlibImage *im)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail(im != NULL, NULL);

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_imlib (gpixmap, im);

	return GTK_WIDGET (gpixmap);
}

/**
 * gnome_pixmap_new_from_imlib_at_size:
 * @im: A pointer to GdkImlibImage data
 * @width: desired width.
 * @height: desired height.
 *
 * Description: Returns a widget that contains the image scaled to
 * @width by @height pixels, or %NULL if it fails to load the image.
 * Note that @im will not be * rendered after this call.
 *
 * Returns: A new #GnomePixmap widget or %NULL
 */
GtkWidget *
gnome_pixmap_new_from_imlib_at_size (GdkImlibImage *im, int width, int height)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail (im != NULL, NULL);
	g_return_val_if_fail (width > 0, NULL);
	g_return_val_if_fail (height > 0, NULL);

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_imlib_at_size (gpixmap, im, width, height);

	return GTK_WIDGET (gpixmap);
}

/**
 * gnome_pixmap_new_from_gnome_pixmap:
 * @gpixmap_old: Another GnomePixmap widget
 *
 * Description: Returns a widget that contains a copy of @gpixmap_old
 *
 * Returns: A new #GnomePixmap widget
 */
GtkWidget *
gnome_pixmap_new_from_gnome_pixmap (GnomePixmap *gpixmap_old)
{
	GnomePixmap *gpixmap;
	GtkRequisition req;
	GdkVisual *visual;
	GdkGC *gc;

	g_return_val_if_fail(gpixmap_old != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_PIXMAP(gpixmap_old), NULL);

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gtk_widget_size_request (GTK_WIDGET(gpixmap_old), &req);
	if (GTK_WIDGET(gpixmap_old)->window)
		visual = gdk_window_get_visual (GTK_WIDGET(gpixmap_old)->window);
	else
		visual = gdk_imlib_get_visual();
	gpixmap->pixmap = gdk_pixmap_new (gpixmap_old->pixmap,
					  req.width, req.height,
					  visual->depth);
	gc = gdk_gc_new (gpixmap->pixmap);
	gdk_draw_pixmap (gpixmap->pixmap, gc, gpixmap_old->pixmap, 0, 0, 0, 0,
			 req.width, req.height);
	gdk_gc_destroy (gc);
	gpixmap->mask = gdk_pixmap_new (gpixmap_old->mask,
					req.width, req.height, 1);
	gc = gdk_gc_new (gpixmap->mask);
	gdk_draw_pixmap (gpixmap->mask, gc, gpixmap_old->mask, 0, 0, 0, 0,
			 req.width, req.height);
	gdk_gc_destroy (gc);

	return GTK_WIDGET (gpixmap);
}

static void
setup_window_and_style (GnomePixmap *gpixmap)
{
	GdkWindowAttr attributes;
	gint attributes_mask;
	GtkWidget *widget;
	gint w, h;
	GdkVisual *imvisual;
	GdkColormap *imcolormap;
	GdkVisual *visual;
	GdkColormap *colormap;

	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));

	widget = GTK_WIDGET (gpixmap);

	if (widget->window)
		gdk_window_unref (widget->window);
#if 0
	/* FIXME: do we have to detach the style?  Does it matter if we change the window? */
	if (widget->style)
		gtk_style_detach (widget->style);
#endif
	if (gpixmap->pixmap) {
		gdk_window_get_size (gpixmap->pixmap, &w, &h);
	} else {
		w = h = 0;
	}
	imvisual = gdk_imlib_get_visual ();
	imcolormap = gdk_imlib_get_colormap ();
	visual = gtk_widget_get_visual (widget);
	colormap = gtk_widget_get_colormap (widget);
	if(GDK_VISUAL_XVISUAL(imvisual)->visualid !=
	   GDK_VISUAL_XVISUAL(visual)->visualid) {
		GTK_WIDGET_UNSET_FLAGS (widget, GTK_NO_WINDOW);
		attributes.window_type = GDK_WINDOW_CHILD;
		attributes.x = widget->allocation.x + (widget->allocation.width - w) / 2;
		attributes.y = widget->allocation.y + (widget->allocation.height - h) / 2;
		attributes.width = w;
		attributes.height = h;
		attributes.wclass = GDK_INPUT_OUTPUT;
		attributes.visual = gpixmap->pixmap?imvisual:visual;
		attributes.colormap = gpixmap->pixmap?imcolormap:colormap;
		attributes.event_mask = (gtk_widget_get_events (widget)
					 | GDK_EXPOSURE_MASK);

		attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

		widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
						 &attributes, attributes_mask);
		gdk_window_set_user_data (widget->window, widget);

		if (gpixmap->mask)
			gtk_widget_shape_combine_mask (widget, gpixmap->mask, 0, 0);
	} else {
		GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);
		if (widget->parent) {
			widget->window = gtk_widget_get_parent_window (widget);
			gdk_window_ref (widget->window);
		}
	}

	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

static void
gnome_pixmap_realize (GtkWidget *widget)
{
	GnomePixmap *gpixmap;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (widget));

	gpixmap = GNOME_PIXMAP (widget);
	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	setup_window_and_style (gpixmap);
}

static void
gnome_pixmap_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GnomePixmap *gpixmap;
	gint w, h;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (widget));
	g_return_if_fail (requisition != NULL);

	gpixmap = GNOME_PIXMAP (widget);

	if (gpixmap->pixmap)
		gdk_window_get_size (gpixmap->pixmap, &w, &h);
	else
		w = h = 0;

	requisition->width = w;
	requisition->height = h;
}

static void
gnome_pixmap_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (widget));
	g_return_if_fail (allocation != NULL);

	widget->allocation = *allocation;

	if (GTK_WIDGET_REALIZED (widget) &&
	    !(GTK_WIDGET_FLAGS(widget)&GTK_NO_WINDOW)) {
		GtkRequisition req;
		gtk_widget_get_child_requisition (widget, &req);
		gdk_window_move (widget->window,
				 allocation->x + (allocation->width - req.width) / 2,
				 allocation->y + (allocation->height - req.height) / 2);
	}
}

static void
paint (GnomePixmap *gpixmap, GdkRectangle *area)
{
	GtkWidget *widget = GTK_WIDGET(gpixmap);
	if (!gpixmap->pixmap) {
		gdk_window_clear_area (widget->window,
				       area->x, area->y,
				       area->width, area->height);
		return;
	}
	if(!(GTK_WIDGET_FLAGS(widget)&GTK_NO_WINDOW)) {
		gdk_draw_pixmap (widget->window,
				 widget->style->black_gc,
				 gpixmap->pixmap,
				 area->x, area->y,
				 area->x, area->y,
				 area->width, area->height);
	} else {
		int x,y;
		GtkRequisition req;
		gtk_widget_get_child_requisition (widget, &req);
		x = widget->allocation.x + (widget->allocation.width - req.width) / 2;
		y = widget->allocation.y + (widget->allocation.height - req.height) / 2;

		if (gpixmap->mask) {
			gdk_gc_set_clip_mask (widget->style->black_gc, gpixmap->mask);
			gdk_gc_set_clip_origin (widget->style->black_gc, x, y);
		}

		gdk_draw_pixmap (widget->window,
				 widget->style->black_gc,
				 gpixmap->pixmap,
				 0, 0, x, y, -1, -1);

		if (gpixmap->mask) {
			gdk_gc_set_clip_mask (widget->style->black_gc, NULL);
			gdk_gc_set_clip_origin (widget->style->black_gc, 0, 0);
		}
	}
}

static void
gnome_pixmap_draw (GtkWidget *widget, GdkRectangle *area)
{
	GnomePixmap *gpixmap;
	GdkRectangle w_area;
	GdkRectangle p_area;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (widget));
	g_return_if_fail (area != NULL);
	
	if (GTK_WIDGET_DRAWABLE (widget)) {
		gpixmap = GNOME_PIXMAP (widget);

		if(!(GTK_WIDGET_FLAGS(gpixmap)&GTK_NO_WINDOW)) {
			GtkRequisition req;
			gtk_widget_get_child_requisition (widget, &req);
			/* Offset the area because the window does not fill the allocation */
			area->x -= (widget->allocation.width - req.width) / 2;
			area->y -= (widget->allocation.height - req.height) / 2;

			w_area.x = 0;
			w_area.y = 0;
			w_area.width = req.width;
			w_area.height = req.height;

			if (gdk_rectangle_intersect (area, &w_area, &p_area))
				paint (gpixmap, &p_area);
		} else
			paint (gpixmap, area);
	}
}

static gint
gnome_pixmap_expose (GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_PIXMAP (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);
	
	if (GTK_WIDGET_DRAWABLE (widget))
		paint (GNOME_PIXMAP (widget), &event->area);

	return FALSE;
}

static void
finish_load (GnomePixmap *gpixmap, GdkImlibImage *im, int scaled, int width, int height, int destroy)
{
	if (!im)
		return;

	if (scaled)
		gdk_imlib_render (im, width, height);
	else
		gdk_imlib_render (im, im->rgb_width, im->rgb_height);

	gpixmap->pixmap = gdk_imlib_copy_image (im);
	gpixmap->mask = gdk_imlib_copy_mask (im);

	if (destroy)
		gdk_imlib_destroy_image (im);

	if(!(GTK_WIDGET_FLAGS(gpixmap)&GTK_NO_WINDOW)) {
		if (GTK_WIDGET_REALIZED (gpixmap)) {
			if (GTK_WIDGET_MAPPED (gpixmap))
				gdk_window_hide (GTK_WIDGET (gpixmap)->window);
			setup_window_and_style (gpixmap);
		}

		if (GTK_WIDGET_MAPPED (gpixmap))
			gdk_window_show (GTK_WIDGET (gpixmap)->window);
	}

	if (GTK_WIDGET_VISIBLE (gpixmap))
		gtk_widget_queue_resize (GTK_WIDGET (gpixmap));
}

static void
load_file (GnomePixmap *gpixmap, const char *filename, int scaled, int width, int height)
{
	GdkImlibImage *im;

	free_pixmap_and_mask (gpixmap);

	im = gdk_imlib_load_image ((char *)filename);
	finish_load (gpixmap, im, scaled, width, height, 1);
}

static void
load_xpm_d (GnomePixmap *gpixmap, char **xpm_data, int scaled, int width, int height)
{
	GdkImlibImage *im;

	free_pixmap_and_mask (gpixmap);

	im = gdk_imlib_create_image_from_xpm_data (xpm_data);
	finish_load (gpixmap, im, scaled, width, height, 1);
}

static void
load_rgb_d (GnomePixmap *gpixmap, unsigned char *data, unsigned char *alpha,
	    int rgb_width, int rgb_height,
	    int scaled, int width, int height)
{
	GdkImlibImage *im;

	free_pixmap_and_mask (gpixmap);

	im = gdk_imlib_create_image_from_data (data, alpha, rgb_width, rgb_height);
	finish_load (gpixmap, im, scaled, width, height, 1);
}

static void
load_rgb_d_shaped (GnomePixmap *gpixmap, unsigned char *data,
		   unsigned char *alpha,
		   int rgb_width, int rgb_height,
		   int scaled, int width, int height,
		   GdkImlibColor *shape_color)
{
	GdkImlibImage *im;

	free_pixmap_and_mask (gpixmap);

	im = gdk_imlib_create_image_from_data (data, alpha, rgb_width, rgb_height);
	gdk_imlib_set_image_shape (im, shape_color);
	finish_load (gpixmap, im, scaled, width, height, 1);
}

/**
 * gnome_pixmap_load_file:
 * @gpixmap: the #GnomePixmap widget
 * @filename: a new filename
 *
 * Description: Sets the gnome pixmap to image stored
 * in @filename.
 */
void
gnome_pixmap_load_file (GnomePixmap *gpixmap, const char *filename)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (filename != NULL);

	load_file (gpixmap, filename, FALSE, 0, 0);
}

/**
 * gnome_pixmap_load_file_at_size:
 * @gpixmap: the #GnomePixmap widget
 * @filename: a new filename
 * @width: desired width.
 * @height: desired height.
 *
 * Description: Sets the gnome pixmap to image stored in
 * @filename scaled to @width and @height pixels.
 */
void
gnome_pixmap_load_file_at_size (GnomePixmap *gpixmap, const char *filename, int width, int height)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (filename != NULL);
	g_return_if_fail (width > 0);
	g_return_if_fail (height > 0);

	load_file (gpixmap, filename, TRUE, width, height);
}

/**
 * gnome_pixmap_load_xpm_d:
 * @gpixmap: the #GnomePixmap widget
 * @xpm_data: xpm image data
 *
 * Description: Sets the gnome pixmap to image stored in @xpm_data.
 */
void
gnome_pixmap_load_xpm_d (GnomePixmap *gpixmap, char **xpm_data)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (xpm_data != NULL);

	load_xpm_d (gpixmap, xpm_data, FALSE, 0, 0);
}

/**
 * gnome_pixmap_load_xpm_d_at_size:
 * @gpixmap: the #GnomePixmap widget
 * @xpm_data: xpm image data
 * @width: desired width.
 * @height: desired height.
 *
 * Description: Sets the gnome pixmap to image stored in
 * @xpm_data scaled to @width and @height pixels.
 */
void
gnome_pixmap_load_xpm_d_at_size (GnomePixmap *gpixmap, char **xpm_data, int width, int height)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (xpm_data != NULL);
	g_return_if_fail (width > 0);
	g_return_if_fail (height > 0);

	load_xpm_d (gpixmap, xpm_data, TRUE, width, height);
}

/**
 * gnome_pixmap_load_rgb_d:
 * @gpixmap: the #GnomePixmap widget
 * @xpm_data: xpm image data
 * @data: A pointer to an inlined rgb image.
 * @alpha:
 * @rgb_width: the width of the rgb image.
 * @rgb_height: the height of the rgb image.
 *
 * Description: Sets the gnome pixmap to the image.
 */
void
gnome_pixmap_load_rgb_d (GnomePixmap *gpixmap, unsigned char *data, unsigned char *alpha,
			 int rgb_width, int rgb_height)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (data != NULL);
	g_return_if_fail (rgb_width > 0);
	g_return_if_fail (rgb_height > 0);

	load_rgb_d (gpixmap, data, alpha, rgb_width, rgb_height, FALSE, 0, 0);
}

/**
 * gnome_pixmap_load_rgb_d_shaped:
 * @gpixmap: the #GnomePixmap widget
 * @xpm_data: xpm image data
 * @data: A pointer to an inlined rgb image.
 * @alpha:
 * @rgb_width: the width of the rgb image.
 * @rgb_height: the height of the rgb image.
 * @shape_color: which color encodes the transparency
 *
 * Description: Sets the gnome pixmap to the image.
 */
void
gnome_pixmap_load_rgb_d_shaped (GnomePixmap *gpixmap, unsigned char *data,
				unsigned char *alpha,
				int rgb_width, int rgb_height,
				GdkImlibColor *shape_color)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (data != NULL);
	g_return_if_fail (rgb_width > 0);
	g_return_if_fail (rgb_height > 0);

	load_rgb_d_shaped (gpixmap, data, alpha, rgb_width, rgb_height, FALSE,
			   0, 0, shape_color);
}

/**
 * gnome_pixmap_load_rgb_d_at_size:
 * @gpixmap: the #GnomePixmap widget
 * @xpm_data: xpm image data
 * @data: A pointer to an inlined rgb image.
 * @alpha:
 * @rgb_width: the width of the rgb image.
 * @rgb_height: the height of the rgb image.
 * @width: desired width.
 * @height: desired height.
 *
 * Description: Sets the gnome pixmap to the image scaled to
 * @width and @height pixels.
 */
void
gnome_pixmap_load_rgb_d_at_size (GnomePixmap *gpixmap, unsigned char *data, unsigned char *alpha,
				 int rgb_width, int rgb_height,
				 int width, int height)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (data != NULL);
	g_return_if_fail (rgb_width > 0);
	g_return_if_fail (rgb_height > 0);
	g_return_if_fail (width > 0);
	g_return_if_fail (height > 0);

	load_rgb_d (gpixmap, data, alpha, rgb_width, rgb_height, TRUE, width, height);
}

/**
 * gnome_pixmap_load_rgb_d_shaped_at_size:
 * @gpixmap: the #GnomePixmap widget
 * @xpm_data: xpm image data
 * @data: A pointer to an inlined rgb image.
 * @alpha:
 * @rgb_width: the width of the rgb image.
 * @rgb_height: the height of the rgb image.
 * @width: desired width.
 * @height: desired height.
 * @shape_color: which color encodes the transparency
 *
 * Description: Sets the gnome pixmap to the image scaled to
 * @width and @height pixels.
 */
void
gnome_pixmap_load_rgb_d_shaped_at_size (GnomePixmap *gpixmap,
					unsigned char *data,
					unsigned char *alpha,
					int rgb_width, int rgb_height,
					int width, int height,
					GdkImlibColor *shape_color)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (data != NULL);
	g_return_if_fail (rgb_width > 0);
	g_return_if_fail (rgb_height > 0);
	g_return_if_fail (width > 0);
	g_return_if_fail (height > 0);

	load_rgb_d_shaped (gpixmap, data, alpha, rgb_width, rgb_height, TRUE,
			   width, height, shape_color);
}

/**
 * gnome_pixmap_load_imlib:
 * @gpixmap: the #GnomePixmap widget
 * @im: A pointer to GdkImlibImage data
 * @width: desired width.
 * @height: desired height.
 *
 * Description: Sets the gnome pixmap to image stored in @im. Note
 * that @im will not be rendered after this call.
 */
void
gnome_pixmap_load_imlib (GnomePixmap *gpixmap, GdkImlibImage *im)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (im != NULL);

	finish_load (gpixmap, im, 0, 0, 0, 0);
}

/**
 * gnome_pixmap_load_imlib_at_size:
 * @gpixmap: the #GnomePixmap widget
 * @im: A pointer to GdkImlibImage data
 * @width: desired width.
 * @height: desired height.
 *
 * Description: Sets the gnome pixmap to image stored in @im
 * scaled to @width and @height pixels. Note that @im will not
 * be rendered after this call.
 */
void
gnome_pixmap_load_imlib_at_size (GnomePixmap *gpixmap,
				 GdkImlibImage *im, int width, int height)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (im != NULL);
	g_return_if_fail (width > 0);
	g_return_if_fail (height > 0);

	finish_load (gpixmap, im, 1, width, height, 0);
}

