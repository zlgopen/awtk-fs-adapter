import os
import scripts.app_helper as app

helper = app.Helper(ARGUMENTS);

APP_CCFLAGS = '-DWITH_RAM_DISK -DWITH_FS_MT '
APP_CPPPATH=[
  os.path.join(helper.APP_ROOT, 'src/fatfs/include'),
  os.path.join(helper.APP_ROOT, 'src/fatfs/ff/include'),
  os.path.join(helper.APP_ROOT, 'src/spiffs/default'),
  os.path.join(helper.APP_ROOT, 'src/spiffs')
]
helper.add_cpppath(APP_CPPPATH).add_ccflags(APP_CCFLAGS).call(DefaultEnvironment)

SConscript(['src/SConscript', 'demos/SConscript'])
