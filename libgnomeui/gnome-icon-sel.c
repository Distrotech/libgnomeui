/* gnome-icon-sel.c: Copyright (C) 1998 Free Software Foundation
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gnome-icon-sel.h"

#include <config.h>

#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-util.h"
#include "gnome-uidefs.h"
#include "gnome-pixmap.h"

#include <unistd.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define ICON_SIZE 48

static void gnome_icon_selection_class_init (GnomeIconSelectionClass *klass);
static void gnome_icon_selection_init       (GnomeIconSelection      *messagebox);

static void gnome_icon_selection_destroy (GtkObject *gis);

static GtkWindowClass *parent_class;

static int sort_file_list( gconstpointer a, gconstpointer b);

guint
gnome_icon_selection_get_type ()
{
  static guint gis_type = 0;

  if (!gis_type)
    {
      GtkTypeInfo gis_info =
      {
	"GnomeIconSelection",
	sizeof (GnomeIconSelection),
	sizeof (GnomeIconSelectionClass),
	(GtkClassInitFunc) gnome_icon_selection_class_init,
	(GtkObjectInitFunc) gnome_icon_selection_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      gis_type = gtk_type_unique (gtk_vbox_get_type (), &gis_info);
    }

  return gis_type;
}

static void
gnome_icon_selection_class_init (GnomeIconSelectionClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) klass;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = gnome_icon_selection_destroy;
}

static void
gnome_icon_selection_init (GnomeIconSelection *gis)
{
  gis->box = gtk_vbox_new(FALSE, 0);

  gtk_container_add(GTK_CONTAINER(gis), gis->box);

  gtk_widget_show(gis->box);

  gis->clist = gtk_clist_new(1);
  gtk_clist_set_row_height(GTK_CLIST(gis->clist), 
			   ICON_SIZE + GNOME_PAD_SMALL);

  gtk_box_pack_end(GTK_BOX(gis->box) , gis->clist, TRUE, TRUE, 0);

  gtk_widget_show(gis->clist);

  gis->file_list = NULL;
}


GtkWidget* gnome_icon_selection_new (void)
{
  GnomeIconSelection * gis;
  
  gis = gtk_type_new(gnome_icon_selection_get_type());

  return GTK_WIDGET (gis);
}

static void gnome_icon_selection_destroy (GtkObject *gis)
{
  g_return_if_fail(gis != NULL);
  g_return_if_fail(GNOME_IS_ICON_SELECTION(gis));

  /* FIXME Does nothing, should come out */

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* (GTK_OBJECT_CLASS(parent_class)->destroy))(gis);
}

void  gnome_icon_selection_add_defaults   (GnomeIconSelection * gis)
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
  GtkWidget * pixmap;
  gint row;
  gchar * useless[] = { NULL };

#ifdef GNOME_ENABLE_DEBUG
  g_print("Loading: %s\n", path);
#endif
  pixmap = gnome_pixmap_new_from_file (path);
  
  /* distorts things */
  /*
    pixmap = gnome_pixmap_new_from_file_at_size(path, ICON_SIZE, 
    ICON_SIZE);
    */

  /* If we can't load it, we just ignore it. */  
  if (pixmap) {
    row = GTK_CLIST(gis->clist)->rows; /* new row number */
    
    gtk_clist_insert(GTK_CLIST(gis->clist), row, useless);

    gtk_clist_set_pixtext (GTK_CLIST(gis->clist), row, 0, 
			   g_filename_pointer(path), GNOME_PAD_SMALL,
			   GNOME_PIXMAP(pixmap)->pixmap, 
			   GNOME_PIXMAP(pixmap)->mask);

    gtk_clist_set_row_data_full ( GTK_CLIST(gis->clist),
				  row, 
				  g_strdup(path),
				  (GtkDestroyNotify) g_free );
  }
#ifdef GNOME_ENABLE_DEBUG
  else {
    g_print("Failed to load icon.\n");
  }
#endif
}

static int sort_file_list( gconstpointer a, gconstpointer b)
{
  return strcmp( (gchar *)a, (gchar *)b );
}

