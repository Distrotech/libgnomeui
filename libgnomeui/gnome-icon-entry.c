/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
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

/* GnomeIconEntry widget - A button with the icon which allows graphical
 *			   picking of new icons.  The browse dialog is the
 *			   gnome-icon-sel with a gnome-file-entry which is
 *			   similiar to gnome-pixmap-entry.
 *
 *
 * Author: George Lebl <jirka@5z.com>
 * icon selection based on original dentry-edit code which was:
 *	Written by: Havoc Pennington, based on code by John Ellis.
 */
#include <config.h>
#include <unistd.h> /*getcwd*/
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkscrolledwindow.h>

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include <libgnome/gnome-util.h>

#include "gnome-file-entry.h"
#include "gnome-icon-list.h"
#include "gnome-icon-sel.h"
#include "gnome-icon-entry.h"
#include "gnome-pixmap.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

struct _GnomeIconEntryPrivate {
	GtkWidget *fentry;

	GtkWidget *pickbutton;
	
	GtkWidget *pick_dialog;
	gchar *pick_dialog_dir;

	gchar *history_id;
	gchar *browse_dialog_title;
};

static void gnome_icon_entry_class_init (GnomeIconEntryClass *class);
static void gnome_icon_entry_init       (GnomeIconEntry      *ientry);
static void drag_data_get		(GtkWidget          *widget,
					 GdkDragContext     *context,
					 GtkSelectionData   *selection_data,
					 guint               info,
					 guint               time,
					 GnomeIconEntry     *ientry);
static void drag_data_received		(GtkWidget        *widget,
					 GdkDragContext   *context,
					 gint              x,
					 gint              y,
					 GtkSelectionData *selection_data,
					 guint             info,
					 guint32           time,
					 GnomeIconEntry   *ientry);
static void ientry_destroy		(GtkObject *object);
static void ientry_finalize		(GObject *object);
static void ientry_get_arg		(GtkObject *object,
					 GtkArg *arg,
					 guint arg_id);
static void ientry_set_arg		(GtkObject *object,
					 GtkArg *arg,
					 guint arg_id);
static void ientry_browse               (GnomeIconEntry *ientry);

static GtkVBoxClass *parent_class;

static GtkTargetEntry drop_types[] = { { "text/uri-list", 0, 0 } };

enum {
	ARG_0,
	ARG_HISTORY_ID,
	ARG_BROWSE_DIALOG_TITLE,
	ARG_PIXMAP_SUBDIR,
	ARG_FILENAME,
	ARG_PICK_DIALOG
};

enum {
	CHANGED_SIGNAL,
	BROWSE_SIGNAL,
	LAST_SIGNAL
};

static gint gnome_ientry_signals[LAST_SIGNAL] = {0};


guint
gnome_icon_entry_get_type (void)
{
	static guint icon_entry_type = 0;

	if (!icon_entry_type) {
		GtkTypeInfo icon_entry_info = {
			"GnomeIconEntry",
			sizeof (GnomeIconEntry),
			sizeof (GnomeIconEntryClass),
			(GtkClassInitFunc) gnome_icon_entry_class_init,
			(GtkObjectInitFunc) gnome_icon_entry_init,
			NULL,
			NULL,
			NULL
		};

		icon_entry_type = gtk_type_unique (gtk_vbox_get_type (),
						   &icon_entry_info);
	}

	return icon_entry_type;
}

