/* gnome-less.c:
 * Copyright (C) 1998 Free Software Foundation
 * All rights reserved.
 *
 * Written by: Havoc Pennington
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

#include <config.h>
#include "gnome-macros.h"

#include "gnome-less.h"


#include <libgnome/gnome-util.h>

#include <libgnome/gnome-i18n.h>

#include <libgnomeui/gnome-uidefs.h>

#include <unistd.h>
#include <time.h>
#include <string.h> /* memset */
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct _GnomeLessPrivate {
	GtkTextTag *text_tag;

	PangoFontDescription *font_desc;

	int columns;
};

static void gnome_less_class_init (GnomeLessClass *klass);
static void gnome_less_init       (GnomeLess      *messagebox);

static void gnome_less_destroy    (GtkObject      *object);
static void gnome_less_finalize   (GObject        *object);

GNOME_CLASS_BOILERPLATE (GnomeLess, gnome_less,
			 GtkHBox, gtk_hbox)

static void
gnome_less_class_init (GnomeLessClass *klass)
{
  GtkObjectClass *object_class;
  GObjectClass *gobject_class;

  object_class = (GtkObjectClass*) klass;
  gobject_class = (GObjectClass*) klass;

  object_class->destroy = gnome_less_destroy;
  gobject_class->finalize = gnome_less_finalize;
}

static void
gnome_less_init (GnomeLess *gl)
{
	GtkWidget * vscroll;
	GtkTextIter start, end;

	gtk_box_set_homogeneous(GTK_BOX(gl), FALSE);
	gtk_box_set_spacing(GTK_BOX(gl), 0);

	gl->_priv = g_new0(GnomeLessPrivate, 1);

	gl->_priv->columns = -1;

	gl->_priv->font_desc = pango_font_description_copy(GTK_WIDGET(gl)->style->font_desc);

	gl->text_buffer = GTK_TEXT_BUFFER(gtk_text_buffer_new(NULL));
	gtk_object_ref(GTK_OBJECT(gl->text_buffer));
	gl->text_view = GTK_TEXT_VIEW(gtk_text_view_new_with_buffer(gl->text_buffer));
	gtk_object_set(GTK_OBJECT(gl->text_view),
		       "editable", FALSE,
		       NULL);
	gtk_widget_ref(GTK_WIDGET(gl->text_view));
	gtk_widget_show(GTK_WIDGET(gl->text_view));

	/* FIXME: this is because gtktextview is broke! */
	gtk_widget_set_scroll_adjustments(GTK_WIDGET(gl->text_view), NULL, NULL);

	gl->_priv->text_tag = gtk_text_buffer_create_tag(gl->text_buffer, "text_tag", "font_desc", gl->_priv->font_desc, NULL);
	gtk_text_buffer_get_bounds(gl->text_buffer, &start, &end);
	gtk_text_buffer_apply_tag_by_name(gl->text_buffer, "text_tag", &start, &end);

	vscroll = gtk_vscrollbar_new(gl->text_view->vadjustment);
	gtk_widget_show(vscroll);

	gtk_box_pack_start(GTK_BOX(gl), GTK_WIDGET(gl->text_view),
			   TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gl), vscroll, FALSE, FALSE, 0);

	/* Since horizontal scroll doesn't work, use this hack. */
	gtk_widget_set_usize(GTK_WIDGET(gl->text_view), 300, -1); 
	gtk_text_view_set_wrap_mode(gl->text_view, GTK_WRAP_WORD);
}

/**
 * gnome_less_new
 *
 * Description:  Creates a new #GnomeLess widget.
 * 
 * Returns: #GtkWidget pointer to a new #GnomeLess widget
 **/

GtkWidget* gnome_less_new (void)
{
  GnomeLess * gl;
  
  gl = gtk_type_new(gnome_less_get_type());

  return GTK_WIDGET (gl);
}

/**
 * gnome_less_new_fixed_font:
 * @columns: the number of visible columns
 *
 * Description:  Creates a new #GnomeLess widget with the fixed font
 * and with the specified number of columns visible.
 * 
 * Returns: #GtkWidget pointer to a new #GnomeLess widget
 **/

GtkWidget *
gnome_less_new_fixed_font(int columns)
{
	GnomeLess * gl;

	gl = gtk_type_new(gnome_less_get_type());

	gnome_less_set_font_fixed(gl);
	gnome_less_set_width_columns(gl, columns);

	return GTK_WIDGET (gl);
}

