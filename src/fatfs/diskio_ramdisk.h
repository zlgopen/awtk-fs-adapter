#ifndef _RAMDISK_H_
#define _RAMDISK_H_

int RAM_disk_status(void);
int RAM_disk_initialize(void);
int RAM_disk_read(unsigned char* buff, unsigned int sector, unsigned int count);
int RAM_disk_write(const unsigned char* buff, unsigned int sector, unsigned int count);
int RAM_disk_ioctl(unsigned char cmd, void* buff);
DWORD get_fattime(void);

#endif
