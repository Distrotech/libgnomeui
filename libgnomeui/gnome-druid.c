/* gnome-druid.c
 * Copyright (C) 1999 Red Hat, Inc.
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
#include <config.h>
#include "gnome-macros.h"

#include "gnome-druid.h"
#include "gnome-stock.h"
#include "gnome-uidefs.h"
#include <libgnome/gnome-i18n.h>

struct _GnomeDruidPrivate
{
	GnomeDruidPage *current;
	GList *children;

	gboolean show_finish : 1; /* if TRUE, then we are showing the finish button instead of the next button */
};

enum {
	CANCEL,
	LAST_SIGNAL
};
static void gnome_druid_init		(GnomeDruid		 *druid);
static void gnome_druid_class_init	(GnomeDruidClass	 *klass);
static void gnome_druid_destroy         (GtkObject               *object);
static void gnome_druid_finalize        (GObject                 *object);
static void gnome_druid_size_request    (GtkWidget               *widget,
					 GtkRequisition          *requisition);
static void gnome_druid_size_allocate   (GtkWidget               *widget,
					 GtkAllocation           *allocation);
static gint gnome_druid_expose          (GtkWidget               *widget,
					 GdkEventExpose          *event);
static void gnome_druid_map             (GtkWidget               *widget);
static void gnome_druid_unmap           (GtkWidget               *widget);
static GtkType gnome_druid_child_type   (GtkContainer            *container);
static void gnome_druid_add             (GtkContainer            *widget,
					 GtkWidget               *page);
static void gnome_druid_remove          (GtkContainer            *widget,
					 GtkWidget               *child);
static void gnome_druid_forall          (GtkContainer            *container,
					 gboolean                include_internals,
					 GtkCallback             callback,
					 gpointer                callback_data);
static void gnome_druid_back_callback   (GtkWidget               *button,
					 GnomeDruid              *druid);
static void gnome_druid_next_callback   (GtkWidget               *button,
					 GnomeDruid              *druid);
static void gnome_druid_cancel_callback (GtkWidget               *button,
					 GtkWidget               *druid);

static guint druid_signals[LAST_SIGNAL] = { 0 };

/* define the _get_type method and parent_class */
GNOME_CLASS_BOILERPLATE(GnomeDruid, gnome_druid, GtkContainer, gtk_container)

static void
gnome_druid_class_init (GnomeDruidClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	GtkContainerClass *container_class;

	object_class = (GtkObjectClass*) klass;
	gobject_class = (GObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;
	container_class = (GtkContainerClass*) klass;

	druid_signals[CANCEL] = 
		gtk_signal_new ("cancel",
				GTK_RUN_LAST,
				GTK_CLASS_TYPE (object_class),
				GTK_SIGNAL_OFFSET (GnomeDruidClass, cancel),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);
	
	
	object_class->destroy = gnome_druid_destroy;
	gobject_class->finalize = gnome_druid_finalize;
	widget_class->size_request = gnome_druid_size_request;
	widget_class->size_allocate = gnome_druid_size_allocate;
	widget_class->map = gnome_druid_map;
	widget_class->unmap = gnome_druid_unmap;
	widget_class->expose_event = gnome_druid_expose;

	container_class->forall = gnome_druid_forall;
	container_class->add = gnome_druid_add;
	container_class->remove = gnome_druid_remove;
	container_class->child_type = gnome_druid_child_type;
}

static void
gnome_druid_init (GnomeDruid *druid)
{
	druid->_priv = g_new0(GnomeDruidPrivate, 1);

	/* set the default border width */
	GTK_CONTAINER (druid)->border_width = GNOME_PAD_SMALL;

	/* set up the buttons */
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (druid), GTK_NO_WINDOW);
	druid->back = gtk_button_new_from_stock (GNOME_STOCK_BUTTON_PREV);
	GTK_WIDGET_SET_FLAGS (druid->back, GTK_CAN_DEFAULT);
	druid->next = gtk_button_new_from_stock (GNOME_STOCK_BUTTON_NEXT);
	GTK_WIDGET_SET_FLAGS (druid->next, GTK_CAN_DEFAULT);
	GTK_WIDGET_SET_FLAGS (druid->next, GTK_HAS_FOCUS);
	druid->cancel = gtk_button_new_from_stock (GNOME_STOCK_BUTTON_CANCEL);
	GTK_WIDGET_SET_FLAGS (druid->cancel, GTK_CAN_DEFAULT);
	druid->finish = gtk_button_new_from_stock (GNOME_STOCK_BUTTON_APPLY);
	GTK_WIDGET_SET_FLAGS (druid->finish, GTK_CAN_DEFAULT);
	gtk_widget_set_parent (druid->back, GTK_WIDGET (druid));
	gtk_widget_set_parent (druid->next, GTK_WIDGET (druid));
	gtk_widget_set_parent (druid->cancel, GTK_WIDGET (druid));
	gtk_widget_set_parent (druid->finish, GTK_WIDGET (druid));
	gtk_widget_show (druid->back);
	gtk_widget_show (druid->next);
	gtk_widget_show (druid->cancel);
	gtk_widget_show (druid->finish);

	/* other flags */
	druid->_priv->current = NULL;
	druid->_priv->children = NULL;
	druid->_priv->show_finish = FALSE;
	gtk_signal_connect (GTK_OBJECT (druid->back),
			    "clicked",
			    GTK_SIGNAL_FUNC (gnome_druid_back_callback),
			    druid);
	gtk_signal_connect (GTK_OBJECT (druid->next),
			    "clicked",
			    GTK_SIGNAL_FUNC (gnome_druid_next_callback),
			    druid);
	gtk_signal_connect (GTK_OBJECT (druid->cancel),
			    "clicked",
			    GTK_SIGNAL_FUNC (gnome_druid_cancel_callback),
			    druid);
	gtk_signal_connect (GTK_OBJECT (druid->finish),
			    "clicked",
			    GTK_SIGNAL_FUNC (gnome_druid_next_callback),
			    druid);
}



