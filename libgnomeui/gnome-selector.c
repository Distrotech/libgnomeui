/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 4; tab-width: 8 -*- */
/*
 * Copyright (C) 2000 SuSE GmbH
 * Author: Martin Baulig <baulig@suse.de>
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* GnomeSelector widget - pure virtual widget.
 *
 *   Use the Gnome{File,Icon,Pixmap}Selector subclasses.
 *
 * Author: Martin Baulig <baulig@suse.de>
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtksignal.h>
#include <libgnome/gnome-i18n.h>
#include "gnome-macros.h"
#include "gnome-selectorP.h"
#include "gnome-uidefs.h"

#include <libgnome/libgnome-init.h>
#include <bonobo/bonobo-config-database.h>

#include <libgnomeuiP.h>

#undef DEBUG_ASYNC_HANDLE

typedef struct _GnomeSelectorListData    GnomeSelectorListData;
typedef struct _GnomeSelectorHistoryItem GnomeSelectorHistoryItem;
typedef struct _GnomeSelectorAsyncData   GnomeSelectorAsyncData;

struct _GnomeSelectorPrivate {
    Bonobo_ConfigDatabase db;

    gchar       *history_id;
    gchar       *dialog_title;

    gchar       *gconf_history_dir;
    gchar       *gconf_history_key;
    gchar       *gconf_uri_list_key;

    GPtrArray   *lists;

    GSList      *history;
    guint        max_history_length;

    GtkWidget   *entry_widget;
    GtkWidget   *selector_widget;
    GtkWidget   *browse_dialog;

    GtkWidget   *box;
    GtkWidget   *hbox;
    GtkWidget   *browse_button;
    GtkWidget   *clear_button;
    GtkWidget   *default_button;

    guint32      flags;

    guint32      want_default_behaviour : 1;
    guint32      use_default_entry_widget : 1;
    guint32      use_default_selector_widget : 1;
    guint32      use_default_browse_dialog : 1;
    guint32      want_browse_button : 1;
    guint32      want_clear_button : 1;
    guint32      want_default_button : 1;
    guint32      auto_save_history : 1;
    guint32      auto_save_all : 1;

    guint32      constructed : 1;

    guint32      changed : 1;
    guint32      history_changed : 1;
    guint32      need_rebuild : 1;
    guint32      dirty : 1;

    guint        frozen;

    GList       *async_ops;
};

struct _GnomeSelectorListData {
    GQuark quark;
    GSList *list;
};

struct _GnomeSelectorHistoryItem {
    gboolean save;
    gchar *text;
};

struct _GnomeSelectorAsyncData {
    gpointer async_data;
    GDestroyNotify async_data_destroy;
};

struct _GnomeSelectorAsyncHandle {
    int refcount;

    GnomeSelectorAsyncType async_type;

    GnomeSelector *selector;
    GnomeSelectorAsyncFunc async_func;
    gpointer user_data;

    GError *error;

    gboolean success;
    gboolean destroyed;
    gboolean completed;

    gchar *uri;

    GSList *async_data_list;
};

static void gnome_selector_load_all            (GnomeSelector *selector);
#if 0
static void gnome_selector_save_all            (GnomeSelector *selector);
#endif

static void gnome_selector_class_init          (GnomeSelectorClass *class);
static void gnome_selector_init                (GnomeSelector      *selector);
static void gnome_selector_destroy             (GtkObject          *object);
static void gnome_selector_finalize            (GObject            *object);

static void gnome_selector_get_property        (GObject            *object,
                                                guint               param_id,
                                                GValue             *value,
                                                GParamSpec         *pspec);
static void gnome_selector_set_property        (GObject            *object,
                                                guint               param_id,
                                                const GValue       *value,
                                                GParamSpec         *pspec);

static GObject *gnome_selector_constructor     (GType                  type,
						guint                  n_construct_properties,
						GObjectConstructParam *construct_properties);

static void     update_handler                 (GnomeSelector            *selector);
static void     browse_handler                 (GnomeSelector            *selector);
static void     clear_handler                  (GnomeSelector            *selector,
						guint                     list_id);

static gchar   *get_uri_handler                (GnomeSelector            *selector);
static void     set_uri_handler                (GnomeSelector            *selector,
                                                const gchar              *uri,
						GnomeSelectorAsyncHandle *async_handle);

static void     add_uri_handler                (GnomeSelector            *selector,
                                                const gchar              *uri,
                                                gint                      position,
						guint                     list_id,
						GnomeSelectorAsyncHandle *async_handle);

static void     add_file_handler               (GnomeSelector            *selector,
                                                const gchar              *uri,
                                                gint                      position,
						guint                     list_id,
						GnomeSelectorAsyncHandle *async_handle);

static void     add_directory_handler          (GnomeSelector            *selector,
                                                const gchar              *uri,
                                                gint                      position,
						guint                     list_id,
						GnomeSelectorAsyncHandle *async_handle);
static void     add_uri_list_handler           (GnomeSelector            *selector,
						GSList                   *list,
						gint                      position,
						guint                     list_id,
						GnomeSelectorAsyncHandle *async_handle);

static GSList  *get_uri_list_handler           (GnomeSelector            *selector,
                                                guint                     list_id);

static void     do_construct_handler           (GnomeSelector            *selector);



static void     free_entry_func                (gpointer         data,
                                                gpointer         user_data);



#define GNOME_SELECTOR_GCONF_DIR "/desktop/standard/gnome-selector"

enum {
    PROP_0,

    /* Construction properties */
    PROP_ENTRY_WIDGET,
    PROP_SELECTOR_WIDGET,
    PROP_BROWSE_DIALOG,
    PROP_WANT_DEFAULT_BEHAVIOUR,
    PROP_USE_DEFAULT_ENTRY_WIDGET,
    PROP_USE_DEFAULT_SELECTOR_WIDGET,
    PROP_USE_DEFAULT_BROWSE_DIALOG,
    PROP_WANT_BROWSE_BUTTON,
    PROP_WANT_CLEAR_BUTTON,
    PROP_WANT_DEFAULT_BUTTON,
    PROP_AUTO_SAVE_HISTORY,
    PROP_AUTO_SAVE_ALL,

    /* Normal properties */
    PROP_DIALOG_TITLE,
    PROP_HISTORY_ID,
    PROP_SELECTION_MODE
};

enum {
    CHANGED_SIGNAL,
    BROWSE_SIGNAL,
    CLEAR_SIGNAL,
    CHECK_FILENAME_SIGNAL,
    GET_URI_SIGNAL,
    SET_URI_SIGNAL,
    ADD_FILE_SIGNAL,
    CHECK_DIRECTORY_SIGNAL,
    ADD_DIRECTORY_SIGNAL,
    FREEZE_SIGNAL,
    UPDATE_SIGNAL,
    UPDATE_URI_LIST_SIGNAL,
    THAW_SIGNAL,
    GET_SELECTION_MODE_SIGNAL,
    SET_SELECTION_MODE_SIGNAL,
    GET_SELECTION_SIGNAL,
    SELECTION_CHANGED_SIGNAL,
    SET_ENTRY_TEXT_SIGNAL,
    GET_ENTRY_TEXT_SIGNAL,
    ACTIVATE_ENTRY_SIGNAL,
    HISTORY_CHANGED_SIGNAL,
    GET_URI_LIST_SIGNAL,
    ADD_URI_LIST_SIGNAL,
    ADD_URI_SIGNAL,
    LAST_SIGNAL
};

