/* GnomeApp widget (C) 1998 Red Hat Software, Miguel de Icaza, Federico Mena, Chris Toshok.
 *
 * Originally by Elliot Lee, with hacking by Chris Toshok for *_data, Marc Ewing added menu support,
 * toggle and radio support, and I don't know what you other people did :) menu insertion/removal
 * functions by Jaka Mocnik.
 *
 * Major cleanups and rearrangements by Federico Mena.
 */

/* TO-DO list for GnomeAppHelper and friends:
 *
 * - Find out how to disable on-the-fly hotkey changing for menu items (GtkAccelGroup locking?).
 *
 * - Rewrite gnome-popupmenu.
 *
 * - See if the insert/delete/find functions in this file are sane.
 *
 * - Write a custom container for the GnomeApp window so that you can have multiple toolbars and
 *   such.
 *
 * - Fix GtkHandleBox so that it works right (i.e. is easy to re-dock) and so that it allows
 *   dragging into the top/bottom/left/right of the application window as well.
 */

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-help.h"
/* Note that this file must include gnome-i18n, and not gnome-i18nP, so that _() is the same as the
 * one seen by the application.  This is moderately bogus; we should just call gettext() directly
 * here.
 */
#include "libgnome/gnome-i18n.h"
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-stock.h"
#include "gnome-pixmap.h"
#include "gnome-preferences.h"
#include "gnome-stock.h"


#define MENU_PIXMAP_LABEL_SPACING 2	/* Spacing between pixmap and label in menu items */


/* Creates a pixmap appropriate for items.  The window parameter is required by gnome-stock (bleah) */
static GtkWidget *
create_pixmap (GtkWidget *window, GnomeUIPixmapType pixmap_type, gpointer pixmap_info, int indent_missing_pixmaps)
{
	GtkWidget *pixmap;

	pixmap = NULL;

	switch (pixmap_type) {
	case GNOME_APP_PIXMAP_NONE:
		if (indent_missing_pixmaps)
			pixmap = gnome_stock_pixmap_widget (window, GNOME_STOCK_MENU_BLANK);

		break;

	case GNOME_APP_PIXMAP_STOCK:
		pixmap = gnome_stock_pixmap_widget (window, pixmap_info);
		break;

	case GNOME_APP_PIXMAP_DATA:
		pixmap = gnome_pixmap_new_from_xpm_d (pixmap_info);
		break;

	case GNOME_APP_PIXMAP_FILENAME:
		pixmap = gnome_pixmap_new_from_file (pixmap_info);
		break;

	default:
		g_assert_not_reached ();
	}

	return pixmap;
}

/* Creates an hbox with a pixmap and a menu item label in it.  It uses the settings from
 * gnome-preferences to decide whether the menu item should have a pixmap or not.  It returns the
 * created label widget in the *label argument.  If indent_missing_pixmaps is TRUE, then it will
 * use an empty pixmap even if no pixmap is specified, otherwise it will not insert a pixmap.  It will
 * also return the underlined letter's keyval if keyval is not NULL.
 */
static GtkWidget *
create_pixmap_and_label (char *label_text, GnomeUIPixmapType pixmap_type, gpointer pixmap_info,
			 int indent_missing_pixmaps, GtkWidget **label, guint *keyval)
{
	GtkWidget *hbox;
	GtkWidget *pixmap;
	guint kv;

	hbox = gtk_hbox_new (FALSE, MENU_PIXMAP_LABEL_SPACING);

	/* Create the pixmap */

	/* FIXME: this should later allow for on-the-fly configuration of whether pixmaps are
	 * displayed or not
	 */

	pixmap = create_pixmap (hbox, pixmap_type, pixmap_info, indent_missing_pixmaps);

	if (pixmap) {
		gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, FALSE, 0);
		if (gnome_preferences_get_menus_have_icons ())
			gtk_widget_show (pixmap);
	}

	/* Create the label */

	*label = gtk_accel_label_new (label_text);

	kv = gtk_label_parse_uline (GTK_LABEL (*label), label_text);
	if (keyval)
		*keyval = kv;

	gtk_misc_set_alignment (GTK_MISC (*label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), *label, TRUE, TRUE, 0);
	gtk_widget_show (*label);

	gtk_widget_show (hbox);
	return hbox;
}

