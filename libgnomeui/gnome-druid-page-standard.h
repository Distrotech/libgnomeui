/* gnome-druid-page-standard.h
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2001  James M. Cape <jcape@ignore-your.tv>
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

#ifndef GNOME_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include "gnome-druid-page.h"

G_BEGIN_DECLS

#define GNOME_TYPE_DRUID_PAGE_STANDARD            (gnome_druid_page_standard_get_type ())
#define GNOME_DRUID_PAGE_STANDARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_DRUID_PAGE_STANDARD, GnomeDruidPageStandard))
#define GNOME_DRUID_PAGE_STANDARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_DRUID_PAGE_STANDARD, GnomeDruidPageStandardClass))
#define GNOME_IS_DRUID_PAGE_STANDARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_DRUID_PAGE_STANDARD))
#define GNOME_IS_DRUID_PAGE_STANDARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_DRUID_PAGE_STANDARD))
#define GNOME_DRUID_PAGE_STANDARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_DRUID_PAGE_STANDARD, GnomeDruidPageStandardClass))


typedef struct _GnomeDruidPageStandard        GnomeDruidPageStandard;
typedef struct _GnomeDruidPageStandardPrivate GnomeDruidPageStandardPrivate;
typedef struct _GnomeDruidPageStandardClass   GnomeDruidPageStandardClass;

/**
 * GnomeDruidPageStandard
 * @vbox: A packing widget that holds the contents of the page.
 * @title: The title of the displayed page.
 * @logo: The logo of the displayed page.
 * @top_watermark: The watermark at the top of the displated page.
 * @title_foreground: The color of the title text.
 * @background: The color of the background of the top section and title.
 * @logo_background: The background color of the logo.
 * @contents_background: The background color of the contents section.
 *
 * A widget representing pages that are not initial or terminal pages of a
 * druid.
 */
struct _GnomeDruidPageStandard
{
	GnomeDruidPage parent;

	/*< public >*/
	GtkWidget *vbox;
	gchar *title;
	GdkPixbuf *logo;
	GdkPixbuf *top_watermark;
	GdkColor title_foreground;
	GdkColor background;
	GdkColor logo_background;
	GdkColor contents_background;
	
	/*< private >*/
	GnomeDruidPageStandardPrivate *_priv;
};
struct _GnomeDruidPageStandardClass
{
	GnomeDruidPageClass parent_class;

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};

#define gnome_druid_page_standard_set_bg_color      gnome_druid_page_standard_set_background
#define gnome_druid_page_standard_set_logo_bg_color gnome_druid_page_standard_set_logo_background
#define gnome_druid_page_standard_set_title_color   gnome_druid_page_standard_set_title_foreground


GType      gnome_druid_page_standard_get_type                (void) G_GNUC_CONST;
GtkWidget *gnome_druid_page_standard_new                     (void);
GtkWidget *gnome_druid_page_standard_new_with_vals           (const gchar            *title,
							      GdkPixbuf              *logo,
							      GdkPixbuf              *top_watermark);

void       gnome_druid_page_standard_set_title               (GnomeDruidPageStandard *druid_page_standard,
							      const gchar            *title);
void       gnome_druid_page_standard_set_logo                (GnomeDruidPageStandard *druid_page_standard,
							      GdkPixbuf              *logo_image);
void       gnome_druid_page_standard_set_top_watermark       (GnomeDruidPageStandard *druid_page_standard,
							      GdkPixbuf              *top_watermark_image);
void       gnome_druid_page_standard_set_title_foreground    (GnomeDruidPageStandard *druid_page_standard,
							      GdkColor               *color);
void       gnome_druid_page_standard_set_background          (GnomeDruidPageStandard *druid_page_standard,
							      GdkColor               *color);
void       gnome_druid_page_standard_set_logo_background     (GnomeDruidPageStandard *druid_page_standard,
							      GdkColor               *color);
void       gnome_druid_page_standard_set_contents_background (GnomeDruidPageStandard *druid_page_standard,
							      GdkColor               *color);

/* Convenience Function */
void       gnome_druid_page_standard_append_item             (GnomeDruidPageStandard *druid_page_standard,
							      const gchar            *question,
							      GtkWidget              *item,
							      const gchar            *additional_info);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif /* __GNOME_DRUID_PAGE_STANDARD_H__ */

