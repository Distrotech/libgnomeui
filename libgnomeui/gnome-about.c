/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 1998 Cesar Miquel <miquel@df.uba.ar>
 * Based in gnome-about, copyright (C) 1998 Horacio J. Peña
 * Rewrite (C) 2000-2001 Gergõ Érdi <cactus@cactus.rulez.org>
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#include <config.h>
#include "gnome-macros.h"

#include "gnome-about.h"
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-url.h>
#include <libgnome/libgnome-init.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-line.h>
#include <libgnomecanvas/gnome-canvas-rect-ellipse.h>
#include <libgnomecanvas/gnome-canvas-text.h>
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include <libgnomecanvas/gnome-canvas-util.h>
#include <bonobo/bonobo-property-bag-client.h>
#include "gnome-cursors.h"
#include <string.h>
#include <gtk/gtk.h>

#define GNOME_ABOUT_DEFAULT_WIDTH               100
#define GNOME_ABOUT_MAX_WIDTH                   600
#define BASE_LINE_SKIP                          0

/* Font size */
#define GNOME_ABOUT_TITLE_SIZE 1.5

/* Layout */
#define GNOME_ABOUT_PADDING_X 1
#define GNOME_ABOUT_PADDING_Y 1

#define GNOME_ABOUT_LOGO_PADDING_X 2
#define GNOME_ABOUT_LOGO_PADDING_Y 0

#define GNOME_ABOUT_TITLE_PADDING_X 1
#define GNOME_ABOUT_TITLE_PADDING_Y 1
#define GNOME_ABOUT_TITLE_PADDING_UP 6
#define GNOME_ABOUT_TITLE_PADDING_DOWN 7

#define GNOME_ABOUT_TITLE_URL_PADDING_UP 2
#define GNOME_ABOUT_TITLE_URL_PADDING_DOWN 4

#define GNOME_ABOUT_CONTENT_PADDING_X GNOME_ABOUT_TITLE_PADDING_X
#define GNOME_ABOUT_CONTENT_PADDING_Y 1
#define GNOME_ABOUT_CONTENT_SPACING 2
#define GNOME_ABOUT_CONTENT_INDENT 3

#define GNOME_ABOUT_COPYRIGHT_PADDING_UP 2
#define GNOME_ABOUT_COPYRIGHT_PADDING_DOWN 4

#define GNOME_ABOUT_AUTHORS_PADDING_Y 3
#define GNOME_ABOUT_AUTHORS_INDENT 10
#define GNOME_ABOUT_AUTHORS_URL_INDENT 10

#define GNOME_ABOUT_COMMENTS_INDENT 3

/* Colors */
#define GNOME_ABOUT_TITLE_BG "black"
#define GNOME_ABOUT_TITLE_FG "white"

#define GNOME_ABOUT_CONTENTS_BG "#76A184"
#define GNOME_ABOUT_CONTENTS_FG "black"

/* Options */
#define GNOME_ABOUT_SHOW_URLS (about->_priv->show_urls)
#define GNOME_ABOUT_SHOW_LOGO (about->_priv->show_logo)

struct _GnomeAboutPrivate {

	GnomeCanvas *canvas;
	
	gchar *title; /* Program title */
	gchar *url; /* Homepage URL */
	gchar *copyright; /* Copyright */

	/* Contact info */
	GList *names, *author_urls, *author_emails;

	gchar *comments;
	GdkPixbuf *logo;

	gint16 logo_width;
	gint16 logo_height;

	gint16 title_text_width;
	gint16 title_text_height;
	gint16 title_url_width;
	gint16 title_url_height;
	
	gint16 title_width;
	gint16 title_height;
	
	gint16 width;
	gint16 height;

	/* Colours */
	GdkColor *title_fg;
	GdkColor *title_bg;
	GdkColor *contents_fg;
	GdkColor *contents_bg;

	gboolean show_urls;
	gboolean show_logo;
};

static void gnome_about_class_init (GnomeAboutClass *klass);
static void gnome_about_init       (GnomeAbout      *about);
static void gnome_about_destroy    (GtkObject       *object);
static void gnome_about_finalize   (GObject         *object);

static void gnome_about_calc_size (GnomeAbout *about);

static void gnome_about_draw      (GnomeAbout *about);

static void gnome_about_load_logo (GnomeAbout *about,
				   const gchar *logo);

static void gnome_about_display_comments (GnomeAbout  *about,
					  gdouble      x,
					  gdouble      y,
					  gdouble      w,
					  const gchar *comments);

static gint gnome_about_item_cb (GnomeCanvasItem *item,
				 GdkEvent *event,
				 gpointer data);

static void gnome_about_parse_name (gchar **name,
				    gchar **email);

static void gnome_about_fill_style (GnomeAbout *about);

static int get_text_width (GtkWidget *widget,
			   gchar     *text);

static int get_text_height (GtkWidget *widget);

/**
 * gnome_about_get_type
 *
 * Returns the GtkType for the GnomeAbout widget.
 **/
/* here the get_type will be defined */
GNOME_CLASS_BOILERPLATE (GnomeAbout, gnome_about,
			 GtkDialog, gtk_dialog)

static void
gnome_about_class_init (GnomeAboutClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) klass;
	gobject_class = (GObjectClass *) klass;

	object_class->destroy = gnome_about_destroy;
	gobject_class->finalize = gnome_about_finalize;
}

static void
gnome_about_init (GnomeAbout *about)
{
	about->_priv = g_new0 (GnomeAboutPrivate, 1);
}