/* Creates the accelerator for the specified uiinfo item's hotkey */
static void
setup_accelerator (GtkAccelGroup *accel_group, GnomeUIInfo *uiinfo, char *signal_name, int accel_flags)
{
	if (uiinfo->accelerator_key != 0)
		gtk_widget_add_accelerator (uiinfo->widget, signal_name, accel_group,
					    uiinfo->accelerator_key, uiinfo->ac_mods,
					    accel_flags);
}

/* Creates the accelerators for the underlined letter in a menu item's label.  The keyval is what
 * gtk_label_parse_uline() returned.  If accel_group is not NULL, then the keyval will be put with
 * MOD1 as modifier in it (i.e. for Alt-F in the _File menu).  If menu_accel_group is not NULL, then
 * the plain keyval will be put in it (this is for normal items).
 */
static void
setup_underlined_accelerator (GtkAccelGroup *accel_group, GtkAccelGroup *menu_accel_group,
			      GtkWidget *menu_item, guint keyval)
{
	if (keyval == GDK_VoidSymbol)
		return;

	if (accel_group)
		gtk_widget_add_accelerator (menu_item, "activate_item", accel_group,
					    keyval, GDK_MOD1_MASK,
					    0);

	if (menu_accel_group)
		gtk_widget_add_accelerator (menu_item, "activate_item", menu_accel_group,
					    keyval, 0,
					    0);
}

/* Creates a menu item appropriate for the SEPARATOR, ITEM, TOGGLEITEM, or SUBTREE types.  If the
 * item is inside a radio group, then a pointer to the group's list must be specified as well
 * (*radio_group must be NULL for the first item!).  This function does *not* create the submenu of
 * a subtree menu item.
 */
static void
create_menu_item (GnomeUIInfo *uiinfo, int is_radio, GSList **radio_group, GnomeUIBuilderData *uibdata,
		  GtkAccelGroup *accel_group, int insert_shortcuts, int indent_missing_pixmaps,
		  GtkAccelGroup *menu_accel_group)
{
	GtkWidget *contents;
	GtkWidget *label;
	guint keyval;

	/* Create the menu item */

	switch (uiinfo->type) {
	case GNOME_APP_UI_SEPARATOR:
	case GNOME_APP_UI_ITEM:
	case GNOME_APP_UI_SUBTREE:
		if (is_radio) {
			uiinfo->widget = gtk_radio_menu_item_new (*radio_group);
			*radio_group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (uiinfo->widget));
		} else
			uiinfo->widget = gtk_menu_item_new ();

		break;

	case GNOME_APP_UI_TOGGLEITEM:
		uiinfo->widget = gtk_check_menu_item_new ();
		break;

	default:
		g_warning ("Invalid GnomeUIInfo type %d passed to create_menu_item()", (int) uiinfo->type);
		return;
	}

	/* Create the contents of the menu item */

	if (uiinfo->type != GNOME_APP_UI_SEPARATOR) {
		contents = create_pixmap_and_label (_(uiinfo->label), uiinfo->pixmap_type, uiinfo->pixmap_info,
						    indent_missing_pixmaps, &label, &keyval);
		gtk_container_add (GTK_CONTAINER (uiinfo->widget), contents);

		gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), uiinfo->widget);
	}

	/* Set toggle information, if appropriate */

	if ((uiinfo->type == GNOME_APP_UI_TOGGLEITEM) || is_radio) {
		gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (uiinfo->widget), TRUE);
		gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (uiinfo->widget), FALSE);
	}

	/* Set the accelerators */

	if (uiinfo->type == GNOME_APP_UI_SEPARATOR)
		return; /* nothing more to do */

	setup_accelerator (accel_group, uiinfo, "activate", GTK_ACCEL_VISIBLE);
	setup_underlined_accelerator (insert_shortcuts ? accel_group : NULL,
				      menu_accel_group, uiinfo->widget, keyval);

	/* Connect to the signal */

	if (uiinfo->type != GNOME_APP_UI_SUBTREE)
		(* uibdata->connect_func) (uiinfo, "activate", uibdata);
}

