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

#ifdef FIXME
static const gchar *authors[] = {
	"Richard Hestilow",
	"Federico Mena",
	"Eckehard Berns",
	"Havoc Pennington",
	"Miguel de Icaza",
	"Jonathan Blandford",
	"Carlos Perelló Marín",
	"Martin Baulig",
	NULL
};
#endif

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
#ifdef FIXME
	GtkWidget *about = gnome_about_new ("GNOME Test Program", VERSION,
					    "(C) 1998-2001 The Free Software Foundation",
					    "Program to display GNOME functions.",
					    authors,
					    NULL,
					    NULL,
					    NULL);
	gtk_widget_show (about);
#endif
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
        TestGnomeApp		*app;

	app = g_new0 (TestGnomeApp, 1);
	app->app = bonobo_window_new (appname, title);
	if (!normal) {
		gtk_signal_connect(GTK_OBJECT(app->app), "delete_event",
				   GTK_SIGNAL_FUNC(quit_test), app);
	};
	app->ui_container = bonobo_ui_engine_get_ui_container (
		bonobo_window_get_ui_engine (BONOBO_WINDOW (app->app)));

	app->ui_component = bonobo_ui_component_new (appname);
	bonobo_ui_component_set_container (app->ui_component,
					   BONOBO_OBJREF(app->ui_container),
					   NULL);
	bonobo_ui_util_set_ui (app->ui_component, GNOMEUIDATADIR, "testgnome.xml", appname);
	bonobo_ui_component_add_verb_list_with_data (app->ui_component, verbs, app);
	
	return app;
}

/* Creates a color picker with the specified parameters */
static void
create_cp (GtkWidget *table, int dither, int use_alpha, int left, int right, int top, int bottom)
{
	GtkWidget *cp;

	cp = gnome_color_picker_new ();
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

#if 0
/*
 * DItemEdit
 */

static void
ditem_changed_callback(GnomeDItemEdit *dee, gpointer data)
{
	g_print("Changed!\n");
	fflush(stdout);
}

static void
ditem_icon_changed_callback(GnomeDItemEdit *dee, gpointer data)
{
	g_print("Icon changed!\n");
	fflush(stdout);
}

static void
ditem_name_changed_callback(GnomeDItemEdit *dee, gpointer data)
{
	g_print("Name changed!\n");
	fflush(stdout);
}

static void
create_ditem_edit (void)
{
	TestGnomeApp *app;
	GtkWidget *notebook;
	GtkWidget *dee;

	app = create_newwin(TRUE,"testGNOME","Desktop Item Edit");
	
	notebook = gtk_notebook_new();

	bonobo_window_set_contents (BONOBO_WINDOW (app->app), notebook);

	dee = gnome_ditem_edit_new();
	gtk_container_add (GTK_CONTAINER (notebook), dee);

	gnome_ditem_edit_load_file(GNOME_DITEM_EDIT(dee),
				   "/usr/local/share/gnome/apps/grun.desktop");
#ifdef GNOME_ENABLE_DEBUG
	g_print("Dialog (main): %p\n", GNOME_DITEM_EDIT(dee)->icon_dialog);
#endif

	gtk_signal_connect(GTK_OBJECT(dee), "changed",
			   GTK_SIGNAL_FUNC(ditem_changed_callback),
			   NULL);

	gtk_signal_connect(GTK_OBJECT(dee), "icon_changed",
			   GTK_SIGNAL_FUNC(ditem_icon_changed_callback),
			   NULL);

	gtk_signal_connect(GTK_OBJECT(dee), "name_changed",
			   GTK_SIGNAL_FUNC(ditem_name_changed_callback),
			   NULL);

#ifdef GNOME_ENABLE_DEBUG
	g_print("Dialog (main 2): %p\n", GNOME_DITEM_EDIT(dee)->icon_dialog);
#endif

	gtk_widget_show(notebook);
	gtk_widget_show(app->app);

#ifdef GNOME_ENABLE_DEBUG
	g_print("Dialog (main 3): %p\n", GNOME_DITEM_EDIT(dee)->icon_dialog);
#endif

}
#endif
/*
 * Druid
 */
#if 0

typedef struct druid_data
{
	GtkWidget *radio_button; /* if set, goto A, else goto b */
	GtkWidget *target_a;
	GtkWidget *target_b;
} druid_data;

static gboolean
simple_druid_next_callback (GnomeDruidPage *page, GnomeDruid *druid, GnomeDruidPage *next)
{
	gtk_object_set_data (GTK_OBJECT (next), "back", page);
	gnome_druid_set_page (druid,
	                      next);
	return TRUE;
}

static gboolean
complex_druid_next_callback (GnomeDruidPage *page, GnomeDruid *druid, druid_data *data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->radio_button))) {
		gtk_object_set_data (GTK_OBJECT (data->target_a), "back", page);
		gnome_druid_set_page (druid,
				      GNOME_DRUID_PAGE (data->target_a));
	} else {
		gtk_object_set_data (GTK_OBJECT (data->target_b), "back", page);
		gnome_druid_set_page (druid,
				      GNOME_DRUID_PAGE (data->target_b));
	}
	return TRUE;
}

