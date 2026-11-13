#ifndef PERSIST_H
#define PERSIST_H

#include <stdbool.h>
#define PERSIST_MAX_LEN (1u << 20)

typedef struct PersistWriter PersistWriter;
typedef struct PersistReader PersistReader;

typedef enum {
  PERSIST_OK,
  PERSIST_IO_ERROR,
  PERSIST_FORMAT_ERROR,
  PERSIST_MEMORY_ERROR,
  PERSIST_END,
  PERSIST_INVALID_ARGUMENT
} PersistStatus;

PersistWriter *persist_writer_open(const char *path);
PersistStatus persist_write_pair(PersistWriter *w, const char *key,
                                 const char *value);
PersistStatus persist_writer_commit(PersistWriter *w);
void persist_writer_abort(PersistWriter *w);

PersistReader *persist_reader_open(const char *path);
PersistStatus persist_read_pair(PersistReader *r, char **key, char **value);
void persist_reader_close(PersistReader *r);

#endif
