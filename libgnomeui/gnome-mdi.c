/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-mdi.c - implementation of the GnomeMDI object

   Copyright (C) 1997, 1998 Free Software Foundation

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

   see gnome-libs/gnome-hello/gnome-hello-7-mdi.c or gnome-utils/ghex for an example
   of its use & gnome-mdi.h for a short explanation of the signals
*/

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <config.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-util.h"
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-stock.h"
#include "gnome-preferences.h"
#include "gnome-mdi.h"
#include "gnome-mdi-child.h"

#include <unistd.h>
#include <stdio.h>

static void             gnome_mdi_class_init     (GnomeMDIClass  *);
static void             gnome_mdi_init           (GnomeMDI *);
static void             gnome_mdi_destroy        (GtkObject *);
static void             set_active_view          (GnomeMDI *, GtkWidget *);

static GtkWidget       *find_item_by_label       (GtkMenuShell *, gchar *);

static void             child_list_menu_create     (GnomeMDI *, GnomeApp *);
static void             child_list_activated_cb    (GtkWidget *, GnomeMDI *);
void                    child_list_menu_remove_item(GnomeMDI *, GnomeMDIChild *);
void                    child_list_menu_add_item   (GnomeMDI *, GnomeMDIChild *);

static void             app_create               (GnomeMDI *);
static void             app_destroy              (GnomeApp *);
static void             app_set_view             (GnomeMDI *, GnomeApp *, GtkWidget *);

static gint             app_close_top            (GnomeApp *, GdkEventAny *, GnomeMDI *);
static gint             app_close_book           (GnomeApp *, GdkEventAny *, GnomeMDI *);

static void             set_page_by_widget       (GtkNotebook *, GtkWidget *);
static GtkNotebookPage *find_page_by_widget      (GtkNotebook *, GtkWidget *);

static GtkWidget       *book_create              (GnomeMDI *);
static void             book_switch_page         (GtkNotebook *, GtkNotebookPage *,
                                                  gint, GnomeMDI *);
static gint             book_motion              (GtkWidget *widget, GdkEventMotion *e,
												  gpointer data);
static gint             book_button_press        (GtkWidget *widget, GdkEventButton *e,
												  gpointer data);
static gint             book_button_release      (GtkWidget *widget, GdkEventButton *e,
												  gpointer data);
static void             book_add_view            (GtkNotebook *, GtkWidget *);

static void             toplevel_focus           (GnomeApp *, GdkEventFocus *, GnomeMDI *);

static void             top_add_view             (GnomeMDI *, GnomeMDIChild *, GtkWidget *);

static GnomeUIInfo     *copy_ui_info_tree        (GnomeUIInfo *);
static void             free_ui_info_tree        (GnomeUIInfo *);
static gint             count_ui_info_items      (GnomeUIInfo *);

/* keys for stuff that we'll assign to our GnomeApps */
#define MDI_CHILD_KEY              "GnomeMDIChild"
#define ITEM_COUNT_KEY             "MDIChildMenuItems"

static GdkCursor *drag_cursor = NULL;

enum {
	CREATE_MENUS,
	CREATE_TOOLBAR,
	ADD_CHILD,
	REMOVE_CHILD,
	ADD_VIEW,
	REMOVE_VIEW,
	CHILD_CHANGED,
	VIEW_CHANGED,
	APP_CREATED,
	LAST_SIGNAL
};

typedef GtkWidget *(*GnomeMDISignal1) (GtkObject *, gpointer, gpointer);
typedef gboolean   (*GnomeMDISignal2) (GtkObject *, gpointer, gpointer);
typedef void       (*GnomeMDISignal3) (GtkObject *, gpointer, gpointer);

static gint mdi_signals[LAST_SIGNAL];

static GtkObjectClass *parent_class = NULL;

static void gnome_mdi_marshal_1 (GtkObject	    *object,
								 GtkSignalFunc       func,
								 gpointer	     func_data,
								 GtkArg	            *args) {
	GnomeMDISignal1 rfunc;
	gpointer *return_val;
	
	rfunc = (GnomeMDISignal1) func;
	return_val = GTK_RETLOC_POINTER (args[1]);
	
	*return_val = (* rfunc)(object, GTK_VALUE_POINTER(args[0]), func_data);
}

static void gnome_mdi_marshal_2 (GtkObject	    *object,
								 GtkSignalFunc       func,
								 gpointer	     func_data,
								 GtkArg	            *args) {
	GnomeMDISignal2 rfunc;
	gint *return_val;
	
	rfunc = (GnomeMDISignal2) func;
	return_val = GTK_RETLOC_BOOL (args[1]);
	
	*return_val = (* rfunc)(object, GTK_VALUE_POINTER(args[0]), func_data);
}

static void gnome_mdi_marshal_3 (GtkObject	    *object,
								 GtkSignalFunc       func,
								 gpointer	     func_data,
								 GtkArg	            *args) {
	GnomeMDISignal3 rfunc;
	
	rfunc = (GnomeMDISignal3) func;
	
	(* rfunc)(object, GTK_VALUE_POINTER(args[0]), func_data);
}


guint gnome_mdi_get_type () {
	static guint mdi_type = 0;
	
	if (!mdi_type) {
		GtkTypeInfo mdi_info = {
			"GnomeMDI",
			sizeof (GnomeMDI),
			sizeof (GnomeMDIClass),
			(GtkClassInitFunc) gnome_mdi_class_init,
			(GtkObjectInitFunc) gnome_mdi_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};
		
		mdi_type = gtk_type_unique (gtk_object_get_type (), &mdi_info);
	}
	
	return mdi_type;
}

