/* gnome-druid-page-edge.c
 * Copyright (C) 1999  Red Hat, Inc.
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
*/

#include <config.h>
#include "gnome-macros.h"

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include <libgnomecanvas/gnome-canvas-rect-ellipse.h>
#include <libgnomecanvas/gnome-canvas-text.h>
#include "gnome-druid.h"
#include "gnome-uidefs.h"

#include "gnome-druid-page-edge.h"

struct _GnomeDruidPageEdgePrivate
{
	GtkWidget *canvas;
	GnomeCanvasItem *background_item;
	GnomeCanvasItem *textbox_item;
	GnomeCanvasItem *text_item;
	GnomeCanvasItem *logo_item;
	GnomeCanvasItem *logoframe_item;
	GnomeCanvasItem *watermark_item;
	GnomeCanvasItem *top_watermark_item;
	GnomeCanvasItem *title_item;
};

static void gnome_druid_page_edge_init   	(GnomeDruidPageEdge		*druid_page_edge);
static void gnome_druid_page_edge_class_init	(GnomeDruidPageEdgeClass	*klass);
static void gnome_druid_page_edge_destroy 	(GtkObject                      *object);
static void gnome_druid_page_edge_finalize 	(GObject                        *object);
static void gnome_druid_page_edge_setup         (GnomeDruidPageEdge             *druid_page_edge);
static void gnome_druid_page_edge_configure_canvas(GnomeDruidPageEdge           *druid_page_edge);
static void gnome_druid_page_edge_size_allocate (GtkWidget                      *widget,
						 GtkAllocation                  *allocation);
static void gnome_druid_page_edge_prepare	(GnomeDruidPage		        *page,
						 GtkWidget                      *druid,
						 gpointer 			*data);

#define LOGO_WIDTH 50.0
#define DRUID_PAGE_HEIGHT 318
#define DRUID_PAGE_WIDTH 516
#define DRUID_PAGE_LEFT_WIDTH 100.0
#define GDK_COLOR_TO_RGBA(color) GNOME_CANVAS_COLOR ((color).red/256, (color).green/256, (color).blue/256)

GNOME_CLASS_BOILERPLATE (GnomeDruidPageEdge, gnome_druid_page_edge,
			 GnomeDruidPage, gnome_druid_page);

static void
gnome_druid_page_edge_class_init (GnomeDruidPageEdgeClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass*) klass;
	gobject_class = (GObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;

	object_class->destroy = gnome_druid_page_edge_destroy;
	gobject_class->finalize = gnome_druid_page_edge_finalize;
	widget_class->size_allocate = gnome_druid_page_edge_size_allocate;
}

static void
gnome_druid_page_edge_init (GnomeDruidPageEdge *druid_page_edge)
{
	druid_page_edge->_priv = g_new0(GnomeDruidPageEdgePrivate, 1);

	/* initialize the color values */
	druid_page_edge->background_color.red = 6400; /* midnight blue */
	druid_page_edge->background_color.green = 6400;
	druid_page_edge->background_color.blue = 28672;
	druid_page_edge->textbox_color.red = 65280; /* white */
	druid_page_edge->textbox_color.green = 65280;
	druid_page_edge->textbox_color.blue = 65280;
	druid_page_edge->logo_background_color.red = 65280; /* white */
	druid_page_edge->logo_background_color.green = 65280;
	druid_page_edge->logo_background_color.blue = 65280;
	druid_page_edge->title_color.red = 65280; /* white */
	druid_page_edge->title_color.green = 65280;
	druid_page_edge->title_color.blue = 65280;
	druid_page_edge->text_color.red = 0; /* black */
	druid_page_edge->text_color.green = 0;
	druid_page_edge->text_color.blue = 0;
}

/**
 * gnome_druid_page_edge_construct:
 *
 * Description:  Useful for subclassing and binding only
 **/
