/* gnome-druid-page-standard.c
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2001  James M. Cape <jcape@ignore-your.tv>
 * Copyright (C) 2001  Jonathan Blandford <jrb@alum.mit.edu>
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

#include <config.h>
#include "gnome-macros.h"

#include "gnome-druid.h"
#include "gnome-uidefs.h"
#include <libgnome/gnome-i18n.h>

/* FIXME: Are these includes needed */
#include <gtk/gtklabel.h>
#include <gtk/gtklayout.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <string.h>
#include "gnome-druid-page-standard.h"

struct _GnomeDruidPageStandardPrivate
{
	GtkWidget *top_bar;
	GtkWidget *watermark;
	GtkWidget *logo;
	GtkWidget *title_label;

	GtkWidget *left_line;
	GtkWidget *right_line;
	GtkWidget *bottom_line;

	GtkWidget *evbox;

	guint title_foreground_set : 1;
	guint background_set : 1;
	guint logo_background_set : 1;
	guint contents_background_set : 1;
};


static void gnome_druid_page_standard_init          (GnomeDruidPageStandard      *druid_page_standard);
static void gnome_druid_page_standard_class_init    (GnomeDruidPageStandardClass *class);

static void gnome_druid_page_standard_get_property  (GObject                     *object,
						     guint                        prop_id,
						     GValue                      *value,
						     GParamSpec                  *pspec);
static void gnome_druid_page_standard_set_property  (GObject                     *object,
						     guint                        prop_id,
						     const GValue                *value,
						     GParamSpec                  *pspec);
static void gnome_druid_page_standard_finalize      (GObject                     *widget);
static void gnome_druid_page_standard_destroy       (GtkObject                   *object);
static void gnome_druid_page_standard_realize       (GtkWidget                   *widget);
static void gnome_druid_page_standard_style_set     (GtkWidget                   *widget,
						     GtkStyle                    *old_style);
static void gnome_druid_page_standard_prepare       (GnomeDruidPage              *page,
						     GtkWidget                   *druid);
static void gnome_druid_page_standard_size_allocate (GtkWidget                   *widget,
						     GtkAllocation               *allocation);
static void gnome_druid_page_standard_layout_setup  (GnomeDruidPageStandard      *druid_page_standard);
static void gnome_druid_page_standard_set_color     (GnomeDruidPageStandard      *druid_page_standard);


#define LOGO_WIDTH 48
#define DRUID_PAGE_WIDTH 508

enum {
	PROP_0,
	PROP_TITLE,
	PROP_LOGO,
	PROP_TOP_WATERMARK,
	PROP_TITLE_FOREGROUND,
	PROP_TITLE_FOREGROUND_GDK,
	PROP_TITLE_FOREGROUND_SET,
	PROP_BACKGROUND,
	PROP_BACKGROUND_GDK,
	PROP_BACKGROUND_SET,
	PROP_LOGO_BACKGROUND,
	PROP_LOGO_BACKGROUND_GDK,
	PROP_LOGO_BACKGROUND_SET,
	PROP_CONTENTS_BACKGROUND,
	PROP_CONTENTS_BACKGROUND_GDK,
	PROP_CONTENTS_BACKGROUND_SET,
};

GNOME_CLASS_BOILERPLATE (GnomeDruidPageStandard, gnome_druid_page_standard,
			 GnomeDruidPage, gnome_druid_page)

