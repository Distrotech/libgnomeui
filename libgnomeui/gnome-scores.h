/* 
 * G(NOME|TK) Scores Widget by Horacio J. Peña <horape@compendium.com.ar>
 * 
 * Free software (under the terms of the GNU Library General Public License)
 */


#ifndef GNOME_SCORES_H
#define GNOME_SCORES_H
#include <gtk/gtkwindow.h>
#include <libgnome/gnome-defs.h>
#include <time.h>

BEGIN_GNOME_DECLS

#define GNOME_SCORES(obj)          GTK_CHECK_CAST (obj, gnome_scores_get_type (), GnomeScores)
#define GNOME_SCORES_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_scores_get_type (), GnomeScoresClass)
#define GNOME_IS_SCORES(obj)       GTK_CHECK_TYPE (obj, gnome_scores_get_type ())

typedef struct _GnomeScores        GnomeScores;
typedef struct _GnomeScoresClass   GnomeScoresClass;

struct _GnomeScores
{
  GtkWindow window;

  GtkWidget *but_clear;
  guint	    n_scores;

  GtkWidget *logo;
  GtkWidget *vbox;
  GtkWidget **label_names;
  GtkWidget **label_scores;
  GtkWidget **label_times;
};

struct _GnomeScoresClass
{
  GtkWindowClass parent_class;
};

guint      gnome_scores_get_type (void);
GtkWidget* gnome_scores_new (guint n_scores, gchar **names, gfloat *scores,
				time_t *times, guint clear);

void	   gnome_scores_set_logo_label  (GnomeScores *, gchar *, gchar *, GdkColor *);
void	   gnome_scores_set_logo_pixmap (GnomeScores *, gchar *);
void	   gnome_scores_set_logo_widget (GnomeScores *, GtkWidget *);

void	   gnome_scores_set_color       (GnomeScores *, guint, GdkColor*);
void	   gnome_scores_set_def_color   (GnomeScores *, GdkColor*); 

		/* Warning: in gnome_scores_set_colors GdkColor* is an
		   array, not a pointer as in [set|def]_color */
void	   gnome_scores_set_colors      (GnomeScores *, GdkColor*); 

END_GNOME_DECLS

#endif /* GNOME_SCORES_H */
