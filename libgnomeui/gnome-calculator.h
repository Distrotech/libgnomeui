/* GnomeCalculator - double precision simple calculator widget
 *
 * Author: George Lebl <jirka@5z.com>
 */

#ifndef GNOME_CALCULATOR_H
#define GNOME_CALCULATOR_H


#include <gdk/gdk.h>
#include <gtk/gtkvbox.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS


#define GNOME_CALCULATOR(obj)         GTK_CHECK_CAST (obj, gnome_entry_get_type (), GnomeCalculator)
#define GNOME_CALCULATOR_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_calculator_get_type (), GnomeCalculatorClass)
#define GNOME_IS_CALCULATOR(obj)      GTK_CHECK_TYPE (obj, gnome_calculator_get_type ())


typedef struct _GnomeCalculator      GnomeCalculator;
typedef struct _GnomeCalculatorClass GnomeCalculatorClass;

struct _GnomeCalculator {
	GtkVBox vbox;

	GtkWidget *display;

	gdouble result;
	gchar result_string[16];

	gint add_digit;		/*add a digit instead of starting a new
				  number*/
	gdouble memory;

	GList *stack;
};

struct _GnomeCalculatorClass {
	GtkVBoxClass parent_class;
};


guint		 gnome_calculator_get_type	(void);
GtkWidget	*gnome_calculator_new		(void);
void		 gnome_calculator_clear		(void);

END_GNOME_DECLS

#endif
