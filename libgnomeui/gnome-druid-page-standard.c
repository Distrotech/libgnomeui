/* gnome-druid-page-standard.c
 * Copyright (C) 1999  Red Hat, Inc.
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

#include <config.h>

#include <gnome.h>
#include "gnome-druid.h"
#include "gnome-druid-page-standard.h"
static void gnome_druid_page_standard_init		(GnomeDruidPageStandard		 *druid_page_standard);
static void gnome_druid_page_standard_class_init	(GnomeDruidPageStandardClass	 *klass);
static void gnome_druid_page_standard_construct     (GnomeDruidPageStandard             *druid_page_standard);
static void gnome_druid_page_standard_finalize          (GtkObject                       *widget);
static void gnome_druid_page_standard_configure_size(GnomeDruidPageStandard             *druid_page_standard,
						  gint                             width,
						  gint                             height);
static void gnome_druid_page_standard_size_allocate     (GtkWidget                       *widget,
							 GtkAllocation                   *allocation);
static void gnome_druid_page_standard_realize           (GtkWidget                       *widget);
static void gnome_druid_page_standard_prepare (GnomeDruidPage *page,
				   GtkWidget *druid,
				   gpointer *data);


static GnomeDruidPageClass *parent_class = NULL;

#define LOGO_WIDTH 50.0
#define DRUID_PAGE_WIDTH 516
GtkType
gnome_druid_page_standard_get_type (void)
{
  static GtkType druid_page_standard_type = 0;

  if (!druid_page_standard_type)
    {
      static const GtkTypeInfo druid_page_standard_info =
      {
        "GnomeDruidPageStandard",
        sizeof (GnomeDruidPageStandard),
        sizeof (GnomeDruidPageStandardClass),
        (GtkClassInitFunc) gnome_druid_page_standard_class_init,
        (GtkObjectInitFunc) gnome_druid_page_standard_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      druid_page_standard_type = gtk_type_unique (gnome_druid_page_get_type (), &druid_page_standard_info);
    }

  return druid_page_standard_type;
}

static void
gnome_druid_page_standard_class_init (GnomeDruidPageStandardClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
	object_class = (GtkObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;
	object_class->finalize = gnome_druid_page_standard_finalize;
	widget_class->size_allocate = gnome_druid_page_standard_size_allocate;
	widget_class->realize = gnome_druid_page_standard_realize;

	parent_class = gtk_type_class (gnome_druid_page_get_type ());

}
static void
gnome_druid_page_standard_init (GnomeDruidPageStandard *druid_page_standard)
{
	GtkRcStyle *rc_style;
	GtkWidget *vbox;
	GtkWidget *hbox;

	/* initialize the color values */
	druid_page_standard->background_color.red = 6400; /* midnight blue */
	druid_page_standard->background_color.green = 6400;
	druid_page_standard->background_color.blue = 28672;
	druid_page_standard->logo_background_color.red = 65280; /* white */
	druid_page_standard->logo_background_color.green = 65280;
	druid_page_standard->logo_background_color.blue = 65280;
	druid_page_standard->title_color.red = 65280; /* white */
	druid_page_standard->title_color.green = 65280;
	druid_page_standard->title_color.blue = 65280;

	/* Set up the widgets */
	vbox = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 0);
	druid_page_standard->vbox = gtk_vbox_new (FALSE, 0);
	
	gtk_widget_push_visual (gdk_imlib_get_visual ());
	gtk_widget_push_colormap (gdk_imlib_get_colormap ());
	druid_page_standard->canvas = gnome_canvas_new ();
	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();
	
	druid_page_standard->side_bar = gtk_drawing_area_new ();
	druid_page_standard->bottom_bar = gtk_drawing_area_new ();
	druid_page_standard->right_bar = gtk_drawing_area_new ();
	gtk_drawing_area_size (GTK_DRAWING_AREA (druid_page_standard->side_bar),
			       15, 10);
	gtk_drawing_area_size (GTK_DRAWING_AREA (druid_page_standard->bottom_bar),
			       10, 1);
	gtk_drawing_area_size (GTK_DRAWING_AREA (druid_page_standard->right_bar),
			       1, 10);
	rc_style = gtk_rc_style_new ();
	rc_style->bg[GTK_STATE_NORMAL].red = 6400;
	rc_style->bg[GTK_STATE_NORMAL].green = 6400;
	rc_style->bg[GTK_STATE_NORMAL].blue = 28672;
	rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BG;
	gtk_widget_modify_style (druid_page_standard->side_bar, rc_style);
	gtk_widget_modify_style (druid_page_standard->bottom_bar, rc_style);
	gtk_widget_modify_style (druid_page_standard->right_bar, rc_style);
	gtk_rc_style_unref (rc_style);

	/* FIXME: can I just ref the old style? */
	rc_style = gtk_rc_style_new ();
	rc_style->bg[GTK_STATE_NORMAL].red = 6400;
	rc_style->bg[GTK_STATE_NORMAL].green = 6400;
	rc_style->bg[GTK_STATE_NORMAL].blue = 28672;
	rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BG;
	gtk_widget_modify_style (druid_page_standard->canvas, rc_style);
	gtk_rc_style_unref (rc_style);
	gtk_box_pack_start (GTK_BOX (vbox), druid_page_standard->canvas, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (vbox), druid_page_standard->bottom_bar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), druid_page_standard->side_bar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), druid_page_standard->vbox, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), druid_page_standard->right_bar, FALSE, FALSE, 0);
	gtk_widget_set_usize (druid_page_standard->canvas, 508, LOGO_WIDTH + GNOME_PAD * 2);
	gtk_container_set_border_width (GTK_CONTAINER (druid_page_standard), 0);
	gtk_container_add (GTK_CONTAINER (druid_page_standard), vbox);
	gtk_widget_show_all (vbox);
}