static void
gnome_druid_destroy (GtkObject *object)
{
	GnomeDruid *druid;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_DRUID (object));

	druid = GNOME_DRUID (object);

	if(druid->back) {
		gtk_widget_destroy (druid->back);
		druid->back = NULL;
	}
	if(druid->next) {
		gtk_widget_destroy (druid->next);
		druid->next = NULL;
	}
	if(druid->cancel) {
		gtk_widget_destroy (druid->cancel);
		druid->cancel = NULL;
	}
	if(druid->finish) {
		gtk_widget_destroy (druid->finish);
		druid->finish = NULL;
	}

	/* Remove all children, we set current to NULL so
	 * that the remove code doesn't try to do anything funny */
	druid->_priv->current = NULL;
	while (druid->_priv->children != NULL) {
		GnomeDruidPage *child = druid->_priv->children->data;
		gtk_container_remove (GTK_CONTAINER (druid), GTK_WIDGET(child));
	}

	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_druid_finalize (GObject *object)
{
	GnomeDruid *druid;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_DRUID (object));

	druid = GNOME_DRUID (object);

	g_free(druid->_priv);
	druid->_priv = NULL;

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}

static void
gnome_druid_size_request (GtkWidget *widget,
			  GtkRequisition *requisition)
{
	guint16 temp_width, temp_height;
	GList *list;
	GnomeDruid *druid;
	GtkRequisition child_requisition;
	GnomeDruidPage *child;
	int border;
	
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_DRUID (widget));

	druid = GNOME_DRUID (widget);
	temp_height = temp_width = 0;

	border = GTK_CONTAINER(widget)->border_width;

	/* We find the maximum size of all children widgets */
	for (list = druid->_priv->children; list; list = list->next) {
		child = GNOME_DRUID_PAGE (list->data);
		if (GTK_WIDGET_VISIBLE (child)) {
			gtk_widget_size_request (GTK_WIDGET (child), &child_requisition);
			temp_width = MAX (temp_width, child_requisition.width);
			temp_height = MAX (temp_height, child_requisition.height);
			if (GTK_WIDGET_MAPPED (child) && child != druid->_priv->current)
				gtk_widget_unmap (GTK_WIDGET(child));
		}
	}
	
        requisition->width = temp_width + 2 * border;
        requisition->height = temp_height + 2 * border;

	/* In an Attempt to show how the widgets are packed,
	 * here's a little diagram.
	 * 
	 * ------------- [  Back  ] [  Next  ]    [ Cancel ]
	 *    \
	 *     This part needs to be at least 1 button width.
	 *     In addition, there is 1/4 X Button width between Cancel and Next,
	 *     and a GNOME_PAD_SMALL between Next and Back.
	 */
	/* our_button width is temp_width and temp_height */
	temp_height = 0;
	temp_width = 0;

	gtk_widget_size_request (druid->back, &child_requisition);
	temp_width = MAX (temp_width, child_requisition.width);
	temp_height = MAX (temp_height, child_requisition.height);

	gtk_widget_size_request (druid->next, &child_requisition);
	temp_width = MAX (temp_width, child_requisition.width);
	temp_height = MAX (temp_height, child_requisition.height);

	gtk_widget_size_request (druid->cancel, &child_requisition);
	temp_width = MAX (temp_width, child_requisition.width);
	temp_height = MAX (temp_height, child_requisition.height);

	gtk_widget_size_request (druid->finish, &child_requisition);
	temp_width = MAX (temp_width, child_requisition.width);
	temp_height = MAX (temp_height, child_requisition.height);

	temp_width += border * 2;
	temp_height += GNOME_PAD_SMALL;

	temp_width = temp_width * 17/4  + GNOME_PAD_SMALL * 3;

	/* pick which is bigger, the buttons, or the GnomeDruidPages */
	requisition->width = MAX (temp_width, requisition->width);
	requisition->height += temp_height + GNOME_PAD_SMALL * 2;
}

