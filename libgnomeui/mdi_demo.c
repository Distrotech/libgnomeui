/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
#include <gnome.h>

static GnomeMDI *mdi;
static gint child_counter = 1;

#define COUNTER_KEY "counter"

static void
exit_cb(GtkWidget *w, gpointer user_data)
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
add_child_cb(GtkWidget *w, gpointer user_data)
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
remove_child_cb(GtkWidget *w, gpointer user_data)
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
	GNOMEUIINFO_ITEM("Add child", NULL, add_child_cb, NULL),
	GNOMEUIINFO_ITEM("Remove child", NULL, remove_child_cb, NULL),
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

int
main(int argc, char **argv)
{
#if 0
  gnome_init ("mdi_demo", "1.0", argc, argv);
#else
  gnome_program_init ("mdi_demo", "2.0", argc, argv, GNOMEUI_INIT, NULL);
#endif

  mdi = gnome_mdi_new("MDIDemo", "MDI Demo");
  gnome_mdi_set_mode(mdi, GNOME_MDI_TOPLEVEL);
  gnome_mdi_set_menubar_template(mdi, main_menu);
  gnome_mdi_set_child_menu_path(mdi, "MDI");
  gnome_mdi_set_child_list_path(mdi, "Children/");
  gtk_signal_connect(GTK_OBJECT(mdi), "destroy",
					 GTK_SIGNAL_FUNC(exit_cb), NULL);
  gnome_mdi_open_toplevel(mdi);

  gtk_main ();

  return 0;
}
