// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.
// simple memory allocation functions for requesting memory from the system.

void *memory_allocate(size_t size)
{
	return malloc(size);
}

void memory_deallocate(void *ptr)
{
	free(ptr);
}

void *memory_reallocate(void *ptr, size_t size, bool keep_contents = true)
{
	if(!keep_contents || !ptr)
	{
		free(ptr);
		return malloc(size);
	}
	return realloc(ptr, size);
}