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
 *   should be removed from MDI
 *
 * GtkMenubar *create_menus(GnomeMDI *)
 *   should return a GtkMenubar for the GnomeApps when the GnomeUIInfo way with using menu
 *   template is not sufficient. This signal is emitted when a new GnomeApp that
 *   needs new menubar is created but ONLY if the menu template is NULL!
 *
 * GtkToolbar *create_toolbar(GnomeMDI *)
 *   should return a GtkToolbar for the GnomeApps when the GnomeUIInfo way with using toolbar
 *   template is not sufficient. This signal is emitted when a new GnomeApp that
 *   needs new toolbar is created but ONLY if the toolbar template is NULL!
 *
 * void app_created(GnomeMDI *, GnomeApp *)
 *   is called with each newly created GnomeApp to allow the MDI user to customize it.
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

#define GNOME_MDI_MS         (1L << 0)      /* for MS-like MDI - not supported right now */
#define GNOME_MDI_NOTEBOOK   (1L << 1)      /* notebook */
#define GNOME_MDI_MODAL      (1L << 2)      /* "modal" app */
#define GNOME_MDI_TOPLEVEL   (1L << 3)      /* many toplevel windows */

#define GNOME_MDI_MODE_FLAGS (GNOME_MDI_MS | GNOME_MDI_NOTEBOOK | GNOME_MDI_MODAL | GNOME_MDI_TOPLEVEL)

struct _GnomeMDI {
  GtkObject object;

  gint32 flags;

  gchar *appname, *title;
  gchar *dnd_type;

  /* probably only one of these would do, but... redundancy rules ;) */
  GnomeMDIChild *active_child;
  GtkWidget *active_view;
  GnomeApp *active_window;

  GList *windows;   /* toplevel windows - GnomeApp widgets */
  GList *children;  /* children - GnomeMDIChild objects*/

  GnomeUIInfo *menu_template;
  GnomeUIInfo *toolbar_template;

  /* paths for insertion of mdi_child specific menus and mdi_child list menu via
     gnome-app-helper routines */
  gchar *child_menu_path;
  gchar *child_menu_label;
  gchar *child_list_path;

  GnomeRootWin *root_window; /* this is needed for DND */
};

struct _GnomeMDIClass
{
  GtkObjectClass parent_class;

  GtkMenuBar *(*create_menus)(GnomeMDI *);
  GtkToolbar *(*create_toolbar)(GnomeMDI *);
  gint        (*add_child)(GnomeMDI *, GnomeMDIChild *); 
  gint        (*remove_child)(GnomeMDI *, GnomeMDIChild *); 
  gint        (*add_view)(GnomeMDI *, GtkWidget *); 
  gint        (*remove_view)(GnomeMDI *, GtkWidget *); 
  void        (*app_created)(GnomeMDI *, GnomeApp *);
};

GtkObject     *gnome_mdi_new                (gchar *, gchar *);

void          gnome_mdi_set_mode            (GnomeMDI *, gint);

/* setting the menu and toolbar stuff */
void          gnome_mdi_set_menu_template   (GnomeMDI*, GnomeUIInfo *);
void          gnome_mdi_set_toolbar_template(GnomeMDI*, GnomeUIInfo *);
void          gnome_mdi_set_child_menu_path (GnomeMDI *, gchar *);
void          gnome_mdi_set_child_menu_label(GnomeMDI *, gchar *);
void          gnome_mdi_set_child_list_path (GnomeMDI *, gchar *);

GnomeMDIChild *gnome_mdi_active_child       (GnomeMDI*);

gint          gnome_mdi_add_view            (GnomeMDI *, GnomeMDIChild *);
gint          gnome_mdi_remove_view         (GnomeMDI *, GtkWidget *, gint);

gint          gnome_mdi_add_child           (GnomeMDI *, GnomeMDIChild *);
gint          gnome_mdi_remove_child        (GnomeMDI *, GnomeMDIChild *, gint);
gint          gnome_mdi_remove_all          (GnomeMDI *, gint);

END_GNOME_DECLS

#endif /* __GNOME_MDI_H__ */
