/*
 * Copyright (C) 2000 Red Hat, Inc.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* TODO:
 * Turn GnomeHelpView into a widget with a window (cut & paste eventbox code) so that we can handle right-click on it
 * to get the style menu
 *
 * Add sanity checking to all parameters of API functions
 */

#include <glib.h>
#include <ctype.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-url.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-config.h>
#include <libgnome/gnome-portability.h>
#include "libgnome/gnome-i18nP.h"
#include "gnome-helpsys.h"
#include "gnome-stock.h"
#include "gnome-uidefs.h"
#include "gnome-app-helper.h"
#include "gnome-popup-menu.h"
#include "gnome-textfu.h"
#include "gnome-cursors.h"
#include "gnome-dialog-util.h"
#include <libgnome/gnomelib-init2.h>
#include <gdk/gdkx.h>

#include <stdio.h>
#include <string.h>

enum {
  ARG_NONE=0,
  ARG_APP_STYLE,
  ARG_APP_STYLE_PRIO,
  ARG_TOPLEVEL
};

typedef enum {
  URL_SAME_APP,
  URL_GENERAL_HELPSYSTEM,
  URL_WEB
} HelpURLType;

static void gnome_help_view_class_init (GnomeHelpViewClass *class);
static void gnome_help_view_init (GnomeHelpView *help_view);
static void gnome_help_view_destroy (GtkObject *obj);
static void gnome_help_view_set_arg (GtkObject *obj, GtkArg *arg, guint arg_id);
static void gnome_help_view_get_arg (GtkObject *obj, GtkArg *arg, guint arg_id);
static void gnome_help_view_size_request  (GtkWidget      *widget,
					   GtkRequisition *requisition);
static void gnome_help_view_size_allocate (GtkWidget      *widget,
					   GtkAllocation  *allocation);
static void gnome_help_view_select_style(GtkWidget *btn, GnomeHelpView *help_view);
static char *gnome_help_view_find_help_id(GnomeHelpView *help_view, const char *widget_id);
static void gnome_help_view_show_url(GnomeHelpView *help_view, const char *url, HelpURLType type);
static void gnome_help_view_set_style_popup(gpointer dummy, GnomeHelpView *help_view);
static void gnome_help_view_set_style_embedded(gpointer dummy, GnomeHelpView *help_view);
static void gnome_help_view_set_style_browser(gpointer dummy, GnomeHelpView *help_view);
static gint gnome_help_view_process_event(GtkWidget *btn, GdkEvent *event, GnomeHelpView *help_view);

