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

#include <gnome.h>
#include "gnome-druid-page-edge.h"
#include "gnome-canvas-pixbuf.h"
#include "gnome-druid.h"

struct _GnomeDruidPageEdgePrivate
{
	GtkWidget *canvas;
	GnomeCanvasItem *background_item;
	GnomeCanvasItem *textbox_item;
	GnomeCanvasItem *text_item;
	GnomeCanvasItem *logo_item;
	GnomeCanvasItem *logoframe_item;
	GnomeCanvasItem *watermark_item;
	GnomeCanvasItem *title_item;
};

static void gnome_druid_page_edge_init   	(GnomeDruidPageEdge		*druid_page_edge);
static void gnome_druid_page_edge_class_init	(GnomeDruidPageEdgeClass	*klass);
static void gnome_druid_page_edge_destroy 	(GtkObject                      *object);
static void gnome_druid_page_edge_finalize 	(GObject                        *object);
static void gnome_druid_page_edge_setup         (GnomeDruidPageEdge             *druid_page_edge);
static void gnome_druid_page_edge_configure_canvas(GnomeDruidPage               *druid_page);
static void gnome_druid_page_edge_size_allocate (GtkWidget                      *widget,
						 GtkAllocation                  *allocation);
static void gnome_druid_page_edge_prepare	(GnomeDruidPage		        *page,
						 GtkWidget                      *druid,
						 gpointer 			*data);
static void gnome_druid_page_edge_set_sidebar_shown (GnomeDruidPage		*druid_page,
						 gboolean			 sidebar_shown);

static GnomeDruidPageClass *parent_class = NULL;

#define LOGO_WIDTH 50.0
#define DRUID_PAGE_HEIGHT 318
#define DRUID_PAGE_WIDTH 516
#define DRUID_PAGE_LEFT_WIDTH 100.0
#define GDK_COLOR_TO_RGBA(color) GNOME_CANVAS_COLOR ((color).red/256, (color).green/256, (color).blue/256)


GtkType
gnome_druid_page_edge_get_type (void)
{
	static GtkType druid_page_edge_type = 0;

	if (druid_page_edge_type == 0) {
		static const GtkTypeInfo druid_page_edge_info = {
			"GnomeDruidPageEdge",
			sizeof (GnomeDruidPageEdge),
			sizeof (GnomeDruidPageEdgeClass),
			(GtkClassInitFunc) gnome_druid_page_edge_class_init,
			(GtkObjectInitFunc) gnome_druid_page_edge_init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL
		};

		druid_page_edge_type =
			gtk_type_unique (gnome_druid_page_get_type (),
					 &druid_page_edge_info);
	}

	return druid_page_edge_type;
}

