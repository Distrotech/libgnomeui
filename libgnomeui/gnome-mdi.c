/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi.c - implementation of the GnomeMDI object

   Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
   All rights reserved.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
*/
/*
  @NOTATION@
*/
#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>


#include <libgnome/gnome-config.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-preferences.h>
#include <libgnome/gnome-marshal.h>
#include <libgnomeui/gnome-app.h>
#include <bonobo/bonobo-dock-layout.h>
#include <bonobo/bonobo-win.h>
#include <bonobo/bonobo-ui-container.h>
#include <bonobo/bonobo-ui-component.h>
#include <bonobo/bonobo-ui-util.h>
#include "gnome-pouch.h"
#include "gnome-roo.h"
#include "gnome-macros.h"
#include "gnome-mdi.h"
#include "gnome-mdi-child.h"
#include "gnome-mdiP.h"

#include <libgnomeuiP.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>

static void            gnome_mdi_class_init(GnomeMDIClass  *);
static void            gnome_mdi_init(GnomeMDI *);
static void            gnome_mdi_finalize(GObject *);
static gboolean        gnome_mdi_add_child_handler(GnomeMDI *mdi, GnomeMDIChild *child);
static gboolean        gnome_mdi_remove_child_handler(GnomeMDI *mdi, GnomeMDIChild *child);
static gboolean        gnome_mdi_add_view_handler(GnomeMDI *mdi, GtkWidget *widget);
static void            gnome_mdi_app_created_handler(GnomeMDI *, BonoboWindow *, BonoboUIComponent *);
static void            gnome_mdi_view_changed(GnomeMDI *, GtkWidget *);

static void            child_list_create(GnomeMDI *, BonoboUIEngine *, BonoboUIComponent *);

static GtkWidget       *app_create(GnomeMDI *);
static GtkWidget       *app_clone(GnomeMDI *, BonoboWindow *);
static gint            app_toplevel_delete_event(GnomeApp *, GdkEventAny *, GnomeMDI *);
static gint            app_book_delete_event(GnomeApp *, GdkEventAny *, GnomeMDI *);
static gint            app_wiw_delete_event (GnomeApp *app, GdkEventAny *event, GnomeMDI *mdi);
static void            app_focus_in_event(BonoboWindow *, GdkEventFocus *, GnomeMDI *);

static GtkWidget       *book_create(GnomeMDI *);
static void            book_switch_page(GtkNotebook *, GtkNotebookPage *, gint, GnomeMDI *);
static gint            book_motion(GtkWidget *widget, GdkEventMotion *e, gpointer data);
static gint            book_button_press(GtkWidget *widget, GdkEventButton *e, gpointer data);
static gint            book_button_release(GtkWidget *widget, GdkEventButton *e, gpointer data);
static void            book_add_view(GtkNotebook *, GtkWidget *);
static void            set_page_by_widget(GtkNotebook *, GtkWidget *);

static GtkWidget       *pouch_create (GnomeMDI *mdi);
static void            pouch_add_view (GnomePouch *pouch, GtkWidget *view);
static void            pouch_unselect_child(GnomePouch *pouch, GnomeRoo *roo, GnomeMDI *mdi);
static void            pouch_select_child(GnomePouch *pouch, GnomeRoo *roo, GnomeMDI *mdi);
static void            pouch_close_child(GnomePouch *pouch, GnomeRoo *roo, GnomeMDI *mdi);

static void            top_add_view(GnomeMDI *, GtkWidget *, GnomeMDIChild *, GtkWidget *);
static void            set_active_view(GnomeMDI *, GtkWidget *);

static void            remove_view(GnomeMDI *, GtkWidget *);
static void            remove_child(GnomeMDI *, GnomeMDIChild *);

/* convenience functions that call child's "virtual" functions */
static BonoboUINode    *child_get_node(GnomeMDIChild *);

/* a macro for getting the app's pouch (app->scrolledwindow->viewport->pouch) */
#define get_pouch_from_app(app) \
        GNOME_POUCH(GTK_BIN(GTK_BIN(GNOME_APP(app)->contents)->child)->child)

static GdkCursor *drag_cursor = NULL;

enum {
	ADD_CHILD,
	REMOVE_CHILD,
	ADD_VIEW,
	REMOVE_VIEW,
	CHILD_CHANGED,
	VIEW_CHANGED,
	APP_CREATED,
	LAST_SIGNAL
};

static gint mdi_signals[LAST_SIGNAL];

GNOME_CLASS_BOILERPLATE (GnomeMDI, gnome_mdi, GObject, gtk_object);

static void
gnome_mdi_class_init (GnomeMDIClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass*) klass;
	
	object_class->finalize = gnome_mdi_finalize;

	mdi_signals[ADD_CHILD] =
		g_signal_newc ("add_child",
					   G_TYPE_FROM_CLASS (object_class),
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET (GnomeMDIClass, add_child),
					   NULL, NULL,
					   gnome_marshal_BOOLEAN__OBJECT,
					   G_TYPE_BOOLEAN, 1, GNOME_TYPE_MDI_CHILD);
	mdi_signals[REMOVE_CHILD] =
		g_signal_newc ("remove_child",
					   G_TYPE_FROM_CLASS (object_class),
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET (GnomeMDIClass, remove_child),
					   NULL, NULL,
					   gnome_marshal_BOOLEAN__OBJECT,
					   G_TYPE_BOOLEAN, 1, GNOME_TYPE_MDI_CHILD);
	mdi_signals[ADD_VIEW] =
		g_signal_newc ("add_view",
					   G_TYPE_FROM_CLASS (object_class),
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET (GnomeMDIClass, add_view),
					   NULL, NULL,
					   gnome_marshal_BOOLEAN__OBJECT,
					   G_TYPE_BOOLEAN, 1, GTK_TYPE_WIDGET);
	mdi_signals[REMOVE_VIEW] =
		g_signal_newc ("remove_view",
					   G_TYPE_FROM_CLASS (object_class),
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET (GnomeMDIClass, remove_view),
					   NULL, NULL,
					   gnome_marshal_BOOLEAN__OBJECT,
					   G_TYPE_BOOLEAN, 1, GTK_TYPE_WIDGET);
	mdi_signals[CHILD_CHANGED] =
		g_signal_newc ("child_changed",
					   G_TYPE_FROM_CLASS (object_class),
					   G_SIGNAL_RUN_FIRST,
					   G_STRUCT_OFFSET (GnomeMDIClass, child_changed),
					   NULL, NULL,
					   g_cclosure_marshal_VOID__OBJECT,
					   G_TYPE_NONE, 1, GNOME_TYPE_MDI_CHILD);
	mdi_signals[VIEW_CHANGED] =
		g_signal_newc ("view_changed",
					   G_TYPE_FROM_CLASS (object_class),
					   G_SIGNAL_RUN_FIRST,
					   G_STRUCT_OFFSET (GnomeMDIClass, view_changed),
					   NULL, NULL,
					   g_cclosure_marshal_VOID__OBJECT,
					   G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
	mdi_signals[APP_CREATED] =
		g_signal_newc ("app_created",
					   G_TYPE_FROM_CLASS (object_class),
					   G_SIGNAL_RUN_FIRST,
					   G_STRUCT_OFFSET (GnomeMDIClass, app_created),
					   NULL, NULL,
					   gnome_marshal_VOID__OBJECT_OBJECT,
					   G_TYPE_NONE, 2,
					   BONOBO_TYPE_WINDOW, BONOBO_UI_COMPONENT_TYPE);
	
	
	klass->add_child = gnome_mdi_add_child_handler;
	klass->remove_child = gnome_mdi_remove_child_handler;
	klass->add_view = gnome_mdi_add_view_handler;
	klass->remove_view = NULL;
	klass->child_changed = NULL;
	klass->view_changed = gnome_mdi_view_changed;
	klass->app_created = gnome_mdi_app_created_handler;
}

