/* GNOME font picker button.
 * Copyright (C) 1998 David Abilleira Freijeiro <odaf@nexo.es>
 * All rights reserved.
 *
 * Based on gnome-color-picker by Federico Mena <federico@nuclecu.unam.mx>
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

#include <config.h>
#include "gnome-macros.h"

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkvseparator.h>
#include <gtk/gtkfontsel.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkstock.h>
#include <libgnomeui/gnome-stock-icons.h>
#include "gnome-font-picker.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>

struct _GnomeFontPickerPrivate {
        gchar         *title;
        
        gchar         *font_name;
        gchar         *preview_text;

        GnomeFontPickerMode mode : 2;

        /* Only for GNOME_FONT_PICKER_MODE_FONT_INFO */
        gboolean      use_font_in_label : 1;
        gboolean      use_font_in_label_size : 1;
        gboolean      show_size : 1;

	GtkWidget     *font_dialog;
        GtkWidget     *inside;
        GtkWidget     *font_label, *vsep, *size_label;
};


#define DEF_FONT_NAME N_("-adobe-times-medium-r-normal-*-14-*-*-*-p-*-*-*")
#define DEF_PREVIEW_TEXT N_("AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz")
#define DEF_TITLE N_("Pick a Font")

/* Signals */
enum {
	FONT_SET,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_TITLE,
	PROP_MODE,
	PROP_FONT_NAME,
	PROP_FONT,
	PROP_PREVIEW_TEXT
};

/* Prototypes */
static void gnome_font_picker_class_init (GnomeFontPickerClass *class);
static void gnome_font_picker_init       (GnomeFontPicker      *cp);
static void gnome_font_picker_destroy    (GtkObject            *object);
static void gnome_font_picker_finalize   (GObject              *object);
static void gnome_font_picker_get_property (GObject *object,
					  guint param_id,
					  GValue *value,
					  GParamSpec * pspec);
static void gnome_font_picker_set_property (GObject *object,
					  guint param_id,
					  const GValue * value,
					  GParamSpec * pspec);

static void gnome_font_picker_clicked(GtkButton *button);

/* Dialog response functions */
static void gnome_font_picker_dialog_ok_clicked(GtkWidget *widget,
                                                  gpointer   data);
static void gnome_font_picker_dialog_cancel_clicked(GtkWidget *widget,
                                                      gpointer   data);

static gboolean gnome_font_picker_dialog_delete_event(GtkWidget *widget,
						      GdkEventAny *ev,
                                                      gpointer   data);
static void gnome_font_picker_dialog_destroy(GtkWidget *widget,
					     gpointer   data);

/* Auxiliary functions */
static GtkWidget *gnome_font_picker_create_inside(GnomeFontPicker *gfs);

static char * gnome_font_picker_font_extract_attr(const gchar *font_name,
						  gint i);
static void gnome_font_picker_font_set_attr(gchar **font_name,
                                              const gchar *attr,
                                              gint i);

static void gnome_font_picker_label_use_font_in_label  (GnomeFontPicker *gfs);
static void gnome_font_picker_update_font_info(GnomeFontPicker *gfs);




static guint font_picker_signals[LAST_SIGNAL] = { 0 };

GNOME_CLASS_BOILERPLATE (GnomeFontPicker, gnome_font_picker,
			 GtkButton, gtk_button)

