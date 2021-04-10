/**
 * File:   fs_os_fatfs.c
 * Author: AWTK Develop Team
 * Brief:  fatfs implemented fs
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
 * 2020-05-04 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif /*WIN32_LEAN_AND_MEAN*/

#include "ff.h"
#include "tkc/fs.h"
#include "tkc/mem.h"
#include "tkc/utils.h"
#include <stdarg.h>

#include "fs_mt.h"

#if defined(LINUX) || defined(WIN32) || defined(MACOS) || defined(HAS_STDIO)
#include <stdio.h>
#else
extern int vsnprintf(char* s, size_t n, const char* format, va_list arg);
#endif

typedef struct _fs_file_ff_t {
  fs_file_t fs_file;
  FIL file;
} fs_file_ff_t;

static const TCHAR* path_from_utf8(TCHAR path[MAX_PATH + 1], const char* utf8_path) {
  tk_strncpy(path, utf8_path, MAX_PATH);
  path[MAX_PATH] = '\0';
  /*FIXME*/
  return path;
}

static const char* path_to_utf8(const TCHAR* path, char utf8_path[MAX_PATH + 1]) {
  tk_strncpy(utf8_path, path, MAX_PATH);
  utf8_path[MAX_PATH] = '\0';
  /*FIXME*/
  return utf8_path;
}

