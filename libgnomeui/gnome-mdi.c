/*
 * gnome-mdi.c - the GnomeMDI object
 * written by Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
 *
 * see gnome-libs/gnome-hello/gnome-hello-7-mdi.c or gnome-utils/ghex for an example
 * of its use & gnome-mdi.h for a short
 * explanation of the signals
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#if 0
#include <gnome-mdi-pouch.h>
#include <gnome-mdi-roo.h>
#endif

#include <config.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-util.h"
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-stock.h"
#include "gnome-mdi.h"
#include "gnome-mdi-child.h"

static void             gnome_mdi_class_init     (GnomeMDIClass  *);
static void             gnome_mdi_init           (GnomeMDI *);
static void             gnome_mdi_destroy        (GtkObject *);
static GnomeMDIChild   *gnome_mdi_find_child     (GnomeMDI *, gchar *);

static GtkWidget       *find_item_by_label       (GtkMenuShell *, gchar *);

static GtkWidget       *child_list_menu_create     (GnomeMDI *);
static void             child_list_menu_remove_item(GnomeMDI *, GnomeMDIChild *);
static void             child_list_menu_add_item   (GnomeMDI *, GnomeMDIChild *);
static void             child_list_activated_cb    (GtkWidget *, GnomeMDI *);

static void             app_create               (GnomeMDI *);
static void             app_destroy              (GnomeApp *);
static void             app_set_title            (GnomeMDI *, GnomeApp *);
static void             app_set_active_view      (GnomeMDI *, GnomeApp *, GtkWidget *);

static gint             app_close_top            (GnomeApp *, GdkEventAny *, GnomeMDI *);
static gint             app_close_book           (GnomeApp *, GdkEventAny *, GnomeMDI *);
static gint             app_close_ms             (GnomeApp *, GdkEventAny *, GnomeMDI *);

static void             set_page_by_widget       (GtkNotebook *, GtkWidget *);

static GtkWidget       *book_create              (GnomeMDI *);
static void             book_switch_page         (GtkNotebook *, GtkNotebookPage *,
						  gint, GnomeMDI *);
static void             book_add_view            (GtkNotebook *, GtkWidget *);
static void             book_remove_view         (GtkWidget *, GnomeMDI *);

static void             toplevel_focus           (GnomeApp *, GdkEventFocus *, GnomeMDI *);

static void             top_add_view             (GnomeMDI *, GnomeMDIChild *, GtkWidget *);

static void             book_drag_begin          (GtkNotebook *, GdkEvent *, GnomeMDI *);
static void             book_drag_req            (GtkNotebook *, GdkEvent *, GnomeMDI *);
static void             book_drop                (GtkNotebook *, GdkEvent *, GnomeMDI *);
static void             rootwin_drop             (GtkWidget *, GdkEvent *, GnomeMDI *);

static GnomeUIInfo     *copy_ui_info_tree        (GnomeUIInfo *);
static void             free_ui_info_tree        (GnomeUIInfo *);
static gint             count_ui_info_items      (GnomeUIInfo *);

#define DND_TYPE            "mdi/bookpage"

/* key for stuff that we'll assign to our GnomeApps */
#define TOOLBAR_INFO_KEY           "MDIToolbarUIInfo"
#define MENUBAR_INFO_KEY           "MDIMenubarUIInfo"
#define MDI_CHILD_LIST_KEY         "MDIChildList"
#define MDI_CHILD_MENU_INFO_KEY    "MDIChildMenuUIInfo"
#define MDI_CHILD_KEY              "GnomeMDIChild"
#define ITEM_COUNT_KEY             "MDIChildMenuItems"

/* hmmm... is it OK to have these two widgets (used for notebook page DnD)
   as global vars? */
static GtkWidget *drag_page = NULL, *drag_page_ok = NULL;

enum {
  CREATE_MENUS,
  CREATE_TOOLBAR,
  ADD_CHILD,
  REMOVE_CHILD,
  ADD_VIEW,
  REMOVE_VIEW,
  MDI_CHILD_CHANGED,
  APP_CREATED,       /* so new GnomeApps can be further customized by applications */
  LAST_SIGNAL
};

typedef GtkWidget *(*GnomeMDISignal1) (GtkObject *, gpointer);
typedef gboolean   (*GnomeMDISignal2) (GtkObject *, gpointer, gpointer);
typedef void       (*GnomeMDISignal3) (GtkObject *, gpointer, gpointer, gpointer);
typedef void       (*GnomeMDISignal4) (GtkObject *, gpointer, gpointer);

static gint mdi_signals[LAST_SIGNAL];

static GtkObjectClass *parent_class = NULL;

static void gnome_mdi_marshal_1 (GtkObject	    *object,
				 GtkSignalFunc       func,
				 gpointer	     func_data,
				 GtkArg	            *args) {
  GnomeMDISignal1 rfunc;
  gpointer *return_val;

  rfunc = (GnomeMDISignal1) func;
  return_val = GTK_RETLOC_POINTER (args[0]);
  
  *return_val = (* rfunc)(object, func_data);
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
  
  (* rfunc)(object, GTK_VALUE_POINTER(args[0]), GTK_VALUE_POINTER(args[1]), func_data);
}

