/* testGNOME - program similar to testgtk which shows gnome lib functions.
 *
 * Authors : Richard Hestilow <hestgray@ionet.net> (GNOME 1.x version)
 * 	     Carlos Perelló Marín <carlos@gnome-db.org> (Ported to GNOME 2.0)
 *
 * Copyright (C) 1998-2001 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <libgnomeui.h>
#include <libgnomeui/gnome-window.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-window.h>
#include <bonobo/bonobo-ui-util.h>

#include "testgnome.h"
#include "bomb.xpm"

static const gchar *authors[] = {
	"Richard Hestilow",
	"Federico Mena",
	"Eckehard Berns",
	"Havoc Pennington",
	"Miguel de Icaza",
	"Jonathan Blandford",
	"Carlos Perelló Marín",
	"Martin Baulig",
	"Test <test@example.com>",
	NULL
};

static void
test_exit (TestGnomeApp *app)
{
	bonobo_object_unref (BONOBO_OBJECT (app->ui_component));

	gtk_widget_destroy (app->app);
	
	g_free (app);

	gtk_main_quit ();
}

static void
verb_FileTest_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	TestGnomeApp *app = user_data;

	test_exit (app);
}

static void
verb_FileClose_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	TestGnomeApp *app = user_data;

	gtk_widget_destroy (app->app);
}

static void
verb_FileExit_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	TestGnomeApp *app = user_data;

	test_exit (app);
}

static void
verb_HelpAbout_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	const char *documentors[] = { "Documentor1", "Documentor2", NULL };
	gtk_show_about_dialog (NULL,
			"name", "GNOME Test Program",
		        "version", VERSION,
			"copyright", "(C) 1998-2001 The Free Software Foundation",
			"comments", "Program to display GNOME functions.",
			"authors", authors,
			"documenters", documentors,
			"website", "http://www.gnome.org/",
			"website-label", "GNOME Web Site",
			NULL);
}

static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("FileTest", verb_FileTest_cb),
	BONOBO_UI_VERB ("FileClose", verb_FileClose_cb),
	BONOBO_UI_VERB ("FileExit", verb_FileExit_cb),
	BONOBO_UI_VERB ("HelpAbout", verb_HelpAbout_cb),
	BONOBO_UI_VERB_END
};

static gint
quit_test (GtkWidget *caller, GdkEvent *event, TestGnomeApp *app)
{
	test_exit (app);

        return TRUE;
}

static TestGnomeApp *
create_newwin(gboolean normal, gchar *appname, gchar *title)
{
        TestGnomeApp *app;

	app = g_new0 (TestGnomeApp, 1);
	app->app = bonobo_window_new (appname, title);
	if (!normal) {
		g_signal_connect(app->app, "delete_event",
				 G_CALLBACK(quit_test), app);
	};
	app->ui_container = bonobo_ui_engine_get_ui_container (
		bonobo_window_get_ui_engine (BONOBO_WINDOW (app->app)));

	app->ui_component = bonobo_ui_component_new (appname);
	bonobo_ui_component_set_container (app->ui_component,
					   BONOBO_OBJREF(app->ui_container),
					   NULL);

	/* This is a test program only run on the build system, so
	 * it's OK to use GNOMEUISRCDIR which is valid of course only
	 * on the build system.
	 */
	bonobo_ui_util_set_ui (app->ui_component, "",
			       GNOMEUISRCDIR "/testgnome.xml",
			       appname, NULL);

	bonobo_ui_component_add_verb_list_with_data (app->ui_component, verbs, app);
	
	return app;
}


static void
color_set (GnomeColorPicker *cp, guint r, guint g, guint b, guint a)
{
	g_print ("Color set: %u %u %u %u\n", r, g, b, a);
}

/* Creates a color picker with the specified parameters */
static void
create_cp (GtkWidget *table, int dither, int use_alpha, int left, int right, int top, int bottom)
{
	GtkWidget *cp;

	cp = gnome_color_picker_new ();
	g_signal_connect (cp, "color_set",
			  G_CALLBACK (color_set), NULL);

	gnome_color_picker_set_dither (GNOME_COLOR_PICKER (cp), dither);
	gnome_color_picker_set_use_alpha (GNOME_COLOR_PICKER (cp), use_alpha);
	gnome_color_picker_set_d (GNOME_COLOR_PICKER (cp), 1.0, 0.0, 1.0, 0.5);

	gtk_table_attach (GTK_TABLE (table), cp,
			  left, right, top, bottom,
			  0, 0, 0, 0);
	gtk_widget_show (cp);
}

