/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME GUI Library - gnome-file-saver.c
 * Copyright (C) 2000  Red Hat Inc.,
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

#include "gnome-file-saver.h"
#include "gnome-gconf.h"
#include <gtk/gtk.h>

enum {
        FILENAME_CHOSEN,
        LAST_SIGNAL
};

enum {
        ARG_0,
        ARG_FILENAME,
        LAST_ARG
};

static void gnome_file_saver_init (GnomeFileSaver* file_saver);
static void gnome_file_saver_class_init (GnomeFileSaverClass* klass);
static void gnome_file_saver_destroy (GtkObject* object);
static void gnome_file_saver_finalize (GtkObject* object);
static void gnome_file_saver_set_arg (GtkObject* object, GtkArg* arg, guint arg_id);
static void gnome_file_saver_get_arg (GtkObject* object, GtkArg* arg, guint arg_id);

static void locations_changed_notify(GConfClient* client, guint cnxn_id, const gchar* key, GConfValue* value, gboolean is_default, gpointer user_data);

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

        object_class = (GtkObjectClass*) klass;

        parent_class = gtk_type_class (gnome_dialog_get_type());

        signals[FILENAME_CHOSEN] =
                gtk_signal_new ("filename_chosen",
                                GTK_RUN_LAST,
                                object_class->type,
                                GTK_SIGNAL_OFFSET (GnomeFileSaverClass, filename_chosen),
                                gtk_marshal_NONE__POINTER,
                                GTK_TYPE_NONE, 1, GTK_TYPE_STRING);
        
        gtk_object_class_add_signals (object_class, signals, 
                                      LAST_SIGNAL);

        
        gtk_object_add_arg_type ("GnomeFileSaver::filename", GTK_TYPE_STRING,
                                 GTK_ARG_READABLE, ARG_FILENAME);

        object_class->set_arg = gnome_file_saver_set_arg;
        object_class->get_arg = gnome_file_saver_get_arg;

        object_class->destroy = gnome_file_saver_destroy;
        object_class->finalize = gnome_file_saver_finalize;
}

void
gnome_file_saver_init (GnomeFileSaver* file_saver)
{
        GnomeDialog *dialog;
        GSList *all_entries;
        GSList *iter;
        
        dialog = GNOME_DIALOG(file_saver);
        
        file_saver->entry = gtk_entry_new();
        file_saver->option = gtk_option_menu_new();
        
        gtk_box_pack_start (GTK_BOX(dialog->vbox),
                            file_saver->entry,
                            FALSE, FALSE, 6);

        gtk_box_pack_start (GTK_BOX(dialog->vbox),
                            file_saver->option,
                            FALSE, FALSE, 6);

        file_saver->menu = gtk_menu_new();
        
        file_saver->conf = gnome_gconf_client_get();

        gtk_object_ref(GTK_OBJECT(file_saver->conf));

        /* No preload because the explicit all_entries call below
           effectively results in a preload. */
        gconf_client_add_dir(file_saver->conf,
                             "/desktop/standard/save-locations",
                             GCONF_CLIENT_PRELOAD_NONE, NULL);

        all_entries =
                gconf_client_all_entries(file_saver->conf,
                                         "/desktop/standard/save-locations",
                                         NULL);

        iter = all_entries;
        while (iter != NULL) {
                GConfEntry *entry = iter->data;
                gchar* full_key;

                full_key = gconf_concat_key_and_dir("/desktop/standard/save-locations",
                                                    gconf_entry_key(entry));

                
                gnome_file_saver_update_location(file_saver,
                                                 full_key,
                                                 gconf_entry_value(entry));

                g_free(full_key);
                gconf_entry_destroy(entry);
                
                iter = g_slist_next(iter);
        }
        g_slist_free(all_entries);
        
        file_saver->conf_notify =
                gconf_client_notify_add(file_saver->conf,
                                        "/desktop/standard/save-locations",
                                        locations_changed_notify,
                                        file_saver, NULL, NULL);

        gtk_widget_show(file_saver->menu);
        
        /* Must do _after_ filling the menu */
        gtk_option_menu_set_menu(GTK_OPTION_MENU(file_saver->option),
                                 file_saver->menu);
        
        gtk_widget_show(file_saver->entry);
        gtk_widget_show(file_saver->option);
}

