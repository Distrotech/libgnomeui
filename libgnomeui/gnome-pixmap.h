#ifndef GNOME_PIXMAP_H
#define GNOME_PIXMAP_H


#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>
#include <gdk_imlib.h>

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS


#define GNOME_TYPE_PIXMAP            (gnome_pixmap_get_type ())
#define GNOME_PIXMAP(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_PIXMAP, GnomePixmap))
#define GNOME_PIXMAP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_PIXMAP, GnomePixmapClass))
#define GNOME_IS_PIXMAP(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_PIXMAP))
#define GNOME_IS_PIXMAP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_PIXMAP))


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


guint      gnome_pixmap_get_type               (void);

GtkWidget *gnome_pixmap_new_from_file          (const char *filename);
GtkWidget *gnome_pixmap_new_from_file_at_size  (const char *filename, int width, int height);
GtkWidget *gnome_pixmap_new_from_xpm_d         (char **xpm_data);
GtkWidget *gnome_pixmap_new_from_xpm_d_at_size (char **xpm_data, int width, int height);
GtkWidget *gnome_pixmap_new_from_rgb_d         (unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height);
GtkWidget *gnome_pixmap_new_from_rgb_d_shaped  (unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height,
						GdkImlibColor *shape_color);
GtkWidget * gnome_pixmap_new_from_rgb_d_shaped_at_size (unsigned char *data,
					        unsigned char *alpha,
					        int rgb_width, int rgb_height,
					        int width, int height,
					        GdkImlibColor *shape_color);
GtkWidget *gnome_pixmap_new_from_rgb_d_at_size (unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height,
						int width, int height);
GtkWidget *gnome_pixmap_new_from_gnome_pixmap  (GnomePixmap *gpixmap);
GtkWidget *gnome_pixmap_new_from_imlib         (GdkImlibImage *im);
GtkWidget *gnome_pixmap_new_from_imlib_at_size (GdkImlibImage *im,
						int width, int height);

void       gnome_pixmap_load_file              (GnomePixmap *gpixmap, const char *filename);
void       gnome_pixmap_load_file_at_size      (GnomePixmap *gpixmap, const char *filename, int width, int height);
void       gnome_pixmap_load_xpm_d             (GnomePixmap *gpixmap, char **xpm_data);
void       gnome_pixmap_load_xpm_d_at_size     (GnomePixmap *gpixmap, char **xpm_data, int width, int height);
void       gnome_pixmap_load_rgb_d             (GnomePixmap *gpixmap, unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height);
void       gnome_pixmap_load_rgb_d_shaped      (GnomePixmap *gpixmap, unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height,
						GdkImlibColor *shape_color);
void gnome_pixmap_load_rgb_d_shaped_at_size    (GnomePixmap *gpixmap,
					        unsigned char *data,
					        unsigned char *alpha,
					        int rgb_width, int rgb_height,
					        int width, int height,
					        GdkImlibColor *shape_color);
void       gnome_pixmap_load_rgb_d_at_size     (GnomePixmap *gpixmap, unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height,
						int width, int height);
void       gnome_pixmap_load_imlib             (GnomePixmap *gpixmap,
						GdkImlibImage *im);
void       gnome_pixmap_load_imlib_at_size     (GnomePixmap *gpixmap,
				                GdkImlibImage *im,
						int width, int height);


END_GNOME_DECLS


#endif /* __GNOME_PIXMAP_H__ */
