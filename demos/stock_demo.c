#include <config.h>
#include <gnome.h>
#include <string.h>


/*
 * message box demo
 */

static void
message_dlg_clicked(GtkWidget *widget, int button,gpointer data)
{
	if (button == 0) /* Yes */
		gtk_main_quit();
	else /* No */
		gnome_dialog_close(GNOME_DIALOG(widget));
}

static gboolean
message_dlg(GtkWidget *widget, gpointer data)
{
	static GtkWidget *box = NULL;

	if (box == NULL) {
	  box = gnome_message_box_new ("Really quit?",
				       GNOME_MESSAGE_BOX_QUESTION,
				       GNOME_STOCK_BUTTON_YES,
				       GNOME_STOCK_BUTTON_NO,
				       NULL);
	  gtk_signal_connect (GTK_OBJECT (box), "clicked",
			      GTK_SIGNAL_FUNC (message_dlg_clicked), NULL);

	  gtk_window_set_modal (GTK_WINDOW(box),TRUE);
	  gnome_dialog_close_hides(GNOME_DIALOG(box), TRUE);
	}
	gtk_widget_show (box);
	return TRUE;
}



/*
 * property box demo
 */

static void
prop_apply(GtkWidget *box, int n)
{
	char s[256];

	if (n != -1) {
		g_snprintf(s, sizeof(s), "Applied changed on page #%d", n + 1);
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



/*
 * main window
 */

static GtkWidget *menu_items[20], *frame;


static GtkWidget *
create_menu(GtkWidget *window)
{
	void gnome_stock_menu_accel_dlg(char *);
	GtkWidget *menubar, *w, *menu;
	GtkAccelGroup *accel;
	int i = 0;
	guchar key;
	guint8 mod;

	accel = gtk_accel_group_new();
	menubar = gtk_menu_bar_new();
	gtk_widget_show(menubar);

	menu = gtk_menu_new();

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_NEW, _("New..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_NEW, &key, &mod))
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_OPEN, _("Open..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_OPEN, &key, &mod))
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_SAVE, _("Save"));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_SAVE, &key, &mod))
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_SAVE_AS, _("Save as..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_SAVE_AS, &key, &mod))
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_CLOSE, _("Close"));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_CLOSE, &key, &mod))
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_PRINT, _("Print..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_PRINT, &key, &mod))
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

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
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

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
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_REDO, _("Redo"));
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
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_COPY, _("Copy"));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_COPY, &key, &mod))
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_PASTE, _("Paste"));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_PASTE, &key, &mod))
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_PROP, _("Properties..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_PROP, &key, &mod))
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

	gtk_signal_connect(GTK_OBJECT(w), "activate",
			   GTK_SIGNAL_FUNC(prop_dlg), NULL);
	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gnome_stock_menu_item(GNOME_STOCK_MENU_PREF, _("Preferences..."));
	gtk_widget_show(w);
	if (gnome_stock_menu_accel(GNOME_STOCK_MENU_PREF, &key, &mod))
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

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
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

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
		gtk_widget_add_accelerator(w, "activate",  accel, key, mod, 0);

	gtk_menu_append(GTK_MENU(menu), w);
	menu_items[i++] = w;

	w = gtk_menu_item_new_with_label(_("Help"));
	gtk_widget_show(w);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(w));
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), w);

	menu_items[i] = NULL;
	/* g_print("%d menu items\n", i); */
	gtk_window_add_accel_group(GTK_WINDOW(window), accel);

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



#if USE_NEW_GNOME_STOCK
static void
toggle_button(GtkWidget *button, GnomeStock *w)
{
	if (!w) return;
	if (!w->icon) return;
	if (0 == strcmp(w->icon, GNOME_STOCK_PIXMAP_TIMER))
		gnome_stock_set_icon(w, GNOME_STOCK_PIXMAP_TIMER_STOP);
	else
		gnome_stock_set_icon(w, GNOME_STOCK_PIXMAP_TIMER);
}
#else /* °USE_NEW_GNOME_STOCK */
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
#endif /* !USE_NEW_GNOME_STOCK */



#undef USE_BUTTON


