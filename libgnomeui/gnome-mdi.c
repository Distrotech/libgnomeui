/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi.c - implementation of the GnomeMDI object

   Copyright (C) 1997 - 2001 Free Software Foundation

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

   Author: Jaka Mocnik <jaka@gnu.org>
*/

#include <config.h>

#ifndef GNOME_DISABLE_DEPRECATED_SOURCE

#include <libgnome/gnome-macros.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gnome-i18nP.h"

#include <libgnome/gnome-config.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-marshal.h>
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include <bonobo/bonobo-dock-layout.h>
#include "gnome-mdi.h"
#include "gnome-mdi-child.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>

static void            gnome_mdi_class_init     (GnomeMDIClass  *);
static void            gnome_mdi_instance_init  (GnomeMDI *);
static void            gnome_mdi_destroy        (GtkObject *);
static void            gnome_mdi_finalize       (GObject *);

static void            child_list_menu_create     (GnomeMDI *, GnomeApp *);
static void            child_list_activated_cb    (GtkWidget *, GnomeMDI *);
void                   _gnome_mdi_child_list_menu_remove_item(GnomeMDI *, GnomeMDIChild *);
void                   _gnome_mdi_child_list_menu_add_item   (GnomeMDI *, GnomeMDIChild *);
static GtkWidget      *find_item_by_child        (GtkMenuShell *, GnomeMDIChild *);

static void            app_create               (GnomeMDI *, gchar *);
static void            app_clone                (GnomeMDI *, GnomeApp *);
static void            app_destroy              (GnomeApp *, GnomeMDI *);
static void            app_set_view             (GnomeMDI *, GnomeApp *, GtkWidget *);

static gint            app_close_top            (GnomeApp *, GdkEventAny *, GnomeMDI *);
static gint            app_close_book           (GnomeApp *, GdkEventAny *, GnomeMDI *);

static GtkWidget       *book_create             (GnomeMDI *);
static void            book_switch_page         (GtkNotebook *, GtkNotebookPage *,
						 gint, GnomeMDI *);
static gint            book_motion              (GtkWidget *widget, GdkEventMotion *e,
						 gpointer data);
static gint            book_button_press        (GtkWidget *widget, GdkEventButton *e,
						 gpointer data);
static gint            book_button_release      (GtkWidget *widget, GdkEventButton *e,
						 gpointer data);
static void            book_add_view            (GtkNotebook *, GtkWidget *);
static void            set_page_by_widget       (GtkNotebook *, GtkWidget *);

static void            toplevel_focus           (GnomeApp *, GdkEventFocus *, GnomeMDI *);

static void            top_add_view             (GnomeMDI *, GnomeMDIChild *, GtkWidget *);

static void            set_active_view          (GnomeMDI *, GtkWidget *);

static GnomeUIInfo     *copy_ui_info_tree       (const GnomeUIInfo *);
static void            free_ui_info_tree        (GnomeUIInfo *);
static gint            count_ui_info_items      (const GnomeUIInfo *);

/* convenience functions that call child's "virtual" functions */
static GList           *child_create_menus      (GnomeMDIChild *, GtkWidget *);
static GtkWidget       *child_set_label         (GnomeMDIChild *, GtkWidget *);

/* keys for stuff that we'll assign to our GnomeApps */
#define GNOME_MDI_TOOLBAR_INFO_KEY           "MDIToolbarUIInfo"
#define GNOME_MDI_MENUBAR_INFO_KEY           "MDIMenubarUIInfo"
#define GNOME_MDI_CHILD_MENU_INFO_KEY        "MDIChildMenuUIInfo"
#define GNOME_MDI_CHILD_KEY                  "GnomeMDIChild"
#define GNOME_MDI_ITEM_COUNT_KEY             "MDIChildMenuItems"

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

typedef gboolean   (*GnomeMDISignal1) (GtkObject *, gpointer, gpointer);
typedef void       (*GnomeMDISignal2) (GtkObject *, gpointer, gpointer);

static gint mdi_signals[LAST_SIGNAL];

GNOME_CLASS_BOILERPLATE (GnomeMDI, gnome_mdi,
			 GtkObject, gtk_object, GTK_TYPE_OBJECT)

static void gnome_mdi_class_init (GnomeMDIClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	
	object_class = (GtkObjectClass*) class;
	gobject_class = (GObjectClass*) class;
	
	object_class->destroy = gnome_mdi_destroy;
	gobject_class->finalize = gnome_mdi_finalize;

	mdi_signals[ADD_CHILD] = gtk_signal_new ("add_child",
						 GTK_RUN_LAST,
						 GTK_CLASS_TYPE (object_class),
						 GTK_SIGNAL_OFFSET (GnomeMDIClass, add_child),
						 gnome_marshal_BOOLEAN__OBJECT,
						 GTK_TYPE_BOOL, 1, GNOME_TYPE_MDI_CHILD);
	mdi_signals[REMOVE_CHILD] = gtk_signal_new ("remove_child",
						    GTK_RUN_LAST,
						    GTK_CLASS_TYPE (object_class),
						    GTK_SIGNAL_OFFSET (GnomeMDIClass, remove_child),
						    gnome_marshal_BOOLEAN__OBJECT,
						    GTK_TYPE_BOOL, 1, GNOME_TYPE_MDI_CHILD);
	mdi_signals[ADD_VIEW] = gtk_signal_new ("add_view",
						GTK_RUN_LAST,
						GTK_CLASS_TYPE (object_class),
						GTK_SIGNAL_OFFSET (GnomeMDIClass, add_view),
						gnome_marshal_BOOLEAN__OBJECT,
						GTK_TYPE_BOOL, 1, GTK_TYPE_WIDGET);
	mdi_signals[REMOVE_VIEW] = gtk_signal_new ("remove_view",
						   GTK_RUN_LAST,
						   GTK_CLASS_TYPE (object_class),
						   GTK_SIGNAL_OFFSET (GnomeMDIClass, remove_view),
						   gnome_marshal_BOOLEAN__OBJECT,
						   GTK_TYPE_BOOL, 1, GTK_TYPE_WIDGET);
	mdi_signals[CHILD_CHANGED] = gtk_signal_new ("child_changed",
						     GTK_RUN_LAST,
						     GTK_CLASS_TYPE (object_class),
						     GTK_SIGNAL_OFFSET (GnomeMDIClass, child_changed),
						     gtk_marshal_VOID__OBJECT,
						     GTK_TYPE_NONE, 1, GNOME_TYPE_MDI_CHILD);
	mdi_signals[VIEW_CHANGED] = gtk_signal_new ("view_changed",
						    GTK_RUN_LAST,
						    GTK_CLASS_TYPE (object_class),
						    GTK_SIGNAL_OFFSET(GnomeMDIClass, view_changed),
						    gtk_marshal_VOID__OBJECT,
						    GTK_TYPE_NONE, 1, GTK_TYPE_WIDGET);
	mdi_signals[APP_CREATED] = gtk_signal_new ("app_created",
						   GTK_RUN_LAST,
						   GTK_CLASS_TYPE (object_class),
						   GTK_SIGNAL_OFFSET (GnomeMDIClass, app_created),
						   gtk_marshal_VOID__OBJECT,
						   GTK_TYPE_NONE, 1, GNOME_TYPE_APP);
	
	class->add_child = NULL;
	class->remove_child = NULL;
	class->add_view = NULL;
	class->remove_view = NULL;
	class->child_changed = NULL;
	class->view_changed = NULL;
	class->app_created = NULL;

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);
}

static void gnome_mdi_finalize (GObject *object)
{
    GnomeMDI *mdi;

	g_return_if_fail(object != NULL);
	g_return_if_fail(GNOME_IS_MDI(object));

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: finalization!\n");
#endif

	mdi = GNOME_MDI(object);

	g_free (mdi->child_menu_path);
	mdi->child_menu_path = NULL;
	g_free (mdi->child_list_path);
	mdi->child_list_path = NULL;
	
	g_free (mdi->appname);
	mdi->appname = NULL;
	g_free (mdi->title);
	mdi->title = NULL;

	if(G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize)(object);
}

