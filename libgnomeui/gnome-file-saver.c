/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME GUI Library - gnome-file-saver.c
 * Copyright (C) 2000  Red Hat Inc.
 * All rights reserved.
 *
 * Author: Havoc Pennington <hp@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#include "gnome-file-saver.h"
#include "gnome-gconf.h"
#include "gnome-pixmap.h"
#include <gtk/gtk.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>
#include "gtkpixmapmenuitem.h"
#include "gnome-stock.h"

#include <libgnomevfs/gnome-vfs-mime.h>

struct _GnomeFileSaverPrivate {
        GtkWidget *filename_entry;

        GtkWidget *location_option;
        GtkWidget *location_menu;
        
        GConfClient *conf;
        guint        conf_notify;

        GSList      *locations;

        GtkWidget   *type_option;
        GtkWidget   *type_menu;
        GtkWidget   *type_pixmap;
        GtkWidget   *type_label;
};



enum {
        FINISHED,
        LAST_SIGNAL
};

enum {
        ARG_0,
        LAST_ARG
};

static void gnome_file_saver_init (GnomeFileSaver* file_saver);
static void gnome_file_saver_class_init (GnomeFileSaverClass* klass);
static void gnome_file_saver_destroy (GtkObject* object);
static void gnome_file_saver_finalize (GObject* object);
static void gnome_file_saver_set_arg (GtkObject* object, GtkArg* arg, guint arg_id);
static void gnome_file_saver_get_arg (GtkObject* object, GtkArg* arg, guint arg_id);

static void locations_changed_notify(GConfClient* client, guint cnxn_id, GConfEntry * entry, gpointer user_data);

static void
gnome_file_saver_remove_location(GnomeFileSaver* file_saver,
                                 const gchar* gconf_key);
static void
gnome_file_saver_update_location(GnomeFileSaver* file_saver,
                                 const gchar* gconf_key,
                                 GConfValue* gconf_value);

static GnomeDialog* parent_class = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

GtkType
gnome_file_saver_get_type (void)
{
        static GtkType our_type = 0;

        if (our_type == 0) {
                static const GtkTypeInfo our_info = {
                        "GnomeFileSaver",
                        sizeof (GnomeFileSaver),
                        sizeof (GnomeFileSaverClass),
                        (GtkClassInitFunc) gnome_file_saver_class_init,
                        (GtkObjectInitFunc) gnome_file_saver_init,
                        /* reserved_1 */ NULL,
                        /* reserved_2 */ NULL,
                        (GtkClassInitFunc) NULL
                };

                our_type = gtk_type_unique (gnome_dialog_get_type(), &our_info);
        }
        
        return our_type;
}

static void
gnome_file_saver_class_init (GnomeFileSaverClass* klass)
{
        GtkObjectClass* object_class;
        GObjectClass* gobject_class;

        object_class = (GtkObjectClass*) klass;
        gobject_class = (GObjectClass*) klass;

        parent_class = gtk_type_class (gnome_dialog_get_type());

        signals[FINISHED] =
                gtk_signal_new ("finished",
                                GTK_RUN_LAST,
                                GTK_CLASS_TYPE (object_class),
                                GTK_SIGNAL_OFFSET (GnomeFileSaverClass, finished),
                                gtk_marshal_NONE__POINTER_POINTER,
                                GTK_TYPE_NONE,
				2,
				GTK_TYPE_STRING,
				GTK_TYPE_STRING);
        
        gtk_object_class_add_signals (object_class, signals, 
                                      LAST_SIGNAL);

	klass->finished = NULL;
        
        object_class->set_arg = gnome_file_saver_set_arg;
        object_class->get_arg = gnome_file_saver_get_arg;

        object_class->destroy = gnome_file_saver_destroy;
        gobject_class->finalize = gnome_file_saver_finalize;
}