static int gnome_selector_signals [LAST_SIGNAL] = {0};

/**
 * gnome_selector_get_type
 *
 * Returns the type assigned to the GnomeSelector widget.
 **/
/* The following defines the get_type */
GNOME_CLASS_BOILERPLATE (GnomeSelector, gnome_selector,
			 GtkVBox, gtk_vbox)


static void
gnome_selector_class_init (GnomeSelectorClass *class)
{
    GtkObjectClass *object_class;
    GObjectClass *gobject_class;

    object_class = (GtkObjectClass *) class;
    gobject_class = (GObjectClass *) class;

    gnome_selector_signals [CHANGED_SIGNAL] =
	gtk_signal_new ("changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   changed),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [BROWSE_SIGNAL] =
	gtk_signal_new ("browse",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   browse),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [CLEAR_SIGNAL] =
	gtk_signal_new ("clear",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   clear),
			gnome_marshal_VOID__UINT,
			GTK_TYPE_NONE, 1,
			GTK_TYPE_UINT);
    gnome_selector_signals [FREEZE_SIGNAL] =
	gtk_signal_new ("freeze",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   freeze),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [UPDATE_SIGNAL] =
	gtk_signal_new ("update",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   update),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [THAW_SIGNAL] =
	gtk_signal_new ("thaw",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   thaw),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [GET_URI_SIGNAL] =
	gtk_signal_new ("get_uri",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   get_uri),
			gnome_marshal_STRING__VOID,
			GTK_TYPE_STRING,
			0);
    gnome_selector_signals [SET_URI_SIGNAL] =
	gtk_signal_new ("set_uri",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   set_uri),
			gnome_marshal_VOID__STRING_BOXED,
			GTK_TYPE_NONE, 2,
			GTK_TYPE_STRING,
			GNOME_TYPE_SELECTOR_ASYNC_HANDLE);
    gnome_selector_signals [ADD_FILE_SIGNAL] =
	gtk_signal_new ("add_file",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   add_file),
			gnome_marshal_VOID__STRING_INT_UINT_BOXED,
			GTK_TYPE_NONE, 4,
			GTK_TYPE_STRING, GTK_TYPE_INT, GTK_TYPE_UINT,
			GNOME_TYPE_SELECTOR_ASYNC_HANDLE);
    gnome_selector_signals [ADD_DIRECTORY_SIGNAL] =
	gtk_signal_new ("add_directory",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   add_directory),
			gnome_marshal_VOID__STRING_INT_UINT_BOXED,
			GTK_TYPE_NONE, 4,
			GTK_TYPE_STRING, GTK_TYPE_INT, GTK_TYPE_UINT,
			GNOME_TYPE_SELECTOR_ASYNC_HANDLE);
    gnome_selector_signals [ADD_URI_SIGNAL] =
	gtk_signal_new ("add_uri",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   add_uri),
			gnome_marshal_VOID__STRING_INT_UINT_BOXED,
			GTK_TYPE_NONE, 4,
			GTK_TYPE_STRING, GTK_TYPE_INT, GTK_TYPE_UINT,
			GNOME_TYPE_SELECTOR_ASYNC_HANDLE);
    gnome_selector_signals [UPDATE_URI_LIST_SIGNAL] =
	gtk_signal_new ("update_uri_list",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   update_uri_list),
			gnome_marshal_VOID__UINT,
			GTK_TYPE_NONE, 1,
			GTK_TYPE_UINT);
    gnome_selector_signals [GET_SELECTION_MODE_SIGNAL] =
	gtk_signal_new ("get_selection_mode",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   get_selection_mode),
			gnome_marshal_ENUM__VOID,
			GTK_TYPE_SELECTION_MODE, 0);
    gnome_selector_signals [SET_SELECTION_MODE_SIGNAL] =
	gtk_signal_new ("set_selection_mode",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   set_selection_mode),
			gtk_marshal_VOID__ENUM,
			GTK_TYPE_NONE, 1,
			GTK_TYPE_SELECTION_MODE);
    gnome_selector_signals [GET_SELECTION_SIGNAL] =
	gtk_signal_new ("get_selection",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   get_selection),
			gnome_marshal_POINTER__VOID,
			GTK_TYPE_POINTER,
			0);
    gnome_selector_signals [SELECTION_CHANGED_SIGNAL] =
	gtk_signal_new ("selection_changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   selection_changed),
			gtk_signal_default_marshaller,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [GET_ENTRY_TEXT_SIGNAL] =
	gtk_signal_new ("get_entry_text",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   get_entry_text),
			gnome_marshal_STRING__VOID,
			GTK_TYPE_STRING,
			0);
    gnome_selector_signals [SET_ENTRY_TEXT_SIGNAL] =
	gtk_signal_new ("set_entry_text",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   set_entry_text),
			gtk_marshal_VOID__STRING,
			GTK_TYPE_NONE,
			1,
			GTK_TYPE_STRING);
    gnome_selector_signals [ACTIVATE_ENTRY_SIGNAL] =
	gtk_signal_new ("activate_entry",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   activate_entry),
			gtk_marshal_VOID__VOID,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [HISTORY_CHANGED_SIGNAL] =
	gtk_signal_new ("history_changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   history_changed),
			gtk_marshal_VOID__VOID,
			GTK_TYPE_NONE,
			0);
    gnome_selector_signals [GET_URI_LIST_SIGNAL] =
	gtk_signal_new ("get_uri_list",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   get_uri_list),
			gnome_marshal_POINTER__BOOLEAN,
			GTK_TYPE_POINTER, 1,
			GTK_TYPE_BOOL);
    gnome_selector_signals [ADD_URI_LIST_SIGNAL] =
	gtk_signal_new ("add_uri_list",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   add_uri_list),
			gnome_marshal_VOID__POINTER_INT_BOOLEAN_BOXED,
			GTK_TYPE_NONE, 4,
			GTK_TYPE_POINTER, GTK_TYPE_INT, GTK_TYPE_BOOL,
			GNOME_TYPE_SELECTOR_ASYNC_HANDLE);
    gnome_selector_signals [CHECK_FILENAME_SIGNAL] =
	gtk_signal_new ("check_filename",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   check_filename),
			gnome_marshal_VOID__STRING_BOXED,
			GTK_TYPE_NONE, 2,
			GTK_TYPE_STRING,
			GNOME_TYPE_SELECTOR_ASYNC_HANDLE);
    gnome_selector_signals [CHECK_DIRECTORY_SIGNAL] =
	gtk_signal_new ("check_directory",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeSelectorClass,
					   check_directory),
			gnome_marshal_VOID__STRING_BOXED,
			GTK_TYPE_NONE, 2,
			GTK_TYPE_STRING,
			GNOME_TYPE_SELECTOR_ASYNC_HANDLE);

    gobject_class->get_property = gnome_selector_get_property;
    gobject_class->set_property = gnome_selector_set_property;
    gobject_class->constructor = gnome_selector_constructor;

    /* Construction properties */
    g_object_class_install_property
	(gobject_class,
	 PROP_ENTRY_WIDGET,
	 g_param_spec_object ("entry_widget", NULL, NULL,
			      GTK_TYPE_WIDGET,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(gobject_class,
	 PROP_SELECTOR_WIDGET,
	 g_param_spec_object ("selector_widget", NULL, NULL,
			      GTK_TYPE_WIDGET,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(gobject_class,
	 PROP_BROWSE_DIALOG,
	 g_param_spec_object ("browse_dialog", NULL, NULL,
			      GTK_TYPE_WIDGET,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(gobject_class,
	 PROP_WANT_DEFAULT_BEHAVIOUR,
	 g_param_spec_boolean ("want_default_behaviour", NULL, NULL,
			       TRUE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(gobject_class,
	 PROP_USE_DEFAULT_ENTRY_WIDGET,
	 g_param_spec_boolean ("use_default_entry_widget", NULL, NULL,
			       TRUE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(gobject_class,
	 PROP_USE_DEFAULT_SELECTOR_WIDGET,
	 g_param_spec_boolean ("use_default_selector_widget", NULL, NULL,
			       TRUE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(gobject_class,
	 PROP_USE_DEFAULT_BROWSE_DIALOG,
	 g_param_spec_boolean ("use_default_browse_dialog", NULL, NULL,
			       TRUE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(gobject_class,
	 PROP_WANT_BROWSE_BUTTON,
	 g_param_spec_boolean ("want_browse_button", NULL, NULL,
			       TRUE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(gobject_class,
	 PROP_WANT_CLEAR_BUTTON,
	 g_param_spec_boolean ("want_clear_button", NULL, NULL,
			       TRUE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(gobject_class,
	 PROP_WANT_DEFAULT_BUTTON,
	 g_param_spec_boolean ("want_default_button", NULL, NULL,
			       TRUE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(gobject_class,
	 PROP_AUTO_SAVE_HISTORY,
	 g_param_spec_boolean ("auto_save_history", NULL, NULL,
			       FALSE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property
	(gobject_class,
	 PROP_AUTO_SAVE_ALL,
	 g_param_spec_boolean ("auto_save_all", NULL, NULL,
			       FALSE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));

    /* Normal properties */
    g_object_class_install_property
	(gobject_class,
	 PROP_DIALOG_TITLE,
	 g_param_spec_string ("dialog_title", NULL, NULL,
			      NULL,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT)));
    g_object_class_install_property
	(gobject_class,
	 PROP_HISTORY_ID,
	 g_param_spec_string ("history_id", NULL, NULL,
			      NULL,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT)));
    g_object_class_install_property
	(gobject_class,
	 PROP_SELECTION_MODE,
	 g_param_spec_enum ("selection_mode", NULL, NULL,
			    GTK_TYPE_SELECTION_MODE, GTK_SELECTION_SINGLE,
			    (G_PARAM_READABLE | G_PARAM_WRITABLE)));

    object_class->destroy = gnome_selector_destroy;
    gobject_class->finalize = gnome_selector_finalize;

    class->do_construct = do_construct_handler;

    class->browse = browse_handler;
    class->clear = clear_handler;
    class->update = update_handler;

    class->get_uri = get_uri_handler;
    class->set_uri = set_uri_handler;

    class->add_uri = add_uri_handler;
    class->add_file = add_file_handler;
    class->add_directory = add_directory_handler;

    class->get_uri_list = get_uri_list_handler;
    class->add_uri_list = add_uri_list_handler;
}

static void
gnome_selector_set_property (GObject *object, guint param_id,
			     const GValue *value, GParamSpec *pspec)
{
    GnomeSelector *selector;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (object));

    selector = GNOME_SELECTOR (object);

    switch (param_id) {
    case PROP_ENTRY_WIDGET:
	g_assert (!selector->_priv->constructed);
	selector->_priv->entry_widget = (GtkWidget *) g_value_get_object (value);
	if (selector->_priv->entry_widget)
	    gtk_widget_ref (selector->_priv->entry_widget);
	break;
    case PROP_SELECTOR_WIDGET:
	g_assert (!selector->_priv->constructed);
	selector->_priv->selector_widget = (GtkWidget *) g_value_get_object (value);
	if (selector->_priv->selector_widget)
	    gtk_widget_ref (selector->_priv->selector_widget);
	break;
    case PROP_BROWSE_DIALOG:
	g_assert (!selector->_priv->constructed);
	selector->_priv->browse_dialog = (GtkWidget *) g_value_get_object (value);
	if (selector->_priv->browse_dialog)
	    gtk_widget_ref (selector->_priv->browse_dialog);
	break;
    case PROP_WANT_DEFAULT_BEHAVIOUR:
	g_assert (!selector->_priv->constructed);
	selector->_priv->want_default_behaviour = g_value_get_boolean (value);
	break;
    case PROP_USE_DEFAULT_ENTRY_WIDGET:
	g_assert (!selector->_priv->constructed);
	selector->_priv->use_default_entry_widget = g_value_get_boolean (value);
	break;
    case PROP_USE_DEFAULT_SELECTOR_WIDGET:
	g_assert (!selector->_priv->constructed);
	selector->_priv->use_default_selector_widget = g_value_get_boolean (value);
	break;
    case PROP_USE_DEFAULT_BROWSE_DIALOG:
	g_assert (!selector->_priv->constructed);
	selector->_priv->use_default_browse_dialog = g_value_get_boolean (value);
	break;
    case PROP_WANT_BROWSE_BUTTON:
	g_assert (!selector->_priv->constructed);
	selector->_priv->want_browse_button = g_value_get_boolean (value);
	break;
    case PROP_WANT_CLEAR_BUTTON:
	g_assert (!selector->_priv->constructed);
	selector->_priv->want_clear_button = g_value_get_boolean (value);
	break;
    case PROP_WANT_DEFAULT_BUTTON:
	g_assert (!selector->_priv->constructed);
	selector->_priv->want_default_button = g_value_get_boolean (value);
	break;
    case PROP_AUTO_SAVE_HISTORY:
	g_assert (!selector->_priv->constructed);
	selector->_priv->auto_save_history = g_value_get_boolean (value);
	break;
    case PROP_AUTO_SAVE_ALL:
	g_assert (!selector->_priv->constructed);
	selector->_priv->auto_save_all = g_value_get_boolean (value);
	break;
   case PROP_DIALOG_TITLE:
	gnome_selector_set_dialog_title (selector, g_value_get_string (value));
	break;
    case PROP_HISTORY_ID:
	gnome_selector_set_history_id (selector, g_value_get_string (value));
	break;
    case PROP_SELECTION_MODE:
	gnome_selector_set_selection_mode (selector, g_value_get_enum (value));
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
gnome_selector_get_property (GObject *object, guint param_id, GValue *value,
			     GParamSpec *pspec)
{
    GnomeSelector *selector;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (object));

    selector = GNOME_SELECTOR (object);

    switch (param_id) {
    case PROP_ENTRY_WIDGET:
	g_value_set_object (value, (GObject *) selector->_priv->entry_widget);
	break;
    case PROP_SELECTOR_WIDGET:
	g_value_set_object (value, (GObject *) selector->_priv->selector_widget);
	break;
    case PROP_BROWSE_DIALOG:
	g_value_set_object (value, (GObject *) selector->_priv->browse_dialog);
	break;
    case PROP_WANT_DEFAULT_BEHAVIOUR:
	g_value_set_boolean (value, selector->_priv->want_default_behaviour);
	break;
    case PROP_USE_DEFAULT_ENTRY_WIDGET:
	g_value_set_boolean (value, selector->_priv->use_default_entry_widget);
	break;
    case PROP_USE_DEFAULT_SELECTOR_WIDGET:
	g_value_set_boolean (value, selector->_priv->use_default_selector_widget);
	break;
    case PROP_USE_DEFAULT_BROWSE_DIALOG:
	g_value_set_boolean (value, selector->_priv->use_default_browse_dialog);
	break;
    case PROP_WANT_BROWSE_BUTTON:
	g_value_set_boolean (value, selector->_priv->want_browse_button);
	break;
    case PROP_WANT_CLEAR_BUTTON:
	g_value_set_boolean (value, selector->_priv->want_clear_button);
	break;
    case PROP_WANT_DEFAULT_BUTTON:
	g_value_set_boolean (value, selector->_priv->want_default_button);
	break;
    case PROP_AUTO_SAVE_HISTORY:
	g_value_set_boolean (value, selector->_priv->auto_save_history);
	break;
    case PROP_AUTO_SAVE_ALL:
	g_value_set_boolean (value, selector->_priv->auto_save_all);
	break;
    case PROP_DIALOG_TITLE:
	g_value_set_string (value, gnome_selector_get_dialog_title (selector));
	break;
    case PROP_HISTORY_ID:
	g_value_set_string (value, gnome_selector_get_history_id (selector));
	break;
    case PROP_SELECTION_MODE:
	g_value_set_enum (value, gnome_selector_get_selection_mode (selector));
	break;
    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
gnome_selector_init (GnomeSelector *selector)
{
    guint i;

    selector->_priv = g_new0 (GnomeSelectorPrivate, 1);

    selector->_priv->changed = FALSE;

    selector->_priv->selector_widget = NULL;
    selector->_priv->browse_dialog = NULL;

    selector->_priv->lists = g_ptr_array_sized_new (GNOME_SELECTOR_LIST_MAX);
    for (i = 0; i < GNOME_SELECTOR_LIST_MAX; i++) {
	GnomeSelectorListData *data;

	data = g_new0 (GnomeSelectorListData, 1);
	g_ptr_array_index (selector->_priv->lists, i) = data;
    }
}

/*
 * Default signal handlers.
 */

static void
update_handler (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

#if 0
    if (selector->_priv->need_rebuild)
	gnome_selector_update_uri_list (selector);
#endif

    if (selector->_priv->history_changed)
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [HISTORY_CHANGED_SIGNAL]);

    selector->_priv->history_changed = FALSE;
    selector->_priv->dirty = FALSE;
}

static void
browse_handler (GnomeSelector *selector)
{
    GnomeSelectorPrivate *priv;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    priv = selector->_priv;

    /*if it already exists make sure it's shown and raised*/
    if (priv->browse_dialog) {
	gtk_widget_show (priv->browse_dialog);
	if (priv->browse_dialog->window)
	    gdk_window_raise (priv->browse_dialog->window);
    }
}

static void
free_entry_func (gpointer data, gpointer user_data)
{
    g_free (data);
}

static void
clear_handler (GnomeSelector *selector, guint list_id)
{
    GSList **list_ptr;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    list_ptr = _gnome_selector_get_list_by_id (selector, list_id);
    g_assert (list_ptr != NULL);

    g_slist_foreach (*list_ptr, free_entry_func, selector);
    g_slist_free (*list_ptr);
    *list_ptr = NULL;
}

static gchar *
get_uri_handler (GnomeSelector *selector)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);

    return gnome_selector_get_entry_text (selector);
}

static void
set_uri_handler (GnomeSelector *selector, const gchar *filename,
		 GnomeSelectorAsyncHandle *async_handle)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gnome_selector_set_entry_text (selector, filename);
    /* gnome_selector_activate_entry (selector); */

    if (async_handle != NULL)
	_gnome_selector_async_handle_completed (async_handle, TRUE);
}

static void
add_uri_handler (GnomeSelector *selector, const gchar *uri, gint position,
		 guint list_id, GnomeSelectorAsyncHandle *async_handle)
{
    GSList **list_ptr;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (uri != NULL);
    g_return_if_fail (position >= -1);

    list_ptr = _gnome_selector_get_list_by_id (selector, list_id);
    g_assert (list_ptr != NULL);

    if (position == -1)
	*list_ptr = g_slist_append (*list_ptr, g_strdup (uri));
    else
	*list_ptr = g_slist_insert (*list_ptr, g_strdup (uri), position);

    if (async_handle != NULL)
	_gnome_selector_async_handle_completed (async_handle, TRUE);
}

static void
add_file_handler (GnomeSelector *selector, const gchar *uri, gint position,
		 guint list_id, GnomeSelectorAsyncHandle *async_handle)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (async_handle != NULL)
	_gnome_selector_async_handle_completed (async_handle, TRUE);
}

static void
add_directory_handler (GnomeSelector *selector, const gchar *uri, gint position,
		       guint list_id, GnomeSelectorAsyncHandle *async_handle)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (async_handle != NULL)
	_gnome_selector_async_handle_completed (async_handle, TRUE);
}

static void
add_uri_list_handler (GnomeSelector *selector, GSList *list, gint position,
		      guint list_id, GnomeSelectorAsyncHandle *async_handle)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (async_handle != NULL)
	_gnome_selector_async_handle_completed (async_handle, TRUE);
}

static GSList *
get_uri_list_handler (GnomeSelector *selector, guint list_id)
{
    GSList **list_ptr;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    list_ptr = _gnome_selector_get_list_by_id (selector, list_id);
    g_assert (list_ptr != NULL);

    return _gnome_selector_deep_copy_slist (*list_ptr);
}

/*
 * Misc callbacks.
 */

static void
browse_clicked_cb (GtkWidget *widget, gpointer data)
{
    gtk_signal_emit (GTK_OBJECT (data),
		     gnome_selector_signals [BROWSE_SIGNAL]);
}

static void
default_clicked_cb (GtkWidget *widget, gpointer data)
{
    GnomeSelector *selector;
    GSList *list;

    g_return_if_fail (data != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (data));

    selector = GNOME_SELECTOR (data);

    list = gnome_selector_get_uri_list (selector, GNOME_SELECTOR_LIST_DEFAULT);
    gnome_selector_clear (selector, GNOME_SELECTOR_LIST_PRIMARY);
    gnome_selector_add_uri_list (selector, NULL, list, 0,
				 GNOME_SELECTOR_LIST_PRIMARY, NULL, NULL);
}

static void
clear_clicked_cb (GtkWidget *widget, gpointer data)
{
    gnome_selector_clear (GNOME_SELECTOR (data), FALSE);
}


static GObject*
gnome_selector_constructor (GType                  type,
			    guint                  n_construct_properties,
			    GObjectConstructParam *construct_properties)
{
    GObject *object = G_OBJECT_CLASS (parent_class)->constructor (type,
								  n_construct_properties,
								  construct_properties);

    return object;
}

static void
do_construct_handler (GnomeSelector *selector)
{
    GnomeSelectorPrivate *priv;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    priv = selector->_priv;

    g_message (G_STRLOC);

    priv->db = gnome_program_get_config_database (gnome_program_get ());

    priv->box = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);

    priv->hbox = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);

    if (priv->entry_widget) {
	gtk_widget_show_all (priv->entry_widget);

	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->entry_widget,
			    TRUE, TRUE, 0);
    }

    gtk_box_pack_start (GTK_BOX (priv->box), priv->hbox,
			TRUE, FALSE, GNOME_PAD_SMALL);

    if (priv->want_browse_button) {
	priv->browse_button = gtk_button_new_with_label (_("Browse..."));

	gtk_signal_connect (GTK_OBJECT (priv->browse_button),
			    "clicked", GTK_SIGNAL_FUNC (browse_clicked_cb),
			    selector);

	gtk_box_pack_start (GTK_BOX (priv->hbox),
			    priv->browse_button, FALSE, FALSE, 0);
    }

    if (priv->want_default_button) {
	priv->default_button = gtk_button_new_with_label (_("Default..."));

	gtk_signal_connect (GTK_OBJECT (priv->default_button),
			    "clicked", GTK_SIGNAL_FUNC (default_clicked_cb),
			    selector);

	gtk_box_pack_start (GTK_BOX (priv->hbox),
			    priv->default_button, FALSE, FALSE, 0);
    }

    if (priv->want_clear_button) {
	priv->clear_button = gtk_button_new_with_label (_("Clear..."));

	gtk_signal_connect (GTK_OBJECT (priv->clear_button),
			    "clicked", GTK_SIGNAL_FUNC (clear_clicked_cb),
			    selector);

	gtk_box_pack_start (GTK_BOX (priv->hbox),
			    priv->clear_button, FALSE, FALSE, 0);
    }

    if (priv->selector_widget) {
	gtk_box_pack_start (GTK_BOX (priv->box),
			    priv->selector_widget, TRUE, TRUE,
			    GNOME_PAD_SMALL);
    }

    gtk_widget_show_all (priv->box);

    gtk_box_pack_start (GTK_BOX (selector), priv->box,
			TRUE, TRUE, GNOME_PAD_SMALL);

    if (priv->auto_save_history)
	gnome_selector_load_history (selector);

    if (priv->auto_save_all)
	gnome_selector_load_all (selector);

    selector->_priv->constructed = TRUE;
}

void
gnome_selector_do_construct (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    GNOME_SELECTOR_CLASS (G_OBJECT_GET_CLASS (selector))->do_construct (selector);
}

static void
gnome_selector_destroy (GtkObject *object)
{
    GnomeSelector *selector;

    /* remember, destroy can be run multiple times! */

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (object));

    selector = GNOME_SELECTOR (object);

#if 0
    if (selector->_priv->client) {
	if (selector->_priv->flags & GNOME_SELECTOR_AUTO_SAVE_ALL)
	    gnome_selector_save_all (selector);

	if (selector->_priv->gconf_history_dir)
	    gconf_client_remove_dir
		(selector->_priv->client,
		 selector->_priv->gconf_history_dir,
		 NULL);

	g_object_unref (G_OBJECT (selector->_priv->client));
	selector->_priv->client = NULL;
    }
#endif

    if (selector->_priv->selector_widget) {
	gtk_widget_unref (selector->_priv->selector_widget);
	selector->_priv->selector_widget = NULL;
    }

    if (selector->_priv->browse_dialog) {
	gtk_widget_unref (selector->_priv->browse_dialog);
	selector->_priv->browse_dialog = NULL;
    }

    if (selector->_priv->entry_widget) {
	gtk_widget_unref (selector->_priv->entry_widget);
	selector->_priv->entry_widget = NULL;
    }

    GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_selector_finalize (GObject *object)
{
    GnomeSelector *selector;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (object));

    selector = GNOME_SELECTOR (object);

    if (selector->_priv) {
	guint i;

	for (i = 0; i < selector->_priv->lists->len; i++) {
	    GnomeSelectorListData *data = g_ptr_array_index
		(selector->_priv->lists, i);

	    g_slist_foreach (data->list, free_entry_func, selector);
	}

	g_ptr_array_free (selector->_priv->lists, TRUE);

	g_free (selector->_priv->dialog_title);
	g_free (selector->_priv->history_id);
	g_free (selector->_priv->gconf_history_dir);
	g_free (selector->_priv->gconf_history_key);
	g_free (selector->_priv->gconf_uri_list_key);
    }

    g_free (selector->_priv);
    selector->_priv = NULL;

    GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}


/**
 * gnome_selector_get_dialog_title
 * @selector: Pointer to GnomeSelector object.
 *
 * Description: Returns the titel of the popup dialog.
 *
 * Returns: Titel of the popup dialog.
 */
const gchar *
gnome_selector_get_dialog_title (GnomeSelector *selector)
{
    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    return selector->_priv->dialog_title;
}


/**
 * gnome_selector_set_dialog_title
 * @selector: Pointer to GnomeSelector object.
 * @dialog_title: New title for the popup dialog.
 *
 * Description: Sets the titel of the popup dialog.
 *
 * This is only used when the widget uses a popup dialog for
 * the actual selector.
 *
 * Returns:
 */
void
gnome_selector_set_dialog_title (GnomeSelector *selector,
				 const gchar *dialog_title)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (selector->_priv->dialog_title) {
	g_free (selector->_priv->dialog_title);
	selector->_priv->dialog_title = NULL;
    }

    /* this is NULL safe. */
    selector->_priv->dialog_title = g_strdup (dialog_title);

    g_object_notify (G_OBJECT (selector), "dialog_title");
}

const gchar *
gnome_selector_get_history_id (GnomeSelector *selector)
{
    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    return selector->_priv->history_id;
}

void
gnome_selector_set_history_id (GnomeSelector *selector,
			       const gchar *history_id)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (selector->_priv->history_id) {
	g_free (selector->_priv->history_id);
	selector->_priv->history_id = NULL;
    }

    /* this is NULL safe. */
    selector->_priv->history_id = g_strdup (history_id);

    g_object_notify (G_OBJECT (selector), "history_id");
}

GSList *
gnome_selector_get_uri_list (GnomeSelector *selector, guint list_id)
{
    GSList *retval = NULL;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [GET_URI_LIST_SIGNAL],
		     list_id, &retval);

    return retval;
}

void
gnome_selector_add_uri_list (GnomeSelector *selector,
			     GnomeSelectorAsyncHandle **async_handle_return,
			     GSList *uri_list, gint position, guint list_id,
			     GnomeSelectorAsyncFunc async_func,
			     gpointer user_data)
{
    GnomeSelectorAsyncHandle *async_handle;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    async_handle = _gnome_selector_async_handle_get
	(selector, GNOME_SELECTOR_ASYNC_TYPE_ADD_URI_LIST,
	 NULL, async_func, user_data);
    if (async_handle_return != NULL)
	*async_handle_return = async_handle;

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [ADD_URI_LIST_SIGNAL],
		     uri_list, position, list_id, async_handle);
}

GtkSelectionMode
gnome_selector_get_selection_mode (GnomeSelector *selector)
{
    GtkSelectionMode mode = 0;

    g_return_val_if_fail (selector != NULL, 0);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), 0);

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [GET_SELECTION_MODE_SIGNAL],
		     &mode);

    return mode;
}

void
gnome_selector_set_selection_mode (GnomeSelector *selector,
				   GtkSelectionMode mode)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [SET_SELECTION_MODE_SIGNAL],
		     (gint) mode);
}


GSList *
gnome_selector_get_selection (GnomeSelector *selector)
{
    GSList *retval = NULL;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [GET_SELECTION_SIGNAL],
		     &retval);

    return retval;
}


