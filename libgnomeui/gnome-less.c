/* gnome-less.c: Copyright (C) 1998 Free Software Foundation
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gnome-less.h"

#include <config.h>

#include "libgnome/gnome-util.h"

#include "libgnome/gnome-i18nP.h"

#include <time.h>
#include <string.h> /* memset */
#include "libgnomeui/gnome-uidefs.h"


static void gnome_less_class_init (GnomeLessClass *klass);
static void gnome_less_init       (GnomeLess      *messagebox);

static void gnome_less_destroy (GtkObject *gl);

static void gnome_less_clear (GnomeLess * gl);
static void gnome_less_set_font(GnomeLess * gl, GdkFont * font);

static gint text_clicked_cb(GtkText * text, GdkEventButton * e, 
			    GnomeLess * gl);

static GtkWindowClass *parent_class;

guint
gnome_less_get_type ()
{
  static guint gl_type = 0;

  if (!gl_type)
    {
      GtkTypeInfo gl_info =
      {
	"GnomeLess",
	sizeof (GnomeLess),
	sizeof (GnomeLessClass),
	(GtkClassInitFunc) gnome_less_class_init,
	(GtkObjectInitFunc) gnome_less_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      gl_type = gtk_type_unique (gtk_vbox_get_type (), &gl_info);
    }

  return gl_type;
}

static void
gnome_less_class_init (GnomeLessClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) klass;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = gnome_less_destroy;
}

static void
gnome_less_init (GnomeLess *gl)
{
  GtkWidget * vscroll;
  GtkWidget * hbox; /* maybe just inherit from hbox? hard to say yet. */
  GtkWidget * mi;

  gl->text = GTK_TEXT(gtk_text_new(NULL, NULL));

  gl->font = NULL;

  gl->popup = GTK_MENU(gtk_menu_new());
  mi = gtk_menu_item_new_with_label("Doesn't do anything yet");
  gtk_menu_append(gl->popup, mi);
  gtk_widget_show(mi);

  gtk_widget_set_events(GTK_WIDGET(gl->text), GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect(GTK_OBJECT(gl->text), "button_press_event",
		     GTK_SIGNAL_FUNC(text_clicked_cb), gl);

  hbox = gtk_hbox_new(FALSE, 0);
  vscroll = gtk_vscrollbar_new(gl->text->vadj);
  
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(gl->text),
		     TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), vscroll, FALSE, FALSE, 0);

  /* Since horizontal scroll doesn't work, use this hack. */
  gtk_widget_set_usize(GTK_WIDGET(gl->text), 300, -1); 

  gtk_widget_show_all(hbox);

  gtk_container_add(GTK_CONTAINER(gl), hbox);
}


GtkWidget* gnome_less_new (void)
{
  GnomeLess * gl;
  
  gl = gtk_type_new(gnome_less_get_type());

  return GTK_WIDGET (gl);
}

static void gnome_less_destroy (GtkObject *gl)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));

  if (GNOME_LESS(gl)->font) gdk_font_unref(GNOME_LESS(gl)->font);

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* (GTK_OBJECT_CLASS(parent_class)->destroy))(gl);
}

static void do_error_message(GnomeLess * gl, const gchar * format, 
			     const gchar * string)
{
  gint len;
  gchar * error_string;

  len = strlen(string) + strlen(format);
  error_string = g_malloc(len);
  g_snprintf(error_string, len, format, string);
  gnome_less_show_string(gl, error_string);
  g_free(error_string);
}

void gnome_less_show_file(GnomeLess * gl, const gchar * path)
{
  FILE * f;
  gchar * not_found = 
    _("This should have displayed the file:\n %s\n"
      "but that file could not be found.");

  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));
  g_return_if_fail(path != NULL);

  if ( ! g_file_exists(path) ) {
    do_error_message(gl, not_found, path);
    return;
  }

  f = fopen(path, "r");

  if ( f == NULL ) {
    g_warning("Couldn't open file %s\n", path);
    return;
  }

  gnome_less_show_filestream(gl, f);

  fclose(f);
}

void gnome_less_show_command(GnomeLess * gl,
			     const gchar * command_line)
{
  FILE * p;
  gchar * no_pipe = 
    _("This should have displayed output from the command:\n %s\n"
      "but there was an error.");

  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));
  g_return_if_fail(command_line != NULL);
  
  p = popen(command_line, "r");

  if ( p == NULL ) {
    do_error_message(gl, no_pipe, command_line);
    return;
  }

  gnome_less_show_filestream(gl, p);

  pclose(p);
}

void gnome_less_show_filestream(GnomeLess * gl, FILE * f)
{
  static const gint bufsize = 1024;
  gchar buffer[bufsize + 1];
  gint bytes_read = bufsize; /* So the while loop starts. */

  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));
  g_return_if_fail(f != NULL);

  memset(buffer, '\0', sizeof(buffer));

  gtk_text_freeze(gl->text);
  
  gnome_less_clear(gl);
  
  while (bytes_read == bufsize) {
    bytes_read = fread(buffer, sizeof(char), bufsize, f);
    
    gtk_text_insert(gl->text, gl->font, NULL, NULL, buffer, bytes_read);
  }

  gtk_text_thaw(gl->text);
}

static void gnome_less_append_string(GnomeLess * gl, const gchar * s)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));
  g_return_if_fail(s != NULL);

  gtk_text_set_point(gl->text, gtk_text_get_length(gl->text));
  gtk_text_insert(gl->text, gl->font, NULL, NULL, s, strlen(s)); 
}

void gnome_less_show_string(GnomeLess * gl, const gchar * s)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));
  g_return_if_fail(s != NULL);

  gtk_text_freeze(gl->text);
  gnome_less_clear(gl);
  gnome_less_append_string(gl, s);
  gtk_text_thaw(gl->text);
}

static void gnome_less_clear (GnomeLess * gl)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));

  gtk_editable_delete_text(GTK_EDITABLE(gl->text), 0,
			   gtk_text_get_length(GTK_TEXT(gl->text)));
}

static void gnome_less_set_font(GnomeLess * gl, GdkFont * font)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));

  /* font is allowed to be NULL */

  if (gl->font) gdk_font_unref(gl->font);
  gl->font = font;
  if (gl->font) {
    gchar eighty_ems[81];
    gdk_font_ref(gl->font);
    /* This is sort of a silly hack, just for now since horizontal
       scroll is broken. */
    memset(eighty_ems, 'M', sizeof(gchar) * 80);
    eighty_ems[80] = '\0';
    gtk_widget_set_usize(GTK_WIDGET(gl->text), 
			 gdk_string_width(font, eighty_ems) + 10,
			 -1);
  }
}

void gnome_less_fixed_font(GnomeLess * gl)
{
  GdkFont * font;
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));

  /* Is this a standard X font? Fixme */
  font = gdk_font_load("-*-courier-medium-r-normal-*-12-*-*-*-*-*-*-*");

  if ( font == NULL ) {
    g_warning("Couldn't load fixed font\n");
    return;
  }

  gnome_less_set_font(gl, font);
}

static gint text_clicked_cb(GtkText * text, GdkEventButton * e, 
			    GnomeLess * gl)
{
  if (e->button == 1) {
    /* Ignore button 1 */
    return TRUE; 
  }

  /* don't change the selection. */
  gtk_signal_emit_stop_by_name (GTK_OBJECT (text), "button_press_event");

  gtk_menu_popup(gl->popup, NULL, NULL, NULL,
                 NULL, e->button, time(NULL));
  return TRUE; 
}
