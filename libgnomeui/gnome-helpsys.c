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

#include "config.h"
#include "gnome-macros.h"

#include <glib.h>
#include <ctype.h>

#include <libgnome/gnome-url.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-config.h>
#include <libgnome/gnome-i18n.h>
#include "gnome-helpsys.h"
#include "gnome-stock.h"
#include "gnome-uidefs.h"
#include "gnome-popup-menu.h"
#include "wap-textfu.h"
#include "gnome-cursors.h"
#include <libgnome/gnome-program.h>
#include <gdk/gdkx.h>

#include "libgnomeuiP.h"

#include <stdio.h>
#include <string.h>

struct _GnomeHelpViewPrivate {
  GtkWidget *popup_menu;

  GtkWidget *toplevel;
  GtkWidget *toolbar, *content, *btn_help, *btn_style, *evbox;

  GnomeURLDisplayContext *url_ctx;

  GtkOrientation orientation;

  GnomeHelpViewStylePriority style_priority;
  GnomeHelpViewStylePriority app_style_priority;

  GnomeHelpViewStyle style : 2;

  GnomeHelpViewStyle app_style : 2; /* Used to properly handle object args for style & prio */ 
};

enum {
  PROP_0 = 0,
  PROP_APP_STYLE,
  PROP_APP_STYLE_PRIORITY,
  PROP_ORIENTATION,
  PROP_TOPLEVEL
};

typedef enum {
  URL_SAME_APP,
  URL_GENERAL_HELPSYSTEM,
  URL_WEB
} HelpURLType;

static void gnome_help_view_class_init (GnomeHelpViewClass *class);
static void gnome_help_view_init (GnomeHelpView *help_view);
static void gnome_help_view_destroy (GtkObject *obj);
static void gnome_help_view_finalize (GObject *obj);
static void gnome_help_view_get_property	(GObject *object,
						 guint param_id,
						 GValue *value,
						 GParamSpec * pspec);
