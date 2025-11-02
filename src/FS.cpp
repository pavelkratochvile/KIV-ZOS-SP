#include "FS.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <array>
#include <vector>
#include <cstdint>

// Konstruktor, destruktor a základní metody FS
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

// Formátování souborového systému
int FS::format(int size){
    FILE *f = fopen(path.c_str(), "wb+");

    if(!f){
        // Chyba při otevírání souboru pro formátování
        return 1;
    }

    superblock s;
    if(init_superblock(&s, size) != 0){
        // Chyba při inicializaci superbloku
        fclose(f);
        return 1;
    }

    // Vytvoreni celeho souboru o velikosti disk_size
    std::fseek(f, s.disk_size - 1, SEEK_SET);
    std::putc('\0', f);

    // Pridani superbloku na jeho zacatek
    std::fseek(f, 0, SEEK_SET);
    std::fwrite(&s, sizeof(superblock), 1, f);

    // Zápis bitmap i-nodů (16 bajtů nul)
    int charcount_inode = (INODE_COUNT + 7) / 8;
    char* buffer_inode = new char[charcount_inode]();
    fwrite(buffer_inode, sizeof(char), charcount_inode, f);
    delete[] buffer_inode;

    // Zápis bitmapy clusterů
    int charcount_cluster = (CLUSTER_COUNT + 7) / 8;
    char* buffer_cluster = new char[charcount_cluster]();
    fwrite(buffer_cluster, sizeof(char), charcount_cluster, f);
    delete[] buffer_cluster;

    // Ulož lokální superblock do členského atributu
    this->sb = s;
    fclose(f);

    // Po formátování otevři soubor a vytvoř root adresář, aby byl FS ihned použitelný
    int rc = this->attach();
    if (rc != 0) {
        return rc;
    }

    // Vytvoření kořenového adresáře
    int rcRoot = this->makeRoot();
    if (rcRoot != 0) {
        return rcRoot;
    }

    return 0;
}

int FS::attach(){
    this->file = fopen(path.c_str(), "rb+");
    if (!this->file) {
        // Souborový systém nelze otevřít
        return 2;
    }

    if (fread(&this->sb, sizeof(superblock), 1, this->file) != 1) {
        // Chyba při čtení superbloku
        std::cerr << "Superblock could not be read" << std::endl;
        fclose(this->file);
        this->file = nullptr;
        return 2;
    }
    // Nastavení aktuálního adresáře na root
    this->CurrentDirInfo.currentDirId = 0;
    this->CurrentDirInfo.name = "root";
    return 0;
}

int FS::makeRoot(){
    // Struktura i-nodu pro root adresář
    pseudo_inode root;
    int32_t cluster = findFreeCluster();

    // Inicializace i-nodu pro root adresář a jeho počátečních hodnot
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

    // Zápis i-nodu do souboru
    fseek(this->file, this->sb.inode_start_adress, SEEK_SET);
    size_t written = fwrite(&root, sizeof(pseudo_inode), 1, this->file);
    if (written != 1) {
        std::cerr << "Error writing root inode to file!" << std::endl;
        return 2;
    }

    // zajistí zapsání do souboru
    fflush(this->file);

    // Další operace: Nastavení bitů v bitmapách a vytvoření položky "."
    int clusterId = (cluster - this->sb.data_start_adress) / this->sb.cluster_size;
    this->setInodeBit(0,1);
    this->setClusterBit(clusterId, 1);
    this->writeDIRItem(cluster, ".", 0);
    this->CurrentDirInfo.currentcluster = cluster;

    return 0;
}

int FS::makeDir(Path path){
    // Získání i-nodu rodičovského adresáře
    int parrentID = getInodeFromPath(path, false);
    
    if(parrentID == -1) {
        // Rodičovský adresář neexistuje
        return 1;
    }
    else if(inodeExists(parrentID, path.path.back())){
        // Adresář již existuje
        return 2;
    }

    // Vytvoření nového adresáře
    pseudo_inode dir;
    int32_t cluster = findFreeCluster();
    int32_t inode = findFreeInode();
    int clusterID = (cluster - this->sb.data_start_adress) / this->sb.cluster_size;
    int inodeID = (inode - this->sb.inode_start_adress) / sizeof(pseudo_inode);

    // Inicializace i-nodu pro nový adresář
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

    // Nastavení bitů v bitmapách
    this->setClusterBit(clusterID, 1);
    this->setInodeBit(inodeID,1);

    // Zápis i-nodu do souboru
    fseek(this->file, inode, SEEK_SET);
    fwrite(&dir, sizeof(pseudo_inode), 1, this->file);
    fflush(this->file);

    int32_t parrentCluster = getClusterbyID(parrentID);

    // Zápis položek "." a ".." do nového adresáře a přidání položky do rodičovského adresáře
    this->writeDIRItem(cluster, ".", inodeID);
    this->writeDIRItem(cluster, "..", parrentID);
    this->writeDIRItem(parrentCluster, path.path.back(), inodeID);

    return 0;
}

