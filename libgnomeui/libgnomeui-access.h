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

void _add_atk_name_desc   (GtkWidget *widget, gchar *name, gchar *desc);
void _add_atk_description (GtkWidget *widget, gchar *desc);
void _add_atk_relation    (GtkWidget *widget1, GtkWidget *widget2,
			   AtkRelationType w1_to_w2, AtkRelationType w2_to_w1);



/* Copied from eel */

typedef void     (*_AccessibilityClassInitFn)    (AtkObjectClass *klass);

AtkObject    *_accessibility_get_atk_object        (gpointer              object);
AtkObject    *_accessibility_for_object            (gpointer              object);
gpointer      _accessibility_get_gobject           (AtkObject            *object);
AtkObject    *_accessibility_set_atk_object_return (gpointer              object,
						    AtkObject            *atk_object);
GType         _accessibility_create_derived_type   (const char           *type_name,
						    GType                 existing_gobject_with_proxy,
						    _AccessibilityClassInitFn class_init);


#endif /* ! __LIBGNOMEUI_ACCESS_H__ */