static void
gnome_about_draw (GnomeAbout *about)
{
	GnomeCanvasGroup *root;
	GnomeCanvasItem *item, *item_email G_GNUC_UNUSED;
	gdouble x, y, x2, y2, h, w;
#if 0
	GList *name, *name_url = 0, *name_email;
	gchar *tmp;
#endif

	GnomeAboutPrivate *priv = about->_priv;
	
	root = gnome_canvas_root(priv->canvas);

	x = GNOME_ABOUT_PADDING_X;
	y = GNOME_ABOUT_PADDING_Y;

	/* Logo */
	if (priv->logo && GNOME_ABOUT_SHOW_LOGO) {
		y += GNOME_ABOUT_LOGO_PADDING_Y;
		x = (priv->width - priv->logo_width) / 2;
		item = gnome_canvas_item_new (root,
					      gnome_canvas_pixbuf_get_type (), 
					      "pixbuf", priv->logo, 
					      "x", x, 
					      "x_set", TRUE, 
					      "y", y,
					      "y_set", TRUE, NULL);

		if (priv->url)
		{
			gtk_signal_connect (GTK_OBJECT(item), "event",
					    (GtkSignalFunc) gnome_about_item_cb,
					    NULL);
			gtk_object_set_data (GTK_OBJECT(item), "url",
					     priv->url);
		}
		
		y += priv->logo_height + GNOME_ABOUT_LOGO_PADDING_Y;
	}
	
	/* Draw the title */
	if (priv->title) {

		/* Title background */
		x = GNOME_ABOUT_PADDING_X + GNOME_ABOUT_TITLE_PADDING_X;
		y += GNOME_ABOUT_TITLE_PADDING_Y;
		h = priv->title_height;
		x2 = priv->width - (GNOME_ABOUT_TITLE_PADDING_X +
				    GNOME_ABOUT_PADDING_X) - 1;
		y2 = y + h - 1;
		gnome_canvas_item_new (root,
				       gnome_canvas_rect_get_type(),
				       "x1", x,
				       "y1", y,
				       "x2", x2,
				       "y2", y2,
				       "fill_color", GNOME_ABOUT_TITLE_BG,
				       NULL);
		
		/* Title text */
		x += priv->width / 2;
		y += GNOME_ABOUT_TITLE_PADDING_UP;
		item = gnome_canvas_item_new (root,
					      gnome_canvas_text_get_type(),
					      "text", priv->title,
					      "scale", GNOME_ABOUT_TITLE_SIZE,
					      "x", x,
					      "y", y,
					      "anchor", GTK_ANCHOR_NORTH,
					      "fill_color", GNOME_ABOUT_TITLE_FG,
					      NULL);
		if (priv->url)
		{
			gtk_signal_connect(GTK_OBJECT(item), "event",
					   (GtkSignalFunc) gnome_about_item_cb,
					   GNOME_ABOUT_TITLE_FG);
			gtk_object_set_data(GTK_OBJECT(item), "url", priv->url);
		}
		y += priv->title_text_height + GNOME_ABOUT_TITLE_PADDING_DOWN;

		if (GNOME_ABOUT_SHOW_URLS && priv->url) {
			y += GNOME_ABOUT_TITLE_URL_PADDING_UP;
			item = gnome_canvas_item_new (root,
						      gnome_canvas_text_get_type(),
						      "x", x,
						      "y", y,
						      "anchor", GTK_ANCHOR_NORTH,
						      "fill_color", GNOME_ABOUT_TITLE_FG,
						      "text", priv->url,
						      NULL);
			gtk_signal_connect(GTK_OBJECT(item), "event",
					   (GtkSignalFunc)gnome_about_item_cb,
					   GNOME_ABOUT_TITLE_FG);
			gtk_object_set_data(GTK_OBJECT(item), "url",
					    priv->url);
			
			y += get_text_height (GTK_WIDGET (priv->canvas)) +
				GNOME_ABOUT_TITLE_URL_PADDING_DOWN;
		}
		x = GNOME_ABOUT_PADDING_X;
		y += GNOME_ABOUT_TITLE_PADDING_Y;
	}

	/* Contents */
	if (priv->copyright || priv->names || priv->comments)
	{
		x = GNOME_ABOUT_PADDING_X + GNOME_ABOUT_CONTENT_PADDING_X;
		y += GNOME_ABOUT_CONTENT_PADDING_Y;
		x2 = priv->width - (GNOME_ABOUT_CONTENT_PADDING_X +
				    GNOME_ABOUT_PADDING_X) - 1;
		y2 = priv->height - (GNOME_ABOUT_CONTENT_PADDING_Y +
				     GNOME_ABOUT_PADDING_Y) - 1;

		gnome_canvas_item_new (root,
				       gnome_canvas_rect_get_type(),
				       "x1", x,
				       "y1", y,
				       "x2", x2,
				       "y2", y2,
				       "fill_color", GNOME_ABOUT_CONTENTS_BG,
				       NULL);
	}

	/* Copyright message */
	if (priv->copyright) {
		x = GNOME_ABOUT_PADDING_X +
			GNOME_ABOUT_CONTENT_PADDING_X +
			GNOME_ABOUT_CONTENT_INDENT;
		y += GNOME_ABOUT_CONTENT_SPACING +
			GNOME_ABOUT_COPYRIGHT_PADDING_UP;

		gnome_canvas_item_new (root,
				       gnome_canvas_text_get_type(),
				       "text", priv->copyright,
				       "x", x,
				       "y", y,
				       "anchor", GTK_ANCHOR_NORTH_WEST,
				       "fill_color", GNOME_ABOUT_CONTENTS_FG,
				       NULL);

		y += get_text_height (GTK_WIDGET (priv->canvas)) +
			GNOME_ABOUT_COPYRIGHT_PADDING_DOWN;
	}
	
#if 0
	/* Authors header */
	if (priv->names) {
		y += GNOME_ABOUT_CONTENT_SPACING;
		x = GNOME_ABOUT_PADDING_X +
			GNOME_ABOUT_CONTENT_PADDING_X +
			GNOME_ABOUT_CONTENT_INDENT;
		if (g_list_length (priv->names) == 1)
			gnome_canvas_item_new (root,
					       gnome_canvas_text_get_type(),
					       "x", x,
					       "y", y,
					       "anchor", GTK_ANCHOR_NORTH_WEST,
					       "text", _("Author:"),
					       "fill_color", GNOME_ABOUT_CONTENTS_FG,
					       NULL);
		else
			gnome_canvas_item_new (root,
					       gnome_canvas_text_get_type(),
					       "x", x,
					       "y", y,
					       "anchor", GTK_ANCHOR_NORTH_WEST,
					       "text", _("Authors:"),
					       "fill_color", GNOME_ABOUT_CONTENTS_FG,
					       NULL);
	}
	x += gdk_string_measure (priv->font_author, _("Authors:")) +
		GNOME_ABOUT_AUTHORS_INDENT;

	/* Authors list */
	name = g_list_first (priv->names);
	name_email = g_list_first (priv->author_emails);
	if (priv->author_urls)
	    name_url = g_list_first (priv->author_urls);

	while (name) {
		h = priv->font_names->descent + priv->font_names->ascent;
		w = gdk_string_measure(priv->font_names, (gchar*) name->data) +
			gdk_string_measure(priv->font_names, " ");

		item = gnome_canvas_item_new (root,
					      gnome_canvas_text_get_type(),
					      "x", x,
					      "y", y,
					      "anchor", GTK_ANCHOR_NORTH_WEST,
					      "fill_color_gdk", priv->contents_fg,
					      "text", (gchar*) name->data,
					      NULL);

		if (name_email->data) {
			x2 = x + w;
			tmp = g_strdup_printf("<%s>", (gchar*) name_email->data);
			item_email = gnome_canvas_item_new (root,
							    gnome_canvas_text_get_type(),
							    "x", x2,
							    "y", y,
							    "anchor", GTK_ANCHOR_NORTH_WEST,
							    "fill_color_gdk", priv->contents_fg,
							    "text", tmp,
							    NULL);

			tmp = g_strdup_printf("mailto:%s", (gchar*) name_email->data);
			gtk_signal_connect (GTK_OBJECT (item_email), "event",
					    (GtkSignalFunc) gnome_about_item_cb,
					    GNOME_ABOUT_CONTENTS_FG);
			gtk_object_set_data_full (GTK_OBJECT (item_email),
						  "url", tmp,
						  (GtkDestroyNotify) g_free);
		}
		
		y += h + BASE_LINE_SKIP;
		name = name->next;
		name_email = name_email->next;

		if (name_url) {
			if (name_url->data) {
				gtk_signal_connect (GTK_OBJECT(item), "event",
						    (GtkSignalFunc) gnome_about_item_cb,
						    GNOME_ABOUT_CONTENTS_FG);
				gtk_object_set_data(GTK_OBJECT(item),
						    "url", 
						    (gchar*)name_url->data);

				if (GNOME_ABOUT_SHOW_URLS) {
					x += GNOME_ABOUT_AUTHORS_URL_INDENT;
					h = priv->font_names_url->descent + 
						priv->font_names->ascent;

					item = gnome_canvas_item_new (root,
								      gnome_canvas_text_get_type(),
								      "x", x,
								      "y", y,
								      "anchor", GTK_ANCHOR_NORTH_WEST,
								      "fill_color_gdk", priv->contents_fg,
								      "text", (gchar*) name_url->data,
								      NULL);
					gtk_signal_connect( GTK_OBJECT(item), 
							    "event",
							    (GtkSignalFunc) gnome_about_item_cb,
							    GNOME_ABOUT_CONTENTS_FG);
					gtk_object_set_data(GTK_OBJECT(item),
							    "url", 
							    (gchar*)name_url->data);
					y += h + BASE_LINE_SKIP;
					x -= GNOME_ABOUT_AUTHORS_URL_INDENT;
				}
			}
			name_url = name_url->next;
		}
	}
#endif

	if (priv->comments) {
		y += GNOME_ABOUT_CONTENT_SPACING;
		x = GNOME_ABOUT_PADDING_X +
			GNOME_ABOUT_CONTENT_PADDING_X +
			GNOME_ABOUT_CONTENT_INDENT +
			GNOME_ABOUT_COMMENTS_INDENT;
		w = priv->width - (2 * (GNOME_ABOUT_PADDING_X +
					GNOME_ABOUT_CONTENT_PADDING_X) +
				   GNOME_ABOUT_CONTENT_INDENT +
				   GNOME_ABOUT_PADDING_Y);
		gnome_about_display_comments (about, 
					      x, y, w,
					      priv->comments);
	}
	
	gnome_canvas_set_scroll_region(about->_priv->canvas,
				       0, 0,
				       about->_priv->width, about->_priv->height);
	gtk_widget_set_usize(GTK_WIDGET(about->_priv->canvas),
			     about->_priv->width, about->_priv->height);
}

