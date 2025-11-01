#include "ConsoleLib.hpp"
#include "Path.hpp"
#include <filesystem>
#include <iostream>
#include <sstream>

int ConsoleLib::format(std::string fileName, int size){
    this->fileSystem = FS(fileName);
    if(fileSystem.format(size) != 0){
        // Chyba při formátování souborového systému
        return 1;
    }
    if(fileSystem.attach() != 0){
        // Chyba při připojování souborového systému
        return 1;
    }
    fileSystem.makeRoot();
    return 0;
}

int ConsoleLib::attach(std::string fileName){
    this->fileSystem = FS(fileName);
    if(fileSystem.attach() != 0){
        // Chyba při připojování souborového systému
        return 2;
    }
    return 0;
}

void ConsoleLib::InitFS(int argc, char** argv, std::string command){
    if(argc != 2){
        // Chyba při inicializaci souborového systému
        std::cout << "FS unable to initialize. Invalid arguments." << std::endl;
        return;
    }
    std::vector<std::string> arguments = this->parseCommand(command);
    std::string path = std::string(argv[1]);

    if(arguments.size() == 2){
        int size = std::stoi(arguments[1]);
        if(size > 32768){
            // Chyba: velikost souborového systému je příliš velká
            std::cout << "FS cant be larger than 32GB." << std::endl;
            return;
        }
        if(this->format(path, size) != 0){
            // Chyba při formátování souborového systému
            std::cout << "FS unable to initialize." << std::endl;
            return;
        }
    }

    if(arguments.size() == 1){
        if(this->attach(path) != 0){
            // Chyba při připojování souborového systému
            return;
        }
    }
    std::cout << "FS initialized successfully." << std::endl;
}

void ConsoleLib::readConsole(int argc, char** argv){
    std::string command;
    std::cout << "=== Filesystem console ===" << std::endl;
    std::cout << "Enter command (or 'exit' to quit):" << std::endl;

    while(true){
        std::cout << "> " << pathToString(this->fileSystem.currentPath) << "$ ";
        std::getline(std::cin, command);
        std::vector<std::string> arguments = this->parseCommand(command);

        if(command == "exit"){
            // Ukončení konzole
            this->exit();
            break;
        }
        else if(arguments[0] == "format"){
            // Inicializace souborového systému
            this->InitFS(argc, argv, command);
        }
        else if(arguments[0] == "mkdir"){
            // Příkaz pro vytvoření adresáře
            this->makeDir(command);
        }
        else if(arguments[0] == "cd"){
            // Příkaz pro změnu adresáře
            this->changeDir(command);
        }
        else if(arguments[0] == "ls"){
            // Příkaz pro výpis obsahu adresáře
            this->listDir(command);
        }
        else if(arguments[0] == "incp"){
            // Příkaz pro kopírování souboru z reálného souborového systému do našeho souborového systému
            this->inCopy(command);
        }
        else if(arguments[0] == "touch"){
            // Příkaz pro vytvoření souboru
            this->makeFile(command);
        }
        else if(arguments[0] == "cat"){
            // Příkaz pro zobrazení obsahu souboru
            this->catFile(command);
        }
        else if(arguments[0] == "outcp"){
            // Příkaz pro zkopírování souboru ven ze souborového systému do reálného souborového systému
            this->outCopy(command);
        }
        else if(arguments[0] == "rmdir"){
            // Příkaz pro odstranění prázdného adresáře
            this->rmDir(command);
        }
        else if(arguments[0] == "pwd"){
            // Příkaz pro zobrazení aktuálního pracovního adresáře
            std::cout << "Path: " << pathToString(this->fileSystem.currentPath) << std::endl;
        }
        else if(arguments[0] == "cp"){
            // Příkaz pro kopírování souboru nebo adresáře
            this->copy(command);
        }
        else if(arguments[0] == "rm"){
            // Příkaz pro odstranění souboru
            this->remove(command);
        }
        else if(arguments[0] == "mv"){
            // Příkaz pro přesunutí souboru
            this->move(command);
        }
        else if(arguments[0] == "statfs"){
            // Příkaz pro zobrazení informací o souborovém systému
            this->statfs();
        }
        else if(arguments[0] == "info"){
            // Příkaz pro zobrazení informací o souboru nebo adresáři
            this->info(command);
        }
        else if(arguments[0] == "load"){
            // Příkaz pro načtení souboru
            this->load(command);
        }
        else if(arguments[0] == "xcp"){
            // Příkaz pro zkopírování souboru
            this->xcopy(command);
        }
        else if(arguments[0] == "add"){
            // Příkaz pro přídíní obsahu souboru na konec jiného souboru
            this->add(command);
        }
        else{
            // Neznámý příkaz
            std::cout << "Command <" << command << "> is unknown!" << std::endl;
        }

    }
}

