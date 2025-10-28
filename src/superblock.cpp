#include <cstdint>
#include <cstring>
#include "superblock.hpp"
#include "pseudo_inode.hpp"

int init_superblock(superblock* sb, int size){
    if(!sb){
        return 1;
    }
    if(size <= 0){
        return 2;  // neplatná velikost
    }
    
    memset(sb, 0, sizeof(superblock));
    
    strncpy(sb->signature, "pavelkr", sizeof(sb->signature) - 1);
    sb->signature[sizeof(sb->signature) - 1] = '\0';
    strncpy(sb->volume_descriptor, "Toto je muj FS", sizeof(sb->volume_descriptor) - 1);
    sb->volume_descriptor[sizeof(sb->volume_descriptor) - 1] = '\0';
    
    sb->cluster_size = CLUSTER_SIZE;

    int32_t size_bytes = size * 1024 * 1024; 
    int32_t metadata_size = sizeof(superblock) + (INODE_COUNT + 7) / 8 + sizeof(pseudo_inode) * INODE_COUNT;
    int32_t available_for_data = size_bytes - metadata_size;
    
    if(available_for_data < CLUSTER_SIZE){
        return 3;  
    }
    
    sb->cluster_count = available_for_data / CLUSTER_SIZE;
    
    // Přesnější výpočet s bitmapou:
    int32_t bitmap_cluster_size = (sb->cluster_count + 7) / 8;
    sb->cluster_count = (available_for_data - bitmap_cluster_size) / CLUSTER_SIZE;
    
    // Přepočítej skutečnou velikost disku
    sb->disk_size = sizeof(superblock) 
                  + (INODE_COUNT + 7) / 8 
                  + (sb->cluster_count + 7) / 8 
                  + sizeof(pseudo_inode) * INODE_COUNT 
                  + sb->cluster_count * CLUSTER_SIZE;
    
    sb->bitmapi_start_adress = sizeof(superblock);
    sb->bitmap_start_adress = sb->bitmapi_start_adress + (INODE_COUNT + 7) / 8;
    sb->inode_start_adress = sb->bitmap_start_adress + (sb->cluster_count + 7) / 8;
    sb->data_start_adress = sb->inode_start_adress + INODE_COUNT * sizeof(pseudo_inode);

    return 0;
}