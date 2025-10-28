#include "dirInfo.hpp"

dirInfo::dirInfo(int currentDirId, int32_t currentcluster, std::string name){
    this->currentDirId = currentDirId;
    this->currentcluster = currentcluster;
    this->name = name;
}