#include "../core/cleaner/cleaner.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <path to binary>" << std::endl;
        return 1;
    }

    MappedFile target;
    if (!utils::openFileAndMap(argv[1], &target)) return 1;

    std::cout << "[+] File mapped at: " << target.pBase << " (Size: " << target.fileSize << " bytes)" << std::endl;

    cleaner::cleanPE(target.pBase);

    utils::closeMappedFile(&target);
    std::cout << "[+] File unmapped and saved." << std::endl;

    std::cin.get();
    return 0;
}