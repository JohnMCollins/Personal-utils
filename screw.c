#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

main(int argc, char **argv)
{
	if  (argc != 4)  {
		fprintf(stderr, "Usage: %s platform subdir cmd\n", argv[0]);
		return  1;
	}

	if  (chdir("/home2/jails") < 0)  {
		fprintf(stderr, "Cannot find jails directory\n");
		return  10;
	}
	
	if  (chdir(argv[1]) < 0)  {
		fprintf(stderr, "Cannot select platform dir %s\n", argv[1]);
		return  2;
	}

	if  (chroot(".") < 0)  {
		fprintf(stderr, "Cannot chroot to %s\n", argv[1]);
		return  3;
	}

	if  (chdir(argv[2]) < 0)  {
		fprintf(stderr, "Cannot select subdir %s\n", argv[2]);
		return  4;
	}

	setuid(getuid());
	execl("/bin/sh", "sh", argv[3], (char *) 0);
	fprintf(stderr, "No shell in %s root???\n", argv[1]);
	return 100;
}

