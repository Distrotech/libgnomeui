/*
 * gnome-mdi.h - definition of a Gnome MDI object
 * written by Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
 */

/*
 * GnomeMDI signals:
 *
 * gint add_child(GnomeMDI *, GnomeMDIChild *)
 * gint add_view(GnomeMDI *, GtkWidget *)
 *   are called before actually adding a mdi_child or a view to the MDI. if the handler returns
 *   TRUE, the action proceeds otherwise the mdi_child or view are not added.
 *
 * gint remove_child(GnomeMDI *, GnomeMDIChild *)
 * gint remove_view(GnomeMDI *, GtkWidget *)
 *   are called before removing mdi_child or view. the handler should return true if the object
 *   is to be removed from MDI
 *
 * GtkMenubar *create_menus(GnomeMDI *, GnomeApp *)
 *   should return a GtkMenubar for the GnomeApps when the GnomeUIInfo way with using menu
 *   template is not sufficient. This signal is emitted when a new GnomeApp that
 *   needs a new menubar is created but ONLY if the menu template is NULL!
 *
 * GtkToolbar *create_toolbar(GnomeMDI *, GnomeApp *)
 *   should return a GtkToolbar for the GnomeApps when the GnomeUIInfo way with using toolbar
 *   template is not sufficient. This signal is emitted when a new GnomeApp that
 *   needs new toolbar is created but ONLY if the toolbar template is NULL!
 *
 * void child_changed(GnomeMDI *, GnomeMDIChild *)
 *   gets called each time when active child is changed with the second argument
 *   pointing to the old child. mdi->active_view and mdi->active_child still already
 *   hold the new values
 *
 * void view_changed(GnomeMDI *, GtkWidget *)
 *   is emitted whenever a view is changed, regardless of it being the view of the same child as
 *   the old view or not. the second argument points to the old view, mdi->active_view and
 *   mdi->active_child hold the new values. if the child has also been changed, this signal is
 *   emitted after the child_changed signal.
 * 
 * void app_created(GnomeMDI *, GnomeApp *)
 *   is called with each newly created GnomeApp to allow the MDI user to customize it.
 *   no contents may be set since GnomeMDI uses them for storing either a view of a child
 *   or a notebook
 */

#ifndef __GNOME_MDI_H__
#define __GNOME_MDI_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-rootwin.h"
#include "gnome-mdi-child.h"

BEGIN_GNOME_DECLS

#define GNOME_MDI(obj)          GTK_CHECK_CAST (obj, gnome_mdi_get_type (), GnomeMDI)
#define GNOME_MDI_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_mdi_get_type (), GnomeMDIClass)
#define GNOME_IS_MDI(obj)       GTK_CHECK_TYPE (obj, gnome_mdi_get_type ())

typedef struct _GnomeMDI       GnomeMDI;
typedef struct _GnomeMDIClass  GnomeMDIClass;

typedef enum {
  GNOME_MDI_NOTEBOOK,
  GNOME_MDI_TOPLEVEL,
  GNOME_MDI_MODAL,
  GNOME_MDI_MS,
  GNOME_MDI_DEFAULT_MODE = 42,
  GNOME_MDI_REDRAW = -1		/* do not change mode, just ``redraw'' the display */
} GnomeMDIMode;

/* the following keys are used to gtk_object_set_data() copies of the appropriate menu and toolbar templates
 * to their GnomeApps. gtk_object_get_data(gnome_mdi_view_get_toplevel(view), key) will give you a pointer to
 * the data if you have a pointer to a view. this might be useful for enabling/disabling menu items (via
 * ui_info[i]->widget) when certain events happen. these GnomeUIInfo structures are exact copies of the
 * template GnomeUIInfo trees.
 */
#define GNOME_MDI_TOOLBAR_INFO_KEY           "MDIToolbarUIInfo"
#define GNOME_MDI_MENUBAR_INFO_KEY           "MDIMenubarUIInfo"
#define GNOME_MDI_CHILD_MENU_INFO_KEY        "MDIChildMenuUIInfo"

struct _GnomeMDI {
  GtkObject object;

  GnomeMDIMode mode;

  GtkPositionType tab_pos;

  gchar *appname, *title;
  gchar *dnd_type;