/* Creates a group of radio menu items */
static void
create_radio_menu_items (GtkMenuShell *menu_shell, GnomeUIInfo *uiinfo, GnomeUIBuilderData *uibdata, int right_justify,
			 GtkAccelGroup *accel_group, int indent_missing_pixmaps, GtkAccelGroup *menu_accel_group, gint pos)
{
	GSList *group;

	group = NULL;

	for (; uiinfo->type != GNOME_APP_UI_ENDOFINFO; uiinfo++)
		switch (uiinfo->type) {
		case GNOME_APP_UI_BUILDER_DATA:
			uibdata = uiinfo->moreinfo;
			break;

		case GNOME_APP_UI_ITEM:
			create_menu_item (uiinfo, TRUE, &group, uibdata, accel_group, FALSE, indent_missing_pixmaps,
					  menu_accel_group);

			if (right_justify)
				gtk_menu_item_right_justify (GTK_MENU_ITEM (uiinfo->widget));

			gtk_menu_shell_insert (menu_shell, uiinfo->widget, pos);
      pos++;

			gtk_widget_show (uiinfo->widget);

			break;

		default:
			g_warning ("GnomeUIInfo element type %d is not valid inside a menu radio item group",
				   (int) uiinfo->type);
		}
}

/* Frees a help menu entry when its corresponding menu item is destroyed */
static void
free_help_menu_entry (GtkWidget *widget, gpointer data)
{
	GnomeHelpMenuEntry *entry;

	entry = data;

	g_free (entry->name);
	g_free (entry->path);
	g_free (entry);
}

/* Creates the menu entries for help topics */
static void
create_help_entries (GtkMenuShell *menu_shell, GnomeUIInfo *uiinfo, int right_justify,
                     GtkAccelGroup *menu_accel_group, gint pos)
{
	char buf[1024];
	char *topic_file;
	char *s;
	FILE *file;
	GnomeHelpMenuEntry *entry;
	GtkWidget *item;
	GtkWidget *contents;
	GtkWidget *label;
	guint keyval;

	if (!uiinfo->moreinfo) {
		g_warning ("GnomeUIInfo->moreinfo cannot be NULL for GNOME_APP_UI_HELP");
		return;
	}

	/* Try to open help topics file */

	topic_file = gnome_help_file_find_file (uiinfo->moreinfo, "topic.dat");

	if (!topic_file || !(file = fopen (topic_file, "r"))) {
		g_warning ("Could not open help topics file %s", topic_file ? topic_file : "NULL");

		if (topic_file)
			g_free (topic_file);

		return;
	}

	/* Read in the help topics and create menu items for them */

	while (fgets (buf, sizeof (buf), file)) {
		/* Format of lines is "help_file_name whitespace* menu_title" */

		for (s = buf; *s && !isspace (*s); s++)
			;

		*s++ = '\0';

		for (; *s && isspace (*s); s++)
			;

		if (s[strlen (s) - 1] == '\n')
			s[strlen (s) - 1] = '\0';

		/* Create help menu entry */

		entry = g_new (GnomeHelpMenuEntry, 1);
		entry->name = g_strdup (uiinfo->moreinfo);
		entry->path = g_strdup (buf);

		item = gtk_menu_item_new ();
		contents = create_pixmap_and_label (s, GNOME_APP_PIXMAP_NONE, NULL, TRUE, &label, &keyval);
		gtk_container_add (GTK_CONTAINER (item), contents);

		setup_underlined_accelerator (NULL, menu_accel_group, item, keyval);

		gtk_signal_connect (GTK_OBJECT (item), "activate",
				    (GtkSignalFunc) gnome_help_display,
				    entry);
		gtk_signal_connect (GTK_OBJECT (item), "destroy",
				    (GtkSignalFunc) free_help_menu_entry,
				    entry);

		if (right_justify)
			gtk_menu_item_right_justify (GTK_MENU_ITEM (item));

		gtk_menu_shell_insert (menu_shell, item, pos);
    pos++;

		gtk_widget_show (item);
	}

	fclose (file);
	
	uiinfo->widget = NULL; /* No relevant widget, as we may have created several of them */
}

