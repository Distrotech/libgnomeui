#ifndef __GNOME_PIXMAP_H__
#define __GNOME_PIXMAP_H__

BEGIN_GNOME_DECLS

void gnome_destroy_pixmap_cache (void);

void gnome_create_pixmap_gdk              (GdkWindow *window,
					   GdkPixmap **pixmap,
					   GdkBitmap **mask,
					   GdkColor  *transparent,
					   char *file);
void gnome_create_pixmap_gtk              (GtkWidget *window,
					   GdkPixmap **pixmap,
					   GdkBitmap **mask,
					   GtkWidget *holder,
					   char *file);
void gnome_create_pixmap_gtk_d            (GtkWidget *window,
					   GdkPixmap **pixmap,
					   GdkBitmap **mask,
					   GtkWidget *holder,
					   char **data);
GtkWidget *gnome_create_pixmap_widget     (GtkWidget *window,
					   GtkWidget *holder,
					   char *file);
GtkWidget *gnome_create_pixmap_widget_d   (GtkWidget *window,
					   GtkWidget *holder,
					   char **data);
void gnome_set_pixmap_widget              (GtkPixmap *pixmap,	
					   GtkWidget *window,
					   GtkWidget *holder,
					   char *file);
void gnome_set_pixmap_widget_d            (GtkPixmap *pixmap,	
					   GtkWidget *window,
					   GtkWidget *holder,
					   char **data);

END_GNOME_DECLS

#endif /* __GNOME_PIXMAP_H__ */
