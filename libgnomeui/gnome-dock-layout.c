/* gnome-dock-layout.c

   Copyright (C) 1998 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/

#include <gtk/gtk.h>
#include <stdio.h>

#include "gnome-dock-layout.h"

static GtkObjectClass *parent_class = NULL;



static void   gnome_dock_layout_class_init   (GnomeDockLayoutClass  *class);

static void   gnome_dock_layout_init         (GnomeDockLayout *layout);

static void   gnome_dock_layout_destroy      (GtkObject *object);

static gint   item_compare_func              (gconstpointer a,
                                              gconstpointer b);

static gint   compare_item_by_name           (gconstpointer a,
                                              gconstpointer b);

static gint   compare_item_by_pointer        (gconstpointer a,
                                              gconstpointer b);

static GList *find                           (GnomeDockLayout *layout,
                                              gconstpointer a,
                                              GCompareFunc func);

static void   remove_item                    (GnomeDockLayout *layout,
                                              GList *list);


static void
gnome_dock_layout_class_init (GnomeDockLayoutClass  *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) class;

  object_class->destroy = gnome_dock_layout_destroy;
}

static void
gnome_dock_layout_init (GnomeDockLayout *layout)
{
  layout->items = NULL;
}

static void
gnome_dock_layout_destroy (GtkObject *object)
{
  GnomeDockLayout *layout;
  GList *lp;

  layout = GNOME_DOCK_LAYOUT (object);

  lp = layout->items;
  while (lp != NULL)
    {
      GList *next;

      next = lp->next;

      remove_item (layout, lp);

      lp = next;
    }
}



static gint
item_compare_func (gconstpointer a,
                   gconstpointer b)
{
  const GnomeDockLayoutItem *item_a, *item_b;

  item_a = a;
  item_b = b;

  if (item_a->position != item_b->position)
    return item_b->position - item_a->position;

  if (item_a->band_num != item_b->band_num)
    return item_b->band_num - item_a->band_num;

  return item_b->band_position - item_a->band_position;
}

static gint
compare_item_by_name (gconstpointer a, gconstpointer b)
{
  const GnomeDockItem *item;
  const gchar *name;

  item = b;
  name = a;

  return strcmp (name, item->name);
}

static gint
compare_item_by_pointer (gconstpointer a, gconstpointer b)
{
  return a != b;
}

static GList *
find (GnomeDockLayout *layout, gconstpointer data, GCompareFunc func)
{
  GList *p;

  for (p = layout->items; p != NULL; p = p->next)
    {
      GnomeDockLayoutItem *item;

      item = p->data;
      if (! (* func) (data, item->item))
        return p;
    }

  return NULL;
}

static void
remove_item (GnomeDockLayout *layout,
             GList *list)
{
  GnomeDockItem *item;

  item = ((GnomeDockLayoutItem *) list->data)->item;

  gtk_widget_unref (GTK_WIDGET (item));

  g_list_remove_link (layout->items, list);

  g_free (list->data);
  g_list_free (list);
}



guint
gnome_dock_layout_get_type (void)
{
  static guint layout_type = 0;
	
  if (layout_type == 0)
    {
      GtkTypeInfo layout_info =
      {
        "GnomeDockLayout",
        sizeof (GnomeDockLayout),
        sizeof (GnomeDockLayoutClass),
        (GtkClassInitFunc) gnome_dock_layout_class_init,
        (GtkObjectInitFunc) gnome_dock_layout_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL,
      };
		
      layout_type = gtk_type_unique (gtk_object_get_type (), &layout_info);
    }

  return layout_type;
}
   
GtkObject *
gnome_dock_layout_new (void)
{
  GnomeDockLayout *new;

  new = gtk_type_new (gnome_dock_layout_get_type ());

  return GTK_OBJECT (new);
}

gboolean
gnome_dock_layout_add (GnomeDockLayout *layout,
                       GnomeDockItem *item,
                       GnomeDockPositionType position,
                       gint band_num,
                       gint band_position,
                       gint offset)
{
  GnomeDockLayoutItem *new;

  new = g_new (GnomeDockLayoutItem, 1);
  new->item = item;
  new->position = position;
  new->band_num = band_num;
  new->band_position = band_position;
  new->offset = offset;

  layout->items = g_list_prepend (layout->items, new);

  gtk_object_ref (GTK_OBJECT (item));

  return TRUE;
}
   
GnomeDockLayoutItem *
gnome_dock_layout_get_item (GnomeDockLayout *layout,
                            GnomeDockItem *item)
{
  GList *list;

  list = find (layout, item, compare_item_by_pointer);

  if (list == NULL)
    return NULL;
  else
    return list->data;
}

GnomeDockLayoutItem *
gnome_dock_layout_get_item_by_name (GnomeDockLayout *layout,
                                    const gchar *name)
{
  GList *list;

  list = find (layout, name, compare_item_by_name);

  if (list == NULL)
    return NULL;
  else
    return list->data;
}

gboolean
gnome_dock_layout_remove_item (GnomeDockLayout *layout,
                               GnomeDockItem *item)
{
  GList *list;

  list = find (layout, item, compare_item_by_pointer);
  if (list == NULL)
    return FALSE;

  remove_item (layout, list);

  return TRUE;
}

gboolean
gnome_dock_layout_remove_item_by_name (GnomeDockLayout *layout,
                                       const gchar *name)
{
  GList *list;

  list = find (layout, name, compare_item_by_name);
  if (list == NULL)
    return FALSE;

  remove_item (layout, list);

  return TRUE;
}



gboolean
gnome_dock_layout_add_to_dock (GnomeDockLayout *layout,
                               GnomeDock *dock)
{
  GnomeDockLayoutItem *item;
  GList *lp;
  gint band_num;

  if (layout->items == NULL)
    return FALSE;

  layout->items = g_list_sort (layout->items, item_compare_func);

  item = layout->items->data;
  band_num = item->band_num + 1;

  for (lp = layout->items; lp != NULL; lp = lp->next)
    {
      item = lp->data;

      gnome_dock_add_item (dock,
                           GTK_WIDGET (item->item),
                           item->position,
                           0,
                           item->offset,
                           0,
                           band_num != item->band_num);

      band_num = item->band_num;

      gtk_widget_show (GTK_WIDGET (item->item));
    }

  return TRUE;
}



/* Layout string functions.  */

