/* 
 * "About..." Widget
 *
 * AUTHOR:
 * Cesar Miquel <miquel@df.uba.ar>
 *
 * DESCRIPTION:
 *	A very specialized widget to display "About this program"-like
 * boxes.
 */

#ifndef __GNOME_ABOUT_H__
#define __GNOME_ABOUT_H__

#include <gdk/gdk.h>
#include <gtk/gtkwindow.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define GNOME_ABOUT(obj)        GTK_CHECK_CAST (obj, gnome_about_get_type (), GnomeAbout)
#define GNOME_ABOUT_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_about_get_type (), GnomeAboutClass)
#define GNOME_IS_ABOUT(obj)       GTK_CHECK_TYPE (obj, gnome_about_get_type ())

typedef struct _GnomeAbout        GnomeAbout;
typedef struct _GnomeAboutClass   GnomeAboutClass;
typedef struct _GnomeAboutButton  GnomeAboutButton;

#define GNOME_ABOUT_BUTTON_WIDTH 100
#define GNOME_ABOUT_BUTTON_HEIGHT 40

struct _GnomeAbout
{
  GtkWindow window;
};

struct _GnomeAboutClass
{
  GtkWindowClass parent_class;

};


guint      gnome_about_get_type       (void);
/* Main routine that creates the widget 
 *
 * USAGE:
 *
 *	gchar *authors[] = {"author1", "author2", ..., NULL};
 *
 *	GtkWidget *about = gnome_about_new ( _("GnoApp"), "1.2b",
 *				_("Copyrigth FSF (C) 1998"),
 *				authors,
 *				"Comment line 1\nLine 2",
 *				"/usr/local/share/pixmaps/gnoapp-logo.xpm");
 *	gtk_widget_show (about);		
 */
GtkWidget* gnome_about_new	(gchar	*title, /* Name of the application. */
				gchar	*version, /* Version. */
				gchar	*copyright, /* Copyright notice (one
							line.) */
				gchar	**authors, /* NULL terminated list of
							authors. */
				gchar	*comments, /* Other comments. */
				gchar	*logo /* A logo pixmap file. */
				);

END_GNOME_DECLS

#endif /* __GNOME_ABOUT_H__ */
