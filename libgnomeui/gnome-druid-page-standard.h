/* gnome-druid-page-standard.h
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
#ifndef __GNOME_DRUID_PAGE_STANDARD_H__
#define __GNOME_DRUID_PAGE_STANDARD_H__

#include <gtk/gtk.h>
#include "gnome-canvas.h"
#include "gnome-druid-page.h"

BEGIN_GNOME_DECLS

#define GNOME_TYPE_DRUID_PAGE_STANDARD            (gnome_druid_page_standard_get_type ())
#define GNOME_DRUID_PAGE_STANDARD(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_DRUID_PAGE_STANDARD, GnomeDruidPageStandard))
#define GNOME_DRUID_PAGE_STANDARD_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_DRUID_PAGE_STANDARD, GnomeDruidPageStandardClass))
#define GNOME_IS_DRUID_PAGE_STANDARD(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_DRUID_PAGE_STANDARD))
#define GNOME_IS_DRUID_PAGE_STANDARD_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_DRUID_PAGE_STANDARD))
#define GNOME_DRUID_PAGE_STANDARD_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_DRUID_PAGE_STANDARD, GnomeDruidPageStandardClass))


typedef struct _GnomeDruidPageStandard        GnomeDruidPageStandard;
typedef struct _GnomeDruidPageStandardPrivate GnomeDruidPageStandardPrivate;
typedef struct _GnomeDruidPageStandardClass   GnomeDruidPageStandardClass;

struct _GnomeDruidPageStandard
{
	GnomeDruidPage parent;

	/*< public >*/
	GtkWidget *vbox;
	GdkPixbuf *logo_image;
	GdkPixbuf *top_watermark_image;

	gchar *title;

	GdkColor background_color;
	GdkColor logo_background_color;
	GdkColor title_color;
	
	/*< private >*/
	GnomeDruidPageStandardPrivate *_priv;
};
struct _GnomeDruidPageStandardClass
{
	GnomeDruidPageClass parent_class;
};


GtkType    gnome_druid_page_standard_get_type      (void) G_GNUC_CONST;
GtkWidget *gnome_druid_page_standard_new           (void);
GtkWidget *gnome_druid_page_standard_new_aa        (void);
GtkWidget *gnome_druid_page_standard_new_with_vals (gboolean		 antialiased,
						    const gchar		*title,
						    GdkPixbuf		*logo,
						    GdkPixbuf		*top_watermark);
void       gnome_druid_page_standard_construct     (GnomeDruidPageStandard *druid_page_standard,
						    gboolean		 antialiased,
						    const gchar		*title,
						    GdkPixbuf		*logo,
						    GdkPixbuf		*top_watermark);
void       gnome_druid_page_standard_set_bg_color  (GnomeDruidPageStandard *druid_page_standard,
						    GdkColor		*color);
void       gnome_druid_page_standard_set_logo_bg_color(GnomeDruidPageStandard *druid_page_standard,
						    GdkColor		*color);
void       gnome_druid_page_standard_set_title_color(GnomeDruidPageStandard *druid_page_standard,
						    GdkColor		*color);
void       gnome_druid_page_standard_set_title     (GnomeDruidPageStandard *druid_page_standard,
						    const gchar		*title);
void       gnome_druid_page_standard_set_logo      (GnomeDruidPageStandard *druid_page_standard,
						    GdkPixbuf		*logo_image);
void       gnome_druid_page_standard_set_top_watermark(GnomeDruidPageStandard *druid_page_standard,
						    GdkPixbuf		*top_watermark_image);

END_GNOME_DECLS

#endif /* __GNOME_DRUID_PAGE_STANDARD_H__ */