static gboolean
druid_back_callback (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	GtkWidget *back_page = NULL;
	back_page = gtk_object_get_data (GTK_OBJECT (page), "back");
	if (back_page) {
		gtk_object_set_data (GTK_OBJECT (page), "back", NULL);
		gnome_druid_set_page (druid,
				      GNOME_DRUID_PAGE (back_page));
		return TRUE;
	}
	return FALSE;
}

/*
 *  * The Druid's control flow looks something like this:
 *   *
 *    * page_start -> page_a -> page_b -> page_d -> page_f -> page_finish
 *     *                      |          \                  /
 *      *                      |            page_e -> page_g
 *       *                       \                  /
 *        *                         page_c ----------
 *         */
static void
create_druid(void)
{
	GtkWidget *window;
	GtkWidget *druid;
	gchar *fname;
	GtkWidget *page_start, *page_finish;
	GtkWidget *page_a, *page_b, *page_c, *page_d, *page_e, *page_f, *page_g;
	GdkPixbuf *logo = NULL;
	GdkPixbuf *watermark = NULL;
	GtkWidget *check_a, *check_b;
	GSList *radio_group;
	druid_data *data;

	/* load the images */
	fname = g_strconcat (GNOMEUIPIXMAPDIR, "gnome-logo-icon.png", NULL);
	if (fname)
		/* FIXME: We must test GError */
		logo = gdk_pixbuf_new_from_file (fname, NULL);
	g_free (fname);

#if 0
	/* We really need a better image for this.  For now, it'll do */
	fname = g_strconcat (GNOMEUIPIXMAPDIR, "gnome-logo-large.png", NULL);
	if (fname)
		/* FIXME: We must test GError */
		watermark = gdk_pixbuf_new_from_file (fname, NULL);
	g_free (fname);
#endif

	/* The initial stuff */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	druid = gnome_druid_new ();

	/* The druid pages. */
	page_start = gnome_druid_page_start_new_with_vals
			("Beginning of the DRUID",
			 "This is a Sample DRUID\nIt will walk you through absolutely nothing. (-:\n\nIt would be nice to have a watermark on the left.",
			 logo,
			 watermark);
	page_a = gnome_druid_page_standard_new_with_vals ("Page A", logo);
	page_b = gnome_druid_page_standard_new_with_vals ("Page B", logo);
	page_c = gnome_druid_page_standard_new_with_vals ("Page C", logo);
	page_d = gnome_druid_page_standard_new_with_vals ("Page D", logo);
	page_e = gnome_druid_page_standard_new_with_vals ("Page E", logo);
	page_f = gnome_druid_page_standard_new_with_vals ("Page F", logo);
	page_g = gnome_druid_page_standard_new_with_vals ("Page G", logo);
	page_finish = gnome_druid_page_finish_new_with_vals
			("End of the DRUID",
			 "I hope you found this demo informative.  You would\nnormally put a message here letting someone know\nthat they'd successfully installed something.",
			 logo,
			 watermark);

	/* set each one up. */
	/* page_a */
	data = g_new (druid_data, 1);
	radio_group = NULL;
	check_a = gtk_radio_button_new_with_label (NULL, "Go to page B");
	radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (check_a));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_a), TRUE);
	check_b = gtk_radio_button_new_with_label (radio_group, "Go to page C");
	gtk_box_pack_start (GTK_BOX (GNOME_DRUID_PAGE_STANDARD (page_a)->vbox),
			    check_a, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GNOME_DRUID_PAGE_STANDARD (page_a)->vbox),
			    check_b, FALSE, FALSE, 0);
	data->radio_button = check_a;
	data->target_a = page_b;
	data->target_b = page_c;
	gtk_signal_connect (GTK_OBJECT (page_a), "next", (GtkSignalFunc) complex_druid_next_callback, (gpointer) data);
	gtk_signal_connect (GTK_OBJECT (page_a), "back", (GtkSignalFunc) druid_back_callback, (gpointer) NULL);

	/* page_b */
	data = g_new (druid_data, 1);
	radio_group = NULL;
	check_a = gtk_radio_button_new_with_label (NULL, "Go to page D");
	radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (check_a));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_a), TRUE);
	check_b = gtk_radio_button_new_with_label (radio_group, "Go to page E");
	gtk_box_pack_start (GTK_BOX (GNOME_DRUID_PAGE_STANDARD (page_b)->vbox),
			    check_a, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GNOME_DRUID_PAGE_STANDARD (page_b)->vbox),
			    check_b, FALSE, FALSE, 0);
	data->radio_button = check_a;
	data->target_a = page_d;
	data->target_b = page_e;
	gtk_signal_connect (GTK_OBJECT (page_b), "next", (GtkSignalFunc) complex_druid_next_callback, (gpointer) data);
	gtk_signal_connect (GTK_OBJECT (page_b), "back", (GtkSignalFunc) druid_back_callback, (gpointer) NULL);

	/* page_c */
	gtk_box_pack_start (GTK_BOX (GNOME_DRUID_PAGE_STANDARD (page_c)->vbox),
			    gtk_label_new ("This page will take you to page G\nYou don't have any say in the matter. (-:"),
			    FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (page_c), "next", (GtkSignalFunc) simple_druid_next_callback, (gpointer) page_g);
	gtk_signal_connect (GTK_OBJECT (page_c), "back", (GtkSignalFunc) druid_back_callback, (gpointer) NULL);

	/* page_d */
	gtk_box_pack_start (GTK_BOX (GNOME_DRUID_PAGE_STANDARD (page_d)->vbox),
			    gtk_label_new ("This page will take you to page F"),
			    FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (page_d), "next", (GtkSignalFunc) simple_druid_next_callback, (gpointer) page_f);
	gtk_signal_connect (GTK_OBJECT (page_d), "back", (GtkSignalFunc) druid_back_callback, (gpointer) NULL);

	/* page_e */
	gtk_box_pack_start (GTK_BOX (GNOME_DRUID_PAGE_STANDARD (page_e)->vbox),
			    gtk_label_new ("This page will take you to page G\n\nShe sells sea shells by the sea shore."),
			    FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (page_e), "next", (GtkSignalFunc) simple_druid_next_callback, (gpointer) page_g);
	gtk_signal_connect (GTK_OBJECT (page_e), "back", (GtkSignalFunc) druid_back_callback, (gpointer) NULL);

	/* page_f */
	gtk_box_pack_start (GTK_BOX (GNOME_DRUID_PAGE_STANDARD (page_f)->vbox),
			    gtk_label_new ("This is a second to last page.\nIt isn't as nice as page G.\n\nClick Next to go to the last page\n"),
			    FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (page_f), "next", (GtkSignalFunc) simple_druid_next_callback, (gpointer) page_finish);
	gtk_signal_connect (GTK_OBJECT (page_f), "back", (GtkSignalFunc) druid_back_callback, (gpointer) NULL);

	/* page_g */
	gtk_box_pack_start (GTK_BOX (GNOME_DRUID_PAGE_STANDARD (page_g)->vbox),
			    gtk_label_new ("This is page G!!!!\n\nyay!!!!!!!"),
			    FALSE, FALSE, 0);
	gtk_signal_connect (GTK_OBJECT (page_g), "back", (GtkSignalFunc) druid_back_callback, (gpointer) NULL);


	/* page_finish */
	gtk_signal_connect (GTK_OBJECT (page_finish), "back", (GtkSignalFunc) druid_back_callback, (gpointer) NULL);

	/*Tie it together */
	gtk_container_add (GTK_CONTAINER (window), druid);
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page_start));
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page_a));
	gome_druid_append_page (GNOME_DRUID (druid),
				GNOME_DRUID_PAGE (page_b));
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page_c));
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page_d));
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page_e));
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page_f));
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page_g));
	gnome_druid_append_page (GNOME_DRUID (druid),
				 GNOME_DRUID_PAGE (page_finish));
	gnome_druid_set_page (GNOME_DRUID (druid), GNOME_DRUID_PAGE (page_start));
	gtk_widget_show_all (window);
}