static void gnome_help_view_set_property	(GObject *object,
						 guint param_id,
						 const GValue * value,
						 GParamSpec * pspec);
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
  GNOMEUIINFO_RADIOITEM(N_("_Popup"),
			N_("Show help in temporary popup windows"),
			gnome_help_view_set_style_popup, NULL),
  GNOMEUIINFO_RADIOITEM(N_("_Embedded"),
			N_("Show help inside the application window"),
			gnome_help_view_set_style_embedded, NULL),
  GNOMEUIINFO_RADIOITEM(N_("_Browser"),
			N_("Show help in a help browser window"),
			gnome_help_view_set_style_browser, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo popup_style_menu[] = {
  GNOMEUIINFO_RADIOLIST(popup_style_menu_items),
  GNOMEUIINFO_END
};

static GdkCursor *choose_cursor = NULL;

/**
 * gnome_href_get_type
 *
 * Returns the type assigned to the GNOME href widget.
 **/
/* The following defines the get_type */
GNOME_CLASS_BOILERPLATE (GnomeHelpView, gnome_help_view,
			 GtkBox, gtk_box)

/* UNUSED
static GdkAtom atom_explain_query = 0, atom_explain_request = 0, atom_explain_query_reply = 0;
*/

static void
gnome_help_view_class_init (GnomeHelpViewClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	object_class->destroy = gnome_help_view_destroy;

	gobject_class->finalize = gnome_help_view_finalize;
	gobject_class->set_property = gnome_help_view_set_property;
	gobject_class->get_property = gnome_help_view_get_property;

	widget_class->size_request = gnome_help_view_size_request;
	widget_class->size_allocate = gnome_help_view_size_allocate;

	g_object_class_install_property (gobject_class,
				      PROP_APP_STYLE,
				      g_param_spec_enum ("app_style",
							 _("App style"),
							 _("The style of GnomeHelpView"),
							 GTK_TYPE_ENUM,
							 GNOME_HELP_BROWSER,
							 (G_PARAM_READABLE |
							  G_PARAM_WRITABLE)));

	g_object_class_install_property (gobject_class,
				      PROP_APP_STYLE_PRIORITY,
				      g_param_spec_enum ("app_style_priority",
							 _("App style priority"),
							 _("The priority of style of GnomeHelpView"),
							 GTK_TYPE_ENUM,
							 G_PRIORITY_LOW,
							 (G_PARAM_READABLE |
							  G_PARAM_WRITABLE)));

	g_object_class_install_property (gobject_class,
				      PROP_ORIENTATION,
				      g_param_spec_enum ("orientation",
							 _("Orientation"),
							 _("Orientation"),
							 GTK_TYPE_ENUM,
							 GTK_ORIENTATION_HORIZONTAL,
							 (G_PARAM_READABLE |
							  G_PARAM_WRITABLE)));

	g_object_class_install_property (gobject_class,
				      PROP_TOPLEVEL,
				      g_param_spec_object ("toplevel",
							   _("Toplevel"),
							   _("The toplevel widget"),
							   GTK_TYPE_WIDGET,
							   (G_PARAM_READABLE |
							    G_PARAM_WRITABLE)));
}

static void
gnome_help_view_init (GnomeHelpView *help_view)
{
  help_view->_priv = g_new0(GnomeHelpViewPrivate, 1);

  help_view->_priv->orientation = GTK_ORIENTATION_VERTICAL;
  help_view->_priv->style = GNOME_HELP_BROWSER;
  help_view->_priv->style_priority = G_PRIORITY_LOW;

  help_view->_priv->toolbar = gtk_toolbar_new(GTK_ORIENTATION_VERTICAL, GTK_TOOLBAR_ICONS);
  help_view->_priv->btn_help = gtk_toolbar_append_item(GTK_TOOLBAR(help_view->_priv->toolbar), _("Help"),
						_("Show help for a specific region of the application"),
						NULL,
						gnome_stock_new_with_icon(GNOME_STOCK_PIXMAP_HELP),
						GTK_SIGNAL_FUNC (gnome_help_view_select_help_cb),
						(gpointer) help_view);
  help_view->_priv->btn_style = gtk_toolbar_append_item(GTK_TOOLBAR(help_view->_priv->toolbar), _("Style"),
						_("Change the way help is displayed"),
						NULL,
						gnome_stock_new_with_icon(GNOME_STOCK_PIXMAP_PREFERENCES),
						GTK_SIGNAL_FUNC (gnome_help_view_select_style),
						(gpointer) help_view);

  help_view->_priv->evbox = gtk_event_box_new();
  gtk_widget_add_events(help_view->_priv->evbox,
			GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_POINTER_MOTION_MASK|GDK_KEY_PRESS_MASK);

  gtk_box_pack_start(GTK_BOX(help_view), help_view->_priv->toolbar, TRUE, TRUE, GNOME_PAD_SMALL);
  gtk_box_pack_start(GTK_BOX(help_view), help_view->_priv->evbox, FALSE, FALSE, 0);

  gtk_widget_show(help_view->_priv->toolbar);
  gtk_widget_show(help_view->_priv->btn_style);
  gtk_widget_show(help_view->_priv->btn_help);
  gtk_widget_show(help_view->_priv->evbox);

  gnome_help_view_set_style(help_view, help_view->_priv->app_style, help_view->_priv->app_style_priority);
  gnome_help_view_set_orientation(help_view, GTK_ORIENTATION_HORIZONTAL);
}

static void
gnome_help_view_destroy (GtkObject *obj)
{
	GnomeHelpView *help_view = (GnomeHelpView *)obj;

	/* remember, destroy can be run multiple times! */

	if(help_view->_priv->popup_menu != NULL) {
		gtk_widget_destroy(help_view->_priv->popup_menu);
		help_view->_priv->popup_menu = NULL;
	}

	if(help_view->_priv->toplevel != NULL) {
		gtk_object_remove_data(GTK_OBJECT(help_view->_priv->toplevel),
				       GNOME_APP_HELP_VIEW_NAME);
		gtk_object_unref(GTK_OBJECT(help_view->_priv->toplevel));
		help_view->_priv->toplevel = NULL;
	}

	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (obj));
}

static void
gnome_help_view_finalize (GObject *obj)
{
	GnomeHelpView *help_view = (GnomeHelpView *)obj;

	g_free(help_view->_priv);
	help_view->_priv = NULL;

	if (choose_cursor != NULL) {
		gdk_cursor_destroy (choose_cursor);
		choose_cursor = NULL;
	}

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (obj));
}

