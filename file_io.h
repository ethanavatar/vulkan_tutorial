#ifndef FILE_IO_H
#define FILE_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef _WIN32
#   define fileno _fileno
#endif

size_t file_size(FILE *file);
int read_all_bytes(FILE *file, char *buffer, size_t bytes);
const char *read_entire_file(const char *filename, size_t *size);

#endif // FILE_IO_H

#ifdef FILE_IO_IMPLEMENTATION

size_t file_size(FILE *file) {
	struct stat file_stat;
	if (fstat(fileno(file), &file_stat) != 0) return 0;
	return file_stat.st_size;
}

int read_all_bytes(FILE *const file, char *buffer, size_t bytes) {
	if (!file) return 0;

	size_t read_bytes = fread(buffer, sizeof(char), bytes, file);
	if (read_bytes != bytes) {
		fprintf(stderr, "Error reading file\n");
		return 0;
	}
	return 1;
}

const char *read_entire_file(const char *filename, size_t *bytes_read) {
 	FILE *file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Error opening file %s\n", filename);
		return NULL;
	}

	size_t size = file_size(file);
	if (bytes_read) *bytes_read = size;

	char *buffer = malloc(size + 1);
	if (!buffer) {
		fprintf(stderr, "Error allocating memory\n");
		fclose(file);
		return NULL;
	}

	if (!read_all_bytes(file, buffer, size)) {
		free(buffer);
		fclose(file);
		return NULL;
	}

	buffer[size] = '\0';
	fclose(file);
	return buffer;
}

#endif // FILE_IO_IMPLEMENTATION
