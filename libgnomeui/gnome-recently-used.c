/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME GUI Library - gnome-recently-used.c
 * Copyright (C) 2000  Red Hat Inc.,
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

#include "gnome-recently-used.h"
#include "gnome-macros.h"

#include <sys/time.h>
#include <stdlib.h>

#include <libgnome/gnome-program.h>

#include <gtk/gtk.h>

#include <string.h>
#include <errno.h>
#include <libgnome/gnome-i18n.h>

/* The default maximum number of documents */
#define DEFAULT_MAX 30

struct _GnomeRecentlyUsedPrivate {
        GHashTable   *hash;
        GConfClient  *conf;
        guint         conf_notify;
        GSList       *add_list;
        gchar        *key_root;
        gboolean      app_specific;
};


enum {
        DOCUMENT_ADDED,
        DOCUMENT_REMOVED,
        DOCUMENT_CHANGED,
        LAST_SIGNAL
};

enum {
        PROP_0,
        PROP_APP_SPECIFIC,
        LAST_PARAM
};

static void                 gnome_recently_used_init               (GnomeRecentlyUsed      *recently_used);
static void                 gnome_recently_used_class_init         (GnomeRecentlyUsedClass *klass);
static void                 gnome_recently_used_destroy            (GtkObject              *object);
static void                 gnome_recently_used_finalize           (GObject                *object);

static void                 gnome_recently_used_get_property          (GObject                *object,
								    guint                   param_id,
								    GValue                 *value,
								    GParamSpec             *pspec);
static void                 gnome_recently_used_set_property          (GObject                *object,
								    guint                   param_id,
								    const GValue           *value,
								    GParamSpec             *pspec);

static GConfValue*          gnome_recent_document_to_gconf_value   (GnomeRecentDocument    *doc);
static GnomeRecentDocument* gnome_recent_document_from_gconf_value (GConfValue             *val);
static void                 gnome_recent_document_fill_from_gconf_value (GnomeRecentDocument *doc,
                                                                         GConfValue        *val);

static void                 gnome_recent_document_set_gconf_key    (GnomeRecentDocument    *doc,
                                                                    const gchar            *key);
static const gchar*         gnome_recent_document_get_gconf_key    (GnomeRecentDocument    *doc);
static void                 documents_changed_notify               (GConfClient            *client,
                                                                    guint                   cnxn_id,
                                                                    GConfEntry             *entry,
                                                                    gpointer                user_data);


static guint signals[LAST_SIGNAL] = { 0 };

GNOME_CLASS_BOILERPLATE(GnomeRecentlyUsed, gnome_recently_used,
			GtkObject, gtk_object)

