/* GNOME Unit spinner
 * Copyright (C) 1999 James Henstridge <james@daa.com.au>
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

#include <ctype.h>

#include "gnome-unit-spinner.h"
#include "gdk/gdkkeysyms.h"

static GtkObjectClass *parent_class;
static GtkObjectClass *entry_class;

static const GnomeUnit *default_unit = NULL;

static void gnome_unit_spinner_class_init(GnomeUnitSpinnerClass *class);
static void gnome_unit_spinner_init(GnomeUnitSpinner *self);

GtkType
gnome_unit_spinner_get_type(void)
{
  static GtkType us_type = 0;

  if (!us_type) {
    GtkTypeInfo us_info = {
      "GnomeUnitSpinner",
      sizeof(GnomeUnitSpinner),
      sizeof(GnomeUnitSpinnerClass),
      (GtkClassInitFunc) gnome_unit_spinner_class_init,
      (GtkObjectInitFunc) gnome_unit_spinner_init,
      NULL,
      NULL,
      NULL
    };
    us_type = gtk_type_unique(gtk_spin_button_get_type(), &us_info);
  }
  return us_type;
}

static void
gnome_unit_spinner_value_changed(GtkAdjustment *adjustment,
				 GnomeUnitSpinner *spinner)
{
  char buf[256];
  GtkSpinButton *sbutton;

  g_return_if_fail(adjustment != NULL);
  g_return_if_fail(spinner != NULL);
  g_return_if_fail(GNOME_IS_UNIT_SPINNER(spinner));

  sbutton = GTK_SPIN_BUTTON(spinner);
  g_snprintf(buf, sizeof(buf), "%0.*f%s", sbutton->digits,
	     adjustment->value, gnome_unit_abbrev(spinner->adj_unit));

  if (strcmp(buf, gtk_entry_get_text(GTK_ENTRY(spinner))))
    gtk_entry_set_text(GTK_ENTRY(spinner), buf);
}

static gint gnome_unit_spinner_focus_out    (GtkWidget *widget,
					     GdkEventFocus *ev);
static gint gnome_unit_spinner_button_press (GtkWidget *widget,
					     GdkEventButton *ev);
static gint gnome_unit_spinner_key_press    (GtkWidget *widget,
					     GdkEventKey *event);
#if 0 /* FIXME */
static void gnome_unit_spinner_activate     (GtkEditable *editable);
#endif

static void
gnome_unit_spinner_class_init(GnomeUnitSpinnerClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkEditableClass *editable_class;

  object_class = (GtkObjectClass *)class;
  widget_class = (GtkWidgetClass *)class;
  editable_class = (GtkEditableClass *)class;

  widget_class->focus_out_event    = gnome_unit_spinner_focus_out;
  widget_class->button_press_event = gnome_unit_spinner_button_press;
  widget_class->key_press_event    = gnome_unit_spinner_key_press;
#if 0 /* FIXME */
  editable_class->activate         = gnome_unit_spinner_activate;
#endif

  parent_class = gtk_type_class(GTK_TYPE_SPIN_BUTTON);
  entry_class  = gtk_type_class(GTK_TYPE_ENTRY);
}

static void
gnome_unit_spinner_init(GnomeUnitSpinner *self)
{
  /* change over to our own print function that appends the unit name on the
   * end */
  if (self->parent.adjustment) {
    gtk_signal_disconnect_by_data(GTK_OBJECT(self->parent.adjustment),
				  (gpointer) self);
    gtk_signal_connect(GTK_OBJECT(self->parent.adjustment), "value_changed",
		       GTK_SIGNAL_FUNC(gnome_unit_spinner_value_changed),
		       (gpointer) self);
  }

  if ( ! default_unit)
    default_unit = gnome_unit_with_name("centimeter");

  self->adj_unit = default_unit;
}

