/* GNOME color selector in a button.
 * Written by Federico Mena <federico@nuclecu.unam.mx>
 */


#ifndef GNOME_COLOR_SELECTOR_H
#define GNOME_COLOR_SELECTOR_H

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

typedef struct _GnomeColorSelector GnomeColorSelector;

typedef void (* SetColorFunc) (GnomeColorSelector *gcs, gpointer data);

struct _GnomeColorSelector {
	double         r, g, b;
	double         tr, tg, tb;
	guchar        *buf;
	SetColorFunc   set_color_func;
	gpointer       set_color_data;
	GtkWidget     *cs_dialog;
	GtkWidget     *preview;
	GtkWidget     *button;
};


GnomeColorSelector* gnome_color_selector_new           (SetColorFunc        set_color_func,
					              	gpointer            set_color_data);
void                gnome_color_selector_destroy       (GnomeColorSelector *gcs);
GtkWidget*          gnome_color_selector_get_button    (GnomeColorSelector *gcs);
void                gnome_color_selector_set_color     (GnomeColorSelector *gcs,
						      	double              r,
						      	double              g,
						      	double              b);
void                gnome_color_selector_set_color_int (GnomeColorSelector *gcs,
						      	int                 r,
						      	int                 g,
						      	int                 b,
							int                 scale);
void                gnome_color_selector_get_color     (GnomeColorSelector *gcs,
						      	double             *r,
						      	double             *g,
						      	double             *b);
void                gnome_color_selector_get_color_int (GnomeColorSelector *gcs,
							int                *r,
							int                *g,
							int                *b,
							int                 scale);


END_GNOME_DECLS

#endif 
