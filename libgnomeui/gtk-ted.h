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
void              gtk_ted_set_app_name    (const gchar *str);
void              gtk_ted_prepare         (GtkTed *ted);
GtkWidget         *gtk_ted_new            (const gchar *dialog_name);
GtkWidget         *gtk_ted_new_layout     (const gchar *dialog_name, const gchar *layout);
void              gtk_ted_add             (GtkTed *ted, GtkWidget *widget, const gchar *name);

struct _GtkTed
{
	GtkTable   table;

	GHashTable *widgets;
	GtkWidget  *widget_box;
	gint        last_col, last_row;
	gint        top_col,  top_row;
	gchar       *dialog_name;
	gint        need_gui;
	gint        in_gui;
};

struct _GtkTedClass {
	GtkTableClass parent_class;
};

END_GNOME_DECLS

#endif /* __GTK_TED_H__ */
