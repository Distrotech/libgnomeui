/* TODO:
 * Turn GnomeHelpView into a widget with a window (cut & paste eventbox code) so that we can handle right-click on it
 * to get the style menu
 *
 * Add sanity checking to all parameters of API functions
 */

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
#include <libgnome/gnomelib-init2.h>

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
static void gnome_help_view_select_help(GtkWidget *btn, GnomeHelpView *help_view);
static void gnome_help_view_select_style(GtkWidget *btn, GnomeHelpView *help_view);
static void gnome_help_view_interact(GtkWidget *btn, GnomeHelpView *help_view);
static char *gnome_help_view_find_help_id(GnomeHelpView *help_view, const char *widget_id);
static const char *gnome_help_view_find_widget_id(GnomeHelpView *help_view, GtkWidget *widget);
static char *gnome_help_view_get_base_url(void);
static void gnome_help_view_show_url(GnomeHelpView *help_view, const char *url, HelpURLType type);
static void gnome_help_view_set_style_popup(gpointer dummy, GnomeHelpView *help_view);
static void gnome_help_view_set_style_embedded(gpointer dummy, GnomeHelpView *help_view);
static void gnome_help_view_set_style_browser(gpointer dummy, GnomeHelpView *help_view);
static gint popup_button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data); 

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
  GnomeHelpViewClass *class;

  help_view->orientation = GTK_ORIENTATION_VERTICAL;
  help_view->style = GNOME_HELP_BROWSER;
  help_view->style_prio = G_PRIORITY_LOW;

  help_view->content = gtk_text_new(NULL, NULL); /* For now */
  help_view->toolbar = gtk_toolbar_new(GTK_ORIENTATION_VERTICAL, GTK_TOOLBAR_ICONS);
  help_view->btn_help = gtk_toolbar_append_item(GTK_TOOLBAR(help_view->toolbar), _("Help"),
						_("Show help for a specific region of the application"),
						NULL,
						gnome_stock_pixmap_widget(GTK_WIDGET(help_view), GNOME_STOCK_PIXMAP_HELP),
						gnome_help_view_select_help, help_view);
  help_view->btn_style = gtk_toolbar_append_item(GTK_TOOLBAR(help_view->toolbar), _("Style"),
						_("Change the way help is displayed"),
						NULL,
						gnome_stock_pixmap_widget(GTK_WIDGET(help_view), GNOME_STOCK_PIXMAP_PREFERENCES),
						gnome_help_view_select_style, help_view);
  help_view->btn_contribute = gtk_toolbar_append_item(GTK_TOOLBAR(help_view->toolbar), _("Interact"),
						      _("Discuss this application with other users"),
						      NULL,
						      gnome_stock_pixmap_widget(GTK_WIDGET(help_view), GNOME_STOCK_PIXMAP_MIC),
						      gnome_help_view_interact, help_view);

  /* XXX fixme */
