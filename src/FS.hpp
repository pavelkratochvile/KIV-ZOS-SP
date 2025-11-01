#pragma once
#include "superblock.hpp"
#include "Path.hpp"
#include "pseudo_inode.hpp"
#include "dirInfo.hpp"
#include <string>
#include <vector>

// Statistiky souborového systému vrácené z getStats()
struct FSStats {
    int64_t total_size = 0;
    int32_t cluster_size = 0;
    int32_t total_clusters = 0;
    int32_t used_clusters = 0;
    int32_t free_clusters = 0;
    int32_t total_inodes = 0;
    int32_t used_inodes = 0;
    int32_t free_inodes = 0;
    int32_t directory_count = 0;
};

class FS{
    public:
        FS(std::string path);
        ~FS();
        FS();
        int format(int size);
        int attach();
        int makeRoot();
        int makeDir(Path path);
        int makeFile(Path path);
        int changeDir(Path path);
        int listDir(Path path);
        int inCopy(Path path, std::string sourcePath);
        int catFile(Path path);
        int outCopy(Path path, std::string destPath);
        int rmDir(Path path);
        int copy(Path source, Path dest, bool removeOriginal);
        int remove(Path source);
        int info(Path path);
        int xcopy(Path source1, Path source2, Path dest);
        int add(Path source, Path dest);

        int32_t findFreeCluster();
        int32_t findFreeInode();
        
        void setInodeBit(int inodeID, int bit);
        void setClusterBit(int clusterID, int bit);
        void writeDIRItem(int32_t clusterAddr, std::string dirName, int32_t inodeID);
        bool inodeExists(int inodeID, const std::string& name) const;
        std::vector<std::string> getAllFDNames(int inodeID);
        int writeContentToFile(int inodeID, std::string content);
        std::string readContentFromFile(int inodeID);
        std::vector<std::string> splitToBlocks(const std::string& content, size_t blockSize);
        void rmDirItemByname(int32_t cluster, std::string name);
        void rmDirItemByID(int32_t cluster, int id);
        
        std::string findNameByInode(int32_t cluster, int32_t inodeID) const;
        int getInodeFromPath(Path path, bool editLast);
        int findInodeIdByName(int32_t cluster, std::string name);
        int32_t getClusterbyID(int id) const;
        void handlePathChange(Path& path);
        bool isFile(int inodeID) const;
        void nullCluster(int32_t cluster);

        dirInfo CurrentDirInfo;
        std::vector<std::string> currentPath;
        
        superblock sb;
        std::string path;
        FILE* file;
    FSStats getStats();
};