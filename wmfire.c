/* wmfire - Zinx Verituse */

/* Oh bah.. X seems to take too many resources.. this takes CPU */
/* I tried to make it as fast as I could.. *sigh*.. */

/* ... */

/* Woohoo, I made it take less CPU, but I doubt it will work on
   different endian machines :( tell me if it does... */

/* Note the lookup tables are still there because I'm too lazy to
   remove them.. the thing that takes up all the CPU is interaction with X */

/* e-mail @ zinx@linuxfreak.com */


/* BEGIN ACTUAL SOURCE CODE */

/* #define OLD_LOOK 1 */

/* Uh.. try not to change this, but if you decide it's needed,
   be sure to edit the pixmap as well */
#define EAT_COLORS 256

/* You're advised to not change this unless you change the pixmap */
#define CMAPS 3

/* Change this if you want, it won't make much difference. */
#define SHOW_LEN 64

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <X11/xpm.h>
#include <X11/Xlib.h>

#include "wmgeneral.h"
#include "wmfire.xpm"
#include "lttr.xpm"

#define FIREW 56	/* If you change this, you're asking for it */
#define FIREH 56
#define FIREBUF_ONELINE (FIREW)

/* the extra line (FIREH +1 ) is so we have some garbage space,
   only 7 or 8 slots of it are actually written to */
int firebuf[FIREW*(FIREH+1)], intense[EAT_COLORS];
#ifdef OLD_LOOK
int lookupLine[FIREH];
#endif

extern XpmIcon wmgen;
extern GC NormalGC;
GC or_GC;

XImage *fireimg;
int bytesperpixel;

Pixmap letters;
#define tolower(c) ((((c)>='A')&&((c)<='Z'))?(c)+('a'-'A'):(c))
char char_lttrs[] = "+-.0123456789 abcdefghijklmnopqrstuvwxyz";