static void gnome_less_destroy (GtkObject *object)
{
	GnomeLess *gl;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail(object != NULL);
	g_return_if_fail(GNOME_IS_LESS(object));

	gl = GNOME_LESS(object);

	if(gl->text_buffer)
		gtk_object_unref(GTK_OBJECT(gl->text_buffer));
	gl->text_buffer = NULL;

	if(gl->text_view)
		gtk_widget_unref(GTK_WIDGET(gl->text_view));
	gl->text_view = NULL;

	if(gl->_priv->font_desc)
		pango_font_description_free(gl->_priv->font_desc);
	gl->_priv->font_desc = NULL;

	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void gnome_less_finalize (GObject *object)
{
	GnomeLess *gl;

	g_return_if_fail(object != NULL);
	g_return_if_fail(GNOME_IS_LESS(object));

	gl = GNOME_LESS(object);

	g_free(gl->_priv);
	gl->_priv = NULL;

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}

/**
 * gnome_less_append_file
 * @gl: Pointer to GnomeLess widget
 * @path: Pathname of file to be displayed
 *
 * Displays a file in a GnomeLess widget.  Appending to any text currently in the widget
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/
gboolean
gnome_less_append_file(GnomeLess * gl, const gchar * path)
{
  FILE * f;

  g_return_val_if_fail(gl != NULL, FALSE);
  g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);
  g_return_val_if_fail(path != NULL, FALSE);

  if ( ! g_file_test(path, G_FILE_TEST_EXISTS) ) {
    /* Leave errno from the stat in g_file_test */
    return FALSE;
  }

  f = fopen(path, "rt");

  if ( f == NULL ) {
    /* Leave errno */
    return FALSE;
  }

  if ( ! gnome_less_append_filestream(gl, f) ) {
    /* Starting to feel like exceptions would be nice. */
    int save_errno = errno;
    fclose(f); /* nothing to do if it fails */
    errno = save_errno;  /* Want to report the root cause of the error */
    return FALSE;
  }

  if (fclose(f) != 0) {
    return FALSE;
  }
  else return TRUE;
}

/**
 * gnome_less_show_file
 * @gl: Pointer to GnomeLess widget
 * @path: Pathname of file to be displayed
 *
 * Displays a file in a GnomeLess widget. Replaces any text already being displayed
 * in the widget.
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/
gboolean
gnome_less_show_file(GnomeLess * gl, const gchar * path)
{
	g_return_val_if_fail(gl != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);
	g_return_val_if_fail(path != NULL, FALSE);

	gnome_less_clear(gl);
	return gnome_less_append_file(gl, path);
}

/**
 * gnome_less_append_command
 * @gl: Pointer to GnomeLess widget
 * @command_line: Command to be executed
 *
 * Runs the shell command specified in @command_line, and places the output of that command
 * in the GnomeLess widget specified by @gl.  Appends to any text currently in the widget.
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/
gboolean
gnome_less_append_command(GnomeLess * gl,
			  const gchar * command_line)
{
  FILE * p;

  g_return_val_if_fail(gl != NULL, FALSE);
  g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);
  g_return_val_if_fail(command_line != NULL, FALSE);
  
  p = popen(command_line, "r");

  if ( p == NULL ) {
    return FALSE;
  }

  if ( ! gnome_less_append_filestream(gl, p) ) {
    int save_errno = errno;
    pclose(p); /* nothing to do if it fails */
    errno = save_errno; /* Report the root cause of the error */
    return FALSE;
  }

  if ( pclose(p) == -1 ) {
    return FALSE;
  }
  else return TRUE;
}

/**
 * gnome_less_show_command
 * @gl: Pointer to GnomeLess widget
 * @command_line: Command to be executed
 *
 * Runs the shell command specified in @command_line, and places the output of that command
 * in the GnomeLess widget specified by @gl. Replaces any text already being displayed in the
 * widget.
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean
gnome_less_show_command(GnomeLess * gl,
			const gchar * command_line)
{
	g_return_val_if_fail(gl != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);
	g_return_val_if_fail(command_line != NULL, FALSE);

	gnome_less_clear(gl);
	return gnome_less_append_command(gl, command_line);
}

#define GLESS_BUFSIZE 1024

/**
 * gnome_less_append_filestream
 * @gl: Pointer to GnomeLess widget
 * @f: Filestream to be displayed in the widget
 *
 * Reads all of the text from filestream @f, and places it in the GnomeLess widget @gl.
 * Appends to any text already in the widget
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean gnome_less_append_filestream(GnomeLess * gl, FILE * f)
{
  gchar buffer [GLESS_BUFSIZE];
  gchar * s;

  g_return_val_if_fail(gl != NULL, FALSE);
  g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);
  g_return_val_if_fail(f != NULL, FALSE);

  errno = 0; /* Reset it to detect errors */
  while (TRUE) {
	  GtkTextIter start, end;

	  s = fgets(buffer, GLESS_BUFSIZE, f);

	  if ( s == NULL ) break;

	  gtk_text_buffer_get_bounds(gl->text_buffer, &start, &end);
	  gtk_text_buffer_insert(gl->text_buffer, &end, buffer, strlen(buffer));
	  gtk_text_buffer_get_bounds(gl->text_buffer, &start, &end);
	  gtk_text_buffer_apply_tag_by_name(gl->text_buffer, "text_tag", &start, &end);
  }

  if ( errno != 0 ) {
    /* We quit on an error, not EOF */
    return FALSE;
  }
  else {
    return TRUE;
  }
}

