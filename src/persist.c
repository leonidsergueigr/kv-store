#include "percist.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PersistWriter {
  FILE *file;
  char *tmp_path;
  char *final_path;
};

struct PersistReader {
  FILE *file;
};

static char *persist_dup(const char *s) {
  size_t n = strlen(s) + 1;
  char *copy = malloc(n);
  if (copy == NULL)
    return NULL;
  memcpy(copy, s, n);
  return copy;
}

PersistWriter *persist_writer_open(const char *path) {
  if (path == NULL)
    return NULL;
  PersistWriter *w = malloc(sizeof *w);
  if (w == NULL)
    return NULL;
  size_t tmp_p_size = strlen(path) + 5;
  w->tmp_path = malloc(tmp_p_size);
  if (w->tmp_path == NULL) {
    free(w);
    return NULL;
  }
  snprintf(w->tmp_path, tmp_p_size, "%s.tmp", path);
  w->final_path = persist_dup(path);
  if (w->final_path == NULL) {
    free(w->tmp_path);
    free(w);
    return NULL;
  }
  w->file = fopen(w->tmp_path, "wb");
  if (w->file == NULL) {
    free(w->tmp_path);
    free(w->final_path);
    free(w);
    return NULL;
  }
  return w;
}

PersistStatus persist_write_pair(PersistWriter *w, const char *key,
                                 const char *value) {
  if (w == NULL || key == NULL || value == NULL) {
    return PERSIST_INVALID_ARGUMENT;
  }
  size_t k = strlen(key);
  size_t v = strlen(value);
  if (k > PERSIST_MAX_LEN || v > PERSIST_MAX_LEN) {
    return PERSIST_FORMAT_ERROR;
  }

  uint32_t key_len = (uint32_t)k;
  uint32_t value_len = (uint32_t)v;

  if (fwrite(&key_len, sizeof(key_len), 1, w->file) != 1) {
    return PERSIST_IO_ERROR;
  }
  if (fwrite(key, sizeof(char), key_len, w->file) != key_len) {
    return PERSIST_IO_ERROR;
  }

  if (fwrite(&value_len, sizeof(value_len), 1, w->file) != 1) {
    return PERSIST_IO_ERROR;
  }
  if (fwrite(value, sizeof(char), value_len, w->file) != value_len) {
    return PERSIST_IO_ERROR;
  }

  return PERSIST_OK;
}

PersistStatus persist_writer_commit(PersistWriter *w) {
  if (w == NULL)
    return PERSIST_INVALID_ARGUMENT;
  PersistStatus status = PERSIST_OK;
  if (fclose(w->file) == EOF) {
    status = PERSIST_IO_ERROR;
  } else if (rename(w->tmp_path, w->final_path) != 0) {
    status = PERSIST_IO_ERROR;
  }
  if (status != PERSIST_OK) {
    remove(w->tmp_path);
  }
  free(w->tmp_path);
  free(w->final_path);
  free(w);
  return status;
}

void persist_writer_abort(PersistWriter *w) {
  if (w == NULL) {
    return;
  }
  if (fclose(w->file) == EOF) {
    fprintf(stderr, "Error while closing the stream\n");
    free(w->tmp_path);
    free(w->final_path);
    free(w);
    return;
  }
  remove(w->tmp_path);
  free(w->tmp_path);
  free(w->final_path);
  free(w);
}

PersistReader *persist_reader_open(const char *path) {
  if (path == NULL)
    return NULL;
  PersistReader *r = malloc(sizeof *r);
  if (r == NULL)
    return NULL;
  r->file = fopen(path, "rb");
  if (r->file == NULL) {
    free(r);
    return NULL;
  }
  return r;
}

PersistStatus persist_read_pair(PersistReader *r, char **key, char **value) {
  if (r == NULL || key == NULL || value == NULL) {
    return PERSIST_INVALID_ARGUMENT;
  }

  uint32_t key_len = 0;
  uint32_t value_len = 0;

  size_t read_bytes = fread(&key_len, 1, sizeof(key_len), r->file);
  if (read_bytes == 0) {
    if (ferror(r->file)) {
      return PERSIST_IO_ERROR;
    }
    if (feof(r->file)) {
      return PERSIST_END;
    }
    return PERSIST_IO_ERROR;
  }

  if (read_bytes != sizeof(key_len)) {
    return PERSIST_FORMAT_ERROR;
  }

  if (key_len > PERSIST_MAX_LEN) {
    return PERSIST_FORMAT_ERROR;
  }

  char *k_buff = malloc((size_t)key_len + 1);
  if (k_buff == NULL)
    return PERSIST_MEMORY_ERROR;

  if (fread(k_buff, sizeof(char), key_len, r->file) != key_len) {
    free(k_buff);
    if (ferror(r->file)) {
      return PERSIST_IO_ERROR;
    }
    if (feof(r->file)) {
      return PERSIST_FORMAT_ERROR;
    }
    return PERSIST_IO_ERROR;
  }
  k_buff[key_len] = '\0';

  if (fread(&value_len, sizeof(value_len), 1, r->file) != 1) {
    free(k_buff);
    if (ferror(r->file)) {
      return PERSIST_IO_ERROR;
    }
    if (feof(r->file)) {
      return PERSIST_FORMAT_ERROR;
    }
    return PERSIST_IO_ERROR;
  }

  if (value_len > PERSIST_MAX_LEN) {
    free(k_buff);
    return PERSIST_FORMAT_ERROR;
  }

  char *v_buff = malloc((size_t)value_len + 1);
  if (v_buff == NULL) {
    free(k_buff);
    return PERSIST_MEMORY_ERROR;
  }

  if (fread(v_buff, sizeof(char), value_len, r->file) != value_len) {
    free(k_buff);
    free(v_buff);
    if (ferror(r->file)) {
      return PERSIST_IO_ERROR;
    }
    if (feof(r->file)) {
      return PERSIST_FORMAT_ERROR;
    }
    return PERSIST_IO_ERROR;
  }
  v_buff[value_len] = '\0';

  *key = k_buff;
  *value = v_buff;

  return PERSIST_OK;
}

void persist_reader_close(PersistReader *r) {
  if (r == NULL)
    return;
  if (fclose(r->file) == EOF) {
    free(r);
    fprintf(stderr, "Error while closing the stream\n");
    return;
  }
  free(r);
}
