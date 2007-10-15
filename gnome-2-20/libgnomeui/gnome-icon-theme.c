/* GnomeIconThemes - a loader for icon-themes
 * gnome-icon-theme.c Copyright (C) 2002 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "gnome-icon-theme.h"

typedef struct _GtkIconThemePrivate GnomeIconThemePrivate;

struct _GtkIconThemePrivate
{
  GtkIconTheme *gtk_theme;
  gboolean allow_svg;
  GnomeIconData icon_data;
};

static void   gnome_icon_theme_class_init (GnomeIconThemeClass *klass);
static void   gnome_icon_theme_init       (GnomeIconTheme      *icon_theme);
static void   gnome_icon_theme_finalize   (GObject             *object);
static void   set_gtk_theme               (GnomeIconTheme      *icon_theme,
					   GtkIconTheme        *gtk_theme);


static guint		 signal_changed = 0;
static GObjectClass     *parent_class = NULL;

GType
gnome_icon_theme_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    {
      const GTypeInfo info =
	{
	  sizeof (GnomeIconThemeClass),
	  NULL,           /* base_init */
	  NULL,           /* base_finalize */
	  (GClassInitFunc) gnome_icon_theme_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (GnomeIconTheme),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) gnome_icon_theme_init
	};

      type = g_type_register_static (G_TYPE_OBJECT, "GnomeIconTheme", &info, 0);
    }

  return type;
}

GnomeIconTheme *
gnome_icon_theme_new (void)
{
  return g_object_new (GNOME_TYPE_ICON_THEME, NULL);
}

static void
gtk_theme_changed_callback (GtkIconTheme *gtk_theme,
			    gpointer user_data)
{
  GnomeIconTheme *icon_theme;

  icon_theme = user_data;
  
  g_signal_emit (G_OBJECT (icon_theme), signal_changed, 0);
}

static void
gnome_icon_theme_class_init (GnomeIconThemeClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gnome_icon_theme_finalize;

  signal_changed = g_signal_new ("changed",
				 G_TYPE_FROM_CLASS (klass),
				 G_SIGNAL_RUN_LAST,
				 G_STRUCT_OFFSET (GnomeIconThemeClass, changed),
				 NULL, NULL,
				 g_cclosure_marshal_VOID__VOID,
				 G_TYPE_NONE, 0);
}

static void
set_gtk_theme (GnomeIconTheme *icon_theme,
	       GtkIconTheme *gtk_theme)
{
  icon_theme->priv->gtk_theme = gtk_theme;
  g_signal_connect_object (gtk_theme,
			   "changed",
			   G_CALLBACK (gtk_theme_changed_callback),
			   icon_theme, 0);
}

static GtkIconTheme *
get_gtk_theme (GnomeIconTheme *icon_theme)
{
  GnomeIconThemePrivate *priv;
  GtkIconTheme *gtk_theme;
  
  priv = icon_theme->priv;

  if (priv->gtk_theme == NULL)
    {
      gtk_theme = gtk_icon_theme_new ();
      gtk_icon_theme_set_screen (gtk_theme, gdk_screen_get_default ());
      set_gtk_theme (icon_theme, gtk_theme);
    }
  
  return priv->gtk_theme;
}

GtkIconTheme  *
_gnome_icon_theme_get_gtk_icon_theme (GnomeIconTheme *icon_theme)
{
  return get_gtk_theme (icon_theme);
}


static void
gnome_icon_theme_init (GnomeIconTheme *icon_theme)
{
  GnomeIconThemePrivate *priv;

  priv = g_new0 (GnomeIconThemePrivate, 1);
  
  icon_theme->priv = priv;

  priv->gtk_theme = NULL;
  priv->allow_svg = FALSE;
}

