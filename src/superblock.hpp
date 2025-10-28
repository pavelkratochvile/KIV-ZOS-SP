#pragma once
#include <cstdint>

constexpr int32_t ID_ITEM_FREE = 0;
constexpr int32_t INODE_COUNT = 128;
constexpr int32_t CLUSTER_COUNT = 1024;
constexpr int32_t CLUSTER_SIZE = 4096;

#pragma pack(push, 1)
struct superblock{
    char signature[9];
    char volume_descriptor[251];
    int32_t disk_size;
    int32_t cluster_size;
    int32_t cluster_count;
    int32_t bitmapi_start_adress;
    int32_t bitmap_start_adress;
    int32_t inode_start_adress;
    int32_t data_start_adress;
};
#pragma pack(pop)

int init_superblock(superblock* sb, int size);