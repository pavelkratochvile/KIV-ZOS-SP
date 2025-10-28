#pragma once
#include "superblock.hpp"
#include "Path.hpp"
#include "pseudo_inode.hpp"
#include "dirInfo.hpp"
#include <string>
#include <vector>

class FS{
    public:
        FS(std::string path);
        ~FS();
        FS();
        int format(int size);
        int attach();
        int32_t findPositionClusterDIR(int32_t direct);
        int32_t findPositionInodeDIR(int32_t inode);
        int32_t findDirectsClusterDIR(int32_t cluster);
        int makeRoot();
        int makeDir(Path path);
        int makeFile(Path path);
        int changeDir(Path path);
        int listDir(Path path);
        int inCopy(Path path, std::string sourcePath);
        int catFile(Path path);
        int outCopy(Path path, std::string destPath);
        int rmDir(Path path);

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
};