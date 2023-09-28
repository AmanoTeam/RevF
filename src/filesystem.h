int remove_file(const char* const filename);
int move_file(const char* const source, const char* const destination);
int copy_file(const char* const source, const char* const destination);

#if defined(_WIN32)
	int is_absolute(const char* const path);
#endif