static void
gnome_about_display_comments (GnomeAbout  *about,
			      gdouble      x,
			      gdouble      y,
			      gdouble      w,
			      const gchar *comments)
{
	char *tok, *p1, *p2, *p3, c;
	char *buffer;
	gdouble ypos, width;
	int done;
	GList *par, *tmp;
	char *tokp;
	GtkWidget *widget = GTK_WIDGET (about->_priv->canvas);
	GnomeCanvasGroup *group = gnome_canvas_root (about->_priv->canvas);
	
	width = w - 2 * (GNOME_ABOUT_PADDING_X +
			 GNOME_ABOUT_CONTENT_PADDING_X +
			 GNOME_ABOUT_CONTENT_INDENT +
			 GNOME_ABOUT_COMMENTS_INDENT);
	if (comments == NULL)
		return;

	/* we need a buffer because strtok will modify the string! */
	/* strtok is a cool but dangerous function. */
	buffer = g_strdup(comments);

	ypos = y;

	/* Make a list with each paragraph */
	par = (GList *)NULL;

	for (tok = strtok_r (buffer, "\n", &tokp); tok;
	     tok = strtok_r (NULL, "\n", &tokp)) {
		par = g_list_append (par, g_strdup(tok));
	}

	/* Print each paragraph */
	tmp = par;
	while (tmp != NULL) {
		done = FALSE;
		p1 = p2 = tmp->data;
		while (!done) {
			c = *p1; *p1 = 0;
			while (get_text_width (widget, p2) < width && c != 0) {
				*p1 = c;
				p1++;
				c = *p1;
				*p1 = 0;
			}
			switch (c) {
			case ' ':
				p1++;
				gnome_canvas_item_new (group,
						       gnome_canvas_text_get_type(),
						       "x", x,
						       "y", ypos,
						       "anchor", GTK_ANCHOR_NORTH_WEST,
						       "fill_color", GNOME_ABOUT_CONTENTS_FG,
						       "text", p2,
						       NULL);
				
				break;
			case '\0':
				done = TRUE;
				gnome_canvas_item_new (group,
						       gnome_canvas_text_get_type(),
						       "x", x,
						       "y", ypos,
						       "anchor", GTK_ANCHOR_NORTH_WEST,
						       "fill_color", GNOME_ABOUT_CONTENTS_FG,
						       "text", p2,
						       NULL);
				break;
			default:
				p3 = p1;
				while (*p1 != ' ' && p1 > p2)
					p1--;
				if (p1 == p2) {
					gnome_canvas_item_new (group,
							       gnome_canvas_text_get_type(),
							       "x", x,
							       "y", ypos,
							       "anchor", GTK_ANCHOR_NORTH_WEST,
							       "fill_color", GNOME_ABOUT_CONTENTS_FG,
							       "text", p2,
							       NULL);

					*p3 = c;
					p1 = p3;
					break;
				} else {
					*p3 = c;
					*p1 = 0;
					p1++;
					gnome_canvas_item_new (group,
							       gnome_canvas_text_get_type(),
							       "x", x,
							       "y", ypos,
							       "anchor", GTK_ANCHOR_NORTH_WEST,
							       "fill_color", GNOME_ABOUT_CONTENTS_FG,
							       "text", p2,
							       NULL);
				}
				break;
			}
			ypos += get_text_height (widget);
			p2 = p1;
		}
		tmp = tmp->next;
		ypos += BASE_LINE_SKIP;	/* Skip a bit */
	}

	/* Free list memory */
	g_list_foreach(par, (GFunc)g_free, NULL);
	g_list_free (par);
	g_free (buffer);
}

