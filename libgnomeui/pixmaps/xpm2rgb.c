#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk_imlib.h>


int
main(int argc, char **argv)
{
	GdkImlibImage *im;
	FILE *f;
	int i, j;

	if (argc != 3) exit(1);
	gtk_init(&argc, &argv);
	gdk_imlib_init();
	im = gdk_imlib_load_image(argv[1]);
	if (!im) exit(1);
	f = fopen("gnome-stock-imlib.h", "at");
	if (!f) exit(1);
	j = 0;
	fprintf(f, "\nstatic const char %s[] = {\n", argv[2]);
	for (i = 0; i < im->rgb_width * im->rgb_height * 3; i++) {
		fprintf(f, "0x%02x,", im->rgb_data[i]);
		if (++j >= 13) {
			fprintf(f, "\n");
			j = 0;
		} else {
			fprintf(f, " ");
		}
	}
	fprintf(f, "\n};\n\n");
	fclose(f);
	gdk_imlib_destroy_image(im);
	return 0;
}