#endif

/*
 * Entry
 */

static void
create_entry(void)
{
	TestGnomeApp *app;
	GtkWidget *entry;

	app = create_newwin(TRUE,"testGNOME","Entry");
	entry = gnome_entry_new();
	g_assert(entry != NULL);
//	gnome_entry_set_text (GNOME_ENTRY(entry), "Foo");
	bonobo_window_set_contents(BONOBO_WINDOW(app->app), entry);
	gtk_widget_show(entry);
	gtk_widget_show(app->app);
}

/*
 * FileEntry
 */

#if 0

static void
file_entry_update_files(GtkWidget *w, GnomeFileEntry *fentry)
{
	char *p;
	char *pp;

	GtkLabel *l1 = gtk_object_get_data(GTK_OBJECT(w),"l1");
	GtkLabel *l2 = gtk_object_get_data(GTK_OBJECT(w),"l2");

	p = gnome_file_entry_get_full_path(fentry,FALSE);
	pp = g_strconcat("File name: ",p,NULL);
	gtk_label_set_text(l1,pp);
	g_free(pp);
	if(p) g_free(p);

	p = gnome_file_entry_get_full_path(fentry,TRUE);
	pp = g_strconcat("File name(if exists only): ",p,NULL);
	gtk_label_set_text(l2,pp);
	g_free(pp);
	if(p) g_free(p);
}