static void
gnome_druid_size_allocate (GtkWidget *widget,
			   GtkAllocation *allocation)
{
	GnomeDruid *druid;
	GtkAllocation child_allocation;
	gint button_height;
	GList *list;
	int border;
	
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_DRUID (widget));

	druid = GNOME_DRUID (widget);
	widget->allocation = *allocation;

	border = GTK_CONTAINER(widget)->border_width;

	/* deal with the buttons */
	child_allocation.width = child_allocation.height = 0;
	child_allocation.width = druid->back->requisition.width;
	child_allocation.height = druid->back->requisition.height;
	child_allocation.width = MAX (child_allocation.width,
			    druid->next->requisition.width);
	child_allocation.height = MAX (child_allocation.height,
			    druid->next->requisition.height);
	child_allocation.width = MAX (child_allocation.width,
			    druid->cancel->requisition.width);
	child_allocation.height = MAX (child_allocation.height,
			    druid->cancel->requisition.height);

	child_allocation.height += GNOME_PAD_SMALL;
	button_height = child_allocation.height;
	child_allocation.width += 2 * GNOME_PAD_SMALL;
	child_allocation.x = allocation->x + allocation->width - GNOME_PAD_SMALL - child_allocation.width;
	child_allocation.y = allocation->y + allocation->height - GNOME_PAD_SMALL - child_allocation.height;
	gtk_widget_size_allocate (druid->cancel, &child_allocation);
	child_allocation.x -= (child_allocation.width * 5 / 4);
	gtk_widget_size_allocate (druid->next, &child_allocation);
	gtk_widget_size_allocate (druid->finish, &child_allocation);
	child_allocation.x -= (GNOME_PAD_SMALL + child_allocation.width);
	gtk_widget_size_allocate (druid->back, &child_allocation);

	/* Put up the GnomeDruidPage */
	child_allocation.x = allocation->x + border;
	child_allocation.y = allocation->y + border;
	child_allocation.width =
		((allocation->width - 2 * border) > 0) ?
		(allocation->width - 2 * border):0;
	child_allocation.height =
		((allocation->height - 2 * border - GNOME_PAD_SMALL - button_height) > 0) ?
		(allocation->height - 2 * border - GNOME_PAD_SMALL - button_height):0;
	for (list = druid->_priv->children; list; list=list->next) {
		GtkWidget *widget = GTK_WIDGET (list->data);
		if (GTK_WIDGET_VISIBLE (widget)) {
			gtk_widget_size_allocate (widget, &child_allocation);
		}
	}
}

static GtkType
gnome_druid_child_type (GtkContainer *container)
{
	return gnome_druid_page_get_type ();
}