void
gnome_file_saver_init (GnomeFileSaver* file_saver)
{
        GnomeDialog *dialog;
        GSList *all_entries;
        GSList *iter;
        GtkWidget *hbox;
        GtkWidget *table;
        GtkWidget *label;
        GtkWidget *frame;
        GtkWidget *vbox;
        gchar *icon;

	file_saver->_priv = g_new0(GnomeFileSaverPrivate, 1);

        gtk_window_set_policy(GTK_WINDOW(file_saver), FALSE, TRUE, FALSE);
        
        dialog = GNOME_DIALOG(file_saver);

        hbox = gtk_hbox_new(FALSE, 6);

        gtk_box_pack_start (GTK_BOX(dialog->vbox),
                            hbox,
                            FALSE, FALSE, 6);

        icon = gnome_pixmap_file("mc/i-regular.png");

        if (icon)
                file_saver->_priv->type_pixmap = gnome_pixmap_new_from_file (icon);
        else
                file_saver->_priv->type_pixmap = gnome_pixmap_new ();

        g_free (icon);
        
        frame = gtk_frame_new (NULL);
        vbox = gtk_vbox_new (FALSE, 0);
        gtk_container_add (GTK_CONTAINER (vbox), frame);
        gtk_container_add (GTK_CONTAINER (frame), file_saver->_priv->type_pixmap);
        
        gtk_box_pack_start (GTK_BOX(hbox),
                            vbox,
                            FALSE, FALSE, 6);
        
        table = gtk_table_new (2, 3, FALSE);

        gtk_box_pack_end (GTK_BOX(hbox),
                          table,
                          TRUE, TRUE, 6);
        
        label = gtk_label_new(_("File Name:"));
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
        gtk_table_attach (GTK_TABLE(table),
                          label,
                          0, 1,
                          0, 1,
                          GTK_FILL, GTK_FILL, 3, 3);

        label = gtk_label_new(_("Location:"));
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
        gtk_table_attach (GTK_TABLE(table),
                          label,
                          0, 1,
                          1, 2,
                          GTK_FILL, GTK_FILL, 3, 3);

        file_saver->_priv->type_label =  gtk_label_new(_("Save As Type:"));
        gtk_misc_set_alignment(GTK_MISC(file_saver->_priv->type_label), 0.0, 0.5);
        gtk_table_attach (GTK_TABLE(table),
                          file_saver->_priv->type_label,
                          0, 1,
                          2, 3,
                          GTK_FILL, GTK_FILL, 3, 3);
        
        file_saver->_priv->filename_entry = gtk_entry_new();

        file_saver->_priv->location_option = gtk_option_menu_new();        
        file_saver->_priv->location_menu = gtk_menu_new();

        file_saver->_priv->type_option = gtk_option_menu_new();        
        file_saver->_priv->type_menu = gtk_menu_new();
        
        gtk_table_attach (GTK_TABLE(table),
                          file_saver->_priv->filename_entry,
                          1, 2,
                          0, 1,
                          GTK_FILL | GTK_EXPAND,
                          GTK_FILL,
                          3, 3);

        gtk_table_attach (GTK_TABLE(table),
                          file_saver->_priv->location_option,
                          1, 2,
                          1, 2,
                          GTK_FILL | GTK_EXPAND,
                          GTK_FILL,
                          3, 3);

        gtk_table_attach (GTK_TABLE(table),
                          file_saver->_priv->type_option,
                          1, 2,
                          2, 3,
                          GTK_FILL | GTK_EXPAND,
                          GTK_FILL,
                          3, 3);
        
        /*
         * Get the GConfClient object
         */ 
        
        file_saver->_priv->conf = gnome_get_gconf_client();

        g_object_ref(G_OBJECT(file_saver->_priv->conf));

        /*
         * Fill in the Location menu
         */
        
        /* No preload because the explicit all_entries call below
           effectively results in a preload. */
        gconf_client_add_dir(file_saver->_priv->conf,
                             "/desktop/standard/save-locations",
                             GCONF_CLIENT_PRELOAD_NONE, NULL);

        all_entries =
                gconf_client_all_entries(file_saver->_priv->conf,
                                         "/desktop/standard/save-locations",
                                         NULL);

        iter = all_entries;
        while (iter != NULL) {
                GConfEntry *entry = iter->data;
                gchar* full_key;

                full_key = gconf_concat_dir_and_key("/desktop/standard/save-locations",
                                                    gconf_entry_get_key(entry));

                
                gnome_file_saver_update_location(file_saver,
                                                 full_key,
                                                 gconf_entry_get_value(entry));

                g_free(full_key);
                gconf_entry_free(entry);
                
                iter = g_slist_next(iter);
        }
        g_slist_free(all_entries);
        
        file_saver->_priv->conf_notify =
                gconf_client_notify_add(file_saver->_priv->conf,
                                        "/desktop/standard/save-locations",
                                        locations_changed_notify,
                                        file_saver, NULL, NULL);

        gtk_widget_show(file_saver->_priv->location_menu);
        
        /* Must do _after_ filling the menu */
        gtk_option_menu_set_menu(GTK_OPTION_MENU(file_saver->_priv->location_option),
                                 file_saver->_priv->location_menu);

        gtk_widget_show(file_saver->_priv->type_menu);
        
        /* Must do _after_ filling the menu */
        gtk_option_menu_set_menu(GTK_OPTION_MENU(file_saver->_priv->type_option),
                                 file_saver->_priv->type_menu);

        gtk_widget_show_all(hbox);

        /* Now hide the type stuff until we get mime types added */
        gtk_widget_hide(file_saver->_priv->type_label);
        gtk_widget_hide(file_saver->_priv->type_option);

        gnome_dialog_append_buttons (GNOME_DIALOG (file_saver),
                                     GNOME_STOCK_BUTTON_OK,
                                     GNOME_STOCK_BUTTON_CANCEL,
                                     NULL);
}

