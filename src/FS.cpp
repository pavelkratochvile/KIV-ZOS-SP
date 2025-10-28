#include "FS.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <array>

FS::FS(std::string path){
    this->path = path;
    this->file = nullptr;
}

FS::FS() {
    this->file = nullptr;
}

FS::~FS(){
    if(this->file){
        fclose(this->file);
    }
}

int FS::format(int size){
    FILE *f = fopen(path.c_str(), "wb+");

    if(!f){
        return 1;
    }

    superblock s;
    if(init_superblock(&s, size) != 0){
        fclose(f);
        return 1;
    }

    /*Vytvoreni celeho souboru o velikosti disc_size*/
    std::fseek(f, s.disk_size - 1, SEEK_SET);
    std::putc('\0', f);

    /*Pridani superbloku na jeho zacatek*/
    std::fseek(f, 0, SEEK_SET); 
    std::fwrite(&s, sizeof(superblock), 1, f);

    /*Zápis bitmap i-nodů (16 bajtů nul)*/
    int charcount_inode = (INODE_COUNT + 7) / 8;
    char* buffer_inode = new char[charcount_inode]();
    //buffer_inode[0] = 'A';  // Nastaví první byte na 'A' (0x41)
    fwrite(buffer_inode, sizeof(char), charcount_inode, f);
    delete[] buffer_inode;

    /*Zapis bitmapy clusterů*/
    int charcount_cluster = (CLUSTER_COUNT + 7) / 8;
    char* buffer_cluster = new char[charcount_cluster]();
    fwrite(buffer_cluster, sizeof(char), charcount_cluster, f);
    delete[] buffer_cluster;

    // Ulož lokální superblock do členského atributu
    this->sb = s;
    fclose(f);
    std::cout << "zformatovano" << std::endl;
    return 0;
}

int FS::attach(){
    this->file = fopen(path.c_str(), "rb+");
    if (!this->file) {
        std::cerr << "Soubor se nepodařilo otevřít" << std::endl;
        return 2;
    }

    if (fread(&this->sb, sizeof(superblock), 1, this->file) != 1) {
        std::cerr << "Superblok se nepodařilo načíst" << std::endl;
        fclose(this->file);
        this->file = nullptr;
        return 2;
    }

    std::cout << "Jmeno makace na FS: " << this->sb.signature << std::endl;
    return 0;
}

int32_t FS::findPositionClusterDIR(int32_t direct){
    int maxItems = CLUSTER_SIZE / sizeof(directory_item);

    for(int i = 0; i < maxItems; i++){
        directory_item* item = reinterpret_cast<directory_item*>(direct + i * sizeof(directory_item));
        if(item->inode == 0){
            std::cout << "Volná pozice na " << i << "-tém dir Itemu" << std::endl;
            return direct + i * sizeof(directory_item);
        }
    }
    return 0;
}

int32_t FS::findDirectsClusterDIR(int32_t cluster){
    int maxDirectsCluster = (CLUSTER_SIZE / sizeof(int32_t));
    for(int i = 0; i < maxDirectsCluster; i++){
        int32_t direct = *reinterpret_cast<int32_t*>(cluster + i * sizeof(int32_t));
        int32_t address = this->findPositionClusterDIR(direct);
        if(address){
            return address;
        }
    }
    return 0;
}

int32_t FS::findPositionInodeDIR(int32_t inodeAddr){
    pseudo_inode* inode = reinterpret_cast<pseudo_inode*>(inodeAddr);
    int32_t address = 0;
    int32_t directTable[DIRECT_COUNT] = {
        inode->direct1,
        inode->direct2,
        inode->direct3,
        inode->direct4,
        inode->direct5
    };

    int32_t indirectTable[INDIRECT_COUNT] = {
        inode->indirect1,
        inode->indirect2
    };
    
    for(int i = 0; i < DIRECT_COUNT; i++){
        address = this->findPositionClusterDIR(directTable[i]);
        if(address){
            return address;
        }
    }
    
    for(int i = 0; i <  INDIRECT_COUNT; i++){
        address = this->findDirectsClusterDIR(indirectTable[i]);
        if(address){
            return address;
        }
    }

    std::cout << "V adresáři už není místo na nové soubory" << std::endl;
    return 0;
}

