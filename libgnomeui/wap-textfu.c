/* wap-textfu.c
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
/* Written by Elliot Lee <sopwith@redhat.com>. This code is pretty aweful, but you should see my other code. */

#define BULLET_WIDTH 20
#define BULLET_HEIGHT 20
#define BASIC_INDENT 25
#define SMALL_FONT_SIZE 12
#define NORMAL_FONT_SIZE 18
#define LARGE_FONT_SIZE 24
#define PARA_SPACE_AFTER 12
#define BULLET_ALIGNMENT 1.0
#define MAX_SCROLL_AMT 20

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <gobject/gobject.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>
#include <gdk/gdkkeysyms.h>
#include "wap-textfu.h"

#ifndef g_alloca
#define g_alloca alloca
#endif

#define my_isspace(x) (isspace(x) || (x) == '\r' || (x) == '\n')
#define LINE_SPACING 4

static const gchar wap_unknown_char_utf8[] = { 0xEF, 0xBF, 0xBD, '\0' };
static void wap_textfu_cleanup_page(WapTextFu *textfu);
/*  static void xml_ctxt_done(WapStreamHandle handle, WapStreamStatus status, gpointer user_data); */
static void tag_option_event_selected(GtkWidget *list_item, GtkObject *textfu);

struct _PangoAttrList
{
  guint ref_count;
  GSList *attributes;
  GSList *attributes_tail;
};

static void
PC(PangoAttrList *attrs)
{
  GSList *ltmp;

  for(ltmp = attrs->attributes; ltmp; ltmp = ltmp->next)
    {
      PangoAttribute *attr = ltmp->data;

      g_assert(attr->start_index < attr->end_index);
    }
}

static void
dump_attrs(PangoAttrList *attrs) G_GNUC_UNUSED;
static void
dump_attrs(PangoAttrList *attrs)
{
  GSList *ltmp;
  const char *attr_names[] = {
    "PANGO_ATTR_LANG",
    "PANGO_ATTR_FAMILY",
    "PANGO_ATTR_STYLE",
    "PANGO_ATTR_WEIGHT",
    "PANGO_ATTR_VARIANT",
    "PANGO_ATTR_STRETCH",
    "PANGO_ATTR_SIZE",
    "PANGO_ATTR_FONT_DESC",
    "PANGO_ATTR_FOREGROUND",
    "PANGO_ATTR_BACKGROUND",
    "PANGO_ATTR_UNDERLINE",
    "PANGO_ATTR_STRIKETHROUGH",
    "PANGO_ATTR_RISE",
    "PANGO_ATTR_SHAPE"
  };

  g_print("Attrs for %p\n", attrs);
  for(ltmp = attrs->attributes; ltmp; ltmp = ltmp->next)
    {
      PangoAttribute *attr = ltmp->data;

      g_print("%d (%s): %d-%d\n",
	      attr->klass->type,
	      (attr->klass->type<(sizeof(attr_names)/sizeof(attr_names[0])))?attr_names[attr->klass->type]:"?",
	      attr->start_index, attr->end_index);
    }
}

typedef struct {
  WapTextFuTagHandler handler;
  guint16 tag_id;

  char tag_name[1];
} TagRegistration;

enum {
  LINK_FOLLOWED,
  ANCHOR_FOLLOWED,
  PREV_FOLLOWED,
  LOAD_DONE,
  LOAD_ERROR,
  ACTIVATE_CARD,
  DEACTIVATE_CARD,
  LAST_SIGNAL
};

static void wap_textfu_init		(WapTextFu		 *textfu);
static void wap_textfu_class_init	(WapTextFuClass	 *klass);
static void wap_textfu_realize            (GtkWidget      *widget);
static void wap_textfu_unrealize          (GtkWidget      *widget);
static void wap_textfu_map                (GtkWidget      *widget);
static void wap_textfu_unrealize          (GtkWidget      *widget);
static void wap_textfu_size_request       (GtkWidget      *widget,
					     GtkRequisition *requisition);
static void wap_textfu_size_allocate      (GtkWidget      *widget,
					     GtkAllocation  *allocation);
static void wap_textfu_draw               (GtkWidget      *widget, 
					     GdkRectangle   *area);
static gint wap_textfu_expose             (GtkWidget      *widget, 
					     GdkEventExpose *event);
static gint wap_textfu_button_release_event(GtkWidget *widget, GdkEventButton *event);
static gint wap_textfu_motion_notify_event(GtkWidget *widget, GdkEventMotion *event);
static gint wap_textfu_key_press_event(GtkWidget *widget, GdkEventKey *event);
static void wap_textfu_set_scroll_adjustments(GtkLayout      *layout,
					      GtkAdjustment  *hadjustment,
					      GtkAdjustment  *vadjustment);

static GtkWidget *wap_pixmap_new_from_uri(WapTextFu *textfu, char *uri, const char *alt_text);

static GtkWidgetClass *parent_class = NULL;
static guint textfu_signals[LAST_SIGNAL] = { 0 };

typedef struct {
  PangoAttribute attr;
  WapAnchorInfo *wai;

  /* Title, accessnum, etc. are stored elsewhere */
  PangoAttribute *fg_color_attr, *bg_color_attr; /* So we can change the color when the link is clicked on */
} LinkURLAttr;

static PangoAttribute *link_url_attr_new(WapAnchorInfo *wai);

static PangoAttribute *link_url_attr_copy(const PangoAttribute *attr)
{
  LinkURLAttr *r, *in = (LinkURLAttr *)attr;
  PangoAttribute *retval = link_url_attr_new(in->wai);

  r = (LinkURLAttr *)retval;
  *r = *in;

  return retval;
}

static void link_url_attr_destroy(PangoAttribute *attr)
{
  g_free(attr);
}

static gboolean
link_url_attr_compare(const PangoAttribute *attr1, const PangoAttribute *attr2)
{
  LinkURLAttr *ua1 = (LinkURLAttr *)attr1, *ua2 = (LinkURLAttr *)attr2;

  return ua1->wai == ua2->wai;
}

static PangoAttrClass link_url_attr_class = {
  0,
  link_url_attr_copy,
  link_url_attr_destroy,
  link_url_attr_compare
};

static PangoAttribute *link_url_attr_new(WapAnchorInfo *wai)
{
  LinkURLAttr *retval = g_new0(LinkURLAttr, 1);

  retval->attr.klass = &link_url_attr_class;
  retval->wai = wai;

  return (PangoAttribute *)retval;
}

typedef struct {
  PangoAttribute *shape_attr;
  GtkWidget *widget;
  gint bullet_level; /* If widget is NULL */
  double alignment;
} PlacedItem;

static void
wap_text_style_free(WapTextStyle *ts)
{
  g_free(ts->font_family);
  g_free(ts);
}

static void
populate_attr_list_2(WapTextFu *textfu, WapTextStyle *ts, int end_offset,
		     int link_start, int font_family_start, int bold_start, int italic_start,
		     int underline_start, int fg_color_start, int bg_color_start, int font_size_start,
		     gboolean color_from_link)
{
  LinkURLAttr *link_attr = NULL;
  PangoAttribute *attr;

  if(!textfu->cur_para)
    return;

  if(ts->wai)
    {
      attr = link_url_attr_new(ts->wai);
      attr->start_index = link_start;
      attr->end_index = end_offset;
      pango_attr_list_insert(textfu->cur_para->attrs, attr);
      link_attr = (LinkURLAttr *)attr;
      ts->wai->attr = attr;
    }

  if(ts->font_family)
    {
      attr = pango_attr_family_new(ts->font_family);
      attr->start_index = font_family_start;
      attr->end_index = end_offset;
      pango_attr_list_insert(textfu->cur_para->attrs, attr);
    }

  if(ts->bold != TEXTFU_UNKNOWN)
    {
      attr = pango_attr_weight_new((ts->bold == TEXTFU_TRUE)?PANGO_WEIGHT_BOLD:PANGO_WEIGHT_NORMAL);
      attr->start_index = bold_start;
      attr->end_index = end_offset;
      pango_attr_list_insert(textfu->cur_para->attrs, attr);
    }

  if(ts->italic != TEXTFU_UNKNOWN)
    {
      attr = pango_attr_style_new((ts->italic == TEXTFU_TRUE)?PANGO_STYLE_ITALIC:PANGO_STYLE_NORMAL);
      attr->start_index = italic_start;
      attr->end_index = end_offset;
      pango_attr_list_insert(textfu->cur_para->attrs, attr);
    }

  if(ts->underline != TEXTFU_UNKNOWN)
    {
      attr = pango_attr_underline_new((ts->underline == TEXTFU_TRUE)?PANGO_UNDERLINE_SINGLE:PANGO_UNDERLINE_NONE);
      attr->start_index = underline_start;
      attr->end_index = end_offset;
      pango_attr_list_insert(textfu->cur_para->attrs, attr);
    }

  if(ts->fg_color_set)
    {
      attr = pango_attr_foreground_new(ts->fg_color.red, ts->fg_color.green, ts->fg_color.blue);
      g_assert(fg_color_start < end_offset);
      attr->start_index = fg_color_start;
      attr->end_index = end_offset;
      pango_attr_list_insert(textfu->cur_para->attrs, attr);

      if(color_from_link)
	link_attr->fg_color_attr = attr;
    }

  if(ts->bg_color_set)
    {
      attr = pango_attr_background_new(ts->bg_color.red, ts->bg_color.green, ts->bg_color.blue);
      g_assert(bg_color_start < end_offset);
      attr->start_index = bg_color_start;
      attr->end_index = end_offset;
      pango_attr_list_insert(textfu->cur_para->attrs, attr);

      if(color_from_link)
	link_attr->bg_color_attr = attr;
    }

  if(ts->font_size_set)
    {
      attr = pango_attr_size_new(ts->font_size * PANGO_SCALE);
      attr->start_index = font_size_start;
      attr->end_index = end_offset;
      pango_attr_list_insert(textfu->cur_para->attrs, attr);
    }
}

static void
populate_attr_list(WapTextFu *textfu, WapTextStyle *ts, gboolean color_from_link)
{
  int real_offset = MAX(ts->start_offset, 0);

  populate_attr_list_2(textfu, ts, textfu->tmp_text->len,
		       real_offset,
		       real_offset,
		       real_offset,
		       real_offset,
		       real_offset,
		       real_offset,
		       real_offset,
		       real_offset,
		       color_from_link);
}