/**
 * gnome_selector_update
 * @selector: Pointer to GnomeSelector object.
 *
 * Description: Updates the file list.
 *
 * Returns:
 */
void
gnome_selector_update (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [UPDATE_SIGNAL]);
}


/**
 * gnome_selector_clear
 * @selector: Pointer to GnomeSelector object.
 *
 * Description: Removes all entries from the selector.
 *
 * Returns:
 */
void
gnome_selector_clear (GnomeSelector *selector, guint list_id)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gnome_selector_freeze (selector);
    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [CLEAR_SIGNAL],
		     list_id);
    gnome_selector_thaw (selector);

    selector->_priv->need_rebuild = TRUE;

    if (selector->_priv->frozen)
	selector->_priv->dirty = TRUE;
    else
	gnome_selector_update (selector);
}

/**
 * gnome_selector_get_uri
 * @selector: Pointer to GnomeSelector object.
 *
 * Description:
 *
 * Returns:
 */
gchar *
gnome_selector_get_uri (GnomeSelector *selector)
{
    gchar *retval = NULL;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [GET_URI_SIGNAL],
		     &retval);

    return retval;
}


/**
 * gnome_selector_set_uri
 * @selector: Pointer to GnomeSelector object.
 * @filename: Filename to set.
 *
 * Description: Sets @filename as the currently selected filename.
 *
 * This calls gnome_selector_check_filename() to make sure @filename
 * is a valid file for this kind of selector.
 *
 * Returns: #TRUE if @filename was ok or #FALSE if not.
 */
