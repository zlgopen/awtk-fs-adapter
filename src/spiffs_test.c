#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif /*WIN32_LEAN_AND_MEAN*/

#include "tkc/fs.h"
#include "spiffs/spiffs.h"

extern fs_t* os_fs_spiffs(void);
extern ret_t os_fs_spiffs_set(spiffs* fs);
s32_t fs_mount_ram(spiffs* fs, void* start_addr, uint32_t size);

int main(int argc, char* argv[]) {
  spiffs myfs;
  uint8_t flash[20 * 1024];
  int ret = fs_mount_ram(&myfs, flash, sizeof(flash));

  os_fs_spiffs_set(&myfs);
  fs_test_file(os_fs_spiffs());

  return 0;
}
