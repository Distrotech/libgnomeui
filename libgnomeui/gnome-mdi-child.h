/*
 * gnome-mdi-child.h - definition of a Gnome MDI Child object
 * written by Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
 */

#ifndef __GNOME_MDI_CHILD_H__
#define __GNOME_MDI_CHILD_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "libgnomeui/gnome-app-helper.h"

BEGIN_GNOME_DECLS

#define GNOME_MDI_CHILD(obj)          GTK_CHECK_CAST (obj, gnome_mdi_child_get_type (), GnomeMDIChild)
#define GNOME_MDI_CHILD_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_mdi_child_get_type (), GnomeMDIChildClass)
#define GNOME_IS_MDI_CHILD(obj)       GTK_CHECK_TYPE (obj, gnome_mdi_child_get_type ())

#define VIEW_GET_CHILD(view)         GNOME_MDI_CHILD(gtk_object_get_data(GTK_OBJECT(view), "GnomeMDIChild"))
#define VIEW_GET_WINDOW(v)           GNOME_APP(gtk_widget_get_toplevel(GTK_WIDGET(v)))
#define VIEW_GET_NAME(v)             (VIEW_GET_CHILD(v)->name)

#define CHILD_LAST_VIEW(child)       (g_list_length(child->views) == 1)

typedef struct _GnomeMDIChild       GnomeMDIChild;
typedef struct _GnomeMDIChildClass  GnomeMDIChildClass;

struct _GnomeMDIChild
{
  GtkObject object;

  gchar *name;

  GList *views;

  GnomeUIInfo *menu_template;
};

struct _GnomeMDIChildClass
{
  GtkObjectClass parent_class;

  GtkWidget * (*create_view)(GnomeMDIChild *); 
  GList     * (*create_menus)(GnomeMDIChild *, GtkWidget *); 
};

guint         gnome_mdi_child_get_type         (void);

GnomeMDIChild *gnome_mdi_child_new             (void);

GtkWidget     *gnome_mdi_child_add_view        (GnomeMDIChild *);
void          gnome_mdi_child_remove_view      (GnomeMDIChild *, GtkWidget *view);

void          gnome_mdi_child_set_name         (GnomeMDIChild *, gchar *);

void          gnome_mdi_child_set_menu_template(GnomeMDIChild *, GnomeUIInfo *);

END_GNOME_DECLS

#endif /* __GNOME_MDI_CHILD_H__ */
