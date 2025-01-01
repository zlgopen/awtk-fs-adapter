/**
 * File:   fs_mt.c
 * Author: AWTK Develop Team
 * Brief:  make fs to support multi thread
 *
 * Copyright (c) 2020 - 2025 Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * 2021-04-10 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#ifndef TK_FS_OS_MT_H
#define TK_FS_OS_MT_H

#include "tkc/str.h"

BEGIN_C_DECLS

/**
 * @method fs_mt_wrap
 * 把fs对象包装成可以多线程访问的fs对象。
 * @annotation ["global"]
 * @param {fs_t*} fs fs对象。
 *
 * @return {fs_t*} 可以多线程访问的fs对象。
 */
fs_t* fs_mt_wrap(fs_t* impl);

END_C_DECLS

#endif /*TK_FS_OS_MT_H*/