void  gnome_icon_selection_add_directory  (GnomeIconSelection * gis,
					   const gchar * dir)
{
  struct stat statbuf;
  struct dirent * de;
  DIR * dp;

  g_return_if_fail(gis != NULL);
  g_return_if_fail(dir != NULL);

  if ( stat(dir, &statbuf) == -1 ) {
    g_warning("GnomeIconSelection: Couldn't stat directory");
    return;
  }

  if ( ! S_ISDIR(statbuf.st_mode) ) {
    g_warning("GnomeIconSelection: not a directory");
    return;
  }

  dp = opendir(dir);

  if ( dp == NULL ) {
    g_warning("GnomeIconSelection: couldn't open directory");
    return;
  }

  while ( (de = readdir(dp)) != NULL ) {
#ifdef GNOME_ENABLE_DEBUG
    g_print("File: %s\n", de->d_name);
#endif
    if ( *(de->d_name) == '.' ) continue; /* skip dotfiles */

    if ( g_is_image_filename(de->d_name) ) {
      gchar * full_path = g_concat_dir_and_file(dir, de->d_name);
#ifdef GNOME_ENABLE_DEBUG
    g_print("Full path: %s\n", full_path);
#endif
      if ( stat(full_path, &statbuf) != -1 ) {
	if ( S_ISREG(statbuf.st_mode) ) {
	  /* Image filename, exists, regular file, go for it. */
          gis->file_list = g_list_insert_sorted(gis->file_list,
            (gchar *)strdup(full_path), sort_file_list);
	}
      }
      g_free(full_path);
    }
  }

  closedir(dp);
}

void  gnome_icon_selection_show_icons  (GnomeIconSelection * gis)
{
  GList * list;
  GtkWidget *label;
  GtkWidget *progressbar;
  int file_count, i;

  g_return_if_fail(gis != NULL);
  g_return_if_fail(gis->file_list != NULL);

  list = gis->file_list;
  
  label = gtk_label_new(_("Loading Icons..."));
  gtk_box_pack_start(GTK_BOX(gis->box),label,FALSE,FALSE,0);
  gtk_widget_show(label);

  progressbar = gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(gis->box),progressbar,FALSE,FALSE,0);
  gtk_widget_show(progressbar);

  file_count = g_list_length(list);
  i = 0;
  
  gtk_clist_freeze(GTK_CLIST(gis->clist));

  while (list) {
    append_an_icon(gis, list->data);
    g_free(list->data);
    list = list->next;
    gtk_progress_bar_update (GTK_PROGRESS_BAR (progressbar), (float)i / file_count);
    while ( gtk_events_pending() ) {
      gtk_main_iteration();
    }
    i++;
  }

  g_list_free(gis->file_list);
  gis->file_list = NULL;

  gtk_clist_thaw(GTK_CLIST(gis->clist));

  gtk_widget_destroy(progressbar);
  gtk_widget_destroy(label);
}


void  gnome_icon_selection_clear          (GnomeIconSelection * gis)
{
  g_return_if_fail(gis != NULL);

  gtk_clist_clear(GTK_CLIST(gis->clist));
}

const gchar * 
gnome_icon_selection_get_icon     (GnomeIconSelection * gis,
				   gboolean full_path)
{
  GList * sel;

  g_return_val_if_fail(gis != NULL, NULL);

  sel = GTK_CLIST(gis->clist)->selection;
  if ( sel ) {
    gchar * p;
    gint row = (gint)sel->data;
    p = gtk_clist_get_row_data(GTK_CLIST(gis->clist),
			       row);
    if (full_path) return p;
    else return g_filename_pointer(p);
  }
  else return NULL;
}

void  gnome_icon_selection_select_icon    (GnomeIconSelection * gis,
					   const gchar * filename)
{
  gint row;
  gint num_rows;
  
  g_return_if_fail(gis != NULL);
  g_return_if_fail(filename != NULL);

  num_rows = GTK_CLIST(gis->clist)->rows;
  row = 0;

  while ( row < num_rows ) {
    gchar * file = 
      gtk_clist_get_row_data(GTK_CLIST(gis->clist),
			     row);
    if ( strcmp(g_filename_pointer(file),filename) == 0 ) {
      gtk_clist_select_row(GTK_CLIST(gis->clist), row, 0);
      return;
    }

    ++row;
  }
}
