#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <vector>
#include <utility>
#include <cstdio>
#include <cstring>
#include <fstream>

#include <dos.h>
#include <direct.h>

std::pair<int, char**> extractArg() {
    unsigned char far* psp = (unsigned char far *)MK_FP(_psp, 0x80);
    char buffer[256];
    int length = *psp;
    psp++;

    for (int i = 0; i < length; i++) {
        buffer[i] = psp[i];
    }
    buffer[length] = '\0';
    std::vector<char*> args;

    char* token = std::strtok(buffer, " ");
    while (token != NULL) {
        args.push_back(token);
        token = std::strtok(NULL, " ");
    }

    char** argv = new char*[args.size() + 1];

    for (size_t j = 0; j < args.size(); ++j) {
        argv[j] = args[j];
    }
    argv[args.size()] = NULL;
    return std::make_pair(static_cast<int>(args.size()), argv);
}

bool interactive = false;
bool verbose = false;
bool force = false;
bool recursive = false;
bool shred = false;

bool confirmDeletion(const char* filename) {
    if (!interactive) return true;

    std::cout << "rm: remove file '" << filename << "'? (y/n): ";
    char response;
    std::cin >> response;
    return response == 'y' || response == 'Y';
}

bool confirmDirectory(const char* dirname) {
    if (!interactive) return true;

    std::cout << "rm: descend into directory '" << dirname << "'? (y/n): ";
    char response;
    std::cin >> response;
    return response == 'y' || response == 'Y';
}

int shredFile(const char* filename) {
    struct find_t fileInfo;

    if (_dos_findfirst(filename, _A_NORMAL, &fileInfo) != 0 && !force) {
        std::cerr << "shred: cannot shred '" << filename << "': No such file or directory\n";
        return 1;
    }

    if (!force && !confirmDeletion(filename)) {
        std::cout << "shred: not shredding '" << filename << "'\n";
        return 0;
    }

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "shred: cannot open '" << filename << "' for shredding\n";
        return 1;
    }

    std::streamsize size = file.tellg();
    file.close();

    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "shred: cannot open '" << filename << "' for writing\n";
        return 1;
    }

    char zero = 0;
    for (std::streamsize i = 0; i < size; i++) {
        outFile.write(&zero, sizeof(char));
    }
    outFile.close();

    if (std::remove(filename) == 0) {
        if (verbose) {
            std::cout << "shredded and removed '" << filename << "'\n";
        }
    } else if (!force) {
        std::cerr << "shred: cannot remove '" << filename << "'\n";
        return 1;
    }

    return 0;
}

int rmFile(const char* filename) {
    struct find_t fileInfo;

    if (_dos_findfirst(filename, _A_NORMAL, &fileInfo) != 0 && !force) {
        std::cerr << "rm: cannot remove '" << filename << "': No such file or directory\n";
        return 1;
    }

    if (!force && !confirmDeletion(filename)) {
        std::cout << "rm: not removing '" << filename << "'\n";
        return 0;
    }

    if (std::remove(filename) == 0) {
        if (verbose) {
            std::cout << "removed '" << filename << "'\n";
        }
    } else if (!force) {
        std::cerr << "rm: cannot remove '" << filename << "'\n";
        return 1;
    }

    return 0;
}

int rmDir(const char* dirname) {
    if (!confirmDirectory(dirname)) return 0;

    struct find_t fileInfo;
    char searchPattern[128];
    std::sprintf(searchPattern, "%s\\*.*", dirname);

    int done = _dos_findfirst(searchPattern, _A_SUBDIR | _A_ARCH, &fileInfo);

    while (!done) {
        if (std::strcmp(fileInfo.name, ".") != 0 && std::strcmp(fileInfo.name, "..") != 0) {
            char fullPath[256];
            std::sprintf(fullPath, "%s\\%s", dirname, fileInfo.name);

            if (fileInfo.attrib & _A_SUBDIR) {
                if (recursive) {
                    rmDir(fullPath);
                } else {
                    std::cerr << "rm: cannot remove '" << fullPath << "': Is a directory\n";
                }
            } else {
                rmFile(fullPath);
            }
        }
        done = _dos_findnext(&fileInfo);
    }

    if (_dos_findfirst(dirname, _A_SUBDIR, &fileInfo) == 0) {
        if (rmdir(dirname) == 0) {
            if (verbose) {
                std::cout << "removed directory '" << dirname << "'\n";
            }
        } else if (!force) {
            std::cerr << "rm: failed to remove directory '" << dirname << "'\n";
        }
    } else if (!force) {
        std::cerr << "rm: cannot find directory '" << dirname << "'\n";
    }

    return 0;
}

