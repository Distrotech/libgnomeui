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

#include <time.h>
#include "gnome-dialog.h"
#include "libgnome/gnome-defs.h"

BEGIN_GNOME_DECLS

#define GNOME_TYPE_SCORES            (gnome_scores_get_type ())
#define GNOME_SCORES(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_SCORES, GnomeScores))
#define GNOME_SCORES_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_SCORES, GnomeScoresClass))
#define GNOME_IS_SCORES(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_SCORES))
#define GNOME_IS_SCORES_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_SCORES))

typedef struct _GnomeScores        GnomeScores;
typedef struct _GnomeScoresClass   GnomeScoresClass;

struct _GnomeScores
{
  GnomeDialog dialog;

  GtkWidget *but_clear;
  guint	    n_scores;

  GtkWidget *logo;
  GtkWidget **label_names;
  GtkWidget **label_scores;
  GtkWidget **label_times;
};

struct _GnomeScoresClass
{
  GnomeDialogClass parent_class;
};

guint      gnome_scores_get_type (void);

/* Does all the work of displaying the best scores. 

   It calls gnome_score_get_notables to retrieve the info,
   creates the window, and show it.

   USAGE:

   pos = gnome_score_log(score, level, TRUE);
   gnome_scores_display (_("Mi game"), "migame", level, pos);
   */
void       /* Doesn't return nothing */
	gnome_scores_display (
		gchar *title,    /* Title. */
		gchar *app_name, /* Name of the application, as in 
				    gnome_score_init. */
		gchar *level, 	 /* Level of the game or NULL. */
		int pos		 /* Position in the top ten of the
				    current player, as returned by
				    gnome_score_log. */
		);

/* Creates the high-scores window. */
GtkWidget* gnome_scores_new (
		guint n_scores, 	/* Number of positions. */
		gchar **names,  	/* Names of the players. */
		gfloat *scores,		/* Scores */
		time_t *times, 		/* Time in which the scores were done */
		guint clear		/* Add a "Clear" Button? */
		);

/* Creates a label to be the logo */
void gnome_scores_set_logo_label (
		GnomeScores *gs,	/* GNOME Scores widget. */
		gchar *txt,		/* Text in the label. */
		gchar *font,		/* Font to use in the label. */
		GdkColor *color		/* Color to use in the label. */
		);

/* Creates a pixmap to be the logo */
void gnome_scores_set_logo_pixmap (
		GnomeScores *gs,	/* GNOME Scores widget. */
		gchar *logo		/* Name of the .xpm. */
		);

/* Set an arbitrary widget to be the logo. */
void gnome_scores_set_logo_widget (
		GnomeScores *gs,	/* GNOME Scores widget. */
		GtkWidget *w 		/* Widget to be used as logo. */
		);

/* Set the color of one entry. */
void gnome_scores_set_color (
		GnomeScores *gs,	/* GNOME Scores widget. */
		guint pos,		/* Entry to be changed. */
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
		gchar *txt		/* Name of the logo. */
		);

/* Set the index of the current player in top ten. */
void gnome_scores_set_current_player (
		GnomeScores *gs,	/* GNOME Scores widget. */
		gint i			/* Index of the current(from 0 to 9). */
		);
END_GNOME_DECLS

#endif /* GNOME_SCORES_H */