static int
get_text_width (GtkWidget *widget,
		gchar     *text)
{
	PangoContext *context = gtk_widget_get_pango_context (widget);
	PangoLayout  *layout  = pango_layout_new (context);

	int pango_width, pango_height;

	pango_layout_set_text (layout, text, -1);
	pango_layout_get_size (layout, &pango_width, &pango_height);

	return pango_width / PANGO_SCALE;
}

static int
get_text_height (GtkWidget *widget)
{
	PangoContext *context = gtk_widget_get_pango_context (widget);
	PangoLayout  *layout  = pango_layout_new (context);

	int pango_width, pango_height;

//	pango_layout_set_text (layout, text, -1);
	pango_layout_get_size (layout, &pango_width, &pango_height);

	return pango_height / PANGO_SCALE;
}


/* ----------------------------------------------------------------------
   NAME:	gnome_about_calc_size
   DESCRIPTION:	Calculates size of window
   ---------------------------------------------------------------------- */

static void
gnome_about_calc_size (GnomeAbout *about)
{
	gint num_pars, i, h, w, len[5], pw;
#if 0
	GList *name, *email = NULL;
	GList *urls = NULL;
	gint tmpl;
#endif
	gint title_h;
	const gchar *p;
	gfloat maxlen;

	PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (about->_priv->canvas));
	PangoLayout  *layout  = pango_layout_new (context);
	int pango_width, pango_height;
	
	w = GNOME_ABOUT_DEFAULT_WIDTH;
	h = GNOME_ABOUT_PADDING_Y;

	pw = 0; /* Maximum of title and content padding */

	len[0] = 0;
	len[1] = 0;
	title_h = 0;
	if (about->_priv->title)
	{
		h += GNOME_ABOUT_TITLE_PADDING_Y;

		pango_layout_set_text (layout, about->_priv->title, -1);
		pango_layout_get_size (layout, &pango_width, &pango_height);

		about->_priv->title_text_width  = (pango_width / PANGO_SCALE) * GNOME_ABOUT_TITLE_SIZE;
		about->_priv->title_text_height = (pango_height / PANGO_SCALE) * GNOME_ABOUT_TITLE_SIZE;
	
		
		len[0] = about->_priv->title_text_width;
		title_h += GNOME_ABOUT_TITLE_PADDING_UP +
			about->_priv->title_text_height +
			GNOME_ABOUT_TITLE_PADDING_DOWN;
			
		h += GNOME_ABOUT_TITLE_PADDING_Y;
		pw = GNOME_ABOUT_TITLE_PADDING_X;
		if (GNOME_ABOUT_SHOW_URLS && about->_priv->url)
		{
			pango_layout_set_text (layout, about->_priv->url, -1);
			pango_layout_get_size (layout, &pango_width, &pango_height);
			
			len[1] = pango_width / PANGO_SCALE;
			title_h += GNOME_ABOUT_TITLE_URL_PADDING_UP +
				pango_height / PANGO_SCALE +
				GNOME_ABOUT_TITLE_URL_PADDING_DOWN;
		}
	}
	about->_priv->title_height = title_h;
	h += title_h;

	if (about->_priv->copyright ||
	    about->_priv->names ||
	    about->_priv->comments)
	{
		h += 2 * GNOME_ABOUT_CONTENT_PADDING_Y;
		pw = MAX(pw, GNOME_ABOUT_CONTENT_PADDING_X);
	}

	len[2] = 0;
	if (about->_priv->copyright)
	{
		pango_layout_set_text (layout, about->_priv->copyright, -1);
		pango_layout_get_size (layout, &pango_width, &pango_height);

		h += GNOME_ABOUT_COPYRIGHT_PADDING_UP +
			pango_height / PANGO_SCALE +
			GNOME_ABOUT_COPYRIGHT_PADDING_DOWN +
			GNOME_ABOUT_CONTENT_SPACING;
		len[2] = pango_width / PANGO_SCALE;
	}