void FS::writeDIRItem(int32_t clusterAddr, std::string dirName, int32_t inodeID){
    if (!this->file) {
        return;
    }

    // Kolik directory_item se vejde do clusteru
    int maxItems = this->sb.cluster_size / sizeof(directory_item);

    directory_item dir;

    // projdi všechny položky v clusteru
    for (int i = 0; i < maxItems; ++i) {
        fseek(this->file, clusterAddr + i * sizeof(directory_item), SEEK_SET);
        fread(&dir, sizeof(directory_item), 1, this->file);

        if (dir.inode == 0 && dir.item_name[0] == '\0') {  // volné místo
            // připrav nový záznam
            dir.inode = inodeID;
            strncpy(dir.item_name, dirName.c_str(), sizeof(dir.item_name) - 1);
            dir.item_name[sizeof(dir.item_name) - 1] = '\0'; // jistota ukončení

            // zapíš do souboru
            fseek(this->file, clusterAddr + i * sizeof(directory_item), SEEK_SET);
            fwrite(&dir, sizeof(directory_item), 1, this->file);
            fflush(this->file);

            return;
        }
    }
}

void FS::setInodeBit(int inodeID, int bit){
    if (bit != 0 && bit != 1) {
        // Chyba: bit musí být 0 nebo 1
        return;
    }

    if (inodeID < 0) {
        // Chyba: neplatné ID i-nodu
        std::cerr << "[FS::setInodeBit] Invalid inode ID!" << std::endl;
        return;
    }
    // Získání adresy bitmapy i-nodů
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
}

void FS::setClusterBit(int clusterID, int bit){
    if (bit != 0 && bit != 1) {
        // Chyba: bit musí být 0 nebo 1
        std::cerr << "[FS::setClusterBit] Bit must be 0 or 1!" << std::endl;
        return;
    }

    if (clusterID < 0) {
        // Chyba: neplatné ID clusteru
        std::cerr << "[FS::setClusterBit] Invalid cluster ID!" << std::endl;
        return;
    }
    // Získání adresy bitmapy clusterů
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
}

int32_t FS::findFreeCluster(){
    if (!this->file || !this->sb.cluster_count) {
        // Chyba: soubor nebo superblock nejsou inicializované
        std::cerr << "[FS::findFreeCluster] Soubor nebo superblock nejsou inicializované!" << std::endl;
        return -1;
    }

    // Získání adresy bitmapy clusterů
    int64_t bitmapStart = this->sb.bitmap_start_adress;
    int32_t totalClusters = this->sb.cluster_count;
    int32_t totalBytes = (totalClusters + 7) / 8; 
    // počet bajtů v bitmapě

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
        // Chyba: soubor nebo superblock nejsou inicializované
        std::cerr << "[FS::findFreeInode] Soubor nebo superblock nejsou inicializované!" << std::endl;
        return -1;
    }

    int64_t bitmapStart = this->sb.bitmapi_start_adress;
    int32_t totalInodes = INODE_COUNT;
    int32_t totalBytes = (totalInodes + 7) / 8; 
    // počet bajtů v bitmapě

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
        // začni od rootu 
        tempID = 0;
    }
    else{
        // začni od aktuálního adresáře
        tempID = CurrentDirInfo.currentDirId;
    }
    int32_t tempCluster = -1;
    int lastItem = 0;
    
    if(editLast == true){
        // projdi celou cestu včetně posledního prvku
        lastItem = path.path.size();
    }
    else{
        // projdi celou cestu kromě posledního prvku
        lastItem = path.path.size() - 1;
    }

    for(int i = 0; i < lastItem; i++){
        // postupně procházej každý prvek cesty
        tempCluster = getClusterbyID(tempID);
        // získej i-číslo podle názvu
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
        // procházej položky v adresáři
        fseek(this->file, cluster + i * sizeof(directory_item), SEEK_SET);
        fread(&dirItem, sizeof(directory_item), 1, this->file);
        if(dirItem.item_name == name){
            return dirItem.inode;
        }
    }
    
    return -1;
}