static void
fill_table(GtkWidget *window, GtkTable *table)
{
	GtkWidget *w, *button;
	gint row, column;

	row = column = 0;

	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_HELP));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = gtk_label_new("Help");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_SEARCH));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_SEARCH));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Search");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_SRCHRPL));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_SRCHRPL));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Search/Replace");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_CLOSE));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = gtk_label_new("Close");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_PRINT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = gtk_label_new("Print");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_PREFERENCES));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = gtk_label_new("Preferences");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_UNDO));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = gtk_label_new("Undo");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_REDO));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = gtk_label_new("Redo");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_REVERT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_REVERT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Revert");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_SPELLCHECK));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_SPELLCHECK));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Spellchecker");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column = 0; row += 3;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_BACK));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_BACK));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Backward");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_FORWARD));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_FORWARD));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Forward");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_FIRST));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_FIRST));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("First");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_LAST));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_LAST));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Last");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_UP));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_UP));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Up");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_DOWN));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_DOWN));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Down");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_TOP));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_TOP));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Top");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_BOTTOM));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_BOTTOM));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Bottom");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_EXEC));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_EXEC));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Exec");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	row += 3;
	column = 0;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_ALIGN_LEFT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_ALIGN_LEFT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Left");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_ALIGN_RIGHT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_ALIGN_RIGHT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Right");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_ALIGN_CENTER));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_ALIGN_CENTER));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Center");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_ALIGN_JUSTIFY));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_ALIGN_JUSTIFY));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Justify");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	row += 3;
	column = 0;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_HOME));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_HOME));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Home");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_STOP));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_STOP));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Stop");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_REFRESH));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_REFRESH));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Refresh");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_INDEX));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_INDEX));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Index");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_MIC));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_MIC));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Microphone");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_VOLUME));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_VOLUME));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Volume");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_LINE_IN));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_LINE_IN));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Line In");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);
        column++;
        w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_CDROM));
        gtk_widget_show(w);
        gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
        w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_CDROM));
        gtk_widget_show(w);
        gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
        w = gtk_label_new("CD Rom");
        gtk_widget_show(w);
        gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);


	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_FONT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_FONT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Font");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column = 0; row += 3;
	button = gtk_button_new();
	gtk_widget_show(button);
	w = gnome_stock_pixmap_widget(button, GNOME_STOCK_PIXMAP_TIMER_STOP);
	gtk_widget_show(w);
	gtk_container_add(GTK_CONTAINER(button), w);
	gtk_table_attach_defaults(table, button, column, column + 1, row, row + 1);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(toggle_button), w);

	button = gtk_toggle_button_new();
	gtk_widget_show(button);
	w = gnome_stock_pixmap_widget(button, GNOME_STOCK_PIXMAP_TIMER);
	gtk_widget_show(w);
	gtk_container_add(GTK_CONTAINER(button), w);
	gtk_table_attach_defaults(table, button, column, column + 1, row + 1, row + 3);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(toggle_button), w);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_MAIL));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_MAIL));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Mail");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_MAIL_NEW));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_MAIL_NEW));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("New Mail");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_MAIL_SND));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_MAIL_SND));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Send Mail");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_MAIL_RCV));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_MAIL_RCV));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Receive Mail");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_MAIL_RPL));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_MAIL_RPL));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Reply to Mail");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_MAIL_FWD));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_MAIL_FWD));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Forward Mail");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_ATTACH));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_ATTACH));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Attach");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_CONVERT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_CONVERT));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Convert");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_JUMP_TO));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_JUMP_TO));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Jump To");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column = 0; row += 3;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_BOOK_RED));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_BOOK_RED));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Book Red");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_BOOK_GREEN));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_BOOK_GREEN));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Book Green");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_BOOK_BLUE));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_BOOK_BLUE));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Book Blue");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_BOOK_YELLOW));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_BOOK_YELLOW));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Book Yellow");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_BOOK_OPEN));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_BOOK_OPEN));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Book Open");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_TRASH));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_TRASH));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Trash");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_UNDELETE));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_UNDELETE));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Undelete");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_PIXMAP_TRASH_FULL));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, GNOME_STOCK_MENU_TRASH_FULL));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
	w = gtk_label_new("Trash Full");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 2, row + 3);

#undef COMPARE_TO_IMLIB
#undef TRY_ENLARGE

#define FILENAME "gnome-audio2.png"
#ifndef COMPARE_TO_IMLIB
	column++;
#else
	row += 3;
	column = 0;
