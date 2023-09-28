#include <stdio.h>

#if defined(_WIN32)
	#include <io.h>
#endif

#if !defined(_WIN32)
	#include <unistd.h>
#endif

#if defined(_WIN32)
	#define fileno _fileno
	#define isatty _isatty
#endif

#include "terminal.h"

int is_atty(FILE* const file) {
	
	const int fd = fileno(file);
	
	if (fd == -1) {
		return -1;
	}
	
	const int status = isatty(fd);
	
	return status;
	
}