static void
gnome_icon_entry_class_init (GnomeIconEntryClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *)class;
	GObjectClass *gobject_class = (GObjectClass *)class;

	parent_class = gtk_type_class (gtk_vbox_get_type ());

	gnome_ientry_signals[CHANGED_SIGNAL] =
		gtk_signal_new("changed",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE(object_class),
			       GTK_SIGNAL_OFFSET(GnomeIconEntryClass,
			       			 changed),
			       gtk_signal_default_marshaller,
			       GTK_TYPE_NONE, 0);
	gnome_ientry_signals[BROWSE_SIGNAL] =
		gtk_signal_new("browse",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE(object_class),
			       GTK_SIGNAL_OFFSET(GnomeIconEntryClass,
			       			 browse),
			       gtk_signal_default_marshaller,
			       GTK_TYPE_NONE, 0);
	gtk_object_class_add_signals (object_class, gnome_ientry_signals,
				      LAST_SIGNAL);
	class->changed = NULL;
	class->browse = ientry_browse;

	gobject_class->finalize = ientry_finalize;
	object_class->destroy = ientry_destroy;

	gtk_object_add_arg_type("GnomeIconEntry::history_id",
				GTK_TYPE_STRING,
				GTK_ARG_READWRITE,
				ARG_HISTORY_ID);
	gtk_object_add_arg_type("GnomeIconEntry::browse_dialog_title",
				GTK_TYPE_STRING,
				GTK_ARG_READWRITE,
				ARG_BROWSE_DIALOG_TITLE);
	gtk_object_add_arg_type("GnomeIconEntry::pixmap_subdir",
				GTK_TYPE_STRING,
				GTK_ARG_WRITABLE,
				ARG_PIXMAP_SUBDIR);
	gtk_object_add_arg_type("GnomeIconEntry::filename",
				GTK_TYPE_STRING,
				GTK_ARG_READWRITE,
				ARG_FILENAME);
	gtk_object_add_arg_type("GnomeIconEntry::pick_dialog",
				GTK_TYPE_POINTER,
				GTK_ARG_READABLE,
				ARG_PICK_DIALOG);

	object_class->set_arg = ientry_set_arg;
	object_class->get_arg = ientry_get_arg;
}

static void
ientry_set_arg (GtkObject *object,
		GtkArg *arg,
		guint arg_id)
{
	GnomeIconEntry *self;

	self = GNOME_ICON_ENTRY (object);

	switch (arg_id) {
	case ARG_HISTORY_ID:
		gnome_icon_entry_set_history_id(self, GTK_VALUE_STRING(*arg));
		break;
	case ARG_BROWSE_DIALOG_TITLE:
		gnome_icon_entry_set_browse_dialog_title
			(self, GTK_VALUE_STRING(*arg));
		break;
	case ARG_PIXMAP_SUBDIR:
		gnome_icon_entry_set_pixmap_subdir(self,
						   GTK_VALUE_STRING(*arg));
		break;
	case ARG_FILENAME:
		gnome_icon_entry_set_filename(self, GTK_VALUE_STRING(*arg));
		break;
	default:
		break;
	}
}

static void
ientry_get_arg (GtkObject *object,
		GtkArg *arg,
		guint arg_id)
{
	GnomeIconEntry *self;

	self = GNOME_ICON_ENTRY (object);

	switch (arg_id) {
	case ARG_HISTORY_ID:
		GTK_VALUE_STRING(*arg) = g_strdup(self->_priv->history_id);
		break;
	case ARG_BROWSE_DIALOG_TITLE:
		GTK_VALUE_STRING(*arg) = g_strdup(self->_priv->browse_dialog_title);
		break;
	case ARG_FILENAME:
		GTK_VALUE_STRING(*arg) = gnome_icon_entry_get_filename(self);
		break;
	case ARG_PICK_DIALOG:
		GTK_VALUE_POINTER(*arg) = gnome_icon_entry_pick_dialog(self);
		break;
	default:
		break;
	}
}


static void
entry_changed(GtkWidget *widget, GnomeIconEntry *ientry)
{
	gchar *t;
        GdkPixbuf *pixbuf;
	GtkWidget *child;
	int w,h;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	t = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(ientry->_priv->fentry),
					   FALSE);

	child = GTK_BIN(ientry->_priv->pickbutton)->child;
	
	if(!t || !g_file_test (t, G_FILE_TEST_ISLINK|G_FILE_TEST_ISFILE) ||
	   !(pixbuf = gdk_pixbuf_new_from_file (t))) {
		if(GNOME_IS_PIXMAP(child)) {
			gtk_drag_source_unset (ientry->_priv->pickbutton);
			gtk_widget_destroy(child);
			child = gtk_label_new(_("No Icon"));
			gtk_widget_show(child);
			gtk_container_add(GTK_CONTAINER(ientry->_priv->pickbutton),
					  child);
		}
		g_free(t);
		return;
	}
	g_free(t);
	w = gdk_pixbuf_get_width(pixbuf);
	h = gdk_pixbuf_get_height(pixbuf);
	if(w>h) {
		if(w>48) {
			h = h*(48.0/w);
			w = 48;
		}
	} else {
		if(h>48) {
			w = w*(48.0/h);
			h = 48;
		}
	}
	if(GNOME_IS_PIXMAP(child)) {
                gnome_pixmap_clear(GNOME_PIXMAP(child));
		gnome_pixmap_set_pixbuf(GNOME_PIXMAP(child), pixbuf);
		gnome_pixmap_set_pixbuf_size (GNOME_PIXMAP(child), w, h);
        } else {
		gtk_widget_destroy(child);
		child = gnome_pixmap_new_from_pixbuf_at_size (pixbuf, w, h);
		gtk_widget_show(child);
		gtk_container_add(GTK_CONTAINER(ientry->_priv->pickbutton), child);

		if(!GTK_WIDGET_NO_WINDOW(child)) {
			gtk_signal_connect (GTK_OBJECT (child), "drag_data_get",
					    GTK_SIGNAL_FUNC (drag_data_get),ientry);
			gtk_drag_source_set (child,
					     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
					     drop_types, 1,
					     GDK_ACTION_COPY);
		}
	}
        gdk_pixbuf_unref(pixbuf);
	gtk_drag_source_set (ientry->_priv->pickbutton,
			     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			     drop_types, 1,
			     GDK_ACTION_COPY);
}

