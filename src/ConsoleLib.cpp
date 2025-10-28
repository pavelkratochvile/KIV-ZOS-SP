#include "ConsoleLib.hpp"
#include "Path.hpp"
#include <filesystem>
#include <iostream>
#include <sstream>

int ConsoleLib::format(std::string fileName, int size){
    this->fileSystem = FS(fileName);
    if(fileSystem.format(size) != 0){
        return 1;
    }
    if(fileSystem.attach() != 0){
        return 1;
    }
    fileSystem.makeRoot();
    return 0;
}

int ConsoleLib::attach(std::string fileName){
    this->fileSystem = FS(fileName);
    if(fileSystem.attach() != 0){
        return 2;
    }
    return 0;
}

int ConsoleLib::InitFS(int argc, char** argv, std::string command){
    if(argc != 2){
        return 1;
    }
    std::vector<std::string> arguments = this->parseCommand(command);
    std::string path = std::string(argv[1]);

    if(arguments.size() == 2){
        int size = std::stoi(arguments[1]);
        if(this->format(path, size) != 0){
            return 1;
        }
    }

    if(arguments.size() == 1){
        if(this->attach(path) != 0){
            return 1;
        }
    }

    return 0;
}

void ConsoleLib::readConsole(int argc, char** argv){
    std::string command;
    std::cout << "=== Konzole souborového systému ===" << std::endl;
    std::cout << "Zadej příkaz (nebo 'exit' pro ukončení):" << std::endl;

    while(true){
        std::cout << "Velikost clusteru: " << this->fileSystem.sb.cluster_size << " bajtů" << std::endl;
        std::cout << "> " << pathToString(this->fileSystem.currentPath) << "$ ";
        std::getline(std::cin, command);
        std::vector<std::string> arguments = this->parseCommand(command);

        if(command == "exit"){
            std::cout << "Ukončuji..." << std::endl;
            break;
        }
        else if(arguments[0] == "format"){
            if(this->InitFS(argc, argv, command) != 0){
                std::cout << "Nebylo to inicializováno" << std::endl;
            }
        }
        else if(arguments[0] == "mkdir"){
            int status = this->makeDir(command);
            std::cout << "status = " << status  << std::endl;
            if(status == 1){
                std::cout << "Unable to create directory: Wrong path." << std::endl;
            }
            else if(status == 2){
                std::cout << "Unable to create directory: Directory already exists." << std::endl;
            }
            else{
                std::cout << "Directory created successfully." << std::endl;
            }
        }
        else if(arguments[0] == "cd"){
            int status = this->changeDir(command);
            if(status == 1){
                std::cout << "Unable to change directory: Wrong path." << std::endl;
            }
            else if(status == 2){
                std::cout << "Unable to change directory: Not a directory." << std::endl;
            }
            else{
                std::cout << "Directory changed successfully." << std::endl;
            }
        }
        else if(arguments[0] == "ls"){
            int status = listDir(command);
            if(status == 1){
                std::cout << "Unable to list directory: Wrong path." << std::endl;
            }
        }
        else if(arguments[0] == "incp"){
            int status = inCopy(command);
            if(status == 1){
                std::cout << "Unable to copy file: Wrong output path." << std::endl;
            }
            else if(status == 2){
                std::cout << "Unable to copy file: Wrong input path." << std::endl;
            }
            else if(status == 3){
                std::cout << "Unable to copy file: Not a regular file but a directory." << std::endl;
            }
            else{
                std::cout << "File copied successfully." << std::endl;
            }
        }
        else if(arguments[0] == "touch"){
            int status = this->makeFile(command);
            if(status == 1){
                std::cout << "Unable to create file: Wrong path." << std::endl;
            }
            else if(status == 2){
                std::cout << "Unable to create file: File already exists." << std::endl;
            }
            else{
                std::cout << "File created successfully." << std::endl;
            }
        }
        else if(arguments[0] == "cat"){
            int status = this->catFile(command);
            if(status == 1){
                std::cout << "Unable to read file: Wrong path." << std::endl;
            }
            else{
                std::cout << "File read successfully." << std::endl;
            }
        }
        else if(arguments[0] == "outcp"){
            int status = this->outCopy(command);
            if(status == 1){
                std::cout << "Unable to copy file: Wrong output path." << std::endl;
            }
            else if(status == 2){
                std::cout << "Unable to copy file: Wrong input path." << std::endl;
            }
            else{
                std::cout << "File copied successfully." << std::endl;
            }
        }
        else if(arguments[0] == "rmdir"){
            int status = this->rmDir(command);
            if(status == 1){
                std::cout << "Unable to remove directory: Wrong path." << std::endl;
            }
            else if(status == 2){
                std::cout << "Unable to remove directory: Cant remove root directory." << std::endl;
            }
            else if(status == 3){
                std::cout << "Unable to remove directory: Not a directory." << std::endl;
            }
            else if(status == 4){
                std::cout << "Unable to remove directory: Directory is not empty." << std::endl;
            }
            else if(status == 5){
                std::cout << "Unable to remove directory: Directory is current working directory." << std::endl;
            }
            else{
                std::cout << "Directory removed successfully." << std::endl;
            }
        }
        else if(arguments[0] == "pwd"){
            std::cout << "Path: " << pathToString(this->fileSystem.currentPath) << std::endl;
        }
        else{
            std::cout << "Příkaz <" << command << "> je neznámý!" << std::endl;
        }

    }
}

