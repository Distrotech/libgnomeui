/* GNOME GUI Library, second iteration of GnomePixmap
 * Copyright (C) 1997, 1998, 1999 the Free Software Foundation
 *
 * 
 * GnomePixmap v2: Havoc Pennington
 */

#include <config.h>
#include "gnome-pixmap.h"

static void gnome_pixmap_class_init    (GnomePixmapClass *class);
static void gnome_pixmap_init          (GnomePixmap      *gpixmap);
static void gnome_pixmap_destroy       (GtkObject        *object);
static gint gnome_pixmap_expose        (GtkWidget        *widget,
					GdkEventExpose   *event);

static void clear_old_images           (GnomePixmap *gpixmap);
static void build_insensitive_pixbuf      (GnomePixmap *gpixmap);
static void build_insensitive_pixmap      (GnomePixmap *gpixmap);

static GtkMiscClass *parent_class = NULL;

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
        guint i;
        
        /* Default to pixmap mode, store data on
           server */
        gpixmap->flags = GNOME_PIXMAP_USE_PIXMAP | GNOME_PIXMAP_BUILD_INSENSITIVE;

        i = 0;
        while (i < 5) {
                gpixmap->image_data[i].pixbuf = NULL;
                gpixmap->image_data[i].pixmap = NULL;
                gpixmap->image_data[i].mask = NULL;

                ++i;
        }
}

GtkWidget*
gnome_pixmap_new(GnomePixmapMode mode)
{
        GtkWidget* widget;
        GnomePixmap* gpixmap;

        widget = gtk_type_new(gnome_pixmap_get_type());
        gpixmap = GNOME_PIXMAP(widget);
        
        switch (mode) {
        case GNOME_PIXMAP_KEEP_PIXMAP:
                gpixmap->flags &= ~GNOME_PIXMAP_USE_PIXBUF;
                gpixmap->flags |= GNOME_PIXMAP_USE_PIXMAP;
                break;

        case GNOME_PIXMAP_KEEP_PIXBUF:
                gpixmap->flags &= ~GNOME_PIXMAP_USE_PIXMAP;
                gpixmap->flags |= GNOME_PIXMAP_USE_PIXBUF;
                break;

        default:
                g_warning("Invalid GnomePixmap mode");
                break;
        }

        return widget;
}

static void
gnome_pixmap_class_init (GnomePixmapClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	parent_class = gtk_type_class (gtk_misc_get_type ());

	widget_class->expose_event = gnome_pixmap_expose;

	object_class->destroy = gnome_pixmap_destroy;
}

static void
gnome_pixmap_destroy (GtkObject *object)
{
	GnomePixmap *gpixmap;
        
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (object));

	gpixmap = GNOME_PIXMAP (object);

        clear_old_images(gpixmap);
        
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
paint_with_pixbuf (GnomePixmap *gpixmap, GdkRectangle *area)
{
	GtkWidget *widget;
        GdkPixbuf *draw_source;
        GdkBitmap *draw_mask;
        
        widget = GTK_WIDGET(gpixmap);

        g_return_if_fail(GTK_WIDGET_DRAWABLE(gpixmap));

        draw_source = gpixmap->image_data[GTK_WIDGET_STATE(widget)].pixbuf;
        draw_mask = gpixmap->image_data[GTK_WIDGET_STATE(widget)].mask;
        
        if (draw_source == NULL &&
            GTK_WIDGET_STATE(widget) == GTK_STATE_INSENSITIVE &&
            (gpixmap->flags & GNOME_PIXMAP_BUILD_INSENSITIVE)) {
          /* Demand-create the insensitive pixmap if we are insensitive
           * and the build_insensitive flag is turned on
           */
          
                build_insensitive_pixbuf(gpixmap);
                
                draw_source = gpixmap->image_data[GTK_STATE_INSENSITIVE].pixbuf;
                draw_mask = gpixmap->image_data[GTK_STATE_INSENSITIVE].mask;
        }
        
        if (draw_source == NULL) {
                /* Try normal, if the other state isn't set */
                draw_source = gpixmap->image_data[GTK_STATE_NORMAL].pixbuf;
                draw_mask = gpixmap->image_data[GTK_STATE_NORMAL].mask;
        }

        if (draw_source != NULL) {
                /* Draw the image */                
                gint x, y;
          
                x = (widget->allocation.x * (1.0 - misc->xalign) +
                     (widget->allocation.x + widget->allocation.width
                      - (widget->requisition.width - misc->xpad * 2)) *
                     misc->xalign) + 0.5;
                y = (widget->allocation.y * (1.0 - misc->yalign) +
                     (widget->allocation.y + widget->allocation.height
                      - (widget->requisition.height - misc->ypad * 2)) *
                     misc->yalign) + 0.5;
                
                if (draw_mask) {
                        gdk_gc_set_clip_mask (widget->style->black_gc, draw_mask);
                        gdk_gc_set_clip_origin (widget->style->black_gc, x, y);
                }

                /* FIXME draw the pixbuf */

                if (draw_mask) {
                        gdk_gc_set_clip_mask (widget->style->black_gc, NULL);
                        gdk_gc_set_clip_origin (widget->style->black_gc, 0, 0);
                }
                
        } else {
                /* Draw nothing */
                g_warning("GnomePixmap expose received, but our requisition should have been 0,0");
        }
}

