; // MyEventProvider.mc 

; // This is the header section.


SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )


FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )

LanguageNames=(English=0x409:MSG00409)


; // The following are the categories of events.

MessageIdTypedef=WORD

MessageId=0x1
SymbolicName=WINDOWS_EVENT_ERROR_CATEGORY
Language=English
Error Events
.

MessageId=0x2
SymbolicName=WINDOWS_EVENT_INFORMATION_CATEGORY
Language=English
Informational Events
.


; // The following are the message definitions.

MessageIdTypedef=DWORD

MessageId=0x101
Severity=Error
Facility=System
SymbolicName=WINDOWS_EVENT_MSG_ERROR
Language=English
Git Sync'd experienced an error. The following error message was logged:%n%1
.

MessageId=0x102
Severity=Informational
Facility=System
SymbolicName=WINDOWS_EVENT_MSG_INFORMATION
Language=English
%1
.