static void
gnome_druid_page_standard_finalize (GtkObject *object)
{
	GnomeDruidPageStandard *druid_page_standard = GNOME_DRUID_PAGE_STANDARD (object);

	g_free (druid_page_standard->title);
	druid_page_standard->title = NULL;
	(* GTK_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gnome_druid_page_standard_configure_size (GnomeDruidPageStandard *druid_page_standard, gint width, gint height)
{
	gnome_canvas_item_set (druid_page_standard->background_item,
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", (gfloat) width,
			       "y2", (gfloat) LOGO_WIDTH + GNOME_PAD * 2,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_standard->logoframe_item,
			       "x1", (gfloat) width - LOGO_WIDTH - GNOME_PAD,
			       "y1", (gfloat) GNOME_PAD,
			       "x2", (gfloat) width - GNOME_PAD,
			       "y2", (gfloat) GNOME_PAD + LOGO_WIDTH,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_standard->logo_item,
			       "x", (gfloat) width - GNOME_PAD - LOGO_WIDTH,
			       "y", (gfloat) GNOME_PAD,
			       "anchor", GTK_ANCHOR_NORTH_WEST,
			       "width", (gfloat) LOGO_WIDTH,
			       "height", (gfloat) LOGO_WIDTH, NULL);
}
static void
gnome_druid_page_standard_construct (GnomeDruidPageStandard *druid_page_standard)
{
	/* set up the rest of the page */
	druid_page_standard->background_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_standard->canvas)),
				       gnome_canvas_rect_get_type (), NULL);

	druid_page_standard->logoframe_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_standard->canvas)),
				       gnome_canvas_rect_get_type (), NULL);

	druid_page_standard->logo_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_standard->canvas)),
				       gnome_canvas_image_get_type (), NULL);
	if (druid_page_standard->logo_image != NULL) {
		gnome_canvas_item_set (druid_page_standard->logo_item,
				       "image", druid_page_standard->logo_image, NULL);
	}
	druid_page_standard->title_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_standard->canvas)),
				       gnome_canvas_text_get_type (), 
				       "text", druid_page_standard->title,
				       "font", "-adobe-helvetica-bold-r-normal-*-*-180-*-*-p-*-iso8859-1",
				       "fontset", "-adobe-helvetica-bold-r-normal-*-*-180-*-*-p-*-iso8859-1,*-r-*",
				       NULL);
	gnome_canvas_item_set (druid_page_standard->title_item,
			       "x", 15.0,
			       "y", (gfloat) GNOME_PAD + LOGO_WIDTH / 2.0,
			       "anchor", GTK_ANCHOR_WEST,
			       NULL);

	gnome_druid_page_standard_configure_size (druid_page_standard, DRUID_PAGE_WIDTH, GNOME_PAD * 2 + LOGO_WIDTH);
	gtk_signal_connect (GTK_OBJECT (druid_page_standard),
			    "prepare",
			    gnome_druid_page_standard_prepare,
			    NULL);

}
static void
gnome_druid_page_standard_prepare (GnomeDruidPage *page,
				   GtkWidget *druid,
				   gpointer *data)
{
	gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), TRUE, TRUE, TRUE);
	gnome_druid_set_show_finish (GNOME_DRUID (druid), FALSE);
	gtk_widget_grab_default (GNOME_DRUID (druid)->next);
}