int FS::makeRoot(){
    pseudo_inode root;
    int32_t cluster = findFreeCluster();

    root.nodeid = 0;
    root.isDirectory = true;
    root.references = 0;
    root.file_size = 0;
    root.direct1 = cluster;
    root.direct2 = 0;
    root.direct3 = 0;
    root.direct4 = 0;
    root.direct5 = 0;
    root.indirect1 = 0;
    root.indirect2 = 0;

    fseek(this->file, this->sb.inode_start_adress, SEEK_SET);
    size_t written = fwrite(&root, sizeof(pseudo_inode), 1, this->file);
    if (written != 1) {
        std::cerr << "Chyba při zápisu root inode do souboru!" << std::endl;
        return 2;
    }

    fflush(this->file); // zajistí zapsání do souboru
    std::cout << "Root inode úspěšně vytvořen na offsetu " << this->sb.inode_start_adress << std::endl;
    
    int clusterId = (cluster - this->sb.data_start_adress) / this->sb.cluster_size;
    this->setInodeBit(0,1);
    this->setClusterBit(clusterId, 1);
    this->writeDIRItem(cluster, ".", 0);

    this->CurrentDirInfo.currentcluster = cluster;
    this->CurrentDirInfo.currentDirId = 0;
    this->CurrentDirInfo.name = "root";

    return 0;
}

int FS::makeDir(Path path){
    int parrentID = getInodeFromPath(path, false);
    
    if(parrentID == -1) {
        return 1;
    }
    else if(inodeExists(parrentID, path.path.back())){
        std::cout << "adresar jiz existuje" << std::endl;
        return 2;
    }

    pseudo_inode dir;
    
    int32_t cluster = findFreeCluster();
    int32_t inode = findFreeInode();
    int clusterID = (cluster - this->sb.data_start_adress) / this->sb.cluster_size;
    int inodeID = (inode - this->sb.inode_start_adress) / sizeof(pseudo_inode);

    dir.nodeid = inodeID;
    dir.isDirectory = true;
    dir.references = 0;
    dir.file_size = 0;
    dir.direct1 = cluster;
    dir.direct2 = 0;
    dir.direct3 = 0;
    dir.direct4 = 0;
    dir.direct5 = 0;
    dir.indirect1 = 0;
    dir.indirect2 = 0;

    this->setClusterBit(clusterID, 1);
    this->setInodeBit(inodeID,1);

    fseek(this->file, inode, SEEK_SET);
    fwrite(&dir, sizeof(pseudo_inode), 1, this->file);
    fflush(this->file);

    int32_t parrentCluster = getClusterbyID(parrentID);

    this->writeDIRItem(cluster, ".", inodeID);
    this->writeDIRItem(cluster, "..", parrentID);
    this->writeDIRItem(parrentCluster, path.path.back(), inodeID);

    return 0;
}

void FS::writeDIRItem(int32_t clusterAddr, std::string dirName, int32_t inodeID){
    if (!this->file) {
        std::cerr << "[FS::writeDIRItem] Soubor FS není otevřen!" << std::endl;
        return;
    }

    // Kolik directory_item se vejde do clusteru
    int maxItems = this->sb.cluster_size / sizeof(directory_item);

    directory_item dir;

    // projdi všechny položky v clusteru
    for (int i = 0; i < maxItems; ++i) {
        fseek(this->file, clusterAddr + i * sizeof(directory_item), SEEK_SET);
        fread(&dir, sizeof(directory_item), 1, this->file);
        std::cout << "[FS::writeDIRItem] Kontroluji pozici " << i 
                  << ": inode=" << dir.inode 
                  << ", name='" << dir.item_name << "'" << std::endl;

        if (dir.inode == 0 && dir.item_name[0] == '\0') {  // volné místo
            // připrav nový záznam
            dir.inode = inodeID;
            strncpy(dir.item_name, dirName.c_str(), sizeof(dir.item_name) - 1);
            dir.item_name[sizeof(dir.item_name) - 1] = '\0'; // jistota ukončení

            // zapíš do souboru
            fseek(this->file, clusterAddr + i * sizeof(directory_item), SEEK_SET);
            fwrite(&dir, sizeof(directory_item), 1, this->file);
            fflush(this->file);

            std::cout << "[FS::writeDIRItem] Zapsán DIR item '" << dirName 
                      << "' na inode " << inodeID 
                      << " do clusteru na indexu " << i << std::endl;
            std::cout << "------------------------" << std::endl;
            return;
        }
    }

    std::cerr << "[FS::writeDIRItem] Cluster je plný, nelze přidat '" << dirName << "'!" << std::endl;
}

