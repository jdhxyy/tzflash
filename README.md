# tzflash

## 1. 介绍
flash管理模块。

## 2. 功能
- TZFlash将flash的读写抽象成文件读写，更易于使用
- 大部分flash写入需要4字节对齐，TZFlash增加cache机制，可以将任意长度的写入flash

## 3. API
```c
// TZFlashLoad 模块载入
// pageSize是flash页大小,alignNum是字节对齐数
void TZFlashLoad(int pageSize, int alignNum, TZFlashEraseFunc eraseFunc, 
    TZFlashWriteFunc writeFunc, TZFlashReadFunc readFunc);

// TZFlashOpen 打开flash文件.
// 注意:打开时会擦除flash.要保证打开的flash大小是页的整数倍,否则会打开失败
// 返回值为0表示打开失败
intptr_t TZFlashOpen(uint32_t addrStart, int size, TZFlashOpenMode mode);

// TZFlashWrite 向flash写入数据
// 成功返回写入的字节数,失败返回-1
int TZFlashWrite(intptr_t fd, uint8_t* bytes, int size);

// TZFlashRead 从flash中读取数据
// 成功返回读取的字节数,失败返回-1
int TZFlashRead(intptr_t fd, uint8_t* bytes, int size);

// TZFlashClose 关闭.关闭时会讲所有数据写入到flash,如果不满足对齐,则会在最后补0
void TZFlashClose(intptr_t fd);

// TZFlashSeek 设置偏移量
// offset是相对起始地址的偏移量.注意如果是写入模式,不允许重定义偏移量到已经写过的区域
bool TZFlashSeek(intptr_t fd, int offset);

// TZFlashErase 擦除flash
// 只读模式下不可以擦除
bool TZFlashErase(intptr_t fd);

// TZFlashGetOffset 获取当前偏移量
int TZFlashGetOffset(intptr_t fd);

// TZFlashSync 将未写到磁盘的数据写入到磁盘
// 注意:会根据字节对齐处理
// 处理逻辑:一般是4字节,则写入的字节数是4的倍数,如果当前缓存里字节数不是4的倍数,会有若干字节没有写入
void TZFlashSync(intptr_t fd);
```