static void
entry_activated(GtkWidget *widget, GnomeIconEntry *ientry)
{
	struct stat buf;
	GnomeIconSelection * gis;
	gchar *filename;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_ENTRY (widget));
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	filename = gtk_entry_get_text (GTK_ENTRY (widget));

	if (!filename)
		return;

	stat (filename, &buf);
	if (S_ISDIR (buf.st_mode)) {
		gis = gtk_object_get_user_data(GTK_OBJECT(ientry));
		gnome_icon_selection_clear (gis, TRUE);
		gnome_icon_selection_add_directory (gis, filename);
		gnome_icon_selection_show_icons(gis);
	} else {
		/* We pretend like ok has been called */
		entry_changed (NULL, ientry);
		gtk_widget_hide (ientry->_priv->pick_dialog);
	}
}

static void
setup_preview(GtkWidget *widget)
{
	gchar *p;
	GList *l;
	GtkWidget *pp = NULL;
        GdkPixbuf *pixbuf;
	int w,h;
	GtkWidget *frame;
	GtkFileSelection *fs;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	frame = gtk_object_get_data(GTK_OBJECT(widget),"frame");
	fs = gtk_object_get_data(GTK_OBJECT(frame),"fs");

	if((l = gtk_container_children(GTK_CONTAINER(frame))) != NULL) {
		pp = l->data;
		g_list_free(l);
	}

	if(pp)
		gtk_widget_destroy(pp);
	
	p = gtk_file_selection_get_filename(fs);
	if(!p || !g_file_test (p,G_FILE_TEST_ISLINK|G_FILE_TEST_ISFILE) ||
	   !(pixbuf = gdk_pixbuf_new_from_file (p)))
		return;

	w = gdk_pixbuf_get_width(pixbuf);
	h = gdk_pixbuf_get_height(pixbuf);
	if(w>h) {
		if(w>100) {
			h = h*(100.0/w);
			w = 100;
		}
	} else {
		if(h>100) {
			w = w*(100.0/h);
			h = 100;
		}
	}
	pp = gnome_pixmap_new_from_pixbuf_at_size (pixbuf, w, h);
	gtk_widget_show(pp);
	gtk_container_add(GTK_CONTAINER(frame),pp);

        gdk_pixbuf_unref(pixbuf);
}

static void
ientry_destroy(GtkObject *object)
{
	GnomeIconEntry *ientry;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (object));

	ientry = GNOME_ICON_ENTRY(object);

	if(ientry->_priv->fentry)
		gtk_widget_unref (ientry->_priv->fentry);
	ientry->_priv->fentry = NULL;

	if(ientry->_priv->pick_dialog)
		gtk_widget_destroy(ientry->_priv->pick_dialog);
	ientry->_priv->pick_dialog = NULL;

	g_free(ientry->_priv->pick_dialog_dir);
	ientry->_priv->pick_dialog_dir = NULL;

	g_free(ientry->_priv->history_id);
	ientry->_priv->history_id = NULL;

	g_free(ientry->_priv->browse_dialog_title);
	ientry->_priv->browse_dialog_title = NULL;

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);

}

static void
ientry_finalize(GObject *object)
{
	GnomeIconEntry *ientry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (object));

	ientry = GNOME_ICON_ENTRY(object);

	g_free(ientry->_priv);
	ientry->_priv = NULL;

	if(G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize)(object);

}

