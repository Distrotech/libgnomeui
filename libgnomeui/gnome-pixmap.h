
#ifndef GNOME_PIXMAP_H
#define GNOME_PIXMAP_H

#include <gtk/gtkmisc.h>

#include <libgnome/gnome-defs.h>

#include <gdk-pixbuf.h>

BEGIN_GNOME_DECLS

typedef enum {
        /* render from a pixbuf, don't create pixmap */
        GNOME_PIXMAP_USE_PIXBUF = 1 << 0,
        /* render from a pixmap, discard pixbuf */
        GNOME_PIXMAP_USE_PIXMAP = 1 << 1,
        GNOME_PIXMAP_BUILD_INSENSITIVE = 1 << 2
} GnomePixmapFlags;

typedef enum {
  GNOME_PIXMAP_KEEP_PIXBUF,
  GNOME_PIXMAP_KEEP_PIXMAP
} GnomePixmapMode;

#define GNOME_PIXMAP(obj)         GTK_CHECK_CAST (obj, gnome_pixmap_get_type (), GnomePixmap)
#define GNOME_PIXMAP_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_pixmap_get_type (), GnomePixmapClass)
#define GNOME_IS_PIXMAP(obj)      GTK_CHECK_TYPE (obj, gnome_pixmap_get_type ())


typedef struct _GnomePixmap GnomePixmap;
typedef struct _GnomePixmapClass GnomePixmapClass;

struct _GnomePixmap {
	GtkMisc misc;

        /*< private >*/
        guint      flags;

        struct {
                GdkPixbuf *pixbuf;
                GdkPixmap *pixmap;
                GdkBitmap *mask;
        } image_data[5]; /* the five states */
};

struct _GnomePixmapClass {
	GtkMiscClass parent_class;
};


guint      gnome_pixmap_get_type               (void);

GtkWidget* gnome_pixmap_new                    (GnomePixmapMode mode);

void       gnome_pixmap_set_build_insensitive  (GnomePixmap *gpixmap,
                                                gboolean build_insensitive);

void       gnome_pixmap_set_mode               (GnomePixmap *gpixmap,
                                                GnomePixmapMode mode);

void       gnome_pixmap_set_pixbufs            (GnomePixmap* gpixmap,
                                                GdkPixbuf*   pixbufs[5],
                                                GdkBitmap*   masks[5]);

void       gnome_pixmap_set_pixmaps            (GnomePixmap* gpixmap,
                                                GdkPixmap*   pixmaps[5],
                                                GdkBitmap*   masks[5]);

GtkWidget *gnome_pixmap_new_from_pixbuf          (GdkPixbuf *pixbuf);
GtkWidget *gnome_pixmap_new_from_pixbuf_at_size  (GdkPixbuf *pixbuf, gint width, gint height);

GtkWidget *gnome_pixmap_new_from_file          (const gchar *filename);
GtkWidget *gnome_pixmap_new_from_file_at_size  (const gchar *filename, gint width, gint height);

GtkWidget *gnome_pixmap_new_from_xpm_d         (gchar **xpm_data);
GtkWidget *gnome_pixmap_new_from_xpm_d_at_size (gchar **xpm_data, gint width, gint height);

END_GNOME_DECLS


#endif /* __GNOME_PIXMAP_H__ */