void FS::setInodeBit(int inodeID, int bit){
    if (bit != 0 && bit != 1) {
        std::cerr << "[FS::setInodeBit] Bit musí být 0 nebo 1!" << std::endl;
        return;
    }

    if (inodeID < 0) {
        std::cerr << "[FS::setInodeBit] Neplatné inode ID!" << std::endl;
        return;
    }
    int64_t bitmapStart = this->sb.bitmapi_start_adress;
    int byteIndex = inodeID / 8;
    int bitOffset = inodeID % 8;

    unsigned char byte = 0;

    // Přečti aktuální bajt
    fseek(this->file, bitmapStart + byteIndex, SEEK_SET);
    size_t readCount = fread(&byte, sizeof(byte), 1, this->file);
    if (readCount != 1) {
        std::cerr << "[FS::setInodeBit] Nepodařilo se načíst bajt z bitmapy!" << std::endl;
        return;
    }

    // Nastav nebo vynuluj konkrétní bit
    if (bit == 1)
        byte |= (1 << bitOffset);     // nastaví bit na 1
    else
        byte &= ~(1 << bitOffset);    // nastaví bit na 0

    // Zapiš zpět na stejné místo
    fseek(this->file, bitmapStart + byteIndex, SEEK_SET);
    fwrite(&byte, sizeof(byte), 1, this->file);
    fflush(this->file);

    std::cout << "[FS::setInodeBit] Inode " << inodeID << " nastaven na " << bit << std::endl;
}

void FS::setClusterBit(int clusterID, int bit){
    if (bit != 0 && bit != 1) {
        std::cerr << "[FS::setInodeBit] Bit musí být 0 nebo 1!" << std::endl;
        return;
    }

    if (clusterID < 0) {
        std::cerr << "[FS::setClusterBit] Neplatné cluster ID!" << std::endl;
        return;
    }
    int64_t bitmapStart = this->sb.bitmap_start_adress;
    int byteIndex = clusterID / 8;
    int bitOffset = clusterID % 8;

    unsigned char byte = 0;

    // Přečti aktuální bajt
    fseek(this->file, bitmapStart + byteIndex, SEEK_SET);
    size_t readCount = fread(&byte, sizeof(byte), 1, this->file);
    if (readCount != 1) {
        std::cerr << "[FS::setClusterBit] Nepodařilo se načíst bajt z bitmapy!" << std::endl;
        return;
    }

    // Nastav nebo vynuluj konkrétní bit
    if (bit == 1)
        byte |= (1 << bitOffset);     // nastaví bit na 1
    else
        byte &= ~(1 << bitOffset);    // nastaví bit na 0

    // Zapiš zpět na stejné místo
    fseek(this->file, bitmapStart + byteIndex, SEEK_SET);
    fwrite(&byte, sizeof(byte), 1, this->file);
    fflush(this->file);

    std::cout << "[FS::setClusterBit] Cluster " << clusterID << " nastaven na " << bit << std::endl;
}

