/**
 * File:   fs_os_spiffs.c
 * Author: AWTK Develop Team
 * Brief:  spiffs implemented fs
 *
 * Copyright (c) 2020 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2020-05-17 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif /*WIN32_LEAN_AND_MEAN*/

#include "tkc/fs.h"
#include "spiffs/spiffs.h"
#include "tkc/mem.h"
#include "tkc/utils.h"
#include <stdarg.h>
#include "fs_mt.h"

#if defined(LINUX) || defined(WIN32) || defined(MACOS) || defined(HAS_STDIO)
#include <stdio.h>
#else
extern int vsnprintf(char* s, size_t n, const char* format, va_list arg);
#endif

static spiffs* sfs = NULL;

typedef struct _fs_file_spiffs_t {
  fs_file_t fs_file;
  spiffs_file file;
} fs_file_spiffs_t;

static int32_t fs_os_file_read(fs_file_t* file, void* buffer, uint32_t size) {
  spiffs_file fp = (((fs_file_spiffs_t*)file)->file);

  return SPIFFS_read(sfs, fp, buffer, size);
}

int32_t fs_os_file_write(fs_file_t* file, const void* buffer, uint32_t size) {
  spiffs_file fp = (((fs_file_spiffs_t*)file)->file);

  return SPIFFS_write(sfs, fp, (void*)buffer, size);
}

int32_t fs_os_file_printf(fs_file_t* file, const char* const format, va_list args) {
  int32_t n = 0;
  char buffer[256];
  /*FIXME*/
  n = vsnprintf(buffer, sizeof(buffer), format, args);
  return_value_if_fail(n >= 0, 0);

  return fs_os_file_write(file, buffer, n);
}

static ret_t fs_os_file_seek(fs_file_t* file, int32_t offset) {
  spiffs_file fp = (((fs_file_spiffs_t*)file)->file);

  return SPIFFS_lseek(sfs, fp, offset, 0) == 0 ? RET_OK : RET_FAIL;
}

static int64_t fs_os_file_tell(fs_file_t* file) {
  spiffs_file fp = (((fs_file_spiffs_t*)file)->file);

  return SPIFFS_tell(sfs, fp);
}

static int64_t fs_os_file_size(fs_file_t* file) {
  spiffs_stat st;
  spiffs_file fp = (((fs_file_spiffs_t*)file)->file);

  if (SPIFFS_fstat(sfs, fp, &st) == 0) {
    return st.size;
  } else {
    return 0;
  }
}

static ret_t fs_stat_from(fs_stat_info_t* fst, spiffs_stat* st) {
  memset(fst, 0x00, sizeof(fs_stat_info_t));

  fst->size = st->size;
  if (st->type == SPIFFS_TYPE_FILE) {
    fst->is_reg_file = TRUE;
  } else if (st->type == SPIFFS_TYPE_DIR) {
    fst->is_dir = TRUE;
  }

  return RET_OK;
}

static ret_t fs_os_file_stat(fs_file_t* file, fs_stat_info_t* fst) {
  spiffs_stat st;
  spiffs_file fp = (((fs_file_spiffs_t*)file)->file);

  if (SPIFFS_fstat(sfs, fp, &st) == 0) {
    return fs_stat_from(fst, &st);
  } else {
    return RET_FAIL;
  }
}

static ret_t fs_os_file_sync(fs_file_t* file) {
  return RET_OK;
}

static ret_t fs_os_file_truncate(fs_file_t* file, int32_t size) {
  /*TODO*/
  return RET_OK;
}

bool_t fs_os_file_eof(fs_file_t* file) {
  spiffs_file fp = (((fs_file_spiffs_t*)file)->file);

  return SPIFFS_eof(sfs, fp);
}

static ret_t fs_os_file_close(fs_file_t* file) {
  spiffs_file fp = (((fs_file_spiffs_t*)file)->file);
  SPIFFS_close(sfs, fp);
  TKMEM_FREE(file);

  return RET_OK;
}

typedef struct _fs_dir_spiffs_t {
  fs_dir_t fs_dir;
  spiffs_DIR dir;
} fs_dir_spiffs_t;

static ret_t fs_os_dir_rewind(fs_dir_t* dir) {
  return RET_NOT_IMPL;
}

