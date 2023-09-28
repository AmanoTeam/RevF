#if defined(_WIN32)
	#include <windows.h>
	#include <math.h>
#endif

#if !defined(_WIN32)
	#include <sys/stat.h>
#endif

#if defined(_WIN32)
	#include "constants.h"
#endif

#include "filesystem.h"
#include "fileinfo.h"

#if defined(_WIN32)
	// The number of hectonanoseconds between 1601/01/01 (windows epoch) and 1970/01/01 (unix epoch).
	static const unsigned long long WINDOWS_EPOCH_DIFF = 116444736000000000;
	
	// 100 hectonanoseconds
	static const int WINDOWS_HNSECS_PER_SEC = 10000000;
#endif

int get_file_info(struct FileInfo* const file_info, const char* const name) {
	
	#if defined(_WIN32)
		DWORD sectors = 0;
		DWORD bytes = 0;
		DWORD free_clusters = 0;
		DWORD clusters = 0;
		
		const int is_abs = is_absolute(name);
		
		#if defined(_UNICODE)
			const int wnames = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
			
			if (wnames == 0) {
				return -1;
			}
			
			wchar_t wname[(is_abs ? wcslen(WIN10LP_PREFIX) : 0) + wnames];
			
			if (is_abs) {
				wcscpy(wname, WIN10LP_PREFIX);
			}
			
			if (MultiByteToWideChar(CP_UTF8, 0, name, -1, wname + (is_abs ? wcslen(WIN10LP_PREFIX) : 0), wnames) == 0) {
				return -1;
			}
			
			wchar_t wdrive[4] = {0};
			
			if (is_abs) {
				wcsncpy(wdrive, wname + wcslen(WIN10LP_PREFIX), 3);
			}
			
			wdrive[3] = L'\0';
			
			const wchar_t* const root = (is_abs ? wdrive : NULL);
			
			if (GetDiskFreeSpaceW(root, &sectors, &bytes, &free_clusters, &clusters) == 0) {
				return -1;
			}
			
			const HANDLE handle = CreateFileW(
				wname,
				0,
				FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL,
				0
			);
		#else
			char drive[4] = {0};
			
			if (is_abs) {
				strncpy(drive, name, 3);
			}
			
			drive[3] = '\0';
			
			const char* const root = (is_abs ? drive : NULL);
			
			if (GetDiskFreeSpaceA(root, &sectors, &bytes, &free_clusters, &clusters) == 0) {
				return -1;
			}
			
			const HANDLE handle = CreateFileA(
				name,
				0,
				FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL,
				0
			);
		#endif
		
		if (handle == INVALID_HANDLE_VALUE) {
			return -1;
		}
		
		BY_HANDLE_FILE_INFORMATION info = {0};
		
		if (GetFileInformationByHandle(handle, &info) == 0) {
			return -1;
		}
		
		if (CloseHandle(handle) == 0) {
			return -1;
		}
		
		file_info->id.device = info.dwVolumeSerialNumber;
		file_info->id.file = (info.nFileIndexLow | (unsigned long long) info.nFileIndexHigh << 32);
		file_info->size = (info.nFileSizeLow | (unsigned long long) info.nFileSizeHigh << 32);
		file_info->total_links = info.nNumberOfLinks;
		file_info->last_access_time = floor(((info.ftLastAccessTime.dwLowDateTime | (unsigned long long) info.ftLastAccessTime.dwHighDateTime << 32) - WINDOWS_EPOCH_DIFF) / WINDOWS_HNSECS_PER_SEC);
		file_info->last_write_time = floor(((info.ftLastWriteTime.dwLowDateTime | (unsigned long long) info.ftLastWriteTime.dwHighDateTime << 32) - WINDOWS_EPOCH_DIFF) / WINDOWS_HNSECS_PER_SEC);
		file_info->creation_time = floor(((info.ftCreationTime.dwLowDateTime | (unsigned long long) info.ftCreationTime.dwHighDateTime << 32) - WINDOWS_EPOCH_DIFF) / WINDOWS_HNSECS_PER_SEC);
		file_info->block_size = sectors * bytes;
		file_info->is_special = 0;
		
		file_info->permissions = (
			FILEPERMISSION_USER_EXEC | FILEPERMISSION_USER_READ | FILEPERMISSION_GROUP_EXEC |
			FILEPERMISSION_GROUP_READ | FILEPERMISSION_OTHERS_EXEC | FILEPERMISSION_OTHERS_READ
		);
		
		if ((info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0) {
			file_info->permissions |= FILEPERMISSION_USER_WRITE | FILEPERMISSION_GROUP_WRITE | FILEPERMISSION_OTHERS_WRITE;
		}
		
		file_info->type = ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) ? FILEINFO_DIRECTORY : FILEINFO_FILE;
		
		if ((info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
			file_info->type++;
		}
	#else
		struct stat st = {0};
		
		if (lstat(name, &st) == -1) {
			return -1;
		}
		
		file_info->id.device = st.st_dev;
		file_info->id.file = st.st_ino;
		file_info->size = st.st_size;
		file_info->total_links = st.st_nlink;
		
		#if defined(__APPLE__)
			file_info->last_access_time = st.st_atimespec.tv_sec;
			file_info->last_write_time = st.st_mtimespec.tv_sec;
			file_info->creation_time = st.st_ctimespec.tv_sec;
		#else
			file_info->last_access_time = st.st_atim.tv_sec;
			file_info->last_write_time = st.st_mtim.tv_sec;
			file_info->creation_time = st.st_ctim.tv_sec;
		#endif
		
		file_info->block_size = st.st_blksize;
		file_info->is_special = 0;
		
		if ((st.st_mode & S_IRUSR) != 0) {
			file_info->permissions |= FILEPERMISSION_USER_READ;
		}
		
		if ((st.st_mode & S_IWUSR) != 0) {
			file_info->permissions |= FILEPERMISSION_USER_WRITE;
		}
		
		if ((st.st_mode & S_IXUSR) != 0) {
			file_info->permissions |= FILEPERMISSION_USER_EXEC;
		}
		
		if ((st.st_mode & S_IRGRP) != 0) {
			file_info->permissions |= FILEPERMISSION_GROUP_READ;
		}
		
		if ((st.st_mode & S_IWGRP) != 0) {
			file_info->permissions |= FILEPERMISSION_GROUP_WRITE;
		}
		
		if ((st.st_mode & S_IXGRP) != 0) {
			file_info->permissions |= FILEPERMISSION_GROUP_EXEC;
		}
		
		if ((st.st_mode & S_IROTH) != 0) {
			file_info->permissions |= FILEPERMISSION_OTHERS_READ;
		}
		
		if ((st.st_mode & S_IWOTH) != 0) {
			file_info->permissions |= FILEPERMISSION_OTHERS_WRITE;
		}
		
		if ((st.st_mode & S_IXOTH) != 0) {
			file_info->permissions |= FILEPERMISSION_OTHERS_EXEC;
		}
		
		if (S_ISDIR(st.st_mode)) {
			file_info->type = FILEINFO_DIRECTORY;
		} else if (S_ISLNK(st.st_mode)) {
			if (stat(name, &st) == -1) {
				return -1;
			}
			
			file_info->type = (S_ISDIR(st.st_mode) ? FILEINFO_DIRECTORY_LINK : FILEINFO_FILE_LINK);
		} else {
			file_info->type = FILEINFO_FILE;
		}
		
		if ((file_info->type == FILEINFO_FILE || file_info->type == FILEINFO_FILE_LINK) && !S_ISREG(st.st_mode)) {
			file_info->is_special = 1;
		}
	#endif
	
	return 0;
	
}