static void gnome_mdi_destroy (GtkObject *object)
{
	GnomeMDI *mdi;

	g_return_if_fail(object != NULL);
	g_return_if_fail(GNOME_IS_MDI (object));
	
	mdi = GNOME_MDI(object);

	gnome_mdi_remove_all(mdi, TRUE);

	/* Note: This is BROOOOOKEN.  We need to figure out
	 * some better way to do this.  This may not even
	 * work right now, but that's what the 1.0 version did
	 *  -George*/

	/* this call tries to behave in a manner similar to
	   destruction of toplevel windows: it unrefs itself,
	   thus taking care of the initial reference added
	   upon mdi creation. */
	if(G_OBJECT (object)->ref_count > 0 && !GTK_OBJECT_DESTROYED(object))
		gtk_object_unref(object);

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

static void gnome_mdi_instance_init (GnomeMDI *mdi)
{
	/* FIXME!
	mdi->mode = gnome_preferences_get_mdi_mode();
	mdi->tab_pos = gnome_preferences_get_mdi_tab_pos();
	 */
	mdi->mode = GNOME_MDI_NOTEBOOK;

	mdi->signal_id = 0;
	mdi->in_drag = FALSE;

	mdi->children = NULL;
	mdi->windows = NULL;
	mdi->registered = NULL;
	
	mdi->active_child = NULL;
	mdi->active_window = NULL;
	mdi->active_view = NULL;
	
	mdi->menu_template = NULL;
	mdi->toolbar_template = NULL;
	mdi->child_menu_path = NULL;
	mdi->child_list_path = NULL;
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
 * A pointer to a new GnomeMDI object.
 **/
GtkObject *gnome_mdi_new(const gchar *appname, const gchar *title) {
	GnomeMDI *mdi;
	
	mdi = gtk_type_new (GNOME_TYPE_MDI);
  
	mdi->appname = g_strdup(appname);
	mdi->title = g_strdup(title);

	return GTK_OBJECT (mdi);
}

static GList *child_create_menus (GnomeMDIChild *child, GtkWidget *view)
{
	if(GNOME_MDI_CHILD_GET_CLASS(child)->create_menus)
		return GNOME_MDI_CHILD_GET_CLASS(child)->create_menus(child, view, NULL);

	return NULL;
}

static GtkWidget *child_set_label (GnomeMDIChild *child, GtkWidget *label)
{
	return GNOME_MDI_CHILD_GET_CLASS(child)->set_label(child, label, NULL);
}

/* the app-helper support routines
 * copying and freeing of GnomeUIInfo trees and counting items in them.
 */
static GnomeUIInfo *copy_ui_info_tree (const GnomeUIInfo source[])
{
	GnomeUIInfo *copy;
	int i, count;
	
	for(count = 0; source[count].type != GNOME_APP_UI_ENDOFINFO; count++)
		;
	
	count++;
	
	copy = g_malloc(count*sizeof(GnomeUIInfo));
	
	if(copy == NULL) {
		g_warning("GnomeMDI: Could not allocate new GnomeUIInfo");
		return NULL;
	}
	
	memcpy(copy, source, count*sizeof(GnomeUIInfo));
	
	for(i = 0; i < count; i++) {
		if( (source[i].type == GNOME_APP_UI_SUBTREE) ||
			(source[i].type == GNOME_APP_UI_SUBTREE_STOCK) ||
			(source[i].type == GNOME_APP_UI_RADIOITEMS) )
			copy[i].moreinfo = copy_ui_info_tree(source[i].moreinfo);
	}
	
	return copy;
}


static gint count_ui_info_items (const GnomeUIInfo *ui_info)
{
	gint num;
	gint count=0;

	for(num = 0; ui_info[num].type != GNOME_APP_UI_ENDOFINFO; num++)
		if (ui_info[num].type != GNOME_APP_UI_HELP &&
			ui_info[num].type != GNOME_APP_UI_BUILDER_DATA)
			count++;

	return count;
}

static void free_ui_info_tree (GnomeUIInfo *root)
{
	int count;
	
	for(count = 0; root[count].type != GNOME_APP_UI_ENDOFINFO; count++)
		if( (root[count].type == GNOME_APP_UI_SUBTREE) ||
			(root[count].type == GNOME_APP_UI_SUBTREE_STOCK) ||
			(root[count].type == GNOME_APP_UI_RADIOITEMS) )
			free_ui_info_tree(root[count].moreinfo);
	
	g_free(root);
}

static void set_page_by_widget (GtkNotebook *book, GtkWidget *child)
{
	gint i;

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: set_page_by_widget");
#endif
	
	i = gtk_notebook_page_num(book, child);
	if(i != gtk_notebook_get_current_page(book))
		gtk_notebook_set_current_page(book, i);
}

static GtkWidget *find_item_by_child (GtkMenuShell *shell, GnomeMDIChild *child)
{
	GList *node;

	node = shell->children;
	while(node) {
		if(gtk_object_get_data(GTK_OBJECT(node->data), GNOME_MDI_CHILD_KEY) == child)
			return GTK_WIDGET(node->data);
		
		node = node->next;
	}

	return NULL;
}

static void child_list_activated_cb (GtkWidget *w, GnomeMDI *mdi)
{
	GnomeMDIChild *child;
	
	child = gtk_object_get_data(GTK_OBJECT(w), GNOME_MDI_CHILD_KEY);
	
	if( child && (child != mdi->active_child) ) {
		if(child->views)
			gnome_mdi_set_active_view(mdi, child->views->data);
		else
			gnome_mdi_add_view(mdi, child);
	}
}

static void child_list_menu_create (GnomeMDI *mdi, GnomeApp *app)
{
	GtkWidget *submenu, *item, *label;
	GList *child;
	gint pos;
	
	if(mdi->child_list_path == NULL)
		return;
	
	submenu = gnome_app_find_menu_pos(app->menubar, mdi->child_list_path, &pos);
	
	if(submenu == NULL)
		return;

	child = mdi->children;
	while(child) {
		item = gtk_menu_item_new();
		gtk_signal_connect(GTK_OBJECT(item), "activate",
						   GTK_SIGNAL_FUNC(child_list_activated_cb), mdi);
		label = child_set_label(GNOME_MDI_CHILD(child->data), NULL);
		gtk_widget_show(label);
		gtk_container_add(GTK_CONTAINER(item), label);
		gtk_object_set_data(GTK_OBJECT(item), GNOME_MDI_CHILD_KEY, child->data);
		gtk_widget_show(item);
		
		gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
		
		child = g_list_next(child);
	}
	
	gtk_widget_queue_resize(submenu);
}

void
_gnome_mdi_child_list_menu_remove_item (GnomeMDI *mdi, GnomeMDIChild *child)
{
	GtkWidget *item, *shell;
	GnomeApp *app;
	GList *app_node;
	gint pos;
	
	if(mdi->child_list_path == NULL)
		return;
	
	app_node = mdi->windows;
	while(app_node) {
		app = GNOME_APP(app_node->data);
		
		shell = gnome_app_find_menu_pos(app->menubar, mdi->child_list_path, &pos);


		if(shell) {
			item = find_item_by_child(GTK_MENU_SHELL(shell), child);
			if(item) {
				gtk_container_remove(GTK_CONTAINER(shell), item);
				gtk_widget_queue_resize (GTK_WIDGET (shell));
			}
		}
		
		app_node = app_node->next;
	}
}

void
_gnome_mdi_child_list_menu_add_item (GnomeMDI *mdi, GnomeMDIChild *child)
{
	GtkWidget *item, *submenu, *label;
	GnomeApp *app;
	GList *app_node;
	gint pos;
	
	if(mdi->child_list_path == NULL)
		return;
	
	app_node = mdi->windows;
	while(app_node) {
		app = GNOME_APP(app_node->data);
		
		submenu = gnome_app_find_menu_pos(app->menubar, mdi->child_list_path, &pos);

		if(submenu) {
			item = gtk_menu_item_new();
			gtk_signal_connect(GTK_OBJECT(item), "activate",
							   GTK_SIGNAL_FUNC(child_list_activated_cb), mdi);
			label = child_set_label(child, NULL);
			gtk_widget_show(label);
			gtk_container_add(GTK_CONTAINER(item), label);
			gtk_object_set_data(GTK_OBJECT(item), GNOME_MDI_CHILD_KEY, child);
			gtk_widget_show(item);
			
			gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
			gtk_widget_queue_resize(submenu);
		}
		
		app_node = app_node->next;
	}
}

static gint book_motion (GtkWidget *widget, GdkEventMotion *e, gpointer data)
{
	GnomeMDI *mdi;

	mdi = GNOME_MDI(data);

	if(!drag_cursor)
		drag_cursor = gdk_cursor_new(GDK_HAND2);

	if(e->window == widget->window) {
		mdi->in_drag = TRUE;
		gtk_grab_add(widget);
		gdk_pointer_grab(widget->window, FALSE,
						 GDK_POINTER_MOTION_MASK |
						 GDK_BUTTON_RELEASE_MASK, NULL,
						 drag_cursor, GDK_CURRENT_TIME);
		if(mdi->signal_id) {
			gtk_signal_disconnect(GTK_OBJECT(widget), mdi->signal_id);
			mdi->signal_id = 0;
		}
	}

	return FALSE;
}

static gint book_button_press (GtkWidget *widget, GdkEventButton *e, gpointer data)
{
	GnomeMDI *mdi;

	mdi = GNOME_MDI(data);

	if(e->button == 1 && e->window == widget->window)
		mdi->signal_id = gtk_signal_connect(GTK_OBJECT(widget), "motion_notify_event",
											GTK_SIGNAL_FUNC(book_motion), mdi);

	return FALSE;
}

static gint book_button_release (GtkWidget *widget, GdkEventButton *e, gpointer data)
{
	gint x = e->x_root, y = e->y_root;
	GnomeMDI *mdi;

	mdi = GNOME_MDI(data);
	
	if(mdi->signal_id) {
		gtk_signal_disconnect(GTK_OBJECT(widget), mdi->signal_id);
		mdi->signal_id = 0;
	}

	if(e->button == 1 && e->window == widget->window && mdi->in_drag) {
		GdkWindow *window;
		GList *child;
		GnomeApp *app;
		GtkWidget *view, *new_book;
		GtkNotebook *old_book = GTK_NOTEBOOK(widget);

		mdi->in_drag = FALSE;
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		gtk_grab_remove(widget);

		window = gdk_window_at_pointer(&x, &y);
		if(window)
			window = gdk_window_get_toplevel(window);

		child = mdi->windows;
		while(child) {
			if(window == GTK_WIDGET(child->data)->window) {
				int cur_page;

				/* page was dragged to another notebook */

				old_book = GTK_NOTEBOOK(widget);
				new_book = GNOME_APP(child->data)->contents;

				if(old_book == (GtkNotebook *)new_book) /* page has been dropped on the source notebook */
					return FALSE;

				cur_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (old_book));
	
				if(cur_page >= 0) {
					view = gtk_notebook_get_nth_page (GTK_NOTEBOOK (old_book), cur_page);
					gtk_container_remove(GTK_CONTAINER(old_book), view);

					book_add_view(GTK_NOTEBOOK(new_book), view);

					app = gnome_mdi_get_app_from_view(view);
					gdk_window_raise(GTK_WIDGET(app)->window);

					cur_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (old_book));

					if(cur_page < 0) {
						mdi->active_window = app;
						app = GNOME_APP(gtk_widget_get_toplevel(GTK_WIDGET(old_book)));
						mdi->windows = g_list_remove(mdi->windows, app);
						gtk_widget_destroy(GTK_WIDGET(app));
					}
				}

				return FALSE;
			}
				
			child = child->next;
		}

		if(g_list_length(old_book->children) == 1)
			return FALSE;

		/* create a new toplevel */
		if(old_book->cur_page) {
			gint width, height;
			int cur_page = gtk_notebook_get_current_page (old_book);

			view = gtk_notebook_get_nth_page (old_book, cur_page);
				
			app = gnome_mdi_get_app_from_view(view);

			width = view->allocation.width;
			height = view->allocation.height;
			
			gtk_container_remove(GTK_CONTAINER(old_book), view);
				
			app_clone(mdi, app);
				
			new_book = book_create(mdi);
	
			book_add_view(GTK_NOTEBOOK(new_book), view);

			gtk_window_set_position(GTK_WINDOW(mdi->active_window), GTK_WIN_POS_MOUSE);
	
			gtk_widget_set_usize (view, width, height);

			if(!GTK_WIDGET_VISIBLE(mdi->active_window))
				gtk_widget_show(GTK_WIDGET(mdi->active_window));
		}
	}

	return FALSE;
}