static WapParagraphInfo *
push_pango_para(WapTextFu *textfu)
{
  GList *ltmp;
  WapTextStyle result;
  PangoAttribute *attr;
  int font_family_start=0, font_size_start=0, bold_start=0, italic_start=0, underline_start=0, bg_color_start=0, fg_color_start=0, link_start=0;
  int end_offset;
  WapParaStyle *ps;
  gboolean got_ul, got_ol;
  int bullet_level = 0, bullet_num = 0;
  int start_offset = 0;
  int first_indent = 0;
  gboolean color_from_link = FALSE;

  if(!textfu->cur_para)
    return NULL; /* nothing to do! */

  memset(&result, 0, sizeof(result));

  g_assert(textfu->cur_card->para_style_stack);
  ps = textfu->cur_card->para_style_stack->data;
  for(ltmp = textfu->cur_card->para_style_stack, got_ul = got_ol = FALSE; ltmp && !(got_ul || got_ol); ltmp = ltmp->next)
    {
      WapParaStyle *pstmp = ltmp->data;

      /* Eek, hardcoding tags in */
      if(pstmp->ul_container)
	got_ul = TRUE;
      else if(pstmp->ol_container)
	got_ol = TRUE;
      bullet_level = pstmp->bullet_level;
      bullet_num = pstmp->bullet_num;
    }
  if(!ltmp)
    bullet_level = bullet_num = 0;

  if(got_ol)
    {
      char cbuf[8];

      switch(bullet_level)
	{
	case 0:
	  g_assert_not_reached();
	  break;
	case 1:
	  g_snprintf(cbuf, sizeof(cbuf), "%d. ", bullet_num + 1);
	  break;
	case 2:
	  g_snprintf(cbuf, sizeof(cbuf), "(%c) ", 'a' + (bullet_num % 26));
	  break;
	default:
	  g_snprintf(cbuf, sizeof(cbuf), "(%d) ", bullet_num + 1);
	  break;
	}

      start_offset = strlen(cbuf);
      g_string_prepend_len(textfu->tmp_text, cbuf, start_offset);
      first_indent = -BULLET_WIDTH * PANGO_SCALE;
    }
  else if(got_ul)
    start_offset = strlen(wap_unknown_char_utf8);

#define CALC_OFFSET() (ts->start_offset>=0)?(ts->start_offset+start_offset):0

  result.bold = result.italic = result.underline = TEXTFU_UNKNOWN;

  for(ltmp = g_list_last(textfu->cur_card->text_style_stack); ltmp; ltmp = ltmp->prev)
    {
      WapTextStyle *ts = ltmp->data;

      if(ts->wai)
	{
	  result.wai = ts->wai;
	  link_start = CALC_OFFSET();
	}

      if(ts->font_family)
	{
	  result.font_family = ts->font_family;
	  font_family_start = CALC_OFFSET();
	}

      if(ts->bold != TEXTFU_UNKNOWN)
	{
	  bold_start = CALC_OFFSET();
	  result.bold = ts->bold;
	}
      if(ts->italic != TEXTFU_UNKNOWN)
	{
	  result.italic = ts->italic;
	  italic_start = CALC_OFFSET();
	}
      if(ts->underline != TEXTFU_UNKNOWN)
	{
	  result.underline = ts->underline;
	  underline_start = CALC_OFFSET();
	}
      if(ts->fg_color_set)
	{
	  result.fg_color_set = TRUE;
	  result.fg_color = ts->fg_color;
	  fg_color_start = CALC_OFFSET();
	  color_from_link = ts->wai?TRUE:FALSE;
	}
      if(ts->bg_color_set)
	{
	  result.bg_color_set = TRUE;
	  result.bg_color = ts->bg_color;
	  bg_color_start = CALC_OFFSET();
	  color_from_link = ts->wai?TRUE:FALSE;
	}
      if(ts->font_size_set)
	{
	  result.font_size_set = TRUE;
	  result.font_size = ts->font_size;
	  font_size_start = CALC_OFFSET();
	}

      ts->start_offset = -1; /* Indicates that it is a carryover from previous paragraph */
    }

  end_offset = textfu->tmp_text->len;

  /* Now we have figured out what the text is supposed to look like, finish creating 'cur_attrs' */

  populate_attr_list_2(textfu, &result, end_offset, link_start,
		       font_family_start, bold_start, italic_start, underline_start,
		       fg_color_start, bg_color_start, font_size_start, color_from_link);
  PC(textfu->cur_para->attrs);

  if(got_ul)
    {
      GList *placed_items = textfu->cur_para->placed_items;
      PlacedItem *pi;
      PangoRectangle bullet_rect;

      pi = g_new0(PlacedItem, 1);

      bullet_rect.x = 0;
      bullet_rect.y = -(BULLET_ALIGNMENT * BULLET_HEIGHT) * PANGO_SCALE;
      bullet_rect.width = BULLET_WIDTH * PANGO_SCALE;
      bullet_rect.height = BULLET_HEIGHT * PANGO_SCALE;
      pi->alignment = BULLET_ALIGNMENT;

      pi->shape_attr = attr = pango_attr_shape_new(&bullet_rect, &bullet_rect);
      pi->shape_attr->start_index = 0;
      pi->shape_attr->end_index = start_offset;
      pi->bullet_level = bullet_level;
      g_string_prepend_len(textfu->tmp_text, wap_unknown_char_utf8, start_offset);

      placed_items = g_list_prepend(placed_items, pi);
      textfu->cur_para->placed_items = placed_items;

      pango_attr_list_insert(textfu->cur_para->attrs, attr);
      first_indent = -BULLET_WIDTH * PANGO_SCALE;
    }

  pango_layout_set_indent(textfu->cur_para->layout, first_indent);
  pango_layout_set_text(textfu->cur_para->layout, textfu->tmp_text->str, textfu->tmp_text->len);

  pango_layout_set_attributes(textfu->cur_para->layout, textfu->cur_para->attrs);
  pango_attr_list_unref(textfu->cur_para->attrs);
  pango_layout_set_alignment(textfu->cur_para->layout, ps->alignment);
  textfu->cur_para->left_indent = ps->left_indent;
  textfu->cur_para->right_indent = ps->right_indent;
  textfu->cur_para->space_after = ps->space_after;

  textfu->cur_card->paragraphs = g_list_append(textfu->cur_card->paragraphs, textfu->cur_para);
  textfu->cur_para = NULL;
  g_string_truncate(textfu->tmp_text, 0);

  return textfu->cur_para;
}

static WapTextStyle *
style_change(WapTextFu *textfu, const char *tagname, gboolean begin_new, gboolean ascend_tree, gboolean is_link)
{
  WapTextStyle *retval = NULL, *old = NULL;
  GList *cur;

  cur = textfu->cur_card->text_style_stack;
  if(cur)
    do {
      old = cur->data;
      if(!strcasecmp(old->style.tag, tagname))
	break;

      cur = ascend_tree?cur->next:NULL;
    } while(cur);

  if(!cur)
    old = NULL;

  if(old)
    {
      /* Pump it into the attribute list */
      populate_attr_list(textfu, old, is_link);

      textfu->cur_card->text_style_stack = g_list_remove_link(textfu->cur_card->text_style_stack, cur);
      g_list_free_1(cur);
    }

  if(begin_new)
    {
      retval = g_new0(WapTextStyle, 1);
      strcpy(retval->style.tag, tagname);

      retval->start_offset = textfu->tmp_text->len;

      if(old && old->replace_prev)
	{
	  retval->wai = old->wai;
	  retval->font_family = g_strdup(old->font_family);
	  retval->font_size = old->font_size;
	  retval->bg_color = old->bg_color;
	  retval->fg_color = old->fg_color;
	  retval->bold = old->bold;
	  retval->italic = old->italic;
	  retval->underline = old->underline;
	  retval->fg_color_set = old->fg_color_set;
	  retval->bg_color_set = old->bg_color_set;
	  retval->font_size_set = old->font_size_set;
	  retval->replace_prev = old->replace_prev;
	}
      else
	{
	  retval->bold = TEXTFU_UNKNOWN;
	  retval->italic = TEXTFU_UNKNOWN;
	  retval->underline = TEXTFU_UNKNOWN;
	}

      textfu->cur_card->text_style_stack = g_list_prepend(textfu->cur_card->text_style_stack, retval);
    }

  g_free(old);

  return retval;
}

/* Called when we parse some displayable content that might require setting up things for a paragraph */
static void
initiate_para(WapTextFu *textfu)
{
  if(!textfu->cur_para)
    {
      textfu->cur_para = g_new0(WapParagraphInfo, 1);
      textfu->cur_para->layout = pango_layout_new(textfu->pango_ctx);
      textfu->cur_para->attrs = pango_attr_list_new();
    }
}

static WapParaStyle *
paragraph_division(WapTextFu *textfu, const char *tagname, gboolean begin_new, gboolean ascend_tree)
{
  WapParaStyle *retval = NULL, *old = NULL;
  GList *cur;

  push_pango_para(textfu);

  cur = textfu->cur_card->para_style_stack;
  if(cur)
    do {
      old = cur->data;
      if(!strcasecmp(old->style.tag, tagname))
	break;

      cur = ascend_tree?cur->next:NULL;
    } while(cur);

  if(!cur)
    old = NULL;

  if(old)
    {
      textfu->cur_card->para_style_stack = g_list_remove_link(textfu->cur_card->para_style_stack, cur);
      g_list_free_1(cur);
    }

  if(begin_new)
    {
      retval = g_new0(WapParaStyle, 1);
      strcpy(retval->style.tag, tagname);
      if(old && old->replace_prev)
	{
	  retval->left_indent = old->left_indent;
	  retval->right_indent = old->right_indent;
	  retval->alignment = old->alignment;
	  retval->replace_prev = old->replace_prev;
	  retval->space_after = old->space_after;
	  /* Don't copy bulleting info - should be zero'd unless explicitly set */
	}
      else
	{
	  if(textfu->cur_card->para_style_stack)
	    {
	      WapParaStyle *parent_ps;

	      parent_ps = textfu->cur_card->para_style_stack->data;
	      retval->left_indent = parent_ps->left_indent;
	      retval->right_indent = parent_ps->right_indent;
	      retval->space_after = parent_ps->space_after;
	    }
	  else
	    {
	      retval->left_indent = retval->right_indent = BASIC_INDENT;
	      retval->space_after = PARA_SPACE_AFTER;
	    }

	  retval->alignment = PANGO_ALIGN_LEFT;
	}

      textfu->cur_card->para_style_stack = g_list_prepend(textfu->cur_card->para_style_stack, retval);
    }

  g_free(old);

  return retval;
}

static gboolean
toboolean(const char *string)
{
  if(string[0] == '1'
     || tolower(string[0]) == 't'
     || tolower(string[0]) == 'y')
    return TRUE;

  return FALSE;
}

static GList *
parse_query_string(const char *str)
{
  char **pieces;
  int j;
  GList *retval = NULL;

  pieces = g_strsplit(str, "&", -1);
  for(j = 0; pieces[j]; j++)
    {
      char **vpieces;
      vpieces = g_strsplit(pieces[j], "=", 2);

      retval = g_list_prepend(retval, vpieces[1]);
      retval = g_list_prepend(retval, vpieces[0]);
      g_free(vpieces);
    }
  g_strfreev(pieces);

  return retval;
}

static PlacedItem *
make_placed_widget(WapTextFu *textfu, GtkWidget *widget, double alignment)
{
  PlacedItem *pi;
  PangoRectangle rect;
  GList *placed_items;
  PangoAttribute *attr;

  g_assert(textfu->in_content);

  initiate_para(textfu);

  placed_items = textfu->cur_para->placed_items;

  pi = g_new0(PlacedItem, 1);
  pi->widget = widget;
  pi->alignment = alignment;

  rect.x = 0;
  rect.y = -(alignment * widget->allocation.height) * PANGO_SCALE;
  rect.width = widget->allocation.width * PANGO_SCALE;
  rect.height = widget->allocation.height * PANGO_SCALE;

  pi->shape_attr = attr = pango_attr_shape_new(&rect, &rect);
  attr->start_index = textfu->tmp_text->len;
  attr->end_index = attr->start_index + strlen(wap_unknown_char_utf8);
  /* Dummy char to take up the spot */
  g_string_append_len(textfu->tmp_text, wap_unknown_char_utf8, strlen(wap_unknown_char_utf8));

  placed_items = g_list_prepend(placed_items, pi);
  textfu->cur_para->placed_items = placed_items;

  pango_attr_list_insert(textfu->cur_para->attrs, attr);

  gtk_layout_put(GTK_LAYOUT(textfu), widget, -100, -100);
  gtk_widget_show(widget);

  return pi;
}

/* tag implementations (X'd needs implementing):
   a
   anchor
   b
   big
   br
   card
   do
   em
   go
   i
   img
   input
   option
   p
   postfield
   prev
   select
   small
   strong
   table
   tr
   td
   u
   wml
 */

/* Ignore all the text underneath this tag */
static void
tag_handler_ignore(WapTextFu *textfu, const char *tag, guint16 tag_id,
		   char **attrs, gboolean is_end)
{
  if(is_end)
    textfu->ignore_counter--;
  else
    textfu->ignore_counter++;
}

static void
link_style_setup(WapTextFu *textfu, WapTextStyle *ts, WapAnchorInfo *wai)
{
  ts->wai = wai;
  ts->underline = TEXTFU_TRUE;
  ts->fg_color.red = ts->fg_color.green = 0; ts->fg_color.blue = 65535;
  gdk_color_alloc(gdk_rgb_get_cmap(), &ts->fg_color);
  ts->fg_color_set = TRUE;

  /* Even if this is currently same as the default bg right now, we will be changing it for selected links */
  ts->bg_color.red = ts->bg_color.green = 65535; ts->bg_color.blue = 65535;
  gdk_color_alloc(gdk_rgb_get_cmap(), &ts->bg_color);
  ts->bg_color_set = TRUE;

  ts->replace_prev = TRUE;
}

static void
tag_handler_a_hdml(WapTextFu *textfu, const char *tag, guint16 tag_id,
		   char **attrs, gboolean is_end)
{
  WapTextStyle *ts;

  ts = style_change(textfu, tag, !is_end, TRUE, TRUE);

  if(ts && attrs)
    {
      int i;
      WapAnchorInfo *wai;

      wai = g_new0(WapAnchorInfo, 1);
      wai->type = ANCHORS_AWAY;
      for(i = 0; attrs[i]; i++)
	{
	  if(!strcasecmp(attrs[i], "task"))
	    {
	      i++;
	      if(!strcasecmp(attrs[i], "go"))
		wai->type = ANCHOR_GO;
	      else if(!strcasecmp(attrs[i], "prev"))
		wai->type = ANCHOR_PREV;
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "dest"))
	    {
	      if(!wai->go_info)
		wai->go_info = g_new0(WapGoInfo, 1);
	      wai->go_info->url = g_strdup(attrs[i++]);
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "method"))
	    {
	      if(!wai->go_info)
		wai->go_info = g_new0(WapGoInfo, 1);

	      wai->go_info->post_method = !strcasecmp(attrs[++i], "post");
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "label"))
	    {
	      wai->title = g_strdup(attrs[++i]);
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "sendreferer"))
	    {
	      if(!wai->go_info)
		wai->go_info = g_new0(WapGoInfo, 1);

	      wai->go_info->sendreferer = toboolean(attrs[++i]);
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "accept-charset"))
	    {
	      if(!wai->go_info)
		wai->go_info = g_new0(WapGoInfo, 1);

	      wai->go_info->charset = g_strdup(attrs[++i]);
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "postdata"))
	    {
	      if(!wai->go_info)
		wai->go_info = g_new0(WapGoInfo, 1);

	      wai->go_info->postvalues = parse_query_string(attrs[++i]);
	      continue;
	    }
	}

      if(wai->go_info)
	wai->go_info->sendreferer = TRUE;
      link_style_setup(textfu, ts, wai);

      textfu->cur_para->anchors = g_list_append(textfu->cur_para->anchors, wai);
    }
}