#if 0
  class = GNOME_HELP_VIEW_CLASS(GTK_OBJECT(help_view)->klass);
  gtk_widget_add_events(GTK_WIDGET(help_view), GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect (GTK_OBJECT(help_view), "button_press_event",
		      GTK_SIGNAL_FUNC(popup_button_pressed), help_view);
#endif

  gtk_box_pack_start(GTK_BOX(help_view), help_view->toolbar, TRUE, TRUE, GNOME_PAD_SMALL);
  gtk_box_pack_start(GTK_BOX(help_view), help_view->content, TRUE, TRUE, GNOME_PAD_SMALL);

  gtk_widget_show(help_view->toolbar);
  gtk_widget_show(help_view->btn_style);
  gtk_widget_show(help_view->btn_help);
  gtk_widget_show(help_view->btn_contribute);

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
  GnomeHelpView *help_view;

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
  guint16 *primary_axis, *secondary_axis, *primary_axis_child, *secondary_axis_child;
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
  gint16 *primary_axis_child, *secondary_axis_child;
  guint16 *primary_size_child, *secondary_size_child, *primary_req_size_child, *secondary_req_size_child;
  gint16 *primary_axis, *secondary_axis;
  guint16 *primary_size, *secondary_size, *primary_req_size, *secondary_req_size;
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
    gtk_widget_show(help_view->content);
  else
    gtk_widget_hide(help_view->content);
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

void
gnome_help_view_show_help(GnomeHelpView *help_view, const char *help_path)
{
}

static const char *
gnome_help_view_find_widget_id(GnomeHelpView *help_view, GtkWidget *widget)
{
  GtkWidget *cur;

  for(cur = widget; cur; cur = cur->parent)
    {
      if(cur->name)
	return cur->name;
    }

  return NULL;
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
}

void
gnome_help_view_set_orientation(GnomeHelpView *help_view, GtkOrientation orientation)
{
  help_view->orientation = orientation;

  /*
    g_print("set_orientation %s\n", (orientation == GTK_ORIENTATION_VERTICAL)?"GTK_ORIENTATION_VERTICAL":"GTK_ORIENTATION_HORIZONTAL");
  */
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

static void
gnome_help_view_select_help(GtkWidget *btn, GnomeHelpView *help_view)
{
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

static gint
popup_button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  GnomeHelpViewClass *class = GNOME_HELP_VIEW_CLASS(GTK_OBJECT(data)->klass);

  if(event->button != 3)
    return FALSE;

  gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "button_press_event");

  popup_set_selection((GnomeHelpView *)data);

  gnome_popup_menu_do_popup(class->popup_menu, NULL, NULL, event, data);

  return TRUE;
}

static void
gnome_help_view_select_style(GtkWidget *btn, GnomeHelpView *help_view)
{
  GnomeHelpViewClass *class = GNOME_HELP_VIEW_CLASS(GTK_OBJECT(help_view)->klass);

  gnome_popup_menu_do_popup(class->popup_menu, NULL, NULL, NULL, help_view);
}

static void
gnome_help_view_interact(GtkWidget *btn, GnomeHelpView *help_view)
{
  char *baseurl;
  char *fullurl;
  const char *widget_id;
  char *help_id;
  const char *app_version, *app_id;
  GnomeProgram *prog;
  int slen;

  baseurl = gnome_help_view_get_base_url();
  widget_id = gnome_help_view_find_widget_id(help_view, help_view->toplevel);

  if(widget_id)
    help_id = gnome_help_view_find_help_id(help_view, widget_id);
  else
    help_id = NULL;

  prog = gnome_program_get();
  app_id = gnome_program_get_name(prog);
  app_version = gnome_program_get_version(prog);

  slen = strlen(baseurl);
  slen += sizeof("goto.cgi?location=help_view&widgetID=&appID=&appVersion=&widgetID=&helpID=");
  slen += widget_id?strlen(widget_id):0;
  slen += help_id?strlen(help_id):0;
  slen += strlen(app_id);
  slen += strlen(app_version);
  fullurl = g_alloca(slen);

  sprintf(fullurl, "%s/jump.cgi?location=help_view&appID=%s&appVersion=%s%s%s%s%s", baseurl, app_id, app_version,
	  widget_id?"&widgetID=":"",
	  widget_id?widget_id:"",
	  help_id?"&helpID=":"",
	  help_id?help_id:"");
  g_free(baseurl);
  g_free(help_id);

  gnome_help_view_show_url(help_view, fullurl, URL_WEB);
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

static char *
gnome_help_view_get_base_url(void)
{
  return gnome_config_get_string("/Web/InteractSite=http://www.gnome.org/interact");
}

static void
gnome_help_view_show_url(GnomeHelpView *help_view, const char *url, HelpURLType type)
{
  gnome_url_show(url);
}

void
gnome_help_view_display(gpointer ignore, GnomeHelpMenuEntry *ent)
{
  gchar *file, *url;
	
  g_assert(ent != NULL);
  g_assert(ent->path != NULL);
  g_assert(ent->name != NULL);

  file = gnome_help_file_path (ent->name, ent->path);
	
  if (!file)
    return;
	
  url = g_alloca (strlen (file)+10);
  strcpy (url,"ghelp:");
  strcat (url, file);
  gnome_url_show(url);
  g_free (file);
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
