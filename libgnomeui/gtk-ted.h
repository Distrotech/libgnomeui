#ifndef __GTK_TED_H__
#define __GTK_TED_H__

#include <gdk/gdk.h>
#include <gtk/gtktable.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define GTK_TED(obj)          GTK_CHECK_CAST (obj, gtk_ted_get_type (), GtkTed)
#define GTK_TED_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_ted_get_type (), GtkTedClass)
#define GTK_IS_TED(obj)       GTK_CHECK_TYPE (obj, gtk_ted_get_type ())

typedef struct _GtkTed           GtkTed;
typedef struct _GtkTedClass      GtkTedClass;

guint		  gtk_ted_get_type        (void);
void              gtk_ted_set_app_name    (char *str);
void              gtk_ted_prepare         (GtkTed *ted);
GtkWidget         *gtk_ted_new            (char *dialog_name);
GtkWidget         *gtk_ted_new_layout     (char *dialog_name, char *layout);
void              gtk_ted_add             (GtkTed *ted, GtkWidget *widget, char *name);

struct _GtkTed
{
	GtkTable   table;

	GHashTable *widgets;
	GtkWidget  *widget_box;
	int        last_col, last_row;
	int        top_col,  top_row;
	char       *dialog_name;
	int        need_gui;
	int        in_gui;
};

struct _GtkTedClass {
	GtkTableClass parent_class;
};

END_GNOME_DECLS

#endif /* __GTK_TED_H__ */
