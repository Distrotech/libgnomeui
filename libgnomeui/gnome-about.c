/* -*- Mode: C; c-set-style: linux indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 1998 Cesar Miquel <miquel@df.uba.ar>
 * Based in gnome-about, copyright (C) 1998 Horacio J. Peña
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
#include <bonobo/bonobo-config-database.h>
#include "gnome-cursors.h"
#include <string.h>
#include <gtk/gtk.h>

#define GNOME_ABOUT_DEFAULT_WIDTH               100
#define GNOME_ABOUT_MAX_WIDTH                   600
#define BASE_LINE_SKIP                          0

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

/* Options */
#define GNOME_ABOUT_SHOW_URLS (priv->show_urls)
#define GNOME_ABOUT_SHOW_LOGO (priv->show_logo)

struct _GnomeAboutPrivate {
	gchar *title; /* Program title */
	gchar *url; /* Homepage URL */
	gchar *copyright; /* Copyright */

	/* Contact info */
	GList *names, *author_urls, *author_emails;

	gchar *comments;
	GdkPixbuf *logo;

	gint16 logo_width;
	gint16 logo_height;
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

	/* Fonts */
	GdkFont *font_title;
	GdkFont *font_url;
	GdkFont *font_copyright;
	GdkFont *font_author;
	GdkFont *font_names;
	GdkFont *font_names_url;
	GdkFont *font_comments;
};

static void gnome_about_class_init (GnomeAboutClass *klass);
static void gnome_about_init       (GnomeAbout      *about);
static void gnome_about_destroy    (GtkObject       *object);
static void gnome_about_finalize   (GObject         *object);

static void gnome_about_calc_size (GnomeAboutPrivate *priv);

static void gnome_about_draw (GnomeCanvas *canvas, GnomeAboutPrivate *priv);

static void gnome_about_load_logo (GnomeAboutPrivate *priv, const gchar *logo);

static void gnome_about_display_comments (GnomeCanvasGroup *group,
					  GdkFont *font,
					  GdkColor *color,
					  gdouble x, gdouble y, gdouble w,
					  const gchar *comments);

static gint gnome_about_item_cb (GnomeCanvasItem *item, GdkEvent
				 *event, gpointer data);
static void gnome_about_parse_name (gchar **name, gchar **email);