static void
tag_handler_a(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  WapTextStyle *ts;
  /* We need to roughly translate this into
     <anchor title=thetitle><go href=thehref/></anchor> */

  initiate_para(textfu);

  if(textfu->is_hdml)
    return tag_handler_a_hdml(textfu, tag, tag_id, attrs, is_end);

  ts = style_change(textfu, tag, !is_end, TRUE, TRUE);

  if(ts && attrs)
    {
      int i;
      char *link_title = NULL, *link_url = NULL;
      WapAnchorInfo *wai;
      
      for(i = 0; attrs[i]; i++)
      {
	if(!strcasecmp(attrs[i], "href"))
	  {
	    link_url = g_strdup(attrs[++i]);
	    continue;
	  }
	if(!strcasecmp(attrs[i], "title"))
	  {
	    link_title = g_strdup(attrs[++i]);
	    continue;
	  }
      }
      
      wai = g_new0(WapAnchorInfo, 1);
      wai->title = link_title;
      wai->type = ANCHOR_GO;
      wai->go_info = g_new0(WapGoInfo, 1);
      wai->go_info->url = link_url;
      wai->go_info->sendreferer = TRUE;

      link_style_setup(textfu, ts, wai);

      textfu->cur_para->anchors = g_list_append(textfu->cur_para->anchors, wai);
    }
}

static void
tag_handler_action(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  int i;
  WapDoInfo *doi;

  if(is_end)
    return;

  doi = g_new0(WapDoInfo, 1);

  for(i = 0; attrs[i]; i++)
    {
      if(!strcasecmp(attrs[i], "type"))
	{
	  i++;
	  if(!strcasecmp(attrs[i], "go"))
	    doi->type = DO_ACCEPT;
	  else if(!strcasecmp(attrs[i], "prev"))
	    doi->type = DO_PREV;
	  else if(!strcasecmp(attrs[i], "help"))
	    doi->type = DO_HELP;
	  else if(!strcasecmp(attrs[i], "soft1"))
	    doi->type = DO_SOFT1;
	  else if(!strcasecmp(attrs[i], "soft2"))
	    doi->type = DO_SOFT2;
	  else if(!strcasecmp(attrs[i], "prev"))
	    doi->type = DO_PREV;
	  else if(!strcasecmp(attrs[i], "send"))
	    doi->type = DO_SEND;
	  else
	    doi->type = DO_NOTHING;

	  continue;
	}

      if(!strcasecmp(attrs[i], "label"))
	{
	  doi->label = g_strdup(attrs[++i]);
	  continue;
	}

      if(!strcasecmp(attrs[i], "name"))
	{
	  doi->name = g_strdup(attrs[++i]);
	  continue;
	}

      if(!strcasecmp(attrs[i], "task"))
	{
	  if(!strcasecmp(attrs[i], "go")
	     || !strcasecmp(attrs[i], "gosub"))
	    doi->task = ACT_GO;
	  else if(!strcasecmp(attrs[i], "prev"))
	    doi->task = ACT_PREV;
	  else
	    doi->task = ACT_NOTHING;
	}

      if(!strcasecmp(attrs[i], "dest"))
	{
	  if(!doi->go)
	    doi->go = g_new0(WapGoInfo, 1);

	  doi->go->url = g_strdup(attrs[++i]);
	  continue;
	}

      if(!strcasecmp(attrs[i], "accept-charset"))
	{
	  if(!doi->go)
	    doi->go = g_new0(WapGoInfo, 1);

	  doi->go->charset = g_strdup(attrs[++i]);
	  continue;
	}

      if(!strcasecmp(attrs[i], "postdata"))
	{
	  if(!doi->go)
	    doi->go = g_new0(WapGoInfo, 1);

	  doi->go->postvalues = parse_query_string(attrs[++i]);
	  continue;
	}

      if(!strcasecmp(attrs[i], "method"))
	{
	  if(!doi->go)
	    doi->go = g_new0(WapGoInfo, 1);

	  if(!strcasecmp(attrs[++i], "post"))
	    doi->go->post_method = TRUE;

	  continue;
	}

      if(!strcasecmp(attrs[i], "sendreferer"))
	{
	  if(!doi->go)
	    doi->go = g_new0(WapGoInfo, 1);

	  doi->go->sendreferer = toboolean(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "src"))
	{
	  doi->img = wap_pixmap_new_from_uri(textfu, attrs[++i], NULL);
	  continue;
	}
    }

  textfu->cur_card->actions = g_list_append(textfu->cur_card->actions, doi);
}

static void
tag_handler_anchor(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  WapTextStyle *ts;

  ts = style_change(textfu, tag, !is_end, TRUE, TRUE);
  textfu->in_anchor = !is_end;

  if(ts)
    {
      int i;
      char *link_title = NULL;
      WapAnchorInfo *wai;
      
      initiate_para(textfu);

      if(attrs)
	for(i = 0; attrs[i]; i++)
	  {
	    if(!strcasecmp(attrs[i], "title"))
	      {
		link_title = g_strdup(attrs[++i]);
		continue;
	      }
	  }
      
      wai = g_new0(WapAnchorInfo, 1);
      wai->title = link_title;
      wai->type = ANCHORS_AWAY;

      link_style_setup(textfu, ts, wai);

      textfu->cur_anchor = wai;
      textfu->cur_para->anchors = g_list_append(textfu->cur_para->anchors, wai);
    }
  else
    textfu->cur_anchor = NULL;
}

static void
tag_handler_b(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  WapTextStyle *ts;

  ts = style_change(textfu, tag, !is_end, TRUE, FALSE);

  if(ts)
    {
      ts->bold = TEXTFU_TRUE;
    }
}

static void
tag_handler_big(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  WapTextStyle *ts;

  ts = style_change(textfu, tag, !is_end, TRUE, FALSE);

  if(ts)
    {
      ts->font_size = LARGE_FONT_SIZE;
      ts->font_size_set = TRUE;
    }
}

static void
tag_handler_br(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  WapParagraphInfo *para;

  para = push_pango_para(textfu);
  if(para)
    para->space_after = 0;
}

static void
tag_handler_card(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  textfu->in_content = !is_end;

  if(is_end)
    {
      g_assert(textfu->cur_card);

      paragraph_division(textfu, tag, FALSE, FALSE);
      style_change(textfu, tag, FALSE, TRUE, FALSE);

      g_list_foreach(textfu->cur_card->para_style_stack, (GFunc)g_free, NULL);
      g_list_free(textfu->cur_card->para_style_stack); textfu->cur_card->para_style_stack = NULL;

      g_list_foreach(textfu->cur_card->para_style_stack, (GFunc)wap_text_style_free, NULL);
      g_list_free(textfu->cur_card->text_style_stack); textfu->cur_card->text_style_stack = NULL;
    }
  else
    {
      WapCard *card;
      int i;
      WapTextStyle *ts;

      card = g_new0(WapCard, 1);
      textfu->cur_card = card;

      textfu->cards = g_list_append(textfu->cards, card);

      if(attrs)
	for(i = 0; attrs[i]; i++)
	  {
	    if(!strcasecmp(attrs[i], "id"))
	      {
		card->id = g_strdup(attrs[++i]);
		continue;
	      }

	    if(!strcasecmp(attrs[i], "title"))
	      {
		card->title = g_strdup(attrs[++i]);
		continue;
	      }

	    if(!strcasecmp(attrs[i], "ontimer"))
	      {
		card->ontimer = g_strdup(attrs[++i]);
		continue;
	      }

	    if(!strcasecmp(attrs[i], "onenterforward"))
	      {
		card->onenterbackward = g_strdup(attrs[++i]);
		continue;
	      }

	    if(!strcasecmp(attrs[i], "onenterbackward"))
	      {
		card->onenterbackward = g_strdup(attrs[++i]);
		continue;
	      }

	    if(!strcasecmp(attrs[i], "newcontext"))
	      {
		card->newcontext = toboolean(attrs[++i]);
		continue;
	      }
	  }
      if(!card->id)
	card->id = g_strdup("X!!X");

      ts = style_change(textfu, tag, TRUE, TRUE, FALSE);
      ts->font_size_set = TRUE;
      ts->font_size = NORMAL_FONT_SIZE;

      paragraph_division(textfu, tag, TRUE, FALSE);
    }
}

static void
tag_handler_ce(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  WapInputInfo *input;
  WapOptionInfo *opt;

  if(is_end)
    return;

  if(textfu->cur_option)
    {
      /* End the previous CE tag */

      PlacedItem *pi;
      GtkWidget *mi, *b;

      pi = textfu->cur_input->placed_item;

      textfu->cur_option->text = g_strdup(textfu->nc_text->str);
      g_strstrip(textfu->cur_option->text);
      if(!strlen(textfu->cur_option->text))
	{
	  g_free(textfu->cur_option->text); textfu->cur_option->text = NULL;
	}

      if(textfu->cur_option->image)
	{
	  mi = gtk_list_item_new();
	  if(textfu->cur_option->text)
	    {
	      b = gtk_hbox_new(FALSE, 5);
	      gtk_container_add(GTK_CONTAINER(b), textfu->cur_option->image);
	      gtk_container_add(GTK_CONTAINER(b), gtk_label_new(textfu->cur_option->text));
	      gtk_container_add(GTK_CONTAINER(mi), b);
	    }
	  else
	    gtk_container_add(GTK_CONTAINER(mi), textfu->cur_option->image);
	}
      else
	mi = gtk_list_item_new_with_label(textfu->cur_option->text?textfu->cur_option->text:"");
      gtk_widget_show_all(mi);

      gtk_object_set_data(GTK_OBJECT(mi), "WapOptionInfo", textfu->cur_option);

      if(textfu->cur_option->onpick)
	gtk_signal_connect(GTK_OBJECT(mi), "select", GTK_SIGNAL_FUNC(tag_option_event_selected), textfu);

      textfu->cur_option->li_widget = mi;
      textfu->in_option = FALSE;
      textfu->cur_option = NULL;

      gtk_container_add(GTK_CONTAINER(pi->widget), mi);
    }

  g_assert(textfu->in_select);
  textfu->in_content = FALSE;
  textfu->in_option = TRUE;
  input = textfu->cur_input;

  opt = textfu->cur_option = g_new0(WapOptionInfo, 1);
  textfu->cur_input->options = g_list_append(textfu->cur_input->options, opt);

  if(attrs)
    {
      int i;

      for(i = 0; attrs[i]; i++)
	{
	  if(!strcasecmp(attrs[i], "value"))
	    {
	      opt->value = g_strdup(attrs[++i]);
	      continue;
	    }

	  /* Bad hack - we don't handle the "task" attribute */
	  if(!strcasecmp(attrs[i], "dest"))
	    {
	      opt->onpick = g_strdup(attrs[++i]);
	      continue;
	    }
	}
    }
}

static void
tag_handler_choice(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  textfu->in_content = !is_end;

  if(is_end)
    {
      textfu->cur_input = NULL;

      paragraph_division(textfu, tag, FALSE, FALSE);
      style_change(textfu, tag, FALSE, TRUE, FALSE);

      g_list_foreach(textfu->cur_card->para_style_stack, (GFunc)g_free, NULL);
      g_list_free(textfu->cur_card->para_style_stack); textfu->cur_card->para_style_stack = NULL;

      g_list_foreach(textfu->cur_card->para_style_stack, (GFunc)wap_text_style_free, NULL);
      g_list_free(textfu->cur_card->text_style_stack); textfu->cur_card->text_style_stack = NULL;
    }
  else
    {
      WapCard *card;
      int i;
      WapTextStyle *ts;
      GtkWidget *widget;

      card = g_new0(WapCard, 1);
      textfu->cur_card = card;

      textfu->cards = g_list_append(textfu->cards, card);

      textfu->cur_input = g_new0(WapInputInfo, 1);
      textfu->cur_card->inputs = g_list_append(textfu->cur_card->inputs, textfu->cur_input);
      /* We purposely don't set in_select and in_content here, we do it in the 'ce' tag handler to be able to grok the
	 line or two of text that comes along */

      for(i = 0; attrs[i]; i++)
	{
	  if(!strcasecmp(attrs[i], "name"))
	    {
	      card->id = g_strdup(attrs[++i]);
	      continue;
	    }

	  if(!strcasecmp(attrs[i], "title"))
	    {
	      card->title = g_strdup(attrs[++i]);
	      continue;
	    }

	  if(!strcasecmp(attrs[i], "key"))
	    {
	      textfu->cur_input->name = g_strdup(attrs[++i]);
	      continue;
	    }
	}

      textfu->cur_input->is_selection = TRUE;
      widget = gtk_list_new();
      gtk_list_set_selection_mode(GTK_LIST(widget), GTK_SELECTION_SINGLE);
      gtk_object_set_data(GTK_OBJECT(widget), "PlacedItem",
			  textfu->cur_input->placed_item = make_placed_widget(textfu, widget, 0.1));

      ts = style_change(textfu, tag, TRUE, TRUE, FALSE);
      ts->font_size_set = TRUE;
      ts->font_size = NORMAL_FONT_SIZE;

      paragraph_division(textfu, tag, TRUE, FALSE);
    }
}

