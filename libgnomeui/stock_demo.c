#include <gnome.h>


static GtkWidget *menu_items[20];


static GtkWidget *
create_menu(GtkWidget *window)
{
        GtkWidget *menubar, *w, *menu;
        GtkAcceleratorTable *accel;
	int i = 0;

        accel = gtk_accelerator_table_new();
        menubar = gtk_menu_bar_new();
        gtk_widget_show(menubar);

        menu = gtk_menu_new();

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_NEW, _("New..."));
        gtk_widget_show(w);
        gtk_widget_install_accelerator(w, accel, "activate",
                                       'N', GDK_CONTROL_MASK);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_OPEN, _("Open..."));
        gtk_widget_show(w);
        gtk_widget_install_accelerator(w, accel, "activate",
                                       'O', GDK_CONTROL_MASK);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_SAVE, _("Save"));
        gtk_widget_show(w);
        gtk_widget_install_accelerator(w, accel, "activate",
                                       'S', GDK_CONTROL_MASK);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_BLANK, _("Save as..."));
        gtk_widget_show(w);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_BLANK, _("Print..."));
        gtk_widget_show(w);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_BLANK, _("Setup Page..."));
        gtk_widget_show(w);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_EXIT, _("Exit"));
        gtk_widget_show(w);
        gtk_widget_install_accelerator(w, accel, "activate",
                                       'Q', GDK_CONTROL_MASK);
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  (GtkSignalFunc)gtk_widget_destroy,
				  GTK_OBJECT(window));
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

        w = gtk_menu_item_new_with_label(_("File"));
        gtk_widget_show(w);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
        gtk_menu_bar_append(GTK_MENU_BAR(menubar), w);
	
        menu = gtk_menu_new();

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_BLANK, _("Undo"));
        gtk_widget_show(w);
        gtk_widget_install_accelerator(w, accel, "activate",
                                       'Z', GDK_CONTROL_MASK);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_BLANK, _("Redo"));
        gtk_widget_show(w);
        gtk_widget_install_accelerator(w, accel, "activate",
                                       'R', GDK_CONTROL_MASK);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_BLANK, _("Delete"));
        gtk_widget_show(w);
        gtk_widget_install_accelerator(w, accel, "activate",
                                       'D', GDK_MOD1_MASK);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_CUT, _("Cut"));
        gtk_widget_show(w);
        gtk_widget_install_accelerator(w, accel, "activate",
                                       'X', GDK_CONTROL_MASK);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_COPY, _("Copy"));
        gtk_widget_show(w);
        gtk_widget_install_accelerator(w, accel, "activate",
                                       'C', GDK_CONTROL_MASK);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_PASTE, _("Paste"));
        gtk_widget_show(w);
        gtk_widget_install_accelerator(w, accel, "activate",
                                       'V', GDK_CONTROL_MASK);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_PROP, _("Properties..."));
        gtk_widget_show(w);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

        w = gtk_menu_item_new_with_label(_("Edit"));
        gtk_widget_show(w);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(w));
        gtk_menu_bar_append(GTK_MENU_BAR(menubar), w);

        menu = gtk_menu_new();

        w = gnome_stock_menu_item(GNOME_STOCK_MENU_ABOUT, _("About"));
        gtk_widget_show(w);
        gtk_widget_install_accelerator(w, accel, "activate",
                                       'A', GDK_CONTROL_MASK);
        gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

        w = gtk_menu_item_new_with_label(_("Help"));
        gtk_widget_show(w);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(w));
        gtk_menu_bar_append(GTK_MENU_BAR(menubar), w);

	menu_items[i] = NULL;
	gtk_window_add_accelerator_table(GTK_WINDOW(window), accel);
	return menubar;
}



typedef struct _TbItems TbItems;
struct _TbItems {
	char *icon;
	GtkWidget *widget; /* will be filled in */
};

static TbItems tb_items[] = {
	{GNOME_STOCK_PIXMAP_NEW, NULL},
	{GNOME_STOCK_PIXMAP_SAVE, NULL},
	{GNOME_STOCK_PIXMAP_OPEN, NULL},
	{GNOME_STOCK_PIXMAP_CUT, NULL},
	{GNOME_STOCK_PIXMAP_COPY, NULL},
	{GNOME_STOCK_PIXMAP_PASTE, NULL},
	{GNOME_STOCK_PIXMAP_PROPERTIES, NULL},
	{NULL, NULL}
};

static GtkWidget *
create_toolbar(GtkWidget *window)
{
	GtkWidget *toolbar;
	TbItems *t;

	toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,
				  GTK_TOOLBAR_BOTH);

	for (t = &tb_items[0]; t->icon; t++) {
		t->widget = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
						    t->icon,
						    NULL,
						    NULL,
						    gnome_stock_pixmap_widget(window, t->icon),
						    NULL, NULL);
	}

	return toolbar;
}



static void
tb_sens(GtkWidget *w, gpointer *data)
{
	TbItems *t;
	GtkWidget **m;

	for (t = tb_items; t->icon; t++) {
		gtk_widget_set_sensitive(t->widget, TRUE);
	}
	for (m = menu_items; *m; m++) {
		gtk_widget_set_sensitive(*m, TRUE);
	}
}



static void
tb_insens(GtkWidget *w, gpointer *data)
{
	TbItems *t;
	GtkWidget **m;

	for (t = tb_items; t->icon; t++) {
		gtk_widget_set_sensitive(t->widget, FALSE);
	}
	for (m = menu_items; *m; m++) {
		gtk_widget_set_sensitive(*m, FALSE);
	}
}



void
main(int argc, char **argv)
{
	GtkWidget *window, *hbox, *vbox, *w;

	gnome_init("stock_demo", &argc, &argv);
#ifdef HAS_GDK_IMLIB
	gdk_imlib_init();
#endif 

	window = gnome_app_new("Gnome Stock Test", "Gnome Stock Test");
	gtk_window_set_wmclass(GTK_WINDOW(window), "stock_test",
			       "GnomeStockTest");

	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
			   GTK_SIGNAL_FUNC(gtk_widget_destroy), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy",
			   GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	w = gtk_label_new("Click on `Cancel' to disable the toolbar and menu items\n"
			  "Click on `OK' to enable the toolbar and menu items\n"
			  "Select File->Exit to exit the app");
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 2);

	hbox = gtk_hbox_new(TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 5);
	gtk_widget_show(hbox);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);
	gtk_signal_connect(GTK_OBJECT(w), "clicked",
			   (GtkSignalFunc)tb_sens, NULL);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_APPLY);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);
	gtk_widget_set_sensitive(w, FALSE);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);
	gtk_signal_connect(GTK_OBJECT(w), "clicked",
			   (GtkSignalFunc)tb_insens, NULL);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_HELP);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);

	gnome_app_set_contents(GNOME_APP(window), vbox);
	gnome_app_set_menus(GNOME_APP(window),
			    GTK_MENU_BAR(create_menu(window)));
	gnome_app_set_toolbar(GNOME_APP(window),
			      GTK_TOOLBAR(create_toolbar(window)));
	gtk_widget_show(window);
	gtk_main();
}