#if 0
	len[3] = 0;
	len[4] = 0;
	if (priv->names) {
		name = g_list_first (priv->names);
		if (priv->author_emails)
			email = g_list_first (priv->author_emails);
		if (GNOME_ABOUT_SHOW_URLS && priv->author_urls)
			urls = g_list_first (priv->author_urls);
		while (name) {
			tmpl = gdk_string_measure (priv->font_names, name->data);
			if (email)
				if (email->data) {
					tmpl += gdk_string_measure(
						priv->font_names, " ");
					tmpl += gdk_string_measure(
						priv->font_names,
						(gchar*)email->data);
				}
			if (tmpl > len[3])
				len[3] = tmpl;
			h += (priv->font_names->descent +
			      priv->font_names->ascent) +
				BASE_LINE_SKIP;

			if (GNOME_ABOUT_SHOW_URLS && urls) {
				if (urls->data) {
					tmpl = gdk_string_measure (priv->font_names_url, urls->data);
					if (tmpl > len[4])
						len[4] = tmpl;
					h += (priv->font_names_url->descent +
					      priv->font_names_url->ascent) + BASE_LINE_SKIP;
				}
				urls = urls->next;
			}
			name = name->next;
			if (email)
				email = email->next;
		}
		tmpl = gdk_string_measure (priv->font_names, _("Authors: "));
		len[3] += tmpl +
			GNOME_ABOUT_PADDING_X +
			GNOME_ABOUT_CONTENT_PADDING_X +
			GNOME_ABOUT_CONTENT_INDENT +
			GNOME_ABOUT_AUTHORS_INDENT;
		h += GNOME_ABOUT_AUTHORS_PADDING_Y +
			GNOME_ABOUT_CONTENT_SPACING;
	}
#endif
	
	maxlen = (gfloat) w;
#if 0
	for (i = 0; i < 5; i++)
#else
	for (i = 0; i < 3; i++)
#endif
		if (len[i] > maxlen)
			maxlen = (gfloat) len[i];
	
	w = (int) (maxlen * 1.2);
	if (w > GNOME_ABOUT_MAX_WIDTH)
		w = GNOME_ABOUT_MAX_WIDTH;

	if (about->_priv->comments) {
		h += GNOME_ABOUT_CONTENT_SPACING;
		num_pars = 1;
		p = about->_priv->comments;
		while (*p != '\0') {
			if (*p == '\n')
				num_pars++;
			p++;
		}
		
		pango_layout_set_text (layout, about->_priv->comments, -1);
		pango_layout_get_size (layout, &pango_width, NULL);

		i = pango_width / PANGO_SCALE;
		
		i /= w - 2 * (GNOME_ABOUT_PADDING_X +
			      GNOME_ABOUT_CONTENT_PADDING_X +
			      GNOME_ABOUT_CONTENT_INDENT +
			      GNOME_ABOUT_COMMENTS_INDENT);
		i += 1 + num_pars;;

		/*
		 * The height of a single line is calculated from the
		 * height of an empty string
		 */
		pango_layout_set_text (layout, 0, 0);
		pango_layout_get_size (layout, NULL, &pango_height);
		
		h += i * (pango_height / PANGO_SCALE);
	}

	g_object_unref (G_OBJECT (layout));

	about->_priv->width = w + 2 * (GNOME_ABOUT_PADDING_X + pw);
	about->_priv->height = h + GNOME_ABOUT_PADDING_Y;

	about->_priv->height += 4 + about->_priv->logo_height;
	about->_priv->width = MAX (about->_priv->width,
				   about->_priv->logo_width +
				   2 * (GNOME_ABOUT_PADDING_X + GNOME_ABOUT_LOGO_PADDING_X));
}

