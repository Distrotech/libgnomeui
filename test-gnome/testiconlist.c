#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <libgnomeui.h>

static void
select_icon (GtkWidget *widget, gint num, GdkEvent *event)
{
	g_print ("selected icon %d\n", num);
}

static void
unselect_icon (GtkWidget *widget, gint num, GdkEvent *event)
{
	g_print ("unselected icon %d\n", num);
}

gint
main (gint argc, gchar **argv)
{
	GnomeProgram *program;
	GtkWidget *window, *scrolled_window, *icon_list, *vbox, *button;
	GSList *ids, *list;
	gint i;
	
	program = gnome_program_init ("testiconlist", "0.0",
			    LIBGNOMEUI_MODULE,
			    argc, argv, NULL);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_ETCHED_IN);
	
	icon_list = gnome_icon_list_new (80, NULL, GNOME_ICON_LIST_IS_EDITABLE);
	gtk_container_add (GTK_CONTAINER (scrolled_window), icon_list);
	
	g_signal_connect (icon_list, "select_icon",
			  G_CALLBACK (select_icon), NULL);
	g_signal_connect (icon_list, "unselect_icon",
			  G_CALLBACK (unselect_icon), NULL);
	
	gnome_icon_list_set_selection_mode (GNOME_ICON_LIST (icon_list), GTK_SELECTION_EXTENDED);

	ids = gtk_stock_list_ids ();


	gnome_icon_list_append_pixbuf (GNOME_ICON_LIST (icon_list),
				       gtk_widget_render_icon (icon_list, GTK_STOCK_OK,
							       GTK_ICON_SIZE_BUTTON,
							       NULL),
				       "slirpe", "What happens if we have a long long line here?");

	gnome_icon_list_append_pixbuf (GNOME_ICON_LIST (icon_list),
				       gtk_widget_render_icon (icon_list, GTK_STOCK_OK,
							       GTK_ICON_SIZE_BUTTON,
							       NULL),
				       "slirpe", "What happens if we have a longlonglongword here?");

	for (list = ids, i = 0; list; list = list->next, i++) {
		gchar *name = list->data;

		if (i > 2)
			break;
		
		gnome_icon_list_append_pixbuf (GNOME_ICON_LIST (icon_list),
					       gtk_widget_render_icon (icon_list, name,
								       GTK_ICON_SIZE_BUTTON,
								       NULL),
					       name, "Sliff! السلام عليكم");
		g_free (name);

	}

	button = gtk_button_new_from_stock (GTK_STOCK_QUIT);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

	gtk_widget_show_all (window);

	gtk_main ();

	g_object_unref (program);
	
	return 0;
}
