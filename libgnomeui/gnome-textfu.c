/* gnome-textfu.c
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
/* Written by Elliot Lee <sopwith@redhat.com>. This code is pretty aweful, but so is all my other code. */

#include <config.h>
#include "gnome-macros.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include <libgnome/gnome-util.h>
#include "gnome-cursors.h"
#include "gnome-helpsys.h"
#include "gnome-pixmap.h"

#include "gnome-textfu.h"

#define my_isspace(x) (isspace(x) || (x) == '\r' || (x) == '\n')
#define LINE_SPACING 4

typedef struct {
  GnomeTextFuTagHandler handler;
  guint16 tag_id;

  char tag_name[1];
} TagRegistration;

typedef struct {
  GdkRegion *region;
  char *link_to;
} TextRegionInfo;

enum {
  ACTIVATE_URI,
  LAST_SIGNAL
};

static void gnome_textfu_init		(GnomeTextFu		 *textfu);
static void gnome_textfu_class_init	(GnomeTextFuClass	 *klass);
static void gnome_real_textfu_activate_uri (GnomeTextFu		 *textfu, const char *uri);
static void gnome_textfu_realize            (GtkWidget      *widget);
static void gnome_textfu_unrealize          (GtkWidget      *widget);
static void gnome_textfu_map                (GtkWidget      *widget);
static void gnome_textfu_unrealize          (GtkWidget      *widget);
static void gnome_textfu_size_request       (GtkWidget      *widget,
					     GtkRequisition *requisition);
static void gnome_textfu_size_allocate      (GtkWidget      *widget,
					     GtkAllocation  *allocation);
static gint gnome_textfu_expose             (GtkWidget      *widget, 
					     GdkEventExpose *event);
static gint gnome_textfu_button_release_event(GtkWidget *widget, GdkEventButton *event);
static gint gnome_textfu_motion_notify_event(GtkWidget *widget, GdkEventMotion *event);

static guint textfu_signals[LAST_SIGNAL] = { 0 };

/* Define boilerplate such as parent_class and _get_type */
GNOME_CLASS_BOILERPLATE (GnomeTextFu, gnome_textfu,
			 GtkLayout, gtk_layout)

static GnomeTextFuItemContainer *
gnome_textfu_item_container_new(void)
{
  GnomeTextFuItemContainer * retval;

  retval = g_new0(GnomeTextFuItemContainer, 1);

  retval->item.type = TEXTFU_ITEM_CONTAINER;

  retval->subitems_newpara = TEXTFU_FALSE;
  retval->subitems_italic = TEXTFU_FALSE;
  retval->subitems_bold = TEXTFU_FALSE;
  retval->subitems_bullet = TEXTFU_FALSE;

  return retval;
}

static GnomeTextFuItemWidget *
gnome_textfu_item_widget_new(GtkWidget *widget)
{
  GnomeTextFuItemWidget * retval;

  retval = g_new0(GnomeTextFuItemWidget, 1);

  retval->item.type = TEXTFU_ITEM_WIDGET;
  retval->widget = widget;

  return retval;
}

static char *
gnome_textfu_resolve_filename(GnomeTextFu *textfu, char *in_filename, const char *type)
{
  int slen;

  if (in_filename[0] == '"')
    in_filename++;
  slen = strlen(in_filename) - 1;
  if (in_filename[slen] == '"')
    in_filename[slen] = '\0';

  if (g_file_test (in_filename, G_FILE_TEST_EXISTS))
    return g_strdup (in_filename);
  else if(in_filename[0] == '/')
    return gnome_help_path_resolve(in_filename, type);
  else
    {
      char *dirname_ret;
      char *retval;
      dirname_ret = g_path_get_dirname (textfu->cur_filename);
      retval = g_concat_dir_and_file (dirname_ret, in_filename);
      g_free (dirname_ret);
      return retval;
    }
}

