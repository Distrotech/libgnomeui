/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-about.c - An about box widget for gnome.

   Copyright (C) 2001 CodeFactory AB
   Copyright (C) 2001, 2002 Anders Carlsson

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Anders Carlsson <andersca@gnu.org>
*/

#include <config.h>
#include "gnome-i18nP.h"

#include "gnome-about.h"

#include <gtk/gtkdialog.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktextview.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkvseparator.h>
#include <libgnome/gnome-macros.h>


/* FIXME: More includes! */
#define _(x) (x)

struct _GnomeAboutPrivate {
	gchar *name;
	gchar *version;
	gchar *copyright;
	gchar *comments;
	gchar *translator_credits;
	
	GSList *authors;
	GSList *documenters;
	
	GtkWidget *logo_image;
	GtkWidget *name_label;
	GtkWidget *comments_label;
	GtkWidget *copyright_label;

	GtkTextBuffer *text_buffer;
};

enum {
	PROP_0,
	PROP_NAME,
	PROP_VERSION,
	PROP_COPYRIGHT,
	PROP_COMMENTS,
	PROP_AUTHORS,
	PROP_DOCUMENTERS,
	PROP_TRANSLATOR_CREDITS,
	PROP_LOGO,
};

static void gnome_about_finalize (GObject *object);
static void gnome_about_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void gnome_about_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

GNOME_CLASS_BOILERPLATE (GnomeAbout, gnome_about,
			 GtkDialog, GTK_TYPE_DIALOG)

static void
gnome_about_update_text_buffer (GnomeAbout *about)
{
	GtkTextIter iter, start_iter, end_iter;
	char *text;
	GSList *tmp;

	gtk_text_buffer_get_bounds (about->_priv->text_buffer, &start_iter, &end_iter);
	gtk_text_buffer_delete (about->_priv->text_buffer, &start_iter, &end_iter);
	
	gtk_text_buffer_get_iter_at_offset (about->_priv->text_buffer, &iter, 0);

	/* Authors */
	text = g_slist_length (about->_priv->authors) == 1 ? _("Author") : _("Authors");

	gtk_text_buffer_insert_with_tags_by_name (about->_priv->text_buffer, &iter,
						  text, -1,
						  "heading", NULL);
	
	gtk_text_buffer_insert (about->_priv->text_buffer, &iter,
				"\n", -1);

	for (tmp = about->_priv->authors; tmp; tmp = tmp->next) {
		text = tmp->data;

		gtk_text_buffer_insert (about->_priv->text_buffer, &iter,
					text, -1);

		gtk_text_buffer_insert (about->_priv->text_buffer, &iter,
				"\n", -1);
	}

	gtk_text_buffer_insert (about->_priv->text_buffer, &iter,
				"\n", -1);

	/* Documenters */
	if (about->_priv->documenters != NULL) {
		text = g_slist_length (about->_priv->documenters) == 1 ? _("Documenter") : _("Documenters");

		gtk_text_buffer_insert_with_tags_by_name (about->_priv->text_buffer, &iter,
							  text, -1,
							  "heading", NULL);
		
		gtk_text_buffer_insert (about->_priv->text_buffer, &iter,
					"\n", -1);
		
		for (tmp = about->_priv->documenters; tmp; tmp = tmp->next) {
			text = tmp->data;
			
			gtk_text_buffer_insert (about->_priv->text_buffer, &iter,
						text, -1);
			
			gtk_text_buffer_insert (about->_priv->text_buffer, &iter,
						"\n", -1);
		}
		
		gtk_text_buffer_insert (about->_priv->text_buffer, &iter,
					"\n", -1);


	}

	/* Translator credits */
	if (about->_priv->translator_credits != NULL) {
		gtk_text_buffer_insert_with_tags_by_name (about->_priv->text_buffer, &iter,
							  _("Translator credits"), -1,
							  "heading", NULL);
		
		gtk_text_buffer_insert (about->_priv->text_buffer, &iter,
					"\n", -1);
		gtk_text_buffer_insert_with_tags_by_name (about->_priv->text_buffer, &iter,
							  about->_priv->translator_credits, -1,
							  "wrapped", NULL);
	}
}


