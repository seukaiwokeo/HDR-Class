#include "hdr.h"

static void printUsage() {
    std::cout << "Usage: program [-u|-p] folder_name" << std::endl;
    std::cout << "\t-u: Unpack files from specified folder" << std::endl;
    std::cout << "\t-p: Pack files into specified folder" << std::endl;
    std::cout << "Example: hdr_class.exe -u ui" << std::endl;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    CHDRSystem hdr;

    if (argc != 3) {
        printUsage();
        return 1;
    }

    std::string option = argv[1];
    std::string folderName = argv[2];

    if (option == "-u") {
        std::cout << folderName << " Unpack Started." << std::endl;
        hdr.Unpack(folderName.c_str());
        std::cout << folderName << " Unpack Ended." << std::endl;
    }
    else if (option == "-p") {
        std::cout << folderName << " Pack Started." << std::endl;
        hdr.Pack(folderName.c_str());
        std::cout << folderName << " Pack Ended." << std::endl;
    }
    else {
        printUsage();
    }

    return 0;
}