#define IMPL_CTAG(x) static GnomeTextFuItem *handler_##x(GnomeTextFu *textfu, const char *tag, guint16 tag_id, char **attrs) \
{ \
GnomeTextFuItemContainer *retval; \
retval = gnome_textfu_item_container_new(); \
retval->subitems_newpara = TEXTFU_FALSE

#define END_CTAG return (GnomeTextFuItem *)retval; }

#define DUMMY_TAG(x) IMPL_CTAG(x); END_CTAG

IMPL_CTAG(popup);
retval->subitems_newpara = TEXTFU_TRUE;
END_CTAG


IMPL_CTAG(sect1);
retval->subitems_newpara = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(title);
retval->subitems_bold = TEXTFU_TRUE;
retval->subitems_font_size = 16;
END_CTAG


IMPL_CTAG(sect2);
retval->subitems_left_indent = 10;
retval->subitems_right_indent = 10;
END_CTAG


IMPL_CTAG(para);
END_CTAG

IMPL_CTAG(ulink);
{
  int i;
  for(i = 0; attrs[i]; i++)
    {
      if(!g_strncasecmp(attrs[i], "url=", strlen("url=")))
	{
	  char *link_ptr;
	  int slen;

	  link_ptr = attrs[i] + strlen("url=");
	  if(*link_ptr == '"')
	    link_ptr++;
	  slen = strlen(link_ptr) - 1;
	  if(link_ptr[slen] == '"')
	    link_ptr[slen] = '\0';

	  retval->link_to = g_strdup(link_ptr);
	}
    }
}
END_CTAG


IMPL_CTAG(itemizedlist);
retval->subitems_bullet = TEXTFU_TRUE;
retval->subitems_newpara = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(listitem);
END_CTAG

IMPL_CTAG(application);
retval->font_name = "fixed";
END_CTAG

IMPL_CTAG(guibutton);
retval->subitems_bold = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(guiicon);
retval->subitems_bold = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(guilabel);
retval->font_name = "fixed";
retval->subitems_bold = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(guimenu);
retval->subitems_bold = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(guimenuitem);
retval->subitems_bold = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(guisubmenu);
retval->subitems_bold = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(keycap);
retval->subitems_bold = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(keycombo);
retval->subitems_bold = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(mousebutton);
retval->subitems_italic = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(citetitle);
retval->subitems_italic = TEXTFU_TRUE;
END_CTAG

IMPL_CTAG(authorgroup);
retval->subitems_ignore = TRUE;
END_CTAG

DUMMY_TAG(author)
DUMMY_TAG(honorific)
DUMMY_TAG(firstname)
DUMMY_TAG(surname)
DUMMY_TAG(othername)
DUMMY_TAG(affiliation)
DUMMY_TAG(orgname)
DUMMY_TAG(jobtitle)
DUMMY_TAG(email)
DUMMY_TAG(copyright)
DUMMY_TAG(revhistory)
DUMMY_TAG(revision)
DUMMY_TAG(revnumber)
DUMMY_TAG(date)

static GnomeTextFuItem *
handler_img(GnomeTextFu *textfu, const char *tag, guint16 tag_id, char **attrs)
{
  GtkWidget *pmap = NULL;
  int i;

  for(i = 0; !pmap && attrs[i]; i++)
    {
      if(!strncmp(attrs[i], "src=", 4))
	{
	  char *filename;
	  filename = gnome_textfu_resolve_filename(textfu, attrs[i] + 4, "image");

	  if(filename)
	    {
	      pmap = gnome_pixmap_new_from_file(filename);
	      gtk_widget_show(pmap);
	      g_free(filename);
	    }
	  else
	    {
	      g_warning("Couldn't find %s for an image", attrs[i]+4);
	    }
	}
    }

  if(!pmap)
    return NULL;

  return (GnomeTextFuItem *)gnome_textfu_item_widget_new(pmap);
}

static void
gnome_textfu_class_init (GnomeTextFuClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) klass;

  widget_class = (GtkWidgetClass*) klass;

  textfu_signals[ACTIVATE_URI] = 
    gtk_signal_new ("activate_uri",
                    GTK_RUN_FIRST,
                    GTK_CLASS_TYPE (object_class),
                    GTK_SIGNAL_OFFSET (GnomeTextFuClass, activate_uri),
                    gtk_marshal_VOID__STRING,
                    GTK_TYPE_NONE, 1, GTK_TYPE_STRING);


  klass->activate_uri = gnome_real_textfu_activate_uri;
  klass->tag_handlers = g_hash_table_new(g_str_hash, g_str_equal);

#define HA(x) gnome_textfu_tagid_alloc(#x, handler_##x)
  HA(img);
  HA(popup);
  HA(sect1);
  HA(title);
  HA(sect2);
  HA(para);
  HA(ulink);
  HA(itemizedlist);
  HA(listitem);
  HA(application);
  HA(guibutton);
  HA(guiicon);
  HA(guilabel);
  HA(guimenu);
  HA(guimenuitem);
  HA(guisubmenu);
  HA(keycap);
  HA(keycombo);
  HA(mousebutton);
  HA(citetitle);
  HA(authorgroup);
  HA(author);
  HA(honorific);
  HA(firstname);
  HA(surname);
  HA(othername);
  HA(affiliation);
  HA(orgname);
  HA(jobtitle);
  HA(email);
  HA(copyright);
  HA(revhistory);
  HA(revnumber);
  HA(revision);
  HA(date);

  widget_class->realize = gnome_textfu_realize;
  widget_class->unrealize = gnome_textfu_unrealize;
  widget_class->size_request = gnome_textfu_size_request;
  widget_class->size_allocate = gnome_textfu_size_allocate;
  widget_class->expose_event = gnome_textfu_expose;
  widget_class->map = gnome_textfu_map;
  widget_class->motion_notify_event = gnome_textfu_motion_notify_event;
  widget_class->button_release_event = gnome_textfu_button_release_event;
}

static void
gnome_textfu_init (GnomeTextFu *textfu)
{
  gtk_widget_add_events(GTK_WIDGET(textfu),
			GDK_EXPOSURE_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK
			|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_MOTION_MASK);
}

GtkWidget *
gnome_textfu_new(void)
{
  GtkWidget *retval;

  retval = gtk_widget_new(gnome_textfu_get_type(), NULL);
  gtk_layout_set_hadjustment(GTK_LAYOUT(retval), NULL);
  gtk_layout_set_vadjustment(GTK_LAYOUT(retval), NULL);

  return retval;
}

