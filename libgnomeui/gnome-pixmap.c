/* GNOME GUI Library
 * Copyright (C) 1997 the Free Software Foundation
 *
 * Authors: Miguel de Icaza
 *          Federico Mena
 */

#include <gtk/gtk.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/libgnome.h"
#include "gnome-messagebox.h"

struct pixmap_item {
	GdkPixmap *pixmap;
	GdkBitmap *mask;
};

static GHashTable *pixmap_hash;


static void
destroy_hash_element (void *key, void *val, void *data)
{
	struct pixmap_item *pi = val;

	gdk_pixmap_unref (pi->pixmap);
	gdk_pixmap_unref (pi->mask);
	g_free (val);
	g_free (key);
}

void
gnome_destroy_pixmap_cache (void)
{
	if (!pixmap_hash)
		return;

	g_hash_table_foreach (pixmap_hash, destroy_hash_element, 0);
	g_hash_table_destroy (pixmap_hash);
	pixmap_hash = NULL;
}

static void
check_hash_table(void)
{
	if (!pixmap_hash)
		pixmap_hash = g_hash_table_new (g_string_hash, g_string_equal);
}

static void
load_pixmap(GdkWindow *window, GdkPixmap **pixmap, GdkBitmap **mask, GdkColor *transparent, char *filename)
{
	struct pixmap_item *pit;

	check_hash_table();

	pit = g_hash_table_lookup(pixmap_hash, filename);

	if (!pit) {
		pit = g_new(struct pixmap_item, 1);
		pit->pixmap = gdk_pixmap_create_from_xpm(window, &pit->mask, transparent, filename);
		
		g_hash_table_insert(pixmap_hash, g_strdup(filename), pit);
	}
	
	*pixmap = pit->pixmap;
	*mask   = pit->mask;
}

void
gnome_create_pixmap_gdk (GdkWindow *window, GdkPixmap **pixmap, GdkBitmap **mask, GdkColor *transparent, char *file)
{
	g_assert(window != NULL);
	g_assert(pixmap != NULL);
	g_assert(mask != NULL);
	g_assert(transparent != NULL);

	if (!file || !g_file_exists(file)) {
		*pixmap = NULL;
		*mask = NULL;
		return;
	}

	load_pixmap(window, pixmap, mask, transparent, file);
}

void
gnome_create_pixmap_gtk (GtkWidget *window, GdkPixmap **pixmap, GdkBitmap **mask, GtkWidget *holder, char *file)
{
	GtkStyle *style;
	
	g_assert(window != NULL);
	g_assert(pixmap != NULL);
	g_assert(mask != NULL);
	g_assert(holder != NULL);

	if (!file || !g_file_exists(file)) {
		*pixmap = NULL;
		*mask = NULL;
		return;
	}

	if (!GTK_WIDGET_REALIZED(window))
		gtk_widget_realize(window);

	style = gtk_widget_get_style (holder);

	load_pixmap(window->window, pixmap, mask,
		    style ? &style->bg[GTK_STATE_NORMAL] : NULL, /* XXX: NULL will make it bomb */
		    file);
}

void
gnome_create_pixmap_gtk_d (GtkWidget *window, GdkPixmap **pixmap, GdkBitmap **mask, GtkWidget *holder,
			   char **data)
{
	GtkStyle *style;
	
	g_assert(window != NULL);
	g_assert(pixmap != NULL);
	g_assert(mask != NULL);
	g_assert(holder != NULL);

	if (!GTK_WIDGET_REALIZED(window))
		gtk_widget_realize(window);

	style = gtk_widget_get_style (holder);

	*pixmap = gdk_pixmap_create_from_xpm_d (window->window, mask, &style->bg[GTK_STATE_NORMAL], data);
}

GtkWidget *
gnome_create_pixmap_widget (GtkWidget *window, GtkWidget *holder, char *file)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	if (!file || !g_file_exists(file))
		return NULL;

	gnome_create_pixmap_gtk(window, &pixmap, &mask, holder, file);

	return gtk_pixmap_new(pixmap, mask);
}

GtkWidget *
gnome_create_pixmap_widget_d (GtkWidget *window, GtkWidget *holder, char **data)
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	gnome_create_pixmap_gtk_d(window, &pixmap, &mask, holder, data);

	return gtk_pixmap_new(pixmap, mask);
}

void
gnome_set_pixmap_widget (GtkPixmap *pixmap, GtkWidget *window, GtkWidget *holder, gchar *file)
{
	GdkPixmap *gpixmap;
	GdkBitmap *mask;
	
	g_assert (pixmap != NULL);

	if (!file || !g_file_exists(file))
		return;

	gnome_create_pixmap_gtk(window, &gpixmap, &mask, holder, file);

	gtk_pixmap_set(pixmap, gpixmap, mask);
}

void
gnome_set_pixmap_widget_d (GtkPixmap *pixmap, GtkWidget *window, GtkWidget *holder, gchar **data)
{
	GdkPixmap *gpixmap;
	GdkBitmap *mask;
	
	g_assert (pixmap != NULL);

	gnome_create_pixmap_gtk_d(window, &gpixmap, &mask, holder, data);

	gtk_pixmap_set(pixmap, gpixmap, mask);
}