GnomeFileSaver*
gnome_file_saver_new (void)
{
        return GNOME_FILE_SAVER (gtk_type_new (gnome_file_saver_get_type ()));
}

static void
gnome_file_saver_destroy (GtkObject* object)
{
        GnomeFileSaver* file_saver;

        file_saver = GNOME_FILE_SAVER (object);
        
        if (file_saver->conf_notify != 0) {
                gconf_client_notify_remove(file_saver->conf,
                                           file_saver->conf_notify);
                file_saver->conf_notify = 0;
        }

        gconf_client_remove_dir(file_saver->conf,
                                "/desktop/standard/save-locations");
        
        gtk_object_unref(GTK_OBJECT(file_saver->conf));
        
        (* GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

static void
gnome_file_saver_finalize (GtkObject* object)
{
        GnomeFileSaver* file_saver;

        file_saver = GNOME_FILE_SAVER (object);


        (* GTK_OBJECT_CLASS(parent_class)->finalize) (object);
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
        case ARG_FILENAME:
                break;
                
        default:
                arg->type = GTK_TYPE_INVALID;
                break;
        }
}

gchar*
gnome_file_saver_get_filename (GnomeFileSaver *saver)
{
        g_return_val_if_fail(GNOME_IS_FILE_SAVER(saver), NULL);

        /* FIXME */
        
        return NULL;
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

static const gchar*
location_mi_get_dirname(GtkWidget *mi)
{
        return gtk_object_get_data(GTK_OBJECT(mi), "dirname");
}

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

static gint
location_compare_human_func(gconstpointer loc1, gconstpointer loc2)
{

        return strcmp(location_mi_get_human_name((GtkWidget*)loc1),
                      location_mi_get_human_name((GtkWidget*)loc2));
}

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
        
        found = g_slist_find_custom(file_saver->locations,
                                    (gchar*)gconf_key,
                                    location_compare_gconf_func);

        mi = found ? found->data : NULL;
        
        if (mi != NULL) {
                file_saver->locations = g_slist_remove(file_saver->locations,
                                                       mi);
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
        
        if (value->type != GCONF_VALUE_PAIR) {
                /* Weird, whatever. */
                gnome_file_saver_remove_location(file_saver,
                                                 key);
                return;
        }
        
        car = gconf_value_car(value);
        cdr = gconf_value_cdr(value);

        if (car == NULL || car->type != GCONF_VALUE_STRING ||
            cdr == NULL || cdr->type != GCONF_VALUE_STRING) {
                /* Tsk tsk */
                gnome_file_saver_remove_location(file_saver,
                                                 key);
                return;
        }
        
        human_name = gconf_value_string(car);
        dirname = gconf_value_string(cdr);

        if (human_name == NULL ||
            dirname == NULL) {
                /* No dice! */
                gnome_file_saver_remove_location(file_saver,
                                                 key);
                return;
        }

        found = g_slist_find_custom(file_saver->locations,
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
                iter = file_saver->locations;
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

                file_saver->locations = g_slist_insert(file_saver->locations,
                                                       mi, count);

                gtk_menu_insert(GTK_MENU(file_saver->menu),
                                mi, count);
        }
}
                
static void
locations_changed_notify(GConfClient* client, guint cnxn_id,
                         const gchar* key, GConfValue* value,
                         gboolean is_default, gpointer user_data)
{
        GnomeFileSaver *file_saver;

        file_saver = GNOME_FILE_SAVER(user_data);
        
        if (value != NULL)
                gnome_file_saver_update_location(file_saver,
                                                 key,
                                                 value);
        else
                gnome_file_saver_remove_location(file_saver,
                                                 key);
}




