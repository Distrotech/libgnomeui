/* gnome-druid.h
 * Copyright (C) 1999  Red Hat, Inc.
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
*/
/* TODO: allow setting bgcolor for all pages globally */
#ifndef __GNOME_DRUID_H__
#define __GNOME_DRUID_H__

#ifndef GNOME_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include "gnome-druid-page.h"


G_BEGIN_DECLS

#define GNOME_TYPE_DRUID            (gnome_druid_get_type ())
#define GNOME_DRUID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_DRUID, GnomeDruid))
#define GNOME_DRUID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_DRUID, GnomeDruidClass))
#define GNOME_IS_DRUID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_DRUID))
#define GNOME_IS_DRUID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_DRUID))
#define GNOME_DRUID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_DRUID, GnomeDruidClass))


typedef struct _GnomeDruid        GnomeDruid;
typedef struct _GnomeDruidPrivate GnomeDruidPrivate;
typedef struct _GnomeDruidClass   GnomeDruidClass;

struct _GnomeDruid
{
	GtkContainer parent;
	GtkWidget *help;
	GtkWidget *back;
	GtkWidget *next;
	GtkWidget *cancel;
	GtkWidget *finish;

	/*< private >*/
	GnomeDruidPrivate *_priv;
};
struct _GnomeDruidClass
{
	GtkContainerClass parent_class;
	
	void     (*cancel)	(GnomeDruid *druid);
	void     (*help)	(GnomeDruid *druid);

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};


GType      gnome_druid_get_type              (void) G_GNUC_CONST;
GtkWidget *gnome_druid_new                   (void);
void	   gnome_druid_set_buttons_sensitive (GnomeDruid *druid,
					      gboolean back_sensitive,
					      gboolean next_sensitive,
					      gboolean cancel_sensitive,
					      gboolean help_sensitive);
void	   gnome_druid_set_show_finish       (GnomeDruid *druid, gboolean show_finish);
void	   gnome_druid_set_show_help         (GnomeDruid *druid, gboolean show_help);
void       gnome_druid_prepend_page          (GnomeDruid *druid, GnomeDruidPage *page);
void       gnome_druid_insert_page           (GnomeDruid *druid, GnomeDruidPage *back_page, GnomeDruidPage *page);
void       gnome_druid_append_page           (GnomeDruid *druid, GnomeDruidPage *page);
void	   gnome_druid_set_page              (GnomeDruid *druid, GnomeDruidPage *page);

/* Pure sugar, methods for making new druids with a window already */
GtkWidget *gnome_druid_new_with_window       (const char *title,
					      GtkWindow *parent,
					      gboolean close_on_cancel,
					      GtkWidget **window);
void       gnome_druid_construct_with_window (GnomeDruid *druid,
					      const char *title,
					      GtkWindow *parent,
					      gboolean close_on_cancel,
					      GtkWidget **window);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* __GNOME_DRUID_H__ */
