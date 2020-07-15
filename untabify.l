%{
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int	colcount = 0, linecount = 1, tabwidth = 8, changes = 0, rewritetab = 0;
char	tftempl[] = "/tmp/untabifyXXXXXX";
%}

%%

[ \t]+$	{	changes++;	}
"\r"	{	changes++;	}
"\n"	{	ECHO;	colcount = 0; linecount++;	}
"\t"	{	do { putc(' ', yyout); colcount++; } while ((colcount % tabwidth) != 0); changes++;	}
"\f"	{	putc('\n', yyout);	colcount = 0; linecount++;  changes++;	}
.	{
		int ech, ch;
		ech = yytext[0];
		ECHO;
		colcount++;
		/* Avoid managling strings other than rewriting	tabs if	required */
		if  (ech == '\'' || ech	== '\"')  {
			for  (;;)  {
				ch = input();
				if  (ch	== EOF)
					error("EOF in string");
				if  (rewritetab	&& ch == '\t')	{
					putc('\\', yyout);
					putc('t', yyout);
					colcount += 2;
					continue;
				}
				putc(ch, yyout);
				colcount++;
				if  (ch	== ech)
					break;
				if  (ch	== '\\')  {
					ch = input();
					if  (ch	== EOF)
						error("EOF in string after \\");
					putc(ch, yyout);
					colcount++;
				}
			}
		}
		changes++;
	}

%%

main(int argc, char **argv)
{
	int	ch, errors = 0, verbose = 0;
	extern	char	*optarg;
	extern	int	optind;

	while  ((ch = getopt(argc, argv, "t:vR")) != EOF)  {
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
		printf("%s : ", argv[optind]);
		if  (verbose)
			printf("%d lines %d changes", linecount, changes);				
		if  (changes != 0)  {
			changes = 0;
			fseek(outf, 0L, 0);
			fseek(inf, 0L, 0);
			ftruncate(fileno(inf), 0L);
			while  ((ch = getc(outf)) != EOF)
				putc(ch, inf);
			changes = 0;
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

		