static void
gnome_druid_page_standard_size_allocate (GtkWidget *widget,
					 GtkAllocation *allocation)
{
	GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (GNOME_DRUID_PAGE_STANDARD (widget)->canvas),
					0.0, 0.0,
					allocation->width,
					allocation->height);
	gnome_druid_page_standard_configure_size (GNOME_DRUID_PAGE_STANDARD (widget),
						  allocation->width,
						  allocation->height);
}
static void
gnome_druid_page_standard_realize (GtkWidget *widget)
{
	GnomeDruidPageStandard *druid_page_standard;
	GdkColormap *cmap = gdk_imlib_get_colormap ();

	druid_page_standard = GNOME_DRUID_PAGE_STANDARD (widget);
	gdk_colormap_alloc_color (cmap, &druid_page_standard->background_color, FALSE, TRUE);
	gdk_colormap_alloc_color (cmap, &druid_page_standard->logo_background_color, FALSE, TRUE);
	gdk_colormap_alloc_color (cmap, &druid_page_standard->title_color, FALSE, TRUE);

	gnome_canvas_item_set (druid_page_standard->background_item,
			       "fill_color_gdk", &(druid_page_standard->background_color),
			       NULL);
	gnome_canvas_item_set (druid_page_standard->logoframe_item,
			       "fill_color_gdk", &druid_page_standard->logo_background_color,
			       NULL);
	gnome_canvas_item_set (druid_page_standard->title_item,
			       "fill_color_gdk", &druid_page_standard->title_color,
			       NULL);
	GTK_WIDGET_CLASS (parent_class)->realize (widget);
}