static void
gnome_font_picker_class_init (GnomeFontPickerClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkButtonClass *button_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;
	button_class = (GtkButtonClass *) class;

	/* By default we link to The World Food Programme */
	g_object_class_install_property
		(gobject_class,
		 PROP_TITLE,
		 g_param_spec_string ("title",
				      _("Title"),
				      _("The title of the selection dialog box"),
				      _(DEF_TITLE),
				      (G_PARAM_READABLE |
				       G_PARAM_WRITABLE)));
	g_object_class_install_property
		(gobject_class,
		 PROP_MODE,
		 g_param_spec_enum ("mode",
				    _("Mode"),
				    _("The mode of operation of the font picker"),
				    G_TYPE_ENUM /* FIXME: should have its own type -George */
				    GNOME_FONT_PICKER_MODE_PIXMAP
				    (G_PARAM_READABLE |
				     G_PARAM_WRITABLE)));
	g_object_class_install_property
		(gobject_class,
		 PROP_FONT_NAME,
		 g_param_spec_string ("font_name",
				      _("Font name"),
				      _("Name of the selected font"),
				      _(DEF_FONT_NAME),
				      (G_PARAM_READABLE |
				       G_PARAM_WRITABLE)));
	g_object_class_install_property
		(gobject_class,
		 PROP_FONT,
		 g_param_spec_pointer ("font",
				       _("Font"),
				       _("The selected GtkFont"),
				       G_PARAM_READABLE));
	g_object_class_install_property
		(gobject_class,
		 PROP_PREVIEW_TEXT,
		 g_param_spec_string ("preview_text",
				      _("Preview text"),
				      _("Preview text shown in the dialog"),
				      _(DEF_PREVIEW_TEXT),
				      (G_PARAM_READABLE |
				       G_PARAM_WRITABLE)));

	font_picker_signals[FONT_SET] =
		gtk_signal_new ("font_set",
				GTK_RUN_FIRST,
				GTK_CLASS_TYPE (object_class),
				GTK_SIGNAL_OFFSET (GnomeFontPickerClass, font_set),
				gtk_marshal_VOID__STRING,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);

	object_class->destroy = gnome_font_picker_destroy;

	gobject_class->finalize = gnome_font_picker_finalize;
	gobject_class->set_property = gnome_font_picker_set_property;
	gobject_class->get_property = gnome_font_picker_get_property;

	button_class->clicked = gnome_font_picker_clicked;

	class->font_set = NULL;
}

static void
gnome_font_picker_init (GnomeFontPicker *gfp)
{

    gfp->_priv                         = g_new0(GnomeFontPickerPrivate, 1);

    /* Initialize fields */
    gfp->_priv->mode                   = GNOME_FONT_PICKER_MODE_PIXMAP;
    gfp->_priv->font_name              = NULL;
    gfp->_priv->preview_text           = NULL;
    gfp->_priv->use_font_in_label      = FALSE;
    gfp->_priv->use_font_in_label_size = 14;
    gfp->_priv->show_size              = TRUE;
    gfp->_priv->font_dialog            = NULL;
    gfp->_priv->title                  = g_strdup(_(DEF_TITLE));


    /* Create pixmap or info widgets */
    gfp->_priv->inside = gnome_font_picker_create_inside(gfp);
    if (gfp->_priv->inside)
        gtk_container_add(GTK_CONTAINER(gfp), gfp->_priv->inside);

    gnome_font_picker_set_font_name(gfp, _(DEF_FONT_NAME));
    gnome_font_picker_set_preview_text(gfp, _(DEF_PREVIEW_TEXT));

    if (gfp->_priv->mode == GNOME_FONT_PICKER_MODE_FONT_INFO) {
        /* Update */
        gnome_font_picker_update_font_info(gfp);
    }
}

static void
gnome_font_picker_destroy (GtkObject *object)
{
    GnomeFontPicker *gfp;
    
    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_FONT_PICKER (object));

    /* remember, destroy can be run multiple times! */

    gfp = GNOME_FONT_PICKER(object);
    
    if (gfp->_priv->font_dialog)
      {
        gtk_widget_destroy(gfp->_priv->font_dialog);
	gfp->_priv->font_dialog = NULL;
      }

    /* g_free handles NULL */
    g_free(gfp->_priv->font_name);
    gfp->_priv->font_name = NULL;

    /* g_free handles NULL */
    g_free(gfp->_priv->preview_text);
    gfp->_priv->preview_text = NULL;

    /* g_free handles NULL */
    g_free(gfp->_priv->title);
    gfp->_priv->title = NULL;

    GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
    
} /* gnome_font_picker_destroy */

static void
gnome_font_picker_finalize (GObject *object)
{
    GnomeFontPicker *gfp;
    
    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_FONT_PICKER (object));

    gfp = GNOME_FONT_PICKER(object);
    
    /* g_free handles NULL */
    g_free(gfp->_priv);
    gfp->_priv = NULL;

    GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
    
} /* gnome_font_picker_finalize */