static void
create_color_picker (void)
{
	TestGnomeApp *app;
	GtkWidget *table;
	GtkWidget *w;

	app = create_newwin (TRUE, "testGNOME", "Color Picker");

	table = gtk_table_new (3, 3, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), GNOME_PAD_SMALL);
	gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	bonobo_window_set_contents (BONOBO_WINDOW (app->app), table);
	gtk_widget_show (table);

	/* Labels */

	w = gtk_label_new ("Dither");
	gtk_table_attach (GTK_TABLE (table), w,
			  1, 2, 0, 1,
			  GTK_FILL,
			  GTK_FILL,
			  0, 0);
	gtk_widget_show (w);

	w = gtk_label_new ("No dither");
	gtk_table_attach (GTK_TABLE (table), w,
			  2, 3, 0, 1,
			  GTK_FILL,
			  GTK_FILL,
			  0, 0);
	gtk_widget_show (w);

	w = gtk_label_new ("No alpha");
	gtk_table_attach (GTK_TABLE (table), w,
			  0, 1, 1, 2,
			  GTK_FILL,
			  GTK_FILL,
			  0, 0);
	gtk_widget_show (w);

	w = gtk_label_new ("Alpha");
	gtk_table_attach (GTK_TABLE (table), w,
			  0, 1, 2, 3,
			  GTK_FILL,
			  GTK_FILL,
			  0, 0);
	gtk_widget_show (w);

	/* Color pickers */

	create_cp (table, TRUE,  FALSE, 1, 2, 1, 2);
	create_cp (table, FALSE, FALSE, 2, 3, 1, 2);
	create_cp (table, TRUE,  TRUE,  1, 2, 2, 3);
	create_cp (table, FALSE, TRUE,  2, 3, 2, 3);

	gtk_widget_show (app->app);
}

/*
 * DateEdit
 */

static void
create_date_edit (void)
{
	GtkWidget *datedit;
	TestGnomeApp *app;
	time_t curtime = time(NULL);

	datedit = gnome_date_edit_new(curtime,1,1);
	app = create_newwin(TRUE,"testGNOME","Date Edit");
	bonobo_window_set_contents (BONOBO_WINDOW (app->app), datedit);
	gtk_widget_show(datedit);
	gtk_widget_show(app->app);
}

/*
 * Entry
 */

static void
create_entry(void)
{
	TestGnomeApp *app;
	GtkWidget *entry;

	app = create_newwin(TRUE,"testGNOME","Entry");
	entry = gnome_entry_new("test-entry");
	g_assert(entry != NULL);
	bonobo_window_set_contents(BONOBO_WINDOW(app->app), entry);
	gtk_widget_show(entry);
	gtk_widget_show(app->app);
}

/*
 * FileEntry
 */

static void
file_entry_update_files(GtkWidget *w, GnomeFileEntry *fentry)
{
	char *p;
	char *pp;

	GtkLabel *l1 = g_object_get_data(G_OBJECT(w),"l1");
	GtkLabel *l2 = g_object_get_data(G_OBJECT(w),"l2");

	p = gnome_file_entry_get_full_path(fentry,FALSE);
	pp = g_strconcat("File name: ",p,NULL);
	gtk_label_set_text(l1,pp);
	g_free(pp);
	g_free(p);

	p = gnome_file_entry_get_full_path(fentry,TRUE);
	pp = g_strconcat("File name(if exists only): ",p,NULL);
	gtk_label_set_text(l2,pp);
	g_free(pp);
	g_free(p);
}

static void
file_entry_modal_toggle(GtkWidget *w, GnomeFileEntry *fentry)
{
	gnome_file_entry_set_modal(fentry,GTK_TOGGLE_BUTTON(w)->active);
}

static void
file_entry_directory_toggle(GtkWidget *w, GnomeFileEntry *fentry)
{
	gnome_file_entry_set_directory_entry (fentry,GTK_TOGGLE_BUTTON(w)->active);
}

static void
create_file_entry(void)
{
	TestGnomeApp *app;
	GtkWidget *entry;
	GtkWidget *l1,*l2;
	GtkWidget *but;
	GtkWidget *box;

	app = create_newwin(TRUE,"testGNOME","File Entry");

	box = gtk_vbox_new(FALSE,5);
	entry = gnome_file_entry_new("Foo","Bar");
	gtk_box_pack_start(GTK_BOX(box),entry,FALSE,FALSE,0);

	l1 = gtk_label_new("File name: ");
	gtk_box_pack_start(GTK_BOX(box),l1,FALSE,FALSE,0);

	l2 = gtk_label_new("File name(if exists only): ");
	gtk_box_pack_start(GTK_BOX(box),l2,FALSE,FALSE,0);

	but = gtk_button_new_with_label("Update file labels");
	g_object_set_data(G_OBJECT(but),"l1",l1);
	g_object_set_data(G_OBJECT(but),"l2",l2);
	g_signal_connect(but,"clicked",
			 G_CALLBACK(file_entry_update_files),
			 entry);
	gtk_box_pack_start(GTK_BOX(box),but,FALSE,FALSE,0);

	but = gtk_toggle_button_new_with_label("Make browse dialog modal");
	g_signal_connect(but,"toggled",
			 G_CALLBACK(file_entry_modal_toggle),
			 entry);
	gtk_box_pack_start(GTK_BOX(box),but,FALSE,FALSE,0);

	but = gtk_toggle_button_new_with_label("Directory only picker");
	g_signal_connect(but,"toggled",
			 G_CALLBACK(file_entry_directory_toggle),
			 entry);
	gtk_box_pack_start(GTK_BOX(box),but,FALSE,FALSE,0);

	bonobo_window_set_contents (BONOBO_WINDOW(app->app), box);
	gtk_widget_show_all(app->app);
}