void burn_it_ALL(int load, int nobar, int noload, char *text) {
	int x, i;
#ifdef OLD_LOOK
	int y;
#endif
	char *tstr;

	/* Set up the hot spots */
	if (noload) {
		for (x = 0; x < 10; x++) {
			firebuf[(random()&0x3f/*%FIREW*/)+((FIREH-1)*FIREBUF_ONELINE)] = random()%EAT_COLORS;
		}
	} else {
		/* Less hot spots, smaller flame. Simple. */
		for (x = 0, i = ((load>>3)+2); x < i; x++) {
			firebuf[(random()&0x3f/*%FIREW*/)+((FIREH-1)*FIREBUF_ONELINE)] = random()%EAT_COLORS;
		}

		if (load < 100) {
			for (x = 0, i = ((100-load)>>4); x < i; x++) {
				firebuf[(random()&0x3f/*%FIREW*/)+((FIREH-1)*FIREBUF_ONELINE)] = firebuf[x+((FIREH-1)*FIREBUF_ONELINE)]>>1;
			}
		}
	}

	/* This is the flame algo, average the pixel, and three touching it
	   on the next line */
#ifndef OLD_LOOK
	switch (bytesperpixel) {
		case 4:
			for (i = ((FIREH-1)*FIREBUF_ONELINE)-1; i > 0; i--) {
				firebuf[i] = (firebuf[i+FIREBUF_ONELINE-1]+
					      firebuf[i+FIREBUF_ONELINE]+
					      firebuf[i+FIREBUF_ONELINE+1]+
					      firebuf[i])>>2;
				*(long*)(fireimg->data+(i<<2)) = *(long*)&intense[firebuf[i]];
			}
			break;
		case 3:
			for (i = ((FIREH-1)*FIREBUF_ONELINE)-1; i > 0; i--) {
				firebuf[i] = (firebuf[i+FIREBUF_ONELINE-1]+
					      firebuf[i+FIREBUF_ONELINE]+
					      firebuf[i+FIREBUF_ONELINE+1]+
					      firebuf[i])>>2;
				/* Any ideas on this one? 24bit will just have to */
				/* remain slow I guess :( */
				x = intense[firebuf[i]];
				*(short*)(fireimg->data+(i*3)) = *(short*)&x;
				*(char*)(fireimg->data+(i*3)+2) = *(char*)(&x+2);
				/* memcpy(fireimg->data+(i*3), &intense[firebuf[i]], 3); */
			}
			break;
		case 2:
			for (i = ((FIREH-1)*FIREBUF_ONELINE)-1; i > 0; i--) {
				firebuf[i] = (firebuf[i+FIREBUF_ONELINE-1]+
					      firebuf[i+FIREBUF_ONELINE]+
					      firebuf[i+FIREBUF_ONELINE+1]+
					      firebuf[i])>>2;
				*(short*)(fireimg->data+(i<<1)) = *(short*)&intense[firebuf[i]];
			}
			break;
		case 1: /* Probably can't run in 8bpp without mod, but hey */
			for (i = ((FIREH-1)*FIREBUF_ONELINE)-1; i > 0; i--) {
				firebuf[i] = (firebuf[i+FIREBUF_ONELINE-1]+
					      firebuf[i+FIREBUF_ONELINE]+
					      firebuf[i+FIREBUF_ONELINE+1]+
					      firebuf[i])>>2;
				*(char*)(fireimg->data+i) = *(char*)&intense[firebuf[i]];
			}
			break;
	}
#else /* OLD_LOOK is defined */
	for (y = FIREH-2; y >= 0; y--) {
		i = lookupLine[y];

		for (x = 1; x < (FIREW-1); x++) {
			firebuf[x+i] =
				(firebuf[x+i+FIREBUF_ONELINE]+
				 firebuf[x+i+FIREBUF_ONELINE+1]+
				 firebuf[x+i+FIREBUF_ONELINE-1]+
				 firebuf[x+i])>>2;

			memcpy(fireimg->data+(fireimg->bytes_per_line*y + x*bytesperpixel),
				&intense[firebuf[x+i]], bytesperpixel);
		}
	}
#endif /* not def OLD_LOOK */
	if (nobar) {
		XPutImage(display, wmgen.pixmap, NormalGC, fireimg, 0, 0, 4, 4, FIREW, FIREH);
	} else {
		i = (load*56)/100;
		i = (i>56)?56:i;
		XCopyArea(display, wmgen.pixmap, wmgen.pixmap, NormalGC, 194, 6, 56, 6, 4, 54);
		XCopyArea(display, wmgen.pixmap, wmgen.pixmap, NormalGC, 194, 0, i, 6, 4, 54);

		XPutImage(display, wmgen.pixmap, NormalGC, fireimg, 0, 7, 4, 4, FIREW, FIREH-7);
	}

	if (text) {
		x = strlen(text);
		for (i = FIREW-(x*6); i < 4; x--, text++) { i = FIREW-(x*6); }
		for (; *text; i += 6, text++) {
			if ((tstr = strchr(char_lttrs, tolower(*text)))) {
				XCopyArea(display, letters, wmgen.pixmap, or_GC, (tstr-char_lttrs)*6, 0, 6, 7, i+4, 4);
			}
		}
	}
}

void loadcmap(int mapnum) {
	XImage *colormap;
	int i;

	colormap = XGetImage(display, wmgen.pixmap, 0, 64+mapnum, EAT_COLORS, 1, AllPlanes, XYPixmap);
	for (i = 0; i < EAT_COLORS; i++) {
		intense[i] = XGetPixel(colormap, i, 0);
	}
}

int scale100(float num, float min, float max) {
	num = ((num - min) * 100) / (max - min);
	if (num < 0) num = 0;
	return (int)num;
}

struct _loadstuff {
	FILE *file;
	float min, max;
	float load;
	char show[SHOW_LEN];
};

int startload(struct _loadstuff *l, char *program) {
	char buffer[1024], *t;

	l->file = popen(program, "r");
	if (!l->file) return 1;

	buffer[0] = 0;
	fgets(buffer, 1024, l->file);

	/* fireload\t<min>\t<max>[\t<undefined...>]\n */
	if (strncmp(buffer, "fireload\t", 9)) goto broken;

	if (!(t = strtok(buffer+9, "\t\n"))) goto broken;
	l->min = atof(t);

	if (!(t = strtok(NULL, "\t\n"))) goto broken;
	l->max = atof(t);

	if (l->min == l->max) goto broken;

	l->load = l->max;
	l->show[0] = 0;

	return 0;
	
 broken:
 	pclose(l->file);
	return 1;
}