/* Returns the menu's internal accel_group, or creates a new one if no accel_group was attached */
static GtkAccelGroup *
get_menu_accel_group (GtkMenuShell *menu_shell)
{
	GtkAccelGroup *ag;

	ag = gtk_object_get_data (GTK_OBJECT (menu_shell), "gnome_menu_accel_group");

	if (!ag) {
		ag = gtk_accel_group_new ();
		gtk_accel_group_attach (ag, GTK_OBJECT (menu_shell));
		gtk_object_set_data (GTK_OBJECT (menu_shell), "gnome_menu_accel_group", ag);
	}

	return ag;
}

/* Performs signal connection as appropriate for interpreters or native bindings */
static void
do_ui_signal_connect (GnomeUIInfo *uiinfo, gchar *signal_name, GnomeUIBuilderData *uibdata)
{
	if (uibdata->is_interp)
		gtk_signal_connect_interp (GTK_OBJECT (uiinfo->widget), signal_name,
					   uibdata->relay_func,
					   uibdata->data ? uibdata->data : uiinfo->user_data,
					   uibdata->destroy_func,
					   FALSE);
	else
		gtk_signal_connect (GTK_OBJECT (uiinfo->widget), signal_name,
				    uiinfo->moreinfo,
				    uibdata->data ? uibdata->data : uiinfo->user_data);
}

void
gnome_app_fill_menu (GtkMenuShell *menu_shell, GnomeUIInfo *uiinfo,
		     GtkAccelGroup *accel_group, int insert_shortcuts, int indent_missing_pixmaps, gint pos)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (menu_shell != NULL);
	g_return_if_fail (GTK_IS_MENU_SHELL (menu_shell));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = NULL;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	return gnome_app_fill_menu_custom (menu_shell, uiinfo, &uibdata, accel_group, insert_shortcuts,
					   indent_missing_pixmaps, pos);
}

