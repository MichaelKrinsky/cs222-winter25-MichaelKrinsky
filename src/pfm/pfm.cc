#include <cstdio>
#include <iostream>
#include "src/include/pfm.h"

#include <unistd.h>

namespace PeterDB {
    PagedFileManager &PagedFileManager::instance() {
        static PagedFileManager _pf_manager = PagedFileManager();
        return _pf_manager;
    }

    PagedFileManager::PagedFileManager() = default;

    PagedFileManager::~PagedFileManager() = default;

    PagedFileManager::PagedFileManager(const PagedFileManager &) = default;

    PagedFileManager &PagedFileManager::operator=(const PagedFileManager &) = default;

    RC PagedFileManager::createFile(const std::string &fileName) {
        std::cout << "create file";
        FILE* fileCheck = fopen(fileName.c_str(), "r");
        if (fileCheck != nullptr) {
            fclose(fileCheck);  // Close the file if it exists
            return 1;
        }

        FILE* file = fopen(fileName.c_str(), "w");
        if (file == nullptr) {
            return -1;  // File creation failed
        }

        // Write empty page
        char emptyPage[4096] = {0};
        size_t written = fwrite(emptyPage, 1, 4096, file);
        if (written != 4096) {
            std::cerr << "Failed to write 4096 bytes to the file" << std::endl;
            return -2;
        }

        // Write page data
        fseek(file, 0, SEEK_SET);
        typedef int PageData[4];
        PageData pageData = {0, 0, 0,0};
        written = fwrite(pageData, sizeof(int), 4, file);
        if (written != 4) {
            std::cerr << "Failed to write to the file" << std::endl;
            return -3;
        }
        fclose(file);  // Close the file after creation
        return 0;
    }

    RC PagedFileManager::destroyFile(const std::string &fileName) {
        if (remove(fileName.c_str()) == 0) {
            return 0;
        }
        return 1;
    }

    RC PagedFileManager::openFile(const std::string &fileName, FileHandle &fileHandle) {
        std::cout << "open file" << std::endl;
        // Check if the file exists
        FILE* file = fopen(fileName.c_str(), "r+");
        if (file == nullptr) {
            return 1;
        }
        // Check if fileHandle already has a file
        if (fileHandle.file != nullptr) {
            return 2;
        }

        // Read the first counters from the start of the file
        fseek(file, 0, SEEK_SET);  // Move file pointer to the beginning
        int counters[4] = {0};   // Buffer to store the integers
        size_t counterSize = fread(counters, sizeof(int), 4, file);

        if (counterSize != 4) {
            std::cerr << "Failed to read the counters from the file" << std::endl;
            return 3;
        }
        fileHandle.file = file;
        fileHandle.readPageCounter = counters[0];
        fileHandle.writePageCounter = counters[1];
        fileHandle.appendPageCounter = counters[2];
        fileHandle.numPages = counters[3];
        std::cout << "Opening file : Counters:" << counters[0] << " " << counters[1] << " " << counters[2] << " " << counters[3] << std::endl;
        // if ( fileHandle.numPages > 5000 ) {
        //     fileHandle.numPages = 0;
        // }
        return 0;
    }

    RC PagedFileManager::closeFile(FileHandle &fileHandle) {
        std::cout << "close file" << std::endl;
        // If fileHandle has a file
        if (fileHandle.file == nullptr){
            return 1;
        }
        // Write counters
        int counters[4];
        counters[0] = fileHandle.readPageCounter;
        counters[1] = fileHandle.writePageCounter;
        counters[2] = fileHandle.appendPageCounter;
        counters[3] = fileHandle.numPages;
        fseek(fileHandle.file, 0, SEEK_SET);  // Move file pointer to the beginning
        size_t counterSize = fwrite(counters, sizeof(int), 4, fileHandle.file);
        if (counterSize != 4) {
            std::cerr << "Failed to write counters to the file" << std::endl;
            return -3;
        }
        fclose(fileHandle.file);
        return 0;
    }

    FileHandle::FileHandle() {
        readPageCounter = 0;
        writePageCounter = 0;
        appendPageCounter = 0;
        numPages = 0;
    }

    FileHandle::~FileHandle() = default;

    RC FileHandle::readPage(PageNum pageNum, void *data) {
        pageNum = pageNum + 1; // Ignore first page
        // If file exists
        if (!file) {
            std::cerr << "Failed to open file: " << std::endl;
            return 1;
        }
        fseek(file, 0, SEEK_END);
        std::size_t offset = pageNum * PAGE_SIZE;  // Calculate the offset
        if (fseek(file, offset, SEEK_SET) != 0) {     // Move the file pointer to the page
            // If page doesn't exist
            std::cerr << "Failed to seek to page " << pageNum << std::endl;
            fseek(file, 0, SEEK_SET); // Reset cursor to start of file
            return 2;
        }
        std::size_t bytesRead = fread(data, 1, PAGE_SIZE, file);  // Read one page
        if (bytesRead > 0) {
        } else {
            std::cerr << "Failed to read data from page " << pageNum << std::endl;
            fseek(file, 0, SEEK_SET); // Reset cursor to start of file
            return 1;
        }
        readPageCounter = readPageCounter + 1;
        fseek(file, 0, SEEK_SET); // Reset cursor to start of file
        return 0;
    }

    RC FileHandle::writePage(PageNum pageNum, const void *data) {
        std::cout << "write file" << std::endl;
        pageNum = pageNum + 1; // Ignore first page
        // If file exists
        if (!file) {
            std::cerr << "Failed to open file: " << std::endl;
            return 1;
        }
        std::size_t offset = pageNum * PAGE_SIZE;  // Calculate the offset
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);  // Reset cursor to start of file
        // Check if pageNum is within the file
        if (fileSize < offset + PAGE_SIZE) {
            std::cout << "File size " << fileSize << " too small" << std::endl;
            return 1;
        }

        if (fseek(file, offset, SEEK_SET) != 0) {
            // Move the file pointer to the page
            // If page doesn't exist
            std::cerr << "Failed to seek to page " << pageNum << std::endl;
            fseek(file, 0, SEEK_SET); // Reset cursor to start of file
            return 2;
        }
        size_t written = fwrite(data, 1, PAGE_SIZE, file); // 1 is the size of a byte
        if (written != PAGE_SIZE) {
            perror("Error writing to file");
        }
        writePageCounter = writePageCounter + 1;
        return 0;
    }

    RC FileHandle::appendPage(const void *data) {
        std::cout << "append file " << std::endl;
        if (!file) {
            std::cerr << "Failed to open file: " << std::endl;
            return 1;
        }
        // Move the file pointer to the end
        if (fseek(file, 0, SEEK_END) != 0) {
            std::cerr << "Failed to seek to end of file" << std::endl;
            return 2;
        }
        // Write the 4096 bytes of data to the end
        size_t written = fwrite(data, 1, PAGE_SIZE, file);
        if (written != PAGE_SIZE) {
            std::cerr << "Error writing " << written << std::endl;
            return 3;
        }
        appendPageCounter = appendPageCounter + 1;
        numPages = numPages + 1;
        fseek(file, 0, SEEK_SET); // Reset cursor to start of file
        return 0;
    }

    unsigned FileHandle::getNumberOfPages() {
        return numPages;
    }

    RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount) {
        readPageCount = readPageCounter;
        writePageCount = writePageCounter;
        appendPageCount = appendPageCounter;
        std::cout << "Counter values: " << readPageCounter << " " << writePageCounter << " " << appendPageCounter << std::endl;
        return 0;
    }

} // namespace PeterDB