# awtk-fs-adapter

## 介绍

AWTK 文件系统适配器。

在嵌入式平台中，有时没有 posix 兼容的文件系统 API，需要把一些文件系统实现，包装成 AWTK 的 fs 接口。本项目提供一些常见文件系统的适配，目前支持的文件系统有：

* [FATFS](https://github.com/abbrev/fatfs) 主要用于访问 TF card。

* [SPIFFS](https://github.com/pellepl/spiffs) 主要用于访问 Nor Flash。

* POSIX 文件系统，兼容 RT-Thread 的文件 DFS 系统接口。
  
## PC 编译
1. 获取 awtk 并编译

```
git clone https://github.com/zlgopen/awtk.git
cd awtk; scons; cd -
```

> PC 版本主要用于功能性测试。

2. 获取 awtk-fs-adapter 并编译

```
git clone https://github.com/zlgopen/awtk-fs-adapter.git
cd awtk-fs-adapter; scons
```
## 嵌入式系统编译

将相应的文件加入工程。

> 如果需要支持多线程，请定义宏 WITH\_FS\_MT，并加入文件 src/fs\_mt.c。

## 其它

* 用户数据目录和临时目录，在 src/fs\_os\_conf.h 中定义，请根据需要修改。