static void
gnome_druid_page_edge_class_init (GnomeDruidPageEdgeClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	GnomeDruidPageClass *page_class;

	object_class = (GtkObjectClass*) klass;
	gobject_class = (GObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;
	page_class = (GnomeDruidPageClass*) klass;

	parent_class = gtk_type_class (gnome_druid_page_get_type ());

	page_class->configure_canvas = gnome_druid_page_edge_configure_canvas;
	page_class->set_sidebar_shown = gnome_druid_page_edge_set_sidebar_shown;

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

void
gnome_druid_page_edge_construct (GnomeDruidPageEdge *druid_page_edge,
				 GnomeEdgePosition   position,
				 gboolean            antialiased,
				 const gchar        *title,
				 const gchar        *text,
				 GdkPixbuf          *logo,
				 GdkPixbuf          *watermark)
{
	GnomeCanvas *canvas;

	g_return_if_fail (druid_page_edge != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (position >= GNOME_EDGE_START &&
			  position < GNOME_EDGE_LAST);

	gnome_druid_page_construct (GNOME_DRUID_PAGE (druid_page_edge), antialiased);

	druid_page_edge->position = position;
	druid_page_edge->title = g_strdup (title ? title : "");
	druid_page_edge->text = g_strdup (text ? text : "");

	if (logo != NULL)
		gdk_pixbuf_ref (logo);
	druid_page_edge->logo_image = logo;

	if (watermark != NULL)
		gdk_pixbuf_ref (watermark);
	druid_page_edge->watermark_image = watermark;

	canvas = gnome_druid_page_get_canvas (GNOME_DRUID_PAGE (druid_page_edge));

	/* Set up the canvas */
	gtk_container_set_border_width (GTK_CONTAINER (druid_page_edge), 0);
	gtk_widget_set_usize (GTK_WIDGET (canvas), DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_widget_show (GTK_WIDGET (canvas));
	gnome_canvas_set_scroll_region (canvas, 0.0, 0.0, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_container_add (GTK_CONTAINER (druid_page_edge), GTK_WIDGET (canvas));

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

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

static void
gnome_druid_page_edge_finalize(GObject *object)
{
	GnomeDruidPageEdge *druid_page_edge = GNOME_DRUID_PAGE_EDGE(object);

	g_free(druid_page_edge->_priv);
	druid_page_edge->_priv = NULL;

	if(G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize)(object);
}

static void
gnome_druid_page_edge_configure_canvas (GnomeDruidPage *druid_page)
{
	GnomeDruidPageEdge *druid_page_edge;
	gfloat watermark_width;
	gfloat watermark_height;
	gfloat watermark_ypos;
	int width, height;
	gboolean sidebar_shown;

	druid_page_edge = GNOME_DRUID_PAGE_EDGE (druid_page);

	width = GTK_WIDGET (druid_page)->allocation.width;
	height = GTK_WIDGET (druid_page)->allocation.height;

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

	sidebar_shown = gnome_druid_page_get_sidebar_shown (druid_page);

	if ( ! sidebar_shown) {
		watermark_width = 0.0;
		watermark_height = 0.0;
	}

	gnome_canvas_item_set (druid_page_edge->_priv->background_item,
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", (gfloat) width,
			       "y2", (gfloat) height,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_edge->_priv->textbox_item,
			       "x1", watermark_width,
			       "y1", LOGO_WIDTH + GNOME_PAD * 2.0,
			       "x2", (gfloat) width,
			       "y2", (gfloat) height,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_edge->_priv->logoframe_item,
			       "x1", (gfloat) width - LOGO_WIDTH -GNOME_PAD,
			       "y1", (gfloat) GNOME_PAD,
			       "x2", (gfloat) width - GNOME_PAD,
			       "y2", (gfloat) GNOME_PAD + LOGO_WIDTH,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_edge->_priv->logo_item,
			       "x", (gfloat) width - GNOME_PAD - LOGO_WIDTH,
			       "y", (gfloat) GNOME_PAD,
			       "width", (gfloat) LOGO_WIDTH,
			       "height", (gfloat) LOGO_WIDTH, NULL);
	if (sidebar_shown) {
		gnome_canvas_item_show (druid_page_edge->_priv->watermark_item);
		gnome_canvas_item_set (druid_page_edge->_priv->watermark_item,
				       "x", 0.0,
				       "y", watermark_ypos,
				       "width", watermark_width,
				       "height", watermark_height,
				       NULL);
	} else {
		gnome_canvas_item_hide (druid_page_edge->_priv->watermark_item);
	}
	gnome_canvas_item_set (druid_page_edge->_priv->title_item,
			       "x", 15.0,
			       "y", (gfloat) GNOME_PAD + LOGO_WIDTH / 2.0,
			       "anchor", GTK_ANCHOR_WEST,
			       NULL);
	gnome_canvas_item_set (druid_page_edge->_priv->text_item,
			       "x", ((width - watermark_width) * 0.5) + watermark_width,
			       "y", LOGO_WIDTH + GNOME_PAD * 2.0 + (height - (LOGO_WIDTH + GNOME_PAD * 2.0))/ 2.0,
			       "anchor", GTK_ANCHOR_CENTER,
			       NULL);
}

static void
gnome_druid_page_edge_setup (GnomeDruidPageEdge *druid_page_edge)
{
	GnomeCanvas *canvas;
	guint32 fill_color, outline_color;

	canvas = gnome_druid_page_get_canvas (GNOME_DRUID_PAGE (druid_page_edge));

	/* set up the rest of the page */
	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->background_color);
	druid_page_edge->_priv->background_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_rect_get_type (),
				       "fill_color_rgba", fill_color,
				       NULL);

	outline_color = fill_color;
	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->textbox_color);
	druid_page_edge->_priv->textbox_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_rect_get_type (),
				       "fill_color_rgba", fill_color,
				       "outline_color_rgba", outline_color,
				       NULL);

	fill_color = GDK_COLOR_TO_RGBA (druid_page_edge->logo_background_color);
	druid_page_edge->_priv->logoframe_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_rect_get_type (),
				       "fill_color_rgba", fill_color,
				       NULL);

	druid_page_edge->_priv->logo_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_pixbuf_get_type (),
				       "x_set", TRUE, "y_set", TRUE,
				       NULL);

	if (druid_page_edge->logo_image != NULL)
		gnome_canvas_item_set (druid_page_edge->_priv->logo_item,
				       "pixbuf", druid_page_edge->logo_image, NULL);

	druid_page_edge->_priv->watermark_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_pixbuf_get_type (),
				       "x_set", TRUE, "y_set", TRUE,
				       NULL);

	if (druid_page_edge->watermark_image != NULL)
		gnome_canvas_item_set (druid_page_edge->_priv->watermark_item,
				       "pixbuf", druid_page_edge->watermark_image, NULL);

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
			    gnome_druid_page_edge_prepare,
			    NULL);
}
static void
gnome_druid_page_edge_prepare (GnomeDruidPage *page,
			       GtkWidget *druid,
			       gpointer *data)
{
	switch (GNOME_DRUID_PAGE_EDGE (page)->position) {
	case GNOME_EDGE_START:
		gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), FALSE, TRUE, TRUE);
		gnome_druid_set_show_finish (GNOME_DRUID (druid), FALSE);
		gtk_widget_grab_default (GNOME_DRUID (druid)->next);
		break;
	case GNOME_EDGE_FINISH:
		gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), TRUE, FALSE, TRUE);
		gnome_druid_set_show_finish (GNOME_DRUID (druid), TRUE);
		gtk_widget_grab_default (GNOME_DRUID (druid)->finish);
		break;
	case GNOME_EDGE_OTHER:
	default:
		break;
	}
}


