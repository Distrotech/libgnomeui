
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

        /* NOTE. In the old GnomePixmap, _lots_ of people were using GnomePixmap to
           load images, sucking out the pixmap field, and then not using the
           GnomePixmap as a widget at all. IF YOU DO THIS I WILL PERSONALLY
           KICK YOUR ASS. Use gdk_pixbuf_new_from_file(). Thank you.
           These are PRIVATE FIELDS which I will gratuitously change just to
           break your broken code.
        */
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

void       gnome_pixmap_clear                  (GnomePixmap *gpixmap);

void       gnome_pixmap_set_pixbufs            (GnomePixmap* gpixmap,
                                                GdkPixbuf*   pixbufs[5],
                                                GdkBitmap*   masks[5]);

void       gnome_pixmap_set_pixmaps            (GnomePixmap* gpixmap,
                                                GdkPixmap*   pixmaps[5],
                                                GdkBitmap*   masks[5]);

void       gnome_pixmap_set_pixbuf               (GnomePixmap *gpixmap, GtkStateType state, GdkPixbuf *pixbuf);
void       gnome_pixmap_set_pixbuf_at_size       (GnomePixmap *gpixmap, GtkStateType state, GdkPixbuf *pixbuf, gint width, gint height);

GtkWidget *gnome_pixmap_new_from_pixbuf          (GdkPixbuf *pixbuf);
GtkWidget *gnome_pixmap_new_from_pixbuf_at_size  (GdkPixbuf *pixbuf, gint width, gint height);

/* note that the new_from_file functions give you no way to detect errors;
   if the file isn't found/loaded, you get an empty widget.
   to detect errors just do:
   GdkPixbuf *pixbuf;
   GtkWidget *gpixmap;
   pixbuf = gdk_pixbuf_new_from_file(filename);
   if (pixbuf != NULL) {
         gpixmap = gnome_pixmap_new_from_pixbuf(pixbuf);
         gdk_pixbuf_unref(pixbuf);
   }
*/
GtkWidget *gnome_pixmap_new_from_file          (const gchar *filename);
GtkWidget *gnome_pixmap_new_from_file_at_size  (const gchar *filename, gint width, gint height);

GtkWidget *gnome_pixmap_new_from_xpm_d         (const gchar **xpm_data);
GtkWidget *gnome_pixmap_new_from_xpm_d_at_size (const gchar **xpm_data, gint width, gint height);

END_GNOME_DECLS

/* PRIVATE INTERNAL FUNCTIONS DO NOT USE */
/* FIXME create a private header and move this there. */
GdkPixbuf *gnome_pixbuf_scale(GdkPixbuf *pixbuf, gint w, gint h);
GdkPixbuf *gnome_pixbuf_build_insensitive(GdkPixbuf *pixbuf,
                                          guchar red_bg, guchar green_bg, guchar blue_bg);
GdkPixbuf *gnome_pixbuf_build_grayscale(GdkPixbuf *pixbuf);
void       gnome_pixbuf_render(GdkPixbuf *pixbuf,
                               GdkPixmap **pixmap,
                               GdkBitmap **mask);

#endif /* __GNOME_PIXMAP_H__ */