static void gnome_about_fill_options (GtkWidget *widget,
				      GnomeAboutPrivate *priv);

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
gnome_about_draw (GnomeCanvas *canvas, 
		  GnomeAboutPrivate *priv)
{
	GnomeCanvasGroup *root;
	GnomeCanvasItem *item, *item_email;
	gdouble x, y, x2, y2, h, w;
	GList *name, *name_url = 0, *name_email;
	gchar *tmp;

	root = gnome_canvas_root(canvas);

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
		if (priv->url) {
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
				       "fill_color_gdk", priv->title_bg,
				       NULL);

		/* Title text */
		x += priv->width / 2;
		y += GNOME_ABOUT_TITLE_PADDING_UP;
		item = gnome_canvas_item_new (root,
					      gnome_canvas_text_get_type(),
					      "x", x,
					      "y", y,
					      "anchor", GTK_ANCHOR_NORTH,
					      "fill_color_gdk", priv->title_fg,
					      "text", priv->title,
					      "font_gdk", priv->font_title,
					      NULL);
		if (priv->url) {
			gtk_signal_connect(GTK_OBJECT(item), "event",
					   (GtkSignalFunc) gnome_about_item_cb,
					   priv->title_fg);
			gtk_object_set_data(GTK_OBJECT(item), "url", priv->url);
		}

		y += priv->font_title->ascent + priv->font_title->descent +
			GNOME_ABOUT_TITLE_PADDING_DOWN;
		if (GNOME_ABOUT_SHOW_URLS && priv->url) {
			y += GNOME_ABOUT_TITLE_URL_PADDING_UP;
			item = gnome_canvas_item_new (root,
						      gnome_canvas_text_get_type(),
						      "x", x,
						      "y", y,
						      "anchor", GTK_ANCHOR_NORTH,
						      "fill_color_gdk", priv->title_fg,
						      "text", priv->url,
						      "font_gdk", priv->font_url,
						      NULL);
			gtk_signal_connect(GTK_OBJECT(item), "event",
					   (GtkSignalFunc)gnome_about_item_cb,
					   priv->title_fg);
			gtk_object_set_data(GTK_OBJECT(item), "url", priv->url);
			y += priv->font_url->ascent +
				priv->font_url->descent +
				GNOME_ABOUT_TITLE_URL_PADDING_DOWN;		
		}
		x = GNOME_ABOUT_PADDING_X;
		y += GNOME_ABOUT_TITLE_PADDING_Y;
	}
	
	/* Contents */
	if (priv->copyright || priv->names || priv->comments) {
		x = GNOME_ABOUT_PADDING_X + GNOME_ABOUT_CONTENT_PADDING_X;
		y += GNOME_ABOUT_CONTENT_PADDING_Y;
		x2 = priv->width - (GNOME_ABOUT_CONTENT_PADDING_X +
				    GNOME_ABOUT_PADDING_X) - 1;
		y2 = priv->height - (GNOME_ABOUT_CONTENT_PADDING_Y +
				     GNOME_ABOUT_PADDING_Y) - 1;
		gnome_canvas_item_new(root,
				      gnome_canvas_rect_get_type(),
				      "x1", x,
				      "y1", y,
				      "x2", x2,
				      "y2", y2,
				      "fill_color_gdk", priv->contents_bg,
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
				       "x", x,
				       "y", y,
				       "anchor", GTK_ANCHOR_NORTH_WEST,
				       "fill_color_gdk", priv->contents_fg,
				       "text", priv->copyright,
				       "font_gdk", priv->font_copyright,
				       NULL);

		y += priv->font_copyright->ascent +
			priv->font_copyright->descent +
			GNOME_ABOUT_COPYRIGHT_PADDING_DOWN;
	}

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
					       "fill_color_gdk", priv->contents_fg,
					       "text", _("Author:"),
					       "font_gdk", priv->font_author,
					       NULL);
		else
			gnome_canvas_item_new (root,
					       gnome_canvas_text_get_type(),
					       "x", x,
					       "y", y,
					       "anchor", GTK_ANCHOR_NORTH_WEST,
					       "fill_color_gdk", priv->contents_fg,
					       "text", _("Authors:"),
					       "font_gdk", priv->font_author,
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
					      "font_gdk", priv->font_names,
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
							    "font_gdk", priv->font_names,
							    NULL);

			tmp = g_strdup_printf("mailto:%s", (gchar*) name_email->data);
			gtk_signal_connect (GTK_OBJECT (item_email), "event",
					    (GtkSignalFunc) gnome_about_item_cb,
					    priv->contents_fg);
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
						    priv->contents_fg);
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
								      "font_gdk", priv->font_names_url,
								      NULL);
					gtk_signal_connect( GTK_OBJECT(item), 
							    "event",
							    (GtkSignalFunc) gnome_about_item_cb,
							    priv->contents_fg);
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
		gnome_about_display_comments(root, priv->font_comments,
					     priv->contents_fg, x, y,
					     w, priv->comments);
	}

	gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas), 0, 0, priv->width,
				       priv->height);
	gtk_widget_set_usize(GTK_WIDGET(canvas), priv->width, priv->height);
}

static void
gnome_about_display_comments (GnomeCanvasGroup *group,
			      GdkFont *font,
			      GdkColor *color,
			      gdouble x, gdouble y, gdouble w,
			      const gchar *comments)
{
	char *tok, *p1, *p2, *p3, c;
	char *buffer;
	gdouble ypos, width;
	int done;
	GList *par, *tmp;
	char *tokp;

	width = w - 16;
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
			while (gdk_string_measure(font, p2) < width && c != 0) {
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
						       "fill_color_gdk",
						       color,
						       "text", p2,
						       "font_gdk", font,
						       NULL);
				
				break;
			case '\0':
				done = TRUE;
				gnome_canvas_item_new (group,
						       gnome_canvas_text_get_type(),
						       "x", x,
						       "y", ypos,
						       "anchor", GTK_ANCHOR_NORTH_WEST,
						       "fill_color_gdk",
						       color,
						       "text", p2,
						       "font_gdk", font,
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
							       "fill_color_gdk",
							       color,
							       "text", p2,
							       "font_gdk", font,
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
							       "fill_color_gdk",
							       color,
							       "text", p2,
							       "font_gdk", font,
							       NULL);
				}
				break;
			}
			ypos += font->descent + font->ascent;
			p2 = p1;
		}
		tmp = tmp->next;
		ypos += BASE_LINE_SKIP;	/* Skip a bit */
	}

	/* Free list memory */
	g_list_foreach(par, (GFunc)g_free, NULL);
	g_list_free (par);
	g_free (buffer);

	return;
}


/* ----------------------------------------------------------------------
   NAME:	gnome_about_calc_size
   DESCRIPTION:	Calculates size of window
   ---------------------------------------------------------------------- */