static inline ret_t fresult_to_ret(FRESULT ret) {
  if (ret == FR_OK) {
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static int32_t fs_os_file_read(fs_file_t* file, void* buffer, uint32_t size) {
  UINT br = 0;
  FIL* fp = &(((fs_file_ff_t*)file)->file);
  FRESULT ret = f_read(fp, buffer, size, &br);

  if (ret == FR_OK) {
    return (int32_t)br;
  } else {
    return -1;
  }
}

int32_t fs_os_file_write(fs_file_t* file, const void* buffer, uint32_t size) {
  UINT bw = 0;
  FIL* fp = &(((fs_file_ff_t*)file)->file);
  FRESULT ret = f_write(fp, buffer, size, &bw);

  if (ret == FR_OK) {
    return (int32_t)bw;
  } else {
    return -1;
  }
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
  FIL* fp = &(((fs_file_ff_t*)file)->file);
  FRESULT ret = f_lseek(fp, offset);

  return fresult_to_ret(ret);
}

static int64_t fs_os_file_tell(fs_file_t* file) {
  FIL* fp = &(((fs_file_ff_t*)file)->file);

  return f_tell(fp);
}

static int64_t fs_os_file_size(fs_file_t* file) {
  FIL* fp = &(((fs_file_ff_t*)file)->file);

  return f_size(fp);
}

static ret_t fs_os_file_stat(fs_file_t* file, fs_stat_info_t* fst) {
  FIL* fp = &(((fs_file_ff_t*)file)->file);

  memset(fst, 0x00, sizeof(fs_stat_info_t));
  fst->size = f_size(fp);
  fst->is_reg_file = TRUE;

  return RET_OK;
}

static ret_t fs_os_file_sync(fs_file_t* file) {
  FIL* fp = &(((fs_file_ff_t*)file)->file);
  return f_sync(fp) == 0 ? RET_OK : RET_FAIL;
}

static ret_t fs_os_file_truncate(fs_file_t* file, int32_t size) {
  FIL* fp = &(((fs_file_ff_t*)file)->file);

  if (size == 0) {
    return fresult_to_ret(f_truncate(fp));
  } else {
    assert(!"not impl");
    return RET_NOT_IMPL;
  }
}

bool_t fs_os_file_eof(fs_file_t* file) {
  FIL* fp = &(((fs_file_ff_t*)file)->file);

  return f_eof(fp);
}

static ret_t fs_os_file_close(fs_file_t* file) {
  FIL* fp = &(((fs_file_ff_t*)file)->file);

  f_close(fp);
  TKMEM_FREE(file);

  return RET_OK;
}

typedef struct _fs_dir_ff_t {
  fs_dir_t fs_dir;
  FF_DIR dir;
  int index;
} fs_dir_ff_t;

static ret_t fs_os_dir_rewind(fs_dir_t* dir) {
  FF_DIR* dp = &(((fs_dir_ff_t*)dir)->dir);
  fs_dir_ff_t* fdir = ((fs_dir_ff_t*)dir);

  fdir->index = -2;

  return fresult_to_ret(f_rewinddir(dp));
}

static ret_t fs_os_dir_read(fs_dir_t* dir, fs_item_t* item) {
  FILINFO fno;
  FF_DIR* dp = &(((fs_dir_ff_t*)dir)->dir);
  fs_dir_ff_t* fdir = ((fs_dir_ff_t*)dir);
  memset(item, 0x00, sizeof(fs_item_t));

  if (fdir->index < 0) {
    item->is_link = FALSE;
    item->is_reg_file = FALSE;
    item->is_dir = TRUE;
    if (fdir->index < -1) {
      tk_strcpy(item->name, ".");
      fdir->index = -1;
    } else {
      tk_strcpy(item->name, "..");
      fdir->index = 0;
    }

    return RET_OK;
  } else {
    FRESULT ret = f_readdir(dp, &fno);

    fdir->index++;
    if (ret == FR_OK) {
      uint8_t type = fno.fattrib;
      item->is_link = FALSE;
      item->is_reg_file = (type & AM_ARC) != 0;
      item->is_dir = (type & AM_DIR) != 0;

      path_to_utf8(fno.fname, item->name);
      if (*item->name) {
        return RET_OK;
      } else {
        return RET_FAIL;
      }
    } else {
      return RET_FAIL;
    }
  }
}

static ret_t fs_os_dir_close(fs_dir_t* dir) {
  FF_DIR* dp = &(((fs_dir_ff_t*)dir)->dir);
  f_closedir(dp);
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
  fs_file_ff_t* ff = TKMEM_ZALLOC(fs_file_ff_t);
  if (ff != NULL) {
    f = (fs_file_t*)ff;
    f->vt = &s_file_vtable;
  }

  return f;
}

/**
 * http://www.cplusplus.com/reference/cstdio/fopen/
 *
 *
 */
static BYTE mode_from_str(fs_t* fs, const char* filename, const char* mode) {
  if (tk_str_eq(mode, "r") || tk_str_eq(mode, "rb")) {
    /* "r"	read: Open file for input operations. The file must exist. */
    return FA_READ;
  } else if (tk_str_eq(mode, "w") || tk_str_eq(mode, "wb")) {
    /* "w"	write: Create an empty file for output operations. If a file with
     * the same name already exists, its contents are discarded and the file is
     * treated as a new empty file. */
    if (fs_file_exist(fs, filename)) {
      fs_remove_file(fs, filename);
    }
    return FA_WRITE | FA_CREATE_NEW;
  } else if (tk_str_eq(mode, "a")) {
    /* "a"	append: Open file for output at the end of a file. Output operations
     * always write data at the end of the file, expanding it. Repositioning
     * operations (fseek, fsetpos, rewind) are ignored. The file is created if
     * it does not exist. */
    if (fs_file_exist(fs, filename)) {
      return FA_OPEN_APPEND | FA_WRITE;
    } else {
      return FA_CREATE_NEW | FA_OPEN_APPEND | FA_WRITE;
    }
  } else if (tk_str_eq(mode, "r+") || tk_str_eq(mode, "rb+")) {
    /* "r+"	read/update: Open a file for update (both for input and output).
     * The file must exist. */
    return FA_WRITE | FA_READ;
  } else if (tk_str_eq(mode, "w+") || tk_str_eq(mode, "wb+")) {
    /* "w+"	write/update: Create an empty file and open it for update (both for
     * input and output). If a file with the same name already exists its
     * contents are discarded and the file is treated as a new empty file.*/
    if (fs_file_exist(fs, filename)) {
      fs_remove_file(fs, filename);
    }
    return FA_WRITE | FA_READ | FA_CREATE_NEW;
  } else if (tk_str_eq(mode, "a+")) {
    /* "a+"	append/update: Open a file for update (both for input and output)
     * with all output operations writing data at the end of the file.
     * Repositioning operations (fseek, fsetpos, rewind) affects the next input
     * operations, but output operations move the position back to the end of
     * file. The file is created if it does not exist. */
    if (fs_file_exist(fs, filename)) {
      return FA_READ | FA_WRITE | FA_OPEN_APPEND;
    } else {
      return FA_READ | FA_WRITE | FA_OPEN_APPEND | FA_CREATE_NEW;
    }
  } else {
    return FA_CREATE_NEW | FA_WRITE | FA_READ;
  }
}

static fs_file_t* fs_os_open_file(fs_t* fs, const char* name, const char* mode) {
  FIL* fp = NULL;
  fs_file_t* file = NULL;
  TCHAR path[MAX_PATH + 1];
  return_value_if_fail(name != NULL && mode != NULL, NULL);
  file = fs_file_create();
  return_value_if_fail(file != NULL, NULL);

  fp = &(((fs_file_ff_t*)file)->file);
  if (f_open(fp, path_from_utf8(path, name), mode_from_str(fs, name, mode)) == FR_OK) {
    return file;
  } else {
    TKMEM_FREE(file);
    return NULL;
  }
}

static ret_t fs_os_remove_file(fs_t* fs, const char* name) {
  TCHAR path[MAX_PATH + 1];
  return_value_if_fail(name != NULL, RET_FAIL);

  return fresult_to_ret(f_unlink(path_from_utf8(path, name)));
}

static bool_t fs_os_file_exist(fs_t* fs, const char* name) {
  FILINFO fno;
  TCHAR path[MAX_PATH + 1];
  return_value_if_fail(name != NULL, FALSE);

  if (f_stat(path_from_utf8(path, name), &fno) == FR_OK) {
    return (fno.fattrib & AM_ARC) != 0;
  } else {
    return FALSE;
  }
}

static ret_t fs_os_file_rename(fs_t* fs, const char* name, const char* new_name) {
  TCHAR path[MAX_PATH + 1];
  TCHAR new_path[MAX_PATH + 1];
  return_value_if_fail(name != NULL && new_name != NULL, RET_BAD_PARAMS);

  return fresult_to_ret(f_rename(path_from_utf8(path, name), path_from_utf8(new_path, new_name)));
}

static const fs_dir_vtable_t s_dir_vtable = {
    .read = fs_os_dir_read, .rewind = fs_os_dir_rewind, .close = fs_os_dir_close};

static fs_dir_t* fs_dir_create(void) {
  fs_dir_t* d = NULL;
  fs_dir_ff_t* fdir = TKMEM_ZALLOC(fs_dir_ff_t);
  if (fdir != NULL) {
    d = (fs_dir_t*)fdir;
    d->vt = &s_dir_vtable;
    fdir->index = -2;
  }

  return d;
}

static fs_dir_t* fs_os_open_dir(fs_t* fs, const char* name) {
  FF_DIR* dp = NULL;
  fs_dir_t* dir = NULL;
  TCHAR path[MAX_PATH + 1];
  return_value_if_fail(name != NULL, NULL);
  dir = fs_dir_create();
  return_value_if_fail(dir != NULL, NULL);
  dp = &(((fs_dir_ff_t*)dir)->dir);

  if (f_opendir(dp, path_from_utf8(path, name)) == FR_OK) {
    return dir;
  } else {
    TKMEM_FREE(dir);
    return NULL;
  }
}

static ret_t fs_os_remove_dir(fs_t* fs, const char* name) {
  TCHAR path[MAX_PATH + 1];
  return_value_if_fail(name != NULL, RET_FAIL);

  return fresult_to_ret(f_rmdir(path_from_utf8(path, name)));
}

static ret_t fs_os_create_dir(fs_t* fs, const char* name) {
  TCHAR path[MAX_PATH + 1];
  return_value_if_fail(name != NULL, RET_FAIL);

  return fresult_to_ret(f_mkdir(path_from_utf8(path, name)));
}

static bool_t fs_os_dir_exist(fs_t* fs, const char* name) {
  FILINFO fno;
  TCHAR path[MAX_PATH + 1];
  return_value_if_fail(name != NULL, FALSE);

  if (f_stat(path_from_utf8(path, name), &fno) == FR_OK) {
    return (fno.fattrib & AM_DIR) != 0;
  } else {
    return FALSE;
  }
}

static ret_t fs_os_dir_rename(fs_t* fs, const char* name, const char* new_name) {
  TCHAR path[MAX_PATH + 1];
  TCHAR new_path[MAX_PATH + 1];
  return_value_if_fail(name != NULL && new_name != NULL, RET_BAD_PARAMS);

  return fresult_to_ret(f_rename(path_from_utf8(path, name), path_from_utf8(new_path, new_name)));
}

static int32_t fs_os_get_file_size(fs_t* fs, const char* name) {
  FILINFO fno;
  TCHAR path[MAX_PATH + 1];
  return_value_if_fail(name != NULL, -1);

  if (f_stat(path_from_utf8(path, name), &fno) == FR_OK) {
    return fno.fsize;
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
  tk_strcpy(path, "app");

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
  TCHAR path[MAX_PATH + 1];
  return_value_if_fail(cwd != NULL, RET_BAD_PARAMS);

  memset(path, 0x00, sizeof(path));
  if (f_getcwd(path, MAX_PATH) == FR_OK) {
    path_to_utf8(path, cwd);

    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static ret_t fs_os_stat(fs_t* fs, const char* name, fs_stat_info_t* fst) {
  FILINFO fno;
  TCHAR path[MAX_PATH + 1];
  return_value_if_fail(name != NULL && fst != NULL, RET_BAD_PARAMS);

  if (f_stat(path_from_utf8(path, name), &fno) == FR_OK) {
    fst->size = fno.fsize;
    fst->is_link = FALSE;
    fst->is_dir = (fno.fattrib & AM_DIR) != 0;
    fst->is_reg_file = (fno.fattrib & AM_ARC) != 0;
    fst->mtime = fno.fdate * 24 * 3600 + fno.ftime;

    return RET_OK;
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
                             .get_user_storage_path = fs_os_get_user_storage_path,
                             .get_temp_path = fs_os_get_temp_path,
                             .stat = fs_os_stat};

fs_t* os_fs_fatfs(void) {
#ifdef WITH_FS_MT
  return fs_mt_wrap((fs_t*)&s_os_fs);
#else
  return (fs_t*)&s_os_fs;
#endif/*WITH_FS_MT*/
}
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
#else
fs_t* os_fs(void) {
  return os_fs_fatfs(&s_os_fs);
}
#endif