int main() {
    std::pair<int, char**> argz = extractArg();
    int argc = argz.first;
    char** argv = argz.second;
    

    if (argc < 1) {
        std::cerr << "rm: missing operand\n"
                  << "Try \'rm --help\' for more information.\n";
        return 1;
    }


    if (std::strcmp(argv[0], "--help") == 0) {
        std::cout << "Usage: rm [OPTION]... [FILE]...\n"
                  << "Remove the FILE(s).\n\n"
                  << "  -i, --interactive    Prompt before every removal\n"
                  << "  -f, --force          Ignore nonexistent files, never prompt\n"
                  << "  -v, --verbose        Explain what is being done\n"
                  << "  -r, --recursive      Remove directories and their contents recursively\n\n"
				  << "  --shred              Writes 0's to the file before removing it, ensuring\n"
				  << "                       that the file is unrecoverable\n"
				  << "  --help               Display this help and exit\n"
                  << "\n"
				  << "Files deleted with rm might be recoverable by reading and parsing raw filesystem"
                  << "data, as by default, rm only removes entries from the file allocation table.\n"
				  << "For assurance that files are truly unrecoverable, consider using --shred option."
				  << "\nLAOMB DOS-tools (https://github.com/laomb/dos-tools)\n"
                  << "Report bugs to <laomb@tillia.cz>.\n";
    }

    int startIndex = 0;
    while (startIndex < argc && argv[startIndex][0] == '-') {
        if (std::strcmp(argv[startIndex], "--shred") == 0) {
            shred = true;
        } else {
            for (int j = 1; argv[startIndex][j] != '\0'; ++j) {
                switch (argv[startIndex][j]) {
                    case 'v':
                        verbose = true;
                        break;
                    case 'i':
                        interactive = true;
                        break;
                    case 'f':
                        force = true;
                        break;
                    case 'r':
                        recursive = true;
                        break;
                    default:
                        std::cerr << "rm: invalid option -- '" << argv[startIndex][j] << "'\n";
                        return 1;
                }
            }
        }
        startIndex++;
    }

    for (int i = startIndex; i < argc; ++i) {
        struct find_t fileInfo;
        if (_dos_findfirst(argv[i], _A_NORMAL, &fileInfo) == 0) {
            if (shred) {
                shredFile(argv[i]);
            } else {
                rmFile(argv[i]);
            }
        } else if (_dos_findfirst(argv[i], _A_SUBDIR, &fileInfo) == 0) {
            char searchPattern[128];
            std::sprintf(searchPattern, "%s\\*.*", argv[i]);
            if (_dos_findfirst(searchPattern, _A_ARCH | _A_SUBDIR, &fileInfo) == 0) {
                bool isEmpty = true;
                while (_dos_findnext(&fileInfo) == 0) {
                    if (std::strcmp(fileInfo.name, ".") != 0 && std::strcmp(fileInfo.name, "..") != 0) {
                        isEmpty = false;
                        break;
                    }
                }

                if (isEmpty) {
                    if (rmdir(argv[i]) == 0) {
                        if (verbose) {
                            std::cout << "removed empty directory '" << argv[i] << "'\n";
                        }
                    } else if (!force) {
                        std::cerr << "rm: failed to remove empty directory '" << argv[i] << "'\n";
                    }
                } else if (recursive) {
                    rmDir(argv[i]);
                } else {
                    std::cerr << "rm: cannot remove '" << argv[i] << "': Directory not empty\n";
                }
            } else if (!force) {
                std::cerr << "rm: cannot remove '" << argv[i] << "': Directory not found\n";
            }
        } else {
            if (!force) {
                std::cerr << "rm: cannot remove '" << argv[i] << "': No such file or directory\n";
            }
        }
    }

    delete[] argv;
    return 0;
}