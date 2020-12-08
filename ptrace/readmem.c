#include "botutil.h"

#define BLOCK_SIGNALS 1

/*
Returns
	0: success,
	1: general failure,
	2: argument issue,
	3: attach failed, try again
*/
int main(int argc, char **argv)
{
	/* parse arguments */

	if (argc < 4)
	{
		fprintf(stderr, "Missing arguments\n");
		return 2;
	}

	pid_t const pid = atoi(argv[1]);
	uintptr_t const address = strtoul(argv[2], NULL, 0);
	size_t const size = strtoul(argv[3], NULL, 0);

	if (pid <= 0 || address == ULONG_MAX || size == ULONG_MAX)
	{
		fprintf(stderr, "Invalid arguments\n");
		return 2;
	}

	/* prepare for memory reads */

	char *mempath = NULL;
	verify(-1 != asprintf(&mempath, "/proc/%d/mem", pid));
	char *outbuf = malloc(size);
	assert(outbuf != NULL);

	/* attach to target process */

	// block all signals, we can't blow up while waiting for the child to stop
	// or the child will freeze when it's SIGSTOP arrives and we don't clear it
#if defined(BLOCK_SIGNALS)
	sigset_t oldset;
	{
		sigset_t newset;
		verify(0 == sigfillset(&newset));
		// out of interest, we ensure the most likely signal is present
		assert(1 == sigismember(&newset, SIGINT));
		verify(0 == sigprocmask(SIG_BLOCK, &newset, &oldset));
	}
#endif

	// attach or exit with code 3
	if (0 != ptrace(PTRACE_ATTACH, pid, NULL, NULL))
	{
		int errattch = errno;
		// if ptrace() gives EPERM, it might be because another process
		// is already attached, there's no guarantee it's still attached by
		// the time we check so this is a best attempt to determine who is
		if (errattch == EPERM)
		{
			pid_t tracer = get_tracer_pid(pid);
			if (tracer != 0)
			{
				fprintf(stderr, "Process %d is currently attached\n", tracer);
				return 3;
			}
		}
		error(errattch == EPERM ? 3 : 1, errattch, "ptrace(PTRACE_ATTACH)");
	}

	//verify(0 == raise(SIGINT));

	wait_until_tracee_stops(pid);

#if defined(BLOCK_SIGNALS)
	verify(0 == sigprocmask(SIG_SETMASK, &oldset, NULL));
#endif

	int memfd = open(mempath, O_RDONLY);
	assert(memfd != -1);

	// read bytes from the tracee's memory
	verify(size == pread(memfd, outbuf, size, address));

	verify(!close(memfd));
	verify(!ptrace(PTRACE_DETACH, pid, NULL, 0));

	// write requested memory region to stdout
	// byte count in nmemb to handle writes of length 0
	verify(size == fwrite(outbuf, 1, size, stdout));

	free(outbuf);
	free(mempath);

	return 0;
}
