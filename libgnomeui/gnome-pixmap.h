#ifndef GNOME_PIXMAP_H
#define GNOME_PIXMAP_H

#include <gtk/gtkmisc.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgnome/gnome-defs.h"

BEGIN_GNOME_DECLS

typedef enum {
	GNOME_PIXMAP_SIMPLE, /* No alpha blending */
	GNOME_PIXMAP_COLOR   /* */
} GnomePixmapDraw;

#define GNOME_PIXMAP(obj)         GTK_CHECK_CAST (obj, gnome_pixmap_get_type (), GnomePixmap)
#define GNOME_PIXMAP_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_pixmap_get_type (), GnomePixmapClass)
#define GNOME_IS_PIXMAP(obj)      GTK_CHECK_TYPE (obj, gnome_pixmap_get_type ())


typedef struct _GnomePixmap GnomePixmap;
typedef struct _GnomePixmapClass GnomePixmapClass;

struct _GnomePixmap {
	GtkMisc misc;

        /* NOTE. In the old GnomePixmap, _lots_ of people were using GnomePixmap to
	 * load images, sucking out the pixmap field, and then not using the
	 * GnomePixmap as a widget at all. IF YOU DO THIS I WILL PERSONALLY
	 * KICK YOUR ASS. Use gdk_pixbuf_new_from_file(). Thank you.
	 * These are PRIVATE FIELDS which I will gratuitously change just to
	 * break your broken code.
	 *                          -  hp + jrb + quartic + Jesse Ventura + GW Bush 
	 */
	GnomePixmapDraw mode;

	GdkPixbuf *original_image;

	GdkPixbuf *original_scaled_image;
	GdkBitmap *original_scaled_mask;
	gint width, height;
	gint alpha_threshold;

        struct {
                GdkPixbuf *pixbuf;
                GdkBitmap *mask;
		gfloat saturation;
		gboolean pixelate;
        } image_data[5]; /* the five states */
};


struct _GnomePixmapClass {
	GtkMiscClass parent_class;
};

guint           gnome_pixmap_get_type                (void);
GtkWidget      *gnome_pixmap_new                     (void);
GtkWidget      *gnome_pixmap_new_from_file           (const gchar      *filename);
GtkWidget      *gnome_pixmap_new_from_file_at_size   (const gchar      *filename,
						      gint              width,
						      gint              height);
GtkWidget      *gnome_pixmap_new_from_xpm_d          (const gchar     **xpm_data);
GtkWidget      *gnome_pixmap_new_from_xpm_d_at_size  (const gchar     **xpm_data,
						      gint              width,
						      gint              height);
GtkWidget      *gnome_pixmap_new_from_pixbuf         (GdkPixbuf        *pixbuf);
GtkWidget      *gnome_pixmap_new_from_pixbuf_at_size (GdkPixbuf        *pixbuf,
						      gint              width,
						      gint              height);
void            gnome_pixmap_set_pixbuf_size         (GnomePixmap      *gpixmap,
						      gint              width,
						      gint              height);
void            gnome_pixmap_get_pixbuf_size         (GnomePixmap      *gpixmap,
						      gint             *width,
						      gint             *height);
void            gnome_pixmap_set_pixbuf              (GnomePixmap      *gpixmap,
						      GdkPixbuf        *pixbuf);
GdkPixbuf      *gnome_pixmap_get_pixbuf              (GnomePixmap      *gpixmap);
   
/* Sets the individual states, instead of generating them. */
void            gnome_pixmap_set_pixbuf_at_state     (GnomePixmap      *gpixmap,
						      GtkStateType      state,
						      GdkPixbuf        *pixbuf,
						      GdkBitmap        *mask);
void            gnome_pixmap_set_pixbufs_at_state    (GnomePixmap      *gpixmap,
						      GdkPixbuf        *pixbufs[5],
						      GdkBitmap        *masks[5]);
void            gnome_pixmap_set_draw_vals           (GnomePixmap      *gpixmap,
						      GtkStateType      state,
						      gfloat            saturation,
						      gboolean          pixelate);
void            gnome_pixmap_set_draw_mode           (GnomePixmap      *gpixmap,
						      GnomePixmapDraw   mode);
GnomePixmapDraw gnome_pixmap_get_draw_mode           (GnomePixmap      *gpixmap);
void            gnome_pixmap_clear                   (GnomePixmap      *gpixmap);
void            gnome_pixmap_set_alpha_threshold     (GnomePixmap      *gpixmap,
						      gint              alpha_threshold);
gint            gnome_pixmap_get_alpha_threshold     (GnomePixmap      *gpixmap);


END_GNOME_DECLS

/* PRIVATE INTERNAL FUNCTIONS DO NOT USE */
/* FIXME create a private header and move this there. */
GdkPixbuf      *gnome_pixbuf_scale                   (GdkPixbuf        *pixbuf,
						      gint              w,
						      gint              h);
GdkPixbuf      *gnome_pixbuf_build_insensitive       (GdkPixbuf        *pixbuf,
						      guchar            red_bg,
						      guchar            green_bg,
						      guchar            blue_bg);
GdkPixbuf      *gnome_pixbuf_build_grayscale         (GdkPixbuf        *pixbuf);
void            gnome_pixbuf_render                  (GdkPixbuf        *pixbuf,
						      GdkPixmap       **pixmap,
						      GdkBitmap       **mask,
						      gint              alpha_threshold);




#endif /* __GNOME_PIXMAP_H__ */