static ret_t fs_os_dir_read(fs_dir_t* dir, fs_item_t* item) {
  struct spiffs_dirent e;
  spiffs_DIR* dp = &(((fs_dir_spiffs_t*)dir)->dir);

  memset(item, 0x00, sizeof(fs_item_t));

  if (SPIFFS_readdir(dp, &e) != NULL) {
    item->is_link = FALSE;
    item->is_dir = e.type == SPIFFS_TYPE_DIR;
    item->is_reg_file = e.type == SPIFFS_TYPE_FILE;
    tk_strncpy(item->name, (char*)(e.name), MAX_PATH);

    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static ret_t fs_os_dir_close(fs_dir_t* dir) {
  spiffs_DIR* dp = &(((fs_dir_spiffs_t*)dir)->dir);
  SPIFFS_closedir(dp);
  TKMEM_FREE(dir);

  return RET_OK;
}

static const fs_file_vtable_t s_file_vtable = {.read = fs_os_file_read,
                                               .write = fs_os_file_write,
                                               .printf = fs_os_file_printf,
                                               .seek = fs_os_file_seek,
                                               .tell = fs_os_file_tell,
                                               .size = fs_os_file_size,
                                               .stat = fs_os_file_stat,
                                               .sync = fs_os_file_sync,
                                               .truncate = fs_os_file_truncate,
                                               .eof = fs_os_file_eof,
                                               .close = fs_os_file_close};

static fs_file_t* fs_file_create(void) {
  fs_file_t* f = NULL;
  fs_file_spiffs_t* spiff = TKMEM_ZALLOC(fs_file_spiffs_t);

  if (spiff != NULL) {
    f = (fs_file_t*)spiff;
    f->vt = &s_file_vtable;
  }

  return f;
}

/**
 * http://www.cplusplus.com/reference/cstdio/fopen/
 *
 *
 */
static int mode_from_str(fs_t* fs, const char* filename, const char* mode) {
  if (tk_str_eq(mode, "r") || tk_str_eq(mode, "rb")) {
    /* "r"	read: Open file for input operations. The file must exist. */
    return SPIFFS_RDONLY;
  } else if (tk_str_eq(mode, "w") || tk_str_eq(mode, "wb")) {
    /* "w"	write: Create an empty file for output operations. If a file with
     * the same name already exists, its contents are discarded and the file is
     * treated as a new empty file. */
    if (fs_file_exist(fs, filename)) {
      fs_remove_file(fs, filename);
    }
    return SPIFFS_WRONLY | SPIFFS_CREAT;
  } else if (tk_str_eq(mode, "a")) {
    /* "a"	append: Open file for output at the end of a file. Output operations
     * always write data at the end of the file, expanding it. Repositioning
     * operations (fseek, fsetpos, rewind) are ignored. The file is created if
     * it does not exist. */
    if (fs_file_exist(fs, filename)) {
      return SPIFFS_APPEND | SPIFFS_WRONLY;
    } else {
      return SPIFFS_CREAT | SPIFFS_APPEND | SPIFFS_WRONLY;
    }
  } else if (tk_str_eq(mode, "r+") || tk_str_eq(mode, "rb+")) {
    /* "r+"	read/update: Open a file for update (both for input and output).
     * The file must exist. */
    return SPIFFS_RDWR;
  } else if (tk_str_eq(mode, "w+") || tk_str_eq(mode, "wb+")) {
    /* "w+"	write/update: Create an empty file and open it for update (both for
     * input and output). If a file with the same name already exists its
     * contents are discarded and the file is treated as a new empty file.*/
    if (fs_file_exist(fs, filename)) {
      fs_remove_file(fs, filename);
    }
    return SPIFFS_RDWR | SPIFFS_CREAT;
  } else if (tk_str_eq(mode, "a+")) {
    /* "a+"	append/update: Open a file for update (both for input and output)
     * with all output operations writing data at the end of the file.
     * Repositioning operations (fseek, fsetpos, rewind) affects the next input
     * operations, but output operations move the position back to the end of
     * file. The file is created if it does not exist. */
    if (fs_file_exist(fs, filename)) {
      return SPIFFS_RDWR | SPIFFS_APPEND;
    } else {
      return SPIFFS_RDWR | SPIFFS_CREAT | SPIFFS_APPEND;
    }
  } else {
    return SPIFFS_CREAT | SPIFFS_RDWR;
  }
}

static fs_file_t* fs_os_open_file(fs_t* fs, const char* name, const char* mode) {
  fs_file_t* file = fs_file_create();
  fs_file_spiffs_t* spiff = (fs_file_spiffs_t*)file;
  return_value_if_fail(file != NULL, NULL);

  spiff->file = SPIFFS_open(sfs, name, mode_from_str(fs, name, mode), 0);

  if (spiff->file >= 0) {
    return file;
  } else {
    TKMEM_FREE(file);
    return NULL;
  }
}

static ret_t fs_os_remove_file(fs_t* fs, const char* name) {
  return SPIFFS_remove(sfs, name) == 0 ? RET_OK : RET_FAIL;
}

static bool_t fs_os_file_exist(fs_t* fs, const char* name) {
  fs_stat_info_t info;

  if (fs_stat(fs, name, &info) == RET_OK) {
    return info.is_reg_file;
  } else {
    return FALSE;
  }
}

static ret_t fs_os_file_rename(fs_t* fs, const char* name, const char* new_name) {
  return SPIFFS_rename(sfs, name, new_name) == 0 ? RET_OK : RET_FAIL;
}

static const fs_dir_vtable_t s_dir_vtable = {
    .read = fs_os_dir_read, .rewind = fs_os_dir_rewind, .close = fs_os_dir_close};

fs_dir_t* fs_dir_create(void) {
  fs_dir_t* d = NULL;
  fs_dir_spiffs_t* fdir = TKMEM_ZALLOC(fs_dir_spiffs_t);
  if (fdir != NULL) {
    d = (fs_dir_t*)fdir;
    d->vt = &s_dir_vtable;
  }

  return d;
}

fs_dir_t* fs_os_open_dir(fs_t* fs, const char* name) {
  fs_dir_t* dir = NULL;
  spiffs_DIR* dp = NULL;
  return_value_if_fail(name != NULL, NULL);

  dir = fs_dir_create();
  return_value_if_fail(dir != NULL, NULL);
  dp = &(((fs_dir_spiffs_t*)dir)->dir);

  if (SPIFFS_opendir(sfs, name, dp) != NULL) {
    return dir;
  } else {
    TKMEM_FREE(dir);
    return NULL;
  }
}

static ret_t fs_os_remove_dir(fs_t* fs, const char* name) {
  return RET_NOT_IMPL;
}

static ret_t fs_os_create_dir(fs_t* fs, const char* name) {
  return RET_NOT_IMPL;
}

static bool_t fs_os_dir_exist(fs_t* fs, const char* name) {
  fs_stat_info_t info;

  if (fs_stat(fs, name, &info) == RET_OK) {
    return info.is_dir;
  } else {
    return FALSE;
  }
}

static ret_t fs_os_dir_rename(fs_t* fs, const char* name, const char* new_name) {
  return SPIFFS_rename(sfs, name, new_name) == 0 ? RET_OK : RET_FAIL;
}

static int32_t fs_os_get_file_size(fs_t* fs, const char* name) {
  fs_stat_info_t info;
  if (fs_stat(fs, name, &info) == RET_OK) {
    return info.size;
  } else {
    return 0;
  }
}

static ret_t fs_os_get_disk_info(fs_t* fs, const char* volume, int32_t* free_kb,
                                 int32_t* total_kb) {
  /*TODO*/
  *free_kb = 0;
  *total_kb = 0;
  (void)fs;
  assert(!"fs_os_get_disk_info not supported yet");

  return RET_NOT_IMPL;
}

static ret_t fs_os_get_exe(fs_t* fs, char path[MAX_PATH + 1]) {
  tk_strcpy(path, "/app/bin");

  return RET_OK;
}

static ret_t fs_os_get_user_storage_path(fs_t* fs, char path[MAX_PATH + 1]) {
  tk_strcpy(path, "/appdata");

  return RET_OK;
}

static ret_t fs_os_get_temp_path(fs_t* fs, char path[MAX_PATH + 1]) {
  tk_strcpy(path, "/tmp");

  return RET_OK;
}

static ret_t fs_os_get_cwd(fs_t* fs, char cwd[MAX_PATH + 1]) {
  tk_strcpy(cwd, "/");

  return RET_OK;
}

static ret_t fs_os_stat(fs_t* fs, const char* name, fs_stat_info_t* fst) {
  spiffs_stat st;
  if (SPIFFS_stat(sfs, name, &st) == 0) {
    return fs_stat_from(fst, &st);
  } else {
    return RET_FAIL;
  }
}

static const fs_t s_os_fs = {.open_file = fs_os_open_file,
                             .remove_file = fs_os_remove_file,
                             .file_exist = fs_os_file_exist,
                             .file_rename = fs_os_file_rename,

                             .open_dir = fs_os_open_dir,
                             .remove_dir = fs_os_remove_dir,
                             .create_dir = fs_os_create_dir,
                             .dir_exist = fs_os_dir_exist,
                             .dir_rename = fs_os_dir_rename,

                             .get_file_size = fs_os_get_file_size,
                             .get_disk_info = fs_os_get_disk_info,
                             .get_cwd = fs_os_get_cwd,
                             .get_exe = fs_os_get_exe,
                             .get_temp_path = fs_os_get_temp_path,
                             .get_user_storage_path = fs_os_get_user_storage_path,
                             .stat = fs_os_stat};

ret_t os_fs_spiffs_set(spiffs* fs) {
  sfs = fs;

  return RET_OK;
}

fs_t* os_fs_spiffs(void) {
#ifdef WITH_FS_MT
  return fs_mt_wrap((fs_t*)&s_os_fs);
#else
  return (fs_t*)&s_os_fs;
#endif/*WITH_FS_MT*/
}

#if defined(MACOS) || defined(LINUX) || defined(WIN32)
#else
fs_t* os_fs(void) {
  return os_fs_spiffs();
}
#endif
