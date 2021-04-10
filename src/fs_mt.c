/**
 * File:   fs_mt.c
 * Author: AWTK Develop Team
 * Brief:  make fs to support multi thread
 *
 * Copyright (c) 2020 - 2021  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2021-04-10 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include "tkc/fs.h"
#include "tkc/mem.h"
#include "tkc/utils.h"
#include "tkc/mutex.h"
#include <stdarg.h>
#include "fs_mt.h"

#ifdef WITH_FS_MT
static fs_t* s_fs_impl;
static tk_mutex_t* s_fs_mutex;

#if defined(LINUX) || defined(WIN32) || defined(MACOS) || defined(HAS_STDIO)
#include <stdio.h>
#else
extern int vsnprintf(char* s, size_t n, const char* format, va_list arg);
#endif

static int32_t fs_mt_file_read(fs_file_t* file, void* buffer, uint32_t size) {
  int32_t result = 0;
  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_read((fs_file_t*)(file->data), buffer, size);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

int32_t fs_mt_file_write(fs_file_t* file, const void* buffer, uint32_t size) {
  int32_t result = 0;
  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_write((fs_file_t*)(file->data), buffer, size);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

int32_t fs_mt_file_printf(fs_file_t* file, const char* const format, va_list args) {
  int32_t n = 0;
  char buffer[256];
  /*FIXME*/
  n = vsnprintf(buffer, sizeof(buffer), format, args);
  return_value_if_fail(n >= 0, 0);

  return fs_mt_file_write(file, buffer, n);
}

