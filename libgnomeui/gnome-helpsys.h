/*
 * Copyright (C) 2000 Red Hat, Inc.
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

#ifndef GNOME_HELPSYS_H
#define GNOME_HELPSYS_H 1


#include <libgnome/gnome-url.h>

#include <gtk/gtk.h>
#include <libgnomeui/gnome-dock-item.h>

G_BEGIN_DECLS

#define GNOME_TYPE_HELP_VIEW            (gnome_help_view_get_type ())
#define GNOME_HELP_VIEW(obj)            (GTK_CHECK_CAST((obj), GNOME_TYPE_HELP_VIEW, GnomeHelpView))
#define GNOME_HELP_VIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST((klass), GNOME_TYPE_HELP_VIEW, GnomeHelpViewClass))
#define GNOME_IS_HELP_VIEW(obj)         (GTK_CHECK_TYPE((obj), GNOME_TYPE_HELP_VIEW))
#define GNOME_IS_HELP_VIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_HELP_VIEW))
#define GNOME_HELP_VIEW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_HELP_VIEW, GnomeHelpViewClass))

typedef enum {
	GNOME_HELP_POPUP,
	GNOME_HELP_EMBEDDED,
	GNOME_HELP_BROWSER
} GnomeHelpViewStyle;

/*FIXME: hack warning, G_PRIOTIY is something completely different
 * makeupourown enum */
typedef int GnomeHelpViewStylePriority; /* Use the G_PRIORITY_* system */

typedef struct _GnomeHelpView        GnomeHelpView;
typedef struct _GnomeHelpViewPrivate GnomeHelpViewPrivate;
typedef struct _GnomeHelpViewClass   GnomeHelpViewClass;

struct _GnomeHelpView {
  GtkBox parent_object;
  
  /*< private >*/
  GnomeHelpViewPrivate *_priv;
};

struct _GnomeHelpViewClass {
  GtkBoxClass parent_class;
};

GtkType		gnome_help_view_get_type	(void) G_GNUC_CONST;

GtkWidget *	gnome_help_view_new		(GtkWidget *toplevel,
						 GnomeHelpViewStyle app_style,
						 GnomeHelpViewStylePriority app_style_priority);

void		gnome_help_view_construct	(GnomeHelpView *self,
						 GtkWidget *toplevel,
						 GnomeHelpViewStyle app_style,
						 GnomeHelpViewStylePriority app_style_priority);

void		gnome_help_view_set_toplevel	(GnomeHelpView *help_view,
						 GtkWidget *toplevel);

GnomeHelpView *	gnome_help_view_find		(GtkWidget *awidget);
void		gnome_help_view_set_visibility	(GnomeHelpView *help_view,
						 gboolean visible);
void		gnome_help_view_set_style	(GnomeHelpView *help_view,
						 GnomeHelpViewStyle style,
						 GnomeHelpViewStylePriority style_priority);
void		gnome_help_view_select_help_cb	(GtkWidget *ignored,
						 GnomeHelpView *help_view);

/* as above, but does a lookup to try and figure out what the help_view is */
void		gnome_help_view_select_help_menu_cb(GtkWidget *widget);

void		gnome_help_view_show_help	(GnomeHelpView *help_view,
						 const char *help_path,
						 const char *help_type);
void		gnome_help_view_show_help_for	(GnomeHelpView *help_view,
						 GtkWidget *widget);
void		gnome_help_view_set_orientation	(GnomeHelpView *help_view,
						 GtkOrientation orientation);
void		gnome_help_view_display		(GnomeHelpView *help_view,
						 const char *help_path);
void		gnome_help_view_display_callback(GtkWidget *widget,
						 const char *help_path);

/*** Non-widget routines ***/

/* Turns a help path into a full URL */
char *		gnome_help_path_resolve		(const char *path,
						 const char *file_type);
GSList *	gnome_help_app_topics		(const char *app_id);

/* Object data on toplevel, name */
#define GNOME_APP_HELP_VIEW_NAME "HelpView"

/*** Utility routines ***/
void		gnome_widget_set_tooltip	(GtkWidget *widget,
						 const char *tiptext);
GtkTooltips *	gnome_widget_get_tooltips	(void);
GtkWidget *	gnome_widget_set_name		(GtkWidget *widget,
						 const char *name);

#define TT_(widget, text) gnome_widget_set_tooltip((widget), (text))
#define H_(widget, help_id) gnome_widget_set_name((widget), (help_id))

G_END_DECLS

#endif