void
gnome_app_fill_menu_custom (GtkMenuShell *menu_shell, GnomeUIInfo *uiinfo, GnomeUIBuilderData *uibdata,
                            GtkAccelGroup *accel_group, int insert_shortcuts, int indent_missing_pixmaps, gint pos)
{
	GnomeUIBuilderData *orig_uibdata;
	GtkAccelGroup *menu_accel_group;
	int right_justify;

	g_return_if_fail (menu_shell != NULL);
	g_return_if_fail (GTK_IS_MENU_SHELL (menu_shell));
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (uibdata != NULL);

	/* Store a pointer to the original uibdata so that we can use it for the subtrees */

	orig_uibdata = uibdata;

	right_justify = FALSE;

	menu_accel_group = get_menu_accel_group (menu_shell);

	for (; uiinfo->type != GNOME_APP_UI_ENDOFINFO; uiinfo++)
		switch (uiinfo->type) {
		case GNOME_APP_UI_BUILDER_DATA:
			/* Set the builder data for subsequent entries in the current uiinfo array */
			uibdata = uiinfo->moreinfo;
			break;

		case GNOME_APP_UI_JUSTIFY_RIGHT:
			/* Remember that subsequent entries should be right-justified in the menu */
			right_justify = TRUE;
			break;

		case GNOME_APP_UI_HELP:
			/* Create entries for the help topics */
			create_help_entries (menu_shell, uiinfo, right_justify, menu_accel_group, pos);
			break;

		case GNOME_APP_UI_RADIOITEMS:
			/* Create the radio item group */
			create_radio_menu_items (menu_shell, uiinfo->moreinfo, uibdata, right_justify, accel_group,
						 indent_missing_pixmaps, menu_accel_group, pos);
			break;

		case GNOME_APP_UI_SEPARATOR:
		case GNOME_APP_UI_ITEM:
		case GNOME_APP_UI_TOGGLEITEM:
		case GNOME_APP_UI_SUBTREE:
			create_menu_item (uiinfo, FALSE, NULL, uibdata, accel_group, insert_shortcuts,
					  indent_missing_pixmaps, menu_accel_group);

			if (uiinfo->type == GNOME_APP_UI_SUBTREE) {
				/* Create the subtree for this item */

				GtkWidget *menu;

				menu = gtk_menu_new ();
				gtk_menu_item_set_submenu (GTK_MENU_ITEM (uiinfo->widget), menu);
				gnome_app_fill_menu_custom (GTK_MENU_SHELL (menu), uiinfo->moreinfo, orig_uibdata,
                                    accel_group, FALSE, TRUE, 0);
			}

			if (right_justify)
				gtk_menu_item_right_justify (GTK_MENU_ITEM (uiinfo->widget));

			gtk_menu_shell_insert (menu_shell, uiinfo->widget, pos);
      pos++;

			gtk_widget_show (uiinfo->widget);
			break;

		default:
			g_warning ("Invalid GnomeUIInfo element type %d\n", (int) uiinfo->type);
		}

	/* Make the end item contain a pointer to the parent menu shell */

	uiinfo->widget = GTK_WIDGET (menu_shell);
}

void
gnome_app_create_menus (GnomeApp *app, GnomeUIInfo *uiinfo)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = NULL;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_create_menus_custom (app, uiinfo, &uibdata);
}

void
gnome_app_create_menus_interp (GnomeApp *app, GnomeUIInfo *uiinfo, GtkCallbackMarshal relay_func, gpointer data,
			       GtkDestroyNotify destroy_func)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = data;
	uibdata.is_interp = TRUE;
	uibdata.relay_func = relay_func;
	uibdata.destroy_func = destroy_func;

	gnome_app_create_menus_custom (app, uiinfo, &uibdata);
}

void
gnome_app_create_menus_with_data (GnomeApp *app, GnomeUIInfo *uiinfo, gpointer user_data)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = user_data;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_create_menus_custom (app, uiinfo, &uibdata);
}

void
gnome_app_create_menus_custom (GnomeApp *app, GnomeUIInfo *uiinfo, GnomeUIBuilderData *uibdata)
{
	GtkWidget *menubar;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (uibdata != NULL);

	menubar = gtk_menu_bar_new ();
	gnome_app_fill_menu_custom (GTK_MENU_SHELL (menubar), uiinfo, uibdata, app->accel_group, TRUE, FALSE, 0);
	gnome_app_set_menus (app, GTK_MENU_BAR (menubar));
}

/* Creates a toolbar item appropriate for the SEPARATOR, ITEM, or TOGGLEITEM types.  If the item is
 * inside a radio group, then a pointer to the group's trailing widget must be specified as well
 * (*radio_group must be NULL for the first item!).
 */