static void
gnome_real_textfu_activate_uri (GnomeTextFu *textfu, const char *uri)
{
  g_return_if_fail (textfu != NULL);
  g_return_if_fail (GNOME_IS_TEXTFU (textfu));
  
}

static void
gnome_textfu_map(GtkWidget *widget)
{
	GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS, map, (widget));

	gtk_widget_queue_draw(widget);
}

static void
gnome_textfu_realize(GtkWidget      *widget)
{
	GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS, realize, (widget));
}

static void
gnome_textfu_unrealize(GtkWidget      *widget)
{
	GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS, unrealize, (widget));
}

static TextRegionInfo *
find_text_region_for_window_coords(GnomeTextFu *textfu, gint x, gint y)
{
  GList *cur;

  x += GTK_LAYOUT(textfu)->xoffset;
  y += GTK_LAYOUT(textfu)->yoffset;

  for(cur = textfu->link_regions; cur; cur = cur->next)
    {
      TextRegionInfo *info;

      info = cur->data;

      if(gdk_region_point_in(info->region, x, y))
	return info;
    }

  return NULL;
}

static gint
gnome_textfu_button_release_event(GtkWidget *widget, GdkEventButton *event)
{
  GnomeTextFu *textfu = GNOME_TEXTFU(widget);
  TextRegionInfo *info = NULL;

  if(event->window != GTK_LAYOUT(widget)->bin_window)
    goto default_handling;

  info = find_text_region_for_window_coords(textfu, event->x, event->y);
  if(!info)
    goto default_handling;

  if(event->type == GDK_BUTTON_RELEASE)
    gtk_signal_emit(GTK_OBJECT(widget), textfu_signals[ACTIVATE_URI], info->link_to);

 default_handling:
  return GNOME_CALL_PARENT_HANDLER_WITH_DEFAULT (GTK_WIDGET_CLASS,
						 button_release_event,
						 (widget, event),
						 (info ? TRUE : FALSE));
}

static gint
gnome_textfu_motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
  static GdkCursor *cursor_hand = NULL;
  GnomeTextFu *textfu = GNOME_TEXTFU(widget);
  TextRegionInfo *info;

  if(event->window != GTK_LAYOUT(widget)->bin_window)
    goto default_handling;

  info = find_text_region_for_window_coords(textfu, event->x, event->y);

  if((info && TRUE) != textfu->linkpoint_cursor)
    {
      GdkCursor *set_new_cursor;

      textfu->linkpoint_cursor = info && TRUE;

      if(info)
	{
	  if(!cursor_hand)
	    cursor_hand = gnome_stock_cursor_new(GNOME_STOCK_CURSOR_POINTING_HAND);
	  set_new_cursor = cursor_hand;
	}
      else
	set_new_cursor = NULL;

      gdk_window_set_cursor(GTK_LAYOUT(widget)->bin_window, set_new_cursor);
    }

 default_handling:
  return GNOME_CALL_PARENT_HANDLER_WITH_DEFAULT (GTK_WIDGET_CLASS,
						 motion_notify_event,
						 (widget, event),
						 FALSE);
}

/* This data structure is really used for drawing just as much as sizing */
typedef struct {
  int xpos, ypos, line_height;
  gboolean did_newpara;

  gboolean drawing : 1;
  gboolean reposition : 1;
} SizingGlobalState;

typedef struct {
  SizingGlobalState global;

  gint ascent, descent;

  /* For use in figuring out the text layout */
  gint8 left_indent, right_indent;
  gint8 font_size;

  gboolean italic : 1;
  gboolean bold : 1;
  gboolean bullet : 1;
  gboolean newpara : 1;

  char *link_to, *font_name;
} SizingState;

static void gnome_textfu_determine_size(GnomeTextFu *textfu, GtkRequisition *requisition, gboolean do_drawing);
static void gnome_textfu_item_determine_size(GnomeTextFu *textfu, GnomeTextFuItem *item, SizingState *ss);

static gboolean /* Returns true if coords are at least partially visible */
gnome_textfu_draw_translate(GnomeTextFu *textfu, SizingState *ss, gint *x, gint *y, gint width, gint height)
{
  GtkLayout *layout = GTK_LAYOUT(textfu);

  *x -= layout->xoffset;
  *y -= layout->yoffset;
  return ((*x + width) >= 0
	  || (*y + height) >= 0
	  || (*x < layout->width)
	  || (*y < layout->height));
}

static void
gnome_textfu_new_paragraph(GnomeTextFu *textfu, SizingState *ss)
{
  if(ss->global.did_newpara || !ss->newpara)
    return;

  ss->global.xpos = ss->left_indent;
  ss->global.ypos += ss->global.line_height;

  ss->global.ypos += 10; /* Inter-paragraph spacing */

  ss->global.line_height = 0;
  ss->global.did_newpara = TRUE;

  if(ss->global.drawing && ss->bullet) /* Draw bullet for the new paragraph, if we're supposed to */
    {
      gint x, y, width=10, height=10;

      x = ss->global.xpos;
      y = ss->global.ypos;

      if(gnome_textfu_draw_translate(textfu, ss, &x, &y, width, height))
	{
	  GdkGC *gc;
	  gc = gdk_gc_new(GTK_LAYOUT(textfu)->bin_window);
	  gdk_draw_arc(GTK_LAYOUT(textfu)->bin_window, gc, TRUE,
		       x, y, width, height, 0, 360);
	  gdk_gc_unref(gc);
	}
    }
}