static void gnome_mdi_class_init (GnomeMDIClass *class) {
	GtkObjectClass *object_class;
	
	object_class = (GtkObjectClass*) class;
	
	object_class->destroy = gnome_mdi_destroy;
	
	mdi_signals[CREATE_MENUS] = gtk_signal_new ("create_menus",
												GTK_RUN_LAST,
												object_class->type,
												GTK_SIGNAL_OFFSET (GnomeMDIClass, create_menus),
												gnome_mdi_marshal_1,
												gtk_widget_get_type(), 1, gnome_app_get_type());
	mdi_signals[CREATE_TOOLBAR] = gtk_signal_new ("create_toolbar",
												  GTK_RUN_LAST,
												  object_class->type,
												  GTK_SIGNAL_OFFSET (GnomeMDIClass, create_toolbar),
												  gnome_mdi_marshal_1,
												  gtk_widget_get_type(), 1, gnome_app_get_type());
	mdi_signals[ADD_CHILD] = gtk_signal_new ("add_child",
											 GTK_RUN_LAST,
											 object_class->type,
											 GTK_SIGNAL_OFFSET (GnomeMDIClass, add_child),
											 gnome_mdi_marshal_2,
											 GTK_TYPE_BOOL, 1, gnome_mdi_child_get_type());
	mdi_signals[REMOVE_CHILD] = gtk_signal_new ("remove_child",
												GTK_RUN_LAST,
												object_class->type,
												GTK_SIGNAL_OFFSET (GnomeMDIClass, remove_child),
												gnome_mdi_marshal_2,
												GTK_TYPE_BOOL, 1, gnome_mdi_child_get_type());
	mdi_signals[ADD_VIEW] = gtk_signal_new ("add_view",
											GTK_RUN_LAST,
											object_class->type,
											GTK_SIGNAL_OFFSET (GnomeMDIClass, add_view),
											gnome_mdi_marshal_2,
											GTK_TYPE_BOOL, 1, gtk_widget_get_type());
	mdi_signals[REMOVE_VIEW] = gtk_signal_new ("remove_view",
											   GTK_RUN_LAST,
											   object_class->type,
											   GTK_SIGNAL_OFFSET (GnomeMDIClass, remove_view),
											   gnome_mdi_marshal_2,
											   GTK_TYPE_BOOL, 1, gtk_widget_get_type());
	mdi_signals[CHILD_CHANGED] = gtk_signal_new ("child_changed",
												 GTK_RUN_LAST,
												 object_class->type,
												 GTK_SIGNAL_OFFSET (GnomeMDIClass, child_changed),
												 gnome_mdi_marshal_3,
												 GTK_TYPE_NONE, 1, gnome_mdi_child_get_type());
	mdi_signals[VIEW_CHANGED] = gtk_signal_new ("view_changed",
												GTK_RUN_LAST,
												object_class->type,
												GTK_SIGNAL_OFFSET(GnomeMDIClass, view_changed),
												gnome_mdi_marshal_3,
												GTK_TYPE_NONE, 1, gtk_widget_get_type());
	mdi_signals[APP_CREATED] = gtk_signal_new ("app_created",
											   GTK_RUN_LAST,
											   object_class->type,
											   GTK_SIGNAL_OFFSET (GnomeMDIClass, app_created),
											   gnome_mdi_marshal_3,
											   GTK_TYPE_NONE, 1, gnome_app_get_type());
	
	gtk_object_class_add_signals (object_class, mdi_signals, LAST_SIGNAL);
	
	class->create_menus = NULL;
	class->create_toolbar = NULL;
	class->add_child = NULL;
	class->remove_child = NULL;
	class->add_view = NULL;
	class->remove_view = NULL;
	class->child_changed = NULL;
	class->view_changed = NULL;
	class->app_created = NULL;
	
	parent_class = gtk_type_class (gtk_object_get_type ());
}

static void gnome_mdi_destroy(GtkObject *object) {
	GnomeMDI *mdi;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_MDI (object));
	
	mdi = GNOME_MDI (object);

	if(mdi->children)
		g_warning("GnomeMDI: Destroying while children still exist!");
	
	if(mdi->child_menu_path)
		g_free(mdi->child_menu_path);
	if(mdi->child_list_path)
		g_free(mdi->child_list_path);
	
	g_free (mdi->appname);
	g_free (mdi->title);
	
	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

static void gnome_mdi_init (GnomeMDI *mdi) {
	mdi->mode = gnome_preferences_get_mdi_mode();
	mdi->tab_pos = gnome_preferences_get_mdi_tab_pos();
	
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

GtkObject *gnome_mdi_new(gchar *appname, gchar *title) {
	GnomeMDI *mdi;
	
	mdi = gtk_type_new (gnome_mdi_get_type ());
  
	mdi->appname = g_strdup(appname);
	mdi->title = g_strdup(title);

	return GTK_OBJECT (mdi);
}

/* the app-helper support routines */
static GnomeUIInfo *copy_ui_info_tree(GnomeUIInfo source[]) {
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
			(source[i].type == GNOME_APP_UI_RADIOITEMS) )
			copy[i].moreinfo = copy_ui_info_tree(source[i].moreinfo);
	}
	
	return copy;
}

