/* GnomeIconTheme - a loader for icon-themes
 * gnome-icon-loader.h Copyright (C) 2002 Red Hat, Inc.
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

#ifndef GNOME_ICON_THEME_H
#define GNOME_ICON_THEME_H

#ifndef GNOME_DISABLE_DEPRECATED

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GNOME_TYPE_ICON_THEME             (gnome_icon_theme_get_type ())
#define GNOME_ICON_THEME(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_ICON_THEME, GnomeIconTheme))
#define GNOME_ICON_THEME_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ICON_THEME, GnomeIconThemeClass))
#define GNOME_IS_ICON_THEME(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_ICON_THEME))
#define GNOME_IS_ICON_THEME_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ICON_THEME))
#define GNOME_ICON_THEME_GET_CLASS(obj)   (G_TYPE_CHECK_GET_CLASS ((obj), GNOME_TYPE_ICON_THEME, GnomeIconThemeClass))

typedef GtkIconTheme GnomeIconTheme;
typedef struct _GnomeIconThemeClass GnomeIconThemeClass;

struct _GnomeIconThemeClass
{
  GObjectClass parent_class;

  void (* changed)  (GnomeIconTheme *icon_theme);
};

typedef struct
{
  int x;
  int y;
} GnomeIconDataPoint;

typedef struct
{
  gboolean has_embedded_rect;
  int x0, y0, x1, y1;
  
  GnomeIconDataPoint *attach_points;
  int n_attach_points;

  char *display_name;
} GnomeIconData;

GType            gnome_icon_theme_get_type              (void) G_GNUC_CONST;

GnomeIconTheme *gnome_icon_theme_new                   (void);
void            gnome_icon_theme_set_search_path       (GnomeIconTheme       *theme,
							const char           *path[],
							int                   n_elements);
void            gnome_icon_theme_get_search_path       (GnomeIconTheme       *theme,
							char                **path[],
							int                  *n_elements);
void            gnome_icon_theme_set_allow_svg         (GnomeIconTheme       *theme,
							gboolean              allow_svg);
gboolean        gnome_icon_theme_get_allow_svg         (GnomeIconTheme       *theme);
void            gnome_icon_theme_append_search_path    (GnomeIconTheme       *theme,
							const char           *path);
void            gnome_icon_theme_prepend_search_path   (GnomeIconTheme       *theme,
							const char           *path);
void            gnome_icon_theme_set_custom_theme      (GnomeIconTheme       *theme,
							const char           *theme_name);
char *          gnome_icon_theme_lookup_icon           (GnomeIconTheme       *theme,
							const char           *icon_name,
							int                   size,
							const GnomeIconData **icon_data,
							int                  *base_size);
gboolean        gnome_icon_theme_has_icon              (GnomeIconTheme       *theme,
							const char           *icon_name);
GList *         gnome_icon_theme_list_icons            (GnomeIconTheme       *theme,
							const char           *context);
char *          gnome_icon_theme_get_example_icon_name (GnomeIconTheme       *theme);
gboolean        gnome_icon_theme_rescan_if_needed      (GnomeIconTheme       *theme);

GnomeIconData * gnome_icon_data_dup                    (const GnomeIconData  *icon_data);
void            gnome_icon_data_free                   (GnomeIconData        *icon_data);

GtkIconTheme   *_gnome_icon_theme_get_gtk_icon_theme    (GnomeIconTheme       *icon_theme);

G_END_DECLS

#endif  /* GNOME_DISABLE_DEPRECATED */


#endif /* GNOME_ICON_THEME_H */
