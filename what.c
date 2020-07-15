/*
 * Copyright (c) Xi Software Ltd. 1997.
 *
 * what.c: created by John M Collins on Wed Oct 15 1997.
 *----------------------------------------------------------------------
 * $Header$
 * $Log$
 *----------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>

int	firstonly = 0;

void	what(FILE *f, char *name)
{
	int	ch, donetit = 0, ccnt;
	char	lbuf[120];

 cont:
	while  ((ch = getc(f)) != EOF)  {
		if  (ch != '@')
			continue;
	hadat:
		do  ch = getc(f);
		while  (ch == '@');
		if  (ch != '(')
			continue;
		ch = getc(f);
		if  (ch != '#')  {
			if  (ch == '@')
				goto  hadat;
			continue;
		}
		ch = getc(f);
		if  (ch != ')')  {
			if  (ch == '@')
				goto  hadat;
			continue;
		}
		do  ch = getc(f);
		while  (isspace(ch));

		if  (!isprint(ch))
			continue;

		ccnt = 0;
		for  (;;)  {
			if  (ccnt >= sizeof(lbuf) - 1)
				goto  cont;
			lbuf[ccnt++] = ch;
			ch = getc(f);
			if  (ch == '\0' || ch == '\n' || !isascii(ch))
				break;
		}
		lbuf[ccnt] = '\0';
		if  (!donetit  &&  name)  {
			(void) printf("%s:\n", name);
			donetit++;
		}
		(void) printf("\t%s\n", lbuf);
		if  (firstonly)
			return;
	}
}

main(int argc, char **argv)
{
	int	haderrs = 0, cnt;

	if  (argc > 1  &&  strcmp(argv[1], "-s") == 0)  {
		firstonly = 1;
		argc--;
		argv++;
	}
	if  (argc == 1)
		what(stdin, (char *) 0);
	else
		for  (cnt = 1;  cnt < argc;  cnt++)  {
			FILE  *f = fopen(argv[cnt], "r");
			if  (f)  {
				what(f, argv[cnt]);
				(void) fclose(f);
			}
			else  {
				(void) fprintf(stderr, "can't open %s\n", argv[cnt]);
				haderrs++;
			}
		}
	return  haderrs > 0? 1: 0;
}