static GnomeUIInfo popup_style_menu_items[] = {
  GNOMEUIINFO_RADIOITEM("_Popup", "Show help in temporary popup windows", gnome_help_view_set_style_popup, NULL),
  GNOMEUIINFO_RADIOITEM("_Embedded", "Show help inside the application window", gnome_help_view_set_style_embedded, NULL),
  GNOMEUIINFO_RADIOITEM("_Browser", "Show help in a help browser window", gnome_help_view_set_style_browser, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo popup_style_menu[] = {
  GNOMEUIINFO_RADIOLIST(popup_style_menu_items),
  GNOMEUIINFO_END
};

GtkType
gnome_help_view_get_type(void)
{
  static GtkType gnome_help_view_type = 0;

  if (!gnome_help_view_type)
    {
      GtkTypeInfo gnome_help_view_info = {
	"GnomeHelpView",
	sizeof (GnomeHelpView),
	sizeof (GnomeHelpViewClass),
	(GtkClassInitFunc) gnome_help_view_class_init,
	(GtkObjectInitFunc) gnome_help_view_init,
	NULL,
	NULL,
	NULL
      };

      gnome_help_view_type = gtk_type_unique (gtk_box_get_type(), &gnome_help_view_info);
    }

  return gnome_help_view_type;
}

static GtkObjectClass *parent_class = NULL;

static GdkAtom atom_explain_query = 0, atom_explain_request = 0, atom_explain_query_reply = 0;

static void
gnome_help_view_class_init (GnomeHelpViewClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;
  parent_class = gtk_type_class(gtk_type_parent(object_class->type));

  object_class->get_arg = gnome_help_view_get_arg;
  object_class->set_arg = gnome_help_view_set_arg;
  object_class->destroy = gnome_help_view_destroy;

  widget_class->size_request = gnome_help_view_size_request;
  widget_class->size_allocate = gnome_help_view_size_allocate;

  class->popup_menu = gnome_popup_menu_new(popup_style_menu);

  gtk_object_add_arg_type ("GnomeHelpView::app_style", GTK_TYPE_ENUM, GTK_ARG_READWRITE|GTK_ARG_CONSTRUCT, ARG_APP_STYLE);
  gtk_object_add_arg_type ("GnomeHelpView::app_style_priority", GTK_TYPE_INT, GTK_ARG_READWRITE|GTK_ARG_CONSTRUCT, ARG_APP_STYLE_PRIO);
  gtk_object_add_arg_type ("GnomeHelpView::toplevel", GTK_TYPE_OBJECT, GTK_ARG_READWRITE|GTK_ARG_CONSTRUCT, ARG_TOPLEVEL);
}

static void
gnome_help_view_init (GnomeHelpView *help_view)
{
  help_view->orientation = GTK_ORIENTATION_VERTICAL;
  help_view->style = GNOME_HELP_BROWSER;
  help_view->style_prio = G_PRIORITY_LOW;

  help_view->toolbar = gtk_toolbar_new(GTK_ORIENTATION_VERTICAL, GTK_TOOLBAR_ICONS);
  help_view->btn_help = gtk_toolbar_append_item(GTK_TOOLBAR(help_view->toolbar), _("Help"),
						_("Show help for a specific region of the application"),
						NULL,
						gnome_stock_new_with_icon(GNOME_STOCK_PIXMAP_HELP),
						gnome_help_view_select_help_cb, help_view);
  help_view->btn_style = gtk_toolbar_append_item(GTK_TOOLBAR(help_view->toolbar), _("Style"),
						_("Change the way help is displayed"),
						NULL,
						gnome_stock_new_with_icon(GNOME_STOCK_PIXMAP_PREFERENCES),
						gnome_help_view_select_style, help_view);

  help_view->evbox = gtk_event_box_new();
  gtk_widget_add_events(help_view->evbox,
			GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_POINTER_MOTION_MASK|GDK_KEY_PRESS_MASK);

  gtk_box_pack_start(GTK_BOX(help_view), help_view->toolbar, TRUE, TRUE, GNOME_PAD_SMALL);
  gtk_box_pack_start(GTK_BOX(help_view), help_view->evbox, FALSE, FALSE, 0);

  gtk_widget_show(help_view->toolbar);
  gtk_widget_show(help_view->btn_style);
  gtk_widget_show(help_view->btn_help);
  gtk_widget_show(help_view->evbox);

  gnome_help_view_set_style(help_view, help_view->app_style, help_view->app_style_priority);
  gnome_help_view_set_orientation(help_view, GTK_ORIENTATION_HORIZONTAL);
}

static void
gnome_help_view_destroy (GtkObject *obj)
{
  gtk_object_remove_data(GTK_OBJECT(GNOME_HELP_VIEW(obj)->toplevel), GNOME_APP_HELP_VIEW_NAME);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (obj);
}

static void
gnome_help_view_set_arg (GtkObject *obj, GtkArg *arg, guint arg_id)
{
  GnomeHelpView *help_view = (GnomeHelpView *)obj;

  switch(arg_id)
    {
    case ARG_APP_STYLE:
      help_view->app_style = GTK_VALUE_ENUM(*arg);
      break;
    case ARG_APP_STYLE_PRIO:
      help_view->app_style_priority = GTK_VALUE_INT(*arg);
      break;
    case ARG_TOPLEVEL:
      help_view->toplevel = GTK_WIDGET(GTK_VALUE_OBJECT(*arg));
      gtk_object_set_data(GTK_VALUE_OBJECT(*arg), GNOME_APP_HELP_VIEW_NAME, help_view);
      break;
    }
}

static void
gnome_help_view_get_arg (GtkObject *obj, GtkArg *arg, guint arg_id)
{
  GnomeHelpView *help_view = GNOME_HELP_VIEW(obj);

  switch(arg_id)
    {
    case ARG_APP_STYLE:
      GTK_VALUE_ENUM(*arg) = help_view->style;
      break;
    case ARG_APP_STYLE_PRIO:
      GTK_VALUE_INT(*arg) = help_view->style_prio;
      break;
    }
}

GtkWidget *
gnome_help_view_new(GtkWidget *toplevel, GnomeHelpViewStyle app_style,
		    GnomeHelpViewStylePriority app_style_priority)
{
  return (GtkWidget *)gtk_object_new(gnome_help_view_get_type(),
				     "toplevel", toplevel,
				     "app_style", app_style,
				     "app_style_priority", app_style_priority,
				     NULL);
}

void
gnome_help_view_set_visibility(GnomeHelpView *help_view, gboolean visible)
{
  if(visible)
    {
      gtk_widget_show(help_view->content);
      gtk_widget_show(help_view->toolbar);
    }
  else
    {
      gtk_widget_hide(help_view->content);
      gtk_widget_hide(help_view->toolbar);
    }
}

GnomeHelpView *
gnome_help_view_find(GtkWidget *awidget)
{
  GtkWidget *tmpw;

  for(tmpw = gtk_widget_get_toplevel(awidget); tmpw && GTK_IS_MENU(tmpw); tmpw = GTK_MENU(tmpw)->toplevel);

  if(tmpw)
    tmpw = gtk_object_get_data(GTK_OBJECT(tmpw), GNOME_APP_HELP_VIEW_NAME);

  return GNOME_IS_HELP_VIEW(tmpw)?((GnomeHelpView *)tmpw):NULL;
}

/* size_request & size_allocate are generalizations of the same
   methods from gtk_hbox to make them work for both horizontal &
   vertical orientations */
static void
gnome_help_view_size_request  (GtkWidget      *widget,
			       GtkRequisition *requisition)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  gint nvis_children;
  gint length;
  guint16 *primary_axis = NULL;
  guint16 *secondary_axis = NULL;
  guint16 *primary_axis_child = NULL;
  guint16 *secondary_axis_child = NULL;
  GnomeHelpView *help_view;
  GtkRequisition child_requisition;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_HELP_VIEW (widget));
  g_return_if_fail (requisition != NULL);

  box = (GtkBox *)widget;
  help_view = (GnomeHelpView *)widget;
  requisition->width = 0;
  requisition->height = 0;
  nvis_children = 0;

  switch(help_view->orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      primary_axis = &requisition->width;
      primary_axis_child = &child_requisition.width;

      secondary_axis = &requisition->height;
      secondary_axis_child = &child_requisition.height;
      break;
    case GTK_ORIENTATION_VERTICAL:
      primary_axis = &requisition->height;
      primary_axis_child = &child_requisition.height;

      secondary_axis = &requisition->width;
      secondary_axis_child = &child_requisition.width;
      break;
    default:
      /* if this is reached the axis pointers would be very wrong */
      g_assert_not_reached();
    }

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  gtk_widget_size_request (child->widget, &child_requisition);

	  if (box->homogeneous)
	    {
	      length = *primary_axis_child + child->padding * 2;
	      *primary_axis = MAX (*primary_axis, length);
	    }
	  else
	    {
	      *primary_axis += *primary_axis_child + child->padding * 2;
	    }

	  *secondary_axis = MAX(*secondary_axis, *secondary_axis_child);

	  nvis_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (box->homogeneous)
	*primary_axis *= nvis_children;
      *primary_axis += (nvis_children - 1) * box->spacing;
    }

  requisition->width += GTK_CONTAINER (box)->border_width * 2;
  requisition->height += GTK_CONTAINER (box)->border_width * 2;
}