/**
 * gnome_less_show_filestream
 * @gl: Pointer to GnomeLess widget
 * @f: Filestream to be displayed in the widget
 *
 * Reads all of the text from filestream @f, and places it in the GnomeLess widget @gl. Replaces any text
 * already being displayed.
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean gnome_less_show_filestream(GnomeLess * gl, FILE * f)
{
  g_return_val_if_fail(gl != NULL, FALSE);
  g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);
  g_return_val_if_fail(f != NULL, FALSE);

  gnome_less_clear(gl);
  return gnome_less_append_filestream(gl, f);
}

/**
 * gnome_less_append_fd
 * @gl: Pointer to GnomeLess widget
 * @file_descriptor: Filestream to be displayed in the widget
 *
 * Reads all of the text from file descriptor @file_descriptor, and places it in the GnomeLess widget @gl.
 * Appends to any text already being displayed.
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean
gnome_less_append_fd(GnomeLess * gl, int file_descriptor)
{
  FILE * f;

  g_return_val_if_fail(gl != NULL, FALSE);
  g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);

  f = fdopen(file_descriptor, "r");

  if ( f && gnome_less_append_filestream(gl, f) ) {
    return TRUE;
  }
  else return FALSE;
}

/**
 * gnome_less_show_fd
 * @gl: Pointer to GnomeLess widget
 * @file_descriptor: Filestream to be displayed in the widget
 *
 * Reads all of the text from file descriptor @file_descriptor, and places it in the GnomeLess widget @gl.
 * Replaces any text already being displayed.
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean
gnome_less_show_fd(GnomeLess * gl, int file_descriptor)
{
	g_return_val_if_fail(gl != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);

	gnome_less_clear(gl);
	return gnome_less_append_fd(gl, file_descriptor);
}

/**
 * gnome_less_append_string:
 * @gl: Pointer to GnomeLess widget
 * @s: String to be displayed
 *
 * Description: Appends the @s string to any string already displayed in the
 * widget.
 **/
void
gnome_less_append_string(GnomeLess * gl, const gchar * s)
{
	GtkTextIter start, end;

	g_return_if_fail(gl != NULL);
	g_return_if_fail(GNOME_IS_LESS(gl));
	g_return_if_fail(s != NULL);

	gtk_text_buffer_get_bounds(gl->text_buffer, &start, &end);
	gtk_text_buffer_insert(gl->text_buffer, &end, s, strlen(s));
	gtk_text_buffer_get_bounds(gl->text_buffer, &start, &end);
	gtk_text_buffer_apply_tag_by_name(gl->text_buffer, "text_tag", &start, &end);
}

/**
 * gnome_less_show_string
 * @gl: Pointer to GnomeLess widget
 * @s: String to be displayed
 *
 * Displays a string in the GnomeLess widget @gl. Replaces any text
 * already being displayed.
 *
 **/
 
void gnome_less_show_string(GnomeLess * gl, const gchar * s)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));
  g_return_if_fail(s != NULL);

  gnome_less_clear(gl);
  gnome_less_append_string(gl, s);
}

/**
 * gnome_less_clear
 * @gl: Pointer to GnomeLess widget
 *
 * Clears all text from GnomeLess widget @gl.
 **/

void gnome_less_clear (GnomeLess * gl)
{
  GtkTextIter start, end;

  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));

  gtk_text_buffer_get_bounds(gl->text_buffer, &start, &end);
  gtk_text_buffer_delete(gl->text_buffer, &start, &end);
}