static gint count_ui_info_items(GnomeUIInfo *ui_info) {
	gint num;
	
	for(num = 0; ui_info[num].type != GNOME_APP_UI_ENDOFINFO; num++)
		;
	
	return num;
}

static void free_ui_info_tree(GnomeUIInfo *root) {
	int count;
	
	for(count = 0; root[count].type != GNOME_APP_UI_ENDOFINFO; count++)
		if( (root[count].type == GNOME_APP_UI_SUBTREE) ||
			(root[count].type == GNOME_APP_UI_RADIOITEMS) )
			free_ui_info_tree(root[count].moreinfo);
	
	g_free(root);
}

static void set_page_by_widget(GtkNotebook *book, GtkWidget *child) {
	GList *page_node;
	gint i = 0;
	
	page_node = book->children;
	while(page_node) {
		if( ((GtkNotebookPage *)page_node->data)->child == child ) {
			gtk_notebook_set_page(book, i);
			return;
		}
		
		i++;
		page_node = g_list_next(page_node);
	}
	
	return;
}

static GtkNotebookPage *find_page_by_widget(GtkNotebook *book, GtkWidget *child) {
	GList *page_node;
	
	page_node = book->children;
	while(page_node) {
		if( ((GtkNotebookPage *)page_node->data)->child == child )
			return ((GtkNotebookPage *)page_node->data);
		
		page_node = g_list_next(page_node);
	}
	
	return NULL;
}

static GtkWidget *find_item_by_label(GtkMenuShell *shell, gchar *label) {
	GList *child;
	GtkWidget *grandchild;
	
	child = shell->children;
	while(child) {
		grandchild = GTK_BIN(child->data)->child;
		if( grandchild &&
			(GTK_IS_LABEL(grandchild)) &&
			(strcmp(GTK_LABEL(grandchild)->label, label) == 0) )
			return GTK_WIDGET(child->data);
		
		child = g_list_next(child);
	}
	
	return NULL;
}

static void child_list_activated_cb(GtkWidget *w, GnomeMDI *mdi) {
	GnomeMDIChild *child;
	
	child = gtk_object_get_data(GTK_OBJECT(w), MDI_CHILD_KEY);
	
	if( child && (child != mdi->active_child) ) {
		if(child->views)
			gnome_mdi_set_active_view(mdi, child->views->data);
		else
			gnome_mdi_add_view(mdi, child);
	}
}

static void child_list_menu_create(GnomeMDI *mdi, GnomeApp *app) {
	GtkWidget *submenu, *item;
	GList *child;
	gint pos;
	
	if(mdi->child_list_path == NULL)
		return;
	
	submenu = gnome_app_find_menu_pos(app->menubar, mdi->child_list_path, &pos);
	
	child = mdi->children;
	while(child) {
		item = gtk_menu_item_new_with_label(GNOME_MDI_CHILD(child->data)->name);
		gtk_signal_connect(GTK_OBJECT(item), "activate",
						   GTK_SIGNAL_FUNC(child_list_activated_cb), mdi);
		gtk_object_set_data(GTK_OBJECT(item), MDI_CHILD_KEY, child->data);
		gtk_widget_show(item);
		
		gtk_menu_shell_insert(GTK_MENU_SHELL(submenu), item, pos);
		
		child = g_list_next(child);
	}
	
	gtk_widget_queue_resize(submenu);
}

void child_list_menu_remove_item(GnomeMDI *mdi, GnomeMDIChild *child) {
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
			item = find_item_by_label(GTK_MENU_SHELL(shell), child->name);
			gtk_container_remove(GTK_CONTAINER(shell), item);
			gtk_widget_queue_resize (GTK_WIDGET (shell));
		}
		
		app_node = g_list_next(app_node);
	}
}

void child_list_menu_add_item(GnomeMDI *mdi, GnomeMDIChild *child) {
	GtkWidget *item, *submenu;
	GnomeApp *app;
	GList *app_node;
	gint pos;
	
	if(mdi->child_list_path == NULL)
		return;
	
	app_node = mdi->windows;
	while(app_node) {
		app = GNOME_APP(app_node->data);
		
		item = gtk_menu_item_new_with_label(child->name);
		gtk_signal_connect(GTK_OBJECT(item), "activate",
						   GTK_SIGNAL_FUNC(child_list_activated_cb), mdi);
		gtk_object_set_data(GTK_OBJECT(item), MDI_CHILD_KEY, child);
		gtk_widget_show(item);
		
		submenu = gnome_app_find_menu_pos(app->menubar, mdi->child_list_path, &pos);
		gtk_menu_shell_insert(GTK_MENU_SHELL(submenu), item, pos);
		gtk_widget_queue_resize(submenu);
		
		app_node = g_list_next(app_node);
	}
}

static gint book_motion(GtkWidget *widget, GdkEventMotion *e, gpointer data) {
	GnomeMDI *mdi;
	guint32 time;

	mdi = GNOME_MDI(data);

	if(!drag_cursor)
		drag_cursor = gdk_cursor_new(GDK_HAND2);

	time = gdk_event_get_time((GdkEvent *)e);

	if(e->window == widget->window) {
		mdi->in_drag = TRUE;
		gtk_grab_add(widget);
		gdk_pointer_grab(widget->window, FALSE,
						 GDK_POINTER_MOTION_MASK |
						 GDK_BUTTON_RELEASE_MASK, NULL,
						 drag_cursor, time);
		if(mdi->signal_id) {
			gtk_signal_disconnect(GTK_OBJECT(widget), mdi->signal_id);
			mdi->signal_id = 0;
		}
	}

	return FALSE;
}

