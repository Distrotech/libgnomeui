#ifndef TESTGNOME_H
#define TESTGNOME_H

#include <bonobo/bonobo-object-client.h>
#include <bonobo/bonobo-ui-container.h>
#include <bonobo/bonobo-ui-component.h>

typedef struct _TestGnomeApp		TestGnomeApp;

struct _TestGnomeApp {
	BonoboUIContainer	*ui_container;
	BonoboUIComponent	*ui_component;

	GtkWidget		*app;

};

void    sample_app_exit         (TestGnomeApp        *app);

#endif /* TESTGNOME_H */
