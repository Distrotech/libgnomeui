/* gnome-href.c
 * Copyright (C) 1998, James Henstridge <james@daa.com.au>
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

#include <string.h> /* for strlen */

#include <gtk/gtklabel.h>
#include <libgnome/gnome-url.h>
#include "gnome-href.h"

static void gnome_href_class_init(GnomeHRefClass *klass);
static void gnome_href_init(GnomeHRef *href);
static void gnome_href_clicked(GtkButton *button);
static void gnome_href_destroy(GtkObject *object);
static void gnome_href_realize(GtkWidget *widget);

static GtkObjectClass *parent_class;


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
  object_class->destroy = gnome_href_destroy;
  widget_class->realize = gnome_href_realize;
  button_class->clicked = gnome_href_clicked;
}

static void gnome_href_init(GnomeHRef *self) {
  self->label = gtk_label_new("");
  gtk_button_set_relief(GTK_BUTTON(self), GTK_RELIEF_NONE);
  gtk_container_add(GTK_CONTAINER(self), self->label);
  gtk_widget_show(self->label);
  self->url = NULL;
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
  GnomeHRef *self;

  g_return_val_if_fail(url != NULL, NULL);

  self = gtk_type_new(gnome_href_get_type());
  gnome_href_set_url(self, url);

  if (!label)
    label = url;

  gnome_href_set_label(self, label);

  return GTK_WIDGET(self);
}


/**
 * gnome_href_get_url
 * @self: Pointer to GnomeHRef widget
 *
 * Description:
 * Returns the pointer to the URL associated with the @self href object.
 *
 * Returns:  Pointer to URL string, or %NULL if failure.
 **/

gchar *gnome_href_get_url(GnomeHRef *self) {
  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_HREF(self), NULL);
  return self->url;
}


/**
 * gnome_href_set_url
 * @self: Pointer to GnomeHRef widget
 * @url: String containing the URL to be stored within @self.
 *
 * Description:
 * Sets the internal URL value within @self to the value of @url.
 **/

void gnome_href_set_url(GnomeHRef *self, const gchar *url) {
  g_return_if_fail(self != NULL);
  g_return_if_fail(GNOME_IS_HREF(self));
  g_return_if_fail(url != NULL);

  if (self->url)
    g_free(self->url);
  self->url = g_strdup(url);
}


/**
 * gnome_href_get_label
 * @self: Pointer to GnomeHRef widget
 *
 * Description:
 * Returns the contents of the label widget used to display the link
 * text.
 *
 * Returns:  Pointer to text contained in the label widget.
 **/

gchar *gnome_href_get_label(GnomeHRef *self) {
  gchar *ret;

  g_return_val_if_fail(self != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_HREF(self), NULL);

  gtk_label_get(GTK_LABEL(self->label), &ret);
  return ret;
}


/**
 * gnome_href_set_label
 * @self: Pointer to GnomeHRef widget
 * @label: New link text for the href object.
 *
 * Description:
 * Sets the internal label widget text (used to display a URL's link
 * text) to the value given in @label.
 **/

void gnome_href_set_label(GnomeHRef *self, const gchar *label) {
  gchar *pattern;

  g_return_if_fail(self != NULL);
  g_return_if_fail(GNOME_IS_HREF(self));
  g_return_if_fail(label != NULL);

  /* pattern used to set underline for string */
  pattern = g_strnfill(strlen(label), '_');
  gtk_label_set_text(GTK_LABEL(self->label), label);
  gtk_label_set_pattern(GTK_LABEL(self->label), pattern);
  g_free(pattern);
}

static void gnome_href_clicked(GtkButton *button) {
  GnomeHRef *self;

  g_return_if_fail(button != NULL);
  g_return_if_fail(GNOME_IS_HREF(button));

  if (GTK_BUTTON_CLASS(parent_class)->clicked)
    (* GTK_BUTTON_CLASS(parent_class)->clicked)(button);
  self = GNOME_HREF(button);
  g_return_if_fail(self->url);

  gnome_url_show(self->url);
}

static void gnome_href_destroy(GtkObject *object) {
  GnomeHRef *self;

  g_return_if_fail(object != NULL);
  g_return_if_fail(GNOME_IS_HREF(object));
  self = GNOME_HREF(object);
  if (self->url)
    g_free(self->url);
  if (parent_class->destroy)
    (* parent_class->destroy)(object);
}

static void gnome_href_realize(GtkWidget *widget) {
  GdkCursor *cursor;

  if (GTK_WIDGET_CLASS(parent_class)->realize)
    (* GTK_WIDGET_CLASS(parent_class)->realize)(widget);
  cursor = gdk_cursor_new(GDK_HAND2);
  gdk_window_set_cursor(widget->window, cursor);
  gdk_cursor_destroy(cursor);
}