static void
gnome_about_calc_size (GnomeAboutPrivate *priv)
{
	GList *name, *email = NULL;
	GList *urls = NULL;
	gint num_pars, i, h, w, len[5], tmpl, pw;
	gint title_h;
	const gchar *p;
	gfloat maxlen;
	
	w = GNOME_ABOUT_DEFAULT_WIDTH;
	h = GNOME_ABOUT_PADDING_Y;

	pw = 0; /* Maximum of title and content padding */

	len[0] = 0;
	len[1] = 0;
	title_h = 0;
	if (priv->title) {
		h += GNOME_ABOUT_TITLE_PADDING_Y;
		len[0] = gdk_string_measure (priv->font_title,
					     priv->title);
		title_h += (priv->font_title->descent + 
			    GNOME_ABOUT_TITLE_PADDING_DOWN) +
			(priv->font_title->ascent + 
			 GNOME_ABOUT_TITLE_PADDING_UP);

		h += GNOME_ABOUT_TITLE_PADDING_Y;
		pw = GNOME_ABOUT_TITLE_PADDING_X;
		if (GNOME_ABOUT_SHOW_URLS && priv->url) {
			title_h += priv->font_url->ascent +
				GNOME_ABOUT_TITLE_URL_PADDING_UP +
				priv->font_url->descent +
				GNOME_ABOUT_TITLE_URL_PADDING_DOWN;
			len[1] = gdk_string_measure (priv->font_url, 
						     priv->url);
		}
	}
	priv->title_height = title_h;
	h += title_h;

	if (priv->copyright || priv->names || priv->comments)
	{
		h += 2 * GNOME_ABOUT_CONTENT_PADDING_Y;
		pw = MAX(pw, GNOME_ABOUT_CONTENT_PADDING_X);
	}

	len[2] = 0;
	if (priv->copyright) {
		h += priv->font_copyright->descent +
			priv->font_copyright->ascent +
			GNOME_ABOUT_COPYRIGHT_PADDING_UP +
			GNOME_ABOUT_COPYRIGHT_PADDING_DOWN +
			GNOME_ABOUT_CONTENT_SPACING;
		len[2] = gdk_string_measure (priv->font_copyright, 
					     priv->copyright);
	}

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
	
	maxlen = (gfloat) w;
	for (i = 0; i < 5; i++)
		if (len[i] > maxlen)
			maxlen = (gfloat) len[i];
	
	w = (int) (maxlen * 1.2);
	if (w > GNOME_ABOUT_MAX_WIDTH)
		w = GNOME_ABOUT_MAX_WIDTH;

	if (priv->comments) {
		h += GNOME_ABOUT_CONTENT_SPACING;
		num_pars = 1;
		p = priv->comments;
		while (*p != '\0') {
			if (*p == '\n')
				num_pars++;
			p++;
		}
		
		i = gdk_string_measure (priv->font_comments, priv->comments);
		i /= w - 16;
		i += 1 + num_pars;;
		h += i * (priv->font_comments->descent + priv->font_comments->ascent);
	}

	priv->width = w + 2 * (GNOME_ABOUT_PADDING_X + pw);
	priv->height = h + GNOME_ABOUT_PADDING_Y;

	priv->height += 4 + priv->logo_height;
	priv->width = MAX (priv->width, (priv->logo_width + 2 *
					 (GNOME_ABOUT_PADDING_X +
					  GNOME_ABOUT_LOGO_PADDING_X)));
	
	return;
}