void
gnome_druid_page_edge_construct (GnomeDruidPageEdge *druid_page_edge,
				 GnomeEdgePosition   position,
				 gboolean            antialiased,
				 const gchar        *title,
				 const gchar        *text,
				 GdkPixbuf          *logo,
				 GdkPixbuf          *watermark,
				 GdkPixbuf          *top_watermark)
{
	GtkWidget *canvas;

	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (position >= GNOME_EDGE_START &&
			  position < GNOME_EDGE_LAST);

	if (antialiased) {
		gtk_widget_push_colormap (gdk_rgb_get_cmap ());
		canvas = gnome_canvas_new_aa();
		gtk_widget_pop_colormap ();
	} else {
		canvas = gnome_canvas_new();
	}

	druid_page_edge->_priv->canvas = canvas;

	druid_page_edge->position = position;
	druid_page_edge->title = g_strdup (title ? title : "");
	druid_page_edge->text = g_strdup (text ? text : "");

	if (logo != NULL)
		gdk_pixbuf_ref (logo);
	druid_page_edge->logo_image = logo;

	if (watermark != NULL)
		gdk_pixbuf_ref (watermark);
	druid_page_edge->watermark_image = watermark;

	if (top_watermark != NULL)
		gdk_pixbuf_ref (top_watermark);
	druid_page_edge->top_watermark_image = top_watermark;

	/* Set up the canvas */
	gtk_container_set_border_width (GTK_CONTAINER (druid_page_edge), 0);
	gtk_widget_set_usize (canvas, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_widget_show (canvas);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas), 0.0, 0.0, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_container_add (GTK_CONTAINER (druid_page_edge), canvas);

	gnome_druid_page_edge_setup (druid_page_edge);
}

