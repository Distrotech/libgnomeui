/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-about.h - An about box widget for gnome.

   Copyright (C) 2001 CodeFactory AB
   Copyright (C) 2001 Anders Carlsson <andersca@codefactory.se>

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Anders Carlsson <andersca@codefactory.se>
*/

#include <config.h>
#include <libgnome/gnome-macros.h>

#include "gnome-i18nP.h"

#include "gnome-about.h"

#include <gtk/gtkbox.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkstock.h>
#include <gtk/gtksignal.h>

#include <string.h>

#define HEADER_FONT "sans bold 20"
#define COPYRIGHT_FONT "sans 11"
#define COMMENTS_FONT "sans 11"
#define NAME_HEADER_FONT "sans 18"
#define NAME_LIST_FONT "sans 12"

#define MAX_ROWS_PER_COLUMN 10
#define COLUMN_PADDING 15

#define MINIMUM_WIDTH 460
#define MINIMUM_HEIGHT 290
#define MINIMUM_HEADER_HEIGHT 30

/* Padding (if needed) between the title and the logo */
#define MINIMUM_TITLE_LOGO_PADDING 10

enum {
	DISPLAYING_AUTHORS = 0,
	DISPLAYING_TRANSLATOR_CREDITS,
	DISPLAYING_DOCUMENTERS,
	NUMBER_OF_DISPLAYS
};

struct _GnomeAboutPrivate {
	gchar *name;
	gchar *version;
	gchar *copyright;
	gchar *comments;
	
	GSList *authors;
	GSList *documenters;

	gchar *translator_credits;
	
	gint displaying_state;
	
	GtkWidget *drawing_area;
	
	GdkPixbuf *background_pixbuf;
	GdkPixbuf *rendered_background_pixbuf;
	gdouble gradient_start_opacity, gradient_end_opacity;
	gdouble gradient_start_position, gradient_end_position;

	GdkPixbuf *logo_pixbuf;
	gint logo_top_padding;
	gint logo_right_padding;
};

typedef struct {
	gchar *name;

	gboolean is_displaying;
	gboolean has_been_displayed;
} GnomeAboutEntry;

enum {
	PROP_0,
	PROP_NAME,
	PROP_VERSION,
	PROP_COPYRIGHT,
	PROP_COMMENTS,
	PROP_AUTHORS,
	PROP_DOCUMENTERS,
	PROP_BACKGROUND,
	PROP_BACKGROUND_START_OPACITY,
	PROP_BACKGROUND_END_OPACITY,
	PROP_BACKGROUND_START_POSITION,
	PROP_BACKGROUND_END_POSITION,
	PROP_TRANSLATOR_CREDITS,
	PROP_LOGO,
	PROP_LOGO_TOP_PADDING,
	PROP_LOGO_RIGHT_PADDING,
};

static void gnome_about_finalize (GObject *object);
static void gnome_about_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void gnome_about_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static gboolean gnome_about_area_expose (GtkWidget *area, GdkEventExpose *event, gpointer data);
static gboolean gnome_about_area_button_press (GtkWidget *area, GdkEventButton *button, gpointer data);

GNOME_CLASS_BOILERPLATE (GnomeAbout, gnome_about,
			 GtkDialog, GTK_TYPE_DIALOG)

static void
gnome_about_instance_init (GnomeAbout *about)
{
	GtkWidget *frame;

	about->_priv = g_new0 (GnomeAboutPrivate, 1);

	about->_priv->gradient_start_opacity = 1.0;
	about->_priv->gradient_end_opacity = 0.0;
	about->_priv->gradient_start_position = 0.25;
	about->_priv->gradient_end_position = 0.75;

	about->_priv->logo_top_padding = 5;
	about->_priv->logo_right_padding = 0;
	
	/* Create the frame */
	frame = gtk_frame_new (NULL);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 8);
	gtk_widget_show (frame);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about)->vbox), frame, TRUE, TRUE, 0);

	/* Create the drawing area */
	about->_priv->drawing_area = gtk_drawing_area_new ();
	gtk_widget_add_events (about->_priv->drawing_area, GDK_BUTTON_PRESS_MASK);
	
	g_signal_connect (G_OBJECT (about->_priv->drawing_area), "expose_event",
			  G_CALLBACK (gnome_about_area_expose), about);
	g_signal_connect (G_OBJECT (about->_priv->drawing_area), "button_press_event",
			  G_CALLBACK (gnome_about_area_button_press), about);
	gtk_widget_show (about->_priv->drawing_area);
	gtk_container_add (GTK_CONTAINER (frame), about->_priv->drawing_area);

	/* Add the OK button */
	gtk_dialog_add_button (GTK_DIALOG (about), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (about), GTK_RESPONSE_OK);
	gtk_signal_connect (GTK_OBJECT (about), "response",
			    GTK_SIGNAL_FUNC (gtk_widget_destroy),
			    NULL);
}