/*
 * IconEntry
 */
static void
icon_entry_changed (GnomeIconEntry *entry, GtkLabel *label)
{
	char *file = gnome_icon_entry_get_filename (entry);
	g_print ("Entry changed, new icon: %s\n",
		 file ? file : "Nothing selected");
	g_free (file);
}
static void
get_icon (GtkWidget *button, GnomeIconEntry *entry)
{
	GtkLabel *label = g_object_get_data (G_OBJECT (button), "label");
	char *file = gnome_icon_entry_get_filename (entry);
	gtk_label_set_text (label, file ? file : "Nothing selected");
	g_free (file);
}

static void
create_icon_entry(void)
{
	TestGnomeApp *app;
	GtkWidget *vbox;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *entry;

	app = create_newwin (TRUE, "testGNOME", "Icon Entry");

	vbox = gtk_vbox_new (FALSE, 5);

	entry = gnome_icon_entry_new ("Foo", "Icon");
	gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);

	button = gtk_button_new_with_label ("Update label below");
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

	label = gtk_label_new ("Nothing selected");
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	g_signal_connect (entry, "changed",
			  G_CALLBACK (icon_entry_changed), NULL);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (get_icon), entry);
	g_object_set_data (G_OBJECT (button), "label", label);

	bonobo_window_set_contents (BONOBO_WINDOW (app->app), vbox);
	gtk_widget_show_all (vbox);
	gtk_widget_show (app->app);
}

/*
 * PixmapEntry
 */
static void
create_pixmap_entry(void)
{
	TestGnomeApp *app;
	GtkWidget *entry;

	app = create_newwin (TRUE, "testGNOME", "Pixmap Entry");

	entry = gnome_pixmap_entry_new ("Foo", "Pixmap", TRUE);

	bonobo_window_set_contents (BONOBO_WINDOW (app->app), entry);
	gtk_widget_show (entry);
	gtk_widget_show (app->app);
}

#if 0
/*
 * FontSelector
 */
static void
create_font_selector (void)
{
	g_warning ("Font '%s'", gnome_font_select ());
}
#endif


/*
 * FontPicker
 */
static void
cfp_ck_UseFont(GtkWidget *widget,GnomeFontPicker *gfp)
{
	gboolean show;

	g_object_get (G_OBJECT (gfp),
		      "use-font-in-label", &show,
		      NULL);
	show = ! show;

	g_object_set (G_OBJECT (gfp),
		      "use-font-in-label", show,
		      NULL);
}

static void
cfp_sp_value_changed(GtkAdjustment *adj,GnomeFontPicker *gfp)
{
	gint size;

	size=(gint)adj->value;

	g_object_set (G_OBJECT (gfp),
		      "label-font-size", size,
		      NULL);

}

static void
cfp_ck_ShowSize(GtkWidget *widget,GnomeFontPicker *gfp)
{
	GtkToggleButton *tb;

	tb=GTK_TOGGLE_BUTTON(widget);

	gnome_font_picker_fi_set_show_size(gfp,tb->active);
}

static void
cfp_set_font(GnomeFontPicker *gfp, gchar *font_name, GtkLabel *label)
{
	g_print("Font name: %s\n",font_name);
	gtk_label_set_text(label,font_name);
}