#endif
	w = GTK_WIDGET(gnome_stock_pixmap_widget(window, FILENAME));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 2);
	w = gtk_label_new(FILENAME);
	gtk_widget_show(w);
#ifdef TRY_ENLARGE
	gtk_table_attach_defaults(table, w, column, column + 3, row + 2, row + 3);
#else
	gtk_table_attach_defaults(table, w, column, column + 2, row + 2, row + 3);
#endif
	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget_at_size(window, FILENAME, 24, 24));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_stock_pixmap_widget_at_size(window, FILENAME, 16, 16));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
#ifdef TRY_ENLARGE
	/* XXX. yikes, that might crash */
	column++;
	w = GTK_WIDGET(gnome_stock_pixmap_widget_at_size(window, FILENAME, 64, 64));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 2);
#endif
#ifdef COMPARE_TO_IMLIB
#define FILENAME2 (gnome_pixmap_file(FILENAME))
	column++;
	w = GTK_WIDGET(gnome_pixmap_new_from_file(FILENAME2));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 2);
	w = gtk_label_new(FILENAME " via Imlib");
	gtk_widget_show(w);
#ifdef TRY_ENLARGE
	gtk_table_attach_defaults(table, w, column, column + 3, row + 2, row + 3);
#else
	gtk_table_attach_defaults(table, w, column, column + 2, row + 2, row + 3);
#endif
	column++;
	w = GTK_WIDGET(gnome_pixmap_new_from_file_at_size(FILENAME2, 24, 24));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = GTK_WIDGET(gnome_pixmap_new_from_file_at_size(FILENAME2, 16, 16));
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
#ifdef TRY_ENLARGE
	column++;
	w = gnome_pixmap_new_from_file_at_size(FILENAME2, 64, 64);
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 2);
#endif
#undef FILENAME2
#endif /* COMPARE_TO_IMLIB */
#undef FILENAME

#ifdef USE_BUTTON
	column++;
	w = gnome_pixmap_new_from_file(gnome_pixmap_file("gnome-unknown.png"));
	w = gnome_pixmap_button(w, "Test");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row, row + 1);
	w = gnome_pixmap_new_from_file(gnome_pixmap_file("gnome-unknown.png"));
	w = gnome_pixmap_button(w, "Test #2");
	gtk_widget_set_sensitive(w, FALSE);
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, column, column + 1, row + 1, row + 2);
#endif /* USE_BUTTON */

	gtk_table_set_col_spacings(table, 10);
	gtk_table_set_row_spacing(table, 2, 10);
	gtk_table_set_row_spacing(table, 5, 10);
	gtk_table_set_row_spacing(table, 8, 10);
}



int
main(int argc, char **argv)
{
	GtkWidget *window, *hbox, *vbox, *table, *w;

	gnome_init("stock_demo", VERSION, argc, argv);

	textdomain(PACKAGE);

	window = gnome_app_new("Gnome Stock Test", "Gnome Stock Test");
	gtk_window_set_wmclass(GTK_WINDOW(window), "stock_test",
			       "GnomeStockTest");

	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
			   GTK_SIGNAL_FUNC(message_dlg), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy",
			   GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	w = gtk_label_new("Click on `Cancel' to disable the toolbar and menu items\n"
			  "Click on `OK' to enable the toolbar and menu items\n"
			  "Select Edit->Properties or the Properties toolbar button\n"
			  "to open a GnomePropertyBox\n"
			  "Select Edit->Preferences to open the Menu Accelerator Configuration Dialog\n"
			  "Select File->Quit or the Close button to get an example message box and the\n"
			  "opportunity to exit this app");
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 2);

	frame = gtk_frame_new("Other Icons");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 2);
	table = gtk_table_new(1, 1, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	fill_table(window, GTK_TABLE(table));

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_YES);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_NO);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_PREV);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_NEXT);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_FONT);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
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

	w = gnome_stock_button(GNOME_STOCK_BUTTON_CLOSE);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 3);
	gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
				  (GtkSignalFunc)message_dlg,
				  NULL);

	gnome_app_set_contents(GNOME_APP(window), vbox);
	gnome_app_set_menus(GNOME_APP(window),
			    GTK_MENU_BAR(create_menu(window)));
	gnome_app_set_toolbar(GNOME_APP(window),
			      GTK_TOOLBAR(create_toolbar(window)));
	gtk_widget_show(window);
	gtk_main();
	return 0;
}