GtkWidget*
gnome_file_saver_new (const gchar* title, const gchar* saver_id)
{
        GtkWidget *widget;

        widget = GTK_WIDGET (gtk_type_new (gnome_file_saver_get_type ()));

        gtk_window_set_title(GTK_WINDOW(widget), title);
        
        return widget;
}

static void
gnome_file_saver_destroy (GtkObject* object)
{
        GnomeFileSaver* file_saver;

	/* remember, destroy can be run multiple times! */

        file_saver = GNOME_FILE_SAVER (object);
        
	if (file_saver->_priv->conf) {
		if (file_saver->_priv->conf_notify != 0) {
			gconf_client_notify_remove(file_saver->_priv->conf,
						   file_saver->_priv->conf_notify);
			file_saver->_priv->conf_notify = 0;
		}

		gconf_client_remove_dir(file_saver->_priv->conf,
					"/desktop/standard/save-locations",
                                        NULL);

		g_object_unref(G_OBJECT(file_saver->_priv->conf));
		file_saver->_priv->conf = NULL;
	}
        
        (* GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

static void
gnome_file_saver_finalize (GObject* object)
{
        GnomeFileSaver* file_saver;

        file_saver = GNOME_FILE_SAVER (object);

	g_free(file_saver->_priv);
	file_saver->_priv = NULL;

        (* G_OBJECT_CLASS(parent_class)->finalize) (object);
}

static void
gnome_file_saver_set_arg (GtkObject* object, GtkArg* arg, guint arg_id)
{
        GnomeFileSaver* file_saver;

        file_saver = GNOME_FILE_SAVER (object);

        switch (arg_id) {

        default:
                g_assert_not_reached();
                break;
        }
}

static void
gnome_file_saver_get_arg (GtkObject* object, GtkArg* arg, guint arg_id)
{
        GnomeFileSaver* file_saver;

        file_saver = GNOME_FILE_SAVER (object);

        switch (arg_id) {
                
        default:
                arg->type = GTK_TYPE_INVALID;
                break;
        }
}

/*
 * MIME type stuff
 */

static void
type_chosen_cb(GtkWidget* mi, GnomeFileSaver* file_saver)
{
        const gchar* mime_type;
        const gchar* icon_file;
        gchar* freeme = NULL;
        GdkPixbuf *pixbuf;
        
        mime_type = gtk_object_get_data(GTK_OBJECT(mi),
                                        "mimetype");

        g_assert(mime_type != NULL);

        icon_file = gnome_mime_get_value(mime_type, "icon-filename");

        if (icon_file == NULL) {
                freeme = gnome_pixmap_file("mc/i-regular.png");
                icon_file = freeme;
        }

        if (icon_file) {
                printf("file %s\n", icon_file);
                pixbuf = gdk_pixbuf_new_from_file(icon_file, NULL);
        } else {
                pixbuf = NULL;
        }

        if (pixbuf) {
                printf("loaded ok\n");
                gnome_pixmap_set_pixbuf (GNOME_PIXMAP(file_saver->_priv->type_pixmap),
                                         pixbuf);
                gdk_pixbuf_unref(pixbuf);
        } else {
                printf("no load\n");
                gnome_pixmap_clear (GNOME_PIXMAP(file_saver->_priv->type_pixmap));
        }
                
        g_free(freeme);
}

void
gnome_file_saver_add_mime_type(GnomeFileSaver *file_saver,
                               const gchar    *mime_type)
{
        GtkWidget *mi;
        GtkWidget *pix;
        const gchar* icon_file;
        gchar* freeme = NULL;
        const gchar* desc;
        GtkWidget *label;

        gtk_widget_show(file_saver->_priv->type_option);
        gtk_widget_show(file_saver->_priv->type_label);
        
        icon_file = gnome_mime_get_value(mime_type, "icon-filename");
        desc = gnome_mime_description(mime_type);
        if (desc == NULL)
                desc = mime_type;

        if (icon_file == NULL) {
                freeme = gnome_pixmap_file("mc/i-regular.png");
                icon_file = freeme;
        }
        
        if (icon_file) 
                pix = gnome_pixmap_new_from_file_at_size(icon_file, 16, 16);
        else
                pix = NULL;

        mi = gtk_pixmap_menu_item_new();
        label = gtk_label_new(desc);
        gtk_container_add(GTK_CONTAINER(mi), label);

        if (pix)
                gtk_pixmap_menu_item_set_pixmap(GTK_PIXMAP_MENU_ITEM(mi),
                                                pix);

        g_free(freeme);

        gtk_object_set_data_full(GTK_OBJECT(mi),
                                 "mimetype",
                                 g_strdup(mime_type),
                                 g_free);

        gtk_signal_connect (GTK_OBJECT(mi),
                            "activate",
                            type_chosen_cb,
                            file_saver);
        
        gtk_widget_show_all(mi);

        gtk_menu_shell_append(GTK_MENU_SHELL(file_saver->_priv->type_menu),
                              mi);        
}

void
gnome_file_saver_add_mime_types(GnomeFileSaver *file_saver,
                                const gchar    *mime_types[])
{
        const gchar** mtp;

        mtp = mime_types;

        while (*mtp) {
                gnome_file_saver_add_mime_type(file_saver, *mtp);
                ++mtp;
        }
}

/*
 * Location list management
 */

static void
location_mi_set(GtkWidget* mi,
                const gchar* human_name,
                const gchar* dirname);

static GtkWidget*
location_mi_new(const gchar* gconf_key,
                const gchar* human_name,
                const gchar* dirname)
{
        GtkWidget *mi;

        g_return_val_if_fail(human_name != NULL, NULL);
        g_return_val_if_fail(dirname != NULL, NULL);
        
        mi = gtk_menu_item_new_with_label (human_name);

        location_mi_set(mi, human_name, dirname);

        gtk_object_set_data_full(GTK_OBJECT(mi), "gconf_key", g_strdup(gconf_key), g_free);

        return mi;
}

static const gchar*
location_mi_get_human_name(GtkWidget *mi)
{
        return gtk_object_get_data(GTK_OBJECT(mi), "name");
}

/*UNUSED
static const gchar*
location_mi_get_dirname(GtkWidget *mi)
{
        return gtk_object_get_data(GTK_OBJECT(mi), "dirname");
}
*/

static const gchar*
location_mi_get_key(GtkWidget *mi)
{
        return gtk_object_get_data(GTK_OBJECT(mi), "gconf_key");
}

static void
location_mi_set(GtkWidget* mi,
                const gchar* human_name,
                const gchar* dirname)
{
        gtk_object_set_data_full(GTK_OBJECT(mi), "dirname", g_strdup(dirname), g_free);
        gtk_object_set_data_full(GTK_OBJECT(mi), "name", g_strdup(human_name), g_free);
}

/*UNUSED
static gint
location_compare_human_func(gconstpointer loc1, gconstpointer loc2)
{

        return strcmp(location_mi_get_human_name((GtkWidget*)loc1),
                      location_mi_get_human_name((GtkWidget*)loc2));
}
*/

static gint
location_compare_gconf_func(gconstpointer loc1, gconstpointer loc2)
{

        return strcmp(location_mi_get_key((GtkWidget*)loc1),
                      location_mi_get_key((GtkWidget*)loc2));
}

static void
gnome_file_saver_remove_location(GnomeFileSaver* file_saver,
                                 const gchar* gconf_key)
{
        GtkWidget *mi;
        GSList *found;
        
        /* This function does NOT assume the location
           is actually in the current list */
        
        found = g_slist_find_custom(file_saver->_priv->locations,
                                    (gchar*)gconf_key,
                                    location_compare_gconf_func);

        mi = found ? found->data : NULL;
        
        if (mi != NULL) {
                file_saver->_priv->locations =
			g_slist_remove(file_saver->_priv->locations, mi);
                gtk_widget_destroy(mi);
        }
}

static void
gnome_file_saver_update_location(GnomeFileSaver* file_saver,
                                 const gchar* key,
                                 GConfValue* value)
{
        GConfValue *car;
        GConfValue *cdr;
        const gchar* human_name;
        const gchar* dirname;
        GtkWidget *mi;
        GSList *found;
        
        if (value == NULL || value->type != GCONF_VALUE_PAIR) {
                /* Weird, whatever. */
                gnome_file_saver_remove_location(file_saver,
                                                 key);
                return;
        }
        
        car = gconf_value_get_car(value);
        cdr = gconf_value_get_cdr(value);

        if (car == NULL || car->type != GCONF_VALUE_STRING ||
            cdr == NULL || cdr->type != GCONF_VALUE_STRING) {
                /* Tsk tsk */
                gnome_file_saver_remove_location(file_saver,
                                                 key);
                return;
        }
        
        human_name = gconf_value_get_string(car);
        dirname = gconf_value_get_string(cdr);

        if (human_name == NULL ||
            dirname == NULL) {
                /* No dice! */
                gnome_file_saver_remove_location(file_saver,
                                                 key);
                return;
        }

        found = g_slist_find_custom(file_saver->_priv->locations,
                                    (gchar*)key,
                                    location_compare_gconf_func);

        mi = found ? found->data : NULL;
        
        if (mi != NULL)
                location_mi_set(mi, human_name, dirname);
        else {
                GSList *iter;
                gint count;
                
                mi = location_mi_new(key, human_name, dirname);

                /* can't use g_slist_insert_sorted because we
                   need to sync with the menu */

                count = 0;
                iter = file_saver->_priv->locations;
                while (iter != NULL) {
                        GtkWidget *old_mi = iter->data;
                        const gchar* old_human_name;

                        old_human_name = location_mi_get_human_name(old_mi);

                        if (strcmp(human_name, old_human_name) >=  0) {
                                /* We're >= than this element */
                                break;
                        }

                        ++count;
                        iter = g_slist_next(iter);
                }

                file_saver->_priv->locations =
			g_slist_insert(file_saver->_priv->locations,
				       mi, count);

                gtk_menu_shell_insert(GTK_MENU_SHELL(file_saver->_priv->location_menu),
                                      mi, count);
        }
}
                
static void
locations_changed_notify(GConfClient* client, guint cnxn_id,
                         GConfEntry *entry, gpointer user_data)
{
        GnomeFileSaver *file_saver;
        GConfValue *value;
        const gchar *key;

        value = gconf_entry_get_value (entry);
        key = gconf_entry_get_key (entry);

        file_saver = GNOME_FILE_SAVER(user_data);
        
        if (value != NULL)
                gnome_file_saver_update_location(file_saver,
                                                 key,
                                                 value);
        else
                gnome_file_saver_remove_location(file_saver,
                                                 key);
}
