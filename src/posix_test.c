#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif /*WIN32_LEAN_AND_MEAN*/

#include "tkc/fs.h"
#include "tkc/utils.h"
#include "tkc/thread.h"
#include "tkc/platform.h"

extern fs_t* os_fs_posix(void);

int main(int argc, char* argv[]) {
  fs_t* fs = os_fs_posix();
  fs_file_t* file = NULL;
  fs_dir_t* dir = NULL;
  str_t str;
  fs_item_t item;
  platform_prepare();
  str_init(&str, 1024);

  assert(fs_create_dir(fs, "test") == RET_OK);
  assert(fs_dir_exist(fs, "test") == TRUE);

  assert(fs_create_dir(fs, "test/test1") == RET_OK);
  assert(fs_dir_exist(fs, "test/test1") == TRUE);
  assert(fs_create_dir(fs, "test/test2") == RET_OK);
  assert(fs_dir_exist(fs, "test/test2") == TRUE);

  dir = fs_open_dir(fs, "test");
  assert(dir != NULL);

  while (fs_dir_read(dir, &item) == RET_OK) {
    if (item.name[0] == '.') {
      continue;
    }

    str_append(&str, item.name);
    str_append(&str, ":");
  }
  fs_dir_close(dir);

  log_debug("%s\n", str.str);
  assert(strcmp(str.str, "test1:test2:") == 0);

  file = fs_open_file(fs, "test/test1/test.txt", "wb+");
  assert(file != NULL);
  fs_file_write(file, "hello", 5);
  fs_file_sync(file);
  fs_file_close(file);

  assert(fs_file_exist(fs, "test/test1/test.txt") == TRUE);
  assert(fs_get_file_size(fs, "test/test1/test.txt") == 5);

  fs_file_rename(fs, "test/test1/test.txt", "test/test1/test1.txt");
  assert(fs_file_exist(fs, "test/test1/test1.txt") == TRUE);

  file = fs_open_file(fs, "test/test1/test1.txt", "r");
  if (file != NULL) {
    str_clear(&str);
    str.size = fs_file_read(file, str.str, str.capacity);
    str.str[str.size] = '\0';
    assert(strcmp(str.str, "hello") == 0);
    assert(fs_file_size(file) == 5);
    fs_file_close(file);
  }

  assert(fs_remove_file(fs, "test/test1/test1.txt") == RET_OK);
  assert(fs_file_exist(fs, "test/test1/test1.txt") == FALSE);

  assert(fs_remove_dir(fs, "test/test1") == RET_OK);
  assert(fs_dir_exist(fs, "test/test1") == FALSE);
  assert(fs_remove_dir(fs, "test/test2") == RET_OK);
  assert(fs_dir_exist(fs, "test/test2") == FALSE);

  assert(fs_remove_dir(fs, "test") == RET_OK);
  str_reset(&str);
  return 0;
}
