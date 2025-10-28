#pragma once
#include <string>
#include <vector>

class Path{
    public:
        Path(std::vector<std::string> path, bool isAbsolut);
        Path();
        ~Path(){};

        bool isAbsolut;
        std::vector<std::string> path;
};