void getload(struct _loadstuff *l) {
	fd_set fds;
	struct timeval tv = {0, 0};
	char buffer[1024], *t;
	int got;


	if (!l->file) return;

	/* In case the load program is moving along at a different speed.. */
	for (;;) {
		FD_ZERO(&fds);
		FD_SET(fileno(l->file), &fds);
		if (select(fileno(l->file)+1, &fds, NULL, NULL, &tv)) {
			fgets(buffer, 1024, l->file);
			got = 1;
		} else break;
	}
	
	if (got) {
		/* <float>\t<what to show>[\t<undefined...>]\n */
		if (!(t = strtok(buffer, "\t\n"))) return;
		l->load = atof(t);
		if (!(t = strtok(NULL, "\t\n"))) return;
		strncpy(l->show, t, SHOW_LEN);
		l->show[SHOW_LEN-1] = 0;
	}
}

void endload(struct _loadstuff *l) {
	if (!l->file) return;
	pclose(l->file);
}

/* Returns 1 for off, 0 for on */
int offon(char *str) {
	if (!strcmp(str, "on") || !strcmp(str, "1") || !strcmp(str, "yes"))
		return 0;
	else if (!strcmp(str, "off") || !strcmp(str, "0") || !strcmp(str, "no"))
		return 1;

	fprintf(stderr, "I have no clue what \"%s\" means; try 1 or 0.\n", str);
	exit(-1);
}

void do_version() {
	fprintf(stderr, "wmfire "VERSION" by Zinx Verituse <zinx@linuxfreak.com>\n\n");
}

void do_help(char *prgname) {
	do_version();
	fprintf(stderr,
"%s\n"
"\t[-L[1|0|on|off|yes|no]] [-B[1|0|on|off|yes|no]]] [-C<0..%d>]\n"
"\t[-s[1|0|on|off|yes|no]] [-P <program [args [...]]>] [-h]\n"
"\n"
"\t-L        Toggles/Sets if flame height follows info provided by -P\n"
"\t-B        Toggles/Sets if load bar shows\n"
"\t-s        Toggles/Sets if the numbers are shown\n"
"\t-C<0..%d> Start at a different colormap\n"
"\t-P <...>  Set \"load program\"; will be expanded by shell...\n"
"\t-d<delay> Set delay between frames (in miliseconds, 1000/delay = fps)\n"
"\t-f<frames>Set number of frames to drop per update\n"
"\t-h\t Shows this short bit of help\n", prgname, CMAPS-1, CMAPS-1);
}