static void
create_font_picker (void)
{
	GtkWidget *fontpicker1,*fontpicker2,*fontpicker3;

	TestGnomeApp *app;
	GtkWidget *vbox,*vbox1,*vbox2,*vbox3;
	GtkWidget *hbox1,*hbox3;
	GtkWidget *frPixmap,*frFontInfo,*frUser;
	GtkWidget *lbPixmap,*lbFontInfo,*lbUser;
	GtkWidget *ckUseFont,*spUseFont,*ckShowSize;
	GtkAdjustment *adj;

	app = create_newwin(TRUE,"testGNOME","Font Picker");


	vbox=gtk_vbox_new(FALSE,5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox),5);
	bonobo_window_set_contents(BONOBO_WINDOW(app->app),vbox);

	/* Pixmap */
	frPixmap=gtk_frame_new("Default Pixmap");
	gtk_box_pack_start(GTK_BOX(vbox),frPixmap,TRUE,TRUE,0);
	vbox1=gtk_vbox_new(FALSE,FALSE);
	gtk_container_add(GTK_CONTAINER(frPixmap),vbox1);
	
	/* GnomeFontPicker with pixmap */
	fontpicker1 = gnome_font_picker_new();
	gtk_container_set_border_width(GTK_CONTAINER(fontpicker1),5);
	gtk_box_pack_start(GTK_BOX(vbox1),fontpicker1,TRUE,TRUE,0);
	lbPixmap=gtk_label_new("If you choose a font it will appear here");
	gtk_box_pack_start(GTK_BOX(vbox1),lbPixmap,TRUE,TRUE,5);

	g_signal_connect(fontpicker1,"font_set",
			 G_CALLBACK(cfp_set_font),
			 lbPixmap);

	/* Font_Info */
	frFontInfo=gtk_frame_new("Font Info");
	gtk_box_pack_start(GTK_BOX(vbox),frFontInfo,TRUE,TRUE,0);
	vbox2=gtk_vbox_new(FALSE,FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2),5);
	gtk_container_add(GTK_CONTAINER(frFontInfo),vbox2);

	fontpicker2 = gnome_font_picker_new();

	/* GnomeFontPicker with fontinfo */
	hbox1=gtk_hbox_new(FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox2),hbox1,FALSE,FALSE,0);
	ckUseFont=gtk_check_button_new_with_label("Use Font in button with size");
	gtk_box_pack_start(GTK_BOX(hbox1),ckUseFont,TRUE,TRUE,0);

	adj=GTK_ADJUSTMENT(gtk_adjustment_new(14,5,150,1,1,1));
	g_signal_connect (adj, "value_changed",
			  G_CALLBACK (cfp_sp_value_changed),
			  fontpicker2);
	spUseFont=gtk_spin_button_new(adj,1,0);
	gtk_box_pack_start(GTK_BOX(hbox1),spUseFont,FALSE,FALSE,0);
	g_object_set_data (G_OBJECT (fontpicker2), "spUseFont", spUseFont);

	g_signal_connect (ckUseFont, "toggled",
			  G_CALLBACK (cfp_ck_UseFont),
			  fontpicker2);

	ckShowSize=gtk_check_button_new_with_label("Show font size");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ckShowSize),TRUE);
	gtk_box_pack_start(GTK_BOX(vbox2),ckShowSize,FALSE,FALSE,5);

	g_signal_connect (ckShowSize, "toggled",
			  G_CALLBACK (cfp_ck_ShowSize),
			  fontpicker2);

	gnome_font_picker_set_mode(GNOME_FONT_PICKER(fontpicker2),GNOME_FONT_PICKER_MODE_FONT_INFO);
	gtk_box_pack_start(GTK_BOX(vbox2),fontpicker2,TRUE,TRUE,0);

	lbFontInfo=gtk_label_new("If you choose a font it will appear here");
	gtk_box_pack_start(GTK_BOX(vbox2),lbFontInfo,TRUE,TRUE,5);


	g_signal_connect(fontpicker2,"font_set",
			 G_CALLBACK(cfp_set_font),lbFontInfo);


	/* User Widget */
	frUser=gtk_frame_new("User Widget");
	gtk_box_pack_start(GTK_BOX(vbox),frUser,TRUE,TRUE,0);
	vbox3=gtk_vbox_new(FALSE,FALSE);
	gtk_container_add(GTK_CONTAINER(frUser),vbox3);
	
	/* GnomeFontPicker with User Widget */
	fontpicker3 = gnome_font_picker_new();
	gnome_font_picker_set_mode(GNOME_FONT_PICKER(fontpicker3),GNOME_FONT_PICKER_MODE_USER_WIDGET);

	hbox3=gtk_hbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(hbox3),gtk_image_new_from_stock
                           (GNOME_STOCK_PIXMAP_SPELLCHECK, GTK_ICON_SIZE_BUTTON),
			   FALSE,FALSE,5);
	gtk_box_pack_start(GTK_BOX(hbox3),gtk_label_new("This is an hbox with pixmap and text"),
			   FALSE,FALSE,5);
	gnome_font_picker_uw_set_widget(GNOME_FONT_PICKER(fontpicker3),hbox3);
	gtk_container_set_border_width(GTK_CONTAINER(fontpicker3),5);
	gtk_box_pack_start(GTK_BOX(vbox3),fontpicker3,TRUE,TRUE,0);

	lbUser=gtk_label_new("If you choose a font it will appear here");
	gtk_box_pack_start(GTK_BOX(vbox3),lbUser,TRUE,TRUE,5);

	g_signal_connect(fontpicker3,"font_set",
			 G_CALLBACK(cfp_set_font),lbUser);

	gtk_widget_show_all(app->app);

}


/*
 * HRef
 */

