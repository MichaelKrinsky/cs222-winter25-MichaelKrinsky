#include "src/include/rbfm.h"
#include <cmath>
#include <iostream>
#include <ostream>
#include <glog/logging.h>


namespace PeterDB {
    RecordBasedFileManager &RecordBasedFileManager::instance() {
        static RecordBasedFileManager _rbf_manager = RecordBasedFileManager();
        return _rbf_manager;
    }

    RecordBasedFileManager::RecordBasedFileManager() = default;

    RecordBasedFileManager::~RecordBasedFileManager() = default;

    RecordBasedFileManager::RecordBasedFileManager(const RecordBasedFileManager &) = default;

    RecordBasedFileManager &RecordBasedFileManager::operator=(const RecordBasedFileManager &) = default;
    void printBytes(int number,const void* data) {
        const unsigned char *byteData = reinterpret_cast<const unsigned char*>(data);  // Use reinterpret_cast
        for (int i = 0; i < number; i++) {
            printf("%02X ", byteData[i]);  // %02X ensures two digits with leading zeros
        }
        std::cout << std::endl;
    }
    RC RecordBasedFileManager::createFile(const std::string &fileName) {
        PagedFileManager &pfm = PagedFileManager::instance();
        return pfm.createFile(fileName);
    }

    RC RecordBasedFileManager::destroyFile(const std::string &fileName) {
        PagedFileManager &pfm = PagedFileManager::instance();
        return pfm.destroyFile(fileName);
    }

    RC RecordBasedFileManager::openFile(const std::string &fileName, FileHandle &fileHandle) {
        PagedFileManager &pfm = PagedFileManager::instance();
        return pfm.openFile(fileName, fileHandle);
    }

    RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
        PagedFileManager &pfm = PagedFileManager::instance();
        return pfm.closeFile(fileHandle);
    }

    unsigned RecordBasedFileManager::getTotalBytesNeeded(const std::vector<Attribute> &recordDescriptor) {
        int numFields = recordDescriptor.size();
        int fieldNullBytes = std::ceil(numFields / 8.0);
        int totalBytesNeeded = 12 + fieldNullBytes; // Size of pointer(8) + size of int(4)
        for (int i = 0; i < numFields; i++) {
            totalBytesNeeded += recordDescriptor[i].length;
        }
        return totalBytesNeeded;
    }

    unsigned RecordBasedFileManager::skipCurrentRecord(const void * data, const std::vector<Attribute> &recordDescriptor, unsigned index) {
        // 4 (num cols) +  n /8 (null bits) + (4n - 1) (offsets)
        unsigned indexOfLastField = 4 + std::ceil(recordDescriptor.size() / 8.0) + 4 * (recordDescriptor.size() - 1);
        const unsigned char *byteData = reinterpret_cast<const unsigned char*>(data);  // Use reinterpret_case
        unsigned newIndex;
        if (recordDescriptor[recordDescriptor.size() - 1].type == TypeVarChar) {
            int stringSize = 0;
            std::memcpy(&stringSize, data + indexOfLastField, sizeof(int));
            stringSize += 4;
            newIndex = index + indexOfLastField + stringSize;
        } else {
            newIndex = index + indexOfLastField + recordDescriptor[recordDescriptor.size() - 1].length;
        }
        return newIndex;
    }
    void RecordBasedFileManager::printPage(const void * data) {
        const unsigned char *byteData = reinterpret_cast<const unsigned char*>(data);  // Use reinterpret_case
        for (int i =0; i < PAGE_SIZE; i++) {
            std::cout << byteData[i];
        }
    }
    unsigned convertToNormalData(const char* dbData, const std::vector<Attribute> &recordDescriptor, void* data) {
        std::cout << "Converting to Normal data" << std::endl;
        int numFields = recordDescriptor.size();
        unsigned char *byteData = reinterpret_cast<unsigned char*>(data);
        int dbDataIndex = 4; // Index in bytes of dbData. Skip the numCols
        int baseDataIndex = 0; // Index we're in for the base data.
        // Get null bits
        int numNullBytes = ceil(recordDescriptor.size() / 8.0);
        for (int i = 0; i < numNullBytes; i++) {
            byteData[baseDataIndex] = dbData[dbDataIndex];
            dbDataIndex++;
            baseDataIndex++; // Skip past the null bytes so we can start reading the data
        }

        dbDataIndex += 4 * numFields; // Skip past pointers;
        //Add the rest of the data
        for (int fieldIndex = 0; fieldIndex < numFields; fieldIndex++) {
            // Handle null bit
            size_t byteIndex = fieldIndex / 8;
            size_t bitIndex = fieldIndex % 8;
            unsigned char bit = (byteData[byteIndex] >> bitIndex) & 0x01;
            if (bit) {
                std::cout << "NULL BIT is 1!" << std::endl;
            }

            // Copy all bytes of the current field
            if ( recordDescriptor[fieldIndex].type == TypeVarChar) {
                int stringSize = 0;
                std::memcpy(&stringSize, dbData + dbDataIndex, sizeof(int));
                stringSize += 4;
                std::memcpy(byteData + baseDataIndex, dbData + dbDataIndex, stringSize);
                dbDataIndex += stringSize;
                baseDataIndex += stringSize;
            } else {
                for (int fieldByte = 0; fieldByte < recordDescriptor[fieldIndex].length; fieldByte++) {
                    if (bit != 1) {
                        byteData[baseDataIndex] = dbData[dbDataIndex];
                        dbDataIndex++;
                        baseDataIndex++;
                    }
                }
            }
        }
        std::cout << "Finished Converting to Normal" << std::endl;
        printBytes(PAGE_SIZE,data);
        return baseDataIndex;
    }


    unsigned convertToDbData(const void* data, const std::vector<Attribute> &recordDescriptor, char* dbData) {
        std::cout << "Converting to DB data" << std::endl;
        int dbDataIndex = 0; // Index in bytes of dbData
        int baseDataIndex = 0; // Index we're in for the base data
        printBytes(4098, data);
        const unsigned char *byteData = reinterpret_cast<const unsigned char*>(data);
        std::cout << "Converting to db data" << std::endl;
        //get numCols
        int *intJustForNumColData = reinterpret_cast<int*>(dbData);
        int *intDbData = reinterpret_cast<int*>(dbData + 5); // Shift to ignore the null bit and numCol
        int numCols = recordDescriptor.size();
        intJustForNumColData[0] = numCols;
        dbDataIndex += 4;

        // Get null bits
        int numNullBytes = ceil(recordDescriptor.size() / 8.0);
        for (int i = 0; i < numNullBytes; i++) {
            dbData[dbDataIndex + i] = byteData[i];
            dbDataIndex++;
            baseDataIndex++; // Skip past the null bytes so we can start reading the data
        }

        // Space for pointers
        dbDataIndex += numCols * 4; // Skip past the pointers

        //Put data
        for (int fieldIndex = 0; fieldIndex < numCols; fieldIndex++) {
            size_t byteIndex = fieldIndex / 8;
            size_t bitIndex = fieldIndex % 8;
            unsigned char bit = (byteData[byteIndex] >> bitIndex) & 0x01;
            if (bit) {
                std::cout << "NULL BIT is 1!" << std::endl;
            }
            // Add pointer to start of next Field
            intDbData[fieldIndex] = dbDataIndex;
            // Copy all bytes of the current field

            if ( recordDescriptor[fieldIndex].type == TypeVarChar) {
                int stringSize = 0;
                std::memcpy(&stringSize, byteData + baseDataIndex, sizeof(int));
                stringSize += 4; // Add 4 bytes for storing string size
                std::memcpy( dbData + dbDataIndex, byteData + baseDataIndex, stringSize);
                dbDataIndex += stringSize;
                baseDataIndex += stringSize;
            } else {
                for (int fieldByte = 0; fieldByte < recordDescriptor[fieldIndex].length; fieldByte++) {
                    if (bit != 1) {
                        dbData[dbDataIndex] = byteData[baseDataIndex];
                        dbDataIndex++;
                        baseDataIndex++;
                    }
                }
            }

        }
        return dbDataIndex;
    }

    void RecordBasedFileManager::insertIntoPage(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                            const void *data, RID &rid) {
        // Get page data
        char newPage[PAGE_SIZE] = {0};

        // Convert data to what will go into db
        char dbData[PAGE_SIZE] = {0};
        int dbDataSize = convertToDbData(data, recordDescriptor, dbData);
        int totalDataSize = dbDataSize + 4 + 4; // data + slot directory

        // Find suitable page
        int slotDirectoryLocation = PAGE_SIZE - 8; // - 4(num records) - 4(space free)
        int pageNum = -1;
        for (int i = 0; i < fileHandle.getNumberOfPages(); i++) {
            int *intSlotDirectoryData = (int*) (newPage + slotDirectoryLocation); // Oth index is the N Rows spot
            fileHandle.readPage(i, newPage); // check page
            int freeSpaceInPage = intSlotDirectoryData[1];
            if (freeSpaceInPage > totalDataSize) {
                std::cout << "Found free page" << freeSpaceInPage << std::endl;
                pageNum = i;
                break;
            }
        }

        // If no page exists
        if (pageNum == -1) {
            createNewPage(fileHandle);
            pageNum = fileHandle.getNumberOfPages() - 1;
            fileHandle.readPage(pageNum, newPage);
        }
        std::cout << "Page number: " << pageNum << std::endl;
        // Set slot directory index to the first index


        int *intSlotDirectoryData = (int*) (newPage + slotDirectoryLocation); // Oth index is the N Rows spot
        int slotDirectoryIndex = intSlotDirectoryData[0]; // Each slot has 2 ints
        // // Insert Data
        int startingDataIndex = 0;
        if (slotDirectoryIndex != 0) {
            //Grab the offset and length of the previous slot directory
            startingDataIndex = intSlotDirectoryData[-slotDirectoryIndex * 2];
            startingDataIndex += intSlotDirectoryData[-slotDirectoryIndex * 2 + 1];
        }
        std::memcpy(newPage + startingDataIndex, dbData, dbDataSize);
        // // Create the slot directory for the new element

        intSlotDirectoryData[-slotDirectoryIndex * 2 - 2] = startingDataIndex; // Offset
        intSlotDirectoryData[-slotDirectoryIndex * 2 - 1] = dbDataSize; // Length
        intSlotDirectoryData[0] += 1; // Increment numRecords counter
        intSlotDirectoryData[1] -= totalDataSize;

        // Set rid
        rid.pageNum = 0;
        rid.slotNum = slotDirectoryIndex;
        // // Save page
        fileHandle.writePage(pageNum, newPage);

    }
    void RecordBasedFileManager::createNewPage(FileHandle &fileHandle) {
        int numberOfIntsInPage = PAGE_SIZE / 4;
        int freeSpace = PAGE_SIZE - 8;
        int newPage[numberOfIntsInPage] = {0};
        newPage[numberOfIntsInPage - 2] = 0;
        newPage[numberOfIntsInPage - 1] = freeSpace; // 4(numrec) + 4(freeSpace)
        fileHandle.appendPage(newPage);
        // printBytes(4096, newPage);

    }
    unsigned RecordBasedFileManager::getFreeSpaceSize(const void * data) {
        const int *intData = static_cast<const int*>(data);
        size_t totalInts = 4096 / sizeof(int);

        int lastInt = intData[totalInts - 1];
        return static_cast<unsigned>(lastInt);
    }
    unsigned RecordBasedFileManager::getNumRecordsInFile(const void * data) {
        const int *intData = static_cast<const int*>(data);
        size_t totalInts = 4096 / sizeof(int);

        int lastInt = intData[totalInts - 2];
        return static_cast<unsigned>(lastInt);
    }

    RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                            const void *data, RID &rid) {
        std::cout << "Inserting Record" << std::endl;
        insertIntoPage(fileHandle, recordDescriptor,data,rid);
        return 0;
    }

    RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                          const RID &rid, void *data) {
        if (fileHandle.getNumberOfPages() < rid.pageNum) {
            return 2;
        }
        char newPage[PAGE_SIZE] = {0};
        fileHandle.readPage(rid.pageNum, newPage);
        int slotDirectoryLocation = PAGE_SIZE - 8; // - 4(num records) - 4(space free)
        int *intSlotDirectoryData = (int*) (newPage + slotDirectoryLocation); // Oth index is the N Rows spot
        if (intSlotDirectoryData[0] < rid.slotNum) {
            return 3;
        }
        int offset = intSlotDirectoryData[-rid.slotNum * 2 - 2];
        int length = intSlotDirectoryData[-rid.slotNum * 2 - 1];
        std::memcpy(data, newPage + offset,length);
        convertToNormalData((const char*)data,recordDescriptor,data);
        return 0;
    }

    RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                            const RID &rid) {
        return -1;
    }

    RC RecordBasedFileManager::printRecord(const std::vector<Attribute> &recordDescriptor, const void *data,
                                           std::ostream &out) {
        const unsigned char *byteData = reinterpret_cast<const unsigned char*>(data);  // Use reinterpret_cast
        int currentParsingIndex = std::ceil(recordDescriptor.size() / 8.0);
        printBytes(4096, data);
        // Loop thru each attribute
        for (int attributeNum = 0; attributeNum < recordDescriptor.size(); attributeNum++) {
            // Get the NULL bit
            size_t byteIndex = attributeNum / 8;
            size_t bitIndex = attributeNum % 8;
            bool bit = (byteData[byteIndex] >> 7 - bitIndex) & 1;
             out << recordDescriptor[attributeNum].name << ": ";
            if (bit) {
                out << "NULL";
            } else {
                if (recordDescriptor[attributeNum].type == TypeVarChar) {
                    int stringSize = 0;
                    std::memcpy(&stringSize, byteData + currentParsingIndex, sizeof(int));
                    currentParsingIndex += 4;
                    for (int i =0; i<stringSize; i++) {
                        out << byteData[currentParsingIndex + i];
                    }
                    currentParsingIndex += stringSize;
                } else if ( recordDescriptor[attributeNum].type == TypeInt){
                    int result = 0;
                    std::memcpy(&result, byteData + currentParsingIndex, sizeof(int));
                    currentParsingIndex += recordDescriptor[attributeNum].length;
                    out << result;
                } else if ( recordDescriptor[attributeNum].type == TypeReal){
                    float result = 0;
                    std::memcpy(&result, byteData + currentParsingIndex, sizeof(float));
                    currentParsingIndex += recordDescriptor[attributeNum].length;
                    out << result;
                }
            }
            if (attributeNum != recordDescriptor.size() - 1) {
                out << ", ";
            }
        }
        return 1;
    }

    RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                            const void *data, const RID &rid) {
        return -1;
    }

    RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                             const RID &rid, const std::string &attributeName, void *data) {
        return -1;
    }

    RC RecordBasedFileManager::scan(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                    const std::string &conditionAttribute, const CompOp compOp, const void *value,
                                    const std::vector<std::string> &attributeNames,
                                    RBFM_ScanIterator &rbfm_ScanIterator) {
        return -1;
    }

} // namespace PeterDB

