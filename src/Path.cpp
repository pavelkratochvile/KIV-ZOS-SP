#include "Path.hpp"

Path::Path(std::vector<std::string> path, bool isAbsolut){
    this->isAbsolut = isAbsolut;
    this->path = path;
}
Path::Path(){
    this->isAbsolut = false;
    this->path = {};
}