static void
browse_clicked(GnomeFileEntry *fentry, GnomeIconEntry *ientry)
{
	GtkWidget *w;
	GtkWidget *hbox;

	GtkFileSelection *fs;

	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	if(!fentry->fsw)
		return;
	fs = GTK_FILE_SELECTION(fentry->fsw);
	
	hbox = fs->file_list;
	do {
		hbox = hbox->parent;
		if(!hbox) {
			g_warning(_("Can't find an hbox, using a normal file "
				    "selection"));
			return;
		}
	} while(!GTK_IS_HBOX(hbox));

	w = gtk_frame_new(_("Preview"));
	gtk_widget_show(w);
	gtk_box_pack_end(GTK_BOX(hbox),w,FALSE,FALSE,0);
	gtk_widget_set_usize(w,110,110);
	gtk_object_set_data(GTK_OBJECT(w),"fs",fs);
	
	gtk_object_set_data(GTK_OBJECT(fs->file_list),"frame",w);
	gtk_signal_connect(GTK_OBJECT(fs->file_list),"select_row",
			   GTK_SIGNAL_FUNC(setup_preview),NULL);
	gtk_object_set_data(GTK_OBJECT(fs->selection_entry),"frame",w);
	gtk_signal_connect_while_alive(GTK_OBJECT(fs->selection_entry),
				       "changed",
				       GTK_SIGNAL_FUNC(setup_preview),NULL,
				       GTK_OBJECT(fs));
}

static void
icon_selected_cb (GnomeIconEntry * ientry)
{
	const gchar * icon;
	GnomeIconSelection * gis;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	gis =  gtk_object_get_user_data(GTK_OBJECT(ientry));
	gnome_icon_selection_stop_loading(gis);
	icon = gnome_icon_selection_get_icon(gis, TRUE);

	if (icon != NULL) {
		GtkWidget *e = gnome_icon_entry_gtk_entry(ientry);
		gtk_entry_set_text(GTK_ENTRY(e),icon);
		entry_changed (NULL, ientry);
	}

	gtk_signal_emit(GTK_OBJECT(ientry),
			gnome_ientry_signals[CHANGED_SIGNAL]);
}

static void
cancel_pressed (GnomeIconEntry * ientry)
{
	GnomeIconSelection * gis;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	gis =  gtk_object_get_user_data(GTK_OBJECT(ientry));
	gnome_icon_selection_stop_loading(gis);
}


static void
gil_icon_selected_cb(GnomeIconList *gil, gint num, GdkEvent *event, GnomeIconEntry *ientry)
{
	const gchar * icon;
	GnomeIconSelection * gis;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	gis =  gtk_object_get_user_data(GTK_OBJECT(ientry));
	icon = gnome_icon_selection_get_icon(gis, TRUE);

	if (icon != NULL) {
		GtkWidget *e = gnome_icon_entry_gtk_entry(ientry);
		gtk_entry_set_text(GTK_ENTRY(e),icon);
		
	}

	if(event && event->type == GDK_2BUTTON_PRESS && ((GdkEventButton *)event)->button == 1) {
		gnome_icon_selection_stop_loading(gis);
		entry_changed (NULL, ientry);
		gtk_widget_hide(ientry->_priv->pick_dialog);
	}

	gtk_signal_emit(GTK_OBJECT(ientry),
			gnome_ientry_signals[CHANGED_SIGNAL]);
}

static void 
dialog_response (GtkWidget *dialog, gint response_id, gpointer data)
{
	GnomeIconEntry *ientry = data;

	if (response_id == 0 /* OK */) {
		icon_selected_cb (ientry);
		gtk_widget_destroy (dialog);
	} else if (response_id == 1 /* Cancel */) {
		cancel_pressed (ientry);
		gtk_widget_destroy (dialog);
	}
}

