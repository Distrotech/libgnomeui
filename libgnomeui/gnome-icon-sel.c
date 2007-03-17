/* gnome-icon-sel.c:
 * Copyright (C) 1998 Free Software Foundation
 * All rights reserved.
 *
 * Written by: Havoc Pennington, based on John Ellis's code.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#include <config.h>
#include <libgnome/gnome-macros.h>

#include <unistd.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

/* Must be before all other gnome includes!! */
#include <glib/gi18n-lib.h>

#include <libgnome/gnome-util.h>
#include "gnome-icon-list.h"
#include "gnome-uidefs.h"

#include "gnome-icon-sel.h"
#include "libgnomeui-access.h"

#include <libgnomevfs/gnome-vfs-ops.h>

#define ICON_SIZE 48

struct _GnomeIconSelectionPrivate {
	GtkWidget * box;

	GtkWidget * gil;

	GList * file_list;

	/* For loading the icons */
	GMainLoop * load_loop;
	guint load_idle;
	int load_i;
	int load_file_count;
	GtkWidget * load_progressbar;
};

static void gnome_icon_selection_destroy    (GtkObject               *object);
static void gnome_icon_selection_finalize   (GObject                 *object);

static int sort_file_list		    (gconstpointer            a,
					     gconstpointer            b);

GNOME_CLASS_BOILERPLATE (GnomeIconSelection, gnome_icon_selection,
			 GtkVBox, GTK_TYPE_VBOX)

static void
gnome_icon_selection_class_init (GnomeIconSelectionClass *klass)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;

  gobject_class = (GObjectClass*) klass;
  object_class = (GtkObjectClass*) klass;

  gobject_class->finalize = gnome_icon_selection_finalize;
  object_class->destroy = gnome_icon_selection_destroy;
}

static void
gnome_icon_selection_instance_init (GnomeIconSelection *gis)
{
	GtkWidget *box;
	GtkWidget *frame;
	GtkWidget *sb;

	gis->_priv = g_new0(GnomeIconSelectionPrivate, 1);

	gis->_priv->box = gtk_vbox_new(FALSE, 0);

	gtk_container_add(GTK_CONTAINER(gis), gis->_priv->box);

	gtk_widget_show(gis->_priv->box);

	box = gtk_hbox_new(FALSE, 5);

	gtk_box_pack_end(GTK_BOX(gis->_priv->box), box, TRUE, TRUE, 0);
	gtk_widget_show(box);
	
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
	gtk_widget_show (frame);

	sb = gtk_vscrollbar_new(NULL);
	gtk_box_pack_end(GTK_BOX(box),sb,FALSE,FALSE,0);
	gtk_widget_show(sb);

	gis->_priv->gil = gnome_icon_list_new(ICON_SIZE+30,
					      gtk_range_get_adjustment (GTK_RANGE (sb)),
					      FALSE);
	gtk_widget_set_size_request (gis->_priv->gil, 350, 300);
	gnome_icon_list_set_selection_mode(GNOME_ICON_LIST(gis->_priv->gil),
					    GTK_SELECTION_SINGLE);

	gtk_container_add (GTK_CONTAINER (frame), gis->_priv->gil);
	gtk_widget_show(gis->_priv->gil);

	g_signal_connect (gis->_priv->gil, "destroy",
			  G_CALLBACK (gtk_widget_destroyed),
			  &(gis->_priv->gil));

	gis->_priv->file_list = NULL;
}


/**
 * gnome_icon_selection_new:
 *
 * Description: Creates a new icon selection widget, it uses GnomeIconList
 * for the listing of icons
 *
 * Returns: Returns the new object
 **/
GtkWidget *
gnome_icon_selection_new (void)
{
  GnomeIconSelection * gis;
  
  gis = g_object_new (GNOME_TYPE_ICON_SELECTION, NULL);

  return GTK_WIDGET (gis);
}

static void
gnome_icon_selection_destroy (GtkObject *object)
{
	GnomeIconSelection *gis;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTION(object));
	
	gis = GNOME_ICON_SELECTION (object);

	if (gis->_priv->load_idle != 0)
		g_source_remove (gis->_priv->load_idle);
	gis->_priv->load_idle = 0;

	if (gis->_priv->load_loop != NULL &&
	    g_main_loop_is_running (gis->_priv->load_loop))
		g_main_loop_quit (gis->_priv->load_loop);

	GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}


static void
gnome_icon_selection_finalize (GObject *object)
{
	GnomeIconSelection *gis;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTION(object));
	
	gis = GNOME_ICON_SELECTION (object);

	/*clear our data if we have some*/
	if (gis->_priv->file_list != NULL) {
		g_list_foreach (gis->_priv->file_list, (GFunc)g_free, NULL);
		g_list_free (gis->_priv->file_list);
		gis->_priv->file_list = NULL;
	}

	g_free (gis->_priv);
	gis->_priv = NULL;

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