static void
gnome_mdi_view_changed (GnomeMDI *mdi, GtkWidget *old_view)
{
#if 0
	GList *menu_list = NULL, *children;
	GtkWidget *parent = NULL, *view;
	GtkWidget *toolbar;
	GnomeApp *app = NULL, *old_app = NULL;
	GnomeMDIChild *child;
	GnomeDockItem *dock_item;
	GnomeUIInfo *ui_info;
	gpointer data;
	gint pos, items;

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: view changed handler called");
#endif

	view = mdi->priv->active_view;

	if(view)
		app = gnome_mdi_get_window_from_view(view);
	if(old_view) {
		data = gtk_object_get_data(GTK_OBJECT(old_view), GNOME_MDI_APP_KEY);
		if(data)
			old_app = GNOME_APP(data);
	}

	if(view && old_view && app != old_app) {
		if(GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(app),
											   GNOME_MDI_ITEM_COUNT_KEY)) != 0)
			return;
	}

	if(app == NULL)
		app = old_app;

	/* free previous child toolbar ui-info */
	ui_info = gtk_object_get_data(GTK_OBJECT(app),
								  GNOME_MDI_CHILD_TOOLBAR_INFO_KEY);
	if(ui_info != NULL) {
		free_ui_info_tree(ui_info);
		gtk_object_set_data(GTK_OBJECT(app),
							GNOME_MDI_CHILD_TOOLBAR_INFO_KEY, NULL);
	}
	ui_info = NULL;

	/* remove old child's toolbar */
	dock_item = gnome_app_get_dock_item_by_name(app,
												GNOME_MDI_CHILD_TOOLBAR_NAME);
	if(dock_item)
		gtk_widget_destroy(GTK_WIDGET(dock_item));

	/* free previous child menu ui-info */
	ui_info = gtk_object_get_data(GTK_OBJECT(app),
								  GNOME_MDI_CHILD_MENU_INFO_KEY);
	if(ui_info != NULL) {
		free_ui_info_tree(ui_info);
		gtk_object_set_data(GTK_OBJECT(app),
							GNOME_MDI_CHILD_MENU_INFO_KEY, NULL);
	}
	ui_info = NULL;
	
	if(mdi->priv->child_menu_path)
		parent = gnome_app_find_menu_pos(app->menubar,
										 mdi->priv->child_menu_path, &pos);

	/* remove old child-specific menus */
	items = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(app),
												GNOME_MDI_ITEM_COUNT_KEY));
	if(items > 0 && parent) {
		GtkWidget *widget;

		/* remove items; should be kept in sync with gnome_app_remove_menus! */
		children = g_list_nth(GTK_MENU_SHELL(parent)->children, pos);
		while(children && items > 0) {
			widget = GTK_WIDGET(children->data);
			children = children->next;

			/* if this item contains a gtkaccellabel, we have to set its
			   accel_widget to NULL so that the item gets unrefed. */
			if(GTK_IS_ACCEL_LABEL(GTK_BIN(widget)->child))
				gtk_accel_label_set_accel_widget(GTK_ACCEL_LABEL(GTK_BIN(widget)->child), NULL);

			gtk_container_remove(GTK_CONTAINER(parent), widget);
			items--;
		}
	}

	items = 0;
	if(view) {
		gtk_object_set_data(GTK_OBJECT(view), GNOME_MDI_APP_KEY, app);
		child = gnome_mdi_get_child_from_view(view);

		/* set the title */
		if( mdi->priv->mode == GNOME_MDI_MODAL ||
			mdi->priv->mode == GNOME_MDI_TOPLEVEL ) {
			/* in MODAL and TOPLEVEL modes the window title includes the active
			   child name: "child_name - mdi_title" */
			gchar *fullname;
			
			fullname = g_strconcat(child->priv->name, " - ", mdi->priv->title, NULL);
			gtk_window_set_title(GTK_WINDOW(app), fullname);
			g_free(fullname);
		}
		else
			gtk_window_set_title(GTK_WINDOW(app), mdi->priv->title);
		
		/* create new child-specific menus */
		if(parent) {
			if(child->priv->menu_template) {
				ui_info = copy_ui_info_tree(child->priv->menu_template);
				gnome_app_insert_menus_with_data(app,
												 mdi->priv->child_menu_path,
												 ui_info, child);
				gtk_object_set_data(GTK_OBJECT(app),
									GNOME_MDI_CHILD_MENU_INFO_KEY, ui_info);
				gtk_object_set_data(GTK_OBJECT(view),
									GNOME_MDI_CHILD_MENU_INFO_KEY, ui_info);
				items = count_ui_info_items(ui_info);
			}
			else {
				menu_list = child_create_menus(child, view);

				if(menu_list) {
					GList *menu;

					items = 0;
					menu = menu_list;					
					while(menu) {
						gtk_menu_shell_insert(GTK_MENU_SHELL(parent),
											  GTK_WIDGET(menu->data), pos);
						menu = menu->next;
						pos++;
						items++;
					}

					g_list_free(menu_list);
				}
				else
					items = 0;
			}
		}

		/* insert the new toolbar */
		if(child->priv->toolbar_template &&
		   (ui_info = copy_ui_info_tree(child->priv->toolbar_template)) != NULL) {
			toolbar = gtk_toolbar_new();
			gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
			gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
			gnome_app_fill_toolbar_with_data(GTK_TOOLBAR(toolbar), ui_info,
											 app->accel_group, child);
			gnome_mdi_child_add_toolbar(child, app, GTK_TOOLBAR(toolbar));
			gtk_object_set_data(GTK_OBJECT(app),
								GNOME_MDI_CHILD_TOOLBAR_INFO_KEY, ui_info);
			gtk_object_set_data(GTK_OBJECT(view),
								GNOME_MDI_CHILD_TOOLBAR_INFO_KEY, ui_info);
		}
	}
	else
		gtk_window_set_title(GTK_WINDOW(app), mdi->priv->title);

	gtk_object_set_data(GTK_OBJECT(app), GNOME_MDI_ITEM_COUNT_KEY,
						GINT_TO_POINTER(items));

	if(parent)
		gtk_widget_queue_resize(parent);
#endif
}

static void
gnome_mdi_app_created_handler (GnomeMDI *mdi, BonoboWindow *win, BonoboUIComponent *component)
{
    bonobo_ui_component_set_translate (component, "/", mdi->priv->menu_template, NULL);
    bonobo_ui_component_set_translate (component, "/toolbar", mdi->priv->toolbar_template, NULL);

	child_list_create (mdi, bonobo_window_get_ui_engine (win), component);
}

static gboolean
gnome_mdi_add_child_handler (GnomeMDI *mdi, GnomeMDIChild *child)
{
	return TRUE;
}

static gboolean
gnome_mdi_remove_child_handler (GnomeMDI *mdi, GnomeMDIChild *child)
{
	return TRUE;
}

static gboolean
gnome_mdi_add_view_handler (GnomeMDI *mdi, GtkWidget *widget)
{
	return TRUE;
}

static void
gnome_mdi_finalize (GObject *object)
{
    GnomeMDI *mdi;
	GList *child_node;
	GnomeMDIChild *child;

	g_return_if_fail(object != NULL);
	g_return_if_fail(GNOME_IS_MDI(object));

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: finalization!\n");
#endif

	mdi = GNOME_MDI(object);

	/* remove all remaining children */
	child_node = mdi->priv->children;
	while(child_node) {
		child = GNOME_MDI_CHILD(child_node->data);
		child_node = child_node->next;
		remove_child(mdi, child);
	}

	/* this call tries to behave in a manner similar to
	   destruction of toplevel windows: it unrefs itself,
	   thus taking care of the initial reference added
	   upon mdi creation. */
	if (mdi->priv->has_user_refcount != 0) {
		mdi->priv->has_user_refcount = 0;
		g_object_unref(object);
	}

	if(mdi->priv->child_menu_path)
		g_free(mdi->priv->child_menu_path);
	if(mdi->priv->child_list_path)
		g_free(mdi->priv->child_list_path);
	
	g_free(mdi->priv->appname);
	g_free(mdi->priv->title);

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}

static void
gnome_mdi_init (GnomeMDI *mdi)
{
	mdi->priv = g_new0(GnomeMDIPrivate, 1);

#ifdef FIXME
	mdi->priv->mode = gnome_preferences_get_mdi_mode();
	mdi->priv->tab_pos = gnome_preferences_get_mdi_tab_pos();
#endif
	
	mdi->priv->signal_id = 0;
	mdi->priv->in_drag = FALSE;

	mdi->priv->children = NULL;
	mdi->priv->windows = NULL;
	mdi->priv->registered = NULL;
	
	mdi->priv->active_child = NULL;
	mdi->priv->active_window = NULL;
	mdi->priv->active_view = NULL;
	
	mdi->priv->menu_template = NULL;
	mdi->priv->toolbar_template = NULL;
	mdi->priv->child_menu_path = NULL;
	mdi->priv->child_list_path = NULL;

	mdi->priv->has_user_refcount = 1;
}

static BonoboUINode *
child_get_node (GnomeMDIChild *child)
{
	if (GNOME_MDI_CHILD_GET_CLASS(child)->get_node != NULL)
		return GNOME_MDI_CHILD_GET_CLASS(child)->get_node(child);
	return NULL;
}

static GtkWidget *
child_set_label (GnomeMDIChild *child, GtkWidget *widget)
{
	return NULL;
}

static void
set_page_by_widget (GtkNotebook *book, GtkWidget *child)
{
	gint i;

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: set_page_by_widget");
#endif
	
	i = gtk_notebook_page_num(book, child);
	if(i != gtk_notebook_get_current_page(book))
		gtk_notebook_set_page(book, i);
}

static void
child_list_create (GnomeMDI *mdi, BonoboUIEngine *engine, BonoboUIComponent *component)
{
	BonoboUINode *submenu;
	GList *child_node;
	gchar *path;
	
	if(mdi->priv->child_list_path == NULL)
		return;
	
	submenu = bonobo_ui_node_new ("placeholder");

	child_node = mdi->priv->children;
	while(child_node) {
		BonoboUINode *node;

		node = child_get_node (child_node->data);
		if (node)
			bonobo_ui_node_add_child (submenu, node);
		
		child_node = child_node->next;
	}

	bonobo_ui_util_translate_ui (submenu);

	path = g_strdup_printf ("%s/placeholder", mdi->priv->child_list_path);
	bonobo_ui_engine_xml_rm (engine, path, "GnomeMDIWindow");
	bonobo_ui_engine_xml_merge_tree (engine, mdi->priv->child_list_path, submenu, "GnomeMDIWindow");
	g_free (path);
}

static void
child_list_update (GnomeMDI *mdi)
{
	BonoboWindow *window;
	GList *window_node;
	
	if(mdi->priv->child_list_path == NULL)
		return;
	
	window_node = mdi->priv->windows;
	while(window_node) {
		Bonobo_UIComponent uic;

		window = BONOBO_WINDOW(window_node->data);
		uic = bonobo_ui_engine_get_component(bonobo_window_get_ui_engine(window),
											 "GnomeMDIWindow");

		child_list_create (mdi, bonobo_window_get_ui_engine(window), NULL);

		window_node = window_node->next;
	}
}

void
gnome_mdi_child_list_remove (GnomeMDI *mdi, GnomeMDIChild *child)
{
	child_list_update (mdi);
}

void
gnome_mdi_child_list_add (GnomeMDI *mdi, GnomeMDIChild *child)
{
	child_list_update (mdi);
}

static gint
book_motion (GtkWidget *widget, GdkEventMotion *e, gpointer data)
{
	GnomeMDI *mdi;

	mdi = GNOME_MDI(data);

	if(!drag_cursor)
		drag_cursor = gdk_cursor_new(GDK_FLEUR);

	if(e->window == widget->window) {
		mdi->priv->in_drag = TRUE;
		gtk_grab_add(widget);
		gdk_pointer_grab(widget->window, FALSE,
						 GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
						 NULL, drag_cursor, GDK_CURRENT_TIME);
		if(mdi->priv->signal_id) {
			gtk_signal_disconnect(GTK_OBJECT(widget), mdi->priv->signal_id);
			mdi->priv->signal_id = 0;
		}
	}

	return FALSE;
}

