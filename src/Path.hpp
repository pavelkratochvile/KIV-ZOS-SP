#pragma once
#include <string>
#include <vector>

class Path{
    public:
        // Konstruktor a destruktor
        Path(std::vector<std::string> path, bool isAbsolut);
        Path();
        ~Path(){};

        // Atributy cesty
        bool isAbsolut;
        std::vector<std::string> path;
};