int ConsoleLib::makeFile(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    if(arguments.size() != 2){
        return 1;
    }
    
    Path path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    int status = this->fileSystem.makeFile(path);

    if(status != 0){
        return status;
    }
    return 0;
}

int ConsoleLib::makeDir(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    if(arguments.size() != 2){
        return 1;
    }
    
    Path path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    int status = this->fileSystem.makeDir(path);
    if(status == 1){
        return 1;
    }
    else if(status == 2){
        return 2;
    }
    return 0;
}

int ConsoleLib::rmDir(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path path;
    if(arguments.size() > 2){
        return 1;
    }

    if(arguments.size() == 1){
        path = Path();
    }
    else{
        path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    }
    int status = this->fileSystem.rmDir(path);
    return status;
}

int ConsoleLib::listDir(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path path;
    if(arguments.size() > 2){
        return 1;
    }

    if(arguments.size() == 1){
        path = Path();
    }
    else{
        path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    }
    
    int status = this->fileSystem.listDir(path);
    if(status == 1){
        return 1;
    }
    return 0;
}

int ConsoleLib::inCopy(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path path;
    if(arguments.size() != 3){
        std::cout << "Incorrect number of arguments for inCopy." << std::endl;
        return 1;
    }
    else{
        path = Path(this->pathToVector(arguments[2]), arguments[2][0] == '/');
    }
    
    int status = this->fileSystem.inCopy(path, std::string(arguments[1]));
    
    if(status != 0){
        return status;
    }
    else{
        return 0;
    }
}

int ConsoleLib::outCopy(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path path;
    if(arguments.size() != 3){
        std::cout << "Incorrect number of arguments for inCopy." << std::endl;
        return 1;
    }
    else{
        path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    }   
    int status = this->fileSystem.outCopy(path, std::string(arguments[2]));
    return status;

}

int ConsoleLib::catFile(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    if(arguments.size() != 2){
        return 1;
    }
    
    Path path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    int status = this->fileSystem.catFile(path);
    return status;
}

int ConsoleLib::changeDir(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    if(arguments.size() != 2){
        return 1;
    }
    
    Path path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    int status = this->fileSystem.changeDir(path);
    if(status != 0){
        return status;
    }
    return 0;
}

std::vector<std::string> ConsoleLib::parseCommand(std::string command){
    std::stringstream ss(command);
    std::string word;
    std::vector<std::string> parts;

    while(ss >> word){
        parts.push_back(word);
    }
    return parts;
}

std::vector<std::string> ConsoleLib::pathToVector(std::string str){
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    char delimeter = '/';

    while(std::getline(ss, token, delimeter)){
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}
std::string ConsoleLib::pathToString(const std::vector<std::string>& path){
    std::string result;
    for(const auto& part : path){
        result += "/" + part;
    }
    return result.empty() ? "/" : result;
}

ConsoleLib::ConsoleLib() {
}

ConsoleLib::~ConsoleLib() {
}