static gint book_button_press(GtkWidget *widget, GdkEventButton *e, gpointer data) {
	GnomeMDI *mdi;

	mdi = GNOME_MDI(data);

	if(e->button == 1 && e->window == widget->window)
		mdi->signal_id = gtk_signal_connect(GTK_OBJECT(widget), "motion_notify_event",
											GTK_SIGNAL_FUNC(book_motion), mdi);

	return FALSE;
}

static gint book_button_release(GtkWidget *widget, GdkEventButton *e, gpointer data) {
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
		gdk_pointer_ungrab(e->time);
		gtk_grab_remove(widget);

		window = gdk_window_at_pointer(&x, &y);
		if(window)
			window = gdk_window_get_toplevel(window);

		child = mdi->windows;
		while(child) {
			if(window == GTK_WIDGET(child->data)->window) {
				/* page was dragged to another notebook */

				old_book = GTK_NOTEBOOK(widget);
				new_book = GNOME_APP(child->data)->contents;

				if(old_book == (GtkNotebook *)new_book) /* page has been dropped on the source notebook */
					return FALSE;
	
				if(old_book->cur_page) {
					view = old_book->cur_page->child;
					gtk_container_remove(GTK_CONTAINER(old_book), view);
		
					book_add_view(GTK_NOTEBOOK(new_book), view);
		
					if(old_book->cur_page == NULL) {
						app = GNOME_APP(GTK_WIDGET(old_book)->parent->parent);
						mdi->windows = g_list_remove(mdi->windows, app);
						gtk_widget_destroy(GTK_WIDGET(app));
					}
				}

				return FALSE;
			}
				
			child = child->next;
		}

		/* they want us to create a new toplevel! */
		if(g_list_length(old_book->children) == 1)
			return FALSE;
			
		if(old_book->cur_page) {
			gint width, height;
				
			view = old_book->cur_page->child;
	
			width = view->allocation.width;
			height = view->allocation.height;
			
			gtk_container_remove(GTK_CONTAINER(old_book), view);
				
			app_create(mdi);
				
			new_book = book_create(mdi);
	
			book_add_view(GTK_NOTEBOOK(new_book), view);
				
			gtk_window_position(GTK_WINDOW(mdi->active_window), GTK_WIN_POS_MOUSE);
	
			gtk_widget_set_usize (view, width, height);
	
			gtk_widget_show(GTK_WIDGET(mdi->active_window));
		}
	}

	return FALSE;
}


static GtkWidget *book_create(GnomeMDI *mdi) {
	GtkWidget *us, *rw;
	
	us = gtk_notebook_new();

	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(us), mdi->tab_pos);
	
	gnome_app_set_contents(mdi->active_window, us);
	
	gtk_widget_realize(us);
	
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

static void book_add_view(GtkNotebook *book, GtkWidget *view) {
	GnomeMDIChild *child;
	GtkWidget *title;

	child = gnome_mdi_get_child_from_view(view);

	gtk_signal_emit_by_name(GTK_OBJECT(child), "set_book_label", NULL, &title);

	gtk_notebook_append_page(book, view, title);

	set_page_by_widget(book, view);  
}

static void book_switch_page(GtkNotebook *book, GtkNotebookPage *page, gint page_num, GnomeMDI *mdi) {
	GnomeApp *app;
#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: switching pages");
#endif
	app = GNOME_APP(gtk_widget_get_toplevel(GTK_WIDGET(book)));
	
	if(page_num != -1)
		app_set_view(mdi, app, page->child);
	else
		app_set_view(mdi, app, NULL);  
}

static void toplevel_focus(GnomeApp *app, GdkEventFocus *event, GnomeMDI *mdi) {
	/* updates active_view and active_child when a new toplevel receives focus */
	g_return_if_fail(GNOME_IS_APP(app));
#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: toplevel receiving focus");
#endif
	mdi->active_window = app;
	
	if((mdi->mode == GNOME_MDI_TOPLEVEL) || (mdi->mode == GNOME_MDI_MODAL))
		set_active_view(mdi, app->contents);
	else if((mdi->mode == GNOME_MDI_NOTEBOOK) && GTK_NOTEBOOK(app->contents)->cur_page)
		set_active_view(mdi, GTK_NOTEBOOK(app->contents)->cur_page->child);
	else
		set_active_view(mdi, NULL);
}

static gint app_close_top(GnomeApp *app, GdkEventAny *event, GnomeMDI *mdi) {
	GnomeMDIChild *child = NULL;
	GList *child_node;
	gint handler_ret = TRUE;
	
	if(g_list_length(mdi->windows) == 1) {
		/* since this is the last window, we only close it and destroy the MDI if
		   ALL the remaining children can be closed, which we check in advance */
		child_node = mdi->children;
		while(child_node) {
			gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_CHILD], child_node->data, &handler_ret);
			
			if(handler_ret == FALSE)
				return TRUE;
			
			child_node = g_list_next(child_node);
		}
		
		gnome_mdi_remove_all(mdi, TRUE);
		mdi->windows = g_list_remove(mdi->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));
		
		/* only destroy mdi if there are no external windows registered with it. */
		if(mdi->registered == NULL)
			gtk_object_destroy(GTK_OBJECT(mdi));
	}
	else if(app->contents) {
		child = gnome_mdi_get_child_from_view(app->contents);
		if(g_list_length(child->views) == 1) {
			/* if this is the last view, we should remove the child! */
			if(!gnome_mdi_remove_child(mdi, child, FALSE))
				return TRUE;
		}
		else
			gnome_mdi_remove_view(mdi, app->contents, FALSE);
	}
	
	return FALSE;
}

