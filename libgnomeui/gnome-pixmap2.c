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
static void gnome_pixmap_realize       (GtkWidget        *widget);
static void gnome_pixmap_size_request  (GtkWidget        *widget,
					GtkRequisition   *requisition);
static void gnome_pixmap_size_allocate (GtkWidget        *widget,
					GtkAllocation    *allocation);
static void gnome_pixmap_draw          (GtkWidget        *widget,
					GdkRectangle     *area);
static gint gnome_pixmap_expose        (GtkWidget        *widget,
					GdkEventExpose   *event);


static GtkMiscClass *parent_class;

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
        gpixmap->flags = 0;
        gpixmap->flags |= GNOME_PIXMAP_USE_PIXMAP;


        i = 0;
        while (i < 5) {
                gpixmap->image_data[i].pixbuf = NULL;
                gpixmap->image_data[i].pixmap = NULL;
                gpixmap->image_data[i].mask = NULL;

                ++i;
        }
}

static void
gnome_pixmap_class_init (GnomePixmapClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	parent_class = gtk_type_class (gtk_misc_get_type ());

	widget_class->realize = gnome_pixmap_realize;
	widget_class->size_request = gnome_pixmap_size_request;
	widget_class->size_allocate = gnome_pixmap_size_allocate;
	widget_class->draw = gnome_pixmap_draw;
	widget_class->expose_event = gnome_pixmap_expose;

	object_class->destroy = gnome_pixmap_destroy;
}

static void
gnome_pixmap_destroy (GtkObject *object)
{
	GnomePixmap *gpixmap;
        guint i;
        
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (object));

	gpixmap = GNOME_PIXMAP (object);

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
        
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_pixmap_realize (GtkWidget *widget)
{
	GnomePixmap *gpixmap;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (widget));

	gpixmap = GNOME_PIXMAP (widget);

        /* FIXME */
}

static void
gnome_pixmap_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GnomePixmap *gpixmap;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (widget));
	g_return_if_fail (requisition != NULL);

	gpixmap = GNOME_PIXMAP (widget);

        /* FIXME */
}

static void
gnome_pixmap_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_PIXMAP (widget));
	g_return_if_fail (allocation != NULL);

        /* FIXME */
}

static void
paint_with_pixbuf (GnomePixmap *gpixmap, GdkRectangle *area)
{
	GtkWidget *widget;
        GdkPixbuf *draw_source;
        
        widget = GTK_WIDGET(gpixmap);

        g_return_if_fail(GTK_WIDGET_DRAWABLE(gpixmap));

        draw_source = gpixmap->image_data[GTK_WIDGET_STATE(widget)].pixbuf;

        if (draw_source == NULL) {
                /* Try normal, if the other state isn't set */
                draw_source = gpixmap->image_data[GTK_STATE_NORMAL].pixbuf;
        }

        if (draw_source != NULL) {
                /* Draw the image */                
                
                

        } else {
                /* Draw blank space */
                
        }
}

static void
paint_with_pixmap (GnomePixmap *gpixmap, GdkRectangle *area)
{
	GtkWidget *widget;
        GdkPixmap *draw_source;
        GdkBitmap *draw_mask;
        
        widget = GTK_WIDGET(gpixmap);

        g_return_if_fail(GTK_WIDGET_DRAWABLE(gpixmap));

        draw_source = gpixmap->image_data[GTK_WIDGET_STATE(widget)].pixmap;
        draw_mask = gpixmap->image_data[GTK_WIDGET_STATE(widget)].mask;

        if (draw_source == NULL) {
                /* fall back to normal, if no special case */
                draw_source = gpixmap->image_data[GTK_STATE_NORMAL].pixmap;
                draw_mask = gpixmap->image_data[GTK_STATE_NORMAL].mask;
        }

        if (draw_source != NULL) {
                /* Draw the image */
                

        } else {
                /* Draw blank space */

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

static void
gnome_pixmap_draw (GtkWidget *widget, GdkRectangle *area)
{
	GnomePixmap *gpixmap;

	if (GTK_WIDGET_DRAWABLE (widget))
		paint (GNOME_PIXMAP (widget), area);   
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




