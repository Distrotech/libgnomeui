/* GNOME font picker button.
 (C) David Abilleira Freijeiro 1998 <odaf@nexo.es>
 Based on gnome-color-picker by Federico Mena <federico@nuclecu.unam.mx>
 
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
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkvseparator.h>
#include <gtk/gtkfontsel.h>
#include <gdk_imlib.h>
#include <libgnomeui/gnome-pixmap.h>
#include <libgnomeui/gnome-stock.h>
#include "libgnomeui/gnome-window-icon.h"
#include "gnome-font-picker.h"
#include <libgnome/gnome-i18n.h>

#include <string.h>
#include <stdio.h>
#include <ctype.h>



#define DEF_FONT_NAME "-adobe-times-medium-r-normal-*-14-*-*-*-p-*-iso8859-1"
#define DEF_PREVIEW_TEXT "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz"

/* Signals */
enum {
	FONT_SET,
	LAST_SIGNAL
};

typedef void (* GnomeFontPickerSignal1) (GtkObject *object,
					  gchar    *font_name,
                                          gpointer data);

/* Prototipes */
static void gnome_font_picker_marshal_signal_1 (GtkObject     *object,
						 GtkSignalFunc  func,
						 gpointer       func_data,
						 GtkArg        *args);

static void gnome_font_picker_class_init (GnomeFontPickerClass *class);
static void gnome_font_picker_init       (GnomeFontPicker      *cp);
static void gnome_font_picker_destroy    (GtkObject             *object);

static void gnome_font_picker_clicked(GtkButton *button);

/* Dialog response functions */
static void gnome_font_picker_dialog_ok_clicked(GtkWidget *widget,
                                                  gpointer   data);
static void gnome_font_picker_dialog_cancel_clicked(GtkWidget *widget,
                                                      gpointer   data);

static gboolean gnome_font_picker_dialog_delete_event(GtkWidget *widget,GdkEventAny *ev,
                                                      gpointer   data);
void gnome_font_picker_dialog_destroy(GtkWidget *widget,
                                          gpointer   data);

/* Auxiliary functions */
GtkWidget *gnome_font_picker_create_inside(GnomeFontPicker *gfs);

void gnome_font_picker_font_extract_attr(gchar *font_name,
                                                  gchar *attr,
                                                  gint i);
void gnome_font_picker_font_set_attr(gchar **font_name,
                                              const gchar *attr,
                                              gint i);

void gnome_font_picker_label_use_font_in_label  (GnomeFontPicker *gfs);
void gnome_font_picker_update_font_info(GnomeFontPicker *gfs);




static guint font_picker_signals[LAST_SIGNAL] = { 0 };

static GtkButtonClass *parent_class;


GtkType
gnome_font_picker_get_type (void)
{
	static GtkType fp_type = 0;

	if (!fp_type) {
		GtkTypeInfo fp_info = {
			"GnomeFontPicker",
			sizeof (GnomeFontPicker),
			sizeof (GnomeFontPickerClass),
			(GtkClassInitFunc) gnome_font_picker_class_init,
			(GtkObjectInitFunc) gnome_font_picker_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		fp_type = gtk_type_unique (gtk_button_get_type (), &fp_info);
	}

	return fp_type;
}

static void
gnome_font_picker_class_init (GnomeFontPickerClass *class)
{
	GtkObjectClass *object_class;
	GtkButtonClass *button_class;

	object_class = (GtkObjectClass *) class;
	button_class = (GtkButtonClass *) class;

	parent_class = gtk_type_class (gtk_button_get_type ());

	font_picker_signals[FONT_SET] =
		gtk_signal_new ("font_set",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeFontPickerClass, font_set),
				gnome_font_picker_marshal_signal_1,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);

	gtk_object_class_add_signals (object_class, font_picker_signals, LAST_SIGNAL);

	object_class->destroy = gnome_font_picker_destroy;
	button_class->clicked = gnome_font_picker_clicked;

	class->font_set = NULL;
}