int32_t FS::findFreeCluster(){
    if (!this->file || !this->sb.cluster_count) {
        std::cerr << "[FS::findFreeCluster] Soubor nebo superblock nejsou inicializované!" << std::endl;
        return -1;
    }

    int64_t bitmapStart = this->sb.bitmap_start_adress;
    int32_t totalClusters = this->sb.cluster_count;
    int32_t totalBytes = (totalClusters + 7) / 8; // počet bajtů v bitmapě

    for (int32_t byteIndex = 0; byteIndex < totalBytes; ++byteIndex) {
        unsigned char byte = 0;

        // načti celý bajt
        fseek(this->file, bitmapStart + byteIndex, SEEK_SET);
        size_t readCount = fread(&byte, sizeof(byte), 1, this->file);
        if (readCount != 1) {
            std::cerr << "[FS::findFreeCluster] Nepodařilo se načíst bajt z bitmapy!" << std::endl;
            return -1;
        }

        // projdi jednotlivé bity v bajtu
        for (int bitOffset = 0; bitOffset < 8; ++bitOffset) {
            int clusterID = byteIndex * 8 + bitOffset;
            if (clusterID >= totalClusters) break; // přeskok, pokud je poslední bajt částečný

            if ((byte & (1 << bitOffset)) == 0) {
                // první volný bit = volný cluster
                return clusterID * this->sb.cluster_size + this->sb.data_start_adress;
            }
        }
    }

    // žádný volný cluster nebyl nalezen
    return -1;
}

int32_t FS::findFreeInode(){
    if (!this->file) {
        std::cerr << "[FS::findFreeInode] Soubor nebo superblock nejsou inicializované!" << std::endl;
        return -1;
    }

    int64_t bitmapStart = this->sb.bitmapi_start_adress;
    int32_t totalInodes = INODE_COUNT;
    int32_t totalBytes = (totalInodes + 7) / 8; // počet bajtů v bitmapě

    for (int32_t byteIndex = 0; byteIndex < totalBytes; ++byteIndex) {
        unsigned char byte = 0;

        // načti celý bajt
        fseek(this->file, bitmapStart + byteIndex, SEEK_SET);
        size_t readCount = fread(&byte, sizeof(byte), 1, this->file);
        if (readCount != 1) {
            std::cerr << "[FS::findFreeInode] Nepodařilo se načíst bajt z bitmapy!" << std::endl;
            return -1;
        }

        // projdi jednotlivé bity v bajtu
        for (int bitOffset = 0; bitOffset < 8; ++bitOffset) {
            int inodeID = byteIndex * 8 + bitOffset;
            if (inodeID >= totalInodes) break; // přeskok, pokud je poslední bajt částečný

            if ((byte & (1 << bitOffset)) == 0) {
                // první volný bit = volný cluster
                return inodeID * sizeof(pseudo_inode) + this->sb.inode_start_adress;
            }
        }
    }
    return -1;
}

int FS::getInodeFromPath(Path path, bool editLast){
    int tempID = 0;
    
    if(path.isAbsolut == true){
        tempID = 0;
    }
    else{
        tempID = CurrentDirInfo.currentDirId;
    }
    int32_t tempCluster = -1;
    int lastItem = 0;
    
    if(editLast == true){
        lastItem = path.path.size();
    }
    else{
        lastItem = path.path.size() - 1;
    }

    for(int i = 0; i < lastItem; i++){
        tempCluster = getClusterbyID(tempID);
        tempID = findInodeIdByName(tempCluster, path.path[i]);
        if(tempID == -1){
            return -1;
        }
        else if(i == lastItem -1 && tempID != -1){
            return tempID;
        }
    }
    return tempID;
}

int FS::findInodeIdByName(int32_t cluster, std::string name){
    directory_item dirItem;

    for(int i = 0; i < static_cast<int>(this->sb.cluster_size / sizeof(directory_item)); i++){
        fseek(this->file, cluster + i * sizeof(directory_item), SEEK_SET);
        fread(&dirItem, sizeof(directory_item), 1, this->file);
        if(dirItem.item_name == name){
            return dirItem.inode;
        }
    }
    
    return -1;
}

