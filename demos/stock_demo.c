#include <config.h>
#include <gnome.h>
#include <string.h>



static void
message_dlg(GtkWidget *widget, gpointer data)
{
	GnomeMessageBox *box;

	box = GNOME_MESSAGE_BOX (gnome_message_box_new ("Really quit?",
							"question",
				     			GNOME_STOCK_BUTTON_YES,
				     			GNOME_STOCK_BUTTON_NO,
				     			NULL));
	/* Quit no matter what.  */
	gtk_signal_connect (GTK_OBJECT (box), "clicked",
			    GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

	gnome_message_box_set_modal (box);
	gtk_widget_show (GTK_WIDGET (box));
}



static void
prop_apply(GtkWidget *box, int n)
{
	char s[256];

	if (n != -1) {
		sprintf(s, "Applied changed on page #%d", n + 1);
		gtk_widget_show(gnome_message_box_new(s, "info",
				GNOME_STOCK_BUTTON_OK, NULL));
	}
}


static void
prop_dlg(GtkWidget *widget, gpointer data)
{
	GnomePropertyBox *box;
	GtkWidget *w, *label;

	box = GNOME_PROPERTY_BOX(gnome_property_box_new());
	w = gtk_button_new_with_label("Click me (Page #1)");
	gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed),
				  GTK_OBJECT(box));
	gtk_widget_show(w);
	label = gtk_label_new("Page #1");
	gtk_widget_show(label);
	gnome_property_box_append_page(box, w, label);
	w = gtk_button_new_with_label("Click me (Page #2)");
	gtk_widget_show(w);
	gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed),
				  GTK_OBJECT(box));
	label = gtk_label_new("Page #2");
	gtk_widget_show(label);
	gnome_property_box_append_page(box, w, label);
	gtk_signal_connect(GTK_OBJECT(box), "apply",
			   (GtkSignalFunc)prop_apply, NULL);
	gtk_widget_show(GTK_WIDGET(box));
}


static GtkWidget *menu_items[20], *frame;


static GtkWidget *
create_menu(GtkWidget *window)
{
	void gnome_stock_menu_accel_dlg(char *);
	GtkWidget *menubar, *w, *menu;
	GtkAcceleratorTable *accel;
	int i = 0;
	guchar key;
	guint8 mod;

	accel = gtk_accelerator_table_new();
	menubar = gtk_menu_bar_new();
	gtk_widget_show(menubar);

	menu = gtk_menu_new();

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_NEW, _("New..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_NEW, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_OPEN, _("Open..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_OPEN, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_SAVE, _("Save"));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_SAVE, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_SAVE_AS, _("Save as..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_SAVE_AS, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_PRINT, _("Print..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_PRINT, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_BLANK, _("Setup Page..."));
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_QUIT, _("Quit"));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_QUIT, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  (GtkSignalFunc)message_dlg,
				  GTK_OBJECT(window));
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new_with_label(_("File"));
	gtk_widget_show(w);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), w);
	
	menu = gtk_menu_new();

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_UNDO, _("Undo"));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_UNDO, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_BLANK, _("Redo"));
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_BLANK, _("Delete"));
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_CUT, _("Cut"));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_CUT, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_COPY, _("Copy"));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_COPY, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_PASTE, _("Paste"));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_PASTE, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_PROP, _("Properties..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_PROP, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_signal_connect(GTK_OBJECT(w), "activate",
			   GTK_SIGNAL_FUNC(prop_dlg), NULL);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_PREF, _("Preferences..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_PREF, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_signal_connect_object(GTK_OBJECT(w), "activate",
				  GTK_SIGNAL_FUNC(gnome_stock_menu_accel_dlg),
				  NULL);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_SCORES, _("Scores..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_SCORES, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
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
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_ABOUT, &key, &mod))
		gtk_widget_install_accelerator(w, accel, "activate", key, mod);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new_with_label(_("Help"));
	gtk_widget_show(w);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(w));
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), w);

	menu_items[i] = NULL;
	/* g_print("%d menu items\n", i); */
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
	{GNOME_STOCK_PIXMAP_SAVE_AS, NULL},
	{GNOME_STOCK_PIXMAP_OPEN, NULL},
	{GNOME_STOCK_PIXMAP_CUT, NULL},
	{GNOME_STOCK_PIXMAP_COPY, NULL},
	{GNOME_STOCK_PIXMAP_PASTE, NULL},
#define TB_PROP 7
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
	gtk_signal_connect(GTK_OBJECT(tb_items[TB_PROP].widget), "clicked",
				      (GtkSignalFunc)prop_dlg, NULL);

	return toolbar;
}