void
gnome_selector_set_uri (GnomeSelector *selector,
			GnomeSelectorAsyncHandle **async_handle_return,
			const gchar *filename,
			GnomeSelectorAsyncFunc async_func,
			gpointer user_data)
{
    GnomeSelectorAsyncHandle *async_handle;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (filename != NULL);

    async_handle = _gnome_selector_async_handle_get
	(selector, GNOME_SELECTOR_ASYNC_TYPE_SET_URI,
	 filename, async_func, user_data);
    if (async_handle_return != NULL)
	*async_handle_return = async_handle;

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [SET_URI_SIGNAL],
		     filename, async_handle);
}


/**
 * gnome_selector_update_uri_list
 * @selector: Pointer to GnomeSelector object.
 *
 * Description: Updates the internal file list.
 *
 * Returns:
 */
void
gnome_selector_update_uri_list (GnomeSelector *selector, guint list_id)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [UPDATE_URI_LIST_SIGNAL],
		     list_id);
}


/**
 * gnome_selector_freeze
 * @selector: Pointer to GnomeSelector object.
 *
 * Description:
 *
 * Returns:
 */
void
gnome_selector_freeze (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    selector->_priv->frozen++;

    /* Note that we only emit the signal once. */
    if (selector->_priv->frozen == 1)
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [FREEZE_SIGNAL]);
}

