#include "spiffs.h"

#define RAM_SIZE 100 * 1024

static u8_t s_flash[RAM_SIZE];

static s32_t _read(
#if SPIFFS_HAL_CALLBACK_EXTRA
    spiffs *fs,
#endif
    u32_t addr, u32_t size, u8_t *dst) {
  memcpy(dst, s_flash + addr, size);
  return 0;
}

static s32_t _write(
#if SPIFFS_HAL_CALLBACK_EXTRA
    spiffs *fs,
#endif
    u32_t addr, u32_t size, u8_t *src) {
  int i;

  for (i = 0; i < size; i++) {
    s_flash[addr+i] = src[i]; 
  }
  return 0;
}
static s32_t _erase(
#if SPIFFS_HAL_CALLBACK_EXTRA
    spiffs *fs,
#endif
    u32_t addr, u32_t size) {
  memset(s_flash+addr, 0xff, size);
  return 0;
}

static void spiffs_check_cb_f(
#if SPIFFS_HAL_CALLBACK_EXTRA
        spiffs *fs,
#endif
    spiffs_check_type type, spiffs_check_report report,
    u32_t arg1, u32_t arg2) {
}

static u8_t _work[512];
static u8_t _fds[256];
static u32_t _fds_sz = 256;
static u8_t _cache[4096];
static u32_t _cache_sz = 4096;

s32_t fs_mount_ram(spiffs* fs) {
  spiffs_config c;
  c.hal_erase_f = _erase;
  c.hal_read_f = _read;
  c.hal_write_f = _write;
#if SPIFFS_SINGLETON == 0
  c.log_block_size = 4096;
  c.log_page_size = 256;
  c.phys_addr = 0;
  c.phys_erase_block = 4096;
  c.phys_size = RAM_SIZE;
#endif
  return SPIFFS_mount(fs, &c, _work, _fds, _fds_sz, _cache, _cache_sz, spiffs_check_cb_f);
}
