#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
	#include <windows.h>
#endif

#include "constants.h"
#include "os.h"

char* get_temporary_directory(void) {
	/*
	Returns the temporary directory of the current user for applications to
	save temporary files in.
	
	On Windows, it calls GetTempPath().
	On Posix based platforms, it will check "TMPDIR", "TEMP", "TMP" and "TEMPDIR" environment variables in order.
	
	Returns NULL on error.
	*/
	
	#if defined(_WIN32)
		#if defined(_UNICODE)
			const size_t wdirectorys = (size_t) GetTempPathW(0, NULL);
			
			if (wdirectorys == 0) {
				return NULL;
			}
			
			wchar_t wdirectory[wdirectorys + 1];
			
			const DWORD code = GetTempPathW((DWORD) (sizeof(wdirectory) / sizeof(*wdirectory)), wdirectory);
			
			if (code == 0) {
				return 0;
			}
			
			const int directorys = WideCharToMultiByte(CP_UTF8, 0, wdirectory, -1, NULL, 0, NULL, NULL);
			
			if (directorys == 0) {
				return NULL;
			}
			
			char* temporary_directory = malloc((size_t) directorys);
			
			if (temporary_directory == NULL) {
				return NULL;
			}
			
			if (WideCharToMultiByte(CP_UTF8, 0, wdirectory, -1, temporary_directory, directorys, NULL, NULL) == 0) {
				return NULL;
			}
		#else
			const size_t directorys = (size_t) GetTempPathA(0, NULL);
			
			if (directorys == 0) {
				return NULL;
			}
			
			char directory[directorys + 1];
			const DWORD code = GetTempPathA((DWORD) sizeof(directory), directory);
			
			if (code == 0) {
				return 0;
			}
			
			char* temporary_directory = malloc(sizeof(directory));
			
			if (temporary_directory == NULL) {
				return NULL;
			}
			
			strcpy(temporary_directory, directory);
		#endif
	#else
		const char* const keys[] = {
			"TMPDIR",
			"TEMP",
			"TMP",
			"TEMPDIR"
		};
		
		for (size_t index = 0; index < (sizeof(keys) / sizeof(*keys)); index++) {
			const char* const key = keys[index];
			const char* const value = getenv(key);
			
			if (value == NULL) {
				continue;
			}
			
			char* temporary_directory = malloc(strlen(value) + 1);
			
			if (temporary_directory == NULL) {
				return NULL;
			}
			
			strcpy(temporary_directory, value);
			
			return temporary_directory;
		}
		
		const char* const tmp = "/tmp";
		
		char* temporary_directory = malloc(strlen(tmp) + 1);
		
		if (temporary_directory == NULL) {
			return NULL;
		}
		
		strcpy(temporary_directory, tmp);
	#endif
	
	// Strip trailing path separator
	if (strlen(temporary_directory) > 1) {
		char* ptr = strchr(temporary_directory, '\0') - 1;
		
		if (*ptr == *PATH_SEPARATOR) {
			*ptr = '\0';
		}
	}
	
	return temporary_directory;
	
}
