
#ifndef GNOME_PIXMAP2_H
#define GNOME_PIXMAP2_H

#include <gtk/gtkmisc.h>

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

typedef enum {
        /* render from a pixbuf, don't create pixmap */
        GNOME_PIXMAP_USE_PIXBUF = 1 << 0,
        /* render from a pixmap, discard pixbuf */
        GNOME_PIXMAP_USE_PIXMAP = 1 << 1
} GnomePixmapFlags;

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

GtkWidget* gnome_pixmap_new                    (void);



END_GNOME_DECLS


#endif /* __GNOME_PIXMAP_H__ */
