#ifndef GNOME_HELPSYS_H
#define GNOME_HELPSYS_H 1

#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-url.h>

#include <gtk/gtk.h>
#include <libgnomeui/gnome-dock-item.h>

BEGIN_GNOME_DECLS

#define GNOME_HELP_VIEW(obj) GTK_CHECK_CAST(obj, gnome_help_view_get_type(), GnomeHelpView)
#define GNOME_HELP_VIEW_CLASS(class) GTK_CHECK_CLASS_CAST(class, gnome_help_view_get_type(), GnomeHelpViewClass)
#define GNOME_IS_HELP_VIEW(obj) GTK_CHECK_TYPE(obj, gnome_help_view_get_type())

typedef enum {
	GNOME_HELP_POPUP,
	GNOME_HELP_EMBEDDED,
	GNOME_HELP_BROWSER
} GnomeHelpViewStyle;
typedef int GnomeHelpViewStylePriority; /* Use the G_PRIORITY_* system */

typedef struct _GnomeHelpViewClass GnomeHelpViewClass;
typedef struct _GnomeHelpView GnomeHelpView;

struct _GnomeHelpView {
  GtkBox parent_object;

  GtkWidget *toplevel;
  GtkWidget *toolbar, *content, *btn_help, *btn_style, *evbox;

  GnomeURLDisplayContext url_ctx;

  GtkOrientation orientation;

  GnomeHelpViewStylePriority style_prio;
  GnomeHelpViewStylePriority app_style_priority;

  GnomeHelpViewStyle style : 2;

  GnomeHelpViewStyle app_style : 2; /* Used to properly handle object args for style & prio */ 
};

struct _GnomeHelpViewClass {
  GtkBoxClass parent_class;
  GtkWidget *popup_menu;
};

GtkType gnome_help_view_get_type(void);
GtkWidget *gnome_help_view_new(GtkWidget *toplevel, GnomeHelpViewStyle app_style,
			       GnomeHelpViewStylePriority app_style_priority);
GnomeHelpView *gnome_help_view_find(GtkWidget *awidget);
void gnome_help_view_set_visibility(GnomeHelpView *help_view, gboolean visible);
void gnome_help_view_set_style(GnomeHelpView *help_view, GnomeHelpViewStyle style, GnomeHelpViewStylePriority style_priority);
void gnome_help_view_select_help_cb(GtkWidget *ignored, GnomeHelpView *help_view);

/* as above, but does a lookup to try and figure out what the help_view is */
void gnome_help_view_select_help_menu_cb(GtkWidget *widget);

void gnome_help_view_show_help(GnomeHelpView *help_view, const char *help_path, const char *help_type);
void gnome_help_view_show_help_for(GnomeHelpView *help_view, GtkWidget *widget);
void gnome_help_view_set_orientation(GnomeHelpView *help_view, GtkOrientation orientation);
void gnome_help_view_display (GnomeHelpView *help_view, const char *help_path);
void gnome_help_view_display_callback (GtkWidget *widget, const char *help_path);

/*** Non-widget routines ***/

/* Turns a help path into a full URL */
char *gnome_help_path_resolve(const char *path, const char *file_type);
GSList *gnome_help_app_topics(const char *app_id);

/* Object data on toplevel, name */
#define GNOME_APP_HELP_VIEW_NAME "HelpView"

/*** Utility routines ***/
void gnome_widget_set_tooltip (GtkWidget *widget, const char *tiptext);
GtkWidget *gnome_widget_set_name(GtkWidget *widget, const char *name);

#define TT_(widget, text) gnome_widget_set_tooltip((widget), (text))
#define H_(widget, help_id) gnome_widget_set_name((widget), (help_id))

END_GNOME_DECLS

#endif