int32_t FS::getClusterbyID(int id) const{
    pseudo_inode inode;
    fseek(this->file, this->sb.inode_start_adress + id * sizeof(pseudo_inode), SEEK_SET);
    fread(&inode, sizeof(pseudo_inode), 1, this->file);
    return inode.direct1;
}

bool FS::inodeExists(int inodeID, const std::string& name) const{
    directory_item dir;
    int32_t cluster = getClusterbyID(inodeID);
    int maxItems = static_cast<int>(this->sb.cluster_size / sizeof(directory_item));
    for(int i = 0; i < maxItems; i++){
        fseek(this->file, cluster + i * sizeof(directory_item), SEEK_SET);
        fread(&dir, sizeof(directory_item), 1, this->file);
        if(name == std::string(dir.item_name)){
            return true;
        }
    }
    return false;
}

std::string FS::findNameByInode(int32_t cluster, int32_t inodeID) const {
    if (!this->file) return std::string();
    directory_item dir;
    int maxItems = static_cast<int>(this->sb.cluster_size / sizeof(directory_item));

    for (int i = 0; i < maxItems; ++i) {
        long pos = static_cast<long>(cluster) + static_cast<long>(i) * static_cast<long>(sizeof(directory_item));
        if (fseek(this->file, pos, SEEK_SET) != 0) continue;
        if (fread(&dir, sizeof(directory_item), 1, this->file) != 1) continue;
        dir.item_name[sizeof(dir.item_name) - 1] = '\0';
        if (dir.inode == inodeID) {
            return std::string(dir.item_name);
        }
    }
    return std::string();
}

int FS::changeDir(Path path){
    int inodeID = getInodeFromPath(path, true);
    if(inodeID == -1){
        return 1;
    }
    else if(isFile(inodeID)){
        return 2;
    }

    handlePathChange(path);
    CurrentDirInfo.currentDirId = inodeID;
    CurrentDirInfo.currentcluster = getClusterbyID(inodeID);
    CurrentDirInfo.name = path.path.back();
    return 0;
}

int FS::rmDir(Path path){
    int inodeID = getInodeFromPath(path, true);
    if(inodeID == -1){
        return 1; // wrong path
    }
    else if(inodeID == 0){
        return 2; // cant rm root
    }
    else if(isFile(inodeID)){
        return 3; // not directory
    }
    else if(inodeID == this->CurrentDirInfo.currentDirId){
        return 5; // current dir
    }
    std::vector<std::string> names = getAllFDNames(inodeID);
    if(names.size() > 2){
        return 4; //not empty
    }
    nullCluster(getClusterbyID(inodeID));
    int clusterID = (getClusterbyID(inodeID) - this->sb.data_start_adress) / this->sb.cluster_size;
    this->setClusterBit(clusterID, 0);
    this->setInodeBit(inodeID, 0);

    int parentInodeID = getInodeFromPath(path, false);
    int32_t parentCluster = getClusterbyID(parentInodeID);
    directory_item dirItem;
    int maxItems = static_cast<int>(this->sb.cluster_size / sizeof(directory_item));
    
    for(int i = 0; i < maxItems; i++){
        fseek(this->file, parentCluster + i * sizeof(directory_item), SEEK_SET);
        fread(&dirItem, sizeof(directory_item), 1, this->file);
        if(dirItem.item_name == path.path.back()){
            dirItem.inode = 0;
            dirItem.item_name[0] = '\0';
            fseek(this->file, parentCluster + i * sizeof(directory_item), SEEK_SET);
            fwrite(&dirItem, sizeof(directory_item), 1, this->file);
            fflush(this->file);
            break;
        }
    }
    return 0;
}
void FS::nullCluster(int32_t cluster){
    for(int i = 0; i < CLUSTER_SIZE; i++){
        char zero = 0;
        fseek(this->file, cluster + i, SEEK_SET);
        fwrite(&zero, sizeof(char), 1, this->file);
    }
}

