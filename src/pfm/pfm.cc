#include <cstdio>
#include <iostream>
#include "src/include/pfm.h"

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
        FILE* fileCheck = fopen(fileName.c_str(), "r");
        if (fileCheck != nullptr) {
            fclose(fileCheck);  // Close the file if it exists
            return 1;
        }
        FILE* file = fopen(fileName.c_str(), "w");
        if (file == nullptr) {
            return -1;  // File creation failed
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
        // Check if the file exists
        FILE* file = fopen(fileName.c_str(), "r");
        if (file == nullptr) {
            return -1;
        }
        // Check if fileHandle already has a file
        if (fileHandle.file == nullptr) {
            fileHandle.file = file;
            return 0;
        }
        // fileHandle already has a file
        return 1;
    }

    RC PagedFileManager::closeFile(FileHandle &fileHandle) {
        // If fileHandle has a file
        if (fileHandle.file == nullptr){
            return 1;
        }
        fclose(fileHandle.file);
        // Flush all pages to disk TODO???
        return 0;
    }

    FileHandle::FileHandle() {
        readPageCounter = 0;
        writePageCounter = 0;
        appendPageCounter = 0;
    }

    FileHandle::~FileHandle() = default;

    RC FileHandle::readPage(PageNum pageNum, void *data) {
        return -1;
    }

    RC FileHandle::writePage(PageNum pageNum, const void *data) {
        return -1;
    }

    RC FileHandle::appendPage(const void *data) {
        return -1;
    }

    unsigned FileHandle::getNumberOfPages() {
        return -1;
    }

    RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount) {
        return -1;
    }

} // namespace PeterDB