/**
 * gnome_selector_is_frozen
 * @selector: Pointer to GnomeSelector object.
 *
 * Description:
 *
 * Returns:
 */
gboolean
gnome_selector_is_frozen (GnomeSelector *selector)
{
    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), FALSE);

    return selector->_priv->frozen ? TRUE : FALSE;
}


/**
 * gnome_selector_thaw
 * @selector: Pointer to GnomeSelector object.
 *
 * Description:
 *
 * Returns:
 */
void
gnome_selector_thaw (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    g_return_if_fail (selector->_priv->frozen > 0);

    selector->_priv->frozen--;

    /* Note that we only emit the signal once. */
    if (selector->_priv->frozen == 0) {
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [THAW_SIGNAL]);

	if (selector->_priv->dirty) {
	    selector->_priv->dirty = FALSE;
	    gnome_selector_update (selector);
	}
    }
}


/**
 * gnome_selector_get_entry_text
 * @selector: Pointer to GnomeSelector object.
 *
 * Description:
 *
 * Returns:
 */
gchar *
gnome_selector_get_entry_text (GnomeSelector *selector)
{
    gchar *retval = NULL;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [GET_ENTRY_TEXT_SIGNAL],
		     &retval);

    return retval;
}


/**
 * gnome_selector_set_entry_text
 * @selector: Pointer to GnomeSelector object.
 * @text: The text which should be set.
 *
 * Description:
 *
 * Returns:
 */
