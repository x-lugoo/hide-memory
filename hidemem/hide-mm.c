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

#define __NR_memfd_hide 441
static const int prot = PROT_READ | PROT_WRITE;
static const int mode = MAP_SHARED;

static int memfd_secret(unsigned long flags)
{
        return syscall(__NR_memfd_hide, flags);
}


int main(int argc, char *argv[])
{
        int fd; 
	char *p;
	char const str[] = "Jeff Xie\n";

	p = malloc(sizeof(str));
	if (!p)
		handle_error("malloc");
	printf("malloc p:0x%llx\n", p);
	memcpy(p, str, sizeof(str));
	printf("malloc str:%s\n", p);

        fd = memfd_secret(0);
        if (fd < 0) 
		handle_error("memfd");

	ftruncate(fd, 4096);
	printf("entring mmap\n");
	p = mmap(NULL, 4096, prot, mode, fd, 0);
	if (p == MAP_FAILED)
	     handle_error("mmap");

	printf("mmap p:0x%llx\n", p);
	memcpy(p, str, sizeof(str));
	printf("mmap str:%s\n", p);
	sleep(10000);
        close(fd);
	return 0;
}



