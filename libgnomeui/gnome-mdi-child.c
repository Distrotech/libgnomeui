#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-mdi-child.h"

static void gnome_mdi_child_class_init       (GnomeMDIChildClass *klass);
static void gnome_mdi_child_init             (GnomeMDIChild *);
static void gnome_mdi_child_destroy          (GtkObject *);
static void gnome_mdi_child_real_changed     (GnomeMDIChild *, gpointer);

enum {
  CREATE_VIEW,
  CREATE_MENUS,
  LAST_SIGNAL
};

typedef GtkWidget *(*GnomeMDIChildSignal1) (GtkObject *, gpointer);
typedef GtkWidget *(*GnomeMDIChildSignal2) (GtkObject *, gpointer, gpointer);

static GtkObjectClass *parent_class = NULL;

static gint mdi_child_signals[LAST_SIGNAL];

static void gnome_mdi_child_marshal_1 (GtkObject	    *object,
				      GtkSignalFunc   func,
				      gpointer	    func_data,
				      GtkArg	    *args) {
  GnomeMDIChildSignal1 rfunc;
  gpointer *return_val;
  
  rfunc = (GnomeMDIChildSignal1) func;
  return_val = GTK_RETLOC_POINTER (args[0]);
  
  *return_val = (* rfunc)(object, func_data);
}

static void gnome_mdi_child_marshal_2 (GtkObject	    *object,
				      GtkSignalFunc   func,
				      gpointer	    func_data,
				      GtkArg	    *args) {
  GnomeMDIChildSignal2 rfunc;
  gpointer *return_val;
  
  rfunc = (GnomeMDIChildSignal2) func;
  return_val = GTK_RETLOC_POINTER (args[1]);
  
  *return_val = (* rfunc)(object, GTK_VALUE_POINTER(args[0]), func_data);
}

guint gnome_mdi_child_get_type () {
  static guint mdi_child_type = 0;

  if (!mdi_child_type) {
    GtkTypeInfo mdi_child_info = {
      "GnomeMDIChild",
      sizeof (GnomeMDIChild),
      sizeof (GnomeMDIChildClass),
      (GtkClassInitFunc) gnome_mdi_child_class_init,
      (GtkObjectInitFunc) gnome_mdi_child_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL,
    };
    
    mdi_child_type = gtk_type_unique (gtk_object_get_type (), &mdi_child_info);
  }
  
  return mdi_child_type;
}

static void gnome_mdi_child_class_init (GnomeMDIChildClass *class) {
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;

  object_class->destroy = gnome_mdi_child_destroy;

  mdi_child_signals[CREATE_VIEW] = gtk_signal_new ("create_view",
						  GTK_RUN_LAST,
						  object_class->type,
						  GTK_SIGNAL_OFFSET (GnomeMDIChildClass, create_view),
						  gnome_mdi_child_marshal_1,
						  GTK_TYPE_POINTER, 0);
  mdi_child_signals[CREATE_MENUS] = gtk_signal_new ("create_menus",
						  GTK_RUN_LAST,
						  object_class->type,
						  GTK_SIGNAL_OFFSET (GnomeMDIChildClass, create_menus),
						  gnome_mdi_child_marshal_2,
						  GTK_TYPE_POINTER, 1, GTK_TYPE_POINTER);

  gtk_object_class_add_signals (object_class, mdi_child_signals, LAST_SIGNAL);

  class->create_view = NULL;
  class->create_menus = NULL;

  parent_class = gtk_type_class (gtk_object_get_type ());
}

static void gnome_mdi_child_init (GnomeMDIChild *mdi_child) {
  mdi_child->name = NULL;
  mdi_child->views = NULL;
  mdi_child->changed = FALSE;
}

GnomeMDIChild *gnome_mdi_child_new () {
  GnomeMDIChild *mdi_child;

  mdi_child = gtk_type_new (gnome_mdi_child_get_type ());

  return mdi_child;
}

static void gnome_mdi_child_destroy(GtkObject *obj) {
  GnomeMDIChild *mdi_child;

#ifdef DEBUG
  printf("GnomeMDIChild: destroying!\n");
#endif

  mdi_child = GNOME_MDI_CHILD(obj);

  while(mdi_child->views)
    gnome_mdi_child_remove_view(mdi_child, GTK_WIDGET(mdi_child->views->data));

  if(mdi_child->name)
    free(mdi_child->name);

  if(GTK_OBJECT_CLASS(parent_class)->destroy)
    (* GTK_OBJECT_CLASS(parent_class)->destroy)(GTK_OBJECT(mdi_child));
}

GtkWidget *gnome_mdi_child_add_view(GnomeMDIChild *mdi_child) {
  GtkWidget *view;

  gtk_signal_emit (GTK_OBJECT (mdi_child), mdi_child_signals[CREATE_VIEW], &view);

  mdi_child->views = g_list_append(mdi_child->views, view);

  gtk_object_set_data(GTK_OBJECT(view), "GnomeMDIChild", mdi_child);

  gtk_widget_ref(view);

  return view;
}

void gnome_mdi_child_remove_view(GnomeMDIChild *mdi_child, GtkWidget *view) {
  mdi_child->views = g_list_remove(mdi_child->views, view);

  gtk_widget_destroy(view);
}

void gnome_mdi_child_set_name(GnomeMDIChild *mdi_child, gchar *name) {
  gchar *old_name = mdi_child->name;

  mdi_child->name = (gchar *)strdup(name);

  if(old_name)
    free(old_name);
}

void gnome_mdi_child_set_menu_template(GnomeMDIChild *mdi_child, GnomeUIInfo *menu_tmpl) {
  mdi_child->menu_template = menu_tmpl;
}






