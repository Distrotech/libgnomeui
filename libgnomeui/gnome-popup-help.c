/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Popup menus for GNOME
 * 
 * Copyright (C) 1998 Jonathan Blandford
 *
 * Authors: Jonathan Blandford <jrb@redhat.com>
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
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkeditable.h>
#include <gtk/gtkentry.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18n.h"
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-stock.h"
#include "gnome-popup-help.h"
#include "gnome-popup-menu.h"
#include "libgnomeui/gnome-window-icon.h"
/* Prototypes */
static void help_callback (GtkWidget *widget, gpointer data);
static void helpwindow_destroy_callback (GtkWidget *widget, gpointer data);
static void cut_callback (GtkWidget *widget, gpointer data);
static void copy_callback (GtkWidget *widget, gpointer data);
static void paste_callback (GtkWidget *widget, gpointer data);
static gint popup_pre_callback (GtkWidget *widget, GdkEventButton *event, GnomeUIInfo *cutptr);
static GnomeUIInfo *append_ui_info (GnomeUIInfo *base, GnomeUIInfo *info, GnomeUIInfo **cutptr);

static GnomeUIInfo separator[] = {
        {GNOME_APP_UI_SEPARATOR},
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}
};
static GnomeUIInfo helpmenu[] = {
        {GNOME_APP_UI_ITEM, N_("_Help with this"), NULL, help_callback, NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL},
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}
};
static GnomeUIInfo cutcopymenu[] = {
        {GNOME_APP_UI_ITEM, N_("Cu_t"), NULL, cut_callback, NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT, 0, 0, NULL},
        {GNOME_APP_UI_ITEM, N_("_Copy"), NULL, copy_callback, NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY, 0, 0, NULL},
        {GNOME_APP_UI_ITEM, N_("_Paste"), NULL, paste_callback, NULL, NULL,
         GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PASTE, 0, 0, NULL},
        {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL, NULL, NULL,
         GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL}
};

static void
gtk_tooltips_free_string (gpointer data, gpointer user_data)
{
  if (data)
    g_free (data);
}

static void
gnome_popup_help_layout_text (GtkWidget *helpwindow, GtkTooltipsData *data, gchar* text)
     /* Swiped from gtktooltips.c  (: */
{
  gchar *row_end, *row_text, *break_pos;
  gint i, row_width, window_width = 0;
  size_t len;


  g_list_foreach (data->row, gtk_tooltips_free_string, 0);
  if (data->row)
    g_list_free (data->row);
  data->row = 0;
  data->font = helpwindow->style->font;
  data->width = 0;


  while (*text)
    {
      row_end = strchr (text, '\n');
      if (!row_end)
       row_end = strchr (text, '\0');

      len = row_end - text + 1;
      row_text = g_new(gchar, len);
      memcpy (row_text, text, len - 1);
      row_text[len - 1] = '\0';

      /* now either adjust the window's width or shorten the row until
        it fits in the window */

      while (1)
       {
         row_width = gdk_string_width (data->font, row_text);
         if (!window_width)
           {
             /* make an initial guess at window's width: */

             if (row_width > gdk_screen_width () / 4)
               window_width = gdk_screen_width () / 4;
             else
               window_width = row_width;
           }
         if (row_width <= window_width)
           break;

         if (strchr (row_text, ' '))
           {
             /* the row is currently too wide, but we have blanks in
                the row so we can break it into smaller pieces */

             gint avg_width = row_width / strlen (row_text);

             i = window_width;
             if (avg_width)
               i /= avg_width;
             if ((size_t) i >= len)
               i = len - 1;

             break_pos = strchr (row_text + i, ' ');
             if (!break_pos)
               {
                 break_pos = row_text + i;
                 while (*--break_pos != ' ');
               }
             *break_pos = '\0';
           }
         else
           {
             /* we can't break this row into any smaller pieces, so
                we have no choice but to widen the window: */

             window_width = row_width;
             break;
           }
       }
      if (row_width > data->width)
       data->width = row_width;
      data->row = g_list_append (data->row, row_text);
      text += strlen (row_text);
      if (!*text)
       break;

      if (text[0] == '\n' && text[1])
       /* end of paragraph and there is more text to come */
       data->row = g_list_append (data->row, 0);
      ++text;  /* skip blank or newline */
    }
  data->width += 8;    /* leave some border */
}