static void
create_toolbar_item (GtkToolbar *toolbar, GnomeUIInfo *uiinfo, int is_radio, GtkWidget **radio_group,
		     GnomeUIBuilderData *uibdata, GtkAccelGroup *accel_group)
{
	GtkWidget *pixmap;
	GtkToolbarChildType type;

	switch (uiinfo->type) {
	case GNOME_APP_UI_SEPARATOR:
		gtk_toolbar_append_space (toolbar);
		uiinfo->widget = NULL; /* no meaningful widget for a space */
		break;

	case GNOME_APP_UI_ITEM:
	case GNOME_APP_UI_TOGGLEITEM:
		/* Create the icon */

		pixmap = create_pixmap (GTK_WIDGET (toolbar), uiinfo->pixmap_type, uiinfo->pixmap_info, TRUE);

		/* Create the toolbar item */

		if (is_radio)
			type = GTK_TOOLBAR_CHILD_RADIOBUTTON;
		else if (uiinfo->type == GNOME_APP_UI_ITEM)
			type = GTK_TOOLBAR_CHILD_BUTTON;
		else
			type = GTK_TOOLBAR_CHILD_TOGGLEBUTTON;

		uiinfo->widget =
			gtk_toolbar_append_element (toolbar,
						    type,
						    is_radio ? *radio_group : NULL,
						    _(uiinfo->label),
						    _(uiinfo->hint),
						    NULL,
						    pixmap,
						    NULL,
						    NULL);

		if (is_radio)
			*radio_group = uiinfo->widget;

		break;

	default:
		g_warning ("Invalid GnomeUIInfo type %d passed to create_toolbar_item()", (int) uiinfo->type);
		return;
	}

	if (uiinfo->type == GNOME_APP_UI_SEPARATOR)
		return; /* nothing more to do */

	/* Set the accelerator and connect to the signal */

	if (is_radio || (uiinfo->type == GNOME_APP_UI_TOGGLEITEM)) {
		setup_accelerator (accel_group, uiinfo, "toggled", 0);
		(* uibdata->connect_func) (uiinfo, "toggled", uibdata);
	} else {
		setup_accelerator (accel_group, uiinfo, "clicked", 0);
		(* uibdata->connect_func) (uiinfo, "clicked", uibdata);
	}
}

static void
create_radio_toolbar_items (GtkToolbar *toolbar, GnomeUIInfo *uiinfo, GnomeUIBuilderData *uibdata,
			    GtkAccelGroup *accel_group)
{
	GtkWidget *group;

	group = NULL;

	for (; uiinfo->type != GNOME_APP_UI_ENDOFINFO; uiinfo++)
		switch (uiinfo->type) {
		case GNOME_APP_UI_BUILDER_DATA:
			uibdata = uiinfo->moreinfo;
			break;

		case GNOME_APP_UI_ITEM:
			create_toolbar_item (toolbar, uiinfo, TRUE, &group, uibdata, accel_group);
			break;

		default:
			g_warning ("GnomeUIInfo element type %d is not valid inside a toolbar radio item group",
				   (int) uiinfo->type);
		}
}

void
gnome_app_fill_toolbar (GtkToolbar *toolbar, GnomeUIInfo *uiinfo, GtkAccelGroup *accel_group)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (GTK_IS_TOOLBAR (toolbar));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = NULL;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_fill_toolbar_custom (toolbar, uiinfo, &uibdata, accel_group);
}

void
gnome_app_fill_toolbar_custom (GtkToolbar *toolbar, GnomeUIInfo *uiinfo, GnomeUIBuilderData *uibdata,
			       GtkAccelGroup *accel_group)
{
	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (GTK_IS_TOOLBAR (toolbar));
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (uibdata != NULL);

	for (; uiinfo->type != GNOME_APP_UI_ENDOFINFO; uiinfo++)
		switch (uiinfo->type) {
		case GNOME_APP_UI_BUILDER_DATA:
			/* Set the builder data for subsequent entries in the current uiinfo array */
			uibdata = uiinfo->moreinfo;
			break;

		case GNOME_APP_UI_RADIOITEMS:
			/* Create the radio item group */
			create_radio_toolbar_items (toolbar, uiinfo->moreinfo, uibdata, accel_group);
			break;

		case GNOME_APP_UI_SEPARATOR:
		case GNOME_APP_UI_ITEM:
		case GNOME_APP_UI_TOGGLEITEM:
			create_toolbar_item (toolbar, uiinfo, FALSE, NULL, uibdata, accel_group);
			break;

		default:
			g_warning ("Invalid GnomeUIInfo element type %d\n", (int) uiinfo->type);
		}

	/* Make the end item contain a pointer to the parent toolbar */

	uiinfo->widget = GTK_WIDGET (toolbar);
}

