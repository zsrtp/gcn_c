#include "card.h"
#include <stddef.h>

// __CARDBlock variable used in main code
// 0x8044cbc0 in US; Specifically have to use this one and not make a new one
typedef struct CARDBlock {
    uint8_t unk[0x110];
} CARDBlock;
//static_assert(sizeof(CARDBlock) == 0x110);

extern CARDBlock __CARDBlock[2]; // One for each memory card slot

// Vanilla functions used in main code
void __CARDDefaultApiCallback(int32_t chn, int32_t result);
void __CARDSyncCallback(int32_t chn, int32_t result);
int32_t __CARDGetControlBlock(int32_t chn, void** card);
int32_t __CARDPutControlBlock(void* card, int32_t result);
int32_t __CARDSync(int32_t chn);
int32_t __CARDUpdateFatBlock(int32_t chn, void* fatBlock, CARDCallback callback);
void* __CARDGetDirBlock(void* card);
int32_t __CARDUpdateDir(int32_t chn, CARDCallback callback);
int32_t memcmp(const void* str1, const void* str2, size_t n);
int32_t strcmp(const char* str1, const char* str2);
void* memset(void* dst, int val, size_t n);

// Main code
int32_t card_free_block(int32_t chn, uint16_t block, CARDCallback callback)
{
    uint32_t card = (uint32_t)(&__CARDBlock[chn]);
    if (*(int32_t*)card == 0)
    {
        return NoCard;
    }
    
    uint16_t* fatBlock = *(uint16_t**)(card + 0x88);
    
    while (block != 0xFFFF)
    {
        uint32_t tempBlock = (uint32_t)(block);
        if ((tempBlock < 5) || (*(uint16_t*)(card + 0x10) <= tempBlock))
        {
            return Broken;
        }
        
        block = fatBlock[tempBlock];
        fatBlock[tempBlock] = 0;
        fatBlock[3] += 1;
    }
    
    return __CARDUpdateFatBlock(chn, fatBlock, callback);
}

void delete_callback(int32_t chn, int32_t result) {
    uint32_t card = (uint32_t)(&__CARDBlock[chn]);
    CARDCallback* cardApiCbAddress = (CARDCallback*)(card + 0xD0);
    CARDCallback cb = *cardApiCbAddress;
    *cardApiCbAddress = (CARDCallback)NULL;
    
    int32_t ret = result;
    if (ret >= Ready)
    {
        uint16_t* currFileBlockAddr = (uint16_t*)(card + 0xBE);
        
        ret = card_free_block(chn, *currFileBlockAddr, cb);
        if (ret >= Ready)
        {
            return;
        }
    }
    
    __CARDPutControlBlock((void*)card, ret);
    
    if (cb)
    {
        cb(chn, ret);
    }
}

int32_t card_get_file_no(void* card, const char* fileName, int32_t* fileNo)
{
    int32_t cardIsAttached = *(int32_t*)((uint32_t)card);
    if (cardIsAttached == 0)
    {
        return NoCard;
    }
    
    uint32_t dirBlock = (uint32_t)(__CARDGetDirBlock(card));
    uint8_t* cardDiskGameCode = *(uint8_t**)(((uint32_t)card) + 0x10C);
    
    int32_t i;
    for (i = 0; i < 127; i++)
    {
        uint8_t* currentDirBlock = (uint8_t*)(dirBlock + (i * 0x40));
        uint8_t* gameCode = &currentDirBlock[0];
        
        if (gameCode[0] != 0xFF)
        {
            const char* currentFileName = (const char*)(&currentDirBlock[0x8]);
            if (strcmp(fileName, currentFileName) == 0)
            {
                if ((cardDiskGameCode && (cardDiskGameCode[0] != 0xFF) && 
                   (memcmp(&gameCode[0], &cardDiskGameCode[0], 4) != 0)) || 
                   ((cardDiskGameCode[0x4] != 0xFF) && 
                   (memcmp(&gameCode[0x4], &cardDiskGameCode[0x4], 2) != 0)))
                {
                    continue;
                }

                *fileNo = i;
                break;
            }
        }
    }
    
    if (i >= 127)
    {
        return NoFile;
    }
    
    return Ready;
}

int32_t CARDDeleteAsync(int32_t chn, const char* fileName, CARDCallback callback)
{
    uint32_t card;
    int32_t ret = __CARDGetControlBlock(chn, (void**)(&card));
    if (ret < Ready)
    {
        return ret;
    }
    
    int32_t fileNo;
    ret = card_get_file_no((void*)card, fileName, &fileNo);
    if(ret < Ready)
    {
        __CARDPutControlBlock((void*)card, ret);
        return ret;
    }
    
    uint32_t dirBlock = (uint32_t)(__CARDGetDirBlock((void*)card));
    uint32_t entry = dirBlock + (fileNo * 0x40);
    
    uint16_t* blockAddr = (uint16_t*)(entry + 0x36);
    uint16_t* currFileBlockAddr = (uint16_t*)(card + 0xBE);
    *currFileBlockAddr = *blockAddr;
    
    memset((void*)entry, -1, 0x40);
    
    CARDCallback cb = callback;
    if (!cb)
    {
        cb = __CARDDefaultApiCallback;
    }
    
    CARDCallback* cardApiCbAddress = (CARDCallback*)(card + 0xD0);
    *cardApiCbAddress = cb;
    
    ret = __CARDUpdateDir(chn, delete_callback);
    if (ret >= Ready)
    {
        return ret;
    }

    __CARDPutControlBlock((void*)card, ret);
    return ret;
}

int32_t CARDDelete(int32_t chn, const char* fileName)
{
    int32_t ret = CARDDeleteAsync(chn, fileName, __CARDSyncCallback);
    if (ret >= Ready)
    {
        ret = __CARDSync(chn);
    }
    return ret;
}