/**
 * gnome_less_write_fd 
 * @gl: Pointer to GnomeLess widget
 * @fd: File descriptor
 *
 * Writes the text displayed in the GnomeLess widget @gl to file descriptor @fd.
 *
 * Returns: %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean
gnome_less_write_fd(GnomeLess * gl, int fd)
{
	gchar * contents;
	gsize len;
	gsize bytes_written;
	GtkTextIter start, end;


	g_return_val_if_fail(gl != NULL, FALSE);
	g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);
	g_return_val_if_fail(fd >= 0, FALSE);

	gtk_text_buffer_get_bounds(gl->text_buffer, &start, &end);
	contents = gtk_text_buffer_get_text(gl->text_buffer, &start, &end, FALSE);
	if(contents)
		len = strlen(contents);
	else
		len = 0;

	if(len > 0)
		bytes_written = write(fd, contents, len);
	else
		bytes_written = 0;

	g_free(contents); 

	if(bytes_written != len)
		return FALSE;
	else
		return TRUE;
}

/**
 * gnome_less_write_file
 * @gl: Pointer to GnomeLess widget
 * @path: Path of file to be written
 *
 * Writes the text displayed in the GnomeLess widget @gl to the file specified by @path.
 *
 * Returns: %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean gnome_less_write_file   (GnomeLess * gl, const gchar * path)
{
  int fd;

  g_return_val_if_fail(gl != NULL, FALSE);
  g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);
  g_return_val_if_fail(path != NULL, FALSE);
  
  fd = open( path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );

  if ( fd == -1 ) return FALSE; /* Leaving errno set */

  if ( ! gnome_less_write_fd(gl, fd) ) {
    int save_errno = errno;
    close(fd); /* If it fails, nothing to do */
    errno = save_errno; /* Return root cause of problem. */
    return FALSE;
  }

  if ( close(fd) != 0 ) {
    /* Error, leave errno and return */
    return FALSE;
  }
  else return TRUE;
}

static void
setup_columns(GnomeLess *gl)
{
	if(gl->_priv->columns <= 0) {
		/* Set to 300 pixels if no columns */
		gtk_widget_set_usize(GTK_WIDGET(gl->text_view), 300, -1); 
	} else {
		int i;
		char *text = g_new(char, gl->_priv->columns + 1);
		PangoLayout *layout;
		PangoRectangle rect;

		for(i = 0; i < gl->_priv->columns; i++)
			text[i] = 'X';
		text[i] = '\0';

		layout = gtk_widget_create_pango_layout(GTK_WIDGET(gl), text);
		pango_layout_set_font_description(layout, gl->_priv->font_desc);

		pango_layout_set_width(layout, -1);
		pango_layout_get_extents(layout, NULL, &rect);

		gtk_widget_set_usize(GTK_WIDGET(gl->text_view),
				     PANGO_PIXELS(rect.width) + 6, -1); 

		g_object_unref(G_OBJECT(layout));
	}
}

/**
 * gnome_less_set_font_string:
 * @gl: Pointer to GnomeLess widget
 * @font: text of the string.
 *
 * Description:  Sets the current font of the widget to the @font.
 * This should be in the format that pango understands such as "Sans 12"
 **/
void
gnome_less_set_font_string(GnomeLess * gl, const gchar * font)
{
	g_return_if_fail(gl != NULL);
	g_return_if_fail(GNOME_IS_LESS(gl));
	g_return_if_fail(font != NULL);

	pango_font_description_free(gl->_priv->font_desc);
	gl->_priv->font_desc = pango_font_description_from_string(font);

	gtk_object_set(GTK_OBJECT(gl->_priv->text_tag),
		       "font_desc", gl->_priv->font_desc,
		       NULL);

	setup_columns(gl);
}

/**
 * gnome_less_set_font_description:
 * @gl: Pointer to GnomeLess widget
 * @font_desc: the PangoFontDescription of the text
 *
 * Description:  Sets the current font of the widget to the @font_desc.
 **/
void
gnome_less_set_font_description(GnomeLess * gl, const PangoFontDescription * font_desc)
{
	g_return_if_fail(gl != NULL);
	g_return_if_fail(GNOME_IS_LESS(gl));
	g_return_if_fail(font_desc != NULL);

	pango_font_description_free(gl->_priv->font_desc);
	gl->_priv->font_desc = pango_font_description_copy(font_desc);

	gtk_object_set(GTK_OBJECT(gl->_priv->text_tag),
		       "font_desc", gl->_priv->font_desc,
		       NULL);

	setup_columns(gl);
}

/**
 * gnome_less_set_font_fixed:
 * @gl: Pointer to GnomeLess widget
 *
 * Description:  Sets the current font to the "fixed" font.
 * This is useful for displaying things that need to be ordered
 * in columns.
 **/