static void
tb_sens(GtkWidget *w, gpointer *data)
{
	TbItems *t;
	GtkWidget **m;

	gtk_widget_set_sensitive(frame, TRUE);
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

	gtk_widget_set_sensitive(frame, FALSE);
	for (t = tb_items; t->icon; t++) {
		gtk_widget_set_sensitive(t->widget, FALSE);
	}
	for (m = menu_items; *m; m++) {
		gtk_widget_set_sensitive(*m, FALSE);
	}
}



static void
toggle_button(GtkWidget *button, GnomeStockPixmapWidget *w)
{
	if (!w) return;
	if (!w->icon) return;
	if (0 == strcmp(w->icon, GNOME_STOCK_PIXMAP_TIMER))
		gnome_stock_pixmap_widget_set_icon(w, GNOME_STOCK_PIXMAP_TIMER_STOP);
	else
		gnome_stock_pixmap_widget_set_icon(w, GNOME_STOCK_PIXMAP_TIMER);
}



static void
fill_table(GtkWidget *window, GtkTable *table)
{
	GtkWidget *w, *button;

	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_HELP));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 1, 0, 1);
	w = gtk_label_new("Help");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 1, 1, 2);

	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_SEARCH));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 1, 2, 0, 1);
	w = gtk_label_new("Search");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 1, 2, 1, 2);

	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_PRINT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 2, 3, 0, 1);
	w = gtk_label_new("Print");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 2, 3, 1, 2);

	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_BACK));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 3, 4, 0, 1);
	w = gtk_label_new("Backward");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 3, 4, 1, 2);

	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_FORWARD));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 4, 5, 0, 1);
	w = gtk_label_new("Forward");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 4, 5, 1, 2);

	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_PREFERENCES));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 5, 6, 0, 1);
	w = gtk_label_new("Preferences");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 5, 6, 1, 2);

	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_UNDO));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 6, 7, 0, 1);
	w = gtk_label_new("Undo");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 6, 7, 1, 2);

	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_SEARCH));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 1, 2, 2, 3);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_BACK));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 3, 4, 2, 3);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_FORWARD));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 4, 5, 2, 3);

	button = gtk_button_new();
	gtk_widget_show(button);
	w = gnome_stock_pixmap_widget(button, GNOME_STOCK_PIXMAP_TIMER);
	gtk_widget_show(w);
	gtk_container_add(GTK_CONTAINER(button), w);
	gtk_table_attach_defaults(table, button, 0, 1, 2, 3);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(toggle_button), w);

	button = gtk_toggle_button_new();
	gtk_widget_show(button);
	w = gnome_stock_pixmap_widget(button, GNOME_STOCK_PIXMAP_TIMER);
	gtk_widget_show(w);
	gtk_container_add(GTK_CONTAINER(button), w);
	gtk_table_attach_defaults(table, button, 2, 3, 2, 3);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(toggle_button), w);
}



void
main(int argc, char **argv)
{
	GtkWidget *window, *hbox, *vbox, *table, *w;

	gnome_init("stock_demo", NULL, argc, argv, 0, NULL);
	textdomain(PACKAGE);

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
			  "Select Edit->Properties or the Properties toolbar button\n"
			  "to open a GnomePropertyBox\n"
			  "Select File->Quit or the Close button to exit the app");
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 2);

	frame = gtk_frame_new("Other Icons");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 2);
	table = gtk_table_new(1, 1, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	fill_table(window, GTK_TABLE(table));

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 5);
	gtk_widget_show(hbox);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_CLOSE);
	gtk_widget_show(w);
	gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 3);
	gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
				  (GtkSignalFunc)gtk_widget_destroy,
				  GTK_OBJECT(window));

	w = gnome_stock_button(GNOME_STOCK_BUTTON_HELP);
	gtk_widget_show(w);
	gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 3);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_NO);
	gtk_widget_show(w);
	gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 3);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_YES);
	gtk_widget_show(w);
	gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 3);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
	gtk_widget_show(w);
	gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 3);
	gtk_signal_connect(GTK_OBJECT(w), "clicked",
			   (GtkSignalFunc)tb_insens, NULL);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_APPLY);
	gtk_widget_show(w);
	gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 3);
	gtk_widget_set_sensitive(w, FALSE);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
	gtk_widget_show(w);
	gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 3);
	gtk_signal_connect(GTK_OBJECT(w), "clicked",
			   (GtkSignalFunc)tb_sens, NULL);

	gnome_app_set_contents(GNOME_APP(window), vbox);
	gnome_app_set_menus(GNOME_APP(window),
			    GTK_MENU_BAR(create_menu(window)));
	gnome_app_set_toolbar(GNOME_APP(window),
			      GTK_TOOLBAR(create_toolbar(window)));
	gtk_widget_show(window);
	gtk_main();
}
