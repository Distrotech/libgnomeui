#ifndef GNOME_ICON_TEXT_H
#define GNOME_ICON_TEXT_H

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

typedef struct {
	char *text;
	int width;
} GnomeIconTextInfoRow;

typedef struct {
	GList *rows;
	GdkFont *font;
	int width;
	int height;
	int baseline_skip;
} GnomeIconTextInfo;

GnomeIconTextInfo *gnome_icon_layout_text    (GdkFont *font, char *text,
					      char *separators, int max_width,
					      int confine);

void               gnome_icon_paint_text     (GnomeIconTextInfo *ti,
					      GdkDrawable *drawable, GdkGC *gc,
					      int x, int y,
					      GtkJustification just);

void               gnome_icon_text_info_free (GnomeIconTextInfo *ti);

END_GNOME_DECLS

#endif /* GNOME_ICON_TEXT_H */