static gint
book_button_press (GtkWidget *widget, GdkEventButton *e, gpointer data)
{
	GnomeMDI *mdi;

	mdi = GNOME_MDI(data);

	if(e->button == 1 && e->window == widget->window)
		mdi->priv->signal_id = gtk_signal_connect(GTK_OBJECT(widget),
												  "motion_notify_event",
												  GTK_SIGNAL_FUNC(book_motion),
												  mdi);

	return FALSE;
}

static gint
book_button_release (GtkWidget *widget, GdkEventButton *e, gpointer data)
{
	gint x = e->x_root, y = e->y_root;
	GnomeMDI *mdi;

	mdi = GNOME_MDI(data);
	
	if(mdi->priv->signal_id) {
		gtk_signal_disconnect(GTK_OBJECT(widget), mdi->priv->signal_id);
		mdi->priv->signal_id = 0;
	}

	if(e->button == 1 && e->window == widget->window && mdi->priv->in_drag) {
		GdkWindow *window;
		GList *window_node;
		BonoboWindow *app;
		GtkWidget *view, *new_book, *new_app;
		GtkNotebook *old_book = GTK_NOTEBOOK(widget);
		gint old_page_no;

		mdi->priv->in_drag = FALSE;
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		gtk_grab_remove(widget);

		window = gdk_window_at_pointer(&x, &y);
		if(window)
			window = gdk_window_get_toplevel(window);

		window_node = mdi->priv->windows;
		while(window_node) {
			if(window == GTK_WIDGET(window_node->data)->window) {
				gint cur_page_no;

				/* page was dragged to another notebook */
				old_book = GTK_NOTEBOOK(widget);
				new_book = GNOME_APP(window_node->data)->contents;

				if(old_book == (GtkNotebook *)new_book) /* page has been dropped on the source notebook */
					return FALSE;
	
				cur_page_no = gtk_notebook_get_current_page(old_book);
				if(cur_page_no >= 0) {
					gint new_page_no;

					view = gtk_notebook_get_nth_page(old_book, cur_page_no);
					gtk_container_remove(GTK_CONTAINER(old_book), view);

					/* a trick to make view_changed signal get emitted
					   properly, taking care of updating the gnomeapps */
					new_page_no = gtk_notebook_get_current_page(GTK_NOTEBOOK(new_book));
					if(new_page_no >= 0) {
						mdi->priv->active_view =
							gtk_notebook_get_nth_page(GTK_NOTEBOOK(new_book), new_page_no);
						mdi->priv->active_child =
							gnome_mdi_get_child_from_view(mdi->priv->active_view);
					}
					else {
						mdi->priv->active_view = NULL;
						mdi->priv->active_child = NULL;
					}

					book_add_view(GTK_NOTEBOOK(new_book), view);

					app = gnome_mdi_get_window_from_view(view);
					gdk_window_raise(GTK_WIDGET(app)->window);

					if(old_book->cur_page == NULL) {
						mdi->priv->active_window = app;
						app = BONOBO_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(old_book)));
						mdi->priv->windows =
							g_list_remove(mdi->priv->windows, app);
						gtk_widget_destroy(GTK_WIDGET(app));
					}
				}

				return FALSE;
			}
				
			window_node = window_node->next;
		}

		if(g_list_length(old_book->children) == 1)
			return FALSE;

		/* create a new toplevel */
		old_page_no = gtk_notebook_get_current_page(old_book);
		if(old_page_no >= 0) {
			gint width, height;
				
			view = gtk_notebook_get_nth_page(old_book, old_page_no);
	
			app = gnome_mdi_get_window_from_view(view);

			width = view->allocation.width;
			height = view->allocation.height;
			
			gtk_container_remove(GTK_CONTAINER(old_book), view);

			new_app = app_clone(mdi, app);
			new_book = book_create(mdi);
			gnome_app_set_contents(GNOME_APP(new_app), new_book);
			book_add_view(GTK_NOTEBOOK(new_book), view);

			gtk_window_set_position(GTK_WINDOW(new_app), GTK_WIN_POS_MOUSE);
			gtk_widget_set_usize (view, width, height);

			if(!GTK_WIDGET_VISIBLE(new_app))
				gtk_widget_show(new_app);
		}
	}

	return FALSE;
}

static GtkWidget *
book_create (GnomeMDI *mdi)
{
	GtkWidget *us;
	
	us = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(us), mdi->priv->tab_pos);
	gtk_widget_add_events(us, GDK_BUTTON1_MOTION_MASK);
	gtk_signal_connect(GTK_OBJECT(us), "switch_page",
					   GTK_SIGNAL_FUNC(book_switch_page), mdi);
	gtk_signal_connect(GTK_OBJECT(us), "button_press_event",
					   GTK_SIGNAL_FUNC(book_button_press), mdi);
	gtk_signal_connect(GTK_OBJECT(us), "button_release_event",
					   GTK_SIGNAL_FUNC(book_button_release), mdi);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(us), TRUE);
	gtk_widget_show(us);
	
	return us;
}

static void 
pouch_unselect_child(GnomePouch *pouch, GnomeRoo *roo, GnomeMDI *mdi)
{
	set_active_view(mdi, NULL);
}

static void
pouch_select_child(GnomePouch *pouch, GnomeRoo *roo, GnomeMDI *mdi)
{
	if(roo)
		set_active_view(mdi, GTK_BIN(roo)->child);
	else
		set_active_view(mdi, NULL);
}

static void
pouch_close_child(GnomePouch *pouch, GnomeRoo *roo, GnomeMDI *mdi)
{
	GnomeMDIChild *child;

	child = gnome_mdi_get_child_from_view(GTK_BIN(roo)->child);
	if(g_list_length(child->priv->views) == 1) {
		/* if this is the last view, we have to remove the child! */
		if(!gnome_mdi_remove_child(mdi, child))
			return;
	}
	else
		gnome_mdi_remove_view(mdi, GTK_BIN(roo)->child);
}

static GtkWidget *
pouch_create (GnomeMDI *mdi)
{
	GtkWidget *sw, *pouch;

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(sw);
	pouch = gnome_pouch_new();
	gnome_pouch_enable_popup_menu(GNOME_POUCH(pouch), TRUE);
	gtk_signal_connect(GTK_OBJECT(pouch), "select-child",
					   GTK_SIGNAL_FUNC(pouch_select_child), mdi);
	gtk_signal_connect(GTK_OBJECT(pouch), "unselect-child",
					   GTK_SIGNAL_FUNC(pouch_unselect_child), mdi);
	gtk_signal_connect(GTK_OBJECT(pouch), "close-child",
					   GTK_SIGNAL_FUNC(pouch_close_child), mdi);
	gtk_widget_show(pouch);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), pouch);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
								   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	return sw;
}

static void
book_add_view (GtkNotebook *book, GtkWidget *view)
{
	GnomeMDIChild *child;
	GtkWidget *title;

	child = gnome_mdi_get_child_from_view(view);

	title = child_set_label(child, NULL);

	gtk_notebook_append_page(book, view, title);

	if(g_list_length(book->children) > 1)
		set_page_by_widget(book, view);
}

static void
pouch_add_view (GnomePouch *pouch, GtkWidget *view)
{
	GnomeMDIChild *child;
	GtkWidget *roo;

	child = gnome_mdi_get_child_from_view(view);

	roo = gnome_roo_new();
	gnome_roo_set_title(GNOME_ROO(roo), child->priv->name);
	gtk_container_add(GTK_CONTAINER(roo), view);
	gtk_widget_show(roo);
	gtk_container_add(GTK_CONTAINER(pouch), roo);
	gnome_pouch_select_roo(pouch, GNOME_ROO(roo));
}

static void
book_switch_page (GtkNotebook *book, GtkNotebookPage *page, gint page_num, GnomeMDI *mdi)
{
#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: switching pages");
#endif

	if(page) {
		GtkWidget *view;

		view = gtk_notebook_get_nth_page(book, page_num);
		if(view != mdi->priv->active_view)
			set_active_view(mdi, view);
	}
	else
		set_active_view(mdi, NULL);  
}

static void
app_focus_in_event (BonoboWindow *win, GdkEventFocus *event, GnomeMDI *mdi)
{
	/* updates active_view and active_child when a new toplevel receives focus */
	g_return_if_fail(BONOBO_IS_WINDOW(win));
#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: toplevel receiving focus");
#endif

	if(mdi->priv->mode == GNOME_MDI_TOPLEVEL || 
	   mdi->priv->mode == GNOME_MDI_MODAL)
		set_active_view(mdi, bonobo_window_get_contents(win));
#if 0
	else if(mdi->priv->mode == GNOME_MDI_WIW) {
		GnomeRoo *roo = gnome_pouch_get_selected(get_pouch_from_app(app));
		if(roo)
			set_active_view(mdi, GTK_BIN(roo)->child);
		else
			set_active_view(mdi, NULL);
	}
	else if(mdi->priv->mode == GNOME_MDI_NOTEBOOK) {
		gint cur_page_no = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->contents));
		if(cur_page_no >= 0)
			set_active_view(mdi, gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->contents), cur_page_no));
		else
			set_active_view(mdi, NULL);
	}
#endif

	mdi->priv->active_window = win;
}

static GtkWidget *
app_clone (GnomeMDI *mdi, BonoboWindow *app)
{
#if 0
	GnomeDockLayout *layout;
	gchar *layout_string = NULL;
#endif
	GtkWidget *new_app;

#if 0
	if(app) {
		layout = gnome_dock_get_layout(GNOME_DOCK(app->dock));
		layout_string = gnome_dock_layout_create_string(layout);
		gtk_object_unref(GTK_OBJECT(layout));
	}
#endif

	new_app = app_create(mdi);

#if 0
	if(layout_string) {
		if(GNOME_APP(new_app)->layout)
			gnome_dock_layout_parse_string(GNOME_APP(new_app)->layout, layout_string);
		g_free(layout_string);
	}
#endif

	return new_app;
}