static void
gnome_about_load_logo(GnomeAbout  *about, 
		      const gchar *logo)
{
	gchar *filename;

	if (logo && GNOME_ABOUT_SHOW_LOGO) {
		if (g_path_is_absolute (logo) && g_file_test (logo, G_FILE_TEST_EXISTS))
			filename = g_strdup (logo);
		else
			filename = gnome_program_locate_file
				(gnome_program_get (),
				 GNOME_FILE_DOMAIN_PIXMAP,
				 logo, TRUE, NULL);
		
		if (filename != NULL) {
			GdkPixbuf *pixbuf;
			GError *error;

			error = NULL;
			pixbuf = gdk_pixbuf_new_from_file(filename, &error);
			if (error != NULL) {
				g_warning (G_STRLOC ": cannot load `%s': %s",
					   filename, error->message);
				g_error_free (error);
			}

			if (pixbuf != NULL) {
				about->_priv->logo = pixbuf;
				about->_priv->logo_width = gdk_pixbuf_get_width (pixbuf);
				about->_priv->logo_height = gdk_pixbuf_get_height (pixbuf);
				
			}
			
			g_free (filename);
		}
	}
}

/* ----------------------------------------------------------------------
   NAME:	gnome_fill_info
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static void
gnome_about_fill_info (GnomeAbout  *about,
		       const gchar *title,
		       const gchar *version,
		       const gchar *url,
		       const gchar *copyright,
		       const gchar **authors,
		       const gchar **author_urls,
		       const gchar *comments,
		       const gchar *logo)
{
	int author_num = 0, i;

	/* Fill options */
	gnome_about_fill_style (about);

	/* Load logo */
	gnome_about_load_logo (about, logo);
	
	/* fill struct */
	if (title)
		about->_priv->title = g_strconcat (title, " ", 
						  version ? version : "", NULL);
	else
		about->_priv->title = NULL;
	
	if (url)
		about->_priv->url = g_strdup (url);
	else
		about->_priv->url = NULL;
	
	if (copyright)
		about->_priv->copyright = g_strdup (copyright);
	else
		about->_priv->copyright = NULL;

	if (comments)
		about->_priv->comments = g_strdup (comments);
	else
		about->_priv->comments = NULL;

	if (authors && authors[0]) {
		gchar *name, *email;
		while (*authors) {
			name = g_strdup (*authors);
			gnome_about_parse_name (&name, &email);
			
			about->_priv->names = g_list_append (about->_priv->names, name);
			about->_priv->author_emails = g_list_append (about->_priv->author_emails, 
								     email);
			
			authors++;
			author_num++;
		}
	}

	about->_priv->author_urls = NULL;
	if (author_urls) {
		for (i = 0; i < author_num; i++, author_urls++) {
			about->_priv->author_urls = g_list_append (
				about->_priv->author_urls,
				g_strdup (*author_urls));
		}
	}
	
        /* Calculate width of window */
	gnome_about_calc_size (about);
}

static void gnome_about_fill_style (GnomeAbout *about)
{
	GtkStyle *style G_GNUC_UNUSED;
    
	Bonobo_ConfigDatabase cd = gnome_program_get_config_database (gnome_program_get ());

	g_return_if_fail (about->_priv != NULL);

	/* Read GConf options */
	about->_priv->show_urls = bonobo_pbclient_get_boolean (cd, "/about/show_urls", NULL);
	about->_priv->show_logo = bonobo_pbclient_get_boolean (cd, "/about/show_logo", NULL);

#if 0
	/* Create fonts and get colors*/
	/* FIXME: dirty hack, but it solves i18n problem without rewriting the
	   drawing code..  */
  	GTK_WIDGET_SET_FLAGS(widget, GTK_RC_STYLE); 
	style = gtk_style_ref (widget->style);
	gtk_widget_ensure_style (widget);

	gtk_widget_set_name (widget, "Title");
	priv->title_bg = gdk_color_copy (&widget->style->bg[GTK_STATE_NORMAL]);
	g_print ("title:RGB, %d, %d, %d\n", priv->title_bg->red,
		 priv->title_bg->green, priv->title_bg->blue);
	priv->title_fg = gdk_color_copy (&widget->style->fg[GTK_STATE_NORMAL]);
	gdk_color_alloc (gtk_widget_get_default_colormap (), priv->title_bg);
	gdk_color_alloc (gtk_widget_get_default_colormap (), priv->title_fg);

	gtk_widget_set_name (widget, "Comments");
	priv->contents_bg = gdk_color_copy (&widget->style->bg[GTK_STATE_NORMAL]);
	priv->contents_fg = gdk_color_copy (&widget->style->fg[GTK_STATE_NORMAL]);
	gdk_color_alloc (gtk_widget_get_default_colormap (), priv->contents_bg);
	gdk_color_alloc (gtk_widget_get_default_colormap (), priv->contents_fg);
	
	gtk_widget_set_style (widget, style);
	gtk_style_unref (style);
#else
	about->_priv->title_bg = g_new (GdkColor, 1);
	about->_priv->title_bg->red = 0;
	about->_priv->title_bg->green = 0;
	about->_priv->title_bg->blue = 0;
	
	about->_priv->title_fg = g_new (GdkColor, 1);
	about->_priv->title_fg->red = 255;
	about->_priv->title_fg->green = 255;
	about->_priv->title_fg->blue = 255;
	
	about->_priv->contents_bg = g_new (GdkColor, 1);
	about->_priv->contents_bg->red = 0;
	about->_priv->contents_bg->green = 0;
	about->_priv->contents_bg->blue = 0;
	
	about->_priv->contents_fg = g_new (GdkColor, 1);
	about->_priv->contents_fg->red = 255;
	about->_priv->contents_fg->green = 255;
	about->_priv->contents_fg->blue = 255;
#endif
}

