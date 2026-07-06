#include "mainUI.h"
#include "../ui/ui.h"
#include <stdexcept>
#include <iostream>
#include <string>
#include <memory>


 int UIstart(int argc, char** argv, std::string status, std::shared_ptr<Disassembler> disassembler) {
     std::cout << "Analyzing file " << argv[argc - 1];
     std::cout << status;
     std::cout << " Architecture -> " << disassembler->getArchitecture() << "\n\n";
     return 0;
 }