static void
gnome_druid_page_edge_destroy(GtkObject *object)
{
	GnomeDruidPageEdge *druid_page_edge = GNOME_DRUID_PAGE_EDGE(object);

	/* remember, destroy can be run multiple times! */

	if (druid_page_edge->logo_image != NULL)
		gdk_pixbuf_unref (druid_page_edge->logo_image);
	druid_page_edge->logo_image = NULL;

	if (druid_page_edge->watermark_image != NULL)
		gdk_pixbuf_unref (druid_page_edge->watermark_image);
	druid_page_edge->watermark_image = NULL;

	g_free (druid_page_edge->text);
	druid_page_edge->text = NULL;
	g_free (druid_page_edge->title);
	druid_page_edge->title = NULL;

	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_druid_page_edge_finalize(GObject *object)
{
	GnomeDruidPageEdge *druid_page_edge = GNOME_DRUID_PAGE_EDGE(object);

	g_free(druid_page_edge->_priv);
	druid_page_edge->_priv = NULL;

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}

static void
gnome_druid_page_edge_configure_canvas (GnomeDruidPageEdge *druid_page_edge)
{
	gfloat watermark_width;
	gfloat watermark_height;
	gfloat watermark_ypos;
	int width, height;

	width = GTK_WIDGET (druid_page_edge)->allocation.width;
	height = GTK_WIDGET (druid_page_edge)->allocation.height;

	watermark_width = DRUID_PAGE_LEFT_WIDTH;
	watermark_height = (gfloat) height - LOGO_WIDTH + GNOME_PAD * 2.0;
	watermark_ypos = LOGO_WIDTH + GNOME_PAD * 2.0;

	if (druid_page_edge->watermark_image) {
		watermark_width = gdk_pixbuf_get_width (druid_page_edge->watermark_image);
		watermark_height = gdk_pixbuf_get_height (druid_page_edge->watermark_image);
		watermark_ypos = (gfloat) height - watermark_height;
		if (watermark_width < 1)
			watermark_width = 1.0;
		if (watermark_height < 1)
			watermark_height = 1.0;
	}

	gnome_canvas_item_set (druid_page_edge->_priv->background_item,
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", (double) width,
			       "y2", (double) height,
			       NULL);
	gnome_canvas_item_set (druid_page_edge->_priv->textbox_item,
			       "x1", (double) watermark_width,
			       "y1", (double) (LOGO_WIDTH + GNOME_PAD * 2.0),
			       "x2", (double) width,
			       "y2", (double) height,
			       NULL);
	if (druid_page_edge->logo_image != NULL) {
		gnome_canvas_item_show (druid_page_edge->_priv->logoframe_item);
		gnome_canvas_item_set (druid_page_edge->_priv->logoframe_item,
				       "x1", (double) (width - LOGO_WIDTH -GNOME_PAD),
				       "y1", (double) (GNOME_PAD),
				       "x2", (double) (width - GNOME_PAD),
				       "y2", (double) (GNOME_PAD + LOGO_WIDTH),
				       "width_units", 1.0,
				       NULL);
	} else {
		gnome_canvas_item_hide (druid_page_edge->_priv->logoframe_item);
	}
	gnome_canvas_item_set (druid_page_edge->_priv->logo_item,
			       "x", (double) (width - GNOME_PAD - LOGO_WIDTH),
			       "y", (double) (GNOME_PAD),
			       "width", (double) (LOGO_WIDTH),
			       "height", (double) (LOGO_WIDTH),
			       NULL);
	gnome_canvas_item_set (druid_page_edge->_priv->watermark_item,
			       "x", 0.0,
			       "y", (double) watermark_ypos,
			       "width", (double) watermark_width,
			       "height", (double) watermark_height,
			       NULL);

	gnome_canvas_item_set (druid_page_edge->_priv->title_item,
			       "x", 15.0,
			       "y", (double) (GNOME_PAD + LOGO_WIDTH / 2.0),
			       "anchor", GTK_ANCHOR_WEST,
			       NULL);
	gnome_canvas_item_set (druid_page_edge->_priv->text_item,
			       "x", (double) (((width - watermark_width) * 0.5) + watermark_width),
			       "y", (double) (LOGO_WIDTH + GNOME_PAD * 2.0 + (height - (LOGO_WIDTH + GNOME_PAD * 2.0))/ 2.0),
			       "anchor", GTK_ANCHOR_CENTER,
			       NULL);
}

static void
gnome_druid_page_edge_setup (GnomeDruidPageEdge *druid_page_edge)
{
	GnomeCanvas *canvas;
	guint32 fill_color;

	canvas = GNOME_CANVAS (druid_page_edge->_priv->canvas);

	/* set up the rest of the page */
	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->background_color);
	druid_page_edge->_priv->background_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_rect_get_type (),
				       "fill_color_rgba", fill_color,
				       NULL);

	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->textbox_color);
	druid_page_edge->_priv->textbox_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_rect_get_type (),
				       "fill_color_rgba", fill_color,
				       NULL);

	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->logo_background_color);
	druid_page_edge->_priv->logoframe_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_rect_get_type (),
				       "fill_color_rgba", fill_color,
				       NULL);
	if (druid_page_edge->logo_image == NULL) {
		gnome_canvas_item_hide (druid_page_edge->_priv->logoframe_item);
	}

	druid_page_edge->_priv->top_watermark_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_pixbuf_get_type (),
				       "x", 0.0,
				       "y", 0.0,
				       "x_set", TRUE,
				       "y_set", TRUE,
				       NULL);

	if (druid_page_edge->top_watermark_image != NULL)
		gnome_canvas_item_set (druid_page_edge->_priv->top_watermark_item,
				       "pixbuf", druid_page_edge->top_watermark_image,
				       NULL);

	druid_page_edge->_priv->logo_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_pixbuf_get_type (),
				       "x_set", TRUE,
				       "y_set", TRUE,
				       NULL);

	if (druid_page_edge->logo_image != NULL)
		gnome_canvas_item_set (druid_page_edge->_priv->logo_item,
				       "pixbuf", druid_page_edge->logo_image, NULL);

	druid_page_edge->_priv->watermark_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_pixbuf_get_type (),
				       "x_set", TRUE,
				       "y_set", TRUE,
				       NULL);

	if (druid_page_edge->watermark_image != NULL)
		gnome_canvas_item_set (druid_page_edge->_priv->watermark_item,
				       "pixbuf", druid_page_edge->watermark_image,
				       NULL);

	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->title_color);
	druid_page_edge->_priv->title_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_text_get_type (),
				       "text", druid_page_edge->title,
				       "fill_color_rgba", fill_color,
				       "fontset", _("-adobe-helvetica-bold-r-normal-*-*-180-*-*-p-*-*-*,*-r-*"),
				       NULL);

	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->text_color);
	druid_page_edge->_priv->text_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_text_get_type (),
				       "text", druid_page_edge->text,
				       "justification", GTK_JUSTIFY_LEFT,
				       "fontset", _("-adobe-helvetica-medium-r-normal-*-*-120-*-*-p-*-*-*,*-r-*"),
				       "fill_color_rgba", fill_color,
				       NULL);

	gtk_signal_connect (GTK_OBJECT (druid_page_edge),
			    "prepare",
			    GTK_SIGNAL_FUNC (gnome_druid_page_edge_prepare),
			    NULL);
}