static void
tag_handler_display(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  textfu->in_content = !is_end;

  if(is_end)
    {
      g_assert(textfu->cur_card);

      paragraph_division(textfu, tag, FALSE, FALSE);
      style_change(textfu, tag, FALSE, TRUE, FALSE);

      g_list_foreach(textfu->cur_card->para_style_stack, (GFunc)g_free, NULL);
      g_list_free(textfu->cur_card->para_style_stack); textfu->cur_card->para_style_stack = NULL;

      g_list_foreach(textfu->cur_card->para_style_stack, (GFunc)wap_text_style_free, NULL);
      g_list_free(textfu->cur_card->text_style_stack); textfu->cur_card->text_style_stack = NULL;
    }
  else
    {
      WapCard *card;
      int i;
      WapTextStyle *ts;

      card = g_new0(WapCard, 1);
      textfu->cur_card = card;

      textfu->cards = g_list_append(textfu->cards, card);

      for(i = 0; attrs[i]; i++)
	{
	  if(!strcasecmp(attrs[i], "name"))
	    {
	      card->id = g_strdup(attrs[++i]);
	      continue;
	    }

	  if(!strcasecmp(attrs[i], "title"))
	    {
	      card->title = g_strdup(attrs[++i]);
	      continue;
	    }
	}

      ts = style_change(textfu, tag, TRUE, TRUE, FALSE);
      ts->font_size_set = TRUE;
      ts->font_size = NORMAL_FONT_SIZE;

      paragraph_division(textfu, tag, TRUE, FALSE);
    }
}

static void
tag_handler_do(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  textfu->in_do = !is_end;
  textfu->in_content = is_end;

  if(!is_end)
    {
      int i;
      WapDoInfo *doi = g_new0(WapDoInfo, 1);

      for(i = 0; attrs[i]; i++)
	{
	  if(!strcasecmp(attrs[i], "type"))
	    {
	      i++;
	      if(!strcasecmp(attrs[i], "accept"))
		doi->type = DO_ACCEPT;
	      else if(!strcasecmp(attrs[i], "delete"))
		doi->type = DO_DELETE;
	      else if(!strcasecmp(attrs[i], "help"))
		doi->type = DO_HELP;
	      else if(!strcasecmp(attrs[i], "options"))
		doi->type = DO_OPTIONS;
	      else if(!strcasecmp(attrs[i], "prev"))
		{
		  doi->type = DO_PREV;
		  doi->task = ACT_PREV;
		}
	      else
		doi->type = DO_NOTHING;

	      continue;
	    }

	  if(!strcasecmp(attrs[i], "label"))
	    {
	      doi->label = g_strdup(attrs[++i]);
	      continue;
	    }

	  if(!strcasecmp(attrs[i], "name"))
	    {
	      doi->name = g_strdup(attrs[++i]);
	      continue;
	    }

	  /* Exactly why are we supposed to care about the 'optional' attribute? */
	}

      if(!doi->label)
	{
	  switch(doi->task)
	    {
	    case ACT_GO:
	      doi->label = g_strdup("Go");
	      break;
	    case ACT_PREV:
	      doi->label = g_strdup("Prev");
	      break;
	    default:
	      doi->label = g_strdup("Unknown");
	      break;
	    }
	}

      textfu->cur_card->actions = g_list_append(textfu->cur_card->actions, doi);
      textfu->cur_do = doi;
    }
  else
    {
      textfu->cur_do = NULL;
    }
}

static void
tag_handler_em(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  WapTextStyle *ts;

  ts = style_change(textfu, tag, !is_end, TRUE, FALSE);

  if(ts)
    {
      ts->italic = TEXTFU_TRUE;
    }
}

static void
tag_handler_entry(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  int i;
  WapInputInfo *input;
  GtkWidget *widget;
  WapCard *card;
  WapTextStyle *ts;

  if(is_end)
    {
      g_assert(textfu->cur_card);

      paragraph_division(textfu, tag, FALSE, FALSE);
      style_change(textfu, tag, FALSE, TRUE, FALSE);

      g_list_foreach(textfu->cur_card->para_style_stack, (GFunc)g_free, NULL);
      g_list_free(textfu->cur_card->para_style_stack); textfu->cur_card->para_style_stack = NULL;

      g_list_foreach(textfu->cur_card->para_style_stack, (GFunc)wap_text_style_free, NULL);
      g_list_free(textfu->cur_card->text_style_stack); textfu->cur_card->text_style_stack = NULL;
    }

  card = g_new0(WapCard, 1);
  textfu->cur_card = card;

  textfu->cards = g_list_append(textfu->cards, card);

  ts = style_change(textfu, tag, TRUE, TRUE, FALSE);
  ts->font_size_set = TRUE;
  ts->font_size = NORMAL_FONT_SIZE;
  paragraph_division(textfu, tag, TRUE, FALSE);

  input = g_new0(WapInputInfo, 1);
  input->ivalue = -1;
  textfu->cur_card->inputs = g_list_append(textfu->cur_card->inputs, input);

  for(i = 0; attrs[i]; i++)
    {
      if(!strcasecmp(attrs[i], "name"))
	{
	  card->id = g_strdup(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "title"))
	{
	  card->title = g_strdup(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "format"))
	{
	  input->fmt = g_strdup(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "default"))
	{
	  input->value = g_strdup(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "emptyok"))
	{
	  input->emptyok = toboolean(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "maxlength"))
	{
	  input->maxlen = atoi(attrs[++i]);
	  continue;
	}
    }

  widget = input->maxlen?gtk_entry_new_with_max_length(input->maxlen):gtk_entry_new();
  gtk_object_set_data(GTK_OBJECT(widget), "PlacedItem", input->placed_item = make_placed_widget(textfu, widget, 1.0));
}

/* Dunno how to handle fieldset */

static void
tag_handler_go(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  if(is_end)
    {
      if(textfu->in_do)
	textfu->cur_do->go = textfu->cur_go;
      else if(textfu->in_anchor)
	{
	  textfu->cur_anchor->go_info = textfu->cur_go;
	  textfu->cur_anchor->type = ANCHOR_GO;
	}
      else
	g_assert_not_reached();

      textfu->cur_go = NULL;
    }
  else
    {
      int i;
      WapGoInfo *go;

      go = textfu->cur_go = g_new0(WapGoInfo, 1);
      for(i = 0; attrs[i]; i++)
	{
	  if(!strcasecmp(attrs[i], "method"))
	    {
	      if(!strcasecmp(attrs[++i], "post"))
		go->post_method = TRUE;
	      continue;
	    }

	  if(!strcasecmp(attrs[i], "sendreferer"))
	    {
	      go->sendreferer = toboolean(attrs[++i]);
	      continue;
	    }

	  if(!strcasecmp(attrs[i], "href"))
	    {
	      go->url = g_strdup(attrs[++i]);
	      continue;
	    }
	}
    }
}

static void
tag_handler_hdml(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  textfu->is_hdml = !is_end;
}

static void
tag_handler_i(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  return tag_handler_em(textfu, "em", tag_id, attrs, is_end);
}

static void
tag_handler_img(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  char *url = NULL, *alt = NULL;
  int i;
  GtkWidget *widget;
  double align = 0.0;

  if(is_end)
    return;

  for(i = 0; attrs[i]; i++)
    {
      if(!strcasecmp(attrs[i], "src"))
	{
	  url = g_strdup(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "alt"))
	{
	  alt = g_strdup(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "align"))
	{
	  i++;
	  if(!strcasecmp(attrs[i], "top"))
	    align = 0.0;
	  else if(!strcasecmp(attrs[i], "center"))
	    align = 0.5;
	  else
	    align = 1.0;
	}
    }

  widget = wap_pixmap_new_from_uri(textfu, url, alt);
  g_free(url);
  g_free(alt);

  if(textfu->in_option)
    textfu->cur_option->image = widget;
  else if(textfu->in_do)
    {
      textfu->cur_do->img = gtk_widget_ref(widget);
      /* gtk_object_sink(GTK_OBJECT(widget)); */
    }
  else
    gtk_object_set_data(GTK_OBJECT(widget), "PlacedItem", make_placed_widget(textfu, widget, align));
}

static void
tag_handler_input(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  int i;
  WapInputInfo *input;
  GtkWidget *widget;

  if(is_end)
    return;

  input = g_new0(WapInputInfo, 1);
  input->ivalue = -1;
  textfu->cur_card->inputs = g_list_append(textfu->cur_card->inputs, input);
  for(i = 0; attrs[i]; i++)
    {
      if(!strcasecmp(attrs[i], "name"))
	{
	  input->name = g_strdup(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "title"))
	{
	  input->title = g_strdup(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "value"))
	{
	  input->value = g_strdup(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "emptyok"))
	{
	  input->emptyok = toboolean(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "maxlength"))
	{
	  input->maxlen = atoi(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "format"))
	{
	  input->fmt = g_strdup(attrs[++i]);
	  continue;
	}
    }

  widget = input->maxlen?gtk_entry_new_with_max_length(input->maxlen):gtk_entry_new();
  gtk_object_set_data(GTK_OBJECT(widget), "PlacedItem", input->placed_item = make_placed_widget(textfu, widget, 1.0));
}

static void
tag_option_event_selected(GtkWidget *list_item, GtkObject *textfu)
{
  WapOptionInfo *woi = gtk_object_get_data(GTK_OBJECT(list_item), "WapOptionInfo");

  gtk_signal_emit(textfu, textfu_signals[LINK_FOLLOWED], woi->onpick);
}

static void
tag_handler_option(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  WapOptionInfo *option;

  if(is_end)
    {
      PlacedItem *pi;
      GtkWidget *mi, *b;

      pi = textfu->cur_input->placed_item;

      textfu->cur_option->text = g_strdup(textfu->nc_text->str);
      g_strstrip(textfu->cur_option->text);
      if(!strlen(textfu->cur_option->text))
	{
	  g_free(textfu->cur_option->text); textfu->cur_option->text = NULL;
	}

      if(textfu->cur_option->image)
	{
	  mi = gtk_list_item_new();
	  if(textfu->cur_option->text)
	    {
	      b = gtk_hbox_new(FALSE, 5);
	      gtk_container_add(GTK_CONTAINER(b), textfu->cur_option->image);
	      gtk_container_add(GTK_CONTAINER(b), gtk_label_new(textfu->cur_option->text));
	      gtk_container_add(GTK_CONTAINER(mi), b);
	    }
	  else
	    gtk_container_add(GTK_CONTAINER(mi), textfu->cur_option->image);
	}
      else
	mi = gtk_list_item_new_with_label(textfu->cur_option->text?textfu->cur_option->text:"");
      gtk_widget_show_all(mi);

      gtk_object_set_data(GTK_OBJECT(mi), "WapOptionInfo", textfu->cur_option);

      if(textfu->cur_option->onpick)
	gtk_signal_connect(GTK_OBJECT(mi), "select", GTK_SIGNAL_FUNC(tag_option_event_selected), textfu);

      textfu->cur_option->li_widget = mi;
      textfu->in_option = FALSE;
      textfu->cur_option = NULL;

      gtk_container_add(GTK_CONTAINER(pi->widget), mi);
      return;
    }

  g_assert(textfu->in_select);

  g_string_truncate(textfu->nc_text, 0);
  textfu->in_content = FALSE;
  textfu->in_option = TRUE;
  textfu->cur_option = option = g_new0(WapOptionInfo, 1);
  textfu->cur_input->options = g_list_append(textfu->cur_input->options, option);
  if(attrs)
    {
      int i;

      for(i = 0; attrs[i]; i++)
	{
	  if(!strcasecmp(attrs[i], "onpick"))
	    {
	      option->onpick = g_strdup(attrs[++i]);
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "value"))
	    {
	      option->value = g_strdup(attrs[++i]);
	      continue;
	    }
	}
    }
}

static void
tag_handler_p(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  WapParaStyle *ps;

  ps = paragraph_division(textfu, tag, !is_end, FALSE);

  if(ps && attrs)
    {
      int i;
      for(i = 0; attrs[i]; i++)
	{
	  if(!strcasecmp(attrs[i], "align"))
	    {
	      i++;
	      if(!strcasecmp(attrs[i], "left"))
		{
		  ps->alignment = PANGO_ALIGN_LEFT;
		}
	      else if(!strcasecmp(attrs[i], "center"))
		{
		  ps->alignment = PANGO_ALIGN_CENTER;
		}
	      else if(!strcasecmp(attrs[i], "right"))
		{
		  ps->alignment = PANGO_ALIGN_RIGHT;
		}
	    }
	}

      ps->space_after = PARA_SPACE_AFTER;
    }
}

static void
tag_handler_postfield(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  int i;
  char *name = NULL, *value = NULL;

  g_assert(textfu->cur_go);
  if(is_end)
    return;

  for(i = 0; attrs[i]; i++)
    {
      if(!strcasecmp(attrs[i], "name"))
	{
	  name = g_strdup(attrs[++i]);
	  continue;
	}
      if(!strcasecmp(attrs[i], "value"))
	{
	  value = g_strdup(attrs[++i]);
	  continue;
	}
    }

  textfu->cur_go->postvalues = g_list_prepend(textfu->cur_go->postvalues, value);
  textfu->cur_go->postvalues = g_list_prepend(textfu->cur_go->postvalues, name);
}

static void
tag_handler_prev(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  if(!is_end)
    {
      if(textfu->in_do)
	textfu->cur_do->type = DO_PREV;
      else if(textfu->in_anchor)
	textfu->cur_anchor->type = ANCHOR_PREV;
    }
}