static void
gnome_font_picker_set_property (GObject *object,
				guint param_id,
				const GValue * value,
				GParamSpec * pspec)
{
	GnomeFontPicker *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_FONT_PICKER (object));

	self = GNOME_FONT_PICKER (object);

	switch (param_id) {
	case PARAM_TITLE:
		gnome_font_picker_set_title (self, g_value_get_string (value));
		break;
	case PARAM_MODE:
		gnome_font_picker_set_mode (self, g_value_get_enum (value));
		break;
	case PARAM_FONT_NAME:
		gnome_font_picker_set_font_name (self,
						 g_value_get_string (value));
		break;
	case PARAM_PREVIEW_TEXT:
		gnome_font_picker_set_preview_text
			(self, g_value_get_string (value));
		break;

	default:
		break;
	}
} /* gnome_font_picker_set_property */

static void
gnome_font_picker_get_property (GObject *object,
				guint param_id,
				GValue *value,
				GParamSpec * pspec)
{
	GnomeFontPicker *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_FONT_PICKER (object));

	self = GNOME_FONT_PICKER (object);

	switch (param_id) {
	case PARAM_TITLE:
		g_value_set_string (value, gnome_font_picker_get_title (self));
		break;
	case PARAM_MODE:
		g_value_set_enum (value, gnome_font_picker_get_title (self));
		break;
	case PARAM_FONT_NAME:
		g_value_set_string (value,
				    gnome_font_picker_get_font_name (self));
		break;
	case PARAM_FONT:
		g_value_set_pointer (value, gnome_font_picker_get_font (self));
		break;
	case PARAM_PREVIEW_TEXT:
		g_value_set_string (value,
				    gnome_font_picker_get_preview_text (self));
		break;
	default:
		break;
	}
} /* gnome_font_picker_get_property */

/*************************************************************************
 Public functions
 *************************************************************************/


/**
 * gnome_font_picker_new
 *
 * Description:
 * Create new font picker widget.
 *
 * Returns:
 * Pointer to new font picker widget.
 */

GtkWidget *
gnome_font_picker_new (void)
{
	return GTK_WIDGET (gtk_type_new (gnome_font_picker_get_type ()));
} /* gnome_font_picker_new */



/**
 * gnome_font_picker_set_title
 * @gfp: Pointer to GNOME font picker widget.
 * @title: String containing font selection dialog title.
 *
 * Description:
 * Sets the title for the font selection dialog.  If @title is %NULL,
 * then the default is used.
 */

void
gnome_font_picker_set_title (GnomeFontPicker *gfp, const gchar *title)
{
	g_return_if_fail (gfp != NULL);
	g_return_if_fail (GNOME_IS_FONT_PICKER (gfp));

	if ( ! title)
		title = _(DEF_TITLE);

	/* g_free handles NULL */
	g_free (gfp->_priv->title);
        gfp->_priv->title = g_strdup (title);

        /* If FontDialog is created change title */
        if (gfp->_priv->font_dialog)
            gtk_window_set_title(GTK_WINDOW(gfp->_priv->font_dialog),
				 gfp->_priv->title);
} /* gnome_font_picker_set_title */

/**
 * gnome_font_picker_get_title
 * @gfp: Pointer to GNOME font picker widget.
 *
 * Description:
 * Retrieve name of the font selection dialog title
 *
 * Returns:
 * Pointer to an internal copy of the title string
 */

const gchar*
gnome_font_picker_get_title(GnomeFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_FONT_PICKER (gfp), NULL);

    return gfp->_priv->title;
} /* gnome_font_picker_get_title */

/**
 * gnome_font_picker_get_mode
 * @gfp: Pointer to GNOME font picker widget.
 *
 * Description:
 * Returns current font picker button mode (or what to show).  Possible
 * values include %GNOME_FONT_PICKER_MODE_PIXMAP,
 * %GNOME_FONT_PICKER_MODE_FONT_INFO, and 
 * %GNOME_FONT_PICKER_MODE_USER_WIDGET.
 *
 * Returns:
 * Button mode currently set in font picker widget, or 
 * %GNOME_FONT_PICKER_MODE_UNKNOWN on error.
 */