static void
gnome_help_view_set_property (GObject *object,
			      guint param_id,
			      const GValue * value,
			      GParamSpec * pspec)
{
	static gboolean set_style = FALSE;
	static gboolean set_priority = FALSE;
	static GnomeHelpViewStyle style = GNOME_HELP_BROWSER;
	static GnomeHelpViewStylePriority priority = G_PRIORITY_LOW;

	GnomeHelpView *help_view = (GnomeHelpView *)object;

	switch(param_id) {
	case PROP_APP_STYLE:
		style = g_value_get_enum (value);
		set_style = TRUE;
		break;
	case PROP_APP_STYLE_PRIORITY:
		priority = g_value_get_enum (value);
		set_priority = TRUE;
		break;
	case PROP_ORIENTATION:
		gnome_help_view_set_orientation (help_view, 
						 g_value_get_enum (value));
		break;
	case PROP_TOPLEVEL:
		gnome_help_view_set_toplevel (help_view,
					      (GtkWidget *) g_value_get_object (value));
		break;
	}

	/* A bad hack, style and priority need to be set at the same
	 * time, this is ugly, should be somehow gotten rid of, by
	 * redesigning the priority beast
	 * -George */
	if (set_style && set_priority) {
		gnome_help_view_set_style(help_view, style, priority);
		set_style = FALSE;
		set_priority = FALSE;
	}
}

static void
gnome_help_view_get_property (GObject *object,
			   guint param_id,
			   GValue *value,
			   GParamSpec * pspec)
{
	GnomeHelpView *help_view = GNOME_HELP_VIEW (object);

	switch(param_id) {
	case PROP_APP_STYLE:
		g_value_set_enum (value, help_view->_priv->style);
		break;
	case PROP_APP_STYLE_PRIORITY:
		g_value_set_enum (value, help_view->_priv->style_priority);
		break;
	case PROP_ORIENTATION:
		g_value_set_enum (value, help_view->_priv->orientation);
		break;
	case PROP_TOPLEVEL:
		/* Don't use G_OBJECT cast as it could be null */
		g_value_set_object (value, (GObject *)help_view->_priv->toplevel);
		break;
	}
}

GtkWidget *
gnome_help_view_new(GtkWidget *toplevel, GnomeHelpViewStyle app_style,
		    GnomeHelpViewStylePriority app_style_priority)
{
  return (GtkWidget *)g_object_new (gnome_help_view_get_type(),
				    "toplevel", toplevel,
				    "app_style", app_style,
				    "app_style_priority", app_style_priority,
				    NULL);
}

void
gnome_help_view_construct(GnomeHelpView *self,
			  GtkWidget *toplevel,
			  GnomeHelpViewStyle app_style,
			  GnomeHelpViewStylePriority app_style_priority)
{
	g_return_if_fail (self != NULL);
	g_return_if_fail (GNOME_IS_HELP_VIEW (self));

	g_object_set (G_OBJECT (self),
		      "toplevel", toplevel,
		      "app_style", app_style,
		      "app_style_priority", app_style_priority,
		      NULL);
}

void
gnome_help_view_set_toplevel(GnomeHelpView *help_view,
			     GtkWidget *toplevel)
{
	g_return_if_fail(help_view != NULL);
	g_return_if_fail(GNOME_IS_HELP_VIEW(help_view));
	g_return_if_fail(toplevel != NULL);
	g_return_if_fail(GTK_IS_WIDGET(toplevel));

	if(help_view->_priv->toplevel) {
		gtk_object_remove_data(GTK_OBJECT(help_view->_priv->toplevel),
				       GNOME_APP_HELP_VIEW_NAME);
		gtk_object_unref(GTK_OBJECT(help_view->_priv->toplevel));
	}
	help_view->_priv->toplevel = toplevel;
	gtk_object_ref(GTK_OBJECT(help_view->_priv->toplevel));
	gtk_object_set_data(GTK_OBJECT(toplevel), GNOME_APP_HELP_VIEW_NAME,
			    help_view);
}

void
gnome_help_view_set_visibility(GnomeHelpView *help_view, gboolean visible)
{
  if(visible)
    {
      gtk_widget_show(help_view->_priv->content);
      gtk_widget_show(help_view->_priv->toolbar);
    }
  else
    {
      gtk_widget_hide(help_view->_priv->content);
      gtk_widget_hide(help_view->_priv->toolbar);
    }
}