static void
gnome_druid_page_edge_prepare (GnomeDruidPage *page,
			       GtkWidget *druid,
			       gpointer *data)
{
	switch (GNOME_DRUID_PAGE_EDGE (page)->position) {
	case GNOME_EDGE_START:
		gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), FALSE, TRUE, TRUE, TRUE);
		gnome_druid_set_show_finish (GNOME_DRUID (druid), FALSE);
		gtk_widget_grab_default (GNOME_DRUID (druid)->next);
		break;
	case GNOME_EDGE_FINISH:
		gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), TRUE, FALSE, TRUE, TRUE);
		gnome_druid_set_show_finish (GNOME_DRUID (druid), TRUE);
		gtk_widget_grab_default (GNOME_DRUID (druid)->finish);
		break;
	case GNOME_EDGE_OTHER:
		gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), TRUE, TRUE, TRUE, TRUE);
		gnome_druid_set_show_finish (GNOME_DRUID (druid), FALSE);
	default:
		break;
	}
}

static void
gnome_druid_page_edge_size_allocate   (GtkWidget               *widget,
				       GtkAllocation           *allocation)
{
	GnomeDruidPageEdge *druid_page_edge;

	druid_page_edge = GNOME_DRUID_PAGE_EDGE (widget);

	GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS, size_allocate,
				   (widget, allocation));

	gnome_canvas_set_scroll_region (GNOME_CANVAS(druid_page_edge->_priv->canvas),
					0.0, 0.0,
					allocation->width,
					allocation->height);
	gnome_druid_page_edge_configure_canvas (druid_page_edge);
}

/**
 * gnome_druid_page_edge_new:
 * @position: Position in druid
 *
 * Description: Creates a new GnomeDruidPageEdge widget.
 *
 * Returns: #GtkWidget pointer to a new #GnomeDruidPageEdge.
 **/
/* Public functions */
GtkWidget *
gnome_druid_page_edge_new (GnomeEdgePosition position)
{
	GnomeDruidPageEdge *retval;

	g_return_val_if_fail (position >= GNOME_EDGE_START &&
			      position < GNOME_EDGE_LAST, NULL);

	retval = gtk_type_new (gnome_druid_page_edge_get_type ());

	gnome_druid_page_edge_construct (retval,
					 position,
					 FALSE,
					 NULL,
					 NULL,
					 NULL,
					 NULL,
					 NULL);

	return GTK_WIDGET (retval);
}

/**
 * gnome_druid_page_edge_new_aa:
 * @position: Position in druid
 *
 * Description: Creates a new GnomeDruidPageEdge widget.  The
 * internal canvas is created in an antialiased mode.
 *
 * Returns: #GtkWidget pointer to a new #GnomeDruidPageEdge.
 **/
