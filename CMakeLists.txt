cmake_minimum_required(VERSION 3.10)

# Nastavení názvu projektu
project(ZOS-SP)

# Nastavení C++ standardu
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Nastavení výstupního adresáře pro binární soubory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Přidání zdrojových souborů
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/FS.cpp
    src/superblock.cpp
    src/pseudo_inode.cpp
    src/ConsoleLib.cpp
    src/dirInfo.cpp
    src/Path.cpp
)

# Volitelně: povolit varování při kompilaci
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
