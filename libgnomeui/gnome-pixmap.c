/* GNOME GUI Library
 * Copyright (C) 1997 the Free Software Foundation
 *
 * Authors: Miguel de Icaza
 *          Federico Mena
 */


#include <gdk_imlib.h>
#include "libgnome/gnome-defs.h"
#include <gtk/gtk.h>                  /* These two includes should be remove once everyting */
#include "libgnome/libgnome.h"        /* is switched to use the GnomePixmap widget.         */
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


static GtkWidgetClass *parent_class;


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
	GTK_WIDGET_SET_FLAGS (gpixmap, GTK_BASIC);

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

GtkWidget *
gnome_pixmap_new_from_file (char *filename)
{
	GnomePixmap *gpixmap;

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_file (gpixmap, filename);

	return GTK_WIDGET (gpixmap);
}

GtkWidget *
gnome_pixmap_new_from_file_at_size (char *filename, int width, int height)
{
	GnomePixmap *gpixmap;

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_file_at_size (gpixmap, filename, width, height);

	return GTK_WIDGET (gpixmap);
}

GtkWidget *
gnome_pixmap_new_from_xpm_d (char **xpm_data)
{
	GnomePixmap *gpixmap;

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_xpm_d (gpixmap, xpm_data);

	return GTK_WIDGET (gpixmap);
}

GtkWidget *
gnome_pixmap_new_from_xpm_d_at_size (char **xpm_data, int width, int height)
{
	GnomePixmap *gpixmap;

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_xpm_d_at_size (gpixmap, xpm_data, width, height);

	return GTK_WIDGET (gpixmap);
}

GtkWidget *
gnome_pixmap_new_from_rgb_d (unsigned char *data, unsigned char *alpha,
			     int rgb_width, int rgb_height)
{
	GnomePixmap *gpixmap;

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_rgb_d (gpixmap, data, alpha,
				 rgb_width, rgb_height);

	return GTK_WIDGET (gpixmap);
}

GtkWidget *
gnome_pixmap_new_from_rgb_d_at_size (char *data, unsigned char *alpha,
				     int rgb_width, int rgb_height,
				     int width, int height)
{
	GnomePixmap *gpixmap;

	gpixmap = gtk_type_new (gnome_pixmap_get_type ());
	gnome_pixmap_load_rgb_d_at_size (gpixmap, data, alpha,
					 rgb_width, rgb_height,
					 width, height);

	return GTK_WIDGET (gpixmap);
}

static void
setup_window_and_style (GnomePixmap *gpixmap)
{
	GdkWindowAttr attributes;
	gint attributes_mask;
	GtkWidget *widget;
	gint w, h;
	GdkVisual *visual;
	GdkColormap *colormap;

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
		visual = gdk_imlib_get_visual ();
		colormap = gdk_imlib_get_colormap ();
	} else {
		w = h = 0;
		visual = gtk_widget_get_visual (widget);
		colormap = gtk_widget_get_colormap (widget);
	}

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x + (widget->allocation.width - w) / 2;
	attributes.y = widget->allocation.y + (widget->allocation.height - h) / 2;
	attributes.width = w;
	attributes.height = h;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = visual;
	attributes.colormap = colormap;
	attributes.event_mask = (gtk_widget_get_events (widget)
				 | GDK_EXPOSURE_MASK);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
					 &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, widget);

	if (gpixmap->mask)
		gdk_window_shape_combine_mask (widget->window, gpixmap->mask, 0, 0);

	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

void
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

	widget->requisition.width = requisition->width = w;
	widget->requisition.height = requisition->height = h;
}

static void
gnome_pixmap_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (widget));
	g_return_if_fail (allocation != NULL);

	widget->allocation = *allocation;

	if (GTK_WIDGET_REALIZED (widget))
		gdk_window_move (widget->window,
				 allocation->x + (allocation->width - widget->requisition.width) / 2,
				 allocation->y + (allocation->height - widget->requisition.height) / 2);
}

