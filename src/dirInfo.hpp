#pragma once
#include <iostream>
#include <string>
#include <cstdint>

class dirInfo{
    public:
        dirInfo(int currentDirId, int32_t currentcluster, std::string name);
        dirInfo(){};
        ~dirInfo(){};
        int currentDirId;
        int32_t currentcluster;
        std::string name;
};