static void
gnome_help_view_size_allocate (GtkWidget      *widget,
			       GtkAllocation  *allocation)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  GtkAllocation child_allocation;
  gint nvis_children;
  gint nexpand_children;
  gint child_dimension;
  gint dimension;
  gint extra;
  gint position;
  gint16 *primary_axis_child = NULL, *secondary_axis_child = NULL;
  guint16 *primary_size_child = NULL, *secondary_size_child = NULL;
  guint16 *primary_req_size_child = NULL, *secondary_req_size_child = NULL;
  gint16 *primary_axis = NULL, *secondary_axis = NULL;
  guint16 *primary_size = NULL, *secondary_size = NULL;
  guint16 *primary_req_size = NULL, *secondary_req_size = NULL;
  GtkRequisition child_requisition;
  GnomeHelpView *help_view;
  gboolean got_req = FALSE;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_HELP_VIEW (widget));
  g_return_if_fail (allocation != NULL);

  box = (GtkBox *)widget;
  help_view = (GnomeHelpView *)widget;

  widget->allocation = *allocation;

  switch(help_view->orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      primary_axis = &allocation->x;
      primary_axis_child = &child_allocation.x;
      primary_size = &allocation->width;
      primary_size_child = &child_allocation.width;
      primary_req_size = &widget->requisition.width;
      primary_req_size_child = &child_requisition.width;

      secondary_axis = &allocation->y;
      secondary_axis_child = &child_allocation.y;
      secondary_size = &allocation->height;
      secondary_size_child = &child_allocation.height;
      secondary_req_size = &widget->requisition.height;
      secondary_req_size_child = &child_requisition.height;
      break;
    case GTK_ORIENTATION_VERTICAL:
      primary_axis = &allocation->y;
      primary_axis_child = &child_allocation.y;
      primary_size = &allocation->height;
      primary_size_child = &child_allocation.height;
      primary_req_size = &widget->requisition.height;
      primary_req_size_child = &child_requisition.height;

      secondary_axis = &allocation->x;
      secondary_axis_child = &child_allocation.x;
      secondary_size = &allocation->width;
      secondary_size_child = &child_allocation.width;
      secondary_req_size = &widget->requisition.width;
      secondary_req_size_child = &child_requisition.width;
      break;
    default:
      g_assert_not_reached();
    }

  nvis_children = 0;
  nexpand_children = 0;
  children = box->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  nvis_children += 1;
	  if (child->expand)
	    nexpand_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (box->homogeneous)
	{
	  dimension = (*primary_size -
		   GTK_CONTAINER (box)->border_width * 2 -
		   (nvis_children - 1) * box->spacing);
	  extra = dimension / nvis_children;
	}
      else if (nexpand_children > 0)
	{
	  dimension = (gint) *primary_size - (gint) *primary_req_size;
	  extra = dimension / nexpand_children;
	}
      else
	{
	  dimension = 0;
	  extra = 0;
	}

      position = *primary_axis + GTK_CONTAINER (box)->border_width;
      *secondary_axis_child = *secondary_axis + GTK_CONTAINER (box)->border_width;
      *secondary_size_child = MAX (1, (gint) *secondary_size - (gint) GTK_CONTAINER (box)->border_width * 2);

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  got_req = 0;

	  if ((child->pack == GTK_PACK_START) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      if (box->homogeneous)
		{
		  if (nvis_children == 1)
		    child_dimension = dimension;
		  else
		    child_dimension = extra;

		  nvis_children -= 1;
		  dimension -= extra;
		}
	      else
		{
		  got_req = TRUE;
		  gtk_widget_get_child_requisition (child->widget, &child_requisition);

		  child_dimension = *primary_req_size_child + child->padding * 2;

		  if (child->expand)
		    {
		      if (nexpand_children == 1)
			child_dimension += dimension;
		      else
			child_dimension += extra;

		      nexpand_children -= 1;
		      dimension -= extra;
		    }
		}

	      if (child->fill)
		{
		  *primary_size_child = MAX (1, (gint) child_dimension - (gint) child->padding * 2);
		  *primary_axis_child = position + child->padding;
		}
	      else
		{
		  if(!got_req)
		    gtk_widget_get_child_requisition (child->widget, &child_requisition);
		  *primary_size_child = *primary_req_size_child;
		  *primary_axis_child = position + (child_dimension - *primary_size_child) / 2;
		}

	      gtk_widget_size_allocate (child->widget, &child_allocation);

	      position += child_dimension + box->spacing;
	    }
	}

      position = *primary_axis + *primary_size - GTK_CONTAINER (box)->border_width;

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_END) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      gtk_widget_get_child_requisition (child->widget, &child_requisition);

              if (box->homogeneous)
                {
                  if (nvis_children == 1)
                    child_dimension = dimension;
                  else
                    child_dimension = extra;

                  nvis_children -= 1;
                  dimension -= extra;
                }
              else
                {
		  child_dimension = *primary_req_size_child + child->padding * 2;

                  if (child->expand)
                    {
                      if (nexpand_children == 1)
                        child_dimension += dimension;
                      else
                        child_dimension += extra;

                      nexpand_children -= 1;
                      dimension -= extra;
                    }
                }

              if (child->fill)
                {
                  *primary_size_child = MAX (1, (gint)child_dimension - (gint)child->padding * 2);
                  *primary_axis_child = position + child->padding - child_dimension;
                }
              else
                {
		  *primary_size_child = *primary_req_size_child;
                  *primary_axis_child = position + (child_dimension - *primary_size_child) / 2 - child_dimension;
                }

              gtk_widget_size_allocate (child->widget, &child_allocation);

              position -= (child_dimension + box->spacing);
	    }
	}
    }
}

