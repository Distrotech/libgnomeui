#ifndef GNOME_GTK_UTILS_H
#define GNOME_GTK_UTILS_H 1

/*
 *  Description: Creates a GtkLabel with 'labelstr' as its text.
 *               Creates a GtkHBox
 *               Inserts the GtkLabel and then 'child' into the GtkHBox
 *	         Returns the GtkHBox
*/
GtkWidget *gnome_build_labelled_widget(char *labelstr, GtkWidget *child);

#endif