static gint
app_toplevel_delete_event (GnomeApp *app, GdkEventAny *event, GnomeMDI *mdi)
{
	GnomeMDIChild *child = NULL;
	
	if(g_list_length(mdi->priv->windows) == 1) {
		if(!gnome_mdi_remove_all(mdi))
			return TRUE;

		mdi->priv->windows = g_list_remove(mdi->priv->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));
		
		/* only destroy mdi if there are no external objects registered
		   with it. */
		if(mdi->priv->registered == NULL)
			gtk_object_destroy(GTK_OBJECT(mdi));
	}
	else if(app->contents) {
		child = gnome_mdi_get_child_from_view(app->contents);
		if(g_list_length(child->priv->views) == 1) {
			/* if this is the last view, we have to remove the child! */
			if(!gnome_mdi_remove_child(mdi, child))
				return TRUE;
		}
		else
			if(!gnome_mdi_remove_view(mdi, app->contents))
				return TRUE;
	}
	else {
		mdi->priv->windows = g_list_remove(mdi->priv->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));
	}

	return FALSE;
}

static gint
app_wiw_delete_event (GnomeApp *app, GdkEventAny *event, GnomeMDI *mdi)
{
	BonoboWindow *win = (BonoboWindow *) app;
	GnomeMDIChild *child;
	GtkWidget *view;
	GList *view_node, *node;
	gint ret = FALSE;
	
	if(g_list_length(mdi->priv->windows) == 1) {		
		if(!gnome_mdi_remove_all(mdi))
			return TRUE;

		mdi->priv->windows = g_list_remove(mdi->priv->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));
		
		/* only destroy mdi if there are no non-MDI windows registered
		   with it. */
		if(mdi->priv->registered == NULL)
			gtk_object_destroy(GTK_OBJECT(mdi));
	}
	else {
		/* first check if all the children in this notebook can be removed */
		view_node = GTK_FIXED(get_pouch_from_app(app))->children;
		if(!view_node) {
			mdi->priv->windows = g_list_remove(mdi->priv->windows, app);
			gtk_widget_destroy(GTK_WIDGET(app));
			return FALSE;
		}

		while(view_node) {
			view = GTK_BIN(((GtkFixedChild *)view_node->data)->widget)->child;
			child = gnome_mdi_get_child_from_view(view);
			
			view_node = view_node->next;
			
			node = child->priv->views;
			while(node) {
				if(gnome_mdi_get_window_from_view(node->data) != win)
					break;
				
				node = node->next;
			}
			
			if(node == NULL) {   /* all the views reside in this GnomeApp */
				g_signal_emit (G_OBJECT (mdi), mdi_signals [REMOVE_CHILD], 0,
							   G_OBJECT (child), &ret);
				if(!ret)
					return TRUE;
			}
		}

		ret = FALSE;
		/* now actually remove all children/views! */
		view_node = GTK_NOTEBOOK(app->contents)->children;
		while(view_node) {
			view = GTK_BIN(((GtkFixedChild *)view_node->data)->widget)->child;
			child = gnome_mdi_get_child_from_view(view);
			
			view_node = view_node->next;
			
			/* if this is the last view, remove the child */
			if(g_list_length(child->priv->views) == 1)
				remove_child(mdi, child);
			else
				if(!gnome_mdi_remove_view(mdi, view))
					ret = TRUE;
		}
	}

	return ret;
}

static gint
app_book_delete_event (GnomeApp *app, GdkEventAny *event, GnomeMDI *mdi)
{
	BonoboWindow *win = (BonoboWindow *) app;
	GnomeMDIChild *child;
	GtkWidget *view;
	gint page_no = 0;
	GList *node;
	gint ret = FALSE;
	
	if(g_list_length(mdi->priv->windows) == 1) {		
		if(!gnome_mdi_remove_all(mdi))
			return TRUE;

		mdi->priv->windows = g_list_remove(mdi->priv->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));
		
		/* only destroy mdi if there are no non-MDI windows registered
		   with it. */
		if(mdi->priv->registered == NULL)
			gtk_object_destroy(GTK_OBJECT(mdi));
	}
	else {
		/* first check if all the children in this notebook can be removed */
		view = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->contents), 0);
		if(!view) {
			mdi->priv->windows = g_list_remove(mdi->priv->windows, app);
			gtk_widget_destroy(GTK_WIDGET(app));
			return FALSE;
		}

		page_no = 0;
		while (view != NULL) {
			child = gnome_mdi_get_child_from_view(view);

			view = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->contents),
											 ++page_no);
			
			node = child->priv->views;
			while(node) {
				if(gnome_mdi_get_window_from_view(node->data) != win)
					break;
				
				node = node->next;
			}

			if(node == NULL) {   /* all the views reside in this GnomeApp */
				g_signal_emit (G_OBJECT (mdi), mdi_signals [REMOVE_CHILD], 0,
							   G_OBJECT (child), &ret);
				if(!ret)
					return TRUE;
			}
		} while (view != NULL);

		ret = FALSE;
		/* now actually remove all children/views! */
		page_no = 0;
		view = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->contents), page_no);
		while(view != NULL) {
			child = gnome_mdi_get_child_from_view(view);
			
			view = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->contents),
											 ++page_no);
			
			/* if this is the last view, remove the child */
			if(g_list_length(child->priv->views) == 1)
				remove_child(mdi, child);
			else
				if(!gnome_mdi_remove_view(mdi, view))
					ret = TRUE;
		}
	}
	
	return ret;
}

static void
app_destroy (BonoboWindow *win, GnomeMDI *mdi)
{
	bonobo_ui_engine_deregister_component(bonobo_window_get_ui_engine(win), "GnomeMDIWindow");

	child_list_update (mdi);

	if(mdi->priv->active_window == win) {
		if(mdi->priv->windows != NULL)
			mdi->priv->active_window = BONOBO_WINDOW(mdi->priv->windows->data);
		else
			mdi->priv->active_window = NULL;
	}
}

static GtkWidget *
app_create (GnomeMDI *mdi)
{
	GtkWidget *window;
	BonoboWindow *win;
	BonoboUIEngine *engine;
	BonoboUIContainer *container;
	BonoboUIComponent *component;
	GtkSignalFunc func = NULL;

	window = bonobo_window_new(mdi->priv->appname, mdi->priv->title);
	win = BONOBO_WINDOW(window);

	engine = bonobo_window_get_ui_engine(win);

    container = bonobo_ui_container_new();
	bonobo_ui_container_set_engine(container, engine);

	component = bonobo_ui_component_new("GnomeMDIWindow");
    bonobo_ui_component_set_container(component, BONOBO_OBJREF (container));

	gtk_window_set_wmclass(GTK_WINDOW(win), mdi->priv->appname, mdi->priv->appname);
  
	gtk_window_set_policy(GTK_WINDOW(win), TRUE, TRUE, FALSE);
  
	mdi->priv->windows = g_list_append(mdi->priv->windows, window);

	switch(mdi->priv->mode) {
	case GNOME_MDI_TOPLEVEL:
	case GNOME_MDI_MODAL:
		func = GTK_SIGNAL_FUNC(app_toplevel_delete_event);
		break;
	case GNOME_MDI_NOTEBOOK:
		func = GTK_SIGNAL_FUNC(app_book_delete_event);
		break;
	case GNOME_MDI_WIW:
		func = GTK_SIGNAL_FUNC(app_wiw_delete_event);
		break;
	default:
		break;
	}
	
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
					   func, mdi);
	gtk_signal_connect(GTK_OBJECT(window), "focus_in_event",
					   GTK_SIGNAL_FUNC(app_focus_in_event), mdi);

	gtk_signal_connect(GTK_OBJECT(window), "destroy",
					   GTK_SIGNAL_FUNC(app_destroy), mdi);

	g_signal_emit(G_OBJECT(mdi), mdi_signals[APP_CREATED], 0, window, component);

	return window;
}

static void
remove_view (GnomeMDI *mdi, GtkWidget *view)
{
	GtkWidget *parent;
	BonoboWindow *window;
	GnomeMDIChild *child;

	child = gnome_mdi_get_child_from_view(view);

	parent = view->parent;

	if(!parent)
		return;

	window = gnome_mdi_get_window_from_view(view);

	if(mdi->priv->mode == GNOME_MDI_TOPLEVEL ||
	   mdi->priv->mode == GNOME_MDI_MODAL) {
		gtk_container_remove(GTK_CONTAINER(parent), view);
		// window->contents = NULL;

		/* if this is NOT the last toplevel or a registered object exists,
		   destroy the toplevel */
		set_active_view(mdi, NULL);
		if(g_list_length(mdi->priv->windows) > 1 || mdi->priv->registered) {
			mdi->priv->windows = g_list_remove(mdi->priv->windows, window);
			gtk_widget_destroy(GTK_WIDGET(window));
		}
	}
	else if(mdi->priv->mode == GNOME_MDI_NOTEBOOK) {
		gtk_container_remove(GTK_CONTAINER(parent), view);
		if(GTK_NOTEBOOK(parent)->cur_page == NULL) {
			set_active_view(mdi, NULL);
			if(g_list_length(mdi->priv->windows) > 1 || mdi->priv->registered) {
				/* if this is NOT the last toplevel or a registered object
				   exists, destroy the toplevel */
				mdi->priv->windows = g_list_remove(mdi->priv->windows, window);
				gtk_widget_destroy(GTK_WIDGET(window));
			}
		}
		else
			set_active_view(mdi, gtk_notebook_get_nth_page(GTK_NOTEBOOK(parent), gtk_notebook_get_current_page(GTK_NOTEBOOK(parent))));
	}
	else if(mdi->priv->mode == GNOME_MDI_WIW) {
		/* remove the roo from the pouch */
		GtkFixed *fixed  = GTK_FIXED(parent->parent);
		gtk_container_remove(GTK_CONTAINER(fixed), parent);
		if(fixed->children == NULL) {
			set_active_view(mdi, NULL);
			if(g_list_length(mdi->priv->windows) > 1 || mdi->priv->registered) {
				/* if this is NOT the last toplevel or a registered object
				   exists, destroy the toplevel */
				mdi->priv->windows = g_list_remove(mdi->priv->windows, window);
				gtk_widget_destroy(GTK_WIDGET(window));
			}
		}
	}

	/* remove this view from the child's view list unless in MODAL mode */
	if(mdi->priv->mode != GNOME_MDI_MODAL)
		gnome_mdi_child_remove_view(child, view);
}