static gint app_close_book(GnomeApp *app, GdkEventAny *event, GnomeMDI *mdi) {
	GnomeMDIChild *child;
	GtkWidget *view;
	GList *page_node, *node;
	gint handler_ret = TRUE;
	
	if(g_list_length(mdi->windows) == 1) {
		node = mdi->children;
		while(node) {
			gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_CHILD], node->data, &handler_ret);
			
			if(handler_ret == FALSE)
				return TRUE;
			
			node = g_list_next(node);
		}
		
		gnome_mdi_remove_all(mdi, TRUE);
		mdi->windows = g_list_remove(mdi->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));
		
		/* only destroy mdi if there are no non-MDI windows registered with it. */
		if(mdi->registered == NULL)
			gtk_object_destroy(GTK_OBJECT(mdi));
	}
	else {
		/* first check if all the children in this notebook can be removed */
		page_node = GTK_NOTEBOOK(app->contents)->children;
		while(page_node) {
			view = ((GtkNotebookPage *)page_node->data)->child;
			child = gnome_mdi_get_child_from_view(view);
			
			page_node = g_list_next(page_node);
			
			node = child->views;
			while(node) {
				if(gnome_mdi_get_app_from_view(node->data) != app)
					break;
				
				node = g_list_next(node);
			}
			
			if(node == NULL) {   /* all the views reside in this GnomeApp */
				gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_CHILD], child, &handler_ret);
				if(handler_ret == FALSE)
					return TRUE;
			}
		}
		
		/* now actually remove all children/views! */
		page_node = GTK_NOTEBOOK(app->contents)->children;
		while(page_node) {
			view = ((GtkNotebookPage *)page_node->data)->child;
			child = gnome_mdi_get_child_from_view(view);
			
			page_node = g_list_next(page_node);
			
			/* if this is the last view, remove the child */
			if(g_list_length(child->views) == 1)
				gnome_mdi_remove_child(mdi, child, TRUE);
			else
				gnome_mdi_remove_view(mdi, view, TRUE);
		}
	}
	
	return FALSE;
}

static void app_set_view(GnomeMDI *mdi, GnomeApp *app, GtkWidget *view) {
	GList *menu_list = NULL, *children;
	GtkWidget *parent;
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
	
	/* remove old child-specific menus */
	items = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(app), ITEM_COUNT_KEY));
	if( items > 0 ) {
		parent = gnome_app_find_menu_pos(app->menubar, mdi->child_menu_path, &pos);
		if(parent != NULL) {
			/* remove items */
			children = g_list_nth(GTK_MENU_SHELL(parent)->children, pos);
			while(children && items > 0) {
				gtk_container_remove(GTK_CONTAINER(parent), GTK_WIDGET(children->data));
				children = g_list_nth(GTK_MENU_SHELL(parent)->children, pos);
				items--;
			}
		}
		
		gtk_widget_queue_resize(parent);
	}
	
	if(view) {
		child = gnome_mdi_get_child_from_view(view);
		
		/* set the title */
		if( (mdi->mode == GNOME_MDI_MODAL) || (mdi->mode == GNOME_MDI_TOPLEVEL) ) {
			/* in MODAL and TOPLEVEL modes the window title includes the active
			   child name: "child_name - mdi_title" */
			gchar *fullname;
			
			fullname = g_copy_strings(child->name, " - ", mdi->title, NULL);
			gtk_window_set_title(GTK_WINDOW(app), fullname);
			g_free(fullname);
		}
		else
			gtk_window_set_title(GTK_WINDOW(app), mdi->title);
		
		/* create new child-specific menus */
		if( child->menu_template &&
			( (ui_info = copy_ui_info_tree(child->menu_template)) != NULL) ) {
			gnome_app_insert_menus_with_data(app, mdi->child_menu_path, ui_info, child);
			gtk_object_set_data(GTK_OBJECT(app), GNOME_MDI_CHILD_MENU_INFO_KEY, ui_info);
			gtk_object_set_data(GTK_OBJECT(app), ITEM_COUNT_KEY, GINT_TO_POINTER(count_ui_info_items(ui_info)));
		}
		else {
			gtk_signal_emit_by_name(GTK_OBJECT(child), "create_menus", view, &menu_list);
			
			if(menu_list) {
				parent = gnome_app_find_menu_pos(app->menubar, mdi->child_menu_path, &pos);
				
				if(parent) {
					items = 0;
					
					while(menu_list) {
						gtk_menu_shell_insert(GTK_MENU_SHELL(parent), GTK_WIDGET(menu_list->data), pos);
						menu_list = g_list_remove(menu_list, menu_list->data);
						pos++;
						items++;
					}
					
					gtk_object_set_data(GTK_OBJECT(app), ITEM_COUNT_KEY, GINT_TO_POINTER(items));
					
					gtk_widget_queue_resize(parent);
				}
			}
		}
	}
	else {
		gtk_window_set_title(GTK_WINDOW(app), mdi->title);
		gtk_object_set_data(GTK_OBJECT(app), ITEM_COUNT_KEY, NULL);
	}
	
	set_active_view(mdi, view);
}

