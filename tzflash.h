// Copyright 2019-2021 The jdh99 Authors. All rights reserved.
// flash����ģ��
// Authors: jdh99 <jdh821@163.com>

#ifndef TZFLASH_H
#define TZFLASH_H

#include <stdint.h>
#include <stdbool.h>

// tzmalloc��ǩ���ֽ���
#define TZFALSH_MALLOC_TAG "tzflash"
#define TZFLASH_MALLOC_TOTAL 4096

// TZFlashEraseFunc ��������:����flash
// addr:��ʼ��ַ.size:�����ֽ���
// �ɹ�����true,ʧ�ܷ���false
typedef bool (*TZFlashEraseFunc)(uint32_t addr, int size);

// TZFlashWriteFunc ��������:д��flash
// addr:��ʼ��ַ.bytes:��д����ֽ���.size:д���ֽ���
// �ɹ�����true,ʧ�ܷ���false
typedef bool (*TZFlashWriteFunc)(uint32_t addr, uint8_t* bytes, int size);

// TZFlashReadFunc ��������:��ȡflash
// addr:��ʼ��ַ.bytes:��ȡ���ֽ�����ŵĻ���.size:��ȡ���ֽ���
// �ɹ�����true,ʧ�ܷ���false
typedef bool (*TZFlashReadFunc)(uint32_t addr, uint8_t* bytes, int size);

// TZFlashOpenMode ��ģʽ
typedef enum {
    TZFLASH_READ_ONLY = 0,
    TZFLASH_WRITE_ONLY,
    TZFLASH_READ_AND_WRITE
} TZFlashOpenMode;

// TZFlashLoad ģ������
// pageSize��flashҳ��С,alignNum���ֽڶ�����
bool TZFlashLoad(int pageSize, int alignNum, TZFlashEraseFunc eraseFunc, 
    TZFlashWriteFunc writeFunc, TZFlashReadFunc readFunc);

// TZFlashOpen ��flash�ļ�.
// ע��:��ʱ�����flash.Ҫ��֤�򿪵�flash��С��ҳ��������,������ʧ��
// ����ֵΪ0��ʾ��ʧ��
intptr_t TZFlashOpen(uint32_t addrStart, int size, TZFlashOpenMode mode);

// TZFlashWrite ��flashд������
// �ɹ�����д����ֽ���,ʧ�ܷ���-1
int TZFlashWrite(intptr_t fd, uint8_t* bytes, int size);

// TZFlashRead ��flash�ж�ȡ����
// �ɹ����ض�ȡ���ֽ���,ʧ�ܷ���-1
int TZFlashRead(intptr_t fd, uint8_t* bytes, int size);

// TZFlashClose �ر�.�ر�ʱ�ὲ��������д�뵽flash,������������,��������0
void TZFlashClose(intptr_t fd);

// TZFlashSeek ����ƫ����
// offset�������ʼ��ַ��ƫ����.ע�������д��ģʽ,�������ض���ƫ�������Ѿ�д��������
bool TZFlashSeek(intptr_t fd, int offset);

// TZFlashErase ����flash
// ֻ��ģʽ�²����Բ���
bool TZFlashErase(intptr_t fd);

// TZFlashGetOffset ��ȡ��ǰƫ����
int TZFlashGetOffset(intptr_t fd);

// TZFlashSync ��δд�����̵�����д�뵽����
// ע��:������ֽڶ��봦��
// �����߼�:һ����4�ֽ�,��д����ֽ�����4�ı���,�����ǰ�������ֽ�������4�ı���,���������ֽ�û��д��
void TZFlashSync(intptr_t fd);

#endif