static void
remove_child (GnomeMDI *mdi, GnomeMDIChild *child)
{
	GList *view_node;
	GtkWidget *view;

	view_node = child->priv->views;
	while(view_node) {
		view = GTK_WIDGET(view_node->data);
		view_node = view_node->next;
		remove_view(mdi, GTK_WIDGET(view));
	}

	mdi->priv->children = g_list_remove(mdi->priv->children, child);

	gnome_mdi_child_list_remove(mdi, child);

	if(child == mdi->priv->active_child)
		mdi->priv->active_child = NULL;

	child->priv->parent = NULL;

	gtk_object_unref(GTK_OBJECT(child));

	if(mdi->priv->mode == GNOME_MDI_MODAL && mdi->priv->children) {
#if 0
		GnomeMDIChild *next_child = mdi->priv->children->data;

		if(next_child->priv->views) {
			gnome_app_set_contents(mdi->priv->active_window,
								   GTK_WIDGET(next_child->priv->views->data));
			set_active_view(mdi, GTK_WIDGET(next_child->priv->views->data));
		}
		else
			gnome_mdi_add_view(mdi, next_child);
#endif
	}
}

static void
top_add_view (GnomeMDI *mdi, GtkWidget *win, GnomeMDIChild *child, GtkWidget *view)
{
	if(child && view)
		bonobo_window_set_contents(BONOBO_WINDOW(win), view);

	set_active_view(mdi, view);

	if(!GTK_WIDGET_VISIBLE(win))
		gtk_widget_show(win);
}

static void
set_active_view (GnomeMDI *mdi, GtkWidget *view)
{
	GnomeMDIChild *old_child;
	GtkWidget *old_view;

#ifdef GNOME_ENABLE_DEBUG
	g_message("setting active_view to %08lx\n", (gulong)view);
#endif

	old_child = mdi->priv->active_child;
	old_view = mdi->priv->active_view;

	if(!view) {
		mdi->priv->active_child = NULL;
		mdi->priv->active_view = NULL;
	}

	if(view == old_view)
		return;

	if(view) {
		mdi->priv->active_child = gnome_mdi_get_child_from_view(view);
		mdi->priv->active_window = gnome_mdi_get_window_from_view(view);
	}

	mdi->priv->active_view = view;

	if(mdi->priv->active_child != old_child)
		g_signal_emit(G_OBJECT(mdi), mdi_signals[CHILD_CHANGED], 0,
					  old_child);

	g_signal_emit(G_OBJECT(mdi), mdi_signals[VIEW_CHANGED], 0, old_view);
}

/* the two functions below are supposed to be non-static but are not part
   of the public API */
GtkWidget *
gnome_mdi_new_toplevel (GnomeMDI *mdi)
{
	GtkWidget *book, *pouch, *app;

	if(mdi->priv->mode != GNOME_MDI_MODAL || mdi->priv->windows == NULL) {
		app = app_clone(mdi, mdi->priv->active_window);

		if(mdi->priv->mode == GNOME_MDI_NOTEBOOK) {
			book = book_create(mdi);
			bonobo_window_set_contents(BONOBO_WINDOW(app), book);
		}
		else if(mdi->priv->mode == GNOME_MDI_WIW) {
			pouch = pouch_create(mdi);
			bonobo_window_set_contents(BONOBO_WINDOW(app), pouch);
		}

		return app;
	}

	return GTK_WIDGET(mdi->priv->active_window);
}

void
gnome_mdi_update_child (GnomeMDI *mdi, GnomeMDIChild *child)
{
	GtkWidget *view, *title;
	GList *view_node;

	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));
	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_MDI_CHILD(child));

	view_node = child->priv->views;
	while(view_node) {
		view = GTK_WIDGET(view_node->data);

		/* for the time being all that update_child() does is update the
		   children's names */
		if(mdi->priv->mode == GNOME_MDI_MODAL ||
		   mdi->priv->mode == GNOME_MDI_TOPLEVEL) {
			/* in MODAL and TOPLEVEL modes the window title includes the active
			   child name: "child_name - mdi_title" */
			gchar *fullname;
      
			fullname = g_strconcat(child->priv->name, " - ", mdi->priv->title, NULL);
			gtk_window_set_title(GTK_WINDOW(gnome_mdi_get_window_from_view(view)),
								 fullname);
			g_free(fullname);
		}
		else if(mdi->priv->mode == GNOME_MDI_NOTEBOOK) {
			gint page_num;

			page_num = gtk_notebook_page_num(GTK_NOTEBOOK(view->parent), view);
			if(page_num >= 0) {
				GtkWidget *old_label;

				old_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(view->parent),
													   view);
				title = child_set_label(child, old_label);
			}
		}
		else if(mdi->priv->mode == GNOME_MDI_WIW) {
			gnome_roo_set_title(GNOME_ROO(view->parent), child->priv->name);
		}

		view_node = view_node->next;
	}
}

/**
 * gnome_mdi_construct:
 * @mdi: A #GnomeMDI object to construct.
 * @appname: Application name as used in filenames and paths.
 * @title: Title of the application windows.
 * 
 * Description:
 * Constructs a #GnomeMDI.
 **/
void
gnome_mdi_construct(GnomeMDI *mdi, const gchar *appname, const gchar *title)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	mdi->priv->appname = g_strdup(appname);
	mdi->priv->title = g_strdup(title);
}

/**
 * gnome_mdi_new:
 * @appname: Application name as used in filenames and paths.
 * @title: Title of the application windows.
 * 
 * Description:
 * Creates a new MDI object. @appname and @title are used for
 * MDI's calling gnome_app_new(). 
 * 
 * Return value:
 * A pointer to a new #GnomeMDI object.
 **/
GnomeMDI *
gnome_mdi_new (const gchar *appname, const gchar *title)
{
	GnomeMDI *mdi;
	
	mdi = gtk_type_new (gnome_mdi_get_type ());
  
	gnome_mdi_construct(mdi, appname, title);

	return mdi;
}

/**
 * gnome_mdi_set_active_view:
 * @mdi: A pointer to a #GnomeMDI object.
 * @view: A pointer to the view that is to become the active one.
 * 
 * Description:
 * Sets the active view to @view. It also raises the window containing it
 * and gives it focus.
 **/
void
gnome_mdi_set_active_view (GnomeMDI *mdi, GtkWidget *view)
{
	GtkWindow *window;
	
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));
	g_return_if_fail(view != NULL);
	g_return_if_fail(GTK_IS_WIDGET(view));

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: setting active view");
#endif

	if(mdi->priv->mode == GNOME_MDI_NOTEBOOK)
		set_page_by_widget(GTK_NOTEBOOK(view->parent), view);
	else if(mdi->priv->mode == GNOME_MDI_WIW)
		gnome_pouch_select_roo(GNOME_POUCH(view->parent->parent), GNOME_ROO(view->parent));
	else if(mdi->priv->mode == GNOME_MDI_MODAL) {
#ifdef FIXME
		if(mdi->priv->active_window->contents) {
			remove_view(mdi, mdi->priv->active_window->contents);
			mdi->priv->active_window->contents = NULL;
		}
		gnome_app_set_contents(mdi->priv->active_window, view);
#endif
		set_active_view(mdi, view);
	}

	window = GTK_WINDOW(gnome_mdi_get_window_from_view(view));
	
	/* TODO: hmmm... I dont know how to give focus to the window, so that it
	   would receive keyboard events */
	gdk_window_raise(GTK_WIDGET(mdi->priv->active_window)->window);
}

/**
 * gnome_mdi_add_view:
 * @mdi: A pointer to a #GnomeMDI object.
 * @child: A pointer to a child.
 * 
 * Description:
 * Creates a new view of the child and adds it to the MDI. #GnomeMDIChild
 * @child has to be added to the MDI with a call to gnome_mdi_add_child
 * before its views are added to the MDI. 
 * An "add_view" signal is emitted to the MDI after the view has been
 * created, but before it is shown and added to the MDI, with a pointer to
 * the created view as its parameter. The view is added to the MDI only if
 * the signal handler (if it exists) returns %TRUE. If the handler returns
 * %FALSE, the created view is destroyed and not added to the MDI. 
 * 
 * Return value:
 * %TRUE if adding the view succeeded and %FALSE otherwise.
 **/