static void app_destroy(GnomeApp *app) {
	GnomeUIInfo *ui_info;
	
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

static void app_create(GnomeMDI *mdi) {
	GtkWidget *window;
	GtkMenuBar *menubar = NULL;
	GtkToolbar *toolbar = NULL;
	GtkSignalFunc func = NULL;
	GnomeUIInfo *ui_info;
	
	window = gnome_app_new(mdi->appname, mdi->title);
	
#if 0
	/* is this really necessary? */
	gtk_window_set_wmclass (GTK_WINDOW (window), mdi->appname, mdi->appname);
#endif
  
	gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, FALSE);
  
	gtk_widget_realize(window);

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
					   GTK_SIGNAL_FUNC(app_destroy), NULL);

	/* set up menus */
	if(mdi->menu_template) {
		ui_info = copy_ui_info_tree(mdi->menu_template);
		gnome_app_create_menus_with_data(GNOME_APP(window), ui_info, mdi);
		gtk_object_set_data(GTK_OBJECT(window), GNOME_MDI_MENUBAR_INFO_KEY, ui_info);
	}
	else {
		/* we use the (hopefully) supplied create_menus signal handler */
		gtk_signal_emit (GTK_OBJECT (mdi), mdi_signals[CREATE_MENUS], window, &menubar);
		
		if(menubar) {
			gtk_widget_show(GTK_WIDGET(menubar));
			gnome_app_set_menus(GNOME_APP(window), GTK_MENU_BAR(menubar));
		}
	}
	
	child_list_menu_create(mdi, GNOME_APP(window));
	
	/* create toolbar */
	if(mdi->toolbar_template) {
		ui_info = copy_ui_info_tree(mdi->toolbar_template);
		gnome_app_create_toolbar_with_data(GNOME_APP(window), ui_info, mdi);
		gtk_object_set_data(GTK_OBJECT(window), GNOME_MDI_TOOLBAR_INFO_KEY, ui_info);
	}
	else {
		gtk_signal_emit(GTK_OBJECT (mdi), mdi_signals[CREATE_TOOLBAR], window, &toolbar);
		
		if(toolbar) {
			gtk_widget_show(GTK_WIDGET(toolbar));
			gnome_app_set_toolbar(GNOME_APP(window), toolbar);
		}
	}
	
	mdi->active_window = GNOME_APP(window);
	mdi->active_child = NULL;
	mdi->active_view = NULL;
	
	gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[APP_CREATED], window);
}

static void top_add_view(GnomeMDI *mdi, GnomeMDIChild *child, GtkWidget *view) {
	GnomeApp *window;
	
	if (mdi->active_window->contents != NULL)
		app_create(mdi);
	
	window = mdi->active_window;

	if(child && view)
		gnome_app_set_contents(window, view);

	app_set_view(mdi, window, view);

	if(!GTK_WIDGET_VISIBLE(window))
		gtk_widget_show(GTK_WIDGET(window));
}

static void set_active_view(GnomeMDI *mdi, GtkWidget *view) {
	GnomeMDIChild *child, *old_child;
	GtkWidget *old_view;

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomeMDI: set_active_view() called");
#endif

	if(view == mdi->active_view)
		return;

	if(view)
		child = gnome_mdi_get_child_from_view(view);
	else
		child = NULL;
	
	old_child = mdi->active_child;
	old_view = mdi->active_view;

	mdi->active_view = view;
	mdi->active_child = child;

	if(child != old_child)
		gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[CHILD_CHANGED], old_child);

	gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[VIEW_CHANGED], old_view);
}

void gnome_mdi_set_active_view(GnomeMDI *mdi, GtkWidget *view) {
	GtkWindow *window;
	
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));
	g_return_if_fail(view != NULL);
	g_return_if_fail(GTK_IS_WIDGET(view));
	
	window = GTK_WINDOW(gnome_mdi_get_app_from_view(view));
	
	if(mdi->mode == GNOME_MDI_NOTEBOOK)
		set_page_by_widget(GTK_NOTEBOOK(GNOME_APP(window)->contents), view);
  
	/* TODO: hmmm... I dont know how to give focus to the window, so that it would
	   receive keyboard events */
	gdk_window_raise(GTK_WIDGET(window)->window);
	gtk_window_set_focus(window, view);
	gtk_window_activate_focus(window);
}

gint gnome_mdi_add_view(GnomeMDI *mdi, GnomeMDIChild *child) {
	GtkWidget *view;
	gint ret = TRUE;
	
	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI_CHILD(child), FALSE);
	
	view = gnome_mdi_child_add_view(child);
	
	if(!view)
		return FALSE;

	gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[ADD_VIEW], view, &ret);

	if(ret == FALSE) {
		gnome_mdi_child_remove_view(child, view);
		return FALSE;
	}

	if(mdi->active_window == NULL) {
		app_create(mdi);
		gtk_widget_show(GTK_WIDGET(mdi->active_window));
	}

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
	
	gtk_widget_show(view);

	return TRUE;
}

