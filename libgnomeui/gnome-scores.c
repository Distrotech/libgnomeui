/*
 * G(NOME|TK) Scores Widget by Horacio J. Peña <horape@compendium.com.ar>
 *
 * Free software (under the terms of the GNU Library General Public License)
 */

#include <string.h>
#include "gnome-scores.h"
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-i18n.h"
#include "libgnome/gnome-score.h"
#include "libgnome/gnome-string.h"

#include "gtk/gtk.h"

#include "time.h"

static void gnome_scores_class_init (GnomeScoresClass *klass);
static void gnome_scores_init       (GnomeScores      *scores);

guint
gnome_scores_get_type ()
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
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};

		scores_type = gtk_type_unique (gtk_window_get_type (), &scores_info);
	}

	return scores_type;
}

static void 
gnome_scores_init (GnomeScores *gs) {}

static void
gnome_scores_class_init (GnomeScoresClass *class) {}

GtkWidget * 
gnome_scores_new (  guint n_scores, 
		    gchar **names, 
		    gfloat *scores ,
		    time_t *times,
		    guint clear)
{
	GtkWidget *retval = gtk_type_new(gnome_scores_get_type());
	GnomeScores  *gs = GNOME_SCORES(retval); 
	GtkTable	*table;
	GtkWidget	*hor_table;
	GtkWidget	*label;
	GtkWidget	*hSep;
	GtkWidget	*button1;
	gchar     	tmp[10];
	gchar     	*tmp2;
	int i;

	gs->logo = 0;
	gs->but_clear = 0;
	gs->n_scores = n_scores;

	table    = GTK_TABLE( gtk_table_new (n_scores+1, 3, FALSE) );
	gtk_table_set_col_spacings (table, 30);
	gtk_table_set_row_spacings (table,  5);

	label = gtk_label_new ( _("User") );
	gtk_widget_show (label);
	gtk_table_attach_defaults ( table, label, 0, 1, 0, 1);
	label = gtk_label_new ( _("Score") );
	gtk_widget_show (label);
	gtk_table_attach_defaults ( table, label, 1, 2, 0, 1);
	label = gtk_label_new ( _("Date") );
	gtk_widget_show (label);
	gtk_table_attach_defaults ( table, label, 2, 3, 0, 1);

	gs->label_names  = g_malloc(sizeof(GtkWidget*)*n_scores);
	gs->label_scores = g_malloc(sizeof(GtkWidget*)*n_scores);
	gs->label_times  = g_malloc(sizeof(GtkWidget*)*n_scores);

	for(i=0; i < n_scores; i++) {
		gs->label_names[i] = gtk_label_new ( names[i] );
		gtk_widget_show ( gs->label_names[i] );
		gtk_table_attach_defaults ( table, gs->label_names[i], 0, 1, i+1, i+2);

		g_snprintf(tmp,10,"%5.2f", scores[i]);
		gs->label_scores[i] = gtk_label_new ( tmp );
		gtk_widget_show ( gs->label_scores[i] );
		gtk_table_attach_defaults ( table, gs->label_scores[i], 1, 2, i+1, i+2);

		tmp2 = ctime( &(times[i]) );
		tmp2[strlen(tmp2)-1]=0;
		gs->label_times[i] = gtk_label_new ( tmp2 );
		gtk_widget_show ( gs->label_times[i] );
		gtk_table_attach_defaults ( table, gs->label_times[i], 2, 3, i+1, i+2);
  	}
	gtk_widget_show (GTK_WIDGET(table));

	button1 = gtk_button_new_with_label ( _("OK") );
	gtk_signal_connect_object (GTK_OBJECT (button1), "clicked",
				   GTK_SIGNAL_FUNC (gtk_widget_destroy),
				   GTK_OBJECT (gs));
	gtk_widget_show (button1);
	hor_table     = gtk_table_new (1, clear?5:3, TRUE); 
	gtk_table_attach_defaults ( GTK_TABLE(hor_table), button1, 1, 2, 0, 1);
	gtk_widget_show (hor_table);

	if(clear) {
		gs->but_clear = gtk_button_new_with_label ( _("Clear") );
		gtk_widget_show (gs->but_clear);
		gtk_table_attach_defaults ( GTK_TABLE(hor_table), gs->but_clear, 3, 4, 0, 1);
  	}

	gs->vbox = gtk_vbox_new (FALSE, 5);
	gtk_widget_show (gs->vbox);
	gtk_box_pack_end (GTK_BOX(gs->vbox), hor_table, TRUE, TRUE, 0);
	hSep = gtk_hseparator_new ();
	gtk_widget_show (hSep);
	gtk_box_pack_end (GTK_BOX(gs->vbox), hSep, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX(gs->vbox), GTK_WIDGET(table), TRUE, TRUE, 0);
	hSep = gtk_hseparator_new ();
	gtk_widget_show (hSep);
	gtk_box_pack_end (GTK_BOX(gs->vbox), hSep, TRUE, TRUE, 0);

	gtk_container_add( GTK_CONTAINER(gs), gs->vbox);
	gtk_window_set_title (GTK_WINDOW (gs), _("Top Ten") );
	gtk_container_border_width (GTK_CONTAINER (gs), 5);

	return retval;
}

