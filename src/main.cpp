#include <iostream>
#include "FS.hpp"
#include "ConsoleLib.hpp"

int main(int argc, char** argv){
    if(argc < 2){
        std::cout << "Nebyl zadán žádný souborový systém." << std::endl;
        return 1;
    }
    
    ConsoleLib console = ConsoleLib();
    console.readConsole(argc, argv);
    return 0;
}