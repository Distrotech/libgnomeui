#include <gtk/gtk.h>
#include "libgnome/gnome-defs.h"
#include "gnome-colors.h"


GdkVisual   *gnome_visual = NULL;
GdkColormap *gnome_colormap = NULL;

gulong gnome_black_pixel;
gulong gnome_gray_pixel;
gulong gnome_white_pixel;

GtkDitherInfo *gnome_red_ordered_dither;
GtkDitherInfo *gnome_green_ordered_dither;
GtkDitherInfo *gnome_blue_ordered_dither;
GtkDitherInfo *gnome_gray_ordered_dither;

guchar ***gnome_ordered_dither_matrix;

gulong *gnome_lookup_red;
gulong *gnome_lookup_green;
gulong *gnome_lookup_blue;

gulong *gnome_color_pixel_vals;
gulong *gnome_gray_pixel_vals;

static double gamma_val         = 1.0; /* FIXME: these should be read from the configuration files */
static int color_cube_shades[4] = { 6, 6, 4, 24 };
static int install_cmap         = FALSE;


static void
set_app_colors (void)
{
	gnome_black_pixel = gnome_colors_get_pixel(0, 0, 0);
	gnome_gray_pixel  = gnome_colors_get_pixel(127, 127, 127);
	gnome_white_pixel = gnome_colors_get_pixel(255, 255, 255);
}


static unsigned
gamma_correct(int intensity, double gamma)
{
	unsigned val;
	double   ind;
	double   one_over_gamma;

	if(gamma == 0.0)
	    one_over_gamma = 1.0;
	else
	    one_over_gamma = 1.0 / gamma;

	ind = (double) intensity / 255.0;
	val = (int) (255 * pow(ind, one_over_gamma) + 0.5);

	return val;
}


void
gnome_colors_init(void)
{
	GtkPreviewInfo *info;

	gtk_preview_set_gamma(gamma_val);
	gtk_preview_set_color_cube(color_cube_shades[0],
				   color_cube_shades[1],
				   color_cube_shades[2],
				   color_cube_shades[3]);
	gtk_preview_set_install_cmap(install_cmap);

	gtk_widget_set_default_visual(gtk_preview_get_visual());
	gtk_widget_set_default_colormap(gtk_preview_get_cmap());

	info = gtk_preview_get_info();

	gnome_visual           = info->visual;
	gnome_colormap         = info->cmap;
	gnome_color_pixel_vals = info->color_pixels;
	gnome_gray_pixel_vals  = info->gray_pixels;

	gnome_red_ordered_dither   = info->dither_red;
	gnome_green_ordered_dither = info->dither_green;
	gnome_blue_ordered_dither  = info->dither_blue;
	gnome_gray_ordered_dither  = info->dither_gray;

	gnome_ordered_dither_matrix = info->dither_matrix;

	gnome_lookup_red   = info->lookup_red;
	gnome_lookup_green = info->lookup_green;
	gnome_lookup_blue  = info->lookup_blue;

	set_app_colors();
}


gulong
gnome_colors_get_pixel(int red, int green, int blue)
{
	gulong pixel;

	if (gnome_visual->depth == 8)
		pixel = gnome_color_pixel_vals[(gnome_red_ordered_dither[red].s[1] +
						gnome_green_ordered_dither[green].s[1] +
						gnome_blue_ordered_dither[blue].s[1])];
	else
		gnome_colors_store_color(&pixel, red, green, blue);

	return pixel;
}


void
gnome_colors_store_color(gulong *pixel, int red, int green, int blue)
{
	GdkColor col;

	red   = gamma_correct(red, gamma_val);
	green = gamma_correct(green, gamma_val);
	blue  = gamma_correct(blue, gamma_val);

	col.red   = red | (red << 8);
	col.green = green | (green << 8);
	col.blue  = blue | (blue << 8);
	col.pixel = *pixel;

	if (gnome_visual->depth == 8)
		gdk_color_change(gnome_colormap, &col);
	else
		gdk_color_alloc(gnome_colormap, &col);

	*pixel = col.pixel;
}