GnomeFontPickerMode
gnome_font_picker_get_mode        (GnomeFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, GNOME_FONT_PICKER_MODE_UNKNOWN);
    g_return_val_if_fail (GNOME_IS_FONT_PICKER (gfp),
			  GNOME_FONT_PICKER_MODE_UNKNOWN);

    return gfp->_priv->mode;
} /* gnome_font_picker_get_mode */


/**
 * gnome_font_picker_set_mode
 * @gfp: Pointer to GNOME font picker widget.
 * @mode: Value of subsequent font picker button mode (or what to show)
 *
 * Description:
 * Set value of subsequent font picker button mode (or what to show).
 */

void       gnome_font_picker_set_mode        (GnomeFontPicker *gfp,
                                              GnomeFontPickerMode mode)
{
    g_return_if_fail (gfp != NULL);
    g_return_if_fail (GNOME_IS_FONT_PICKER (gfp));
    g_return_if_fail (mode >= 0 && mode < GNOME_FONT_PICKER_MODE_UNKNOWN);

    if (gfp->_priv->mode != mode) {

        gfp->_priv->mode = mode;

        /* Next sentence will destroy gfp->_priv->inside after removing it */
        gtk_container_remove(GTK_CONTAINER(gfp), gfp->_priv->inside);
        gfp->_priv->inside = gnome_font_picker_create_inside(gfp);
        if (gfp->_priv->inside)
            gtk_container_add(GTK_CONTAINER(gfp), gfp->_priv->inside);
        
        if (gfp->_priv->mode == GNOME_FONT_PICKER_MODE_FONT_INFO) {
            /* Update */
            gnome_font_picker_update_font_info(gfp);
        }
    }
} /* gnome_font_picker_set_mode */


/**
 * gnome_font_picker_fi_set_use_font_in_label
 * @gfp: Pointer to GNOME font picker widget.
 * @use_font_in_label: If %TRUE, font name will be written using font chosen.
 * @size: Display font using this point size.
 *
 * Description:
 * If @use_font_in_label is %TRUE, font name will be written using font chosen
 * by user and using @size passed to this function.  This only applies if
 * current button mode is %GNOME_FONT_PICKER_MODE_FONT_INFO.
 */

void  gnome_font_picker_fi_set_use_font_in_label (GnomeFontPicker *gfp,
                                                  gboolean use_font_in_label,
                                                  gint size)
{
    g_return_if_fail (gfp != NULL);
    g_return_if_fail (GNOME_IS_FONT_PICKER (gfp));

    if (gfp->_priv->mode==GNOME_FONT_PICKER_MODE_FONT_INFO) {
        if (gfp->_priv->use_font_in_label!=use_font_in_label ||
	    gfp->_priv->use_font_in_label_size!=size) {

            gfp->_priv->use_font_in_label=use_font_in_label;
            gfp->_priv->use_font_in_label_size=size;

            if (!gfp->_priv->use_font_in_label)
                gtk_widget_restore_default_style(gfp->_priv->font_label);
            else
                gnome_font_picker_label_use_font_in_label(gfp);
        }
    }

} /* gnome_font_picker_fi_set_use_font_in_label */


/**
 * gnome_font_picker_fi_set_show_size
 * @gfp: Pointer to GNOME font picker widget.
 * @show_size: %TRUE if font size should be displayed in dialog.
 *
 * Description:
 * If @show_size is %TRUE, font size will be displayed along with font chosen
 * by user.  This only applies if current button mode is
 * %GNOME_FONT_PICKER_MODE_FONT_INFO.
 */

void       gnome_font_picker_fi_set_show_size (GnomeFontPicker *gfp,
                                               gboolean show_size)
{
    g_return_if_fail (gfp != NULL);
    g_return_if_fail (GNOME_IS_FONT_PICKER (gfp));

    if (gfp->_priv->mode==GNOME_FONT_PICKER_MODE_FONT_INFO) {
        if (show_size!=gfp->_priv->show_size)
        {
            gfp->_priv->show_size=show_size;

            /* Next sentence will destroy gfp->_priv->inside after removing it */
            gtk_container_remove(GTK_CONTAINER(gfp),gfp->_priv->inside);
            gfp->_priv->inside=gnome_font_picker_create_inside(gfp);
            if (gfp->_priv->inside)
                gtk_container_add(GTK_CONTAINER(gfp),gfp->_priv->inside);
            
            gnome_font_picker_update_font_info(gfp);
            
        } /* if (show_size... */
    }
} /* gnome_font_picker_fi_set_show_size */


