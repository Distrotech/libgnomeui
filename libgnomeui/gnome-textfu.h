/* gnome-textfu.h
 * Copyright (C) 2000 Red Hat, Inc.
 * All rights reserved.
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
*/
#ifndef __GNOME_TEXTFU_H__
#define __GNOME_TEXTFU_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define GNOME_TYPE_TEXTFU			(gnome_textfu_get_type ())
#define GNOME_TEXTFU(obj)			(GTK_CHECK_CAST ((obj), GNOME_TYPE_TEXTFU, GnomeTextFu))
#define GNOME_TEXTFU_CLASS(klass)		(GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_TEXTFU, GnomeTextFuClass))
#define GNOME_IS_TEXTFU(obj)			(GTK_CHECK_TYPE ((obj), GNOME_TYPE_TEXTFU))
#define GNOME_IS_TEXTFU_CLASS(klass)		(GTK_CHECK_CLASS_TYPE ((obj), GNOME_TYPE_TEXTFU))
#define GNOME_TEXTFU_GET_CLASS(obj)             (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_TEXTFU, GnomeTextFuClass))


typedef enum {
  TEXTFU_ITEM_TEXT,
  TEXTFU_ITEM_CONTAINER,
  TEXTFU_ITEM_WIDGET
} GnomeTextFuItemType;

typedef struct _GnomeTextFuItem GnomeTextFuItem;

struct _GnomeTextFuItem {
  GnomeTextFuItem *up;
  GnomeTextFuItemType type;
  guint16 x, y;
};

typedef struct {
  GnomeTextFuItem item;

  gpointer link_region;

  char text[1];
} GnomeTextFuItemText;

typedef enum {
  TEXTFU_FALSE,
  TEXTFU_TRUE,
  TEXTFU_UNKNOWN
} GnomeTextFuTruthValue;

typedef struct {
  GnomeTextFuItem item;

  GSList *children;

  /* For use to modify various specific subitems that we might encounter */

  char *link_to, *font_name;

  GdkFont *font; /* internal */

  gint8 subitems_font_size, subitems_left_indent, subitems_right_indent;
  
  GnomeTextFuTruthValue subitems_newpara : 2;
  GnomeTextFuTruthValue subitems_italic : 2;
  GnomeTextFuTruthValue subitems_bold : 2;
  GnomeTextFuTruthValue subitems_bullet : 2;

  gboolean subitems_ignore : 1;

  gboolean inherited_font : 1; /* internal */
} GnomeTextFuItemContainer;

typedef struct {
  GnomeTextFuItem item;

  guint16 last_xpos, last_ypos;
  GtkWidget *widget;
} GnomeTextFuItemWidget;

typedef struct _GnomeTextFu       GnomeTextFu;
typedef struct _GnomeTextFuClass  GnomeTextFuClass;

struct _GnomeTextFu
{
  GtkLayout parent;

  /* All fields are private, don't touch */
  char *cur_filename;

  guint16 width;

  GnomeTextFuItem *item_tree;
  GList *link_regions;

  gboolean linkpoint_cursor;
};

struct _GnomeTextFuClass
{
  GtkLayoutClass parent_class;

  GHashTable *tag_handlers;

  /* Signals go here */
  void (*activate_uri)	(GnomeTextFu *textfu, const char *uri);
};

typedef GnomeTextFuItem * (*GnomeTextFuTagHandler)(GnomeTextFu *textfu, const char *tag, guint16 tag_id, char **attrs);

GtkType    gnome_textfu_get_type   (void) G_GNUC_CONST;
GtkWidget *gnome_textfu_new        (void);
void       gnome_textfu_load_file  (GnomeTextFu *textfu, const char *filename);
guint16    gnome_textfu_tagid_alloc(const char *name, GnomeTextFuTagHandler handler);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GNOME_TEXTFU_H__ */