static void gnome_mdi_marshal_4 (GtkObject	    *object,
				 GtkSignalFunc       func,
				 gpointer	     func_data,
				 GtkArg	            *args) {
  GnomeMDISignal4 rfunc;

  rfunc = (GnomeMDISignal4) func;
  
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
					      GTK_TYPE_POINTER, 0);
  mdi_signals[CREATE_TOOLBAR] = gtk_signal_new ("create_toolbar",
						GTK_RUN_LAST,
						object_class->type,
						GTK_SIGNAL_OFFSET (GnomeMDIClass, create_menus),
						gnome_mdi_marshal_1,
						GTK_TYPE_POINTER, 0);
  mdi_signals[ADD_CHILD] = gtk_signal_new ("add_child",
					   GTK_RUN_LAST,
					   object_class->type,
					   GTK_SIGNAL_OFFSET (GnomeMDIClass, add_child),
					   gnome_mdi_marshal_2,
					   GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  mdi_signals[REMOVE_CHILD] = gtk_signal_new ("remove_child",
					      GTK_RUN_LAST,
					      object_class->type,
					      GTK_SIGNAL_OFFSET (GnomeMDIClass, remove_child),
					      gnome_mdi_marshal_2,
					      GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  mdi_signals[ADD_VIEW] = gtk_signal_new ("add_view",
					  GTK_RUN_LAST,
					  object_class->type,
					  GTK_SIGNAL_OFFSET (GnomeMDIClass, add_view),
					  gnome_mdi_marshal_2,
					  GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  mdi_signals[REMOVE_VIEW] = gtk_signal_new ("remove_view",
					     GTK_RUN_LAST,
					     object_class->type,
					     GTK_SIGNAL_OFFSET (GnomeMDIClass, remove_view),
					     gnome_mdi_marshal_2,
					     GTK_TYPE_BOOL, 1, GTK_TYPE_POINTER);
  mdi_signals[APP_CREATED] = gtk_signal_new ("app_created",
					     GTK_RUN_LAST,
					     object_class->type,
					     GTK_SIGNAL_OFFSET (GnomeMDIClass, app_created),
					     gnome_mdi_marshal_4,
					     GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

  gtk_object_class_add_signals (object_class, mdi_signals, LAST_SIGNAL);

  class->create_menus = NULL;
  class->create_toolbar = NULL;
  class->add_child = NULL;
  class->remove_child = NULL;
  class->add_view = NULL;
  class->remove_view = NULL;
  class->app_created = NULL;

  parent_class = gtk_type_class (gtk_object_get_type ());
}

static void gnome_mdi_destroy(GtkObject *object) {
  GnomeMDI *mdi;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_MDI (object));

  mdi = GNOME_MDI (object);

  if(mdi->children)
    g_warning("GnomeMDI: Destroying when mdi_children still exist!");

  if(mdi->child_menu_path)
    g_free(mdi->child_menu_path);
  if(mdi->child_menu_label)
    g_free(mdi->child_menu_label);
  if(mdi->child_list_path)
    g_free(mdi->child_list_path);

  g_free (mdi->appname);
  g_free (mdi->title);
  g_free (mdi->dnd_type);

  if(GTK_OBJECT_CLASS(parent_class)->destroy)
    (* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

static void gnome_mdi_init (GnomeMDI *mdi) {
  gchar hostname[128] = "\0", pid[12];

  mdi->flags = 0;

  mdi->children = NULL;
  mdi->windows = NULL;

  mdi->active_child = NULL;
  mdi->active_window = NULL;
  mdi->active_view = NULL;

  mdi->root_window = NULL;

  mdi->menu_template = NULL;
  mdi->toolbar_template = NULL;
  mdi->child_menu_path = NULL;
  mdi->child_menu_label = NULL;
  mdi->child_list_path = NULL;

  gethostname(hostname, 127);
  sprintf(pid, "%d", getpid());
  mdi->dnd_type = g_copy_strings(DND_TYPE, hostname, pid, NULL);
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

static gint book_has_child(GtkNotebook *book, GtkWidget *child) {
  GList *page_node;

  page_node = book->children;
  while(page_node) {
    if( ((GtkNotebookPage *)page_node->data)->child == child)
      return TRUE;

    page_node = g_list_next(page_node);
  }

  return FALSE;
}

static void set_page_by_widget(GtkNotebook *book, GtkWidget *child) {
  GList *page_node;
  gint i = 0;

  page_node = book->children;
  while(page_node) {
    if( ((GtkNotebookPage *)page_node->data)->child == child) {
      gtk_notebook_set_page(book, i);
      return;
    }

    i++;
    page_node = g_list_next(page_node);
  }

  return;
}
static GtkWidget *find_item_by_label(GtkMenuShell *shell, gchar *label) {
  GList *child;
  GtkWidget *grandchild;

  child = shell->children;
  while(child) {
    grandchild = GTK_BIN(child->data)->child;
    if( (GTK_IS_LABEL(grandchild)) &&
	(strcmp(GTK_LABEL(grandchild)->label, label) == 0) )
      return GTK_WIDGET(child->data);

    child = g_list_next(child);
  }

  return NULL;
}

static void child_list_activated_cb(GtkWidget *w, GnomeMDI *mdi) {
  GnomeMDIChild *child;
  GtkWindow *window;

  child = gtk_object_get_data(GTK_OBJECT(w), MDI_CHILD_KEY);

  if( child && (child != mdi->active_child) ) {
    mdi->active_child = child;

    if(child->views)
      mdi->active_view = GTK_WIDGET(child->views->data);
    else
      gnome_mdi_add_view(mdi, child);

    window = GTK_WINDOW(VIEW_GET_WINDOW(mdi->active_view));

    if(mdi->flags & GNOME_MDI_NOTEBOOK)
      set_page_by_widget(GTK_NOTEBOOK(GNOME_APP(window)->contents), mdi->active_view);
    
    /* TODO: hmmm... I dont know how to give focus to the window, so that it would
       receive keyboard events */
    gdk_window_raise(GTK_WIDGET(window)->window);
    gtk_window_set_focus(window, mdi->active_view);
    gtk_window_activate_focus(window);
  }
}

static GtkWidget *child_list_menu_create(GnomeMDI *mdi) {
  GtkWidget *menu, *item, *title_item;
  GList *child;

  title_item = gtk_menu_item_new_with_label( (mdi->child_menu_label)?(mdi->child_menu_label):(_("Children")) );
  gtk_widget_show(title_item);

  if(mdi->children) {
    menu = gtk_menu_new();

    child = mdi->children;
    while(child) {
      item = gtk_menu_item_new_with_label(GNOME_MDI_CHILD(child->data)->name);
      gtk_signal_connect(GTK_OBJECT(item), "activate",
			 GTK_SIGNAL_FUNC(child_list_activated_cb), mdi);
      gtk_object_set_data(GTK_OBJECT(item), MDI_CHILD_KEY, child->data);

      gtk_widget_show(item);

      gtk_menu_append(GTK_MENU(menu), item);

      child = g_list_next(child);
    }

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(title_item), menu);
  }

  return title_item;
}

static void child_list_menu_remove_item(GnomeMDI *mdi, GnomeMDIChild *child) {
  GtkWidget *item;
  GtkMenuItem *menu;
  GtkMenuShell *shell;

  GnomeApp *app;
  GList *app_node;

  app_node = mdi->windows;
  while(app_node) {
    app = GNOME_APP(app_node->data);
    menu = GTK_MENU_ITEM(gtk_object_get_data(GTK_OBJECT(app->menubar), MDI_CHILD_LIST_KEY));

    shell = GTK_MENU_SHELL(menu->submenu);

    item = find_item_by_label(shell, child->name);
    
    gtk_container_remove(GTK_CONTAINER(shell), item);
    
    gtk_widget_queue_resize (GTK_WIDGET (shell));

    app_node = g_list_next(app_node);
  }
}

static void child_list_menu_add_item(GnomeMDI *mdi, GnomeMDIChild *child) {
  GtkWidget *item, *submenu;
  GtkMenuItem *menu;
  GnomeApp *app;
  GList *app_node;

  app_node = mdi->windows;
  while(app_node) {
    app = GNOME_APP(app_node->data);

    item = gtk_menu_item_new_with_label(child->name);
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       GTK_SIGNAL_FUNC(child_list_activated_cb), mdi);
    gtk_object_set_data(GTK_OBJECT(item), MDI_CHILD_KEY, child);
    gtk_widget_show(item);

    menu = GTK_MENU_ITEM(gtk_object_get_data(GTK_OBJECT(app->menubar), MDI_CHILD_LIST_KEY));

    if((submenu = menu->submenu) == NULL) {
      submenu = gtk_menu_new();

      gtk_menu_item_set_submenu(menu, submenu);
    }

    gtk_menu_append(GTK_MENU(submenu), item);

    gtk_widget_queue_resize(GTK_WIDGET(submenu));

    app_node = g_list_next(app_node);
  }
}

static GtkWidget *book_create(GnomeMDI *mdi) {
  GtkWidget *us, *rw;

  us = gtk_notebook_new();

  gnome_app_set_contents(mdi->active_window, us);

  gtk_widget_realize(us);

  gtk_signal_connect(GTK_OBJECT(us), "switch_page",
		     GTK_SIGNAL_FUNC(book_switch_page), mdi);
  gtk_signal_connect(GTK_OBJECT(us), "drag_begin_event",
		     GTK_SIGNAL_FUNC(book_drag_begin), mdi);
  gtk_signal_connect(GTK_OBJECT(us), "drag_request_event",
		     GTK_SIGNAL_FUNC(book_drag_req), mdi);
  gtk_signal_connect (GTK_OBJECT(us), "drop_data_available_event",
		      GTK_SIGNAL_FUNC(book_drop), mdi);
  
  /* if DnD icons are not loaded yet, load them */
  if(!drag_page)
    drag_page = gnome_stock_transparent_window (GNOME_STOCK_PIXMAP_NOT, NULL);
  if(!drag_page_ok)
    drag_page_ok = gnome_stock_transparent_window (GNOME_STOCK_PIXMAP_BOOK_BLUE, NULL);

  /* if root window is not set up as our drop zone, set it up */
  if(!mdi->root_window) {
    rw = gnome_rootwin_new ();
    
    gtk_signal_connect (GTK_OBJECT(rw), "drop_data_available_event",
			GTK_SIGNAL_FUNC(rootwin_drop), mdi);
    
    gtk_widget_realize (rw);
    gtk_widget_dnd_drop_set (rw, TRUE, &mdi->dnd_type, 1, FALSE);
    gtk_widget_show (rw);
    mdi->root_window = GNOME_ROOTWIN (rw);
  }

  gtk_widget_dnd_drop_set (us, TRUE, &mdi->dnd_type, 1, FALSE);
  gtk_widget_dnd_drag_set (us, TRUE, &mdi->dnd_type, 1);

  gtk_widget_show(us);

  return us;
}

static void book_add_view(GtkNotebook *book, GtkWidget *view) {
  GnomeMDIChild *child;
  GtkWidget *title_label;

  child = VIEW_GET_CHILD(view);

  title_label = gtk_label_new(child->name);

  gtk_notebook_append_page(book, view, title_label);

  set_page_by_widget(book, view);  
}

static void book_remove_view(GtkWidget *view, GnomeMDI *mdi) {
  GnomeMDIChild *child;

  child = VIEW_GET_CHILD(view);

  gnome_mdi_remove_view(mdi, view, FALSE);
}

static void book_switch_page(GtkNotebook *book, GtkNotebookPage *page, gint page_num, GnomeMDI *mdi) {
  GnomeApp *app;

  if(page_num == -1)
    return;

  mdi->active_view = GTK_WIDGET(page->child);
  mdi->active_child = VIEW_GET_CHILD(mdi->active_view);

  app = VIEW_GET_WINDOW(mdi->active_view);

  app_set_active_view(mdi, app, mdi->active_view);
}

static void toplevel_focus(GnomeApp *app, GdkEventFocus *event, GnomeMDI *mdi) {
  /* updates active_view and active_child when a new toplevel receives focus */
  g_return_if_fail(GNOME_IS_APP(app));

  mdi->active_window = app;

  if(app->contents == NULL)
    return;

  if((mdi->flags & GNOME_MDI_TOPLEVEL) || (mdi->flags & GNOME_MDI_MODAL)) {
    mdi->active_view = app->contents;
    mdi->active_child = VIEW_GET_CHILD(mdi->active_view);
  }
  else if((mdi->flags & GNOME_MDI_NOTEBOOK) && (GTK_NOTEBOOK(app->contents)->cur_page != NULL)) {
    mdi->active_view = GTK_NOTEBOOK(app->contents)->cur_page->child;
    mdi->active_child = VIEW_GET_CHILD(mdi->active_view);
  }
}

static void book_drag_begin(GtkNotebook *book, GdkEvent *event, GnomeMDI *mdi) {
  GdkPoint hotspot = { 5, 5 };

  if (drag_page && drag_page_ok) {
    gdk_dnd_set_drag_shape (drag_page->window, &hotspot,
			    drag_page_ok->window, &hotspot);	

    gtk_widget_show (drag_page_ok);
    gtk_widget_show (drag_page);
  }
}

static void book_drag_req(GtkNotebook *book, GdkEvent *event, GnomeMDI *mdi) {
  gtk_widget_dnd_data_set(GTK_WIDGET(book), event, book, sizeof(book));
}

static void book_drop(GtkNotebook *book, GdkEvent *event, GnomeMDI *mdi) {
  GtkWidget *view;
  GtkNotebook *old_book;
  GnomeApp *app;

  if(strcmp(event->dropdataavailable.data_type, mdi->dnd_type) != 0)
    return;

#if 0
  /* can get this to work */
  old_book = GTK_NOTEBOOK(event->dropdataavailable.data);
#else
  /* so this is a workaround */
  old_book = GTK_NOTEBOOK(mdi->active_window->contents);
#endif

  if(book == old_book)
    return;

  if(old_book->cur_page) {
    view = old_book->cur_page->child;
    gtk_container_remove(GTK_CONTAINER(old_book), view);

    book_add_view(book, view);

    if(old_book->cur_page == NULL) {
      app = GNOME_APP(GTK_WIDGET(old_book)->parent->parent);
      mdi->windows = g_list_remove(mdi->windows, app);
      gtk_widget_destroy(GTK_WIDGET(app));
    }
  }
}

static void rootwin_drop(GtkWidget *rw, GdkEvent *event, GnomeMDI *mdi) {
  GtkWidget *view, *new_book;
  GtkNotebook *old_book;

  if(strcmp(event->dropdataavailable.data_type, mdi->dnd_type) != 0)
    return;

#if 0
  /* dont know why this doesnt work... */
  old_book = GTK_NOTEBOOK(event->dropdataavailable.data);
#else
  /* so this is a workaround */
  old_book = GTK_NOTEBOOK(mdi->active_window->contents);
#endif 

  if(g_list_length(old_book->children) == 1)
    return;

  if(old_book->cur_page) {
    view = old_book->cur_page->child;
    gtk_container_remove(GTK_CONTAINER(old_book), view);

    app_create(mdi);

    new_book = book_create(mdi);

    book_add_view(GTK_NOTEBOOK(new_book), view);

    gtk_widget_show(GTK_WIDGET(mdi->active_window));
  }
}

static gint app_close_top(GnomeApp *app, GdkEventAny *event, GnomeMDI *mdi) {
  GnomeMDIChild *child = NULL;
  GList *child_node;
  gint handler_ret = TRUE;

  if(g_list_length(mdi->windows) == 1) {
    /* since this is the last window, we only close it and destroy the MDI if
       ALL the remaining mdi_children can be closed, which we check in advance */
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
    gtk_object_destroy(GTK_OBJECT(mdi));
  }
  else if(app->contents) {
    child = VIEW_GET_CHILD(app->contents);
    if(CHILD_LAST_VIEW(child)) {

      /* if this is the last view, we should remove the mdi_child! */
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
    gtk_object_destroy(GTK_OBJECT(mdi));
  }
  else {
    /* first check if all the mdi_children in this notebook can be removed */
    page_node = GTK_NOTEBOOK(app->contents)->children;
    while(page_node) {
      view = ((GtkNotebookPage *)page_node->data)->child;
      child = VIEW_GET_CHILD(view);

      page_node = g_list_next(page_node);

      node = child->views;
      while(node) {
	if(VIEW_GET_WINDOW(node->data) != app)
	  break;

	node = g_list_next(node);
      }

      if(node == NULL) {   /* all the views reside in this GnomeApp */
	gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_CHILD], child, &handler_ret);
	if(handler_ret == FALSE)
	  return TRUE;
      }
    }

    /* now actually remove all mdi_children/views! */
    page_node = GTK_NOTEBOOK(app->contents)->children;
    while(page_node) {
      view = ((GtkNotebookPage *)page_node->data)->child;
      child = VIEW_GET_CHILD(view);
      
      page_node = g_list_next(page_node);

      if(CHILD_LAST_VIEW(child))
	gnome_mdi_remove_child(mdi, child, TRUE);
      else
	gnome_mdi_remove_view(mdi, view, TRUE);
    }
  }

  return FALSE;
}

static void app_set_title(GnomeMDI *mdi, GnomeApp *app) {
  gchar *fullname;
  GnomeMDIChild *child;

  if( (mdi->flags & (GNOME_MDI_MODAL | GNOME_MDI_TOPLEVEL)) &&
      app->contents && (child = VIEW_GET_CHILD(app->contents)) ) {
    fullname = g_copy_strings(child->name, " - ", mdi->title, NULL);
    gtk_window_set_title(GTK_WINDOW(app), fullname);
    g_free(fullname);
  }
  else
    gtk_window_set_title(GTK_WINDOW(app), mdi->title);
}

static void app_set_active_view(GnomeMDI *mdi, GnomeApp *app, GtkWidget *view) {
  GList *menu_list = NULL, *children;
  GtkWidget *parent;
  GnomeMDIChild *child;
  GnomeUIInfo *ui_info;
  gint pos, items;

  app_set_title(mdi, app);

  ui_info = gtk_object_get_data(GTK_OBJECT(app), MDI_CHILD_MENU_INFO_KEY);
  if(ui_info != NULL) {
    free_ui_info_tree(ui_info);
    gtk_object_set_data(GTK_OBJECT(app), MDI_CHILD_MENU_INFO_KEY, NULL);
  }
  ui_info = NULL;

  items = (gint)gtk_object_get_data(GTK_OBJECT(app), ITEM_COUNT_KEY);
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
    child = VIEW_GET_CHILD(view);

    if( child->menu_template &&
        ( (ui_info = copy_ui_info_tree(child->menu_template)) != NULL) ) {
      gnome_app_insert_menus_with_data(app, mdi->child_menu_path, ui_info, child);
      gtk_object_set_data(GTK_OBJECT(app), MDI_CHILD_MENU_INFO_KEY, ui_info);
      gtk_object_set_data(GTK_OBJECT(app), ITEM_COUNT_KEY, (gpointer)count_ui_info_items(ui_info));
    }
    else {
      gtk_signal_emit_by_name(GTK_OBJECT(child), "create_menus", view, &menu_list);

      parent = gnome_app_find_menu_pos(app->menubar, mdi->child_menu_path, &pos);

      if(menu_list && parent) {
	items = 0;

	while(menu_list) {
	  gtk_menu_shell_insert(GTK_MENU_SHELL(parent), GTK_WIDGET(menu_list->data), pos);
	  menu_list = g_list_remove(menu_list, menu_list->data);
	  pos++;
	  items++;
	}

	gtk_object_set_data(GTK_OBJECT(app), ITEM_COUNT_KEY, (gpointer)items);

	gtk_widget_queue_resize(parent);
      }
    }
  }
  else 
    gtk_object_set_data(GTK_OBJECT(app), ITEM_COUNT_KEY, NULL);

  mdi->active_view = view;
}

static void app_destroy(GnomeApp *app) {
  GnomeUIInfo *ui_info;

  /* free stuff that got allocated for this GnomeApp */

  ui_info = gtk_object_get_data(GTK_OBJECT(app), MENUBAR_INFO_KEY);
  if(ui_info)
    free_ui_info_tree(ui_info);

  ui_info = gtk_object_get_data(GTK_OBJECT(app), TOOLBAR_INFO_KEY);
  if(ui_info)
    free_ui_info_tree(ui_info);

  ui_info = gtk_object_get_data(GTK_OBJECT(app), MDI_CHILD_MENU_INFO_KEY);
  if(ui_info)
    free_ui_info_tree(ui_info);
}

static void app_create(GnomeMDI *mdi) {
  GtkWidget *window;
  GtkWidget *child_menu, *parent;
  GtkMenuBar *menubar = NULL;
  GtkToolbar *toolbar = NULL;
  GtkSignalFunc func;
  GnomeUIInfo *ui_info;
  gint pos = 0;

  window = gnome_app_new(mdi->appname, mdi->title);

#if 0
  /* is this really necessary? */
  gtk_window_set_wmclass (GTK_WINDOW (window), mdi->appname, mdi->appname);
#endif

  gtk_widget_realize(window);

  mdi->windows = g_list_append(mdi->windows, window);

  if((mdi->flags & GNOME_MDI_TOPLEVEL) || (mdi->flags & GNOME_MDI_MODAL))
    func = GTK_SIGNAL_FUNC(app_close_top);
  else if(mdi->flags & GNOME_MDI_NOTEBOOK)
    func = GTK_SIGNAL_FUNC(app_close_book);
#if 0
  else if(mdi->flags & GNOME_MDI_MS)
    func = GTK_SIGNAL_FUNC(app_close_ms);
#endif

  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		     func, mdi);
  gtk_signal_connect(GTK_OBJECT(window), "focus_in_event",
		     GTK_SIGNAL_FUNC(toplevel_focus), mdi);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(app_destroy), NULL);

  gtk_window_position (GTK_WINDOW(window), GTK_WIN_POS_MOUSE);

  /* set up menus */
  if(mdi->menu_template) {
    ui_info = copy_ui_info_tree(mdi->menu_template);
    gnome_app_create_menus(GNOME_APP(window), ui_info);
    gtk_object_set_data(GTK_OBJECT(window), MENUBAR_INFO_KEY, ui_info);

    menubar = GTK_MENU_BAR(GNOME_APP(window)->menubar);
  }
  else {
    /* we use the (hopefully) supplied create_menus signal handler */
    gtk_signal_emit (GTK_OBJECT (mdi), mdi_signals[CREATE_MENUS], &menubar);

    if(menubar) {
      gtk_widget_show(GTK_WIDGET(menubar));
      gnome_app_set_menus(GNOME_APP(window), GTK_MENU_BAR(menubar));
    }
  }

  /* we add the mdi_child list menu to the menubar */
  if( menubar && (parent = gnome_app_find_menu_pos(GTK_WIDGET(menubar), mdi->child_list_path, &pos)) ) {
    child_menu = child_list_menu_create(mdi);
    gtk_menu_shell_insert(GTK_MENU_SHELL(parent), child_menu, pos);
    gtk_object_set_data(GTK_OBJECT(menubar), MDI_CHILD_LIST_KEY, child_menu);
  }

  /* create toolbar */
  if(mdi->toolbar_template) {
    ui_info = copy_ui_info_tree(mdi->menu_template);
    gnome_app_create_toolbar(GNOME_APP(window), ui_info);
    gtk_object_set_data(GTK_OBJECT(window), TOOLBAR_INFO_KEY, ui_info);
  }
  else {
    gtk_signal_emit(GTK_OBJECT (mdi), mdi_signals[CREATE_TOOLBAR], &toolbar);

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
  gboolean app_full = FALSE;

  if (mdi->active_window->contents != NULL) {
    app_full = TRUE;
    app_create(mdi);
  }

  window = mdi->active_window;

  if(child && view)
    gnome_app_set_contents(window, view);

  app_set_active_view(mdi, window, view);

  if(!GTK_WIDGET_VISIBLE(window))
    gtk_widget_show(GTK_WIDGET(window));

  if(!app_full) {
    mdi->active_view = view;
    mdi->active_child = child;
  }
}