void
gnome_app_create_toolbar (GnomeApp *app, GnomeUIInfo *uiinfo)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = NULL;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_create_toolbar_custom (app, uiinfo, &uibdata);
}

void
gnome_app_create_toolbar_interp (GnomeApp *app, GnomeUIInfo *uiinfo,
				 GtkCallbackMarshal relay_func, gpointer data,
				 GtkDestroyNotify destroy_func)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = data;
	uibdata.is_interp = TRUE;
	uibdata.relay_func = relay_func;
	uibdata.destroy_func = destroy_func;

	gnome_app_create_toolbar_custom (app, uiinfo, &uibdata);
}

void
gnome_app_create_toolbar_with_data (GnomeApp *app, GnomeUIInfo *uiinfo, gpointer user_data)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = user_data;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_create_toolbar_custom (app, uiinfo, &uibdata);
}

void
gnome_app_create_toolbar_custom (GnomeApp *app, GnomeUIInfo *uiinfo, GnomeUIBuilderData *uibdata)
{
	GtkWidget *toolbar;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (uibdata != NULL);

	toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
	gnome_app_fill_toolbar_custom (GTK_TOOLBAR (toolbar), uiinfo, uibdata, app->accel_group);
	gnome_app_set_toolbar (app, GTK_TOOLBAR (toolbar));
}

/* menu insertion/removal functions
 * <jaka.mocnik@kiss.uni-lj.si>
 *
 * the path argument should be in the form "File/.../.../Something".
 * "" will insert the item as the first one in the menubar
 * "File/" will insert it as the first one in the File menu
 * "File/Settings" will insert it after the Setting item in the File menu
 * use of  "File/<Separator>" should be obvious. however this stops after the first separator.
 * I hope this explains use of the insert/remove functions well enough.
 */

/* finds menu item described by path (see below for details) starting in the GtkMenuShell top
 * and returns its parent GtkMenuShell and the position after this item in pos:
 * gtk_menu_shell_insert(p, w, pos) would then insert widget w in GtkMenuShell p right after
 * the menu item described by path.
 */
GtkWidget *
gnome_app_find_menu_pos (GtkWidget *parent,
			 gchar *path,
			 gint *pos)
{
	GtkBin *item;
	gchar *label = NULL;
	GList *children, *hbox_children;
	gchar *name_end;
	gint p, path_len;
	
	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);
	g_return_val_if_fail (pos != NULL, NULL);

	children = GTK_MENU_SHELL (parent)->children;
	
	name_end = strchr(path, '/');
	if(name_end == NULL)
		path_len = strlen(path);
	else
		path_len = name_end - path;
	
	if (path_len == 0) {
		*pos = 0;
		return parent;
	}
	
	p = 0;
	
	while( children ) {
    label = NULL;
		p++;
		item = GTK_BIN(children->data);
		
		if(!item->child)                    /* this is a separator, right? */
			label = "<Separator>";
		else if(GTK_IS_LABEL(item->child))  /* a simple item with a label */
			label = GTK_LABEL (item->child)->label;
		else if(GTK_IS_HBOX(item->child)) { /* an item with a hbox (pixmap + label) */
      hbox_children = gtk_container_children(GTK_CONTAINER(item->child));
      while( hbox_children && (label == NULL) ) {
        if(GTK_IS_LABEL(hbox_children->data))
          label = GTK_LABEL(hbox_children->data)->label;

        hbox_children = g_list_next(hbox_children);
      }
    }
		else                                /* something that we just can't handle */
			label = NULL;
		
		if( label && (path_len == strlen(label)) && (strncmp(path, label, path_len) == 0) ) {
			if(name_end == NULL) {
				*pos = p;
				return parent;
			}
			else if(GTK_MENU_ITEM(item)->submenu)
				return gnome_app_find_menu_pos
					(GTK_MENU_ITEM(item)->submenu, (gchar *)(name_end + 1), pos);
			else
				return NULL;
		}
		
		children = g_list_next(children);
	}
	
	return NULL;
}