GnomeHelpView *
gnome_help_view_find(GtkWidget *awidget)
{
  GtkWidget *tmpw;
  GnomeHelpView *help_view;

  for(tmpw = gtk_widget_get_toplevel(awidget);
      tmpw && GTK_IS_MENU(tmpw);
      tmpw = GTK_MENU(tmpw)->toplevel)
	  ;

  if(tmpw)
	  help_view = gtk_object_get_data(GTK_OBJECT(tmpw),
					  GNOME_APP_HELP_VIEW_NAME);
  else
	  help_view = NULL;

  if(GNOME_IS_HELP_VIEW(tmpw))
	  return help_view;
  else
	  return NULL;
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
  gint *primary_axis = NULL;
  gint *secondary_axis = NULL;
  gint *primary_axis_child = NULL;
  gint *secondary_axis_child = NULL;
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

  switch(help_view->_priv->orientation)
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
  gint *primary_axis_child = NULL, *secondary_axis_child = NULL;
  gint *primary_size_child = NULL, *secondary_size_child = NULL;
  gint *primary_req_size_child = NULL, *secondary_req_size_child = NULL;
  gint *primary_axis = NULL, *secondary_axis = NULL;
  gint *primary_size = NULL, *secondary_size = NULL;
  gint *primary_req_size = NULL, *secondary_req_size = NULL;
  GtkRequisition child_requisition;
  GnomeHelpView *help_view;
  gboolean got_req = FALSE;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_HELP_VIEW (widget));
  g_return_if_fail (allocation != NULL);

  box = (GtkBox *)widget;
  help_view = (GnomeHelpView *)widget;

  widget->allocation = *allocation;

  switch(help_view->_priv->orientation)
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
  if(help_view->_priv->style == GNOME_HELP_EMBEDDED)
    {
      if(help_view->_priv->content) /* Popup help */
	gtk_widget_destroy(help_view->_priv->content->parent);

      help_view->_priv->content = wap_textfu_new();
      gtk_box_pack_start(GTK_BOX(help_view), help_view->_priv->content, TRUE, TRUE, GNOME_PAD_SMALL);
      gtk_widget_show(help_view->_priv->content);
    }
  else
    {
      if(help_view->_priv->content)
	{
	  gtk_container_remove(GTK_CONTAINER(help_view), help_view->_priv->content);
	  help_view->_priv->content = NULL;
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
  help_view->_priv->app_style = style;
  help_view->_priv->app_style_priority = style_priority;

  if(style_priority <= help_view->_priv->style_priority)
    {
      GnomeHelpViewStyle old_style = help_view->_priv->style;

      help_view->_priv->style = style;
      help_view->_priv->style_priority = style_priority;

      if(old_style != style)
	gnome_help_view_update_style(help_view);
    }

  gnome_help_view_set_orientation(help_view, help_view->_priv->orientation);
}

static void
gtk_widget_destroy_2(GtkWidget *x, GnomeHelpView *help_view)
{
  help_view->_priv->content = NULL;
}

static void G_GNUC_UNUSED
gnome_help_view_popup_activate_uri(WapTextFu *tf, const char *uri, GnomeHelpView *help_view)
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
  WapTextFu *tf;
  GtkWidget *win;

  if(!help_view->_priv->content)
    {
      win = gtk_window_new(GTK_WINDOW_POPUP);
      tf = WAP_TEXTFU(wap_textfu_new());
      wap_textfu_load_file(tf, file_path);

      gtk_signal_connect_while_alive(GTK_OBJECT(win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroy_2), help_view, GTK_OBJECT(help_view));
      gtk_signal_connect(GTK_OBJECT(help_view), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroy_2), win);

/*        gtk_signal_connect(GTK_OBJECT(tf), "activate_uri", GTK_SIGNAL_FUNC(gnome_help_view_popup_activate_uri), help_view); */

      gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(tf));
      gtk_widget_show_all(win);

      gtk_signal_connect_after(GTK_OBJECT(win), "button_release_event", GTK_SIGNAL_FUNC(do_popup_destroy), NULL);
    }
  else
    wap_textfu_load_file(WAP_TEXTFU(help_view->_priv->content), file_path);
}