static void
href_cb(GtkObject *button)
{
	GtkWidget *href = g_object_get_data (G_OBJECT (button), "href");
	GtkWidget *url_ent = g_object_get_data (G_OBJECT (button), "url");
	GtkWidget *label_ent = g_object_get_data (G_OBJECT (button), "label");
	gchar *url, *text;

	url = gtk_editable_get_chars (GTK_EDITABLE(url_ent), 0, -1);
	text = gtk_editable_get_chars (GTK_EDITABLE(label_ent), 0, -1);
	if (!text || ! text[0])
		text = url;
	gnome_href_set_url(GNOME_HREF(href), url);
	gnome_href_set_text (GNOME_HREF(href), text);
}

static void
create_href(void)
{
	TestGnomeApp *app;
	GtkWidget *vbox, *href, *ent1, *ent2, *wid;

	app = create_newwin(TRUE,"testGNOME","HRef test");
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	bonobo_window_set_contents(BONOBO_WINDOW(app->app), vbox);

	href = gnome_href_new("http://www.gnome.org/", "Gnome Website");
	gtk_box_pack_start(GTK_BOX(vbox), href, FALSE, FALSE, 0);

	wid = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), wid, TRUE, FALSE, 0);

	wid = gtk_label_new("The launch behaviour of the\n"
			    "configured with the control center");
	gtk_box_pack_start(GTK_BOX(vbox), wid, TRUE, FALSE, 0);

	ent1 = gtk_entry_new();
	gtk_entry_set_text (GTK_ENTRY(ent1), "http://www.gnome.org/");
	gtk_box_pack_start (GTK_BOX(vbox), ent1, TRUE, TRUE, 0);

	ent2 = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY(ent2), "Gnome Website");
	gtk_box_pack_start (GTK_BOX(vbox), ent2, TRUE, TRUE, 0);

	wid = gtk_button_new_with_label ("set href props");
	g_object_set_data (G_OBJECT(wid), "href", href);
	g_object_set_data (G_OBJECT(wid), "url", ent1);
	g_object_set_data (G_OBJECT(wid), "label", ent2);
	g_signal_connect (wid, "clicked",
			  G_CALLBACK(href_cb), NULL);
	gtk_box_pack_start (GTK_BOX(vbox), wid, TRUE, TRUE, 0);

	gtk_widget_show_all(app->app);
}

/*
 * IconList
 */

static void
select_icon (GnomeIconList *gil, gint n, GdkEvent *event, gpointer data)
{
	printf ("Icon %d selected", n);

	if (event)
		printf (" with event type %d\n", (int) event->type);
	else
		printf ("\n");
}

static void
unselect_icon (GnomeIconList *gil, gint n, GdkEvent *event, gpointer data)
{
	printf ("Icon %d unselected", n);

	if (event)
		printf (" with event type %d\n", (int) event->type);
	else
		printf ("\n");
}

static void
create_icon_list(void)
{
	TestGnomeApp *app;
	GtkWidget *sw;
	GtkWidget *iconlist;
	GdkPixbuf *pix;
	int i;

	app = create_newwin(TRUE,"testGNOME","Icon List");

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	bonobo_window_set_contents (BONOBO_WINDOW (app->app), sw);
	gtk_widget_set_size_request (sw, 430, 300);
	gtk_widget_show (sw);

	iconlist = gnome_icon_list_new (80, NULL, GNOME_ICON_LIST_IS_EDITABLE);
	gtk_container_add (GTK_CONTAINER (sw), iconlist);
	g_signal_connect (iconlist, "select_icon",
			  G_CALLBACK (select_icon),
			  NULL);
	g_signal_connect (iconlist, "unselect_icon",
			  G_CALLBACK (unselect_icon),
			  NULL);

	GTK_WIDGET_SET_FLAGS(iconlist, GTK_CAN_FOCUS);
	pix = gdk_pixbuf_new_from_xpm_data ((const gchar **)bomb_xpm);

	gtk_widget_grab_focus (iconlist);

	gnome_icon_list_freeze (GNOME_ICON_LIST (iconlist));

	for (i = 0; i < 30; i++) {
		gnome_icon_list_append_pixbuf (GNOME_ICON_LIST(iconlist), pix, "bomb.xpm", "Foo");
		gnome_icon_list_append_pixbuf (GNOME_ICON_LIST(iconlist), pix, "bomb.xpm", "Bar");
		gnome_icon_list_append_pixbuf (GNOME_ICON_LIST(iconlist), pix, "bomb.xpm", "LaLa");
	}

	gnome_icon_list_append (GNOME_ICON_LIST(iconlist), "non-existant.png", "No Icon");

	gnome_icon_list_set_selection_mode (GNOME_ICON_LIST (iconlist), GTK_SELECTION_EXTENDED);
	gnome_icon_list_thaw (GNOME_ICON_LIST (iconlist));
	gtk_widget_show (iconlist);
	gtk_widget_show(app->app);
}


