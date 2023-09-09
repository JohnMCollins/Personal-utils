

%{
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int	colcount = 0, linecount = 1, tabwidth = 4, changes = 0, rewritetab = 0, pendlines = 0;
char	tftempl[] = "/tmp/untabifyXXXXXX";
%}

%%

[ \t]+$	{	changes++;	}
"\r"	{	changes++;	}
"\n"	{	colcount = 0; pendlines++; linecount++;	}
^[ \t]+	{
			int  cnt;
			while (pendlines > 0)  {
				putc('\n', yyout);
				pendlines--;
			}
			for  (cnt = 0;  cnt < yyleng;  cnt++)  {
				int  ch = yytext[cnt];
				if  (ch == ' ')  {
					putc(' ', yyout);
					colcount++;
				}
				else  {
					do { putc(' ', yyout); colcount++; } while ((colcount % tabwidth) != 0);
					changes++;
				}
			}
		}
"\f"	{	putc('\n', yyout);	colcount = 0; linecount++;  changes++;	}
"\t"	{
			if  (rewritetab)  {
				do {
					putc(' ', yyout);
					colcount++;
				}  while  ((colcount % tabwidth) != 0);
				changes++;
			}
			else  {
				ECHO;
				colcount += tabwidth - colcount % tabwidth;
			}
		}
.	{
		while (pendlines > 0)  {
			putc('\n', yyout);
			pendlines--;
		}
		ECHO;
		colcount++;
	}

%%

int  main(int argc, char **argv)
{
	int	ch, errors = 0, verbose = 0, nofinalnl;
	char  *tw;
	extern	char	*optarg;
	extern	int	optind;

	if  ((tw = getenv("TABWIDTH")))  {
		int  ntw = atoi(tw);
		if  (ntw > 1  &&  ntw < 100)
			tabwidth = ntw;
	}

	while  ((ch = getopt(argc, argv, "t:vRN")) != EOF)  {
		switch  (ch)  {
		default:
			fprintf(stderr, "Usage: %s [ -t n ] [-R] file [file...]\n", argv[0]);
			return  10;
		case  't':
			tabwidth = atoi(optarg);
			if  (tabwidth <= 0)  {
				fprintf(stderr, "invalid tab width %s, resetting to 8\n", optarg);
				return  11;
			}
			break;
		case 'v':
			verbose++;
			break;
		case 'R':
			rewritetab++;
			break;
		case 'N':
			nofinalnl++;
			break;
		}
	}

	for (; argv[optind]; optind++)  {
		char	tfbuf[sizeof(tftempl)];
		int	tfd;
		FILE	*inf, *outf;

		if  (!(inf = fopen(argv[optind], "r+")))  {
			fprintf(stderr, "Cannot open %s for update\n", argv[optind]);
			errors++;
			continue;
		}
		strcpy(tfbuf, tftempl);
		tfd = mkstemp(tfbuf);
		outf = fdopen(tfd, "w+");
		yyin = inf;
		yyout = outf;
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
		printf("%s : ", argv[optind]);
		if  (verbose)
			printf("%d lines %d changes", linecount, changes);
		if  (changes != 0)  {
			changes = 0;
			fseek(outf, 0L, 0);
			fseek(inf, 0L, 0);
			ch = ftruncate(fileno(inf), 0L);
			while  ((ch = getc(outf)) != EOF)
				putc(ch, inf);
			printf(" - updated\n");
		}
		else
			printf(" - not changed\n");

		fclose(inf);
		fclose(outf);
		unlink(tfbuf);
		linecount = colcount = 0;
	}

	return  errors > 0? 1: 0;
}
