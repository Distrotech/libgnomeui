#ifndef TESTGNOME_H
#define TESTGNOME_H

#include <bonobo/bonobo-object-client.h>
#include <bonobo/bonobo-ui-container.h>

typedef struct _TestGnomeApp		TestGnomeApp;

struct _TestGnomeApp {
	BonoboUIContainer	*ui_container;

	GtkWidget		*app;

};

void    sample_app_exit         (TestGnomeApp        *app);

#endif /* TESTGNOME_H */
