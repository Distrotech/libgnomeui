/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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

/*
 * G(NOME|TK) Scores Widget by Horacio J. Peña <horape@compendium.com.ar>
 *
 */

#include <config.h>
#include <string.h>
#include "gnome-scores.h"

#include <libgnome/gnome-util.h>
#include <libgnome/gnome-config.h>
#include <libgnome/gnome-score.h>
#include <libgnome/gnome-i18n.h>
#include "gnome-stock.h"
#include "gtk/gtk.h"

#include "time.h"

struct _GnomeScoresPrivate
{
  GtkWidget  *but_clear;
  GtkTable   *table;

  GtkWidget  *logo_container;
  GtkWidget  *logo;
  GtkWidget **label_names;
  GtkWidget **label_scores;
  GtkWidget **label_times;

  guint	      n_scores;
};

static void gnome_scores_class_init (GnomeScoresClass *klass);
static void gnome_scores_init       (GnomeScores      *scores);
static void gnome_scores_destroy    (GtkObject        *object);
static void gnome_scores_finalize   (GObject          *object);

static GnomeDialogClass *parent_class = NULL;

/**
 * gnome_scores_get_type:
 *
 * Returns the GtkType for the GnomeScores widget
 */
guint
gnome_scores_get_type (void)
{
	static guint scores_type = 0;

	if (!scores_type)
	{
		GtkTypeInfo scores_info =
		{
			"GnomeScores",
			sizeof (GnomeScores),
			sizeof (GnomeScoresClass),
			(GtkClassInitFunc) gnome_scores_class_init,
			(GtkObjectInitFunc) gnome_scores_init,
			NULL,
			NULL,
			NULL
		};

		scores_type = gtk_type_unique (gnome_dialog_get_type (), 
					       &scores_info);
	}

	return scores_type;
}

static void 
gnome_scores_init (GnomeScores *gs)
{
	GtkWidget *label;
	const gchar * buttons[] = { GNOME_STOCK_BUTTON_OK, NULL };

	gs->_priv = g_new0(GnomeScoresPrivate, 1);

	gnome_dialog_constructv(GNOME_DIALOG(gs),
				_("Top Ten"),
				buttons);

	gs->_priv->logo = NULL;
	gs->_priv->but_clear = NULL;

	gs->_priv->table    = GTK_TABLE( gtk_table_new (0, 3, FALSE) );
	gtk_table_set_col_spacings (gs->_priv->table, 30);
	gtk_table_set_row_spacings (gs->_priv->table,  5);

	label = gtk_label_new ( _("User") );
	gtk_widget_show (label);
	gtk_table_attach_defaults ( gs->_priv->table, label, 0, 1, 0, 1);
	label = gtk_label_new ( _("Score") );
	gtk_widget_show (label);
	gtk_table_attach_defaults ( gs->_priv->table, label, 1, 2, 0, 1);
	label = gtk_label_new ( _("Date") );
	gtk_widget_show (label);
	gtk_table_attach_defaults ( gs->_priv->table, label, 2, 3, 0, 1);

	gtk_widget_show(GTK_WIDGET(gs->_priv->table));

	gtk_box_pack_end (GTK_BOX(GNOME_DIALOG(gs)->vbox),
			    GTK_WIDGET(gs->_priv->table),
			    TRUE, TRUE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (gs), 5);

	gnome_dialog_set_close (GNOME_DIALOG (gs), TRUE);

	gs->_priv->logo_container = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(gs->_priv->logo_container);

	gtk_box_pack_end (GTK_BOX(GNOME_DIALOG(gs)->vbox),
			  gs->_priv->logo_container,
			  FALSE, FALSE, 0);
}

static void
gnome_scores_class_init (GnomeScoresClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	object_class->destroy = gnome_scores_destroy;
	gobject_class->finalize = gnome_scores_finalize;

	parent_class = gtk_type_class (gnome_dialog_get_type ());
}