/* Used as a callback for menu items in the GnomeAppHelper test; just prints the string contents of
 * the data pointer.
 */
static void
item_activated (GtkWidget *widget, gpointer data)
{
	printf ("%s activated\n", (char *) data);
}

/* Menu definitions for the GnomeAppHelper test */

static GnomeUIInfo helper_file_menu[] = {
	{ GNOME_APP_UI_ITEM, "_New", "Create a new file", item_activated, "file/new", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW, 'n', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, "_Open...", "Open an existing file", item_activated, "file/open", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN, 'o', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, "_Save", "Save the current file", item_activated, "file/save", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 's', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, "Save _as...", "Save the current file with a new name", item_activated, "file/save as", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE_AS, 0, 0, NULL },

	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, "_Print...", "Print the current file", item_activated, "file/print", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PRINT, 'p', GDK_CONTROL_MASK, NULL },

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_MENU_CLOSE_ITEM(item_activated, "file/close"),
	GNOMEUIINFO_MENU_EXIT_ITEM(item_activated, "file/exit"),

	GNOMEUIINFO_END
};

static GnomeUIInfo helper_edit_menu[] = {
	{ GNOME_APP_UI_ITEM, "_Undo", "Undo the last operation", item_activated, "edit/undo", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_UNDO, 'z', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, "_Redo", "Redo the last undo-ed operation", item_activated, "edit/redo", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REDO, 0, 0, NULL },

	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, "Cu_t", "Cut the selection to the clipboard", item_activated, "edit/cut", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT, 'x', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, "_Copy", "Copy the selection to the clipboard", item_activated, "edit/copy", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY, 'c', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, "_Paste", "Paste the contents of the clipboard", item_activated, "edit/paste", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PASTE, 'v', GDK_CONTROL_MASK, NULL },
	GNOMEUIINFO_END
};

