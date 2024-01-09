
%array%
%{
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int	colcount = 0,				/* Column number we've got to on output */
	currcol =	0,				/* Column number we're at on input taking into account tabs and spaces which might be replaced */
	linecount = 1,
	intabwidth = 4,				/* Tab width on input */
	outtabwidth = 4,			/* Tab width on ouput */
	maxspaces = 1,				/* Maximum number of spaces before we start replacing internal tabs */
	rewritetab = 0,				/* Replace internal tabs with spaces */
	changes	= 0,				/* Changes made */
	pendlines = 0,				/* Count of lines pending to trim trailing \ns */
	savelength = 0,				/* Length of saved whitespace */
	instring = 0;				/* If we are in a string, this is the character that started the string */

char	tftempl[] = "/tmp/tabifyXXXXXX";
char	savewhite[YYLMAX];

/* Output pending newlines after we read new stuff */

void	writependlines()
{
	while (pendlines > 0)  {
		putc('\n', yyout);
		pendlines--;
	}
}

/* Keep tabs on tabs without writing anything */

void	accumwhite()
{
	int i;
	for (i = 0;  i < yyleng;  i++)  {
		int  ch = yytext[i];
		savewhite[i] = ch;
		if (ch == ' ')
			currcol++;
		else
			currcol += intabwidth - currcol % intabwidth;
		}
		savelength = yyleng;
}

/* Output pending white space replacing by tabs where possible */

void	writependspace()
{
	int  priospace = colcount > 0;

	if  (currcol != colcount)  {
		int  newlen = 0;
		char	newwhite[YYLMAX];
		while  (currcol	> colcount)  {
			int  newcol = colcount + outtabwidth - colcount % outtabwidth;
			if  (newcol > currcol)
				break;
			if  (priospace  &&  newcol - colcount <= maxspaces)
				break;
			putc('\t', yyout);
			newwhite[newlen++] = '\t';
			colcount = newcol;
		}
		while  (currcol	> colcount)  {
			colcount++;
			putc(' ', yyout);
			newwhite[newlen++] = ' ';
		}
		if  (newlen != savelength || strncmp(newwhite, savewhite, newlen) != 0)
			changes++;
	}
}

%}

%%

[ \t]+$	{	changes++;	}
"\r"	{	changes++;	}
"\n"	{	colcount = currcol = instring = 0; pendlines++; linecount++;	}
^[ \t]+	{	accumwhite();	}
"\f"	{	putc('\n', yyout);	colcount = currcol = 0;	linecount++;  changes++;	}
"\\'"	{
			if  (!instring)  {
				writependlines();
				writependspace();
			}
			ECHO;
			colcount += yyleng;
			currcol += yyleng;
		}
'\"'	{
			if  (!instring)  {
				writependlines();
				writependspace();
			}
			ECHO;
			colcount += yyleng;
			currcol += yyleng;
		}
\"|\'	{	/* Start or end of string */
			if  (instring)  {
				ECHO;
				colcount++;
				currcol++;
				if  (instring == yytext[0])
					instring = 0;
			}
			else  {
				writependlines();
				writependspace();
				instring = yytext[0];
				ECHO;
				colcount++;
				currcol++;
			}
		}
[ \t]+	{	/* Spaces or tabs but not at the beginning or end */
			if (instring)  {
				int  i, hadtabs = 0;
				for  (i = 0;  i < yyleng;  i++)  {
					if  (yytext[i] == '\t')
						hadtabs++;
					putc(yytext[i], yyout);
					currcol++;
					colcount++;
				}
				if  (hadtabs > 0)
					fprintf(stderr, "Line %d may have tabs in string, edit by hand if needed\n", linecount);
			}
			else  if  (rewritetab)  {
				int  i;
				for  (i = 0;  i < yyleng;  i++)
					if  (yytext[i] == ' ')  {
						putc(' ', yyout);
						currcol++;
						colcount++;
					}
					else  {
						do  {
							putc(' ', yyout);
							currcol++;
							colcount++;
						}  while  (currcol % outtabwidth != 0);
						changes++;
					}
			}
			else
				accumwhite();
	}
.	{
		if (!instring)  {
			writependlines();
			writependspace();
		}	
		ECHO;
		colcount++;
		currcol++;
	}
%%

int main(int argc, char **argv)
{
	int	ch, errors = 0,	verbose	= 0, nofinalnl = 0;
	char	*tw;
	extern	char	*optarg;
	extern	int	optind;

	if  ((tw = getenv("TABWIDTH")))  {
		int  ntw = atoi(tw);
		if  (ntw > 1  &&  ntw < 100)
			intabwidth = outtabwidth = ntw;
	}

	while  ((ch = getopt(argc, argv, "t:O:I:vNRM:")) != EOF)  {
		switch	(ch)  {
		default:
			fprintf(stderr,	"Usage: %s [ -t n ] [ -I n] [ -O n] [ -o file ] [ -M n [-R] [file]\n",	argv[0]);
			return	10;
		case  't':
			intabwidth = outtabwidth = atoi(optarg);
			break;
		case 'I':
			intabwidth = atoi(optarg);
			break;
		case 'O':
			outtabwidth = atoi(optarg);
			break;
		case 'M':
			maxspaces = atoi(optarg);
			break;
		case 'R':
			rewritetab++;
			break;
		case 'v':
			verbose++;
			break;
		case 'N':
			nofinalnl++;
			break;
		}
	}
	
	if  (intabwidth <= 1 || outtabwidth <= 1)  {
		fprintf(stderr,	"invalid tab width(s) in %d or out %d\n", intabwidth, outtabwidth);
		return	11;
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
		printf("%s : ",	argv[optind]);
		if  (verbose)
			printf("%d lines %d changes", linecount, changes);
		if  (changes !=	0)  {
			changes	= 0;
			fseek(outf, 0L,	0);
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
		linecount = colcount = instring = 0;
	}

	return	errors > 0? 1: 0;
}