static void
file_entry_modal_toggle(GtkWidget *w, GnomeFileEntry *fentry)
{
	gnome_file_entry_set_modal(fentry,GTK_TOGGLE_BUTTON(w)->active);
}

static void
file_entry_directory_toggle(GtkWidget *w, GnomeFileEntry *fentry)
{
	gnome_file_entry_set_directory(fentry,GTK_TOGGLE_BUTTON(w)->active);
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
	gtk_object_set_data(GTK_OBJECT(but),"l1",l1);
	gtk_object_set_data(GTK_OBJECT(but),"l2",l2);
	gtk_signal_connect(GTK_OBJECT(but),"clicked",
			   GTK_SIGNAL_FUNC(file_entry_update_files),
			   entry);
	gtk_box_pack_start(GTK_BOX(box),but,FALSE,FALSE,0);

	but = gtk_toggle_button_new_with_label("Make browse dialog modal");
	gtk_signal_connect(GTK_OBJECT(but),"toggled",
			   GTK_SIGNAL_FUNC(file_entry_modal_toggle),
			   entry);
	gtk_box_pack_start(GTK_BOX(box),but,FALSE,FALSE,0);

	but = gtk_toggle_button_new_with_label("Directory only picker");
	gtk_signal_connect(GTK_OBJECT(but),"toggled",
			   GTK_SIGNAL_FUNC(file_entry_directory_toggle),
			   entry);
	gtk_box_pack_start(GTK_BOX(box),but,FALSE,FALSE,0);

	bonobo_window_set_contents (BONOBO_WINDOW(app->app), box);
	gtk_widget_show_all(app->app);
}

