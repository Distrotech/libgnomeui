/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

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


#define GNOME_TYPE_CALCULATOR            (gnome_calculator_get_type ())
#define GNOME_CALCULATOR(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CALCULATOR, GnomeCalculator))
#define GNOME_CALCULATOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CALCULATOR, GnomeCalculatorClass))
#define GNOME_IS_CALCULATOR(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CALCULATOR))
#define GNOME_IS_CALCULATOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CALCULATOR))
#define GNOME_CALCULATOR_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_CALCULATOR, GnomeCalculatorClass))


typedef struct _GnomeCalculator        GnomeCalculator;
typedef struct _GnomeCalculatorPrivate GnomeCalculatorPrivate;
typedef struct _GnomeCalculatorClass   GnomeCalculatorClass;

typedef enum {
	GNOME_CALCULATOR_DEG,
	GNOME_CALCULATOR_RAD,
	GNOME_CALCULATOR_GRAD
} GnomeCalculatorMode;

struct _GnomeCalculator {
	GtkVBox vbox;

	/*< private >*/
	GnomeCalculatorPrivate *_priv;
};

struct _GnomeCalculatorClass {
	GtkVBoxClass parent_class;

	void (* result_changed)(GnomeCalculator *gc,
				gdouble result);
};


GtkType		 gnome_calculator_get_type	(void);
GtkWidget	*gnome_calculator_new		(void);
void		 gnome_calculator_clear		(GnomeCalculator *gc,
						 const gboolean reset);
void		 gnome_calculator_set		(GnomeCalculator *gc,
						 gdouble result);
gdouble		 gnome_calculator_get_result	(GnomeCalculator *gc);
GtkAccelGroup   *gnome_calculator_get_accel_group(GnomeCalculator *gc);
const char	*gnome_calculator_get_result_string(GnomeCalculator *gc);

END_GNOME_DECLS

#endif