void FS::handlePathChange(Path& path){
    if (path.isAbsolut) {
        currentPath.clear();
        for (const auto& part : path.path) {
            if (part == "." ) {
                continue;
            }
            else if (part == "..") {
                if (!currentPath.empty()) currentPath.pop_back();
            }
            else {
                currentPath.push_back(part);
            }
        }
    }
    else if(path.isAbsolut == false){
        for(size_t i = 0; i < path.path.size(); i++){
            if(path.path[i] == ".."){
                currentPath.pop_back();
            }
            else if(path.path[i] == "."){
                continue;
            }
            else{
                currentPath.push_back(path.path[i]);
            }
        }
    }
    return;
}

int FS::listDir(Path path){
    int inodeID;
    if(path.path.empty()){
        inodeID = this->CurrentDirInfo.currentDirId;
    }
    else{
        inodeID = getInodeFromPath(path, true);
    }
    if(inodeID == -1){
        return 1;
    }
    std::vector<std::string> names = getAllFDNames(inodeID);
    for(size_t i = 0; i < names.size(); i++){
        std::cout << names[i] << "  ";
    }
    std::cout << std::endl;
    return 0;
}
std::vector<std::string> FS::getAllFDNames(int inodeID){
    std::vector<std::string> names;
    int32_t cluster = getClusterbyID(inodeID);
    directory_item item;

    int maxItems = static_cast<int>(this->sb.cluster_size / sizeof(directory_item));
    for(int i = 0; i < maxItems; i++){
        fseek(this->file, cluster + i * sizeof(directory_item), SEEK_SET);
        fread(&item, sizeof(directory_item), 1, this->file);
        if (item.item_name[0] != '\0') {
            names.push_back(std::string(item.item_name));
        }
    }
    return names;
}

int FS::inCopy(Path path, std::string sourcePath) {
    int inodeID;
    std::string content;
    
    if (path.path.empty()) {
        inodeID = this->CurrentDirInfo.currentDirId;
    } else {
        inodeID = getInodeFromPath(path, true);
    }

    if (inodeID == -1) {
        std::cerr << "Cesta neexistuje v souborovém systému." << std::endl;
        return 1;
    } else if (!isFile(inodeID)) {
        std::cerr << "Cesta nevede k souboru." << std::endl;
        return 3;
    }

    // 🔧 Otevři soubor v binárním režimu a načti celý obsah
    std::ifstream inFile(sourcePath, std::ios::binary);
    if (!inFile) {
        std::cerr << "Soubor se nepodařilo otevřít: " << sourcePath << std::endl;
        return 2;
    }

    std::ostringstream ss;
    ss << inFile.rdbuf();   // načte celý obsah najednou
    content = ss.str();

    inFile.close();

    int status = writeContentToFile(inodeID, content);
    return status;
}