static void
gnome_help_view_update_style(GnomeHelpView *help_view)
{
  if(help_view->style == GNOME_HELP_EMBEDDED)
    {
      if(help_view->content) /* Popup help */
	gtk_widget_destroy(help_view->content->parent);

      help_view->content = gnome_textfu_new();
      gtk_box_pack_start(GTK_BOX(help_view), help_view->content, TRUE, TRUE, GNOME_PAD_SMALL);
      gtk_widget_show(help_view->content);
    }
  else
    {
      if(help_view->content)
	{
	  gtk_container_remove(GTK_CONTAINER(help_view), help_view->content);
	  help_view->content = NULL;
	}
    }
}

static void
gnome_help_view_set_style_popup(gpointer dummy, GnomeHelpView *help_view)
{
  gnome_help_view_set_style(help_view, GNOME_HELP_POPUP, G_PRIORITY_HIGH);
}

static void
gnome_help_view_set_style_embedded(gpointer dummy, GnomeHelpView *help_view)
{
  gnome_help_view_set_style(help_view, GNOME_HELP_EMBEDDED, G_PRIORITY_HIGH);
}

static void
gnome_help_view_set_style_browser(gpointer dummy, GnomeHelpView *help_view)
{
  gnome_help_view_set_style(help_view, GNOME_HELP_BROWSER, G_PRIORITY_HIGH);
}