gint gnome_mdi_add_toplevel_view(GnomeMDI *mdi, GnomeMDIChild *child) {
	GtkWidget *view;
	gint ret = TRUE;
	
	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI_CHILD(child), FALSE);
	
	view = gnome_mdi_child_add_view(child);

	if(!view)
		return FALSE;

	gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[ADD_VIEW], view, &ret);

	if(ret == FALSE) {
		gnome_mdi_child_remove_view(child, view);
		return FALSE;
	}

	if(mdi->active_window == NULL) {
		app_create(mdi);
		gtk_widget_show(GTK_WIDGET(mdi->active_window));
	}

	if(mdi->mode == GNOME_MDI_NOTEBOOK) {
		if(mdi->active_window->contents == NULL)
			book_create(mdi);
		else if(GTK_NOTEBOOK(mdi->active_window->contents)->cur_page) {
			app_create(mdi);
			book_create(mdi);
			gtk_widget_show(GTK_WIDGET(mdi->active_window));
		}
		/* add a new toplevel unless the current one is empty */
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
	
	gtk_widget_show(view);

	return TRUE;
}

gint gnome_mdi_remove_view(GnomeMDI *mdi, GtkWidget *view, gint force) {
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

	window = gnome_mdi_get_app_from_view(view);

	gtk_container_remove(GTK_CONTAINER(parent), view);

	if( (mdi->mode == GNOME_MDI_TOPLEVEL) || (mdi->mode == GNOME_MDI_MODAL) ) {
		window->contents = NULL;

		/* if this is NOT the last toplevel, we destroy it */
		if(g_list_length(mdi->windows) > 1) {
			mdi->windows = g_list_remove(mdi->windows, window);
			gtk_widget_destroy(GTK_WIDGET(window));
		}
		else
			app_set_view(mdi, window, NULL);
	}
	else if( (mdi->mode == GNOME_MDI_NOTEBOOK) &&
			 (GTK_NOTEBOOK(parent)->cur_page == NULL) ) {
		if(g_list_length(mdi->windows) > 1) {
			/* if this is NOT the last toplevel, destroy it! */
			mdi->windows = g_list_remove(mdi->windows, window);
			gtk_widget_destroy(GTK_WIDGET(window));
		}
		else
			app_set_view(mdi, window, NULL);
	}

	/* remove this view from the child's view list */
	gnome_mdi_child_remove_view(child, view);

	return TRUE;
}

gint gnome_mdi_add_child(GnomeMDI *mdi, GnomeMDIChild *child) {
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

	child_list_menu_add_item(mdi, child);

	return TRUE;
}

gint gnome_mdi_remove_child(GnomeMDI *mdi, GnomeMDIChild *child, gint force) {
	gint ret = TRUE;

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

	child->parent = NULL;

	while(child->views)
		gnome_mdi_remove_view(mdi, GTK_WIDGET(child->views->data), TRUE);

	mdi->children = g_list_remove(mdi->children, child);

	child_list_menu_remove_item(mdi, child);

	gtk_object_destroy(GTK_OBJECT(child));

	return TRUE;
}

gint gnome_mdi_remove_all(GnomeMDI *mdi, gint force) {
	GList *child_node;
	GnomeMDIChild *child;
	gint handler_ret = TRUE;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);

	/* first check if removal of any child will be prevented by the remove_child signal handler */
	if(!force) {
		child_node = mdi->children;
		while(child_node) {
			gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_CHILD], child_node->data, &handler_ret);

			/* if any of the children may not be removed, none will be */
			if(handler_ret == FALSE)
				return FALSE;

			child_node = g_list_next(child_node);
		}
	}

	/* remove all the children with force arg set to true so that remove_child handlers
	   are not called again */
	while(mdi->children) {
		child = GNOME_MDI_CHILD(mdi->children->data);
		mdi->children = g_list_next(mdi->children);
		gnome_mdi_remove_child(mdi, child, TRUE);
	}

	return TRUE;
}

void gnome_mdi_open_toplevel(GnomeMDI *mdi) {
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	if( mdi->mode != GNOME_MDI_MODAL || mdi->windows == NULL ) {
		app_create(mdi);
		if(mdi->mode == GNOME_MDI_NOTEBOOK)
			book_create(mdi);

		gtk_widget_show(GTK_WIDGET(mdi->active_window));
	}
}

void gnome_mdi_update_child(GnomeMDI *mdi, GnomeMDIChild *child) {
	GtkWidget *view, *title;
	GList *view_node;

	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));
	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_MDI_CHILD(child));

	view_node = child->views;
	while(view_node) {
		view = GTK_WIDGET(view_node->data);

		/* for the time being all that update_child() does is update the children's names */
		if( (mdi->mode == GNOME_MDI_MODAL) || (mdi->mode == GNOME_MDI_TOPLEVEL) ) {
			/* in MODAL and TOPLEVEL modes the window title includes the active
			   child name: "child_name - mdi_title" */
			gchar *fullname;
      
			fullname = g_copy_strings(child->name, " - ", mdi->title, NULL);
			gtk_window_set_title(GTK_WINDOW(gnome_mdi_get_app_from_view(view)), fullname);
			g_free(fullname);
		}
		else if(mdi->mode == GNOME_MDI_NOTEBOOK) {
			GtkNotebookPage *page;

			page = find_page_by_widget(GTK_NOTEBOOK(view->parent), view);
			if(page)
				gtk_signal_emit_by_name(GTK_OBJECT(child), "set_book_label", page->tab_label, &title);
		}
		
		view_node = g_list_next(view_node);
	}
}

