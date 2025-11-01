#pragma once
#include <cstdint>

// Počet i-nodů v souborovém systému
constexpr int32_t INODE_COUNT = 128;

// Konstantní hodnoty pro počet clusterů
constexpr int32_t CLUSTER_COUNT = 1024;

// Velikost jednoho clusteru v bajtech
constexpr int32_t CLUSTER_SIZE = 4096;

#pragma pack(push, 1)
struct superblock{
    char signature[9];               // Název souborového systému
    char volume_descriptor[251];     // Popis FS
    int32_t disk_size;               // Velikost disku v bytech
    int32_t cluster_size;            // Velikost jednoho clusteru v bytech
    int32_t cluster_count;           // Celkový počet clusterů
    int32_t bitmapi_start_adress;    // Adresa začátku bitmapy i-nodů
    int32_t bitmap_start_adress;     // Adresa začátku bitmapy clusterů
    int32_t inode_start_adress;      // Adresa začátku i-nodů 
    int32_t data_start_adress;     
};
#pragma pack(pop)

int init_superblock(superblock* sb, int size);