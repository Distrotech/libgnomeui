/* GNOME GUI Library
 * Copyright (C) 1998 Cesar Miquel <miquel@df.uba.ar>
 * Based in gnome-about, copyright (C) 1998 Horacio J. Peña
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA. 
 */

#include <config.h>
#include "gnome-about.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-url.h"
#include "gnome-stock.h"
#include "gnome-canvas.h"
#include "gnome-canvas-rect-ellipse.h"
#include "gnome-canvas-text.h"
#include "gnome-canvas-pixbuf.h"
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

#define GNOME_ABOUT_TITLE_BACKGROUND 0x000000ff
#define GNOME_ABOUT_TITLE_COLOR 0xffffffff
#define GNOME_ABOUT_TITLE_PADDING_X 1
#define GNOME_ABOUT_TITLE_PADDING_Y 1
#define GNOME_ABOUT_TITLE_PADDING_UP 6
#define GNOME_ABOUT_TITLE_PADDING_DOWN 7

#define GNOME_ABOUT_TITLE_URL_PADDING_UP 2
#define GNOME_ABOUT_TITLE_URL_PADDING_DOWN 4

#define GNOME_ABOUT_CONTENT_BACKGROUND 0x76a184ff
#define GNOME_ABOUT_CONTENT_COLOR 0x000000ff
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
#define GNOME_ABOUT_SHOW_URLS 0

typedef struct
{
	gchar *title;
	gchar *url;
	gchar *copyright;
	GList *names;
	GList *author_urls;
	gchar *comments;
	GdkPixbuf *logo;
	GdkFont *font_title,
		*font_title_url,
		*font_copyright,
		*font_author,
		*font_names,
		*font_names_url,
		*font_comments;
	gint16 logo_w, logo_h, title_w, title_h;
	gint16 w, h;
} GnomeAboutInfo;

static void gnome_about_class_init (GnomeAboutClass *klass);
static void gnome_about_init       (GnomeAbout      *about);

static void gnome_about_calc_size (GnomeAboutInfo *ai);

static void gnome_about_draw (GnomeCanvas *canvas, GnomeAboutInfo *ai);

static void gnome_about_load_logo (GnomeAboutInfo *ai, const gchar *logo);

static void gnome_about_display_comments (GnomeCanvasGroup *group,
					  GdkFont *font,
					  gdouble x, gdouble y, gdouble w,
					  const gchar *comments);

static gint gnome_about_item_cb (GnomeCanvasItem *item, GdkEvent
				 *event, gpointer data);


/**
 *g nome_about_get_type
 *
 * Returns the GtkType for the GnomeAbout widget.
 **/

guint
gnome_about_get_type ()
{
	static guint about_type = 0;

	if (!about_type)
	{
		GtkTypeInfo about_info =
		{
			"GnomeAbout",
			sizeof (GnomeAbout),
			sizeof (GnomeAboutClass),
			(GtkClassInitFunc) gnome_about_class_init,
			(GtkObjectInitFunc) gnome_about_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};

		about_type = gtk_type_unique (gnome_dialog_get_type (), &about_info);
	}

	return about_type;
}

static void
gnome_about_class_init (GnomeAboutClass *klass)
{
}

static void
gnome_about_init (GnomeAbout *about)
{
}