/**
 * gnome_unit_spinner_new:
 * @adjustment: the adjustment specifying the range of the spinner
 * @digits: The number of decimal digits to display
 * @adj_unit: the default unit for the spinner.
 * 
 * Creates a new GnomeUnitSpinner.  A GnomeUnitSpinner is basically a
 * GtkSpinButton that displays the units the value is in terms of.  It
 * also allows you to enter a value with a different unit suffix, and
 * then convert the value to the correct set of units.
 * 
 * Return value: the new GnomeUnitSpinner.
 **/
GtkWidget *
gnome_unit_spinner_new(GtkAdjustment *adjustment, guint digits,
		       const GnomeUnit *adj_unit)
{
  GnomeUnitSpinner *self;

  self = gtk_type_new(gnome_unit_spinner_get_type());

  gnome_unit_spinner_construct(self, adjustment, digits, adj_unit);

  return GTK_WIDGET(self);
}

/**
 * gnome_unit_spinner_construct:
 * @self: The object to construct
 * @adjustment: the adjustment specifying the range of the spinner
 * @digits: The number of decimal digits to display
 * @adj_unit: the default unit for the spinner.
 * 
 * Constructor for language bindings or subclassing only, for C
 * use #gnome_unit_spinner_new
 **/
void
gnome_unit_spinner_construct(GnomeUnitSpinner *self,
			     GtkAdjustment *adjustment, guint digits,
			     const GnomeUnit *adj_unit)
{
  gtk_spin_button_configure(GTK_SPIN_BUTTON(self), adjustment, 0.0, digits);

  if (adjustment) {
    gtk_signal_disconnect_by_data(GTK_OBJECT(adjustment),
				  (gpointer) self);
    gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		       GTK_SIGNAL_FUNC(gnome_unit_spinner_value_changed),
		       (gpointer) self);
  }

  if (adj_unit)
    self->adj_unit = adj_unit;
}

/**
 * gnome_unit_spinner_set_value:
 * @self: the GnomeUnitSpinner
 * @val: the new value
 * @unit: the units the new value is in, or NULL for the spinner's default.
 * 
 * Change the value of the GnomeUnitSpinner.
 **/
void
gnome_unit_spinner_set_value(GnomeUnitSpinner *self, gfloat val,
			     const GnomeUnit *unit)
{
  GtkSpinButton *sbutton;
  gfloat adj_val;

  g_return_if_fail(self != NULL);
  g_return_if_fail(GNOME_IS_UNIT_SPINNER(self));

  sbutton = GTK_SPIN_BUTTON(self);
  if (unit) {
    adj_val = gnome_unit_convert(val, unit, self->adj_unit);
  } else {
    unit = self->adj_unit;
    adj_val = val;
  }

  if (adj_val < sbutton->adjustment->lower)
    adj_val = sbutton->adjustment->lower;
  else if (adj_val > sbutton->adjustment->upper)
    adj_val = sbutton->adjustment->upper;
  if (adj_val != sbutton->adjustment->value) {
    sbutton->adjustment->value = adj_val;
    gtk_adjustment_value_changed(sbutton->adjustment);
  }
}

/**
 * gnome_unit_spinner_get_value:
 * @self: the GnomeUnitSpinner
 * @unit: the unit you want the result in, or NULL for the spinner's default.
 * 
 * returns the value of the GnomeUnitSpinner in a particular set of units.
 * 
 * Return value: the value.
 **/
gfloat
gnome_unit_spinner_get_value(GnomeUnitSpinner *self, const GnomeUnit *unit)
{
  GtkSpinButton *sbutton;

  g_return_val_if_fail(self != NULL, 0);
  g_return_val_if_fail(GNOME_IS_UNIT_SPINNER(self), 0);

  sbutton = GTK_SPIN_BUTTON(self);
  if (unit)
    return gnome_unit_convert(sbutton->adjustment->value, self->adj_unit,
			      unit);
  else
    return sbutton->adjustment->value;
}

/**
 * gnome_unit_spinner_change_units:
 * @self: the GnomeUnitSpinner
 * @unit: the new set of units.
 * 
 * Changes the default set of units for the GnomeUnitSpinner.
 **/