static GdkFont *
gnome_textfu_get_font(GnomeTextFu *textfu, GnomeTextFuItem *item, SizingState *ss)
{
  char fontname[512];
  GdkFont *retval;
  const char *foundry = "*",
    *family = "helvetica",
    *weight = "medium",
    *slant = "r",
    *set_width = "normal",
    *charset = "*",
    *pixel_size = "*",
    *spacing = "*"
    ;
  int point_size = 120;

  GnomeTextFuItemContainer *citem = (GnomeTextFuItemContainer *)item;

  while(citem && (citem->item.type != TEXTFU_ITEM_CONTAINER || citem->inherited_font))
    citem = (GnomeTextFuItemContainer *)citem->item.up;

  g_assert(citem);

  if(!citem->font)
    {
      if(ss->font_name)
	family = ss->font_name;
      if(ss->italic)
	slant = "o";
      if(ss->bold)
	weight = "bold";
      point_size = ss->font_size * 10;

      g_snprintf(fontname, sizeof(fontname),"-%s-%s-%s-%s-%s-*-%s-%d-*-*-%s-*-%s",
		 foundry, family, weight, slant, set_width,
		 pixel_size, point_size, spacing, charset);

      citem->font = retval = gdk_fontset_load(fontname);
    }

  /* We need a constant ascent/descent for all usages of this font */
  gdk_string_extents(citem->font, "TLyg", NULL, NULL, NULL, &ss->ascent, &ss->descent);

  return citem->font;
}

static void
gnome_textfu_container_determine_size(GnomeTextFu *textfu, GnomeTextFuItemContainer *citem,
				      SizingState *ss)
{
  GSList *cur;

  citem->item.x = ss->global.xpos;
  citem->item.y = ss->global.ypos;

  if(citem->subitems_ignore)
    return;

  if(citem->subitems_left_indent > 0)
    ss->left_indent += citem->subitems_left_indent;
  if(citem->subitems_right_indent > 0)
    ss->right_indent += citem->subitems_right_indent;

  if(!citem->inherited_font)
    {
      if(citem->subitems_bold != TEXTFU_UNKNOWN)
	ss->bold = citem->subitems_bold;
      if(citem->subitems_italic != TEXTFU_UNKNOWN)
	ss->italic = citem->subitems_italic;
      if(citem->subitems_font_size > 0)
	ss->font_size = citem->subitems_font_size;
      if(citem->font_name)
	ss->font_name = citem->font_name;
      if(citem->link_to)
	ss->link_to = citem->link_to;
    }

  if(citem->subitems_newpara != TEXTFU_UNKNOWN)
    ss->newpara = citem->subitems_newpara;
  if(citem->subitems_bullet != TEXTFU_UNKNOWN)
    ss->bullet = citem->subitems_bullet;

  for(cur = citem->children; cur; cur = cur->next)
    {
      GnomeTextFuItem *subitem = cur->data;
      SizingState sub_state;

      sub_state = *ss;

      gnome_textfu_new_paragraph(textfu, &sub_state);

      gnome_textfu_item_determine_size(textfu, subitem, &sub_state);

      ss->global = sub_state.global;
      ss->global.did_newpara = FALSE;
    }
}

static void
gnome_textfu_text_free_region_info(TextRegionInfo *info, GnomeTextFu *textfu)
{
  gdk_region_destroy(info->region);
  g_free(info);
}

static void
gnome_textfu_text_free_region(GnomeTextFu *textfu, TextRegionInfo *info)
{
  textfu->link_regions = g_list_remove(textfu->link_regions, info);

  gnome_textfu_text_free_region_info(info, textfu);
}

static void
gnome_textfu_text_free_regions(GnomeTextFu *textfu)
{
  g_list_foreach(textfu->link_regions, (GFunc)gnome_textfu_text_free_region_info, textfu);
  g_list_free(textfu->link_regions);
  textfu->link_regions = NULL;
}

static void
gnome_textfu_text_set_region(GnomeTextFu *textfu, GnomeTextFuItemText *item, char *link_to, GdkRegion *region)
{
  TextRegionInfo *info = NULL;

  if(item->link_region != NULL)
    gnome_textfu_text_free_region(textfu, item->link_region);

  if(link_to)
    {
      info = g_new(TextRegionInfo, 1);

      info->region = region;
      info->link_to = link_to;
      textfu->link_regions = g_list_prepend(textfu->link_regions, info);
    }

  item->link_region = info;
}

#define REGION_ADD_RECT(rx, ry, rwidth, rheight)		\
	{							\
		GdkRectangle rect;				\
		rect.x = rx;					\
		rect.y = ry;					\
		rect.width = rwidth;				\
		rect.height = rheight;				\
		gdk_region_union_with_rect(link_region, &rect);	\
	}

