#pragma once
#include <stdint.h>

struct DVDDiskID {
    char game_name[4];
    char company[2];
    uint8_t disk_number;
    uint8_t game_version;
    uint8_t is_streaming;
    uint8_t streaming_buffer_size;
    uint8_t padding[22];
};

struct DVDCommandBlock {
    DVDCommandBlock* next;
    DVDCommandBlock* previous;
    uint32_t command;
    signed int state;
    uint32_t offset;
    uint32_t length;
    void* buffer;
    uint32_t current_transfer_size;
    uint32_t transferred_size;
    DVDDiskID* disk_id;
    void (*DVDCBCallback)(signed int result, DVDCommandBlock* block);
    void* user_data;
};

struct DVDFileInfo {
    DVDCommandBlock block;
    uint32_t start_address;
    uint32_t length;
    void (*DVDCallback)(signed int result, DVDFileInfo* fileInfo);
};

extern "C" {
bool DVDOpen(const char* fileName, DVDFileInfo* fileInfo);
signed long DVDReadPrio(DVDFileInfo* fileInfo, void* buffer, signed long length, signed long offset, signed long prio);
bool DVDClose(DVDFileInfo* fileInfo);
}