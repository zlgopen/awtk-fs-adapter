#include "ff.h"
#include <string.h>
#include "diskio.h"
#include "fatfs_diskio.h"

#ifdef WITH_RAM_DISK
/*
 * https://github.com/David-Croose/FatFS_MinGW
 */
#define CFG_RAMDISK_SIZE (1000 * 1024)
#define CFG_RAMDISK_SECTOR_SIZE (512)
#define RAMDISK_SECTOR_TOTAL ((CFG_RAMDISK_SIZE) / (CFG_RAMDISK_SECTOR_SIZE))

static unsigned char rambuf[RAMDISK_SECTOR_TOTAL][CFG_RAMDISK_SECTOR_SIZE];

int RAM_disk_status(void) {
  return 0;
}

int RAM_disk_initialize(void) {
  return 0;
}

int RAM_disk_read(unsigned char* buff, unsigned int sector, unsigned int count) {
  memcpy(buff, &rambuf[sector], count * CFG_RAMDISK_SECTOR_SIZE);
  return 0;
}

int RAM_disk_write(const unsigned char* buff, unsigned int sector, unsigned int count) {
  memcpy(&rambuf[sector], buff, count * CFG_RAMDISK_SECTOR_SIZE);
  return 0;
}

int RAM_disk_ioctl(unsigned char cmd, void* buff) {
  int res;

  switch (cmd) {
    case CTRL_SYNC:
      res = RES_OK;
      break;
    case GET_SECTOR_SIZE:
      *(DWORD*)buff = CFG_RAMDISK_SECTOR_SIZE;
      res = RES_OK;
      break;
    case GET_BLOCK_SIZE:
      *(WORD*)buff = 1;
      res = RES_OK;
      break;
    case GET_SECTOR_COUNT:
      *(DWORD*)buff = RAMDISK_SECTOR_TOTAL;
      res = RES_OK;
      break;
    default:
      res = RES_PARERR;
      break;
  }

  return res;
}

DWORD get_fattime(void) {
  return 0;
}

DSTATUS ff_disk_status(BYTE pdrv) {
  return RAM_disk_status();
}

DSTATUS ff_disk_initialize(BYTE pdrv) {
  return RAM_disk_initialize();
}

DRESULT ff_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
  return RAM_disk_read(buff, sector, count);
}

DRESULT ff_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) {
  return RAM_disk_write(buff, sector, count);
}

DRESULT ff_disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
  return RAM_disk_ioctl(cmd, buff);
}

#endif /*WITH_RAM_DISK*/
