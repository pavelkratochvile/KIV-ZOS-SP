#include <iostream>
#include "FS.hpp"
#include "ConsoleLib.hpp"

int main(int argc, char** argv){
    // Kontrola, zda byl zadán souborový systém jako argument
    if(argc < 2){
        std::cout << "Nebyl zadán žádný souborový systém." << std::endl;
        return 1;
    }

    // Inicializace konzole
    ConsoleLib console = ConsoleLib();
    // Spuštění čtení příkazů z konzole
    console.readConsole(argc, argv);
    
    return 0;
}