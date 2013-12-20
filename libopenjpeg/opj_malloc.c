/*
 * opj_malloc.c
 *
 *  Created on: 11/11/2013
 *      Author: Bart Milne (milne.bje@gmail.com)
 *
 *  PURPOSE: A replacement for the macro-based memory allocation functions in
 *  opj_malloc.h . This eases debugging and development.
 */

#include "opj_malloc.h"

/*
 * OpenJPEG malloc, calloc, realloc and free functions. Works in the same way
 * as the library functions in <stdlib.h>.
 */
void * opj_malloc (size_t size)
{
	return malloc(size);
}

void * opj_calloc (size_t num, size_t size)
{
	return calloc(num, size);
}

void* opj_realloc (void * ptr, size_t size)
{
	return realloc(ptr, size);
}

void opj_free (void* ptr)
{
	free(ptr);
}


/*
 * Aligned malloc. This is intended to align memory on 16-byte boundaries.
 * Note that the exact function for both allocating and freeing used depends
 * on the operating system environment.
 * Please note this code has ONLY been tested for when HAVE_MEMALIGN macro is
 * active. The other cases have not been tested.
 */
void* __attribute__ ((malloc)) opj_aligned_malloc(size_t size)
{
	void *mem = NULL;

#ifdef HAVE_MM_MALLOC
	mem = _mm_malloc(size, OPJ_MM_MALLOC_BYTE_ALIGN);

#elif defined (HAVE_MEMALIGN)
	mem = memalign(OPJ_MM_MALLOC_BYTE_ALIGN, size);

#elif defined (HAVE_POSIX_MEMALIGN)
	int rval;
	rval = posix_memalign(&mem, OPJ_MM_MALLOC_BYTE_ALIGN, size);
	if (rval){
		mem = NULL;
	}

#else
	mem = malloc(size);
#endif

	return mem;
}

void opj_aligned_free(void *ptr)
{
#ifdef HAVE_MM_MALLOC
	_mm_free(ptr);
#else
	free(ptr);
#endif
}


/***** END OF FILE *****/