/* ----------------------------------------------------------------------
   NAME:	gnome_about_destroy
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static void
gnome_about_destroy (GtkObject *object)
{
	GnomeAbout *self = GNOME_ABOUT(object);

	/* remember, destroy can be run multiple times! */

	if(self->_priv) {
		/* Free memory used for title, copyright and comments */
		g_free (self->_priv->title);
		g_free (self->_priv->copyright);
		g_free (self->_priv->comments);

#if 0
		/* Free GUI's. */
		pango_font_description_free (self->_priv->font_title);
		pango_font_description_free (self->_priv->font_url);
		pango_font_description_free (self->_priv->font_copyright);
		pango_font_description_free (self->_priv->font_author);
		pango_font_description_free (self->_priv->font_names);
		pango_font_description_free (self->_priv->font_comments);
#endif
		
		/* Free colors */
		gdk_color_free (self->_priv->title_fg);
		gdk_color_free (self->_priv->title_bg);
		gdk_color_free (self->_priv->contents_fg);
		gdk_color_free (self->_priv->contents_bg);


		/* Free memory used for authors */
		g_list_foreach (self->_priv->names, (GFunc) g_free, NULL);
		g_list_free (self->_priv->names);
	}
	
	g_free (self->_priv);
	self->_priv = NULL;

	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_about_finalize (GObject *object)
{
	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}

/**
 * gnome_about_new
 * @title: Name of app.
 * @version: version number
 * @copyright: copyright string
 * @authors: %NULL terminated list of authors
 * @comments: Other comments
 * @logo: a logo pixmap file.
 *
 * Creates a new GNOME About dialog.  @title, @version,
 * @copyright, and @authors are displayed first, in that order.
 * @comments is typically the location for multiple lines of text, if
 * necessary.  (Separate with "\n".)  @logo is the filename of a
 * optional pixmap to be displayed in the dialog, typically a product or
 * company logo of some sort; set to %NULL if no logo file is available.
 *
 * Returns:
 * &GtkWidget pointer to new dialog
 **/

GtkWidget*
gnome_about_new (const gchar *title,
		 const gchar *version,
		 const gchar *copyright,
		 const gchar **authors,
		 const gchar *comments,
		 const gchar *logo)
{
    return gnome_about_new_with_url (title, version, NULL, copyright,
				     authors, NULL, comments, logo);
}

/**
 * gnome_about_new_with_url
 * @title: Name of app.
 * @version: version number
 * @url: Application URL
 * @copyright: copyright string
 * @authors: %NULL terminated list of authors
 * @author_urls: %NULL terminated list of URLs associated with the authors
 * @comments: Other comments
 * @logo: a logo pixmap file.
 *
 * Creates a new GNOME About dialog.  @title, @version,
 * @copyright, and @authors are displayed first, in that order.
 * @comments is typically the location for multiple lines of text, if
 * necessary.  (Separate with "\n".)  @logo is the filename of a
 * optional pixmap to be displayed in the dialog, typically a product or
 * company logo of some sort; set to %NULL if no logo file is available.
 * The @url argument specifies a URL to load if the user clicks on
 * either the title or the logo. Pass %NULL if no URL is available.
 *
 * The @author_urls is a list of URLs you'd like to be associated with
 * the authors. It must be either %NULL or exactly as long as @authors.
 *
 * Returns:
 * &GtkWidget pointer to new dialog
 **/

GtkWidget*
gnome_about_new_with_url (const gchar *title,
			  const gchar *version,
			  const gchar *url,
			  const gchar *copyright,
			  const gchar **authors,
			  const gchar **author_urls,
			  const gchar *comments,
			  const gchar *logo)
{
    GnomeAbout *about;
    
    if (!title) title = "";
    if (!version) version = "";
    if (!copyright) copyright = "";
/*	g_return_val_if_fail (authors != NULL, NULL);*/
    if (!comments) comments = "";
    
    about = gtk_type_new (gnome_about_get_type());
    
    gnome_about_construct(about, title, version, url, copyright,
			  authors, author_urls, comments, logo);
    
    return GTK_WIDGET (about);
}

/**
 * gnome_about_construct
 * @about: Pointer to new GNOME about object
 * @title: Name of app.
 * @version: version number
 * @url: Application URL
 * @copyright: copyright string
 * @authors: %NULL terminated list of authors
 * @author_urls: %NULL terminated list of URLs associated with the authors
 * @comments: Other comments
 * @logo: a logo pixmap file.
 *
 * Constructs a new GNOME About dialog, given an object
 * @about newly created via the Gtk type system.  @title, @version,
 * @copyright, and @authors are displayed first, in that order.
 * @comments is typically the location for multiple lines of text, if
 * necessary.  (Separate with "\n".)  @logo is the filename of a
 * optional pixmap to be displayed in the dialog, typically a product or
 * company logo of some sort; set to %NULL if no logo file is available.
 * The @url argument specifies a URL to load if the user clicks on
 * either the title or the logo. Pass %NULL if no URL is available.
 *
 * The @author_urls is a list of URLs you'd like to be associated with
 * the authors. It must be either %NULL or exactly as long as @authors.
 *
 * Note:
 * Only for use by bindings to languages other than C; don't use
 * in applications.
 *
 */