#endif

/*
 * FontPicker
 */
#if 0
static void
cfp_ck_UseFont(GtkWidget *widget,GnomeFontPicker *gfp)
{
	gboolean show;
	gint size;

	show=!gfp->use_font_in_label;
	size=gfp->use_font_in_label_size;

	gnome_font_picker_fi_set_use_font_in_label(gfp,show,size);

}

static void
cfp_sp_value_changed(GtkAdjustment *adj,GnomeFontPicker *gfp)
{
	gboolean show;
	gint size;

	show=gfp->use_font_in_label;
	size=(gint)adj->value;

	gnome_font_picker_fi_set_use_font_in_label(gfp,show,size);

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
	frPixmap=gtk_frame_new(_("Default Pixmap"));
	gtk_box_pack_start(GTK_BOX(vbox),frPixmap,TRUE,TRUE,0);
	vbox1=gtk_vbox_new(FALSE,FALSE);
	gtk_container_add(GTK_CONTAINER(frPixmap),vbox1);
	
	/* GnomeFontPicker with pixmap */
	fontpicker1 = gnome_font_picker_new();
	gtk_container_set_border_width(GTK_CONTAINER(fontpicker1),5);
	gtk_box_pack_start(GTK_BOX(vbox1),fontpicker1,TRUE,TRUE,0);
	lbPixmap=gtk_label_new(_("If you choose a font it will appear here"));
	gtk_box_pack_start(GTK_BOX(vbox1),lbPixmap,TRUE,TRUE,5);

	gtk_signal_connect(GTK_OBJECT(fontpicker1),"font_set",
			   GTK_SIGNAL_FUNC(cfp_set_font),lbPixmap);

	/* Font_Info */
	frFontInfo=gtk_frame_new("Font Info");
	gtk_box_pack_start(GTK_BOX(vbox),frFontInfo,TRUE,TRUE,0);
	vbox2=gtk_vbox_new(FALSE,FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2),5);
	gtk_container_add(GTK_CONTAINER(frFontInfo),vbox2);

	/* GnomeFontPicker with fontinfo */
	hbox1=gtk_hbox_new(FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox2),hbox1,FALSE,FALSE,0);
	ckUseFont=gtk_check_button_new_with_label(_("Use Font in button with size"));
	gtk_box_pack_start(GTK_BOX(hbox1),ckUseFont,TRUE,TRUE,0);

	adj=GTK_ADJUSTMENT(gtk_adjustment_new(14,5,150,1,1,1));
	spUseFont=gtk_spin_button_new(adj,1,0);
	gtk_box_pack_start(GTK_BOX(hbox1),spUseFont,FALSE,FALSE,0);

	ckShowSize=gtk_check_button_new_with_label(_("Show font size"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ckShowSize),TRUE);
	gtk_box_pack_start(GTK_BOX(vbox2),ckShowSize,FALSE,FALSE,5);

	fontpicker2 = gnome_font_picker_new();
	gnome_font_picker_set_mode(GNOME_FONT_PICKER(fontpicker2),GNOME_FONT_PICKER_MODE_FONT_INFO);
	gtk_box_pack_start(GTK_BOX(vbox2),fontpicker2,TRUE,TRUE,0);

	gtk_signal_connect(GTK_OBJECT(ckUseFont),"toggled",
			   (GtkSignalFunc)cfp_ck_UseFont,fontpicker2);

	gtk_signal_connect(GTK_OBJECT(ckShowSize),"toggled",
			   (GtkSignalFunc)cfp_ck_ShowSize,fontpicker2);

	gtk_signal_connect(GTK_OBJECT(adj),"value_changed",
			   (GtkSignalFunc)cfp_sp_value_changed,fontpicker2);

	lbFontInfo=gtk_label_new(_("If you choose a font it will appear here"));
	gtk_box_pack_start(GTK_BOX(vbox2),lbFontInfo,TRUE,TRUE,5);


	gtk_signal_connect(GTK_OBJECT(fontpicker2),"font_set",
			   GTK_SIGNAL_FUNC(cfp_set_font),lbFontInfo);


	/* User Widget */
	frUser=gtk_frame_new("User Widget");
	gtk_box_pack_start(GTK_BOX(vbox),frUser,TRUE,TRUE,0);
	vbox3=gtk_vbox_new(FALSE,FALSE);
	gtk_container_add(GTK_CONTAINER(frUser),vbox3);
	
	/* GnomeFontPicker with User Widget */
	fontpicker3 = gnome_font_picker_new();
	gnome_font_picker_set_mode(GNOME_FONT_PICKER(fontpicker3),GNOME_FONT_PICKER_MODE_USER_WIDGET);

	hbox3=gtk_hbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(hbox3),gnome_stock_new_with_icon(GNOME_STOCK_PIXMAP_SPELLCHECK),
			   FALSE,FALSE,5);
	gtk_box_pack_start(GTK_BOX(hbox3),gtk_label_new(_("This is an hbox with pixmap and text")),
			   FALSE,FALSE,5);
	gnome_font_picker_uw_set_widget(GNOME_FONT_PICKER(fontpicker3),hbox3);
	gtk_container_set_border_width(GTK_CONTAINER(fontpicker3),5);
	gtk_box_pack_start(GTK_BOX(vbox3),fontpicker3,TRUE,TRUE,0);

	lbUser=gtk_label_new(_("If you choose a font it will appear here"));
	gtk_box_pack_start(GTK_BOX(vbox3),lbUser,TRUE,TRUE,5);

	gtk_signal_connect(GTK_OBJECT(fontpicker3),"font_set",
			   GTK_SIGNAL_FUNC(cfp_set_font),lbUser);

	gtk_widget_show_all(app->app);

}
#endif
/*
 * HRef
 */