/* Public functions */
GtkWidget *
gnome_druid_page_edge_new_aa (GnomeEdgePosition position)
{
	GnomeDruidPageEdge *retval;

	g_return_val_if_fail (position >= GNOME_EDGE_START &&
			      position < GNOME_EDGE_LAST, NULL);

	retval = gtk_type_new (gnome_druid_page_edge_get_type ());

	gnome_druid_page_edge_construct (retval,
					 position,
					 TRUE,
					 NULL,
					 NULL,
					 NULL,
					 NULL,
					 NULL);

	return GTK_WIDGET (retval);
}

/**
 * gnome_druid_page_edge_new_with_vals:
 * @position: Position in druid
 * @antialiased: Use an antialiased canvas
 * @title: The title.
 * @text: The introduction text.
 * @logo: The logo in the upper right corner.
 * @watermark: The watermark on the left.
 * @top_watermark: The watermark on the left.
 *
 * Description: This will create a new GNOME Druid Edge page, with the values
 * given.  It is acceptable for any of them to be %NULL.
 * Position should be %GNOME_EDGE_START, %GNOME_EDGE_FINISH
 * or %GNOME_EDGE_OTHER.
 *
 * Returns: #GtkWidget pointer to a new #GnomeDruidPageEdge.
 **/
GtkWidget *
gnome_druid_page_edge_new_with_vals (GnomeEdgePosition position,
				     gboolean antialiased,
				     const gchar *title,
				     const gchar* text,
				     GdkPixbuf *logo,
				     GdkPixbuf *watermark,
				     GdkPixbuf *top_watermark)
{
	GnomeDruidPageEdge *retval;

	g_return_val_if_fail (position >= GNOME_EDGE_START &&
			      position < GNOME_EDGE_LAST, NULL);

	retval = gtk_type_new (gnome_druid_page_edge_get_type ());

	gnome_druid_page_edge_construct (retval,
					 position,
					 antialiased,
					 title,
					 text,
					 logo,
					 watermark,
					 top_watermark);
	return GTK_WIDGET (retval);
}

/**
 * gnome_druid_page_edge_set_bg_color:
 * @druid_page_edge: A DruidPageEdge.
 * @color: The new background color.
 *
 * Description:  This will set the background color to be the @color.  You do
 * not need to allocate the color, as the @druid_page_edge will do it for you.
 **/
void
gnome_druid_page_edge_set_bg_color      (GnomeDruidPageEdge *druid_page_edge,
					 GdkColor *color)
{
	guint fill_color;

	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (color != NULL);

	druid_page_edge->background_color.red = color->red;
	druid_page_edge->background_color.green = color->green;
	druid_page_edge->background_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->background_color);

	gnome_canvas_item_set (druid_page_edge->_priv->background_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}

void
gnome_druid_page_edge_set_textbox_color (GnomeDruidPageEdge *druid_page_edge,
					 GdkColor *color)
{
	guint fill_color;

	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (color != NULL);

	druid_page_edge->textbox_color.red = color->red;
	druid_page_edge->textbox_color.green = color->green;
	druid_page_edge->textbox_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->textbox_color);
	gnome_canvas_item_set (druid_page_edge->_priv->textbox_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}

void
gnome_druid_page_edge_set_logo_bg_color (GnomeDruidPageEdge *druid_page_edge,
					 GdkColor *color)
{
	guint fill_color;

	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (color != NULL);

	druid_page_edge->logo_background_color.red = color->red;
	druid_page_edge->logo_background_color.green = color->green;
	druid_page_edge->logo_background_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->logo_background_color);
	gnome_canvas_item_set (druid_page_edge->_priv->logoframe_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}

void
gnome_druid_page_edge_set_title_color   (GnomeDruidPageEdge *druid_page_edge,
					 GdkColor *color)
{
	guint32 fill_color;

	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (color != NULL);

	druid_page_edge->title_color.red = color->red;
	druid_page_edge->title_color.green = color->green;
	druid_page_edge->title_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->title_color);
	gnome_canvas_item_set (druid_page_edge->_priv->title_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}