static void
tag_handler_select(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  int i;
  WapInputInfo *input;
  GtkWidget *widget;

  if(is_end)
    {
      textfu->cur_input = NULL;
      textfu->in_content = TRUE;
      textfu->in_select = FALSE;
      return;
    }

  textfu->cur_input = input = g_new0(WapInputInfo, 1);
  textfu->cur_card->inputs = g_list_append(textfu->cur_card->inputs, input);
  if(attrs)
    {
      for(i = 0; attrs[i]; i++)
	{
	  if(!strcasecmp(attrs[i], "name"))
	    {
	      input->name = g_strdup(attrs[++i]);
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "title"))
	    {
	      input->title = g_strdup(attrs[++i]);
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "value"))
	    {
	      input->value = g_strdup(attrs[++i]);
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "multiple"))
	    {
	      input->multiple = toboolean(attrs[++i]);
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "ivalue"))
	    {
	      input->ivalue = atoi(attrs[++i]);
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "iname"))
	    {
	      input->iname = g_strdup(attrs[++i]);
	      continue;
	    }
	}
    }

  input->is_selection = TRUE;

  widget = gtk_list_new();
  gtk_list_set_selection_mode(GTK_LIST(widget), input->multiple?GTK_SELECTION_MULTIPLE:GTK_SELECTION_SINGLE);

  gtk_object_set_data(GTK_OBJECT(widget), "PlacedItem", input->placed_item = make_placed_widget(textfu, widget, 0.1));
  textfu->in_content = FALSE;
  textfu->in_select = TRUE;
}

static void
tag_handler_small(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  WapTextStyle *ts;

  ts = style_change(textfu, tag, !is_end, TRUE, FALSE);

  if(ts)
    {
      ts->font_size = SMALL_FONT_SIZE;
      ts->font_size_set = TRUE;
    }
}

static void
tag_handler_strong(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  return tag_handler_b(textfu, "b", tag_id, attrs, is_end);
}

/* The next three are very bad hacks to emulate tables... */
static void
tag_handler_table(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  push_pango_para(textfu);
}

static void
tag_handler_td(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  g_assert(textfu->in_content);

  if(is_end && !my_isspace(textfu->tmp_text->str[textfu->tmp_text->len - 1]))
    g_string_append_c(textfu->tmp_text, ' ');
}

static void
tag_handler_tr(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  g_assert(textfu->in_content);

  if(!is_end)
    push_pango_para(textfu);
}

static void
tag_handler_timer(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  g_assert(textfu->cur_card);

  if(!is_end && attrs)
    {
      WapTimerInfo *timer;
      int i;

      timer = g_new0(WapTimerInfo, 1);
      for(i = 0; attrs[i]; i++)
	{
	  if(!strcasecmp(attrs[i], "name"))
	    {
	      timer->name = g_strdup(attrs[++i]);
	      continue;
	    }
	  if(!strcasecmp(attrs[i], "name"))
	    {
	      timer->value = atoi(attrs[++i]);
	      continue;
	    }
	}

      textfu->cur_card->timers = g_list_prepend(textfu->cur_card->timers, timer);
    }
}

static void
tag_handler_u(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  WapTextStyle *ts;

  ts = style_change(textfu, tag, !is_end, TRUE, FALSE);

  if(ts)
    {
      ts->underline = TEXTFU_TRUE;
    }
}

static void
tag_handler_wml(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end)
{
  if(is_end)
    {
      textfu->cur_card = NULL;
    }
  else
    {
      int i;

      g_free(textfu->lang); textfu->lang = NULL;

      if(attrs)
	for(i = 0; attrs[i]; i++)
	  {
	    if(!strcasecmp(attrs[i], "xml:lang"))
	      {
		textfu->lang = g_strdup(attrs[++i]);
		pango_context_set_lang(textfu->pango_ctx, textfu->lang);
		continue;
	      }
	  }
      if(!textfu->lang)
	pango_context_set_lang(textfu->pango_ctx, "en");
    }
}

/* Image loading */
typedef struct {
  WapTextFu *textfu;
  GtkWidget *pmap;
  GdkPixbufLoader *loader;
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;
} PixmapLoaderInfo;

#if 0
static void wap_pixmap_prepared(GdkPixbufLoader *loader,
				PixmapLoaderInfo *pli)
{
  GdkPixbuf *pb;
  GdkPixmap *pmap = NULL;
  GdkBitmap *mask = NULL;

  g_assert(loader == pli->loader);
  pb = gdk_pixbuf_loader_get_pixbuf(loader);
  g_assert(pb);

  gdk_pixbuf_render_pixmap_and_mask(pb, &pmap, &mask, 127);

  gtk_pixmap_set(GTK_PIXMAP(pli->pmap), pmap, mask);
  pli->pixmap = pmap;
  pli->bitmap = mask;
}

static void wap_pixmap_updated(GdkPixbufLoader *loader,
			       guint            x,
			       guint            y,
			       guint            width,
			       guint            height,
			       PixmapLoaderInfo *pli)
{
  GdkPixbuf *pb;

  pb = gdk_pixbuf_loader_get_pixbuf(pli->loader);
  gdk_pixbuf_render_to_drawable_alpha(pb, pli->pixmap, 0, 0, 0, 0,
				      gdk_pixbuf_get_width(pb),
				      gdk_pixbuf_get_height(pb),
				      GDK_PIXBUF_ALPHA_BILEVEL, 127, GDK_RGB_DITHER_NORMAL, 0, 0);
}

static void wap_pixmap_closed(GdkPixbufLoader *loader,
			      PixmapLoaderInfo *pli)
{
  wap_pixmap_prepared(loader, pli); /* Do it all again */
}
#endif

#if 0
static void wap_pixmap_start(WapStreamHandle handle, const char *mime_type, gpointer user_data)
{
  PixmapLoaderInfo *pli = user_data;

  if(mime_type)
    {
      char *ctmp;
      int n;

      n = strlen("image/");
      if(strncmp(mime_type, "image/", n))
	return;

      mime_type += n;

      ctmp = strrchr(mime_type, '.');
      if(ctmp)
	mime_type = ctmp+1;
      if(!strncmp(mime_type, "x-", 2))
	mime_type += 2;

      pli->loader = gdk_pixbuf_loader_new_with_type(mime_type);
    }
  else
    pli->loader = gdk_pixbuf_loader_new();

  gtk_signal_connect(GTK_OBJECT(pli->loader), "area_prepared", wap_pixmap_prepared, pli);
  gtk_signal_connect(GTK_OBJECT(pli->loader), "area_updated", wap_pixmap_updated, pli);
  gtk_signal_connect(GTK_OBJECT(pli->loader), "closed", wap_pixmap_closed, pli);
}

static void wap_pixmap_end(WapStreamHandle handle, WapStreamStatus status, gpointer user_data)
{
  PixmapLoaderInfo *pli = user_data;

  gtk_widget_queue_resize(GTK_WIDGET(pli->textfu));
  if(pli->loader)
    {
      gdk_pixbuf_loader_close(pli->loader);
      g_object_unref(G_OBJECT(pli->loader));
      pli->loader = NULL;
    }
  if(pli->pmap)
    {
      gtk_widget_unref(pli->pmap);
      pli->pmap = NULL;
    }

  g_free(pli);
}

static void wap_pixmap_write(WapStreamHandle handle, const guchar *buffer, size_t size, gpointer user_data)
{
  PixmapLoaderInfo *pli = user_data;

  if(!pli->loader)
    {
      g_warning("Don't have a loader for this image!");
      return;
    }

  gdk_pixbuf_loader_write(pli->loader, buffer, size);
}
#endif

static GtkWidget *
wap_pixmap_new_from_uri(WapTextFu *textfu, char *uri, const char *alt_text)
{
  GtkWidget *retval;
  GdkPixmap *pmap;
  PixmapLoaderInfo *pli;
  int n;

  if(uri[0] == '"')
    uri++;
  n = strlen(uri) - 1;
  if(uri[n] == '"')
    uri[n] = '\0';
  pmap = gdk_pixmap_new(NULL, 1, 1, 24); /* GDK_ROOT_PARENT () */
  retval = gtk_pixmap_new(pmap, NULL);
  gtk_widget_ref(retval);
  gdk_pixmap_unref(pmap);

  pli = g_new0(PixmapLoaderInfo, 1);
  pli->textfu = textfu;
  pli->pmap = retval;
  pli->loader = NULL;

  return retval;
}