int main(int argc, char *argv[]) {
	int i, frames = 0, load, loadtick, nobar = 0, noload = 0,
	    cmap = 0, shownice = 1, shownums = 1;
	int delay = 20000, fskip = 0;

	char *load_prog = "fireload_cpu";
	struct _loadstuff loads;
	
	XEvent event;
	char *errchar;

	while ((i = getopt(argc, argv, "L::B::s::C:F:S:m:x:hvP:d:f:")) != EOF) {
		switch (i) {
			case 'L':
				if (optarg) noload = offon(optarg);
				else noload = !noload;
				break;
			case 'B':
				if (optarg) nobar = offon(optarg);
				else nobar = !nobar;
				break;
			case 's':
				if (optarg) shownums = !offon(optarg);
				else shownums = !shownums;
				break;
			case 'C':
				errchar = NULL;
				cmap = strtol(optarg, &errchar, 10);
				if (*errchar || cmap >= CMAPS || cmap < 0) {
					do_help(argv[0]);
					exit(-1);
				}
				break;
				
			case 'P':
				load_prog = optarg;
				break;

			case 'd':
				errchar = NULL;
				delay = strtol(optarg, &errchar, 10);
				if (*errchar || delay < 0) {
					do_help(argv[0]);
					exit(-1);
				}
				delay *= 1000;
				break;
				
			case 'f':
				errchar = NULL;
				fskip = strtol(optarg, &errchar, 10);
				if (*errchar || fskip < 0) {
					do_help(argv[0]);
					exit(-1);
				}
				break;

			case 'v':
				do_version();
				exit(0);
				break;
			case 'h':
			case '?':
			case ':':
				do_help(argv[0]);
				exit(0);
				break;
			default:
				fprintf(stderr, "Uhh.. You shouldn't be seeing this, getopt() returned '%c' (%02x)\n", i, i);
				exit(-1);
				break;
		}
	}

	srand(time(NULL)*getpid()+getuid());

	/* Open the window in X for our sinister uses */
	{
		char mask[64*64];
		XGCValues xgcv;

		createXBMfromXPM(mask, wmfire_xpm, 64, 64);
		openXwindow(argc, argv, wmfire_xpm, mask, 64, 64);
		XpmCreatePixmapFromData(display, wmgen.pixmap, lttr_xpm, &letters, NULL, NULL);

		xgcv.function = GXor;
		xgcv.graphics_exposures = 0;
		or_GC = XCreateGC(display, wmgen.pixmap, GCFunction| GCGraphicsExposures, &xgcv);
	}

	cmap %= CMAPS;
	loadcmap(cmap);

	/* Fix up a lookup table for lines */
#ifdef OLD_LOOK
	for (i = 0; i < FIREH; i++) {
		lookupLine[i] = i*FIREBUF_ONELINE;
	}
#endif

	/* Set up a few places the user might wanna click... */
	AddMouseRegion(0, 4, 54, 61, 61);
	AddMouseRegion(1, 4, 12, 58, 53);
	AddMouseRegion(2, 4, 4, 61, 11);

	fireimg = XGetImage(display, wmgen.pixmap, 69, 4, FIREW, FIREH, AllPlanes, ZPixmap);
	bytesperpixel = fireimg->bytes_per_line/fireimg->width;

	if (nobar) XCopyArea(display, wmgen.pixmap, wmgen.pixmap, NormalGC, 65, 0, 64, 64, 0, 0);
	else XCopyArea(display, wmgen.pixmap, wmgen.pixmap, NormalGC, 130, 0, 64, 64, 0, 0);

	startload(&loads, load_prog);

#ifdef HAVE_POLL
	delay /= 1000; /* poll uses milliseconds */
#endif

	for(;;) {
		if (XPending(display)) {
			XNextEvent(display, &event);
			switch (event.type) {
				case DestroyNotify:
					XCloseDisplay(display);
					exit(0);
					break;
				case ButtonPress:
					if (event.xbutton.button != 1) {
						cmap++;
						cmap %= CMAPS;
						loadcmap(cmap);
						break;
					}
					i = CheckMouseRegion(event.xbutton.x, event.xbutton.y);
					switch (i) {
						case 0:
							nobar = !nobar;
							if (nobar) XCopyArea(display, wmgen.pixmap, wmgen.pixmap, NormalGC, 65, 0, 64, 64, 0, 0);
							else XCopyArea(display, wmgen.pixmap, wmgen.pixmap, NormalGC, 130, 0, 64, 64, 0, 0);
							break;
						case 1: noload = !noload;
							break;
						case 2:
							shownums = !shownums;
							break;
					}
			}
		}

		getload(&loads);
		if (shownums)
			burn_it_ALL(scale100(loads.load, loads.min, loads.max), nobar, noload, loads.show);
		else
			burn_it_ALL(scale100(loads.load, loads.min, loads.max), nobar, noload, NULL);

		if ((frames--) <= 0) {
			RedrawWindow();
			frames = fskip;
		}
#ifdef HAVE_POLL /* Are there systems without poll()? */
		poll(NULL, 0, delay);
#else
		usleep(delay);
#endif
	}

	fprintf(stderr, "Congratulations, something truely awful has happened.\n");
	exit(-1);
}
