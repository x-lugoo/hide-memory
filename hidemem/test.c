#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

 #define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(void)
{
	char *p;
	char const str[] = "Jeff Xie\n";
	p = malloc(sizeof(str));
	if (!p)
		handle_error("malloc");
	printf("p:0x%llx\n", p);
	memcpy(p, str, sizeof(str));
	printf("str:%s\n", p);
	sleep(10000);

	return 0;
}

