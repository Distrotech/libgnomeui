/* gnome-href.c
 * Copyright (C) 1998-2001, James Henstridge <james@daa.com.au>
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
#include <libgnome/gnome-macros.h>

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include <string.h> /* for strlen */

#include <gtk/gtk.h>
#include <libgnome/gnome-url.h>
#include "gnome-href.h"

struct _GnomeHRefPrivate {
	gchar *url;
	GtkWidget *label;
};

static void gnome_href_clicked		(GtkButton *button);
static void gnome_href_destroy		(GtkObject *object);
static void gnome_href_finalize		(GObject *object);
static void gnome_href_get_property	(GObject *object,
					 guint param_id,
					 GValue *value,
					 GParamSpec * pspec);
static void gnome_href_set_property	(GObject *object,
					 guint param_id,
					 const GValue * value,
					 GParamSpec * pspec);
static void gnome_href_realize		(GtkWidget *widget);
static void drag_data_get    		(GnomeHRef          *href,
					 GdkDragContext     *context,
					 GtkSelectionData   *selection_data,
					 guint               info,
					 guint               time,
					 gpointer            data);

static const GtkTargetEntry http_drop_types[] = {
	{ "text/uri-list",       0, 0 },
	{ "x-url/http",          0, 0 },
	{ "_NETSCAPE_URL",       0, 0 }
};
static const GtkTargetEntry ftp_drop_types[] = {
	{ "text/uri-list",       0, 0 },
	{ "x-url/ftp",           0, 0 },
	{ "_NETSCAPE_URL",       0, 0 }
};
static const GtkTargetEntry other_drop_types[] = {
	{ "text/uri-list",       0, 0 },
	{ "_NETSCAPE_URL",       0, 0 }
};

static const gint n_http_drop_types = 
   sizeof(http_drop_types) / sizeof(http_drop_types[0]);
static const gint n_ftp_drop_types = 
   sizeof(ftp_drop_types) / sizeof(ftp_drop_types[0]);
static const gint n_other_drop_types = 
   sizeof(other_drop_types) / sizeof(other_drop_types[0]);


enum {
	PROP_0,
	PROP_URL,
	PROP_TEXT
};

/**
 * gnome_href_get_type
 *
 * Returns the type assigned to the GNOME href widget.
 **/
/* The following defines the get_type */
GNOME_CLASS_BOILERPLATE (GnomeHRef, gnome_href,
			 GtkButton, GTK_TYPE_BUTTON)

static void
gnome_href_class_init (GnomeHRefClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *)klass;
	GObjectClass *gobject_class = (GObjectClass *)klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;
	GtkButtonClass *button_class = (GtkButtonClass *)klass;
	
	object_class->destroy = gnome_href_destroy;
	
	gobject_class->finalize = gnome_href_finalize;
	gobject_class->set_property = gnome_href_set_property;
	gobject_class->get_property = gnome_href_get_property;

	widget_class->realize = gnome_href_realize;
	button_class->clicked = gnome_href_clicked;

	/* By default we link to The World Food Programme */
	g_object_class_install_property (gobject_class,
					 PROP_URL,
					 g_param_spec_string ("url",
							      _("URL"),
							      _("The URL that GnomeHRef activates"),
							      "http://www.wfp.org",
							      (G_PARAM_READABLE |
							       G_PARAM_WRITABLE)));
	g_object_class_install_property (gobject_class,
					 PROP_TEXT,
					 g_param_spec_string ("text",
							      _("Text"),
							      _("The text on the button"),
							      _("End World Hunger"),
							      (G_PARAM_READABLE |
							       G_PARAM_WRITABLE)));

	gtk_widget_class_install_style_property (GTK_WIDGET_CLASS (gobject_class),
						 g_param_spec_boxed ("link_colour",
								     _("Link colour"),
								     _("Colour ysed to draw the link"),
								     GDK_TYPE_COLOR,
								     G_PARAM_READABLE));
}