static void
gnome_textfu_text_determine_size(GnomeTextFu *textfu, GnomeTextFuItemText *item, SizingState *ss)
{
  GdkFont *font;
  gint lbearing, rbearing, width, ascent, descent;
  gint space_left;
  char *remaining_text;
  GdkGC *gc = NULL;
  double avg_width;
  GdkDrawable *outwin;
  gint remaining_len;
  GdkGCValues vals;
  GdkGCValuesMask mask = 0;
  GdkRegion *link_region = NULL;

  outwin = GTK_LAYOUT(textfu)->bin_window;
  if(ss->global.drawing)
    {
      if(ss->link_to)
	{
	  mask |= GDK_GC_FOREGROUND;
	  vals.foreground.red = 20 << 8;
	  vals.foreground.green = 20 << 8;
	  vals.foreground.blue = 65535;

	  gdk_colormap_alloc_color(gdk_rgb_get_cmap(), &vals.foreground, TRUE, TRUE);

	  link_region = gdk_region_new();
	}

      gc = gdk_gc_new_with_values(outwin, &vals, mask);
    }

  font = gnome_textfu_get_font(textfu, (GnomeTextFuItem *)item, ss);

  g_return_if_fail(font);

  remaining_text = item->text;

  if(ss->newpara)
    while(*remaining_text && my_isspace(*remaining_text)) remaining_text++;

  if(!*remaining_text)
    return; /* Nothing to draw */
    
  remaining_len = strlen(remaining_text);
  gdk_string_extents(font, remaining_text, &lbearing, &rbearing, &width, NULL, NULL);
  ascent = ss->ascent;
  descent = ss->descent;

  avg_width = width;
  avg_width /= strlen(remaining_text);
  ss->global.line_height = MAX(ss->global.line_height, ascent + descent + LINE_SPACING);
  space_left = (textfu->width - ss->right_indent) - ss->global.xpos;

  while((width > space_left) && (remaining_len > 0))
    {
      int chunklen;

      space_left = (textfu->width - ss->right_indent) - ss->global.xpos;
      chunklen = (int) (((double)space_left)/avg_width);
      while(!my_isspace(remaining_text[chunklen]) && chunklen > 0) chunklen--;
      if(chunklen <= 0) /* try over-shooting instead */
	{
	  chunklen = space_left/avg_width; 
	  while(!my_isspace(remaining_text[chunklen]) && (chunklen < remaining_len)) chunklen++;
	}
      if(chunklen <= 0)
	chunklen = space_left/avg_width;
      if(chunklen <= 0)
	goto out; /* It's hopeless */

      if(ss->global.drawing)
	{
	  gint x, y, width, height;

	  height = ascent + descent + LINE_SPACING;
	  width = gdk_text_width(font, remaining_text, chunklen);
	  x = ss->global.xpos;
	  y = ss->global.ypos + ss->global.line_height - height;

	  if(gnome_textfu_draw_translate(textfu, ss, &x, &y, width, height))
	    {
	      y += height;
	      gdk_draw_text(outwin, font, gc, x, y, remaining_text, chunklen);

	      if(ss->link_to)
		{
		  gdk_draw_line(outwin, gc, x, y + 1, x + width, y + 1);

		  x = ss->global.xpos;
		  y = ss->global.ypos + ss->global.line_height - height;
		  REGION_ADD_RECT(x, y, width, height+1);
		}
	    }
	}
      remaining_text += chunklen;
      remaining_len -= chunklen;
      while(*remaining_text && my_isspace(*remaining_text))
	{
	  remaining_text++;
	  remaining_len--;
	}

      gdk_string_extents(font, remaining_text, &lbearing, &rbearing, &width, NULL, NULL);
      space_left = (textfu->width - ss->right_indent) - ss->global.xpos;
      ss->global.xpos = ss->left_indent;
      ss->global.ypos += ss->global.line_height;
      ss->global.line_height = ascent + descent + LINE_SPACING;
    }

  {
      gint x, y, width, height;

      height = ascent + descent + LINE_SPACING;
      width = gdk_string_width(font, remaining_text);

      x = ss->global.xpos;
      y = ss->global.ypos + ss->global.line_height - height;

      if(ss->global.drawing)
	{
	  if(gnome_textfu_draw_translate(textfu, ss, &x, &y, width, height))
	    {
	      y += height;
	      gdk_draw_string(outwin, font, gc, x, y, remaining_text);

	      if(ss->link_to)
		{
		  gdk_draw_line(outwin, gc, x, y + 1, x + width, y + 1);

		  x = ss->global.xpos;
		  y = ss->global.ypos + ss->global.line_height - height;
		  REGION_ADD_RECT(x, y, width, height+1);
		}
	    }
	}

      ss->global.xpos += width;

    }

 out:
  if(ss->global.drawing)
    {
      gdk_gc_unref(gc);

      if(ss->link_to)
	{
	  gdk_colormap_free_colors(gdk_rgb_get_cmap(), &vals.foreground, 1);
	  gnome_textfu_text_set_region(textfu, item, ss->link_to, link_region);
	}
    }
}

