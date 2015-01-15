/* fireload_file .. accesory to wmfire - Zinx Verituse */
/* e-mail @ zinx@linuxfreak.com */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

char *getheight(char *scanstring, char *filename) {
	static char buffer[1024];
	char *num;
	FILE *f;
	int ch;

	if (!(f = fopen(filename, "r"))) return NULL;

	for (; *scanstring && !feof(f); scanstring++) {
		switch (*scanstring) {
			case '%':
				if (scanstring[1] == 's') {
					while (!feof(f) && isspace(ch = fgetc(f)));
					while (!feof(f) && !isspace(ch = fgetc(f)));
					scanstring++;
				}
				break;
			default:
				while (!feof(f) && (ch = fgetc(f)) != *scanstring);
				break;
		}
	}

	if (feof(f)) {
		fclose(f);
		return NULL;
	}
	/* Premature EOF */

	fgets(buffer, 1024, f);

	num = buffer + strspn(buffer, " \t\n");
	num[strcspn(num, " \t\n")] = 0;

	fclose(f);
	
	return num;
}

void do_help(char *prgname) {
	printf("%s -F <file> -S <string> -m <minimum> -x <maximum>\n", prgname);
}

int main(int argc, char *argv[]) {
	int ch;

	float min = 0, max = 0, numf;
	char *filename = NULL, *scanstr = NULL, *num;

	while ((ch = getopt(argc, argv, "F:S:m:x:h")) != EOF) {
		switch (ch) {
			case 'F':
				filename = optarg;
				break;
			case 'S':
				scanstr = optarg;
				break;
			case 'm':
				min = atof(optarg);
				break;
			case 'x':
				max = atof(optarg);
				break;
			default:
				do_help(argv[0]);
				exit(0);
				break;
		}
	}

	if (!filename || !scanstr || !(max - min)) {
		do_help(argv[0]);		
		exit(-1);
	}

	printf("fireload\t%f\t%f\n", min, max); fflush(stdout);

	for (;;) {
		num = getheight(scanstr, filename);
		if (!num) {
			printf("%f\tERROR\n", max); fflush(stdout);
		} else {
			printf("%f\t%s\n", atof(num), num); fflush(stdout);
		}
		usleep(100000);
	}

	exit(0);
}