void
gnome_help_view_show_help(GnomeHelpView *help_view, const char *help_path, const char *help_type)
{
  char *file_type, *file_path;
  GnomeHelpViewStyle style;

  g_message("show_help: %s, %s", help_path, help_type);

  style = help_view->_priv->style;
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
      wap_textfu_load_file(WAP_TEXTFU(help_view->_priv->content), file_path);
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
      if(cur->name) {
	g_print ("widget: %s\n", cur->name);
	help_path = gnome_help_view_find_help_id(help_view, cur->name);
      }
    }

  if(help_path)
    gnome_help_view_show_help(help_view, help_path, "popup");
  else
    {
      GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_INFO,
						 GTK_BUTTONS_OK,
						 _("No help is available for the selected "
						   "portion of the application."));
      gtk_dialog_run(GTK_DIALOG(dialog));
    }
}

void
gnome_help_view_set_orientation(GnomeHelpView *help_view, GtkOrientation orientation)
{
  help_view->_priv->orientation = orientation;

  switch(orientation)
    {
    case GTK_ORIENTATION_VERTICAL:
      if(help_view->_priv->style == GNOME_HELP_EMBEDDED)
	gtk_toolbar_set_orientation(GTK_TOOLBAR(help_view->_priv->toolbar), GTK_ORIENTATION_HORIZONTAL);
      else
	gtk_toolbar_set_orientation(GTK_TOOLBAR(help_view->_priv->toolbar), GTK_ORIENTATION_VERTICAL);
      break;
    case GTK_ORIENTATION_HORIZONTAL:
      if(help_view->_priv->style == GNOME_HELP_EMBEDDED)
	gtk_toolbar_set_orientation(GTK_TOOLBAR(help_view->_priv->toolbar), GTK_ORIENTATION_VERTICAL);
      else
	gtk_toolbar_set_orientation(GTK_TOOLBAR(help_view->_priv->toolbar), GTK_ORIENTATION_HORIZONTAL);
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
  gtk_signal_disconnect_by_func(GTK_OBJECT(help_view->_priv->evbox), GTK_SIGNAL_FUNC(gnome_help_view_process_event), help_view);
  gtk_signal_emit_stop_by_name(GTK_OBJECT(evbox), "event");
  gtk_grab_remove(help_view->_priv->evbox);
/*    gdk_pointer_ungrab(evb->time); */
  gdk_window_set_cursor (help_view->_priv->toplevel->window, NULL);
  gdk_flush();

  if(evb->window)
    {
      gdk_window_get_user_data(evb->window, (gpointer *)&chosen_widget);

      if(help_view->_priv->btn_help && (chosen_widget == help_view->_priv->btn_help))
	/* do nothing - they canceled out of it */;
      else if(chosen_widget)
	gnome_help_view_show_help_for(help_view, chosen_widget);
      else
	{
	  GtkWidget *dialog;

	  dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(help_view->_priv->btn_help)),
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  GTK_MESSAGE_INFO,
					  GTK_BUTTONS_OK,
					  _("No help is available for the selected "
					    "portion of the application."));
	  gtk_dialog_run(GTK_DIALOG(dialog));
	}
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
  g_return_if_fail(help_view->_priv->evbox->window != NULL);

#if 0
  SourceState *ss;

  ss = g_new0(SourceState, 1);
  ss->help_view = help_view;
  ss->cur_toplevel = None;

  gdk_window_add_filter(help_view->_priv->evbox->window, gnome_help_view_process_event, ss);
#else

  gtk_signal_connect(GTK_OBJECT(help_view->_priv->evbox), "event", 
		     GTK_SIGNAL_FUNC(gnome_help_view_process_event), 
		     help_view);

#endif

  if(!choose_cursor)
    choose_cursor = gnome_stock_cursor_new(GNOME_STOCK_CURSOR_WHATISIT);

  gtk_grab_add(help_view->_priv->evbox);
  gdk_window_set_cursor (help_view->_priv->toplevel->window, 
			 choose_cursor);

#if 0
  gdk_pointer_grab(help_view->_priv->evbox->window, TRUE, 
  		   GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_KEY_PRESS_MASK|GDK_POINTER_MOTION_MASK, 
  		   NULL, choose_cursor, GDK_CURRENT_TIME); 
#endif
}