gint gnome_mdi_add_view(GnomeMDI *mdi, GnomeMDIChild *child) {
  GtkWidget *view;
  gint ret = TRUE;

  view = gnome_mdi_child_add_view(child);

  gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[ADD_VIEW], view, &ret);

  if(ret == FALSE)
    gnome_mdi_child_remove_view(child, view);
  else if(mdi->flags & GNOME_MDI_NOTEBOOK)
    book_add_view(GTK_NOTEBOOK(mdi->active_window->contents), view);
  else if(mdi->flags & GNOME_MDI_TOPLEVEL)
    /* add a new toplevel unless the remaining one is empty */
    top_add_view(mdi, child, view);
  else if(mdi->flags & GNOME_MDI_MODAL) {
    /* replace the existing view if there is one */
    if(mdi->active_window->contents) {
      gnome_mdi_remove_view(mdi, mdi->active_window->contents, TRUE);
      mdi->active_window->contents = NULL;
    }

    mdi->active_view = view;
    mdi->active_child = child;

    gnome_app_set_contents(mdi->active_window, view);
    app_set_active_view(mdi, mdi->active_window, view);
  }
#if 0
  else if(mdi->flags & GNOME_MDI_MS)
    gnome_mdi_pouch_add(GNOME_MDI_POUCH(mdi->active_window->contents), view);