static void
paint (GnomePixmap *gpixmap, GdkRectangle *area)
{
	if (gpixmap->pixmap)
		gdk_draw_pixmap (gpixmap->widget.window,
				 gpixmap->widget.style->black_gc,
				 gpixmap->pixmap,
				 area->x, area->y,
				 area->x, area->y,
				 area->width, area->height);
	else
		gdk_window_clear_area (gpixmap->widget.window,
				       area->x, area->y,
				       area->width, area->height);
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

		/* Offset the area because the window does not fill the allocation */

		area->x -= (widget->allocation.width - widget->requisition.width) / 2;
		area->y -= (widget->allocation.height - widget->requisition.height) / 2;

		w_area.x = 0;
		w_area.y = 0;
		w_area.width = widget->requisition.width;
		w_area.height = widget->requisition.height;

		if (gdk_rectangle_intersect (area, &w_area, &p_area))
			paint (gpixmap, &p_area);
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
finish_load (GnomePixmap *gpixmap, GdkImlibImage *im, int scaled, int width, int height)
{
	if (!im)
		return;

	if (scaled)
		gdk_imlib_render (im, width, height);
	else
		gdk_imlib_render (im, im->rgb_width, im->rgb_height);

	gpixmap->pixmap = gdk_imlib_copy_image (im);
	gpixmap->mask = gdk_imlib_copy_mask (im);

	gdk_imlib_destroy_image (im);

	if (GTK_WIDGET_REALIZED (gpixmap))
		setup_window_and_style (gpixmap);

	if (GTK_WIDGET_VISIBLE (gpixmap))
		gtk_widget_queue_resize (GTK_WIDGET (gpixmap));
}

static void
load_file (GnomePixmap *gpixmap, char *filename, int scaled, int width, int height)
{
	GdkImlibImage *im;

	free_pixmap_and_mask (gpixmap);

	im = gdk_imlib_load_image (filename);
	finish_load (gpixmap, im, scaled, width, height);
}

static void
load_xpm_d (GnomePixmap *gpixmap, char **xpm_data, int scaled, int width, int height)
{
	GdkImlibImage *im;

	free_pixmap_and_mask (gpixmap);

	im = gdk_imlib_create_image_from_xpm_data (xpm_data);
	finish_load (gpixmap, im, scaled, width, height);
}

static void
load_rgb_d (GnomePixmap *gpixmap, unsigned char *data, unsigned char *alpha,
	    int rgb_width, int rgb_height,
	    int scaled, int width, int height)
{
	GdkImlibImage *im;

	free_pixmap_and_mask (gpixmap);

	im = gdk_imlib_create_image_from_data (data, alpha, rgb_width, rgb_height);
	finish_load (gpixmap, im, scaled, width, height);
}

void
gnome_pixmap_load_file (GnomePixmap *gpixmap, char *filename)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (filename != NULL);

	load_file (gpixmap, filename, FALSE, 0, 0);
}

void
gnome_pixmap_load_file_at_size (GnomePixmap *gpixmap, char *filename, int width, int height)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (filename != NULL);
	g_return_if_fail (width > 0);
	g_return_if_fail (height > 0);

	load_file (gpixmap, filename, TRUE, width, height);
}

void
gnome_pixmap_load_xpm_d (GnomePixmap *gpixmap, char **xpm_data)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (gpixmap));
	g_return_if_fail (xpm_data != NULL);

	load_xpm_d (gpixmap, xpm_data, FALSE, 0, 0);
}

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


/* FIXME:  These are the old gnome-pixmap API functions.  They should
 * be flushed out in favor of the GnomePixmap widget.
 */

struct pixmap_item {
	GdkPixmap *pixmap;
	GdkBitmap *mask;
};

static GHashTable *pixmap_hash;


static void
destroy_hash_element (void *key, void *val, void *data)
{
	struct pixmap_item *pi = val;

	gdk_pixmap_unref (pi->pixmap);
	gdk_pixmap_unref (pi->mask);
	g_free (val);
	g_free (key);
}

void
gnome_destroy_pixmap_cache (void)
{
	if (!pixmap_hash)
		return;

	g_hash_table_foreach (pixmap_hash, destroy_hash_element, 0);
	g_hash_table_destroy (pixmap_hash);
	pixmap_hash = NULL;
}