void
gnome_selector_set_entry_text (GnomeSelector *selector, const gchar *text)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [SET_ENTRY_TEXT_SIGNAL],
		     text);
}

void
gnome_selector_activate_entry (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [ACTIVATE_ENTRY_SIGNAL]);
}

guint
gnome_selector_get_history_length (GnomeSelector *selector)
{
    g_return_val_if_fail (selector != NULL, 0);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), 0);

    return selector->_priv->max_history_length;
}

static void
free_history_item (GnomeSelectorHistoryItem *item, gpointer data)
{
    if (item != NULL)
	g_free (item->text);
    g_free (item);
}

static gint
compare_history_items (gconstpointer a, gconstpointer b)
{
    GnomeSelectorHistoryItem *ia, *ib;

    ia = (GnomeSelectorHistoryItem *) a;
    ib = (GnomeSelectorHistoryItem *) b;

    return strcmp (ia->text, ib->text);
}

static void
set_history_changed (GnomeSelector *selector)
{
    if (selector->_priv->frozen)
	selector->_priv->history_changed = TRUE;
    else
	gtk_signal_emit (GTK_OBJECT (selector),
			 gnome_selector_signals [HISTORY_CHANGED_SIGNAL]);
}

void
gnome_selector_set_history_length (GnomeSelector *selector,
				   guint history_length)
{
    guint old_length;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    /* truncate history if necessary. */
    old_length = g_slist_length (selector->_priv->history);

    if (old_length < history_length) {
	guint prev_pos = (history_length > 0) ? history_length-1 : 0;
	GSList *c = g_slist_nth (selector->_priv->history, prev_pos);

	if (c) {
	    g_slist_foreach (c->next, (GFunc) free_history_item,
			     selector);
	    g_slist_free (c->next);
	    c->next = NULL;

	    set_history_changed (selector);
	}
    }

    selector->_priv->max_history_length = history_length;
}

static void
_gnome_selector_add_history (GnomeSelector *selector, gboolean save,
			     const gchar *text, gboolean append)
{
    GnomeSelectorHistoryItem *item;
    GSList *node;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (text != NULL);

    item = g_new0 (GnomeSelectorHistoryItem, 1);
    item->text = g_strdup (text);
    item->save = save;

    /* if it's already in the history */
    node = g_slist_find_custom (selector->_priv->history, item,
				compare_history_items);
    if (node) {
	free_history_item (node->data, selector);
	selector->_priv->history = g_slist_remove
	    (selector->_priv->history, node->data);
    }

    if (append)
	selector->_priv->history = g_slist_append
	    (selector->_priv->history, item);
    else
	selector->_priv->history = g_slist_prepend
	    (selector->_priv->history, item);

    set_history_changed (selector);
}

void
gnome_selector_prepend_history (GnomeSelector *selector, gboolean save,
				const gchar *text)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (text != NULL);

    _gnome_selector_add_history (selector, save, text, FALSE);
}

void
gnome_selector_append_history (GnomeSelector *selector, gboolean save,
			       const gchar *text)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (text != NULL);

    _gnome_selector_add_history (selector, save, text, TRUE);
}