static gint
gnome_popup_help_expose (GtkWidget *darea, GdkEventExpose *event, GtkTooltipsData *data)
{
        GtkStyle *style;
        gint y, baseline_skip, gap;
        GList *el;
      
        style = darea->style;
        
        gap = (style->font->ascent + style->font->descent) / 4;
        if (gap < 2)
                gap = 2;
        baseline_skip = style->font->ascent + style->font->descent + gap;

        gtk_paint_flat_box(style, darea->window,
                           GTK_STATE_NORMAL, GTK_SHADOW_OUT, 
                           NULL, GTK_WIDGET(darea), "tooltip",
                           0, 0, -1, -1);

        y = style->font->ascent + 4;
  
        for (el = data->row; el; el = el->next) {
                if (el->data) {
                        gtk_paint_string (style, darea->window, 
                                          GTK_STATE_NORMAL, 
                                          NULL, GTK_WIDGET(darea), "tooltip",
                                          4, y, el->data);
                        y += baseline_skip;
                }
                else
                        y += baseline_skip / 2;
        }
        return FALSE;
}

static void
gnome_popup_help_size_window (GtkWidget *helpwindow, GtkTooltipsData *data, gint *h, gint *w)
{
  GtkStyle *style;
  gint gap, baseline_skip;
  GList *el;

  style = helpwindow->style;

  gap = (style->font->ascent + style->font->descent) / 4;
  if (gap < 2)
          gap = 2;
  baseline_skip = style->font->ascent + style->font->descent + gap;
  *w = data->width;
  *h = 8 - gap;
  for (el = data->row; el; el = el->next)
          if (el->data)
                  *h += baseline_skip;
          else
                  *h += baseline_skip / 2;
  if (*h < 8)
          *h = 8;

  gtk_widget_set_usize (helpwindow, *w + 1, *h + 1);
}

static void
gnome_popup_help_place_window (GtkWidget *helpwindow, GtkWidget *widget, GtkTooltipsData *data, gint h, gint w)
{
  gint x, y, scr_w, scr_h;

  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();

  gdk_window_get_pointer (NULL, &x, NULL, NULL);
  gdk_window_get_origin (widget->window, NULL, &y);
  if (GTK_WIDGET_NO_WINDOW (widget))
    y += widget->allocation.y;

  x -= ((w >> 1) + 4);
  if ((x + w) > scr_w)
    x -= (x + w) - scr_w;
  else if (x < 0)
    x = 0;

  if ((y + h + widget->allocation.height + 4) > scr_h)
    y = y - h - 4;
  else
    y = y + widget->allocation.height + 4;
  gtk_widget_set_uposition (helpwindow, x, y);
  gtk_widget_show_now (helpwindow);
}

static GnomeUIInfo *
append_ui_info (GnomeUIInfo *base, GnomeUIInfo *info, GnomeUIInfo **cutptr)
{
        GnomeUIInfo *retval;
        gint j, i = 0;

	for (retval = base; retval&&retval->type != GNOME_APP_UI_ENDOFINFO; retval++, i++);
	for (retval = info; retval&&retval->type != GNOME_APP_UI_ENDOFINFO; retval++, i++);
        retval = g_new (GnomeUIInfo, i+1);

        for (i = 0;base&&base->type != GNOME_APP_UI_ENDOFINFO; base++, i++) {
                retval[i] = base[0];
        }

        if (info == cutcopymenu)
                *cutptr = retval + i;
        for (j = 0;info&&info->type != GNOME_APP_UI_ENDOFINFO; info++, j++) {
                retval[i + j] = info[0];
        }

        /* get the GNOME_APP_UI_ENDOFINFO */
        retval[i+j] = info[0];

        /* er, i should free base here... */
        return retval;
}

