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
		w = h = 1;
		visual = gtk_widget_get_visual (widget);
		colormap = gtk_widget_get_colormap (widget);
	}

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
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

	if (GTK_WIDGET_REALIZED (widget))
		gdk_window_move (widget->window, allocation->x, allocation->y);
}

static gint
gnome_pixmap_expose (GtkWidget *widget, GdkEventExpose *event)
{
	GnomePixmap *gpixmap;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_PIXMAP (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		gpixmap = GNOME_PIXMAP (widget);

		if (gpixmap->pixmap)
			gdk_draw_pixmap (widget->window,
					 widget->style->black_gc,
					 gpixmap->pixmap,
					 0, 0, 0, 0, -1, -1);
		else
			gdk_window_clear (widget->window);
	}

	return FALSE;
}

static void
load_file (GnomePixmap *gpixmap, char *filename, int sized, int width, int height)
{
	GdkImlibImage *im;

	free_pixmap_and_mask (gpixmap);

	im = gdk_imlib_load_image (filename);
	if (!im)
		return;

	if (sized)
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

	gnome_create_pixmap_gtk_d(window, &pixmap, &mask, holder, data);

	return gtk_pixmap_new(pixmap, mask);
}

void
gnome_set_pixmap_widget (GtkPixmap *pixmap, GtkWidget *window, GtkWidget *holder, gchar *file)
{
	GdkPixmap *gpixmap;
	GdkBitmap *mask;
	
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
	
	g_assert (pixmap != NULL);

	gnome_create_pixmap_gtk_d(window, &gpixmap, &mask, holder, data);

	gtk_pixmap_set(pixmap, gpixmap, mask);
}
