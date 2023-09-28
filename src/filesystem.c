#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
	#include <windows.h>
#endif

#if defined(__APPLE__)
	#include <sys/param.h>
	#include <copyfile.h>
#endif

#if !defined(_WIN32)
	#include <unistd.h>
	#include <sys/stat.h>
	#include <errno.h>
#endif

#include "fstream.h"
#include "constants.h"
#include "filesystem.h"

#if defined(_WIN32)
	int is_absolute(const char* const path) {
		/*
		Checks whether a given path is absolute.
		
		On Windows, network paths are considered absolute too.
		*/
		
		#if defined(_WIN32)
			return (*path == *PATH_SEPARATOR || (strlen(path) > 1 && isalpha(*path) && path[1] == *COLON));
		#else
			return (*path == *PATH_SEPARATOR);
		#endif
		
	}
#endif

int remove_file(const char* const filename) {
	/*
	Removes a file from disk.
	
	On Windows, ignores the read-only attribute.
	This does not fail if the file never existed in the first place.
	
	Returns (0) on success, (-1) on error.
	*/
	
	#if defined(_WIN32)
		#if defined(_UNICODE)
			const int is_abs = is_absolute(filename);
			
			const int wfilenames = MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
			
			if (wfilenames == 0) {
				return -1;
			}
			
			wchar_t wfilename[(is_abs ? wcslen(WIN10LP_PREFIX) : 0) + wfilenames];
			
			if (is_abs) {
				wcscpy(wfilename, WIN10LP_PREFIX);
			}
			
			if (MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfilename + (is_abs ? wcslen(WIN10LP_PREFIX) : 0), wfilenames) == 0) {
				return -1;
			}
			
			const BOOL status = DeleteFileW(wfilename);
		#else
			const BOOL status = DeleteFileA(filename);
		#endif
		
		if (status == 0) {
			if (GetLastError() == ERROR_ACCESS_DENIED) {
				#if defined(_UNICODE)
					const BOOL status = SetFileAttributesW(wfilename, FILE_ATTRIBUTE_NORMAL);
				#else
					const BOOL status = SetFileAttributesA(filename, FILE_ATTRIBUTE_NORMAL);
				#endif
				
				if (status == 1) {
					#if defined(_UNICODE)
						const BOOL status = DeleteFileW(wfilename);
					#else
						const BOOL status = DeleteFileA(filename);
					#endif
					
					if (status == 1) {
						return 0;
					}
				}
			} else if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND) {
				return 0;
			}
			
			return -1;
		}
	#else
		if (unlink(filename) == -1) {
			if (errno == ENOENT) {
				return 0;
			}
			
			return -1;
		}
	#endif
	
	return 0;
	
}

