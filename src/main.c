#include <stdio.h>
#include <string.h>

#if defined(_WIN32) && defined(_UNICODE)
	#include <fcntl.h>
	#include <io.h>
#endif

#include "argparser.h"
#include "constants.h"
#include "errors.h"
#include "fileinfo.h"
#include "filesystem.h"
#include "fstream.h"
#include "os.h"
#include "reverse_memcpy.h"
#include "revf.h"
#include "stringu.h"
#include "terminal.h"
#include "walkdir.h"

static const char* temporary_directory = NULL;

static int file_reverse(const char* const filename) {
	
	struct FStream* source_stream = fstream_open(filename, FSTREAM_READ);
	
	if (source_stream == NULL) {
		const struct SystemError error = get_system_error();
		fprintf(stderr, "fatal error: could not open file at '%s': %s\r\n", filename, error.message);
		
		return -1;
	}
	
	if (fstream_seek(source_stream, 0, FSTREAM_SEEK_END) == -1) {
		const struct SystemError error = get_system_error();
		fprintf(stderr, "fatal error: could not seek file at '%s': %s\r\n", filename, error.message);
		
		fstream_close(source_stream);
		
		return -1;
	}
	
	long int file_size = fstream_tell(source_stream);
	
	if (file_size == -1) {
		const struct SystemError error = get_system_error();
		fprintf(stderr, "fatal error: could not get current file position of '%s': %s\r\n", filename, error.message);
		
		fstream_close(source_stream);
		
		return -1;
	}
	
	if (fstream_seek(source_stream, 0, FSTREAM_SEEK_BEGIN) == -1) {
		const struct SystemError error = get_system_error();
		fprintf(stderr, "fatal error: could not seek file at '%s': %s\r\n", filename, error.message);
		
		fstream_close(source_stream);
		
		return -1;
	}
	
	const char* const name = basename(filename);
	
	char temporary_file[strlen(temporary_directory) + strlen(PATH_SEPARATOR) + strlen(name) + 1];
	strcpy(temporary_file, temporary_directory);
	strcat(temporary_file, PATH_SEPARATOR);
	strcat(temporary_file, name);
	
	struct FStream* destination_stream = fstream_open(temporary_file, FSTREAM_WRITE);
	
	if (destination_stream == NULL) {
		const struct SystemError error = get_system_error();
		fprintf(stderr, "fatal error: could not create file at '%s': %s\r\n", temporary_file, error.message);
		
		fstream_close(source_stream);
		
		return -1;
	}
	
	char chunk[8192];
	char reverse_chunk[8192];
	
	while (file_size != 0) {
		size_t rsize = sizeof(chunk);
		
		file_size -= rsize;
		
		if (file_size < 0) {
			rsize -= (size_t) abs((int) file_size);
			file_size -= file_size;
		}
		
		if (fstream_seek(source_stream, file_size, FSTREAM_SEEK_BEGIN) == -1) {
			const struct SystemError error = get_system_error();
			fprintf(stderr, "fatal error: could not seek file at '%s': %s\r\n", filename, error.message);
			
			fstream_close(source_stream);
			fstream_close(destination_stream);
			
			return -1;
		}
		
		const ssize_t size = fstream_read(source_stream, chunk, rsize);
		
		if (size == -1) {
			const struct SystemError error = get_system_error();
			fprintf(stderr, "fatal error: could not read contents of file at '%s': %s\r\n", filename, error.message);
			
			fstream_close(source_stream);
			fstream_close(destination_stream);
			
			return -1;
		}
		
		reverse_memcpy(reverse_chunk, chunk, rsize);
		
		const int status = fstream_write(destination_stream, reverse_chunk, rsize);
		
		if (status == -1) {
			const struct SystemError error = get_system_error();
			fprintf(stderr, "fatal error: could not write to file at '%s': %s\r\n", temporary_file, error.message);
			
			fstream_close(source_stream);
			fstream_close(destination_stream);
			
			return -1;
		}
	}
	
	fstream_close(source_stream);
	fstream_close(destination_stream);
	
	source_stream = NULL;
	destination_stream = NULL;
	
	 if (move_file(temporary_file, filename) == -1) {
		const struct SystemError error = get_system_error();
		fprintf(stderr, "fatal error: could not move file from '%s' to '%s': %s\r\n", temporary_file, filename, error.message);
		
		return -1;
	}
	
	return 0;
	
}