GtkType
wap_textfu_get_type (void)
{
  static GtkType textfu_type = 0;

  if (!textfu_type)
    {
      static const GtkTypeInfo textfu_info =
      {
        "WapTextFu",
        sizeof (WapTextFu),
        sizeof (WapTextFuClass),
        (GtkClassInitFunc) wap_textfu_class_init,
        (GtkObjectInitFunc) wap_textfu_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      textfu_type = gtk_type_unique (gtk_layout_get_type (), &textfu_info);
    }

  return textfu_type;
}

static void
wap_textfu_class_init (WapTextFuClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkLayoutClass *layout_class;

  link_url_attr_class.type = pango_attr_type_register("link_url");

  object_class = (GtkObjectClass*) klass;

  widget_class = (GtkWidgetClass*) klass;

  layout_class = (GtkLayoutClass*) klass;

  parent_class = gtk_type_class(g_type_parent(GTK_CLASS_TYPE(object_class)));

  textfu_signals[LINK_FOLLOWED] = 
    gtk_signal_new ("link_followed",
                    GTK_RUN_FIRST,
                    GTK_CLASS_TYPE (object_class),
                    GTK_SIGNAL_OFFSET (WapTextFuClass, link_followed),
                    gtk_marshal_VOID__STRING,
                    GTK_TYPE_NONE, 1, GTK_TYPE_STRING);
  textfu_signals[ANCHOR_FOLLOWED] = 
    gtk_signal_new ("anchor_followed",
                    GTK_RUN_FIRST,
                    GTK_CLASS_TYPE (object_class),
                    GTK_SIGNAL_OFFSET (WapTextFuClass, anchor_followed),
                    gtk_marshal_VOID__POINTER,
                    GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
  textfu_signals[PREV_FOLLOWED] = 
    gtk_signal_new ("prev_followed",
                    GTK_RUN_FIRST,
                    GTK_CLASS_TYPE (object_class),
                    GTK_SIGNAL_OFFSET (WapTextFuClass, prev_followed),
                    gtk_marshal_VOID__VOID,
                    GTK_TYPE_NONE, 0);
  textfu_signals[LOAD_DONE] = 
    gtk_signal_new ("load_done",
                    GTK_RUN_FIRST,
                    GTK_CLASS_TYPE (object_class),
                    GTK_SIGNAL_OFFSET (WapTextFuClass, load_done),
                    gtk_marshal_VOID__VOID,
                    GTK_TYPE_NONE, 0);
  textfu_signals[LOAD_ERROR] = 
    gtk_signal_new ("load_error",
                    GTK_RUN_FIRST,
                    GTK_CLASS_TYPE (object_class),
                    GTK_SIGNAL_OFFSET (WapTextFuClass, load_error),
                    gtk_marshal_VOID__VOID,
                    GTK_TYPE_NONE, 0);
  textfu_signals[ACTIVATE_CARD] = 
    gtk_signal_new ("activate_card",
                    GTK_RUN_FIRST,
                    GTK_CLASS_TYPE (object_class),
                    GTK_SIGNAL_OFFSET (WapTextFuClass, activate_card),
                    gtk_marshal_VOID__POINTER,
                    GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);
  textfu_signals[DEACTIVATE_CARD] = 
    gtk_signal_new ("deactivate_card",
                    GTK_RUN_FIRST,
                    GTK_CLASS_TYPE (object_class),
                    GTK_SIGNAL_OFFSET (WapTextFuClass, deactivate_card),
                    gtk_marshal_VOID__POINTER,
                    GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);


  klass->tag_handlers = g_hash_table_new(g_str_hash, g_str_equal);

#define IGNORE_TAG(x) wap_textfu_tagid_alloc(#x, tag_handler_ignore)
#define ADD_HANDLER(x) wap_textfu_tagid_alloc(#x, tag_handler_##x)
  ADD_HANDLER(a);
  ADD_HANDLER(anchor);
  ADD_HANDLER(b);
  ADD_HANDLER(big);
  ADD_HANDLER(br);
  ADD_HANDLER(card);
  ADD_HANDLER(do);
  ADD_HANDLER(em);
  ADD_HANDLER(go);
  ADD_HANDLER(i);
  ADD_HANDLER(img);
  ADD_HANDLER(input);
  ADD_HANDLER(option);
  ADD_HANDLER(p);
  ADD_HANDLER(postfield);
  ADD_HANDLER(prev);
  ADD_HANDLER(select);
  ADD_HANDLER(small);
  ADD_HANDLER(strong);
  ADD_HANDLER(timer);
  ADD_HANDLER(table);
  ADD_HANDLER(tr);
  ADD_HANDLER(td);
  ADD_HANDLER(u);
  ADD_HANDLER(wml);

  /* HDML */
  /* HDML <a> tag is handled too as part of wml handler */
  ADD_HANDLER(action);
  ADD_HANDLER(ce);
  ADD_HANDLER(choice);
  ADD_HANDLER(display);
  ADD_HANDLER(entry);
  ADD_HANDLER(hdml);

  IGNORE_TAG(head);
  IGNORE_TAG(template);

  widget_class->realize = wap_textfu_realize;
  widget_class->unrealize = wap_textfu_unrealize;
  widget_class->size_request = wap_textfu_size_request;
  widget_class->size_allocate = wap_textfu_size_allocate;
  widget_class->expose_event = wap_textfu_expose;
  widget_class->map = wap_textfu_map;
  widget_class->motion_notify_event = wap_textfu_motion_notify_event;
  widget_class->button_release_event = wap_textfu_button_release_event;
  widget_class->key_press_event = wap_textfu_key_press_event;

  layout_class->set_scroll_adjustments = wap_textfu_set_scroll_adjustments;
}

static void
wap_textfu_init (WapTextFu *textfu)
{
  GTK_WIDGET_SET_FLAGS(GTK_WIDGET(textfu), GTK_CAN_FOCUS);
  
  gtk_widget_add_events(GTK_WIDGET(textfu),
			GDK_EXPOSURE_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK
			|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_MOTION_MASK
			|GDK_KEY_PRESS_MASK|GDK_KEY_RELEASE_MASK);
  textfu->tmp_text = g_string_new(NULL);
  textfu->nc_text = g_string_new(NULL);
  textfu->pango_ctx = gdk_pango_context_get();
}

GtkWidget *
wap_textfu_new(void)
{
  GtkWidget *retval;

  retval = gtk_widget_new(wap_textfu_get_type(), NULL);
  gtk_layout_set_hadjustment(GTK_LAYOUT(retval), NULL);
  gtk_layout_set_vadjustment(GTK_LAYOUT(retval), NULL);

  return retval;
}

static void
wap_textfu_map(GtkWidget *widget)
{
  if(parent_class->map)
    parent_class->map(widget);
}

static void
wap_textfu_realize(GtkWidget      *widget)
{
  WapTextFu *textfu = WAP_TEXTFU(widget);
  GdkColor colr_white, colr_black;

  if(parent_class->realize)
    parent_class->realize(widget);

  gdk_color_white(gdk_rgb_get_cmap(), &colr_white);
  gdk_color_black(gdk_rgb_get_cmap(), &colr_black);
  gdk_window_set_background(widget->window, &colr_white);
  gdk_window_set_background(GTK_LAYOUT(widget)->bin_window, &colr_white);

  textfu->drawing_gc = gdk_gc_new(GTK_LAYOUT(widget)->bin_window);
  gdk_gc_copy(textfu->drawing_gc, widget->style->fg_gc[GTK_STATE_NORMAL]);
  gdk_gc_set_foreground(textfu->drawing_gc, &colr_black);
}

static void
wap_textfu_unrealize(GtkWidget      *widget)
{
  WapTextFu *textfu = WAP_TEXTFU(widget);

  if(parent_class->unrealize)
    parent_class->unrealize(widget);

  gdk_gc_unref(textfu->drawing_gc);
}

/* Multipurpose - either gets bounding box or does a redraw */
static void
wap_textfu_get_anchor_vbounds(WapTextFu *textfu, WapParagraphInfo *para, WapAnchorInfo *wai, int *starty, int *endy)
{
  PangoAttribute *attr;
  PangoRectangle r;
  int y1, y2;

  attr = wai->attr;

  pango_layout_index_to_pos(para->layout, attr->start_index, &r);
  y1 = r.y;
  y2 = r.y+r.height;

  pango_layout_index_to_pos(para->layout, attr->end_index, &r);
  y1 = MIN(y1, r.y);
  y2 = MAX(y2, r.y+r.height);

  if(starty)
    *starty = PANGO_PIXELS(y1) + para->y_offset;
  if(endy)
    *endy = PANGO_PIXELS(y2) + para->y_offset;
}

static void
wap_textfu_redraw_anchor(WapTextFu *textfu, WapParagraphInfo *para, WapAnchorInfo *wai)
{
  PangoAttribute *attr;
  PangoRectangle r;
  GSList *lines;
  int i, n;

  attr = wai->attr;

  /* This is essentially the hypothetical pango_layout_range_get_extents() with some redrawing code thrown in */
  for(i = n = 0, lines = pango_layout_get_lines(para->layout); lines && i < attr->end_index; lines = lines->next, n++)
    {
      PangoLayoutLine *line = lines->data;

      if((i + line->length) > attr->start_index
	 && i < attr->end_index)
	{
	  GdkRectangle redrawrect;
	  GdkSegment seg;
	  int line_start_idx, line_end_idx;

	  line_start_idx = MAX(i, attr->start_index);
	  pango_layout_index_to_pos(para->layout, line_start_idx, &r);
	  seg.x1 = r.x;
	  seg.y1 = r.y;
	  seg.x2 = r.x+r.width;
	  seg.y2 = r.y+r.height;

	  line_end_idx = MIN(i + line->length - 1, attr->end_index);
	  pango_layout_index_to_pos(para->layout, line_end_idx, &r);
	  seg.x1 = MIN(seg.x1, r.x);
	  seg.y1 = MIN(seg.y1, r.y);
	  seg.x2 = MAX(seg.x2, r.x+r.width);
	  seg.y2 = MAX(seg.y2, r.y+r.height);

	  redrawrect.x = PANGO_PIXELS(seg.x1) + para->left_indent;
	  redrawrect.y = PANGO_PIXELS(seg.y1) + para->y_offset;
	  redrawrect.width = PANGO_PIXELS(seg.x2 - seg.x1);
	  redrawrect.height = PANGO_PIXELS(seg.y2 - seg.y1);

	  gdk_window_invalidate_rect(GTK_LAYOUT(textfu)->bin_window, &redrawrect, TRUE);
	}

      i += line->length;
    }
}

static void
wap_textfu_unselect_anchor(WapTextFu *textfu)
{
  WapAnchorInfo *wai;
  LinkURLAttr *attr;
  PangoAttrColor *bgcolor, *fgcolor;

  g_return_if_fail(textfu->active_card->active_anchor);

  wai = textfu->active_card->active_anchor;
  attr = wai->attr;
  bgcolor = (PangoAttrColor *)attr->bg_color_attr;
  bgcolor->color.red = bgcolor->color.green = bgcolor->color.blue = 65535;

  fgcolor = (PangoAttrColor *)attr->fg_color_attr;
  fgcolor->color.red = fgcolor->color.green = 0;
  fgcolor->color.blue = 65535;

  textfu->active_card->active_anchor = NULL;
  pango_layout_set_attributes(textfu->active_card->active_anchor_para->layout, textfu->active_card->active_anchor_para->attrs);
  wap_textfu_redraw_anchor(textfu, textfu->active_card->active_anchor_para, wai);
}

static void
wap_textfu_select_anchor(WapTextFu *textfu, WapParagraphInfo *para, WapAnchorInfo *wai)
{
  LinkURLAttr *attr;
  PangoAttrColor *bgcolor, *fgcolor;

  if(wai == textfu->active_card->active_anchor)
    return;

  if(textfu->active_card->active_anchor)
    wap_textfu_unselect_anchor(textfu);

  attr = wai->attr;
  bgcolor = (PangoAttrColor *)attr->bg_color_attr;
  bgcolor->color.red = bgcolor->color.green = 0; bgcolor->color.blue = 65535;

  fgcolor = (PangoAttrColor *)attr->fg_color_attr;
  fgcolor->color.red = fgcolor->color.green = fgcolor->color.blue = 65535;

  textfu->active_card->active_anchor = wai;
  textfu->active_card->active_anchor_para = para;
  pango_layout_set_attributes(textfu->active_card->active_anchor_para->layout, textfu->active_card->active_anchor_para->attrs);
  wap_textfu_redraw_anchor(textfu, textfu->active_card->active_anchor_para, wai);
}

static void
wap_textfu_activate_anchor(WapTextFu *textfu, WapAnchorInfo *wai)
{
  gtk_signal_emit(GTK_OBJECT(textfu), textfu_signals[ANCHOR_FOLLOWED], wai);
}

static void
wap_textfu_find_anchor(WapTextFu *textfu, gint evx, gint evy, WapAnchorInfo **wai, WapParagraphInfo **para)
{
  GList *ltmp;
  int y_sum = BASIC_INDENT;
  GtkLayout *layout;
  int event_x, event_y, px, py;
  gboolean trailing;
  int idx;

  layout = GTK_LAYOUT(textfu);
  event_x = evx + layout->xoffset;
  event_y = evy + layout->yoffset;

  for(*wai = NULL, *para = NULL, ltmp = textfu->active_card->paragraphs; y_sum < event_y && ltmp; ltmp = ltmp->next)
    {
      WapParagraphInfo *check_para = ltmp->data;

      if(event_y < (check_para->y_offset + check_para->height) && event_y >= y_sum)
	{
	  *para = check_para;
	  break;
	}

      y_sum = check_para->y_offset + check_para->height + check_para->space_after;
    }

  if(!*para)
    return;

  /* Now, para is the paragraph that was clicked on and y_sum is the offset of the start of the paragraph */
  trailing = FALSE;
  px = PANGO_SCALE*(event_x - (*para)->left_indent);
  py = PANGO_SCALE*(event_y - (*para)->y_offset);
  if(px < 0 || !pango_layout_xy_to_index((*para)->layout, px, py, &idx, &trailing))
    return;

  g_print("pango says the event [%d, %d] is at index %d of paragraph %p\n", px, py, idx, *para);

  {
    PangoAttrIterator *iter;
    PangoAttribute *attr = NULL;
    int rstart, rend;

    iter = pango_attr_list_get_iterator((*para)->attrs);
    rstart = rend = -1;
    for(pango_attr_iterator_range(iter, &rstart, &rend);
	(idx > rend || idx < rstart) && pango_attr_iterator_next(iter);
	pango_attr_iterator_range(iter, &rstart, &rend));

    if(idx >= rstart && idx <= rend)
      attr = pango_attr_iterator_get(iter, link_url_attr_class.type);

    pango_attr_iterator_destroy(iter);

    if(attr)
      {
	LinkURLAttr *lua;
	lua = (LinkURLAttr *)attr;
	*wai = lua->wai;
      }
  }
}

static gint
wap_textfu_button_release_event(GtkWidget *widget, GdkEventButton *event)
{
  WapTextFu *textfu = WAP_TEXTFU(widget);
  WapAnchorInfo *wai;
  WapParagraphInfo *para;

  if(event->window != GTK_LAYOUT(widget)->bin_window
     || !textfu->active_card
     || event->type != GDK_BUTTON_RELEASE)
    goto default_handling;

  wap_textfu_find_anchor(textfu, event->x, event->y, &wai, &para);
  if(wai)
    {
      wap_textfu_select_anchor(textfu, para, wai);
      wap_textfu_activate_anchor(textfu, wai);
    }

 default_handling:
  if(parent_class->button_release_event)
    return parent_class->button_release_event(widget, event);
  else
    return wai?TRUE:FALSE;
}

static gint
wap_textfu_motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
  if(event->window != GTK_LAYOUT(widget)->bin_window)
    goto default_handling;

  /* Don't handle mouseovers for now... */
#if 0
  gpointer info = NULL;
  if((info && TRUE) != textfu->linkpoint_cursor)
    {
      GdkCursor *set_new_cursor;

      textfu->linkpoint_cursor = info && TRUE;

      if(info)
	{
	  if(!cursor_hand)
	    cursor_hand = /* wap_stock_cursor_new(WAP_STOCK_CURSOR_POINTING_HAND) */ NULL;
	  set_new_cursor = cursor_hand;
	}
      else
	set_new_cursor = NULL;

      gdk_window_set_cursor(GTK_LAYOUT(widget)->bin_window, set_new_cursor);
    }
#endif

 default_handling:
  if(parent_class->motion_notify_event)
    return parent_class->motion_notify_event(widget, event);
  else
    return FALSE;
}

static GList *
wap_textfu_paragraphs_in_viewport(WapTextFu *textfu)
{
  GList *ltmp, *retval;
  int vstart, vend, y;

  y = GTK_LAYOUT(textfu)->vadjustment->value;
  vstart = y - MAX_SCROLL_AMT;
  vend = y + GTK_WIDGET(textfu)->allocation.height + MAX_SCROLL_AMT;

  for(retval = NULL, ltmp = textfu->active_card->paragraphs; ltmp; ltmp = ltmp->next)
    {
      WapParagraphInfo *para = ltmp->data;

      if((para->y_offset + para->height) < vstart)
	continue;
      if(para->y_offset > vend)
	break;

      retval = g_list_append(retval, para);
    }

  return retval;
}

static void
wap_textfu_pick_anchor(WapTextFu *textfu, gboolean is_next)
{
  GList *ltmp = NULL, *piv = NULL;
  WapAnchorInfo *wai = NULL;
  WapParagraphInfo *para = NULL;
  gint scroll_amount = 0;
  GList *ltmp2 = NULL;
  int y1, y2;
  int vstart, vend;

  vstart = vend = GTK_LAYOUT(textfu)->vadjustment->value;
  vend += GTK_WIDGET(textfu)->allocation.height;

  piv = wap_textfu_paragraphs_in_viewport(textfu);

  if(textfu->active_card->active_anchor)
    {
      ltmp = g_list_find(piv, textfu->active_card->active_anchor_para);
      if(ltmp)
	{
	  ltmp2 = g_list_find(textfu->active_card->active_anchor_para->anchors, textfu->active_card->active_anchor);
	  if(ltmp2)
	    ltmp2 = is_next?ltmp2->next:ltmp2->prev;
	  if(!ltmp2)
	    ltmp = is_next?ltmp->next:ltmp->prev;
	  if(!ltmp)
	    goto out; /* nothing prev/next to go to - just scroll things */
	}
    }

    /* If user presses 'up' w/o selected link then we pick the last visible link on the page */

  if(!ltmp)
    {
      if(is_next)
	ltmp = piv;
      else
	ltmp = g_list_last(piv);
    }

  for(; ltmp; ltmp = is_next?ltmp->next:ltmp->prev)
    {
      WapParagraphInfo *check_para = ltmp->data;

      if(!ltmp2)
	ltmp2 = is_next?check_para->anchors:g_list_last(check_para->anchors);

      for(; ltmp2; ltmp2 = is_next?ltmp2->next:ltmp2->prev)
	{
	  WapAnchorInfo *check_wai = ltmp2->data;

	  wap_textfu_get_anchor_vbounds(textfu, check_para, check_wai, &y1, &y2);

	  if((is_next && (y1 > (vstart - MAX_SCROLL_AMT)))
	     || (!is_next && (y2 < (vend+MAX_SCROLL_AMT))))
	    {
	      para = check_para;
	      wai = check_wai;
	      goto out;
	    }
	}
    }
 out:

  if(wai)
    {
      if(is_next)
	scroll_amount = MAX(y2 - vend, 0);
      else
	scroll_amount = MIN(y1 - vstart, 0);
    }
  if(!wai || abs(scroll_amount) > MAX_SCROLL_AMT)
    {
      scroll_amount = is_next?MAX_SCROLL_AMT:-MAX_SCROLL_AMT;
      wai = NULL;
    }

  if(scroll_amount)
    gtk_adjustment_set_value(GTK_LAYOUT(textfu)->vadjustment,
			     CLAMP(GTK_LAYOUT(textfu)->vadjustment->value + scroll_amount,
				   GTK_LAYOUT(textfu)->vadjustment->lower,
				   GTK_LAYOUT(textfu)->vadjustment->upper - GTK_WIDGET(textfu)->allocation.height));

  if(wai)
    wap_textfu_select_anchor(textfu, para, wai);

  g_list_free(piv);
}

static void
wap_textfu_handle_external_scrolling(GtkAdjustment *adjustment, WapTextFu *textfu)
{
  /* If someone uses scroll bars to move the view we need to handle the interaction with the active_anchor stuff.
     For now, just deselect it if it is offscreen. */

  if(textfu->active_card
     && textfu->active_card->active_anchor)
    {
      int y1, y2;

      wap_textfu_get_anchor_vbounds(textfu, textfu->active_card->active_anchor_para,
				   textfu->active_card->active_anchor, &y1, &y2);

      if(y2 < GTK_LAYOUT(textfu)->vadjustment->value
	 || y1 > (GTK_LAYOUT(textfu)->vadjustment->value + GTK_WIDGET(textfu)->allocation.height))
	wap_textfu_unselect_anchor(textfu);
    }
}

static gint
wap_textfu_key_press_event(GtkWidget *widget, GdkEventKey *event)
{
  WapTextFu *textfu = textfu = WAP_TEXTFU(widget);
  gboolean gotit = FALSE;

  if(!textfu->active_card)
    goto default_handling;

  gotit = TRUE;
  /* We basically handle scrolling keys and Enter */
  switch(event->keyval)
    {
    case GDK_Return:
    case GDK_KP_Enter:
    case GDK_Right:
    case GDK_KP_Right:
      if(textfu->active_card->active_anchor)
	wap_textfu_activate_anchor(textfu, textfu->active_card->active_anchor);
      break;
    case GDK_Left:
    case GDK_KP_Left:
      gtk_signal_emit(GTK_OBJECT(textfu), textfu_signals[PREV_FOLLOWED]);
      break;
    case GDK_Up:
    case GDK_KP_Up:
      wap_textfu_pick_anchor(textfu, FALSE);
      break;
    case GDK_Down:
    case GDK_KP_Down:
      wap_textfu_pick_anchor(textfu, TRUE);
      break;
    default:
      gotit = FALSE;
      break;
    }

 default_handling:
  if(!gotit && parent_class->key_press_event)
    return parent_class->key_press_event(widget, event);
  else
    return gotit;
}

static void
wap_textfu_size_request(GtkWidget      *widget,
			  GtkRequisition *requisition)
{
  WapTextFu *textfu = textfu = WAP_TEXTFU(widget);

  if(parent_class->size_request)
    parent_class->size_request(widget, requisition);
}

static void
wap_textfu_redraw(GtkWidget *widget)
{
  GdkRectangle redraw_me;

  if(!GTK_LAYOUT(widget)->bin_window)
    return;

  redraw_me.x = GTK_LAYOUT(widget)->hadjustment->value;
  redraw_me.y = GTK_LAYOUT(widget)->vadjustment->value;
  redraw_me.width = widget->allocation.width;
  redraw_me.height = widget->allocation.height;
  gdk_window_invalidate_rect(GTK_LAYOUT(widget)->bin_window, &redraw_me, TRUE);
}

static void
wap_textfu_size_allocate(GtkWidget      *widget,
			 GtkAllocation  *allocation)
{
  WapTextFu *textfu;

  textfu = WAP_TEXTFU(widget);

  if(allocation->x == widget->allocation.x
     && allocation->y == widget->allocation.y
     && allocation->width == widget->allocation.width
     && allocation->height == widget->allocation.height
     && textfu->old_width == allocation->width)
    return; /* Nothing to do here */

  if(parent_class->size_allocate)
    parent_class->size_allocate(widget, allocation);

  if(textfu->old_width != allocation->width)
    {
      /* Relayout */
      gint fu_height = BASIC_INDENT;
      GList *ltmp;
      int n;

      if(textfu->active_card)
	for(n = 0, ltmp = textfu->active_card->paragraphs; ltmp; ltmp = ltmp->next, n++)
	  {
	    WapParagraphInfo *para = ltmp->data;
	    int pwid, pheight;
	    /* Move old placed items off-screen */
	    GList *ltmp2;

	    for(ltmp2 = para->placed_items; ltmp2; ltmp2 = ltmp2->next)
	      {
		PlacedItem *pl = ltmp2->data;
		PangoAttrShape *sa;

		sa = (PangoAttrShape *)pl->shape_attr;
		sa->ink_rect.width = sa->logical_rect.width = pl->widget->allocation.width * PANGO_SCALE;
		sa->ink_rect.height = sa->logical_rect.height = pl->widget->allocation.height * PANGO_SCALE;
		sa->ink_rect.y = sa->logical_rect.y = -(sa->ink_rect.height * pl->alignment);
		sa->ink_rect.x = sa->logical_rect.x = 0;
	      }

	    pango_layout_set_width(para->layout, (allocation->width - (para->left_indent+para->right_indent))*PANGO_SCALE);

	    pango_layout_get_pixel_size(para->layout, &pwid, &pheight);

	    for(ltmp2 = para->placed_items; ltmp2; ltmp2 = ltmp2->next)
	      {
		PlacedItem *pl = ltmp2->data;
		PangoRectangle rect;

		pango_layout_index_to_pos(para->layout, pl->shape_attr->start_index, &rect);
		rect.x = PANGO_PIXELS(rect.x);
		rect.y = PANGO_PIXELS(rect.y);
		rect.width = PANGO_PIXELS(rect.width);
		rect.height = PANGO_PIXELS(rect.height);
		g_print("moving %p to [%d, %d]\n",
			pl->widget,
			rect.x + para->left_indent,
			rect.y + fu_height);
		gtk_layout_move(GTK_LAYOUT(textfu), pl->widget, rect.x + para->left_indent, rect.y + fu_height);
	      }

	    para->y_offset = fu_height;
	    para->height = pheight;

	    fu_height += pheight + para->space_after;
	  }
      fu_height += BASIC_INDENT;

      gtk_layout_set_size(GTK_LAYOUT(widget), GTK_LAYOUT(widget)->width, fu_height);
      textfu->old_width = allocation->width;
      wap_textfu_redraw(widget);
    }
}

static void
wap_textfu_set_scroll_adjustments(GtkLayout      *layout,
				  GtkAdjustment  *hadjustment,
				  GtkAdjustment  *vadjustment)
{
  WapTextFu *textfu = WAP_TEXTFU(layout);

  if(textfu->vscroll_tag)
    gtk_signal_disconnect(GTK_OBJECT(layout->vadjustment), textfu->vscroll_tag);

  textfu->vscroll_tag = gtk_signal_connect(GTK_OBJECT(vadjustment), "value_changed",
					   GTK_SIGNAL_FUNC(wap_textfu_handle_external_scrolling),
					   textfu);

  if(GTK_LAYOUT_CLASS(parent_class)->set_scroll_adjustments)
    GTK_LAYOUT_CLASS(parent_class)->set_scroll_adjustments(layout, hadjustment, vadjustment);
}

static void
wap_textfu_draw(GtkWidget      *widget, 
		GdkRectangle   *area)
{
  GList *ltmp;
  int y;
  WapTextFu *textfu;
  GtkLayout *layout;
  int view_start, view_end;
  GdkRectangle myrect;
  int n;

  textfu = WAP_TEXTFU(widget);
  layout = GTK_LAYOUT(widget);

  if(!textfu->active_card
     || textfu->old_width != widget->allocation.width)
    return;

  myrect = *area;
  myrect.x += layout->xoffset;
  myrect.y += layout->yoffset;
  gdk_gc_set_clip_rectangle(textfu->drawing_gc, &myrect);

  view_start = area->y + layout->yoffset;
  view_end = area->y + area->height + layout->yoffset;

  for(n = 0, y = BASIC_INDENT, ltmp = textfu->active_card->paragraphs; ltmp && y < view_end; ltmp = ltmp->next, n++)
    {
      WapParagraphInfo *para = ltmp->data;

      y = para->y_offset;
      if((y + para->height) >= view_start)
	 gdk_draw_layout(GTK_LAYOUT(widget)->bin_window, textfu->drawing_gc,
			 para->left_indent + layout->xoffset, y + layout->yoffset,
			 para->layout);

      y = para->y_offset + para->height + para->space_after;
    }
}

static gint
wap_textfu_expose(GtkWidget      *widget, 
		  GdkEventExpose *event)
{
  if(parent_class->expose_event)
    parent_class->expose_event(widget, event);

  if(event->window == GTK_LAYOUT(widget)->bin_window)
    wap_textfu_draw(widget, &event->area);

  return TRUE;
}

/* libxml handlers  */

static void
wml_startDocument(void *ctx)
{
}

static void
wml_endDocument(void *ctx)
{
}

static void
wml_startElement(void *ctx, const xmlChar *name,
		 const xmlChar **atts)
{
  WapTextFu *textfu = WAP_TEXTFU(ctx);
  TagRegistration *tr;
  WapTextFuClass *klass;
  gchar *lowercase_name;

  lowercase_name = g_strdup (name);
  g_strdown(lowercase_name);
  klass = WAP_TEXTFU_CLASS(gtk_type_class(wap_textfu_get_type()));

  tr = g_hash_table_lookup(klass->tag_handlers, lowercase_name);
  g_free(lowercase_name);
  if(!tr)
    return;

  if(textfu->ignore_counter > 0
     && tr->handler != tag_handler_ignore)
    return;

  tr->handler(textfu, name, tr->tag_id, (char **)atts, FALSE);
}

static void
wml_endElement(void *ctx, const xmlChar *name)
{
  WapTextFu *textfu = WAP_TEXTFU(ctx);
  TagRegistration *tr;
  WapTextFuClass *klass;
  gchar *lowercase_name;

  lowercase_name = g_strdup (name);
  g_strdown(lowercase_name);
  klass = WAP_TEXTFU_CLASS(gtk_type_class(wap_textfu_get_type()));

  tr = g_hash_table_lookup(klass->tag_handlers, lowercase_name);
  g_free(lowercase_name);
  if(!tr)
     return;

  if(textfu->ignore_counter > 0
     && tr->handler != tag_handler_ignore)
    return;

    tr->handler(textfu, name, tr->tag_id, NULL, TRUE);
}

static void
wml_characters(void *ctx, const xmlChar *ch,
	       int len)
{
  WapTextFu *textfu = WAP_TEXTFU(ctx);
  GString *str;
  gboolean head_space = FALSE, tail_space = FALSE;
  gboolean do_initiate = FALSE;

  if(textfu->ignore_counter > 0)
    return;

  if(textfu->in_content)
    {
      do_initiate = TRUE;

      str = textfu->tmp_text;
    }
  else
    {
      str = textfu->nc_text;
    }

  while(len > 0 && my_isspace(*ch))
    {
      ch++;
      len--;
      head_space = TRUE;
    }
  while(len > 0 && my_isspace(ch[len-1]))
    {
      len--;
      tail_space = TRUE;
    }

  if(len > 0)
    {
      int i;
      gboolean prev_was_space = FALSE;

      if(do_initiate)
	initiate_para(textfu);

      if(str->len > 0
	 && !my_isspace(str->str[str->len-1])
	 && head_space)
	g_string_append_c(str, ' ');

      /* XXX fixme slow */
      for(i = 0; i < len; i++)
	{
	  gboolean this_is_space = my_isspace(ch[i]);
	  char nc;

	  if(this_is_space)
	    nc = ' ';
	  else
	    nc = ch[i];

	  if(!(this_is_space && prev_was_space))
	    g_string_append_c(str, nc);

	  prev_was_space = this_is_space;
	}

      if(tail_space)
	g_string_append_c(str, ' ');
    }
}


static void
wml_error(void *ctx, const char *msg, ...)
{
  /* WapTextFu *textfu = WAP_TEXTFU(ctx); */
  va_list args;

  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);

/*    xml_ctxt_done(NULL, WAP_STREAM_ERROR, textfu); */
}

