/* wap-textfu.h
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
#ifndef __WAP_TEXTFU_H__
#define __WAP_TEXTFU_H__

#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <pango/pango.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define WAP_TYPE_TEXTFU			(wap_textfu_get_type ())
#define WAP_TEXTFU(obj)			(GTK_CHECK_CAST ((obj), WAP_TYPE_TEXTFU, WapTextFu))
#define WAP_TEXTFU_CLASS(klass)		(GTK_CHECK_CLASS_CAST ((klass), WAP_TYPE_TEXTFU, WapTextFuClass))
#define WAP_IS_TEXTFU(obj)			(GTK_CHECK_TYPE ((obj), WAP_TYPE_TEXTFU))
#define WAP_IS_TEXTFU_CLASS(klass)		(GTK_CHECK_CLASS_TYPE ((obj), WAP_TYPE_TEXTFU))
#define WAP_TEXTFU_GET_CLASS(obj)             (GTK_CHECK_GET_CLASS ((obj), WAP_TYPE_TEXTFU, WapTextFuClass))

typedef enum {
  TEXTFU_ITEM_TEXT,
  TEXTFU_ITEM_CONTAINER,
  TEXTFU_ITEM_WIDGET
} WapTextFuItemType;

typedef struct {
  char tag[16];
} WapStyle;

typedef struct {
  WapStyle style;

  PangoAlignment alignment;
  gint16 left_indent, right_indent;
  gint16 bullet_num, bullet_level;
  gint16 space_after;
  gboolean ul_container : 1;
  gboolean ol_container : 1;
  gboolean replace_prev : 1;
} WapParaStyle;

typedef enum {
  TEXTFU_FALSE,
  TEXTFU_TRUE,
  TEXTFU_UNKNOWN
} WapTextFuTruthValue;

typedef struct {
  char *url;
  char *charset;
  GList *postvalues;

  gboolean sendreferer : 1;
  gboolean post_method : 1;
} WapGoInfo;

typedef struct {
  char *title;
  gpointer attr;
  enum { ANCHOR_GO, ANCHOR_PREV, ANCHOR_REFRESH, ANCHORS_AWAY } type;
  WapGoInfo *go_info;
  GList *setvars;
} WapAnchorInfo;

typedef struct {
  enum { DO_ACCEPT, DO_DELETE, DO_HELP, DO_OPTIONS, DO_PREV,
	 DO_SOFT1, DO_SOFT2, DO_SEND, DO_NOTHING } type;
  enum { ACT_GO, ACT_PREV, ACT_NOTHING } task;
  WapGoInfo *go;
  char *label, *name;
  GtkWidget *img;
} WapDoInfo;

typedef struct {
  WapStyle style;

  WapAnchorInfo *wai;
  char *font_family;
  int font_size;
  GdkColor fg_color, bg_color;

  int start_offset;
  guint bold : 2, italic : 2, underline : 2;

  guint replace_prev : 1;
  guint fg_color_set : 1;
  guint bg_color_set : 1;
  guint font_size_set : 1;
} WapTextStyle;

typedef struct {
  PangoLayout *layout;
  PangoAttrList *attrs;
  int left_indent, right_indent;
  GList *placed_items;
  int space_after;

  GList *anchors;

  int y_offset, height;
} WapParagraphInfo;

typedef struct {
  char *text, *onpick, *value;
  GtkWidget *image, *li_widget;
} WapOptionInfo;

typedef struct {
  char *name, *title, *value, *fmt, *iname;
  gpointer placed_item;
  GList *options;
  int maxlen, ivalue;
  gboolean emptyok : 1, multiple : 1, is_selection : 1;
} WapInputInfo;

typedef struct {
  char *name;
  guint value;
  guint tag;
} WapTimerInfo;

typedef struct {
  char *id, *title;
  char *onenterforward, *onenterbackward, *ontimer;

  WapAnchorInfo *active_anchor;
  WapParagraphInfo *active_anchor_para;

  GList *paragraphs;
  GList *actions;
  GList *inputs;
  GList *timers;
  guint newcontext : 1;

  /* Layout stuff */
  GList *para_style_stack;
  GList *text_style_stack;
} WapCard;

typedef struct _WapTextFu       WapTextFu;
typedef struct _WapTextFuClass  WapTextFuClass;

struct _WapTextFu
{
  GtkLayout parent;

  /* All fields are private, don't touch */
  char *cur_filename;

  guint old_width;

  guint vscroll_tag;

  WapCard *active_card;
  GList *cards;
  char *lang;

  GdkGC *drawing_gc;

  /* Parser info */
  WapCard *cur_card;
  WapGoInfo *cur_go;
  WapDoInfo *cur_do;
  WapAnchorInfo *cur_anchor;
  WapInputInfo *cur_input;
  WapOptionInfo *cur_option;

  xmlParserCtxtPtr ctxt;
  GString *tmp_text, *nc_text;
  WapParagraphInfo *cur_para;
  PangoContext *pango_ctx;

  gint ignore_counter;
  gboolean in_content : 1, in_do : 1, in_anchor : 1, in_select : 1, in_option : 1;
  gboolean last_was_placed : 1, is_hdml : 1;
};

struct _WapTextFuClass
{
  GtkLayoutClass parent_class;

  GHashTable *tag_handlers;

  /* Signals */
  void (*link_followed) (WapTextFu *textfu, const char *url);
  void (*anchor_followed) (WapTextFu *textfu, WapAnchorInfo *anchor);
  void (*prev_followed) (WapTextFu *textfu);
  void (*load_done)     (WapTextFu *textfu);
  void (*load_error)    (WapTextFu *textfu);
  void (*activate_card) (WapTextFu *textfu, WapCard *card);
  void (*deactivate_card) (WapTextFu *textfu, WapCard *card);
};

typedef void (*WapTextFuTagHandler)(WapTextFu *textfu, const char *tag, guint16 tag_id, char **attrs, gboolean is_end);

GtkType    wap_textfu_get_type   (void);
GtkWidget *wap_textfu_new        (void);
void       wap_textfu_load_file  (WapTextFu *textfu, const char *filename);
void       wap_textfu_load_go_info  (WapTextFu *textfu, WapGoInfo *go);
void       wap_textfu_activate_card(WapTextFu *textfu, WapCard *card);
guint16    wap_textfu_tagid_alloc(const char *name, WapTextFuTagHandler handler);
void wap_textfu_postvalues_free(GList *postvalues);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __WAP_TEXTFU_H__ */
