#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef NDEBUG
#	define verify(expr) (((void)(0))(expr))
#else
#	define verify assert
#endif

__attribute__((format(printf, 6, 7), noreturn))
void requiref_fail(
		char const *expr,
		int code,
		char const *file,
		int line,
		char const *func,
		char const *fmt,
		...)
{
	int const errnum = errno;
	fprintf(stderr, "%s: %s:%d: %s: '%s' failed",
			program_invocation_name, file, line, func, expr);
	if (fmt != NULL)
	{
		fprintf(stderr, ": ");
		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	if (errnum != 0)
	{
		fprintf(stderr, ": %s", strerror(errnum));
	}
	fprintf(stderr, ".\n");
	exit(code);
}

#define requiref(expr, code, fmt, ...) \
	((expr) || (requiref_fail(#expr, code, __FILE__, __LINE__, __PRETTY_FUNCTION__, fmt, ##__VA_ARGS__), false))

#define require2(expr, code) requiref(expr, code, NULL)

#define require(expr) require2(expr, EXIT_FAILURE)

// returns the pid of a tracer, 0 if there is none
// the function will asplode if something is not right
pid_t get_tracer_pid(pid_t pid)
{
	pid_t tracepid = -1;
	char *sttspath = NULL;
	verify(-1 != asprintf(&sttspath, "/proc/%d/status", pid));
	FILE *sttsfile = fopen(sttspath, "r");
	assert(sttsfile != NULL);
	free(sttspath);
	char *line = NULL;
	size_t linesize;
	while (-1 != getline(&line, &linesize, sttsfile))
	{
		char const *const fieldnam = "TracerPid:";
		if (0 == strncmp(line, fieldnam, strlen(fieldnam)))
		{
			verify(1 == sscanf(line + strlen(fieldnam), "%d", &tracepid));
			break;
		}
	}
	require(tracepid != -1);
	free(line);
	verify(0 == fclose(sttsfile));
	return tracepid;
}

// wait for tracee to receive SIGSTOP
// pass all other signals on to tracee
void wait_until_tracee_stops(pid_t pid)
{
	while (true)
	{
		int status = 0;
		pid_t waitret = waitpid(pid, &status, 0);
		if (waitret != pid)
		{
			perror("waitpid()");
			exit(1);
		}
		if (!WIFSTOPPED(status))
		{
			fprintf(stderr, "Unhandled status change: %d\n", status);
			exit(1);
		}
		if (WSTOPSIG(status) == SIGSTOP)
		{
			return;
		}
		else
		{
			int signal = WSTOPSIG(status);
			fprintf(stderr, "Passing signal to child: %d\n", signal);
			long ptraceret = ptrace(PTRACE_CONT, pid, NULL, WSTOPSIG(status));
			if (ptraceret != 0)
			{
				perror("ptrace(PTRACE_CONT)");
				exit(1);
			}
		}
	}
}