/**
 * gnome_font_picker_uw_set_widget
 * @gfp: Pointer to GNOME font picker widget.
 * @widget: User widget to display for inside of font picker.
 *
 * Set the user-supplied @widget as the inside of the font picker.
 * This only applies with GNOME_FONT_PICKER_MODE_USER_WIDGET.
 */

void       gnome_font_picker_uw_set_widget    (GnomeFontPicker *gfp,
                                                        GtkWidget *widget)
{
    g_return_if_fail (gfp != NULL);
    g_return_if_fail (GNOME_IS_FONT_PICKER (gfp));

    if (gfp->_priv->mode==GNOME_FONT_PICKER_MODE_USER_WIDGET) {
        if (gfp->_priv->inside)
            gtk_container_remove(GTK_CONTAINER(gfp),gfp->_priv->inside);
        
        gfp->_priv->inside=widget;
        if (gfp->_priv->inside)
            gtk_container_add(GTK_CONTAINER(gfp),gfp->_priv->inside);
    }

} /* gnome_font_picker_uw_set_widget */

/**
 * gnome_font_picker_uw_get_widget
 * @gfp: Pointer to GNOME font picker widget.
 *
 * Get the user-supplied widget inside of the font picker.
 * This only applies with GNOME_FONT_PICKER_MODE_USER_WIDGET.
 *
 * Returns: a #GtkWidget, or %NULL if
 * not in GNOME_FONT_PICKER_MODE_USER_WIDGET mode
 */

GtkWidget *
gnome_font_picker_uw_get_widget (GnomeFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL);
    g_return_val_if_fail (GNOME_IS_FONT_PICKER (gfp));

    if (gfp->_priv->mode == GNOME_FONT_PICKER_MODE_USER_WIDGET) {
	    return gfp->_priv->inside;
    } else {
	    return NULL;
    }
} /* gnome_font_picker_uw_set_widget */

/**
 * gnome_font_picker_get_font_name
 * @gfp: Pointer to GNOME font picker widget.
 *
 * Description:
 * Retrieve name of font from font selection dialog.
 *
 * Returns:
 * Pointer to an internal copy of the font name.
 */

const gchar*	   gnome_font_picker_get_font_name    (GnomeFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_FONT_PICKER (gfp), NULL);

    if (gfp->_priv->font_dialog) {
	/* g_free handles NULL */
	g_free(gfp->_priv->font_name);
        gfp->_priv->font_name = g_strdup(gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog)));
    }

    return gfp->_priv->font_name;
} /* gnome_font_picker_get_font_name */


/**
 * gnome_font_picker_get_font
 * @gfp: Pointer to GNOME font picker widget.
 *
 * Description:
 * Retrieve font info from font selection dialog.
 *
 * Returns:
 * Return value of gtk_font_selection_dialog_get_font, or %NULL if
 * font dialog is not being displayed.  The value is not a copy but
 * an internal value and should not be modified.
 */

GdkFont*   gnome_font_picker_get_font	       (GnomeFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_FONT_PICKER (gfp), NULL);

    /* FIXME: no GtkFont when there is no dialog? huh?
     *  -George */
    
    return (gfp->_priv->font_dialog ?
             gtk_font_selection_dialog_get_font(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog)) :
             NULL);
} /* gnome_font_picker_get_font */


/**
 * gnome_font_picker_set_font_name
 * @gfp: Pointer to GNOME font picker widget.
 * @fontname: Name of font to display in font selection dialog
 *
 * Description:
 * Set or update currently-displayed font in font picker dialog.
 *
 * Returns:
 * Return value of gtk_font_selection_dialog_set_font_name if the
 * font selection dialog exists, otherwise %FALSE.
 */