gint
gnome_mdi_add_view (GnomeMDI *mdi, GnomeMDIChild *child)
{
	GtkWidget *view, *book, *pouch;
	GtkWidget *app;
	gint ret = TRUE;
	
	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI_CHILD(child), FALSE);
	
	if(mdi->priv->mode != GNOME_MDI_MODAL || child->priv->views == NULL)
		view = gnome_mdi_child_add_view(child);
	else {
		view = GTK_WIDGET(child->priv->views->data);

		if(child == mdi->priv->active_child)
			return TRUE;
	}

	g_signal_emit (G_OBJECT (mdi), mdi_signals [ADD_VIEW], 0,
				   G_OBJECT (view), &ret);

	if(ret == FALSE) {
		gnome_mdi_child_remove_view(child, view);
		return FALSE;
	}

	if(mdi->priv->windows == NULL || mdi->priv->active_window == NULL) {
		app = app_create(mdi);
		gtk_widget_show(app);
	}
	else
		app = GTK_WIDGET(mdi->priv->active_window);

	/* this reference will compensate the view's unrefing
	   when removed from its parent later, as we want it to
	   stay valid until removed from the child with a call
	   to gnome_mdi_child_remove_view() */
	gtk_widget_ref(view);

	if(!GTK_WIDGET_VISIBLE(view))
		gtk_widget_show(view);

	if(mdi->priv->mode == GNOME_MDI_NOTEBOOK) {
		if(bonobo_window_get_contents(BONOBO_WINDOW(app)) == NULL) {
			book = book_create(mdi);
			bonobo_window_set_contents(BONOBO_WINDOW(app), book);
		}
		book_add_view(GTK_NOTEBOOK(bonobo_window_get_contents(BONOBO_WINDOW(app))), view);
	}
	else if(mdi->priv->mode == GNOME_MDI_WIW) {
		if(bonobo_window_get_contents(BONOBO_WINDOW(app)) == NULL) {
			pouch = pouch_create(mdi);
			gnome_app_set_contents(GNOME_APP(app), pouch);
		}
		pouch_add_view(get_pouch_from_app(app), view);
	}
	else if(mdi->priv->mode == GNOME_MDI_TOPLEVEL) {
		/* add a new toplevel unless the remaining one is empty */
		if(app == NULL || bonobo_window_get_contents (BONOBO_WINDOW (app)) != NULL)
			app = app_clone(mdi, BONOBO_WINDOW(app));
		else
			app = GTK_WIDGET(mdi->priv->active_window);
		top_add_view(mdi, app, child, view);
	}
#ifdef FIXME
	else if(mdi->priv->mode == GNOME_MDI_MODAL) {
		/* replace the existing view if there is one */
		if(bonobo_window_get_contents(BONOBO_WINDOW(app))) {
			remove_view(mdi, bonobo_window_get_contents(BONOBO_WINDOW(app)));
			// bonobo_window_get_contents(BONOBO_WINDOW(app)) = NULL;
		}

		gnome_app_set_contents(GNOME_APP(app), view);
		set_active_view(mdi, view);
	}
#endif

	return TRUE;
}

/**
 * gnome_mdi_add_toplevel_view:
 * @mdi: A pointer to a #GnomeMDI object.
 * @child: A pointer to a #GnomeMDIChild object to be added to the MDI.
 * 
 * Description:
 * Creates a new view of the child and adds it to the MDI; it behaves the
 * same way as gnome_mdi_add_view in %GNOME_MDI_MODAL and %GNOME_MDI_TOPLEVEL
 * modes, but in %GNOME_MDI_NOTEBOOK or %GNOME_MDI_WIW mode, the view is added
 * in a new toplevel window unless the active one has no views in it. 
 * 
 * Return value: 
 * %TRUE if adding the view succeeded and %FALSE otherwise.
 **/
gint
gnome_mdi_add_toplevel_view (GnomeMDI *mdi, GnomeMDIChild *child)
{
	GtkWidget *view, *app, *pouch;
	gint ret = TRUE;
	
	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI_CHILD(child), FALSE);

	if(mdi->priv->mode != GNOME_MDI_MODAL || child->priv->views == NULL)
		view = gnome_mdi_child_add_view(child);
	else {
		view = GTK_WIDGET(child->priv->views->data);

		if(child == mdi->priv->active_child)
			return TRUE;
	}

	if(!view)
		return FALSE;

	g_signal_emit (G_OBJECT (mdi), mdi_signals [ADD_VIEW], 0,
				   G_OBJECT (view), &ret);

	if(ret == FALSE) {
		gnome_mdi_child_remove_view(child, view);
		return FALSE;
	}

	app = gnome_mdi_new_toplevel(mdi);

	/* this reference will compensate the view's unrefing
	   when removed from its parent later, as we want it to
	   stay valid until removed from the child with a call
	   to gnome_mdi_child_remove_view() */
	gtk_widget_ref(view);

	if(!GTK_WIDGET_VISIBLE(view))
		gtk_widget_show(view);

	if(mdi->priv->mode == GNOME_MDI_NOTEBOOK)
		book_add_view(GTK_NOTEBOOK(bonobo_window_get_contents(BONOBO_WINDOW(app))), view);
	else if(mdi->priv->mode == GNOME_MDI_WIW) {
		if(bonobo_window_get_contents(BONOBO_WINDOW(app)) == NULL) {
			pouch = pouch_create(mdi);
			gnome_app_set_contents(GNOME_APP(app), pouch);
		}
		pouch_add_view(get_pouch_from_app(app), view);
	}
	else if(mdi->priv->mode == GNOME_MDI_TOPLEVEL)
		/* add a new toplevel unless the remaining one is empty */
		top_add_view(mdi, app, child, view);
#ifdef FIXME
	else if(mdi->priv->mode == GNOME_MDI_MODAL) {
		/* replace the existing view if there is one */
		if(bonobo_window_get_contents(BONOBO_WINDOW(app))) {
			remove_view(mdi, mdi->priv->active_window->contents);
			// bonobo_window_get_contents(BONOBO_WINDOW(app)) = NULL;
		}

		gnome_app_set_contents(GNOME_APP(app), view);
		set_active_view(mdi, view);
	}
#endif

	if(!GTK_WIDGET_VISIBLE(app))
		gtk_widget_show(app);
	
	return TRUE;
}

/**
 * gnome_mdi_remove_view:
 * @mdi: A pointer to a #GnomeMDI object.
 * @view: View to remove.
 * 
 * Description:
 * Removes a view from an MDI. 
 * A "remove_view" signal is emitted to the MDI before actually removing
 * view. The view is removed only if the signal handler (if it exists)
 * returns %TRUE. 
 * 
 * Return value: 
 * %TRUE if the view was removed and %FALSE otherwise.
 **/
gint
gnome_mdi_remove_view (GnomeMDI *mdi, GtkWidget *view)
{
	gint ret = TRUE;
	
	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(view != NULL, FALSE);
	g_return_val_if_fail(GTK_IS_WIDGET(view), FALSE);

	g_signal_emit (G_OBJECT (mdi), mdi_signals [REMOVE_VIEW], 0,
				   G_OBJECT (view), &ret);

	if(ret == FALSE)
		return FALSE;

	remove_view(mdi, view);

	return TRUE;
}

/**
 * gnome_mdi_add_child:
 * @mdi: A pointer to a #GnomeMDI object.
 * @child: A pointer to a #GnomeMDIChild to add to the MDI.
 * 
 * Description:
 * Adds a new child to the MDI. No views are added: this has to be done with
 * a call to gnome_mdi_add_view. 
 * First an "add_child" signal is emitted to the MDI with a pointer to the
 * child as its parameter. The child is added to the MDI only if the signal
 * handler (if it exists) returns %TRUE. If the handler returns %FALSE, the
 * child is not added to the MDI. 
 * 
 * Return value: 
 * %TRUE if the child was added successfully and %FALSE otherwise.
 **/
gint
gnome_mdi_add_child (GnomeMDI *mdi, GnomeMDIChild *child)
{
	gint ret = TRUE;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI_CHILD(child), FALSE);

	g_signal_emit (G_OBJECT (mdi), mdi_signals [ADD_CHILD], 0,
				   G_OBJECT (child), &ret);

	if(ret == FALSE)
		return FALSE;

	child->priv->parent = GTK_OBJECT(mdi);

	mdi->priv->children = g_list_append(mdi->priv->children, child);

	gnome_mdi_child_list_add(mdi, child);

	return TRUE;
}

/**
 * gnome_mdi_remove_child:
 * @mdi: A pointer to a #GnomeMDI object.
 * @child: Child to remove.
 * 
 * Description:
 * Removes a child and all of its views from the MDI. 
 * A "remove_child" signal is emitted to the MDI with @child as its parameter
 * before actually removing the child. The child is removed only if the signal
 * handler (if it exists) returns %TRUE. 
 * 
 * Return value: 
 * %TRUE if the removal was successful and %FALSE otherwise.
 **/
gint
gnome_mdi_remove_child (GnomeMDI *mdi, GnomeMDIChild *child)
{
	gint ret = TRUE;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI_CHILD(child), FALSE);

	g_signal_emit (G_OBJECT (mdi), mdi_signals [REMOVE_CHILD], 0,
				   G_OBJECT (child), &ret);

	if(ret == FALSE)
		return FALSE;

	remove_child(mdi, child);

	return TRUE;
}

/**
 * gnome_mdi_remove_all:
 * @mdi: A pointer to a #GnomeMDI object.
 * 
 * Description:
 * Attempts to remove all children and all views from the MDI. 
 * A "remove_child" signal is emitted to the MDI for each child before
 * actually removing it. If signal handler for a child (if it exists)
 * returns %TRUE, the child will be removed.
 * 
 * Return value:
 * %TRUE if the removal was successful and %FALSE otherwise.
 **/
gint
gnome_mdi_remove_all (GnomeMDI *mdi)
{
	GList *child_node;
	GnomeMDIChild *child;
	gint handler_ret = TRUE, ret = TRUE;
	
	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);

	child_node = mdi->priv->children;
	while(child_node) {
		handler_ret = TRUE;
		g_signal_emit (G_OBJECT (mdi), mdi_signals [REMOVE_CHILD], 0,
					   G_OBJECT (child_node->data), &handler_ret);

		child = GNOME_MDI_CHILD(child_node->data);
		child_node = child_node->next;
		if(handler_ret)
			remove_child(mdi, child);
		else
			ret = FALSE;
	}

	return ret;
}

/**
 * gnome_mdi_open_toplevel:
 * @mdi: A pointer to a #GnomeMDI object.
 * 
 * Description:
 * Opens a new toplevel window (unless in %GNOME_MDI_MODAL mode and a
 * toplevel window is already open). This is usually used only for opening
 * the initial window on startup (just before calling gtk_main()) if no
 * windows were open because a session was restored or children were added
 * because of command line args).
 **/