#if 0
/*UNUSED*/
static void
popup_set_selection (GnomeHelpView *help_view)
{
  int n;

  switch(help_view->_priv->style)
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
#endif

static void
gnome_help_view_select_style(GtkWidget *btn, GnomeHelpView *help_view)
{
  if( ! help_view->_priv->popup_menu)
	  help_view->_priv->popup_menu =gnome_popup_menu_new(popup_style_menu);


  gnome_popup_menu_do_popup(help_view->_priv->popup_menu,
			    NULL, NULL, NULL, help_view, btn);
}

static char *
gnome_help_view_find_help_id(GnomeHelpView *help_view, const char *widget_id)
{
  char *filename, buf[512];
  FILE *fh = NULL;
  char *retval = NULL;
  
  g_snprintf(buf, sizeof(buf),
	     "help/%s/widget-help-map.txt",
	     gnome_program_get_name(gnome_program_get()));
  filename = gnome_program_locate_file (gnome_program_get (),
					GNOME_FILE_DOMAIN_DATADIR,
					buf, TRUE, NULL);

  if(filename == NULL) {
    /* Search the current directory as well */
    if (g_file_test ("widget-help-map.txt", G_FILE_TEST_EXISTS))
      filename = g_strdup ("widget-help-map.txt");
    else
      return NULL;
  }
  
  fh = fopen(filename, "r");
  g_free(filename);
  
  if(!fh)
    return NULL;
  
  while(!retval && fgets(buf, sizeof(buf), fh))
    {
      char *ctmp;
      
      g_strstrip(buf);
      if(buf[0] == '#' || buf[0] == '\0')
	continue;
      
      ctmp = strchr(buf, '=');
      if(!ctmp)
	continue;
      *ctmp = '\0';
      
      if(strcmp(buf, widget_id))
	continue;
      
      retval = g_strdup(ctmp + 1);
    }
  
  fclose(fh);
  return retval;
}

static void
gnome_help_view_show_url(GnomeHelpView *help_view, const char *url, HelpURLType type)
{
  char *url_type = NULL;
  GError *error = NULL;

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

  help_view->_priv->url_ctx =
	  gnome_url_show_full(help_view->_priv->url_ctx, url, url_type,
			      GNOME_URL_DISPLAY_NEWWIN |
			      GNOME_URL_DISPLAY_CLOSE_ATEXIT,
			      &error);
  if(error != NULL) {
  /*FIXME: properly handle the error!*/
	  g_warning (_("Cought GnomeURL error: %s"), error->message);
	  g_clear_error (&error);
  }
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
        /* FIXME: Handle errors */

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
 *
 * Returns: a newly allocated string with the URL
 */
char *
gnome_help_path_resolve(const char *path, const char *file_type)
{
  const GList *language_list;
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
      res = gnome_program_locate_file (gnome_program_get (),
				       GNOME_FILE_DOMAIN_HELP,
				       fnbuf, TRUE, NULL);
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
 * Returns: A newly allocated GSList of newly allocate strings.
 * The strings are in pairs, topic description then help path.
 */
GSList *
gnome_help_app_topics(const char *app_id)
{
  const GList *language_list;
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
      topicfn = gnome_program_locate_file (gnome_program_get (),
					   GNOME_FILE_DOMAIN_HELP,
					   fnbuf, TRUE, NULL);

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
	g_return_val_if_fail(widget != NULL, NULL);
	g_return_val_if_fail(GTK_IS_WIDGET(widget), NULL);

	gtk_widget_set_name(widget, name);

	return widget;
}

/*
 * Tooltips
 */

static GtkTooltips *tooltips = NULL;

/**
 * gnome_widget_set_tooltip:
 * @widget: A widget
 * @tiptext: The text to be placed in the tooltip
 *
 * Description:  Sets a tooltip for a widget
 */
void
gnome_widget_set_tooltip (GtkWidget *widget, const char *tiptext)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(GTK_IS_WIDGET(widget));

	if( ! tooltips)
		tooltips = gtk_tooltips_new();

	gtk_tooltips_set_tip(tooltips, widget, tiptext, NULL);
}

/**
 * gnome_widget_get_tooltips:
 *
 * Description:  Gets the tooltips object so that you can
 * manipulate tooltips that were set with #gnome_widget_set_tooltip
 * 
 * Returns:  a #GtkTooltips object
 */
GtkTooltips *
gnome_widget_get_tooltips (void)
{
	if( ! tooltips)
		tooltips = gtk_tooltips_new();

	return tooltips;
}