static void
gnome_textfu_widget_determine_size(GnomeTextFu *textfu, GnomeTextFuItemWidget *item, SizingState *ss)
{
  GnomeTextFuItemWidget *eitem = item;
  gint last_xpos, last_ypos;

  if((ss->global.xpos + eitem->widget->requisition.width) > textfu->width)
    {
      ss->global.xpos = ss->left_indent;
      ss->global.ypos += ss->global.line_height;
    }

  eitem->item.x = ss->global.xpos;
  eitem->item.y = ss->global.ypos;

  if(ss->global.reposition && !ss->global.drawing)
    {
      last_xpos = eitem->last_xpos;
      last_ypos = eitem->last_ypos;
      if(last_xpos != ss->global.xpos || last_ypos != ss->global.ypos)
	{
	  gtk_layout_move(GTK_LAYOUT(textfu), eitem->widget, ss->global.xpos, ss->global.ypos);

	  eitem->last_xpos = ss->global.xpos;
	  eitem->last_ypos = ss->global.ypos;
	}
    }
  ss->global.xpos += eitem->widget->requisition.width;
  ss->global.line_height = MAX(ss->global.line_height, eitem->widget->requisition.height);
  ss->global.did_newpara = FALSE;
}

static void
gnome_textfu_item_determine_size(GnomeTextFu *textfu, GnomeTextFuItem *item, SizingState *ss)
{
  switch(item->type)
    {
    case TEXTFU_ITEM_CONTAINER:
      gnome_textfu_container_determine_size(textfu, (GnomeTextFuItemContainer *)item, ss);
      break;
    case TEXTFU_ITEM_TEXT:
      gnome_textfu_text_determine_size(textfu, (GnomeTextFuItemText *)item, ss);
      break;
    case TEXTFU_ITEM_WIDGET:
      gnome_textfu_widget_determine_size(textfu, (GnomeTextFuItemWidget *)item, ss);
      break;
    default:
      g_assert_not_reached();
      break;
    }
}

static void
gnome_textfu_determine_size(GnomeTextFu *textfu, GtkRequisition *requisition, gboolean do_drawing)
{
  SizingState ss;
  gint real_textfu_width;

  memset(&ss, 0, sizeof(ss));

  ss.global.drawing = do_drawing;

  /* Top margin */
  ss.global.ypos = 10;

  real_textfu_width = textfu->width;

  if(requisition)
    {
      textfu->width = gdk_screen_width()/5;
    }
  ss.global.reposition = requisition?FALSE:TRUE;

  gnome_textfu_item_determine_size(textfu, textfu->item_tree, &ss);

  /* Bottom margin */
  ss.global.ypos += 10;

  if(requisition)
    {
      requisition->width = textfu->width;
      requisition->height = ss.global.ypos + ss.global.line_height;
    }

  textfu->width = real_textfu_width;
}

static void
gnome_textfu_size_request(GtkWidget      *widget,
			  GtkRequisition *requisition)
{
  GnomeTextFu *textfu;

  textfu = GNOME_TEXTFU(widget);

  GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS,
			     size_request,
			     (widget, requisition));

  if(textfu->item_tree)
    {
      GtkRequisition my_requisition;

      gnome_textfu_determine_size(textfu, &my_requisition, FALSE);

      requisition->width = MAX(requisition->width, my_requisition.width);
      requisition->height = MAX(requisition->height, my_requisition.height);
    }
}

static void
gnome_textfu_size_allocate(GtkWidget      *widget,
			   GtkAllocation  *allocation)
{
  GnomeTextFu *textfu;

  textfu = GNOME_TEXTFU(widget);

  GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS,
			     size_allocate,
			     (widget, allocation));

  if(textfu->width != allocation->width)
    {
      textfu->width = allocation->width;

      gnome_textfu_determine_size(textfu, NULL, FALSE);

      gtk_widget_queue_draw(widget);
    }
}

static gint
gnome_textfu_expose(GtkWidget      *widget, 
		    GdkEventExpose *event)
{
  GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS,
			     expose_event,
			     (widget, event));

  gnome_textfu_determine_size(GNOME_TEXTFU(widget), NULL, TRUE);

  return TRUE;
}

static void
parse_tag(char *tag_start, char *tag_end, char *tag_name, char **tag_attrs)
{
  char *ctmp, *attr_start;
  int i;

  ctmp = tag_start;
  while(*ctmp && !my_isspace(*ctmp)) ctmp++;
  if(my_isspace(*ctmp))
    {
      *ctmp = '\0';
      ctmp++;
    }

  strcpy(tag_name, tag_start);
  g_strdown(tag_name);

  for(i = 0; *ctmp && (i < 32); i++)
    {
      attr_start = ctmp;
      while(*ctmp && !my_isspace(*ctmp)) ctmp++;

      tag_attrs[i] = attr_start;
      if(*ctmp)
	{
	  *ctmp = '\0';
	  ctmp++;
	}
    }
  tag_attrs[i] = NULL;
}

