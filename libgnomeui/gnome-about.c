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
#include "gnome-stock.h"
#include <string.h>
#include <gtk/gtk.h>

/* FONTS */
#define HELVETICA_20_BFONT "-adobe-helvetica-bold-r-normal-*-20-*-*-*-*-*-*-*"
#define HELVETICA_14_BFONT "-adobe-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*"
#define HELVETICA_12_BFONT "-adobe-helvetica-bold-r-normal-*-12-*-*-*-*-*-*-*"
#define HELVETICA_12_FONT  "-adobe-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*"
#define HELVETICA_10_FONT  "-adobe-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*"

#define GNOME_ABOUT_DEFAULT_WIDTH               100
#define GNOME_ABOUT_MAX_WIDTH                   600
#define BASE_LINE_SKIP                          4

typedef struct
{
	gchar	*title;
	gchar *copyright;
	GList *names;
	gchar *comments;
	GdkPixmap *logo;
	GdkBitmap *mask;
	gint logo_w, logo_h;
	gint w, h;
	GdkColor light_green;
	GdkFont *font_title,
		*font_copyright,
		*font_author,
		*font_names,
		*font_comments;
} GnomeAboutInfo;

static void gnome_about_class_init (GnomeAboutClass *klass);
static void gnome_about_init       (GnomeAbout      *about);
static void gnome_about_repaint    (GtkWidget *w, 
				    GdkEventExpose *,
				    GnomeAboutInfo *data);

static void gnome_about_display_comments (GdkWindow *win, 
					  GdkFont *font,
					  GdkGC *gc, 
					  gint x, gint y, gint w, 
					  const gchar *comments);


/**
 * gnome_about_get_type:
 *
 * Returns the GtkType for the GnomeAbout widget.
 */
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
gnome_about_repaint (GtkWidget *widget, 
		     GdkEventExpose *event,
		     GnomeAboutInfo *gai)
{
	GdkWindow *win = widget->window;
	GdkColor *black = &widget->style->black;
	GdkColor *white = &widget->style->white;
	GdkColor *light_green = &gai->light_green;
	GList *name;
	static GdkGC *gc = NULL;
	int h, y, x;

	if (gc == NULL)
		gc = gdk_gc_new (win);

	gdk_window_clear (win);
	
	/* Draw pixmap first */
	y = 1;
	if (gai->logo)
	{
		y += 2;
		x = (gai->w - gai->logo_w) / 2;
		if (gai->mask) {
			gdk_gc_set_clip_mask (gc, gai->mask);
			gdk_gc_set_clip_origin (gc, x, y);
		}
		gdk_draw_pixmap (win, gc, gai->logo, 
				 0, 0, x, y,
				 gai->logo_w, gai->logo_h);
		if (gai->mask) {
			gdk_gc_set_clip_mask (gc, NULL);
			gdk_gc_set_clip_origin (gc, 0, 0);
		}
		y += 2 + gai->logo_h;
	}
      
	/* Draw title */
	h = gai->font_title->descent + gai->font_title->ascent + 13;
	if (gai->title)
	{
		y += 2;
		gdk_gc_set_foreground (gc, black);
		gdk_gc_set_background (gc, black);
		gdk_draw_rectangle (win, gc, TRUE, 2, y, gai->w - 5, h);
		y += h - 7 - gai->font_title->descent;
		x = (gai->w - gdk_string_measure (gai->font_title, gai->title)) / 2;
		gdk_gc_set_foreground (gc, white);
		gdk_draw_string (win, gai->font_title, gc, 
				 x, y, 
				 gai->title);
		y += 7 + gai->font_title->descent + 2;
	}

	if (gai->copyright || gai->names || gai->comments)
	{
		gdk_gc_set_foreground (gc, light_green);
		gdk_gc_set_background (gc, light_green);
		gdk_draw_rectangle (win, gc, TRUE, 
				    2, y, 
				    gai->w-5, gai->h - y - 3);
	}

	gdk_gc_set_foreground (gc, black);

	if (gai->copyright)
	{
		y += 4 + gai->font_copyright->ascent;
		gdk_draw_string (win, gai->font_copyright, gc, 5, y, gai->copyright);
		y += gai->font_copyright->descent + 4;
	}

	if (gai->names)
	{
		y += 2 + gai->font_author->ascent;
		if (g_list_length (gai->names) == 1)
			gdk_draw_string (win, gai->font_author, gc, 5, y, _("Author:") );
		else
			gdk_draw_string (win, gai->font_author, gc, 5, y, _("Authors:") );
	}
	x = 5 + gdk_string_measure (gai->font_author, _("Authors:")) + 10;

	name = g_list_first (gai->names);
	while (name)
	{
		h = gai->font_names->descent + gai->font_names->ascent;
		gdk_draw_string (win, gai->font_names, gc, x, y, (char *)name->data);
		name = name->next;
		if (name != NULL)
			y += h;
	}

	if (gai->comments)
	{
		y += 2 + 2 * gai->font_comments->ascent;
		gnome_about_display_comments (win, gai->font_comments, gc, 
					      8, y, gai->w, 
					      gai->comments);
	}

	return;


}