static void
href_cb(GtkObject *button)
{
	GtkWidget *href = gtk_object_get_data(button, "href");
	GtkWidget *url_ent = gtk_object_get_data(button, "url");
	GtkWidget *label_ent = gtk_object_get_data(button, "label");
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
	gtk_object_set_data (GTK_OBJECT(wid), "href", href);
	gtk_object_set_data (GTK_OBJECT(wid), "url", ent1);
	gtk_object_set_data (GTK_OBJECT(wid), "label", ent2);
	gtk_signal_connect (GTK_OBJECT(wid), "clicked",
			    GTK_SIGNAL_FUNC(href_cb), NULL);
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
	gtk_widget_set_usize (sw, 430, 300);
	gtk_widget_show (sw);

	iconlist = gnome_icon_list_new (80, GNOME_ICON_LIST_IS_EDITABLE);
	gtk_container_add (GTK_CONTAINER (sw), iconlist);
	gtk_signal_connect (GTK_OBJECT (iconlist), "select_icon",
			    GTK_SIGNAL_FUNC (select_icon),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (iconlist), "unselect_icon",
			    GTK_SIGNAL_FUNC (unselect_icon),
			    NULL);

	GTK_WIDGET_SET_FLAGS(iconlist, GTK_CAN_FOCUS);
	pix = gdk_pixbuf_new_from_xpm_data ((const gchar **)bomb_xpm);
/*	gdk_imlib_render (pix, pix->rgb_width, pix->rgb_height);*/

	gtk_widget_grab_focus (iconlist);

	gnome_icon_list_freeze (GNOME_ICON_LIST (iconlist));

	for (i = 0; i < 30; i++) {
		gnome_icon_list_append_pixbuf (GNOME_ICON_LIST(iconlist), pix, "bomb.xpm", "Foo");
		gnome_icon_list_append_pixbuf (GNOME_ICON_LIST(iconlist), pix, "bomb.xpm", "Bar");
		gnome_icon_list_append_pixbuf (GNOME_ICON_LIST(iconlist), pix, "bomb.xpm", "LaLa");
	}


	gnome_icon_list_set_selection_mode (GNOME_ICON_LIST (iconlist), GTK_SELECTION_EXTENDED);
	gnome_icon_list_thaw (GNOME_ICON_LIST (iconlist));
	gtk_widget_show (iconlist);
	gtk_widget_show(app->app);
}

