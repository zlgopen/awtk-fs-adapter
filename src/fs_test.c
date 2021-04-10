
#include "tkc/fs.h"
#include "tkc/utils.h"
#include "tkc/thread.h"
#include "tkc/platform.h"
#define NR 50
#define TEST_DATA "hello"

static fs_t* s_fs = NULL;

static void* work_thread(void* args) {
  uint32_t i = 0;
  char buff[32];
  fs_t* fs = s_fs;
  fs_file_t* fp = NULL;
  char path[MAX_PATH + 1];
  char filename[MAX_PATH + 1];
  uint32_t id = (uint32_t)tk_pointer_to_int(args);

  log_debug("%u start\n", id);
  for(i = 0; i < NR; i++) {
    tk_snprintf(path, MAX_PATH, "0:/%u/%u", id, i);
    tk_snprintf(filename, MAX_PATH, "0:/%u/%u/test.txt", id, i);
    if(!fs_dir_exist(fs, path)) {
     assert(fs_create_dir_r(fs, path) == RET_OK);
    }
    fp = fs_open_file(fs, filename, "w+");
    assert(fp != NULL);
    assert(fs_file_write(fp, TEST_DATA, strlen(TEST_DATA)) == strlen(TEST_DATA));
    assert(fs_file_close(fp) == RET_OK);
    assert(fs_get_file_size(fs, filename) == strlen(TEST_DATA));

    fp = fs_open_file(fs, filename, "rb");
    assert(fp != NULL);
    memset(buff, 0x00, sizeof(buff));
    assert(fs_file_read(fp, buff, sizeof(buff)) == strlen(TEST_DATA));
    assert(strcmp(buff, TEST_DATA) == 0);
    assert(fs_file_close(fp) == RET_OK);

    assert(fs_file_exist(fs, filename) == TRUE);
    assert(fs_remove_file(fs, filename) == RET_OK);
  }
  tk_snprintf(path, MAX_PATH, "0:/%u", id);
  assert(fs_remove_dir_r(fs, path) == RET_OK);
 
  return NULL;
}

void test_fs(fs_t* fs) {
  uint32_t i = 0;
  tk_thread_t* threads[20];

  s_fs = fs;
  fs_test(fs);

  for(i = 0; i < ARRAY_SIZE(threads); i++) {
    threads[i] = tk_thread_create(work_thread, tk_pointer_from_int(i));
    tk_thread_start(threads[i]);
  }
  
  for(i = 0; i < ARRAY_SIZE(threads); i++) {
    tk_thread_join(threads[i]);
    tk_thread_destroy(threads[i]);
    log_debug("%u stop\n", i);
  }
}
