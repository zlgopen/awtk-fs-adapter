#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif /*WIN32_LEAN_AND_MEAN*/

#include "ff.h"
#include "tkc/fs.h"

extern fs_t* os_fs_fatfs(void);

int main(int argc, char* argv[]) {
  FATFS fatfs;
  BYTE work[FF_MAX_SS];
  fs_t* fs = os_fs_fatfs();
  assert(f_mkfs("0:", FM_FAT, 0, work, sizeof(work)) == FR_OK);
  assert(f_mount(&fatfs, "0:", 0) == FR_OK);

  fs_test(fs);

  assert(f_mount(0, "0:", 0) == FR_OK);

  return 0;
}