static void
gnome_font_picker_init (GnomeFontPicker *gfp)
{

    /* Initialize fields */
    gfp->mode          = GNOME_FONT_PICKER_MODE_PIXMAP;
    gfp->font_name     = NULL;
    gfp->preview_text  = NULL;
    gfp->use_font_in_label     = FALSE;
    gfp->use_font_in_label_size = 14;
    gfp->show_size     = TRUE;
    gfp->font_dialog   = NULL;
    gfp->title         = g_strdup(_("Pick a Font"));


    /* Create pixmap or info widgets */
    gfp->inside=gnome_font_picker_create_inside(gfp);
    if (gfp->inside)
        gtk_container_add(GTK_CONTAINER(gfp),gfp->inside);

    gnome_font_picker_set_font_name(gfp,DEF_FONT_NAME);
    gnome_font_picker_set_preview_text(gfp,DEF_PREVIEW_TEXT);

    if (gfp->mode==GNOME_FONT_PICKER_MODE_FONT_INFO) {
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

    gfp=GNOME_FONT_PICKER(object);
    
    if (gfp->font_dialog)
      {
        gtk_widget_destroy(gfp->font_dialog);
	gfp->font_dialog = NULL;
      }

    if (gfp->font_name)
      {
        g_free(gfp->font_name);
	gfp->font_name = NULL;
      }

    if (gfp->preview_text)
      {
        g_free(gfp->preview_text);
	gfp->preview_text = NULL;
      }

    if (gfp->title)
      {
        g_free(gfp->title);
	gfp->title = NULL;
      }

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
    
} /* gnome_font_picker_destroy */


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
 * Sets the title for the font selection dialog.
 */

void
gnome_font_picker_set_title (GnomeFontPicker *gfp, const gchar *title)
{
	g_return_if_fail (gfp != NULL);
	g_return_if_fail (GNOME_IS_FONT_PICKER (gfp));

	if (gfp->title)
		g_free (gfp->title);

        gfp->title = g_strdup (title);

        /* If FontDialog is created change title */
        if (gfp->font_dialog)
            gtk_window_set_title(GTK_WINDOW(gfp->font_dialog),gfp->title);
}


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

    return gfp->mode;
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
    g_return_if_fail (mode != GNOME_FONT_PICKER_MODE_UNKNOWN);

    if (gfp->mode!=mode)
    {
        gfp->mode=mode;
        /* Next sentence will destroy gfp->inside after removing it */
        gtk_container_remove(GTK_CONTAINER(gfp),gfp->inside);
        gfp->inside=gnome_font_picker_create_inside(gfp);
        if (gfp->inside)
            gtk_container_add(GTK_CONTAINER(gfp),gfp->inside);
        
        if (gfp->mode==GNOME_FONT_PICKER_MODE_FONT_INFO) {
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

    if (gfp->mode==GNOME_FONT_PICKER_MODE_FONT_INFO) {
        if (gfp->use_font_in_label!=use_font_in_label ||
	    gfp->use_font_in_label_size!=size) {

            gfp->use_font_in_label=use_font_in_label;
            gfp->use_font_in_label_size=size;

            if (!gfp->use_font_in_label)
                gtk_widget_restore_default_style(gfp->font_label);
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

    if (gfp->mode==GNOME_FONT_PICKER_MODE_FONT_INFO) {
        if (show_size!=gfp->show_size)
        {
            gfp->show_size=show_size;

            /* Next sentence will destroy gfp->inside after removing it */
            gtk_container_remove(GTK_CONTAINER(gfp),gfp->inside);
            gfp->inside=gnome_font_picker_create_inside(gfp);
            if (gfp->inside)
                gtk_container_add(GTK_CONTAINER(gfp),gfp->inside);
            
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

    if (gfp->mode==GNOME_FONT_PICKER_MODE_USER_WIDGET) {
        if (gfp->inside)
            gtk_container_remove(GTK_CONTAINER(gfp),gfp->inside);
        
        gfp->inside=widget;
        if (gfp->inside)
            gtk_container_add(GTK_CONTAINER(gfp),gfp->inside);
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

gchar*	   gnome_font_picker_get_font_name    (GnomeFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_FONT_PICKER (gfp), NULL);

    if (gfp->font_dialog) {
        if (gfp->font_name)
            g_free(gfp->font_name);
        gfp->font_name=g_strdup(gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(gfp->font_dialog)));
    }

    return gfp->font_name;
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
 * font dialog is not being displayed.
 */

GdkFont*   gnome_font_picker_get_font	       (GnomeFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_FONT_PICKER (gfp), NULL);
    
    return (gfp->font_dialog ?
             gtk_font_selection_dialog_get_font(GTK_FONT_SELECTION_DIALOG(gfp->font_dialog)) :
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
    gchar *tmp;
    
    g_return_val_if_fail (gfp != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_FONT_PICKER (gfp), FALSE);
    g_return_val_if_fail (fontname != NULL, FALSE);

    tmp=g_strdup(fontname);
    if (gfp->font_name) g_free(gfp->font_name);
    gfp->font_name= tmp;

    if (gfp->mode==GNOME_FONT_PICKER_MODE_FONT_INFO)
	gnome_font_picker_update_font_info(gfp);

    if (gfp->font_dialog)
        return gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(gfp->font_dialog),gfp->font_name);
    else
        return FALSE;
} /* gnome-font_selector_set_font_name */


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

gchar*	   gnome_font_picker_get_preview_text (GnomeFontPicker *gfp)
{
    g_return_val_if_fail (gfp != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_FONT_PICKER (gfp), NULL);

    if (gfp->font_dialog) {
        if (gfp->preview_text)
            g_free(gfp->preview_text);
        gfp->preview_text=g_strdup(gtk_font_selection_dialog_get_preview_text(GTK_FONT_SELECTION_DIALOG(gfp->font_dialog)));
    }

    return gfp->preview_text;
    
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
    gchar *tmp;
    
    g_return_if_fail (gfp != NULL);
    g_return_if_fail (GNOME_IS_FONT_PICKER (gfp));
    g_return_if_fail (text != NULL);

    tmp=g_strdup(text);
    if (gfp->preview_text) g_free(gfp->preview_text);
    gfp->preview_text=tmp;

    if (gfp->font_dialog)
        gtk_font_selection_dialog_set_preview_text(GTK_FONT_SELECTION_DIALOG(gfp->font_dialog),gfp->preview_text);

} /* gnome_font_picker_set_preview_text */

/* ************************************************************************
 Internal functions
 **************************************************************************/

static void
gnome_font_picker_marshal_signal_1 (GtkObject *object, GtkSignalFunc func, gpointer func_data, GtkArg *args)
{
	GnomeFontPickerSignal1 rfunc;

	rfunc = (GnomeFontPickerSignal1) func;
	(* rfunc) (object,
		   GTK_VALUE_POINTER (args[0]),
		   func_data);
} /* gnome_font_picker_marshal_signal_1 */

static void
gnome_font_picker_clicked(GtkButton *button)
{
    GnomeFontPicker      *gfp;
    GtkFontSelectionDialog *fsd;
    
    gfp = GNOME_FONT_PICKER(button);

    if (!gfp->font_dialog) {
        gfp->font_dialog=gtk_font_selection_dialog_new(gfp->title);	
        fsd=GTK_FONT_SELECTION_DIALOG(gfp->font_dialog);
	gnome_window_icon_set_from_default (GTK_WINDOW (fsd));
        /* If there is a grabed window, set new dialog as modal */
        if (gtk_grab_get_current())
            gtk_window_set_modal(GTK_WINDOW(gfp->font_dialog),TRUE);

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

    if (!GTK_WIDGET_VISIBLE(gfp->font_dialog)) {
        
        /* Set font and preview text */
        gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(gfp->font_dialog),
                                                gfp->font_name);
        gtk_font_selection_dialog_set_preview_text(GTK_FONT_SELECTION_DIALOG(gfp->font_dialog),
                                                   gfp->preview_text);
        
        gtk_widget_show(gfp->font_dialog);
    } else if (gfp->font_dialog->window) {
	/*raise the window so that if it is obscured that we see it*/
	gdk_window_raise(gfp->font_dialog->window);
    }/* if */
} /* gnome_font_picker_clicked */

static void
gnome_font_picker_dialog_ok_clicked(GtkWidget *widget,
				gpointer   data)
{
    GnomeFontPicker *gfp;

    gfp = GNOME_FONT_PICKER (data);

    gtk_widget_hide(gfp->font_dialog);
    
    /* These calls will update gfp->font_name and gfp->preview_text */
    gnome_font_picker_get_font_name(gfp);
    gnome_font_picker_get_preview_text(gfp);

    /* Set label font */
    if (gfp->mode==GNOME_FONT_PICKER_MODE_FONT_INFO)
        gnome_font_picker_update_font_info(gfp);

    /* Emit font_set signal */
    gtk_signal_emit(GTK_OBJECT(gfp),font_picker_signals[FONT_SET],gfp->font_name);
    
} /* gnome_font_picker_dialog_ok_clicked */


static void
gnome_font_picker_dialog_cancel_clicked(GtkWidget *widget,
				    gpointer   data)
{
    GnomeFontPicker *gfp;

    gfp = GNOME_FONT_PICKER (data);

    gtk_widget_hide(gfp->font_dialog);

    /* Restore old values */
    gnome_font_picker_set_font_name(gfp,gfp->font_name);
    gnome_font_picker_set_preview_text(gfp,gfp->preview_text);

} /* gnome_font_picker_dialog_cancel_clicked */

static gboolean
gnome_font_picker_dialog_delete_event(GtkWidget *widget,GdkEventAny *ev,
                                          gpointer   data)
{
    GnomeFontPicker *gfp;

    gfp = GNOME_FONT_PICKER (data);
    
    /* Here we restore old values */
    gnome_font_picker_set_font_name(gfp,gfp->font_name);
    gnome_font_picker_set_preview_text(gfp,gfp->preview_text);

    return FALSE;
} /* gnome_font_picker_dialog_delete_event */

void
gnome_font_picker_dialog_destroy(GtkWidget *widget,
                                          gpointer   data)
{
    GnomeFontPicker *gfp;

    gfp = GNOME_FONT_PICKER (data);
    
    /* Query GtkFontSelectionDialog before it get destroyed */
    /* These calls will update gfp->font_name and gfp->preview_text */
    gnome_font_picker_get_font_name(gfp);
    gnome_font_picker_get_preview_text(gfp);

    /* Dialog will get destroyed so reference is no valid now */
    gfp->font_dialog=NULL;
} /* gnome_font_picker_dialog_destroy */

GtkWidget *gnome_font_picker_create_inside(GnomeFontPicker *gfp)
{
    GtkWidget *widget;
    
    if (gfp->mode==GNOME_FONT_PICKER_MODE_PIXMAP) {
        widget=gnome_stock_new_with_icon(GNOME_STOCK_PIXMAP_FONT);
        gtk_widget_show(widget);
    }
    else if (gfp->mode==GNOME_FONT_PICKER_MODE_FONT_INFO) {
        widget=gtk_hbox_new(FALSE,0);

        gfp->font_label=gtk_label_new(_("Font"));
            
        gtk_label_set_justify(GTK_LABEL(gfp->font_label),GTK_JUSTIFY_LEFT);
        gtk_box_pack_start(GTK_BOX(widget),gfp->font_label,TRUE,TRUE,5);

        if (gfp->show_size)
        {
            gfp->vsep=gtk_vseparator_new();
            gtk_box_pack_start(GTK_BOX(widget),gfp->vsep,FALSE,FALSE,0);
            
            gfp->size_label=gtk_label_new("14");
            gtk_box_pack_start(GTK_BOX(widget),gfp->size_label,FALSE,FALSE,5);
        }
        
        gtk_widget_show_all(widget);



        
    } else widget=NULL; /* GNOME_FONT_PICKER_MODE_USER_WIDGET */
    
    return widget;
        
} /* gnome_font_picker_create_inside */

void gnome_font_picker_font_extract_attr(gchar *font_name,
                                                  gchar *attr,
                                                  gint i)
{
    gchar *pTmp;

    /* Search paramether */
    for (pTmp=font_name; i!=0; i--,pTmp++)
        pTmp=(gchar *)strchr(pTmp,'-');

    if (*pTmp!=0) {
        while (*pTmp!='-' && *pTmp!=0) {
            *attr=*pTmp;
            attr++; pTmp++;
        }
        *attr=0;
    }
    else strcpy(attr,"Unknown");

} /* gnome_font_picker_font_extract_attr */

void gnome_font_picker_font_set_attr(gchar **font_name,
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
    *font_name=g_strdup(pTgt);
    g_free(pTgt);
} /* gnome_font_picker_font_set_attr */

void gnome_font_picker_label_use_font_in_label  (GnomeFontPicker *gfp)
{
    GdkFont *font;
    GtkStyle *style;
    gchar *pStr,size[20];
    
    /* Change size */
    pStr=g_strdup(gfp->font_name);
    g_snprintf(size, sizeof(size), "%d",gfp->use_font_in_label_size);
    gnome_font_picker_font_set_attr(&pStr,size,7);


    /* Load font */
    font=gdk_font_load(pStr);
    if (!font) {
      g_free (pStr);
      return; /* Use widget default */
    }

    g_return_if_fail( font != NULL );

    /* Change label style */
    gtk_widget_ensure_style(gfp->font_label);
    style=gtk_style_copy(gfp->font_label->style);
    gdk_font_unref(style->font);
    style->font=font;
    gtk_widget_set_style(gfp->font_label,style);
    gtk_style_unref(style);

    g_free(pStr);    

} /* gnome_font_picker_set_label_font */

void gnome_font_picker_update_font_info(GnomeFontPicker *gfp)
{
    gchar *pTmp;

    pTmp=g_strdup(gfp->font_name);

    /* Extract font name */
    gnome_font_picker_font_extract_attr(gfp->font_name,pTmp,2);
    *pTmp=toupper(*pTmp);
    gtk_label_set_text(GTK_LABEL(gfp->font_label),pTmp);

    /* Extract font size */
    if (gfp->show_size)
    {
        gnome_font_picker_font_extract_attr(gfp->font_name,pTmp,7);
        gtk_label_set_text(GTK_LABEL(gfp->size_label),pTmp);
    }

    if (gfp->use_font_in_label)
        gnome_font_picker_label_use_font_in_label(gfp);
        
    g_free(pTmp);
    
} /* gnome_font_picker_update_font_info */