gchar *
gnome_dock_layout_create_string (GnomeDockLayout *layout)
{
  GList *lp;
  guint tmp_count, tmp_alloc;
  gchar **tmp;
  gchar *retval;

  if (layout->items == NULL)
    return NULL;

  tmp_alloc = 512;
  tmp = g_new (gchar *, tmp_alloc);

  tmp_count = 0;

  for (lp = layout->items; lp != NULL; lp = lp->next)
    {
      GnomeDockLayoutItem *i;

      i = lp->data;

      if (tmp_alloc - tmp_count <= 2)
        {
          tmp_alloc *= 2;
          tmp = g_renew (char *, tmp, tmp_alloc);
        }

      tmp[tmp_count] = g_strdup_printf ("%s\\%d,%d,%d,%d",
                                        i->item->name,
                                        i->position,
                                        i->band_num,
                                        i->band_position,
                                        i->offset);

      tmp_count++;
    }

  tmp[tmp_count] = 0;

  retval = g_strjoinv ("\\", tmp);
  g_strfreev (tmp);

  return retval;
}

gboolean gnome_dock_layout_parse_string (GnomeDockLayout *layout,
                                         const gchar *string)
{
  gchar **tmp, **p;

  if (string == NULL)
    return FALSE;

  tmp = g_strsplit (string, "\\", 0);
  if (tmp == NULL)
    return FALSE;

  p = tmp;
  while (*p != NULL)
    {
      GList *lp;

      if (*(p + 1) == NULL)
        {
          g_strfreev (tmp);
          return FALSE;
        }

      lp = find (layout, *p, compare_item_by_name);

      if (lp != NULL)
        {
          GnomeDockLayoutItem *i;
          gint position, band_num, band_position, offset;

          if (sscanf (*(p + 1), "%d,%d,%d,%d",
                      &position, &band_num, &band_position, &offset) != 4)
            {
              g_strfreev (tmp);
              return FALSE;
            }

          i = lp->data;

          i->position = (GnomeDockPositionType) position;
          i->band_num = band_num;
          i->band_position = band_position;
          i->offset = offset;
        }

      p += 2;
    }

  g_strfreev (tmp);

  return TRUE;
}