int copy_file(const char* const source, const char* const destination) {
	/*
	Copies a file from source to destination.
	
	On the Windows platform this will copy the source file's attributes into destination.
	On Mac OS X, copyfile() C API will be used (available since OS X 10.5).
	
	If destination already exists, the file attributes will be preserved and the content overwritten.
	
	Returns (0) on success, (-1) on error.
	*/
	
	#if defined(_WIN32)
		#if defined(_UNICODE)
			int is_abs = is_absolute(source);
			
			const int wsources = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
			
			if (wsources == 0) {
				return -1;
			}
			
			wchar_t wsource[(is_abs ? wcslen(WIN10LP_PREFIX) : 0) + wsources];
			
			if (is_abs) {
				wcscpy(wsource, WIN10LP_PREFIX);
			}
			
			if (MultiByteToWideChar(CP_UTF8, 0, source, -1, wsource + (is_abs ? wcslen(WIN10LP_PREFIX) : 0), wsources) == 0) {
				return -1;
			}
			
			is_abs = is_absolute(destination);
			
			const int wdestinations = MultiByteToWideChar(CP_UTF8, 0, destination, -1, NULL, 0);
			
			if (wdestinations == 0) {
				return -1;
			}
			
			wchar_t wdestination[(is_abs ? wcslen(WIN10LP_PREFIX) : 0) + wdestinations];
			
			if (is_abs) {
				wcscpy(wdestination, WIN10LP_PREFIX);
			}
			
			if (MultiByteToWideChar(CP_UTF8, 0, destination, -1, wdestination + (is_abs ? wcslen(WIN10LP_PREFIX) : 0), wdestinations) == 0) {
				return -1;
			}
			
			if (CopyFileW(wsource, wdestination, FALSE) == 0) {
				return -1;
			}
		#else
			if (CopyFileA(source, destination, FALSE) == 0) {
				return -1;
			}
		#endif
	#elif defined(__APPLE__)
		copyfile_state_t state = copyfile_state_alloc();
		
		const int status = copyfile(source, destination, state, COPYFILE_DATA);
		const int status2 = copyfile_state_free(state);
		
		if (status != 0 || status2 != 0) {
			return -1;
		}
	#else
		// Generic version which works for any platform
		struct FStream* const istream = fstream_open(source, FSTREAM_READ);
		
		if (istream == NULL) {
			return -1;
		}
		
		struct FStream* const ostream = fstream_open(destination, FSTREAM_WRITE);
		
		if (ostream == NULL) {
			fstream_close(istream);
			return -1;
		}
		
		char chunk[8192] = {'\0'};
		
		while (1) {
			const ssize_t size = fstream_read(istream, chunk, sizeof(chunk));
			
			if (size == -1) {
				fstream_close(istream);
				fstream_close(ostream);
				return -1;
			}
			
			if (size == 0) {
				if (fstream_close(istream) == -1 || fstream_close(ostream) == -1) {
					return -1;
				}
				break;
			}
			
			const int status = fstream_write(ostream, chunk, (size_t) size);
			
			if (status == -1) {
				fstream_close(istream);
				fstream_close(ostream);
				return -1;
			}
		}
	#endif
	
	return 0;

}

int move_file(const char* const source, const char* const destination) {
	/*
	Moves a file from source to destination.
	
	Symlinks are not followed: if source is a symlink, it is itself moved, not it's target.
	If destination already exists, it will be overwritten.
	
	Returns (0) on success, (-1) on error.
	*/
	
	#if defined(_WIN32)
		#if defined(_UNICODE)
			int is_abs = is_absolute(source);
			
			const int wsources = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
			
			if (wsources == 0) {
				return -1;
			}
			
			wchar_t wsource[(is_abs ? wcslen(WIN10LP_PREFIX) : 0) + wsources];
			
			if (is_abs) {
				wcscpy(wsource, WIN10LP_PREFIX);
			}
			
			if (MultiByteToWideChar(CP_UTF8, 0, source, -1, wsource + (is_abs ? wcslen(WIN10LP_PREFIX) : 0), wsources) == 0) {
				return -1;
			}
			
			is_abs = is_absolute(destination);
			
			const int wdestinations = MultiByteToWideChar(CP_UTF8, 0, destination, -1, NULL, 0);
			
			if (wdestinations == 0) {
				return -1;
			}
			
			wchar_t wdestination[(is_abs ? wcslen(WIN10LP_PREFIX) : 0) + wdestinations];
			
			if (is_abs) {
				wcscpy(wdestination, WIN10LP_PREFIX);
			}
			
			if (MultiByteToWideChar(CP_UTF8, 0, destination, -1, wdestination + (is_abs ? wcslen(WIN10LP_PREFIX) : 0), wdestinations) == 0) {
				return -1;
			}
			
			const BOOL status = MoveFileExW(wsource, wdestination, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
		#else
			const BOOL status = MoveFileExA(source, destination, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
		#endif
		
		if (status == 0) {
			if (GetLastError() == ERROR_ACCESS_DENIED) {
				if (copy_file(source, destination) == -1) {
					return -1;
				}
				
				if (remove_file(source) == 0) {
					return 0;
				}
			}
			
			return -1;
		}
	#else
		if (rename(source, destination) == -1) {
			if (errno == EXDEV) {
				if (copy_file(source, destination) == -1) {
					return -1;
				}
				
				if (remove_file(source) == 0) {
					return 0;
				}
			}
			
			return -1;
		}
	#endif
	
	return 0;
	
}