void
gnome_help_view_set_style(GnomeHelpView *help_view,
			  GnomeHelpViewStyle style,
			  GnomeHelpViewStylePriority style_priority)
{
  help_view->app_style = style;
  help_view->app_style_priority = style_priority;

  if(style_priority <= help_view->style_prio)
    {
      GnomeHelpViewStyle old_style = help_view->style;

      help_view->style = style;
      help_view->style_prio = style_priority;

      if(old_style != style)
	gnome_help_view_update_style(help_view);
    }

  gnome_help_view_set_orientation(help_view, help_view->orientation);
}

static void
gtk_widget_destroy_2(GtkWidget *x, GnomeHelpView *help_view)
{
  help_view->content = NULL;
}

static void
gnome_help_view_popup_activate_uri(GnomeTextFu *tf, const char *uri, GnomeHelpView *help_view)
{
  gnome_help_view_show_url(help_view, uri, URL_GENERAL_HELPSYSTEM);
}

static gint
do_popup_destroy(GtkWidget *win, GdkEvent *event)
{
  gtk_widget_destroy(win);
  return TRUE;
}

static void
gnome_help_view_popup(GnomeHelpView *help_view, const char *file_path)
{
  GnomeTextFu *tf;
  GtkWidget *win;

  if(!help_view->content)
    {
      win = gtk_window_new(GTK_WINDOW_POPUP);
      tf = GNOME_TEXTFU(gnome_textfu_new());
      gnome_textfu_load_file(tf, file_path);

      gtk_signal_connect_while_alive(GTK_OBJECT(win), "destroy", gtk_widget_destroy_2, help_view, GTK_OBJECT(help_view));
      gtk_signal_connect(GTK_OBJECT(help_view), "destroy", gtk_widget_destroy_2, win);

      gtk_signal_connect(GTK_OBJECT(tf), "activate_uri", gnome_help_view_popup_activate_uri, help_view);

      gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(tf));
      gtk_widget_show_all(win);

      gtk_signal_connect_after(GTK_OBJECT(win), "button_release_event", GTK_SIGNAL_FUNC(do_popup_destroy), NULL);
    }
  else
    gnome_textfu_load_file(GNOME_TEXTFU(help_view->content), file_path);
}

void
gnome_help_view_show_help(GnomeHelpView *help_view, const char *help_path, const char *help_type)
{
  char *file_type, *file_path;
  GnomeHelpViewStyle style;

  g_message("show_help: %s, %s", help_path, help_type);

  style = help_view->style;
  if(!help_type || strcmp(help_type, "popup"))
    style = GNOME_HELP_BROWSER;

  switch(style)
    {
    case GNOME_HELP_POPUP:
    case GNOME_HELP_EMBEDDED:
      file_type = "textfu";
      break;
    default:
      file_type = "html";
      break;
    }

  file_path = gnome_help_path_resolve(help_path, file_type);

  if(!file_path && !strcmp(file_type, "textfu"))
    {
      /* Fall back to browser if we can't find HTML */
      file_path = gnome_help_path_resolve(help_path, "html");
      style = GNOME_HELP_BROWSER;
    }

  if(!file_path)
    return;

  switch(style)
    {
    case GNOME_HELP_EMBEDDED:
      gnome_textfu_load_file(GNOME_TEXTFU(help_view->content), file_path);
      break;
    case GNOME_HELP_POPUP:
      gnome_help_view_popup(help_view, file_path);
      break;
    case GNOME_HELP_BROWSER:
      gnome_help_view_show_url(help_view, file_path, URL_GENERAL_HELPSYSTEM);
      break;
    }

  g_free(file_path);
}