static void
check_hash_table(void)
{
	if (!pixmap_hash)
		pixmap_hash = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
load_pixmap(GdkWindow *window, GdkPixmap **pixmap, GdkBitmap **mask, GdkColor *transparent, char *filename)
{
	struct pixmap_item *pit;

	check_hash_table();

	pit = g_hash_table_lookup(pixmap_hash, filename);

	if (!pit) {
		pit = g_new(struct pixmap_item, 1);
		pit->pixmap = gdk_pixmap_create_from_xpm(window, &pit->mask, transparent, filename);
		
		g_hash_table_insert(pixmap_hash, g_strdup(filename), pit);
	}
	
	*pixmap = pit->pixmap;
	*mask   = pit->mask;
}

void
gnome_create_pixmap_gdk (GdkWindow *window, GdkPixmap **pixmap, GdkBitmap **mask, GdkColor *transparent, char *file)
{
	printf ("gnome_create_pixmap_gdk: this function is obsolete and evil; please use gdk_imlib instead\n");

	g_assert(window != NULL);
	g_assert(pixmap != NULL);
	g_assert(mask != NULL);
	g_assert(transparent != NULL);

	if (!file || !g_file_exists(file)) {
		*pixmap = NULL;
		*mask = NULL;
		return;
	}

	load_pixmap(window, pixmap, mask, transparent, file);
}

void
gnome_create_pixmap_gtk (GtkWidget *window, GdkPixmap **pixmap, GdkBitmap **mask, GtkWidget *holder, char *file)
{
	GtkStyle *style;
	
	printf ("gnome_create_pixmap_gtk: this function is obsolete and evil; please use gdk_imlib instead\n");

	g_assert(window != NULL);
	g_assert(pixmap != NULL);
	g_assert(mask != NULL);
	g_assert(holder != NULL);

	if (!file || !g_file_exists(file)) {
		*pixmap = NULL;
		*mask = NULL;
		return;
	}

	if (!GTK_WIDGET_REALIZED(window))
		gtk_widget_realize(window);

	style = gtk_widget_get_style (holder);

	load_pixmap(window->window, pixmap, mask,
		    style ? &style->bg[GTK_STATE_NORMAL] : NULL, /* XXX: NULL will make it bomb */
		    file);
}

void
gnome_create_pixmap_gtk_d (GtkWidget *window, GdkPixmap **pixmap, GdkBitmap **mask, GtkWidget *holder,
			   char **data)
{
	GtkStyle *style;
	
	printf ("gnome_create_pixmap_gtk_d: this function is obsolete and evil; please use gdk_imlib instead\n");

	g_assert(window != NULL);
	g_assert(pixmap != NULL);
	g_assert(mask != NULL);
	g_assert(holder != NULL);

	if (!GTK_WIDGET_REALIZED(window))
		gtk_widget_realize(window);

	style = gtk_widget_get_style (holder);

	*pixmap = gdk_pixmap_create_from_xpm_d (window->window, mask, &style->bg[GTK_STATE_NORMAL], data);
}

GtkWidget *
gnome_create_pixmap_widget (GtkWidget *window, GtkWidget *holder, char *file)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	printf ("gnome_create_pixmap_widget: this function is obsolete and evil; please use GnomePixmap instead\n");

	if (!file || !g_file_exists(file))
		return NULL;

	gnome_create_pixmap_gtk(window, &pixmap, &mask, holder, file);

	return gtk_pixmap_new(pixmap, mask);
}

GtkWidget *
gnome_create_pixmap_widget_d (GtkWidget *window, GtkWidget *holder, char **data)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	printf ("gnome_create_pixmap_widget_d: this function is obsolete and evil; please use GnomePixmap instead\n");

	gnome_create_pixmap_gtk_d(window, &pixmap, &mask, holder, data);

	return gtk_pixmap_new(pixmap, mask);
}

void
gnome_set_pixmap_widget (GtkPixmap *pixmap, GtkWidget *window, GtkWidget *holder, gchar *file)
{
	GdkPixmap *gpixmap;
	GdkBitmap *mask;

	printf ("gnome_set_pixmap_widget: this function is obsolete and evil; please use gdk_imlib instead\n");
	
	g_assert (pixmap != NULL);

	if (!file || !g_file_exists(file))
		return;

	gnome_create_pixmap_gtk(window, &gpixmap, &mask, holder, file);

	gtk_pixmap_set(pixmap, gpixmap, mask);
}

void
gnome_set_pixmap_widget_d (GtkPixmap *pixmap, GtkWidget *window, GtkWidget *holder, gchar **data)
{
	GdkPixmap *gpixmap;
	GdkBitmap *mask;
	
	printf ("gnome_set_pixmap_widget_d: this function is obsolete and evil; please use gdk_imlib instead\n");
	
	g_assert (pixmap != NULL);

	gnome_create_pixmap_gtk_d(window, &gpixmap, &mask, holder, data);

	gtk_pixmap_set(pixmap, gpixmap, mask);
}