void
gnome_less_set_font_fixed(GnomeLess * gl)
{
	char *font;

	g_return_if_fail(gl != NULL);
	g_return_if_fail(GNOME_IS_LESS(gl));

	pango_font_description_free(gl->_priv->font_desc);
	font = g_strdup_printf("fixed %d", GTK_WIDGET(gl)->style->font_desc->size / PANGO_SCALE);
	gl->_priv->font_desc = pango_font_description_from_string(font);
	g_free(font);

	gtk_object_set(GTK_OBJECT(gl->_priv->text_tag),
		       "font_desc", gl->_priv->font_desc,
		       NULL);

	setup_columns(gl);
}

/**
 * gnome_less_set_font_standard:
 * @gl: Pointer to GnomeLess widget
 *
 * Description:  Sets the current font to the standard font
 * This is the font that's from the current style of the widget.
 **/
void
gnome_less_set_font_standard(GnomeLess * gl)
{
	g_return_if_fail(gl != NULL);
	g_return_if_fail(GNOME_IS_LESS(gl));

	pango_font_description_free(gl->_priv->font_desc);
	gl->_priv->font_desc = pango_font_description_copy(GTK_WIDGET(gl)->style->font_desc);

	gtk_object_set(GTK_OBJECT(gl->_priv->text_tag),
		       "font_desc", gl->_priv->font_desc,
		       NULL);

	setup_columns(gl);
}


/**
 * gnome_less_set_width_columns:
 * @gl: Pointer to GnomeLess widget
 * @columns: number of columns
 *
 * Description:  Sets the number of visible columns.  This will
 * only work well with fixed width fonts, such as "fixed"
 **/
void
gnome_less_set_width_columns(GnomeLess * gl, int columns)
{
	g_return_if_fail(gl != NULL);
	g_return_if_fail(GNOME_IS_LESS(gl));

	gl->_priv->columns = columns;

	setup_columns(gl);
}

/**
 * gnome_less_set_wrap_mode:
 * @gl: Pointer to GnomeLess widget
 * @wrap_mode: The wrap mode to set
 *
 * Description:  Sets the wrap mode of the internal text widget to
 * @wrap_mode.
 **/
void
gnome_less_set_wrap_mode(GnomeLess * gl, GtkWrapMode wrap_mode)
{
	g_return_if_fail(gl != NULL);
	g_return_if_fail(GNOME_IS_LESS(gl));

	gtk_text_view_set_wrap_mode(gl->text_view, wrap_mode);
}

/**
 * gnome_less_get_font_description:
 * @gl: Pointer to GnomeLess widget
 *
 * Description:  Gets the font description currently used
 * in the widget;
 *
 * Returns:  A new copy of the font description, you should
 * free it yourself.
 **/
PangoFontDescription *
gnome_less_get_font_description(GnomeLess * gl)
{
	g_return_val_if_fail(gl != NULL, NULL);
	g_return_val_if_fail(GNOME_IS_LESS(gl), NULL);

	return pango_font_description_copy(gl->_priv->font_desc);
}


#ifndef GNOME_EXCLUDE_DEPRECATED_SOURCE

/***************************************************************************/
/* DEPRECATED */
/**
 * gnome_less_reshow
 * @gl: Pointer to GnomeLess widget
 *
 * Description: Deprecated
 **/
 
void gnome_less_reshow(GnomeLess * gl)
{
	g_warning("gnome_less_reshow deprecated, font is now updated on the fly");
}

/**
 * gnome_less_set_font
 * @gl: Pointer to GnomeLess widget
 * @font: Pointer to GdkFont
 *
 * Description: Deprecated, use #gnome_less_set_font_string
 **/

void gnome_less_set_font(GnomeLess * gl, GdkFont * font)
{
	g_warning("gnome_less_set_font deprecated, use gnome_less_set_font_string");
}

/**
 * gnome_less_set_fixed_font
 * @gl: Pointer to GNOME Less widget
 * @fixed: Whether or not to use a fixed font
 *
 * Description:  Deprecated, use #gnome_less_set_font_fixed
 **/
void gnome_less_set_fixed_font  (GnomeLess * gl, gboolean fixed)
{
	g_warning("gnome_less_set_fixed_font deprecated, use gnome_less_set_font_fixed");

	if(fixed) {
		gnome_less_set_font_fixed(gl);
	} else {
		gnome_less_set_font_standard(gl);
	}
}

#endif /* not GNOME_EXCLUDE_DEPRECATED_SOURCE */