static void
gnome_druid_page_edge_size_allocate   (GtkWidget               *widget,
				       GtkAllocation           *allocation)
{
	GnomeCanvas *canvas;

	canvas = gnome_druid_page_get_canvas (GNOME_DRUID_PAGE (widget));

	GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

	gnome_canvas_set_scroll_region (canvas,
					0.0, 0.0,
					allocation->width,
					allocation->height);
	gnome_druid_page_configure_canvas (GNOME_DRUID_PAGE (widget));
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
				     GdkPixbuf *watermark)
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
					 watermark);
	return GTK_WIDGET (retval);
}

/**
 * gnome_druid_page_edge_set_bg_color:
 * @druid_page_edge: A DruidPageEdge.
 * @color: The new background color.
 *
 * This will set the background color to be the @color.  You do not
 * need to allocate the color, as the @druid_page_edge will do it for
 * you.
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

	gnome_canvas_item_set (druid_page_edge->_priv->textbox_item,
			       "outline_color_rgba", fill_color,
			       NULL);
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
gnome_druid_page_edge_set_text          (GnomeDruidPageEdge *druid_page_edge,
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
void
gnome_druid_page_edge_set_logo          (GnomeDruidPageEdge *druid_page_edge,
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
			       "pixbuf", druid_page_edge->logo_image, NULL);
}
void
gnome_druid_page_edge_set_watermark     (GnomeDruidPageEdge *druid_page_edge,
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
			       "pixbuf", druid_page_edge->watermark_image, NULL);
}

static void
gnome_druid_page_edge_set_sidebar_shown (GnomeDruidPage *druid_page, gboolean sidebar_shown)
{
	g_return_if_fail (druid_page != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_EDGE (druid_page));

	if (GNOME_DRUID_PAGE_GET_CLASS(druid_page)->set_sidebar_shown)
		GNOME_DRUID_PAGE_GET_CLASS(druid_page)->set_sidebar_shown(druid_page, sidebar_shown);

	gtk_widget_queue_resize (GTK_WIDGET (druid_page));
}

