#ifndef GNOME_PROPERTIES_H
#define GNOME_PROPERTIES_H

BEGIN_GNOME_DECLS

typedef struct {
	GtkWidget *notebook;

	GList *props;
} GnomePropertyConfigurator;

/* This is the first parameter to the callback function */
typedef enum {
	GNOME_PROPERTY_READ,
	GNOME_PROPERTY_WRITE,
	GNOME_PROPERTY_APPLY,
	GNOME_PROPERTY_SETUP
} GnomePropertyRequest;

GnomePropertyConfigurator
     *gnome_property_configurator_new (void);
  
void gnome_property_configurator_destroy (GnomePropertyConfigurator *);
void gnome_property_configurator_register (GnomePropertyConfigurator *, 
					   int (*callback)(GnomePropertyRequest));
void gnome_property_configurator_setup (GnomePropertyConfigurator *);
gint gnome_property_configurator_request (GnomePropertyConfigurator *,
					  GnomePropertyRequest);
void gnome_property_configurator_request_foreach (GnomePropertyConfigurator *th,
						  GnomePropertyRequest r);

END_GNOME_DECLS

#endif