#endif

  gtk_widget_show(view);

  return ret;
}

gint gnome_mdi_remove_view(GnomeMDI *mdi, GtkWidget *view, gint force) {
  GtkWidget *parent;
  GnomeApp *window;
  GnomeMDIChild *child;
  gint ret = TRUE;

  if(!force)
    gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_VIEW], view, &ret);

  if(ret == FALSE)
    return FALSE;

  if(view == mdi->active_view) {
    mdi->active_view = NULL;
    mdi->active_child = NULL;
  }

  child = VIEW_GET_CHILD(view);

  parent = view->parent;

  window = VIEW_GET_WINDOW(view);

#if 0
  if(mdi->flags & GNOME_MDI_MS)
    gnome_mdi_pouch_remove(GNOME_MDI_POUCH(GTK_WIDGET(parent)->parent), view);
  else
    gtk_container_remove(GTK_CONTAINER(parent), view);
#else
  gtk_container_remove(GTK_CONTAINER(parent), view);
#endif

  if(mdi->flags & (GNOME_MDI_TOPLEVEL | GNOME_MDI_MODAL)) {
    window->contents = NULL;

    /* if this is NOT the last toplevel, we destroy it */
    if(g_list_length(mdi->windows) > 1) {
      mdi->windows = g_list_remove(mdi->windows, window);
      gtk_widget_destroy(GTK_WIDGET(window));
    }
    else
      app_set_active_view(mdi, window, NULL);
  }
  else if( (mdi->flags & GNOME_MDI_NOTEBOOK) &&
	   (GTK_NOTEBOOK(parent)->cur_page == NULL) &&
	   (g_list_length(mdi->windows) > 1) ) {
    /* if this is NOT the last toplevel, destroy it! */
    mdi->windows = g_list_remove(mdi->windows, window);
    gtk_widget_destroy(GTK_WIDGET(window));
  }
  else if(mdi->active_child == NULL)
    app_set_active_view(mdi, window, NULL);

  /* remove this view from the mdi_child's view list */
  gnome_mdi_child_remove_view(child, view);

  return TRUE;
}