static ret_t fs_mt_file_seek(fs_file_t* file, int32_t offset) {
  ret_t result = RET_FAIL;
  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_seek((fs_file_t*)(file->data), offset);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static int64_t fs_mt_file_tell(fs_file_t* file) {
  int64_t result = -1;
  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_tell((fs_file_t*)(file->data));
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static int64_t fs_mt_file_size(fs_file_t* file) {
  int64_t result = -1;
  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_size((fs_file_t*)(file->data));
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_file_stat(fs_file_t* file, fs_stat_info_t* fst) {
  ret_t result = RET_FAIL;
  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_stat((fs_file_t*)(file->data), fst);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_file_sync(fs_file_t* file) {
  ret_t result = RET_FAIL;
  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_sync((fs_file_t*)(file->data));
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_file_truncate(fs_file_t* file, int32_t size) {
  ret_t result = RET_FAIL;
  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_truncate((fs_file_t*)(file->data), size);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

bool_t fs_mt_file_eof(fs_file_t* file) {
  bool_t result = TRUE;
  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_eof((fs_file_t*)(file->data));
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_file_close(fs_file_t* file) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_close((fs_file_t*)(file->data));
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_dir_rewind(fs_dir_t* dir) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_dir_rewind((fs_dir_t*)(dir->data));
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_dir_read(fs_dir_t* dir, fs_item_t* item) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_dir_read((fs_dir_t*)(dir->data), item);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_dir_close(fs_dir_t* dir) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_dir_close((fs_dir_t*)(dir->data));
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static const fs_file_vtable_t s_file_vtable = {.read = fs_mt_file_read,
                                               .write = fs_mt_file_write,
                                               .printf = fs_mt_file_printf,
                                               .seek = fs_mt_file_seek,
                                               .tell = fs_mt_file_tell,
                                               .size = fs_mt_file_size,
                                               .stat = fs_mt_file_stat,
                                               .sync = fs_mt_file_sync,
                                               .truncate = fs_mt_file_truncate,
                                               .eof = fs_mt_file_eof,
                                               .close = fs_mt_file_close};

static fs_file_t* fs_mt_open_file(fs_t* fs, const char* name, const char* mode) {
  fs_file_t* file = TKMEM_ZALLOC(fs_file_t);
  return_value_if_fail(file != NULL, NULL);

  file->vt = &s_file_vtable;
  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    file->data = fs_open_file(s_fs_impl, name, mode);
    if (file->data == NULL) {
      TKMEM_FREE(file);
    }
    tk_mutex_unlock(s_fs_mutex);
  }

  return file;
}

static ret_t fs_mt_remove_file(fs_t* fs, const char* name) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_remove_file(s_fs_impl, name);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static bool_t fs_mt_file_exist(fs_t* fs, const char* name) {
  bool_t result = FALSE;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_exist(s_fs_impl, name);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_file_rename(fs_t* fs, const char* name, const char* new_name) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_file_rename(s_fs_impl, name, new_name);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static const fs_dir_vtable_t s_dir_vtable = {
    .read = fs_mt_dir_read, .rewind = fs_mt_dir_rewind, .close = fs_mt_dir_close};

static fs_dir_t* fs_mt_open_dir(fs_t* fs, const char* name) {
  fs_dir_t* dir = TKMEM_ZALLOC(fs_dir_t);
  return_value_if_fail(dir != NULL, NULL);
  dir->vt = &(s_dir_vtable);

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    dir->data = fs_open_dir(s_fs_impl, name);
    if (dir->data == NULL) {
      TKMEM_FREE(dir);
    }
    tk_mutex_unlock(s_fs_mutex);
  }

  return dir;
}

static ret_t fs_mt_remove_dir(fs_t* fs, const char* name) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_remove_dir(s_fs_impl, name);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_create_dir(fs_t* fs, const char* name) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_create_dir(s_fs_impl, name);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static bool_t fs_mt_dir_exist(fs_t* fs, const char* name) {
  bool_t result = FALSE;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_dir_exist(s_fs_impl, name);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_dir_rename(fs_t* fs, const char* name, const char* new_name) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_dir_rename(s_fs_impl, name, new_name);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static int32_t fs_mt_get_file_size(fs_t* fs, const char* name) {
  int32_t result = 0;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_get_file_size(s_fs_impl, name);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_get_disk_info(fs_t* fs, const char* volume, int32_t* free_kb,
                                 int32_t* total_kb) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_get_disk_info(s_fs_impl, volume, free_kb, total_kb);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_get_exe(fs_t* fs, char path[MAX_PATH + 1]) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_get_exe(s_fs_impl, path);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_get_user_storage_path(fs_t* fs, char path[MAX_PATH + 1]) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_get_user_storage_path(s_fs_impl, path);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_get_temp_path(fs_t* fs, char path[MAX_PATH + 1]) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_get_temp_path(s_fs_impl, path);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_get_cwd(fs_t* fs, char cwd[MAX_PATH + 1]) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_get_cwd(s_fs_impl, cwd);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static ret_t fs_mt_stat(fs_t* fs, const char* name, fs_stat_info_t* fst) {
  ret_t result = RET_FAIL;

  if (tk_mutex_lock(s_fs_mutex) == RET_OK) {
    result = fs_stat(s_fs_impl, name, fst);
    tk_mutex_unlock(s_fs_mutex);
  }

  return result;
}

static const fs_t s_os_fs_mt = {.open_file = fs_mt_open_file,
                                .remove_file = fs_mt_remove_file,
                                .file_exist = fs_mt_file_exist,
                                .file_rename = fs_mt_file_rename,

                                .open_dir = fs_mt_open_dir,
                                .remove_dir = fs_mt_remove_dir,
                                .create_dir = fs_mt_create_dir,
                                .dir_exist = fs_mt_dir_exist,
                                .dir_rename = fs_mt_dir_rename,

                                .get_file_size = fs_mt_get_file_size,
                                .get_disk_info = fs_mt_get_disk_info,
                                .get_cwd = fs_mt_get_cwd,
                                .get_exe = fs_mt_get_exe,
                                .get_user_storage_path = fs_mt_get_user_storage_path,
                                .get_temp_path = fs_mt_get_temp_path,
                                .stat = fs_mt_stat};

fs_t* fs_mt_wrap(fs_t* impl) {
  s_fs_impl = impl;

  if (s_fs_mutex == NULL) {
    s_fs_mutex = tk_mutex_create();
  }

  return (fs_t*)&s_os_fs_mt;
}
#else
fs_t* fs_mt_wrap(fs_t* impl) {
  return impl;
}
#endif /*WITH_FS_MT*/