int FS::writeContentToFile(int inodeID, std::string content) {
    if (inodeID < 0) {
        std::cerr << "Chyba: Neplatný inodeID." << std::endl;
        return 1;
    }
    // povolit i prázdný obsah pokud chceme truncovat:
    // if (content.empty()) { ... }

    // 1) Načti inode z disku
    pseudo_inode inode;
    fseek(this->file, this->sb.inode_start_adress + inodeID * sizeof(pseudo_inode), SEEK_SET);
    if (fread(&inode, sizeof(pseudo_inode), 1, this->file) != 1) {
        std::cerr << "Nelze načíst inode." << std::endl;
        return 1;
    }

    // 2) Rozbij obsah na bloky
    std::vector<std::string> blocks = splitToBlocks(content, this->sb.cluster_size);
    size_t toWrite = blocks.size();
    size_t written = 0;

    // 3) Přímo pracuj s políčky inode (ujisti se, že pseudo_inode má direct1..5 a indirect1..2)
    int32_t *directsArr[DIRECT_COUNT] = {
        &inode.direct1, &inode.direct2, &inode.direct3, &inode.direct4, &inode.direct5
    };
    int32_t *indirectsArr[INDIRECT_COUNT] = {
        &inode.indirect1, &inode.indirect2
    };

    // 4) Direct pointers
    for (size_t i = 0; i < DIRECT_COUNT && written < toWrite; ++i) {
        if (*directsArr[i] == 0) {
            int32_t freeCluster = findFreeCluster();
            if (freeCluster == -1) {
                std::cerr << "Není dostatek místa na disku!" << std::endl;
                return 2;
            }
            *directsArr[i] = freeCluster;
            int clusterID = (freeCluster - this->sb.data_start_adress) / this->sb.cluster_size;
            setClusterBit(clusterID, 1);
        }
        // zápis dat do clusteru (předpoklad: *directsArr[i] je byte-offset)
        fseek(this->file, *directsArr[i], SEEK_SET);
        fwrite(blocks[written].data(), 1, blocks[written].size(), this->file);
        fflush(this->file);
        ++written;
    }

    // 5) Indirect pointers
    for (size_t i = 0; i < INDIRECT_COUNT && written < toWrite; ++i) {
        // pokud indirect ukazatel nulový -> alokuj cluster pro tabulku ukazatelů a vynuluj ji
        if (*indirectsArr[i] == 0) {
            int32_t freeCluster = findFreeCluster();
            if (freeCluster == -1) {
                std::cerr << "Není dostatek místa na disku!" << std::endl;
                return 2;
            }
            *indirectsArr[i] = freeCluster;
            int clusterID = (freeCluster - this->sb.data_start_adress) / this->sb.cluster_size;
            setClusterBit(clusterID, 1);

            // vynulovat obsah tohoto clusteru (pole int32_t)
            size_t entries = this->sb.cluster_size / sizeof(int32_t);
            std::vector<int32_t> zeros(entries, 0);
            fseek(this->file, *indirectsArr[i], SEEK_SET);
            fwrite(zeros.data(), sizeof(int32_t), entries, this->file);
            fflush(this->file);
        }

        // načíst indirect block (pole adres)
        size_t entries = this->sb.cluster_size / sizeof(int32_t);
        std::vector<int32_t> indirectBlock(entries, 0);
        fseek(this->file, *indirectsArr[i], SEEK_SET);
        fread(indirectBlock.data(), sizeof(int32_t), entries, this->file);

        for (size_t j = 0; j < entries && written < toWrite; ++j) {
            if (indirectBlock[j] == 0) {
                int32_t freeCluster = findFreeCluster();
                if (freeCluster == -1) {
                    std::cerr << "Není dostatek místa na disku!" << std::endl;
                    return 2;
                }
                indirectBlock[j] = freeCluster;
                int clusterID = (freeCluster - this->sb.data_start_adress) / this->sb.cluster_size;
                setClusterBit(clusterID, 1);

                // uložit aktualizovaný indirectBlock zpět (můžete takto ukládat průběžně)
                fseek(this->file, *indirectsArr[i], SEEK_SET);
                fwrite(indirectBlock.data(), sizeof(int32_t), entries, this->file);
                fflush(this->file);
            }

            // zapsat data do přiděleného clusteru
            fseek(this->file, indirectBlock[j], SEEK_SET);
            fwrite(blocks[written].data(), 1, blocks[written].size(), this->file);
            fflush(this->file);
            ++written;
        }
    }

    // 6) Po skončení aktualizuj inode (file_size atd.) a ulož jej jednou
    inode.file_size = content.size();
    fseek(this->file, this->sb.inode_start_adress + inodeID * sizeof(pseudo_inode), SEEK_SET);
    fwrite(&inode, sizeof(pseudo_inode), 1, this->file);
    fflush(this->file);

    std::cout << "Zapsáno bloků: " << written << std::endl;
    return 0;
}

std::vector<std::string> FS::splitToBlocks(const std::string& content, size_t blockSize) {
    std::vector<std::string> blocks;

    for (size_t i = 0; i < content.size(); i += blockSize) {
        blocks.push_back(content.substr(i, blockSize));
    }

    return blocks;
}