static void
gnome_druid_page_standard_class_init (GnomeDruidPageStandardClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	GnomeDruidPageClass *druid_page_class;

	object_class = (GtkObjectClass*) class;
	gobject_class = (GObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;
	druid_page_class = (GnomeDruidPageClass*) class;

	gobject_class->get_property = gnome_druid_page_standard_get_property;
	gobject_class->set_property = gnome_druid_page_standard_set_property;
	gobject_class->finalize = gnome_druid_page_standard_finalize;
	object_class->destroy = gnome_druid_page_standard_destroy;
	widget_class->size_allocate = gnome_druid_page_standard_size_allocate;
	widget_class->realize = gnome_druid_page_standard_realize;
	widget_class->style_set = gnome_druid_page_standard_style_set;
	druid_page_class->prepare = gnome_druid_page_standard_prepare;

	g_object_class_install_property (gobject_class,
					 PROP_TITLE,
					 g_param_spec_string ("title",
							      _("Title"),
							      _("Title of the druid"),
							      NULL,
							      G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_LOGO,
					 g_param_spec_object ("logo",
							      _("Logo"),
							      _("Logo image"),
							      GDK_TYPE_PIXBUF,
							      G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_TOP_WATERMARK,
					 g_param_spec_object ("top_watermark",
							      _("Top Watermark"),
							      _("Watermark image for the top"),
							      GDK_TYPE_PIXBUF,
							      G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_TITLE_FOREGROUND,
					 g_param_spec_string ("title_foreground",
							      _("Title Foreground"),
							      _("Foreground color of the title"),
							      NULL,
							      G_PARAM_WRITABLE));

	g_object_class_install_property (gobject_class,
					 PROP_TITLE_FOREGROUND_GDK,
					 g_param_spec_boxed ("title_foreground_gdk",
							     _("Title Foreground Color"),
							     _("Foreground color of the title as a GdkColor"),
							     GDK_TYPE_COLOR,
							     G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_TITLE_FOREGROUND_SET,
					 g_param_spec_boolean ("title_foreground_set",
							       _("Title Foreground color set"),
							       _("Foreground color of the title is set"),
							       FALSE,
							       G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_BACKGROUND,
					 g_param_spec_string ("background",
							      _("Background Color"),
							      _("Background color"),
							      NULL,
							      G_PARAM_WRITABLE));

	g_object_class_install_property (gobject_class,
					 PROP_BACKGROUND_GDK,
					 g_param_spec_boxed ("background_gdk",
							     _("Background Color"),
							     _("Background color as a GdkColor"),
							     GDK_TYPE_COLOR,
							     G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_BACKGROUND_SET,
					 g_param_spec_boolean ("background_set",
							       _("Background color set"),
							       _("Background color is set"),
							       FALSE,
							       G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_LOGO_BACKGROUND,
					 g_param_spec_string ("logo_background",
							      _("Logo Background Color"),
							      _("Logo Background color"),
							      NULL,
							      G_PARAM_WRITABLE));

	g_object_class_install_property (gobject_class,
					 PROP_LOGO_BACKGROUND_GDK,
					 g_param_spec_boxed ("logo_background_gdk",
							     _("Logo Background Color"),
							     _("Logo Background color as a GdkColor"),
							     GDK_TYPE_COLOR,
							     G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_LOGO_BACKGROUND_SET,
					 g_param_spec_boolean ("logo_background_set",
							       _("Logo Background color set"),
							       _("Logo Background color is set"),
							       FALSE,
							       G_PARAM_READWRITE));
}

static void
gnome_druid_page_standard_init (GnomeDruidPageStandard *druid_page_standard)
{

	GtkWidget *vbox;
	GtkWidget *hbox;

	druid_page_standard->_priv = g_new0(GnomeDruidPageStandardPrivate, 1);

	/* Top VBox */
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (druid_page_standard), vbox);
	gtk_widget_show (vbox);

	/* Top bar layout widget */
	druid_page_standard->_priv->top_bar = gtk_layout_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox),
			    druid_page_standard->_priv->top_bar,
	                    FALSE, FALSE, 0);
	gtk_widget_set_size_request (druid_page_standard->_priv->top_bar, -1, 52);
	gtk_widget_show (druid_page_standard->_priv->top_bar);

	/* Top Bar Watermark */
	druid_page_standard->_priv->watermark = gtk_image_new_from_pixbuf (NULL);
	gtk_layout_put (GTK_LAYOUT (druid_page_standard->_priv->top_bar),
	                druid_page_standard->_priv->watermark, 0, 0);
	gtk_widget_show (druid_page_standard->_priv->watermark);

	/* Title label */
	druid_page_standard->_priv->title_label = gtk_label_new (NULL);
	gtk_layout_put (GTK_LAYOUT (druid_page_standard->_priv->top_bar),
	                druid_page_standard->_priv->title_label, 16, 2);
	gtk_widget_set_size_request (druid_page_standard->_priv->title_label, -1, LOGO_WIDTH);
	gtk_widget_show (druid_page_standard->_priv->title_label);

	/* Top bar logo*/
	druid_page_standard->_priv->logo = gtk_image_new_from_pixbuf (NULL);
	gtk_layout_put (GTK_LAYOUT (druid_page_standard->_priv->top_bar),
	                druid_page_standard->_priv->logo,
	                (DRUID_PAGE_WIDTH - (LOGO_WIDTH + 2)), 2);
	gtk_widget_show (druid_page_standard->_priv->logo);

	/* HBox for contents row */
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);

	/* Left line */
	druid_page_standard->_priv->left_line = gtk_drawing_area_new ();
	gtk_drawing_area_size (GTK_DRAWING_AREA (druid_page_standard->_priv->left_line), 1, 1);
	gtk_box_pack_start (GTK_BOX (hbox), druid_page_standard->_priv->left_line,
	                    FALSE, FALSE, 0);
	gtk_widget_show (druid_page_standard->_priv->left_line);

	/* Contents Event Box (used for styles) */
	druid_page_standard->_priv->evbox = gtk_event_box_new ();
	gtk_box_pack_start (GTK_BOX (hbox), druid_page_standard->_priv->evbox, TRUE, TRUE, 0);
	gtk_widget_show (druid_page_standard->_priv->evbox);

	/* Contents Vbox */
	druid_page_standard->vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (druid_page_standard->vbox), 16);
	gtk_container_add (GTK_CONTAINER (druid_page_standard->_priv->evbox), druid_page_standard->vbox);
	gtk_widget_show (druid_page_standard->vbox);

	/* Right line */
	druid_page_standard->_priv->right_line = gtk_drawing_area_new ();
	gtk_drawing_area_size (GTK_DRAWING_AREA (druid_page_standard->_priv->right_line),
	                       1, 1);
	gtk_box_pack_start (GTK_BOX (hbox), druid_page_standard->_priv->right_line,
	                    FALSE, FALSE, 0);
	gtk_widget_show (druid_page_standard->_priv->right_line);

	/* Bottom line */
	druid_page_standard->_priv->bottom_line = gtk_drawing_area_new ();
	gtk_drawing_area_size (GTK_DRAWING_AREA (druid_page_standard->_priv->bottom_line),
	                       1, 1);
	gtk_box_pack_start (GTK_BOX (vbox), druid_page_standard->_priv->bottom_line,
	                    FALSE, FALSE, 0);
	gtk_widget_show (druid_page_standard->_priv->bottom_line);
}

static void
get_color_arg (GValue *value, GdkColor *orig)
{
  GdkColor *color;

  color = g_new (GdkColor, 1);
  *color = *orig;
  g_value_init (value, GDK_TYPE_COLOR);
  g_value_set_boxed (value, color);
}

static void
gnome_druid_page_standard_get_property (GObject    *object,
					guint       prop_id,
					GValue     *value,
					GParamSpec *pspec)
{
	GnomeDruidPageStandard *druid_page_standard = GNOME_DRUID_PAGE_STANDARD (object);

	switch (prop_id) {
	case PROP_TITLE:
		g_value_set_string (value, druid_page_standard->title);
		break;
	case PROP_LOGO:
		g_value_set_object (value, druid_page_standard->logo);
		break;
	case PROP_TOP_WATERMARK:
		g_value_set_object (value, druid_page_standard->top_watermark);
		break;
	case PROP_TITLE_FOREGROUND_GDK:
		get_color_arg (value, & (druid_page_standard->title_foreground));
		break;
	case PROP_TITLE_FOREGROUND_SET:
		g_value_set_boolean (value, druid_page_standard->_priv->title_foreground_set);
		break;
	case PROP_BACKGROUND_GDK:
		get_color_arg (value, & (druid_page_standard->background));
		break;
	case PROP_BACKGROUND_SET:
		g_value_set_boolean (value, druid_page_standard->_priv->background_set);
		break;
	case PROP_LOGO_BACKGROUND_GDK:
		get_color_arg (value, & (druid_page_standard->logo_background));
		break;
	case PROP_LOGO_BACKGROUND_SET:
		g_value_set_boolean (value, druid_page_standard->_priv->logo_background_set);
		break;
	case PROP_CONTENTS_BACKGROUND_GDK:
		get_color_arg (value, & (druid_page_standard->contents_background));
		break;
	case PROP_CONTENTS_BACKGROUND_SET:
		g_value_set_boolean (value, druid_page_standard->_priv->contents_background_set);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, get_property, (object, prop_id, value, pspec));
}

static void
gnome_druid_page_standard_set_property (GObject      *object,
					guint         prop_id,
					const GValue *value,
					GParamSpec   *pspec)
{
	GnomeDruidPageStandard *druid_page_standard;
	GdkColor color;

	druid_page_standard = GNOME_DRUID_PAGE_STANDARD (object);

	switch (prop_id) {
	case PROP_TITLE:
		gnome_druid_page_standard_set_title (druid_page_standard, g_value_get_string (value));
		break;
	case PROP_LOGO:
		gnome_druid_page_standard_set_logo (druid_page_standard, g_value_get_object (value));
		break;
	case PROP_TOP_WATERMARK:
		gnome_druid_page_standard_set_top_watermark (druid_page_standard, g_value_get_object (value));
		break;
	case PROP_TITLE_FOREGROUND:
	case PROP_TITLE_FOREGROUND_GDK:
		if (prop_id == PROP_TITLE_FOREGROUND_GDK)
			color = *((GdkColor *)g_value_get_boxed (value));
		else if (! gdk_color_parse (g_value_get_string (value), &color)) {
			g_warning ("Don't know color `%s'", g_value_get_string (value));
			break;
		}
		gnome_druid_page_standard_set_title_foreground (druid_page_standard, &color);
		if (druid_page_standard->_priv->title_foreground_set == FALSE) {
			druid_page_standard->_priv->title_foreground_set = TRUE;
			g_object_notify (object, "title_foreground_set");
		}
		break;
	case PROP_TITLE_FOREGROUND_SET:
		druid_page_standard->_priv->title_foreground_set = g_value_get_boolean (value);
		break;
	case PROP_BACKGROUND:
	case PROP_BACKGROUND_GDK:
		if (prop_id == PROP_BACKGROUND_GDK)
			color = *((GdkColor *)g_value_get_boxed (value));
		else if (! gdk_color_parse (g_value_get_string (value), &color)) {
			g_warning ("Don't know color `%s'", g_value_get_string (value));
			break;
		}
		gnome_druid_page_standard_set_background (druid_page_standard, &color);
		if (druid_page_standard->_priv->background_set == FALSE) {
			druid_page_standard->_priv->background_set = TRUE;
			g_object_notify (object, "background_set");
		}
		break;
	case PROP_BACKGROUND_SET:
		druid_page_standard->_priv->background_set = g_value_get_boolean (value);
		break;
	case PROP_LOGO_BACKGROUND:
	case PROP_LOGO_BACKGROUND_GDK:
		if (prop_id == PROP_LOGO_BACKGROUND_GDK)
			color = *((GdkColor *)g_value_get_boxed (value));
		else if (! gdk_color_parse (g_value_get_string (value), &color)) {
			g_warning ("Don't know color `%s'", g_value_get_string (value));
			break;
		}
		gnome_druid_page_standard_set_logo_background (druid_page_standard, &color);
		if (druid_page_standard->_priv->logo_background_set == FALSE) {
			druid_page_standard->_priv->logo_background_set = TRUE;
			g_object_notify (object, "logo_background_set");
		}
		break;
	case PROP_LOGO_BACKGROUND_SET:
		druid_page_standard->_priv->logo_background_set = g_value_get_boolean (value);
		break;
	case PROP_CONTENTS_BACKGROUND:
	case PROP_CONTENTS_BACKGROUND_GDK:
		if (prop_id == PROP_CONTENTS_BACKGROUND_GDK)
			color = *((GdkColor *)g_value_get_boxed (value));
		else if (! gdk_color_parse (g_value_get_string (value), &color)) {
			g_warning ("Don't know color `%s'", g_value_get_string (value));
			break;
		}
		gnome_druid_page_standard_set_contents_background (druid_page_standard, &color);
		if (druid_page_standard->_priv->contents_background_set == FALSE) {
			druid_page_standard->_priv->contents_background_set = TRUE;
			g_object_notify (object, "contents_background_set");
		}
		break;
	case PROP_CONTENTS_BACKGROUND_SET:
		druid_page_standard->_priv->contents_background_set = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, set_property, (object, prop_id, value, pspec));
}

static void
gnome_druid_page_standard_finalize (GObject *object)
{
	GnomeDruidPageStandard *druid_page_standard = GNOME_DRUID_PAGE_STANDARD(object);

	g_free (druid_page_standard->_priv);
	druid_page_standard->_priv = NULL;

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}


static void
gnome_druid_page_standard_destroy (GtkObject *object)
{
	GnomeDruidPageStandard *druid_page_standard = GNOME_DRUID_PAGE_STANDARD (object);

	/* remember, destroy can be run multiple times! */

	if (druid_page_standard->logo != NULL)
		gdk_pixbuf_unref (druid_page_standard->logo);
	druid_page_standard->logo = NULL;

	if (druid_page_standard->top_watermark != NULL)
		gdk_pixbuf_unref (druid_page_standard->top_watermark);
	druid_page_standard->top_watermark = NULL;

	g_free (druid_page_standard->title);
	druid_page_standard->title = NULL;

	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_druid_page_standard_realize (GtkWidget *widget)
{
	GnomeDruidPageStandard *druid_page_standard = GNOME_DRUID_PAGE_STANDARD (widget);

	GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS, realize, (widget));
	
	gnome_druid_page_standard_set_color (druid_page_standard);
}

static void
gnome_druid_page_standard_style_set (GtkWidget *widget,
				     GtkStyle  *old_style)
{
	GnomeDruidPageStandard *druid_page_standard = GNOME_DRUID_PAGE_STANDARD (widget);

	GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS, style_set, (widget, old_style));
	
	gnome_druid_page_standard_set_color (druid_page_standard);
}

static void
gnome_druid_page_standard_prepare (GnomeDruidPage *page,
				   GtkWidget *druid)
{
	gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), TRUE, TRUE, TRUE, TRUE);
	gnome_druid_set_show_finish (GNOME_DRUID (druid), FALSE);
	gtk_widget_grab_default (GNOME_DRUID (druid)->next);
}

GtkWidget *
gnome_druid_page_standard_new (void)
{
	GnomeDruidPageStandard *retval;

	retval = gtk_type_new (gnome_druid_page_standard_get_type ());

	return GTK_WIDGET (retval);
}

GtkWidget *
gnome_druid_page_standard_new_with_vals (const gchar *title,
					 GdkPixbuf   *logo,
					 GdkPixbuf   *top_watermark)
{
	GnomeDruidPageStandard *retval;

	retval = gtk_type_new (gnome_druid_page_standard_get_type ());

	gnome_druid_page_standard_set_title (retval, title);
	gnome_druid_page_standard_set_logo (retval, logo);
	gnome_druid_page_standard_set_top_watermark (retval, top_watermark);
	return GTK_WIDGET (retval);
}



/**
 * gnome_druid_page_standard_set_logo:
 * @druid_page_standard: the #GnomeDruidPageStandard to work on
 * @title: the string to use as the new title text
 *
 * Description:  Sets the title #GtkLabel to display the passed string.
 **/
void
gnome_druid_page_standard_set_title (GnomeDruidPageStandard *druid_page_standard,
				     const gchar            *title)
{
	gchar *title_string;
	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	g_free (druid_page_standard->title);
	druid_page_standard->title = g_strdup (title);
	title_string = g_strconcat ("<span size=\"xx-large\" weight=\"ultrabold\">",
				    title ? title : "", "</span>", NULL);
	gtk_label_set_label (GTK_LABEL (druid_page_standard->_priv->title_label), title_string);
	gtk_label_set_use_markup (GTK_LABEL (druid_page_standard->_priv->title_label), TRUE);
	g_free (title_string);
	g_object_notify (G_OBJECT (druid_page_standard), "title");
}


/**
 * gnome_druid_page_standard_set_logo:
 * @druid_page_standard: the #GnomeDruidPageStandard to work on
 * @logo_image: The #GdkPixbuf to use as a logo
 *
 * Description:  Sets a #GdkPixbuf as the logo in the top right corner.
 * If %NULL, then no logo will be displayed.
 **/
void
gnome_druid_page_standard_set_logo (GnomeDruidPageStandard *druid_page_standard,
				    GdkPixbuf              *logo_image)
{
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	if (logo_image != NULL)
		gdk_pixbuf_ref (logo_image);
	if (druid_page_standard->logo)
		gdk_pixbuf_unref (druid_page_standard->logo);

	druid_page_standard->logo = logo_image;
	gtk_image_set_from_pixbuf (GTK_IMAGE (druid_page_standard->_priv->logo), logo_image);
	g_object_notify (G_OBJECT (druid_page_standard), "logo");
}



/**
 * gnome_druid_page_standard_set_top_watermark:
 * @druid_page_standard: the #GnomeDruidPageStandard to work on
 * @top_watermark_image: The #GdkPixbuf to use as a top watermark
 *
 * Description:  Sets a #GdkPixbuf as the watermark on top of the top
 * strip on the druid.  If #top_watermark_image is %NULL, it is reset
 * to the normal color.
 **/
void
gnome_druid_page_standard_set_top_watermark (GnomeDruidPageStandard *druid_page_standard,
					     GdkPixbuf              *top_watermark_image)
{
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	if (top_watermark_image != NULL)
		gdk_pixbuf_ref (top_watermark_image);
	if (druid_page_standard->top_watermark)
		gdk_pixbuf_unref (druid_page_standard->top_watermark);

	druid_page_standard->top_watermark = top_watermark_image;

	gtk_image_set_from_pixbuf (GTK_IMAGE (druid_page_standard->_priv->watermark), top_watermark_image);
	g_object_notify (G_OBJECT (druid_page_standard), "top_watermark");
}

void
gnome_druid_page_standard_set_title_foreground (GnomeDruidPageStandard *druid_page_standard,
						GdkColor               *color)
{
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (color != NULL);

	druid_page_standard->title_foreground.red = color->red;
	druid_page_standard->title_foreground.green = color->green;
	druid_page_standard->title_foreground.blue = color->blue;

	gtk_widget_modify_fg (druid_page_standard->_priv->title_label, GTK_STATE_NORMAL, color);

	g_object_notify (G_OBJECT (druid_page_standard), "title_foreground");
	if (druid_page_standard->_priv->title_foreground_set == FALSE) {
		druid_page_standard->_priv->title_foreground_set = TRUE;
		g_object_notify (G_OBJECT (druid_page_standard), "title_foreground_set");
	}
}

void
gnome_druid_page_standard_set_background (GnomeDruidPageStandard *druid_page_standard,
					  GdkColor               *color)
{
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (color != NULL);

	druid_page_standard->background.red = color->red;
	druid_page_standard->background.green = color->green;
	druid_page_standard->background.blue = color->blue;

	gtk_widget_modify_bg (druid_page_standard->_priv->top_bar, GTK_STATE_NORMAL, color);
	gtk_widget_modify_bg (druid_page_standard->_priv->left_line, GTK_STATE_NORMAL, color);
	gtk_widget_modify_bg (druid_page_standard->_priv->right_line, GTK_STATE_NORMAL, color);
	gtk_widget_modify_bg (druid_page_standard->_priv->bottom_line, GTK_STATE_NORMAL, color);

	g_object_notify (G_OBJECT (druid_page_standard), "background");
	if (druid_page_standard->_priv->background_set == FALSE) {
		druid_page_standard->_priv->background_set = TRUE;
		g_object_notify (G_OBJECT (druid_page_standard), "background_set");
	}
}

void
gnome_druid_page_standard_set_logo_background (GnomeDruidPageStandard *druid_page_standard,
					       GdkColor               *color)
{
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (color != NULL);

	druid_page_standard->logo_background.red = color->red;
	druid_page_standard->logo_background.green = color->green;
	druid_page_standard->logo_background.blue = color->blue;

	gtk_widget_modify_bg (druid_page_standard->_priv->logo, GTK_STATE_NORMAL, color);
	g_object_notify (G_OBJECT (druid_page_standard), "logo_background");
	if (druid_page_standard->_priv->logo_background_set == FALSE) {
		druid_page_standard->_priv->logo_background_set = TRUE;
		g_object_notify (G_OBJECT (druid_page_standard), "logo_background_set");
	}
}


void
gnome_druid_page_standard_set_contents_background (GnomeDruidPageStandard *druid_page_standard,
					       GdkColor               *color)
{
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (color != NULL);

	druid_page_standard->contents_background.red = color->red;
	druid_page_standard->contents_background.green = color->green;
	druid_page_standard->contents_background.blue = color->blue;

	gtk_widget_modify_bg (druid_page_standard->_priv->evbox, GTK_STATE_NORMAL, color);

	g_object_notify (G_OBJECT (druid_page_standard), "contents_background");
	if (druid_page_standard->_priv->contents_background_set == FALSE) {
		druid_page_standard->_priv->contents_background_set = TRUE;
		g_object_notify (G_OBJECT (druid_page_standard), "contents_background_set");
	}
}


/**
 * gnome_druid_page_standard_append_item:
 * @druid_page_standard: The #GnomeDruidPageStandard to work on
 * @question: The text to place above the item
 * @item: The #GtkWidget to be included
 * @additional_info: The text to be placed below the item in a smaller
 * font
 *
 * Description: Convenience function to add a #GtkWidget to the
 * #GnomeDruidPageStandard vbox.  WRITEME
 **/
void
gnome_druid_page_standard_append_item (GnomeDruidPageStandard *druid_page_standard,
                                       const gchar            *question,
                                       GtkWidget              *item,
                                       const gchar            *additional_info)
{
	GtkWidget *q_label;
	GtkWidget *a_label;
	GtkWidget *vbox;
	gchar *a_text;

	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (GTK_IS_WIDGET (item));

	/* Create the vbox to hold the three new items */
	vbox = gtk_vbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (druid_page_standard->vbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show (vbox);

	/* Create the question label if question is not empty */
	if (question != NULL && strcmp (question, "") != 0) {
		q_label = gtk_label_new (NULL);
		gtk_label_set_label (GTK_LABEL (q_label),question);
		gtk_label_set_use_markup (GTK_LABEL (q_label), TRUE);
		gtk_label_set_use_underline (GTK_LABEL (q_label), TRUE);
		gtk_label_set_mnemonic_widget (GTK_LABEL (q_label), item);
		gtk_label_set_justify (GTK_LABEL (q_label), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment (GTK_MISC (q_label), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (vbox), q_label, FALSE, FALSE, 0);
		gtk_widget_show (q_label);
	}

	/* Append/show the item */
	gtk_box_pack_start (GTK_BOX (vbox), item, FALSE, FALSE, 0);
	gtk_widget_show (item);

	/* Create the "additional info" label if additional_info is not empty */
	if (additional_info != NULL && additional_info[0] != '\000') {
		a_text = g_strconcat ("<span size=\"small\">", additional_info, "</span>", NULL);
		a_label = gtk_label_new (NULL);
		gtk_label_set_label (GTK_LABEL (a_label), a_text);
		gtk_label_set_use_markup (GTK_LABEL (a_label), TRUE);
		g_free (a_text);
		gtk_label_set_justify (GTK_LABEL (a_label), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment (GTK_MISC (a_label), 0.0, 0.5);
		gtk_misc_set_padding (GTK_MISC (a_label), 24, 0);
		gtk_box_pack_start (GTK_BOX (vbox), a_label, FALSE, FALSE, 0);
		gtk_widget_show (a_label);
	}
}

static void
gnome_druid_page_standard_size_allocate (GtkWidget *widget,
					 GtkAllocation *allocation)
{
	GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS, size_allocate,
				   (widget, allocation));
	gnome_druid_page_standard_layout_setup (GNOME_DRUID_PAGE_STANDARD (widget));
}

static void
gnome_druid_page_standard_layout_setup (GnomeDruidPageStandard *druid_page_standard)
{
	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	gtk_layout_move (GTK_LAYOUT (druid_page_standard->_priv->top_bar),
	                 druid_page_standard->_priv->logo,
	                 (druid_page_standard->_priv->top_bar->allocation.width - (LOGO_WIDTH + 2)), 2);
	gtk_layout_move (GTK_LAYOUT (druid_page_standard->_priv->top_bar),
	                 druid_page_standard->_priv->title_label, 16, 2);
}

static void
gnome_druid_page_standard_set_color (GnomeDruidPageStandard *druid_page_standard)
{
	GtkWidget *widget = GTK_WIDGET (druid_page_standard);
	if (druid_page_standard->_priv->background_set == FALSE) {
		druid_page_standard->background.red = widget->style->bg[GTK_STATE_SELECTED].red;
		druid_page_standard->background.green = widget->style->bg[GTK_STATE_SELECTED].green;
		druid_page_standard->background.blue = widget->style->bg[GTK_STATE_SELECTED].blue;
	}
	if (druid_page_standard->_priv->logo_background_set == FALSE) {
		druid_page_standard->logo_background.red = widget->style->bg[GTK_STATE_SELECTED].red;
		druid_page_standard->logo_background.green = widget->style->bg[GTK_STATE_SELECTED].green;
		druid_page_standard->logo_background.blue = widget->style->bg[GTK_STATE_SELECTED].blue;
	}
	if (druid_page_standard->_priv->title_foreground_set == FALSE) {
		druid_page_standard->title_foreground.red = widget->style->fg[GTK_STATE_SELECTED].red;
		druid_page_standard->title_foreground.green = widget->style->fg[GTK_STATE_SELECTED].green;
		druid_page_standard->title_foreground.blue = widget->style->fg[GTK_STATE_SELECTED].blue;
	}
	if (druid_page_standard->_priv->contents_background_set == FALSE) {
		druid_page_standard->contents_background.red = widget->style->bg[GTK_STATE_PRELIGHT].red;
		druid_page_standard->contents_background.green = widget->style->bg[GTK_STATE_PRELIGHT].green;
		druid_page_standard->contents_background.blue = widget->style->bg[GTK_STATE_PRELIGHT].blue;
	}

	gtk_widget_modify_fg (druid_page_standard->_priv->title_label, GTK_STATE_NORMAL, &(druid_page_standard->title_foreground));
	gtk_widget_modify_bg (druid_page_standard->_priv->top_bar, GTK_STATE_NORMAL, &(druid_page_standard->background));
	gtk_widget_modify_bg (druid_page_standard->_priv->left_line, GTK_STATE_NORMAL, &(druid_page_standard->background));
	gtk_widget_modify_bg (druid_page_standard->_priv->right_line, GTK_STATE_NORMAL, &(druid_page_standard->background));
	gtk_widget_modify_bg (druid_page_standard->_priv->bottom_line, GTK_STATE_NORMAL, &(druid_page_standard->background));
	gtk_widget_modify_bg (druid_page_standard->_priv->evbox, GTK_STATE_NORMAL,&(druid_page_standard->contents_background));
}
