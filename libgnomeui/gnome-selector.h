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

#ifndef GNOME_SELECTOR_H
#define GNOME_SELECTOR_H


#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>


G_BEGIN_DECLS


#define GNOME_TYPE_SELECTOR            (gnome_selector_get_type ())
#define GNOME_SELECTOR(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_SELECTOR, GnomeSelector))
#define GNOME_SELECTOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_SELECTOR, GnomeSelectorClass))
#define GNOME_IS_SELECTOR(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_SELECTOR))
#define GNOME_IS_SELECTOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_SELECTOR))
#define GNOME_SELECTOR_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_SELECTOR, GnomeSelectorClass))


typedef struct _GnomeSelector             GnomeSelector;
typedef struct _GnomeSelectorPrivate      GnomeSelectorPrivate;
typedef struct _GnomeSelectorClass        GnomeSelectorClass;

typedef struct _GnomeSelectorAsyncHandle  GnomeSelectorAsyncHandle;
typedef gint                              GnomeSelectorAsyncType;

typedef void (*GnomeSelectorAsyncFunc)   (GnomeSelector            *selector,
                                          GnomeSelectorAsyncHandle *async_handle,
                                          GnomeSelectorAsyncType    async_type,
                                          const char               *uri,
                                          GError                   *error,
                                          gboolean                  success,
                                          gpointer                  user_data);

enum {
    GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET    = 1 << 0,
    GNOME_SELECTOR_DEFAULT_SELECTOR_WIDGET = 1 << 1,
    GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG   = 1 << 2,
    GNOME_SELECTOR_WANT_BROWSE_BUTTON      = 1 << 3,
    GNOME_SELECTOR_WANT_CLEAR_BUTTON       = 1 << 4,
    GNOME_SELECTOR_WANT_DEFAULT_BUTTON     = 1 << 5,
    GNOME_SELECTOR_AUTO_SAVE_HISTORY       = 1 << 16,
    GNOME_SELECTOR_AUTO_SAVE_ALL           = 1 << 17
};

enum {
    GNOME_SELECTOR_LIST_PRIMARY            = 0,
    GNOME_SELECTOR_LIST_DEFAULT,
    GNOME_SELECTOR_LIST_MAX
};

#define GNOME_SELECTOR_USER_FLAGS          (~((1 << 16)-1))

struct _GnomeSelector {
    GtkVBox vbox;
        
    /*< private >*/
    GnomeSelectorPrivate *_priv;
};

struct _GnomeSelectorClass {
    GtkVBoxClass parent_class;

    void     (*changed)               (GnomeSelector            *selector);
    void     (*browse)                (GnomeSelector            *selector);
    void     (*clear)                 (GnomeSelector            *selector,
                                       guint                     list_id);

    void     (*freeze)                (GnomeSelector            *selector);
    void     (*update)                (GnomeSelector            *selector);
    void     (*thaw)                  (GnomeSelector            *selector);

    gchar *  (*get_uri)               (GnomeSelector            *selector);
    void     (*set_uri)               (GnomeSelector            *selector,
                                       const gchar              *filename,
                                       GnomeSelectorAsyncHandle *async_handle);

    gchar *  (*get_entry_text)        (GnomeSelector            *selector);
    void     (*set_entry_text)        (GnomeSelector            *selector,
                                       const gchar              *text);
    void     (*activate_entry)        (GnomeSelector            *selector);

    void     (*update_uri_list)       (GnomeSelector            *selector,
                                       guint                     list_id);

    GSList * (*get_uri_list)          (GnomeSelector            *selector,
                                       guint                     list_id);
    void     (*add_uri_list)          (GnomeSelector            *selector,
                                       GSList                   *list,
                                       gint                      position,
                                       guint                     list_id,
                                       GnomeSelectorAsyncHandle *async_handle);

    GtkSelectionMode (*get_selection_mode) (GnomeSelector       *selector);
    void     (*set_selection_mode)    (GnomeSelector            *selector,
                                       GtkSelectionMode          mode);
    GSList * (*get_selection)         (GnomeSelector            *selector);

    void     (*selection_changed)     (GnomeSelector            *selector);
    void     (*history_changed)       (GnomeSelector            *selector);

    /* Check whether it's ok to add the file/directory to the selector. */
    void     (*check_filename)        (GnomeSelector            *selector,
                                       const gchar              *filename,
                                       GnomeSelectorAsyncHandle *async_handle);
    void     (*check_directory)       (GnomeSelector            *selector,
                                       const gchar              *directory,
                                       GnomeSelectorAsyncHandle *async_handle);

    /* Add a file/directory to the selector. */
    void     (*add_file)              (GnomeSelector            *selector,
                                       const gchar              *filename,
                                       gint                      position,
                                       guint                     list_id,
                                       GnomeSelectorAsyncHandle *async_handle);
    void     (*add_directory)         (GnomeSelector            *selector,
                                       const gchar              *directory,
                                       gint                      position,
                                       guint                     list_id,
                                       GnomeSelectorAsyncHandle *async_handle);

    void     (*add_uri)               (GnomeSelector            *selector,
                                       const gchar              *filename,
                                       gint                      position,
                                       guint                     list_id,
                                       GnomeSelectorAsyncHandle *async_handle);

    /* For subclasses only. */
    void     (*do_construct)          (GnomeSelector            *selector);
};

guint
gnome_selector_get_type           (void) G_GNUC_CONST;

/* This is a purely virtual class, so there is no _new method.
 * Use gnome_{file,icon,pixmap}_selector_new instead. */

/* This is ONLY for subclasses ! */
void
gnome_selector_do_construct       (GnomeSelector             *selector);