static void
gnome_druid_map (GtkWidget *widget)
{
	GnomeDruid *druid;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_DRUID (widget));

	druid = GNOME_DRUID (widget);
	GTK_WIDGET_SET_FLAGS (druid, GTK_MAPPED);

	gtk_widget_map (druid->back);
	if (druid->_priv->show_finish)
		gtk_widget_map (druid->finish);
	else
		gtk_widget_map (druid->next);
	gtk_widget_map (druid->cancel);
	if (druid->_priv->current &&
	    GTK_WIDGET_VISIBLE (druid->_priv->current) &&
	    !GTK_WIDGET_MAPPED (druid->_priv->current)) {
		gtk_widget_map (GTK_WIDGET (druid->_priv->current));
	}
}

static void
gnome_druid_unmap (GtkWidget *widget)
{
	GnomeDruid *druid;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_DRUID (widget));

	druid = GNOME_DRUID (widget);
	GTK_WIDGET_UNSET_FLAGS (druid, GTK_MAPPED);

	gtk_widget_unmap (druid->back);
	if (druid->_priv->show_finish)
		gtk_widget_unmap (druid->finish);
	else
		gtk_widget_unmap (druid->next);
	gtk_widget_unmap (druid->cancel);
	if (druid->_priv->current &&
	    GTK_WIDGET_VISIBLE (druid->_priv->current) &&
	    GTK_WIDGET_MAPPED (druid->_priv->current))
		gtk_widget_unmap (GTK_WIDGET (druid->_priv->current));
}
static void
gnome_druid_add (GtkContainer *widget,
		 GtkWidget *page)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_DRUID (widget));
	g_return_if_fail (page != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE (page));

	gnome_druid_append_page (GNOME_DRUID (widget), GNOME_DRUID_PAGE (page));
}
static void
gnome_druid_remove (GtkContainer *widget,
		    GtkWidget *child)
{
	GnomeDruid *druid;
	GList *list;
	
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_DRUID (widget));
	g_return_if_fail (child != NULL);

	druid = GNOME_DRUID (widget);

	list = g_list_find (druid->_priv->children, child);
	/* Is it a page? */ 
	if (list != NULL) {
		/* If we are mapped and visible, we want to deal with changing the page. */
		if ((GTK_WIDGET_MAPPED (GTK_WIDGET (widget))) &&
		    (list->data == (gpointer) druid->_priv->current) &&
		    (list->next != NULL)) {
			gnome_druid_set_page (druid, GNOME_DRUID_PAGE (list->next->data));
		}
	}
	druid->_priv->children = g_list_remove (druid->_priv->children, child);
	gtk_widget_unparent (child);
}

static void
gnome_druid_forall (GtkContainer *container,
		    gboolean      include_internals,
		    GtkCallback   callback,
		    gpointer      callback_data)
{
	GnomeDruid *druid;
	GnomeDruidPage *child;
	GList *children;

	g_return_if_fail (container != NULL);
	g_return_if_fail (GNOME_IS_DRUID (container));
	g_return_if_fail (callback != NULL);

	druid = GNOME_DRUID (container);

	children = druid->_priv->children;
	while (children) {
		child = children->data;
		children = children->next;

		(* callback) (GTK_WIDGET (child), callback_data);
	}
	if (include_internals) {
		(* callback) (druid->back, callback_data);
		(* callback) (druid->next, callback_data);
		(* callback) (druid->cancel, callback_data);
		(* callback) (druid->finish, callback_data);
	}
}

static gint
gnome_druid_expose (GtkWidget      *widget,
		    GdkEventExpose *event)
{
	GnomeDruid *druid;
	GtkWidget *child;
	GList *children;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_DRUID (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		druid = GNOME_DRUID (widget);
		children = druid->_priv->children;

		while (children) {
			child = GTK_WIDGET (children->data);
			children = children->next;

			gtk_container_propagate_expose (GTK_CONTAINER (widget),
							child, event);
		}
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
						druid->back, event);
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
						druid->next, event);
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
						druid->cancel, event);
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
						druid->finish, event);
	}
	return FALSE;
}