void
gnome_help_view_show_help_for(GnomeHelpView *help_view, GtkWidget *widget)
{
  GtkWidget *cur;
  char *help_path = NULL;

  for(cur = widget; cur && !help_path; cur = cur->parent)
    {
      if(cur->name)
	help_path = gnome_help_view_find_help_id(help_view, cur->name);
    }

  if(help_path)
    gnome_help_view_show_help(help_view, help_path, "popup");
  else
    gnome_ok_dialog_parented(_("No help is available for the selected portion of the application."),
			     GTK_WINDOW(gtk_widget_get_toplevel(widget)));
}

void
gnome_help_view_set_orientation(GnomeHelpView *help_view, GtkOrientation orientation)
{
  help_view->orientation = orientation;

  switch(orientation)
    {
    case GTK_ORIENTATION_VERTICAL:
      if(help_view->style == GNOME_HELP_EMBEDDED)
	gtk_toolbar_set_orientation(GTK_TOOLBAR(help_view->toolbar), GTK_ORIENTATION_HORIZONTAL);
      else
	gtk_toolbar_set_orientation(GTK_TOOLBAR(help_view->toolbar), GTK_ORIENTATION_VERTICAL);
      break;
    case GTK_ORIENTATION_HORIZONTAL:
      if(help_view->style == GNOME_HELP_EMBEDDED)
	gtk_toolbar_set_orientation(GTK_TOOLBAR(help_view->toolbar), GTK_ORIENTATION_VERTICAL);
      else
	gtk_toolbar_set_orientation(GTK_TOOLBAR(help_view->toolbar), GTK_ORIENTATION_HORIZONTAL);
      break;
    }
}

/* The wonderful what-is-this protocol:
   Source:
     _EXPLAIN_QUERY - ask if help is supported
     _EXPLAIN_REQUEST - user has requested for an explanation

   Target:
     _EXPLAIN_QUERY_REPLY - affirm that help is supported
 */
typedef struct {
  GnomeHelpView *help_view;

  Window cur_toplevel;
  gboolean sent_query : 1, got_reply : 1;
} SourceState;

static GdkCursor *choose_cursor = NULL;

#if 0
static GdkFilterReturn
gnome_help_view_process_event(GdkXEvent *xevent, GdkEvent *event, GnomeHelpView *help_view)
{
  GdkFilterReturn retval = GDK_FILTER_CONTINUE;
  XEvent *xev = (XEvent *)xevent;

  switch(xev->type)
    {
    case ClientMessage:
      if(!atom_explain_query_reply)
	atom_explain_query_reply = gdk_atom_intern("_EXPLAIN_QUERY_REPLY", FALSE);
      retval = GDK_FILTER_REMOVE;
      break;
    case MotionNotify:
      retval = GDK_FILTER_REMOVE;
      break;
    case ButtonRelease:
      retval = GDK_FILTER_REMOVE;
      break;
    default:
      break;
    }

  return retval;
}
#else
static gint
gnome_help_view_process_event(GtkWidget *evbox, GdkEvent *event, GnomeHelpView *help_view)
{
  GdkEventButton *evb;
  GtkWidget *chosen_widget;

  if(event->type != GDK_BUTTON_RELEASE)
    return TRUE;
  
  evb = (GdkEventButton *)event;
  gtk_signal_disconnect_by_func(GTK_OBJECT(help_view->evbox), GTK_SIGNAL_FUNC(gnome_help_view_process_event), help_view);
  gtk_signal_emit_stop_by_name(GTK_OBJECT(evbox), "event");
  gtk_grab_remove(help_view->evbox);
  gdk_pointer_ungrab(evb->time);
  gdk_flush();

  if(evb->window)
    {
      gdk_window_get_user_data(evb->window, (gpointer *)&chosen_widget);

      if(help_view->btn_help && (chosen_widget == help_view->btn_help))
	/* do nothing - they canceled out of it */;
      else if(chosen_widget)
	gnome_help_view_show_help_for(help_view, chosen_widget);
      else
	gnome_ok_dialog_parented(_("No help is available for the selected portion of the application."),
				 GTK_WINDOW(gtk_widget_get_toplevel(help_view->btn_help)));
    }

  return TRUE;
#endif
}

