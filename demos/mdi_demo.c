/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
#include <libgnome.h>
#include <libgnomeui/gnome-init.h>
#include <libgnomeui/gnome-mdi.h>
#include <libgnomeui/gnome-mdi-generic-child.h>
#include <libgnomeui/gnome-mdi-child.h>
#include <bonobo/bonobo-ui-component.h>

static GnomeMDI *mdi;
static gint child_counter = 1;

#define COUNTER_KEY "counter"

static void
exit_cb(GtkWidget *w, gpointer user_data)
{
	gtk_main_quit();
}

static void
quit_cmd (BonoboUIComponent *uic,
		  gpointer           user_data,
		  const char        *verbname)
{
	gtk_main_quit();
}

static void
mode_cb(GtkWidget *w, gpointer user_data)
{
	gpointer real_user_data;
	GnomeMDIMode mode;

	real_user_data = gtk_object_get_data(GTK_OBJECT(w), GNOMEUIINFO_KEY_UIDATA);
	mode = GPOINTER_TO_INT(real_user_data);
	gnome_mdi_set_mode(mdi, mode);
}

static GtkWidget *
view_creator(GnomeMDIChild *child, gpointer user_data)
{
	GtkWidget *label;

	label = gtk_label_new(gnome_mdi_child_get_name(child));
	
	return label;
}

static gchar *
child_config(GnomeMDIChild *child, gpointer user_data)
{
	gchar *conf;
	guint *counter;

	counter = gtk_object_get_data(GTK_OBJECT(child), COUNTER_KEY);
	if(counter)
		conf = g_strdup_printf("%d", *counter);
	else
		conf = NULL;

	return conf;
}

static void
add_view_cb(GtkWidget *w, gpointer user_data)
{
	GtkWidget *view;

	view = gnome_mdi_get_active_view(mdi);
	if(!view)
		return;
	gnome_mdi_add_view(mdi, gnome_mdi_get_child_from_view(view));
}

static void
remove_view_cb(GtkWidget *w, gpointer user_data)
{
	GtkWidget *view;

	view = gnome_mdi_get_active_view(mdi);
	if(!view)
		return;
	gnome_mdi_remove_view(mdi, view);
}

static void
increase_cb(GtkWidget *w, gpointer user_data)
{
	GtkWidget *view;
	GnomeMDIChild *child;
	const GList *view_node;
	gint *counter;
	gchar *name;

	view = gnome_mdi_get_active_view(mdi);
	if(!view)
		return;
	child = gnome_mdi_get_child_from_view(view);
	counter = gtk_object_get_data(GTK_OBJECT(child), COUNTER_KEY);
	(*counter) = child_counter++;
	name = g_strdup_printf("Child #%d", *counter);
	gnome_mdi_child_set_name(child, name);
	g_free(name);
	view_node = gnome_mdi_child_get_views(child);
	while(view_node) {
		view = view_node->data;
		gtk_label_set(GTK_LABEL(view), gnome_mdi_child_get_name(child));
		view_node = view_node->next;
	}
}