static void
wml_fatalError(void *ctx, const char *msg, ...)
{
  /* WapTextFu *textfu = WAP_TEXTFU(ctx); */
  va_list args;

  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);

/*    xml_ctxt_done(NULL, WAP_STREAM_ERROR, textfu); */
}

static xmlSAXHandler wml_handler = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  wml_startDocument,
  wml_endDocument,
  wml_startElement,
  wml_endElement,
  NULL,
  wml_characters,
  NULL,
  NULL,
  NULL,
  NULL,
  wml_error,
  wml_fatalError,
  NULL,
  NULL
};

#if 0
static void
xml_ctxt_start (WapStreamHandle stream,
		const char *mime_type,
		gpointer user_data)
{
#if 0
  /* Can diff between HDML & WML here, eventually */
  if(strcmp(mime_type, "text/vnd.wap.wml"))
    /*      xml_ctxt_done(stream, WAP_STREAM_ERROR, user_data)*/ ;
#endif
}

static void xml_ctxt_done(int handle, int status, gpointer user_data)
{

}

static void
xml_ctxt_write(WapStreamHandle handle, const guchar *buffer, size_t size, gpointer user_data)
{
  WapTextFu *textfu = user_data;

  if(!textfu->doc_stream)
    return;

  xmlParseChunk(textfu->ctxt, buffer, size, 0);
}
#endif