static void
gnome_icon_theme_finalize (GObject *object)
{
  GnomeIconTheme *icon_theme;
  GnomeIconThemePrivate *priv;

  icon_theme = GNOME_ICON_THEME (object);
  priv = icon_theme->priv;

  if (priv->gtk_theme)
    g_object_unref (priv->gtk_theme);
  
  g_free (priv);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
gnome_icon_theme_set_allow_svg (GnomeIconTheme      *icon_theme,
				 gboolean              allow_svg)
{
  allow_svg = allow_svg != FALSE;

  if (allow_svg == icon_theme->priv->allow_svg)
    return;
  
  icon_theme->priv->allow_svg = allow_svg;
  
  g_signal_emit (G_OBJECT (icon_theme), signal_changed, 0);
}

gboolean
gnome_icon_theme_get_allow_svg (GnomeIconTheme *icon_theme)
{
  return icon_theme->priv->allow_svg;
}

void
gnome_icon_theme_set_search_path (GnomeIconTheme *icon_theme,
				  const char *path[],
				  int         n_elements)
{
  gtk_icon_theme_set_search_path (get_gtk_theme (icon_theme),
				  path, n_elements);
}


void
gnome_icon_theme_get_search_path (GnomeIconTheme      *icon_theme,
				  char                 **path[],
				  int                   *n_elements)
{
  gtk_icon_theme_get_search_path (get_gtk_theme (icon_theme),
				  path, n_elements);
}

void
gnome_icon_theme_append_search_path (GnomeIconTheme      *icon_theme,
				     const char          *path)
{
  gtk_icon_theme_append_search_path (get_gtk_theme (icon_theme), path);
}

void
gnome_icon_theme_prepend_search_path (GnomeIconTheme      *icon_theme,
				       const char           *path)
{
  gtk_icon_theme_prepend_search_path (get_gtk_theme (icon_theme), path);
}

void
gnome_icon_theme_set_custom_theme (GnomeIconTheme *icon_theme,
				   const char *theme_name)
{
  gtk_icon_theme_set_custom_theme (get_gtk_theme (icon_theme), theme_name);
}

char *
gnome_icon_theme_lookup_icon (GnomeIconTheme      *icon_theme,
			      const char           *icon_name,
			      int                   size,
			      const GnomeIconData **icon_data_out,
			      int                  *base_size)
{
  GnomeIconThemePrivate *priv;
  int i;
  char *icon;
  GtkIconInfo *info;
  GtkIconLookupFlags flags;
  GdkRectangle rectangle;
  GdkPoint *points;
  int n_points;
  GnomeIconData *icon_data;
  priv = icon_theme->priv;

  if (icon_data_out)
    *icon_data_out = NULL;
  
  if (priv->allow_svg)
      flags = GTK_ICON_LOOKUP_FORCE_SVG;
  else
      flags = GTK_ICON_LOOKUP_NO_SVG;
  
  info = gtk_icon_theme_lookup_icon (get_gtk_theme (icon_theme),
				     icon_name, size, flags);

  if (info == NULL)
    return NULL;

  icon = g_strdup (gtk_icon_info_get_filename (info));
  
  if (base_size != NULL)
    *base_size = gtk_icon_info_get_base_size (info);

  /* Free the old info */

  icon_data = &priv->icon_data;
  g_free (icon_data->display_name);
  g_free (icon_data->attach_points);
  memset (icon_data, 0, sizeof (GnomeIconData));
  
  icon_data->display_name = g_strdup (gtk_icon_info_get_display_name (info));

  gtk_icon_info_set_raw_coordinates (info, TRUE);
  icon_data->has_embedded_rect = gtk_icon_info_get_embedded_rect (info, &rectangle);
  if (icon_data->has_embedded_rect)
    {
      icon_data->x0 = rectangle.x;
      icon_data->y0 = rectangle.y;
      icon_data->x1 = rectangle.x + rectangle.width;
      icon_data->y1 = rectangle.y + rectangle.height;
    }
  
  if (gtk_icon_info_get_attach_points (info, &points, &n_points))
    {
      icon_data->n_attach_points = n_points;
      icon_data->attach_points = g_new (GnomeIconDataPoint, n_points);
      for (i = 0; i < n_points; i++)
	{
	  icon_data->attach_points[i].x = points[i].x;
	  icon_data->attach_points[i].y = points[i].y;
	}
      g_free (points);
    }

  if (icon_data_out != NULL &&
      (icon_data->has_embedded_rect ||
       icon_data->attach_points != NULL ||
       icon_data->display_name != NULL))
    *icon_data_out = icon_data;
  
   
  gtk_icon_info_free (info);
    
  return icon;
}

gboolean 
gnome_icon_theme_has_icon (GnomeIconTheme      *icon_theme,
			    const char           *icon_name)
{
  return gtk_icon_theme_has_icon (get_gtk_theme (icon_theme), icon_name);
}



GList *
gnome_icon_theme_list_icons (GnomeIconTheme *icon_theme,
			     const char *context)
{
  return gtk_icon_theme_list_icons (get_gtk_theme (icon_theme), context);
}

char *
gnome_icon_theme_get_example_icon_name (GnomeIconTheme *icon_theme)
{
  return gtk_icon_theme_get_example_icon_name (get_gtk_theme (icon_theme));
}

gboolean
gnome_icon_theme_rescan_if_needed (GnomeIconTheme *icon_theme)
{
  return gtk_icon_theme_rescan_if_needed (get_gtk_theme (icon_theme));
}

void
gnome_icon_data_free (GnomeIconData *icon_data)
{
  g_free (icon_data->attach_points);
  g_free (icon_data->display_name);
  g_free (icon_data);
}

GnomeIconData *
gnome_icon_data_dup (const GnomeIconData *icon_data)
{
  GnomeIconData *copy;

  copy = g_memdup (icon_data, sizeof (GnomeIconData));
  
  copy->display_name = g_strdup (copy->display_name);
  
  if (copy->attach_points)
    copy->attach_points = g_memdup (copy->attach_points,
				    copy->n_attach_points * sizeof (GnomeIconDataPoint));
  return copy;
}