static void
gnome_href_instance_init (GnomeHRef *href)
{
        GdkColor *link_colour;
	GdkColor blue = { 0, 0x0000, 0x0000, 0xffff };

	href->_priv = g_new0(GnomeHRefPrivate, 1);

	href->_priv->label = gtk_label_new("");
	gtk_widget_ref(href->_priv->label);

	gtk_widget_style_get (GTK_WIDGET(href),
			      "link_colour", &link_colour,
			      NULL);
	if (!link_colour)
		link_colour = &blue;
	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_NORMAL, link_colour);
	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_ACTIVE, link_colour);
	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_PRELIGHT, link_colour);
	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_SELECTED, link_colour);

	gtk_button_set_relief(GTK_BUTTON(href), GTK_RELIEF_NONE);

	gtk_container_add(GTK_CONTAINER(href), href->_priv->label);
	gtk_widget_show(href->_priv->label);

	href->_priv->url = NULL;

	/* the source dest is set on set_url */
	gtk_signal_connect (GTK_OBJECT (href), "drag_data_get",
			    GTK_SIGNAL_FUNC (drag_data_get), NULL);
}

static void
drag_data_get(GnomeHRef          *href,
	      GdkDragContext     *context,
	      GtkSelectionData   *selection_data,
	      guint               info,
	      guint               time,
	      gpointer            data)
{
	g_return_if_fail (href != NULL);
	g_return_if_fail (GNOME_IS_HREF (href));

	if( ! href->_priv->url) {
		/*FIXME: cancel the drag*/
		return;
	}

	/* if this doesn't look like an url, it's probably a file */
	if(strchr(href->_priv->url, ':') == NULL) {
		char *s = g_strdup_printf("file:%s\r\n", href->_priv->url);
		gtk_selection_data_set (selection_data,
					selection_data->target,
					8, s, strlen(s)+1);
		g_free(s);
	} else {
		gtk_selection_data_set (selection_data,
					selection_data->target,
					8, href->_priv->url, strlen(href->_priv->url)+1);
	}
}

/**
 * gnome_href_new
 * @url: URL assigned to this object.
 * @text: Text associated with the URL.
 *
 * Description:
 * Created a GNOME href object, a label widget with a clickable action
 * and an associated URL.  If @text is set to %NULL, @url is used as
 * the text for the label.
 *
 * Returns:  Pointer to new GNOME href widget.
 **/

