#pragma once
#include <cstdint>
#include "superblock.hpp"

constexpr int DIRECT_COUNT = 5;
constexpr int INDIRECT_COUNT = 2;

struct pseudo_inode {
    int32_t nodeid;                 //ID i-uzlu, pokud ID = ID_ITEM_FREE, je polozka volna
    bool isDirectory;               //soubor, nebo adresar
    int8_t references;              //počet odkazů na i-uzel, používá se pro hardlinky
    int32_t file_size;              //velikost souboru v bytech
    int32_t direct1;                // 1. přímý odkaz na datové bloky
    int32_t direct2;                // 2. přímý odkaz na datové bloky
    int32_t direct3;                // 3. přímý odkaz na datové bloky
    int32_t direct4;                // 4. přímý odkaz na datové bloky
    int32_t direct5;                // 5. přímý odkaz na datové bloky
    int32_t indirect1;              // 1. nepřímý odkaz (odkaz - datové bloky)
    int32_t indirect2;              // 2. nepřímý odkaz (odkaz - odkaz - datové bloky)
};

struct directory_item {
    int32_t inode;                   // inode odpovídající souboru
    char item_name[12];              //8+3 + /0 C/C++ ukoncovaci string znak
};