static GtkWidget *book_create (GnomeMDI *mdi)
{
	GtkWidget *us;
	
	us = gtk_notebook_new();

	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(us), mdi->tab_pos);
	
	gtk_widget_show(us);

	gnome_app_set_contents(mdi->active_window, us);
	
	gtk_widget_add_events(us, GDK_BUTTON1_MOTION_MASK);

	gtk_signal_connect(GTK_OBJECT(us), "switch_page",
			   GTK_SIGNAL_FUNC(book_switch_page), mdi);
	gtk_signal_connect(GTK_OBJECT(us), "button_press_event",
			   GTK_SIGNAL_FUNC(book_button_press), mdi);
	gtk_signal_connect(GTK_OBJECT(us), "button_release_event",
			   GTK_SIGNAL_FUNC(book_button_release), mdi);

	gtk_notebook_set_scrollable(GTK_NOTEBOOK(us), TRUE);
	
	return us;
}

static void book_add_view (GtkNotebook *book, GtkWidget *view)
{
	GnomeMDIChild *child;
	GtkWidget *title;

	child = gnome_mdi_get_child_from_view(view);

	title = child_set_label(child, NULL);

	gtk_notebook_append_page(book, view, title);

	if(g_list_length(book->children) > 1)
		set_page_by_widget(book, view);
}

static void book_switch_page(GtkNotebook *book, GtkNotebookPage *pg, gint page_num, GnomeMDI *mdi)
{
	GnomeApp *app;
	GtkWidget *page;

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: switching pages");
#endif
	app = GNOME_APP(gtk_widget_get_toplevel(GTK_WIDGET(book)));

	page = gtk_notebook_get_nth_page (book, page_num);

	if(page != NULL) {
		if(page != mdi->active_view)
			app_set_view(mdi, app, page);
	} else {
		app_set_view(mdi, app, NULL);  
	}
}

static void toplevel_focus (GnomeApp *app, GdkEventFocus *event, GnomeMDI *mdi)
{
	/* updates active_view and active_child when a new toplevel receives focus */
	g_return_if_fail(GNOME_IS_APP(app));
#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: toplevel receiving focus");
#endif
	mdi->active_window = app;
	
	if((mdi->mode == GNOME_MDI_TOPLEVEL) || (mdi->mode == GNOME_MDI_MODAL)) {
		set_active_view(mdi, app->contents);
	} else if((mdi->mode == GNOME_MDI_NOTEBOOK) && GTK_NOTEBOOK(app->contents)->cur_page) {
		int cur_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (app->contents));
		GtkWidget *child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (app->contents), cur_page);
		set_active_view(mdi, child);
	} else {
		set_active_view(mdi, NULL);
	}
}

static void app_clone(GnomeMDI *mdi, GnomeApp *app)
{
	BonoboDockLayout *layout;
	gchar *layout_string = NULL;

	if(app) {
		layout = bonobo_dock_get_layout(BONOBO_DOCK(app->dock));
		layout_string = bonobo_dock_layout_create_string(layout);
		gtk_object_unref(GTK_OBJECT(layout));
	}

	app_create(mdi, layout_string);

	if(layout_string)
		g_free(layout_string);
}