static void
ientry_browse(GnomeIconEntry *ientry)
{
	GnomeFileEntry *fe;
	gchar *p;
	gchar *curfile;
	GtkWidget *tl;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	fe = GNOME_FILE_ENTRY(ientry->_priv->fentry);
	p = gnome_file_entry_get_full_path(fe,FALSE);
	curfile = gnome_icon_entry_get_filename(ientry);

	/* Are we part of a modal window?  If so, we need to be modal too. */
	tl = gtk_widget_get_toplevel (GTK_WIDGET (ientry->_priv->pickbutton));
	
	if(!p) {
		if(fe->default_path)
			p = g_strdup(fe->default_path);
		else {
			/*get around the g_free/free issue*/
			gchar *cwd = g_get_current_dir ();
			p = g_strdup(cwd);
			g_free(cwd);
		}
		gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ientry->_priv->fentry))),
				    p);
	}

	/*figure out the directory*/
	if(!g_file_test (p,G_FILE_TEST_ISDIR)) {
		gchar *d;
		d = g_path_get_dirname (p);
		g_free (p);
		p = d;
		if(!g_file_test (p,G_FILE_TEST_ISDIR)) {
			g_free (p);
			if(fe->default_path)
				p = g_strdup(fe->default_path);
			else {
				/*get around the g_free/free issue*/
				gchar *cwd = g_get_current_dir ();
				p = g_strdup(cwd);
				free(cwd);
			}
			gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ientry->_priv->fentry))),
				    p);
			g_return_if_fail(g_file_test (p,G_FILE_TEST_ISDIR));
		}
	}
	

	if(ientry->_priv->pick_dialog==NULL ||
	   ientry->_priv->pick_dialog_dir==NULL ||
	   strcmp(p,ientry->_priv->pick_dialog_dir)!=0) {
		GtkWidget * iconsel;
		GtkWidget * gil;
		
		if(ientry->_priv->pick_dialog) {
			gtk_container_remove (GTK_CONTAINER (ientry->_priv->fentry->parent), ientry->_priv->fentry);
			gtk_widget_destroy(ientry->_priv->pick_dialog);
		}
		
		g_free(ientry->_priv->pick_dialog_dir);
		ientry->_priv->pick_dialog_dir = p;
		ientry->_priv->pick_dialog = 
			gtk_dialog_new_with_buttons (ientry->_priv->browse_dialog_title,
						     GTK_WINDOW (tl), 
						     (GTK_WINDOW (tl)->modal ? GTK_DIALOG_MODAL : 0),
						     GTK_STOCK_OK,
						     GTK_STOCK_CANCEL,
						     NULL);
		gtk_signal_connect (GTK_OBJECT (ientry->_priv->pick_dialog),
				    "destroy",
				    GTK_SIGNAL_FUNC (gtk_widget_destroyed),
				    &ientry->_priv->pick_dialog);

		gtk_window_set_policy(GTK_WINDOW(ientry->_priv->pick_dialog), 
				      TRUE, TRUE, TRUE);


		iconsel = gnome_icon_selection_new();

		gtk_object_set_user_data(GTK_OBJECT(ientry), iconsel);

		gnome_icon_selection_add_directory(GNOME_ICON_SELECTION(iconsel),
						   ientry->_priv->pick_dialog_dir);


		gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(ientry->_priv->pick_dialog)->vbox),
				   ientry->_priv->fentry, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(ientry->_priv->pick_dialog)->vbox),
				   iconsel, TRUE, TRUE, 0);

		gtk_widget_show_all(ientry->_priv->pick_dialog);

		gnome_icon_selection_show_icons(GNOME_ICON_SELECTION(iconsel));

		if(curfile != NULL) {
			char *base = g_path_get_basename(curfile);
			gnome_icon_selection_select_icon(GNOME_ICON_SELECTION(iconsel), 
							 base);
			g_free(base);
		}

		gtk_signal_connect (GTK_OBJECT (ientry->_priv->pick_dialog), 
				    "response",
				    GTK_SIGNAL_FUNC (dialog_response),
				    ientry);

		gil = gnome_icon_selection_get_gil(GNOME_ICON_SELECTION(iconsel));
		gtk_signal_connect_after(GTK_OBJECT(gil), "select_icon",
					 GTK_SIGNAL_FUNC(gil_icon_selected_cb),
					 ientry);
	} else {
		GnomeIconSelection *gis =
			gtk_object_get_user_data(GTK_OBJECT(ientry));
		if(!GTK_WIDGET_VISIBLE(ientry->_priv->pick_dialog))
			gtk_widget_show(ientry->_priv->pick_dialog);
		if(gis) gnome_icon_selection_show_icons(gis);
	}
}

static void
show_icon_selection(GtkButton * b, GnomeIconEntry * ientry)
{
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	gtk_signal_emit(GTK_OBJECT(ientry),
			gnome_ientry_signals[BROWSE_SIGNAL]);
}