static void
gnome_about_display_comments (GdkWindow *win, 
			      GdkFont *font,
			      GdkGC *gc, 
			      gint x, gint y, gint w, 
			      const gchar *comments)
{
	char *tok, *p1, *p2, *p3, c;
	char *buffer;
	int  ypos, width, done;
	GList *par, *tmp;
	char *endp;
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
			while ( gdk_string_measure (font, p2) < width && c != 0)
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
				gdk_draw_string (win, font, gc, x, ypos, p2);
				break;
			case '\0':
				done = TRUE;
				gdk_draw_string (win, font, gc, x, ypos, p2);
				break;
			default:
				p3 = p1;
				while (*p1 != ' ' && p1 > p2)
					p1--;
				if (p1 == p2) 
				{
					gdk_draw_string (win, font, gc, x, ypos, p2);
					*p3 = c;
					p1 = p3;
					break;
				}
				else
				{
					*p3 = c;
					*p1 = 0; 
					p1++;
					gdk_draw_string (win, font, gc, x, ypos, p2);
				}
				break;
			}
			ypos += font->descent + font->ascent;
			p2 = p1;
		}
		tmp = tmp->next;
		ypos += BASE_LINE_SKIP; /* Skip a bit */
	}

	/* Free list memory */
	g_list_foreach(par, (GFunc)g_free, NULL);
	g_list_free (par);
	g_free (buffer);

	return;
}


/* ----------------------------------------------------------------------
   NAME:	gnome_about_calc_size
   DESCRIPTION:	Calculates size of window WITHOUT counting pixmap. 
   ---------------------------------------------------------------------- */

static void
gnome_about_calc_size (GnomeAboutInfo *gai)
{
	GList *name;
	gint num_pars, i, h, w, len[4], tmpl;
	const gchar *p;
	gfloat maxlen;

	w = GNOME_ABOUT_DEFAULT_WIDTH;
	h = 1;

	if (gai->title)
	{
		len[0] = gdk_string_measure (gai->font_title, gai->title);
		h += 4 + gai->font_title->descent + gai->font_title->ascent + 14;
	}
	else
		len[0] = 0;

	if (gai->copyright)
	{
		h += gai->font_copyright->descent + gai->font_copyright->ascent + 8;
		len[1] = gdk_string_measure (gai->font_copyright, gai->copyright);
	}
	else
		len[1] = 0;

	len[2] = 0;
	if (gai->names)
	{
		name = g_list_first (gai->names);
		while (name)
		{
			tmpl = gdk_string_measure (gai->font_names, name->data);
			if (tmpl > len[2])
				len[2] = tmpl;
			name = name->next;
		}
		tmpl = gdk_string_measure (gai->font_names, _("Authors: "));
		len[2] += tmpl + 15;
		h += g_list_length (gai->names) * 
			(gai->font_names->descent + 
			 gai->font_names->ascent + 
			 BASE_LINE_SKIP ) + 4;
	}

	maxlen = (gfloat) w;
	for (i=0; i<3; i++)
		if (len[i] > maxlen)
			maxlen = (gfloat) len[i];
    
	w = (int) (maxlen * 1.2);
	if ( w > GNOME_ABOUT_MAX_WIDTH )
		w = GNOME_ABOUT_MAX_WIDTH;

	if (gai->comments)
	{
		num_pars = 1;
		p = gai->comments;
		while (*p != '\0')
		{
			if (*p == '\n')
				num_pars++;
			p++;
		}
      
		i = gdk_string_measure (gai->font_comments, gai->comments);
		i /= w - 16;
		i += 1 + num_pars;;
		h += i * (gai->font_comments->descent + gai->font_comments->ascent);
	}

	gai->w = w+4;
	gai->h = h+3;

	return;
}  

