## Project 1 Report


### 1. Basic information
 - Team #: 10    
 - Github Repo Link: https://github.com/MichaelKrinsky/cs222-winter25-MichaelKrinsky
 - Student 1 UCI NetID: makrinsk
 - Student 1 Name: Michael Krinsky
 - Student 2 UCI NetID (if applicable): NA
 - Student 2 Name (if applicable): NA


### 2. Internal Record Format
- Show your record format design.
  First 4 bytes: Number of columns in the record
  Next ceil(num_col / 8) bytes: Null bits
  Next num_col * 4 bytes are integer values, where the nth integer represents the offset from the start of the page to the beginning of the nth field. 
  Rest of bytes are copied directly from the input record


- Describe how you store a null field.
  Null fields are not stored but instead are identified using the null bits at the beginning of each record. the pointer that points to a null value still exists, but points to where it would be if it had started.


- Describe how you store a VarChar field.
  Varchars are stored using 4 + N bytes. the first 4 bytes is an integer describing how long the varchar is, and the next N bytes are the characters in the varchar.


- Describe how your record design satisfies O(1) field access.
  To find the specified field, use the integer offsets as described in the record format section and add the offset to the pointer to the start of the page. Then interpret the value at that location based off of the type dictated in the field descriptor.


### 3. Page Format
- Show your page format design.
  The 0th index is the start of the records field. Records are placed sequentially directly after one another. At the end of the file, the last 8 bytes are reserved for 2 integers corresponding with the number of records in the page and the number of free bytes in the page. Before those two integers is the start of the slot directory which is explained below.


- Explain your slot directory design if applicable.
  The slot directory begins at directly before the last 2 integers in the file. Each set of 8 bytes before the last 2 integers corresponds to a single slot directory index, where the first 4 bytes are the offset for the beginning of the record from the start of the page and the next four bytes are for the total size of the record. When a new record is added, the 8 bytes to the left of the slot directory are taken for the new slot.



### 4. Page Management
- Show your algorithm of finding next available-space page when inserting a record.
    for each page in the file:
       check the page's free size int at the end of the page
    if there is no page with free space:
       create a new page and append it to the file
          


- How many hidden pages are utilized in your design?
    1 hidden page is utilized in the design to store the three counters as well as a variable for storing the total number of pages in the file.


- Show your hidden page(s) format design if applicable
  First 4 bytes: readPageCounter
  Next 4 bytes: writePageCounter
  Next 4 bytes: appendPageCounter
  Next 4 Bytes: numPageCounter


### 5. Implementation Detail
- Other implementation details goes here.



### 6. Member contribution (for team of two)
- Explain how you distribute the workload in team.



### 7. Other (optional)
- Freely use this section to tell us about things that are related to the project 1, but not related to the other sections (optional)



- Feedback on the project to help improve the project. (optional)
  Update the gtests to delete the files that the database uses because otherwise it can start up new gtests with file contents that break the code automatically.