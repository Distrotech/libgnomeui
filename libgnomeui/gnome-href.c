/* gnome-href.c
 * Copyright (C) 1998, James Henstridge <james@daa.com.au>
 * All rights reserved.
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

#include "config.h"

#include <string.h> /* for strlen */

#include <gtk/gtk.h>
#include <libgnome/gnome-url.h>
#include "gnome-href.h"
#include "gnome-dialog-util.h"
#include "gnome-cursors.h"

#include "libgnome/gnome-i18nP.h"

static void gnome_href_class_init(GnomeHRefClass *klass);
static void gnome_href_init(GnomeHRef *href);
static void gnome_href_clicked(GtkButton *button);
static void gnome_href_destroy(GtkObject *object);
static void gnome_href_get_arg(GtkObject *object,
			       GtkArg *arg,
			       guint arg_id);
static void gnome_href_set_arg(GtkObject *object,
			       GtkArg *arg,
			       guint arg_id);
static void gnome_href_realize(GtkWidget *widget);

static GtkObjectClass *parent_class;

enum {
	ARG_0,
	ARG_URL,
	ARG_LABEL
};

/**
 * gnome_href_get_type
 *
 * Returns the type assigned to the GNOME href widget.
 **/

guint gnome_href_get_type(void) {
  static guint href_type = 0;
  if (!href_type) {
    GtkTypeInfo href_info = {
      "GnomeHRef",
      sizeof(GnomeHRef),
      sizeof(GnomeHRefClass),
      (GtkClassInitFunc) gnome_href_class_init,
      (GtkObjectInitFunc) gnome_href_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL,
    };
    href_type = gtk_type_unique(gtk_button_get_type(), &href_info);
  }
  return href_type;
}

static void gnome_href_class_init(GnomeHRefClass *klass) {
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkButtonClass *button_class;

  object_class = GTK_OBJECT_CLASS(klass);
  widget_class = GTK_WIDGET_CLASS(klass);
  button_class = GTK_BUTTON_CLASS(klass);
  parent_class = GTK_OBJECT_CLASS(gtk_type_class(gtk_button_get_type()));

  gtk_object_add_arg_type("GnomeHRef::url",
			  GTK_TYPE_POINTER,
			  GTK_ARG_READWRITE,
			  ARG_URL);
  gtk_object_add_arg_type("GnomeHRef::label",
			  GTK_TYPE_POINTER,
			  GTK_ARG_READWRITE,
			  ARG_LABEL);

  object_class->destroy = gnome_href_destroy;
  object_class->set_arg = gnome_href_set_arg;
  object_class->get_arg = gnome_href_get_arg;
  widget_class->realize = gnome_href_realize;
  button_class->clicked = gnome_href_clicked;
}

static void gnome_href_init(GnomeHRef *href) {
  href->label = gtk_label_new("");
  gtk_button_set_relief(GTK_BUTTON(href), GTK_RELIEF_NONE);
  gtk_container_add(GTK_CONTAINER(href), href->label);
  gtk_widget_show(href->label);
  href->url = NULL;
}

/**
 * gnome_href_construct
 * @href: Pointer to GnomeHRef widget
 * @url: URL assigned to this object.
 * @label: Text associated with the URL.
 *
 * Description:
 * For bindings and subclassing, in C you should use #gnome_href_new
 *
 * Returns:
 **/

void gnome_href_construct(GnomeHRef *href, const gchar *url, const gchar *label) {
	
  g_return_if_fail(href != NULL);
  g_return_if_fail(GNOME_IS_HREF(href));
  g_return_if_fail(url != NULL);

  gnome_href_set_url(href, url);

  if (!label)
    label = url;

  gnome_href_set_label(href, label);
}


/**
 * gnome_href_new
 * @url: URL assigned to this object.
 * @label: Text associated with the URL.
 *
 * Description:
 * Created a GNOME href object, a label widget with a clickable action
 * and an associated URL.  If @label is set to %NULL, @url is used as
 * the label.
 *
 * Returns:  Pointer to new GNOME href widget.
 **/

GtkWidget *gnome_href_new(const gchar *url, const gchar *label) {
  GnomeHRef *href;

  g_return_val_if_fail(url != NULL, NULL);

  href = gtk_type_new(gnome_href_get_type());

  gnome_href_construct(href, url, label);

  return GTK_WIDGET(href);
}


/**
 * gnome_href_get_url
 * @href: Pointer to GnomeHRef widget
 *
 * Description:
 * Returns the pointer to the URL associated with the @href href object.  Note
 * that the string should not be freed as it is internal memory.
 *
 * Returns:  Pointer to an internal URL string, or %NULL if failure.
 **/