static gint app_close_top (GnomeApp *app, GdkEventAny *event, GnomeMDI *mdi)
{
	GnomeMDIChild *child = NULL;
	
	if(g_list_length(mdi->windows) == 1) {
		if(!gnome_mdi_remove_all(mdi, FALSE))
			return TRUE;

		mdi->windows = g_list_remove(mdi->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));
		
		/* only destroy mdi if there are no external objects registered
		   with it. */
		if(mdi->registered == NULL)
			gtk_object_destroy(GTK_OBJECT(mdi));
	}
	else if(app->contents) {
		child = gnome_mdi_get_child_from_view(app->contents);
		if(g_list_length(child->views) == 1) {
			/* if this is the last view, we have to remove the child! */
			if(!gnome_mdi_remove_child(mdi, child, FALSE))
				return TRUE;
		}
		else
			gnome_mdi_remove_view(mdi, app->contents, FALSE);
	}
	else {
		mdi->windows = g_list_remove(mdi->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));
	}

	return FALSE;
}

static gint app_close_book (GnomeApp *app, GdkEventAny *event, GnomeMDI *mdi)
{
	GnomeMDIChild *child;
	GtkWidget *view;
	gint handler_ret = TRUE;
	
	if(g_list_length(mdi->windows) == 1) {		
		if(!gnome_mdi_remove_all(mdi, FALSE))
			return TRUE;

		mdi->windows = g_list_remove(mdi->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));
		
		/* only destroy mdi if there are no non-MDI windows registered
		   with it. */
		if(mdi->registered == NULL)
			gtk_object_destroy(GTK_OBJECT(mdi));
	}
	else {
		GList *children = gtk_container_get_children (GTK_CONTAINER (app->contents));
		GList *li;

		/* first check if all the children in this notebook can be removed */
		if (children == NULL) {
			mdi->windows = g_list_remove(mdi->windows, app);
			gtk_widget_destroy(GTK_WIDGET(app));
			return FALSE;
		}

		for (li = children; li != NULL; li = li->next) {
			GList *node;
			view = li->data;

			child = gnome_mdi_get_child_from_view(view);
			
			node = child->views;
			while(node) {
				if(gnome_mdi_get_app_from_view(node->data) != app)
					break;
				
				node = node->next;
			}
			
			if(node == NULL) {   /* all the views reside in this GnomeApp */
				gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_CHILD],
						child, &handler_ret);
				if(handler_ret == FALSE) {
					g_list_free (children);
					return TRUE;
				}
			}
		}
		
		/* now actually remove all children/views! */
		for (li = children; li != NULL; li = li->next) {
			view = li->data;

			child = gnome_mdi_get_child_from_view(view);
			
			/* if this is the last view, remove the child */
			if(g_list_length(child->views) == 1)
				gnome_mdi_remove_child(mdi, child, TRUE);
			else
				gnome_mdi_remove_view(mdi, view, TRUE);
		}

		g_list_free (children);
	}
	
	return FALSE;
}

static void app_set_view (GnomeMDI *mdi, GnomeApp *app, GtkWidget *view)
{
	GList *menu_list = NULL, *children;
	GtkWidget *parent = NULL;
	GnomeMDIChild *child;
	GnomeUIInfo *ui_info;
	gint pos, items;

	/* free previous child ui-info */
	ui_info = gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_CHILD_MENU_INFO_KEY);
	if(ui_info != NULL) {
		free_ui_info_tree(ui_info);
		gtk_object_set_data(GTK_OBJECT(app), GNOME_MDI_CHILD_MENU_INFO_KEY, NULL);
	}
	ui_info = NULL;
	
	if(mdi->child_menu_path)
		parent = gnome_app_find_menu_pos(app->menubar, mdi->child_menu_path, &pos);

	/* remove old child-specific menus */
	items = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_ITEM_COUNT_KEY));
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
		child = gnome_mdi_get_child_from_view(view);
		
		/* set the title */
		if( (mdi->mode == GNOME_MDI_MODAL) || (mdi->mode == GNOME_MDI_TOPLEVEL) ) {
			/* in MODAL and TOPLEVEL modes the window title includes the active
			   child name: "child_name - mdi_title" */
			gchar *fullname;
			
			fullname = g_strconcat(child->name, " - ", mdi->title, NULL);
			gtk_window_set_title(GTK_WINDOW(app), fullname);
			g_free(fullname);
		}
		else
			gtk_window_set_title(GTK_WINDOW(app), mdi->title);
		
		/* create new child-specific menus */
		if(parent) {
			if( child->menu_template &&
				( (ui_info = copy_ui_info_tree(child->menu_template)) != NULL) ) {
				gnome_app_insert_menus_with_data(app, mdi->child_menu_path, ui_info, child);
				gtk_object_set_data(GTK_OBJECT(app), GNOME_MDI_CHILD_MENU_INFO_KEY, ui_info);
				items = count_ui_info_items(ui_info);
			}
			else {
				menu_list = child_create_menus(child, view);
			
				if(menu_list) {
					GList *menu;

					items = 0;
					menu = menu_list;					
					while(menu) {
						gtk_menu_shell_insert(GTK_MENU_SHELL(parent), GTK_WIDGET(menu->data), pos);
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
	}
	else
		gtk_window_set_title(GTK_WINDOW(app), mdi->title);

	gtk_object_set_data(GTK_OBJECT(app), GNOME_MDI_ITEM_COUNT_KEY, GINT_TO_POINTER(items));

	if(parent)
		gtk_widget_queue_resize(parent);

	set_active_view(mdi, view);
}

static void app_destroy (GnomeApp *app, GnomeMDI *mdi)
{
	GnomeUIInfo *ui_info;
	
	if(mdi->active_window == app)
		mdi->active_window = (mdi->windows != NULL)?GNOME_APP(mdi->windows->data):NULL;

	/* free stuff that got allocated for this GnomeApp */

	ui_info = gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_MENUBAR_INFO_KEY);
	if(ui_info)
		free_ui_info_tree(ui_info);
	
	ui_info = gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_TOOLBAR_INFO_KEY);
	if(ui_info)
		free_ui_info_tree(ui_info);
	
	ui_info = gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_CHILD_MENU_INFO_KEY);
	if(ui_info)
		free_ui_info_tree(ui_info);
}

static void app_create (GnomeMDI *mdi, gchar *layout_string)
{
	GtkWidget *window;
	GnomeApp *app;
	GtkSignalFunc func = NULL;
	GnomeUIInfo *ui_info;
	
	window = gnome_app_new(mdi->appname, mdi->title);

	app = GNOME_APP(window);

	/* don't do automagical layout saving */
	app->enable_layout_config = FALSE;
	
	gtk_window_set_wmclass (GTK_WINDOW (window), mdi->appname, mdi->appname);
  
	gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, FALSE);
  
	mdi->windows = g_list_append(mdi->windows, window);

	if( (mdi->mode == GNOME_MDI_TOPLEVEL) || (mdi->mode == GNOME_MDI_MODAL))
		func = GTK_SIGNAL_FUNC(app_close_top);
	else if(mdi->mode == GNOME_MDI_NOTEBOOK)
		func = GTK_SIGNAL_FUNC(app_close_book);
	
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
					   func, mdi);
	gtk_signal_connect(GTK_OBJECT(window), "focus_in_event",
                     GTK_SIGNAL_FUNC(toplevel_focus), mdi);
	gtk_signal_connect(GTK_OBJECT(window), "destroy",
					   GTK_SIGNAL_FUNC(app_destroy), mdi);

	/* set up menus */
	if(mdi->menu_template) {
		ui_info = copy_ui_info_tree(mdi->menu_template);
		gnome_app_create_menus_with_data(app, ui_info, mdi);
		gtk_object_set_data(GTK_OBJECT(window), GNOME_MDI_MENUBAR_INFO_KEY, ui_info);
	}
		
	/* create toolbar */
	if(mdi->toolbar_template) {
		ui_info = copy_ui_info_tree(mdi->toolbar_template);
		gnome_app_create_toolbar_with_data(app, ui_info, mdi);
		gtk_object_set_data(GTK_OBJECT(window), GNOME_MDI_TOOLBAR_INFO_KEY, ui_info);
	}
	
	mdi->active_window = app;
	mdi->active_child = NULL;
	mdi->active_view = NULL;
	
	gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[APP_CREATED], window);

	child_list_menu_create(mdi, app);

	if(layout_string && app->layout)
		bonobo_dock_layout_parse_string(app->layout, layout_string);
}