static void
gnome_about_draw (GnomeCanvas *canvas, GnomeAboutInfo *ai)
{
#define DRAW_TEXT(X,Y,C,F,S) \
	gnome_canvas_item_new (\
		root,\
		gnome_canvas_text_get_type(),\
		"x", (X),\
		"y", (Y),\
		"anchor", GTK_ANCHOR_NORTH_WEST,\
		"fill_color_rgba", (C),\
		"text", (S),\
		"font_gdk", (F),\
		NULL)

	GnomeCanvasGroup *root;
	GnomeCanvasItem *item;
	gdouble x, y, x2, y2, h, w;
	GList *name;

	root = gnome_canvas_root(canvas);

	x = GNOME_ABOUT_PADDING_X;
	y = GNOME_ABOUT_PADDING_Y;

	/* Logo */
	if (ai->logo)
	{
		y += GNOME_ABOUT_LOGO_PADDING_Y;
		x = (ai->w - ai->logo_w) / 2;
		item = gnome_canvas_item_new(root,
					     gnome_canvas_pixbuf_get_type
					     (), "pixbuf", ai->logo, "x",
					     x, "x_set", TRUE, "y", y,
					     "y_set", TRUE, NULL);
		if (ai->url)
		{
		    gtk_signal_connect(GTK_OBJECT(item), "event",
				       (GtkSignalFunc) gnome_about_item_cb,
				       ai);
		    gtk_object_set_data(GTK_OBJECT(item), "url",
					ai->url);
		}
		y += ai->logo_h + GNOME_ABOUT_LOGO_PADDING_Y;
	}

	/* Draw the title */
	if (ai->title)
	{
		/* Title background */
		x = GNOME_ABOUT_PADDING_X + GNOME_ABOUT_TITLE_PADDING_X;
		y += GNOME_ABOUT_TITLE_PADDING_Y;
		h = ai->title_h;
		x2 =
		    ai->w - (GNOME_ABOUT_TITLE_PADDING_X +
			     GNOME_ABOUT_PADDING_X) - 1;
		y2 = y + h - 1;
		gnome_canvas_item_new(root,
				      gnome_canvas_rect_get_type(),
				      "x1", x,
				      "y1", y,
				      "x2", x2,
				      "y2", y2,
				      "fill_color_rgba",
				      GNOME_ABOUT_TITLE_BACKGROUND, NULL);
		/* Title text */
		x += ai->w / 2;
		y += GNOME_ABOUT_TITLE_PADDING_UP;
		item = gnome_canvas_item_new(
			root,
			gnome_canvas_text_get_type(),
			"x", x,
			"y", y,
			"anchor", GTK_ANCHOR_NORTH,
			"fill_color_rgba", GNOME_ABOUT_TITLE_COLOR,
			"text", ai->title,
			"font_gdk", ai->font_title,
			NULL);
		if (ai->url)
		{
		    gtk_signal_connect(GTK_OBJECT(item), "event",
				       (GtkSignalFunc) gnome_about_item_cb, ai);
		    gtk_object_set_data(GTK_OBJECT(item), "url", ai->url);
		}
		y += ai->font_title->ascent + ai->font_title->descent +
			GNOME_ABOUT_TITLE_PADDING_DOWN;
#if GNOME_ABOUT_SHOW_URLS
		if (ai->url)
		{
			y += GNOME_ABOUT_TITLE_URL_PADDING_UP;
			item = gnome_canvas_item_new(
				root,
				gnome_canvas_text_get_type(),
				"x", x,
				"y", y,
				"anchor", GTK_ANCHOR_NORTH,
				"fill_color_rgba", GNOME_ABOUT_TITLE_COLOR,
				"text", ai->url,
				"font_gdk", ai->font_title_url,
				NULL);
			gtk_signal_connect(GTK_OBJECT(item), "event",
					   (GtkSignalFunc)gnome_about_item_cb, ai);
			gtk_object_set_data(GTK_OBJECT(item), "url", ai->url);
			y += ai->font_title_url->ascent +
				ai->font_title->descent +
				GNOME_ABOUT_TITLE_URL_PADDING_DOWN;
		}
#endif /* GNOME_ABOUT_SHOW_URLS */
		x = GNOME_ABOUT_PADDING_X;
		y += GNOME_ABOUT_TITLE_PADDING_Y;
	}

	/* Contents */
	if (ai->copyright || ai->names || ai->comments)
	{
		x = GNOME_ABOUT_PADDING_X + GNOME_ABOUT_CONTENT_PADDING_X;
		y += GNOME_ABOUT_CONTENT_PADDING_Y;
		x2 = ai->w - (GNOME_ABOUT_CONTENT_PADDING_X +
			      GNOME_ABOUT_PADDING_X) - 1;
		y2 = ai->h - (GNOME_ABOUT_CONTENT_PADDING_Y +
			      GNOME_ABOUT_PADDING_Y) - 1;
		gnome_canvas_item_new(root,
				      gnome_canvas_rect_get_type(),
				      "x1", x,
				      "y1", y,
				      "x2", x2,
				      "y2", y2,
				      "fill_color_rgba",
				      GNOME_ABOUT_CONTENT_BACKGROUND,
				      NULL);
	}

	/* Copyright message */
	if (ai->copyright)
	{
		x = GNOME_ABOUT_PADDING_X +
		    GNOME_ABOUT_CONTENT_PADDING_X +
		    GNOME_ABOUT_CONTENT_INDENT;
		y +=
		    GNOME_ABOUT_CONTENT_SPACING +
		    GNOME_ABOUT_COPYRIGHT_PADDING_UP;
		DRAW_TEXT(x, y, GNOME_ABOUT_CONTENT_COLOR,
			  ai->font_copyright, ai->copyright);
		y +=
		    ai->font_copyright->ascent +
		    ai->font_copyright->descent +
		    GNOME_ABOUT_COPYRIGHT_PADDING_DOWN;
	}

	/* Authors list */
	if (ai->names)
	{
		y += GNOME_ABOUT_CONTENT_SPACING;
		x = GNOME_ABOUT_PADDING_X +
		    GNOME_ABOUT_CONTENT_PADDING_X +
		    GNOME_ABOUT_CONTENT_INDENT;
		if (g_list_length(ai->names) == 1)
			DRAW_TEXT(x, y, GNOME_ABOUT_CONTENT_COLOR,
				  ai->font_author, _("Author:"));
		else
			DRAW_TEXT(x, y, GNOME_ABOUT_CONTENT_COLOR,
				  ai->font_author, _("Authors:"));
	}
	x += gdk_string_measure(ai->font_author, _("Authors:")) +
	    GNOME_ABOUT_AUTHORS_INDENT;

	name = g_list_first(ai->names);
	if (ai->author_urls)
	{
	    GList* name_url = g_list_first(ai->author_urls);
	    while (name)
	    {
		h = ai->font_names->descent + ai->font_names->ascent;
		item = DRAW_TEXT(x, y, GNOME_ABOUT_CONTENT_COLOR,
				 ai->font_names, ((char *) name->data));
		y += h + BASE_LINE_SKIP;
		if (name_url->data)
		{
		    gtk_signal_connect(GTK_OBJECT(item), "event",
				       (GtkSignalFunc) gnome_about_item_cb, ai);
		    gtk_object_set_data(GTK_OBJECT(item), "url", (char*)name_url->data);
#if GNOME_ABOUT_SHOW_URLS
		    x += GNOME_ABOUT_AUTHORS_URL_INDENT;
		    h = ai->font_names_url->descent + ai->font_names->ascent;
		    item = DRAW_TEXT(x, y, GNOME_ABOUT_CONTENT_COLOR,
				     ai->font_names_url, ((char *)name_url->data));
		    y += h + BASE_LINE_SKIP;
		    gtk_signal_connect(GTK_OBJECT(item), "event",
				       (GtkSignalFunc) gnome_about_item_cb, ai);
		    gtk_object_set_data(GTK_OBJECT(item), "url", (char*)name_url->data);
		    x -= GNOME_ABOUT_AUTHORS_URL_INDENT;
#endif
		}
		name = name->next;
		name_url = name_url->next;
	    }
	}
	else
	{   
	    while (name)
	    {
		h = ai->font_names->descent + ai->font_names->ascent;
		item = DRAW_TEXT(x, y, GNOME_ABOUT_CONTENT_COLOR,
				 ai->font_names, ((char *) name->data));
		y += h + BASE_LINE_SKIP;
		name = name->next;
	    }
	}

	if (ai->comments)
	{
		y += GNOME_ABOUT_CONTENT_SPACING;
		x = GNOME_ABOUT_PADDING_X +
			GNOME_ABOUT_CONTENT_PADDING_X +
			GNOME_ABOUT_CONTENT_INDENT +
			GNOME_ABOUT_COMMENTS_INDENT;
		w = ai->w - (2 * (GNOME_ABOUT_PADDING_X +
				  GNOME_ABOUT_CONTENT_PADDING_X) +
			     GNOME_ABOUT_CONTENT_INDENT +
			     GNOME_ABOUT_PADDING_Y);
		gnome_about_display_comments(root, ai->font_comments, x, y,
					     w, ai->comments);
	}


	gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas), 0, 0, ai->w,
				       ai->h);
	gtk_widget_set_usize(GTK_WIDGET(canvas), ai->w, ai->h);

