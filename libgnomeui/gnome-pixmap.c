/* GNOME GUI Library, second iteration of GnomePixmap
 * Copyright (C) 1997, 1998, 1999 the Free Software Foundation
 *
 * 
 * GnomePixmap v2: Havoc Pennington
 */

#include <config.h>
#include "gnome-pixmap.h"
#include <stdio.h>

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

		pixmap_type = gtk_type_unique (gtk_misc_get_type (), &pixmap_info);
	}

	return pixmap_type;
}

static void
gnome_pixmap_init (GnomePixmap *gpixmap)
{
        guint i;

        GTK_WIDGET_SET_FLAGS(GTK_WIDGET(gpixmap), GTK_NO_WINDOW);
        
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

        gnome_pixmap_set_mode(gpixmap, mode);
        
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
        GtkMisc   *misc;

        misc = GTK_MISC(gpixmap);
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

        }
}

static void
paint_with_pixmap (GnomePixmap *gpixmap, GdkRectangle *area)
{
	GtkWidget *widget;
        GdkPixmap *draw_source;
        GdkBitmap *draw_mask;
        GtkMisc   *misc;
        
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
                /* Draw nothing */

        }
}

static void
paint (GnomePixmap *gpixmap, GdkRectangle *area)
{
	GtkWidget *widget;
        
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

void
gnome_pixmap_set_mode               (GnomePixmap *gpixmap,
                                     GnomePixmapMode mode)
{
  g_return_if_fail(gpixmap != NULL);
  g_return_if_fail(GNOME_IS_PIXMAP(gpixmap));
        
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

  /* FIXME update the current pixmap contents */
  g_warning("%s not finished", __FUNCTION__);
}


/*
 * Set image data, create with image data 
 */

static void
clear_image(GnomePixmap *gpixmap, GtkStateType state)
{
        if (gpixmap->image_data[state].pixbuf != NULL) {
                gdk_pixbuf_unref(gpixmap->image_data[state].pixbuf);
                gpixmap->image_data[state].pixbuf = NULL;
                printf("Clearing pixbuf\n");
        }

        if (gpixmap->image_data[state].pixmap != NULL) {
                gdk_pixmap_unref(gpixmap->image_data[state].pixmap);
                gpixmap->image_data[state].pixmap = NULL;
                printf("Clearing pixmap\n");
        }

        if (gpixmap->image_data[state].mask != NULL) {
                gdk_bitmap_unref(gpixmap->image_data[state].mask);
                gpixmap->image_data[state].mask = NULL;
                printf("Clearing mask\n");
        }

}

static void
clear_old_images(GnomePixmap *gpixmap)
{
        guint i;
        
        i = 0;
        while (i < 5) {

                clear_image(gpixmap, i);
                
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
        
        oldwidth = GTK_WIDGET (gpixmap)->requisition.width;
        oldheight = GTK_WIDGET (gpixmap)->requisition.height;
        
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
                GTK_WIDGET (gpixmap)->requisition.width = width + GTK_MISC (gpixmap)->xpad * 2;
                GTK_WIDGET (gpixmap)->requisition.height = height + GTK_MISC (gpixmap)->ypad * 2;
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
set_pixbuf(GnomePixmap* gpixmap, GtkStateType state, GdkPixbuf* pixbuf, GdkBitmap* mask)
{
        clear_image(gpixmap, state);
        
        if (pixbuf != NULL) {
                if (pixbuf != NULL) {
                        gpixmap->image_data[state].pixbuf = pixbuf;
                        
                        gdk_pixbuf_ref(pixbuf);
                }
                
                if (mask != NULL) {
                        gpixmap->image_data[state].mask = mask;
                } else {
                        /* Create the mask */
                        if (gdk_pixbuf_get_has_alpha(pixbuf)) {
                                GdkBitmap *mask = NULL;
                                
                                mask = gdk_pixmap_new(NULL,
                                                      gdk_pixbuf_get_width(pixbuf),
                                                      gdk_pixbuf_get_height(pixbuf),
                                                      1);
                                
                                gdk_pixbuf_render_threshold_alpha(pixbuf, mask,
                                                                  0, 0, 0, 0,
                                                                  gdk_pixbuf_get_width(pixbuf),
                                                                  gdk_pixbuf_get_height(pixbuf),
                                                                  1); /* 1 means only alpha = 0 is excluded by the mask */
                                
                                gpixmap->image_data[state].mask = mask;
                        }
                }
        }
}

static void
set_pixbufs(GnomePixmap* gpixmap, GdkPixbuf* pixbufs[5], GdkBitmap* masks[5])
{
        guint i;

        gpixmap->flags &= ~GNOME_PIXMAP_USE_PIXMAP;
        gpixmap->flags |= GNOME_PIXMAP_USE_PIXBUF;
        
        i = 0;
        while (i < 5) {
                set_pixbuf(gpixmap,
                           i,
                           pixbufs ? pixbufs[i] : NULL,
                           masks ? masks[i] : NULL);
                        
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

                if (pixmaps != NULL) {
                        if (pixmaps[i] != NULL) {
                                gpixmap->image_data[i].pixmap = pixmaps[i];
                                
                                gdk_pixmap_ref(pixmaps[i]);
                        }
                }

                if (masks != NULL) {
                        if (masks[i] != NULL) {
                                gpixmap->image_data[i].mask = masks[i];
                                
                                gdk_bitmap_ref(masks[i]);
                        }
                }
                        
                ++i;
        }

        resize_to_fit(gpixmap);
}


void
gnome_pixmap_clear (GnomePixmap *gpixmap)
{
        g_return_if_fail(gpixmap != NULL);
        g_return_if_fail(GNOME_IS_PIXMAP(gpixmap));
        
        clear_old_images(gpixmap);
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

void
gnome_pixmap_set_pixbuf               (GnomePixmap *gpixmap, GtkStateType state, GdkPixbuf *pixbuf)
{
        g_return_if_fail(gpixmap != NULL);
        g_return_if_fail(GNOME_IS_PIXMAP(gpixmap));

        set_pixbuf(gpixmap, state, pixbuf, NULL);
        
        resize_to_fit(gpixmap);
}

void
gnome_pixmap_set_pixbuf_at_size       (GnomePixmap *gpixmap, GtkStateType state, GdkPixbuf *pixbuf, gint width, gint height)
{
        g_return_if_fail(gpixmap != NULL);
        g_return_if_fail(GNOME_IS_PIXMAP(gpixmap));

        if (width < 0)
                width = gdk_pixbuf_get_width(pixbuf);
        if (height < 0)
                height = gdk_pixbuf_get_height(pixbuf);

        if (width != gdk_pixbuf_get_width(pixbuf) ||
            height != gdk_pixbuf_get_height(pixbuf)) {

                GdkPixbuf *scaled;
                
                scaled = gnome_pixbuf_scale(pixbuf, width, height);
                
                set_pixbuf(gpixmap, state, scaled, NULL);

                gdk_pixbuf_unref(scaled);

        } else {
                
                set_pixbuf(gpixmap, state, pixbuf, NULL);
                
        }
                
        resize_to_fit(gpixmap);
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
                        /* just forget the original pixbuf, we didn't
                           own a reference yet */
                        pixbuf = gnome_pixbuf_scale(pixbuf, width, height);
                }
                
                gnome_pixmap_set_pixbuf(gpixmap, GTK_STATE_NORMAL, pixbuf);
        }

        return widget;
}


GtkWidget*
gnome_pixmap_new_from_file_at_size          (const gchar *filename, gint width, gint height)
{
        GtkWidget *widget = NULL;
        GdkPixbuf *pixbuf;

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
gnome_pixmap_new_from_xpm_d_at_size (const char **xpm_data, int width, int height)
{
        GtkWidget *widget = NULL;
        GdkPixbuf *pixbuf;
        
        pixbuf = gdk_pixbuf_new_from_xpm_data(xpm_data);
        
        if (pixbuf != NULL) {
                widget = gnome_pixmap_new_from_pixbuf_at_size(pixbuf, width, height);
                
                gdk_pixbuf_unref(pixbuf);
        }

        return widget;
}

GtkWidget*
gnome_pixmap_new_from_xpm_d         (const char **xpm_data)
{
        return gnome_pixmap_new_from_xpm_d_at_size(xpm_data, -1, -1);
}

/*
 * Build insensitive 
 */

static void
build_insensitive_pixbuf      (GnomePixmap *gpixmap)
{
        GdkPixbuf *normal;
        GdkBitmap *normal_mask;
        gint32 red, green, blue;
        GdkPixbuf *insensitive;
        GdkColor color, c;
        gint w, h, x, y;
        GtkStyle *style;
        GtkWidget *widget;
 
        
        widget = GTK_WIDGET (gpixmap);

        g_return_if_fail(widget != NULL);

        normal = gpixmap->image_data[GTK_STATE_NORMAL].pixbuf;
        normal_mask = gpixmap->image_data[GTK_STATE_NORMAL].mask;

        if (normal == NULL)
                return; /* Give up */

        g_return_if_fail(gpixmap->image_data[GTK_STATE_INSENSITIVE].pixbuf == NULL);
        g_return_if_fail(gpixmap->image_data[GTK_STATE_INSENSITIVE].mask == NULL);
        
        style = gtk_widget_get_style(widget);
        
        color = style->bg[0];
        red = color.red/255;
        green = color.green/255;
        blue = color.blue/255;

        insensitive = gnome_pixbuf_build_insensitive(normal, red, green, blue);

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

GdkPixbuf*
gnome_pixbuf_scale(GdkPixbuf* gdk_pixbuf,
                   gint w, gint h)
{
        ArtPixBuf* retval;
        ArtPixBuf* pixbuf;
	unsigned char *data, *p, *p2, *p3, *data_old;
        long w_old, h_old;
        long rowstride_old;
	long x, y, w2, h2, x_old, y_old, w_old3, x2, y2, i, x3, y3;
	long yw, xw, ww, hw, r, g, b, a, r2, g2, b2, a2;
	int new_channels;
	int new_rowstride;
        
	g_return_val_if_fail (gdk_pixbuf != NULL, NULL);

        pixbuf = gdk_pixbuf->art_pixbuf;
        
	g_return_val_if_fail (pixbuf->format == ART_PIX_RGB, NULL);
	g_return_val_if_fail (pixbuf->n_channels == 3 || pixbuf->n_channels == 4, NULL);
	g_return_val_if_fail (pixbuf->bits_per_sample == 8, NULL);
        g_return_val_if_fail (w > 0, NULL);
        g_return_val_if_fail (h > 0, NULL);
        
        w_old = pixbuf->width;
        h_old = pixbuf->height;

        /* optimization if no scaling is needed */
        if (w_old == w && h_old == h) {
                GdkPixbuf *copy;

                copy = gdk_pixbuf_new(gdk_pixbuf_get_format(gdk_pixbuf),
                                      gdk_pixbuf_get_has_alpha(gdk_pixbuf),
                                      gdk_pixbuf_get_bits_per_sample(gdk_pixbuf),
                                      gdk_pixbuf_get_width(gdk_pixbuf),
                                      gdk_pixbuf_get_height(gdk_pixbuf));

                memcpy(copy->art_pixbuf->pixels,
                       gdk_pixbuf->art_pixbuf->pixels,
                       copy->art_pixbuf->height * copy->art_pixbuf->rowstride);

                return copy;
        }
        
        rowstride_old = pixbuf->rowstride;
        data_old = pixbuf->pixels;
        
	/* Always align rows to 32-bit boundaries */

	new_channels = pixbuf->has_alpha ? 4 : 3;
	new_rowstride = 4 * ((new_channels * w + 3) / 4);

        data = g_malloc (h * new_rowstride);

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

			r2 = g2 = b2 = a2 = 0;
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

	return gdk_pixbuf_new_from_art_pixbuf(retval);
}

GdkPixbuf *
gnome_pixbuf_build_insensitive(GdkPixbuf* normal,
                               guchar red_bg, guchar green_bg, guchar blue_bg)
{
        gint32 red, green, blue;
        GdkPixbuf *insensitive;
        gint w, h, x, y;

        g_return_val_if_fail(normal != NULL, NULL);

        w = gdk_pixbuf_get_width(normal);
        h = gdk_pixbuf_get_height(normal);
        
        insensitive = gdk_pixbuf_new(gdk_pixbuf_get_format(normal),
                                     gdk_pixbuf_get_has_alpha(normal),
                                     gdk_pixbuf_get_bits_per_sample(normal),
                                     w, h);
        
        red = red_bg;
        green = green_bg;
        blue = blue_bg;
        
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

        return insensitive;
}

void
gnome_pixbuf_render(GdkPixbuf *pixbuf,
                    GdkPixmap **pixmap,
                    GdkBitmap **mask_retval)
{
        GdkBitmap *mask = NULL;
        
        g_return_if_fail(pixbuf != NULL);
        
        /* generate mask */
        if (gdk_pixbuf_get_has_alpha(pixbuf)) {
                mask = gdk_pixmap_new(NULL,
                                      gdk_pixbuf_get_width(pixbuf),
                                      gdk_pixbuf_get_height(pixbuf),
                                      1);
                
                gdk_pixbuf_render_threshold_alpha(pixbuf, mask,
                                                  0, 0, 0, 0,
                                                  gdk_pixbuf_get_width(pixbuf),
                                                  gdk_pixbuf_get_height(pixbuf),
                                                  1); /* 1 means only alpha = 0 is excluded by the mask */
        }

        /* Draw to pixmap */

        if (pixmap != NULL) {
                GdkGC* gc;
                
                *pixmap = gdk_pixmap_new(NULL,
                                         gdk_pixbuf_get_width(pixbuf),
                                         gdk_pixbuf_get_height(pixbuf),
                                         gdk_rgb_get_visual()->depth);

                gc = gdk_gc_new(*pixmap);

                gdk_gc_set_clip_mask(gc, mask);
                
                gdk_pixbuf_render_to_drawable(pixbuf, *pixmap,
                                              gc,
                                              0, 0, 0, 0,
                                              gdk_pixbuf_get_width(pixbuf),
                                              gdk_pixbuf_get_height(pixbuf),
                                              GDK_RGB_DITHER_NORMAL,
                                              0, 0);

                gdk_gc_unref(gc);
        }

        if (mask_retval)
                *mask_retval = mask;
        else
                gdk_bitmap_unref(mask);
}