void
gnome_unit_spinner_change_units(GnomeUnitSpinner *self, const GnomeUnit *unit)
{
  GtkSpinButton *sbutton;
  GtkAdjustment *adj;

  g_return_if_fail(self != NULL);
  g_return_if_fail(unit != NULL);

  if (self->adj_unit == unit)
    return;

  sbutton = GTK_SPIN_BUTTON(self);
  adj = sbutton->adjustment;
  adj->lower = gnome_unit_convert(adj->lower, self->adj_unit, unit);
  adj->upper = gnome_unit_convert(adj->upper, self->adj_unit, unit);
  adj->value = gnome_unit_convert(adj->value, self->adj_unit, unit);
  adj->step_increment = gnome_unit_convert(adj->step_increment,
					   self->adj_unit, unit);
  adj->page_increment = gnome_unit_convert(adj->page_increment,
					   self->adj_unit, unit);
  adj->page_size = gnome_unit_convert(adj->page_size, self->adj_unit, unit);
  self->adj_unit = unit;

  gtk_adjustment_value_changed(sbutton->adjustment);
}

static void
gnome_unit_spinner_update(GnomeUnitSpinner *self)
{
  GtkSpinButton *sbutton;
  gdouble val;
  gchar *extra = NULL;

  g_return_if_fail(self != NULL);
  g_return_if_fail(GNOME_IS_UNIT_SPINNER(self));

  sbutton = GTK_SPIN_BUTTON(self);
  val = g_strtod(gtk_entry_get_text(GTK_ENTRY(self)), &extra);

  /* get rid of extra white space after number */
  while (*extra && isspace(*extra)) extra++;
  if (*extra) {
    const GnomeUnit *disp_unit = gnome_unit_with_abbrev(extra);

    if (disp_unit) {
      val = gnome_unit_convert(val, disp_unit, self->adj_unit);
    }
  }
  /* convert to prefered units */
  if (val < sbutton->adjustment->lower)
    val = sbutton->adjustment->lower;
  else if (val > sbutton->adjustment->upper)
    val = sbutton->adjustment->upper;
  sbutton->adjustment->value = val;
  gtk_adjustment_value_changed(sbutton->adjustment);
}

static gint
gnome_unit_spinner_focus_out(GtkWidget *widget, GdkEventFocus *event)
{
  g_return_val_if_fail(widget != NULL, 0);
  g_return_val_if_fail(GNOME_IS_UNIT_SPINNER(widget), 0);

#if 0 /* FIXME */
  if (GTK_EDITABLE(widget)->editable)
    gnome_unit_spinner_update(GNOME_UNIT_SPINNER(widget));
#endif
  return GTK_WIDGET_CLASS(entry_class)->focus_out_event(widget, event);
}

static gint
gnome_unit_spinner_button_press(GtkWidget *widget, GdkEventButton *event)
{
  g_return_val_if_fail(widget != NULL, 0);
  g_return_val_if_fail(GNOME_IS_UNIT_SPINNER(widget), 0);

  gnome_unit_spinner_update(GNOME_UNIT_SPINNER(widget));
  return GTK_WIDGET_CLASS(parent_class)->button_press_event(widget, event);
}

static gint
gnome_unit_spinner_key_press(GtkWidget *widget, GdkEventKey *event)
{
  /* gint key = event->keyval; */

  g_return_val_if_fail(widget != NULL, 0);
  g_return_val_if_fail(GNOME_IS_UNIT_SPINNER(widget), 0);

#if 0 /* FIXME */
  if (GTK_EDITABLE (widget)->editable &&
      (key == GDK_Up || key == GDK_Down || 
       key == GDK_Page_Up || key == GDK_Page_Down))
    gnome_unit_spinner_update (GNOME_UNIT_SPINNER(widget));
#endif
  return GTK_WIDGET_CLASS(parent_class)->key_press_event(widget, event);
}

#if 0 /* FIXME */

static void
gnome_unit_spinner_activate(GtkEditable *editable)
{
  g_return_if_fail(editable != NULL);
  g_return_if_fail(GNOME_IS_UNIT_SPINNER(editable));

  if (editable->editable)
    gnome_unit_spinner_update(GNOME_UNIT_SPINNER(editable));
}

#endif
