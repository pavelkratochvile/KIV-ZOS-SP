#pragma once
#include "superblock.hpp"
#include "Path.hpp"
#include "pseudo_inode.hpp"
#include "dirInfo.hpp"
#include <string>
#include <vector>

// Statistiky souborového systému vrácené z getStats()
struct FSStats {
    int64_t total_size = 0;         // Celková velikost souborového systému v bytech
    int32_t cluster_size = 0;       // Velikost clusteru
    int32_t total_clusters = 0;     // Počet clusterů
    int32_t used_clusters = 0;      // Počet využitých clusterů
    int32_t free_clusters = 0;      // Počet volných clusterů
    int32_t total_inodes = 0;       // Počet i-nodů
    int32_t used_inodes = 0;        // Počet využitých i-nodů
    int32_t free_inodes = 0;        // Počet volných i-nodů
    int32_t directory_count = 0;    // Počet adresářů
};

class FS{
    public:
        // Konstruktor a destruktor
        FS(std::string path);
        ~FS();
        FS();

        // Instance souborového systému
        superblock sb;
        std::string path;
        FILE* file;
        dirInfo CurrentDirInfo;
        std::vector<std::string> currentPath;
        
        /* -------------- Příkazy -------------- */

        // Formátování souborového systému
        int format(int size);
        
        // Připojení existujícího souborového systému
        int attach();
        
        // Vytvoření kořenového adresáře
        int makeRoot();

        // Vytvoření adresáře
        int makeDir(Path path);
        
        // Vytvoření souboru
        int makeFile(Path path);
        
        // Změna aktuálního adresáře
        int changeDir(Path path);
        
        // Výpis obsahu adresáře
        int listDir(Path path);
        
        // Kopírování souboru z reálného souborového systému do našeho souborového systému
        int inCopy(Path path, std::string sourcePath);
        
        // Zobrazení obsahu souboru
        int catFile(Path path);
        
        // Zkopírování souboru ven ze souborového systému do reálného souborového systému
        int outCopy(Path path, std::string destPath);
        
        // Odstranění prázdného adresáře
        int rmDir(Path path);
        
        // Kopírování souboru nebo adresáře nebo přesun
        int copy(Path source, Path dest, bool removeOriginal);
        
        // Odstranění souboru
        int remove(Path source);
        
        // Přesunutí souboru
        int info(Path path);

        // Rozšířené kopírování dvou souborů do jednoho
        int xcopy(Path source1, Path source2, Path dest);
        
        // Přidání obsahu souboru na konec jiného souboru
        int add(Path source, Path dest);


        /* -------------- Pomocné funkce -------------- */

        
        // Metoda pro nalezení volného clusteru
        int32_t findFreeCluster();
        
        // Metoda pro nalezení volného i-nodu
        int32_t findFreeInode();

        // Metoda pro nastavení bitu i-nodu na pozadovanou hodnotu (0 nebo 1)
        void setInodeBit(int inodeID, int bit);
        
        // Metoda pro nastavení bitu clusteru na pozadovanou hodnotu (0 nebo 1)
        void setClusterBit(int clusterID, int bit);
        
        // Metoda pro zápis položky adresáře. clusterAddr je adresa clusteru, kam se má položka zapsat
        void writeDIRItem(int32_t clusterAddr, std::string dirName, int32_t inodeID);

        // Metoda pro kontrolu existence souboru/adresáře v adresáři s daným i-nodeID
        bool inodeExists(int inodeID, const std::string& name) const;
        
        // Metoda pro získání všech názvů souborů a adresářů v adresáři s daným i-nodeID
        std::vector<std::string> getAllFDNames(int inodeID);
        
        // Metoda pro zápis obsahu do souboru s daným i-nodeID
        int writeContentToFile(int inodeID, std::string content);
        
        // Metoda pro čtení obsahu ze souboru s daným i-nodeID
        std::string readContentFromFile(int inodeID);

        // Metoda pro rozdělení obsahu na bloky o velikosti blockSize(blockSize je většinou velikost clusteru)
        std::vector<std::string> splitToBlocks(const std::string& content, size_t blockSize);
        
        // Metoda pro odstranění položky adresáře podle jména
        void rmDirItemByname(int32_t cluster, std::string name);
        
        // Metoda pro odstranění položky adresáře podle i-node ID
        void rmDirItemByID(int32_t cluster, int id);

        // Metoda pro nalezení názvu souboru/adresáře podle i-node ID v daném clusteru
        std::string findNameByInode(int32_t cluster, int32_t inodeID) const;
        
        // Metoda pro získání i-node ID ze zadané cesty
        int getInodeFromPath(Path path, bool editLast);
        
        // Metoda pro hledání i-node ID podle jména v daném clusteru
        int findInodeIdByName(int32_t cluster, std::string name);

        // Metoda pro získání clusteru podle i-node ID
        int32_t getClusterbyID(int id) const;
        
        // Metoda pro zpracování změny cesty
        void handlePathChange(Path& path);

        // Metoda pro kontrolu existence souboru/adresáře v adresáři s daným i-nodeID. True = soubor, False = adresář
        bool isFile(int inodeID) const;
        
        // Metoda pro vymazání obsahu clusteru (nastavení všech bajtů na 0)
        void nullCluster(int32_t cluster);
        
        // Metoda pro získání statistik souborového systému
        FSStats getStats();
};