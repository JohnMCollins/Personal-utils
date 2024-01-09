#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

void	usage(char *prog)
{
	fprintf(stderr,	"Usage: %s [-a/m/c] [-f fmt] file [file...]\n",	prog);
	exit(100);
}

int  main(int argc, char **argv)
{
	int	 ch, aletter = 'm', errors = 0;
	char	*format	= "%Y/%M/%d";
	extern	char  *optarg;
	extern	int	 optind;

	while  ((ch = getopt(argc, argv, "amcf:")) != EOF)
		switch	(ch)  {
		case  'a':
		case  'm':
		case  'c':
			aletter	= ch;
			break;
		case  'f':
			format = optarg;
			break;
		default:
			usage(argv[0]);
		}

	for  (argv += optind;  *argv;  argv++)	{
		char	*file =	*argv, *cp;
		time_t	st;
		struct	tm  *tp;
		struct	stat  sbuf;
		if  (stat(file,	&sbuf) < 0)  {
			fprintf(stderr,	"Cannot read file %s - %s\n", file, strerror(errno));
			errors++;
			continue;
		}
		switch	(aletter)  {
		default:
			st = sbuf.st_mtime;
			break;
		case  'a':
			st = sbuf.st_atime;
			break;
		case  'c':
			st = sbuf.st_ctime;
			break;
		}
		tp = localtime(&st);
		for  (cp = format;  *cp;  cp++)	 {
			if (*cp	!= '%')	 {
				putchar(*cp);
				continue;
			}
			if  (!*++cp)
				break;
			switch	(*cp)  {
			default:
				putchar(*cp);
				break;
			case  'Y':
				printf("%.4d", tp->tm_year+1900);
				break;
			case  'y':
				printf("%.2d", tp->tm_year % 100);
				break;
			case  'M':
				printf("%.2d", tp->tm_mon+1);
				break;
			case  'd':case	'D':
				printf("%.2d", tp->tm_mday);
				break;
			case  'h':
				printf("%.2d", tp->tm_hour);
				break;
			case  'm':
				printf("%.2d", tp->tm_min);
				break;
			case  's':
				printf("%.2d", tp->tm_sec);
				break;
			}
		}
		putchar('\n');
	}
	if  (errors > 0)
		return	1;
	return	0;
}