/* checks whether this is a valid filename/directory. */
void
gnome_selector_check_filename     (GnomeSelector             *selector,
                                   GnomeSelectorAsyncHandle **async_handle_return,
                                   const gchar               *filename,
                                   GnomeSelectorAsyncFunc     async_func,
                                   gpointer                   user_data);
void
gnome_selector_check_directory    (GnomeSelector             *selector,
                                   GnomeSelectorAsyncHandle **async_handle_return,
                                   const gchar               *directory,
                                   GnomeSelectorAsyncFunc     async_func,
                                   gpointer                   user_data);

/* Add file/directory. */
void
gnome_selector_add_file           (GnomeSelector             *selector,
                                   GnomeSelectorAsyncHandle **async_handle_return,
                                   const gchar               *filename,
                                   gint                       position,
                                   guint                      list_id,
                                   GnomeSelectorAsyncFunc     async_func,
                                   gpointer                   user_data);

void
gnome_selector_add_directory      (GnomeSelector             *selector,
                                   GnomeSelectorAsyncHandle **async_handle_return,
                                   const gchar               *directory,
                                   gint                       position,
                                   guint                      list_id,
                                   GnomeSelectorAsyncFunc     async_func,
                                   gpointer                   user_data);

void
gnome_selector_add_uri            (GnomeSelector             *selector,
                                   GnomeSelectorAsyncHandle **async_handle_return,
                                   const gchar               *filename,
                                   gint                       position,
                                   guint                      list_id,
                                   GnomeSelectorAsyncFunc     async_func,
                                   gpointer                   user_data);


/* Get/set file list (set will replace the old file list). */
GSList *
gnome_selector_get_uri_list       (GnomeSelector             *selector,
                                   guint                      list_id);
void
gnome_selector_add_uri_list       (GnomeSelector             *selector,
                                   GnomeSelectorAsyncHandle **async_handle_return,
                                   GSList                    *uri_list,
                                   gint                       position,
                                   guint                      list_id,
                                   GnomeSelectorAsyncFunc     async_func,
                                   gpointer                   user_data);

/* Get/set URI. */
gchar *
gnome_selector_get_uri            (GnomeSelector             *selector);

void
gnome_selector_set_uri            (GnomeSelector             *selector,
                                   GnomeSelectorAsyncHandle **async_handle_return,
                                   const gchar               *filename,
                                   GnomeSelectorAsyncFunc     async_func,
                                   gpointer                   user_data);

/* Remove all entries from the selector. */
void
gnome_selector_clear              (GnomeSelector             *selector,
                                   guint                      list_id);

/* Updates the internal file list. This will also read all the directories
 * from the directory list and add the files to an internal list. */
void
gnome_selector_update_uri_list    (GnomeSelector             *selector,
                                   guint                      list_id);

/* Get/set the selection mode. */
GtkSelectionMode
gnome_selector_get_selection_mode (GnomeSelector             *selector);

void
gnome_selector_set_selection_mode (GnomeSelector             *selector,
                                   GtkSelectionMode           mode);

/* Returns the current selection. */
GSList *
gnome_selector_get_selection      (GnomeSelector             *selector);

/* To avoid excesive recomputes during insertion/deletion */
void
gnome_selector_freeze             (GnomeSelector             *selector);

gboolean
gnome_selector_is_frozen          (GnomeSelector             *selector);

void
gnome_selector_thaw               (GnomeSelector             *selector);

/* Perform an update (also works in frozen state). */
void
gnome_selector_update             (GnomeSelector             *selector);

/* Get/set the dialog title. */
const gchar *
gnome_selector_get_dialog_title   (GnomeSelector             *selector);

void
gnome_selector_set_dialog_title   (GnomeSelector             *selector,
                                   const gchar               *dialog_title);

/* Get/set the history id. */
const gchar *
gnome_selector_get_history_id     (GnomeSelector             *selector);

void
gnome_selector_set_history_id     (GnomeSelector             *selector,
                                   const gchar               *history_id);

/* Get/set the text in the entry widget. */
gchar *
gnome_selector_get_entry_text     (GnomeSelector             *selector);

void
gnome_selector_set_entry_text     (GnomeSelector             *selector,
                                   const gchar               *text);

/* If the entry widget is derived from GtkEditable, then we can use this
 * function to send an "activate" signal to it. */
void
gnome_selector_activate_entry     (GnomeSelector             *selector);

/* Get/set maximum number of history items we save. */
guint
gnome_selector_get_history_length (GnomeSelector             *selector);

void
gnome_selector_set_history_length (GnomeSelector             *selector,
                                   guint                      history_length);

/* Append/Prepend an item to the history. */
void
gnome_selector_prepend_history    (GnomeSelector             *selector,
                                   gboolean                   save,
                                   const gchar               *text);

void
gnome_selector_append_history     (GnomeSelector             *selector,
                                   gboolean                   save,
                                   const gchar               *text);

/* Get/set the history. */
GSList *
gnome_selector_get_history        (GnomeSelector             *selector);

void
gnome_selector_set_history        (GnomeSelector             *selector,
                                   GSList                    *history);

/* Load/save/clear the history. */
void
gnome_selector_load_history       (GnomeSelector             *selector);

void
gnome_selector_save_history       (GnomeSelector             *selector);

void
gnome_selector_clear_history      (GnomeSelector             *selector);

/* Async operations. */
void
gnome_selector_cancel_async_operation (GnomeSelector             *selector,
                                       GnomeSelectorAsyncHandle  *async_handle);

void
gnome_selector_async_handle_ref   (GnomeSelectorAsyncHandle  *async_handle);

void
gnome_selector_async_handle_unref (GnomeSelectorAsyncHandle  *async_handle);

G_END_DECLS

#endif