/* ----------------------------------------------------------------------
   NAME:	gnome_fill_info
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static GnomeAboutInfo*
gnome_fill_info (GtkWidget *widget,
		 const gchar	*title,
		 const gchar	*version,
		 const gchar   *copyright,
		 const gchar   **authors,
		 const gchar   *comments,
		 const gchar   *logo)
{
	GnomeAboutInfo *gai;
	static GdkColor light_green = {0, 30208, 41216, 33792};
	GtkStyle *style;

	/* alloc mem for struct */
	gai = g_new (GnomeAboutInfo, 1);

	/* Create fonts */
	/* FIXME: dirty hack, but it solves i18n problem without rewriting the
	   drawing code..  */
	style = gtk_style_ref (widget->style);

	gtk_widget_set_name (widget, "Title");
	gai->font_title = gdk_font_ref (widget->style->font);

	gtk_widget_set_name (widget, "Copyright");
	gai->font_copyright = gdk_font_ref (widget->style->font);

	gtk_widget_set_name (widget, "Author");
	gai->font_author = gdk_font_ref (widget->style->font);

	gtk_widget_set_name (widget, "Names");
	gai->font_names = gdk_font_ref (widget->style->font);

	gtk_widget_set_name (widget, "Comments");
	gai->font_comments = gdk_font_ref (widget->style->font);

	gtk_widget_set_style (widget, style);
	gtk_style_unref (style);
  
	/* Add color */
	memcpy (&gai->light_green, &light_green, sizeof (GdkColor));
	/*   gdk_color_alloc (gdk_colormap_get_system (), &gai->light_green); */

	/* FIXME: this should use a GdkColorContext for allocation.
	 * The cc structure should reside in the GnomeAboutClass so
	 * that all about boxes share it. */
	gdk_color_alloc (gtk_widget_get_default_colormap (), &gai->light_green);

	/* fill struct */
	if(title) 
	  gai->title = g_strconcat (title, " ", version ? version : "", NULL);
	else
	  gai->title = NULL;
  
	gai->copyright = g_strdup(copyright);
  
	gai->comments = g_strdup(comments);
  
	gai->names = NULL;
	if (authors && authors[0])
	{
		while( *authors ) 
		{
			gai->names = g_list_append (gai->names,
						    g_strdup(*authors));
			authors++;
		}
	}

	/* Calculate width of window */
	gnome_about_calc_size (gai);


	/* Done */
	return gai;
}

/* ----------------------------------------------------------------------
   NAME:	gnome_destroy_about
   DESCRIPTION:	
   ---------------------------------------------------------------------- */

