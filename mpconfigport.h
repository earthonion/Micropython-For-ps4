/* This file is part of the MicroPython project, http://micropython.org/
 * The MIT License (MIT)
 * Copyright (c) 2022-2023 Damien P. George
 */

// Include common MicroPython embed configuration.
#include <port/mpconfigport_common.h>
// ---- Core features you need for file I/O + imports
#define MICROPY_CONFIG_ROM_LEVEL                (MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES)

#define MICROPY_ENABLE_COMPILER                 (1)
#define MICROPY_ENABLE_GC                       (1)

#define MICROPY_PY_SYS                          (1)   // needed to set sys.path
#define MICROPY_PY_IO                           (1)   // enables open()
#define MICROPY_PY_UOS                          (1)   // os-like APIs (listdir, stat, etc.)
#define MICROPY_VFS                             (1)
#define MICROPY_VFS_POSIX                       (1)   // use host POSIX I/O
#define MICROPY_PY_UTIME                        (1)   // time + sleeps needed by many libs


#define MICROPY_PY_UJSON                        (1)
#define MICROPY_PY_URE                          (1)
#define MICROPY_PY_STRUCT                       (1)
#define MICROPY_PY_BINASCII                     (1)

void mp_embed_write(const char *str, size_t len);
#define MP_PLAT_PRINT_STRN(str, len) mp_embed_write(str, output)
