/**
 * File:   fs_os_posix.c
 * Author: AWTK Develop Team
 * Brief:  posix implemented fs
 *
 * Copyright (c) 2024 - 2024  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2024-10-31 Li XianJing <xianjimli@hotmail.com> created
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
#include "fs_os_conf.h"

#if defined(LINUX) || defined(WIN32) || defined(MACOS) || defined(HAS_STDIO)
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#elif defined(RT_THREAD)
#include "dfs_poxis.h"
#endif

typedef struct _fs_file_posix_t {
  fs_file_t fs_file;
  int file;
} fs_file_posix_t;

static inline ret_t fresult_to_ret(FRESULT ret) {
  if (ret == FR_OK) {
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static int32_t fs_os_file_read(fs_file_t* file, void* buffer, uint32_t size) {
  int fd = ((fs_file_posix_t*)file)->file;

  return (int32_t)read(fd, buffer, size);
}

int32_t fs_os_file_write(fs_file_t* file, const void* buffer, uint32_t size) {
  int fd = ((fs_file_posix_t*)file)->file;
  return (int32_t)write(fd, buffer, size);
}

int32_t fs_os_file_printf(fs_file_t* file, const char* const format, va_list args) {
  int32_t n = 0;
  char buffer[256] = {0};
  n = tk_vsnprintf(buffer, sizeof(buffer), format, args);
  return_value_if_fail(n >= 0, 0);

  return fs_os_file_write(file, buffer, n);
}

static ret_t fs_os_file_seek(fs_file_t* file, int32_t offset) {
  int fd = ((fs_file_posix_t*)file)->file;
  int32_t ret = (int32_t)lseek(fd, offset, SEEK_SET);

  return ret == offset ? RET_OK : RET_FAIL;
}

static int64_t fs_os_file_tell(fs_file_t* file) {
  int fd = ((fs_file_posix_t*)file)->file;

  (void)fd;
  assert(!"not supported yet");
  return 0;
}

static int64_t fs_os_file_size(fs_file_t* file) {
  struct stat buf;
  int fd = ((fs_file_posix_t*)file)->file;

  if (fstat(fd, &buf) == 0) {
    return buf.st_size;
  }

  return 0;
}

static ret_t fs_stat_info_from_stat(fs_stat_info_t* fst, struct stat* st) {
  return_value_if_fail(fst != NULL && st != NULL, RET_BAD_PARAMS);

  fst->dev = st->st_dev;
  fst->ino = st->st_ino;
  fst->mode = st->st_mode;
  fst->nlink = st->st_nlink;
  fst->uid = st->st_uid;
  fst->gid = st->st_gid;
  fst->rdev = st->st_rdev;
  fst->size = st->st_size;
  fst->atime = st->st_atime;
  fst->mtime = st->st_mtime;
  fst->ctime = st->st_ctime;
  fst->is_dir = (st->st_mode & S_IFDIR) != 0;
#ifdef S_IFLNK
  fst->is_link = (st->st_mode & S_IFLNK) != 0;
#else
  fst->is_link = FALSE;
#endif /*S_IFLNK*/
  fst->is_reg_file = (st->st_mode & S_IFREG) != 0;

  return RET_OK;
}

static ret_t fs_os_file_stat(fs_file_t* file, fs_stat_info_t* fst) {
  struct stat buf;
  int fd = ((fs_file_posix_t*)file)->file;

  memset(fst, 0x00, sizeof(fs_stat_info_t));
  if (fstat(fd, &buf) == 0) {
    return fs_stat_info_from_stat(fst, &buf);
  } else {
    return RET_FAIL;
  }
}

static ret_t fs_os_file_sync(fs_file_t* file) {
  int fd = ((fs_file_posix_t*)file)->file;
  return fsync(fd) == 0 ? RET_OK : RET_FAIL;
}

static ret_t fs_os_file_truncate(fs_file_t* file, int32_t size) {
  assert(!"not impl");
  return RET_NOT_IMPL;
}

bool_t fs_os_file_eof(fs_file_t* file) {
  assert(!"not impl");
  return FALSE;
}

static ret_t fs_os_file_close(fs_file_t* file) {
  int fd = ((fs_file_posix_t*)file)->file;

  close(fd);
  TKMEM_FREE(file);

  return RET_OK;
}

typedef struct _fs_dir_posix_t {
  fs_dir_t fs_dir;
  DIR* dir;
} fs_dir_posix_t;