gboolean   gnome_font_picker_set_font_name    (GnomeFontPicker *gfp,
					       const gchar       *fontname)
{
    g_return_val_if_fail (gfp != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_FONT_PICKER (gfp), FALSE);
    g_return_val_if_fail (fontname != NULL, FALSE);

    /* g_free handles NULL */
    g_free(gfp->_priv->font_name);
    gfp->_priv->font_name = g_strdup(fontname);
    
    if (gfp->_priv->mode == GNOME_FONT_PICKER_MODE_FONT_INFO)
	gnome_font_picker_update_font_info(gfp);

    if (gfp->_priv->font_dialog)
        return gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog), gfp->_priv->font_name);
    else
        return FALSE;
} /* gnome_font_picker_set_font_name */


/**
 * gnome_font_picker_get_preview_text
 * @gfp: Pointer to GNOME font picker widget.
 *
 * Description:
 * Retrieve preview text from font selection dialog if available.
 *
 * Returns:
 * Reference to internal copy of preview text string, or %NULL if no
 * font dialog is being displayed.
 */

const gchar*	   gnome_font_picker_get_preview_text (GnomeFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_FONT_PICKER (gfp), NULL);

    if (gfp->_priv->font_dialog) {
        /* g_free handles NULL */
        g_free(gfp->_priv->preview_text);
        gfp->_priv->preview_text = g_strdup(gtk_font_selection_dialog_get_preview_text(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog)));
    }

    return gfp->_priv->preview_text;
    
} /* gnome_font_picker_get_preview_text */


/**
 * gnome_font_picker_set_preview_text
 * @gfp: Pointer to GNOME font picker widget.
 * @text: New preview text
 *
 * Description:
 * Set preview text in font picker, and in font selection dialog if one
 * is being displayed.
 */

void	   gnome_font_picker_set_preview_text (GnomeFontPicker *gfp,
                                               const gchar     *text)
{
    g_return_if_fail (gfp != NULL);
    g_return_if_fail (GNOME_IS_FONT_PICKER (gfp));
    g_return_if_fail (text != NULL);

    /* g_free handles NULL */
    g_free(gfp->_priv->preview_text);
    gfp->_priv->preview_text = g_strdup(text);

    if (gfp->_priv->font_dialog)
        gtk_font_selection_dialog_set_preview_text(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog), gfp->_priv->preview_text);

} /* gnome_font_picker_set_preview_text */

/* ************************************************************************
 Internal functions
 **************************************************************************/

static void
gnome_font_picker_clicked(GtkButton *button)
{
    GnomeFontPicker      *gfp;
    GtkFontSelectionDialog *fsd;
    
    gfp = GNOME_FONT_PICKER(button);

    if (!gfp->_priv->font_dialog) {
        GtkWidget *parent;

        parent = gtk_widget_get_toplevel(GTK_WIDGET(gfp));
      
        gfp->_priv->font_dialog=gtk_font_selection_dialog_new(gfp->_priv->title);

        if (parent)
            gtk_window_set_transient_for(GTK_WINDOW(gfp->_priv->font_dialog),
                                         GTK_WINDOW(parent));
        
        fsd=GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog);

        /* If there is a grabed window, set new dialog as modal */
        if (gtk_grab_get_current())
            gtk_window_set_modal(GTK_WINDOW(gfp->_priv->font_dialog),TRUE);

        gtk_signal_connect(GTK_OBJECT(fsd->ok_button), "clicked",
                           (GtkSignalFunc) gnome_font_picker_dialog_ok_clicked,
                           gfp);

        gtk_signal_connect(GTK_OBJECT(fsd->cancel_button), "clicked",
                           (GtkSignalFunc) gnome_font_picker_dialog_cancel_clicked,
                           gfp);
        gtk_signal_connect(GTK_OBJECT(fsd),"delete_event",
                           (GtkSignalFunc) gnome_font_picker_dialog_delete_event,
                           gfp);
        gtk_signal_connect(GTK_OBJECT(fsd),"destroy",
                           (GtkSignalFunc) gnome_font_picker_dialog_destroy,
                           gfp);
                           
    } /* if */

    if (!GTK_WIDGET_VISIBLE(gfp->_priv->font_dialog)) {
        
        /* Set font and preview text */
        gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog),
                                                gfp->_priv->font_name);
        gtk_font_selection_dialog_set_preview_text(GTK_FONT_SELECTION_DIALOG(gfp->_priv->font_dialog),
                                                   gfp->_priv->preview_text);
        
        gtk_widget_show(gfp->_priv->font_dialog);
    } else if (gfp->_priv->font_dialog->window) {
	/*raise the window so that if it is obscured that we see it*/
	gdk_window_raise(gfp->_priv->font_dialog->window);
    }/* if */
} /* gnome_font_picker_clicked */