int32_t FS::getClusterbyID(int id) const{
    // Získání i-nodu podle ID
    pseudo_inode inode;
    fseek(this->file, this->sb.inode_start_adress + id * sizeof(pseudo_inode), SEEK_SET);
    fread(&inode, sizeof(pseudo_inode), 1, this->file);
    return inode.direct1;
}

bool FS::inodeExists(int inodeID, const std::string& name) const{
    // Zkontroluj, zda položka s daným názvem existuje v adresáři reprezentovaném inodeID
    directory_item dir;
    int32_t cluster = getClusterbyID(inodeID);
    int maxItems = static_cast<int>(this->sb.cluster_size / sizeof(directory_item));
    
    for(int i = 0; i < maxItems; i++){
        // procházej položky v adresáři
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
        // Procházej položky v adresáři ale hledej podle i-nodu
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

FSStats FS::getStats() {
    FSStats s;
    if (!this->file) return s;

    // Základní informace ze superbloku
    s.total_size = this->sb.disk_size;
    s.cluster_size = this->sb.cluster_size;
    s.total_clusters = this->sb.cluster_count;
    s.total_inodes = INODE_COUNT;

    // cluster bitmap
    int32_t totalClusters = this->sb.cluster_count;
    int32_t clusterBytes = (totalClusters + 7) / 8;
    std::vector<unsigned char> clusterMap(clusterBytes, 0);
    if (fseek(this->file, this->sb.bitmap_start_adress, SEEK_SET) == 0) {
        fread(clusterMap.data(), 1, clusterBytes, this->file);
    }
    // Iterace přes bity a počítání použitých clusterů
    int usedClusters = 0;
    for (int i = 0; i < clusterBytes; ++i) {
        unsigned char b = clusterMap[i];
        for (int bit = 0; bit < 8; ++bit) {
            int idx = i * 8 + bit;
            if (idx >= totalClusters) break;
            if (b & (1 << bit)) ++usedClusters;
        }
    }
    // Nastavení statistik
    s.used_clusters = usedClusters;
    s.free_clusters = totalClusters - usedClusters;

    // inode bitmap
    int32_t totalInodes = INODE_COUNT;
    int32_t inodeBytes = (totalInodes + 7) / 8;
    std::vector<unsigned char> inodeMap(inodeBytes, 0);
    if (fseek(this->file, this->sb.bitmapi_start_adress, SEEK_SET) == 0) {
        fread(inodeMap.data(), 1, inodeBytes, this->file);
    }
    // Iterace přes bity a počítání použitých i-nodů
    int usedInodes = 0;
    for (int i = 0; i < inodeBytes; ++i) {
        unsigned char b = inodeMap[i];
        for (int bit = 0; bit < 8; ++bit) {
            int idx = i * 8 + bit;
            if (idx >= totalInodes) break;
            if (b & (1 << bit)) ++usedInodes;
        }
    }
    // Nastavení statistik
    s.used_inodes = usedInodes;
    s.free_inodes = totalInodes - usedInodes;

    // Počet adresářů počítaný procházením alokovaných i-nodů
    int dirCount = 0;
    for (int i = 0; i < totalInodes; ++i) {
        int byteIndex = i / 8;
        int bitOffset = i % 8;
        if (byteIndex < inodeBytes && (inodeMap[byteIndex] & (1 << bitOffset))) {
            pseudo_inode inode;
            if (fseek(this->file, this->sb.inode_start_adress + i * sizeof(pseudo_inode), SEEK_SET) != 0) continue;
            if (fread(&inode, sizeof(pseudo_inode), 1, this->file) != 1) continue;
            if (inode.isDirectory) ++dirCount;
        }
    }
    s.directory_count = dirCount;

    return s;
}

int FS::changeDir(Path path){
    int inodeID = getInodeFromPath(path, true);
    if(inodeID == -1){
        // Neplatná cesta
        return 1;
    }
    else if(isFile(inodeID)){
        // Cesta nevede do adresáře
        return 2;
    }

    // Aktualizace aktuálního adresáře
    handlePathChange(path);
    CurrentDirInfo.currentDirId = inodeID;
    CurrentDirInfo.currentcluster = getClusterbyID(inodeID);
    CurrentDirInfo.name = path.path.back();
    return 0;
}

int FS::rmDir(Path path){
    int inodeID = getInodeFromPath(path, true);
    if(inodeID == -1){
        // neplatná cesta
        return 1; 
    }
    else if(inodeID == 0){
        // pokus o smazání root adresáře
        return 2; 
    }
    else if(isFile(inodeID)){
        // cesta nevede do adresáře
        return 3; 

    }
    else if(inodeID == this->CurrentDirInfo.currentDirId){
        // pokus o smazání aktuálního adresáře
        return 5; 
    }
    std::vector<std::string> names = getAllFDNames(inodeID);
    if(names.size() > 2){
        // adresář není prázdný
        return 4; 
    }

    // Smazání adresáře
    nullCluster(getClusterbyID(inodeID));
    int clusterID = (getClusterbyID(inodeID) - this->sb.data_start_adress) / this->sb.cluster_size;
    this->setClusterBit(clusterID, 0);
    this->setInodeBit(inodeID, 0);

    // Odstranění položky adresáře z rodičovského adresáře
    int parentInodeID = getInodeFromPath(path, false);
    int32_t parentCluster = getClusterbyID(parentInodeID);
    rmDirItemByname(parentCluster, path.path.back());
    return 0;
}

void FS::rmDirItemByname(int32_t parentCluster, std::string name){
    directory_item dirItem;
    int maxItems = static_cast<int>(this->sb.cluster_size / sizeof(directory_item));
    
    for(int i = 0; i < maxItems; i++){
        // procházej položky v adresáři
        fseek(this->file, parentCluster + i * sizeof(directory_item), SEEK_SET);
        fread(&dirItem, sizeof(directory_item), 1, this->file);
        if(dirItem.item_name == name){
            // smazání položky
            dirItem.inode = 0;
            dirItem.item_name[0] = '\0';
            fseek(this->file, parentCluster + i * sizeof(directory_item), SEEK_SET);
            fwrite(&dirItem, sizeof(directory_item), 1, this->file);
            fflush(this->file);
            break;
        }
    }
}

void FS::rmDirItemByID(int32_t parentCluster, int id){
    directory_item dirItem;
    int maxItems = static_cast<int>(this->sb.cluster_size / sizeof(directory_item));
    
    for(int i = 0; i < maxItems; i++){
        // procházej položky v adresáři
        fseek(this->file, parentCluster + i * sizeof(directory_item), SEEK_SET);
        fread(&dirItem, sizeof(directory_item), 1, this->file);
        if(dirItem.inode == id){
            // smazání položky
            dirItem.inode = 0;
            dirItem.item_name[0] = '\0';
            fseek(this->file, parentCluster + i * sizeof(directory_item), SEEK_SET);
            fwrite(&dirItem, sizeof(directory_item), 1, this->file);
            fflush(this->file);
            break;
        }
    }
}


void FS::nullCluster(int32_t cluster){
    // Přepsání obsahu clusteru nulami
    for(int i = 0; i < CLUSTER_SIZE; i++){
        char zero = 0;
        fseek(this->file, cluster + i, SEEK_SET);
        fwrite(&zero, sizeof(char), 1, this->file);
    }
}

void FS::handlePathChange(Path& path){
    if (path.isAbsolut) {
        // Absolutní cesta - začni od rootu
        currentPath.clear();
        for (const auto& part : path.path) {
            if (part == "." ) {
                // aktuální adresář - nic nedělej
                continue;
            }
            else if (part == "..") {
                // rodičovský adresář - jdi o úroveň výš
                if (!currentPath.empty()) currentPath.pop_back();
            }
            else {
                // normální část cesty - přidej ji do currentPath
                currentPath.push_back(part);
            }
        }
    }
    else if(path.isAbsolut == false){
        for(size_t i = 0; i < path.path.size(); i++){
            if(path.path[i] == ".."){
                // rodičovský adresář - jdi o úroveň výš
                currentPath.pop_back();
            }
            else if(path.path[i] == "."){
                // aktuální adresář - nic nedělej
                continue;
            }
            else{
                // normální část cesty - přidej ji do currentPath
                currentPath.push_back(path.path[i]);
            }
        }
    }
    return;
}

int FS::listDir(Path path){
    int inodeID;
    if(path.path.empty()){
        // pokud je cesta prázdná, použij aktuální adresář
        inodeID = this->CurrentDirInfo.currentDirId;
    }
    else{
        // relativní cesta - zjisti inode podle cesty
        inodeID = getInodeFromPath(path, true);
    }
    if(inodeID == -1){
        // neplatná cesta
        return 1;
    }
    std::vector<std::string> names = getAllFDNames(inodeID);
    for(size_t i = 0; i < names.size(); i++){
        // výpis názvů souborů a adresářů
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
        // procházej položky v adresáři
        fseek(this->file, cluster + i * sizeof(directory_item), SEEK_SET);
        fread(&item, sizeof(directory_item), 1, this->file);
        if (item.item_name[0] != '\0') {
            // přidej název do seznamu, pokud není prázdný
            names.push_back(std::string(item.item_name));
        }
    }
    return names;
}

int FS::inCopy(Path path, std::string sourcePath) {
    int inodeID;
    std::string content;
    
    if (path.path.empty()) {
        // pokud je cesta prázdná, použij aktuální adresář
        inodeID = this->CurrentDirInfo.currentDirId;
    } else {
        // relativní cesta - zjisti inode podle cesty
        inodeID = getInodeFromPath(path, true);
    }

    if (inodeID == -1) {
        // neplatná cesta
        std::cerr << "Cesta neexistuje v souborovém systému." << std::endl;
        return 1;
    }

    if(!isFile(inodeID)){
        // pokud cílový soubor neexistuje, vytvoř ho
        path.path.push_back(sourcePath.substr(sourcePath.find_last_of("/\\") + 1));
        makeFile(path);
        inodeID = getInodeFromPath(path, true);
    }

    // 🔧 Otevři soubor v binárním režimu a načti celý obsah
    std::ifstream inFile(sourcePath, std::ios::binary);
    if (!inFile) {
        std::cerr << "Soubor se nepodařilo otevřít: " << sourcePath << std::endl;
        return 2;
    }

    std::ostringstream ss;
    ss << inFile.rdbuf();
    // načte celý obsah najednou
    content = ss.str();

    inFile.close();

    int status = writeContentToFile(inodeID, content);
    return status;
}

int FS::writeContentToFile(int inodeID, std::string content) {
    if (inodeID < 0) {
        // neplatný inodeID
        std::cerr << "Chyba: Neplatný inodeID." << std::endl;
        return 1;
    }

    // Načti inode z disku
    pseudo_inode inode;
    fseek(this->file, this->sb.inode_start_adress + inodeID * sizeof(pseudo_inode), SEEK_SET);
    if (fread(&inode, sizeof(pseudo_inode), 1, this->file) != 1) {
        std::cerr << "Nelze načíst inode." << std::endl;
        return 1;
    }

    // Rozbij obsah na bloky
    std::vector<std::string> blocks = splitToBlocks(content, this->sb.cluster_size);
    size_t toWrite = blocks.size();
    size_t written = 0;

    // Přímo pracuj s políčky inode (ujisti se, že pseudo_inode má direct1..5 a indirect1..2)
    int32_t *directsArr[DIRECT_COUNT] = {
        &inode.direct1, &inode.direct2, &inode.direct3, &inode.direct4, &inode.direct5
    };
    int32_t *indirectsArr[INDIRECT_COUNT] = {
        &inode.indirect1, &inode.indirect2
    };

    // Direct pointers
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

    // Indirect pointers
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

    // Po skončení aktualizuj inode (file_size atd.) a ulož jej jednou
    inode.file_size = content.size();
    fseek(this->file, this->sb.inode_start_adress + inodeID * sizeof(pseudo_inode), SEEK_SET);
    fwrite(&inode, sizeof(pseudo_inode), 1, this->file);
    fflush(this->file);
    return 0;
}

std::vector<std::string> FS::splitToBlocks(const std::string& content, size_t blockSize) {
    std::vector<std::string> blocks;

    // Rozbij obsah na bloky
    for (size_t i = 0; i < content.size(); i += blockSize) {
        blocks.push_back(content.substr(i, blockSize));
    }

    return blocks;
}

std::string FS::readContentFromFile(int inodeID){
    if(inodeID < 0 || !isFile(inodeID)){
        // neplatný inodeID nebo není soubor
        return "";
    }

    // Načti inode z disku
    pseudo_inode inode;
    fseek(this->file, this->sb.inode_start_adress + inodeID * sizeof(pseudo_inode), SEEK_SET);
    fread(&inode, sizeof(pseudo_inode), 1, this->file);

    // --- DIRECT BLOCKS ---
    std::string content;
    size_t remaining = inode.file_size;
    size_t blockSize = this->sb.cluster_size;
    std::vector<char> buffer(blockSize);

    int32_t directs[] = {
        inode.direct1, inode.direct2, inode.direct3,
        inode.direct4, inode.direct5
    };

    // Čtení z přímých bloků
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
        // pokud indirect ukazatel nulový, pokračuj
        if (indirects[i] == 0) continue;

        size_t entries = blockSize / sizeof(int32_t);
        std::vector<int32_t> indirectBlock(entries, 0);

        fseek(this->file, indirects[i], SEEK_SET);
        fread(indirectBlock.data(), sizeof(int32_t), entries, this->file);

        for (size_t j = 0; j < entries && remaining > 0; j++) {
            // pokud je blok nulový, pokračuj
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
        // neplatná cesta
        return 2;
    }
    std::string content = readContentFromFile(inodeID);
    // Zápis do externího souboru
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
        // neplatná cesta
        return 1;
    }
    // Výpis obsahu souboru na konzoli
    std::string content = readContentFromFile(inodeID);
    std::cout << content << std::endl;
    return 0;
}

bool FS::isFile(int inodeID) const{
    // Získání i-nodu podle ID
    pseudo_inode inode;
    fseek(this->file, this->sb.inode_start_adress + inodeID * sizeof(pseudo_inode), SEEK_SET);
    fread(&inode, sizeof(pseudo_inode), 1, this->file);
    return !inode.isDirectory;
}

int FS::makeFile(Path path){
    int parrentInodeID = getInodeFromPath(path, false);
    if(parrentInodeID == -1){
        // neplatná cesta
        return 1;
    }
    else if(inodeExists(parrentInodeID, path.path.back())){
        // soubor již existuje
        return 2;
    }

    // Vytvoření nového i-nodu
    int32_t inodePos = findFreeInode();
    int inodeID = (inodePos - this->sb.inode_start_adress) / sizeof(pseudo_inode);

    int32_t parrentCluster = getClusterbyID(parrentInodeID);
    writeDIRItem(parrentCluster, path.path.back(), inodeID);

    pseudo_inode file;

    // Inicializace i-nodu
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

int FS::copy(Path source, Path dest, bool removeOriginal){
    int sourceID = getInodeFromPath(source, true);
    int destID = getInodeFromPath(dest, true);

    if(sourceID == -1){
        // neplatný inodeID nebo není soubor
        return 2;
    }
    else if(!isFile(sourceID)){
        // zdroj není soubor
        return 4;
    }

    // soubor neexistuje a je treba ho vytvorit
    if(destID == -1){
        int parentDestID = getInodeFromPath(dest, false);
        if(parentDestID == -1){
            std::cout << "fakt neexistuje" << std::endl;
            return 3;
        }
        makeFile(dest);
        destID = getInodeFromPath(dest, true);
    }
    // soubor existuje a je to adresar
    else if(destID != -1 && !isFile(destID)){
        std::string fileName = source.path.back();
        dest.path.push_back(fileName);
        makeFile(dest);
        destID = getInodeFromPath(dest, true);
    }
    // jinak soubor existuje a je to soubor, prepiseme ho

    std::string content = readContentFromFile(sourceID);
    writeContentToFile(destID, content);
    if(removeOriginal){
        remove(source);
    }
    return 0;
}

int FS::remove(Path source){
    int inodeID = getInodeFromPath(source, true);
    // validate inode and ensure it's a regular file
    if(inodeID == -1){
        // neplatná cesta
        return 1;
    }
    bool isFileFlag = isFile(inodeID);
    if(!isFileFlag){
        // cesta je adresář
        return 3;
    }

    // Odebrání položky adresáře z rodičovského adresáře
    int parentInodeID = getInodeFromPath(source, false);
    int32_t parentCluster = getClusterbyID(parentInodeID);
    rmDirItemByID(parentCluster, inodeID);
    
    // Uvolnění alokovaných clusterů a i-nodu
    pseudo_inode inode;
    fseek(this->file, this->sb.inode_start_adress + inodeID * sizeof(pseudo_inode), SEEK_SET);
    fread(&inode, sizeof(pseudo_inode), 1, this->file);
    
    std::vector<int32_t> directs = {
        inode.direct1, inode.direct2, inode.direct3,
        inode.direct4, inode.direct5
    };
    std::vector<int32_t> indirects = {
        inode.indirect1, inode.indirect2
    };

    for(size_t i = 0; i < directs.size(); i++){
        if(directs[i] != 0){
            // nulování clusteru
            nullCluster(directs[i]);
            int clusterID = (directs[i] - this->sb.data_start_adress) / this->sb.cluster_size;
            this->setClusterBit(clusterID, 0);
        }
    }
    for(size_t i = 0; i < indirects.size(); i++){
        if(indirects[i] != 0){
            size_t entries = this->sb.cluster_size / sizeof(int32_t);
            std::vector<int32_t> indirectBlock(entries, 0);

            fseek(this->file, indirects[i], SEEK_SET);
            fread(indirectBlock.data(), sizeof(int32_t), entries, this->file);

            for(size_t j = 0; j < entries; j++){
                if(indirectBlock[j] != 0){
                    // nulování clusteru
                    nullCluster(indirectBlock[j]);
                    int clusterID = (indirectBlock[j] - this->sb.data_start_adress) / this->sb.cluster_size;
                    this->setClusterBit(clusterID, 0);
                }
            }
            int clusterID = (indirects[i] - this->sb.data_start_adress) / this->sb.cluster_size;
            this->setClusterBit(clusterID, 0);
        }
    }
    this->setInodeBit(inodeID, 0);
    return 0;
}

int FS::info(Path path){
    // Získání informací o souboru nebo adresáři
    int inodeID = getInodeFromPath(path, true);
    int parentID = getInodeFromPath(path, false);
    if(inodeID == -1 || parentID == -1){
        return 1;
    }
    // Získání názvu podle i-nodu
    std::string name = findNameByInode(getClusterbyID(parentID), inodeID);
    pseudo_inode inode;
    fseek(this->file, this->sb.inode_start_adress + inodeID * sizeof(pseudo_inode), SEEK_SET);
    fread(&inode, sizeof(pseudo_inode), 1, this->file);
    std::cout << "Name: " << name << std::endl;
    std::cout << "Inode ID: " << inodeID << std::endl;
    std::cout << "Type: " << (inode.isDirectory ? "Directory" : "File") <<  std::endl;
    std::cout << "Size: " << inode.file_size << " bytes" << std::endl;
    return 0;
}

int FS::xcopy(Path source1, Path source2, Path dest){
    // Získání i-nodů podle cest
    int source1ID = getInodeFromPath(source1, true);
    int source2ID = getInodeFromPath(source2, true);
    int destDirID = getInodeFromPath(dest, false);

    if(source1ID == -1 || source2ID == -1 || destDirID == -1){
        return 1;
    }
    // soubor neexistuje a je treba ho vytvorit
    makeFile(dest);
    std::string source1Content = readContentFromFile(source1ID);
    std::string source2Content = readContentFromFile(source2ID);
    std::string destContent = source1Content + source2Content;

    // Získání i-nodu cílového souboru
    int destID = getInodeFromPath(dest, true);
    writeContentToFile(destID, destContent);
    return 0;
}

int FS::add(Path source, Path dest){
    // Získání i-nodů podle cest
    int sourceID = getInodeFromPath(source, true);
    int destID = getInodeFromPath(dest, true);

    if(sourceID == -1 || destID == -1){
        return 1;
    }

    // Přidání obsahu zdrojového souboru na konec cílového souboru
    std::string sourceContent = readContentFromFile(sourceID);
    std::string destContent = readContentFromFile(destID);
    std::string finalContent = destContent + sourceContent;
    writeContentToFile(destID, finalContent);
    return 0;
}