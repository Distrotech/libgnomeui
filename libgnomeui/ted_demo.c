/*
 * This program ilustrates how the Ted widget is used for GUI design
 *
 */
#include <config.h>
#include <gnome.h>
#include "gtk-ted.h"

static void
clicked ()
{
	printf ("Button clicked\n");
}

static void
xrandom_widgets (GtkTed *t)
{
	GtkWidget *l;
	int i;

	for (i = 0; i < 4; i++){
		char buf [40];

		g_snprintf (buf, sizeof(buf), "Button-%d", i);
		l = gtk_button_new_with_label (buf);
		gtk_widget_show (l);
		gtk_ted_add (t, l, buf);
		gtk_signal_connect (GTK_OBJECT (l), "clicked", GTK_SIGNAL_FUNC (clicked), NULL);
	}

	l = gtk_label_new ("This is a windowless widget");
	gtk_widget_show (l);
	gtk_ted_add (t, l, "Label-0");
	l = gtk_entry_new ();
	gtk_widget_show (l);
	gtk_ted_add (t, l, "Entry-0");
}

int
main (int argc, char *argv[])
{
	GtkWidget *t, *w;
	
	gnome_init ("ted_demo", VERSION, argc, argv);

	gtk_ted_set_app_name ("MyApp");
	
	w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	t = gtk_ted_new ("DialogName");
	gtk_widget_show (t);
	gtk_container_add (GTK_CONTAINER (w), t);
	
	xrandom_widgets (GTK_TED(t));
	gtk_ted_prepare (GTK_TED(t));
	gtk_widget_show (w);
	
	gtk_main ();
	return 0;
}