static void
drag_data_received (GtkWidget        *widget,
		    GdkDragContext   *context,
		    gint              x,
		    gint              y,
		    GtkSelectionData *selection_data,
		    guint             info,
		    guint32           time,
		    GnomeIconEntry   *ientry)
{
	GList *files, *li;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	/*here we extract the filenames from the URI-list we recieved*/
	files = gnome_uri_list_extract_filenames(selection_data->data);
	/*if there's isn't a file*/
	if(!files) {
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}

	for(li = files; li!=NULL; li = li->next) {
		/* FIXME! we have to do this by hand nowdays, no ditem */
#ifdef FIXME
		const char *mimetype;

		mimetype = gnome_mime_type(li->data);

		if(mimetype
			&& !strcmp(mimetype, "application/x-gnome-app-info")) {
			/* hmmm a desktop, try loading the icon from that */
			GnomeDesktopItem * item;
			const char *icon;

			item = gnome_desktop_item_new_from_file
				(li->data,
				 GNOME_DESKTOP_ITEM_LOAD_NO_SYNC |
				 GNOME_DESKTOP_ITEM_LOAD_NO_OTHER_SECTIONS);
			if(!item)
				continue;
			icon = gnome_desktop_item_get_icon_path(item);

			if(gnome_icon_entry_set_filename(ientry, icon)) {
				gnome_desktop_item_unref(item);
				break;
			}
			gnome_desktop_item_unref(item);
#endif
		} else if(gnome_icon_entry_set_filename(ientry, li->data))
			break;
	}

	/*free the list of files we got*/
	gnome_uri_list_free_strings (files);
}

static void  
drag_data_get  (GtkWidget          *widget,
		GdkDragContext     *context,
		GtkSelectionData   *selection_data,
		guint               info,
		guint               time,
		GnomeIconEntry     *ientry)
{
	gchar *string;
	gchar *file;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	file = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(ientry->_priv->fentry),
					      TRUE);

	if(!file) {
		/*FIXME: cancel the drag*/
		return;
	}

	string = g_strdup_printf("file:%s\r\n",file);
	g_free(file);
	gtk_selection_data_set (selection_data,
				selection_data->target,
				8, string, strlen(string)+1);
	g_free(string);
}


static void
gnome_icon_entry_init (GnomeIconEntry *ientry)
{
	GtkWidget *w;
	gchar *p;

	ientry->_priv = g_new0(GnomeIconEntryPrivate, 1);

	gtk_box_set_spacing (GTK_BOX (ientry), 4);

	ientry->_priv->pick_dialog = NULL;
	ientry->_priv->pick_dialog_dir = NULL;
	
	w = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_widget_show(w);
	gtk_box_pack_start (GTK_BOX (ientry), w, TRUE, TRUE, 0);
	ientry->_priv->pickbutton = gtk_button_new_with_label(_("No Icon"));
	gtk_drag_dest_set (GTK_WIDGET (ientry->_priv->pickbutton),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, 1, GDK_ACTION_COPY);
	gtk_signal_connect (GTK_OBJECT (ientry->_priv->pickbutton),
			    "drag_data_received",
			    GTK_SIGNAL_FUNC (drag_data_received),ientry);
	gtk_signal_connect (GTK_OBJECT (ientry->_priv->pickbutton),
			    "drag_data_get",
			    GTK_SIGNAL_FUNC (drag_data_get),ientry);

	gtk_signal_connect(GTK_OBJECT(ientry->_priv->pickbutton), "clicked",
			   GTK_SIGNAL_FUNC(show_icon_selection),ientry);
	/*FIXME: 60x60 is just larger then default 48x48, though icon sizes
	  are supposed to be selectable I guess*/
	gtk_widget_set_usize(ientry->_priv->pickbutton,60,60);
	gtk_container_add (GTK_CONTAINER (w), ientry->_priv->pickbutton);
	gtk_widget_show (ientry->_priv->pickbutton);

	ientry->_priv->fentry = gnome_file_entry_new (NULL, _("Browse"));
	/*BORPORP */
	gnome_file_entry_set_modal (GNOME_FILE_ENTRY (ientry->_priv->fentry),
				    TRUE);
	gtk_widget_ref (ientry->_priv->fentry);
	gtk_signal_connect_after(GTK_OBJECT(ientry->_priv->fentry),
				 "browse_clicked",
				 GTK_SIGNAL_FUNC(browse_clicked),
				 ientry);

	gtk_widget_show (ientry->_priv->fentry);
	
	p = gnome_pixmap_file(".");
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(ientry->_priv->fentry),p);
	g_free(p);
	
	w = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(ientry->_priv->fentry));