static GnomeUIInfo helper_style_radio_items[] = {
	{ GNOME_APP_UI_ITEM, "_10 points", NULL, item_activated, "style/10 points", NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "_20 points", NULL, item_activated, "style/20 points", NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "_30 points", NULL, item_activated, "style/30 points", NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "_40 points", NULL, item_activated, "style/40 points", NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	GNOMEUIINFO_END
};

static GnomeUIInfo helper_style_menu[] = {
	{ GNOME_APP_UI_TOGGLEITEM, "_Bold", "Make the selection bold", item_activated, "style/bold", NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_TOGGLEITEM, "_Italic", "Make the selection italic", item_activated, "style/bold", NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_RADIOLIST (helper_style_radio_items),
	GNOMEUIINFO_END
};

static GnomeUIInfo helper_help_menu[] = {
	{ GNOME_APP_UI_ITEM, "_About...", "Displays information about the program", item_activated, "help/about", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL },
	GNOMEUIINFO_END
};

static GnomeUIInfo helper_main_menu[] = {
	{ GNOME_APP_UI_SUBTREE, "_File", "File operations", helper_file_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, "_Edit", "Editing commands", helper_edit_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, "_Style", "Style settings", helper_style_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, "_Help", "Help on the program", helper_help_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	GNOMEUIINFO_END
};

/* Toolbar definition for the GnomeAppHelper test */

static GnomeUIInfo helper_toolbar_radio_items[] = {
	{ GNOME_APP_UI_ITEM, "Red", "Set red color", item_activated, "toolbar/red", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_BOOK_RED, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Green", "Set green color", item_activated, "toolbar/green", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_BOOK_GREEN, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Blue", "Set blue color", item_activated, "toolbar/blue", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_BOOK_BLUE, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Yellow", "Set yellow color", item_activated, "toolbar/yellow", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_BOOK_YELLOW, 0, 0, NULL },
	GNOMEUIINFO_END
};

static GnomeUIInfo helper_toolbar[] = {
	{ GNOME_APP_UI_ITEM, "New", "Create a new file", item_activated, "toolbar/new", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_NEW, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Open", "Open an existing file", item_activated, "toolbar/open", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_OPEN, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Save", "Save the current file", item_activated, "toolbar/save", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Print", "Print the current file", item_activated, "toolbar/print", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PRINT, 0, 0, NULL },

	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, "Undo", "Undo the last operation", item_activated, "toolbar/undo", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_UNDO, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Redo", "Redo the last undo-ed operation", item_activated, "toolbar/redo", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_REDO, 0, 0, NULL },

	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, "Cut", "Cut the selection to the clipboard", item_activated, "toolbar/cut", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CUT, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Copy", "Copy the selection to the clipboard", item_activated, "toolbar/copy", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_COPY, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Paste", "Paste the contents of the clipboard", item_activated, "toolbar/paste", NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PASTE, 0, 0, NULL },

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_RADIOLIST (helper_toolbar_radio_items),
	GNOMEUIINFO_END
};

/* These three functions insert some silly text in the GtkEntry specified in the user data */

static void
insert_red (GtkWidget *widget, gpointer data)
{
	int pos;
	GtkWidget *entry;

	g_print ("in callback, data is: %p\n", data);
	
	entry = GTK_WIDGET (data);

	pos = gtk_editable_get_position (GTK_EDITABLE (entry));
	gtk_editable_insert_text (GTK_EDITABLE (entry), "red book ", strlen ("red book "), &pos);
}

static void
insert_green (GtkWidget *widget, gpointer data)
{
	int pos;
	GtkWidget *entry;

	entry = GTK_WIDGET (data);

	pos = gtk_editable_get_position (GTK_EDITABLE (entry));
	gtk_editable_insert_text (GTK_EDITABLE (entry), "green book ", strlen ("green book "), &pos);
}

static void
insert_blue (GtkWidget *widget, gpointer data)
{
	int pos;
	GtkWidget *entry;

	entry = GTK_WIDGET (data);

	pos = gtk_editable_get_position (GTK_EDITABLE (entry));
	gtk_editable_insert_text (GTK_EDITABLE (entry), "blue book ", strlen ("blue book "), &pos);
}

/* Shared popup menu definition for the GnomeAppHelper test */

static GnomeUIInfo helper_shared_popup_dup[] = {
	{ GNOME_APP_UI_ITEM, "Insert a _red book", NULL, insert_red, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_RED, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Insert a _green book", NULL, insert_green, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_GREEN, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Insert a _blue book", NULL, insert_blue, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_BLUE, 0, 0, NULL },
	GNOMEUIINFO_END
};

static GnomeUIInfo helper_shared_popup[] = {
	{ GNOME_APP_UI_ITEM, "Insert a _red book", NULL, insert_red, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_RED, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Insert a _green book", NULL, insert_green, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_GREEN, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Insert a _blue book", NULL, insert_blue, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BOOK_BLUE, 0, 0, NULL },
        GNOMEUIINFO_SUBTREE("Subtree", helper_shared_popup_dup),
	GNOMEUIINFO_END
};

/* These function change the fill color of the canvas item specified in the user data */

static void
set_cyan (GtkWidget *widget, gpointer data)
{
	GnomeCanvasItem *item;

	item = GNOME_CANVAS_ITEM (data);

	gnome_canvas_item_set (item,
			       "fill_color", "cyan",
			       NULL);
}

static void
set_magenta (GtkWidget *widget, gpointer data)
{
	GnomeCanvasItem *item;

	item = GNOME_CANVAS_ITEM (data);

	gnome_canvas_item_set (item,
			       "fill_color", "magenta",
			       NULL);
}

static void
set_yellow (GtkWidget *widget, gpointer data)
{
	GnomeCanvasItem *item;

	item = GNOME_CANVAS_ITEM (data);

	gnome_canvas_item_set (item,
			       "fill_color", "yellow",
			       NULL);
}

/* Explicit popup menu definition for the GnomeAppHelper test */


static GnomeUIInfo helper_explicit_popup_dup[] = {
	{ GNOME_APP_UI_ITEM, "Set color to _cyan", NULL, set_cyan, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Set color to _magenta", NULL, set_magenta, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Set color to _yellow", NULL, set_yellow, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	GNOMEUIINFO_END
};

static GnomeUIInfo helper_explicit_popup[] = {
	{ GNOME_APP_UI_ITEM, "Set color to _cyan", NULL, set_cyan, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Set color to _magenta", NULL, set_magenta, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Set color to _yellow", NULL, set_yellow, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
        GNOMEUIINFO_SUBTREE("Subtree", helper_explicit_popup_dup),
	GNOMEUIINFO_END
};

/* Event handler for canvas items in the explicit popup menu demo */

static gint
item_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
{
	if (!((event->type == GDK_BUTTON_PRESS) && (event->button.button == 3)))
		return FALSE;

	gnome_popup_menu_do_popup (GTK_WIDGET (data), NULL, NULL, (GdkEventButton *) event, item, NULL);

	return TRUE;
}

static void
delete_event (GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(data);
}

static void
create_app_helper (GtkWidget *widget, gpointer data)
{
	GtkWidget *app;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *vbox2;
	GtkWidget *w;
	GtkWidget *popup;
        GnomeAppBar *bar;
	GnomeCanvasItem *item;

	app = gnome_app_new ("testGNOME", "GnomeAppHelper test");
	gnome_app_create_menus (GNOME_APP (app), helper_main_menu);
	gnome_app_create_toolbar (GNOME_APP (app), helper_toolbar);

        bar = GNOME_APPBAR(gnome_appbar_new(FALSE, TRUE, GNOME_PREFERENCES_USER));
        gnome_app_set_statusbar(GNOME_APP(app), GTK_WIDGET(bar));

        gnome_app_install_appbar_menu_hints(GNOME_APPBAR(bar), helper_main_menu);

	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);

	/* Shared popup menu */

	popup = gnome_popup_menu_new (helper_shared_popup);

	frame = gtk_frame_new ("Shared popup menu");
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show (frame);

	vbox2 = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER (vbox2), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), vbox2);
	gtk_widget_show (vbox2);

	w = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (vbox2), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	gnome_popup_menu_attach (popup, w, w);
	
	w = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (vbox2), w, FALSE, FALSE, 0);
	gtk_widget_show (w);
	gnome_popup_menu_attach (popup, w, w);

	/* Popup menu explicitly popped */

	popup = gnome_popup_menu_new (helper_explicit_popup);

	frame = gtk_frame_new ("Explicit popup menu");
	gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
	gtk_widget_show (frame);

	w = gnome_canvas_new ();
	gtk_widget_set_size_request ((w), 200, 100);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (w), 0.0, 0.0, 200.0, 100.0);
	gtk_container_add (GTK_CONTAINER (frame), w);
	gtk_widget_show (w);

	g_signal_connect (w, "destroy",
			  G_CALLBACK (delete_event),
			  popup);

	item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (w)),
				      gnome_canvas_ellipse_get_type (),
				      "x1", 5.0,
				      "y1", 5.0,
				      "x2", 95.0,
				      "y2", 95.0,
				      "fill_color", "white",
				      "outline_color", "black",
				      NULL);
	g_signal_connect (item, "event",
			  G_CALLBACK (item_event),
			  popup);

	item = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (w)),
				      gnome_canvas_ellipse_get_type (),
				      "x1", 105.0,
				      "y1", 0.0,
				      "x2", 195.0,
				      "y2", 95.0,
				      "fill_color", "white",
				      "outline_color", "black",
				      NULL);
	g_signal_connect (item, "event",
			  G_CALLBACK (item_event),
			  popup);

	gnome_app_set_contents (GNOME_APP (app), vbox);
	gtk_widget_show (app);
}