#undef DRAW_TEXT
}

static void
gnome_about_display_comments (GnomeCanvasGroup *group,
			      GdkFont *font,
			      gdouble x, gdouble y, gdouble w,
			      const gchar *comments)
{
#define DRAW_CURRENT_TEXT \
	gnome_canvas_item_new (\
		group,\
		gnome_canvas_text_get_type(),\
		"x", x,\
		"y", ypos,\
		"anchor", GTK_ANCHOR_NORTH_WEST,\
		"fill_color_rgba",\
		GNOME_ABOUT_CONTENT_COLOR,\
		"text", p2,\
		"font_gdk", font,\
		NULL);

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
	while (tmp != NULL)
	{
		done = FALSE;
		p1 = p2 = tmp->data;
		while (!done)
		{
			c = *p1; *p1 = 0;
			while (gdk_string_measure(font, p2) < width && c != 0)
			{
				*p1 = c;
				p1++;
				c = *p1;
				*p1 = 0;
			}
			switch (c)
			{
			case ' ':
				p1++;
				DRAW_CURRENT_TEXT;
				break;
			case '\0':
				done = TRUE;
				DRAW_CURRENT_TEXT;
				break;
			default:
				p3 = p1;
				while (*p1 != ' ' && p1 > p2)
					p1--;
				if (p1 == p2)
				{
					DRAW_CURRENT_TEXT;
					*p3 = c;
					p1 = p3;
					break;
				}
				else
				{
					*p3 = c;
					*p1 = 0;
					p1++;
					DRAW_CURRENT_TEXT;
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
#undef DRAW_CURRENT_TEXT
}


/* ----------------------------------------------------------------------
   NAME:	gnome_about_calc_size
   DESCRIPTION:	Calculates size of window
   ---------------------------------------------------------------------- */

static void
gnome_about_calc_size (GnomeAboutInfo *ai)
{
	GList *name;
#if GNOME_ABOUT_SHOW_URLS
	GList *urls = NULL;
#endif /* GNOME_ABOUT_SHOW_URLS */
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
	if (ai->title)
	{
		h += GNOME_ABOUT_TITLE_PADDING_Y;
		len[0] = gdk_string_measure (ai->font_title,
					     ai->title);
		title_h += (ai->font_title->descent + GNOME_ABOUT_TITLE_PADDING_DOWN) +
			(ai->font_title->ascent + GNOME_ABOUT_TITLE_PADDING_UP);
		h += GNOME_ABOUT_TITLE_PADDING_Y;
		pw = GNOME_ABOUT_TITLE_PADDING_X;
#if GNOME_ABOUT_SHOW_URLS
		if (ai->url)
		{
			title_h += ai->font_title_url->ascent +
				GNOME_ABOUT_TITLE_URL_PADDING_UP +
				ai->font_title_url->descent +
				GNOME_ABOUT_TITLE_URL_PADDING_DOWN;
			len[1] = gdk_string_measure (ai->font_title_url, ai->url);
		}
#endif /* GNOME_ABOUT_SHOW_URLS */
	}
	ai->title_h = title_h;
	h += title_h;

	if (ai->copyright || ai->names || ai->comments)
	{
		h += 2 * GNOME_ABOUT_CONTENT_PADDING_Y;
		pw = MAX(pw, GNOME_ABOUT_CONTENT_PADDING_X);
	}

	len[2] = 0;
	if (ai->copyright)
	{
		h += ai->font_copyright->descent +
			ai->font_copyright->ascent +
			GNOME_ABOUT_COPYRIGHT_PADDING_UP +
			GNOME_ABOUT_COPYRIGHT_PADDING_DOWN +
			GNOME_ABOUT_CONTENT_SPACING;
		len[2] = gdk_string_measure (ai->font_copyright, ai->copyright);
	}

	len[3] = 0;
	len[4] = 0;
	if (ai->names)
	{
		name = g_list_first (ai->names);
#if GNOME_ABOUT_SHOW_URLS
		if (ai->author_urls)
			urls = g_list_first (ai->author_urls);
#endif
		while (name)
		{
			tmpl = gdk_string_measure (ai->font_names, name->data);
			if (tmpl > len[3])
				len[3] = tmpl;
			h += (ai->font_names->descent +
			      ai->font_names->ascent) + BASE_LINE_SKIP;
#if GNOME_ABOUT_SHOW_URLS
			if (urls)
			{
				if (urls->data)
				{
				    tmpl = gdk_string_measure (ai->font_names_url, urls->data);
				    if (tmpl > len[4])
					len[4] = tmpl;
				    h += (ai->font_names_url->descent +
					  ai->font_names_url->ascent) + BASE_LINE_SKIP;
				}
				urls = urls->next;
			}
#endif
			name = name->next;
		}
		tmpl = gdk_string_measure (ai->font_names, _("Authors: "));
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

	if (ai->comments)
	{
		h += GNOME_ABOUT_CONTENT_SPACING;
		num_pars = 1;
		p = ai->comments;
		while (*p != '\0')
		{
			if (*p == '\n')
				num_pars++;
			p++;
		}
		
		i = gdk_string_measure (ai->font_comments, ai->comments);
		i /= w - 16;
		i += 1 + num_pars;;
		h += i * (ai->font_comments->descent + ai->font_comments->ascent);
	}

	ai->w = w + 2 * (GNOME_ABOUT_PADDING_X + pw);
	ai->h = h + GNOME_ABOUT_PADDING_Y;

	ai->h += 4 + ai->logo_h;
	ai->w = MAX (ai->w, (ai->logo_w + 2 *
			     (GNOME_ABOUT_PADDING_X +
			      GNOME_ABOUT_LOGO_PADDING_X)));
	
	return;
}

static void
gnome_about_load_logo(GnomeAboutInfo *ai, const gchar *logo)
{
	gchar *filename;

	if (logo)
	{
		filename = gnome_pixmap_file(logo);
		if (filename != NULL)
		{
			GdkPixbuf *pixbuf;
			pixbuf = gdk_pixbuf_new_from_file(filename);
			if (pixbuf != NULL)
			{
				ai->logo = pixbuf;
				ai->logo_w = pixbuf->art_pixbuf->width;
				ai->logo_h = pixbuf->art_pixbuf->height;
				
			}
		}
	}
}

/* ----------------------------------------------------------------------
   NAME:	gnome_fill_info
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static GnomeAboutInfo *
gnome_about_fill_info (GtkWidget *widget,
		       const gchar *title,
		       const gchar *version,
		       const gchar *url,
		       const gchar *copyright,
		       const gchar **authors,
		       const gchar **author_urls,
		       const gchar *comments,
		       const gchar *logo)
{
	GnomeAboutInfo *ai;
	GtkStyle *style;
	int author_num = 0, i;

	/* alloc mem for struct */
	ai = g_new (GnomeAboutInfo, 1);

	/* Create fonts */
	/* FIXME: dirty hack, but it solves i18n problem without rewriting the
	   drawing code..  */
	GTK_WIDGET_SET_FLAGS(widget, GTK_RC_STYLE);
	style = gtk_style_ref (widget->style);

	gtk_widget_set_name (widget, "Title");
	ai->font_title = gdk_font_ref (widget->style->font);

	gtk_widget_set_name (widget, "TitleUrl");
	ai->font_title_url = gdk_font_ref (widget->style->font);
	
	gtk_widget_set_name (widget, "Copyright");
	ai->font_copyright = gdk_font_ref (widget->style->font);

	gtk_widget_set_name (widget, "Author");
	ai->font_author = gdk_font_ref (widget->style->font);

	gtk_widget_set_name (widget, "Names");
	ai->font_names = gdk_font_ref (widget->style->font);

	gtk_widget_set_name (widget, "NamesUrl");
	ai->font_names_url = gdk_font_ref (widget->style->font);
	
	gtk_widget_set_name (widget, "Comments");
	ai->font_comments = gdk_font_ref (widget->style->font);

	gtk_widget_set_style (widget, style);
	gtk_style_unref (style);

	/*Load logo */
	ai->logo = NULL;
	ai->logo_h = 0;
	ai->logo_w = 0;
	gnome_about_load_logo (ai, logo);

	/*fill struct */
	if (title)
	    ai->title = g_strconcat(title, " ", version ? version : "", NULL);
	else
	    ai->title = NULL;
	
	if (url)
	    ai->url = g_strdup (url);
	else
	    ai->url = NULL;
	
	ai->copyright = g_strdup (copyright);

	ai->comments = g_strdup (comments);

	ai->names = NULL;
	if (authors && authors[0])
	{
	    while (*authors)
	    {
		ai->names = g_list_append (ai->names,
					   g_strdup (*authors));
		authors++;
		author_num++;
	    }
	}

	ai->author_urls = NULL;
	if (author_urls)
	    for (i = 0; i < author_num; i++, author_urls++)
	    {
		ai->author_urls = g_list_append (ai->author_urls,
						 g_strdup (*author_urls));
	    }

	/* Calculate width of window */
	gnome_about_calc_size (ai);


	/* Done */
	return ai;
}

/* ----------------------------------------------------------------------
   NAME:	gnome_destroy_about
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static void
gnome_destroy_about (GtkWidget *widget, gpointer *data)
{
	GnomeAboutInfo *ai;

	ai = (GnomeAboutInfo *)data;

	/* Free memory used for title, copyright and comments */
	g_free (ai->title);
	g_free (ai->copyright);
	g_free (ai->comments);

	/* Free GUI's. */
	gdk_font_unref (ai->font_title);
	gdk_font_unref (ai->font_copyright);
	gdk_font_unref (ai->font_author);
	gdk_font_unref (ai->font_names);
	gdk_font_unref (ai->font_comments);

	/* Free memory used for authors */
	g_list_foreach (ai->names, (GFunc) g_free, NULL);
	g_list_free (ai->names);

	g_free (ai);
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
    return gnome_about_new_url (title, version, NULL, copyright,
				authors, NULL, comments, logo);
}

/**
 * gnome_about_new_url
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
gnome_about_new_url (const gchar *title,
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
	GnomeAboutInfo *ai;
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
	gtk_container_add(GTK_CONTAINER (GNOME_DIALOG(about)->vbox),
			  GTK_WIDGET(frame));
	gtk_widget_show (frame);

        gtk_widget_push_visual(gdk_rgb_get_visual());
        gtk_widget_push_colormap(gdk_rgb_get_cmap());
	canvas = gnome_canvas_new ();
        gtk_widget_pop_visual();
        gtk_widget_pop_colormap();
	gtk_container_add (GTK_CONTAINER(frame), canvas);

	/* Fill the GnomeAboutInfo */
	ai = gnome_about_fill_info (canvas,
				    title, version, url,
				    copyright,
				    authors, author_urls,
				    comments,
				    logo);
	gnome_about_calc_size (ai);
	gnome_about_draw (GNOME_CANVAS(canvas), ai);
	gtk_widget_show (canvas);

	gtk_signal_connect (GTK_OBJECT(about),"destroy",
			    GTK_SIGNAL_FUNC(gnome_destroy_about),ai);

	gnome_dialog_append_button (GNOME_DIALOG (about),
				    GNOME_STOCK_BUTTON_OK);

	gnome_dialog_set_close( GNOME_DIALOG(about),
				TRUE );
}

static gint
gnome_about_item_cb(GnomeCanvasItem *item, GdkEvent *event,
		    gpointer data)
{
	gchar *url = gtk_object_get_data(GTK_OBJECT(item), "url");
	GdkCursor *cursor;
	GdkWindow *window = GTK_WIDGET(item->canvas)->window;

	if (!url)
	    return 0;
	
	switch (event->type)
	{
	case GDK_ENTER_NOTIFY:
	    if (url)
	    {
		cursor = gnome_stock_cursor_new (GNOME_STOCK_CURSOR_POINTING_HAND); 
		gdk_window_set_cursor(window, cursor);
		gdk_cursor_destroy(cursor);
	    }
	    break;
	case GDK_LEAVE_NOTIFY:
		if (url)
		{
		    gdk_window_set_cursor(window, NULL);
		}
		break;
	case GDK_BUTTON_PRESS:
	    if (url)
	    {
		gnome_url_show(url);
	    }
	    break;
	default:
	    break;
	}
	return 0;
}
