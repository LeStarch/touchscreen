#include <stdlib.h>
/**
 * Safely malloc a length of memory
 * len - length of memory segment to allocate
 */
void* safe_malloc(size_t len) {
	int i = 0;
	for (i = 0; i < 3; i++) {
		void* mem = malloc(len);
		if (mem != NULL) {
			return mem;
		}
	}
	return NULL;
}