GnomeUIInfo real_child_menu[] =
{
	GNOMEUIINFO_ITEM("Add view", NULL, add_view_cb, NULL),
	GNOMEUIINFO_ITEM("Remove view", NULL, remove_view_cb, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_ITEM("Increase counter", NULL, increase_cb, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo child_menu[] =
{
	GNOMEUIINFO_SUBTREE("Child", real_child_menu),
	GNOMEUIINFO_END
};

static GnomeUIInfo child_toolbar[] =
{
	GNOMEUIINFO_ITEM_NONE(N_("Child Item 1"),
						  N_("Hint for item 1"),
						  NULL),
	GNOMEUIINFO_ITEM_NONE(N_("Child Item 2"),
						  N_("Hint for item 2"),
						  NULL),
	GNOMEUIINFO_END
};

static void
add_child_cmd (BonoboUIComponent *uic,
			   gpointer           user_data,
			   const char        *verbname)
{
	GnomeMDIGenericChild *child;
	gchar *name;
	gint *counter;

	name = g_strdup_printf("Child #%d", child_counter);
	child = gnome_mdi_generic_child_new(name);
	gnome_mdi_generic_child_set_view_creator(child, view_creator, NULL);
	gnome_mdi_generic_child_set_config_func(child, child_config, NULL);
	gnome_mdi_child_set_menu_template(GNOME_MDI_CHILD(child), child_menu);
	gnome_mdi_child_set_toolbar_template(GNOME_MDI_CHILD(child), child_toolbar);
	gnome_mdi_child_set_toolbar_position(GNOME_MDI_CHILD(child), GNOME_DOCK_ITEM_BEH_NORMAL,
										 GNOME_DOCK_TOP, 1, 1, 0);
	counter = g_malloc(sizeof(gint));
	*counter = child_counter++;
	gtk_object_set_data(GTK_OBJECT(child), COUNTER_KEY, counter);
	g_free(name);
	gnome_mdi_add_child(mdi, GNOME_MDI_CHILD(child));
	gnome_mdi_add_view(mdi, GNOME_MDI_CHILD(child));
}

static void
remove_child_cmd (BonoboUIComponent *uic,
				  gpointer           user_data,
				  const char        *verbname)
{
	GtkWidget *view;

	view = gnome_mdi_get_active_view(mdi);
	if(!view)
		return;
	gnome_mdi_remove_child(mdi, gnome_mdi_get_child_from_view(view));
}

GnomeUIInfo empty_menu[] =
{
	GNOMEUIINFO_END
};

GnomeUIInfo mode_list [] = 
{
	GNOMEUIINFO_RADIOITEM_DATA("Toplevel", NULL, mode_cb, GINT_TO_POINTER(GNOME_MDI_TOPLEVEL), NULL),
	GNOMEUIINFO_RADIOITEM_DATA("Notebook", NULL, mode_cb, GINT_TO_POINTER(GNOME_MDI_NOTEBOOK), NULL),
	GNOMEUIINFO_RADIOITEM_DATA("Window-in-Window", NULL, mode_cb, GINT_TO_POINTER(GNOME_MDI_WIW), NULL),
	GNOMEUIINFO_RADIOITEM_DATA("Modal", NULL, mode_cb, GINT_TO_POINTER(GNOME_MDI_MODAL), NULL),
	GNOMEUIINFO_END
};

GnomeUIInfo mode_menu[] =
{
	GNOMEUIINFO_RADIOLIST(mode_list),
	GNOMEUIINFO_END
};

GnomeUIInfo mdi_menu[] =
{
#if 0
	GNOMEUIINFO_ITEM("Add child", NULL, add_child_cb, NULL),
	GNOMEUIINFO_ITEM("Remove child", NULL, remove_child_cb, NULL),
#endif
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_SUBTREE("Mode", mode_menu),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_EXIT_ITEM(exit_cb, NULL),
	GNOMEUIINFO_END
};

GnomeUIInfo main_menu[] =
{
	GNOMEUIINFO_SUBTREE("MDI", mdi_menu),
	GNOMEUIINFO_SUBTREE("Children", empty_menu),
	GNOMEUIINFO_END
};

static const gchar *menu_xml = 
"<menu>\n"
"    <submenu name=\"MDI\" _label=\"_MDI\">\n"
"        <menuitem name=\"AddChild\" verb=\"\" _label=\"Add Child\"/>\n"
"        <menuitem name=\"RemoveChild\" verb=\"\" _label=\"Remove Child\"/>\n"
"        <separator/>"
"        <menuitem name=\"FileExit\" verb=\"\" _label=\"Exit\"/>\n"
"    </submenu>\n"
"    <submenu name=\"Children\" _label=\"_Children\">\n"
"        <placeholder/>\n"
"    </submenu>\n"
"</menu>\n";

BonoboUIVerb verbs [] = {
    BONOBO_UI_VERB ("FileExit", quit_cmd),

    BONOBO_UI_VERB ("AddChild", add_child_cmd),
    BONOBO_UI_VERB ("RemoveChild", remove_child_cmd),

    BONOBO_UI_VERB_END
};

static void
app_created_cb (GnomeMDI *mdi, BonoboWindow *win, BonoboUIComponent *component)
{
	bonobo_ui_component_add_verb_list (component, verbs);
}

int
main(int argc, char **argv)
{
  gnome_program_init ("mdi_demo", "2.0", &libgnomeui_module_info,
					  argc, argv, NULL);

#if 1
    g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
#endif

  mdi = gnome_mdi_new("MDIDemo", "MDI Demo");
  gnome_mdi_set_mode(mdi, GNOME_MDI_TOPLEVEL);
  gnome_mdi_set_menubar_template(mdi, menu_xml);
  gnome_mdi_set_child_menu_path(mdi, "MDI");
  gnome_mdi_set_child_list_path(mdi, "/menu/Children");
  g_signal_connectc(G_OBJECT(mdi), "app_created",
					G_CALLBACK(app_created_cb), NULL, FALSE);
  gtk_signal_connect(GTK_OBJECT(mdi), "destroy",
					 GTK_SIGNAL_FUNC(exit_cb), NULL);
  gnome_mdi_open_toplevel(mdi);

  gtk_main ();

  return 0;
}