/* removes num items from the existing app's menu structure begining with item described
 * by path
 */
void
gnome_app_remove_menus(GnomeApp *app,
		       gchar *path,
		       gint items)
{
	GtkWidget *parent, *child;
	GList *children;
	gint pos;
	
	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP(app));
	
	/* find the first item (which is actually at position pos-1) to remove */
	parent = gnome_app_find_menu_pos(app->menubar, path, &pos);
	
	/* in case of path ".../" remove the first item */
	if(pos == 0)
		pos = 1;
	
	if( parent == NULL ) {
		g_warning("gnome_app_remove_menus: couldn't find first item to remove!");
		return;
	}
	
	/* remove items */
	children = g_list_nth(GTK_MENU_SHELL(parent)->children, pos - 1);
	while(children && items > 0) {
		child = GTK_WIDGET(children->data);
		/* children = g_list_next(children); */
		gtk_container_remove(GTK_CONTAINER(parent), child);
		children = g_list_nth(GTK_MENU_SHELL(parent)->children, pos - 1);
		items--;
	}
	
	gtk_widget_queue_resize(parent);
}

/* inserts menus described by uiinfo in existing app's menu structure right after the item described by path.
 */
void
gnome_app_insert_menus_custom (GnomeApp *app,
			       gchar *path,
			       GnomeUIInfo *uiinfo,
			       GnomeUIBuilderData *uibdata)
{
	GtkWidget *parent;
	gint pos;
	
	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP(app));
	g_return_if_fail (app->menubar != NULL);
	
	/* find the parent menushell and position for insertion of menus */
	parent = gnome_app_find_menu_pos(app->menubar, path, &pos);
	if(parent == NULL) {
		g_warning("gnome_app_insert_menus_custom: couldn't find insertion point for menus!");
		return;
	}
	
	/* create menus and insert them */
	gnome_app_fill_menu_custom (GTK_MENU_SHELL (parent), uiinfo, uibdata, app->accel_group, TRUE, FALSE, pos);
}

void
gnome_app_insert_menus (GnomeApp *app,
			gchar *path,
			GnomeUIInfo *menuinfo)
{
	GnomeUIBuilderData uidata =
	{
		do_ui_signal_connect,
		NULL, FALSE, NULL, NULL
	};
	
	gnome_app_insert_menus_custom (app, path, menuinfo, &uidata);
}

void
gnome_app_insert_menus_with_data (GnomeApp *app,
				  gchar *path,
				  GnomeUIInfo *menuinfo,
				  gpointer data)
{
	GnomeUIBuilderData uidata =
	{
		do_ui_signal_connect,
		NULL, FALSE, NULL, NULL
	};
	
	uidata.data = data;
	
	gnome_app_insert_menus_custom (app, path, menuinfo, &uidata);
}

void
gnome_app_insert_menus_interp (GnomeApp *app,
			       gchar *path,
			       GnomeUIInfo *menuinfo,
			       GtkCallbackMarshal relay_func,
			       gpointer data,
			       GtkDestroyNotify destroy_func)
{
	GnomeUIBuilderData uidata =
	{
		do_ui_signal_connect,
		NULL, FALSE, NULL, NULL
	};
	
	uidata.data = data;
	uidata.is_interp = TRUE;
	uidata.relay_func = relay_func;
	uidata.destroy_func = destroy_func;
	
	gnome_app_insert_menus_custom(app, path, menuinfo, &uidata);
}