static void
gnome_about_instance_init (GnomeAbout *about)
{
	GtkWidget *vbox, *hbox, *separator;
	GtkWidget *scrolled_window, *text_view;
	
	about->_priv = g_new0 (GnomeAboutPrivate, 1);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about)->vbox), hbox, TRUE, TRUE, 0);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	separator = gtk_vseparator_new ();
	gtk_box_pack_start (GTK_BOX (hbox), separator, FALSE, FALSE, 0);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	gtk_box_pack_start (GTK_BOX (hbox), scrolled_window, TRUE, TRUE, 8);

	about->_priv->text_buffer = gtk_text_buffer_new (NULL);

	gtk_text_buffer_create_tag (about->_priv->text_buffer, "heading",
				    "weight", PANGO_WEIGHT_BOLD,
				    NULL);

	gtk_text_buffer_create_tag (about->_priv->text_buffer, "wrapped",
				    "wrap_mode", GTK_WRAP_WORD,
				    NULL);

	text_view = gtk_text_view_new_with_buffer (about->_priv->text_buffer);
			      
	gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
	gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);

	gtk_widget_modify_base (text_view, GTK_STATE_NORMAL,
				&GTK_WIDGET (about)->style->bg[GTK_STATE_NORMAL]);

	about->_priv->logo_image = gtk_image_new ();
	gtk_box_pack_start (GTK_BOX (vbox), about->_priv->logo_image, FALSE, FALSE, 0);
	about->_priv->name_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox), about->_priv->name_label, FALSE, FALSE, 0);

	about->_priv->comments_label = gtk_label_new (NULL);
	gtk_label_set_line_wrap (GTK_LABEL (about->_priv->comments_label), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), about->_priv->comments_label, FALSE, FALSE, 0);

	about->_priv->copyright_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox), about->_priv->copyright_label, FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);

	/* Add the OK button */
	gtk_dialog_add_button (GTK_DIALOG (about), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (about), GTK_RESPONSE_OK);
	g_signal_connect (about, "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	
	gtk_window_set_resizable (GTK_WINDOW (about), FALSE);
}

