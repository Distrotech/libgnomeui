/* GNOME GUI Library
 * Copyright (C) 1997 the Free Software Foundation
 *
 * Special widget for the root window.
 *
 * Author: Federico Mena
 */

#include <gdk/gdkprivate.h>
#include "libgnome/gnome-defs.h"
#include "gnome-rootwin.h"


static void gnome_rootwin_class_init    (GnomeRootWinClass *class);
static void gnome_rootwin_invalid_op    (GtkWidget         *widget);
static void gnome_rootwin_show          (GtkWidget         *widget);
static void gnome_rootwin_hide          (GtkWidget         *widget);
static void gnome_rootwin_map           (GtkWidget         *widget);
static void gnome_rootwin_unmap         (GtkWidget         *widget);
static void gnome_rootwin_realize       (GtkWidget         *widget);
static void gnome_rootwin_size_request  (GtkWidget         *widget,
					 GtkRequisition    *requisition);
static void gnome_rootwin_size_allocate (GtkWidget         *widget,
					 GtkAllocation     *allocation);
static void gnome_rootwin_parent_set    (GtkWidget         *widget,
					 GtkWidget         *previous_parent);
static gint gnome_rootwin_expose        (GtkWidget      *widget,
				         GdkEventExpose *event);
static void gnome_rootwin_draw          (GtkWidget    *widget,
					 GdkRectangle *area);
static void gnome_rootwin_draw_focus    (GtkWidget *widget);
static void gnome_rootwin_draw_default  (GtkWidget *widget);



static GtkWindowClass *parent_class;


guint
gnome_rootwin_get_type (void)
{
	static guint rootwin_type = 0;

	if (!rootwin_type) {
		GtkTypeInfo rootwin_info = {
			"GnomeRootWin",
			sizeof (GnomeRootWin),
			sizeof (GnomeRootWinClass),
			(GtkClassInitFunc) gnome_rootwin_class_init,
			(GtkObjectInitFunc) NULL,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		rootwin_type = gtk_type_unique (gtk_window_get_type (), &rootwin_info);
	}

	return rootwin_type;
}

static void
gnome_rootwin_class_init (GnomeRootWinClass *class)
{
	GtkWidgetClass *widget_class;

	widget_class = (GtkWidgetClass *) class;

	parent_class = gtk_type_class (gtk_window_get_type ());

	widget_class->show = gnome_rootwin_show;
	widget_class->hide = gnome_rootwin_hide;
	widget_class->show_all = gnome_rootwin_invalid_op;
	widget_class->hide_all = gnome_rootwin_invalid_op;
	widget_class->map = gnome_rootwin_map;
	widget_class->unmap = gnome_rootwin_unmap;
	widget_class->realize = gnome_rootwin_realize;
	/* FIXME: we need a special unrealize method, but I am not
	 * sure how to implement it.  Gdk won't let us destroy a root
	 * window.
	 */
	widget_class->size_request = gnome_rootwin_size_request;
	widget_class->size_allocate = gnome_rootwin_size_allocate;
	widget_class->parent_set = gnome_rootwin_parent_set;

        widget_class->expose_event = gnome_rootwin_expose;
        widget_class->draw = gnome_rootwin_draw;
        widget_class->draw_focus = gnome_rootwin_draw_focus;
         widget_class->draw_default = gnome_rootwin_draw_default;
}

static gint
gnome_rootwin_expose (GtkWidget      *widget,
		      GdkEventExpose *event)
{
	return FALSE;
}

static void
gnome_rootwin_draw (GtkWidget    *widget,
		    GdkRectangle *area)
{
}

static void
gnome_rootwin_draw_focus (GtkWidget *widget)
{
}

static void
gnome_rootwin_draw_default (GtkWidget *widget)
{
}

GtkWidget *
gnome_rootwin_new (void)
{
	return GTK_WIDGET (gtk_type_new (gnome_rootwin_get_type ()));
}

static void
gnome_rootwin_invalid_op (GtkWidget *widget)
{
	g_error ("Invalid operation on root window!");
}

/* The following four functions are no-ops that just set the appropriate
 * flags on the widget.  We don't want to do normal-window stuff to the
 * root window, do we? :-)
 */

static void
gnome_rootwin_show (GtkWidget *widget)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ROOTWIN (widget));

	GTK_WIDGET_SET_FLAGS (widget, GTK_VISIBLE);

	gtk_widget_map (widget);
}

static void
gnome_rootwin_hide (GtkWidget *widget)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ROOTWIN (widget));

	GTK_WIDGET_UNSET_FLAGS (widget, GTK_VISIBLE);

	gtk_widget_unmap (widget);
}

static void
gnome_rootwin_map (GtkWidget *widget)
{
	GtkWindow *window;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ROOTWIN (widget));

	GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

	window = GTK_WINDOW (widget);

	if (window->bin.child
	    && GTK_WIDGET_VISIBLE (window->bin.child)
	    && !GTK_WIDGET_MAPPED (window->bin.child))
		gtk_widget_map (window->bin.child);

 	gtk_widget_queue_draw (widget); /* special case; need to force "expose" */
}

static void
gnome_rootwin_unmap (GtkWidget *widget)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ROOTWIN (widget));

	GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

	gdk_window_clear (widget->window); /* special case, need to force "expose unders" */
}

static void
gnome_rootwin_realize (GtkWidget *widget)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GNOME_IS_ROOTWIN (widget));

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	widget->window = gdk_window_foreign_new (gdk_root_window);
	gdk_window_set_user_data (widget->window, widget);

	/* Here we set the widget's events plus the window's events plus the normal
	 * GtkWindow events.
	 */

	gdk_window_set_events (widget->window,
			       (gtk_widget_get_events (widget)
				| gdk_window_get_events (widget->window)
				| GDK_STRUCTURE_MASK));

	widget->style = gtk_style_attach (widget->style, widget->window);

	/* I think setting the root window's background would be evil in this case. */
/* 	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL); */
}

static void
gnome_rootwin_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	g_warning ("Eek, gnome_rootwin_size_request() called!");
}

static void
gnome_rootwin_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	g_warning ("Eek, gnome_rootwin_size_allocate() called!");
}

static void
gnome_rootwin_parent_set (GtkWidget *widget, GtkWidget *previous_parent)
{
	g_error ("Eek, gnome_rootwin_parent_set() called!");
}