gint gnome_mdi_add_child(GnomeMDI *mdi, GnomeMDIChild *child) {
  GtkWidget *view;
  gint ret = TRUE;

  g_return_val_if_fail(mdi != NULL, FALSE);
  g_return_val_if_fail(GNOME_IS_MDI(mdi), FALSE);
  g_return_val_if_fail(child != NULL, FALSE);
  g_return_val_if_fail(GNOME_IS_MDI_CHILD(child), FALSE);

  gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[ADD_CHILD], child, &ret);

  if(ret == FALSE)
    return FALSE;

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

  if(!force)
    gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_CHILD], child, &ret);

  if(ret == FALSE)
    return FALSE;

  while(child->views)
    gnome_mdi_remove_view(mdi, GTK_WIDGET(child->views->data), TRUE);

  mdi->children = g_list_remove(mdi->children, child);

  if(mdi->active_child == child) {
    mdi->active_child = NULL;
    mdi->active_view = NULL;
  }

  child_list_menu_remove_item(mdi, child);

  gtk_object_destroy(GTK_OBJECT(child));

  return TRUE;
}

gint gnome_mdi_remove_all(GnomeMDI *mdi, gint force) {
  GList *child_node;
  GnomeMDIChild *child;
  gint handler_ret = TRUE;

  if(!force) {
    child_node = mdi->children;
    while(child_node) {
      gtk_signal_emit(GTK_OBJECT(mdi), mdi_signals[REMOVE_CHILD], child_node->data, &handler_ret);

      if(handler_ret == FALSE)
	return FALSE;

      child_node = g_list_next(child_node);
    }
  }

  while(mdi->children) {
    child = GNOME_MDI_CHILD(mdi->children->data);
    mdi->children = g_list_next(mdi->children);
    gnome_mdi_remove_child(mdi, child, TRUE);
  }

  return TRUE;
}