static void
helpwindow_click_callback (GtkWidget *widget, gpointer data)
{
        gtk_widget_hide (widget->parent);
        gdk_pointer_ungrab (GDK_CURRENT_TIME);

}

static void
helpwindow_destroy_callback (GtkWidget *widget, gpointer data)
{
        g_free ((gchar*) data);
}

static void
help_callback (GtkWidget *menu, gpointer unused)
{
        GtkTooltipsData *data;
        gint h, w;
        GtkWidget *widget, *helpwindow, *darea;

        gchar *text;


        /* get the */
        text = (gchar *) (gtk_object_get_data (GTK_OBJECT (menu->parent), 
                                                "gnome_popup_help_text"));
        helpwindow = gtk_object_get_data (GTK_OBJECT (menu->parent),
                                                      "gnome_popup_help_window");
        widget = gtk_object_get_data (GTK_OBJECT (menu->parent), 
                                      "gnome_popup_help_widget");
        data = gtk_object_get_data (GTK_OBJECT (menu->parent), 
                                    "gnome_popup_help_data");
        if (helpwindow == NULL) {
                data = g_malloc (sizeof (GtkTooltipsData));
                data->row = NULL;

                helpwindow = gtk_window_new (GTK_WINDOW_POPUP);
                gnome_window_icon_set_from_default (GTK_WINDOW (helpwindow));
                gtk_window_set_policy (GTK_WINDOW (helpwindow), FALSE, FALSE, TRUE);
                
                darea = gtk_drawing_area_new ();
                gtk_widget_set_events (darea, GDK_BUTTON_PRESS_MASK);
                gtk_container_add (GTK_CONTAINER (helpwindow), darea);
                gtk_widget_show (darea);

                gnome_popup_help_layout_text (helpwindow, data, text);
                gnome_popup_help_size_window (helpwindow, data, &h, &w);
                gnome_popup_help_place_window (helpwindow, widget, data, h, w);

                gtk_signal_connect (GTK_OBJECT (darea), "expose_event",
                                    GTK_SIGNAL_FUNC (gnome_popup_help_expose), 
                                    data);
                gtk_signal_connect (GTK_OBJECT (darea), "button_press_event",
                                    GTK_SIGNAL_FUNC (helpwindow_click_callback), 
                                    NULL);
                gtk_signal_connect (GTK_OBJECT (helpwindow),
                                    "destroy",
                                    helpwindow_destroy_callback,
                                    text);
                gtk_object_set_data (GTK_OBJECT (menu->parent), 
                                     "gnome_popup_help_data", data);
                gtk_object_set_data (GTK_OBJECT (menu->parent), 
                                     "gnome_popup_help_window", helpwindow);

                gdk_pointer_grab (darea->window,
                                  FALSE,
                                  GDK_BUTTON_PRESS_MASK,
                                  NULL,
                                  NULL,
                                  GDK_CURRENT_TIME);
        } else {
                gnome_popup_help_place_window (helpwindow, widget, 
                                               data, 
                                               helpwindow->allocation.height,
                                               helpwindow->allocation.width);
                gdk_flush ();
                gdk_pointer_grab (GTK_BIN(helpwindow)->child->window,
                                  FALSE,
                                  GDK_BUTTON_PRESS_MASK,
                                  NULL,
                                  NULL,
                                  GDK_CURRENT_TIME);
        }

}

static gint
popup_pre_callback (GtkWidget *widget, GdkEventButton *event, GnomeUIInfo *cutptr) 
{
        if (event->button != 3)
		return FALSE;

        if (GTK_EDITABLE (widget)->selection_start_pos == GTK_EDITABLE (widget)->selection_end_pos) {
                gtk_widget_set_sensitive (cutptr[0].widget, FALSE);
                gtk_widget_set_sensitive (cutptr[1].widget, FALSE);
        } else {
                gtk_widget_set_sensitive (cutptr[0].widget, TRUE);
                gtk_widget_set_sensitive (cutptr[1].widget, TRUE);
        }

        if (GTK_EDITABLE (widget)->editable)
                gtk_widget_set_sensitive (cutptr[2].widget, TRUE);
        else
                gtk_widget_set_sensitive (cutptr[2].widget, FALSE);
        
        return FALSE;
}


