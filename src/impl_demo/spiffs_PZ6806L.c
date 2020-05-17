#include "spiffs.h"
#include <assert.h>
#include "flash.h"

static s32_t _read(
#if SPIFFS_HAL_CALLBACK_EXTRA
    spiffs* fs,
#endif
    u32_t addr, u32_t size, u8_t* dst) {
  
  
  EN25QXX_Read(dst, addr, size);
  
  return 0;      
}

static s32_t _write(
#if SPIFFS_HAL_CALLBACK_EXTRA
    spiffs* fs,
#endif
    u32_t addr, u32_t size, u8_t* src) {
  EN25QXX_Write(src, addr, size);
  
  return 0;     
}
static s32_t _erase(
#if SPIFFS_HAL_CALLBACK_EXTRA
    spiffs* fs,
#endif
    u32_t addr, u32_t size) {
  EN25QXX_Erase_Sector(addr);
      
  return 0;
}

static void spiffs_check_cb_f(
#if SPIFFS_HAL_CALLBACK_EXTRA
    spiffs* fs,
#endif
    spiffs_check_type type, spiffs_check_report report, u32_t arg1, u32_t arg2) {
  return;
}

static u8_t _fds[64];
static u8_t _work[512];
static u8_t _cache[4096];
static u32_t _fds_sz = 64;
static u32_t _cache_sz = 4096;

s32_t fs_mount_flash(spiffs* fs) {
  spiffs_config c;

  c.hal_erase_f = _erase;
  c.hal_read_f = _read;
  c.hal_write_f = _write;

#if SPIFFS_SINGLETON == 0
  c.phys_erase_block = 512;
  c.log_block_size = 1024;
  c.log_page_size = 256;
  c.phys_size = 100 * 1024;
  c.phys_addr = 0;
#endif

  return SPIFFS_mount(fs, &c, _work, _fds, _fds_sz, _cache, _cache_sz, spiffs_check_cb_f);
}
