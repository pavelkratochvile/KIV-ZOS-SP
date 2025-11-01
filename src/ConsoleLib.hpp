#pragma once
#include"FS.hpp"
#include<iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

class ConsoleLib{
    public:
        // Konstruktor a destruktor
        ConsoleLib();
        ~ConsoleLib();

        // Instance souborového systému
        FS fileSystem;

        /* -------------- Příkazy -------------- */

        // Inicializace souborového systému
        void InitFS(int argc, char** argv, std::string command);

        // Příkaz pro vytvoření adresáře
        void makeDir(std::string command);
        
        // Příkaz pro vytvoření souboru
        void makeFile(std::string command);
        
        // Příkaz pro změnu adresáře
        void changeDir(std::string command);
        
        // Příkaz pro výpis obsahu adresáře
        void listDir(std::string command);
        
        // Příkaz pro kopírování souboru z reálného souborového systému do našeho souborového systému
        void inCopy(std::string command);
        
        // Příkaz pro zkopírování souboru ven ze souborového systému do reálného souborového systému
        void outCopy(std::string command);
        
        // Příkaz pro zobrazení obsahu souboru
        void catFile(std::string command);
        
        // Příkaz pro odstranění prázdného adresáře
        void rmDir(std::string command);
        
        // Příkaz pro kopírování souboru nebo adresáře
        void copy(std::string command);
        
        // Příkaz pro odstranění souboru
        void remove(std::string command);
        
        // Příkaz pro přesunutí souboru
        void move(std::string command);
        
        // Příkaz pro načtení a vykonání příkazů ze souboru
        void load(std::string command);
        
        // Příkaz pro zobrazení informací o souboru nebo adresáři
        void info(std::string command);
        
        // Příkaz pro kopírování dvou souborů do jednoho
        void xcopy(std::string command);
        
        // Příkaz pro přídíní obsahu souboru na konec jiného souboru
        void add(std::string command);
        
        // Příkaz pro zobrazení statistik souborového systému
        void statfs();
        
        // Příkaz pro ukončení konzole
        void exit();

        /* -------------- Pomocné funkce -------------- */

        // Formátování souborového systému
        int format(std::string fileName, int size);
        
        // Připojení souborového systému k existujícímu souboru
        int attach(std::string fileName);
        
        // Převod cesty ze vektoru řetězců na řetězec
        std::string pathToString(const std::vector<std::string>& path);
        
        // Převod cesty ze řetězce na vektor řetězců
        std::vector<std::string> pathToVector(std::string path);

        // Čtení příkazů z konzole (while loop)
        void readConsole(int argc, char** argv);
        
        // Parsování příkazu na jednotlivé argumenty
        std::vector<std::string> parseCommand(std::string command);
};