GSList *
gnome_selector_get_history (GnomeSelector *selector)
{
    GSList *retval = NULL, *c;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    for (c = selector->_priv->history; c; c = c->next) {
	GnomeSelectorHistoryItem *item = c->data;

	retval = g_slist_prepend (retval, g_strdup (item->text));
    }

    return g_slist_reverse (retval);
}

void
gnome_selector_set_history (GnomeSelector *selector, GSList *history)
{
    GSList *c;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    gnome_selector_clear_history (selector);

    for (c = history; c; c = c->next)
	gnome_selector_prepend_history (selector, TRUE, c->data);

    selector->_priv->history = g_slist_reverse (selector->_priv->history);

    set_history_changed (selector);
}

static GSList * G_GNUC_UNUSED
_gnome_selector_history_to_list (GnomeSelector *selector, gboolean only_save)
{
    GSList *thelist = NULL, *c;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    for (c = selector->_priv->history; c; c = c->next) {
	GnomeSelectorHistoryItem *item = c->data;

	if (only_save && !item->save)
	    continue;

	thelist = g_slist_prepend (thelist, item->text);
    }

    thelist = g_slist_reverse (thelist);
    return thelist;
}

void
gnome_selector_load_history (GnomeSelector *selector)
{
#if 0
    GSList *thelist;
#endif

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (!selector->_priv->gconf_history_key)
	return;

#if 0
    thelist = gconf_client_get_list (selector->_priv->client,
				     selector->_priv->gconf_history_key,
				     GCONF_VALUE_STRING, NULL);

    gnome_selector_set_history (selector, thelist);

    g_slist_foreach (thelist, (GFunc) g_free, NULL);
    g_slist_free (thelist);
#endif
}

void
gnome_selector_save_history (GnomeSelector *selector)
{
#if 0
    GSList *thelist;
    gboolean result;
#endif

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (!selector->_priv->gconf_history_key)
	return;

#if 0
    thelist = _gnome_selector_history_to_list (selector, TRUE);

    result = gconf_client_set_list (selector->_priv->client,
				    selector->_priv->gconf_history_key,
				    GCONF_VALUE_STRING, thelist, NULL);

    g_slist_free (thelist);
#endif
}

void
gnome_selector_clear_history (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    g_slist_foreach (selector->_priv->history,
		     (GFunc) free_history_item, selector);
    g_slist_free (selector->_priv->history);
    selector->_priv->history = NULL;

    set_history_changed (selector);
}

guint
_gnome_selector_register_list (GnomeSelector *selector, GQuark list_quark)
{
    GnomeSelectorListData *data;
    guint i;

    g_return_val_if_fail (selector != NULL, 0);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), 0);

    data = g_new0 (GnomeSelectorListData, 1);
    data->quark = list_quark;

    g_ptr_array_add (selector->_priv->lists, data);

    for (i = 0; i < selector->_priv->lists->len; i++)
	if (g_ptr_array_index (selector->_priv->lists, i) == data)
	    return i;

    g_assert_not_reached ();
    return 0;
}

void
_gnome_selector_unregister_list (GnomeSelector *selector, guint list_id)
{
    GnomeSelectorListData *data;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    data = g_ptr_array_remove_index (selector->_priv->lists, list_id);
    if (data == NULL)
	return;

    _gnome_selector_deep_free_slist (data->list);
    g_free (data);
}

GSList **
_gnome_selector_get_list_by_id (GnomeSelector *selector, guint list_id)
{
    GnomeSelectorListData *data;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    data = g_ptr_array_index (selector->_priv->lists, list_id);
    if (data == NULL)
	return NULL;
    else
	return &data->list;
}

GSList *
_gnome_selector_deep_copy_slist (GSList *thelist)
{
    GSList *retval = NULL, *c;

    for (c = thelist; c; c = c->next)
	retval = g_slist_prepend (retval, g_strdup (c->data));

    retval = g_slist_reverse (retval);
    return retval;
}

void
_gnome_selector_deep_free_slist (GSList *thelist)
{
    g_slist_foreach (thelist, (GFunc) g_free, NULL);
    g_slist_free (thelist);
}

#if 0
static void
gnome_selector_save_all (GnomeSelector *selector)
{
    GSList *uri_list;
    gboolean result;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (!selector->_priv->gconf_uri_list_key)
	return;

    uri_list = gnome_selector_get_uri_list (selector, FALSE);

    result = gconf_client_set_list (selector->_priv->client,
				    selector->_priv->gconf_uri_list_key,
				    GCONF_VALUE_STRING, uri_list,
				    NULL);

    _gnome_selector_deep_free_slist (uri_list);
}
#endif

static void
gnome_selector_load_all (GnomeSelector *selector)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));

    if (!selector->_priv->gconf_uri_list_key)
	return;

#if 0
    selector->_priv->uri_list = gconf_client_get_list
	(selector->_priv->client,
	 selector->_priv->gconf_uri_list_key,
	 GCONF_VALUE_STRING, NULL);

    selector->_priv->need_rebuild = TRUE;

    if (selector->_priv->frozen)
	selector->_priv->dirty = TRUE;
    else
	gnome_selector_update (selector);
#endif
}

GnomeSelectorAsyncHandle *
_gnome_selector_async_handle_get (GnomeSelector *selector,
				  GnomeSelectorAsyncType async_type,
				  const char *uri,
				  GnomeSelectorAsyncFunc async_func,
				  gpointer async_data)
{
    GnomeSelectorAsyncHandle *async_handle;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_SELECTOR (selector), NULL);

    async_handle = g_new0 (GnomeSelectorAsyncHandle, 1);
    async_handle->refcount = 1;
    async_handle->selector = selector;
    async_handle->async_type = async_type;
    async_handle->async_func = async_func;
    async_handle->user_data = async_data;
    async_handle->uri = g_strdup (uri);

    gtk_object_ref (GTK_OBJECT (async_handle->selector));

    selector->_priv->async_ops = g_list_prepend (selector->_priv->async_ops,
						 async_handle);

    return async_handle;
}

void
_gnome_selector_async_handle_add (GnomeSelectorAsyncHandle *async_handle,
				  gpointer async_data,
				  GDestroyNotify async_data_destroy)
{
    GnomeSelector *selector;
    GnomeSelectorAsyncData *data;

    g_return_if_fail (async_handle != NULL);
    g_assert (GNOME_IS_SELECTOR (async_handle->selector));
    g_assert (!async_handle->destroyed && !async_handle->completed);

    selector = async_handle->selector;

    data = g_new0 (GnomeSelectorAsyncData, 1);
    data->async_data = async_data;
    data->async_data_destroy = async_data_destroy;

    async_handle->async_data_list = g_slist_prepend
	(async_handle->async_data_list, data);
}

void
_gnome_selector_async_handle_remove (GnomeSelectorAsyncHandle *async_handle,
				     gpointer async_data)
{
    GSList *c;

    for (c = async_handle->async_data_list; c; c = c->next) {
	GnomeSelectorAsyncData *data = c->data;

	if (data->async_data == async_data) {
	    async_handle->async_data_list = g_slist_remove
		(async_handle->async_data_list, data);

	    if (data->async_data_destroy)
		data->async_data_destroy (async_data);

	    g_free (data);

#ifdef DEBUG_ASYNC_HANDLE
	    g_message (G_STRLOC ": %p - %d - %p", async_handle,
		       async_handle->completed, async_handle->async_data_list);
#endif

	    if (async_handle->completed &&
		async_handle->async_data_list == NULL)
		_gnome_selector_async_handle_destroy (async_handle);

	    return;
	}
    }

    g_assert_not_reached ();
}