/*
 * ImageEntry
 */

static void
create_image_entry(void)
{
	TestGnomeApp *app;
	GtkWidget *entry;

	app = create_newwin(TRUE,"testGNOME","Pixmap Entry");
	/* FIXME: The gnome_image_entry_new param. must be reviewed */
	entry = gnome_image_entry_new_pixmap_entry (0,0);
	bonobo_window_set_contents(BONOBO_WINDOW(app->app),entry);
	gtk_widget_show(entry);
	gtk_widget_show(app->app);
}
#if 0
/*
 * ImageSelector
 */

static void
create_image_selector (void)
{

}

/*
 * GnomePaperSelector
 */

static void
create_papersel(void)
{
	TestGnomeApp *app;
	GtkWidget *papersel;

	app = create_newwin(TRUE,"testGNOME","Paper Selection");
	papersel = gnome_paper_selector_new( );
	bonobo_window_set_contents(BONOBO_WINDOW(app->app),papersel);
	gtk_widget_show(papersel);
	gtk_widget_show(app->app);
}

#endif
#if 0
/*
 * UnitSpinner
 */

static void
create_unit_spinner (void)
{

}
#endif
int
main (int argc, char **argv)
{
	struct {
		char *label;
		void (*func) ();
	} buttons[] =
	  {
		{ "color picker", create_color_picker },
		{ "date edit", create_date_edit },
/*		{ "ditem edit", create_ditem_edit },*/
/*		{ "druid", create_druid },*/
		{ "entry", create_entry },
/*		{ "file entry", create_file_entry },*/
/*		{ "font picker", create_font_picker },*/
		{ "href", create_href },
		{ "icon list", create_icon_list },
		{ "image entry", create_image_entry },
/*		{ "image selector", create_image_selector },*/
/*		{ "paper selector", create_papersel },*/
/*		{ "unit spinner", create_unit_spinner },*/
	  };
	int nbuttons = sizeof (buttons) / sizeof (buttons[0]);
	TestGnomeApp *app;
	GtkWidget *box1;
	GtkWidget *box2;
	GtkWidget *button;
	GtkWidget *scrolled_window;
	int i;

	
	gnome_program_init ("testGNOME", VERSION, &libgnomeui_module_info,
			    argc, argv, NULL);

	app = create_newwin (FALSE, "testGNOME", "testGNOME");
	gtk_widget_set_usize (app->app, 200, 300);
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
			gtk_signal_connect (GTK_OBJECT (button),
					    "clicked",
					    GTK_SIGNAL_FUNC(buttons[i].func),
					    NULL);
		else
			gtk_widget_set_sensitive (button, FALSE);
		gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
		gtk_widget_show (button);
	}

	gtk_widget_show (app->app);

	bonobo_main ();

	return 0;
}

