// Copyright 2019-2021 The jdh99 Authors. All rights reserved.
// flash管理模块
// Authors: jdh99 <jdh821@163.com>

#include "tzflash.h"
#include "tzmalloc.h"

#include <string.h>

#define CACHE_SIZE 64

#pragma pack(1)

// tFlashInterface flash管理接口
typedef struct {
    int pageSize;
    int alignNum;
    TZFlashEraseFunc erase;
    TZFlashWriteFunc write;
    TZFlashReadFunc read;
} tFlashInterface;

// tFlashFile flash文件类型
typedef struct {
    uint32_t addrStart;
    uint32_t addrStop;
    uint32_t offset;
    TZFlashOpenMode mode;
    uint8_t cache[CACHE_SIZE];
    uint32_t cacheLen;
} tFlashFile;

#pragma pack()

static tFlashInterface gInterface;

// tzmalloc用户id
static int gMid = -1;

// TZFlashLoad 模块载入
// pageSize是flash页大小,alignNum是字节对齐数
void TZFlashLoad(int pageSize, int alignNum, TZFlashEraseFunc eraseFunc, 
    TZFlashWriteFunc writeFunc, TZFlashReadFunc readFunc) {
    gInterface.pageSize = pageSize;
    gInterface.alignNum = alignNum;
    gInterface.erase = eraseFunc;
    gInterface.write = writeFunc;
    gInterface.read = readFunc;

    gMid = TZMallocRegister(0, TZFALSH_MALLOC_TAG, TZFLASH_MALLOC_TOTAL);
}

// TZFlashOpen 打开flash文件.
// 注意:只写模式打开时会擦除flash.要保证打开的flash大小是页的整数倍,否则会打开失败
// 返回值为0表示打开失败
intptr_t TZFlashOpen(uint32_t addrStart, int size, TZFlashOpenMode mode) {
    if (mode == TZFLASH_WRITE_ONLY) {
        if (size % gInterface.pageSize != 0) {
            return 0;
        }
        if (gInterface.erase(addrStart, size) == false) {
            return 0;
        }
    }

    tFlashFile* file = TZMalloc(gMid, sizeof(tFlashFile));
    if (file == NULL) {
        return 0;
    }
    file->addrStart = addrStart;
    file->addrStop = addrStart + (uint32_t)size - 1;
    file->mode = mode;
    file->offset = file->addrStart;
    file->cacheLen = 0;
    return (intptr_t)file;
}

// TZFlashWrite 向flash写入数据
// 成功返回写入的字节数,失败返回-1
int TZFlashWrite(intptr_t fd, uint8_t* bytes, int size) {
    tFlashFile* file = (tFlashFile*)fd;
    if (file->mode == TZFLASH_READ_ONLY) {
        return -1;
    }
    if (file->offset + file->cacheLen + (uint32_t)size > file->addrStop) {
        return -1;
    }

    int delta = 0;
    int writeNum = 0;
    for (;;) {
        delta = CACHE_SIZE - (int)file->cacheLen;
        // 比cache可写入空间小则直接写入
        if (size < delta) {
            memcpy(file->cache + file->cacheLen, bytes + writeNum, (size_t)size);
            file->cacheLen += (uint32_t)size;
            writeNum += size;
            break;
        }

        // 写满cache,再全部写入flash
        memcpy(file->cache + file->cacheLen, bytes + writeNum, (size_t)delta);
        file->cacheLen += (uint32_t)delta;
        gInterface.write(file->offset, file->cache, (int)file->cacheLen);
        file->offset += file->cacheLen;
        file->cacheLen = 0;
        size -= delta;
        writeNum += delta;
    }
    return writeNum;
}

// TZFlashRead 从flash中读取数据
// 成功返回读取的字节数,失败返回-1
int TZFlashRead(intptr_t fd, uint8_t* bytes, int size) {
    tFlashFile* file = (tFlashFile*)fd;
    if (file->mode == TZFLASH_WRITE_ONLY) {
        return -1;
    }
    if (file->offset > file->addrStop) {
        return 0;
    }
    int delta = (int)(file->addrStop - file->offset + 1);
    int readSize = delta > size ? size : delta;
    gInterface.read(file->offset, bytes, readSize);
    file->offset += (uint32_t)readSize;
    return readSize;
}

// TZFlashClose 关闭
void TZFlashClose(intptr_t fd) {
    tFlashFile* file = (tFlashFile*)fd;
    if (file->mode != TZFLASH_READ_ONLY && file->cacheLen > 0) {
        gInterface.write(file->offset, file->cache, (int)file->cacheLen);
    }
    TZFree(file);
}

// TZFlashClose 关闭.关闭时会讲所有数据写入到flash,如果不满足对齐,则会在最后补0
void TZFlashClose(intptr_t fd) {
    tFlashFile* file = (tFlashFile*)fd;
    if (file->mode != TZFLASH_READ_ONLY && file->cacheLen > 0) {
        int size = file->cacheLen / gInterface.alignNum * gInterface.alignNum;
        if (size > 0) {
            gInterface.write(file->offset, file->cache, size);
            file->offset += (uint32_t)size;
        }
        file->cacheLen = file->cacheLen % gInterface.alignNum;
        if (file->cacheLen > 0) {
            memmove(file->cache, file->cache + size, file->cacheLen);
            memset(file->cache + file->cacheLen, 0, gInterface.alignNum - file->cacheLen);
            gInterface.write(file->offset, file->cache, gInterface.alignNum);
        }
    }
    TZFree(file);
}

// TZFlashSeek 设置偏移量
// offset是相对起始地址的偏移量.注意如果是写入模式,不允许重定义偏移量到已经写过的区域
bool TZFlashSeek(intptr_t fd, int offset) {
    tFlashFile* file = (tFlashFile*)fd;
    int newOffset = offset + (int)file->addrStart;
    if (file->mode == TZFLASH_WRITE_ONLY) {
        if (newOffset < (int)file->offset) {
            return false;
        }
    }
    if (newOffset > (int)file->addrStop) {
        return false;
    }
    file->offset = (uint32_t)newOffset;
    return true;
}

// TZFlashErase 擦除flash
// 只读模式下不可以擦除
bool TZFlashErase(intptr_t fd) {
    tFlashFile* file = (tFlashFile*)fd;
    if (file->mode == TZFLASH_READ_ONLY) {
        return false;
    }

    int size = (int)(file->addrStop - file->addrStart + 1);
    if (size % gInterface.pageSize != 0) {
        return false;
    }
    return gInterface.erase(file->addrStart, size);
}

// TZFlashGetOffset 获取当前偏移量
int TZFlashGetOffset(intptr_t fd) {
    tFlashFile* file = (tFlashFile*)fd;
    return (int)(file->offset - file->addrStart);
}

// TZFlashSync 将未写到磁盘的数据写入到磁盘
// 注意:会根据字节对齐处理
// 处理逻辑:一般是4字节,则写入的字节数是4的倍数,如果当前缓存里字节数不是4的倍数,会有若干字节没有写入
void TZFlashSync(intptr_t fd) {
    tFlashFile* file = (tFlashFile*)fd;
    if (file->mode != TZFLASH_READ_ONLY && file->cacheLen > 0) {
        int size = file->cacheLen / gInterface.alignNum * gInterface.alignNum;
        if (size == 0) {
            return;
        } 
        gInterface.write(file->offset, file->cache, size);
        file->offset += (uint32_t)size;
        file->cacheLen = file->cacheLen % gInterface.alignNum;
        if (file->cacheLen > 0) {
            memmove(file->cache, file->cache + size, file->cacheLen);
        }
    }
}