GtkWidget *
gnome_druid_page_standard_new (void)
{
	GtkWidget *retval = GTK_WIDGET (gtk_type_new (gnome_druid_page_standard_get_type ()));
	GNOME_DRUID_PAGE_STANDARD (retval)->title = g_strdup ("");
	GNOME_DRUID_PAGE_STANDARD (retval)->logo_image = NULL;
	gnome_druid_page_standard_construct (GNOME_DRUID_PAGE_STANDARD (retval));
	return retval;
}
GtkWidget *
gnome_druid_page_standard_new_with_vals (const gchar *title, GdkImlibImage *logo)
{
	GtkWidget *retval = GTK_WIDGET (gtk_type_new (gnome_druid_page_standard_get_type ()));
	GNOME_DRUID_PAGE_STANDARD (retval)->title = g_strdup (title);
	GNOME_DRUID_PAGE_STANDARD (retval)->logo_image = logo;
	gnome_druid_page_standard_construct (GNOME_DRUID_PAGE_STANDARD (retval));
	return retval;
}
void
gnome_druid_page_standard_set_bg_color      (GnomeDruidPageStandard *druid_page_standard,
					     GdkColor *color)
{
	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	if (GTK_WIDGET_REALIZED (druid_page_standard)) {
		GdkColormap *cmap = gdk_imlib_get_colormap ();

		gdk_colormap_free_colors (cmap, &druid_page_standard->background_color, 1);
	}
	druid_page_standard->background_color.red = color->red;
	druid_page_standard->background_color.green = color->green;
	druid_page_standard->background_color.blue = color->blue;

	if (GTK_WIDGET_REALIZED (druid_page_standard)) {
		GtkStyle *style;
		GdkColormap *cmap = gdk_imlib_get_colormap ();

		gdk_colormap_alloc_color (cmap, &druid_page_standard->background_color, FALSE, TRUE);

		style = gtk_style_copy (gtk_widget_get_style (druid_page_standard->side_bar));
		style->bg[GTK_STATE_NORMAL].red = color->red;
		style->bg[GTK_STATE_NORMAL].green = color->green;
		style->bg[GTK_STATE_NORMAL].blue = color->blue;
		gtk_widget_set_style (druid_page_standard->side_bar, style);
		gtk_widget_set_style (druid_page_standard->bottom_bar, style);
		gtk_widget_set_style (druid_page_standard->right_bar, style);

		gnome_canvas_item_set (druid_page_standard->background_item,
				       "fill_color_gdk", &druid_page_standard->background_color,
				       NULL);
	} else {
		GtkRcStyle *rc_style;

		rc_style = gtk_rc_style_new ();
		rc_style->bg[GTK_STATE_NORMAL].red = color->red;
		rc_style->bg[GTK_STATE_NORMAL].green = color->green;
		rc_style->bg[GTK_STATE_NORMAL].blue = color->blue;
		rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BG;
		gtk_rc_style_ref (rc_style);
		gtk_widget_modify_style (druid_page_standard->side_bar, rc_style);
		gtk_rc_style_ref (rc_style);
		gtk_widget_modify_style (druid_page_standard->bottom_bar, rc_style);
		gtk_rc_style_ref (rc_style);
		gtk_widget_modify_style (druid_page_standard->right_bar, rc_style);
	}
}
void
gnome_druid_page_standard_set_logo_bg_color (GnomeDruidPageStandard *druid_page_standard,
					  GdkColor *color)
{
	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	if (GTK_WIDGET_REALIZED (druid_page_standard)) {
		GdkColormap *cmap = gdk_imlib_get_colormap ();

		gdk_colormap_free_colors (cmap, &druid_page_standard->logo_background_color, 1);
	}
	druid_page_standard->logo_background_color.red = color->red;
	druid_page_standard->logo_background_color.green = color->green;
	druid_page_standard->logo_background_color.blue = color->blue;

	if (GTK_WIDGET_REALIZED (druid_page_standard)) {
		GdkColormap *cmap = gdk_imlib_get_colormap ();
		gdk_colormap_alloc_color (cmap, &druid_page_standard->logo_background_color, FALSE, TRUE);
		gnome_canvas_item_set (druid_page_standard->logoframe_item,
				       "fill_color_gdk", &druid_page_standard->logo_background_color,
				       NULL);
	}
}
void
gnome_druid_page_standard_set_title_color   (GnomeDruidPageStandard *druid_page_standard,
					  GdkColor *color)
{
	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	if (GTK_WIDGET_REALIZED (druid_page_standard)) {
		GdkColormap *cmap = gdk_imlib_get_colormap ();

		gdk_colormap_free_colors (cmap, &druid_page_standard->title_color, 1);
	}
	druid_page_standard->title_color.red = color->red;
	druid_page_standard->title_color.green = color->green;
	druid_page_standard->title_color.blue = color->blue;

	if (GTK_WIDGET_REALIZED (druid_page_standard)) {
		GdkColormap *cmap = gdk_imlib_get_colormap ();
		gdk_colormap_alloc_color (cmap, &druid_page_standard->title_color, FALSE, TRUE);
		gnome_canvas_item_set (druid_page_standard->title_item,
				       "fill_color_gdk", &druid_page_standard->title_color,
				       NULL);
	}

}
void
gnome_druid_page_standard_set_title         (GnomeDruidPageStandard *druid_page_standard,
					  const gchar *title)
{
	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	g_free (druid_page_standard->title);
	druid_page_standard->title = g_strdup (title);
	gnome_canvas_item_set (druid_page_standard->title_item,
			       "text", druid_page_standard->title,
			       NULL);
}
void
gnome_druid_page_standard_set_logo          (GnomeDruidPageStandard *druid_page_standard,
					  GdkImlibImage *logo_image)
{
	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	druid_page_standard->logo_image = logo_image;
	gnome_canvas_item_set (druid_page_standard->logo_item,
			       "image", druid_page_standard->logo_image, NULL);
}

