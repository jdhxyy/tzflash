// Copyright 2019-2021 The jdh99 Authors. All rights reserved.
// flash����ģ��
// Authors: jdh99 <jdh821@163.com>

#include "tzflash.h"
#include "tzmalloc.h"

#include <string.h>

#define CACHE_SIZE 64

#pragma pack(1)

// tFlashInterface flash����ӿ�
typedef struct {
    int pageSize;
    int alignNum;
    TZFlashEraseFunc erase;
    TZFlashWriteFunc write;
    TZFlashReadFunc read;
} tFlashInterface;

// tFlashFile flash�ļ�����
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

// tzmalloc�û�id
static int gMid = -1;

// TZFlashLoad ģ������
// pageSize��flashҳ��С,alignNum���ֽڶ�����
void TZFlashLoad(int pageSize, int alignNum, TZFlashEraseFunc eraseFunc, 
    TZFlashWriteFunc writeFunc, TZFlashReadFunc readFunc) {
    gInterface.pageSize = pageSize;
    gInterface.alignNum = alignNum;
    gInterface.erase = eraseFunc;
    gInterface.write = writeFunc;
    gInterface.read = readFunc;

    gMid = TZMallocRegister(0, TZFALSH_MALLOC_TAG, TZFLASH_MALLOC_TOTAL);
}

// TZFlashOpen ��flash�ļ�.
// ע��:ֻдģʽ��ʱ�����flash.Ҫ��֤�򿪵�flash��С��ҳ��������,������ʧ��
// ����ֵΪ0��ʾ��ʧ��
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

// TZFlashWrite ��flashд������
// �ɹ�����д����ֽ���,ʧ�ܷ���-1
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
        // ��cache��д��ռ�С��ֱ��д��
        if (size < delta) {
            memcpy(file->cache + file->cacheLen, bytes + writeNum, (size_t)size);
            file->cacheLen += (uint32_t)size;
            writeNum += size;
            break;
        }

        // д��cache,��ȫ��д��flash
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

// TZFlashRead ��flash�ж�ȡ����
// �ɹ����ض�ȡ���ֽ���,ʧ�ܷ���-1
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

// TZFlashClose �ر�
void TZFlashClose(intptr_t fd) {
    tFlashFile* file = (tFlashFile*)fd;
    if (file->mode != TZFLASH_READ_ONLY && file->cacheLen > 0) {
        gInterface.write(file->offset, file->cache, (int)file->cacheLen);
    }
    TZFree(file);
}

// TZFlashClose �ر�.�ر�ʱ�ὲ��������д�뵽flash,������������,��������0
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

// TZFlashSeek ����ƫ����
// offset�������ʼ��ַ��ƫ����.ע�������д��ģʽ,�������ض���ƫ�������Ѿ�д��������
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

// TZFlashErase ����flash
// ֻ��ģʽ�²����Բ���
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

// TZFlashGetOffset ��ȡ��ǰƫ����
int TZFlashGetOffset(intptr_t fd) {
    tFlashFile* file = (tFlashFile*)fd;
    return (int)(file->offset - file->addrStart);
}

// TZFlashSync ��δд�����̵�����д�뵽����
// ע��:������ֽڶ��봦��
// �����߼�:һ����4�ֽ�,��д����ֽ�����4�ı���,�����ǰ�������ֽ�������4�ı���,���������ֽ�û��д��
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
