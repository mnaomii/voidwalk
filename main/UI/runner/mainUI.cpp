#include "mainUI.h"
#include "../ui/ui.h"
#include <stdexcept>
#include <iostream>
#include <string>
#include <memory>


 int UIstart(int argc, char** argv, std::string status, std::unique_ptr<Disassembler> disassembler) {
     std::cout << status;
     return 0;
 }