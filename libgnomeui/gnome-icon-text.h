#ifndef GNOME_ICON_TEXT_H
#define GNOME_ICON_TEXT_H

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

typedef struct {
	gchar *text;
	gint width;
	GdkWChar *text_wc;	/* text in wide characters */
	gint text_length;	/* number of characters */
} GnomeIconTextInfoRow;

typedef struct {
	GList *rows;
	GdkFont *font;
	gint width;
	gint height;
	gint baseline_skip;
} GnomeIconTextInfo;

GnomeIconTextInfo *gnome_icon_layout_text    (GdkFont *font, const gchar *text,
					      const gchar *separators, gint max_width,
					      gboolean confine);

void               gnome_icon_paint_text     (GnomeIconTextInfo *ti,
					      GdkDrawable *drawable, GdkGC *gc,
					      gint x, gint y,
					      GtkJustification just);

void               gnome_icon_text_info_free (GnomeIconTextInfo *ti);

END_GNOME_DECLS

#endif /* GNOME_ICON_TEXT_H */
