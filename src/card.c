#include "card.h"
#include <string.h>

int __CARDUpdateFatBlock(int param_1, short *param_2, CARDCallback param_3);
int __CARDPutControlBlock(int *param_1, int param_2);
char* __CARDGetDirBlock(int param_1);
void __CARDSyncCallback(int chan, int result);
void __CARDDefaultApiCallback(int chan, int result);
int __CARDGetControlBlock(int param_1,int **param_2);
int __CARDUpdateDir(int param_1, CARDCallback param_2);
int __CARDSync(int param_1);

extern char* __CARDBlock;

int __CARDIsOpened() {
    return 0;
}

int __CARDFreeBlock(int param_1, uint16_t param_2, CARDCallback param_3)
{
    uint32_t uVar1;
    int iVar2;
    short *psVar3;
    
    if (__CARDBlock[param_1 * 0x44] == 0) {
        iVar2 = -3;
    }
    else {
        psVar3 = (short *)*((int *)__CARDBlock + param_1 * 0x44 + 0x22);
        while (param_2 != 0xffff) {
            uVar1 = (uint32_t)param_2;
            if ((param_2 < 5) || (*(uint16_t *)(__CARDBlock + param_1 * 0x44 + 4) <= uVar1)) {
                return -6;
            }
            param_2 = psVar3[uVar1];
            psVar3[uVar1] = 0;
            psVar3[3] = psVar3[3] + 1;
        }
        iVar2 = __CARDUpdateFatBlock(param_1,psVar3,param_3);
    }
    return iVar2;
}

void DeleteCallback(int param_1,int param_2)
{
    CARDCallback pcVar1;
    
    pcVar1 = (CARDCallback)*((int *)__CARDBlock + param_1 * 0x44 + 0x34);
    __CARDBlock[param_1 * 0x44 + 0x34] = 0;
    if (((param_2 < 0) ||
            (param_2 = __CARDFreeBlock(param_1,*(uint16_t *)((int)__CARDBlock + param_1 * 0x110 + 0xbe), pcVar1), param_2 < 0)) &&
         (__CARDPutControlBlock((int*)__CARDBlock + param_1 * 0x44,param_2), pcVar1 != (CARDCallback)0x0)) {
        (*pcVar1)(param_1,param_2);
    }
    return;
}

int __CARDGetFileNo(int *param_1,char *param_2,int *param_3)
{
    char cVar1;
    char cVar2;
    unsigned char bVar3;
    int iVar4;
    int iVar5;
    char *pcVar6;
    char *pcVar7;
    char *__s2;
    char *__s1;
    
    if (*param_1 == 0) {
        iVar5 = NoCard;
    }
    else {
        __s1 = (char *)__CARDGetDirBlock((int)param_1);
        iVar5 = 0;
        do {
            __s2 = (char *)param_1[0x43];
            if (*__s1 == -1) {
                iVar4 = -4;
            }
            else {
                if ((__s2 == __CARDBlock + 0x88) ||
                     ((iVar4 = memcmp(__s1,__s2,4), iVar4 == 0 &&
                        (iVar4 = memcmp(__s1 + 4,__s2 + 1,2), iVar4 == 0)))) {
                    iVar4 = 0;
                }
                else {
                    iVar4 = -10;
                }
            }
            if (-1 < iVar4) {
                pcVar6 = __s1 + 8;
                iVar4 = 0x20;
                pcVar7 = param_2;
                do {
                    iVar4 = iVar4 + -1;
                    if (iVar4 < 0) {
                        if (*pcVar7 == '\0') {
                            bVar3 = 1;
                        }
                        else {
                            bVar3 = 0;
                        }
                        goto LAB_802a49bc;
                    }
                    cVar1 = *pcVar6;
                    pcVar6 = pcVar6 + 1;
                    cVar2 = *pcVar7;
                    pcVar7 = pcVar7 + 1;
                    if (cVar1 != cVar2) {
                        bVar3 = 0;
                        goto LAB_802a49bc;
                    }
                } while (cVar2 != '\0');
                bVar3 = 1;
LAB_802a49bc:
                if (bVar3) {
                    *param_3 = iVar5;
                    return Ready;
                }
            }
            iVar5 = iVar5 + 1;
            __s1 = __s1 + 0x40;
        } while (iVar5 < 0x7f);
        iVar5 = NoFile;
    }
    return iVar5;
}

int32_t CARDDeleteAsync(int chan,char *fileName,CARDCallback callback)
{
    CARDCallback pcVar1;
    int iVar2;
    char* pcVar2;
    void *__s;
    int local_1c;
    int *local_18 [3];
    
    iVar2 = __CARDGetControlBlock(chan,local_18);
    if (-1 < iVar2) {
        iVar2 = __CARDGetFileNo(local_18[0],fileName,&local_1c);
        if (iVar2 < 0) {
            iVar2 = __CARDPutControlBlock(local_18[0],iVar2);
        }
        else {
            iVar2 = __CARDIsOpened();
            if (iVar2 == Ready) {
                pcVar2 = __CARDGetDirBlock((int)local_18[0]);
                __s = (void *)(pcVar2 + local_1c * 0x40);
                *(uint16_t *)((int)local_18[0] + 0xbe) = *(uint16_t *)((int)__s + 0x36);
                memset(__s,0xff,0x40);
                pcVar1 = callback;
                if (callback == (CARDCallback )0x0) {
                    pcVar1 = __CARDDefaultApiCallback;
                }
                *(CARDCallback *)(local_18[0] + 0x34) = pcVar1;
                iVar2 = __CARDUpdateDir(chan,DeleteCallback);
                if (iVar2 < 0) {
                    __CARDPutControlBlock(local_18[0],iVar2);
                }
            }
            else {
                iVar2 = __CARDPutControlBlock(local_18[0],-1);
            }
        }
    }
    return iVar2;
}

int32_t CARDDelete(int chan,char *fileName)
{
    int iVar1;
    
    iVar1 = CARDDeleteAsync(chan,fileName,__CARDSyncCallback);
    if (-1 < iVar1) {
        iVar1 = __CARDSync(chan);
    }
    return iVar1;
}