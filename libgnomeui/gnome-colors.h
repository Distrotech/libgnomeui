/* GNOME color management, based on the GIMP's color management functions. */


#ifndef GNOME_COLORS_H
#define GNOME_COLORS_H

BEGIN_GNOME_DECLS

/* NOTE NOTE NOTE
 *
 * This module is almost straightly ripped off from the GIMP's
 * colormaps.[ch].  I still don't know what stuff is interesting to
 * GNOME apps, so I'm just including most of it here.
 */


/* This is a macro for arranging the red, green, and blue components
 * into a value acceptable to the target X server.  */

#define GNOME_COLOR_COMPOSE(r, g, b) (lookup_red[r] | lookup_green[g] | lookup_blue[b])


/* Visual and colormap used by everything */

extern GdkVisual   *gnome_visual;
extern GdkColormap *gnome_colormap;


/* Pixel values */

extern gulong gnome_black_pixel;
extern gulong gnome_gray_pixel;
extern gulong gnome_white_pixel;

extern GtkDitherInfo *gnome_red_ordered_dither;
extern GtkDitherInfo *gnome_green_ordered_dither;
extern GtkDitherInfo *gnome_blue_ordered_dither;
extern GtkDitherInfo *gnome_gray_ordered_dither;

extern guchar ***gnome_ordered_dither_matrix;

extern gulong *gnome_lookup_red;
extern gulong *gnome_lookup_green;
extern gulong *gnome_lookup_blue;

extern gulong *gnome_color_pixel_vals;
extern gulong *gnome_gray_pixel_vals;


void   gnome_colors_init        (void);
gulong gnome_colors_get_pixel   (int red, int green, int blue);
void   gnome_colors_store_color (gulong *pixel, int red, int green, int blue);


END_GNOME_DECLS

#endif