static void
gnome_druid_back_callback (GtkWidget *button, GnomeDruid *druid)
{
	GList *list;
	g_return_if_fail (druid->_priv->current != NULL);

	if (gnome_druid_page_back (druid->_priv->current))
		return;

	/* Make sure that we have a next list item */
	list = g_list_find (druid->_priv->children, druid->_priv->current);
	g_return_if_fail (list->prev != NULL);
	gnome_druid_set_page (druid, GNOME_DRUID_PAGE (list->prev->data));
}
static void
gnome_druid_next_callback (GtkWidget *button, GnomeDruid *druid)
{
	GList *list;
	g_return_if_fail (druid->_priv->current != NULL);

	if (druid->_priv->show_finish == FALSE) {
		if (gnome_druid_page_next (druid->_priv->current))
			return;

		/* Make sure that we have a next list item */
		list = g_list_find (druid->_priv->children,
				    druid->_priv->current);
		/* this would be a bug */
		g_assert (list != NULL);

		list = list->next;
		while (list != NULL &&
		        ! GTK_WIDGET_VISIBLE (list->data))
			list = list->next;

		if ( ! list)
			return;

		gnome_druid_set_page (druid, GNOME_DRUID_PAGE (list->data));
	} else {
		gnome_druid_page_finish (druid->_priv->current);
	}
}
static void
gnome_druid_cancel_callback (GtkWidget *button, GtkWidget *druid)
{
     if (GNOME_DRUID (druid)->_priv->current) {
	     if (gnome_druid_page_cancel (GNOME_DRUID (druid)->_priv->current))
		     return;

	     gtk_signal_emit (GTK_OBJECT (druid), druid_signals [CANCEL]);
     }
}

/* Public Functions */
/**
 * gnome_druid_new:
 *
 * Description: Creates a new #GnomeDruid widget.  You need to add this
 * to a dialog yourself, it is not a dialog.
 *
 * Returns:  A new #GnomeDruid widget
 **/
GtkWidget *
gnome_druid_new (void)
{
	return GTK_WIDGET (gtk_type_new (gnome_druid_get_type ()));
}

/**
 * gnome_druid_new_with_window:
 * @title: A title of the window
 * @parent: The parent of this window (transient_for)
 * @close_on_cancel: Close the window when cancel is pressed
 * @window: Optional return of the #GtkWindow created
 *
 * Description: Creates a new #GnomeDruid widget.  It also creates a new
 * toplevel window with the title of @title (which can be %NULL) and a parent
 * of @parent (which also can be %NULL).  The window and the druid will both be
 * shown.  If you need the window widget pointer you can optionally get it
 * through the last argument.  When the druid gets destroyed, so will the
 * window that is created here.
 *
 * Returns:  A new #GnomeDruid widget
 **/
GtkWidget *
gnome_druid_new_with_window (const char *title,
			     GtkWindow *parent,
			     gboolean close_on_cancel,
			     GtkWidget **window)
{
	GtkWidget *druid = gtk_type_new (gnome_druid_get_type ());

	/* make sure we always set window to NULL, even in 
	 * case of precondition errors */
	if (window != NULL)
		*window = NULL;

	g_return_val_if_fail (parent == NULL ||
			      GTK_IS_WINDOW (parent),
			      NULL);

	gnome_druid_construct_with_window (GNOME_DRUID (druid),
					   title,
					   parent,
					   close_on_cancel,
					   window);

	return druid;
}

/**
 * gnome_druid_construct_with_window:
 * @druid: The #GnomeDruid
 * @title: A title of the window
 * @parent: The parent of this window (transient_for)
 * @close_on_cancel: Close the window when cancel is pressed
 * @window: Optional return of the #GtkWindow created
 *
 * Description: Creates a new toplevel window with the title of @title (which
 * can be %NULL) and a parent of @parent (which also can be %NULL).  The @druid
 * will be placed inside this window.  The window and the druid will both be
 * shown.  If you need the window widget pointer you can optionally get it
 * through the last argument.  When the druid gets destroyed, so will the
 * window that is created here.
 * See #gnome_druid_new_with_window.
 **/
