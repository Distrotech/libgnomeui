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
#include "gnome-macros.h"

#include <unistd.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include <libgnome/gnome-util.h>
#include "gnome-icon-list.h"
#include "gnome-uidefs.h"

#include "gnome-icon-sel.h"

#include <libgnomevfs/gnome-vfs-mime.h>

#define ICON_SIZE 48

struct _GnomeIconSelectionPrivate {
  GtkWidget * box;

  GtkWidget * gil;

  GList * file_list;
  
  gboolean stop_loading; /* a flag set to stop the loading of images in midprocess */
};

static void gnome_icon_selection_class_init (GnomeIconSelectionClass *klass);
static void gnome_icon_selection_init       (GnomeIconSelection      *gis);

static void gnome_icon_selection_destroy    (GtkObject               *object);
static void gnome_icon_selection_finalize   (GObject                 *object);

static int sort_file_list		    (gconstpointer            a,
					     gconstpointer            b);

GNOME_CLASS_BOILERPLATE (GnomeIconSelection, gnome_icon_selection,
			 GtkVBox, gtk_vbox)

static void
gnome_icon_selection_class_init (GnomeIconSelectionClass *klass)
{
  GtkObjectClass *object_class;
  GObjectClass *gobject_class;

  object_class = (GtkObjectClass*) klass;
  gobject_class = (GObjectClass*) klass;

  object_class->destroy = gnome_icon_selection_destroy;
  gobject_class->finalize = gnome_icon_selection_finalize;
}

static void
gnome_icon_selection_init (GnomeIconSelection *gis)
{
	GtkAdjustment *vadj;
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

	gis->_priv->gil = gnome_icon_list_new(ICON_SIZE+30, FALSE);
	gtk_widget_set_usize(gis->_priv->gil,350,300);
	gnome_icon_list_set_selection_mode(GNOME_ICON_LIST(gis->_priv->gil),
					    GTK_SELECTION_SINGLE);

	vadj = gtk_layout_get_vadjustment(GTK_LAYOUT(gis->_priv->gil));
	sb = gtk_vscrollbar_new(vadj);
	gtk_box_pack_end(GTK_BOX(box),sb,FALSE,FALSE,0);
	gtk_widget_show(sb);

	gtk_container_add (GTK_CONTAINER (frame), gis->_priv->gil);
	gtk_widget_show(gis->_priv->gil);

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
  
  gis = gtk_type_new(gnome_icon_selection_get_type());

  return GTK_WIDGET (gis);
}

static void
gnome_icon_selection_destroy (GtkObject *object)
{
	GnomeIconSelection *gis;

	g_return_if_fail(object != NULL);
	g_return_if_fail(GNOME_IS_ICON_SELECTION(object));

	/* remember, destroy can be run multiple times! */
	
	gis = GNOME_ICON_SELECTION(object);

	/*clear our data if we have some*/
	if(gis->_priv->file_list) {
		g_list_foreach(gis->_priv->file_list,(GFunc)g_free,NULL);
		g_list_free(gis->_priv->file_list);
		gis->_priv->file_list = NULL;
	}

	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_icon_selection_finalize (GObject *object)
{
	GnomeIconSelection *gis;

	g_return_if_fail(object != NULL);
	g_return_if_fail(GNOME_IS_ICON_SELECTION(object));
	
	gis = GNOME_ICON_SELECTION(object);

	g_free(gis->_priv);
	gis->_priv = NULL;

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}

/**
 * gnome_icon_selection_add_defaults:
 * @gis: GnomeIconSelection to work with
 *
 * Description: Adds the default pixmap directory into the selection
 * widget. It doesn't show the icons in the selection until you
 * do #gnome_icon_selection_show_icons.
 *
 * Returns:
 **/
void
gnome_icon_selection_add_defaults (GnomeIconSelection * gis)
{
  gchar *pixmap_dir;

  g_return_if_fail(gis != NULL);

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
		gdk_pixbuf_unref(pixbuf);
		pixbuf = scaled;

		/* sanity */
		if(pixbuf == NULL)
			return;
	}
	
	base = g_path_get_basename(path);
	pos = gnome_icon_list_append_pixbuf(GNOME_ICON_LIST(gis->_priv->gil),
					    pixbuf, path, base);
	g_free(base);
	gdk_pixbuf_unref(pixbuf); /* I'm so glad that gdk-pixbuf has eliminated the former lameness of imlib! :) */
}

static int sort_file_list( gconstpointer a, gconstpointer b)
{
	return strcmp( (gchar *)a, (gchar *)b );
}

/**
 * gnome_icon_selection_add_directory:
 * @gis: GnomeIconSelection to work with
 * @dir: directory with pixmaps
 *
 * Description: Adds the icons from the directory @dir to the
 * selection widget. It doesn't show the icons in the selection
 * until you do #gnome_icon_selection_show_icons.
 *
 * Returns:
 **/
