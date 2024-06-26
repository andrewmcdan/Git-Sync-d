 // MyEventProvider.mc 
 // This is the header section.
 // The following are the categories of events.
//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_SYSTEM                  0x0
#define FACILITY_RUNTIME                 0x2
#define FACILITY_STUBS                   0x3
#define FACILITY_IO_ERROR_CODE           0x4


//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: WINDOWS_EVENT_ERROR_CATEGORY
//
// MessageText:
//
// Error Events
//
#define WINDOWS_EVENT_ERROR_CATEGORY     ((WORD)0x00000001L)

//
// MessageId: WINDOWS_EVENT_INFORMATION_CATEGORY
//
// MessageText:
//
// Informational Events
//
#define WINDOWS_EVENT_INFORMATION_CATEGORY ((WORD)0x00000002L)

 // The following are the message definitions.
//
// MessageId: WINDOWS_EVENT_MSG_ERROR
//
// MessageText:
//
// Git Sync'd experienced an error. The following error message was logged:%n%1
//
#define WINDOWS_EVENT_MSG_ERROR          ((DWORD)0xC0000101L)

//
// MessageId: WINDOWS_EVENT_MSG_INFORMATION
//
// MessageText:
//
// %1
//
#define WINDOWS_EVENT_MSG_INFORMATION    ((DWORD)0x40000102L)

