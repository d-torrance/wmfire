/* fireload_cpu .. accessory to wmfire - Zinx Verituse */
/* e-mail @ zinx@linuxfreak.com */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#ifdef hpux
#  include <sys/pstat.h>
#  define USER_ST       0
#  define NICE_ST       1
#  define SYS_ST        2
#  define IDLE_ST       3
#endif

#ifdef __SOLARIS__
#  include <kstat.h>
#  include <sys/sysinfo.h>
   typedef struct cpu_struct {
     kstat_t *cs_kstat;
     cpu_stat_t cs_new;
     cpu_stat_t cs_old;
   } cpu_struct;
#  define DELTA(i, x) (cpuinfo[i].cs_new.x - cpuinfo[i].cs_old.x)
#endif

#ifdef __NetBSD__
/********************************
 * NetBSD code by Hubert Feyrer *
 ********************************/
int getload(int cpunum, int shownice) {
       double load;
       getloadavg(&load, 1);
       return (int)(load * 100);
}
#elif defined(__SOLARIS__)   /*ifdef __NetBSD__ */
/*****************************
 * Solaris code by John Hitt *
 *****************************/
static kstat_ctl_t *kc;
kstat_t *ksp;
cpu_struct *cpuinfo;
int ncpus;

int getload(int cpunum, int shownice) {

  int load,i,ticks,hz,j;
  double percent, etime;

  cpu_sysinfo_t cpustuff;

  load=0;
  load=0;
  hz = sysconf(_SC_CLK_TCK);
  for (i=0; i < ncpus; i++) {
    cpuinfo[i].cs_old = cpuinfo[i].cs_new;
    if (kstat_read(kc, cpuinfo[i].cs_kstat, (void *)&cpuinfo[i].cs_new) == -1)
      printf("kstat_read(): error getting cpuinfo\n");
    ticks = 0;
    for (j=0; j < CPU_STATES; j++)
      ticks += DELTA(i, cpu_sysinfo.cpu[j]);
    etime = (double)ticks / hz;
    if (etime == 0.0) etime = 1.0;
    percent = 100.0 / etime / hz;
#ifdef DEBUG
    printf("----------------------------------\n");
    printf("ticks: %d\n", ticks);
    printf("percent: %.2f\n", percent);
    printf("cpu %d: %d\n", i, (int)(DELTA(i, cpu_sysinfo.cpu[CPU_USER]) * percent) + (int)(DELTA(i, cpu_sysinfo.cpu[CPU_KERNEL]) * percent));
    printf("----------------------------------\n");
#endif
    load += (int)(DELTA(i, cpu_sysinfo.cpu[CPU_USER]) * percent) + (int)(DELTA(i, cpu_sysinfo.cpu[CPU_KERNEL]) * percent);
  }

  return (int)load/ncpus;
}
#else
int getload(int cpunum, int shownice) {
	static int lastloadstuff = 0, lastidle = 0, lastnice = 0;
	int loadline = 0, loadstuff = 0, nice = 0, idle = 0;
#ifdef hpux
	/*****************************************
 	* HP-UX code (mostly) by Lyonel VINCENT *
 	*****************************************/
	struct pst_dynamic pst_d;
	int i;

	pstat_getdynamic(&pst_d, sizeof(pst_d), (size_t)1, 0);

	if (cpunum < 0 || cpunum >= psd_proc_cnt) {
		for (i = 0; i < PST_MAX_CPUSTATES; i++) {
			switch(i) {
				case NICE_ST:
					nice = pst_d.psd_cpu_time[i];
					break;
				case IDLE_ST:
					idle = pst_d.psd_cpu_time[i];
					break;
				default: /* USER, SYS, BLOCK, SWAIT, INTR, SSYS ... */
					load += pst_d.psd_cpu_time[i];
					break;
			}
		}
	} else /* cpunum >= 0 && cpunum < psd_proc_cnt */ {
		for (i = 0; i < PST_MAX_CPUSTATES; i++) {
			switch(i) {
				case NICE_ST:
					nice = pst_d.psd_mp_cpu_time[cpunum][i];
					break;
				case IDLE_ST:
					idle = pst_d.psd_mp_cpu_time[cpunum][i];
					break;
				default: /* USER, SYS, BLOCK, SWAIT, INTR, SSYS ... */
					load += pst_d.psd_mp_cpu_time[cpunum][i];
					break;
			}
		}
	}
#else /* hope this is a Linux box :) */
	FILE *stat = fopen("/proc/stat", "r");
	char temp[128], *p;

	if (!stat) return 100;

	while (fgets(temp, 128, stat)) {
		if (!strncmp(temp, "cpu", 3) && (temp[3] == ' ' && cpunum < 0) || (temp[3] != ' ' && atol(&temp[3]) == cpunum)) {
			p = strtok(temp, " \t\n");
			loadstuff = atol(p = strtok(NULL, " \t\n"));
			nice = atol(p = strtok(NULL, " \t\n"));
			loadstuff += atol(p = strtok(NULL, " \t\n"));
			idle = atol(p = strtok(NULL, " \t\n"));
			break;
		}
	}

	fclose(stat);
#endif /*def hpux */

	if (!lastloadstuff && !lastidle && !lastnice) {
		lastloadstuff = loadstuff; lastidle = idle; lastnice = nice;
		return 0;
	}

	if (shownice) {
		loadline = (loadstuff-lastloadstuff)+(idle-lastidle);
	} else {
		loadline = (loadstuff-lastloadstuff)+(nice-lastnice)+(idle-lastidle);
	}
	if (loadline)
		loadline = ((loadstuff-lastloadstuff)*100)/loadline;
	else loadline = 100;

	lastloadstuff = loadstuff; lastidle = idle; lastnice = nice;

	return loadline;
}
#endif /*def __NetBSD__ */

void do_help(char *prgname) {
	printf("%s [-c<cpunum>] [-n] [-h]\n", prgname);
}

int main(int argc, char *argv[]) {
	int ch, cpunum = -1, shownice = 1;

	while ((ch = getopt(argc, argv, "c:hn")) != EOF) {
		switch (ch) {
			case 'c':
				cpunum = atoi(optarg);
				break;
			case 'n':
				shownice = 0;
				break;
			default: /* h is handled here */
				do_help(argv[0]);
				exit(0);
		}
	}

	printf("fireload\t0\t100\n"); fflush(stdout);

	for (;;) {
		ch = getload(cpunum, shownice);
		printf("%d\t%d\n", ch, ch); fflush(stdout);
		usleep(100000);
	}

	exit(0);
}