static GnomeMDIChild *gnome_mdi_find_child(GnomeMDI *mdi, gchar *name) {
  GList *child;

  child = mdi->children;
  while(child) {
    if(strcmp(GNOME_MDI_CHILD(child->data)->name, name) == 0)
      return GNOME_MDI_CHILD(child->data);

    child = g_list_next(child);
  }

  return NULL;
}

void gnome_mdi_set_mode(GnomeMDI *mdi, gint mode) {
  GtkWidget *view;
  GnomeMDIChild *child;
  GList *child_node, *view_node;

  if(mode & mdi->flags)
    return;

  /* remove all views from their parents */
  child_node = mdi->children;
  while(child_node) {
    child = GNOME_MDI_CHILD(child_node->data);
    view_node = child->views;
    while(view_node) {
      view = GTK_WIDGET(view_node->data);

      if(mdi->flags & (GNOME_MDI_TOPLEVEL | GNOME_MDI_MODAL))
	VIEW_GET_WINDOW(view)->contents = NULL;

      gtk_container_remove(GTK_CONTAINER(view->parent), view);

      view_node = g_list_next(view_node);

      /* if we are to change mode to MODAL, destroy all views except the active one */
      if( (mdi->flags & GNOME_MDI_MODAL)  && (view != mdi->active_view) )
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

  mdi->flags &= ~GNOME_MDI_MODE_FLAGS;
  mdi->flags |= mode;

  app_create(mdi);

  if(mdi->flags & GNOME_MDI_NOTEBOOK)
    book_create(mdi);
#if 0
  else if(mdi->flags & GNOME_MDI_MS) {
    us = gnome_mdi_pouch_new();
    gtk_widget_show(us);
    gnome_app_set_contents(mdi->active_window, us);
  }
#endif

  /* re-implant views in proper containers */
  child_node = mdi->children;
  while(child_node) {
    child = GNOME_MDI_CHILD(child_node->data);
    view_node = child->views;
    while(view_node) {
      view = GTK_WIDGET(view_node->data);

      if(mdi->flags & GNOME_MDI_NOTEBOOK)
	book_add_view(GTK_NOTEBOOK(mdi->active_window->contents), view);
      else if(mdi->flags & GNOME_MDI_TOPLEVEL)
	/* add a new toplevel unless the remaining one is empty */
	top_add_view(mdi, child, view);
      else if(mdi->flags & GNOME_MDI_MODAL) {
	/* replace the existing view if there is one */
	if(mdi->active_window->contents) {
	  gnome_mdi_remove_view(mdi, mdi->active_window->contents, TRUE);
	  mdi->active_window->contents = NULL;
	}
	mdi->active_view = view;
	mdi->active_child = child;
	gnome_app_set_contents(mdi->active_window, view);
	app_set_active_view(mdi, mdi->active_window, view);
      }
#if 0
      else if(mdi->flags & GNOME_MDI_MS)
	gnome_mdi_pouch_add(GNOME_MDI_POUCH(mdi->active_window->contents), view);
#endif

      view_node = g_list_next(view_node);
    }
    child_node = g_list_next(child_node);
  }  

  if(!GTK_WIDGET_VISIBLE(mdi->active_window))
    gtk_widget_show(GTK_WIDGET(mdi->active_window));
}

GnomeMDIChild *gnome_mdi_active_child(GnomeMDI *mdi) {
  if(mdi->active_view)
    return(VIEW_GET_CHILD(mdi->active_view));

  return NULL;
}

void gnome_mdi_set_menu_template(GnomeMDI *mdi, GnomeUIInfo *menu_tmpl) {
  mdi->menu_template = menu_tmpl;
}

void gnome_mdi_set_toolbar_template(GnomeMDI *mdi, GnomeUIInfo *tbar_tmpl) {
  mdi->toolbar_template = tbar_tmpl;
}

void gnome_mdi_set_child_menu_path(GnomeMDI *mdi, gchar *path) {
  if(mdi->child_menu_path)
    g_free(mdi->child_menu_path);

  mdi->child_menu_path = g_strdup(path);
}

void gnome_mdi_set_child_menu_label(GnomeMDI *mdi, gchar *label) {
  if(mdi->child_menu_label)
    g_free(mdi->child_menu_label);

  mdi->child_menu_label = g_strdup(label);
}

void gnome_mdi_set_child_list_path(GnomeMDI *mdi, gchar *path) {
  if(mdi->child_list_path)
    g_free(mdi->child_list_path);

  mdi->child_list_path = g_strdup(path);
}