static void top_add_view (GnomeMDI *mdi, GnomeMDIChild *child, GtkWidget *view)
{
	GnomeApp *window;

	if (mdi->active_window->contents != NULL)
		app_clone(mdi, mdi->active_window);
	
	window = mdi->active_window;

	if(child && view)
		gnome_app_set_contents(window, view);

	app_set_view(mdi, window, view);

	if(!GTK_WIDGET_VISIBLE(window))
		gtk_widget_show(GTK_WIDGET(window));
}

static void set_active_view (GnomeMDI *mdi, GtkWidget *view)
{
	GnomeMDIChild *old_child;
	GtkWidget *old_view;

#ifdef GNOME_ENABLE_DEBUG
	g_message("setting active_view to %08lx\n", view);
#endif

	old_child = mdi->active_child;
	old_view = mdi->active_view;

	if(!view) {
		mdi->active_view = NULL;
		mdi->active_child = NULL;
	}

	if(view == old_view)
		return;
	
	if(view) {
		mdi->active_child = gnome_mdi_get_child_from_view(view);
		mdi->active_window = gnome_mdi_get_app_from_view(view);
	}

	mdi->active_view = view;

	if(mdi->active_child != old_child)
		gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[CHILD_CHANGED], old_child);

	gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[VIEW_CHANGED], old_view);
}

/**
 * gnome_mdi_set_active_view:
 * @mdi: A pointer to an MDI object.
 * @view: A pointer to the view that is to become the active one.
 * 
 * Description:
 * Sets the active view to @view. It also raises the window containing it
 * and gives it focus.
 **/
void gnome_mdi_set_active_view (GnomeMDI *mdi, GtkWidget *view)
{
	GtkWindow *window;
	
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));
	g_return_if_fail(view != NULL);
	g_return_if_fail(GTK_IS_WIDGET(view));

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: setting active view");
#endif
	
	if(mdi->mode == GNOME_MDI_NOTEBOOK)
		set_page_by_widget(GTK_NOTEBOOK(view->parent), view);
	if(mdi->mode == GNOME_MDI_MODAL) {
		if(mdi->active_window->contents) {
			gnome_mdi_remove_view(mdi, mdi->active_window->contents, TRUE);
			mdi->active_window->contents = NULL;
		}
		
		gnome_app_set_contents(mdi->active_window, view);
		app_set_view(mdi, mdi->active_window, view);
	}

	window = GTK_WINDOW(gnome_mdi_get_app_from_view(view));
	
	/* TODO: hmmm... I dont know how to give focus to the window, so that it
	   would receive keyboard events */
	gdk_window_raise(GTK_WIDGET(window)->window);

	set_active_view(mdi, view);
}

/**
 * gnome_mdi_add_view:
 * @mdi: A pointer to a GnomeMDI object.
 * @child: A pointer to a child.
 * 
 * Description:
 * Creates a new view of the child and adds it to the MDI. GnomeMDIChild
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
gint gnome_mdi_add_view (GnomeMDI *mdi, GnomeMDIChild *child)
{
	GtkWidget *view;
	gint ret = TRUE;
	
	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI_CHILD(child), FALSE);
	
	if(mdi->mode != GNOME_MDI_MODAL || child->views == NULL)
		view = gnome_mdi_child_add_view(child);
	else {
		view = GTK_WIDGET(child->views->data);

		if(child == mdi->active_child)
			return TRUE;
	}

	gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[ADD_VIEW], view, &ret);

	if(ret == FALSE) {
		gnome_mdi_child_remove_view(child, view);
		return FALSE;
	}

	if(mdi->active_window == NULL) {
		app_create(mdi, NULL);
		gtk_widget_show(GTK_WIDGET(mdi->active_window));
	}

	/* this reference will compensate the view's unrefing
	   when removed from its parent later, as we want it to
	   stay valid until removed from the child with a call
	   to gnome_mdi_child_remove_view() */
	gtk_widget_ref(view);

	if(!GTK_WIDGET_VISIBLE(view))
		gtk_widget_show(view);

	if(mdi->mode == GNOME_MDI_NOTEBOOK) {
		if(mdi->active_window->contents == NULL)
			book_create(mdi);
		book_add_view(GTK_NOTEBOOK(mdi->active_window->contents), view);
	}
	else if(mdi->mode == GNOME_MDI_TOPLEVEL)
		/* add a new toplevel unless the remaining one is empty */
		top_add_view(mdi, child, view);
	else if(mdi->mode == GNOME_MDI_MODAL) {
		/* replace the existing view if there is one */
		if(mdi->active_window->contents) {
			gnome_mdi_remove_view(mdi, mdi->active_window->contents, TRUE);
			mdi->active_window->contents = NULL;
		}

		gnome_app_set_contents(mdi->active_window, view);
		app_set_view(mdi, mdi->active_window, view);
	}

	return TRUE;
}

/**
 * gnome_mdi_add_toplevel_view:
 * @mdi: A pointer to a GnomeMDI object.
 * @child: A pointer to a GnomeMDIChild object to be added to the MDI.
 * 
 * Description:
 * Creates a new view of the child and adds it to the MDI; it behaves the
 * same way as gnome_mdi_add_view in %GNOME_MDI_MODAL and %GNOME_MDI_TOPLEVEL
 * modes, but in %GNOME_MDI_NOTEBOOK mode, the view is added in a new
 * toplevel window unless the active one has no views in it. 
 * 
 * Return value: 
 * %TRUE if adding the view succeeded and %FALSE otherwise.
 **/
gint gnome_mdi_add_toplevel_view (GnomeMDI *mdi, GnomeMDIChild *child)
{
	GtkWidget *view;
	gint ret = TRUE;
	
	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI_CHILD(child), FALSE);
	
	if(mdi->mode != GNOME_MDI_MODAL || child->views == NULL)
		view = gnome_mdi_child_add_view(child);
	else {
		view = GTK_WIDGET(child->views->data);

		if(child == mdi->active_child)
			return TRUE;
	}

	if(!view)
		return FALSE;

	gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[ADD_VIEW], view, &ret);

	if(ret == FALSE) {
		gnome_mdi_child_remove_view(child, view);
		return FALSE;
	}

	gnome_mdi_open_toplevel(mdi);

	/* this reference will compensate the view's unrefing
	   when removed from its parent later, as we want it to
	   stay valid until removed from the child with a call
	   to gnome_mdi_child_remove_view() */
	gtk_widget_ref(view);

	if(!GTK_WIDGET_VISIBLE(view))
		gtk_widget_show(view);

	if(mdi->mode == GNOME_MDI_NOTEBOOK)
		book_add_view(GTK_NOTEBOOK(mdi->active_window->contents), view);
	else if(mdi->mode == GNOME_MDI_TOPLEVEL)
		/* add a new toplevel unless the remaining one is empty */
		top_add_view(mdi, child, view);
	else if(mdi->mode == GNOME_MDI_MODAL) {
		/* replace the existing view if there is one */
		if(mdi->active_window->contents) {
			gnome_mdi_remove_view(mdi, mdi->active_window->contents, TRUE);
			mdi->active_window->contents = NULL;
		}

		gnome_app_set_contents(mdi->active_window, view);
		app_set_view(mdi, mdi->active_window, view);
	}
	
	return TRUE;
}