static void
create_about_box (void)
{
	const char *documentors[] = { "Documentor1", "Documentor2", NULL };
	GtkWidget *about_box = gnome_about_new ("Test GNOME",
						VERSION,
						"(c) 2001 Foo, Inc.\n(c) 2001 Bar, Inc.",
						"This is the testing gnome application about box",
						authors,
						documentors,
						"Translation credits",
						NULL /* logo pixbuf */);
	gtk_widget_show (about_box);
}
				
int
main (int argc, char **argv)
{
	struct {
		char *label;
		void (*func) ();
	} buttons[] =
	  {
		  { "app window", create_app_helper },
		  { "color picker", create_color_picker },
		  { "date edit", create_date_edit },
		  { "entry", create_entry },
		  { "file entry", create_file_entry },
		  { "pixmap entry", create_pixmap_entry },
		  { "icon entry", create_icon_entry },
		  { "font picker", create_font_picker },
		  { "href", create_href },
		  { "icon list", create_icon_list },
		  { "about box", create_about_box }
	  };
	int nbuttons = sizeof (buttons) / sizeof (buttons[0]);
	GnomeProgram *program;
	TestGnomeApp *app;
	GtkWidget *box1;
	GtkWidget *box2;
	GtkWidget *button;
	GtkWidget *scrolled_window;
	int i;

	program = gnome_program_init ("testGNOME", VERSION,
			    LIBGNOMEUI_MODULE,
			    argc, argv,
			    GNOME_PARAM_GOPTION_CONTEXT,
			    g_option_context_new ("test-gnome"),
			    NULL);

	app = create_newwin (FALSE, "testGNOME", "testGNOME");
	gtk_window_set_default_size (GTK_WINDOW (app->app), 200, 300);
	box1 = gtk_vbox_new (FALSE, 0);
	bonobo_window_set_contents (BONOBO_WINDOW (app->app), box1);
	gtk_widget_show (box1);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 10);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (scrolled_window)->vscrollbar,
				GTK_CAN_FOCUS);
	gtk_box_pack_start (GTK_BOX (box1), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show (scrolled_window);
	box2 = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), box2);
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (box2), gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(scrolled_window)));
	gtk_widget_show (box2);
	for (i = 0; i < nbuttons; i++) {
		button = gtk_button_new_with_label (buttons[i].label);
		if (buttons[i].func)
			g_signal_connect (button, "clicked",
					  G_CALLBACK(buttons[i].func),
					  NULL);
		else
			gtk_widget_set_sensitive (button, FALSE);
		gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
		gtk_widget_show (button);
	}

	gtk_widget_show (app->app);

	gtk_main ();

	g_object_unref (program);

	return 0;
}