void
gnome_help_view_select_help_menu_cb(GtkWidget *widget)
{
  GnomeHelpView *hv = gnome_help_view_find(widget);

  if(!hv)
    {
      g_warning("Can't find help view for this widget");
      return;
    }

  gnome_help_view_select_help_cb(widget, hv);
}

void
gnome_help_view_select_help_cb(GtkWidget *ignored, GnomeHelpView *help_view)
{
  g_return_if_fail(!help_view->evbox->window);

#if 0
  SourceState *ss;

  ss = g_new0(SourceState, 1);
  ss->help_view = help_view;
  ss->cur_toplevel = None;

  gdk_window_add_filter(help_view->evbox->window, gnome_help_view_process_event, ss);
#else
  gtk_signal_connect(GTK_OBJECT(help_view->evbox), "event", GTK_SIGNAL_FUNC(gnome_help_view_process_event), help_view);
#endif
  if(!choose_cursor)
    choose_cursor = gnome_stock_cursor_new(GNOME_STOCK_CURSOR_POINTING_HAND);
  gtk_grab_add(help_view->evbox);
  gdk_pointer_grab(help_view->evbox->window, TRUE,
		   GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_KEY_PRESS_MASK|GDK_POINTER_MOTION_MASK,
		   NULL, choose_cursor, GDK_CURRENT_TIME);
}

static void
popup_set_selection (GnomeHelpView *help_view)
{
  int n;

  switch(help_view->style)
    {
    case GNOME_HELP_POPUP:
      n = 0;
      break;
    case GNOME_HELP_EMBEDDED:
      n = 1;
      break;
    case GNOME_HELP_BROWSER:
      n = 2;
      break;
    default:
      n = -1;
    }

  if(n < 0)
    return;

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(popup_style_menu_items[n].widget), TRUE);
}

static void
gnome_help_view_select_style(GtkWidget *btn, GnomeHelpView *help_view)
{
  GnomeHelpViewClass *class = GNOME_HELP_VIEW_CLASS(GTK_OBJECT(help_view)->klass);

  gnome_popup_menu_do_popup(class->popup_menu, NULL, NULL, NULL, help_view, btn);
}

static char *
gnome_help_view_find_help_id(GnomeHelpView *help_view, const char *widget_id)
{
  char *fn, relfn[512];
  FILE *fh = NULL;
  char *retval = NULL;

  g_snprintf(relfn, sizeof(relfn), "help/%s/widget-help-map.txt", gnome_program_get_name(gnome_program_get()));
  fn = gnome_datadir_file(relfn);
  if(!fn)
    return NULL;

  fh = fopen(fn, "r");
  if(!fh)
    goto out;
  while(!retval && fgets(relfn, sizeof(relfn), fh))
    {
      char *ctmp;

      g_strstrip(relfn);
      if(relfn[0] == '#' || relfn[0] == '\0')
	continue;

      ctmp = strchr(relfn, '=');
      if(!ctmp)
	continue;
      *ctmp = '\0';

      if(strcmp(relfn, widget_id))
	continue;

      retval = g_strdup(ctmp + 1);
    }

 out:
  fclose(fh);
  return retval;
}

static void
gnome_help_view_show_url(GnomeHelpView *help_view, const char *url, HelpURLType type)
{
  char *url_type = NULL;

  switch(type)
    {
    case URL_SAME_APP:
    case URL_GENERAL_HELPSYSTEM:
      url_type = "help";
      break;
    case URL_WEB:
      url_type = NULL;
      break;
    }

  help_view->url_ctx = gnome_url_show_full(help_view->url_ctx, url, url_type,
					   GNOME_URL_DISPLAY_NEWWIN|GNOME_URL_DISPLAY_CLOSE_ATEXIT);
}

void
gnome_help_view_display (GnomeHelpView *help_view, const char *help_path)
{
  char *url;

  url = gnome_help_path_resolve(help_path, "html");

  if(url)
    {
      if(help_view)
	gnome_help_view_show_url(help_view, url, URL_GENERAL_HELPSYSTEM);
      else
	gnome_url_show(url);

      g_free(url);
    }
}