/**
 * gnome_mdi_remove_view:
 * @mdi: A pointer to a GnomeMDI object.
 * @view: View to remove.
 * @force: If TRUE, the "remove_view" signal is not emitted.
 * 
 * Description:
 * Removes a view from an MDI. 
 * A "remove_view" signal is emitted to the MDI before actually removing
 * view. The view is removed only if the signal handler (if it exists and
 * the @force is set to %FALSE) returns %TRUE. 
 * 
 * Return value: 
 * %TRUE if the view was removed and %FALSE otherwise.
 **/
gint gnome_mdi_remove_view (GnomeMDI *mdi, GtkWidget *view, gint force)
{
	GtkWidget *parent;
	GnomeApp *window;
	GnomeMDIChild *child;
	gint ret = TRUE;
	
	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(view != NULL, FALSE);
	g_return_val_if_fail(GTK_IS_WIDGET(view), FALSE);

	if(!force)
		gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_VIEW], view, &ret);

	if(ret == FALSE)
		return FALSE;

	child = gnome_mdi_get_child_from_view(view);

	parent = view->parent;

	if(!parent)
		return TRUE;

	window = gnome_mdi_get_app_from_view(view);

	gtk_container_remove(GTK_CONTAINER(parent), view);

	if(view == mdi->active_view)
	   mdi->active_view = NULL;

	if( (mdi->mode == GNOME_MDI_TOPLEVEL) || (mdi->mode == GNOME_MDI_MODAL) ) {
		window->contents = NULL;

		/* if this is NOT the last toplevel or a registered object exists,
		   destroy the toplevel */
		if(g_list_length(mdi->windows) > 1 || mdi->registered) {
			mdi->windows = g_list_remove(mdi->windows, window);
			gtk_widget_destroy(GTK_WIDGET(window));
			if(mdi->active_window && view == mdi->active_view)
				gnome_mdi_set_active_view(mdi, gnome_mdi_get_view_from_window(mdi, mdi->active_window));
		}
		else
			app_set_view(mdi, window, NULL);
	}
	else if(mdi->mode == GNOME_MDI_NOTEBOOK) {
		if(GTK_NOTEBOOK(parent)->cur_page == NULL) {
			if(g_list_length(mdi->windows) > 1 || mdi->registered) {
				/* if this is NOT the last toplevel or a registered object
				   exists, destroy the toplevel */
				mdi->windows = g_list_remove(mdi->windows, window);
				gtk_widget_destroy(GTK_WIDGET(window));
				if(mdi->active_window && view == mdi->active_view)
					mdi->active_view = gnome_mdi_get_view_from_window(mdi, mdi->active_window);
			}
			else
				app_set_view(mdi, window, NULL);
		}
		else
			app_set_view(mdi, window, gtk_notebook_get_nth_page(GTK_NOTEBOOK(parent), gtk_notebook_get_current_page(GTK_NOTEBOOK(parent))));
	}

	/* remove this view from the child's view list unless in MODAL mode */
	if(mdi->mode != GNOME_MDI_MODAL)
		gnome_mdi_child_remove_view(child, view);

	return TRUE;
}

/**
 * gnome_mdi_add_child:
 * @mdi: A pointer to a GnomeMDI object.
 * @child: A pointer to a GnomeMDIChild to add to the MDI.
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
gint gnome_mdi_add_child (GnomeMDI *mdi, GnomeMDIChild *child)
{
	gint ret = TRUE;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI_CHILD(child), FALSE);

	gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[ADD_CHILD], child, &ret);

	if(ret == FALSE)
		return FALSE;

	child->parent = GTK_OBJECT(mdi);

	mdi->children = g_list_append(mdi->children, child);

	_gnome_mdi_child_list_menu_add_item(mdi, child);

	return TRUE;
}

/**
 * gnome_mdi_remove_child:
 * @mdi: A pointer to a GnomeMDI object.
 * @child: Child to remove.
 * @force: If TRUE, the "remove_child" signal is not emitted
 * 
 * Description:
 * Removes a child and all of its views from the MDI. 
 * A "remove_child" signal is emitted to the MDI with @child as its parameter
 * before actually removing the child. The child is removed only if the signal
 * handler (if it exists and the @force is set to %FALSE) returns %TRUE. 
 * 
 * Return value: 
 * %TRUE if the removal was successful and %FALSE otherwise.
 **/
gint gnome_mdi_remove_child (GnomeMDI *mdi, GnomeMDIChild *child, gint force)
{
	gint ret = TRUE;
	GList *view_node;
	GtkWidget *view;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI_CHILD(child), FALSE);

	/* if force is set to TRUE, don't call the remove_child handler (ie there is no way for the
	   user to stop removal of the child) */
	if(!force)
		gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_CHILD], child, &ret);

	if(ret == FALSE)
		return FALSE;

	view_node = child->views;
	while(view_node) {
		view = GTK_WIDGET(view_node->data);
		view_node = view_node->next;
		gnome_mdi_remove_view(mdi, GTK_WIDGET(view), TRUE);
	}

	mdi->children = g_list_remove(mdi->children, child);

	_gnome_mdi_child_list_menu_remove_item(mdi, child);

	if(child == mdi->active_child)
		mdi->active_child = NULL;

	child->parent = NULL;

	gtk_object_unref(GTK_OBJECT(child));

	if(mdi->mode == GNOME_MDI_MODAL && mdi->children) {
		GnomeMDIChild *next_child = mdi->children->data;

		if(next_child->views) {
			gnome_app_set_contents(mdi->active_window,
								   GTK_WIDGET(next_child->views->data));
			app_set_view(mdi, mdi->active_window,
						 GTK_WIDGET(next_child->views->data));
		}
		else
			gnome_mdi_add_view(mdi, next_child);
	}

	return TRUE;
}

/**
 * gnome_mdi_remove_all:
 * @mdi: A pointer to a GnomeMDI object.
 * @force: If TRUE, the "remove_child" signal is not emitted
 * 
 * Description:
 * Removes all children and all views from the MDI. 
 * A "remove_child" signal is emitted to the MDI for each child before
 * actually trying to remove any. If signal handlers for all children (if
 * they exist and the @force is set to %FALSE) return %TRUE, all children
 * and their views are removed and none otherwise. 
 * 
 * Return value:
 * %TRUE if the removal was successful and %FALSE otherwise.
 **/
gint gnome_mdi_remove_all (GnomeMDI *mdi, gint force)
{
	GList *child_node;
	gint handler_ret = TRUE;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);

	/* first check if removal of any child will be prevented by the
	   remove_child signal handler */
	if(!force) {
		child_node = mdi->children;
		while(child_node) {
			gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_CHILD],
							child_node->data, &handler_ret);

			/* if any of the children may not be removed, none will be */
			if(handler_ret == FALSE)
				return FALSE;

			child_node = child_node->next;
		}
	}

	/* remove all the children with force arg set to true so that remove_child
	   handlers are not called again */
	while(mdi->children)
		gnome_mdi_remove_child(mdi, GNOME_MDI_CHILD(mdi->children->data), TRUE);

	return TRUE;
}

/**
 * gnome_mdi_open_toplevel:
 * @mdi: A pointer to a GnomeMDI object.
 * 
 * Description:
 * Opens a new toplevel window (unless in %GNOME_MDI_MODAL mode and a
 * toplevel window is already open). This is usually used only for opening
 * the initial window on startup (just before calling gtkmain()) if no
 * windows were open because a session was restored or children were added
 * because of command line args).
 **/
void gnome_mdi_open_toplevel (GnomeMDI *mdi)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	if( mdi->mode != GNOME_MDI_MODAL || mdi->windows == NULL ) {
		app_clone(mdi, mdi->active_window);

		if(mdi->mode == GNOME_MDI_NOTEBOOK)
			book_create(mdi);
		
		gtk_widget_show(GTK_WIDGET(mdi->active_window));
	}
}