GnomeMDIChild *gnome_mdi_find_child(GnomeMDI *mdi, gchar *name) {
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

void gnome_mdi_set_mode(GnomeMDI *mdi, GnomeMDIMode mode) {
	GtkWidget *view;
	GnomeMDIChild *child;
	GList *child_node, *view_node;
	gint width = -1, height = -1;

	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	if(mode == GNOME_MDI_DEFAULT_MODE)
		mode = gnome_preferences_get_mdi_mode();

	if(mdi->windows == NULL)
		gnome_mdi_open_toplevel(mdi);

	if(mode == GNOME_MDI_REDRAW)
		mode = mdi->mode;
	else if(mdi->mode == mode)
		return;

	/* Get current width and height. */
	if (mdi->active_view) {
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

			if( (mdi->mode == GNOME_MDI_TOPLEVEL) || (mdi->mode == GNOME_MDI_MODAL) )
				gnome_mdi_get_app_from_view(view)->contents = NULL;

			gtk_container_remove(GTK_CONTAINER(view->parent), view);

			view_node = g_list_next(view_node);

			/* if we are to change mode to MODAL, destroy all views except the active one */
			if( (mode == GNOME_MDI_MODAL) && (view != mdi->active_view) )
				gnome_mdi_child_remove_view(child, view);
		}
		child_node = g_list_next(child_node);
	}

	/* remove all GnomeApps */
	while(mdi->windows) {
		gtk_widget_destroy(GTK_WIDGET(mdi->windows->data));
		mdi->windows = g_list_remove(mdi->windows, mdi->windows->data);
	}

	mdi->active_window = NULL;

	mdi->mode = mode;

	app_create(mdi);

	if(mdi->mode == GNOME_MDI_NOTEBOOK)
		book_create(mdi);

	/* re-implant views in proper containers */
	child_node = mdi->children;
	while(child_node) {
		child = GNOME_MDI_CHILD(child_node->data);
		view_node = child->views;
		while(view_node) {
			view = GTK_WIDGET(view_node->data);

			if ((width != -1) && (height != -1))
				gtk_widget_set_usize (view, width, height);

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

			view_node = g_list_next(view_node);
		}
		child_node = g_list_next(child_node);
	}

	if(!GTK_WIDGET_VISIBLE(mdi->active_window))
		gtk_widget_show(GTK_WIDGET(mdi->active_window));
}

GnomeMDIChild *gnome_mdi_active_child(GnomeMDI *mdi) {
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);

	if(mdi->active_view)
		return(gnome_mdi_get_child_from_view(mdi->active_view));

	return NULL;
}

GtkWidget *gnome_mdi_active_view(GnomeMDI *mdi) {
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);

	return mdi->active_view;
}

GnomeApp *gnome_mdi_active_window(GnomeMDI *mdi) {
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);

	return mdi->active_window;
}

void gnome_mdi_set_menu_template(GnomeMDI *mdi, GnomeUIInfo *menu_tmpl) {
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	mdi->menu_template = menu_tmpl;
}

void gnome_mdi_set_toolbar_template(GnomeMDI *mdi, GnomeUIInfo *tbar_tmpl) {
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	mdi->toolbar_template = tbar_tmpl;
}

void gnome_mdi_set_child_menu_path(GnomeMDI *mdi, gchar *path) {
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	if(mdi->child_menu_path)
		g_free(mdi->child_menu_path);

	mdi->child_menu_path = g_strdup(path);
}

void gnome_mdi_set_child_list_path(GnomeMDI *mdi, gchar *path) {
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	if(mdi->child_list_path)
		g_free(mdi->child_list_path);

	mdi->child_list_path = g_strdup(path);
}

void gnome_mdi_register(GnomeMDI *mdi, GtkObject *o) {
	if(!g_slist_find(mdi->registered, o))
		mdi->registered = g_slist_append(mdi->registered, o);
}

void gnome_mdi_unregister(GnomeMDI *mdi, GtkObject *o) {
	mdi->registered = g_slist_remove(mdi->registered, o);
}

GnomeMDIChild *gnome_mdi_get_child_from_view(GtkWidget *view) {
	return GNOME_MDI_CHILD(gtk_object_get_data(GTK_OBJECT(view), MDI_CHILD_KEY));
}

GnomeApp *gnome_mdi_get_app_from_view(GtkWidget *view) {
	return GNOME_APP(gtk_widget_get_toplevel(GTK_WIDGET(view)));
}

GtkWidget *gnome_mdi_get_view_from_window(GnomeMDI *mdi, GnomeApp *app) {
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_MDI(mdi), NULL);
	g_return_val_if_fail(app != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_APP(app), NULL);

	if((mdi->mode == GNOME_MDI_TOPLEVEL) || (mdi->mode == GNOME_MDI_MODAL))
		return app->contents;
	else if((mdi->mode == GNOME_MDI_NOTEBOOK) && GTK_NOTEBOOK(app->contents)->cur_page)
		return GTK_NOTEBOOK(app->contents)->cur_page->child;
	else
		return NULL;
}

void gnome_mdi_set_tab_pos(GnomeMDI *mdi, GtkPositionType pos) {
	GList *app_node;

	g_return_if_fail(mdi != NULL);
	g_return_if_fail(GNOME_IS_MDI(mdi));

	if(mdi->tab_pos == pos)
		return;

	mdi->tab_pos = pos;

	if(mdi->mode != GNOME_MDI_NOTEBOOK)
		return;

	app_node = mdi->windows;
	while(app_node) {
		gtk_notebook_set_tab_pos(GTK_NOTEBOOK(GNOME_APP(app_node->data)->contents), pos);
		app_node = g_list_next(app_node);
	}
}