static ret_t fs_os_dir_rewind(fs_dir_t* dir) {
  DIR* dp = ((fs_dir_posix_t*)dir)->dir;

  rewinddir(dp);

  return RET_OK;
}

static ret_t fs_os_dir_read(fs_dir_t* dir, fs_item_t* item) {
  struct dirent* ent = NULL;
  DIR* dp = ((fs_dir_posix_t*)dir)->dir;

  memset(item, 0x00, sizeof(fs_item_t));
  ent = readdir(dp);
  if (ent != NULL) {
#if defined(RT_THREAD)
    item->is_reg_file = ent->fd;
    item->is_dir = !ent->fd;
    tk_strcpy(item->name, ent->buf, sizeof(item->name) - 1);
#else
    uint8_t type = ent->d_type;
    item->is_dir = (type & DT_DIR) != 0;
    item->is_link = (type & DT_LNK) != 0;
    item->is_reg_file = (type & DT_REG) != 0;
#ifdef WIN32
    str_t str;
    str_init(&str, wcslen(ent->d_name) * 4 + 1);
    str_from_wstr_with_len(&str, ent->d_name, wcslen(ent->d_name));
    tk_strncpy(item->name, str.str, MAX_PATH);
    str_reset(&str);
#else
    tk_strncpy(item->name, ent->d_name, MAX_PATH);
#endif
#endif/*RT_THREAD */
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static ret_t fs_os_dir_close(fs_dir_t* dir) {
  DIR* dp = ((fs_dir_posix_t*)dir)->dir;

  closedir(dp);
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
  fs_file_posix_t* ff = TKMEM_ZALLOC(fs_file_posix_t);

  if (ff != NULL) {
    f = (fs_file_t*)ff;
    f->vt = &s_file_vtable;
  }

  return f;
}

static uint32_t mode_from_str(fs_t* fs, const char* filename, const char* mode) {
  if (tk_str_eq(mode, "r") || tk_str_eq(mode, "rb")) {
    /* "r"	read: Open file for input operations. The file must exist. */
    return O_RDONLY;
  } else if (tk_str_eq(mode, "w") || tk_str_eq(mode, "wb")) {
    /* "w"	write: Create an empty file for output operations. If a file with
     * the same name already exists, its contents are discarded and the file is
     * treated as a new empty file. */
    if (fs_file_exist(fs, filename)) {
      fs_remove_file(fs, filename);
    }
    return O_RDWR | O_CREAT;
  } else if (tk_str_eq(mode, "a")) {
    /* "a"	append: Open file for output at the end of a file. Output operations
     * always write data at the end of the file, expanding it. Repositioning
     * operations (fseek, fsetpos, rewind) are ignored. The file is created if
     * it does not exist. */
    if (fs_file_exist(fs, filename)) {
      return O_APPEND | O_RDWR;
    } else {
      return O_APPEND | O_CREAT | O_RDWR;
    }
  } else if (tk_str_eq(mode, "r+") || tk_str_eq(mode, "rb+")) {
    /* "r+"	read/update: Open a file for update (both for input and output).
     * The file must exist. */
    return O_RDWR;
  } else if (tk_str_eq(mode, "w+") || tk_str_eq(mode, "wb+")) {
    /* "w+"	write/update: Create an empty file and open it for update (both for
     * input and output). If a file with the same name already exists its
     * contents are discarded and the file is treated as a new empty file.*/
    if (fs_file_exist(fs, filename)) {
      fs_remove_file(fs, filename);
    }
    return O_RDWR | O_CREAT;
  } else if (tk_str_eq(mode, "a+")) {
    /* "a+"	append/update: Open a file for update (both for input and output)
     * with all output operations writing data at the end of the file.
     * Repositioning operations (fseek, fsetpos, rewind) affects the next input
     * operations, but output operations move the position back to the end of
     * file. The file is created if it does not exist. */
    if (fs_file_exist(fs, filename)) {
      return O_RDWR | O_APPEND;
    } else {
      return O_RDWR | O_APPEND | O_CREAT;
    }
  } else {
    return O_RDWR | O_CREAT;
  }
}

static fs_file_t* fs_os_open_file(fs_t* fs, const char* name, const char* mode) {
  int fd = -1;
  fs_file_t* file = NULL;
  return_value_if_fail(name != NULL && mode != NULL, NULL);
  file = fs_file_create();
  return_value_if_fail(file != NULL, NULL);

  fd = open(name, mode_from_str(fs, name, mode));
  if (fd >= 0) {
    ((fs_file_posix_t*)file)->file = fd;
    return file;
  } else {
    log_warn("open %s %s failed\n", name, mode);
    TKMEM_FREE(file);
    return NULL;
  }
}

static ret_t fs_os_remove_file(fs_t* fs, const char* name) {
  return_value_if_fail(name != NULL, RET_FAIL);

  if (unlink(name) == 0) {
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static bool_t fs_os_file_exist(fs_t* fs, const char* name) {
  fs_stat_info_t fst;
  return_value_if_fail(name != NULL, FALSE);

  if (fs_stat(fs, name, &fst) == RET_OK) {
    return fst.is_reg_file;
  } else {
    return FALSE;
  }
}

static ret_t fs_os_file_rename(fs_t* fs, const char* name, const char* new_name) {
  return_value_if_fail(name != NULL && new_name != NULL, RET_BAD_PARAMS);

  if (rename(name, new_name) == 0) {
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static const fs_dir_vtable_t s_dir_vtable = {
    .read = fs_os_dir_read, .rewind = fs_os_dir_rewind, .close = fs_os_dir_close};

static fs_dir_t* fs_dir_create(void) {
  fs_dir_t* d = NULL;
  fs_dir_posix_t* fdir = TKMEM_ZALLOC(fs_dir_posix_t);

  if (fdir != NULL) {
    d = (fs_dir_t*)fdir;
    d->vt = &s_dir_vtable;
  }

  return d;
}

static fs_dir_t* fs_os_open_dir(fs_t* fs, const char* name) {
  DIR* dp = NULL;
  fs_dir_t* dir = NULL;
  return_value_if_fail(name != NULL, NULL);
  dir = fs_dir_create();
  return_value_if_fail(dir != NULL, NULL);
  dp = opendir(name);
  if (dp != NULL) {
    ((fs_dir_posix_t*)dir)->dir = dp;
    return dir;
  } else {
    TKMEM_FREE(dir);
    return NULL;
  }
}

static ret_t fs_os_remove_dir(fs_t* fs, const char* name) {
  return_value_if_fail(name != NULL, RET_FAIL);
  if (rmdir(name) == 0) {
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static ret_t fs_os_create_dir(fs_t* fs, const char* name) {
  return_value_if_fail(name != NULL, RET_FAIL);

  if (mkdir(name, 0755) == 0) {
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static bool_t fs_os_dir_exist(fs_t* fs, const char* name) {
  fs_stat_info_t fst;
  return_value_if_fail(name != NULL, FALSE);

  if (fs_stat(fs, name, &fst) == RET_OK) {
    return fst.is_dir;
  } else {
    return FALSE;
  }
}

static ret_t fs_os_dir_rename(fs_t* fs, const char* name, const char* new_name) {
  return_value_if_fail(name != NULL && new_name != NULL, RET_BAD_PARAMS);

  if (rename(name, new_name) == 0) {
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static int32_t fs_os_get_file_size(fs_t* fs, const char* name) {
  fs_stat_info_t fst;
  return_value_if_fail(name != NULL, FALSE);

  if (fs_stat(fs, name, &fst) == RET_OK) {
    return fst.size;
  } else {
    return FALSE;
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
  tk_strcpy(path, TK_APP_DATA_DIR);

  return RET_OK;
}

static ret_t fs_os_get_temp_path(fs_t* fs, char path[MAX_PATH + 1]) {
  tk_strcpy(path, TK_TEMP_DIR);

  return RET_OK;
}

static ret_t fs_os_get_cwd(fs_t* fs, char cwd[MAX_PATH + 1]) {
  return_value_if_fail(cwd != NULL, RET_BAD_PARAMS);

  if (getcwd(cwd, MAX_PATH) != NULL) {
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static ret_t fs_os_stat(fs_t* fs, const char* name, fs_stat_info_t* fst) {
  struct stat buf;
  return_value_if_fail(name != NULL && fst != NULL, RET_BAD_PARAMS);

  memset(fst, 0x00, sizeof(fs_stat_info_t));
  if (stat(name, &buf) == 0) {
    return fs_stat_info_from_stat(fst, &buf);
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

fs_t* os_fs_posix(void) {
#ifdef WITH_FS_MT
  return fs_mt_wrap((fs_t*)&s_os_fs);
#else
  return (fs_t*)&s_os_fs;
#endif /*WITH_FS_MT*/
}
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
#else
fs_t* os_fs(void) {
  return os_fs_posix();
}
#endif