GtkWidget *
gnome_href_new(const gchar *url, const gchar *text)
{
  GnomeHRef *href;

  g_return_val_if_fail(url != NULL, NULL);

  href = g_object_new (GNOME_TYPE_HREF,
		       "url", url,
		       "text", text,
		       NULL);

  return GTK_WIDGET (href);
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

const gchar *gnome_href_get_url(GnomeHRef *href) {
  g_return_val_if_fail(href != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_HREF(href), NULL);
  return href->_priv->url;
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

  if (href->_priv->url) {
	  gtk_drag_source_unset(GTK_WIDGET(href));
	  g_free(href->_priv->url);
  }
  href->_priv->url = g_strdup(url);
  if(strncmp(url, "http://", 7) == 0 ||
     strncmp(url, "https://", 8) == 0) {
	  gtk_drag_source_set (GTK_WIDGET(href),
			       GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			       http_drop_types, n_http_drop_types,
			       GDK_ACTION_COPY);
  } else if(strncmp(url, "ftp://", 6) == 0) {
	  gtk_drag_source_set (GTK_WIDGET(href),
			       GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			       ftp_drop_types, n_ftp_drop_types,
			       GDK_ACTION_COPY);
  } else {
	  gtk_drag_source_set (GTK_WIDGET(href),
			       GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			       other_drop_types, n_other_drop_types,
			       GDK_ACTION_COPY);
  }
}


/**
 * gnome_href_get_text
 * @href: Pointer to GnomeHRef widget
 *
 * Description:
 * Returns the contents of the label widget used to display the link text.
 * Note that the string should not be freed as it points to internal memory.
 *
 * Returns:  Pointer to text contained in the label widget.
 **/

const gchar *gnome_href_get_text(GnomeHRef *href) {
  gchar *ret;

  g_return_val_if_fail(href != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_HREF(href), NULL);

  gtk_label_get(GTK_LABEL(href->_priv->label), &ret);
  return ret;
}


/**
 * gnome_href_set_text
 * @href: Pointer to GnomeHRef widget
 * @text: New link text for the href object.
 *
 * Description:
 * Sets the internal label widget text (used to display a URL's link
 * text) to the value given in @label.
 **/
void
gnome_href_set_text (GnomeHRef *href, const gchar *text)
{
  gchar *markup;

  g_return_if_fail(href != NULL);
  g_return_if_fail(GNOME_IS_HREF(href));
  g_return_if_fail(text != NULL);

  /* underline the string */
  markup = g_strdup_printf("<u>%s</u>", text);
  gtk_label_set_markup(GTK_LABEL(href->_priv->label), text);
  g_free(markup);
}

#ifndef GNOME_DISABLE_DEPRECATED_SOURCE

/**
 * gnome_href_get_label
 * @href: Pointer to GnomeHRef widget
 *
 * Description:
 * deprecated, use #gnome_href_get_text
 **/

const gchar *
gnome_href_get_label(GnomeHRef *href)
{
	g_warning("gnome_href_get_label is deprecated, use gnome_href_get_text");
	return gnome_href_get_text(href);
}


/**
 * gnome_href_set_label
 * @href: Pointer to GnomeHRef widget
 * @label: New link text for the href object.
 *
 * Description:
 * deprecated, use #gnome_href_set_text
 **/

void
gnome_href_set_label (GnomeHRef *href, const gchar *label)
{
	g_return_if_fail(href != NULL);
	g_return_if_fail(GNOME_IS_HREF(href));

	g_warning("gnome_href_set_label is deprecated, use gnome_href_set_text");
	gnome_href_set_text(href, label);
}

#endif /* not GNOME_DISABLE_DEPRECATED_SOURCE */

static void
gnome_href_clicked (GtkButton *button)
{
  GnomeHRef *href;

  g_return_if_fail(button != NULL);
  g_return_if_fail(GNOME_IS_HREF(button));

  GNOME_CALL_PARENT (GTK_BUTTON_CLASS, clicked, (button));

  href = GNOME_HREF(button);

  g_return_if_fail(href->_priv->url != NULL);

  /* FIXME: Use the error variable from gnome_url_show */
  if(!gnome_url_show(href->_priv->url, NULL)) {
      GtkWidget *dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("Error occured while trying to launch the "
						   "URL handler.\n"
						   "Please check the settings in the "
						   "Control Center if they are correct."));

      gtk_dialog_run(GTK_DIALOG(dialog));
  }
}

static void
gnome_href_destroy (GtkObject *object)
{
	GnomeHRef *href;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_HREF(object));

	href = GNOME_HREF (object);

	if (href->_priv->label != NULL) {
		gtk_widget_unref (href->_priv->label);
		href->_priv->label = NULL;
	}

	GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_href_finalize (GObject *object)
{
	GnomeHRef *href;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_HREF(object));

	href = GNOME_HREF (object);

	g_free (href->_priv->url);
	href->_priv->url = NULL;

	g_free (href->_priv);
	href->_priv = NULL;

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
gnome_href_realize(GtkWidget *widget)
{
	GdkCursor *cursor;

	GNOME_CALL_PARENT (GTK_WIDGET_CLASS, realize, (widget));

	cursor = gdk_cursor_new(GDK_HAND2);
	gdk_window_set_cursor(widget->window, cursor);
	gdk_cursor_destroy(cursor);
}

static void
gnome_href_set_property (GObject *object,
			 guint param_id,
			 const GValue * value,
			 GParamSpec * pspec)
{
	GnomeHRef *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_HREF (object));

	self = GNOME_HREF (object);

	switch (param_id) {
	case PROP_URL:
		gnome_href_set_url(self, g_value_get_string (value));
		break;
	case PROP_TEXT:
		gnome_href_set_text(self, g_value_get_string (value));
		break;
	default:
		break;
	}
}

static void
gnome_href_get_property (GObject *object,
		      guint param_id,
		      GValue *value,
		      GParamSpec * pspec)
{
	GnomeHRef *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_HREF (object));

	self = GNOME_HREF (object);

	switch (param_id) {
	case PROP_URL:
		g_value_set_string (value,
				    gnome_href_get_url(self));
		break;
	case PROP_TEXT:
		g_value_set_string (value,
				    gnome_href_get_text(self));
		break;
	default:
		break;
	}
}