void
gnome_help_view_display_callback (GtkWidget *widget, const char *help_path)
{
  GnomeHelpView *help_view;

  help_view = gnome_help_view_find(widget);

  gnome_help_view_display(help_view, help_path);
}

/**
 * gnome_help_path_resolve:
 * @path: A help path (of the form appname/chaptername/sectionname
 * @file_type: The type of the file to be found, normally "html"
 *
 * Turns a "help path" into a full URL.
 */
char *
gnome_help_path_resolve(const char *path, const char *file_type)
{
  GList *language_list;
  char fnbuf[PATH_MAX], tmppath[PATH_MAX];
  char *appname, *filepath, *sectpath;
  char *res;
  char *ctmp;

  g_return_val_if_fail(path, NULL);
  if(!file_type)
    file_type = "html";

  strcpy(tmppath, path);
  appname = tmppath;

  filepath = strchr(tmppath, '/');

  if(filepath)
    {
      *filepath = '\0';
      filepath++;
      sectpath = strchr(filepath, '/');
      if(sectpath)
	{
	  *sectpath = '\0';
	  sectpath++;
	}
    }
  else
    {
      sectpath = NULL;
      filepath = appname;
    }

  for(language_list = gnome_i18n_get_language_list (NULL), res = NULL; !res && language_list;
      language_list = language_list->next)
    {
      const char *lang;
		
      lang = language_list->data;
		
      g_snprintf(fnbuf, sizeof(fnbuf), "%s/%s/%s.%s", appname, lang, filepath, file_type);
      res = gnome_help_file (fnbuf);
    }

  if(res)
    {
      ctmp = g_strdup_printf("file:///%s%s%s", res, sectpath?"#":"", sectpath?sectpath:"");
      g_free(res);
      res = ctmp;
    }
	
  return res;
}

/**
 * gnome_help_app_topics:
 * @app_id: The application's ID string
 *
 * Returns: A GSList of strings. The strings are in pairs, topic description then help path.
 */
GSList *
gnome_help_app_topics(const char *app_id)
{
  GList *language_list;
  GSList *retval;
  char *topicfn;
  char fnbuf[PATH_MAX], aline[LINE_MAX];
  FILE *fh;

  for(language_list = gnome_i18n_get_language_list (NULL), topicfn = NULL; !topicfn && language_list;
      language_list = language_list->next)
    {
      const char *lang;
		
      lang = language_list->data;
		
      g_snprintf(fnbuf, sizeof(fnbuf), "%s/%s/topic.list", app_id, lang);
      topicfn = gnome_help_file (fnbuf);

/*        language_list = language_list->next;  */
    }

  if(!topicfn)
    return NULL;

  fh = fopen(topicfn, "r");
  g_free(topicfn);
  if(!fh)
    return NULL;

  retval = NULL;
  while(fgets(aline, sizeof(aline), fh))
    {
      char *path_part, *name_part, *ctmp;

      g_strstrip(aline);

      if(aline[0] == '\0' || aline[0] == '#')
	continue;

      for(ctmp = aline; *ctmp && !isspace(*ctmp); ctmp++) /**/;
      if(*ctmp == '\0')
	continue;

      *ctmp = '\0'; ctmp++;
      path_part = aline;
      while(*ctmp && isspace(*ctmp)) ctmp++;
      if(*ctmp == '\0')
	continue;

      name_part = ctmp;

      g_snprintf(fnbuf, sizeof(fnbuf), "%s/%s", app_id, path_part);
      retval = g_slist_append(retval, g_strdup(name_part));
      retval = g_slist_append(retval, g_strdup(fnbuf));
    }

  return retval;
}

/**
 * gnome_widget_set_name:
 * @widget: A widget
 * @name: A name
 *
 * Sets the widget's name and returns the widget.
 */
GtkWidget *
gnome_widget_set_name(GtkWidget *widget, const char *name)
{
  gtk_widget_set_name(widget, name);

  return widget;
}

/**
 * gnome_widget_set_tooltip:
 * @widget: A widget
 * @tiptext: The text to be placed in the tooltip
 *
 * Returns:
 */
void
gnome_widget_set_tooltip (GtkWidget *widget, const char *tiptext)
{
  static GtkTooltips *tips = NULL;

  if(!tips)
    tips = gtk_tooltips_new();

  gtk_tooltips_set_tip(tips, widget, tiptext, NULL);
}
