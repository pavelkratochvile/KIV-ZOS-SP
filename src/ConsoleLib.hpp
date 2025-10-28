#pragma once
#include"FS.hpp"
#include<iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

class ConsoleLib{
    public:
        ConsoleLib();
        ~ConsoleLib();
        FS fileSystem;
        int format(std::string fileName, int size);
        int attach(std::string fileName);

        int InitFS(int argc, char** argv, std::string command);
        int makeDir(std::string command);
        int makeFile(std::string command);
        int changeDir(std::string command);
        int listDir(std::string command);
        int inCopy(std::string command);
        int outCopy(std::string command);
        int catFile(std::string command);
        int rmDir(std::string command);

        std::string pathToString(const std::vector<std::string>& path);
        std::vector<std::string> pathToVector(std::string path);

        void readConsole(int argc, char** argv);
        std::vector<std::string> parseCommand(std::string command);
};