static void
gnome_about_load_logo(GnomeAboutPrivate *priv, 
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
				priv->logo = pixbuf;
				priv->logo_width = gdk_pixbuf_get_width (pixbuf);
				priv->logo_height = gdk_pixbuf_get_height (pixbuf);
				
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
gnome_about_fill_info (GtkWidget *widget,
		       GnomeAboutPrivate *priv,
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
	gnome_about_fill_options (widget, priv);

	/* Load logo */
	gnome_about_load_logo (priv, logo);
	
	/* fill struct */
	if (title)
		priv->title = g_strconcat (title, " ", 
					   version ? version : "", NULL);
	else
		priv->title = NULL;
	
	if (url)
		priv->url = g_strdup (url);
	else
		priv->url = NULL;
	
	if (copyright)
		priv->copyright = g_strdup (copyright);
	else
		priv->copyright = NULL;

	if (comments)
		priv->comments = g_strdup (comments);
	else
		priv->comments = NULL;

	if (authors && authors[0]) {
		gchar *name, *email;
		while (*authors) {
			name = g_strdup (*authors);
			gnome_about_parse_name (&name, &email);
			
			priv->names = g_list_append (priv->names, name);
			priv->author_emails = g_list_append (priv->author_emails, 
							     email);
			
			authors++;
			author_num++;
		}
	}

	priv->author_urls = NULL;
	if (author_urls) {
		for (i = 0; i < author_num; i++, author_urls++) {
			priv->author_urls = g_list_append (priv->author_urls,
							 g_strdup (*author_urls));
		}
	}

        /* Calculate width of window */
	gnome_about_calc_size (priv);
}

static void gnome_about_fill_options (GtkWidget *widget,
				      GnomeAboutPrivate *priv)
{
	GtkStyle *style;
    
	Bonobo_ConfigDatabase cd = gnome_program_get_config_database (gnome_program_get ());

	g_return_if_fail (priv != NULL);

	/* Read GConf options */
	priv->show_urls = bonobo_config_get_boolean (cd, "/about/show_urls", NULL);
	priv->show_logo = bonobo_config_get_boolean (cd, "/about/show_logo", NULL);
	
	/* Create fonts and get colors*/
	/* FIXME: dirty hack, but it solves i18n problem without rewriting the
	   drawing code..  */
  	GTK_WIDGET_SET_FLAGS(widget, GTK_RC_STYLE); 
	style = gtk_style_ref (widget->style);
	gtk_widget_ensure_style (widget);

	gtk_widget_set_name (widget, "Title");
	priv->font_title = gdk_font_ref (widget->style->font);
	priv->title_bg = gdk_color_copy (&widget->style->bg[GTK_STATE_NORMAL]);
	g_print ("title:RGB, %d, %d, %d\n", priv->title_bg->red,
		 priv->title_bg->green, priv->title_bg->blue);
	priv->title_fg = gdk_color_copy (&widget->style->fg[GTK_STATE_NORMAL]);
	gdk_color_alloc (gtk_widget_get_default_colormap (), priv->title_bg);
	gdk_color_alloc (gtk_widget_get_default_colormap (), priv->title_fg);

	
	gtk_widget_set_name (widget, "TitleUrl");
	gtk_widget_ensure_style (widget);
	priv->font_url = gdk_font_ref (widget->style->font);
	
	gtk_widget_set_name (widget, "Copyright");
	priv->font_copyright = gdk_font_ref (widget->style->font);
	
	gtk_widget_set_name (widget, "Author");
	priv->font_author = gdk_font_ref (widget->style->font);

	gtk_widget_set_name (widget, "Names");
	priv->font_names = gdk_font_ref (widget->style->font);

	gtk_widget_set_name (widget, "NamesUrl");
	priv->font_names_url = gdk_font_ref (widget->style->font);
	
	gtk_widget_set_name (widget, "Comments");
	priv->font_comments = gdk_font_ref (widget->style->font);
	priv->contents_bg = gdk_color_copy (&widget->style->bg[GTK_STATE_NORMAL]);
	priv->contents_fg = gdk_color_copy (&widget->style->fg[GTK_STATE_NORMAL]);
	gdk_color_alloc (gtk_widget_get_default_colormap (), priv->contents_bg);
	gdk_color_alloc (gtk_widget_get_default_colormap (), priv->contents_fg);
	
	gtk_widget_set_style (widget, style);
	gtk_style_unref (style);

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

		/* Free GUI's. */
		gdk_font_unref (self->_priv->font_title);
		gdk_font_unref (self->_priv->font_url);
		gdk_font_unref (self->_priv->font_copyright);
		gdk_font_unref (self->_priv->font_author);
		gdk_font_unref (self->_priv->font_names);
		gdk_font_unref (self->_priv->font_comments);

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
	GtkWidget *canvas;
	
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

        gtk_widget_push_colormap(gdk_rgb_get_cmap());
	canvas = gnome_canvas_new ();
        gtk_widget_pop_colormap();
	gtk_container_add (GTK_CONTAINER(frame), canvas);

	/* Fill the GnomeAboutPriv structure */
	gnome_about_fill_info (GTK_WIDGET (about),
			       about->_priv,
			       title, version, url,
			       copyright,
			       authors, author_urls,
			       comments,
			       logo);

	gnome_about_draw (GNOME_CANVAS(canvas), about->_priv);
	gtk_widget_show (canvas);

	gtk_dialog_add_button (GTK_DIALOG (about),
			       GTK_STOCK_BUTTON_OK,
			       GTK_RESPONSE_OK);
}

static gint
gnome_about_item_cb(GnomeCanvasItem *item, GdkEvent *event,
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
				GdkColor *color = (GdkColor*)data;
				
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
									   "fill_color_gdk", color,
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

