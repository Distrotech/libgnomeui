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

#include "libgnome/gnome-defs.h"
#include "gnome-dialog.h"

BEGIN_GNOME_DECLS

#define GNOME_TYPE_ABOUT            (gnome_about_get_type ())
#define GNOME_ABOUT(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ABOUT, GnomeAbout))
#define GNOME_ABOUT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ABOUT, GnomeAboutClass))
#define GNOME_IS_ABOUT(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ABOUT))
#define GNOME_IS_ABOUT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ABOUT))

typedef struct _GnomeAbout        GnomeAbout;
typedef struct _GnomeAboutClass   GnomeAboutClass;

struct _GnomeAbout
{
  GnomeDialog dialog;
};

struct _GnomeAboutClass
{
  GnomeDialogClass parent_class;
};


guint      gnome_about_get_type       (void);
/* Main routine that creates the widget 
 *
 * USAGE:
 *
 *	const gchar *authors[] = {"author1", "author2", ..., NULL};
 *        (note: not having the 'const' will cause a warning during compile.)
 * 
 *	GtkWidget *about = gnome_about_new ( _("GnoApp"), "1.2b",
 *				_("Copyright FSF (C) 1998"),
 *				authors,
 *				"Comment line 1\nLine 2",
 *				"/usr/local/share/pixmaps/gnoapp-logo.xpm");
 *	gtk_widget_show (about);		
 */

GtkWidget* gnome_about_new	(const gchar	*title, /* Name of the application. */
				 const gchar	*version, /* Version. */
				 const gchar	*copyright, /* Copyright notice (one
						       line.) */
				 const gchar	**authors, /* NULL terminated list of
						      authors. */
				 const gchar	*comments, /* Other comments. */
				 const gchar	*logo /* A logo pixmap file. */
				 );

/* Only for use by bindings to languages other than C; don't use
   in applications. */
void
gnome_about_construct (GnomeAbout *about,
		       const gchar	*title,
		       const gchar	*version,
		       const gchar   *copyright,
		       const gchar   **authors,
		       const gchar   *comments,
		       const gchar   *logo);
END_GNOME_DECLS

#endif /* __GNOME_ABOUT_H__ */


