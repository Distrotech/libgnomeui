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
 * "High Scores" Widget 
 *
 * AUTHOR: 
 * Horacio J. Peña <horape@compendium.com.ar>
 *
 * This is free software (under the terms of the GNU LGPL)
 *
 * USAGE:
 * Use the gnome_scores_display. The other functions are going to be
 * discontinued... (ok, i should add pixmap support to *_display 
 * before)
 *
 * DESCRIPTION:
 * A specialized widget to display "High Scores" for games. It's 
 * very integrated with the gnome-score stuff so you only need to
 * call one function to do all the work...
 *
 */

#ifndef GNOME_SCORES_H
#define GNOME_SCORES_H

#ifndef GNOME_DISABLE_DEPRECATED

#include <time.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GNOME_TYPE_SCORES            (gnome_scores_get_type ())
#define GNOME_SCORES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_SCORES, GnomeScores))
#define GNOME_SCORES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_SCORES, GnomeScoresClass))
#define GNOME_IS_SCORES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_SCORES))
#define GNOME_IS_SCORES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_SCORES))
#define GNOME_SCORES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_SCORES, GnomeScoresClass))

typedef struct _GnomeScores        GnomeScores;
typedef struct _GnomeScoresPrivate GnomeScoresPrivate;
typedef struct _GnomeScoresClass   GnomeScoresClass;

struct _GnomeScores
{
  GtkDialog dialog;

  /*< private >*/
  GnomeScoresPrivate *_priv;
};

struct _GnomeScoresClass
{
  GtkDialogClass parent_class;

  /* Padding for possible expansion */
  gpointer padding1;
  gpointer padding2;
};

GType      gnome_scores_get_type (void) G_GNUC_CONST;

/* Does all the work of displaying the best scores. 

   It calls gnome_score_get_notables to retrieve the info,
   creates the window, and show it.

   USAGE:

   pos = gnome_score_log(score, level, TRUE);
   gnome_scores_display (_("Mi game"), "migame", level, pos);
   */
GtkWidget *       /* returns the pointer to the displayed window */
	gnome_scores_display (
		const gchar *title,    /* Title. */
		const gchar *app_name, /* Name of the application, as in 
				    gnome_score_init. */
		const gchar *level, 	 /* Level of the game or NULL. */
		int pos		 /* Position in the top ten of the
				    current player, as returned by
				    gnome_score_log. */
		);
/* Same as above, but with a pixmap logo instead of just text */
GtkWidget *
	gnome_scores_display_with_pixmap (const gchar *pixmap_logo,
					  const gchar *app_name,
					  const gchar *level,
					  int pos);

/* Creates the high-scores window. */
GtkWidget* gnome_scores_new (
		guint n_scores, 	/* Number of positions. */
		gchar **names,  	/* Names of the players. */
		gfloat *scores,		/* Scores */
		time_t *times, 		/* Time in which the scores were done */
		gboolean clear		/* Add a "Clear" Button? */
		);

/* Constructor for bindings / subclassing */
void gnome_scores_construct (GnomeScores *gs,
			     guint n_scores, 
			     gchar **names, 
			     gfloat *scores,
			     time_t *times,
			     gboolean clear);

/* Creates a label to be the logo */
void gnome_scores_set_logo_label (
		GnomeScores *gs,	/* GNOME Scores widget. */
		const gchar *txt,	/* Text in the label. */
		const gchar *font,	/* Font to use in the label. */
		GdkColor *col		/* Color to use in the label. */
		);

/* Creates a pixmap to be the logo */
void gnome_scores_set_logo_pixmap (
		GnomeScores *gs,	/* GNOME Scores widget. */
		const gchar *pix_name	/* Name of the .xpm. */
		);

/* Set an arbitrary widget to be the logo. */
void gnome_scores_set_logo_widget (
		GnomeScores *gs,	/* GNOME Scores widget. */
		GtkWidget *w 		/* Widget to be used as logo. */
		);

/* Set the color of one entry. */
void gnome_scores_set_color (
		GnomeScores *gs,	/* GNOME Scores widget. */
		guint n,		/* Entry to be changed. */
		GdkColor *col		/* Color. */
		);

/* Set the default color of the entries. */
void gnome_scores_set_def_color (
		GnomeScores *gs,	/* GNOME Scores widget. */
		GdkColor *col		/* Color. */
		); 

/* Set the color of all the entries. */
void gnome_scores_set_colors (
		GnomeScores *gs,	
		GdkColor *col		/* Array of colors. */
		);


/* Creates a label to be the logo */
void gnome_scores_set_logo_label_title (
		GnomeScores *gs,	/* GNOME Scores widget. */
		const gchar *txt	/* Name of the logo. */
		);

/* Set the index of the current player in top ten. */
void gnome_scores_set_current_player (
		GnomeScores *gs,	/* GNOME Scores widget. */
		gint i			/* Index of the current(from 0 to 9). */
		);
G_END_DECLS


#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* GNOME_SCORES_H */