static void
gnome_scores_destroy(GtkObject *object)
{
	GnomeScores *gs = GNOME_SCORES(object);

	/* remember, destroy can be run multiple times! */

	g_free(gs->_priv->label_names);
	gs->_priv->label_names = NULL;
	g_free(gs->_priv->label_scores);
	gs->_priv->label_scores = NULL;
	g_free(gs->_priv->label_times);
	gs->_priv->label_times = NULL;

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

static void
gnome_scores_finalize(GObject *object)
{
	GnomeScores *gs = GNOME_SCORES(object);

	g_free(gs->_priv);
	gs->_priv = NULL;

	if(G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize) (object);
}

/**
 * gnome_scores_construct:
 * @n_scores: Number of positions.
 * @names: Names of the players.
 * @scores: Scores
 * @times: Time in which the scores were done
 * @clear: Add a "Clear" Button?
 *
 * Description: useful for language bindings and subclassing, not to be used by itself,
 * see #gnome_scores_new
 *
 * Returns:
 */
void
gnome_scores_construct (  GnomeScores *gs,
			  guint n_scores, 
			  gchar **names, 
			  gfloat *scores,
			  time_t *times,
			  gboolean clear)
{
	gchar     	tmp[10];
	gchar     	tmp2[256];
	guint           i;

	gs->_priv->n_scores = n_scores;

	gs->_priv->label_names  = g_malloc(sizeof(GtkWidget*) * n_scores);
	gs->_priv->label_scores = g_malloc(sizeof(GtkWidget*) * n_scores);
	gs->_priv->label_times  = g_malloc(sizeof(GtkWidget*) * n_scores);

	for(i=0; i < n_scores; i++) {
		gs->_priv->label_names[i] = gtk_label_new ( names[i] );
		gtk_widget_show ( gs->_priv->label_names[i] );
		gtk_table_attach_defaults ( gs->_priv->table, gs->_priv->label_names[i], 0, 1, i+1, i+2);

		g_snprintf(tmp,sizeof(tmp),"%5.2f", scores[i]);
		gs->_priv->label_scores[i] = gtk_label_new ( tmp );
		gtk_widget_show ( gs->_priv->label_scores[i] );
		gtk_table_attach_defaults ( gs->_priv->table, gs->_priv->label_scores[i], 1, 2, i+1, i+2);

		/* the localized string should fit (after replacing the %a %b
		   etc) in ~18 chars.
		   %a is abbreviated weekday, %A is full weekday,
		   %b %B are abbreviated and full monthname, %Y is year,
		   %d is day of month, %m is month number, %T full hour,
		   %H hours, %M minutes, %S seconds
		*/
		if(strftime(tmp2, sizeof(tmp2), _("%a %b %d %T %Y"),
			    localtime( &(times[i]) )) == 0) {
			/* according to docs, if the string does not fit, the
			 * contents of tmp2 are undefined, thus just use
			 * ??? */
			strcpy(tmp2, "???");
		}
		tmp2[sizeof(tmp2)-1] = '\0'; /* just for sanity */
		gs->_priv->label_times[i] = gtk_label_new ( tmp2 );
		gtk_widget_show ( gs->_priv->label_times[i] );
		gtk_table_attach_defaults ( gs->_priv->table, gs->_priv->label_times[i], 2, 3, i+1, i+2);
  	}

	/* 
	if(clear) {
	  gs->_priv->but_clear = gtk_button_new_with_label ( _("Clear") );
	  gtk_widget_show (gs->_priv->but_clear);
	  gtk_table_attach_defaults ( GTK_TABLE(hor_table), gs->_priv->but_clear, 3, 4, 0, 1);
  	}
	*/
}

/**
 * gnome_scores_new:
 * @n_scores: Number of positions.
 * @names: Names of the players.
 * @scores: Scores
 * @times: Time in which the scores were done
 * @clear: Add a "Clear" Button?
 *
 * Description: Creates the high-scores window.
 *
 * Returns: A new #GnomeScores widget
 */
GtkWidget * 
gnome_scores_new (  guint n_scores, 
		    gchar **names, 
		    gfloat *scores,
		    time_t *times,
		    gboolean clear)
{
	GtkWidget *retval = gtk_type_new(gnome_scores_get_type());

	gnome_scores_construct(GNOME_SCORES(retval), n_scores, names, scores, times, clear);

	return retval;
}

/**
 * gnome_scores_set_color:
 * @gs: A #GnomeScores widget
 * @n: Entry to be changed.
 * @col: Color.
 *
 * Description: Set the color of one entry.
 *
 * Returns:
 */
void
gnome_scores_set_color(GnomeScores *gs, guint n, GdkColor *col)
{
	GtkStyle *s = gtk_style_new();

	g_return_if_fail(gs != NULL);
	g_return_if_fail(GNOME_IS_SCORES(gs));
	g_return_if_fail(col != NULL);
	g_return_if_fail(n < gs->_priv->n_scores);

	/* i believe that i should copy the default style
	   and change only the fg field, how? */

	memcpy((void *) &s->fg[0], col, sizeof(GdkColor) );
	gtk_widget_set_style(GTK_WIDGET(gs->_priv->label_names[n]), s);
	gtk_widget_set_style(GTK_WIDGET(gs->_priv->label_scores[n]), s);
	gtk_widget_set_style(GTK_WIDGET(gs->_priv->label_times[n]), s);

	gtk_style_unref(s);
}

/**
 * gnome_scores_set_def_color:
 * @gs: A #GnomeScores widget
 * @col: Color
 *
 * Description: Set the default color of the entries.
 *
 * Returns:
 */
void
gnome_scores_set_def_color(GnomeScores *gs, GdkColor *col)
{
	unsigned int i;

	g_return_if_fail(gs != NULL);
	g_return_if_fail(GNOME_IS_SCORES(gs));
	g_return_if_fail(col != NULL);

	for(i=0;i<gs->_priv->n_scores;i++) {
		gnome_scores_set_color(gs, i, col);
	}
}

/**
 * gnome_scores_set_colors:
 * @gs: A #GnomeScores widget
 * @col: Array of colors.
 *
 * Description: Set the color of all the entries.
 *
 * Returns:
 */
void
gnome_scores_set_colors(GnomeScores *gs, GdkColor *col)
{
	unsigned int i;

	g_return_if_fail(gs != NULL);
	g_return_if_fail(GNOME_IS_SCORES(gs));
	g_return_if_fail(col != NULL);

	for(i=0;i<gs->_priv->n_scores;i++) {
		gnome_scores_set_color(gs, i, col+i);
	}
}

/**
 * gnome_scores_set_current_player:
 * @gs: A #GnomeScores widget
 * @i: Index of the current(from 0 to 9).
 *
 * Description: Set the index of the current player in top ten.
 *
 * Returns:
 */
void
gnome_scores_set_current_player (GnomeScores *gs, gint i)
{
	g_return_if_fail(gs != NULL);
	g_return_if_fail(GNOME_IS_SCORES(gs));
	g_return_if_fail(i < gs->_priv->n_scores);

	gtk_widget_set_name(GTK_WIDGET(gs->_priv->label_names[i]), "CurrentPlayer");
	gtk_widget_set_name(GTK_WIDGET(gs->_priv->label_scores[i]), "CurrentPlayer");
	gtk_widget_set_name(GTK_WIDGET(gs->_priv->label_times[i]), "CurrentPlayer");
}

/**
 * gnome_scores_set_logo_label_title:
 * @gs: A #GnomeScores widget
 * @txt: Name of the logo.
 *
 * Description: Creates a label to be the logo
 *
 * Returns:
 */
void
gnome_scores_set_logo_label_title (GnomeScores *gs, gchar *txt)
{
	g_return_if_fail(gs != NULL);
	g_return_if_fail(GNOME_IS_SCORES(gs));
	g_return_if_fail(txt != NULL);

	if(gs->_priv->logo) {
		gtk_widget_destroy(gs->_priv->logo);
		gs->_priv->logo = NULL;
	}

	gs->_priv->logo = gtk_label_new(txt);
	gtk_widget_set_name(GTK_WIDGET(gs->_priv->logo), "Logo");
	gtk_container_add(GTK_CONTAINER(gs->_priv->logo_container), gs->_priv->logo);
	gtk_widget_show (gs->_priv->logo);
}


/**
 * gnome_scores_set_logo_label:
 * @gs: A #GnomeScores widget
 * @txt: Text in the label.
 * @font: Font to use in the label.
 * @col: Color to use in the label.
 *
 * Description: Creates a label to be the logo
 *
 * Returns:
 */
void
gnome_scores_set_logo_label (GnomeScores *gs, gchar *txt, gchar *font,
			     GdkColor *col)
{
	GtkStyle *s = gtk_style_new(); /* i believe that i should copy the default style
					  and change only the fg & font fields, how? */
	GdkFont *f;
	gchar *fo;

	g_return_if_fail(gs != NULL);
	g_return_if_fail(GNOME_IS_SCORES(gs));

	if(gs->_priv->logo) {
		gtk_widget_destroy(gs->_priv->logo);
		gs->_priv->logo = NULL;
	}

	if(col)
		memcpy((void *) &s->fg[0], col, sizeof(GdkColor) );

	if( font ) 
		fo = font;
	else 
		fo = _("-freefont-garamond-*-*-*-*-30-170-*-*-*-*-*-*,*-r-*");

	if(( f = gdk_fontset_load ( fo ) ))
		s->font = f;

	gs->_priv->logo = gtk_label_new(txt);
	gtk_widget_set_style(GTK_WIDGET(gs->_priv->logo), s);
	gtk_style_unref(s);
	gtk_container_add(GTK_CONTAINER(gs->_priv->logo_container), gs->_priv->logo);
	gtk_widget_show (gs->_priv->logo);
}

/**
 * gnome_scores_set_logo_widget:
 * @gs: A #GnomeScores widget
 * @w: Widget to be used as logo.
 *
 * Description:  Set an arbitrary widget to be the logo.
 *
 * Returns:
 */
void
gnome_scores_set_logo_widget (GnomeScores *gs, GtkWidget *w)
{
	g_return_if_fail(gs != NULL);
	g_return_if_fail(GNOME_IS_SCORES(gs));
	g_return_if_fail(w != NULL);
	g_return_if_fail(GTK_IS_WIDGET(w));

	if(gs->_priv->logo) {
		gtk_widget_destroy(gs->_priv->logo);
		gs->_priv->logo = NULL;
	}

	gs->_priv->logo = w;
	gtk_container_add(GTK_CONTAINER(gs->_priv->logo_container), gs->_priv->logo);

	gtk_widget_show(w);
}

/**
 * gnome_scores_set_logo_pixmap:
 * @gs: A #GnomeScores widget
 * @pix_name: filename of a pixmap
 *
 * Description:  Sets the logo on the scores dialog box to a pixmap
 *
 * Returns:
 */
void
gnome_scores_set_logo_pixmap (GnomeScores *gs, gchar *pix_name)
{
	g_return_if_fail(gs != NULL);
	g_return_if_fail(GNOME_IS_SCORES(gs));
	g_return_if_fail(pix_name != NULL);

	if(gs->_priv->logo) {
		gtk_widget_destroy(gs->_priv->logo);
		gs->_priv->logo = NULL;
	}

	gs->_priv->logo = gnome_pixmap_new_from_file (pix_name);

	gtk_container_add(GTK_CONTAINER(gs->_priv->logo_container), gs->_priv->logo);
	gtk_widget_show (gs->_priv->logo);
}

/**
 * gnome_scores_display:
 * @title: Title of the app to be displayed as logo
 * @app_name: Name of the application, as in gnome_score_init.
 * @level: Level of the game or %NULL.
 * @pos: Position in the top ten of the current player, as returned by gnome_score_log.
 *
 * Description:  Does all the work of displaying the best scores. 
 * It calls gnome_score_get_notables to retrieve the info, creates the window,
 * and show it.
 *
 * Returns:  If a dialog is displayed return it's pointer.  It can also
 * be %NULL if no dialog is displayed
 */
GtkWidget *
gnome_scores_display (gchar *title, gchar *app_name, gchar *level, int pos)
{
	GtkWidget *hs = NULL;
	gchar **names = NULL;
	gfloat *scores = NULL;
	time_t *scoretimes = NULL;
	gint top;

	top = gnome_score_get_notable(app_name, level, &names, &scores, &scoretimes);
	if (top > 0) {
		hs = gnome_scores_new(top, names, scores, scoretimes, 0);
		gnome_scores_set_logo_label_title (GNOME_SCORES(hs), title);
		if(pos)
 			gnome_scores_set_current_player(GNOME_SCORES(hs), pos-1);
		
		gtk_widget_show (hs);
		g_strfreev(names);
		g_free(scores);
		g_free(scoretimes);
	} 

	return hs;
}

/**
 * gnome_scores_display_with_pixmap:
 * @pixmap_logo: Filename of a logo pixmap to display
 * @app_name: Name of the application, as in gnome_score_init.
 * @level: Level of the game or %NULL.
 * @pos: Position in the top ten of the current player, as returned by gnome_score_log.
 *
 * Description:  Does all the work of displaying the best scores. 
 * It calls gnome_score_get_notables to retrieve the info, creates the window,
 * and show it.
 *
 * Returns:  If a dialog is displayed return it's pointer.  It can also
 * be %NULL if no dialog is displayed
 */
GtkWidget *
gnome_scores_display_with_pixmap (gchar *pixmap_logo, gchar *app_name, gchar *level, int pos)
{
	GtkWidget *hs = NULL;
	gchar **names = NULL;
	gfloat *scores = NULL;
	time_t *scoretimes = NULL;
	gint top;

	top = gnome_score_get_notable(app_name, level, &names, &scores, &scoretimes);
	if (top > 0) {
		hs = gnome_scores_new(top, names, scores, scoretimes, 0);
		gnome_scores_set_logo_pixmap (GNOME_SCORES(hs), pixmap_logo);
		if(pos)
 			gnome_scores_set_current_player(GNOME_SCORES(hs), pos-1);
		
		gtk_widget_show (hs);
		g_strfreev(names);
		g_free(scores);
		g_free(scoretimes);
	} 

	return hs;
}