void
gnome_mdi_open_toplevel (GnomeMDI *mdi)
{
	GtkWidget *toplevel;

	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	if((toplevel = gnome_mdi_new_toplevel(mdi)) != NULL)
		
	if(!GTK_WIDGET_VISIBLE(toplevel))
		gtk_widget_show(toplevel);
}

/**
 * gnome_mdi_find_child:
 * @mdi: A pointer to a #GnomeMDI object.
 * @name: A string with a name of the child to find.
 * 
 * Description:
 * Finds a child named @name.
 * 
 * Return value: 
 * A pointer to the #GnomeMDIChild object if the child was found and NULL
 * otherwise.
 **/
GnomeMDIChild *
gnome_mdi_find_child (GnomeMDI *mdi, const gchar *name)
{
	GList *child_node;

	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);

	child_node = mdi->priv->children;
	while(child_node) {
		if(!strcmp(GNOME_MDI_CHILD(child_node->data)->priv->name, name))
			return GNOME_MDI_CHILD(child_node->data);

		child_node = g_list_next(child_node);
	}

	return NULL;
}

/**
 * gnome_mdi_get_children:
 * @mdi: A pointer to a #GnomeMDI object.
 *
 * Description:
 * Returns a list of all children handled by the MDI @mdi.
 * 
 * Return value: 
 * A pointer to a GList of all children handled by the MDI @mdi
 **/
const GList *
gnome_mdi_get_children (GnomeMDI *mdi)
{
	return mdi->priv->children;
}

/**
 * gnome_mdi_get_windows:
 * @mdi: A pointer to a #GnomeMDI object.
 *
 * Description:
 * Returns a list of all toplevel windows opened by the MDI @mdi.
 * 
 * Return value: 
 * A pointer to a GList of all toplevel windows opened by the MDI @mdi
 **/
const GList *
gnome_mdi_get_windows (GnomeMDI *mdi)
{
	return mdi->priv->windows;
}

/**
 * gnome_mdi_set_mode:
 * @mdi: A pointer to a GnomeMDI object.
 * @mode: New mode.
 *
 * Description:
 * Sets the MDI mode to mode. Possible values are %GNOME_MDI_TOPLEVEL,
 * %GNOME_MDI_NOTEBOOK, %GNOME_MDI_MODAL and %GNOME_MDI_DEFAULT.
 **/
void
gnome_mdi_set_mode (GnomeMDI *mdi, GnomeMDIMode mode)
{
	GtkWidget *view, *book, *pouch;
	GnomeMDIChild *child;
	GList *child_node, *view_node, *app_node;
	gint windows = (mdi->priv->windows != NULL);
	guint16 width = 0, height = 0;

	/* FIXME: implement switching to WiW mode */

	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

#ifdef FIXME
	if(mode == GNOME_MDI_DEFAULT_MODE)
		mode = gnome_preferences_get_mdi_mode();
#endif

	if(mdi->priv->active_view) {
		width = mdi->priv->active_view->allocation.width;
		height = mdi->priv->active_view->allocation.height;
	}

	/* remove all views from their parents */
	child_node = mdi->priv->children;
	while(child_node) {
		child = GNOME_MDI_CHILD(child_node->data);
		view_node = child->priv->views;
		while(view_node) {
			view = GTK_WIDGET(view_node->data);

			if(view->parent)
				gtk_container_remove(GTK_CONTAINER(view->parent), view);

			view_node = view_node->next;
		}
		child_node = child_node->next;
	}

	/* remove all GnomeApps but the active one */
	app_node = mdi->priv->windows;
	while(app_node) {
		if(BONOBO_WINDOW(app_node->data) != mdi->priv->active_window)
			gtk_widget_destroy(GTK_WIDGET(app_node->data));
		app_node = app_node->next;
	}

	if(mdi->priv->windows)
		g_list_free(mdi->priv->windows);

	if(mdi->priv->active_window) {
#if 0
		if(mdi->priv->mode == GNOME_MDI_NOTEBOOK ||
		   mdi->priv->mode == GNOME_MDI_WIW)
			gtk_container_remove(GTK_CONTAINER(mdi->priv->active_window->dock),
								 GNOME_DOCK(mdi->priv->active_window->dock)->client_area);

		mdi->priv->active_window->contents = NULL;
#endif

		if( (mdi->priv->mode == GNOME_MDI_TOPLEVEL) || (mdi->priv->mode == GNOME_MDI_MODAL))
			gtk_signal_disconnect_by_func(GTK_OBJECT(mdi->priv->active_window),
										  GTK_SIGNAL_FUNC(app_toplevel_delete_event), mdi);
		else if(mdi->priv->mode == GNOME_MDI_NOTEBOOK)
			gtk_signal_disconnect_by_func(GTK_OBJECT(mdi->priv->active_window),
										  GTK_SIGNAL_FUNC(app_book_delete_event), mdi);
		else if(mdi->priv->mode == GNOME_MDI_WIW)
			gtk_signal_disconnect_by_func(GTK_OBJECT(mdi->priv->active_window),
										  GTK_SIGNAL_FUNC(app_wiw_delete_event), mdi);

		if( (mode == GNOME_MDI_TOPLEVEL) || (mode == GNOME_MDI_MODAL))
			gtk_signal_connect(GTK_OBJECT(mdi->priv->active_window), "delete_event",
							   GTK_SIGNAL_FUNC(app_toplevel_delete_event), mdi);
		else if(mode == GNOME_MDI_NOTEBOOK)
			gtk_signal_connect(GTK_OBJECT(mdi->priv->active_window), "delete_event",
							   GTK_SIGNAL_FUNC(app_book_delete_event), mdi);
		else if(mode == GNOME_MDI_WIW)
			gtk_signal_connect(GTK_OBJECT(mdi->priv->active_window), "delete_event",
							   GTK_SIGNAL_FUNC(app_wiw_delete_event), mdi);
		
		mdi->priv->windows = g_list_append(NULL, mdi->priv->active_window);

		if(mode == GNOME_MDI_NOTEBOOK) {
			book = book_create(mdi);
			bonobo_window_set_contents(mdi->priv->active_window, book);
		}
		else if(mode == GNOME_MDI_WIW) {
			pouch = pouch_create(mdi);
			bonobo_window_set_contents(mdi->priv->active_window, pouch);
		}
	}

	mdi->priv->mode = mode;

	/* re-implant views in proper containers */
	child_node = mdi->priv->children;
	while(child_node) {
		child = GNOME_MDI_CHILD(child_node->data);
		view_node = child->priv->views;
		while(view_node) {
			view = GTK_WIDGET(view_node->data);

			if(width != 0)
				gtk_widget_set_usize(view, width, height);

			if(mdi->priv->mode == GNOME_MDI_NOTEBOOK)
				book_add_view(GTK_NOTEBOOK(bonobo_window_get_contents(mdi->priv->active_window)), view);
			else if(mdi->priv->mode == GNOME_MDI_WIW)
				pouch_add_view(get_pouch_from_app(mdi->priv->active_window), view);
			else if(mdi->priv->mode == GNOME_MDI_TOPLEVEL) {
				/* add a new toplevel unless the remaining one is empty */
				if(bonobo_window_get_contents(mdi->priv->active_window) != NULL)
					mdi->priv->active_window = BONOBO_WINDOW(app_clone(mdi, mdi->priv->active_window));
				top_add_view(mdi, GTK_WIDGET(mdi->priv->active_window), child, view);
			}
			else if(mdi->priv->mode == GNOME_MDI_MODAL) {
				/* replace the existing view if there is one */
				if(bonobo_window_get_contents(mdi->priv->active_window) == NULL) {
					bonobo_window_set_contents(mdi->priv->active_window, view);
					set_active_view(mdi, view);
				}
			}

			view_node = view_node->next;

			gtk_widget_show(GTK_WIDGET(mdi->priv->active_window));
		}
		child_node = child_node->next;
	}

	if(windows && !mdi->priv->active_window)
		gnome_mdi_open_toplevel(mdi);
}

/**
 * gnome_mdi_get_active_child:
 * @mdi: A pointer to a #GnomeMDI object.
 * 
 * Description:
 * Returns a pointer to the active #GnomeMDIChild object.
 * 
 * Return value: 
 * A pointer to the active #GnomeMDIChild object. %NULL, if there is none.
 **/
GnomeMDIChild *
gnome_mdi_get_active_child (GnomeMDI *mdi)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);

	if(mdi->priv->active_view)
		return(gnome_mdi_get_child_from_view(mdi->priv->active_view));

	return NULL;
}

/**
 * gnome_mdi_get_active_view:
 * @mdi: A pointer to a #GnomeMDI object.
 * 
 * Description:
 * Returns a pointer to the active view (the one with the focus).
 * 
 * Return value: 
 * A pointer to a #GtkWidget.
 **/
GtkWidget *
gnome_mdi_get_active_view (GnomeMDI *mdi)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);

	return mdi->priv->active_view;
}

/**
 * gnome_mdi_get_active_window:
 * @mdi: A pointer to a #GnomeMDI object.
 * 
 * Description:
 * Returns a pointer to the toplevel window containing the active view.
 * 
 * Return value:
 * A pointer to a #GnomeApp that has the focus.
 **/
BonoboWindow *
gnome_mdi_get_active_window (GnomeMDI *mdi)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);

	return mdi->priv->active_window;
}

/**
 * gnome_mdi_set_menubar_template:
 * @mdi: A pointer to a #GnomeMDI object.
 * @menu_tmpl: A #GnomeUIInfo array describing the menu.
 * 
 * Description:
 * This function sets the template for menus that appear in each toplevel
 * window to menu_template. For each new toplevel window created by the MDI,
 * this structure is copied, the menus are created with
 * #gnome_app_create_menus_with_data() function with mdi as the callback
 * user data. Finally, the pointer to the copy is assigned to the new
 * toplevel window (a #GnomeApp widget) and can be obtained by calling
 * #gnome_mdi_get_menubar_info.
 **/
