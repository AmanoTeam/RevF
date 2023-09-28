static const char EQUAL[] = "=";

#if defined(_WIN32)
	#define PATH_SEPARATOR "\\"
#else
	#define PATH_SEPARATOR "/"
#endif

#if defined(_WIN32) && defined(_UNICODE)
	static const wchar_t WIN10LP_PREFIX[] = L"\\\\?\\";
#endif

#if defined(_WIN32)
	static const char ASTERISK[] = "*";
	static const char COLON[] = ":";
#endif

#pragma once