void
gnome_about_construct (GnomeAbout *about,
		       const gchar *title,
		       const gchar *version,
		       const gchar *url,
		       const gchar *copyright,
		       const gchar **authors,
		       const gchar **author_urls,
		       const gchar *comments,
		       const gchar *logo)
{
	GtkWidget *frame;
	
	gtk_window_set_title (GTK_WINDOW(about), _("About"));
	gtk_window_set_policy (GTK_WINDOW(about), FALSE, FALSE, TRUE);

	/* x = (gdk_screen_width ()  - w) / 2; */
	/* y = (gdk_screen_height () - h) / 2;    */
	/* gtk_widget_set_uposition ( GTK_WIDGET (about), x, y); */

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
	gtk_container_add(GTK_CONTAINER (GTK_DIALOG(about)->vbox),
			  GTK_WIDGET(frame));
	gtk_widget_show (frame);
	
//        gtk_widget_push_colormap(gdk_rgb_get_cmap());
	about->_priv->canvas = GNOME_CANVAS (gnome_canvas_new ());
//        gtk_widget_pop_colormap();

	gtk_container_add (GTK_CONTAINER(frame),
			   GTK_WIDGET (about->_priv->canvas));

	/* Fill the GnomeAboutPriv structure */
	gnome_about_fill_info (about,
			       title, version, url,
			       copyright,
			       authors, author_urls,
			       comments,
			       logo);

	gnome_about_draw (about);
	gtk_widget_show (GTK_WIDGET (about->_priv->canvas));

	gtk_dialog_add_button (GTK_DIALOG (about),
			       GTK_STOCK_OK,
			       GTK_RESPONSE_OK);
}

static gint
gnome_about_item_cb (GnomeCanvasItem *item, GdkEvent *event,
		     gpointer data)
{
	
	gchar *url = gtk_object_get_data(GTK_OBJECT(item), "url");
	GdkCursor *cursor;
	GdkWindow *window = GTK_WIDGET(item->canvas)->window;
	static GnomeCanvasItem *underline = NULL;

	if (!url)
		return 0;
	
	
	switch (event->type) {
	case GDK_ENTER_NOTIFY:
		if (url) {
			if (underline == NULL) {
				gdouble x1, x2, y1, y2;
				GnomeCanvasPoints *points;
				gchar *color = (gchar*)data;
				
				if (color) {
					points = gnome_canvas_points_new (2);
					
					gnome_canvas_item_get_bounds (item, &x1, &y1, &x2, &y2);
					
					points->coords[0] = x1;
					points->coords[1] = y2;
					points->coords[2] = x2;
					points->coords[3] = y2;
					
					underline = gnome_canvas_item_new (gnome_canvas_root (item->canvas),
									   gnome_canvas_line_get_type (),
									   "points", points,
									   "fill_color", color,
									   NULL);
					gnome_canvas_points_unref (points);
				}
			}
			cursor = gnome_stock_cursor_new (GNOME_STOCK_CURSOR_POINTING_HAND); 
			gdk_window_set_cursor(window, cursor);
			gdk_cursor_destroy(cursor);
		}
		break;
	case GDK_LEAVE_NOTIFY:
		if (url) {
			if (underline != NULL) {
				gtk_object_destroy (GTK_OBJECT (underline));
				underline = NULL;
			}
			gdk_window_set_cursor(window, NULL);
		}
		break;
	case GDK_BUTTON_PRESS:
		if (url) {
			if(!gnome_url_show(url)) {
				GtkWidget *dialog;

				dialog = gtk_message_dialog_new (NULL, 0,
								 GTK_MESSAGE_ERROR,
								 GTK_BUTTONS_OK,
								 _("Error occured while "
								   "trying to launch the "
								   "URL handler.\n"
								   "Please check the "
								   "settings in the "
								   "Control Center if they "
								   "are correct."));

				gtk_dialog_run (GTK_DIALOG (dialog));
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

static void
gnome_about_parse_name(gchar **name, gchar **email)
{
	int closed_bracket = 0;
	int at_num = 0;
	gchar *buffer, *name_buf = *name;
	int buffer_len = 0;
	gchar *email_start = 0;
	
	buffer = g_new(gchar, strlen(*name));

	for (name_buf = strchr(*name, '<'); name_buf;
	     name_buf = strchr(name_buf++, '<')) {
		email_start = name_buf;
		closed_bracket = at_num = 0;
		name_buf++;
		
		for (buffer_len = 0; *name_buf; name_buf++, buffer_len++) {
			if (*name_buf == '>') {
				closed_bracket = 1;
				break;
			}
			if (*name_buf == '<') {
				name_buf--;
				break;
			}
			if (*name_buf == '@')
				at_num++;
			
			buffer[buffer_len] = *name_buf;
		}
	}
	
	buffer[buffer_len] = '\0';
	if (closed_bracket && at_num == 1) {
		*email = buffer;
		*email_start = '\0';
		return;
	}

	g_free(buffer);
	*email = NULL;
}