void
gnome_mdi_set_menubar_template (GnomeMDI *mdi, const gchar *menu_tmpl)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	mdi->priv->menu_template = g_strdup (menu_tmpl);
}

/**
 * gnome_mdi_set_toolbar_template:
 * @mdi: A pointer to a #GnomeMDI object.
 * @tbar_tmpl: A #GnomeUIInfo array describing the toolbar.
 * 
 * Description:
 * This function sets the template for toolbar that appears in each toplevel
 * window to toolbar_template. For each new toplevel window created by the MDI,
 * this structure is copied, the toolbar is created with
 * #gnome_app_create_toolbar_with_data( function with mdi as the callback
 * user data. Finally, the pointer to the copy is assigned to the new toplevel
 * window (a #GnomeApp widget) and can be retrieved with a call to
 * #gnome_mdi_get_toolbar_info. 
 **/
void
gnome_mdi_set_toolbar_template (GnomeMDI *mdi, const gchar *tbar_tmpl)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	mdi->priv->toolbar_template = g_strdup (tbar_tmpl);
}

/**
 * gnome_mdi_set_child_menu_path:
 * @mdi: A pointer to a #GnomeMDI object. 
 * @path: A menu path where the child menus should be inserted.
 * 
 * Description:
 * Sets the desired position of child-specific menus (which are added to and
 * removed from the main menus as views of different children are activated).
 * See #gnome_app_find_menu_pos for details on menu paths. 
 **/
void
gnome_mdi_set_child_menu_path (GnomeMDI *mdi, const gchar *path)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	if(mdi->priv->child_menu_path)
		g_free(mdi->priv->child_menu_path);

	mdi->priv->child_menu_path = g_strdup(path);
}

/**
 * gnome_mdi_set_child_list_path:
 * @mdi: A pointer to a #GnomeMDI object.
 * @path: A menu path where the child list menu should be inserted
 * 
 * Description:
 * Sets the position for insertion of menu items used to activate the MDI
 * children that were added to the MDI. See #gnome_app_find_menu_pos() for
 * details on menu paths. If the path is not set or set to %NULL, these menu
 * items aren't going to be inserted in the MDI menu structure. Note that if
 * you want all menu items to be inserted in their own submenu, you have to
 * create that submenu (and leave it empty, of course).
 **/
void
gnome_mdi_set_child_list_path (GnomeMDI *mdi, const gchar *path)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	if(mdi->priv->child_list_path)
		g_free(mdi->priv->child_list_path);

	mdi->priv->child_list_path = g_strdup(path);
}

/**
 * gnome_mdi_register:
 * @mdi: A pointer to a #GnomeMDI object.
 * @object: Object to register.
 * 
 * Description:
 * Registers #GObject @object with MDI. 
 * This is mostly intended for applications that open other windows besides
 * those opened by the MDI and want to continue to run even when no MDI
 * windows exist (an example of this would be GIMP's window with tools, if
 * the pictures were MDI children). As long as there is an object registered
 * with the MDI, the MDI will not destroy itself when the last of its windows
 * is closed. If no objects are registered, closing the last MDI window
 * results in MDI being destroyed. 
 **/
void
gnome_mdi_register (GnomeMDI *mdi, GObject *object)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));
	g_return_if_fail(object != NULL);
	g_return_if_fail(GTK_IS_OBJECT(object));

	if(!g_slist_find(mdi->priv->registered, object))
		mdi->priv->registered = g_slist_append(mdi->priv->registered, object);
}

/**
 * gnome_mdi_unregister:
 * @mdi: A pointer to a #GnomeMDI object.
 * @object: Object to unregister.
 * 
 * Description:
 * Removes #GObject @object from the list of registered objects. 
 **/
void
gnome_mdi_unregister (GnomeMDI *mdi, GObject *object)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));
	g_return_if_fail(object != NULL);
	g_return_if_fail(GTK_IS_OBJECT(object));

	mdi->priv->registered = g_slist_remove(mdi->priv->registered, object);
}

/**
 * gnome_mdi_get_child_from_view:
 * @view: A pointer to a #GtkWidget.
 * 
 * Description:
 * Returns a child that @view is a view of.
 * 
 * Return value:
 * A pointer to the #GnomeMDIChild the view belongs to.
 **/
GnomeMDIChild *
gnome_mdi_get_child_from_view (GtkWidget *view)
{
	gpointer child;

	g_return_val_if_fail(view != NULL, NULL);
	g_return_val_if_fail(GTK_IS_WIDGET(view), NULL);

	if((child = gtk_object_get_data(GTK_OBJECT(view), GNOME_MDI_CHILD_KEY)) != NULL)
		return GNOME_MDI_CHILD(child);

	return NULL;
}

/**
 * gnome_mdi_get_window_from_view:
 * @view: A pointer to a #GtkWidget.
 * 
 * Description:
 * Returns the toplevel window for this view.
 * 
 * Return value:
 * A pointer to the #GnomeApp containg the specified view.
 **/
BonoboWindow *
gnome_mdi_get_window_from_view (GtkWidget *view)
{
	GtkWidget *app;

	g_return_val_if_fail(view != NULL, NULL);
	g_return_val_if_fail(GTK_IS_WIDGET(view), NULL);

	app = gtk_widget_get_toplevel(view);
	if(app)
		return BONOBO_WINDOW(app);

	return NULL;
}

/**
 * gnome_mdi_get_view_from_window:
 * @mdi: A pointer to a #GnomeMDI object.
 * @app: A pointer to a #GnomeApp widget.
 * 
 * Description:
 * Returns the pointer to the view in the MDI toplevel window @app.
 * If the mode is set to %GNOME_MDI_NOTEBOOK or %GNOME_MDI_WIW, the
 * currently selected view is returned.
 * 
 * Return value: 
 * A pointer to a view.
 **/
GtkWidget *
gnome_mdi_get_view_from_window (GnomeMDI *mdi, GnomeApp *app)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);
	g_return_val_if_fail(app != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_APP(app), NULL);

	if((mdi->priv->mode == GNOME_MDI_TOPLEVEL) ||
	   (mdi->priv->mode == GNOME_MDI_MODAL))
		return app->contents;
	else if(mdi->priv->mode == GNOME_MDI_NOTEBOOK) {
		gint cur_page_no = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->contents));
		if (cur_page_no >= 0)
			return gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->contents), cur_page_no);
		else
			return NULL;
	}
	else if(mdi->priv->mode == GNOME_MDI_WIW)
		return GTK_WIDGET(gnome_pouch_get_selected(get_pouch_from_app(app)));
	else
		return NULL;
}

/**
 * gnome_mdi_get_menubar_info:
 * @app: A pointer to a #GnomeApp widget created by the MDI.
 * 
 * Return value: 
 * A #GnomeUIInfo array used for menubar in @app if the menubar has been created with a template.
 * %NULL otherwise.
 **/
const GnomeUIInfo *
gnome_mdi_get_menubar_info (GnomeApp *app)
{
	g_return_val_if_fail(app != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_APP(app), NULL);

	return gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_MENUBAR_INFO_KEY);
}

/**
 * gnome_mdi_get_toolbar_info:
 * @app: A pointer to a #GnomeApp widget created by the MDI.
 * 
 * Return value: 
 * A #GnomeUIInfo array used for toolbar in @app if the toolbar has been created with a template.
 * %NULL otherwise.
 **/
const GnomeUIInfo *
gnome_mdi_get_toolbar_info (GnomeApp *app)
{
	g_return_val_if_fail(app != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_APP(app), NULL);

	return gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_TOOLBAR_INFO_KEY);
}

/**
 * gnome_mdi_get_child_menu_info:
 * @app: A pointer to a #GnomeApp widget created by the MDI.
 * 
 * Return value: 
 * A #GnomeUIInfo array used for child's menus in @app if they have been created with a template.
 * %NULL otherwise.
 **/
const GnomeUIInfo *
gnome_mdi_get_child_menu_info (GnomeApp *app)
{
	g_return_val_if_fail(app != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_APP(app), NULL);

	return gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_CHILD_MENU_INFO_KEY);
}

/**
 * gnome_mdi_get_child_toolbar_info:
 * @app: A pointer to a #GnomeApp widget created by the MDI.
 * 
 * Return value: 
 * A #GnomeUIInfo array used for child's toolbar in @app if it has been created
 * with a template. %NULL otherwise.
 **/
const GnomeUIInfo *
gnome_mdi_get_child_toolbar_info(GnomeApp *app)
{
	g_return_val_if_fail(app != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_APP(app), NULL);

	return gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_CHILD_TOOLBAR_INFO_KEY);
}

/**
 * gnome_mdi_get_view_menu_info:
 * @view: A pointer to a widget that is a view of an MDI child.
 * 
 * Return value: 
 * A #GnomeUIInfo array used for @view's menus if they have been created with a
 * template. %NULL otherwise.
 **/
const GnomeUIInfo *
gnome_mdi_get_view_menu_info(GtkWidget *view)
{
	g_return_val_if_fail(view != NULL, NULL);
	g_return_val_if_fail(GTK_IS_WIDGET(view), NULL);

	return gtk_object_get_data(GTK_OBJECT(view), GNOME_MDI_CHILD_MENU_INFO_KEY);
}

/**
 * gnome_mdi_get_view_toolbar_info:
 * @view: A pointer to a widget that is a view of an MDI child.
 * 
 * Return value: 
 * A #GnomeUIInfo array used for @view's toolbar if it has been created with a
 * template. %NULL otherwise.
 **/
const GnomeUIInfo *
gnome_mdi_get_view_toolbar_info(GtkWidget *view)
{
	g_return_val_if_fail(view != NULL, NULL);
	g_return_val_if_fail(GTK_IS_WIDGET(view), NULL);

	return gtk_object_get_data(GTK_OBJECT(view), GNOME_MDI_CHILD_TOOLBAR_INFO_KEY);
}