void
_gnome_selector_async_handle_completed (GnomeSelectorAsyncHandle *async_handle,
					gboolean success)
{
    GnomeSelector *selector;

    g_return_if_fail (async_handle != NULL);
    g_assert (GNOME_IS_SELECTOR (async_handle->selector));
    g_assert (!async_handle->destroyed);

    selector = async_handle->selector;

#ifdef DEBUG_ASYNC_HANDLE
    g_message (G_STRLOC ": %p - %d - %d - %p", async_handle,
	       async_handle->completed, success,
	       async_handle->async_data_list);
#endif

    if (!async_handle->completed)
	async_handle->success = success;

    async_handle->completed = TRUE;

    if (async_handle->async_data_list == NULL)
	_gnome_selector_async_handle_destroy (async_handle);
}

void
_gnome_selector_async_handle_destroy (GnomeSelectorAsyncHandle *async_handle)
{
    GnomeSelector *selector;
    GSList *c;

    g_return_if_fail (async_handle != NULL);
    g_assert (GNOME_IS_SELECTOR (async_handle->selector));

    selector = async_handle->selector;

#ifdef DEBUG_ASYNC_HANDLE
    g_message (G_STRLOC ": %p - %d", async_handle, async_handle->refcount);
#endif

    for (c = async_handle->async_data_list; c; c = c->next) {
	GnomeSelectorAsyncData *async_data = c->data;

	if (async_data->async_data_destroy)
	    async_data->async_data_destroy (async_data->async_data);
    }

    g_slist_foreach (async_handle->async_data_list, (GFunc) g_free, NULL);
    g_slist_free (async_handle->async_data_list);
    async_handle->async_data_list = NULL;

    if (async_handle->async_func)
	async_handle->async_func (selector, async_handle,
				  async_handle->async_type, async_handle->uri,
				  async_handle->error, async_handle->success,
				  async_handle->user_data);

    selector->_priv->async_ops = g_list_remove (selector->_priv->async_ops,
						async_handle);

    async_handle->completed = TRUE;
    async_handle->destroyed = TRUE;

    gnome_selector_async_handle_unref (async_handle);
}

void
_gnome_selector_async_handle_set_error (GnomeSelectorAsyncHandle *async_handle, GError *error)
{
    g_return_if_fail (async_handle != NULL);
    g_assert (GNOME_IS_SELECTOR (async_handle->selector));

    /* Don't override. */
    if (async_handle->error != NULL)
	return;

    async_handle->error = g_error_copy (error);
    async_handle->success = FALSE;
}

void
gnome_selector_async_handle_ref (GnomeSelectorAsyncHandle *async_handle)
{
    g_return_if_fail (async_handle != NULL);
    g_assert (GNOME_IS_SELECTOR (async_handle->selector));

    async_handle->refcount++;
}

void
gnome_selector_async_handle_unref (GnomeSelectorAsyncHandle *async_handle)
{
    GnomeSelector *selector;

    g_return_if_fail (async_handle != NULL);
    g_assert (GNOME_IS_SELECTOR (async_handle->selector));

    selector = async_handle->selector;

    async_handle->refcount--;
    if (async_handle->refcount <= 0) {
	if (async_handle->error)
	    g_error_free (async_handle->error);

	gtk_object_unref (GTK_OBJECT (async_handle->selector));
	g_assert (async_handle->destroyed);

	g_free (async_handle->uri);
	g_free (async_handle);
    }
}

void
gnome_selector_check_filename (GnomeSelector *selector,
			       GnomeSelectorAsyncHandle **async_handle_return,
			       const gchar *filename,
			       GnomeSelectorAsyncFunc async_func,
			       gpointer user_data)
{
    GnomeSelectorAsyncHandle *async_handle;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (filename != NULL);
    g_return_if_fail (async_func != NULL);

    async_handle = _gnome_selector_async_handle_get
	(selector, GNOME_SELECTOR_ASYNC_TYPE_CHECK_FILENAME,
	 filename, async_func, user_data);
    if (async_handle_return != NULL)
	*async_handle_return = async_handle;

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [CHECK_FILENAME_SIGNAL],
		     filename, async_handle);
}

void
gnome_selector_check_directory (GnomeSelector *selector,
				GnomeSelectorAsyncHandle **async_handle_return,
				const gchar *directory,
				GnomeSelectorAsyncFunc async_func,
				gpointer user_data)
{
    GnomeSelectorAsyncHandle *async_handle;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (directory != NULL);
    g_return_if_fail (async_func != NULL);

    async_handle = _gnome_selector_async_handle_get
	(selector, GNOME_SELECTOR_ASYNC_TYPE_CHECK_DIRECTORY,
	 directory, async_func, user_data);
    if (async_handle_return != NULL)
	*async_handle_return = async_handle;

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [CHECK_DIRECTORY_SIGNAL],
		     directory, async_handle);

}

void
gnome_selector_add_file (GnomeSelector *selector,
			 GnomeSelectorAsyncHandle **async_handle_return,
			 const gchar *filename, gint position, guint list_id,
			 GnomeSelectorAsyncFunc async_func,
			 gpointer user_data)
{
    GnomeSelectorAsyncHandle *async_handle;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (filename != NULL);

    async_handle = _gnome_selector_async_handle_get
	(selector, GNOME_SELECTOR_ASYNC_TYPE_ADD_FILE,
	 filename, async_func, user_data);
    if (async_handle_return != NULL)
	*async_handle_return = async_handle;

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [ADD_FILE_SIGNAL],
		     filename, position, list_id, async_handle);
}

void
gnome_selector_add_directory (GnomeSelector *selector,
			      GnomeSelectorAsyncHandle **async_handle_return,
			      const gchar *directory, gint position,
			      guint list_id, GnomeSelectorAsyncFunc async_func,
			      gpointer user_data)
{
    GnomeSelectorAsyncHandle *async_handle;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (directory != NULL);

    async_handle = _gnome_selector_async_handle_get
	(selector, GNOME_SELECTOR_ASYNC_TYPE_ADD_DIRECTORY,
	 directory, async_func, user_data);
    if (async_handle_return != NULL)
	*async_handle_return = async_handle;

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [ADD_DIRECTORY_SIGNAL],
		     directory, position, list_id, async_handle);
}

void
gnome_selector_add_uri (GnomeSelector *selector,
			GnomeSelectorAsyncHandle **async_handle_return,
			const gchar *uri, gint position, guint list_id,
			GnomeSelectorAsyncFunc async_func,
			gpointer user_data)
{
    GnomeSelectorAsyncHandle *async_handle;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (uri != NULL);

    async_handle = _gnome_selector_async_handle_get
	(selector, GNOME_SELECTOR_ASYNC_TYPE_ADD_URI,
	 uri, async_func, user_data);
    if (async_handle_return != NULL)
	*async_handle_return = async_handle;

    gtk_signal_emit (GTK_OBJECT (selector),
		     gnome_selector_signals [ADD_URI_SIGNAL],
		     uri, position, list_id, async_handle);
}


void
gnome_selector_cancel_async_operation (GnomeSelector *selector, GnomeSelectorAsyncHandle *async_handle)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (selector));
    g_return_if_fail (async_handle != NULL);

    _gnome_selector_async_handle_destroy (async_handle);
}