void
gnome_scores_set_color(GnomeScores *gs, guint n, GdkColor *col)
{
	GtkStyle *s = gtk_style_new(); /* i believe that i should copy the default style
					  and change only the fg field, how? */

	memcpy((void *) &s->fg[0], col, sizeof(GdkColor) );
	gtk_widget_set_style(GTK_WIDGET(gs->label_names[n]), s);
	gtk_widget_set_style(GTK_WIDGET(gs->label_scores[n]), s);
	gtk_widget_set_style(GTK_WIDGET(gs->label_times[n]), s);
/* Can i free s?, how? */
}

void
gnome_scores_set_def_color(GnomeScores *gs, GdkColor *col)
{
	int i;

	for(i=0;i<gs->n_scores;i++) {
		gnome_scores_set_color(gs, i, col);
	}
}

void
gnome_scores_set_colors(GnomeScores *gs, GdkColor *col)
{
	int i;

	for(i=0;i<gs->n_scores;i++) {
		gnome_scores_set_color(gs, i, col+i);
	}
}

void gnome_scores_set_logo_label (GnomeScores *gs, gchar *txt, gchar *font,
				  GdkColor *col)
{
	GtkStyle *s = gtk_style_new(); /* i believe that i should copy the default style
					  and change only the fg & font fields, how? */
	GdkFont *f;
	gchar *fo;

	if(gs->logo) {
		g_print("Warning: gnome_scores_set_logo_* can be called only once\n");
		return;
	}

	if(col)
		memcpy((void *) &s->fg[0], col, sizeof(GdkColor) );

	if( font ) 
		fo = font;
	else 
		fo = "-freefont-garamond-*-*-*-*-30-170-*-*-*-*-iso8859-1";

	if( f = gdk_fontset_load ( fo ) )
		s->font = f;

	gs->logo = gtk_label_new(txt);
	gtk_widget_set_style(GTK_WIDGET(gs->logo), s);
	gtk_box_pack_end (GTK_BOX(gs->vbox), gs->logo, TRUE, TRUE, 0);
	gtk_widget_show (gs->logo);
}

void gnome_scores_set_logo_widget (GnomeScores *gs, GtkWidget *w)
{

	if(gs->logo) {
		g_print("Warning: gnome_scores_set_logo_* can be called only once\n");
		return;
	}

	gs->logo = w;
	gtk_box_pack_end (GTK_BOX(gs->vbox), gs->logo, TRUE, TRUE, 0);
}

void gnome_scores_set_logo_pixmap (GnomeScores *gs, gchar *pix_name)
{
	GdkPixmap *pixmap;
	GtkStyle *style;
	GdkBitmap *mask;

	if(gs->logo) {
		g_print("Warning: gnome_scores_set_logo_* can be called only once\n");
		return;
	}

	style = gtk_widget_get_style( GTK_WIDGET(gs) );

	pixmap = gdk_pixmap_create_from_xpm ( GTK_WIDGET(gs)->window, &mask,
					      &style->bg[GTK_STATE_NORMAL],
					      pix_name);
	gs->logo = gtk_pixmap_new (pixmap, mask);

	gtk_box_pack_end (GTK_BOX(gs->vbox), gs->logo, TRUE, TRUE, 0);
	gtk_widget_show (gs->logo);
}

void
gnome_scores_display (gchar *title, gchar *app_name, gchar *level, int pos)
{
	GtkWidget *hs;
	GdkColor ctitle = {0, 0, 0, 65535};
	GdkColor col = {0, 65535, 0, 0};
	gchar **names = NULL;
	gfloat *scores = NULL;
	time_t *scoretimes = NULL;
	gint top;

	top = gnome_score_get_notable(app_name, level, &names, &scores, &scoretimes);
	if (top > 0){
		hs = gnome_scores_new(top, names, scores, scoretimes, 0);
		gnome_scores_set_logo_label (GNOME_SCORES(hs), title, 0, 
					     &ctitle);
		if(pos)
			gnome_scores_set_color(GNOME_SCORES(hs), pos-1, &col);

		gtk_widget_show (hs);
		gnome_string_array_free(names);
		g_free(scores);
		g_free(scoretimes);
	} 
}