std::string FS::readContentFromFile(int inodeID){
    if(inodeID < 0 || !isFile(inodeID)){
        return "";
    }

    pseudo_inode inode;
    fseek(this->file, this->sb.inode_start_adress + inodeID * sizeof(pseudo_inode), SEEK_SET);
    fread(&inode, sizeof(pseudo_inode), 1, this->file);

    std::string content;
    size_t remaining = inode.file_size;
    size_t blockSize = this->sb.cluster_size;
    std::vector<char> buffer(blockSize);

    int32_t directs[] = {
        inode.direct1, inode.direct2, inode.direct3,
        inode.direct4, inode.direct5
    };

    for (int i = 0; i < 5 && remaining > 0; i++) {
        if (directs[i] == 0) continue;

        fseek(this->file, directs[i], SEEK_SET);
        size_t toRead = std::min(blockSize, remaining);
        fread(buffer.data(), 1, toRead, this->file);

        content.append(buffer.data(), toRead);
        remaining -= toRead;
    }

    // --- INDIRECT BLOCKS ---
    int32_t indirects[] = { inode.indirect1, inode.indirect2 };

    for (int i = 0; i < 2 && remaining > 0; i++) {
        if (indirects[i] == 0) continue;

        size_t entries = blockSize / sizeof(int32_t);
        std::vector<int32_t> indirectBlock(entries, 0);

        fseek(this->file, indirects[i], SEEK_SET);
        fread(indirectBlock.data(), sizeof(int32_t), entries, this->file);

        for (size_t j = 0; j < entries && remaining > 0; j++) {
            if (indirectBlock[j] == 0) continue;

            fseek(this->file, indirectBlock[j], SEEK_SET);
            size_t toRead = std::min(blockSize, remaining);
            fread(buffer.data(), 1, toRead, this->file);

            content.append(buffer.data(), toRead);
            remaining -= toRead;
        }
    }

    return content;
}

int FS::outCopy(Path path, std::string destPath){
    int inodeID;
    inodeID = getInodeFromPath(path, true);
    if(inodeID == -1){
        return 2;
    }
    std::string content = readContentFromFile(inodeID);
    std::ofstream outFile(destPath, std::ios_base::binary);
    if(!outFile){
        return 1;
    }
    outFile << content;
    return 0;
}

int FS::catFile(Path path){
    int inodeID;
    inodeID = getInodeFromPath(path, true);
    if(inodeID == -1){
        return 1;
    }
    std::string content = readContentFromFile(inodeID);
    std::cout << content << std::endl;
    return 0;
}

bool FS::isFile(int inodeID) const{
    pseudo_inode inode;
    fseek(this->file, this->sb.inode_start_adress + inodeID * sizeof(pseudo_inode), SEEK_SET);
    fread(&inode, sizeof(pseudo_inode), 1, this->file);
    return !inode.isDirectory;
}

int FS::makeFile(Path path){
    int parrentInodeID = getInodeFromPath(path, false);
    if(parrentInodeID == -1){
        return 1;
    }
    else if(inodeExists(parrentInodeID, path.path.back())){
        return 2;
    }

    int32_t inodePos = findFreeInode();
    int inodeID = (inodePos - this->sb.inode_start_adress) / sizeof(pseudo_inode);

    int32_t parrentCluster = getClusterbyID(parrentInodeID);
    writeDIRItem(parrentCluster, path.path.back(), inodeID);

    pseudo_inode file;

    file.nodeid = inodeID;
    file.isDirectory = false;
    file.references = 0;
    file.file_size = 0;
    file.direct1 = 0;
    file.direct2 = 0;
    file.direct3 = 0;
    file.direct4 = 0;
    file.direct5 = 0;
    file.indirect1 = 0;
    file.indirect2 = 0;

    this->setInodeBit(inodeID, 1);

    fseek(this->file, inodePos, SEEK_SET);
    fwrite(&file, sizeof(pseudo_inode), 1, this->file);
    fflush(this->file);
    return 0;
}