static void
gnome_font_picker_dialog_ok_clicked(GtkWidget *widget,
				    gpointer   data)
{
    GnomeFontPicker *gfp;

    gfp = GNOME_FONT_PICKER (data);

    gtk_widget_hide(gfp->_priv->font_dialog);
    
    /* These calls will update gfp->_priv->font_name and gfp->_priv->preview_text */
    gnome_font_picker_get_font_name(gfp);
    gnome_font_picker_get_preview_text(gfp);

    /* Set label font */
    if (gfp->_priv->mode == GNOME_FONT_PICKER_MODE_FONT_INFO)
        gnome_font_picker_update_font_info(gfp);

    /* Emit font_set signal */
    gtk_signal_emit(GTK_OBJECT(gfp),
		    font_picker_signals[FONT_SET],
		    gfp->_priv->font_name);
    
} /* gnome_font_picker_dialog_ok_clicked */


static void
gnome_font_picker_dialog_cancel_clicked(GtkWidget *widget,
					gpointer   data)
{
    GnomeFontPicker *gfp;

    gfp = GNOME_FONT_PICKER (data);

    gtk_widget_hide(gfp->_priv->font_dialog);

    /* Restore old values */
    gnome_font_picker_set_font_name(gfp,gfp->_priv->font_name);
    gnome_font_picker_set_preview_text(gfp,gfp->_priv->preview_text);

} /* gnome_font_picker_dialog_cancel_clicked */

static gboolean
gnome_font_picker_dialog_delete_event(GtkWidget *widget, GdkEventAny *ev,
				      gpointer   data)
{
    GnomeFontPicker *gfp;

    gfp = GNOME_FONT_PICKER (data);
    
    /* Here we restore old values */
    gnome_font_picker_set_font_name(gfp,gfp->_priv->font_name);
    gnome_font_picker_set_preview_text(gfp,gfp->_priv->preview_text);

    return FALSE;
} /* gnome_font_picker_dialog_delete_event */

static void
gnome_font_picker_dialog_destroy(GtkWidget *widget,
				 gpointer   data)
{
    GnomeFontPicker *gfp;

    gfp = GNOME_FONT_PICKER (data);
    
    /* Query GtkFontSelectionDialog before it get destroyed */
    /* These calls will update gfp->_priv->font_name and gfp->_priv->preview_text */
    gnome_font_picker_get_font_name(gfp);
    gnome_font_picker_get_preview_text(gfp);

    /* Dialog will get destroyed so reference is no valid now */
    gfp->_priv->font_dialog = NULL;
} /* gnome_font_picker_dialog_destroy */

GtkWidget *gnome_font_picker_create_inside(GnomeFontPicker *gfp)
{
    GtkWidget *widget;
    
    if (gfp->_priv->mode==GNOME_FONT_PICKER_MODE_PIXMAP) {
        widget=gtk_image_new_from_stock(GTK_STOCK_SELECT_FONT,
					GTK_ICON_SIZE_BUTTON);
        gtk_widget_show(widget);
    } else if (gfp->_priv->mode==GNOME_FONT_PICKER_MODE_FONT_INFO) {
        widget=gtk_hbox_new(FALSE,0);

        gfp->_priv->font_label=gtk_label_new(_("Font"));
            
        gtk_label_set_justify(GTK_LABEL(gfp->_priv->font_label),GTK_JUSTIFY_LEFT);
        gtk_box_pack_start(GTK_BOX(widget),gfp->_priv->font_label,TRUE,TRUE,5);

        if (gfp->_priv->show_size) {
            gfp->_priv->vsep=gtk_vseparator_new();
            gtk_box_pack_start(GTK_BOX(widget),gfp->_priv->vsep,FALSE,FALSE,0);
            
            gfp->_priv->size_label=gtk_label_new("14");
            gtk_box_pack_start(GTK_BOX(widget),gfp->_priv->size_label,FALSE,FALSE,5);
        }

        gtk_widget_show_all(widget);

    } else /* GNOME_FONT_PICKER_MODE_USER_WIDGET */ {
        widget=NULL;
    }
    
    return widget;
        
} /* gnome_font_picker_create_inside */

