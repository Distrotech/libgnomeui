#ifndef GNOME_PIXMAP_H
#define GNOME_PIXMAP_H


#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>


BEGIN_GNOME_DECLS


#define GNOME_PIXMAP(obj)         GTK_CHECK_CAST (obj, gnome_pixmap_get_type (), GnomePixmap)
#define GNOME_PIXMAP_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_pixmap_get_type (), GnomePixmapClass)
#define GNOME_IS_PIXMAP(obj)      GTK_CHECK_TYPE (obj, gnome_pixmap_get_type ())


typedef struct _GnomePixmap GnomePixmap;
typedef struct _GnomePixmapClass GnomePixmapClass;

struct _GnomePixmap {
	GtkWidget widget;

	GdkPixmap *pixmap;
	GdkBitmap *mask;
};

struct _GnomePixmapClass {
	GtkWidgetClass parent_class;
};


guint      gnome_pixmap_get_type      (void);

GtkWidget *gnome_pixmap_new_from_file (char *filename);

void       gnome_pixmap_load_file     (GnomePixmap *gpixmap, char *filename);


/* FIXME:  These are the old gnome-pixmap API functions.  They should
 * be flushed out in favor of the GnomePixmap widget.
 */

void gnome_destroy_pixmap_cache (void);

void gnome_create_pixmap_gdk              (GdkWindow *window,
					   GdkPixmap **pixmap,
					   GdkBitmap **mask,
					   GdkColor  *transparent,
					   char *file);
void gnome_create_pixmap_gtk              (GtkWidget *window,
					   GdkPixmap **pixmap,
					   GdkBitmap **mask,
					   GtkWidget *holder,
					   char *file);
void gnome_create_pixmap_gtk_d            (GtkWidget *window,
					   GdkPixmap **pixmap,
					   GdkBitmap **mask,
					   GtkWidget *holder,
					   char **data);
GtkWidget *gnome_create_pixmap_widget     (GtkWidget *window,
					   GtkWidget *holder,
					   char *file);
GtkWidget *gnome_create_pixmap_widget_d   (GtkWidget *window,
					   GtkWidget *holder,
					   char **data);
void gnome_set_pixmap_widget              (GtkPixmap *pixmap,	
					   GtkWidget *window,
					   GtkWidget *holder,
					   char *file);
void gnome_set_pixmap_widget_d            (GtkPixmap *pixmap,	
					   GtkWidget *window,
					   GtkWidget *holder,
					   char **data);


END_GNOME_DECLS


#endif /* __GNOME_PIXMAP_H__ */