/**
 * gnome_icon_selection_add_defaults:
 * @gis: GnomeIconSelection to work with
 *
 * Description: Adds the default pixmap directory into the selection
 * widget. It doesn't show the icons in the selection until you
 * do #gnome_icon_selection_show_icons.
 **/
void
gnome_icon_selection_add_defaults (GnomeIconSelection * gis)
{
  gchar *pixmap_dir;

  g_return_if_fail (gis != NULL);
  g_return_if_fail (GNOME_IS_ICON_SELECTION (gis));

  pixmap_dir = gnome_unconditional_datadir_file("pixmaps");
  
  gnome_icon_selection_add_directory(gis, pixmap_dir);

  g_free(pixmap_dir);
}

static void 
append_an_icon(GnomeIconSelection * gis, const gchar * path)
{
	GdkPixbuf *pixbuf;
	int pos;
	int w, h;
	char *base;
	
	pixbuf = gdk_pixbuf_new_from_file(path, NULL);
	/*if I can't load it, ignore it*/
	if(pixbuf == NULL)
		return;
	
	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);
	if(w > h) {
		if(w > ICON_SIZE) {
			h = h * ((double)ICON_SIZE / w);
			w = ICON_SIZE;
		}
	} else {
		if(h > ICON_SIZE) {
			w = w * ((double)ICON_SIZE / h);
			h = ICON_SIZE;
		}
	}
	w = (w > 0) ? w : 1;
	h = (h > 0) ? h : 1;
	
	if (w != gdk_pixbuf_get_width (pixbuf) ||
	    h != gdk_pixbuf_get_height (pixbuf)) {
		GdkPixbuf *scaled;
	        scaled = gdk_pixbuf_scale_simple(pixbuf, w, h,
						 GDK_INTERP_BILINEAR);
		g_object_unref (G_OBJECT (pixbuf));
		pixbuf = scaled;

		/* sanity */
		if(pixbuf == NULL)
			return;
	}
	
	base = g_path_get_basename(path);
	pos = gnome_icon_list_append_pixbuf(GNOME_ICON_LIST(gis->_priv->gil),
					    pixbuf, path, base);
	g_free(base);
	g_object_unref (G_OBJECT (pixbuf)); /* I'm so glad that gdk-pixbuf has eliminated the former lameness of imlib! :) */
}

static int sort_file_list( gconstpointer a, gconstpointer b)
{
	return strcoll( (gchar *)a, (gchar *)b );
}

/**
 * gnome_icon_selection_add_directory:
 * @gis: GnomeIconSelection to work with
 * @dir: directory with pixmaps
 *
 * Description: Adds the icons from the directory @dir to the
 * selection widget. It doesn't show the icons in the selection
 * until you do #gnome_icon_selection_show_icons.
 **/
void
gnome_icon_selection_add_directory (GnomeIconSelection * gis,
				    const gchar * dir)
{
  struct dirent * de;
  DIR * dp;

  g_return_if_fail (gis != NULL);
  g_return_if_fail (GNOME_IS_ICON_SELECTION (gis));
  g_return_if_fail (dir != NULL);

  if ( ! g_file_test (dir, G_FILE_TEST_IS_DIR)) {
	  g_warning(_("GnomeIconSelection: '%s' does not exist or is not "
		      "a directory"), dir);
	  return;
  }

  dp = opendir(dir);

  if ( dp == NULL ) {
    g_warning(_("GnomeIconSelection: couldn't open directory '%s'"), dir);
    return;
  }

  while ( (de = readdir(dp)) != NULL ) {
    const char *mimetype;
    char *uri;
    GnomeVFSFileInfo *info;
    gchar *full_path;
    
    if ( *(de->d_name) == '.' ) continue; /* skip dotfiles */

    full_path = g_build_filename (dir, de->d_name, NULL);
    uri = g_filename_to_uri (full_path, "localhost", NULL);
    info = gnome_vfs_file_info_new ();
    g_free (full_path);
    /* FIXME: OK to do synchronous I/O here? */
    gnome_vfs_get_file_info (uri, info, GNOME_VFS_FILE_INFO_GET_MIME_TYPE | 
			     GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
    mimetype = info->mime_type;
    g_free (uri);

    if (mimetype != NULL &&
	strncmp(mimetype, "image", strlen("image")) == 0 ) {
      gchar * full_path = g_build_filename (dir, de->d_name, NULL);

      if (g_file_test (full_path, G_FILE_TEST_IS_REGULAR)) {
	      /* Image filename, exists, regular file, go for it. */
	      gis->_priv->file_list =
		      g_list_insert_sorted(gis->_priv->file_list,
					   g_strdup (full_path),
					   sort_file_list);
      }
      g_free(full_path);
    }
    gnome_vfs_file_info_unref (info);
  }

  closedir(dp);
}

static gboolean
load_idle_func (gpointer data)
{
	GnomeIconSelection * gis = data;
	GList * list = gis->_priv->file_list;

	if (list == NULL) {
		if (gis->_priv->load_loop != NULL &&
		    g_main_loop_is_running (gis->_priv->load_loop))
			g_main_loop_quit (gis->_priv->load_loop);
		gis->_priv->load_idle = 0;
		return FALSE;
	}

	GDK_THREADS_ENTER();

	append_an_icon (gis, list->data);

	g_free (list->data);
	gis->_priv->file_list = g_list_remove_link (gis->_priv->file_list, list);
	g_list_free_1 (list);

	/* If icons were added while adding */
	if (gis->_priv->load_i > gis->_priv->load_file_count)
		gis->_priv->load_file_count = gis->_priv->load_i;

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (gis->_priv->load_progressbar),
				       (double)gis->_priv->load_i / gis->_priv->load_file_count);

	gis->_priv->load_i ++;

	GDK_THREADS_LEAVE();

	return TRUE;
}

