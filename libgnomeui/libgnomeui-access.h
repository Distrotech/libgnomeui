#ifndef __LIBGNOMEUI_ACCESS_H__
#define __LIBGNOMEUI_ACCESS_H__
/*
 * Accessibility convenience functions.
 *
 * Copyright 2002, Sun Microsystems.
 */

#include <glib-object.h>
#include <atk/atkobject.h>
#include <atk/atkregistry.h>
#include <atk/atkrelationset.h>
#include <atk/atkobjectfactory.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtklabel.h>

void _add_atk_name_desc (GtkWidget *widget, gchar *name, gchar *desc);
void _set_relation      (GtkWidget *widget, GtkLabel *label, int set_for);

#endif /* ! __LIBGNOMEUI_ACCESS_H__ */