static void
placed_item_free(PlacedItem *pi)
{
  gtk_container_remove(GTK_CONTAINER(pi->widget->parent), pi->widget);
  g_free(pi);
}

static void
wap_go_free(WapGoInfo *go)
{
  g_free(go->url);
  g_free(go->charset); 
  g_list_foreach(go->postvalues, (GFunc)g_free, NULL);
  g_list_free(go->postvalues);
  g_free(go);
}

static void
wap_anchor_free(WapAnchorInfo *wai)
{
  g_free(wai->title);
  if(wai->go_info)
    wap_go_free(wai->go_info);
  g_list_foreach(wai->setvars, (GFunc)g_free, NULL);
  g_list_free(wai->setvars);
  g_free(wai);
}

static void
wap_paragraph_free(WapParagraphInfo *para)
{
  g_object_unref(G_OBJECT(para->layout));

  g_list_foreach(para->anchors, (GFunc)wap_anchor_free, NULL);
  g_list_free(para->anchors);

  g_list_foreach(para->placed_items, (GFunc)placed_item_free, NULL);
  g_list_free(para->placed_items);

  g_free(para);
}

static void
wap_action_free(WapDoInfo *doi)
{
  if(doi->go)
    wap_go_free(doi->go);
  g_free(doi->label);
  g_free(doi->name);
  if(doi->img)
    gtk_widget_unref(doi->img);
  g_free(doi);
}

static void
wap_option_free(WapOptionInfo *option)
{
  g_free(option->text);
  g_free(option->onpick);
  g_free(option);
}

static void
wap_input_free(WapInputInfo *input)
{
  g_free(input->name);
  g_free(input->title);
  g_free(input->value);
  g_free(input->fmt);
  g_free(input->iname);

  g_list_foreach(input->options, (GFunc)wap_option_free, NULL);
  g_list_free(input->options);

  g_free(input);
}

static void
wap_card_free(WapCard *card)
{
  g_list_foreach(card->paragraphs, (GFunc)wap_paragraph_free, NULL);
  g_list_free(card->paragraphs);
  g_list_foreach(card->actions, (GFunc)wap_action_free, NULL);
  g_list_free(card->actions);
  g_list_foreach(card->inputs, (GFunc)wap_input_free, NULL);
  g_list_free(card->inputs);

  g_free(card->id);
  g_free(card->title);
  g_free(card->onenterforward);
  g_free(card->onenterbackward);
  g_free(card->ontimer);

  g_assert(!card->para_style_stack);
  g_assert(!card->text_style_stack);
  g_free(card);
}

static void
wap_textfu_cleanup_page(WapTextFu *textfu)
{
  g_free(textfu->cur_filename);
  textfu->cur_filename = NULL;
  if(textfu->active_card)
    gtk_signal_emit(GTK_OBJECT(textfu), textfu_signals[DEACTIVATE_CARD], textfu->active_card);
  textfu->cur_card = textfu->active_card = NULL;
  textfu->cur_go = NULL;
  textfu->cur_do = NULL;
  textfu->cur_anchor = NULL;
  textfu->cur_input = NULL;
  textfu->cur_option = NULL;
  g_string_truncate(textfu->tmp_text, 0);
  g_string_truncate(textfu->nc_text, 0);
  g_list_foreach(textfu->cards, (GFunc)wap_card_free, NULL);
  g_list_free(textfu->cards); textfu->cards = NULL;
}

static GList *
wap_textfu_copy_postvalues(GList *postvalues)
{
  GList *retval = NULL;

  for(postvalues = g_list_last(postvalues); postvalues; postvalues = postvalues->prev)
    retval = g_list_prepend(retval, g_strdup(postvalues->data));

  return retval;
}

static GList *
wap_textfu_build_postvalues(WapTextFu *textfu)
{
  GList *retval, *ltmp;

  for(ltmp = textfu->active_card->inputs, retval = NULL; ltmp; ltmp = ltmp->next)
    {
      WapInputInfo *input = ltmp->data;
      PlacedItem *item = input->placed_item;

      if(input->is_selection)
	{
	  if(GTK_IS_LIST(item->widget))
	    {
	      GList *selection;

	      for(selection = GTK_LIST(item->widget)->selection; selection; selection = selection->next)
		{
		  WapOptionInfo *option = gtk_object_get_data(GTK_OBJECT(selection->data), "WapOptionInfo");

		  retval = g_list_prepend(retval, g_strdup(option->value?option->value:option->text));
		  retval = g_list_prepend(retval, g_strdup(input->name));
		}
	    }
	  else
	    g_assert_not_reached();
	}
      else
	{
	  retval = g_list_prepend(retval, g_strdup(gtk_entry_get_text(GTK_ENTRY(item->widget))));
	  retval = g_list_prepend(retval, g_strdup(input->name));
	}
    }

  return retval;
}

static char *
wap_textfu_get_var(WapTextFu *textfu, const char *varname, int n)
{
  GList *l;
  WapInputInfo *input;
  PlacedItem *item;

  g_assert(varname[0] == '$');
  varname++;
  if(varname[0] == '(')
    {
      varname++;
      n--;
    }

  printf("checking for variable %.*s\n", n, varname);
  for(l = textfu->active_card->inputs; l; l = l->next)
    {
      input = l->data;

      if(!strncasecmp(input->name, varname, n))
	break;
    }
  if(!l)
    return NULL;

  item = input->placed_item;
  if(input->is_selection)
    {
      g_assert(!input->multiple);

      if(GTK_IS_LIST(item->widget))
	{
	  GList *selection;

	  for(selection = GTK_LIST(item->widget)->selection; selection; selection = selection->next)
	    {
	      WapOptionInfo *option = gtk_object_get_data(GTK_OBJECT(selection->data), "WapOptionInfo");

	      return g_strdup(option->value?option->value:option->text);
	    }
	}
      else
	g_assert_not_reached();
    }

  return g_strdup(gtk_entry_get_text(GTK_ENTRY(item->widget)));
}

static char *
wap_textfu_subst_vars(WapTextFu *textfu, char *str)
{
  char *ctmp, *retval, *varnamestart;
  GString *build;
  int i;

  build = g_string_new(NULL);
  for(i = 0, varnamestart = NULL; str[i]; i++)
    {
      if(varnamestart)
	{
	  if(!isalnum(str[i])
	     && (str[i] != '('
		 || (str+i-varnamestart) > 1))
	    {
	      ctmp = wap_textfu_get_var(textfu, varnamestart, str+i-varnamestart-1);
	      if(ctmp)
		g_string_append(build, ctmp);
	      g_free(ctmp);
	      if(!ctmp)
		g_string_append_len(build, varnamestart, str+i-varnamestart);
	      varnamestart = NULL;
	    }
	}
      else if(str[i] == '$')
	{
	  varnamestart = str + i;
	}
      else
	g_string_append_c(build, str[i]);
    }
  if(varnamestart)
    {
      ctmp = wap_textfu_get_var(textfu, varnamestart, strlen(varnamestart));
      if(ctmp)
	g_string_append(build, ctmp);
      else
	g_string_append(build, varnamestart);
      g_free(ctmp);
    }

  retval = build->str;
  g_string_free(build, FALSE);

  g_print("var substitution results in '%s'\n", retval);

  return retval;
}

void
wap_textfu_load_go_info(WapTextFu *textfu, WapGoInfo *go)
{
  GList *postvalues = NULL;
  char *real_url;

  g_return_if_fail (textfu != NULL);
  g_return_if_fail (WAP_IS_TEXTFU (textfu));

  /* out with old */
  real_url = wap_textfu_subst_vars(textfu, go->url);

  if(textfu->active_card && go->post_method)
    postvalues = wap_textfu_build_postvalues(textfu);

  wap_textfu_cleanup_page(textfu);
  GTK_LAYOUT(textfu)->vadjustment->upper = 0;
  GTK_LAYOUT(textfu)->hadjustment->upper = 0;
  GTK_LAYOUT(textfu)->vadjustment->value = 0;
  GTK_LAYOUT(textfu)->hadjustment->value = 0;
  gtk_adjustment_changed(GTK_LAYOUT(textfu)->vadjustment);
  gtk_adjustment_changed(GTK_LAYOUT(textfu)->hadjustment);
  textfu->old_width = 0; /* Force resize */
  wap_textfu_redraw(GTK_WIDGET(textfu));

  /* in with new */
  textfu->ctxt = xmlCreatePushParserCtxt(&wml_handler, textfu, NULL, 0, NULL);
  textfu->cur_filename = real_url;
  textfu->in_content = FALSE;

  if(go->post_method)
     postvalues = g_list_concat(postvalues, wap_textfu_copy_postvalues(go->postvalues));

}

void
wap_textfu_load_file(WapTextFu *textfu, const char *file)
{
  WapGoInfo go;

  memset(&go, 0, sizeof(go));
  go.url = (char *)file;
  go.sendreferer = TRUE;
  wap_textfu_load_go_info(textfu, &go);
}

typedef struct {
  WapTimerInfo *timer;
  WapTextFu *textfu;
} TimerCallbackInfo;

static gboolean
wap_textfu_timer_cb(gpointer data)
{
  TimerCallbackInfo *tci = data;

  tci->timer->tag = 0;

  if(tci->textfu->active_card->ontimer)
    gtk_signal_emit(GTK_OBJECT(tci->textfu), textfu_signals[LINK_FOLLOWED], tci->textfu->active_card->ontimer);

  return FALSE;
}

void
wap_textfu_activate_card(WapTextFu *textfu, WapCard *card)
{
  GList *ltmp;

  if(card == textfu->active_card)
    return;

  if(textfu->active_card)
    {
      /* Move old placed items off-screen */
      for(ltmp = textfu->active_card->paragraphs; ltmp; ltmp = ltmp->next)
	{
	  WapParagraphInfo *pi = ltmp->data;
	  GList *ltmp2;

	  for(ltmp2 = pi->placed_items; ltmp2; ltmp2 = ltmp2->next)
	    {
	      PlacedItem *pl = ltmp2->data;

	      gtk_layout_move(GTK_LAYOUT(textfu), pl->widget, -10000, -10000);
	    }
	}

      gtk_signal_emit(GTK_OBJECT(textfu), textfu_signals[DEACTIVATE_CARD], textfu->active_card);
    }

  card->active_anchor = NULL;
  textfu->old_width = 0;
  textfu->active_card = card;

  gtk_signal_emit(GTK_OBJECT(textfu), textfu_signals[ACTIVATE_CARD], card);
  gtk_widget_queue_resize(GTK_WIDGET(textfu));

  for(ltmp = textfu->active_card->timers; ltmp; ltmp = ltmp->next)
    {
      WapTimerInfo *timer = ltmp->data;

      if(!timer->tag)
	{
	  TimerCallbackInfo *tci = g_new(TimerCallbackInfo, 1);

	  tci->timer = timer;
	  tci->textfu = textfu;
	  timer->tag = g_timeout_add_full(G_PRIORITY_DEFAULT, timer->value * 100 /* 10ths of seconds into 1000ths of seconds */,
					  wap_textfu_timer_cb, tci, g_free);
	}
    }
}

guint16
wap_textfu_tagid_alloc(const char *name, WapTextFuTagHandler handler)
{
  WapTextFuClass *klass;
  TagRegistration *cur_reg, *new_reg;
  static guint16 tag_counter = 1;

  klass = WAP_TEXTFU_CLASS(gtk_type_class(wap_textfu_get_type()));

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

void
wap_textfu_postvalues_free(GList *postvalues)
{
  g_list_foreach(postvalues, (GFunc)g_free, NULL);
  g_list_free(postvalues);
}