static int directory_reverse(const char* const directory) {
	
	struct WalkDir walkdir = {0};
	
	if (walkdir_init(&walkdir, directory) == -1) {
		return -1;
	}
	
	while (1) {
		const struct WalkDirItem* const item = walkdir_next(&walkdir);
		
		if (item == NULL) {
			break;
		}
		
		if (strcmp(item->name, ".") == 0 || strcmp(item->name, "..") == 0) {
			continue;
		}
		
		char path[strlen(directory) + strlen(PATH_SEPARATOR) + strlen(item->name) + 1];
		strcpy(path, directory);
		strcat(path, PATH_SEPARATOR);
		strcat(path, item->name);
		
		switch (item->type) {
			case WALKDIR_ITEM_DIRECTORY: {
				if (directory_reverse(path) == -1) {
					walkdir_free(&walkdir);
					return -1;
				}
				
				break;
			}
			case WALKDIR_ITEM_FILE:
			case WALKDIR_ITEM_UNKNOWN: {
				if (file_reverse(path) == -1) {
					walkdir_free(&walkdir);
					return -1;
				}
				
				break;
			}
		}
	}
	
	walkdir_free(&walkdir);
	
	return 0;
	
}

int main(int argc, argv_t* argv[]) {
	
	#if defined(_WIN32) && defined(_UNICODE)
		_setmode(_fileno(stdout), _O_WTEXT);
		_setmode(_fileno(stderr), _O_WTEXT);
		_setmode(_fileno(stdin), _O_WTEXT);
	#endif
	
	if (!is_atty(stdin)) {
		fprintf(stderr, "fatal error: will not read from standard input\r\n");
		return EXIT_FAILURE;
	}
	
	if (argc == 1) {
		fprintf(stderr, "%s\n", REVF_DESCRIPTION);
		return EXIT_FAILURE;
	}
	
	temporary_directory = get_temporary_directory();
	
	if (temporary_directory == NULL) {
		const struct SystemError error = get_system_error();
		fprintf(stderr, "fatal error: could not get temporary directory: %s\r\n", error.message);
		
		return EXIT_FAILURE;
	}
	
	int recursive = 0;
	
	struct ArgumentParser argparser = {0};
	argparser_init(&argparser, argc, argv);
	
	while (1) {
		const struct Argument* const argument = argparser_next(&argparser);
		
		if (argument == NULL) {
			break;
		}
		
		if (strcmp(argument->key, "r") == 0 || strcmp(argument->key, "recursive") == 0) {
			recursive = 1;
		} else if (strcmp(argument->key, "v") == 0 || strcmp(argument->key, "version") == 0) {
			printf("%s v%s (+%s)\n", REVF_NAME, REVF_VERSION, REVF_REPOSITORY);
			return EXIT_SUCCESS;
		} else if (strcmp(argument->key, "h") == 0 || strcmp(argument->key, "help") == 0) {
			printf("%s\n", REVF_DESCRIPTION);
			return EXIT_SUCCESS;
		} else {
			const char* const path = argument->key;
			
			struct FileInfo info = {0};
			
			if (get_file_info(&info, path) == -1) {
				const struct SystemError error = get_system_error();
				fprintf(stderr, "fatal error: could not stat file at '%s': %s\n", path, error.message);
				
				return EXIT_FAILURE;
			}
			
			switch (info.type) {
				case FILEINFO_FILE:
				case FILEINFO_FILE_LINK: {
					if (file_reverse(path) == -1) {
						return EXIT_FAILURE;
					}
					
					break;
				}
				case FILEINFO_DIRECTORY:
				case FILEINFO_DIRECTORY_LINK: {
					if (!recursive) {
						fprintf(stderr, "fatal error: refusing to recurse down into directory '%s'\n", path);
						return EXIT_FAILURE;
					}
					
					if (directory_reverse(path) == -1) {
						return EXIT_FAILURE;
					}
					
					break;
				}
			}
		}
	}
	
	return EXIT_SUCCESS;
	
}