void ConsoleLib::exit(){
    std::cout << "Ending..." << std::endl;
    return;
}

void ConsoleLib::makeFile(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    if(arguments.size() != 2){
        // Nesprávný počet argumentů pro vytvoření souboru
        std::cout << "Unable to create file: Invalid number of arguments." << std::endl;
        return;
    }
    
    Path path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    int status = this->fileSystem.makeFile(path);

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

void ConsoleLib::makeDir(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    if(arguments.size() != 2){
        // Nesprávný počet argumentů pro vytvoření adresáře
        std::cout << "Unable to create directory: Invalid number of arguments." << std::endl;
        return;
    }
    
    Path path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    int status = this->fileSystem.makeDir(path);
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

void ConsoleLib::remove(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path path;
    if(arguments.size() != 2){
        // Nesprávný počet argumentů pro odstranění souboru
        std::cout << "Unable to remove file: Invalid number of arguments." << std::endl;
        return;
    }
    else{
        path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    }

    int status = this->fileSystem.remove(path);
    if(status == 1){
        std::cout << "Unable to remove file: Wrong path." << std::endl;
    }
    else if(status == 3){
        std::cout << "Unable to remove file: Path is a directory." << std::endl;
    }
    else{
        std::cout << "File removed successfully." << std::endl;
    }
}

void ConsoleLib::rmDir(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path path;
    if(arguments.size() > 2){
        // Nesprávný počet argumentů pro odstranění adresáře
        std::cout << "Unable to remove directory: Invalid number of arguments." << std::endl;
        return;
    }

    if(arguments.size() == 1){
        path = Path();
    }
    else{
        // Cesta k adresáři k odstranění
        path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    }
    int status = this->fileSystem.rmDir(path);
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

void ConsoleLib::listDir(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path path;
    if(arguments.size() > 2){
        // Nesprávný počet argumentů pro výpis adresáře
        std::cout << "Unable to list directory: Invalid number of arguments." << std::endl;
        return;
    }

    if(arguments.size() == 1){
        // Výpis aktuálního adresáře
        path = Path();
    }
    else{
        // Cesta k adresáři k výpisu
        path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    }
    
    int status = this->fileSystem.listDir(path);
    if(status == 1){
        std::cout << "Unable to list directory: Wrong path." << std::endl;
    }
}

void ConsoleLib::inCopy(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path path;
    if(arguments.size() != 3){
        // Nesprávný počet argumentů pro inCopy
        std::cout << "Incorrect number of arguments for inCopy." << std::endl;
        return;
    }
    else{
        // Cesta k souboru v souborovém systému
        path = Path(this->pathToVector(arguments[2]), arguments[2][0] == '/');
    }
    
    int status = this->fileSystem.inCopy(path, std::string(arguments[1]));
    
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

void ConsoleLib::xcopy(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path source1;
    Path source2;
    Path dest;
    
    if(arguments.size() != 4){
        // Nesprávný počet argumentů pro xcopy
        std::cout << "Incorrect number of arguments for xcopy." << std::endl;
        return;
    }
    else{
        // Cesty k souborům a cílovému adresáři
        source1 = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
        source2 = Path(this->pathToVector(arguments[2]), arguments[2][0] == '/');
        dest = Path(this->pathToVector(arguments[3]), arguments[3][0] == '/');
    }
    int status = this->fileSystem.xcopy(source1, source2, dest);
    if(status == 1){
        std::cout << "Wrong paths!";
    }
    else{
        std::cout << "Files copied successfully." << std::endl;
    }
}

void ConsoleLib::add(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path source;
    Path dest;
    if(arguments.size() != 3){
        // Nesprávný počet argumentů pro add
        std::cout << "Incorrect number of arguments for add." << std::endl;
        return;
    }
    else{
        source = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
        dest = Path(this->pathToVector(arguments[2]), arguments[2][0] == '/');
    }
    int status = this->fileSystem.add(source, dest);
    
    if(status == 1){
        std::cout << "Wrong path." << std::endl;
        return;
    }
    else{
        std::cout << "File added successfully." << std::endl;
    }
}

void ConsoleLib::outCopy(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path path;
    if(arguments.size() != 3){
        // Nesprávný počet argumentů pro outCopy
        std::cout << "Incorrect number of arguments for outCopy: Invalid number of arguments." << std::endl;
        return;
    }
    else{
        path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    }   
    int status = this->fileSystem.outCopy(path, std::string(arguments[2]));
    
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

void ConsoleLib::catFile(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    if(arguments.size() != 2){
        // Nesprávný počet argumentů pro čtení souboru
        std::cout << "Unable to read file: Invalid number of arguments." << std::endl;
        return;
    }
    
    Path path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    int status = this->fileSystem.catFile(path);
    if(status == 1){
        std::cout << "Unable to read file: Wrong path." << std::endl;
    }
    else{
        std::cout << "File read successfully." << std::endl;
    }
}

void ConsoleLib::changeDir(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    if(arguments.size() != 2){
        // Nesprávný počet argumentů pro změnu adresáře
        std::cout << "Unable to change directory: Invalid number of arguments." << std::endl;
        return;
    }
    
    Path path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    int status = this->fileSystem.changeDir(path);
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

void ConsoleLib::copy(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path source;
    Path dest;
    if(arguments.size() != 3){
        // Nesprávný počet argumentů pro kopírování souboru
        std::cout << "Unable to copy file: Wrong number of arguments." << std::endl;
        return;
    }
    else{
        source = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
        dest = Path(this->pathToVector(arguments[2]), arguments[2][0] == '/');
    }
    int status = this->fileSystem.copy(source, dest, false);

    if(status == 2){
        std::cout << "Unable to copy file: Wrong source path." << std::endl;
    }
    else if(status == 3){
        std::cout << "Unable to copy file: Wrong destination path." << std::endl;
    }
    else if(status == 4){
        std::cout << "Unable to copy file: Source is not a regular file." << std::endl;
    }
    else if(status == 5){
        std::cout << "Unable to copy file: Destination is not a regular file." << std::endl;
    }
    else{
        std::cout << "File copied successfully." << std::endl;
    }
}


void ConsoleLib::move(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    Path source;
    Path dest;
    if(arguments.size() != 3){
        // Nesprávný počet argumentů pro kopírování souboru
        std::cout << "Unable to copy file: Wrong number of arguments." << std::endl;
        return;
    }
    else{
        source = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
        dest = Path(this->pathToVector(arguments[2]), arguments[2][0] == '/');
    }
    int status = this->fileSystem.copy(source, dest, true);
    
    if(status == 2){
        std::cout << "Unable to copy file: Wrong source path." << std::endl;
    }
    else if(status == 3){
        std::cout << "Unable to copy file: Wrong destination path." << std::endl;
    }
    else if(status == 4){
        std::cout << "Unable to copy file: Source is not a regular file." << std::endl;
    }
    else if(status == 5){
        std::cout << "Unable to copy file: Destination is not a regular file." << std::endl;
    }
    else{
        std::cout << "File copied successfully." << std::endl;
    }
}

void ConsoleLib::statfs(){
    FSStats st = this->fileSystem.getStats();
    if (st.total_size == 0) {
        // Nesprávný počet argumentů pro získání statistik souborového systému
        std::cout << "Filesystem not attached or empty." << std::endl;
        return;
    }

    // Formátovaný výstup statistik souborového systému do human readable formátu
    auto human = [](int64_t bytes) {
        const char* units[] = {"B","KB","MB","GB","TB"};
        double v = (double)bytes;
        int u = 0;
        while (v >= 1024.0 && u < 4) { v /= 1024.0; ++u; }
        char buf[64];
        snprintf(buf, sizeof(buf), "%.2f %s", v, units[u]);
        return std::string(buf);
    };

    // Výpis statistik
    std::cout << "Filesystem statistics:" << std::endl;
    std::cout << "  Total size:    " << st.total_size << " bytes (" << human(st.total_size) << ")" << std::endl;
    std::cout << "  Cluster size:  " << st.cluster_size << " bytes" << std::endl;
    std::cout << "  Total clusters:" << st.total_clusters << std::endl;
    std::cout << "  Used clusters: " << st.used_clusters << " (" << human((int64_t)st.used_clusters * st.cluster_size) << ")" << std::endl;
    std::cout << "  Free clusters: " << st.free_clusters << " (" << human((int64_t)st.free_clusters * st.cluster_size) << ")" << std::endl;
    std::cout << "  Total inodes:  " << st.total_inodes << std::endl;
    std::cout << "  Used inodes:   " << st.used_inodes << std::endl;
    std::cout << "  Free inodes:   " << st.free_inodes << std::endl;
    std::cout << "  Directories:   " << st.directory_count << std::endl;
}

void ConsoleLib::load(std::string command) {
    std::vector<std::string> arguments = this->parseCommand(command);
    if (arguments.size() != 2) {
        // Nesprávný počet argumentů pro načtení souboru
        std::cout << "Incorrect number of arguments for load." << std::endl;
        return;
    }

    std::string filename = arguments[1];
    std::ifstream script(filename);
    if (!script) {
        // Soubor nebyl nalezen
        std::cout << "File not found." << std::endl;
        return;
    }

    std::string line;
    while (std::getline(script, line)) {
        // odstranění bílých znaků na začátku a konci řádku
        auto l = line;
        while (!l.empty() && isspace((unsigned char)l.front())) l.erase(l.begin());
        while (!l.empty() && isspace((unsigned char)l.back())) l.pop_back();
        if (l.empty()) continue;

        // naparsování příkazu a jeho argumentů
        std::vector<std::string> args = this->parseCommand(l);
        if (args.empty()) continue;

        // zpracování příkazu
        std::string cmd = args[0];
        if (cmd == "format") {
            // přeskočit formátování v rámci skriptu
            std::cout << "Skipping 'format' in script." << std::endl;
        }

        // další příkazy
        else if (cmd == "mkdir") this->makeDir(l);
        else if (cmd == "cd") this->changeDir(l);
        else if (cmd == "ls") this->listDir(l);
        else if (cmd == "incp") this->inCopy(l);
        else if (cmd == "touch") this->makeFile(l);
        else if (cmd == "cat") this->catFile(l);
        else if (cmd == "outcp") this->outCopy(l);
        else if (cmd == "rmdir") this->rmDir(l);
        else if (cmd == "pwd") std::cout << "Path: " << pathToString(this->fileSystem.currentPath) << std::endl;
        else if (cmd == "cp") this->copy(l);
        else if (cmd == "rm") this->remove(l);
        else if (cmd == "mv") this->move(l);
        else if (cmd == "xcp") this->xcopy(l);
        else if (cmd == "exit") this->exit();
        else if (cmd == "add") this->add(l);
        else if (cmd == "statfs") {

            // Formátovaný výstup statistik souborového systému do human readable formátu
            std::vector<std::string> dummy{"statfs"};
            FSStats st = this->fileSystem.getStats();
            // Pokud není souborový systém připojen, vypiš hlášení
            if (st.total_size == 0) std::cout << "Filesystem not attached or empty." << std::endl;
            else {
                std::cout << "Total size: " << st.total_size << std::endl;
            }
        }
        // Rekurzivní načtení jiného skriptu
        else if (cmd == "load") {
            this->load(l);
        }
        else {
            std::cout << "Unknown command in script: " << cmd << std::endl;
        }
    }
    // Úspěšné načtení skriptu
    std::cout << "File loaded." << std::endl;
    return;
}

void ConsoleLib::info(std::string command){
    std::vector<std::string> arguments = this->parseCommand(command);
    if(arguments.size() != 2){
        // Nesprávný počet argumentů pro získání informací o souboru nebo adresáři
        std::cout << "Unable to get info: Incorrect number of arguments." << std::endl;
        return;
    }
    
    Path path = Path(this->pathToVector(arguments[1]), arguments[1][0] == '/');
    int status = this->fileSystem.info(path);
    if(status == 1){
        std::cout << "Unable to get info: Wrong path." << std::endl;
    }
}


std::vector<std::string> ConsoleLib::parseCommand(std::string command){
    std::stringstream ss(command);
    std::string word;
    std::vector<std::string> parts;

    // Rozdělení příkazu na jednotlivé části podle mezer
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

    // Rozdělení cesty na jednotlivé části podle '/'
    while(std::getline(ss, token, delimeter)){
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}
std::string ConsoleLib::pathToString(const std::vector<std::string>& path){
    std::string result;
    // Sestavení cesty zpět na string
    for(const auto& part : path){
        result += "/" + part;
    }
    return result.empty() ? "/" : result;
}

ConsoleLib::ConsoleLib() {
}

ConsoleLib::~ConsoleLib() {
}