void
gnome_druid_page_edge_set_text_color    (GnomeDruidPageEdge *druid_page_edge,
					 GdkColor *color)
{
	guint32 fill_color;

	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (color != NULL);

	druid_page_edge->text_color.red = color->red;
	druid_page_edge->text_color.green = color->green;
	druid_page_edge->text_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->text_color);
	gnome_canvas_item_set (druid_page_edge->_priv->text_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}

void
gnome_druid_page_edge_set_text (GnomeDruidPageEdge *druid_page_edge,
				const gchar *text)
{
	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));

	g_free (druid_page_edge->text);
	druid_page_edge->text = g_strdup (text ? text : "");
	gnome_canvas_item_set (druid_page_edge->_priv->text_item,
			       "text", druid_page_edge->text,
			       NULL);
}

void
gnome_druid_page_edge_set_title         (GnomeDruidPageEdge *druid_page_edge,
					 const gchar *title)
{
	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));

	g_free (druid_page_edge->title);
	druid_page_edge->title = g_strdup (title ? title : "");
	gnome_canvas_item_set (druid_page_edge->_priv->title_item,
			       "text", druid_page_edge->title,
			       NULL);
}

/**
 * gnome_druid_page_edge_set_logo:
 * @druid_page_edge: the #GnomeDruidPageEdge to work on
 * @logo_image: The #GdkPixbuf to use as a logo
 *
 * Description:  Sets a #GdkPixbuf as the logo in the top right corner.
 * If %NULL, then no logo will be displayed.
 **/
void
gnome_druid_page_edge_set_logo (GnomeDruidPageEdge *druid_page_edge,
				GdkPixbuf *logo_image)
{
	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (logo_image != NULL);

	if (druid_page_edge->logo_image != NULL)
		gdk_pixbuf_unref (druid_page_edge->logo_image);

	druid_page_edge->logo_image = logo_image;
	if (logo_image != NULL )
		gdk_pixbuf_ref (logo_image);
	gnome_canvas_item_set (druid_page_edge->_priv->logo_item,
			       "pixbuf", druid_page_edge->logo_image,
			       NULL);
}

/**
 * gnome_druid_page_edge_set_watermark:
 * @druid_page_edge: the #GnomeDruidPageEdge to work on
 * @top_watermark_image: The #GdkPixbuf to use as a watermark
 *
 * Description:  Sets a #GdkPixbuf as the watermark on the left
 * strip on the druid.  If #top_watermark_image is %NULL, it is reset
 * to the normal color.
 **/
void
gnome_druid_page_edge_set_watermark (GnomeDruidPageEdge *druid_page_edge,
				     GdkPixbuf *watermark)
{
	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (watermark != NULL);

	if (druid_page_edge->watermark_image != NULL)
		gdk_pixbuf_unref (druid_page_edge->watermark_image);

	druid_page_edge->watermark_image = watermark;
	if (watermark != NULL )
		gdk_pixbuf_ref (watermark);
	gnome_canvas_item_set (druid_page_edge->_priv->watermark_item,
			       "pixbuf", druid_page_edge->watermark_image,
			       NULL);
}

/**
 * gnome_druid_page_edge_set_top_watermark:
 * @druid_page_edge: the #GnomeDruidPageEdge to work on
 * @top_watermark_image: The #GdkPixbuf to use as a top watermark
 *
 * Description:  Sets a #GdkPixbuf as the watermark on top of the top
 * strip on the druid.  If #top_watermark_image is %NULL, it is reset
 * to the normal color.
 **/
void
gnome_druid_page_edge_set_top_watermark (GnomeDruidPageEdge *druid_page_edge,
					 GdkPixbuf *top_watermark_image)
{
	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));

	if (druid_page_edge->top_watermark_image)
		gdk_pixbuf_unref (druid_page_edge->top_watermark_image);

	druid_page_edge->top_watermark_image = top_watermark_image;
	if (top_watermark_image != NULL)
		gdk_pixbuf_ref (top_watermark_image);
	gnome_canvas_item_set (druid_page_edge->_priv->top_watermark_item,
			       "pixbuf", druid_page_edge->top_watermark_image,
			       NULL);
}
