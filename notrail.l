%{
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int	linecount = 1, changes	= 0, pendlines = 0;
char	tftempl[] = "/tmp/notrailXXXXXX";

%}

%%

[ \t]+$	{	changes++;	}
"\r"	{	changes++;	}
"\n"	{	pendlines++;	linecount++;	}
.	{	while (pendlines > 0)  {
			putc('\n', yyout);
			pendlines--;
		}
		ECHO;
	}
%%

int  main(int argc, char **argv)
{
	int	ch, errors = 0,	verbose	= 2, nofinalnl = 0;
	extern	char	*optarg;
	extern	int	optind;

	while  ((ch = getopt(argc, argv, "qv")) != EOF)  {
		switch	(ch)  {
		default:
			fprintf(stderr,	"Usage: %s [-v] [-q] [file]\n",	argv[0]);
			return	10;
		case 'v':
			verbose++;
			break;
		case 'q':
			verbose = 0;
			break;
		}
	}

	for (; argv[optind]; optind++)	{
		char	tfbuf[sizeof(tftempl)];
		int	tfd;
		FILE	*inf, *outf;

		if  (!(inf = fopen(argv[optind], "r+")))  {
			fprintf(stderr,	"Cannot open %s for update\n", argv[optind]);
			errors++;
			continue;
		}
		strcpy(tfbuf, tftempl);
		tfd = mkstemp(tfbuf);
		outf = fdopen(tfd, "w+");
		yyin = inf;
		yyout =	outf;
		yylex();
		if  (pendlines != 0)  {
			if (nofinalnl)
				changes++;
			else  {
				if (pendlines != 1)
					changes++;
				putc('\n', outf);
			}
		}
		if (changes != 0)  {
			fseek(outf, 0L,	0);
			fseek(inf, 0L, 0);
			ch = ftruncate(fileno(inf), 0L);
			while  ((ch = getc(outf)) != EOF)
				putc(ch, inf);
		}
		fclose(inf);
		fclose(outf);
		unlink(tfbuf);
		if  (verbose > 0)  {
			if  (verbose > 2)
				printf("%s: %d lines %d changes", argv[optind], linecount, changes);
			else  if  (verbose > 1)  {
				printf("%s: - ", argv[optind]);
				if  (changes != 0)
					printf("updated\n");
				else
					printf("not changed\n");
			}
			else if (changes != 0)
				printf("%s:  - updated\n", argv[optind]);
		}
		linecount = changes = 0;
	}

	return	errors > 0? 1: 0;
}