static void
cut_callback (GtkWidget *widget, gpointer data)
{
        GtkWidget *editable;
        editable = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (widget->parent), 
                                                    "gnome_popup_help_widget"));
        gtk_editable_cut_clipboard (GTK_EDITABLE (editable));
}

static void
copy_callback (GtkWidget *widget, gpointer data)
{
        GtkWidget *editable;
        editable = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (widget->parent), 
                                                    "gnome_popup_help_widget"));
        gtk_editable_copy_clipboard (GTK_EDITABLE (editable));
}

static void
paste_callback (GtkWidget *widget, gpointer data)
{
        GtkWidget *editable;
        editable = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (widget->parent), 
                                                    "gnome_popup_help_widget"));
        gtk_editable_paste_clipboard (GTK_EDITABLE (editable));
}


/**
 * gnome_widget_add_help_with_uidata:
 * @widget: The widget to add the popup help to.
 * @help: The help message text.
 * @menuinfo: The template for possible additional menu items.
 * @user_data: The user data to be passed to menu callbacks.
 * 
 * This creates a popup menu on @widget with one entry.  The menu, invoked by
 * pressing button three on the widget, will have one entry entitled: "Help with
 * this."  Selecting this entry will bring up a tooltip with the help variable
 * as its text.  In addition, if the widget is a descendent of #GtkEditable, it
 * will add "cut", "copy", and "paste" to the menu.  If @help is NULL, then it
 * will <emphasis>just</emphasis> add the "cut", "copy", and "paste".  Finally,
 * if @menuinfo is non-NULL, it will append the menu defined by it on the end of
 * the popup menu, with @user_data passed to the callbacks.  This function
 * currently only works on non GTK_WIDGET_NO_WINDOW () widgets (ie. it only
 * works on widgets with windows.)  If you would actually like a handle to the
 * popup menu, call gnome_popup_menu_get() as normal.

 * 
 * Return value: 
 **/
/* public function. */
void
gnome_widget_add_help_with_uidata (GtkWidget *widget,
				   const gchar *help,
				   GnomeUIInfo *menuinfo,
                                   gpointer user_data)
{
        GnomeUIInfo *finalmenu = NULL;
        GtkWidget *menu;
        GnomeUIInfo *cutptr = NULL;

        g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (GTK_IS_WIDGET (widget));

        /* set up the menu type */
        if (help != NULL) {
                finalmenu = append_ui_info (finalmenu, helpmenu, NULL);
        }
        if (GTK_IS_EDITABLE (widget)) {
                if (help != NULL)
                        finalmenu = append_ui_info (finalmenu, separator, NULL);
                finalmenu = append_ui_info (finalmenu, cutcopymenu, &cutptr);
        }

        if (menuinfo != NULL) {
                if ((help != NULL) || (GTK_IS_EDITABLE (widget)))
                        finalmenu = append_ui_info (finalmenu, separator, NULL);
                finalmenu = append_ui_info (finalmenu, menuinfo, NULL);
        }
        
        if (finalmenu == NULL)
                /* hmmm, no tooltip set, no extra data, and no cut copy paste.
                 * we do nothing then.*/
                return;

        menu = gnome_popup_menu_new (finalmenu);
        /* now, we tweak. */
        if (GTK_IS_EDITABLE (widget)) {
                gtk_signal_connect (GTK_OBJECT (widget), "button_press_event",
                                    (GtkSignalFunc) popup_pre_callback,
                                    cutptr);
        }
        gnome_popup_menu_attach (menu, widget, user_data);
        
        if (help != NULL) {
                /* set the tool tip! */
                gtk_object_set_data (GTK_OBJECT (menu), 
                                     "gnome_popup_help_text",g_strdup(help));
                gtk_object_set_data (GTK_OBJECT (menu), 
                                     "gnome_popup_help_widget", widget);
        }
}
