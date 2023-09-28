#include <time.h>

struct FileID {
	unsigned long int device;
	unsigned long int file;
};

enum FileInfoType {
	FILEINFO_FILE,
	FILEINFO_FILE_LINK,
	FILEINFO_DIRECTORY,
	FILEINFO_DIRECTORY_LINK
};

struct FileInfo {
	struct FileID id;
	enum FileInfoType type;
	long int size;
	long int total_links;
	time_t last_access_time;
	time_t last_write_time;
	time_t creation_time;
	long int block_size;
	int permissions;
	int is_special;
};

#define FILEPERMISSION_USER_EXEC 0x00000001
#define FILEPERMISSION_USER_WRITE 0x00000002
#define FILEPERMISSION_USER_READ 0x00000004
#define FILEPERMISSION_GROUP_EXEC 0x00000010
#define FILEPERMISSION_GROUP_WRITE 0x00000020
#define FILEPERMISSION_GROUP_READ 0x00000040
#define FILEPERMISSION_OTHERS_EXEC 0x00000080
#define FILEPERMISSION_OTHERS_WRITE 0x00000100
#define FILEPERMISSION_OTHERS_READ 0x00000200

int get_file_info(struct FileInfo* const file_info, const char* const name);

#pragma once