static void
handle_tag(GnomeTextFu *textfu, GHashTable *tag_handlers,
	   GnomeTextFuItem **stack, int *stack_cur,
	   const char *tag_name, char **tag_attrs)
{
  TagRegistration *tr;
  GnomeTextFuItem *new_item;

  if(tag_name[0] == '/')
    {
      if(*stack_cur > 0)
	(*stack_cur)--;
      else
	g_warning("Too many close tags %s", tag_name);
      return;
    }

  tr = g_hash_table_lookup(tag_handlers, tag_name);
  if(!tr)
    {
      g_warning("No handler for a %s tag", tag_name);
      return;
    }

  new_item = tr->handler(textfu, tag_name, tr->tag_id, tag_attrs);

  if(new_item)
    {
      GnomeTextFuItemContainer *container;
      container = (GnomeTextFuItemContainer *)stack[*stack_cur];
      container->children = g_slist_append(container->children, new_item);
      new_item->up = (GnomeTextFuItem *)container;

      switch(new_item->type)
	{
	case TEXTFU_ITEM_WIDGET:
	  gtk_layout_put(GTK_LAYOUT(textfu), ((GnomeTextFuItemWidget *)new_item)->widget, 0, 0);
	  break;
	case TEXTFU_ITEM_CONTAINER:
	  {
	    container = (GnomeTextFuItemContainer *)new_item;
	    if(container->subitems_italic == TEXTFU_UNKNOWN
	       && container->subitems_bold == TEXTFU_UNKNOWN
	       && container->subitems_font_size <= 0
	       && !container->font_name
	       && !container->link_to)
	      container->inherited_font = TRUE;

	    stack[++(*stack_cur)] = new_item;
	  }
	  break;
	default:
	  break;
	}

    }
}

static gpointer
handle_text(GnomeTextFu *textfu, GnomeTextFuItem **stack, int stack_cur,
	    char *text_start, char *text_end)
{
  GnomeTextFuItemContainer *container;
  GnomeTextFuItemText *new_item;
  char *ctmp, *space_start;

  /* lame hack for getting rid of extra whitespace, oh well ;-) */

  for(space_start = NULL, ctmp = text_start; ctmp < text_end; ctmp++)
    {
      if(my_isspace(*ctmp))
	{
	  if(!space_start)
	    space_start = ctmp;
	}
      else if(space_start)
	{
	  int nmove = (ctmp - space_start) - 1;

	  if(nmove > 0)
	    {
	      memmove(space_start + 1, ctmp, text_end - ctmp);
	      text_end -= nmove;
	      ctmp -= nmove;
	    }

	  space_start = NULL;
	}
    }
  if(space_start)
    {
      int nmove = (ctmp - space_start) - 1;

      if(nmove > 0)
	{
	  memmove(space_start + 1, ctmp, text_end - ctmp);
	  text_end -= nmove;
	  ctmp -= nmove;
	}
    }

  if((text_end <= text_start) || !*text_start)
    return NULL;

  for(space_start = text_start; *space_start && my_isspace(*space_start); space_start++) /**/ ;

  if(! *space_start) /* weed out text things that are just a newline */
    return NULL;

  new_item = g_malloc0(G_STRUCT_OFFSET(GnomeTextFuItemText, text) + (text_end - text_start) + 1);

  new_item->item.type = TEXTFU_ITEM_TEXT;
  strncpy(new_item->text, text_start, text_end - text_start);
  new_item->text[text_end - text_start] = '\0';

  container = (GnomeTextFuItemContainer *)stack[stack_cur];
  container->children = g_slist_append(container->children, new_item);
  new_item->item.up = (GnomeTextFuItem *)container;

  return NULL;
}