static void
gnome_about_class_init (GnomeAboutClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
	object_class = (GObjectClass *)klass;
	widget_class = (GtkWidgetClass *)klass;
	
	object_class->set_property = gnome_about_set_property;
	object_class->get_property = gnome_about_get_property;
	object_class->finalize = gnome_about_finalize;

	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      _("Program name"),
							      _("The name of the program"),
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_VERSION,
					 g_param_spec_string ("version",
							      _("Program version"),
							      _("The version of the program"),
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_COPYRIGHT,
					 g_param_spec_string ("copyright",
							      _("Copyright string"),
							      _("Copyright information for the program"),
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_COMMENTS,
					 g_param_spec_string ("comments",
							      _("Comments string"),
							      _("Comments about the program"),
							      NULL,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_AUTHORS,
					 g_param_spec_value_array ("authors",
								   _("Authors"),
								   _("List of authors of the programs"),
								   g_param_spec_string ("author-entry",
											_("Author entry"),
											_("A single author entry"),
											NULL,
											G_PARAM_READWRITE),
								   G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
					 PROP_TRANSLATOR_CREDITS,
					 g_param_spec_string ("translator_credits",
							      _("Translator credits"),
							      _("Credits to the translators. This string should be marked as translatable"),
							      NULL,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_DOCUMENTERS,
					 g_param_spec_value_array ("documenters",
								   _("Documenters"),
								   _("List of people documenting the program"),
								   g_param_spec_string ("documenter-entry",
											_("Documenter entry"),
											_("A single documenter entry"),
											NULL,
											G_PARAM_READWRITE),
								   G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
					 PROP_BACKGROUND,
					 g_param_spec_object ("background",
							      _("Background"),
							      _("The background pixbuf to use"),
							      GDK_TYPE_PIXBUF,
							      G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
					 PROP_BACKGROUND_START_OPACITY,
					 g_param_spec_float ("background_start_opacity",
							     _("Background start opacity"),
							     _("Opacity at the start of the background"),
							     0.0,
							     1.0,
							     1.0,
							     G_PARAM_READWRITE));
	
	g_object_class_install_property (object_class,
					 PROP_BACKGROUND_END_OPACITY,
					 g_param_spec_float ("background_end_opacity",
							     _("Background end opacity"),
							     _("Opacity at the end of the background"),
							     0.0,
							     1.0,
							     0.0,
							     G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_BACKGROUND_START_POSITION,
					 g_param_spec_float ("background_start_position",
							     _("Background start position"),
							     _("Position at the start of the background"),
							     0.0,
							     1.0,
							     0.25,
							     G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_BACKGROUND_END_POSITION,
					 g_param_spec_float ("background_end_position",
							     _("Background end position"),
							     _("Position at the end of the background"),
							     0.0,
							     1.0,
							     0.75,
							     G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_LOGO,
					 g_param_spec_object ("logo",
							      _("Logo"),
							      _("A logo for the about box"),
							      GDK_TYPE_PIXBUF,
							      G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
					 PROP_LOGO_TOP_PADDING,
					 g_param_spec_int ("logo_top_padding",
							   _("Logo top padding"),
							   _("How far the logo is from the top"),
							   0,
							   G_MAXINT,
							   5,
							   G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_LOGO_RIGHT_PADDING,
					 g_param_spec_int ("logo_right_padding",
							   _("Logo right padding"),
							   _("How far the logo is from the right"),
							   0,
							   G_MAXINT,
							   5,
							   G_PARAM_READWRITE));

	/* Widget style properties */
	gtk_widget_class_install_style_property (widget_class,
						 g_param_spec_string ("header_font",
								      _("Header font"),
								      _("Header font description as a string"),
								      HEADER_FONT,
								      G_PARAM_READWRITE));
	gtk_widget_class_install_style_property (widget_class,
						 g_param_spec_string ("copyright_font",
								      _("Copyright font"),
								      _("Copyright font description as a string"),
								      COPYRIGHT_FONT,
								      G_PARAM_READWRITE));
	gtk_widget_class_install_style_property (widget_class,
						 g_param_spec_string ("comments_font",
								      _("Comments font"),
								      _("Comments font description as a string"),
								      COMMENTS_FONT,
								      G_PARAM_READWRITE));
	gtk_widget_class_install_style_property (widget_class,
						 g_param_spec_string ("name_header_font",
								      _("Name header font"),
								      _("Name header font description as a string"),
								      NAME_HEADER_FONT,
								      G_PARAM_READWRITE));
	gtk_widget_class_install_style_property (widget_class,
						 g_param_spec_string ("name_list_font",
								      _("Name list font"),
								      _("Name list font description as a string"),
								      NAME_LIST_FONT,
								      G_PARAM_READWRITE));
}

static PangoLayout *
gnome_about_create_pango_layout_with_style (GnomeAbout *about, const gchar *widget_style, PangoFontDescription **font_desc)
{
	PangoLayout *layout;
	PangoFontDescription *font_description;
	gchar *font_name;
	
	gtk_widget_style_get (GTK_WIDGET (about), widget_style, &font_name, NULL);
	layout = gtk_widget_create_pango_layout (GTK_WIDGET (about), NULL);

	font_description = pango_font_description_from_string (font_name);
	pango_layout_set_font_description (layout, font_description);
	
	g_free (font_name);

	if (font_desc != NULL) {
		*font_desc = font_description;
	}
	else {
		pango_font_description_free (font_description);
	}

	return layout;
}

static GSList *
gnome_about_get_displayed_person_list (GnomeAbout *about)
{
	switch (about->_priv->displaying_state) {
	case DISPLAYING_AUTHORS:
		return about->_priv->authors;
	case DISPLAYING_DOCUMENTERS:
		return about->_priv->documenters;
	case DISPLAYING_TRANSLATOR_CREDITS:
		return NULL;
	default:
		g_assert_not_reached ();
	}

	return NULL;
}

static gint
gnome_about_get_widest_entry (GnomeAbout *about)
{
	PangoLayout *name_list_layout;
	gint width;
	gint widest_entry;
	GSList *list;
	gint i;
	
	widest_entry = 0;

	name_list_layout = gnome_about_create_pango_layout_with_style (about, "name_list_font", NULL);

	for (i = 0; i < NUMBER_OF_DISPLAYS; i++) {
		switch (i) {
		case DISPLAYING_AUTHORS:
			list = about->_priv->authors;
			break;
		case DISPLAYING_DOCUMENTERS:
			list = about->_priv->documenters;
			break;
		case DISPLAYING_TRANSLATOR_CREDITS:
			/* Don't do anything for the translator credits */
			list = NULL;
			break;
		default:
			g_assert_not_reached ();
			list = NULL; /* silence warning */
		}
		
		for (; list; list = list->next) {
			GnomeAboutEntry *entry = list->data;
			
			pango_layout_set_text (name_list_layout, entry->name, -1);
			pango_layout_get_pixel_size (name_list_layout, &width, NULL);
			
			if (width > widest_entry) {
				widest_entry = width;
			}
		}
	}

	g_object_unref (G_OBJECT (name_list_layout));
	
	return widest_entry;
}

static gint
gnome_about_get_number_of_undisplayed_entries (GnomeAbout *about)
{
	gint undisplayed;
	GSList *list;
	
	undisplayed = 0;

	for (list = gnome_about_get_displayed_person_list (about); list; list = list->next) {
		GnomeAboutEntry *entry = list->data;

		if (entry->has_been_displayed == FALSE) {
			undisplayed++;
		}
	}

	return undisplayed;
}


static void
gnome_about_reset_displayed_person_list (GnomeAbout *about)
{
	GSList *list;

	for (list = gnome_about_get_displayed_person_list (about); list; list = list->next) {
		GnomeAboutEntry *entry = list->data;

		if (entry->has_been_displayed == TRUE) {
			entry->has_been_displayed = FALSE;
		}
	}
}

static void
gnome_about_draw_logo (GnomeAbout *about, GdkRectangle *area)
{
	GdkPixbuf *pixbuf;
	gint x, y;
	
	pixbuf = about->_priv->logo_pixbuf;

	if (pixbuf == NULL)
		return;

	y = about->_priv->logo_top_padding;
	x = about->_priv->drawing_area->allocation.width - about->_priv->logo_right_padding - gdk_pixbuf_get_width (pixbuf);
	
	gdk_pixbuf_render_to_drawable_alpha (pixbuf, about->_priv->drawing_area->window,
					     0, 0,
					     x, y,
					     gdk_pixbuf_get_width (pixbuf),
					     gdk_pixbuf_get_height (pixbuf),
					     GDK_PIXBUF_ALPHA_FULL,
					     0,
					     GDK_RGB_DITHER_NORMAL,
					     gdk_pixbuf_get_width (pixbuf),
					     gdk_pixbuf_get_height (pixbuf));


}

static void
gnome_about_draw_background (GnomeAbout *about, GdkRectangle *area)
{
	GdkPixbuf *pixbuf;

	pixbuf = about->_priv->rendered_background_pixbuf;

	if (pixbuf == NULL)
		return;
	
	gdk_pixbuf_render_to_drawable_alpha (pixbuf, about->_priv->drawing_area->window,
					     0, 0, 0, 30,
					     gdk_pixbuf_get_width (pixbuf),
					     gdk_pixbuf_get_height (pixbuf),
					     GDK_PIXBUF_ALPHA_BILEVEL,
					     128,
					     GDK_RGB_DITHER_NORMAL,
					     gdk_pixbuf_get_width (pixbuf),
					     gdk_pixbuf_get_height (pixbuf));

}

static void
gnome_about_draw_copyright (GnomeAbout *about, GdkRectangle *area)
{
	PangoLayout *copyright_layout;

	copyright_layout = gnome_about_create_pango_layout_with_style (about, "copyright_font", NULL);
	pango_layout_set_text (copyright_layout, about->_priv->copyright, -1);

	gdk_draw_layout (about->_priv->drawing_area->window, about->_priv->drawing_area->style->black_gc, 3, 30, copyright_layout);

	g_object_unref (G_OBJECT (copyright_layout));
}

static void
gnome_about_draw_comments (GnomeAbout *about, GdkRectangle *area)
{
	PangoLayout *comments_layout;

	comments_layout = gnome_about_create_pango_layout_with_style (about, "comments_font", NULL);
	pango_layout_set_text (comments_layout, about->_priv->comments, -1);
	pango_layout_set_width (comments_layout, 150 * PANGO_SCALE);
	
	gdk_draw_layout (about->_priv->drawing_area->window, about->_priv->drawing_area->style->black_gc, 8, 85, comments_layout);

	g_object_unref (G_OBJECT (comments_layout));
}

static gchar *
gnome_about_get_header_string (GnomeAbout *about)
{
	gchar *text;

	text = g_strdup_printf ("%s %s", about->_priv->name, about->_priv->version);
	
	return text;
}

static void
gnome_about_draw_header (GnomeAbout *about, GdkRectangle *area)
{
	PangoLayout *header_layout;
	gchar *text;
	gint y;
	GdkGC *gc;
	
	gdk_draw_rectangle (about->_priv->drawing_area->window, about->_priv->drawing_area->style->white_gc, TRUE,
			    area->x, area->y, area->width, area->height);

	for (y = 0; y < 30; y++) {
		if (y % 2 == 0)
			gc = about->_priv->drawing_area->style->dark_gc[GTK_STATE_SELECTED];
		else
			gc = about->_priv->drawing_area->style->light_gc[GTK_STATE_SELECTED];
		
		gdk_draw_line (about->_priv->drawing_area->window, gc, 0, y, about->_priv->drawing_area->allocation.width, y);
	}

	header_layout = gnome_about_create_pango_layout_with_style (about, "header_font", NULL);
	text = gnome_about_get_header_string (about);
	
	pango_layout_set_text (header_layout, text, -1);

	/* Draw our header */
	gdk_draw_layout (about->_priv->drawing_area->window, about->_priv->drawing_area->style->black_gc, 4, 2, header_layout);
	gdk_draw_layout (about->_priv->drawing_area->window, about->_priv->drawing_area->style->white_gc, 3, 1, header_layout);

	g_free (text);
	g_object_unref (G_OBJECT (header_layout));
}

static void
gnome_about_draw_names (GnomeAbout *about, GdkRectangle *area)
{
	PangoLayout *name_list_layout;
	PangoContext *context;
	PangoFontMetrics *metrics;
	PangoFontDescription *font_description;
	GSList *list;
	gint rows_per_column;
	gint x, y;
	gint height;
	gint rows, cols;

	name_list_layout = gnome_about_create_pango_layout_with_style (about, "name_list_font", &font_description);

	context = gtk_widget_get_pango_context (GTK_WIDGET (about));

	metrics = pango_context_get_metrics (context,
					   font_description,
					   pango_context_get_language (context));

	height = pango_font_metrics_get_ascent (metrics) / PANGO_SCALE +
                 pango_font_metrics_get_descent (metrics) / PANGO_SCALE + 2;
	
	pango_font_metrics_unref (metrics);

	rows_per_column = gnome_about_get_number_of_undisplayed_entries (about);
	rows_per_column = rows_per_column / 2 + rows_per_column % 2;
	rows_per_column = MIN (rows_per_column, MAX_ROWS_PER_COLUMN);

	x = 170;
	y = 85;
	rows = 0;
	cols = 0;

	list = gnome_about_get_displayed_person_list (about);
	
	for (; list; list = list->next) {
		GnomeAboutEntry *entry = list->data;

		if (entry->has_been_displayed == TRUE) {
			continue;
		}

		entry->is_displaying = TRUE;

		pango_layout_set_text (name_list_layout, entry->name, -1);
		gdk_draw_layout (about->_priv->drawing_area->window, about->_priv->drawing_area->style->black_gc,
				 x, y, name_list_layout);
		y += height;
		rows++;

		if (rows >= rows_per_column) {

			cols++;

			if (cols >= 2)
				break;
			
			rows = 0;
			x += gnome_about_get_widest_entry (about) + COLUMN_PADDING;
			y = 85;
		}
	}

	pango_font_description_free (font_description);
	g_object_unref (G_OBJECT (name_list_layout));
}

static void
gnome_about_draw_name_header (GnomeAbout *about, GdkRectangle *area)
{
	PangoLayout *name_header_layout;
	gchar *header_str;

	name_header_layout = gnome_about_create_pango_layout_with_style (about, "name_header_font", NULL);
	
	switch (about->_priv->displaying_state) {
	case DISPLAYING_AUTHORS:
		header_str = g_slist_length (about->_priv->authors) == 1 ? _("Author") : _("Authors");
		break;
	case DISPLAYING_TRANSLATOR_CREDITS:
		header_str = _("Translator credits");
		break;
	case DISPLAYING_DOCUMENTERS:
		header_str = g_slist_length (about->_priv->documenters) == 1 ? _("Documenter") : _("Documenters");
		break;
	default:
		g_assert_not_reached ();
		header_str = NULL; /* silence warning */
	}
	
 	pango_layout_set_text (name_header_layout, header_str, -1);
	gdk_draw_layout (about->_priv->drawing_area->window, about->_priv->drawing_area->style->black_gc, 170, 60 , name_header_layout);

	g_object_unref (G_OBJECT (name_header_layout));
}

static gboolean
gnome_about_area_button_press (GtkWidget *area, GdkEventButton *button, gpointer data)
{
	GnomeAbout *about = data;
	GSList *list;
	gint undisplayed_entries;

	undisplayed_entries = 0;
	
	for (list = gnome_about_get_displayed_person_list (about); list; list = list->next) {
		GnomeAboutEntry *entry = list->data;
		
		if (entry->is_displaying == TRUE) {
			entry->is_displaying = FALSE;
			entry->has_been_displayed = TRUE;
		}

		if (entry->has_been_displayed == FALSE) {
			undisplayed_entries++;
		}
	}

	/* We've displayed all entries */
	if (undisplayed_entries == 0) {
		switch (about->_priv->displaying_state) {
		case DISPLAYING_AUTHORS:
			if (about->_priv->translator_credits != NULL)
				about->_priv->displaying_state = DISPLAYING_TRANSLATOR_CREDITS;
			else if (about->_priv->documenters != NULL)
				about->_priv->displaying_state = DISPLAYING_DOCUMENTERS;
			break;
		case DISPLAYING_TRANSLATOR_CREDITS:
			if (about->_priv->documenters != NULL)
				about->_priv->displaying_state = DISPLAYING_DOCUMENTERS;
			else if (about->_priv->authors != NULL)
				about->_priv->displaying_state = DISPLAYING_AUTHORS;
			break;
		case DISPLAYING_DOCUMENTERS:
			if (about->_priv->authors != NULL)
				about->_priv->displaying_state = DISPLAYING_AUTHORS;
			else if (about->_priv->translator_credits != NULL)
				about->_priv->displaying_state = DISPLAYING_TRANSLATOR_CREDITS;
			break;
		default:
			g_assert_not_reached ();
		}

		/* Reset the list */
		gnome_about_reset_displayed_person_list (about);
	}
	
	gtk_widget_queue_draw (area);

	return TRUE;
}

static void
gnome_about_update_size (GnomeAbout *about)
{
	PangoLayout *layout;
	gint width, height;
	gint layout_width, layout_height;
	gint tmp_width;
	
	gchar *text;
	
	width = MINIMUM_WIDTH;
	height = MINIMUM_HEIGHT;

	/* First, check the width of the header */
	layout = gnome_about_create_pango_layout_with_style (about, "header_font", NULL);
	text = gnome_about_get_header_string (about);
	pango_layout_set_text (layout, text, -1);
	pango_layout_get_pixel_size (layout, &layout_width, &layout_height);
	g_free (text);

	tmp_width = layout_width +
		MINIMUM_TITLE_LOGO_PADDING +
		(about->_priv->logo_pixbuf ? gdk_pixbuf_get_width (about->_priv->logo_pixbuf) : 0) +
		about->_priv->logo_right_padding;

	if (tmp_width > width)
		width = tmp_width;
	
	g_print ("updating size!\n");
	gtk_widget_set_size_request (GTK_WIDGET (about->_priv->drawing_area), width, height);
}

static void
gnome_about_draw_translator_credits (GnomeAbout *about, GdkRectangle *area)
{
	PangoLayout *layout;
	gint x, y;

	x = 170;
	y = 85;
	
	layout = gnome_about_create_pango_layout_with_style (about, "name_list_font", NULL);

	pango_layout_set_text (layout, about->_priv->translator_credits, -1);
	gdk_draw_layout (about->_priv->drawing_area->window, about->_priv->drawing_area->style->black_gc,
			 x, y, layout);
	g_object_unref (G_OBJECT (layout));
}

static gboolean
gnome_about_area_expose (GtkWidget *area, GdkEventExpose *event, gpointer data)
{
	GnomeAbout *about = data;

	gnome_about_draw_header (about, &event->area);
	gnome_about_draw_background (about, &event->area);
	gnome_about_draw_comments (about, &event->area);
	gnome_about_draw_name_header (about, &event->area);

	if (about->_priv->displaying_state == DISPLAYING_TRANSLATOR_CREDITS)
		gnome_about_draw_translator_credits (about, &event->area);
	else
		gnome_about_draw_names (about, &event->area);
	
	gnome_about_draw_copyright (about, &event->area);
	gnome_about_draw_logo (about, &event->area);
	
	return TRUE;
}

static void
gnome_about_set_name (GnomeAbout *about, const gchar *name)
{
	gchar *title_string;

	g_free (about->_priv->name);
	about->_priv->name = g_strdup (name ? name : "");

	title_string = g_strdup_printf (_("About %s"), name);
	gtk_window_set_title (GTK_WINDOW (about), title_string);
	g_free (title_string);

	gtk_widget_queue_draw (GTK_WIDGET (about));
}

static void
gnome_about_set_version (GnomeAbout *about, const gchar *version)
{
	if (about->_priv->version != NULL &&
	    version != NULL &&
	    strcmp (about->_priv->version, version) == 0) {
		return;
	}
	
	g_free (about->_priv->version);
	about->_priv->version = g_strdup (version ? version : "");

	gnome_about_update_size (about);
}

static void
gnome_about_set_copyright (GnomeAbout *about, const gchar *copyright)
{
	g_free (about->_priv->copyright);
	about->_priv->copyright = g_strdup (copyright ? copyright : "");

	gnome_about_update_size (about);
}

static guchar
lighten_component (guchar cur_value, gdouble opacity)
{
	int new_value = cur_value;

	new_value += ((255 - cur_value) * (1.0 - opacity));
	
	if (new_value > 255) {
		new_value = 255;
	}
	return (guchar) new_value;
}

static void
gnome_about_update_background (GnomeAbout *about)
{
	GdkPixbuf *pixbuf;
	gint i, j;
	guchar *pixels;
	gdouble opacity, position;
	gint width, height;
	
	if (about->_priv->rendered_background_pixbuf != NULL) {
		g_object_unref (about->_priv->rendered_background_pixbuf);
	}

	if (about->_priv->background_pixbuf == NULL) {
		about->_priv->rendered_background_pixbuf = NULL;
		return;
	}
	
	pixbuf = gdk_pixbuf_copy (about->_priv->background_pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	
	for (i = 0; i < height; i++) {

		pixels = gdk_pixbuf_get_pixels (pixbuf) + i * gdk_pixbuf_get_rowstride (pixbuf);

		for (j = 0; j < width; j++) {
			position = (gdouble)j / (gdouble)width;

			if (position < about->_priv->gradient_start_position) {
				opacity = about->_priv->gradient_start_opacity;
			}
			else if (position > about->_priv->gradient_end_position) {
				opacity = about->_priv->gradient_end_opacity;
			}
			else {
				gdouble normalized_position;

				normalized_position = (position - about->_priv->gradient_start_position) / (about->_priv->gradient_end_position - about->_priv->gradient_start_position);
				opacity = about->_priv->gradient_start_opacity + (about->_priv->gradient_end_opacity - about->_priv->gradient_start_opacity) * normalized_position;
			}
			
			*pixels++ = lighten_component (*pixels, opacity);
			*pixels++ = lighten_component (*pixels, opacity);
			*pixels++ = lighten_component (*pixels, opacity);

			if (gdk_pixbuf_get_has_alpha (pixbuf) == TRUE)
				pixels++;
		}
	}

	about->_priv->rendered_background_pixbuf = pixbuf;

	gtk_widget_queue_draw (about->_priv->drawing_area);
}

static void
gnome_about_set_logo (GnomeAbout *about, GdkPixbuf *pixbuf)
{
	if (pixbuf)
		g_object_ref (pixbuf);

	if (about->_priv->logo_pixbuf)
		g_object_unref (about->_priv->logo_pixbuf);

	about->_priv->logo_pixbuf = pixbuf;

	gtk_widget_queue_draw (about->_priv->drawing_area);
}

static void
gnome_about_set_background (GnomeAbout *about, GdkPixbuf *pixbuf)
{
	if (pixbuf)
		g_object_ref (pixbuf);
	
	if (about->_priv->background_pixbuf)
		g_object_unref (about->_priv->background_pixbuf);

	about->_priv->background_pixbuf = pixbuf;

	gnome_about_update_background (about);
	
	g_print ("setting background %p\n", pixbuf);
}

static void
gnome_about_set_comments (GnomeAbout *about, const gchar *comments)
{
	g_free (about->_priv->comments);
	about->_priv->comments = g_strdup (comments ? comments : "");

	gtk_widget_queue_draw (about->_priv->drawing_area);
}

static void
gnome_about_free_single_person (gpointer data, gpointer user_data)
{
	GnomeAboutEntry *entry = data;

	g_free (entry->name);
	g_free (entry);
}

static void
gnome_about_free_person_list (GSList *list)
{
	if (list == NULL)
		return;
	
	g_slist_foreach (list, gnome_about_free_single_person, NULL);
	g_slist_free (list);
}

static void
gnome_about_set_translator_credits (GnomeAbout *about, const gchar *translator_credits)
{
	g_free (about->_priv->translator_credits);

	about->_priv->translator_credits = g_strdup (translator_credits);

	gtk_widget_queue_draw (about->_priv->drawing_area);
}

static void
gnome_about_set_persons (GnomeAbout *about, guint prop_id, const GValue *persons)
{
	GValueArray *value_array;
	gint i;
	GSList *list;

	/* Free the old list */
	switch (prop_id) {
	case PROP_AUTHORS:
		list = about->_priv->authors;
		break;
	case PROP_DOCUMENTERS:
		list = about->_priv->documenters;
		break;
	default:
		g_assert_not_reached ();
		list = NULL; /* silence warning */
	}

	gnome_about_free_person_list (list);
	list = NULL;
	
	value_array = g_value_get_boxed (persons);

	if (value_array == NULL) {
		return;
	}
	
	for (i = 0; i < value_array->n_values; i++) {
		GnomeAboutEntry *entry;

		entry = g_new0 (GnomeAboutEntry, 1);
		entry->name = g_value_dup_string (&value_array->values[i]);
		entry->is_displaying = FALSE;
		entry->has_been_displayed = FALSE;

		g_assert (&value_array->values[i] == value_array->values + i);
		
		list = g_slist_prepend (list, entry);
	}

	switch (prop_id) {
	case PROP_AUTHORS:
		about->_priv->authors = list;
		break;
	case PROP_DOCUMENTERS:
		about->_priv->documenters = list;
		break;
	default:
		g_assert_not_reached ();
	}
}

static void
gnome_about_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GnomeAbout *about;

	about = GNOME_ABOUT (object);

	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, about->_priv->name);
		break;
	case PROP_VERSION:
		g_value_set_string (value, about->_priv->version);
		break;
	case PROP_COPYRIGHT:
		g_value_set_string (value, about->_priv->copyright);
		break;
	case PROP_COMMENTS:
		g_value_set_string (value, about->_priv->comments);
		break;
	case PROP_TRANSLATOR_CREDITS:
		g_value_set_string (value, about->_priv->translator_credits);
		break;
	case PROP_BACKGROUND:
		g_value_set_object (value, about->_priv->background_pixbuf);
		break;
 	case PROP_BACKGROUND_START_OPACITY:
		g_value_set_float (value, about->_priv->gradient_start_opacity);
		break;
	case PROP_BACKGROUND_END_OPACITY:
		g_value_set_float (value, about->_priv->gradient_end_opacity);
		break;
 	case PROP_BACKGROUND_START_POSITION:
		g_value_set_float (value, about->_priv->gradient_start_position);
		break;
	case PROP_BACKGROUND_END_POSITION:
		g_value_set_float (value, about->_priv->gradient_end_position);
		break;
	case PROP_LOGO:
		g_value_set_object (value, about->_priv->logo_pixbuf);
		break;
	case PROP_LOGO_TOP_PADDING:
		g_value_set_int (value, about->_priv->logo_top_padding);
		break;
	case PROP_LOGO_RIGHT_PADDING:
		g_value_set_int (value, about->_priv->logo_right_padding);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
	g_print ("get property!\n");
}

static void
gnome_about_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GnomeAbout *about;

	about = GNOME_ABOUT (object);
	
	switch (prop_id) {
	case PROP_NAME:
		gnome_about_set_name (about, g_value_get_string (value));
		break;
	case PROP_VERSION:
		gnome_about_set_version (about, g_value_get_string (value));
		break;
	case PROP_COPYRIGHT:
		gnome_about_set_copyright (about, g_value_get_string (value));
		break;
	case PROP_COMMENTS:
		gnome_about_set_comments (about, g_value_get_string (value));
		break;
	case PROP_AUTHORS:
	case PROP_DOCUMENTERS:
		gnome_about_set_persons (about, prop_id, value);
		break;
	case PROP_TRANSLATOR_CREDITS:
		gnome_about_set_translator_credits (about, g_value_get_string (value));
		break;
	case PROP_BACKGROUND:
		gnome_about_set_background (about, g_value_get_object (value));
		break;
	case PROP_BACKGROUND_START_OPACITY:
		about->_priv->gradient_start_opacity = g_value_get_float (value);
		gnome_about_update_background (about);
		break;
	case PROP_BACKGROUND_END_OPACITY:
		about->_priv->gradient_end_opacity = g_value_get_float (value);
		gnome_about_update_background (about);
		break;
	case PROP_BACKGROUND_START_POSITION:
		about->_priv->gradient_start_position = g_value_get_float (value);
		gnome_about_update_background (about);
		break;
	case PROP_BACKGROUND_END_POSITION:
		about->_priv->gradient_end_position = g_value_get_float (value);
		gnome_about_update_background (about);
		break;
	case PROP_LOGO:
		gnome_about_set_logo (about, g_value_get_object (value));
		break;
	case PROP_LOGO_TOP_PADDING:
		about->_priv->logo_top_padding = g_value_get_int (value);
		gtk_widget_queue_draw (about->_priv->drawing_area);
		break;
	case PROP_LOGO_RIGHT_PADDING:
		about->_priv->logo_right_padding = g_value_get_int (value);
		gtk_widget_queue_draw (about->_priv->drawing_area);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

GtkWidget *
gnome_about_new (const gchar  *name,
		 const gchar  *version,
		 const gchar  *copyright,
		 const gchar  *comments,
		 const gchar **authors,
		 const gchar **documenters,
		 const gchar  *translator_credits,
		 GdkPixbuf    *logo_pixbuf)
{
	GnomeAbout *about;
	GValueArray *authors_array;
	GValueArray *documenters_array;
	static GdkPixbuf *background_pixbuf = NULL;
	gint i;
	
	g_return_val_if_fail (authors != NULL, NULL);
	
	about = g_object_new (GNOME_TYPE_ABOUT, NULL);

	authors_array = g_value_array_new (0);
	
	for (i = 0; authors[i] != NULL; i++) {
		GValue value = {0, };
		
		g_value_init (&value, G_TYPE_STRING);
			g_value_set_static_string (&value, authors[i]);
			authors_array = g_value_array_append (authors_array, &value);
	}

	if (documenters != NULL) {
		documenters_array = g_value_array_new (0);

		for (i = 0; documenters[i] != NULL; i++) {
			GValue value = {0, };
			
			g_value_init (&value, G_TYPE_STRING);
			g_value_set_static_string (&value, documenters[i]);
			documenters_array = g_value_array_append (documenters_array, &value);
		}

	}
	else {
		documenters_array = NULL;
	}

	if (background_pixbuf == NULL) {
		background_pixbuf = gdk_pixbuf_new_from_file ("gnome.jpg", NULL);
	}
	
	g_object_set (G_OBJECT (about),
		      "name", name,
		      "version", version,
		      "copyright", copyright,
		      "comments", comments,
		      "authors", authors_array,
		      "documenters", documenters_array,
		      "background", background_pixbuf,
		      "translator_credits", translator_credits,
		      "logo", logo_pixbuf,
		      NULL);

	if (authors_array != NULL) {
		g_value_array_free (authors_array);
	}
	if (documenters_array != NULL) {
		g_value_array_free (documenters_array);
	}

	gtk_window_set_resizable (GTK_WINDOW (about), FALSE);
	
	return GTK_WIDGET (about);
}

static void
gnome_about_finalize (GObject *object)
{
	GnomeAbout *about = GNOME_ABOUT (object);

	g_free (about->_priv->name);
	g_free (about->_priv->version);
	g_free (about->_priv->copyright);
	g_free (about->_priv->comments);

	gnome_about_free_person_list (about->_priv->authors);
	gnome_about_free_person_list (about->_priv->documenters);

	g_free (about->_priv->translator_credits);
	
	if (about->_priv->background_pixbuf)
		gdk_pixbuf_unref (about->_priv->background_pixbuf);
	if (about->_priv->rendered_background_pixbuf)
		gdk_pixbuf_unref (about->_priv->rendered_background_pixbuf);
	if (about->_priv->logo_pixbuf)
		gdk_pixbuf_unref (about->_priv->logo_pixbuf);

	/* sanity: */
	memset (about->_priv, 0xff, sizeof (GnomeAboutPrivate));

	g_free (about->_priv);
	about->_priv = NULL;

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}