static void
gnome_about_class_init (GnomeAboutClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;
	
	object_class = (GObjectClass *)klass;
	widget_class = (GtkWidgetClass *)klass;
	
	object_class->set_property = gnome_about_set_property;
	object_class->get_property = gnome_about_get_property;

	object_class->finalize = gnome_about_finalize;
	
	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      _("Program name"),
							      _("The name of the program"),
							      NULL,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_VERSION,
					 g_param_spec_string ("version",
							      _("Program version"),
							      _("The version of the program"),
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_COPYRIGHT,
					 g_param_spec_string ("copyright",
							      _("Copyright string"),
							      _("Copyright information for the program"),
							      NULL,
							      G_PARAM_READWRITE));
	
	g_object_class_install_property (object_class,
					 PROP_COMMENTS,
					 g_param_spec_string ("comments",
							      _("Comments string"),
							      _("Comments about the program"),
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_AUTHORS,
					 g_param_spec_value_array ("authors",
								   _("Authors"),
								   _("List of authors of the programs"),
								   g_param_spec_string ("author-entry",
											_("Author entry"),
											_("A single author entry"),
											NULL,
											G_PARAM_READWRITE),
								   G_PARAM_WRITABLE));
	g_object_class_install_property (object_class,
					 PROP_DOCUMENTERS,
					 g_param_spec_value_array ("documenters",
								   _("Documenters"),
								   _("List of people documenting the program"),
								   g_param_spec_string ("documenter-entry",
											_("Documenter entry"),
											_("A single documenter entry"),
											NULL,
											G_PARAM_READWRITE),
								   G_PARAM_WRITABLE));

	g_object_class_install_property (object_class,
					 PROP_TRANSLATOR_CREDITS,
					 g_param_spec_string ("translator_credits",
							      _("Translator credits"),
							      _("Credits to the translators. This string should be marked as translatable"),
							      NULL,
							      G_PARAM_READWRITE));
	
	g_object_class_install_property (object_class,
					 PROP_LOGO,
					 g_param_spec_object ("logo",
							      _("Logo"),
							      _("A logo for the about box"),
							      GDK_TYPE_PIXBUF,
							      G_PARAM_WRITABLE));

}

static void
gnome_about_set_comments (GnomeAbout *about, const gchar *comments)
{
	g_free (about->_priv->comments);
	about->_priv->comments = comments ? g_strdup (comments) : NULL;

	gtk_label_set_text (GTK_LABEL (about->_priv->comments_label), about->_priv->comments);
}

static void
gnome_about_set_translator_credits (GnomeAbout *about, const gchar *translator_credits)
{
	g_free (about->_priv->translator_credits);

	about->_priv->translator_credits = g_strdup (translator_credits);

	gnome_about_update_text_buffer (about);
}

static void
gnome_about_set_copyright (GnomeAbout *about, const gchar *copyright)
{
	g_free (about->_priv->copyright);
	about->_priv->copyright = copyright ? g_strdup (copyright) : NULL;

	gtk_label_set_text (GTK_LABEL (about->_priv->copyright_label), about->_priv->copyright);
}

static void
gnome_about_set_version (GnomeAbout *about, const gchar *version)
{
	gchar *name_string;
	
	g_free (about->_priv->version);
	about->_priv->version = version ? g_strdup (version) : NULL;

	if (about->_priv->version != NULL) {
		name_string = g_strdup_printf ("<span size=\"xx-large\" weight=\"bold\">%s %s</span>", about->_priv->name, about->_priv->version);
	}
	else {
		name_string = g_strdup_printf ("<span size=\"xx-large\" weight=\"bold\">%s</span>", about->_priv->name);
	}

	gtk_label_set_markup (GTK_LABEL (about->_priv->name_label), name_string);
	g_free (name_string);

}

static void
gnome_about_set_name (GnomeAbout *about, const gchar *name)
{
	gchar *title_string;
	gchar *name_string;
	
	g_free (about->_priv->name);
	about->_priv->name = g_strdup (name ? name : "");

	title_string = g_strdup_printf (_("About %s"), name);
	gtk_window_set_title (GTK_WINDOW (about), title_string);
	g_free (title_string);

	if (about->_priv->version != NULL) {
		name_string = g_strdup_printf ("<span size=\"xx-large\" weight=\"bold\">%s %s</span>", about->_priv->name, about->_priv->version);
	}
	else {
		name_string = g_strdup_printf ("<span size=\"xx-large\" weight=\"bold\">%s</span>", about->_priv->name);
	}

	gtk_label_set_markup (GTK_LABEL (about->_priv->name_label), name_string);
	g_free (name_string);
}

static void
gnome_about_free_person_list (GSList *list)
{
	if (list == NULL)
		return;
	
	g_slist_foreach (list, g_free, NULL);
	g_slist_free (list);
}

static void
gnome_about_finalize (GObject *object)
{
	GnomeAbout *about = GNOME_ABOUT (object);

	g_free (about->_priv->name);
	g_free (about->_priv->version);
	g_free (about->_priv->copyright);
	g_free (about->_priv->comments);

	gnome_about_free_person_list (about->_priv->authors);
	gnome_about_free_person_list (about->_priv->documenters);

	g_free (about->_priv->translator_credits);

	g_free (about->_priv);
	about->_priv = NULL;

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
gnome_about_set_persons (GnomeAbout *about, guint prop_id, const GValue *persons)
{
		GValueArray *value_array;
	gint i;
	GSList *list;

	/* Free the old list */
	switch (prop_id) {
	case PROP_AUTHORS:
		list = about->_priv->authors;
		break;
	case PROP_DOCUMENTERS:
		list = about->_priv->documenters;
		break;
	default:
		g_assert_not_reached ();
		list = NULL; /* silence warning */
	}

	gnome_about_free_person_list (list);
	list = NULL;
	
	value_array = g_value_get_boxed (persons);

	if (value_array == NULL) {
		return;
	}
	
	for (i = 0; i < value_array->n_values; i++) {
		list = g_slist_prepend (list, g_value_dup_string (&value_array->values[i]));
	}

	list = g_slist_reverse (list);
	
	switch (prop_id) {
	case PROP_AUTHORS:
		about->_priv->authors = list;
		break;
	case PROP_DOCUMENTERS:
		about->_priv->documenters = list;
		break;
	default:
		g_assert_not_reached ();
	}

	gnome_about_update_text_buffer (about);
}

static void
gnome_about_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_NAME:
		gnome_about_set_name (GNOME_ABOUT (object), g_value_get_string (value));
		break;
	case PROP_VERSION:
		gnome_about_set_version (GNOME_ABOUT (object), g_value_get_string (value));
		break;
	case PROP_COMMENTS:
		gnome_about_set_comments (GNOME_ABOUT (object), g_value_get_string (value));
		break;
	case PROP_COPYRIGHT:
		gnome_about_set_copyright (GNOME_ABOUT (object), g_value_get_string (value));
		break;
	case PROP_LOGO:
		gtk_image_set_from_pixbuf (GTK_IMAGE (GNOME_ABOUT (object)->_priv->logo_image), g_value_get_object (value));
		break;
	case PROP_AUTHORS:
	case PROP_DOCUMENTERS:
		gnome_about_set_persons (GNOME_ABOUT (object), prop_id, value);
		break;
	case PROP_TRANSLATOR_CREDITS:
		gnome_about_set_translator_credits (GNOME_ABOUT (object), g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gnome_about_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

GtkWidget *
gnome_about_new (const gchar  *name,
		 const gchar  *version,
		 const gchar  *copyright,
		 const gchar  *comments,
		 const gchar **authors,
		 const gchar **documenters,
		 const gchar  *translator_credits,
		 GdkPixbuf    *logo_pixbuf)
{
	GnomeAbout *about;
	
	g_return_val_if_fail (authors != NULL, NULL);
	
	about = g_object_new (GNOME_TYPE_ABOUT, NULL);
        gnome_about_construct(about, 
			      name, version, copyright, 
			      comments, authors, documenters, 
			      translator_credits, logo_pixbuf);

	return GTK_WIDGET(about);
}

void
gnome_about_construct (GnomeAbout *about,
		       const gchar  *name,
		       const gchar  *version,
		       const gchar  *copyright,
		       const gchar  *comments,
		       const gchar **authors,
		       const gchar **documenters,
		       const gchar  *translator_credits,
		       GdkPixbuf    *logo_pixbuf)
{
	GValueArray *authors_array;
	GValueArray *documenters_array;
	gint i;

	authors_array = g_value_array_new (0);
	
	for (i = 0; authors[i] != NULL; i++) {
		GValue value = {0, };
		
		g_value_init (&value, G_TYPE_STRING);
			g_value_set_static_string (&value, authors[i]);
			authors_array = g_value_array_append (authors_array, &value);
	}

	if (documenters != NULL) {
		documenters_array = g_value_array_new (0);

		for (i = 0; documenters[i] != NULL; i++) {
			GValue value = {0, };
			
			g_value_init (&value, G_TYPE_STRING);
			g_value_set_static_string (&value, documenters[i]);
			documenters_array = g_value_array_append (documenters_array, &value);
		}

	}
	else {
		documenters_array = NULL;
	}

	g_object_set (G_OBJECT (about),
		      "name", name,
		      "version", version,
		      "copyright", copyright,
		      "comments", comments,
		      "authors", authors_array,
		      "documenters", documenters_array,
		      "translator_credits", translator_credits,
		      "logo", logo_pixbuf,
		      NULL);

	if (authors_array != NULL) {
		g_value_array_free (authors_array);
	}
	if (documenters_array != NULL) {
		g_value_array_free (documenters_array);
	}
}