static GnomeTextFuItem *
gnome_textfu_parse(GnomeTextFu *textfu)
{
  int fd;
  int bracecount, quotecount, in_comment;
  struct stat sbuf;
  char *mem = NULL, *tag_start = NULL, *tag_end, *text_start;
  int size_left, n = 0;
  char tag_name[32];
  char *attrs[32]; /* Ick. Hardcoded maximum of 32 attributes */
  GHashTable *tag_handlers;

  GnomeTextFuItem *retval = NULL;
  GnomeTextFuItemContainer *container;
  GnomeTextFuItem *stack[1024];
  int stack_cur = 0;

  tag_handlers = GNOME_TEXTFU_GET_CLASS(textfu)->tag_handlers;

  fd = open(textfu->cur_filename, O_RDONLY);
  if(fd < 0)
    return NULL;

  fstat(fd, &sbuf);

  mem = g_malloc(sbuf.st_size + 1);
  size_left = sbuf.st_size;
  while((size_left > 0) && (n = read(fd, mem + sbuf.st_size - size_left, size_left)) > 0)
    size_left -= n;
  if(n <= 0)
    goto out;

  retval = stack[0] = g_malloc0(sizeof(GnomeTextFuItemContainer));
  container = ((GnomeTextFuItemContainer *)retval);
  /* Set document defaults */
  container->subitems_font_size = 0;
  container->font_name = "helvetica";
  container->subitems_left_indent = 10;
  container->subitems_right_indent = 10;
  container->subitems_newpara = TEXTFU_TRUE;
  container->subitems_italic = TEXTFU_FALSE;
  container->subitems_bold = TEXTFU_FALSE;
  container->subitems_bullet = TEXTFU_FALSE;
  container->inherited_font = FALSE;

  stack_cur = 0;
  retval->type = TEXTFU_ITEM_CONTAINER;

  bracecount = quotecount = in_comment = 0;
  text_start = NULL;
  for(tag_end = mem; *tag_end; tag_end++)
    {
      switch(*tag_end)
	{
	case '<':
	  if(in_comment || quotecount)
	    break;

	  if(!bracecount)
	    {
	      if(text_start)
		{
		  *tag_end = '\0';
		  handle_text(textfu, stack, stack_cur, text_start, tag_end);
		  *tag_end = '<';
		  text_start = NULL;
		}
	      tag_start = tag_end;
	    }
	  if(!strncmp(tag_start, "<!", 2))
	    in_comment++;
	  else
	    bracecount++;
	  break;
	case '>':
	  if(!tag_start) /* just for sanity - George */
	    break;
	  if(!strncmp(tag_start, "<!", 2))
	    {
	      if(!strncmp(tag_end - 2, "-->", 3))
		in_comment--;
	      else
		break; /* Ignore bogus comment termination */
	    }

	  if(in_comment || quotecount)
	    break;

	  bracecount--;
	  if(!bracecount)
	    {
	      quotecount = 0;
	      *tag_start = *tag_end = '\0';

	      tag_start++;
	      if(tag_start >= tag_end) /* Empty tag, ignore */
		break;

	      parse_tag(tag_start, tag_end, tag_name, attrs);
	      handle_tag(textfu, tag_handlers, stack, &stack_cur, tag_name, attrs);
	    }
	  break;
	case '"':
	  if(bracecount)
	    quotecount = !quotecount;
	  break;
	default:
	  if(!text_start && !bracecount)
	    text_start = tag_end;
	  break;
	}
    }
  if(text_start)
    text_start = handle_text(textfu, stack, stack_cur, text_start, tag_end);

 out:
  g_free (mem);
  close(fd);
  return retval;
}

static void
gnome_textfu_item_destroy(GnomeTextFuItem *textfu)
{
  if(textfu == NULL)
    return;

  switch(textfu->type)
    {
    case TEXTFU_ITEM_CONTAINER:
      g_slist_foreach(((GnomeTextFuItemContainer *)textfu)->children, (GFunc)gnome_textfu_item_destroy, NULL);
      g_slist_free(((GnomeTextFuItemContainer *)textfu)->children);
      ((GnomeTextFuItemContainer *)textfu)->children = NULL;
      break;
    default:
      break;
    }
  g_free(textfu);
}

static void
dump_tree(GnomeTextFuItem *item, guint indent)
{
  int i;
  for(i = 0; i < indent; i++)
    g_print(" ");

  switch(item->type)
    {
    case TEXTFU_ITEM_CONTAINER:
      {
	GSList *children = ((GnomeTextFuItemContainer *)item)->children;

	g_print("CONTAINER: %d items\n", g_slist_length(children));
	g_slist_foreach(children, (GFunc)dump_tree, GUINT_TO_POINTER(indent + 4));
      }
      break;
    case TEXTFU_ITEM_WIDGET:
      {
	GnomeTextFuItemWidget *eitem = (GnomeTextFuItemWidget *)item;
	g_print("WIDGET: %dx%d\n", eitem->widget->requisition.width, eitem->widget->requisition.height);
      }
      break;
    case TEXTFU_ITEM_TEXT:
      {
	GnomeTextFuItemText *titem = (GnomeTextFuItemText *)item;
	g_print("TEXT: %16.16s%s\n", titem->text, (strlen(titem->text) > 16)?"...":"");
      }
      break;
    default:
      break;
    }
}

void
gnome_textfu_load_file(GnomeTextFu *textfu, const char *filename)
{
  g_return_if_fail (textfu != NULL);
  g_return_if_fail (GNOME_IS_TEXTFU (textfu));

  /* out with old */
  gnome_textfu_item_destroy(textfu->item_tree);
  textfu->item_tree = NULL;

  gnome_textfu_text_free_regions(textfu);

  g_free(textfu->cur_filename);

  /* in with new */
  textfu->cur_filename = g_strdup(filename);
  textfu->item_tree = gnome_textfu_parse(textfu);
  dump_tree(textfu->item_tree, 0);
}

guint16
gnome_textfu_tagid_alloc(const char *name, GnomeTextFuTagHandler handler)
{
  GnomeTextFuClass *klass;
  TagRegistration *cur_reg, *new_reg;
  static guint16 tag_counter = 1;

  klass = GNOME_TEXTFU_CLASS(gtk_type_class(gnome_textfu_get_type()));

  cur_reg = g_hash_table_lookup(klass->tag_handlers, name);
  if(cur_reg)
    return 0;

  new_reg = g_malloc(G_STRUCT_OFFSET(TagRegistration, tag_name) + strlen(name) + 1);
  new_reg->handler = handler;
  new_reg->tag_id = tag_counter++;
  strcpy(new_reg->tag_name, name);
  g_strdown(new_reg->tag_name);
  g_hash_table_insert(klass->tag_handlers, new_reg->tag_name, new_reg);

  return new_reg->tag_id;
}
