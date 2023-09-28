#include <stdlib.h>

#include "reverse_memcpy.h"

char* reverse_memcpy(char* const destination, const char* const source, const size_t num) {
	
	for (size_t index = num; index > 0; index--) {
		destination[num - index] = source[index - 1];
	}
	
	return destination;
	
}
/*
int main() {
	
	char a[] = "sexo";
	char b[400] = {0};
	
	reverse_memcpy(b, a, strlen(a));
	puts(b);
}
*/