static void
gnome_destroy_about (GtkWidget *widget, gpointer *data)
{
	GnomeAboutInfo *gai;

	gai = (GnomeAboutInfo *)data;

	/* Free memory used for title, copyright and comments */
	g_free (gai->title);
	g_free (gai->copyright);
	g_free (gai->comments);

	/* Free GUI's. */
	gdk_font_unref (gai->font_title);
	gdk_font_unref (gai->font_copyright);
	gdk_font_unref (gai->font_author);
	gdk_font_unref (gai->font_names);
	gdk_font_unref (gai->font_comments);

	/* Free memory used for authors */
	g_list_foreach(gai->names, (GFunc)g_free, NULL);
	g_list_free (gai->names);

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
 * Description: Creates a new GNOME About dialog.  @title, @version,
 * @copyright, and @authors are displayed first, in that order.
 * @comments is typically the location for multiple lines of text, if
 * necessary.  (Separate with "\n".)  @logo is the filename of a
 * optional pixmap to be displayed in the dialog, typically a product or
 * company logo of some sort; set to %NULL if no logo file is available.
 *
 * Returns: &GtkWidget pointer to new dialog
 **/

GtkWidget* 
gnome_about_new (const gchar	*title,
		 const gchar	*version,
		 const gchar   *copyright,
		 const gchar   **authors,
		 const gchar   *comments,
		 const gchar   *logo)
{
	GnomeAbout *about;

	if(!title) title = "";
	if(!version) version = "";
	if(!copyright) copyright = "";
	g_return_val_if_fail (authors != NULL, NULL);
	if(!comments) comments = "";

	about = gtk_type_new (gnome_about_get_type ());

	gnome_about_construct(about, title, version, copyright,
			      authors, comments, logo);

	return GTK_WIDGET (about);
}

void
gnome_about_construct (GnomeAbout *about,
		       const gchar	*title,
		       const gchar	*version,
		       const gchar   *copyright,
		       const gchar   **authors,
		       const gchar   *comments,
		       const gchar   *logo)
{
	GnomeAboutInfo *ai;
	GtkWidget *frame;
	GtkWidget *drawing_area;
	GtkStyle *style;

	gint w,h;
	char *filename;


	gtk_window_set_title (GTK_WINDOW (about), _("About"));
	gtk_window_set_policy (GTK_WINDOW (about), FALSE, FALSE, TRUE);

	/* x = (gdk_screen_width ()  - w) / 2; */
	/* y = (gdk_screen_height () - h) / 2;    */
	/* gtk_widget_set_uposition ( GTK_WIDGET (about), x, y); */

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
	gtk_container_add (GTK_CONTAINER (GNOME_DIALOG(about)->vbox),
			   GTK_WIDGET(frame));
	gtk_widget_show (frame);

	drawing_area = gtk_drawing_area_new ();

	/* Make it have white bg color */
	gtk_widget_set_name (drawing_area, "DrawingArea");
	gtk_container_add (GTK_CONTAINER (frame), drawing_area);
	style = gtk_widget_get_style (drawing_area);

	ai = gnome_fill_info (drawing_area,
			      title, version, 
			      copyright, authors, 
			      comments, logo);
	gtk_signal_connect(GTK_OBJECT(about),"destroy",
			   GTK_SIGNAL_FUNC(gnome_destroy_about),ai);
  
	w = ai->w; h = ai->h;
	/* x = (gdk_screen_width ()  - w) / 2; */
	/* y = (gdk_screen_height () - h) / 2;    */
	/* gtk_widget_set_uposition ( GTK_WIDGET (about), x, y); */

	if (logo)
	{
		filename = gnome_pixmap_file (logo);
		if (filename
		    && gdk_imlib_load_file_to_pixmap (filename, &ai->logo,
						      &ai->mask))
		{
			gdk_window_get_size ((GdkWindow *) ai->logo,
					     &ai->logo_w, &ai->logo_h);
			h += 4 + ai->logo_h;
			ai->h = h;
			ai->w = MAX (w, (ai->logo_w + 6)); 
			w = ai->w;
		}
		else
			ai->logo = NULL;
		g_free(filename);
	}
	else
		ai->logo = NULL;

	gtk_widget_set_usize ( GTK_WIDGET (drawing_area), w, h);
	gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK);

	gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
			    (GtkSignalFunc) gnome_about_repaint, (gpointer) ai);

	gtk_widget_show (drawing_area);
                                                 
	gnome_dialog_append_button ( GNOME_DIALOG(about),
				     GNOME_STOCK_BUTTON_OK);

	gnome_dialog_set_close( GNOME_DIALOG(about),
				TRUE );
}