/*	gtk_signal_connect_while_alive(GTK_OBJECT(w), "changed",
				       GTK_SIGNAL_FUNC(entry_changed),
				       ientry, GTK_OBJECT(ientry));*/
	gtk_signal_connect_while_alive(GTK_OBJECT(w), "activate",
				       GTK_SIGNAL_FUNC(entry_activated),
				       ientry, GTK_OBJECT(ientry));
	
	
	/*just in case there is a default that is an image*/
	entry_changed(w,ientry);
}

/**
 * gnome_icon_entry_construct:
 * @ientry: the GnomeIconEntry to work with
 * @history_id: the id given to #gnome_entry_new in the browse dialog
 * @browse_dialog_title: title of the icon selection dialog
 *
 * Description: For language bindings and subclassing, from C use
 * #gnome_icon_entry_new
 *
 * Returns:
 **/
void
gnome_icon_entry_construct (GnomeIconEntry *ientry,
			    const gchar *history_id,
			    const gchar *browse_dialog_title)
{
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	gnome_icon_entry_set_history_id(ientry, history_id);
	gnome_icon_entry_set_browse_dialog_title(ientry, browse_dialog_title);
}

/**
 * gnome_icon_entry_new:
 * @history_id: the id given to #gnome_entry_new
 * @browse_dialog_title: title of the browse dialog and icon selection dialog
 *
 * Description: Creates a new icon entry widget
 *
 * Returns: Returns the new object
 **/
GtkWidget *
gnome_icon_entry_new (const gchar *history_id, const gchar *browse_dialog_title)
{
	GnomeIconEntry *ientry;

	ientry = gtk_type_new (gnome_icon_entry_get_type ());

	gnome_icon_entry_construct (ientry, history_id, browse_dialog_title);
	
	return GTK_WIDGET (ientry);
}


/**
 * gnome_icon_entry_set_pixmap_subdir:
 * @ientry: the GnomeIconEntry to work with
 * @subdir: subdirectory
 *
 * Description: Sets the subdirectory below gnome's default
 * pixmap directory to use as the default path for the file
 * entry.  The path can also be an absolute one.  If %NULL is passed
 * then the pixmap directory itself is used.
 *
 * Returns:
 **/
void
gnome_icon_entry_set_pixmap_subdir(GnomeIconEntry *ientry,
				   const gchar *subdir)
{
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));
	
	if(!subdir)
		subdir = ".";

	if(g_path_is_absolute(subdir)) {
		gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(ientry->_priv->fentry), subdir);
	} else {
		gchar *p = gnome_pixmap_file(subdir);
		gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(ientry->_priv->fentry), p);
		g_free(p);
	}
}

/**
 * gnome_icon_entry_set_filename:
 * @ientry: the GnomeIconEntry to work with
 * @filename: a filename
 * 
 * Description: Sets the icon of GnomeIconEntry to be the one pointed to by
 * @filename (in the current subdirectory).
 *
 * Returns: %TRUE if icon was loaded ok, %FALSE otherwise
 **/
gboolean
gnome_icon_entry_set_filename(GnomeIconEntry *ientry,
			      const gchar *filename)
{
	GtkWidget *child;

	g_return_val_if_fail (ientry != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), FALSE);
	
	if(!filename)
		filename = "";

	gtk_entry_set_text (GTK_ENTRY (gnome_icon_entry_gtk_entry (ientry)),
			    filename);
	entry_changed (NULL, ientry);
	gtk_signal_emit(GTK_OBJECT(ientry),
			gnome_ientry_signals[CHANGED_SIGNAL]);

	child = GTK_BIN(ientry->_priv->pickbutton)->child;
	/* this happens if it doesn't exist or isn't an image */
	if(!GNOME_IS_PIXMAP(child))
		return FALSE;

	return TRUE;
}

/**
 * gnome_icon_entry_get_filename:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Gets the file name of the image if it was possible
 * to load it into the preview. That is, it will only return a filename
 * if the image exists and it was possible to load it as an image.
 *
 * Returns: a newly allocated string with the path or %NULL if it
 * couldn't load the file
 **/