static void
paint_with_pixmap (GnomePixmap *gpixmap, GdkRectangle *area)
{
	GtkWidget *widget;
        GdkPixmap *draw_source;
        GdkBitmap *draw_mask;
        
        widget = GTK_WIDGET(gpixmap);
        misc = GTK_MISC (widget);
        
        g_return_if_fail(GTK_WIDGET_DRAWABLE(gpixmap));

        draw_source = gpixmap->image_data[GTK_WIDGET_STATE(widget)].pixmap;
        draw_mask = gpixmap->image_data[GTK_WIDGET_STATE(widget)].mask;

        if (draw_source == NULL &&
            GTK_WIDGET_STATE(widget) == GTK_STATE_INSENSITIVE &&
            (gpixmap->flags & GNOME_PIXMAP_BUILD_INSENSITIVE)) {
          /* Demand-create the insensitive pixmap if we are insensitive
           * and the build_insensitive flag is turned on
           */
                build_insensitive_pixmap(gpixmap);
                
                draw_source = gpixmap->image_data[GTK_STATE_INSENSITIVE].pixmap;
                draw_mask = gpixmap->image_data[GTK_STATE_INSENSITIVE].mask;
        }
        
        if (draw_source == NULL) {
                /* fall back to normal, if no special case */
                draw_source = gpixmap->image_data[GTK_STATE_NORMAL].pixmap;
                draw_mask = gpixmap->image_data[GTK_STATE_NORMAL].mask;
        }

        if (draw_source != NULL) {
                /* Draw the image */
                gint x, y;
          
                x = (widget->allocation.x * (1.0 - misc->xalign) +
                     (widget->allocation.x + widget->allocation.width
                      - (widget->requisition.width - misc->xpad * 2)) *
                     misc->xalign) + 0.5;
                y = (widget->allocation.y * (1.0 - misc->yalign) +
                     (widget->allocation.y + widget->allocation.height
                      - (widget->requisition.height - misc->ypad * 2)) *
                     misc->yalign) + 0.5;
                
                if (draw_mask) {
                        gdk_gc_set_clip_mask (widget->style->black_gc, draw_mask);
                        gdk_gc_set_clip_origin (widget->style->black_gc, x, y);
                }

                /* FIXME we are always redrawing the whole thing */
                gdk_draw_pixmap (widget->window,
                                 widget->style->black_gc,
                                 draw_source,
                                 0, 0,
                                 x, y,
                                 -1, -1);

                if (draw_mask) {
                        gdk_gc_set_clip_mask (widget->style->black_gc, NULL);
                        gdk_gc_set_clip_origin (widget->style->black_gc, 0, 0);
                }

        } else {
                /* Shouldn't be getting this */
                g_warning("GnomePixmap expose received, but our requisition should have been 0,0");
        }
}