/**
 * gnome_mdi_update_child:
 * @mdi: A pointer to a GnomeMDI object.
 * @child: Child to update.
 * 
 * Description:
 * Updates all notebook labels of @child's views and their window titles
 * after its name changes. It is not required if gnome_mdi_child_set_name()
 * is used for setting the child's name.
 **/
void gnome_mdi_update_child (GnomeMDI *mdi, GnomeMDIChild *child)
{
	GtkWidget *view, *title;
	GList *view_node;

	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));
	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_MDI_CHILD(child));

	view_node = child->views;
	while(view_node) {
		view = GTK_WIDGET(view_node->data);

		/* for the time being all that update_child() does is update the
		   children's names */
		if( (mdi->mode == GNOME_MDI_MODAL) ||
			(mdi->mode == GNOME_MDI_TOPLEVEL) ) {
			/* in MODAL and TOPLEVEL modes the window title includes the active
			   child name: "child_name - mdi_title" */
			gchar *fullname;
      
			fullname = g_strconcat(child->name, " - ", mdi->title, NULL);
			gtk_window_set_title(GTK_WINDOW(gnome_mdi_get_app_from_view(view)),
								 fullname);
			g_free(fullname);
		}
		else if(mdi->mode == GNOME_MDI_NOTEBOOK) {
			GtkWidget *tab_label;

			tab_label = gtk_notebook_get_tab_label
				(GTK_NOTEBOOK (view->parent), view);
			if(tab_label != NULL)
				title = child_set_label(child, tab_label);
		}
		
		view_node = g_list_next(view_node);
	}
}

/**
 * gnome_mdi_find_child:
 * @mdi: A pointer to a GnomeMDI object.
 * @name: A string with a name of the child to find.
 * 
 * Description:
 * Finds a child named @name.
 * 
 * Return value: 
 * A pointer to the GnomeMDIChild object if the child was found and NULL
 * otherwise.
 **/
GnomeMDIChild *gnome_mdi_find_child (GnomeMDI *mdi, const gchar *name)
{
	GList *child_node;

	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);

	child_node = mdi->children;
	while(child_node) {
		if(strcmp(GNOME_MDI_CHILD(child_node->data)->name, name) == 0)
			return GNOME_MDI_CHILD(child_node->data);

		child_node = g_list_next(child_node);
	}

	return NULL;
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
void gnome_mdi_set_mode (GnomeMDI *mdi, GnomeMDIMode mode)
{
	GtkWidget *view;
	GnomeMDIChild *child;
	GList *child_node, *view_node, *app_node;
	gint windows = (mdi->windows != NULL);
	guint16 width = 0, height = 0;

	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	/* FIXME! 
	if(mode == GNOME_MDI_DEFAULT_MODE)
		mode = gnome_preferences_get_mdi_mode();
		*/
	mode = GNOME_MDI_NOTEBOOK;

	if(mdi->active_view) {
		width = mdi->active_view->allocation.width;
		height = mdi->active_view->allocation.height;
	}

	/* remove all views from their parents */
	child_node = mdi->children;
	while(child_node) {
		child = GNOME_MDI_CHILD(child_node->data);
		view_node = child->views;
		while(view_node) {
			view = GTK_WIDGET(view_node->data);

			if(view->parent) {
				if( (mdi->mode == GNOME_MDI_TOPLEVEL) ||
					(mdi->mode == GNOME_MDI_MODAL) )
					gnome_mdi_get_app_from_view(view)->contents = NULL;

				gtk_container_remove(GTK_CONTAINER(view->parent), view);
			}

			view_node = view_node->next;

			/* if we are to change mode to MODAL, destroy all views except
			   the active one */
			/* if( (mode == GNOME_MDI_MODAL) && (view != mdi->active_view) )
			   gnome_mdi_child_remove_view(child, view); */
		}
		child_node = child_node->next;
	}

	/* remove all GnomeApps but the active one */
	app_node = mdi->windows;
	while(app_node) {
		if(GNOME_APP(app_node->data) != mdi->active_window)
			gtk_widget_destroy(GTK_WIDGET(app_node->data));
		app_node = app_node->next;
	}

	if(mdi->windows)
		g_list_free(mdi->windows);

	if(mdi->active_window) {
		if(mdi->mode == GNOME_MDI_NOTEBOOK)
			gtk_container_remove(GTK_CONTAINER(mdi->active_window->dock),
					     BONOBO_DOCK(mdi->active_window->dock)->client_area);

		mdi->active_window->contents = NULL;

		if( (mdi->mode == GNOME_MDI_TOPLEVEL) || (mdi->mode == GNOME_MDI_MODAL))
			gtk_signal_disconnect_by_func(GTK_OBJECT(mdi->active_window),
										  GTK_SIGNAL_FUNC(app_close_top), mdi);
		else if(mdi->mode == GNOME_MDI_NOTEBOOK)
			gtk_signal_disconnect_by_func(GTK_OBJECT(mdi->active_window),
										  GTK_SIGNAL_FUNC(app_close_book), mdi);

		if( (mode == GNOME_MDI_TOPLEVEL) || (mode == GNOME_MDI_MODAL))
			gtk_signal_connect(GTK_OBJECT(mdi->active_window), "delete_event",
							   GTK_SIGNAL_FUNC(app_close_top), mdi);
		else if(mode == GNOME_MDI_NOTEBOOK)
			gtk_signal_connect(GTK_OBJECT(mdi->active_window), "delete_event",
							   GTK_SIGNAL_FUNC(app_close_book), mdi);

		mdi->windows = g_list_append(NULL, mdi->active_window);

		if(mode == GNOME_MDI_NOTEBOOK) 
			book_create(mdi);
	}

	mdi->mode = mode;

	/* re-implant views in proper containers */
	child_node = mdi->children;
	while(child_node) {
		child = GNOME_MDI_CHILD(child_node->data);
		view_node = child->views;
		while(view_node) {
			view = GTK_WIDGET(view_node->data);

			if(width != 0)
				gtk_widget_set_usize(view, width, height);

			if(mdi->mode == GNOME_MDI_NOTEBOOK)
				book_add_view(GTK_NOTEBOOK(mdi->active_window->contents), view);
			else if(mdi->mode == GNOME_MDI_TOPLEVEL)
				/* add a new toplevel unless the remaining one is empty */
				top_add_view(mdi, child, view);
			else if(mdi->mode == GNOME_MDI_MODAL) {
				/* replace the existing view if there is one */
				if(mdi->active_window->contents) {
					gnome_mdi_remove_view(mdi, mdi->active_window->contents, TRUE);
					mdi->active_window->contents = NULL;
				}
				gnome_app_set_contents(mdi->active_window, view);
				app_set_view(mdi, mdi->active_window, view);

				mdi->active_view = view;
			}

			view_node = view_node->next;

			gtk_widget_show(GTK_WIDGET(mdi->active_window));
		}
		child_node = child_node->next;
	}

	if(windows && !mdi->active_window)
		gnome_mdi_open_toplevel(mdi);
}

/**
 * gnome_mdi_get_active_child:
 * @mdi: A pointer to a GnomeMDI object.
 * 
 * Description:
 * Returns a pointer to the active GnomeMDIChild object.
 * 
 * Return value: 
 * A pointer to the active GnomeMDIChild object. %NULL, if there is none.
 **/
GnomeMDIChild *gnome_mdi_get_active_child (GnomeMDI *mdi)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);

	if(mdi->active_view)
		return(gnome_mdi_get_child_from_view(mdi->active_view));

	return NULL;
}

/**
 * gnome_mdi_get_active_view:
 * @mdi: A pointer to a GnomeMDI object.
 * 
 * Description:
 * Returns a pointer to the active view (the one with the focus).
 * 
 * Return value: 
 * A pointer to a GtkWidget *.
 **/
GtkWidget *gnome_mdi_get_active_view(GnomeMDI *mdi)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);

	return mdi->active_view;
}

/**
 * gnome_mdi_get_active_window:
 * @mdi: A pointer to a GnomeMDI object.
 * 
 * Description:
 * Returns a pointer to the toplevel window containing the active view.
 * 
 * Return value:
 * A pointer to a GnomeApp that has the focus.
 **/