  /* probably only one of these would do, but... redundancy rules ;) */
  GnomeMDIChild *active_child;
  GtkWidget *active_view;
  GnomeApp *active_window;

  GList *windows;     /* toplevel windows - GnomeApp widgets */
  GList *children;    /* children - GnomeMDIChild objects*/

  GSList *registered; /* see comment for gnome_mdi_(un)register() functions below for an explanation. */

  GnomeUIInfo *menu_template;
  GnomeUIInfo *toolbar_template;

  /* paths for insertion of mdi_child specific menus and mdi_child list menu via
     gnome-app-helper routines */
  gchar *child_menu_path;
  gchar *child_list_path;

  GnomeRootWin *root_window; /* this is needed for DND */
};

struct _GnomeMDIClass
{
  GtkObjectClass parent_class;

  GtkMenuBar *(*create_menus)(GnomeMDI *, GnomeApp *);
  GtkToolbar *(*create_toolbar)(GnomeMDI *, GnomeApp *);
  gint        (*add_child)(GnomeMDI *, GnomeMDIChild *); 
  gint        (*remove_child)(GnomeMDI *, GnomeMDIChild *); 
  gint        (*add_view)(GnomeMDI *, GtkWidget *); 
  gint        (*remove_view)(GnomeMDI *, GtkWidget *); 
  void        (*child_changed)(GnomeMDI *, GnomeMDIChild *);
  void        (*view_changed)(GnomeMDI *, GtkWidget *);
  void        (*app_created)(GnomeMDI *, GnomeApp *);
};

guint         gnome_mdi_get_type            (void);

GtkObject     *gnome_mdi_new                (gchar *, gchar *);

void          gnome_mdi_set_mode            (GnomeMDI *, GnomeMDIMode);

void          gnome_mdi_set_tab_pos         (GnomeMDI *, GtkPositionType);

/* setting the menu and toolbar stuff */
void          gnome_mdi_set_menu_template   (GnomeMDI *, GnomeUIInfo *);
void          gnome_mdi_set_toolbar_template(GnomeMDI *, GnomeUIInfo *);
void          gnome_mdi_set_child_menu_path (GnomeMDI *, gchar *);
void          gnome_mdi_set_child_list_path (GnomeMDI *, gchar *);

/* manipulating views */
gint          gnome_mdi_add_view            (GnomeMDI *, GnomeMDIChild *);
gint          gnome_mdi_add_toplevel_view   (GnomeMDI *, GnomeMDIChild *);
gint          gnome_mdi_remove_view         (GnomeMDI *, GtkWidget *, gint);

GtkWidget     *gnome_mdi_active_view        (GnomeMDI *);
void          gnome_mdi_set_active_view     (GnomeMDI *, GtkWidget *);

/* manipulating children */
gint          gnome_mdi_add_child           (GnomeMDI *, GnomeMDIChild *);
gint          gnome_mdi_remove_child        (GnomeMDI *, GnomeMDIChild *, gint);
gint          gnome_mdi_remove_all          (GnomeMDI *, gint);

void          gnome_mdi_update_child        (GnomeMDI *, GnomeMDIChild *);

GnomeMDIChild *gnome_mdi_active_child       (GnomeMDI *);
GnomeMDIChild *gnome_mdi_find_child         (GnomeMDI *, gchar *);

/*
 * the following two functions are here to make life easier if an application opens windows
 * that should "keep the app alive" even if there are no MDI windows open. any such windows
 * should be registered with the MDI: as long as there is a window registered, the MDI will
 * not destroy itself (even if the last of its windows is closed). on the other hand, closing
 * the last MDI window when no other windows are registered with the MDI will result in MDI
 * being gtk_object_destroy()ed.
 */
void          gnome_mdi_register            (GnomeMDI *, GtkWidget *);
void          gnome_mdi_unregister          (GnomeMDI *, GtkWidget *);

/*
 * convenience functions for retrieveing GnomeMDIChild and GnomeApp
 * objects associated with a particular view. These obsolete the
 * VIEW_GET_*() macros.
 */
GnomeApp      *gnome_mdi_get_app_from_view  (GtkWidget *);
GnomeMDIChild *gnome_mdi_get_child_from_view(GtkWidget *);

END_GNOME_DECLS

#endif /* __GNOME_MDI_H__ */
