#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif /*WIN32_LEAN_AND_MEAN*/

#include "tkc/fs.h"
#include "spiffs/spiffs.h"

extern fs_t* os_fs_spiffs(void);
extern ret_t os_fs_spiffs_set(spiffs* fs);
extern s32_t fs_mount_ram(spiffs* fs);

int main(int argc, char* argv[]) {
  spiffs myfs;
  int ret = fs_mount_ram(&myfs);
  os_fs_spiffs_set(&myfs);
  fs_test_file(os_fs_spiffs());

  return 0;
}