GnomeApp *gnome_mdi_get_active_window (GnomeMDI *mdi)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);

	return mdi->active_window;
}

/**
 * gnome_mdi_set_menubar_template:
 * @mdi: A pointer to a GnomeMDI object.
 * @menu_tmpl: A GnomeUIInfo array describing the menu.
 * 
 * Description:
 * This function sets the template for menus that appear in each toplevel
 * window to menu_template. For each new toplevel window created by the MDI,
 * this structure is copied, the menus are created with
 * gnome_app_create_menus_with_data() function with mdi as the callback
 * user data. Finally, the pointer to the copy is assigned to the new
 * toplevel window (a GnomeApp widget) and can be obtained by calling
 * &gnome_mdi_get_menubar_info.
 **/
void gnome_mdi_set_menubar_template (GnomeMDI *mdi, GnomeUIInfo *menu_tmpl)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	mdi->menu_template = menu_tmpl;
}

/**
 * gnome_mdi_set_toolbar_template:
 * @mdi: A pointer to a GnomeMDI object.
 * @tbar_tmpl: A GnomeUIInfo array describing the toolbar.
 * 
 * Description:
 * This function sets the template for toolbar that appears in each toplevel
 * window to toolbar_template. For each new toplevel window created by the MDI,
 * this structure is copied, the toolbar is created with
 * gnome_app_create_toolbar_with_data() function with mdi as the callback
 * user data. Finally, the pointer to the copy is assigned to the new toplevel
 * window (a GnomeApp widget) and can be retrieved with a call to
 * &gnome_mdi_get_toolbar_info. 
 **/
void gnome_mdi_set_toolbar_template (GnomeMDI *mdi, GnomeUIInfo *tbar_tmpl)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	mdi->toolbar_template = tbar_tmpl;
}

/**
 * gnome_mdi_set_child_menu_path:
 * @mdi: A pointer to a GnomeMDI object. 
 * @path: A menu path where the child menus should be inserted.
 * 
 * Description:
 * Sets the desired position of child-specific menus (which are added to and
 * removed from the main menus as views of different children are activated).
 * See gnome_app_find_menu_pos for details on menu paths. 
 **/
void gnome_mdi_set_child_menu_path (GnomeMDI *mdi, const gchar *path)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	if(mdi->child_menu_path)
		g_free(mdi->child_menu_path);

	mdi->child_menu_path = g_strdup(path);
}

/**
 * gnome_mdi_set_child_list_path:
 * @mdi: A pointer to a GnomeMDI object.
 * @path: A menu path where the child list menu should be inserted
 * 
 * Description:
 * Sets the position for insertion of menu items used to activate the MDI
 * children that were added to the MDI. See gnome_app_find_menu_pos for
 * details on menu paths. If the path is not set or set to %NULL, these menu
 * items aren't going to be inserted in the MDI menu structure. Note that if
 * you want all menu items to be inserted in their own submenu, you have to
 * create that submenu (and leave it empty, of course).
 **/
void gnome_mdi_set_child_list_path (GnomeMDI *mdi, const gchar *path)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	if(mdi->child_list_path)
		g_free(mdi->child_list_path);

	mdi->child_list_path = g_strdup(path);
}

/**
 * gnome_mdi_register:
 * @mdi: A pointer to a GnomeMDI object.
 * @object: Object to register.
 * 
 * Description:
 * Registers GtkObject @object with MDI. 
 * This is mostly intended for applications that open other windows besides
 * those opened by the MDI and want to continue to run even when no MDI
 * windows exist (an example of this would be GIMP's window with tools, if
 * the pictures were MDI children). As long as there is an object registered
 * with the MDI, the MDI will not destroy itself when the last of its windows
 * is closed. If no objects are registered, closing the last MDI window
 * results in MDI being destroyed. 
 **/
void gnome_mdi_register (GnomeMDI *mdi, GtkObject *object)
{
	if(!g_slist_find(mdi->registered, object))
		mdi->registered = g_slist_append(mdi->registered, object);
}

/**
 * gnome_mdi_unregister:
 * @mdi: A pointer to a GnomeMDI object.
 * @object: Object to unregister.
 * 
 * Description:
 * Removes GtkObject @object from the list of registered objects. 
 **/
void gnome_mdi_unregister (GnomeMDI *mdi, GtkObject *object)
{
	mdi->registered = g_slist_remove(mdi->registered, object);
}

/**
 * gnome_mdi_get_child_from_view:
 * @view: A pointer to a GtkWidget.
 * 
 * Description:
 * Returns a child that @view is a view of.
 * 
 * Return value:
 * A pointer to the GnomeMDIChild the view belongs to.
 **/
GnomeMDIChild *gnome_mdi_get_child_from_view(GtkWidget *view)
{
	return GNOME_MDI_CHILD(gtk_object_get_data(GTK_OBJECT(view), GNOME_MDI_CHILD_KEY));
}

/**
 * gnome_mdi_get_app_from_view:
 * @view: A pointer to a GtkWidget.
 * 
 * Description:
 * Returns the toplevel window for this view.
 * 
 * Return value:
 * A pointer to the GnomeApp containg the specified view.
 **/
GnomeApp *gnome_mdi_get_app_from_view(GtkWidget *view)
{
	return GNOME_APP(gtk_widget_get_toplevel(GTK_WIDGET(view)));
}

/**
 * gnome_mdi_get_view_from_window:
 * @mdi: A pointer to a GnomeMDI object.
 * @app: A pointer to a GnomeApp widget.
 * 
 * Description:
 * Returns the pointer to the view in the MDI toplevel window @app.
 * If the mode is set to %GNOME_MDI_NOTEBOOK, the view in the current
 * page is returned.
 * 
 * Return value: 
 * A pointer to a view.
 **/
GtkWidget *gnome_mdi_get_view_from_window (GnomeMDI *mdi, GnomeApp *app)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);
	g_return_val_if_fail(app != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_APP(app), NULL);

	if((mdi->mode == GNOME_MDI_TOPLEVEL) || (mdi->mode == GNOME_MDI_MODAL)) {
		return app->contents;
	} else if( (mdi->mode == GNOME_MDI_NOTEBOOK) &&
		   GTK_NOTEBOOK(app->contents)->cur_page) {
		int cur_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (app->contents));
		return gtk_notebook_get_nth_page (GTK_NOTEBOOK (app->contents), cur_page);
	} else {
		return NULL;
	}
}

/**
 * gnome_mdi_get_menubar_info:
 * @app: A pointer to a GnomeApp widget created by the MDI.
 * 
 * Return value: 
 * A GnomeUIInfo array used for menubar in @app if the menubar has been created with a template.
 * %NULL otherwise.
 **/
GnomeUIInfo *gnome_mdi_get_menubar_info (GnomeApp *app)
{
	return gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_MENUBAR_INFO_KEY);
}

/**
 * gnome_mdi_get_toolbar_info:
 * @app: A pointer to a GnomeApp widget created by the MDI.
 * 
 * Return value: 
 * A GnomeUIInfo array used for toolbar in @app if the toolbar has been created with a template.
 * %NULL otherwise.
 **/
GnomeUIInfo *gnome_mdi_get_toolbar_info (GnomeApp *app)
{
	return gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_TOOLBAR_INFO_KEY);
}

/**
 * gnome_mdi_get_child_menu_info:
 * @app: A pointer to a GnomeApp widget created by the MDI.
 * 
 * Return value: 
 * A GnomeUIInfo array used for child's menus in @app if they have been created with a template.
 * %NULL otherwise.
 **/
GnomeUIInfo *gnome_mdi_get_child_menu_info (GnomeApp *app)
{
	return gtk_object_get_data(GTK_OBJECT(app), GNOME_MDI_CHILD_MENU_INFO_KEY);
}

#endif /* GNOME_DISABLE_DEPRECATED_SOURCE */