gchar *gnome_href_get_url(GnomeHRef *href) {
  g_return_val_if_fail(href != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_HREF(href), NULL);
  return href->url;
}


/**
 * gnome_href_set_url
 * @href: Pointer to GnomeHRef widget
 * @url: String containing the URL to be stored within @href.
 *
 * Description:
 * Sets the internal URL value within @href to the value of @url.
 **/

void gnome_href_set_url(GnomeHRef *href, const gchar *url) {
  g_return_if_fail(href != NULL);
  g_return_if_fail(GNOME_IS_HREF(href));
  g_return_if_fail(url != NULL);

  if (href->url)
    g_free(href->url);
  href->url = g_strdup(url);
}


/**
 * gnome_href_get_label
 * @href: Pointer to GnomeHRef widget
 *
 * Description:
 * Returns the contents of the label widget used to display the link text.
 * Note that the string should not be freed as it points to internal memory.
 *
 * Returns:  Pointer to text contained in the label widget.
 **/

gchar *gnome_href_get_label(GnomeHRef *href) {
  gchar *ret;

  g_return_val_if_fail(href != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_HREF(href), NULL);

  gtk_label_get(GTK_LABEL(href->label), &ret);
  return ret;
}


/**
 * gnome_href_set_label
 * @href: Pointer to GnomeHRef widget
 * @label: New link text for the href object.
 *
 * Description:
 * Sets the internal label widget text (used to display a URL's link
 * text) to the value given in @label.
 **/

void gnome_href_set_label(GnomeHRef *href, const gchar *label) {
  gchar *pattern;

  g_return_if_fail(href != NULL);
  g_return_if_fail(GNOME_IS_HREF(href));
  g_return_if_fail(label != NULL);

  /* pattern used to set underline for string */
  pattern = g_strnfill(strlen(label), '_');
  gtk_label_set_text(GTK_LABEL(href->label), label);
  gtk_label_set_pattern(GTK_LABEL(href->label), pattern);
  g_free(pattern);
}

static void gnome_href_clicked(GtkButton *button) {
  GnomeHRef *href;

  g_return_if_fail(button != NULL);
  g_return_if_fail(GNOME_IS_HREF(button));

  if (GTK_BUTTON_CLASS(parent_class)->clicked)
    (* GTK_BUTTON_CLASS(parent_class)->clicked)(button);

  href = GNOME_HREF(button);

  g_return_if_fail(href->url);

  if(!gnome_url_show(href->url))
	  gnome_error_dialog(_("Error occured while trying to launch the "
			       "URL handler.\n"
			       "Please check the settings in the "
			       "Control Center if they are correct."));
}

static void gnome_href_destroy(GtkObject *object) {
  GnomeHRef *href;

  g_return_if_fail(object != NULL);
  g_return_if_fail(GNOME_IS_HREF(object));
  href = GNOME_HREF(object);
  if (href->url)
    g_free(href->url);
  if (parent_class->destroy)
    (* parent_class->destroy)(object);
}

static void gnome_href_realize(GtkWidget *widget) {
  GdkCursor *cursor;

  if (GTK_WIDGET_CLASS(parent_class)->realize)
    (* GTK_WIDGET_CLASS(parent_class)->realize)(widget);
  cursor = gnome_stock_cursor_new(GNOME_STOCK_CURSOR_POINTING_HAND);
  gdk_window_set_cursor(widget->window, cursor);
  gdk_cursor_destroy(cursor);
}

static void
gnome_href_set_arg (GtkObject *object,
		    GtkArg *arg,
		    guint arg_id)
{
	GnomeHRef *self;

	self = GNOME_HREF (object);

	switch (arg_id) {
	case ARG_URL:
		gnome_href_set_url(self, GTK_VALUE_POINTER(*arg));
		break;
	case ARG_LABEL:
		gnome_href_set_label(self, GTK_VALUE_POINTER(*arg));
		break;
	default:
		break;
	}
}

static void
gnome_href_get_arg (GtkObject *object,
		    GtkArg *arg,
		    guint arg_id)
{
	GnomeHRef *self;

	self = GNOME_HREF (object);

	switch (arg_id) {
	case ARG_URL:
		GTK_VALUE_POINTER(*arg) = gnome_href_get_url(self);
		break;
	case ARG_LABEL:
		GTK_VALUE_POINTER(*arg) = gnome_href_get_label(self);
		break;
	default:
		break;
	}
}