/**
 * gnome_icon_selection_show_icons:
 * @gis: GnomeIconSelection to work with
 *
 * Description: Shows the icons inside the widget that
 * were added with #gnome_icon_selection_add_defaults and
 * #gnome_icon_selection_add_directory. Before this function
 * is called the icons aren't actually added to the listing 
 * and can't be picked by the user.
 **/
void
gnome_icon_selection_show_icons (GnomeIconSelection * gis)
{
	GtkWidget *label;
	GtkWidget *progressbar;

	g_return_if_fail (gis != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTION (gis));

	/* Nothing to load */
	if (gis->_priv->file_list == NULL)
		return;

	/* We're already loading! */
	if (gis->_priv->load_loop != NULL)
		return;

	label = gtk_label_new (_("Loading Icons..."));
	_add_atk_relation (GTK_WIDGET (gis), label,
			   ATK_RELATION_LABELLED_BY, ATK_RELATION_LABEL_FOR);
	gtk_box_pack_start (GTK_BOX (gis->_priv->box),
			    label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	g_signal_connect (label, "destroy",
			  G_CALLBACK (gtk_widget_destroyed),
			  &label);

	progressbar = gtk_progress_bar_new ();
	_add_atk_relation (progressbar, label,
			   ATK_RELATION_LABELLED_BY, ATK_RELATION_LABEL_FOR);
	gtk_box_pack_start (GTK_BOX (gis->_priv->box),
			    progressbar, FALSE, FALSE, 0);
	gtk_widget_show (progressbar);
	g_signal_connect (progressbar, "destroy",
			  G_CALLBACK (gtk_widget_destroyed),
			  &progressbar);

	gnome_icon_list_freeze (GNOME_ICON_LIST (gis->_priv->gil));

	/* Ref to avoid object disappearing from under us */
	g_object_ref (G_OBJECT (gis));

	gis->_priv->load_loop = g_main_loop_new (NULL, FALSE);
	gis->_priv->load_i = 0;
	gis->_priv->load_file_count = g_list_length (gis->_priv->file_list);
	gis->_priv->load_progressbar = progressbar;

	if (gis->_priv->load_idle != 0)
		g_source_remove (gis->_priv->load_idle);
	gis->_priv->load_idle = g_idle_add (load_idle_func, gis);

	GDK_THREADS_LEAVE ();  
	g_main_loop_run (gis->_priv->load_loop);
	GDK_THREADS_ENTER ();  

	if (gis->_priv->load_idle != 0)
		g_source_remove (gis->_priv->load_idle);
	gis->_priv->load_idle = 0;

	if (gis->_priv->load_loop != NULL)
		g_main_loop_unref (gis->_priv->load_loop);
	gis->_priv->load_loop = NULL;

	gis->_priv->load_progressbar = NULL;

	if (gis->_priv->gil != NULL)
		gnome_icon_list_thaw (GNOME_ICON_LIST (gis->_priv->gil));

	if (progressbar != NULL)
		gtk_widget_destroy (progressbar);
	if (label != NULL)
		gtk_widget_destroy (label);

	g_object_unref (G_OBJECT (gis));
}

/**
 * gnome_icon_selection_stop_loading:
 * @gis: GnomeIconSelection to work with
 *
 * Description: Stop the loading of images when we are in
 * the loop in show_icons, otherwise it does nothing and is
 * harmless, it should be used say if the dialog was hidden
 * or when we want to quickly stop loading the images to do
 * something else without destroying the icon selection object.
 * The remaining icons can be shown by
 * #gnome_icon_selection_show_icons.
 **/
void
gnome_icon_selection_stop_loading (GnomeIconSelection * gis)
{
	g_return_if_fail (gis != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTION (gis));
	
	if (gis->_priv->load_loop != NULL &&
	    g_main_loop_is_running (gis->_priv->load_loop))
		g_main_loop_quit (gis->_priv->load_loop);
}

/**
 * gnome_icon_selection_clear:
 * @gis: GnomeIconSelection to work with
 * @not_shown: boolean
 *
 * Description: Clear the currently shown icons, the ones
 * that weren't shown yet are not cleared unless the not_shown
 * parameter is given, in which case even those are cleared.
 **/
void
gnome_icon_selection_clear (GnomeIconSelection * gis,
			    gboolean not_shown)
{
	g_return_if_fail(gis != NULL);
	g_return_if_fail(GNOME_IS_ICON_SELECTION(gis));

	/*clear our data if we have some and not_shown is set*/
	if(not_shown &&
	   gis->_priv->file_list != NULL) {
		g_list_foreach(gis->_priv->file_list,(GFunc)g_free,NULL);
		g_list_free(gis->_priv->file_list);
		gis->_priv->file_list = NULL;
	}

	gnome_icon_list_clear(GNOME_ICON_LIST(gis->_priv->gil));
}

/**
 * gnome_icon_selection_get_icon:
 * @gis: GnomeIconSelection to work with
 * @full_path: boolean
 *
 * Description: Gets the currently selected icon name, if
 * full_path is true, it returns the full path to the icon,
 * if none is selected it returns NULL
 *
 * Returns: internal string, it must not be changed or freed
 * or NULL
 **/
gchar * 
gnome_icon_selection_get_icon (GnomeIconSelection * gis,
			       gboolean full_path)
{
	GList * sel;

	g_return_val_if_fail (gis != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_SELECTION (gis), NULL);

	sel = gnome_icon_list_get_selection(GNOME_ICON_LIST(gis->_priv->gil));
	if (sel != NULL) {
		gchar * p;
		gint pos = GPOINTER_TO_INT(sel->data);
		p = gnome_icon_list_get_icon_filename(GNOME_ICON_LIST(gis->_priv->gil), pos);
		if (full_path)
			return g_strdup(p);
		else
			return g_path_get_basename(p);
	} else {
		return NULL;
	}
}

/**
 * gnome_icon_selection_select_icon:
 * @gis: GnomeIconSelection to work with
 * @filename: icon filename
 *
 * Description: Selects the icon @filename. This icon must have
 * already been added and shown * (see @gnome_icon_selection_show_icons)
 **/
void
gnome_icon_selection_select_icon (GnomeIconSelection * gis,
				  const gchar * filename)
{
	GnomeIconList *gil;
	gint pos;
	gint icons;

	g_return_if_fail (gis != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTION (gis));
	g_return_if_fail (filename != NULL);

	gil = GNOME_ICON_LIST(gis->_priv->gil);
	icons = gnome_icon_list_get_num_icons(gil);

	for(pos = 0; pos < icons; pos++) {
		char *base;
		gchar * file = gnome_icon_list_get_icon_filename(gil, pos);
		base = g_path_get_basename(file);
		if (strcmp(base, filename) == 0) {
			gnome_icon_list_select_icon(gil, pos);
			g_free(base);
			return;
		}
		g_free(base);
	}
}

/**
 * gnome_icon_selection_get_gil:
 * @gis: GnomeIconSelection to work with
 *
 * Description: Gets the #GnomeIconList widget that is
 * used for the display of icons
 *
 * Returns: a #GtkWidget pointer to the interal widget
 **/
GtkWidget *
gnome_icon_selection_get_gil (GnomeIconSelection * gis)
{
	g_return_val_if_fail(gis != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_ICON_SELECTION(gis), NULL);

	return gis->_priv->gil;
}

/**
 * gnome_icon_selection_get_box:
 * @gis: GnomeIconSelection to work with
 *
 * Description: Gets the #GtkVBox widget that is
 * used to pack the different elements of the selection
 * into.
 *
 * Returns: a #GtkWidget pointer to the interal widget
 **/
GtkWidget *
gnome_icon_selection_get_box (GnomeIconSelection * gis)
{
	g_return_val_if_fail(gis != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_ICON_SELECTION(gis), NULL);

	return gis->_priv->box;
}