static void
gnome_recently_used_class_init (GnomeRecentlyUsedClass* klass)
{
        GtkObjectClass* object_class;
        GObjectClass* gobject_class;

        object_class = (GtkObjectClass*) klass;
        gobject_class = (GObjectClass*) klass;

	g_object_class_install_property (gobject_class,
				      PROP_APP_SPECIFIC,
				      g_param_spec_boolean ("app_specific",
							    _("App Specific"),
							    _("Is this list "
							      "application "
							      "specific"),
							    FALSE,
							    (G_PARAM_READABLE |
							     G_PARAM_WRITABLE)));
        signals[DOCUMENT_ADDED] =
                gtk_signal_new ("document_added",
                                GTK_RUN_FIRST,
                                GTK_CLASS_TYPE (object_class),
                                GTK_SIGNAL_OFFSET (GnomeRecentlyUsedClass, document_added),
                                gtk_marshal_NONE__POINTER,
                                GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

        signals[DOCUMENT_REMOVED] =
                gtk_signal_new ("document_removed",
                                GTK_RUN_FIRST,
                                GTK_CLASS_TYPE (object_class),
                                GTK_SIGNAL_OFFSET (GnomeRecentlyUsedClass, document_removed),
                                gtk_marshal_NONE__POINTER,
                                GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

        signals[DOCUMENT_CHANGED] =
                gtk_signal_new ("document_changed",
                                GTK_RUN_FIRST,
                                GTK_CLASS_TYPE (object_class),
                                GTK_SIGNAL_OFFSET (GnomeRecentlyUsedClass, document_changed),
                                gtk_marshal_NONE__POINTER,
                                GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);


        object_class->destroy = gnome_recently_used_destroy;

        gobject_class->set_property = gnome_recently_used_set_property;
        gobject_class->get_property = gnome_recently_used_get_property;
        gobject_class->finalize = gnome_recently_used_finalize;
}

void
gnome_recently_used_init (GnomeRecentlyUsed* recently_used)
{
	recently_used->_priv = g_new0 (GnomeRecentlyUsedPrivate, 1);

}

/* Does not actually load data, that is, it doesn't init the hash.
 * All this does is set up the keys and the gconf notification */
static void
gnome_recently_used_ensure (GnomeRecentlyUsed *recently_used)
{
	if (recently_used->_priv->conf != NULL) {
		/* if the conf is set, we are all set up */
		return;
	}
        
        if (recently_used->_priv->app_specific) {
                recently_used->_priv->key_root = g_strconcat ("/apps/gnome-settings/",
							      gnome_program_get_name (gnome_program_get()),
							      "/recent-documents",
							      NULL);
	} else {
                recently_used->_priv->key_root = g_strdup ("/desktop/standard/recent-documents");
        }
        
        recently_used->_priv->conf = gnome_get_gconf_client ();

        g_object_ref (G_OBJECT (recently_used->_priv->conf));
        
        /* No preload because the explicit all_entries call when ensuring
           effectively results in a preload. */
        gconf_client_add_dir (recently_used->_priv->conf,
			      recently_used->_priv->key_root,
			      GCONF_CLIENT_PRELOAD_NONE, NULL);
        
        recently_used->_priv->conf_notify =
                gconf_client_notify_add (recently_used->_priv->conf,
					 recently_used->_priv->key_root,
					 documents_changed_notify,
					 recently_used, NULL, NULL);
}

/* Actually load data */
static void
gnome_recently_used_ensure_data (GnomeRecentlyUsed *recently_used)
{
        GSList *all_entries;
        GSList *iter;

	if (recently_used->_priv->hash != NULL) {
		/* if the hash is set, we are all set up */
		return;
	}

	if (recently_used->_priv->conf == NULL)
		gnome_recently_used_ensure (recently_used);
        
        
        all_entries =
                gconf_client_all_entries (recently_used->_priv->conf,
					  recently_used->_priv->key_root,
					  NULL);

        recently_used->_priv->hash = g_hash_table_new (g_str_hash, g_str_equal);
        
        iter = all_entries;
        while (iter != NULL) {
                GConfEntry *entry = iter->data;
                gchar* full_key;
                GnomeRecentDocument *doc;
                
                full_key = gconf_concat_dir_and_key (recently_used->_priv->key_root,
                                                     gconf_entry_get_key(entry));

                doc = gnome_recent_document_from_gconf_value (gconf_entry_get_value (entry));

                if (doc != NULL) {
                        gnome_recent_document_ref (doc);
                        gnome_recent_document_set_gconf_key (doc, full_key);
                        
                        g_hash_table_insert (recently_used->_priv->hash,
					     (gchar*)gnome_recent_document_get_gconf_key(doc),
					     doc);
                }

                g_free (full_key);
                gconf_entry_free (entry);
                
                iter = g_slist_next (iter);
        }
        g_slist_free (all_entries);
}

static void
destroy_foreach (gpointer key, gpointer value, gpointer user_data)
{
        gnome_recent_document_unref (value);
}

static void
gnome_recently_used_clear (GnomeRecentlyUsed *recently_used)
{
        GSList *iter;

	if (recently_used->_priv->add_list != NULL) {
		iter = recently_used->_priv->add_list;
		while (iter != NULL) {
			gnome_recent_document_unref(iter->data);

			iter = g_slist_next(iter);
		}

		g_slist_free(recently_used->_priv->add_list);
		recently_used->_priv->add_list = NULL;
	}

	if (recently_used->_priv->hash != NULL) {
		g_hash_table_foreach(recently_used->_priv->hash,
				     destroy_foreach,
				     recently_used);

		g_hash_table_destroy(recently_used->_priv->hash);
		recently_used->_priv->hash = NULL;
	}

	if (recently_used->_priv->conf != NULL) {
		if (recently_used->_priv->conf_notify != 0) {
			gconf_client_notify_remove(recently_used->_priv->conf,
						   recently_used->_priv->conf_notify);
			recently_used->_priv->conf_notify = 0;
		}

		gconf_client_remove_dir(recently_used->_priv->conf,
					recently_used->_priv->key_root,
                                        NULL);


		g_object_unref(G_OBJECT(recently_used->_priv->conf));
		recently_used->_priv->conf = NULL;
	}
}

GnomeRecentlyUsed*
gnome_recently_used_new (void)
{
        GnomeRecentlyUsed *recently_used;

        recently_used = GNOME_RECENTLY_USED (gtk_type_new (gnome_recently_used_get_type ()));

	recently_used->_priv->app_specific = FALSE;

	gnome_recently_used_ensure (recently_used);

        return recently_used;
}

GnomeRecentlyUsed*
gnome_recently_used_new_app_specific (void)
{
        GnomeRecentlyUsed *recently_used;

        recently_used = GNOME_RECENTLY_USED (gtk_type_new (gnome_recently_used_get_type ()));

	recently_used->_priv->app_specific = FALSE;

	gnome_recently_used_ensure (recently_used);
        
        return recently_used;
}

static void
gnome_recently_used_get_property (GObject      *object,
			       guint         param_id,
			       GValue       *value,
			       GParamSpec   *pspec)
{
        GnomeRecentlyUsed *recently_used;

        recently_used = GNOME_RECENTLY_USED(object);

        switch (param_id) {
        case PROP_APP_SPECIFIC:
		g_value_set_boolean (value,
				     recently_used->_priv->app_specific);
                break;

        default:
                break;
        }
}

static void
gnome_recently_used_set_property (GObject      *object,
			       guint         param_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
        GnomeRecentlyUsed *recently_used;

        recently_used = GNOME_RECENTLY_USED(object);

        switch (param_id) {
        case PROP_APP_SPECIFIC:
		gnome_recently_used_clear (recently_used);
                recently_used->_priv->app_specific = g_value_get_boolean (value);
                gnome_recently_used_ensure (recently_used);
                break;

        default:
                break;
        }
}

static void
gnome_recently_used_destroy (GtkObject* object)
{
        GnomeRecentlyUsed* recently_used;

	/* remember, destroy can be run multiple times! */

        recently_used = GNOME_RECENTLY_USED (object);

	gnome_recently_used_clear (recently_used);

	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_recently_used_finalize (GObject* object)
{
        GnomeRecentlyUsed* recently_used;
        
        recently_used = GNOME_RECENTLY_USED (object);

	g_free(recently_used->_priv);
	recently_used->_priv = NULL;
        
	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}

/*
 * Manipulate the recently used list
 */

/* The minus_one is a flag if we should subtract one, because we're about to 
 * add an entry, This is not a perfect way to keep track of the maximum number
 * of items, there are possible races etc...  But this is 1) simple 2) absolutely
 * non-critical.  In the worst case we'll have one or two entries more if there is
 * a race, I think it's worth the simiplicity. */
static void
gnome_recently_used_truncate (GnomeRecentlyUsed *recently_used,
			      gboolean minus_one)
{
        gchar *full_key;
	int max, size;
	GConfValue *value;
	GSList *list, *li;

        full_key = gconf_concat_dir_and_key(recently_used->_priv->key_root,
                                            "MaximumSize");

	value = gconf_client_get (recently_used->_priv->conf,
				  full_key, NULL);

	g_free (full_key);

	/* use DEFAULT_MAX */
	if (value != NULL) {
		max = gconf_value_get_int (value);
		gconf_value_free (value);
	} else {
		max = DEFAULT_MAX;
	}

	/* Weird value == no limit */
	if (max <= 0)
		return;

	if (minus_one)
		max--;

	size = g_hash_table_size (recently_used->_priv->hash);
	if (size <= max)
		return;

	/* This is inefficent, but ok I'd say */
	list = gnome_recently_used_get_all (recently_used);

	list = g_slist_reverse (list);
	li = list;

	while (size > max &&
	       li != NULL/* just for sanity, should never happen */) {
		GnomeRecentDocument *doc = li->data;

		gnome_recently_used_remove (recently_used, doc);

		li = li->next;
		size --;
	}

	g_slist_free (list);
}

/**
 * gnome_recently_used_add:
 * @recently_used: the #GnomeRecentlyUsed object
 * @doc: document to add
 *
 * Description:  Add a document to the recently used list
 **/
void
gnome_recently_used_add (GnomeRecentlyUsed   *recently_used,
                         GnomeRecentDocument *doc)
{
        gchar *key;
        gchar *full_key;
        GConfValue *val;
        
	g_return_if_fail (recently_used != NULL);
	g_return_if_fail (GNOME_IS_RECENTLY_USED (recently_used));
	g_return_if_fail (doc != NULL);
        g_return_if_fail (gnome_recent_document_get_gconf_key (doc) == NULL);

	/* load data if we haven't done that yet */
	gnome_recently_used_ensure_data (recently_used);

	/* "Make room", that is truncate the list to max (minus one) items */
	gnome_recently_used_truncate (recently_used, TRUE /* minus_one */);

        key = gconf_unique_key ();

        full_key = gconf_concat_dir_and_key(recently_used->_priv->key_root,
                                            key);

        g_free(key);

        gnome_recent_document_set_gconf_key(doc, full_key);

        val = gnome_recent_document_to_gconf_value(doc);
        
        gconf_client_set (recently_used->_priv->conf,
			  full_key,
			  val,
			  NULL);

        gconf_value_free(val);

        g_free (full_key);

        /* Store it in a list; we don't actually add it to the hash
           until we get notification from GConf. But we need to
           keep this same GnomeRecentDocument rather than creating
           another one when we get that notification.
        */
        gnome_recent_document_ref (doc);
        recently_used->_priv->add_list = g_slist_prepend (recently_used->_priv->add_list, doc);
}

void
gnome_recently_used_remove (GnomeRecentlyUsed   *recently_used,
                            GnomeRecentDocument *doc)
{
        g_return_if_fail(GNOME_IS_RECENTLY_USED(recently_used));
        
        gconf_client_unset(recently_used->_priv->conf,
                           gnome_recent_document_get_gconf_key(doc),
                           NULL);
}

static void
listize_foreach(gpointer key, gpointer value, gpointer user_data)
{
        GSList **list = user_data;

        *list = g_slist_prepend(*list, value);
}

static gint
descending_chronological_compare_func(gconstpointer a, gconstpointer b)
{
        GTime a_time;
        GTime b_time;

        a_time = gnome_recent_document_get_creation_time((GnomeRecentDocument*)a);
        b_time = gnome_recent_document_get_creation_time((GnomeRecentDocument*)b);

        if (a_time > b_time)
                return -1;
        else if (a_time < b_time)
                return 1;
        else
                return 0;
}

GSList*
gnome_recently_used_get_all (GnomeRecentlyUsed   *recently_used)
{
        GSList *list = NULL;
        
        g_return_val_if_fail (recently_used != NULL, NULL);
        g_return_val_if_fail (GNOME_IS_RECENTLY_USED(recently_used), NULL);

	/* load data if we haven't done that yet */
	gnome_recently_used_ensure_data (recently_used);

        g_hash_table_foreach (recently_used->_priv->hash, listize_foreach,
			      &list);

        /* Sort from newest to oldest */
        list = g_slist_sort (list, descending_chronological_compare_func);
        
        return list;
}

void
gnome_recently_used_document_changed (GnomeRecentlyUsed   *recently_used,
                                      GnomeRecentDocument *doc)
{
        GConfValue *val;
        const gchar *key;

        g_return_if_fail (recently_used != NULL);
        g_return_if_fail (GNOME_IS_RECENTLY_USED(recently_used));
        g_return_if_fail (doc != NULL);

	/* load data if we haven't done that yet */
	gnome_recently_used_ensure_data (recently_used);
        
        gnome_recent_document_ref(doc);

        key = gnome_recent_document_get_gconf_key(doc);

        g_return_if_fail(key != NULL);
        g_return_if_fail(g_hash_table_lookup(recently_used->_priv->hash, key) == NULL);
        
        val = gnome_recent_document_to_gconf_value(doc);
        
        gconf_client_set(recently_used->_priv->conf,
                         key,
                         val,
                         NULL);

        gconf_value_free(val);        

        gnome_recent_document_unref(doc);
}

static void
documents_changed_notify(GConfClient* client, guint cnxn_id,
                         GConfEntry *entry, gpointer user_data)
{
        GnomeRecentlyUsed *recently_used;
        GnomeRecentDocument *doc;
        GConfValue *value;
        const gchar *key;

        value = gconf_entry_get_value (entry);
        key = gconf_entry_get_key (entry);
        
        recently_used = GNOME_RECENTLY_USED(user_data);

	/* load data if we haven't done that yet */
	gnome_recently_used_ensure_data (recently_used);

        doc = g_hash_table_lookup(recently_used->_priv->hash,
                                  key);

        if (doc != NULL) {
                if (value == NULL) {
                        /* Has been removed */
                        gtk_signal_emit(GTK_OBJECT(recently_used),
                                        signals[DOCUMENT_REMOVED],
                                        doc);

                        gnome_recent_document_unref(doc);
                        
                        g_hash_table_remove(recently_used->_priv->hash, key);
                        
                        return;
                } else {
                        /* This entry has changed */
                        gnome_recent_document_fill_from_gconf_value(doc,
                                                                    value);
                        
                        gtk_signal_emit(GTK_OBJECT(recently_used),
                                        signals[DOCUMENT_CHANGED],
                                        doc);
                        
                        return;
                }
        } else {
                /* This is a new entry */

                GSList *iter;

                if (value == NULL) {
                        /* Someone deleted an entry we never knew about anyway */
                        return;
                }
                
                /* Scan the add list, so we recycle the
                   GnomeRecentDocument if the addition came from the
                   same process (and more importantly maintain the
                   mapping of one GnomeRecentDocument per gconf key) */

                iter = recently_used->_priv->add_list;

                while (iter != NULL) {
                        const gchar* this_key;

                        this_key = gnome_recent_document_get_gconf_key(iter->data);

                        g_assert(this_key != NULL);
                        
                        if (strcmp(key, this_key) == 0) {
                                doc = iter->data;
                                break;
                        }

                        iter = g_slist_next(iter);
                }

                if (doc != NULL) {
                        /* Here we assume the value received is the same
                           as the value we sent (since we don't fill_from_value)
                           which isn't necessarily true but is a lot faster
                           and will pretty much always be true */
                        recently_used->_priv->add_list = g_slist_remove(recently_used->_priv->add_list,
									doc);

                        g_hash_table_insert(recently_used->_priv->hash,
                                            (gchar*)gnome_recent_document_get_gconf_key(doc),
                                            doc);
                } else {
                        doc = gnome_recent_document_from_gconf_value(value);

                        if (doc == NULL) {
                                /* Value was junk somehow */
                                return;
                        }
                        
                        gnome_recent_document_set_gconf_key(doc, key);
                        g_hash_table_insert(recently_used->_priv->hash,
                                            (gchar*)gnome_recent_document_get_gconf_key(doc),
                                            doc);
                }

                if (doc != NULL) {
                        gtk_signal_emit(GTK_OBJECT(recently_used),
                                        signals[DOCUMENT_ADDED],
                                        doc);
                }
        }
}


void
gnome_recently_used_add_simple       (GnomeRecentlyUsed   *recently_used,
                                      const gchar         *command,
                                      const gchar         *menu_text,
                                      const gchar         *menu_pixmap,
                                      const gchar         *menu_hint,
                                      const gchar         *filename,
                                      const gchar         *mime_type)
{
        GnomeRecentDocument *doc;
        const gchar *appid;
        
        doc = gnome_recent_document_new();

        if (command)
                gnome_recent_document_set(doc, "command", command);

        if (menu_text)
                gnome_recent_document_set(doc, "menu-text", menu_text);

        if (menu_pixmap)
                gnome_recent_document_set(doc, "menu-pixmap", menu_pixmap);

        if (menu_hint)
                gnome_recent_document_set(doc, "menu-hint", menu_hint);

        if (filename)
                gnome_recent_document_set(doc, "filename", filename);

        if (mime_type)
                gnome_recent_document_set(doc, "mime-type", mime_type);
        
        appid = gnome_program_get_name(gnome_program_get());

        gnome_recent_document_set(doc, "app", appid);
        
        gnome_recently_used_add (recently_used, doc);

        gnome_recent_document_unref(doc);
}

/*
 * Recent document data type
 */

struct _GnomeRecentDocument {
        guint refcount;

        /* Could use a hash table here and save some code,
           but it's quite a bit more RAM and likely not faster
           for this number of elements */
        gchar *command;
        gchar *menu_text;
        gchar *menu_pixmap;
        gchar *menu_hint;
        gchar *filename;
        gchar *mime_type;
        gchar *gconf_key;
        gchar *app_id;

        GTime creation_time;
};

GnomeRecentDocument*
gnome_recent_document_new (void)
{
        GnomeRecentDocument *doc;

        doc = g_new0(GnomeRecentDocument, 1);

        doc->refcount = 1;

        doc->creation_time = time(NULL);
        
        return doc;
}

GnomeRecentDocument *
gnome_recent_document_ref (GnomeRecentDocument *doc)
{
        g_return_val_if_fail(doc != NULL, NULL);

        doc->refcount += 1;

	return doc;
}

static void
clear_doc (GnomeRecentDocument *doc)
{
        /* ignore gconf_key, creation_time, and refcount
           since we keep those when filling a doc from
           a GConfValue */
        
        g_free(doc->command);
        doc->command = NULL;

        g_free(doc->menu_hint);
        doc->menu_hint = NULL;

        g_free(doc->menu_pixmap);
        doc->menu_pixmap = NULL;

        g_free(doc->menu_text);
        doc->menu_text = NULL;

        g_free(doc->filename);
        doc->filename = NULL;

        g_free(doc->mime_type);
        doc->mime_type = NULL;

        g_free(doc->app_id);
        doc->app_id = NULL;
}

void
gnome_recent_document_unref (GnomeRecentDocument *doc)
{
        g_return_if_fail(doc != NULL);
        g_return_if_fail(doc->refcount > 0);
        
        doc->refcount -= 1;

        if (doc->refcount == 0) {
                clear_doc(doc);

                g_free(doc->gconf_key);
                doc->gconf_key = NULL;

                g_free(doc);
        }
}

static gchar**
find_arg(GnomeRecentDocument *doc,
         const gchar *arg)
{
        if (strcmp(arg, "command") == 0)
                return &doc->command;
        else if (strcmp(arg, "menu-text") == 0)
                return &doc->menu_text;
        else if (strcmp(arg, "menu-pixmap") == 0)
                return &doc->menu_pixmap;
        else if (strcmp(arg, "menu-hint") == 0)
                return &doc->menu_hint;
        else if (strcmp(arg, "filename") == 0)
                return &doc->filename;
        else if (strcmp(arg, "mime-type") == 0)        
                return &doc->mime_type;
        else if (strcmp(arg, "app") == 0)
                return &doc->app_id;
        else {
                g_warning("Unknown GnomeRecentDocument arg %s", arg);
                return NULL;
        }
}

/* Available args:
      "command" - command to run when menu item is selected
      "menu-text" - menu text to display
      "menu-pixmap" - pixmap filename to display
      "menu-hint" - menu hint to put in statusbar
      "filename" - document filename (maybe used if command isn't run)
*/
void
gnome_recent_document_set            (GnomeRecentDocument *doc,
                                      const gchar         *arg,
                                      const gchar         *val)
{
        gchar** setme;

        g_return_if_fail (doc != NULL);
        g_return_if_fail (arg != NULL);

        setme = find_arg(doc, arg);

        /* This check is required, since arg may come in from GConf and
           be invalid */

        setme = find_arg(doc, arg);

        /* This check is required, since arg may come in from GConf and
           be invalid */
        if (setme == NULL) {
                g_warning(_("bad GnomeRecentDocument attribute: %s\n"), arg);
                return;
        }
        
        if (*setme != NULL)
                g_free(*setme);

        *setme = val ? g_strdup(val) : NULL;
}

const gchar*
gnome_recent_document_peek (GnomeRecentDocument *doc,
			    const gchar         *arg)
{
        gchar** getme;

        g_return_val_if_fail (doc != NULL, NULL);
        g_return_val_if_fail (arg != NULL, NULL);

        getme = find_arg(doc, arg);

        if (getme)
                return *getme;
        else {
                g_warning("bad GnomeRecentDocument attribute: %s\n", arg);
                return NULL;
        }
}

GTime
gnome_recent_document_get_creation_time (GnomeRecentDocument *doc)
{
        g_return_val_if_fail (doc != NULL, 0);

        return doc->creation_time;
}

static void
gnome_recent_document_set_gconf_key    (GnomeRecentDocument    *doc,
                                        const gchar            *key)
{
        g_return_if_fail (doc != NULL);
        g_return_if_fail (key != NULL);

        if (doc->gconf_key != NULL)
                g_free(doc->gconf_key);

        doc->gconf_key = g_strdup(key);
}

static const gchar*
gnome_recent_document_get_gconf_key    (GnomeRecentDocument    *doc)
{
        g_return_val_if_fail (doc != NULL, NULL);

        return doc->gconf_key;
}

static gchar*
encode_arg (const gchar* arg, const gchar* val)
{
        if (val == NULL)
                return NULL;
        else
                return g_strconcat(arg, ":", val, NULL);
}

static void
decode_arg(const gchar* arg, gchar** argp, gchar** valp)
{
        const gchar* colon;

        colon = strchr(arg, ':');

        if (colon == NULL)
                goto failed;

        *argp = g_strndup(arg, colon - arg);
        ++colon;
        *valp = g_strdup(colon);
        
        return;
        
 failed:
        *argp = NULL;
        *valp = NULL;
}

static void
add_arg(GnomeRecentDocument *doc, GSList** listp, const gchar* argname, const gchar* argval)
{
        gchar* encoded;
        GConfValue* val;

        if (argval == NULL)
                return;
        
        encoded = encode_arg(argname, argval);

        g_assert(encoded);

                
        val = gconf_value_new(GCONF_VALUE_STRING);
        gconf_value_set_string(val, encoded);
        g_free(encoded);
        
        *listp = g_slist_prepend(*listp, val);
}

static GConfValue*
gnome_recent_document_to_gconf_value (GnomeRecentDocument *doc)
{
        GConfValue *val;
        GSList *list = NULL;
        gchar *t;

	g_return_val_if_fail (doc != NULL, NULL);
        
        val = gconf_value_new(GCONF_VALUE_LIST);
        gconf_value_set_list_type(val, GCONF_VALUE_STRING);
        
        add_arg(doc, &list, "command", doc->command);
        add_arg(doc, &list, "menu-text", doc->menu_text);
        add_arg(doc, &list, "menu-pixmap", doc->menu_pixmap);
        add_arg(doc, &list, "menu-hint", doc->menu_hint);
        add_arg(doc, &list, "filename", doc->filename);
        add_arg(doc, &list, "mime-type", doc->mime_type);
        add_arg(doc, &list, "app", doc->app_id);

        t = g_strdup_printf("%lu", (gulong)doc->creation_time);

        add_arg(doc, &list, "creation-time", t);
        g_free(t);
        
        gconf_value_set_list_nocopy(val, list);

        return val;
}

static gulong
string_to_gulong(const gchar* str)
{
	gulong retval;

	errno = 0;
	retval = strtoul(str, NULL, 10);
	if (errno != 0)
		retval = 0;

	return retval;
}

static gboolean
fill_doc(GnomeRecentDocument **docp, GConfValue *val)
{
        GSList *iter;
        
        if (val == NULL)
                return FALSE;

        if (val->type != GCONF_VALUE_LIST)
                return FALSE;

        if (gconf_value_get_list_type(val) != GCONF_VALUE_STRING)
                return FALSE;
        
        iter = gconf_value_get_list(val);

        while (iter != NULL) {
                GConfValue *elem;

                elem = iter->data;
                
                g_assert(elem != NULL);

                if (elem->type == GCONF_VALUE_STRING) {
                        const gchar* encoded;
                        gchar* arg = NULL;
                        gchar* argval = NULL;
                        
                        encoded = gconf_value_get_string(elem);

                        decode_arg(encoded, &arg, &argval);

                        if (arg) {
                                /* create the doc only if we get > 0 args */
                                if (*docp == NULL)
                                        *docp = gnome_recent_document_new();

                                if (strcmp(arg, "creation-time") == 0) {
                                        (*docp)->creation_time = string_to_gulong(argval);
                                } else {
                                        gnome_recent_document_set(*docp, arg, argval);
                                }
                        }

                        g_free(arg);
                        g_free(argval);
                }
                
                iter = g_slist_next(iter);
        }

        return TRUE;
}

static GnomeRecentDocument*
gnome_recent_document_from_gconf_value (GConfValue *val)
{
        GnomeRecentDocument *doc = NULL;

        fill_doc(&doc, val);

        if (doc && doc->menu_text == NULL) {
                /* Junk value */
                gnome_recent_document_unref(doc);
                return NULL;
        }
                
        return doc;        
}

static void
gnome_recent_document_fill_from_gconf_value (GnomeRecentDocument *doc,
                                             GConfValue *val)
{
        clear_doc(doc);
        fill_doc(&doc, val);
        /* guarantee that menu name always exists */
        if (doc->menu_text == NULL)
                doc->menu_text = g_strdup(_("Unknown"));
}