void
gnome_druid_construct_with_window (GnomeDruid *druid,
				   const char *title,
				   GtkWindow *parent,
				   gboolean close_on_cancel,
				   GtkWidget **window)
{
	GtkWidget *win;

	/* make sure we always set window to NULL, even in 
	 * case of precondition errors */
	if (window != NULL)
		*window = NULL;

	g_return_if_fail (druid != NULL);
	g_return_if_fail (GNOME_IS_DRUID (druid));
	g_return_if_fail (parent == NULL ||
			  GTK_IS_WINDOW (parent));

	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	if (title != NULL)
		gtk_window_set_title (GTK_WINDOW (win), title);
	if (parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (win),
					      parent);

	gtk_widget_show (GTK_WIDGET (druid));

	gtk_container_add (GTK_CONTAINER (win), GTK_WIDGET (druid));

	gtk_widget_show (win);

	if (close_on_cancel) {
		/* Use while_alive just for sanity */
		gtk_signal_connect_object_while_alive
			(GTK_OBJECT (druid), "cancel",
			 GTK_SIGNAL_FUNC (gtk_widget_destroy),
			 GTK_OBJECT (win));
	}

	/* When the druid gets destroyed so does the window */
	/* Use while_alive just for sanity */
	gtk_signal_connect_object_while_alive
		(GTK_OBJECT (druid), "destroy",
		 GTK_SIGNAL_FUNC (gtk_widget_destroy),
		 GTK_OBJECT (win));

	/* return the window */
	if (window != NULL)
		*window = win;
}


/**
 * gnome_druid_set_buttons_sensitive
 * @druid: A Druid.
 * @back_sensitive: The sensitivity of the back button.
 * @next_sensitive: The sensitivity of the next button.
 * @cancel_sensitive: The sensitivity of the cancel button.
 *
 * Description: Sets the sensitivity of the @druid's control-buttons.  If the
 * variables are TRUE, then they will be clickable.  This function is used
 * primarily by the actual GnomeDruidPage widgets.
 **/

void
gnome_druid_set_buttons_sensitive (GnomeDruid *druid,
				   gboolean back_sensitive,
				   gboolean next_sensitive,
				   gboolean cancel_sensitive)
{
	g_return_if_fail (druid != NULL);
	g_return_if_fail (GNOME_IS_DRUID (druid));

	gtk_widget_set_sensitive (druid->back, back_sensitive);
	gtk_widget_set_sensitive (druid->next, next_sensitive);
	gtk_widget_set_sensitive (druid->cancel, cancel_sensitive);
}

static void
undefault_button (GtkWidget *widget)
{
	GtkWidget *toplevel;

	toplevel = gtk_widget_get_toplevel (widget);

	if (GTK_IS_WINDOW (toplevel) &&
	    GTK_WINDOW (toplevel)->default_widget == widget) {
		gtk_window_set_default (GTK_WINDOW (toplevel), NULL);
	}
}

/**
 * gnome_druid_set_show_finish
 * @druid: A Druid widget.
 # @show_finish: If TRUE, then the "Cancel" button is changed to be "Finish"
 *
 * Description: Sets the text on the last button on the @druid.  If @show_finish
 * is TRUE, then the text becomes "Finish".  If @show_finish is FALSE, then the
 * text becomes "Cancel".
 **/
void
gnome_druid_set_show_finish (GnomeDruid *druid,
			     gboolean show_finish)
{
	g_return_if_fail (druid != NULL);
	g_return_if_fail (GNOME_IS_DRUID (druid));

	if (show_finish) {
		undefault_button (druid->next);

		if (GTK_WIDGET_MAPPED (druid->next)) {
			gtk_widget_unmap (druid->next);
			gtk_widget_map (druid->finish);
		}
	} else {
		undefault_button (druid->finish);

		if (GTK_WIDGET_MAPPED (druid->finish)) {
			gtk_widget_unmap (druid->finish);
			gtk_widget_map (druid->next);
		}
	}
	druid->_priv->show_finish = show_finish;
}
/**
 * gnome_druid_prepend_page:
 * @druid: A Druid widget.
 * @page: The page to be inserted.
 * 
 * Description: This will prepend a GnomeDruidPage into the internal list of
 * pages that the @druid has.  Since #GnomeDruid is just a container, you will
 * need to also call #gtk_widget_show on the page, otherwise the page will not
 * be shown.
 **/