void
gnome_icon_selection_add_directory (GnomeIconSelection * gis,
				    const gchar * dir)
{
  struct dirent * de;
  DIR * dp;

  g_return_if_fail(gis != NULL);
  g_return_if_fail(dir != NULL);

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
#ifdef GNOME_ENABLE_DEBUG
    g_print("File: %s\n", de->d_name);
#endif
    if ( *(de->d_name) == '.' ) continue; /* skip dotfiles */

    mimetype = gnome_vfs_mime_type_from_name(de->d_name);
    if (mimetype != NULL &&
	strncmp(mimetype, "image", strlen("image")) == 0 ) {
      gchar * full_path = g_concat_dir_and_file(dir, de->d_name);
#ifdef GNOME_ENABLE_DEBUG
      g_print("Full path: %s\n", full_path);
#endif
      if (g_file_test (full_path, G_FILE_TEST_IS_REGULAR)) {
	      /* Image filename, exists, regular file, go for it. */
	      gis->_priv->file_list =
		      g_list_insert_sorted(gis->_priv->file_list,
					   g_strdup (full_path),
					   sort_file_list);
      }
      g_free(full_path);
    }
  }

  closedir(dp);
}

static void
set_flag(GtkWidget *w, int *flag)
{
	*flag = TRUE;
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
 *
 * Returns:
 **/
void
gnome_icon_selection_show_icons (GnomeIconSelection * gis)
{
  GtkWidget *label;
  GtkWidget *progressbar;
  int file_count, i;
  int local_dest;
  int was_destroyed = FALSE;

  g_return_if_fail(gis != NULL);
  if(!gis->_priv->file_list) return;

  file_count = g_list_length(gis->_priv->file_list);
  i = 0;

  /* Locate previous progressbar/label,
   * if previously called. */
  progressbar = label = NULL;
  progressbar = gtk_object_get_user_data(GTK_OBJECT(gis));
  if (progressbar)
         label = gtk_object_get_user_data(GTK_OBJECT(progressbar));

  if (!label && !progressbar) {
         label = gtk_label_new(_("Loading Icons..."));
         gtk_box_pack_start(GTK_BOX(gis->_priv->box),label,FALSE,FALSE,0);
         gtk_widget_show(label);

         progressbar = gtk_progress_bar_new();
         gtk_box_pack_start(GTK_BOX(gis->_priv->box),progressbar,FALSE,FALSE,0);
         gtk_widget_show(progressbar);

         /* attach label to progressbar, progressbar to gis 
          * for recovery if show_icons() called again */
         gtk_object_set_user_data(GTK_OBJECT(progressbar), label);
         gtk_object_set_user_data(GTK_OBJECT(gis), progressbar);
  } else {
         if (!label && progressbar) g_assert_not_reached();
         if (label && !progressbar) g_assert_not_reached();
  }
         
  gnome_icon_list_freeze(GNOME_ICON_LIST(gis->_priv->gil));

  /* this can be set with the stop_loading method to stop the
     display in the middle */
  gis->_priv->stop_loading = FALSE;
  
  /*bind destroy so that we can bail out of this function if the whole thing
    was destroyed while doing the main_iteration*/
  local_dest = gtk_signal_connect(GTK_OBJECT(gis),"destroy",
				  GTK_SIGNAL_FUNC(set_flag),
				  &was_destroyed);

  while (gis->_priv->file_list) {
	  GList * list = gis->_priv->file_list;
	  append_an_icon(gis, list->data);
	  g_free(list->data);
	  gis->_priv->file_list = g_list_remove_link(gis->_priv->file_list,list);
	  g_list_free_1(list);

	  gtk_progress_bar_update (GTK_PROGRESS_BAR (progressbar),
				   (float)i / file_count);
	  /* FIXME: this should be done either
	   * 1) asynchroniously
	   * 2) with a separate main loop and not the gtk one */
	  while ( gtk_events_pending() ) {
                  gtk_main_iteration();

                  /*if the gis was destroyed from underneath us ... bail out*/
                  if(was_destroyed) 
                          return;
                  
                  if(gis->_priv->stop_loading)
                          goto out;
	  }
	  
	  i++;
  }

 out:
  
  gtk_signal_disconnect(GTK_OBJECT(gis),local_dest);

  gnome_icon_list_thaw(GNOME_ICON_LIST(gis->_priv->gil));

  progressbar = label = NULL;
  progressbar = gtk_object_get_user_data(GTK_OBJECT(gis));
  if (progressbar)
         label = gtk_object_get_user_data(GTK_OBJECT(progressbar));
  if (progressbar)
	  gtk_widget_destroy(progressbar);
  if (label)
	  gtk_widget_destroy(label);

  /* cleanse gis of evil progressbar/label ptrs */
  /* also let previous calls to show_icons() know that rendering is done. */
  gtk_object_set_user_data(GTK_OBJECT(gis), NULL);
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
 * The ramaining icons can be shown by
 * #gnome_icon_selection_show_icons.
 *
 * Returns:
 **/
void
gnome_icon_selection_stop_loading(GnomeIconSelection * gis)
{
	g_return_if_fail(gis != NULL);
	g_return_if_fail(GNOME_IS_ICON_SELECTION(gis));
	
	gis->_priv->stop_loading = TRUE;
}

/**
 * gnome_icon_selection_clear:
 * @gis: GnomeIconSelection to work with
 * @not_shown: boolean
 *
 * Description: Clear the currently shown icons, the ones
 * that weren't shown yet are not cleared unless the not_shown
 * parameter is given, in which case even those are cleared.
 *
 * Returns:
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

	g_return_val_if_fail(gis != NULL, NULL);

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
 *
 * Returns:
 **/
void
gnome_icon_selection_select_icon (GnomeIconSelection * gis,
				  const gchar * filename)
{
	GnomeIconList *gil;
	gint pos;
	gint icons;

	g_return_if_fail(gis != NULL);
	g_return_if_fail(filename != NULL);

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