static void
paint (GnomePixmap *gpixmap, GdkRectangle *area)
{
	GtkWidget *widget;
        GdkPixmap *draw_source;
        GdkBitmap *draw_mask;
        
        widget = GTK_WIDGET(gpixmap);
        
        if (gpixmap->flags & GNOME_PIXMAP_USE_PIXBUF) {
                paint_with_pixbuf(gpixmap, area);
        } else if (gpixmap->flags & GNOME_PIXMAP_USE_PIXMAP) {
                paint_with_pixmap(gpixmap, area);
        } else {
                g_assert_not_reached();
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

void
gnome_pixmap_set_build_insensitive (GnomePixmap *gpixmap,
                                    gboolean build_insensitive)
{
  g_return_if_fail(gpixmap != NULL);
  g_return_if_fail(GNOME_IS_PIXMAP(gpixmap));

  if (build_insensitive)
    gpixmap->flags |= GNOME_PIXMAP_BUILD_INSENSITIVE;
  else
    gpixmap->flags &= ~GNOME_PIXMAP_BUILD_INSENSITIVE;
}

/*
 * Set image data, create with image data 
 */

static void
clear_old_images(GnomePixmap *gpixmap)
{
        guint i;
        
        i = 0;
        while (i < 5) {
                if (gpixmap->image_data[i].pixbuf != NULL) {
                        gdk_pixbuf_unref(gpixmap->image_data[i].pixbuf);
                        gpixmap->image_data[i].pixbuf = NULL;
                }

                if (gpixmap->image_data[i].pixmap != NULL) {
                        gdk_pixmap_unref(gpixmap->image_data[i].pixmap);
                        gpixmap->image_data[i].pixmap = NULL;
                }

                if (gpixmap->image_data[i].mask != NULL) {
                        gdk_bitmap_unref(gpixmap->image_data[i].mask);
                        gpixmap->image_data[i].mask = NULL;
                }
                
                ++i;
        }
}

static void
resize_to_fit(GnomePixmap *gpixmap)
{
        /* We base size on GTK_STATE_NORMAL, or failing that the first state
           we can find. */
        guint i;
        gint width, height;
        gint oldwidth, oldheight;
        
        oldwidth = GTK_WIDGET (pixmap)->requisition.width;
        oldheight = GTK_WIDGET (pixmap)->requisition.height;
        
        width = 0;
        height = 0;

        if (gpixmap->flags & GNOME_PIXMAP_USE_PIXMAP) {

                GdkPixmap *pix = NULL;

                pix = gpixmap->image_data[GTK_STATE_NORMAL].pixmap;

                if (pix == NULL) {
                        i = 0;
                        while (i < 5) {
                                pix = gpixmap->image_data[i].pixmap;

                                if (pix != NULL)
                                        break;

                                ++i;
                        }
                }

                if (pix != NULL) {
                        gdk_window_get_size (pix, &width, &height);
                }

        } else if (gpixmap->flags & GNOME_PIXMAP_USE_PIXMAP) {

                GdkPixbuf *pix = NULL;

                pix = gpixmap->image_data[GTK_STATE_NORMAL].pixbuf;

                if (pix == NULL) {
                        i = 0;
                        while (i < 5) {
                                pix = gpixmap->image_data[i].pixbuf;

                                if (pix != NULL)
                                        break;

                                ++i;
                        }
                }

                if (pix != NULL) {
                        width = pix->art_pixbuf->width;
                        height = pix->art_pixbuf->height;
                }

        } else {
                /* A GnomePixmap invariant is that either USE_PIXMAP or USE_PIXBUF is
                 * always set
                 */
                g_assert_not_reached();
        }

        if (width * height > 0) {
                GTK_WIDGET (gpixmap)->requisition.width = width + GTK_MISC (pixmap)->xpad * 2;
                GTK_WIDGET (gpixmap)->requisition.height = height + GTK_MISC (pixmap)->ypad * 2;
        } else {
                GTK_WIDGET (gpixmap)->requisition.width = 0;
                GTK_WIDGET (gpixmap)->requisition.height = 0;
        }

        if (GTK_WIDGET_VISIBLE (gpixmap)) {
                if ((GTK_WIDGET (gpixmap)->requisition.width != oldwidth) ||
                    (GTK_WIDGET (gpixmap)->requisition.height != oldheight))
                        gtk_widget_queue_resize (GTK_WIDGET (gpixmap));
                else
                        gtk_widget_queue_clear (GTK_WIDGET (gpixmap));
	}
}

static void
set_pixbufs(GnomePixmap* gpixmap, GdkPixbuf* pixbufs[5])
{
        guint i;

        clear_old_images(gpixmap);

        gpixmap->flags &= ~GNOME_PIXMAP_USE_PIXMAP;
        gpixmap->flags |= GNOME_PIXMAP_USE_PIXBUF;
        
        i = 0;
        while (i < 5) {

                if (pixbufs[i] != NULL) {
                        gpixmap->image_data[i].pixbuf = pixbufs[i];
                        
                        gdk_pixbuf_ref(pixbufs[i]);
                
                        gpixmap->image_data[i].mask = ; /* FIXME generate mask */
                }
                        
                ++i;
        }

        resize_to_fit(gpixmap);
}

static void
set_pixmaps(GnomePixmap* gpixmap, GdkPixmap* pixmaps[5], GdkBitmap* masks[5])
{
        guint i;

        clear_old_images(gpixmap);

        gpixmap->flags &= ~GNOME_PIXMAP_USE_PIXBUF;
        gpixmap->flags |= GNOME_PIXMAP_USE_PIXMAP;
        
        i = 0;
        while (i < 5) {

                if (pixmaps[i] != NULL) {
                        gpixmap->image_data[i].pixmap = pixmaps[i];
                        
                        gdk_pixmap_ref(pixmaps[i]);
                }

                if (masks[i] != NULL) {
                        gpixmap->image_data[i].mask = masks[i];

                        gdk_bitmap_ref(masks[i]);
                }
                        
                ++i;
        }

        resize_to_fit(gpixmap);
}



void
gnome_pixmap_set_pixbufs            (GnomePixmap *gpixmap,
                                     GdkPixbuf   *pixbufs[5],
                                     GdkBitmap   *masks[5])
{
        g_return_if_fail(gpixmap != NULL);
        g_return_if_fail(GNOME_IS_PIXMAP(gpixmap));
        
        if (gpixmap->flags & GNOME_PIXMAP_USE_PIXBUF)
                set_pixbufs(gpixmap, pixbufs, masks);
        else {
                /* Convert to pixmaps and don't keep a pixbuf reference */
                /* FIXME */
                
        }
}

void
gnome_pixmap_set_pixmaps            (GnomePixmap *gpixmap,
                                     GdkPixmap   *pixmaps[5],
                                     GdkBitmap   *masks[5])
{
        g_return_if_fail(gpixmap != NULL);
        g_return_if_fail(GNOME_IS_PIXMAP(gpixmap));
        
        if (gpixmap->flags & GNOME_PIXMAP_USE_PIXMAP)
                set_pixmaps(gpixmap, pixmaps, masks);
        else {
                /* Convert to pixbufs and don't keep a pixmap reference */
                /* FIXME */
                
        }
}

GtkWidget*
gnome_pixmap_new_from_pixbuf          (GdkPixbuf *pixbuf)
{
        return gnome_pixmap_new_from_pixbuf_at_size(pixbuf, -1, -1);
}

GtkWidget*
gnome_pixmap_new_from_pixbuf_at_size  (GdkPixbuf *pixbuf, gint width, gint height)
{
        GtkWidget *widget;
        GnomePixmap *gpixmap;
        GdkPixbuf *pixbufs[5] = { NULL, NULL, NULL, NULL, NULL };
        GdkBitmap *masks[5] = { NULL, NULL, NULL, NULL, NULL };
        
        widget = gtk_type_new(gnome_pixmap_get_type());
        gpixmap = GNOME_PIXMAP(widget);

        g_return_val_if_fail(pixbuf != NULL, widget);
        
        /* Using pixmap mode by default */
        
        if (pixbuf != NULL) {
                /* Scale if needed, -1 means "natural dimension" */
                if (width < 0)
                        width = pixbuf->art_pixbuf->width;
                if (height < 0)
                        height = pixbuf->art_pixbuf->height;
                
                if (width != pixbuf->art_pixbuf->width ||
                    height != pixbuf->art_pixbuf->height) {
                        pixbuf = gdk_pixbuf_scale(pixbuf, width, height);
                }
                
                pixbufs[GTK_STATE_NORMAL] = pixbuf;

                masks[GTK_STATE_NORMAL] = gdk_pixbuf_create_mask(pixbuf);

                gnome_pixmap_set_pixbufs(gpixmap, pixbufs, masks);
        }

        return widget;
}


GtkWidget*
gnome_pixmap_new_from_file_at_size          (const gchar *filename, gint width, gint height)
{
        GtkWidget *widget;
        
        pixbuf = gdk_pixbuf_new_from_file(filename);

        if (pixbuf != NULL) {
                widget = gnome_pixmap_new_from_pixbuf_at_size(pixbuf, width, height);
                
                gdk_pixbuf_unref(pixbuf);
        }

        return widget;
}

GtkWidget*
gnome_pixmap_new_from_file          (const char *filename)
{
        return gnome_pixmap_new_from_file_at_size(filename, -1, -1);
}

GtkWidget*
gnome_pixmap_new_from_xpm_d_at_size (char **xpm_data, int width, int height)
{
        GtkWidget *widget;
        
        pixbuf = gdk_pixbuf_new_from_xpm_data(xpm_data);
        
        if (pixbuf != NULL) {
                widget = gnome_pixmap_new_from_pixbuf_at_size(pixbuf, width, height);
                
                gdk_pixbuf_unref(pixbuf);
        }

        return widget;
}

GtkWidget*
gnome_pixmap_new_from_xpm_d         (char **xpm_data)
{
        return gnome_pixmap_new_from_xpm_d_at_size(xpm_data, -1, -1);
}

/*
 * Build insensitive 
 */

static void
build_insensitive_pixbuf      (GnomePixmap *gpixmap)
{
        gint32 red, green, blue;
        GdkPixbuf *normal;
        GdkBitmap *normal_mask;
        GdkPixbuf *insensitive;
        GdkColor color, c;
        GtkStyle *style;
        GtkWidget *widget;
        gint w, h, x, y;
        
        widget = GTK_WIDGET (gpixmap);

        g_return_if_fail(widget != NULL);

        normal = gpixmap->image_data[GTK_STATE_NORMAL].pixbuf;
        normal_mask = gpixmap->image_data[GTK_STATE_NORMAL].mask;

        if (normal == NULL)
                return; /* Give up */

        g_return_if_fail(gpixmap->image_data[GTK_STATE_INSENSITIVE].pixbuf == NULL);
        g_return_if_fail(gpixmap->image_data[GTK_STATE_INSENSITIVE].mask == NULL);

        insensitive = gdk_pixbuf_new(); /* FIXME */
        
        style = gtk_widget_get_style(widget);
        
        color = style->bg[0];
        red = color.red/255;
        green = color.green/255;
        blue = color.blue/255;

        /* Fade each pixel */
        for (y = 0; y < h; y++) {
          for (x = 0; x < w; x++) {
            guchar* src_pixel;
            guchar* dest_pixel;

            src_pixel = normal->art_pixbuf->pixels +
              y * normal->art_pixbuf->rowstride +
              x * (normal->art_pixbuf->has_alpha ? 4 : 3);

            dest_pixel = insensitive->art_pixbuf->pixels +
              y * insensitive->art_pixbuf->rowstride +
              x * (insensitive->art_pixbuf->has_alpha ? 4 : 3);
            
            dest_pixel[0] = ((src_pixel[0] - red) >> 1) + red;
            dest_pixel[1] = ((src_pixel[1] - green) >> 1) + green;
            dest_pixel[2] = ((src_pixel[2] - blue) >> 1) + blue;
          }
        }

        /* Now do another fade pass, skipping every other pixel */
        for (y = 0; y < h; y++) {
          for (x = y % 2; x < w; x += 2) {
            guchar* src_pixel;
            guchar* dest_pixel;

            src_pixel = insensitive->art_pixbuf->pixels +
              y * insensitive->art_pixbuf->rowstride +
              x * (insensitive->art_pixbuf->has_alpha ? 4 : 3);

            dest_pixel = insensitive->art_pixbuf->pixels +
              y * insensitive->art_pixbuf->rowstride +
              x * (insensitive->art_pixbuf->has_alpha ? 4 : 3);
            
            dest_pixel[0] = ((src_pixel[0] - red) >> 1) + red;
            dest_pixel[1] = ((src_pixel[1] - green) >> 1) + green;
            dest_pixel[2] = ((src_pixel[2] - blue) >> 1) + blue;
          }
        }

        gpixmap->image_data[GTK_STATE_INSENSITIVE].pixbuf = insensitive;
        gpixmap->image_data[GTK_STATE_INSENSITIVE].mask   = normal_mask;

        /* Just ref the normal mask, don't create a new mask. */
        if (normal_mask != NULL) {
                gdk_bitmap_ref(normal_mask);
        }
}


static void
build_insensitive_pixmap      (GnomePixmap *gpixmap)
{
        /* FIXME This code is also in gtk/gtkpixmap.c */
        GdkGC *gc;
        GdkPixmap *normal;
        GdkBitmap *normal_mask;
        GdkPixmap *insensitive;
        gint w, h, x, y;
        GdkGCValues vals;
        GdkVisual *visual;
        GdkImage *image;
        GdkColorContext *cc;
        GdkColor color;
        GdkColormap *cmap;
        gint32 red, green, blue;
        GtkStyle *style;
        GtkWidget *widget;
        GdkColor c;
        gboolean failed;

        widget = GTK_WIDGET (gpixmap);

        g_return_if_fail(widget != NULL);

        normal = gpixmap->image_data[GTK_STATE_NORMAL].pixmap;
        normal_mask = gpixmap->image_data[GTK_STATE_NORMAL].mask;

        if (normal == NULL)
                return; /* Give up */

        g_return_if_fail(gpixmap->image_data[GTK_STATE_INSENSITIVE].pixmap == NULL);
        g_return_if_fail(gpixmap->image_data[GTK_STATE_INSENSITIVE].mask == NULL);
        
        gdk_window_get_size(normal, &w, &h);
        image = gdk_image_get(normal, 0, 0, w, h);
        insensitive = gdk_pixmap_new(widget->window, w, h, -1);
        gc = gdk_gc_new (normal);

        visual = gtk_widget_get_visual(widget);
        cmap = gtk_widget_get_colormap(widget);
        cc = gdk_color_context_new(visual, cmap);

        if ((cc->mode != GDK_CC_MODE_TRUE) &&
            (cc->mode != GDK_CC_MODE_MY_GRAY)) {

                gdk_draw_image(insensitive, gc, image, 0, 0, 0, 0, w, h);
                
                style = gtk_widget_get_style(widget);
                color = style->bg[0];
                gdk_gc_set_foreground (gc, &color);
                for (y = 0; y < h; y++) {
                        for (x = y % 2; x < w; x += 2) {
                                gdk_draw_point(insensitive, gc, x, y);
                        }
                }

        } else {

                gdk_gc_get_values(gc, &vals);
                style = gtk_widget_get_style(widget);

                color = style->bg[0];
                red = color.red;
                green = color.green;
                blue = color.blue;

                for (y = 0; y < h; y++) {
                        for (x = 0; x < w; x++) {
                                c.pixel = gdk_image_get_pixel(image, x, y);
                                gdk_color_context_query_color(cc, &c);
                                c.red = (((gint32)c.red - red) >> 1) + red;
                                c.green = (((gint32)c.green - green) >> 1) + green;
                                c.blue = (((gint32)c.blue - blue) >> 1) + blue;
                                c.pixel = gdk_color_context_get_pixel(cc, c.red, c.green, c.blue,
                                                                      &failed);
                                gdk_image_put_pixel(image, x, y, c.pixel);
                        }
                }

                for (y = 0; y < h; y++) {
                        for (x = y % 2; x < w; x += 2) {
                                c.pixel = gdk_image_get_pixel(image, x, y);
                                gdk_color_context_query_color(cc, &c);
                                c.red = (((gint32)c.red - red) >> 1) + red;
                                c.green = (((gint32)c.green - green) >> 1) + green;
                                c.blue = (((gint32)c.blue - blue) >> 1) + blue;
                                c.pixel = gdk_color_context_get_pixel(cc, c.red, c.green, c.blue,
                                                                      &failed);
                                gdk_image_put_pixel(image, x, y, c.pixel);
                        }
                }

                gdk_draw_image(insensitive, gc, image, 0, 0, 0, 0, w, h);
        }

        gpixmap->image_data[GTK_STATE_INSENSITIVE].pixmap = insensitive;
        gpixmap->image_data[GTK_STATE_INSENSITIVE].mask = normal_mask;

        /* Just ref the normal mask, don't create a new mask. */
        if (normal_mask != NULL) {
                gdk_bitmap_ref(normal_mask);
        }

        gdk_image_destroy(image);
        gdk_color_context_free(cc);
        gdk_gc_destroy(gc);
}

/*
 * This really should not be here; but it is until we write the filtered affine
 * transformer for libart
 */

#if 0
/* code snippet to copy for getting base_color from a window */
/* 
	if (window) {
		GtkStyle *style = gtk_widget_get_style(window);
		scale_base.r = style->bg[state].red >> 8;
		scale_base.g = style->bg[state].green >> 8;
		scale_base.b = style->bg[state].blue >> 8;
	}
*/
#endif

static void
free_buffer (gpointer user_data, gpointer data)
{
        g_return_if_fail (data != NULL);
        g_return_if_fail (user_data == NULL);
	g_free (data);
}

ArtPixBuf*
gnome_pixbuf_scale(ArtPixBuf* pixbuf,
                   gint w, gint h)
{
        ArtPixbuf* retval;
	unsigned char *data, *p, *p2, *p3, *data_old;
        long w_old, h_old;
        long rowstride_old;
	long x, y, w2, h2, x_old, y_old, w_old3, x2, y2, i, x3, y3;
	long yw, xw, ww, hw, r, g, b, a, r2, g2, b2, a2;
	int new_channels;
	int new_rowstride;

        
	g_return_val_if_fail (pixbuf != NULL, NULL);	
	g_return_val_if_fail (pixbuf->format == ART_PIX_RGB, NULL);
	g_return_val_if_fail (pixbuf->n_channels == 3 || pixbuf->n_channels == 4, NULL);
	g_return_val_if_fail (pixbuf->bits_per_sample == 8, NULL);
        g_return_val_if_fail (w > 0);
        g_return_val_if_fail (h > 0);
        
        w_old = pixbuf->width;
        h_old = pixbuf->height;
        rowstride_old = pixbuf->rowstride;
        data_old = pixbuf->pixels;
        
        if (base_color != NULL) {
                scale_base.r = base_color->red >> 8;
                scale_base.g = base_color->green >> 8;
                scale_base.b = base_color->blue >> 8;
        }
        
	/* Always align rows to 32-bit boundaries */

	new_channels = pixbuf->has_alpha ? 4 : 3;
	new_rowstride = 4 * ((new_channels * w + 3) / 4);

        data = g_malloc (h * rowstride);

        if (pixbuf->has_alpha)
		retval = art_pixbuf_new_rgba_dnotify (data, w, h, new_rowstride,
                                                      NULL, free_buffer);
	else
                retval = art_pixbuf_new_rgb_dnotify (data, w, h, new_rowstride,
                                                     NULL, free_buffer);
        
	p = data;

        /* Multiply stuff by 256 to avoid floating point. */
        
	ww = (w_old << 8) / w;  /* old-width-per-new-width * 256 */
	hw = (h_old << 8) / h;  /* old-height-per-new-height * 256 */
	h2 = h << 8;
	w2 = w << 8;

	for (y = 0; y < h2; y += 0x100) {
		y_old = (y * h_old) / h;
		y2 = y_old & 0xff;
		y_old >>= 8;
		for (x = 0; x < w2; x += 0x100) {
			x_old = (x * w_old) / w;
			x2 = x_old & 0xff;
			x_old >>= 8;
			i = x_old + (y_old * w_old);
			p2 = data_old + (i * 3);

			r2 = g2 = b2 = 0;
			yw = hw;
			y3 = y2;
			while (yw) {
				xw = ww;
				x3 = x2;
				p3 = p2;
				r = g = b = a = 0;
				while (xw) {

					if ((0x100 - x3) < xw)
						i = 0x100 - x3;
					else
						i = xw;


                                        r += p3[0] * i;
                                        g += p3[1] * i;
                                        b += p3[2] * i;

                                        if (pixbuf->has_alpha) {
                                                a += p3[3] * i;
                                        }
                                        
					p3 += pixbuf->n_channels;
					xw -= i;
					x3 = 0;
				}
				if ((0x100 - y3) < yw) {
					r2 += r * (0x100 - y3);
					g2 += g * (0x100 - y3);
					b2 += b * (0x100 - y3);
                                        if (pixbuf->has_alpha) {
                                                a2 += a * (0x100 - y3);
                                        }
					yw -= 0x100 - y3;
				} else {
					r2 += r * yw;
					g2 += g * yw;
					b2 += b * yw;
                                        if (pixbuf->has_alpha) {
                                                a2 += a * yw;
                                        }
					yw = 0;
				}
				y3 = 0;
				p2 += rowstride_old;
			}
                        r2 /= ww * hw;
                        g2 /= ww * hw;
                        b2 /= ww * hw;

                        *(p++) = r2 & 0xff;
                        *(p++) = g2 & 0xff;
                        *(p++) = b2 & 0xff;
                        
                        if (pixbuf->has_alpha) {
                                a2 /= ww * hw;
                                *(p++) = a2 & 0xff;
                        }
		}
	}

	return retval;
}