gchar *
gnome_icon_entry_get_filename(GnomeIconEntry *ientry)
{
	GtkWidget *child;

	g_return_val_if_fail (ientry != NULL,NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry),NULL);

	child = GTK_BIN(ientry->_priv->pickbutton)->child;
	
	/* this happens if it doesn't exist or isn't an image */
	if( ! GNOME_IS_PIXMAP(child))
		return NULL;
	
	return gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(ientry->_priv->fentry),
					      TRUE);
}

/**
 * gnome_icon_entry_pick_dialog:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: If a pick dialog exists, return a pointer to it or
 * return NULL.  This is if you need to do something with all dialogs.
 * You would use the browse signal with connect_after to get the
 * pick dialog when it is displayed.
 * 
 * Returns: The pick dialog or %NULL if none exists
 **/
GtkWidget *
gnome_icon_entry_pick_dialog(GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL,NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry),NULL);

	return ientry->_priv->pick_dialog;
}

/**
 * gnome_icon_entry_set_browse_dialog_title:
 * @ientry: the GnomeIconEntry to work with
 * @browse_dialog_title: title of the icon selection dialog
 *
 * Description:  Set the title of the browse dialog.  It will not effect
 * an existing dialog.
 *
 * Returns:
 **/
void
gnome_icon_entry_set_browse_dialog_title(GnomeIconEntry *ientry,
					 const gchar *browse_dialog_title)
{
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	g_free(ientry->_priv->browse_dialog_title);
	ientry->_priv->browse_dialog_title = g_strdup(browse_dialog_title);
}

/**
 * gnome_icon_entry_set_history_id:
 * @ientry: the GnomeIconEntry to work with
 * @history_id: the id given to #gnome_entry_new in the browse dialog
 *
 * Description:  Set the history_id of the entry in the browse dialog
 * and reload the history
 *
 * Returns:
 **/
void
gnome_icon_entry_set_history_id(GnomeIconEntry *ientry,
				const gchar *history_id)
{
	GtkWidget *gentry;

	g_free(ientry->_priv->history_id);
	ientry->_priv->history_id = g_strdup(history_id);

	gentry = gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(ientry->_priv->fentry));
	gnome_entry_set_history_id (GNOME_ENTRY(gentry), history_id);
	gnome_entry_load_history (GNOME_ENTRY(gentry));
}

/* DEPRECATED routines left for compatibility only, will disapear in
 * some very distant future */

/**
 * gnome_icon_entry_set_icon:
 * @ientry: the GnomeIconEntry to work with
 * @filename: a filename
 * 
 * Description: Deprecated in favour of #gnome_icon_entry_set_filename
 *
 * Returns:
 **/
void
gnome_icon_entry_set_icon(GnomeIconEntry *ientry,
			  const gchar *filename)
{
	g_warning("gnome_icon_entry_set_icon deprecated, "
		  "use gnome_icon_entry_set_filename!");
	gnome_icon_entry_set_filename(ientry, filename);
}

/**
 * gnome_icon_entry_gnome_file_entry:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Get the GnomeFileEntry widget that's part of the entry
 * DEPRECATED! Use the "changed" signal for getting changes
 *
 * Returns: Returns GnomeFileEntry widget
 **/
GtkWidget *
gnome_icon_entry_gnome_file_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	g_warning("gnome_icon_entry_gnome_file_entry deprecated, "
		  "use changed signal!");
	return ientry->_priv->fentry;
}

/**
 * gnome_icon_entry_gnome_entry:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Get the GnomeEntry widget that's part of the entry
 * DEPRECATED! Use the "changed" signal for getting changes
 *
 * Returns: Returns GnomeEntry widget
 **/
GtkWidget *
gnome_icon_entry_gnome_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	g_warning("gnome_icon_entry_gnome_entry deprecated, "
		  "use changed signal!");

	return gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(ientry->_priv->fentry));
}

/**
 * gnome_icon_entry_gtk_entry:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Get the GtkEntry widget that's part of the entry.
 * DEPRECATED! Use the "changed" signal for getting changes
 *
 * Returns: Returns GtkEntry widget
 **/
GtkWidget *
gnome_icon_entry_gtk_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	g_warning("gnome_icon_entry_gtk_entry deprecated, "
		  "use changed signal!");

	return gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ientry->_priv->fentry));
}