void
gnome_druid_prepend_page (GnomeDruid *druid,
			  GnomeDruidPage *page)
{
	g_return_if_fail (druid != NULL);
	g_return_if_fail (GNOME_IS_DRUID (druid));
	g_return_if_fail (page != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE (page));

	gnome_druid_insert_page (druid, NULL, page);
}
/**
 * gnome_druid_insert_page:
 * @druid: A Druid widget.
 * @back_page: The page prior to the page to be inserted.
 * @page: The page to insert.
 * 
 * Description: This will insert @page after @back_page into the list of
 * internal pages that the @druid has.  If @back_page is not present in the list
 * or %NULL, @page will be prepended to the list.  Since #GnomeDruid is just a
 * container, you will need to also call #gtk_widget_show on the page,
 * otherwise the page will not be shown.
 **/
void
gnome_druid_insert_page (GnomeDruid *druid,
			 GnomeDruidPage *back_page,
			 GnomeDruidPage *page)
{
	GList *list;

	g_return_if_fail (druid != NULL);
	g_return_if_fail (GNOME_IS_DRUID (druid));
	g_return_if_fail (page != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE (page));

	list = g_list_find (druid->_priv->children, back_page);
	if (list == NULL) {
		druid->_priv->children = g_list_prepend (druid->_priv->children, page);
	} else {
		GList *new_el = g_list_alloc ();
		new_el->next = list->next;
		new_el->prev = list;
		if (new_el->next) 
			new_el->next->prev = new_el;
		new_el->prev->next = new_el;
		new_el->data = (gpointer) page;
	}
	gtk_widget_set_parent (GTK_WIDGET (page), GTK_WIDGET (druid));

	if (GTK_WIDGET_REALIZED (GTK_WIDGET (druid)))
		gtk_widget_realize (GTK_WIDGET (page));

	if (GTK_WIDGET_VISIBLE (GTK_WIDGET (druid)) && GTK_WIDGET_VISIBLE (GTK_WIDGET (page))) {
		if (GTK_WIDGET_MAPPED (GTK_WIDGET (page)))
			gtk_widget_unmap (GTK_WIDGET (page));
		gtk_widget_queue_resize (GTK_WIDGET (druid));
	}

	/* if it's the first and only page, we want to bring it to the foreground. */
	if (druid->_priv->children->next == NULL)
		gnome_druid_set_page (druid, page);
}

/**
 * gnome_druid_append_page: 
 * @druid: A Druid widget.
 * @page: The #GnomeDruidPage to be appended.
 * 
 * Description: This will append @page onto the end of the internal list.  
 * Since #GnomeDruid is just a container, you will need to also call
 * #gtk_widget_show on the page, otherwise the page will not be shown.
 **/
void
gnome_druid_append_page (GnomeDruid *druid,
			 GnomeDruidPage *page)
{
	GList *list;
	g_return_if_fail (druid != NULL);
	g_return_if_fail (GNOME_IS_DRUID (druid));
	g_return_if_fail (page != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE (page));

	list = g_list_last (druid->_priv->children);
	if (list) {
		gnome_druid_insert_page (druid, GNOME_DRUID_PAGE (list->data), page);
	} else {
		gnome_druid_insert_page (druid, NULL, page);
	}	
}
/**
 * gnome_druid_set_page:
 * @druid: A Druid widget.
 * @page: The #GnomeDruidPage to be brought to the foreground.
 * 
 * Description: This will make @page the currently showing page in the druid.
 * @page must already be in the druid.
 **/
void
gnome_druid_set_page (GnomeDruid *druid,
		      GnomeDruidPage *page)
{
	GList *list;
	GtkWidget *old = NULL;
	g_return_if_fail (druid != NULL);
	g_return_if_fail (GNOME_IS_DRUID (druid));
	g_return_if_fail (page != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE (page));

	if (druid->_priv->current == page)
	     return;
	list = g_list_find (druid->_priv->children, page);
	g_return_if_fail (list != NULL);

	if ((druid->_priv->current) && (GTK_WIDGET_VISIBLE (druid->_priv->current)) && (GTK_WIDGET_MAPPED (druid))) {
		old = GTK_WIDGET (druid->_priv->current);
	}
	druid->_priv->current = GNOME_DRUID_PAGE (list->data);
	gnome_druid_page_prepare (druid->_priv->current);
	if (GTK_WIDGET_VISIBLE (druid->_priv->current) && (GTK_WIDGET_MAPPED (druid))) {
		gtk_widget_map (GTK_WIDGET (druid->_priv->current));
	}
	if (old && GTK_WIDGET_MAPPED (old))
	  gtk_widget_unmap (old);
}