static char *
gnome_font_picker_font_extract_attr (const gchar *font_name, gint i)
{
	const char *pTmp;
	char *cstr;
	GString *str = g_string_new (NULL);

	/* Search paramether */
	for (pTmp = font_name; *pTmp && i > 0; i--, pTmp++)
		pTmp = (gchar *)strchr (pTmp, '-');

	if (*pTmp != '\0') {
		while (*pTmp != '-' && *pTmp != '\0') {
			g_string_append_c (str, *pTmp);
			pTmp++;
		}
	} else {
		/* note: do not use strncpy, it sucks */
		g_string_assign (str, _("Unknown"));
	}

	cstr = str->str;
	g_string_free (str, FALSE);
	return cstr;
} /* gnome_font_picker_font_extract_attr */

static void gnome_font_picker_font_set_attr(gchar **font_name,
					    const gchar *attr,
					    gint i)
{
    gchar *pTgt;
    gchar *pTmpSrc,*pTmpTgt;


    pTgt=g_malloc(strlen(*font_name)+strlen(attr)+1);
    
    /* Search paramether */
    for (pTmpSrc=*font_name; i!=0; i--,pTmpSrc++)
        pTmpSrc=(gchar *)strchr(pTmpSrc,'-');

    /* Copy until attrib */
    memcpy(pTgt,*font_name,pTmpSrc-*font_name);
    
    /* Copy attrib */
    pTmpTgt=pTgt+(pTmpSrc-*font_name);
    memcpy(pTmpTgt,attr,strlen(attr));
    
    /* Copy until end */
    pTmpSrc=(gchar *)strchr(pTmpSrc,'-');
    pTmpTgt+=strlen(attr);
    strcpy(pTmpTgt,pTmpSrc);

    /* Save result */
    g_free(*font_name);
    *font_name = g_strdup(pTgt);
    g_free(pTgt);
} /* gnome_font_picker_font_set_attr */

static void gnome_font_picker_label_use_font_in_label  (GnomeFontPicker *gfp)
{
    GdkFont *font;
    GtkStyle *style;
    gchar *pStr,size[20];
    
    /* Change size */
    pStr = g_strdup(gfp->_priv->font_name);
    g_snprintf(size, sizeof(size), "%d",gfp->_priv->use_font_in_label_size);
    gnome_font_picker_font_set_attr(&pStr,size,7);


    /* Load font */
    font=gdk_font_load(pStr);
    if (!font) {
      g_free (pStr);
      return; /* Use widget default */
    }

    g_return_if_fail( font != NULL );

    /* Change label style */
    gtk_widget_ensure_style(gfp->_priv->font_label);
    style=gtk_style_copy(gfp->_priv->font_label->style);
    gdk_font_unref(style->font);
    style->font=font;
    gtk_widget_set_style(gfp->_priv->font_label,style);
    gtk_style_unref(style);

    g_free(pStr);    

} /* gnome_font_picker_set_label_font */

static void gnome_font_picker_update_font_info(GnomeFontPicker *gfp)
{
	gchar *attr;
	const char *font_name;

	font_name = gfp->_priv->font_name;

	/* Extract font name */
	attr = gnome_font_picker_font_extract_attr (font_name, 2);

	/* Locale safe toupper */
	if (*attr >= 'a' && *attr <= 'z')
		*attr += 'A' - 'a';

	gtk_label_set_text (GTK_LABEL (gfp->_priv->font_label), attr);

	g_free (attr);

	/* Extract font size */
	if (gfp->_priv->show_size) {
		attr = gnome_font_picker_font_extract_attr (font_name, 7);

		gtk_label_set_text (GTK_LABEL (gfp->_priv->size_label), attr);

		g_free (attr);
	}

	if (gfp->_priv->use_font_in_label)
		gnome_font_picker_label_use_font_in_label (gfp);

} /* gnome_font_picker_update_font_info */
