typedef unsigned char   undefined;

typedef unsigned char    bool;
typedef unsigned char    byte;
typedef unsigned int    dword;
typedef unsigned long long    GUID;
typedef pointer32 ImageBaseOffset32;

typedef unsigned char    uchar;
typedef unsigned int    uint;
typedef unsigned long    ulong;
typedef unsigned char    undefined1;
typedef unsigned short    undefined2;
typedef unsigned int    undefined4;
typedef unsigned long long    undefined5;
typedef unsigned long long    undefined8;
typedef unsigned short    ushort;
typedef unsigned short    wchar16;
typedef short    wchar_t;
typedef unsigned short    word;
typedef struct exception exception, *Pexception;

struct exception { // PlaceHolder Class Structure
};

typedef struct _s_HandlerType _s_HandlerType, *P_s_HandlerType;

typedef struct _s_HandlerType HandlerType;

typedef struct TypeDescriptor TypeDescriptor, *PTypeDescriptor;

typedef int ptrdiff_t;

struct TypeDescriptor {
    void *pVFTable;
    void *spare;
    char name[0];
};

struct _s_HandlerType {
    uint adjectives;
    struct TypeDescriptor *pType;
    ptrdiff_t dispCatchObj;
    void *addressOfHandler;
};

typedef struct _s_UnwindMapEntry _s_UnwindMapEntry, *P_s_UnwindMapEntry;

typedef struct _s_UnwindMapEntry UnwindMapEntry;

typedef int __ehstate_t;

struct _s_UnwindMapEntry {
    __ehstate_t toState;
    void (*action)(void);
};

typedef union IMAGE_RESOURCE_DIRECTORY_ENTRY_DirectoryUnion IMAGE_RESOURCE_DIRECTORY_ENTRY_DirectoryUnion, *PIMAGE_RESOURCE_DIRECTORY_ENTRY_DirectoryUnion;

typedef struct IMAGE_RESOURCE_DIRECTORY_ENTRY_DirectoryStruct IMAGE_RESOURCE_DIRECTORY_ENTRY_DirectoryStruct, *PIMAGE_RESOURCE_DIRECTORY_ENTRY_DirectoryStruct;

struct IMAGE_RESOURCE_DIRECTORY_ENTRY_DirectoryStruct {
    dword OffsetToDirectory:31;
    dword DataIsDirectory:1;
};

union IMAGE_RESOURCE_DIRECTORY_ENTRY_DirectoryUnion {
    dword OffsetToData;
    struct IMAGE_RESOURCE_DIRECTORY_ENTRY_DirectoryStruct IMAGE_RESOURCE_DIRECTORY_ENTRY_DirectoryStruct;
};

typedef struct _s_TryBlockMapEntry _s_TryBlockMapEntry, *P_s_TryBlockMapEntry;

struct _s_TryBlockMapEntry {
    __ehstate_t tryLow;
    __ehstate_t tryHigh;
    __ehstate_t catchHigh;
    int nCatches;
    HandlerType *pHandlerArray;
};

typedef struct _s_TryBlockMapEntry TryBlockMapEntry;

typedef struct CLIENT_ID CLIENT_ID, *PCLIENT_ID;

struct CLIENT_ID {
    void *UniqueProcess;
    void *UniqueThread;
};

typedef struct _s_FuncInfo _s_FuncInfo, *P_s_FuncInfo;

typedef struct _s_FuncInfo FuncInfo;

struct _s_FuncInfo {
    uint magicNumber_and_bbtFlags;
    __ehstate_t maxState;
    UnwindMapEntry *pUnwindMap;
    uint nTryBlocks;
    TryBlockMapEntry *pTryBlockMap;
    uint nIPMapEntries;
    void *pIPToStateMap;
};

typedef struct tagMSG tagMSG, *PtagMSG;

typedef struct tagMSG MSG;

typedef struct HWND__ HWND__, *PHWND__;

typedef struct HWND__ *HWND;

typedef uint UINT;

typedef uint UINT_PTR;

typedef UINT_PTR WPARAM;

typedef long LONG_PTR;

typedef LONG_PTR LPARAM;

typedef ulong DWORD;

typedef struct tagPOINT tagPOINT, *PtagPOINT;

typedef struct tagPOINT POINT;

typedef long LONG;

struct tagPOINT {
    LONG x;
    LONG y;
};

struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
};

struct HWND__ {
    int unused;
};

typedef struct tagWNDCLASSA tagWNDCLASSA, *PtagWNDCLASSA;

typedef struct tagWNDCLASSA WNDCLASSA;

typedef LONG_PTR LRESULT;

typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct HINSTANCE__ HINSTANCE__, *PHINSTANCE__;

typedef struct HINSTANCE__ *HINSTANCE;

typedef struct HICON__ HICON__, *PHICON__;

typedef struct HICON__ *HICON;

typedef HICON HCURSOR;

typedef struct HBRUSH__ HBRUSH__, *PHBRUSH__;

typedef struct HBRUSH__ *HBRUSH;

typedef char CHAR;

typedef CHAR *LPCSTR;

struct HBRUSH__ {
    int unused;
};

struct tagWNDCLASSA {
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
};

struct HICON__ {
    int unused;
};

struct HINSTANCE__ {
    int unused;
};

typedef struct tagMSG *LPMSG;

typedef void (*TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);

typedef struct _cpinfo _cpinfo, *P_cpinfo;

typedef uchar BYTE;

struct _cpinfo {
    UINT MaxCharSize;
    BYTE DefaultChar[2];
    BYTE LeadByte[12];
};

typedef struct _cpinfo *LPCPINFO;

typedef struct _STARTUPINFOA _STARTUPINFOA, *P_STARTUPINFOA;

typedef CHAR *LPSTR;

typedef ushort WORD;

typedef BYTE *LPBYTE;

typedef void *HANDLE;

struct _STARTUPINFOA {
    DWORD cb;
    LPSTR lpReserved;
    LPSTR lpDesktop;
    LPSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    LPBYTE lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
};

typedef struct _PROCESS_INFORMATION _PROCESS_INFORMATION, *P_PROCESS_INFORMATION;

struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
};

typedef struct _SECURITY_ATTRIBUTES _SECURITY_ATTRIBUTES, *P_SECURITY_ATTRIBUTES;

typedef void *LPVOID;

typedef int BOOL;

struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
};

typedef struct _STARTUPINFOA *LPSTARTUPINFOA;

typedef struct _OVERLAPPED _OVERLAPPED, *P_OVERLAPPED;

typedef ulong ULONG_PTR;

typedef union _union_518 _union_518, *P_union_518;

typedef struct _struct_519 _struct_519, *P_struct_519;

typedef void *PVOID;

struct _struct_519 {
    DWORD Offset;
    DWORD OffsetHigh;
};

union _union_518 {
    struct _struct_519 s;
    PVOID Pointer;
};

struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union _union_518 u;
    HANDLE hEvent;
};

typedef struct _PROCESS_INFORMATION *LPPROCESS_INFORMATION;

typedef struct _RTL_CRITICAL_SECTION _RTL_CRITICAL_SECTION, *P_RTL_CRITICAL_SECTION;

typedef struct _RTL_CRITICAL_SECTION *PRTL_CRITICAL_SECTION;

typedef PRTL_CRITICAL_SECTION LPCRITICAL_SECTION;

typedef struct _RTL_CRITICAL_SECTION_DEBUG _RTL_CRITICAL_SECTION_DEBUG, *P_RTL_CRITICAL_SECTION_DEBUG;

typedef struct _RTL_CRITICAL_SECTION_DEBUG *PRTL_CRITICAL_SECTION_DEBUG;

typedef struct _LIST_ENTRY _LIST_ENTRY, *P_LIST_ENTRY;

typedef struct _LIST_ENTRY LIST_ENTRY;

struct _RTL_CRITICAL_SECTION {
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    ULONG_PTR SpinCount;
};

struct _LIST_ENTRY {
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
};

struct _RTL_CRITICAL_SECTION_DEBUG {
    WORD Type;
    WORD CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    DWORD EntryCount;
    DWORD ContentionCount;
    DWORD Flags;
    WORD CreatorBackTraceIndexHigh;
    WORD SpareWORD;
};

typedef DWORD (*PTHREAD_START_ROUTINE)(LPVOID);

typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

typedef struct _OVERLAPPED *LPOVERLAPPED;

typedef struct _EXCEPTION_POINTERS _EXCEPTION_POINTERS, *P_EXCEPTION_POINTERS;

typedef LONG (*PTOP_LEVEL_EXCEPTION_FILTER)(struct _EXCEPTION_POINTERS *);

typedef struct _EXCEPTION_RECORD _EXCEPTION_RECORD, *P_EXCEPTION_RECORD;

typedef struct _EXCEPTION_RECORD EXCEPTION_RECORD;

typedef EXCEPTION_RECORD *PEXCEPTION_RECORD;

typedef struct _CONTEXT _CONTEXT, *P_CONTEXT;

typedef struct _CONTEXT CONTEXT;

typedef CONTEXT *PCONTEXT;

typedef struct _FLOATING_SAVE_AREA _FLOATING_SAVE_AREA, *P_FLOATING_SAVE_AREA;

typedef struct _FLOATING_SAVE_AREA FLOATING_SAVE_AREA;

struct _FLOATING_SAVE_AREA {
    DWORD ControlWord;
    DWORD StatusWord;
    DWORD TagWord;
    DWORD ErrorOffset;
    DWORD ErrorSelector;
    DWORD DataOffset;
    DWORD DataSelector;
    BYTE RegisterArea[80];
    DWORD Cr0NpxState;
};

struct _CONTEXT {
    DWORD ContextFlags;
    DWORD Dr0;
    DWORD Dr1;
    DWORD Dr2;
    DWORD Dr3;
    DWORD Dr6;
    DWORD Dr7;
    FLOATING_SAVE_AREA FloatSave;
    DWORD SegGs;
    DWORD SegFs;
    DWORD SegEs;
    DWORD SegDs;
    DWORD Edi;
    DWORD Esi;
    DWORD Ebx;
    DWORD Edx;
    DWORD Ecx;
    DWORD Eax;
    DWORD Ebp;
    DWORD Eip;
    DWORD SegCs;
    DWORD EFlags;
    DWORD Esp;
    DWORD SegSs;
    BYTE ExtendedRegisters[512];
};

struct _EXCEPTION_RECORD {
    DWORD ExceptionCode;
    DWORD ExceptionFlags;
    struct _EXCEPTION_RECORD *ExceptionRecord;
    PVOID ExceptionAddress;
    DWORD NumberParameters;
    ULONG_PTR ExceptionInformation[15];
};

struct _EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PCONTEXT ContextRecord;
};

typedef struct _SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;

typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;

typedef wchar_t WCHAR;

typedef PVOID PSECURITY_DESCRIPTOR;

typedef LONG *PLONG;

typedef struct _ACL _ACL, *P_ACL;

struct _ACL {
    BYTE AclRevision;
    BYTE Sbz1;
    WORD AclSize;
    WORD AceCount;
    WORD Sbz2;
};

typedef struct _ACL ACL;

typedef CHAR *LPCH;

typedef struct _OSVERSIONINFOA _OSVERSIONINFOA, *P_OSVERSIONINFOA;

struct _OSVERSIONINFOA {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    CHAR szCSDVersion[128];
};

typedef struct _OSVERSIONINFOA *LPOSVERSIONINFOA;

typedef WCHAR *LPWSTR;

typedef WCHAR *LPWCH;

typedef WCHAR *LPCWSTR;

typedef DWORD LCID;

typedef ACL *PACL;

typedef struct IMAGE_DOS_HEADER IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

struct IMAGE_DOS_HEADER {
    char e_magic[2]; // Magic number
    word e_cblp; // Bytes of last page
    word e_cp; // Pages in file
    word e_crlc; // Relocations
    word e_cparhdr; // Size of header in paragraphs
    word e_minalloc; // Minimum extra paragraphs needed
    word e_maxalloc; // Maximum extra paragraphs needed
    word e_ss; // Initial (relative) SS value
    word e_sp; // Initial SP value
    word e_csum; // Checksum
    word e_ip; // Initial IP value
    word e_cs; // Initial (relative) CS value
    word e_lfarlc; // File address of relocation table
    word e_ovno; // Overlay number
    word e_res[4][4]; // Reserved words
    word e_oemid; // OEM identifier (for e_oeminfo)
    word e_oeminfo; // OEM information; e_oemid specific
    word e_res2[10][10]; // Reserved words
    dword e_lfanew; // File address of new exe header
    byte e_program[64]; // Actual DOS program
};

typedef ULONG_PTR SIZE_T;

typedef int (*FARPROC)(void);

typedef struct tagRECT tagRECT, *PtagRECT;

struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};

typedef DWORD *LPDWORD;

typedef WORD *LPWORD;

typedef int INT;

typedef WORD ATOM;

typedef struct HMENU__ HMENU__, *PHMENU__;

struct HMENU__ {
    int unused;
};

typedef struct tagRECT *LPRECT;

typedef BOOL *LPBOOL;

typedef void *LPCVOID;

typedef HINSTANCE HMODULE;

typedef struct HMENU__ *HMENU;

typedef struct IMAGE_OPTIONAL_HEADER32 IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct IMAGE_DATA_DIRECTORY IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

struct IMAGE_DATA_DIRECTORY {
    ImageBaseOffset32 VirtualAddress;
    dword Size;
};

struct IMAGE_OPTIONAL_HEADER32 {
    word Magic;
    byte MajorLinkerVersion;
    byte MinorLinkerVersion;
    dword SizeOfCode;
    dword SizeOfInitializedData;
    dword SizeOfUninitializedData;
    ImageBaseOffset32 AddressOfEntryPoint;
    ImageBaseOffset32 BaseOfCode;
    ImageBaseOffset32 BaseOfData;
    pointer32 ImageBase;
    dword SectionAlignment;
    dword FileAlignment;
    word MajorOperatingSystemVersion;
    word MinorOperatingSystemVersion;
    word MajorImageVersion;
    word MinorImageVersion;
    word MajorSubsystemVersion;
    word MinorSubsystemVersion;
    dword Win32VersionValue;
    dword SizeOfImage;
    dword SizeOfHeaders;
    dword CheckSum;
    word Subsystem;
    word DllCharacteristics;
    dword SizeOfStackReserve;
    dword SizeOfStackCommit;
    dword SizeOfHeapReserve;
    dword SizeOfHeapCommit;
    dword LoaderFlags;
    dword NumberOfRvaAndSizes;
    struct IMAGE_DATA_DIRECTORY DataDirectory[16];
};

typedef struct Var Var, *PVar;

struct Var {
    word wLength;
    word wValueLength;
    word wType;
};

typedef struct IMAGE_RESOURCE_DIRECTORY_ENTRY_NameStruct IMAGE_RESOURCE_DIRECTORY_ENTRY_NameStruct, *PIMAGE_RESOURCE_DIRECTORY_ENTRY_NameStruct;

struct IMAGE_RESOURCE_DIRECTORY_ENTRY_NameStruct {
    dword NameOffset:31;
    dword NameIsString:1;
};

typedef struct IMAGE_FILE_HEADER IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

struct IMAGE_FILE_HEADER {
    word Machine; // 332
    word NumberOfSections;
    dword TimeDateStamp;
    dword PointerToSymbolTable;
    dword NumberOfSymbols;
    word SizeOfOptionalHeader;
    word Characteristics;
};

typedef struct IMAGE_NT_HEADERS32 IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

struct IMAGE_NT_HEADERS32 {
    char Signature[4];
    struct IMAGE_FILE_HEADER FileHeader;
    struct IMAGE_OPTIONAL_HEADER32 OptionalHeader;
};

typedef struct IMAGE_RESOURCE_DIR_STRING_U_16 IMAGE_RESOURCE_DIR_STRING_U_16, *PIMAGE_RESOURCE_DIR_STRING_U_16;

struct IMAGE_RESOURCE_DIR_STRING_U_16 {
    word Length;
    wchar16 NameString[8];
};

typedef struct StringFileInfo StringFileInfo, *PStringFileInfo;

struct StringFileInfo {
    word wLength;
    word wValueLength;
    word wType;
};

typedef struct IMAGE_RESOURCE_DIRECTORY_ENTRY IMAGE_RESOURCE_DIRECTORY_ENTRY, *PIMAGE_RESOURCE_DIRECTORY_ENTRY;

typedef union IMAGE_RESOURCE_DIRECTORY_ENTRY_NameUnion IMAGE_RESOURCE_DIRECTORY_ENTRY_NameUnion, *PIMAGE_RESOURCE_DIRECTORY_ENTRY_NameUnion;

union IMAGE_RESOURCE_DIRECTORY_ENTRY_NameUnion {
    struct IMAGE_RESOURCE_DIRECTORY_ENTRY_NameStruct IMAGE_RESOURCE_DIRECTORY_ENTRY_NameStruct;
    dword Name;
    word Id;
};

struct IMAGE_RESOURCE_DIRECTORY_ENTRY {
    union IMAGE_RESOURCE_DIRECTORY_ENTRY_NameUnion NameUnion;
    union IMAGE_RESOURCE_DIRECTORY_ENTRY_DirectoryUnion DirectoryUnion;
};

typedef struct StringTable StringTable, *PStringTable;

struct StringTable {
    word wLength;
    word wValueLength;
    word wType;
};

typedef struct IMAGE_SECTION_HEADER IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

typedef union Misc Misc, *PMisc;

typedef enum SectionFlags {
    IMAGE_SCN_TYPE_NO_PAD=8,
    IMAGE_SCN_RESERVED_0001=16,
    IMAGE_SCN_CNT_CODE=32,
    IMAGE_SCN_CNT_INITIALIZED_DATA=64,
    IMAGE_SCN_CNT_UNINITIALIZED_DATA=128,
    IMAGE_SCN_LNK_OTHER=256,
    IMAGE_SCN_LNK_INFO=512,
    IMAGE_SCN_RESERVED_0040=1024,
    IMAGE_SCN_LNK_REMOVE=2048,
    IMAGE_SCN_LNK_COMDAT=4096,
    IMAGE_SCN_GPREL=32768,
    IMAGE_SCN_MEM_16BIT=131072,
    IMAGE_SCN_MEM_PURGEABLE=131072,
    IMAGE_SCN_MEM_LOCKED=262144,
    IMAGE_SCN_MEM_PRELOAD=524288,
    IMAGE_SCN_ALIGN_1BYTES=1048576,
    IMAGE_SCN_ALIGN_2BYTES=2097152,
    IMAGE_SCN_ALIGN_4BYTES=3145728,
    IMAGE_SCN_ALIGN_8BYTES=4194304,
    IMAGE_SCN_ALIGN_16BYTES=5242880,
    IMAGE_SCN_ALIGN_32BYTES=6291456,
    IMAGE_SCN_ALIGN_64BYTES=7340032,
    IMAGE_SCN_ALIGN_128BYTES=8388608,
    IMAGE_SCN_ALIGN_256BYTES=9437184,
    IMAGE_SCN_ALIGN_512BYTES=10485760,
    IMAGE_SCN_ALIGN_1024BYTES=11534336,
    IMAGE_SCN_ALIGN_2048BYTES=12582912,
    IMAGE_SCN_ALIGN_4096BYTES=13631488,
    IMAGE_SCN_ALIGN_8192BYTES=14680064,
    IMAGE_SCN_LNK_NRELOC_OVFL=16777216,
    IMAGE_SCN_MEM_DISCARDABLE=33554432,
    IMAGE_SCN_MEM_NOT_CACHED=67108864,
    IMAGE_SCN_MEM_NOT_PAGED=134217728,
    IMAGE_SCN_MEM_SHARED=268435456,
    IMAGE_SCN_MEM_EXECUTE=536870912,
    IMAGE_SCN_MEM_READ=1073741824,
    IMAGE_SCN_MEM_WRITE=2147483648
} SectionFlags;

union Misc {
    dword PhysicalAddress;
    dword VirtualSize;
};

struct IMAGE_SECTION_HEADER {
    char Name[8];
    union Misc Misc;
    ImageBaseOffset32 VirtualAddress;
    dword SizeOfRawData;
    dword PointerToRawData;
    dword PointerToRelocations;
    dword PointerToLinenumbers;
    word NumberOfRelocations;
    word NumberOfLinenumbers;
    enum SectionFlags Characteristics;
};

typedef struct VS_VERSION_INFO VS_VERSION_INFO, *PVS_VERSION_INFO;

struct VS_VERSION_INFO {
    word StructLength;
    word ValueLength;
    word StructType;
    wchar16 Info[16];
    byte Padding[2];
    dword Signature;
    word StructVersion[2];
    word FileVersion[4];
    word ProductVersion[4];
    dword FileFlagsMask[2];
    dword FileFlags;
    dword FileOS;
    dword FileType;
    dword FileSubtype;
    dword FileTimestamp;
};

typedef struct IMAGE_RESOURCE_DATA_ENTRY IMAGE_RESOURCE_DATA_ENTRY, *PIMAGE_RESOURCE_DATA_ENTRY;

struct IMAGE_RESOURCE_DATA_ENTRY {
    dword OffsetToData;
    dword Size;
    dword CodePage;
    dword Reserved;
};

typedef struct VarFileInfo VarFileInfo, *PVarFileInfo;

struct VarFileInfo {
    word wLength;
    word wValueLength;
    word wType;
};

typedef struct IMAGE_RESOURCE_DIRECTORY IMAGE_RESOURCE_DIRECTORY, *PIMAGE_RESOURCE_DIRECTORY;

struct IMAGE_RESOURCE_DIRECTORY {
    dword Characteristics;
    dword TimeDateStamp;
    word MajorVersion;
    word MinorVersion;
    word NumberOfNamedEntries;
    word NumberOfIdEntries;
};

typedef struct StringInfo StringInfo, *PStringInfo;

struct StringInfo {
    word wLength;
    word wValueLength;
    word wType;
};

typedef struct _iobuf _iobuf, *P_iobuf;

struct _iobuf {
    char *_ptr;
    int _cnt;
    char *_base;
    int _flag;
    int _file;
    int _charbuf;
    int _bufsiz;
    char *_tmpfname;
};

typedef struct _iobuf FILE;

typedef struct WSAData WSAData, *PWSAData;

typedef struct WSAData WSADATA;

struct WSAData {
    WORD wVersion;
    WORD wHighVersion;
    char szDescription[257];
    char szSystemStatus[129];
    ushort iMaxSockets;
    ushort iMaxUdpDg;
    char *lpVendorInfo;
};

typedef ushort u_short;

typedef UINT_PTR SOCKET;

typedef WSADATA *LPWSADATA;

typedef struct sockaddr sockaddr, *Psockaddr;

struct sockaddr {
    u_short sa_family;
    char sa_data[14];
};

typedef ulong u_long;

typedef struct hostent hostent, *Phostent;

struct hostent {
    char *h_name;
    char **h_aliases;
    short h_addrtype;
    short h_length;
    char **h_addr_list;
};

typedef uint size_t;

typedef int errno_t;



void *ExceptionList;
undefined LAB_0041928f;
string s_ShellExecute_failed_00420060;
undefined DAT_00420074;
undefined4 DAT_0042007c;
undefined1 DAT_00420080;
undefined4 DAT_0041a214;
undefined LAB_004192a0;
undefined DAT_0040170a;
undefined LAB_004192b8;
undefined LAB_004192d8;
undefined DAT_00423994;
undefined DAT_00423990;
char *DAT_00423998;
undefined DAT_00007f00;
undefined1 DAT_0041a218;
undefined DAT_0041a21c;
string s_WC_TENHOU_WIN_APP_LAUNCHER_00420084;
undefined FUN_00402300;
undefined LAB_00401e30;
undefined4 DAT_0042399c;
string s_EXIT_LAUNCHER_004200a0;
string s_Create_WC__LAUNCHER_failed_004200b0;
undefined DAT_004200cc;
string s_lpCmdLine=%s_004200e0;
string s_----_update_----_004200f0;
undefined DAT_00423988;
pointer PTR_FUN_0041a224;
string s_WSAStartup(0x101)_failed_00420104;
string s_use_wsock32.h_00420120;
undefined LAB_00401ff0;
byte DAT_00423d88;
HANDLE DAT_00423d94;
SOCKET DAT_00423d9c;
undefined LAB_00402250;
undefined DAT_00423d90;
pointer PTR_LAB_0041a228;
pointer PTR_FUN_0041a22c;
pointer PTR_FUN_0041a230;
pointer PTR_FUN_0041a234;
undefined DAT_0041e508;
pointer PTR_FUN_0041a238;
undefined DAT_0042013c;
string s_msctls_progress32_00420144;
LPCSTR DAT_00423a0c;
char *DAT_0042399c;
int *DAT_004239a4;
int DAT_004239a0;
undefined LAB_00419327;
string s_tenhou.exe_00420158;
string s_STATUS_00420164;
string s_LAUNCHER_0042016c;
string s_./config.ini_00420178;
undefined DAT_00420188;
undefined DAT_00420190;
undefined DAT_00420198;
undefined DAT_0042019c;
undefined1 DAT_004201a0;
undefined4 *DAT_004239a4;
undefined LAB_00419359;
byte *DAT_00423a0c;
string s_ChangeDirectoryExeFriend_failed_004201a8;
string s_ChangeDirectoryToLocalAppData_fa_004201c8;
undefined DAT_004201f0;
string s_tenhou_004201f4;
string s_C-EGG_004201fc;
string s_CSIDL_LOCAL_APPDATA_failed_00420204;
undefined1 DAT_004239a8;
undefined4 DAT_004239ac;
undefined DAT_004239b0;
undefined DAT_004239b4;
undefined LAB_00403a60;
undefined4 *DAT_004239ac;
undefined LAB_00419386;
pointer PTR_FUN_0041a240;
undefined LAB_004193f3;
string s_Location:_00420220;
string s_Content-Length:_0042022c;
string s_HTTP/1.0_00420240;
string s_HTTP/1.1_0042024c;
undefined1 DAT_00420258;
string s_Tenhou-Client-Type:_download_00420260;
undefined1 DAT_00420280;
string s_Host:_00420284;
string s_Accept-Encoding:_gzip,_deflate_0042028c;
string s_http://tenhou.net/)_004202b0;
string s_OS%d/%d.%d.%d;_004202c8;
string s_C-EGG_TENHOU;_004202d8;
string s_compatible;_004202e8;
string s_User-Agent:_Mozilla/4.0_(_004202f4;
string s_HTTP/1.0_00420310;
undefined DAT_0042031c;
string s_connect_error=%d_00420324;
undefined DAT_00420338;
undefined1 DAT_0042033c;
string s_localhost_00420340;
undefined LAB_0041942e;
undefined LAB_00419450;
string s_BASEURL=%s_0042034c;
string s_INVALID_BASEURL_HTTP_0042035c;
string s_http://_00420374;
string s_BASEURL=_0042037c;
undefined DAT_00420388;
string s_http://tenhou.net/1/resinfo-rele_004203f0;
undefined DAT_004239a8;
undefined LAB_00419468;
string s_SKIPUPDATE_%s_0042041c;
string s_.skipupdate_0042042c;
undefined DAT_00420438;
undefined DAT_0042043c;
undefined LAB_004194e3;
undefined DAT_00420440;
string s_GOING_DOWNLOAD_%s_END_004204c8;
string s_GOING_DOWNLOAD_%s_BEGIN_004204e0;
undefined DAT_004204fc;
undefined LAB_004194f8;
undefined *PTR_LAB_0041a244;
pointer PTR_LAB_0041a274;
string s_GetTempFileName_failed_00420538;
undefined DAT_00420550;
string s_GetTempPath_failed_00420554;
string s_%c%c%c%c%c%c%c%c%c%c_00420568;
string s_1.2.3_00420580;
undefined s_1.2.3_00420580;
undefined FUN_0040b2f0;
undefined FUN_0040b310;
undefined DAT_0041ab20;
string s_incorrect_length_check_00420588;
string s_incorrect_data_check_004205a0;
string s_invalid_distance_too_far_back_004205b8;
string s_invalid_distance_code_004205d8;
string s_invalid_literal/length_code_004205f0;
string s_invalid_distances_set_0042060c;
string s_invalid_literal/lengths_set_00420624;
string s_invalid_bit_length_repeat_00420640;
string s_too_many_length_or_distance_symb_0042065c;
string s_invalid_code_lengths_set_00420680;
string s_invalid_stored_block_lengths_0042069c;
string s_invalid_block_type_004206bc;
string s_header_crc_mismatch_004206d0;
string s_unknown_header_flags_set_004206e4;
string s_incorrect_header_check_00420700;
string s_invalid_window_size_00420718;
string s_unknown_compression_method_0042072c;
pointer switchdataD_00409304;
undefined DAT_0041a2a0;
undefined DAT_0041aaa0;
string s_insufficient_memory_00420770;
undefined DAT_0041ab88;
string s_buffer_error_00420760;
string s_stream_error_00420790;
undefined DAT_0041ab80;
undefined DAT_0041ab82;
undefined DAT_0041ab84;
undefined DAT_0041ab86;
undefined DAT_0041ac00;
undefined DAT_0041b000;
undefined DAT_0041b400;
undefined DAT_0041b800;
undefined DAT_0041ca56;
undefined DAT_0041ca96;
undefined DAT_0041ccd8;
undefined DAT_0041cd18;
pointer PTR_DAT_004207c8;
pointer PTR_DAT_004207e0;
undefined DAT_004207f8;
undefined1 DAT_0041ce90;
undefined DAT_0041cd58;
undefined DAT_0041cdcc;
undefined DAT_0041d39b;
undefined DAT_0041d49c;
undefined DAT_0041d59c;
undefined DAT_0041d69c;
undefined DAT_0041d710;
undefined LAB_00419550;
undefined LAB_004195c6;
undefined LAB_004195d8;
int DAT_004239a4;
undefined LAB_00419601;
undefined *PTR_LAB_0041d78c;
undefined DAT_0041e8c8;
undefined *PTR_FUN_0041d7ac;
undefined DAT_004239b8;
undefined DAT_0041e938;
undefined *PTR_LAB_0041d7bc;
undefined DAT_0041e9f0;
undefined FUN_00401d40;
undefined DAT_0040f271;
undefined4 DAT_004239d8;
undefined LAB_0040f2f0;
undefined DAT_004239c0;
int DAT_004239d8;
undefined FUN_0040f43e;
undefined FUN_0040f519;
undefined LAB_0040f62c;
undefined4 DAT_004208f8;
undefined4 DAT_004208f4;
undefined4 DAT_004208fc;
int DAT_004208fc;
byte *DAT_00425138;
byte *DAT_00425134;
pointer vftable;
undefined4 DAT_0041d7e8;
undefined FUN_0040fc4a;
DWORD DAT_00420ac0;
undefined *PTR_FUN_00420a9c;
undefined LAB_004134d8;
undefined DAT_0041d808;
undefined *PTR_FUN_00420aa0;
undefined *PTR_FUN_00420a98;
undefined DAT_00420000;
undefined DAT_00420024;
undefined DAT_00420028;
undefined DAT_0042003c;
int DAT_00423a1c;
undefined DAT_00423a18;
undefined1 DAT_00423a14;
undefined4 *DAT_00425138;
int DAT_00425134;
undefined DAT_00420040;
undefined DAT_00420048;
undefined DAT_0042004c;
undefined DAT_00420054;
int DAT_00424f1c;
UINT DAT_00424f04;
LCID DAT_00425124;
undefined1 DAT_00424f20;
undefined4 DAT_00425020;
int DAT_00420f1c;
undefined *PTR_DAT_00420d10;
undefined DAT_0041d818;
undefined DAT_0041d828;
undefined DAT_004239ec;
undefined DAT_004239e8;
undefined DAT_004239e4;
undefined DAT_004239e0;
LPSTR DAT_00425128;
LPSTR DAT_00423a20;
undefined DAT_0041d838;
int DAT_00423a28;
undefined *PTR___exit_00420920;
undefined4 DAT_00423de0;
uint DAT_00423ee0;
uint DAT_00423be4;
undefined DAT_00423acc;
undefined4 DAT_00420928;
undefined DAT_00420a90;
int DAT_00423be8;
int DAT_00423dcc;
uint *DAT_00423dc4;
HANDLE DAT_00423dc8;
uint DAT_0042328c;
undefined DAT_0041d848;
undefined DAT_0041d860;
int DAT_00424f00;
int DAT_00423ee4;
undefined DAT_00423a58;
undefined *PTR_FUN_0042335c;
undefined *PTR_FUN_00423358;
undefined __fassign;
undefined *PTR_FUN_00423360;
undefined FUN_0041690d;
undefined __cfltcvt;
undefined *PTR_FUN_00423364;
undefined *PTR_FUN_00423368;
undefined *PTR_FUN_0042336c;
undefined LAB_00416967;
undefined LAB_004169b5;
pointer PTR_FUN_0041d87c;
undefined DAT_0041d898;
undefined DAT_0041d8a8;
undefined DAT_0041d8c0;
undefined DAT_0041d8d0;
undefined DAT_00420b90;
undefined DAT_0041d8e0;
undefined *PTR_FUN_00420ac4;
undefined DAT_0041d8f8;
byte *DAT_00423be8;
uint *DAT_0042328c;
undefined DAT_0041d910;
undefined DAT_0041d928;
undefined DAT_00420f50;
undefined DAT_00420f70;
undefined DAT_004211b0;
undefined *PTR_DAT_00420acc;
undefined *PTR_DAT_00420ac8;
undefined DAT_0041d920;
undefined DAT_0041d940;
undefined *PTR_DAT_00420b14;
undefined *PTR_DAT_00420b04;
undefined *PTR_DAT_00420af4;
undefined *PTR_DAT_00420ad4;
undefined DAT_00420ad0;
FARPROC DAT_00423ac0;
undefined DAT_0041d9b8;
int DAT_00420c08;
int DAT_00420c0c;
int DAT_00420c14;
undefined4 DAT_00425124;
undefined4 DAT_00424f1c;
int DAT_00423ac4;
undefined4 DAT_00424f10;
undefined4 DAT_00424f14;
undefined4 DAT_00424f18;
undefined DAT_00420c18;
undefined4 DAT_00420c20;
uint DAT_00423ac4;
int DAT_00423c0c;
undefined4 DAT_00424f04;
int DAT_00425130;
int DAT_00423ac8;
UINT DAT_00423c0c;
undefined DAT_0041d9d8;
undefined DAT_00423da8;
int DAT_00423da4;
int DAT_00423bfc;
LCID DAT_00423bfc;
byte *DAT_00425128;
uint *DAT_00423a20;
undefined DAT_004239fc;
undefined DAT_0042512c;
undefined *DAT_00423a0c;
undefined DAT_004239f4;
undefined DAT_004239f0;
undefined DAT_00423ad0;
int DAT_00423bd4;
UINT DAT_00423ee0;
undefined4 DAT_00423de4;
int DAT_00420924;
undefined *DAT_00423bd8;
undefined DAT_0041dcd0;
undefined4 DAT_004211d8;
pointer PTR_LOOP_00421268;
undefined4 DAT_00423bdc;
undefined *DAT_00423bec;
LPVOID DAT_00423dc0;
undefined4 DAT_00423db8;
undefined4 DAT_00423dbc;
LPVOID DAT_00423db4;
undefined4 DAT_00423dc4;
undefined4 DAT_00423dac;
int DAT_00423dbc;
uint DAT_00423dc0;
uint *DAT_00423db8;
uint DAT_00423db0;
int DAT_00423dc0;
int DAT_00423db4;
uint *DAT_00423dc0;
uint *DAT_00423db4;
int DAT_00423db0;
int DAT_00423dac;
int DAT_00421278;
undefined *PTR_LOOP_0042126c;
undefined *PTR_LOOP_00423288;
undefined4 DAT_00421278;
int DAT_00423bf0;
char DAT_00420f20;
undefined1 DAT_00420f20;
int DAT_00423c14;
undefined DAT_0041e098;
int DAT_00423ee0;
int DAT_00423ca8;
uint DAT_004239dc;
undefined DAT_00420d1a;
FARPROC DAT_00423c24;
FARPROC DAT_00423c28;
FARPROC DAT_00423c2c;
undefined DAT_00423480;
undefined DAT_00423498;
byte DAT_00420f20;
undefined4 DAT_00423c38;
undefined4 DAT_00423c34;
undefined4 DAT_00423c3c;
undefined4 DAT_00423c30;
undefined *PTR_DAT_00423550;

undefined4 __cdecl FUN_00401000(LPCSTR param_1,char *param_2)

{
  char cVar1;
  undefined1 *puVar2;
  undefined4 uVar3;
  BOOL BVar4;
  HINSTANCE pHVar5;
  uint uVar6;
  uint uVar7;
  undefined4 *puVar8;
  LPCSTR pCVar9;
  char *pcVar10;
  undefined4 *puVar11;
  undefined1 local_c0 [4];
  undefined4 *local_bc;
  undefined1 local_b0 [4];
  LPSTR local_ac;
  uint local_a8;
  _PROCESS_INFORMATION local_a0;
  _SECURITY_ATTRIBUTES local_90;
  undefined1 local_84 [16];
  undefined1 local_74 [16];
  undefined1 local_64 [20];
  _STARTUPINFOA local_50;
  void *local_c;
  undefined1 *puStack_8;
  int local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_0041928f;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  FUN_00401380(local_b0,'\0');
  local_4 = 0;
  FUN_00401380(local_c0,'\0');
  uVar6 = 0xffffffff;
  pCVar9 = param_1;
  do {
    if (uVar6 == 0) break;
    uVar6 = uVar6 - 1;
    cVar1 = *pCVar9;
    pCVar9 = pCVar9 + 1;
  } while (cVar1 != '\0');
  FUN_004014b0(local_c0,(undefined4 *)param_1,~uVar6 - 1);
  local_4._0_1_ = 1;
  puVar2 = FUN_004017a0(local_84,&DAT_00420080,(int)local_c0);
  local_4._0_1_ = 2;
  puVar2 = FUN_00401be0(local_74,puVar2,&DAT_00420080);
  local_4._0_1_ = 3;
  FUN_004013d0(local_b0,(int)puVar2,0,0xffffffff);
  local_4._0_1_ = 2;
  FUN_00401380(local_74,'\x01');
  local_4._0_1_ = 1;
  FUN_00401380(local_84,'\x01');
  local_4 = (uint)local_4._1_3_ << 8;
  FUN_00401380(local_c0,'\x01');
  if ((param_2 != (char *)0x0) && (*param_2 != '\0')) {
    FUN_00401380(local_c0,'\0');
    uVar6 = 0xffffffff;
    pcVar10 = (char *)&DAT_0042007c;
    do {
      if (uVar6 == 0) break;
      uVar6 = uVar6 - 1;
      cVar1 = *pcVar10;
      pcVar10 = pcVar10 + 1;
    } while (cVar1 != '\0');
    uVar6 = ~uVar6 - 1;
    uVar3 = FUN_00401570(local_c0,uVar6,'\x01');
    if ((char)uVar3 != '\0') {
      puVar8 = &DAT_0042007c;
      puVar11 = local_bc;
      for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
        *puVar11 = *puVar8;
        puVar8 = puVar8 + 1;
        puVar11 = puVar11 + 1;
      }
      for (uVar7 = uVar6 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
        *(undefined1 *)puVar11 = *(undefined1 *)puVar8;
        puVar8 = (undefined4 *)((int)puVar8 + 1);
        puVar11 = (undefined4 *)((int)puVar11 + 1);
      }
      FUN_00401550(local_c0,uVar6);
    }
    local_4._0_1_ = 4;
    puVar2 = FUN_00401be0(local_84,local_c0,param_2);
    local_4._0_1_ = 5;
    FUN_004013d0(local_b0,(int)puVar2,0,0xffffffff);
    local_4._0_1_ = 4;
    FUN_00401380(local_84,'\x01');
    local_4 = (uint)local_4._1_3_ << 8;
    if (local_bc != (undefined4 *)0x0) {
      cVar1 = *(char *)((int)local_bc + -1);
      if ((cVar1 == '\0') || (cVar1 == -1)) {
        FUN_0040ef10((char *)((int)local_bc + -1));
      }
      else {
        *(char *)((int)local_bc + -1) = cVar1 + -1;
      }
    }
  }
  GetStartupInfoA(&local_50);
  local_a0.hProcess = (HANDLE)0x0;
  local_90.lpSecurityDescriptor = local_64;
  local_a0.hThread = (HANDLE)0x0;
  local_a0.dwProcessId = 0;
  local_a0.dwThreadId = 0;
  BVar4 = InitializeSecurityDescriptor(local_64,1);
  if (BVar4 != 0) {
    BVar4 = SetSecurityDescriptorDacl(local_90.lpSecurityDescriptor,1,(PACL)0x0,0);
    if (BVar4 != 0) {
      if (local_ac != (LPSTR)0x0) {
        if ((local_ac[-1] != '\0') && (local_ac[-1] != -1)) {
          FUN_00401570(local_b0,local_a8,'\0');
        }
        if (local_ac != (LPSTR)0x0) {
          local_ac[-1] = -1;
        }
      }
      BVar4 = CreateProcessA((LPCSTR)0x0,local_ac,&local_90,&local_90,1,0,(LPVOID)0x0,(LPCSTR)0x0,
                             &local_50,&local_a0);
      if (BVar4 != 0) {
        WaitForInputIdle(local_a0.hProcess,0xffffffff);
        CloseHandle(local_a0.hProcess);
        goto LAB_004012e2;
      }
    }
  }
  pHVar5 = ShellExecuteA((HWND)0x0,&DAT_00420074,param_1,param_2,(LPCSTR)0x0,1);
  if (pHVar5 < (HINSTANCE)0x20) {
    MessageBoxA((HWND)0x0,s_ShellExecute_failed_00420060,(LPCSTR)0x0,0);
  }
LAB_004012e2:
  if (local_ac != (LPSTR)0x0) {
    cVar1 = local_ac[-1];
    if ((cVar1 != '\0') && (cVar1 != -1)) {
      local_ac[-1] = cVar1 + -1;
      ExceptionList = local_c;
      return 0;
    }
    FUN_0040ef10(local_ac + -1);
  }
  ExceptionList = local_c;
  return 0;
}



void __fastcall FUN_00401340(int param_1)

{
  char cVar1;
  int iVar2;
  
  iVar2 = *(int *)(param_1 + 4);
  if (iVar2 != 0) {
    cVar1 = *(char *)(iVar2 + -1);
    if ((cVar1 == '\0') || (cVar1 == -1)) {
      FUN_0040ef10((char *)(iVar2 + -1));
    }
    else {
      *(char *)(iVar2 + -1) = cVar1 + -1;
    }
  }
  *(undefined4 *)(param_1 + 4) = 0;
  *(undefined4 *)(param_1 + 8) = 0;
  *(undefined4 *)(param_1 + 0xc) = 0;
  return;
}



void __thiscall FUN_00401380(void *this,char param_1)

{
  char cVar1;
  int iVar2;
  
  if ((param_1 != '\0') && (iVar2 = *(int *)((int)this + 4), iVar2 != 0)) {
    cVar1 = *(char *)(iVar2 + -1);
    if ((cVar1 == '\0') || (cVar1 == -1)) {
      FUN_0040ef10((char *)(iVar2 + -1));
    }
    else {
      *(char *)(iVar2 + -1) = cVar1 + -1;
    }
  }
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  return;
}



void * __thiscall FUN_004013d0(void *this,int param_1,uint param_2,uint param_3)

{
  char cVar1;
  undefined1 *puVar2;
  uint uVar3;
  undefined4 *puVar4;
  uint uVar5;
  undefined4 *puVar6;
  
  if (*(uint *)(param_1 + 8) < param_2) {
    FUN_0040f147();
  }
  uVar3 = *(int *)(param_1 + 8) - param_2;
  if (uVar3 < param_3) {
    param_3 = uVar3;
  }
  if (-*(int *)((int)this + 8) - 1U <= param_3) {
    FUN_0040ef1b();
  }
  if (param_3 == 0) {
    return this;
  }
  uVar3 = *(int *)((int)this + 8) + param_3;
  if (0xfffffffd < uVar3) {
    FUN_0040ef1b();
  }
  puVar2 = *(undefined1 **)((int)this + 4);
  if (((puVar2 == (undefined1 *)0x0) || (cVar1 = puVar2[-1], cVar1 == '\0')) || (cVar1 == -1)) {
    if (uVar3 == 0) {
      if (puVar2 == (undefined1 *)0x0) {
        return this;
      }
      *(undefined4 *)((int)this + 8) = 0;
      *puVar2 = 0;
      return this;
    }
    if (uVar3 <= *(uint *)((int)this + 0xc)) goto LAB_00401471;
  }
  else if (uVar3 == 0) {
    puVar2[-1] = cVar1 + -1;
    FUN_00401380(this,'\0');
    return this;
  }
  FUN_00401690(uVar3);
LAB_00401471:
  puVar4 = *(undefined4 **)(param_1 + 4);
  if (puVar4 == (undefined4 *)0x0) {
    puVar4 = &DAT_0041a214;
  }
  puVar4 = (undefined4 *)((int)puVar4 + param_2);
  puVar6 = (undefined4 *)(*(int *)((int)this + 8) + *(int *)((int)this + 4));
  for (uVar5 = param_3 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *puVar6 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar6 = puVar6 + 1;
  }
  for (uVar5 = param_3 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined1 *)puVar6 = *(undefined1 *)puVar4;
    puVar4 = (undefined4 *)((int)puVar4 + 1);
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  *(uint *)((int)this + 8) = uVar3;
  *(undefined1 *)(*(int *)((int)this + 4) + uVar3) = 0;
  return this;
}



void * __thiscall FUN_004014b0(void *this,undefined4 *param_1,uint param_2)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  undefined4 *puVar4;
  
  if (0xfffffffd < param_2) {
    FUN_0040ef1b();
  }
  iVar2 = *(int *)((int)this + 4);
  if (((iVar2 == 0) || (cVar1 = *(char *)(iVar2 + -1), cVar1 == '\0')) || (cVar1 == -1)) {
    if (param_2 == 0) {
      FUN_00401380(this,'\x01');
      return this;
    }
    if ((*(uint *)((int)this + 0xc) < 0x20) && (param_2 <= *(uint *)((int)this + 0xc)))
    goto LAB_00401520;
    FUN_00401380(this,'\x01');
  }
  else if (param_2 == 0) {
    *(char *)(iVar2 + -1) = cVar1 + -1;
    *(undefined4 *)((int)this + 4) = 0;
    *(undefined4 *)((int)this + 8) = 0;
    *(undefined4 *)((int)this + 0xc) = 0;
    return this;
  }
  FUN_00401690(param_2);
LAB_00401520:
  puVar4 = *(undefined4 **)((int)this + 4);
  for (uVar3 = param_2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    *puVar4 = *param_1;
    param_1 = param_1 + 1;
    puVar4 = puVar4 + 1;
  }
  for (uVar3 = param_2 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *(undefined1 *)puVar4 = *(undefined1 *)param_1;
    param_1 = (undefined4 *)((int)param_1 + 1);
    puVar4 = (undefined4 *)((int)puVar4 + 1);
  }
  *(uint *)((int)this + 8) = param_2;
  *(undefined1 *)(param_2 + *(int *)((int)this + 4)) = 0;
  return this;
}



void __thiscall FUN_00401550(void *this,int param_1)

{
  *(int *)((int)this + 8) = param_1;
  *(undefined1 *)(*(int *)((int)this + 4) + param_1) = 0;
  return;
}



int __thiscall FUN_00401570(void *this,uint param_1,char param_2)

{
  char cVar1;
  uint3 uVar5;
  uint uVar2;
  undefined4 uVar3;
  undefined1 *puVar4;
  
  if (0xfffffffd < param_1) {
    FUN_0040ef1b();
  }
  puVar4 = *(undefined1 **)((int)this + 4);
  uVar5 = (uint3)((uint)puVar4 >> 8);
  if (((puVar4 == (undefined1 *)0x0) || (cVar1 = puVar4[-1], cVar1 == '\0')) || (cVar1 == -1)) {
    if (param_1 == 0) {
      if (param_2 == '\0') {
        if (puVar4 != (undefined1 *)0x0) {
          *(undefined4 *)((int)this + 8) = 0;
          *puVar4 = 0;
        }
        return (uint)uVar5 << 8;
      }
      uVar2 = 0;
      if (puVar4 != (undefined1 *)0x0) {
        cVar1 = puVar4[-1];
        if ((cVar1 != '\0') && (cVar1 != -1)) {
          puVar4[-1] = cVar1 + -1;
          *(undefined4 *)((int)this + 4) = 0;
          *(undefined4 *)((int)this + 8) = 0;
          *(undefined4 *)((int)this + 0xc) = 0;
          return (uint)uVar5 << 8;
        }
        uVar2 = FUN_0040ef10(puVar4 + -1);
      }
      *(undefined4 *)((int)this + 4) = 0;
      *(undefined4 *)((int)this + 8) = 0;
      *(undefined4 *)((int)this + 0xc) = 0;
      return uVar2 & 0xffffff00;
    }
    if (param_2 != '\0') {
      if ((0x1f < *(uint *)((int)this + 0xc)) || (*(uint *)((int)this + 0xc) < param_1)) {
        if (puVar4 != (undefined1 *)0x0) {
          cVar1 = puVar4[-1];
          if ((cVar1 != '\0') && (cVar1 != -1)) {
            puVar4[-1] = cVar1 + -1;
            *(undefined4 *)((int)this + 4) = 0;
            *(undefined4 *)((int)this + 8) = 0;
            *(undefined4 *)((int)this + 0xc) = 0;
            uVar3 = FUN_00401690(param_1);
            return CONCAT31((int3)((uint)uVar3 >> 8),1);
          }
          FUN_0040ef10(puVar4 + -1);
        }
        *(undefined4 *)((int)this + 4) = 0;
        *(undefined4 *)((int)this + 8) = 0;
        *(undefined4 *)((int)this + 0xc) = 0;
        uVar3 = FUN_00401690(param_1);
        return CONCAT31((int3)((uint)uVar3 >> 8),1);
      }
      goto LAB_00401682;
    }
    if (param_1 <= *(uint *)((int)this + 0xc)) goto LAB_00401682;
  }
  else if (param_1 == 0) {
    puVar4[-1] = cVar1 + -1;
    *(undefined4 *)((int)this + 4) = 0;
    *(undefined4 *)((int)this + 8) = 0;
    *(undefined4 *)((int)this + 0xc) = 0;
    return (uint)uVar5 << 8;
  }
  puVar4 = (undefined1 *)FUN_00401690(param_1);
LAB_00401682:
  return CONCAT31((int3)((uint)puVar4 >> 8),1);
}



void FUN_00401690(uint param_1)

{
  uint uVar1;
  void *pvStack_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  puStack_c = &LAB_004192a0;
  pvStack_10 = ExceptionList;
  uVar1 = param_1 | 0x1f;
  if (0xfffffffd < (param_1 | 0x1f)) {
    uVar1 = param_1;
  }
  uVar1 = uVar1 + 2;
  local_8 = 0;
  if ((int)uVar1 < 0) {
    uVar1 = 0;
  }
  ExceptionList = &pvStack_10;
  operator_new(uVar1);
  FUN_00401710();
  return;
}



undefined * Catch_004016e9(void)

{
  uint uVar1;
  void *pvVar2;
  int unaff_EBP;
  
  *(int *)(unaff_EBP + -0x14) = *(int *)(unaff_EBP + 8);
  uVar1 = *(int *)(unaff_EBP + 8) + 2;
  if ((int)uVar1 < 0) {
    uVar1 = 0;
  }
  pvVar2 = operator_new(uVar1);
  *(void **)(unaff_EBP + 8) = pvVar2;
  return &DAT_0040170a;
}



void FUN_00401710(void)

{
  char cVar1;
  int iVar2;
  undefined1 *puVar3;
  uint uVar4;
  uint uVar5;
  int unaff_EBX;
  int unaff_EBP;
  uint unaff_ESI;
  undefined4 *puVar6;
  undefined4 *puVar7;
  
  uVar5 = *(uint *)(unaff_EBX + 8);
  if (uVar5 != 0) {
    if (unaff_ESI < uVar5) {
      uVar5 = unaff_ESI;
    }
    puVar6 = *(undefined4 **)(unaff_EBX + 4);
    puVar7 = (undefined4 *)(*(int *)(unaff_EBP + 8) + 1);
    for (uVar4 = uVar5 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
      *puVar7 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar7 = puVar7 + 1;
    }
    for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
      *(undefined1 *)puVar7 = *(undefined1 *)puVar6;
      puVar6 = (undefined4 *)((int)puVar6 + 1);
      puVar7 = (undefined4 *)((int)puVar7 + 1);
    }
    unaff_ESI = *(uint *)(unaff_EBP + -0x14);
  }
  iVar2 = *(int *)(unaff_EBX + 4);
  uVar5 = *(uint *)(unaff_EBX + 8);
  if (iVar2 != 0) {
    cVar1 = *(char *)(iVar2 + -1);
    if ((cVar1 == '\0') || (cVar1 == -1)) {
      FUN_0040ef10((char *)(iVar2 + -1));
    }
    else {
      *(char *)(iVar2 + -1) = cVar1 + -1;
    }
  }
  puVar3 = *(undefined1 **)(unaff_EBP + 8);
  *(undefined4 *)(unaff_EBX + 8) = 0;
  *(undefined1 **)(unaff_EBX + 4) = puVar3 + 1;
  *puVar3 = 0;
  *(uint *)(unaff_EBX + 0xc) = unaff_ESI;
  if (uVar5 <= unaff_ESI) {
    unaff_ESI = uVar5;
  }
  *(uint *)(unaff_EBX + 8) = unaff_ESI;
  *(undefined1 *)(*(int *)(unaff_EBX + 4) + unaff_ESI) = 0;
  ExceptionList = *(void **)(unaff_EBP + -0xc);
  return;
}



undefined1 * __cdecl FUN_004017a0(undefined1 *param_1,char *param_2,int param_3)

{
  char cVar1;
  undefined4 uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  char *pcVar6;
  char *pcVar7;
  undefined1 local_1c [4];
  char *local_18;
  uint local_14;
  undefined4 local_10;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004192b8;
  pvStack_c = ExceptionList;
  local_1c[0] = param_2._0_1_;
  uVar3 = 0xffffffff;
  pcVar6 = param_2;
  do {
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    cVar1 = *pcVar6;
    pcVar6 = pcVar6 + 1;
  } while (cVar1 != '\0');
  uVar3 = ~uVar3 - 1;
  local_18 = (char *)0x0;
  local_14 = 0;
  local_10 = 0;
  ExceptionList = &pvStack_c;
  uVar2 = FUN_00401570(local_1c,uVar3,'\x01');
  if ((char)uVar2 != '\0') {
    pcVar6 = local_18;
    for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
      *(undefined4 *)pcVar6 = *(undefined4 *)param_2;
      param_2 = param_2 + 4;
      pcVar6 = pcVar6 + 4;
    }
    for (uVar4 = uVar3 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
      *pcVar6 = *param_2;
      param_2 = param_2 + 1;
      pcVar6 = pcVar6 + 1;
    }
    FUN_00401550(local_1c,uVar3);
  }
  local_4 = 0;
  uVar3 = 0xffffffff;
  if (*(uint *)(param_3 + 8) != 0xffffffff) {
    uVar3 = *(uint *)(param_3 + 8);
  }
  if (-local_14 - 1 <= uVar3) {
    FUN_0040ef1b();
  }
  if (uVar3 != 0) {
    uVar4 = local_14 + uVar3;
    uVar2 = FUN_00401570(local_1c,uVar4,'\0');
    if ((char)uVar2 != '\0') {
      pcVar6 = *(char **)(param_3 + 4);
      if (pcVar6 == (char *)0x0) {
        pcVar6 = "";
      }
      pcVar7 = local_18 + local_14;
      for (uVar5 = uVar3 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined4 *)pcVar7 = *(undefined4 *)pcVar6;
        pcVar6 = pcVar6 + 4;
        pcVar7 = pcVar7 + 4;
      }
      for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
        *pcVar7 = *pcVar6;
        pcVar6 = pcVar6 + 1;
        pcVar7 = pcVar7 + 1;
      }
      local_18[uVar4] = '\0';
      local_14 = uVar4;
    }
  }
  *param_1 = local_1c[0];
  *(undefined4 *)(param_1 + 4) = 0;
  *(undefined4 *)(param_1 + 8) = 0;
  *(undefined4 *)(param_1 + 0xc) = 0;
  FUN_00401900(param_1,local_1c,0,0xffffffff);
  if (local_18 != (char *)0x0) {
    cVar1 = local_18[-1];
    if ((cVar1 != '\0') && (cVar1 != -1)) {
      local_18[-1] = cVar1 + -1;
      ExceptionList = pvStack_c;
      return param_1;
    }
    FUN_0040ef10(local_18 + -1);
  }
  ExceptionList = pvStack_c;
  return param_1;
}



void * __thiscall FUN_00401900(void *this,void *param_1,uint param_2,uint param_3)

{
  char cVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  undefined4 *puVar5;
  uint uVar6;
  int iVar7;
  undefined4 *puVar8;
  
  if (*(uint *)((int)param_1 + 8) < param_2) {
    FUN_0040f147();
  }
  uVar6 = *(uint *)((int)param_1 + 8) - param_2;
  if (param_3 < uVar6) {
    uVar6 = param_3;
  }
  if (this == param_1) {
    uVar6 = uVar6 + param_2;
    if (*(uint *)((int)this + 8) < uVar6) {
      FUN_0040f147();
    }
    FUN_00401b10(this);
    iVar2 = *(int *)((int)this + 8) - uVar6;
    iVar7 = -1;
    if (iVar2 != -1) {
      iVar7 = iVar2;
    }
    if (iVar7 != 0) {
      puVar5 = (undefined4 *)(*(int *)((int)this + 4) + uVar6);
      FUN_0040f710(puVar5,(undefined4 *)((int)puVar5 + iVar7),iVar2 - iVar7);
      uVar6 = *(int *)((int)this + 8) - iVar7;
      uVar3 = FUN_00401570(this,uVar6,'\0');
      if ((char)uVar3 != '\0') {
        *(uint *)((int)this + 8) = uVar6;
        *(undefined1 *)(uVar6 + *(int *)((int)this + 4)) = 0;
      }
    }
    FUN_00401b10(this);
    uVar6 = *(uint *)((int)this + 8);
    if (uVar6 < param_2) {
      param_2 = uVar6;
    }
    if (param_2 == 0) {
      return this;
    }
    FUN_0040f710(*(undefined4 **)((int)this + 4),
                 (undefined4 *)((int)*(undefined4 **)((int)this + 4) + param_2),uVar6 - param_2);
    uVar6 = *(int *)((int)this + 8) - param_2;
    uVar3 = FUN_00401570(this,uVar6,'\0');
    if ((char)uVar3 == '\0') {
      return this;
    }
    FUN_00401550(this,uVar6);
    return this;
  }
  if ((uVar6 != 0) && (uVar6 == *(uint *)((int)param_1 + 8))) {
    puVar5 = *(undefined4 **)((int)param_1 + 4);
    if (puVar5 == (undefined4 *)0x0) {
      puVar5 = &DAT_0041a214;
    }
    if (*(byte *)((int)puVar5 + -1) < 0xfe) {
      iVar7 = *(int *)((int)this + 4);
      if (iVar7 != 0) {
        cVar1 = *(char *)(iVar7 + -1);
        if ((cVar1 == '\0') || (cVar1 == -1)) {
          FUN_0040ef10((char *)(iVar7 + -1));
        }
        else {
          *(char *)(iVar7 + -1) = cVar1 + -1;
        }
      }
      *(undefined4 *)((int)this + 4) = 0;
      *(undefined4 *)((int)this + 8) = 0;
      *(undefined4 *)((int)this + 0xc) = 0;
      puVar5 = *(undefined4 **)((int)param_1 + 4);
      if (puVar5 == (undefined4 *)0x0) {
        puVar5 = &DAT_0041a214;
      }
      *(undefined4 **)((int)this + 4) = puVar5;
      *(undefined4 *)((int)this + 8) = *(undefined4 *)((int)param_1 + 8);
      *(undefined4 *)((int)this + 0xc) = *(undefined4 *)((int)param_1 + 0xc);
      *(char *)((int)puVar5 + -1) = *(char *)((int)puVar5 + -1) + '\x01';
      return this;
    }
  }
  uVar4 = FUN_00401b00();
  if (uVar4 < uVar6) {
    FUN_0040ef1b();
  }
  iVar7 = *(int *)((int)this + 4);
  if (((iVar7 == 0) || (cVar1 = *(char *)(iVar7 + -1), cVar1 == '\0')) || (cVar1 == -1)) {
    if (uVar6 == 0) {
      FUN_00401380(this,'\x01');
      return this;
    }
    if ((*(uint *)((int)this + 0xc) < 0x20) && (uVar6 <= *(uint *)((int)this + 0xc)))
    goto LAB_00401acc;
    FUN_00401380(this,'\x01');
  }
  else if (uVar6 == 0) {
    *(char *)(iVar7 + -1) = cVar1 + -1;
    FUN_00401380(this,'\0');
    return this;
  }
  FUN_00401690(uVar6);
LAB_00401acc:
  puVar5 = *(undefined4 **)((int)param_1 + 4);
  if (puVar5 == (undefined4 *)0x0) {
    puVar5 = &DAT_0041a214;
  }
  puVar5 = (undefined4 *)(param_2 + (int)puVar5);
  puVar8 = *(undefined4 **)((int)this + 4);
  for (uVar4 = uVar6 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *puVar8 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar8 = puVar8 + 1;
  }
  for (uVar4 = uVar6 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined1 *)puVar8 = *(undefined1 *)puVar5;
    puVar5 = (undefined4 *)((int)puVar5 + 1);
    puVar8 = (undefined4 *)((int)puVar8 + 1);
  }
  *(uint *)((int)this + 8) = uVar6;
  *(undefined1 *)(uVar6 + *(int *)((int)this + 4)) = 0;
  return this;
}



undefined4 FUN_00401b00(void)

{
  return 0xfffffffd;
}



void __fastcall FUN_00401b10(void *param_1)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  char *pcVar5;
  char *pcVar6;
  
  pcVar5 = *(char **)((int)param_1 + 4);
  if (pcVar5 == (char *)0x0) {
    return;
  }
  cVar1 = pcVar5[-1];
  if (cVar1 == '\0') {
    return;
  }
  if (cVar1 == -1) {
    return;
  }
  pcVar5[-1] = cVar1 + -1;
  uVar3 = 0xffffffff;
  *(undefined4 *)((int)param_1 + 4) = 0;
  *(undefined4 *)((int)param_1 + 8) = 0;
  *(undefined4 *)((int)param_1 + 0xc) = 0;
  pcVar6 = pcVar5;
  do {
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    cVar1 = *pcVar6;
    pcVar6 = pcVar6 + 1;
  } while (cVar1 != '\0');
  uVar3 = ~uVar3 - 1;
  if (0xfffffffd < uVar3) {
    FUN_0040ef1b();
  }
  iVar2 = *(int *)((int)param_1 + 4);
  if (((iVar2 == 0) || (cVar1 = *(char *)(iVar2 + -1), cVar1 == '\0')) || (cVar1 == -1)) {
    if (uVar3 == 0) {
      FUN_00401380(param_1,'\x01');
      return;
    }
    if ((*(uint *)((int)param_1 + 0xc) < 0x20) && (uVar3 <= *(uint *)((int)param_1 + 0xc)))
    goto LAB_00401bb7;
    FUN_00401380(param_1,'\x01');
  }
  else if (uVar3 == 0) {
    *(char *)(iVar2 + -1) = cVar1 + -1;
    FUN_00401380(param_1,'\0');
    return;
  }
  FUN_00401690(uVar3);
LAB_00401bb7:
  pcVar6 = *(char **)((int)param_1 + 4);
  for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined4 *)pcVar6 = *(undefined4 *)pcVar5;
    pcVar5 = pcVar5 + 4;
    pcVar6 = pcVar6 + 4;
  }
  for (uVar4 = uVar3 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar6 = *pcVar5;
    pcVar5 = pcVar5 + 1;
    pcVar6 = pcVar6 + 1;
  }
  *(uint *)((int)param_1 + 8) = uVar3;
  *(undefined1 *)(uVar3 + *(int *)((int)param_1 + 4)) = 0;
  return;
}



undefined1 * __cdecl FUN_00401be0(undefined1 *param_1,undefined1 *param_2,char *param_3)

{
  uint uVar1;
  char cVar2;
  undefined4 uVar3;
  uint uVar4;
  uint uVar5;
  char *pcVar6;
  undefined1 local_1c [4];
  int local_18;
  uint local_14;
  undefined4 local_10;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004192d8;
  pvStack_c = ExceptionList;
  local_1c[0] = *param_2;
  local_18 = 0;
  local_14 = 0;
  local_10 = 0;
  ExceptionList = &pvStack_c;
  FUN_00401900(local_1c,param_2,0,0xffffffff);
  uVar4 = 0xffffffff;
  pcVar6 = param_3;
  do {
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    cVar2 = *pcVar6;
    pcVar6 = pcVar6 + 1;
  } while (cVar2 != '\0');
  uVar4 = ~uVar4 - 1;
  local_4 = 0;
  if (-local_14 - 1 <= uVar4) {
    FUN_0040ef1b();
  }
  if (uVar4 != 0) {
    uVar1 = local_14 + uVar4;
    uVar3 = FUN_00401570(local_1c,uVar1,'\0');
    if ((char)uVar3 != '\0') {
      pcVar6 = (char *)(local_14 + local_18);
      for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined4 *)pcVar6 = *(undefined4 *)param_3;
        param_3 = param_3 + 4;
        pcVar6 = pcVar6 + 4;
      }
      for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *pcVar6 = *param_3;
        param_3 = param_3 + 1;
        pcVar6 = pcVar6 + 1;
      }
      *(undefined1 *)(local_18 + uVar1) = 0;
      local_14 = uVar1;
    }
  }
  *param_1 = local_1c[0];
  *(undefined4 *)(param_1 + 4) = 0;
  *(undefined4 *)(param_1 + 8) = 0;
  *(undefined4 *)(param_1 + 0xc) = 0;
  FUN_00401900(param_1,local_1c,0,0xffffffff);
  if (local_18 != 0) {
    cVar2 = *(char *)(local_18 + -1);
    if ((cVar2 != '\0') && (cVar2 != -1)) {
      *(char *)(local_18 + -1) = cVar2 + -1;
      ExceptionList = pvStack_c;
      return param_1;
    }
    FUN_0040ef10((char *)(local_18 + -1));
  }
  ExceptionList = pvStack_c;
  return param_1;
}



void FUN_00401d40(void)

{
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_00401d60(void)

{
  char cVar1;
  HCURSOR pHVar2;
  uint uVar3;
  uint uVar4;
  char *pcVar5;
  char *pcVar6;
  WNDCLASSA local_28;
  
  pHVar2 = LoadCursorA((HINSTANCE)0x0,&DAT_00007f00);
  local_28.hInstance = GetModuleHandleA((LPCSTR)0x0);
  _DAT_00423994 = &DAT_0041a21c;
  _DAT_00423990 = &DAT_0041a218;
  local_28.style = 8;
  local_28.lpfnWndProc = FUN_00402300;
  local_28.cbClsExtra = 0;
  local_28.cbWndExtra = 0;
  local_28.hIcon = (HICON)0x0;
  local_28.hbrBackground = (HBRUSH)0x5;
  local_28.lpszMenuName = (LPCSTR)0x0;
  local_28.lpszClassName = s_WC_TENHOU_WIN_APP_LAUNCHER_00420084;
  local_28.hCursor = pHVar2;
  RegisterClassA(&local_28);
  uVar3 = 0xffffffff;
  pcVar5 = s_WC_TENHOU_WIN_APP_LAUNCHER_00420084;
  do {
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    cVar1 = *pcVar5;
    pcVar5 = pcVar5 + 1;
  } while (cVar1 != '\0');
  DAT_00423998 = operator_new(~uVar3);
  uVar3 = 0xffffffff;
  pcVar5 = s_WC_TENHOU_WIN_APP_LAUNCHER_00420084;
  do {
    pcVar6 = pcVar5;
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    pcVar6 = pcVar5 + 1;
    cVar1 = *pcVar5;
    pcVar5 = pcVar6;
  } while (cVar1 != '\0');
  uVar3 = ~uVar3;
  pcVar5 = pcVar6 + -uVar3;
  pcVar6 = DAT_00423998;
  for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined4 *)pcVar6 = *(undefined4 *)pcVar5;
    pcVar5 = pcVar5 + 4;
    pcVar6 = pcVar6 + 4;
  }
  for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *pcVar6 = *pcVar5;
    pcVar5 = pcVar5 + 1;
    pcVar6 = pcVar6 + 1;
  }
  return;
}



void FUN_00401e20(void)

{
  FUN_0040fac3(&LAB_00401e30);
  return;
}



undefined4 FUN_00401e50(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  HWND pHVar1;
  int iVar2;
  undefined4 uVar3;
  HMODULE hInstance;
  LPVOID lpParam;
  tagMSG local_1c;
  
  DAT_0042399c = param_3;
  FUN_00402040((byte *)s______update______004200f0);
  FUN_00402040((byte *)s_lpCmdLine__s_004200e0);
  pHVar1 = FindWindowA(s_WC_TENHOU_WIN_APP_LAUNCHER_00420084,(LPCSTR)0x0);
  if (pHVar1 != (HWND)0x0) {
    ShowWindow(pHVar1,1);
    SetForegroundWindow(pHVar1);
    return 0;
  }
  iVar2 = FUN_004037e0();
  if (iVar2 != 0) {
    return 0;
  }
  iVar2 = FUN_00403860();
  if (iVar2 != 0) {
    return 0;
  }
  uVar3 = FUN_00404990();
  if ((char)uVar3 == '\0') {
    return 0;
  }
  lpParam = (LPVOID)0x0;
  hInstance = GetModuleHandleA((LPCSTR)0x0);
  pHVar1 = CreateWindowExA(0x88,s_WC_TENHOU_WIN_APP_LAUNCHER_00420084,&DAT_004200cc,0x80c80000,0,0,0
                           ,0,(HWND)0x0,(HMENU)0x0,hInstance,lpParam);
  if (pHVar1 == (HWND)0x0) {
    MessageBoxA((HWND)0x0,s_Create_WC__LAUNCHER_failed_004200b0,&DAT_004200cc,0);
    return 1;
  }
  iVar2 = GetMessageA(&local_1c,(HWND)0x0,0,0);
  while (iVar2 != 0) {
    TranslateMessage(&local_1c);
    DispatchMessageA(&local_1c);
    iVar2 = GetMessageA(&local_1c,(HWND)0x0,0,0);
  }
  FUN_00402040((byte *)s_EXIT_LAUNCHER_004200a0);
  return local_1c.wParam;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_00401fa0(void)

{
  int iVar1;
  WSADATA local_190;
  
  _DAT_00423988 = &PTR_FUN_0041a224;
  iVar1 = WSAStartup(0x101,&local_190);
  if (iVar1 != 0) {
    MessageBoxA((HWND)0x0,s_WSAStartup_0x101__failed_00420104,s_use_wsock32_h_00420120,0x10);
  }
  return;
}



void FUN_00401fe0(void)

{
  FUN_0040fac3(&LAB_00401ff0);
  return;
}



undefined4 * __thiscall FUN_00402010(void *this,byte param_1)

{
  *(undefined ***)this = &PTR_FUN_0041a224;
  WSACancelBlockingCall();
  WSACleanup();
  if ((param_1 & 1) != 0) {
    FUN_0040ef10(this);
  }
  return this;
}



undefined4 __cdecl FUN_00402040(byte *param_1)

{
  int iVar1;
  char local_400 [1023];
  undefined1 local_1;
  
  if (param_1 == (byte *)0x0) {
    return 0;
  }
  if ((DAT_00423d88 & 1) == 0) {
    DAT_00423d88 = DAT_00423d88 | 1;
    FUN_00402100((undefined4 *)&DAT_00423d90);
    FUN_0040fac3(&LAB_00402250);
  }
  WaitForSingleObject(DAT_00423d94,0xffffffff);
  iVar1 = FUN_0040fb04(local_400,0x400,param_1,(undefined4 *)&stack0x00000008);
  local_1 = 0;
  if (iVar1 == -1) {
    iVar1 = 0x3ff;
  }
  else if (iVar1 == 0) goto LAB_004020e3;
  if (DAT_00423d9c != 0xffffffff) {
    send(DAT_00423d9c,local_400,iVar1 + 1,0);
  }
LAB_004020e3:
  SetEvent(DAT_00423d94);
  return 0;
}



undefined4 * __fastcall FUN_00402100(undefined4 *param_1)

{
  SOCKET SVar1;
  int iVar2;
  HANDLE local_14;
  sockaddr local_10;
  
  local_14 = CreateEventA((LPSECURITY_ATTRIBUTES)0x0,0,1,(LPCSTR)0x0);
  param_1[1] = local_14;
  *param_1 = &PTR_FUN_0041a234;
  if (local_14 == (HANDLE)0x0) {
                    // WARNING: Subroutine does not return
    __CxxThrowException_8(&local_14,&DAT_0041e508);
  }
  SVar1 = socket(2,2,0);
  param_1[3] = SVar1;
  param_1[2] = &PTR_FUN_0041a230;
  if (SVar1 != 0xffffffff) {
    local_10.sa_family = 2;
    local_10.sa_data._0_2_ = htons(0x202);
    local_10.sa_data[6] = '\0';
    local_10.sa_data[7] = '\0';
    local_10.sa_data[8] = '\0';
    local_10.sa_data[9] = '\0';
    local_10.sa_data[10] = '\0';
    local_10.sa_data[2] = '\0';
    local_10.sa_data[3] = '\0';
    local_10.sa_data[4] = '\0';
    local_10.sa_data[5] = '\0';
    local_10.sa_data[0xb] = '\0';
    local_10.sa_data[0xc] = '\0';
    local_10.sa_data[0xd] = '\0';
    local_10.sa_data._2_4_ = htonl(0x7f000001);
    iVar2 = connect(param_1[3],&local_10,0x10);
    if (iVar2 == -1) {
      closesocket(param_1[3]);
      param_1[3] = 0xffffffff;
    }
  }
  *param_1 = &PTR_FUN_0041a22c;
  param_1[2] = &PTR_LAB_0041a228;
  return param_1;
}



undefined4 * __thiscall FUN_004021d0(void *this,byte param_1)

{
  *(undefined ***)this = &PTR_FUN_0041a234;
  WaitForSingleObject(*(HANDLE *)((int)this + 4),0xffffffff);
  CloseHandle(*(HANDLE *)((int)this + 4));
  *(undefined4 *)((int)this + 4) = 0;
  if ((param_1 & 1) != 0) {
    FUN_0040ef10(this);
  }
  return this;
}



undefined4 * __thiscall FUN_00402210(void *this,byte param_1)

{
  *(undefined ***)this = &PTR_FUN_0041a230;
  if (*(SOCKET *)((int)this + 4) != 0xffffffff) {
    closesocket(*(SOCKET *)((int)this + 4));
    *(undefined4 *)((int)this + 4) = 0xffffffff;
  }
  if ((param_1 & 1) != 0) {
    FUN_0040ef10(this);
  }
  return this;
}



void __fastcall FUN_00402260(undefined4 *param_1)

{
  undefined4 *puVar1;
  
  if (param_1 == (undefined4 *)0x0) {
    puVar1 = (undefined4 *)0x0;
  }
  else {
    puVar1 = param_1 + 2;
  }
  *puVar1 = &PTR_FUN_0041a230;
  if (puVar1[1] != 0xffffffff) {
    closesocket(puVar1[1]);
    puVar1[1] = 0xffffffff;
  }
  *param_1 = &PTR_FUN_0041a234;
  WaitForSingleObject((HANDLE)param_1[1],0xffffffff);
  CloseHandle((HANDLE)param_1[1]);
  param_1[1] = 0;
  return;
}



undefined4 * __thiscall FUN_004022b0(void *this,byte param_1)

{
  FUN_00402260(this);
  if ((param_1 & 1) != 0) {
    FUN_0040ef10(this);
  }
  return this;
}



undefined4 * __thiscall FUN_004022d0(void *this,byte param_1)

{
  *(undefined1 **)this = &DAT_0041a218;
  if (*(undefined **)((int)this + 8) != (undefined *)0x0) {
    FUN_0040ef10(*(undefined **)((int)this + 8));
  }
  if ((param_1 & 1) != 0) {
    FUN_0040ef10(this);
  }
  return this;
}



LRESULT FUN_00402300(HWND param_1,UINT param_2,WPARAM param_3,LPARAM param_4)

{
  int *hData;
  LRESULT LVar1;
  
  hData = GetPropA(param_1,&DAT_0042013c);
  if (hData == (int *)0x0) {
    if (param_2 == 1) {
      hData = operator_new(0x10);
      if (hData == (int *)0x0) {
        hData = (int *)0x0;
      }
      else {
        hData[1] = -1;
        *(undefined1 *)(hData + 2) = 1;
        hData[3] = 0;
        *hData = (int)&PTR_FUN_0041a238;
      }
      SetPropA(param_1,&DAT_0042013c,hData);
      if (hData != (int *)0x0) goto LAB_00402376;
    }
    LVar1 = DefWindowProcA(param_1,param_2,param_3,param_4);
    return LVar1;
  }
LAB_00402376:
  LVar1 = (**(code **)*hData)(param_1,param_2,param_3,param_4);
  if (param_2 == 2) {
    if (hData != (int *)0x0) {
      (**(code **)(*hData + 4))(1);
    }
    RemovePropA(param_1,&DAT_0042013c);
  }
  return LVar1;
}



LRESULT __thiscall FUN_004023c0(void *this,HWND param_1,uint param_2,WPARAM param_3,int param_4)

{
  HMODULE pHVar1;
  HANDLE pvVar2;
  LRESULT LVar3;
  int nHeight;
  LPVOID pvVar4;
  tagRECT local_20;
  tagRECT local_10;
  
  if (param_2 < 0x102) {
    if (param_2 == 0x101) {
      if (param_3 == 0x1b) {
        PostMessageA(param_1,0x10,0,0);
      }
    }
    else if (param_2 == 1) {
      *(HWND *)((int)this + 0xc) = param_1;
      SetTimer(param_1,100,1000,(TIMERPROC)0x0);
      local_10.left = 0;
      local_10.top = 0;
      local_10.right = 0;
      local_10.bottom = 0;
      local_20.left = 0;
      local_20.top = 0;
      local_20.right = 0x140;
      local_20.bottom = 0x20;
      SystemParametersInfoA(0x30,0,&local_10,0);
      AdjustWindowRectEx(&local_20,*(DWORD *)(param_4 + 0x20),(uint)(*(int *)(param_4 + 8) != 0),
                         *(DWORD *)(param_4 + 0x2c));
      SetWindowPos(param_1,(HWND)0x0,
                   (((local_20.left - local_20.right) - local_10.left) + local_10.right) / 2 +
                   local_10.left,
                   (((local_20.top - local_20.bottom) - local_10.top) + local_10.bottom) / 2 +
                   local_10.top,local_20.right - local_20.left,local_20.bottom - local_20.top,0x14);
      GetClientRect(param_1,&local_10);
      pvVar4 = (LPVOID)0x0;
      nHeight = (local_10.bottom - local_10.top) / 2;
      pHVar1 = GetModuleHandleA((LPCSTR)0x0);
      CreateWindowExA(0,s_msctls_progress32_00420144,(LPCSTR)0x0,0x50000001,0,0,
                      local_10.right - local_10.left,nHeight,param_1,(HMENU)0xbb9,pHVar1,pvVar4);
      pvVar4 = (LPVOID)0x0;
      pHVar1 = GetModuleHandleA((LPCSTR)0x0);
      CreateWindowExA(0,s_msctls_progress32_00420144,(LPCSTR)0x0,0x50000001,0,nHeight,
                      local_10.right - local_10.left,nHeight,param_1,(HMENU)0xbba,pHVar1,pvVar4);
      pvVar2 = FUN_0040fbd3(0x4025b0,0,(int)this);
      *(HANDLE *)((int)this + 4) = pvVar2;
    }
    else if (param_2 == 2) {
      PostQuitMessage(0);
      return 0;
    }
  }
  else if ((param_2 == 0x113) && (param_3 == 100)) {
    KillTimer(param_1,100);
    ShowWindow(param_1,5);
  }
  LVar3 = DefWindowProcA(param_1,param_2,param_3,param_4);
  return LVar3;
}



undefined4 __fastcall FUN_004025c0(HWND param_1)

{
  char cVar1;
  HWND__ HVar2;
  char *pcVar3;
  undefined4 *puVar4;
  int iVar5;
  undefined1 *puVar6;
  HWND pHVar7;
  HMODULE hLibModule;
  byte *pbVar8;
  uint uVar9;
  void *this;
  void *this_00;
  void *this_01;
  WPARAM wParam;
  byte *lpLibFileName;
  HWND pHVar10;
  int *piVar11;
  int *piVar12;
  undefined1 local_175;
  HWND local_174;
  HWND local_170;
  HWND local_16c;
  undefined1 local_168 [4];
  HWND__ local_164;
  LPARAM local_15c;
  undefined1 local_155;
  undefined4 local_154;
  undefined1 local_150 [16];
  undefined1 local_140 [16];
  undefined1 local_130 [4];
  undefined4 *local_12c;
  undefined1 local_120 [16];
  byte local_110;
  undefined4 local_10f;
  void *local_c;
  undefined1 *puStack_8;
  int local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_00419327;
  local_c = ExceptionList;
  wParam = 0;
  ExceptionList = &local_c;
  local_170 = param_1;
  FUN_00402c90(local_168,&local_175,0,&local_155);
  local_4 = 0;
  local_150[0] = local_175;
  FUN_00401380(local_150,'\0');
  uVar9 = 0xffffffff;
  pcVar3 = &DAT_004201a0;
  do {
    if (uVar9 == 0) break;
    uVar9 = uVar9 - 1;
    cVar1 = *pcVar3;
    pcVar3 = pcVar3 + 1;
  } while (cVar1 != '\0');
  FUN_004014b0(local_150,(undefined4 *)&DAT_004201a0,~uVar9 - 1);
  puVar6 = local_150;
  local_4._0_1_ = 1;
  pcVar3 = (char *)FUN_00403ab0();
  FUN_004017a0(local_130,pcVar3,(int)puVar6);
  local_4._0_1_ = 3;
  FUN_00401380(local_150,'\x01');
  local_140[0] = local_175;
  FUN_00401380(local_140,'\0');
  local_4._0_1_ = 4;
  while( true ) {
    puVar4 = local_12c;
    if (local_12c == (undefined4 *)0x0) {
      puVar4 = &DAT_0041a214;
    }
    iVar5 = FUN_00403ac0(puVar4,local_140);
    if ((iVar5 == 0) || ((char)param_1[2].unused == '\0')) break;
    FUN_00405c10(10000,(char *)(param_1 + 2));
  }
  puVar6 = FUN_0040dc30(local_120,local_140,&DAT_00420198,&DAT_0042019c);
  local_4._0_1_ = 5;
  FUN_00402cc0(local_168,puVar6);
  local_4._0_1_ = 4;
  FUN_00402ba0(local_120);
  local_4._0_1_ = 3;
  FUN_00401380(local_140,'\x01');
  local_4 = (uint)local_4._1_3_ << 8;
  FUN_00401380(local_130,'\x01');
  local_174 = (HWND)*(int *)local_164.unused;
  if (local_174 != (HWND)local_164.unused) {
    do {
      pbVar8 = (byte *)&DAT_0041a214;
      if ((byte *)local_174[4].unused != (byte *)0x0) {
        pbVar8 = (byte *)local_174[4].unused;
      }
      local_110 = 0;
      puVar4 = &local_10f;
      for (iVar5 = 0x40; iVar5 != 0; iVar5 = iVar5 + -1) {
        *puVar4 = 0;
        puVar4 = puVar4 + 1;
      }
      *(undefined2 *)puVar4 = 0;
      *(undefined1 *)((int)puVar4 + 2) = 0;
      FUN_0040fffb(pbVar8,(byte *)0x0,(byte *)0x0,(byte *)0x0,&local_110);
      uVar9 = FUN_0040fe66(this,&local_110,&DAT_00420190);
      if ((uVar9 == 0) || (uVar9 = FUN_0040fe66(this_00,&local_110,&DAT_00420188), uVar9 == 0)) {
        FUN_004032b0((int *)&local_174);
      }
      else {
        FUN_00402e50(local_168,&local_16c,&local_174->unused);
        local_174 = local_16c;
      }
    } while (local_174 != (HWND)local_164.unused);
  }
  pHVar7 = GetDlgItem((HWND)param_1[3].unused,0xbb9);
  local_16c = pHVar7;
  local_174 = GetDlgItem((HWND)param_1[3].unused,0xbba);
  PostMessageA(pHVar7,0x406,0,local_15c);
  cVar1 = (char)param_1[2].unused;
  while (pHVar10 = param_1 + 2, cVar1 != '\0') {
    if (local_15c == 0) goto LAB_004028f5;
    wParam = wParam + 1;
    PostMessageA(pHVar7,0x402,wParam,0);
    pbVar8 = *(byte **)(*(int *)local_164.unused + 0x10);
    lpLibFileName = (byte *)&DAT_0041a214;
    if (pbVar8 != (byte *)0x0) {
      lpLibFileName = pbVar8;
    }
    local_110 = 0;
    puVar4 = &local_10f;
    for (iVar5 = 0x40; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    *(undefined2 *)puVar4 = 0;
    *(undefined1 *)((int)puVar4 + 2) = 0;
    FUN_0040fffb(lpLibFileName,(byte *)0x0,(byte *)0x0,(byte *)0x0,&local_110);
    uVar9 = FUN_0040fe66(this_01,&local_110,&DAT_00420188);
    if ((uVar9 == 0) &&
       (hLibModule = LoadLibraryA((LPCSTR)lpLibFileName), param_1 = local_170,
       hLibModule != (HMODULE)0x0)) {
      FreeLibrary(hLibModule);
      param_1 = local_170;
    }
    else {
      pbVar8 = *(byte **)(*(int *)local_164.unused + 0x20);
      if (pbVar8 == (byte *)0x0) {
        pbVar8 = (byte *)&DAT_0041a214;
      }
      pHVar10 = param_1 + 2;
      iVar5 = FUN_00405c50((HWND)param_1[3].unused,local_174,(char *)pHVar10,(char *)lpLibFileName,
                           pbVar8);
      if (iVar5 != 0) break;
    }
    FUN_00402e50(local_168,&local_154,(int *)*(int *)local_164.unused);
    pHVar7 = local_16c;
    cVar1 = (char)param_1[2].unused;
  }
  if (local_15c == 0) {
LAB_004028f5:
    if ((char)pHVar10->unused != '\0') {
      WritePrivateProfileStringA
                (s_STATUS_00420164,s_LAUNCHER_0042016c,DAT_00423a0c,s___config_ini_00420178);
      WritePrivateProfileStringA((LPCSTR)0x0,(LPCSTR)0x0,(LPCSTR)0x0,s___config_ini_00420178);
      FUN_00401000(s_tenhou_exe_00420158,DAT_0042399c);
    }
  }
  HVar2.unused = local_164.unused;
  param_1[1].unused = -1;
  local_4 = 0xffffffff;
  local_170 = (HWND)*(int *)local_164.unused;
  if (local_15c == 0) {
    local_15c = 0;
    pHVar7 = local_170;
    while (local_170 = pHVar7, pHVar7 != (HWND)HVar2.unused) {
      FUN_004032b0((int *)&local_170);
      FUN_00402e50(local_168,&local_174,&pHVar7->unused);
      pHVar7 = local_170;
    }
  }
  else {
    piVar11 = (int *)((HWND)(local_164.unused + 4))->unused;
    if ((int *)((HWND)(local_164.unused + 4))->unused != DAT_004239a4) {
      do {
        FUN_00403300((int *)piVar11[2]);
        piVar12 = (int *)*piVar11;
        FUN_00403710(piVar11 + 3,0);
        FUN_0040ef10((undefined *)piVar11);
        piVar11 = piVar12;
      } while (piVar12 != DAT_004239a4);
    }
    *(int **)(local_164.unused + 4) = DAT_004239a4;
    local_15c = 0;
    *(int *)local_164.unused = local_164.unused;
    *(int *)(local_164.unused + 8) = local_164.unused;
    FUN_00402e40(local_168,&local_154);
  }
  FUN_0040ef10((undefined *)local_164.unused);
  local_164.unused = 0;
  local_15c = 0;
  FUN_0040f272(&local_16c);
  piVar11 = DAT_004239a4;
  DAT_004239a0 = DAT_004239a0 + -1;
  piVar12 = (int *)0x0;
  if (DAT_004239a0 == 0) {
    DAT_004239a4 = (int *)0x0;
    piVar12 = piVar11;
  }
  FUN_0040f30e();
  if (piVar12 != (int *)0x0) {
    FUN_0040ef10((undefined *)piVar12);
  }
  ExceptionList = local_c;
  return 0;
}



undefined4 * __thiscall FUN_00402a60(void *this,byte param_1)

{
  *(undefined ***)this = &PTR_FUN_0041a238;
  *(undefined1 *)((int)this + 8) = 0;
  if (*(HANDLE *)((int)this + 4) != (HANDLE)0xffffffff) {
    WaitForSingleObject(*(HANDLE *)((int)this + 4),0xffffffff);
    *(undefined4 *)((int)this + 4) = 0xffffffff;
  }
  if ((param_1 & 1) != 0) {
    FUN_0040ef10(this);
  }
  return this;
}



void __fastcall FUN_00402aa0(void *param_1)

{
  int *piVar1;
  int *piVar2;
  int *local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  piVar1 = *(int **)((int)param_1 + 4);
  local_c = (int *)*piVar1;
  piVar2 = local_c;
  if (*(int *)((int)param_1 + 0xc) == 0) {
    while (local_c = piVar2, piVar2 != piVar1) {
      FUN_004032b0((int *)&local_c);
      FUN_00402e50(param_1,&local_4,piVar2);
      piVar2 = local_c;
    }
  }
  else {
    piVar2 = (int *)piVar1[1];
    if ((int *)piVar1[1] != DAT_004239a4) {
      do {
        FUN_00403300((int *)piVar2[2]);
        piVar1 = (int *)*piVar2;
        FUN_00403710(piVar2 + 3,0);
        FUN_0040ef10((undefined *)piVar2);
        piVar2 = piVar1;
      } while (piVar1 != DAT_004239a4);
    }
    *(int **)(*(int *)((int)param_1 + 4) + 4) = DAT_004239a4;
    *(undefined4 *)((int)param_1 + 0xc) = 0;
    *(undefined4 *)*(undefined4 *)((int)param_1 + 4) = *(undefined4 *)((int)param_1 + 4);
    *(int *)(*(int *)((int)param_1 + 4) + 8) = *(int *)((int)param_1 + 4);
    FUN_00402e40(param_1,&local_8);
  }
  FUN_0040ef10(*(undefined **)((int)param_1 + 4));
  *(undefined4 *)((int)param_1 + 4) = 0;
  *(undefined4 *)((int)param_1 + 0xc) = 0;
  FUN_0040f272(&local_c);
  piVar1 = DAT_004239a4;
  DAT_004239a0 = DAT_004239a0 + -1;
  piVar2 = (int *)0x0;
  if (DAT_004239a0 == 0) {
    DAT_004239a4 = (int *)0x0;
    piVar2 = piVar1;
  }
  FUN_0040f30e();
  if (piVar2 != (int *)0x0) {
    FUN_0040ef10((undefined *)piVar2);
  }
  return;
}



void __fastcall FUN_00402ba0(void *param_1)

{
  int *piVar1;
  int *piVar2;
  int *local_8;
  undefined4 local_4;
  
  piVar1 = *(int **)((int)param_1 + 4);
  local_8 = (int *)*piVar1;
  piVar2 = local_8;
  if (*(int *)((int)param_1 + 0xc) == 0) {
    while (local_8 = piVar2, piVar2 != piVar1) {
      FUN_004032b0((int *)&local_8);
      FUN_00402e50(param_1,&local_4,piVar2);
      piVar2 = local_8;
    }
  }
  else {
    piVar2 = (int *)piVar1[1];
    if ((int *)piVar1[1] != DAT_004239a4) {
      do {
        FUN_00403300((int *)piVar2[2]);
        piVar1 = (int *)*piVar2;
        FUN_00403710(piVar2 + 3,0);
        FUN_0040ef10((undefined *)piVar2);
        piVar2 = piVar1;
      } while (piVar1 != DAT_004239a4);
    }
    *(int **)(*(int *)((int)param_1 + 4) + 4) = DAT_004239a4;
    *(undefined4 *)((int)param_1 + 0xc) = 0;
    *(undefined4 *)*(undefined4 *)((int)param_1 + 4) = *(undefined4 *)((int)param_1 + 4);
    *(int *)(*(int *)((int)param_1 + 4) + 8) = *(int *)((int)param_1 + 4);
  }
  FUN_0040ef10(*(undefined **)((int)param_1 + 4));
  *(undefined4 *)((int)param_1 + 4) = 0;
  *(undefined4 *)((int)param_1 + 0xc) = 0;
  FUN_0040f272(&local_8);
  piVar1 = DAT_004239a4;
  DAT_004239a0 = DAT_004239a0 + -1;
  piVar2 = (int *)0x0;
  if (DAT_004239a0 == 0) {
    DAT_004239a4 = (int *)0x0;
    piVar2 = piVar1;
  }
  FUN_0040f30e();
  if (piVar2 != (int *)0x0) {
    FUN_0040ef10((undefined *)piVar2);
  }
  return;
}



undefined1 * __thiscall
FUN_00402c90(void *this,undefined1 *param_1,undefined1 param_2,undefined1 *param_3)

{
  undefined1 uVar1;
  
  *(undefined1 *)this = *param_3;
  uVar1 = *param_1;
  *(undefined1 *)((int)this + 8) = param_2;
  *(undefined1 *)((int)this + 1) = uVar1;
  FUN_00403350((int)this);
  return this;
}



void * __thiscall FUN_00402cc0(void *this,void *param_1)

{
  undefined4 *puVar1;
  int *piVar2;
  int *piVar3;
  int *piVar4;
  int *piVar5;
  int *local_8;
  undefined4 local_4;
  
  if (this != param_1) {
    piVar2 = *(int **)((int)this + 4);
    local_8 = (int *)*piVar2;
    piVar5 = local_8;
    if (*(int *)((int)this + 0xc) == 0) {
      while (piVar5 != piVar2) {
        local_8 = piVar5;
        FUN_004032b0((int *)&local_8);
        FUN_00402e50(this,&local_4,piVar5);
        piVar5 = local_8;
      }
    }
    else {
      piVar5 = (int *)piVar2[1];
      if ((int *)piVar2[1] != DAT_004239a4) {
        do {
          FUN_00403300((int *)piVar5[2]);
          piVar2 = (int *)*piVar5;
          FUN_00403710(piVar5 + 3,0);
          FUN_0040ef10((undefined *)piVar5);
          piVar5 = piVar2;
        } while (piVar2 != DAT_004239a4);
      }
      *(int **)(*(int *)((int)this + 4) + 4) = DAT_004239a4;
      *(undefined4 *)((int)this + 0xc) = 0;
      *(undefined4 *)*(undefined4 *)((int)this + 4) = *(undefined4 *)((int)this + 4);
      *(int *)(*(int *)((int)this + 4) + 8) = *(int *)((int)this + 4);
    }
    piVar2 = *(int **)((int)this + 4);
    piVar5 = *(int **)(*(int *)((int)param_1 + 4) + 4);
    piVar4 = piVar2;
    local_8 = piVar5;
    if (piVar5 != DAT_004239a4) {
      do {
        piVar2 = (int *)FUN_00403480(piVar4,piVar5[0xb]);
        if (local_8 == piVar5) {
          local_8 = piVar2;
        }
        piVar3 = FUN_004033f0((int *)piVar5[2],piVar2);
        piVar2[2] = (int)piVar3;
        FUN_004034a0((undefined1 *)(piVar2 + 3),(undefined1 *)(piVar5 + 3));
        *piVar4 = (int)piVar2;
        piVar5 = (int *)*piVar5;
        piVar4 = piVar2;
      } while (piVar5 != DAT_004239a4);
    }
    *piVar2 = (int)DAT_004239a4;
    *(int **)(*(int *)((int)this + 4) + 4) = local_8;
    puVar1 = *(undefined4 **)((int)this + 4);
    *(undefined4 *)((int)this + 0xc) = *(undefined4 *)((int)param_1 + 0xc);
    piVar2 = (int *)puVar1[1];
    if (piVar2 != DAT_004239a4) {
      for (piVar5 = (int *)*piVar2; piVar5 != DAT_004239a4; piVar5 = (int *)*piVar5) {
        piVar2 = piVar5;
      }
      *puVar1 = piVar2;
      piVar2 = *(int **)(*(int *)((int)this + 4) + 4);
      for (piVar5 = (int *)piVar2[2]; piVar5 != DAT_004239a4; piVar5 = (int *)piVar5[2]) {
        piVar2 = piVar5;
      }
      *(int **)(*(int *)((int)this + 4) + 8) = piVar2;
      return this;
    }
    *puVar1 = puVar1;
    *(int *)(*(int *)((int)this + 4) + 8) = *(int *)((int)this + 4);
  }
  return this;
}



void __thiscall FUN_00402e40(void *this,undefined4 *param_1)

{
  *param_1 = **(undefined4 **)((int)this + 4);
  return;
}



void __thiscall FUN_00402e50(void *this,undefined4 *param_1,int *param_2)

{
  char cVar1;
  int *piVar2;
  int iVar3;
  int *piVar4;
  undefined4 *puVar5;
  int *piVar6;
  int *piVar7;
  undefined4 *puVar8;
  undefined4 *puVar9;
  int *piVar10;
  int *piVar11;
  int *local_c;
  undefined1 local_4 [4];
  
  piVar4 = param_2;
  FUN_004032b0((int *)&param_2);
  piVar11 = (int *)*piVar4;
  piVar7 = piVar4 + 2;
  local_c = piVar4;
  piVar10 = piVar7;
  if (piVar11 == DAT_004239a4) {
    piVar11 = (int *)*piVar7;
  }
  else {
    piVar2 = (int *)*piVar7;
    if (piVar2 != DAT_004239a4) {
      local_c = piVar2;
      for (piVar10 = (int *)*piVar2; piVar10 != DAT_004239a4; piVar10 = (int *)*piVar10) {
        local_c = piVar10;
      }
      piVar11 = (int *)local_c[2];
      piVar10 = local_c + 2;
    }
  }
  piVar2 = local_c;
  FUN_0040f272(local_4);
  if (local_c == piVar4) {
    piVar11[1] = local_c[1];
    if (*(int **)(*(int *)((int)this + 4) + 4) == piVar4) {
      *(int **)(*(int *)((int)this + 4) + 4) = piVar11;
    }
    else {
      piVar10 = (int *)piVar4[1];
      if ((int *)*piVar10 == piVar4) {
        *piVar10 = (int)piVar11;
      }
      else {
        piVar10[2] = (int)piVar11;
      }
    }
    piVar10 = *(int **)((int)this + 4);
    if ((int *)*piVar10 == piVar4) {
      if ((int *)*piVar7 == DAT_004239a4) {
        *piVar10 = piVar4[1];
      }
      else {
        piVar2 = (int *)*piVar11;
        piVar7 = piVar11;
        while (piVar6 = piVar2, piVar6 != DAT_004239a4) {
          piVar7 = piVar6;
          piVar2 = (int *)*piVar6;
        }
        *piVar10 = (int)piVar7;
      }
    }
    if (*(int **)(*(int *)((int)this + 4) + 8) == piVar4) {
      if ((int *)*piVar4 == DAT_004239a4) {
        piVar7 = (int *)piVar4[1];
      }
      else {
        piVar10 = (int *)piVar11[2];
        piVar7 = piVar11;
        while (piVar4 = piVar10, piVar4 != DAT_004239a4) {
          piVar7 = piVar4;
          piVar10 = (int *)piVar4[2];
        }
      }
      *(int **)(*(int *)((int)this + 4) + 8) = piVar7;
    }
  }
  else {
    *(int **)(*piVar4 + 4) = local_c;
    *local_c = *piVar4;
    if (local_c == (int *)*piVar7) {
      piVar11[1] = (int)local_c;
    }
    else {
      piVar11[1] = local_c[1];
      *(int **)local_c[1] = piVar11;
      *piVar10 = *piVar7;
      *(int **)(*piVar7 + 4) = local_c;
    }
    if (*(int **)(*(int *)((int)this + 4) + 4) == piVar4) {
      *(int **)(*(int *)((int)this + 4) + 4) = local_c;
    }
    else {
      piVar7 = (int *)piVar4[1];
      if ((int *)*piVar7 == piVar4) {
        *piVar7 = (int)local_c;
      }
      else {
        piVar7[2] = (int)local_c;
      }
    }
    local_c = piVar4;
    piVar2[1] = piVar4[1];
    iVar3 = piVar2[0xb];
    piVar2[0xb] = piVar4[0xb];
    piVar4[0xb] = iVar3;
  }
  if (local_c[0xb] == 1) {
    if (piVar11 != *(int **)(*(int *)((int)this + 4) + 4)) {
      do {
        if (piVar11[0xb] != 1) break;
        piVar7 = *(int **)piVar11[1];
        if (piVar11 == piVar7) {
          piVar7 = (int *)((undefined4 *)piVar11[1])[2];
          if (piVar7[0xb] == 0) {
            piVar7[0xb] = 1;
            *(undefined4 *)(piVar11[1] + 0x2c) = 0;
            iVar3 = piVar11[1];
            piVar7 = *(int **)(iVar3 + 8);
            *(int *)(iVar3 + 8) = *piVar7;
            if ((int *)*piVar7 != DAT_004239a4) {
              ((int *)*piVar7)[1] = iVar3;
            }
            piVar7[1] = *(int *)(iVar3 + 4);
            if (iVar3 == *(int *)(*(int *)((int)this + 4) + 4)) {
              *(int **)(*(int *)((int)this + 4) + 4) = piVar7;
            }
            else {
              piVar10 = *(int **)(iVar3 + 4);
              if (iVar3 == *piVar10) {
                *piVar10 = (int)piVar7;
              }
              else {
                piVar10[2] = (int)piVar7;
              }
            }
            *piVar7 = iVar3;
            *(int **)(iVar3 + 4) = piVar7;
            piVar7 = *(int **)(piVar11[1] + 8);
          }
          if ((*(int *)(*piVar7 + 0x2c) != 1) || (*(int *)(piVar7[2] + 0x2c) != 1)) {
            if (*(int *)(piVar7[2] + 0x2c) == 1) {
              *(undefined4 *)(*piVar7 + 0x2c) = 1;
              iVar3 = *piVar7;
              piVar7[0xb] = 0;
              *piVar7 = *(int *)(iVar3 + 8);
              if (*(int **)(iVar3 + 8) != DAT_004239a4) {
                (*(int **)(iVar3 + 8))[1] = (int)piVar7;
              }
              *(int *)(iVar3 + 4) = piVar7[1];
              if (piVar7 == *(int **)(*(int *)((int)this + 4) + 4)) {
                *(int *)(*(int *)((int)this + 4) + 4) = iVar3;
              }
              else {
                piVar10 = (int *)piVar7[1];
                if (piVar7 == (int *)piVar10[2]) {
                  piVar10[2] = iVar3;
                }
                else {
                  *piVar10 = iVar3;
                }
              }
              *(int **)(iVar3 + 8) = piVar7;
              piVar7[1] = iVar3;
              piVar7 = *(int **)(piVar11[1] + 8);
            }
            piVar7[0xb] = *(int *)(piVar11[1] + 0x2c);
            *(undefined4 *)(piVar11[1] + 0x2c) = 1;
            *(undefined4 *)(piVar7[2] + 0x2c) = 1;
            puVar8 = (undefined4 *)piVar11[1];
            puVar9 = (undefined4 *)puVar8[2];
            puVar8[2] = *puVar9;
            if ((int *)*puVar9 != DAT_004239a4) {
              ((int *)*puVar9)[1] = (int)puVar8;
            }
            puVar9[1] = puVar8[1];
            if (puVar8 == *(undefined4 **)(*(int *)((int)this + 4) + 4)) {
              *(undefined4 **)(*(int *)((int)this + 4) + 4) = puVar9;
              *puVar9 = puVar8;
            }
            else {
              piVar7 = (int *)puVar8[1];
              if (puVar8 == (undefined4 *)*piVar7) {
                *piVar7 = (int)puVar9;
                *puVar9 = puVar8;
              }
              else {
                piVar7[2] = (int)puVar9;
                *puVar9 = puVar8;
              }
            }
LAB_00403216:
            puVar8[1] = puVar9;
            break;
          }
        }
        else {
          if (piVar7[0xb] == 0) {
            piVar7[0xb] = 1;
            *(undefined4 *)(piVar11[1] + 0x2c) = 0;
            piVar7 = (int *)piVar11[1];
            iVar3 = *piVar7;
            *piVar7 = *(int *)(iVar3 + 8);
            if (*(int **)(iVar3 + 8) != DAT_004239a4) {
              (*(int **)(iVar3 + 8))[1] = (int)piVar7;
            }
            *(int *)(iVar3 + 4) = piVar7[1];
            if (piVar7 == *(int **)(*(int *)((int)this + 4) + 4)) {
              *(int *)(*(int *)((int)this + 4) + 4) = iVar3;
            }
            else {
              piVar10 = (int *)piVar7[1];
              if (piVar7 == (int *)piVar10[2]) {
                piVar10[2] = iVar3;
              }
              else {
                *piVar10 = iVar3;
              }
            }
            *(int **)(iVar3 + 8) = piVar7;
            piVar7[1] = iVar3;
            piVar7 = *(int **)piVar11[1];
          }
          if ((*(int *)(piVar7[2] + 0x2c) != 1) || (*(int *)(*piVar7 + 0x2c) != 1)) {
            if (*(int *)(*piVar7 + 0x2c) == 1) {
              *(undefined4 *)(piVar7[2] + 0x2c) = 1;
              piVar10 = (int *)piVar7[2];
              piVar7[0xb] = 0;
              piVar7[2] = *piVar10;
              if ((int *)*piVar10 != DAT_004239a4) {
                ((int *)*piVar10)[1] = (int)piVar7;
              }
              piVar10[1] = piVar7[1];
              if (piVar7 == *(int **)(*(int *)((int)this + 4) + 4)) {
                *(int **)(*(int *)((int)this + 4) + 4) = piVar10;
              }
              else {
                piVar4 = (int *)piVar7[1];
                if (piVar7 == (int *)*piVar4) {
                  *piVar4 = (int)piVar10;
                }
                else {
                  piVar4[2] = (int)piVar10;
                }
              }
              *piVar10 = (int)piVar7;
              piVar7[1] = (int)piVar10;
              piVar7 = *(int **)piVar11[1];
            }
            piVar7[0xb] = *(int *)(piVar11[1] + 0x2c);
            *(undefined4 *)(piVar11[1] + 0x2c) = 1;
            *(undefined4 *)(*piVar7 + 0x2c) = 1;
            puVar8 = (undefined4 *)piVar11[1];
            puVar9 = (undefined4 *)*puVar8;
            *puVar8 = puVar9[2];
            if ((int *)puVar9[2] != DAT_004239a4) {
              ((int *)puVar9[2])[1] = (int)puVar8;
            }
            puVar9[1] = puVar8[1];
            if (puVar8 == *(undefined4 **)(*(int *)((int)this + 4) + 4)) {
              *(undefined4 **)(*(int *)((int)this + 4) + 4) = puVar9;
            }
            else {
              puVar5 = (undefined4 *)puVar8[1];
              if (puVar8 == (undefined4 *)puVar5[2]) {
                puVar5[2] = puVar9;
              }
              else {
                *puVar5 = puVar9;
              }
            }
            puVar9[2] = puVar8;
            goto LAB_00403216;
          }
        }
        piVar7[0xb] = 0;
        piVar11 = (int *)piVar11[1];
      } while (piVar11 != *(int **)(*(int *)((int)this + 4) + 4));
    }
    piVar11[0xb] = 1;
  }
  FUN_0040f30e();
  iVar3 = local_c[8];
  if (iVar3 != 0) {
    cVar1 = *(char *)(iVar3 + -1);
    if ((cVar1 == '\0') || (cVar1 == -1)) {
      FUN_0040ef10((char *)(iVar3 + -1));
    }
    else {
      *(char *)(iVar3 + -1) = cVar1 + -1;
    }
  }
  local_c[8] = 0;
  local_c[9] = 0;
  local_c[10] = 0;
  iVar3 = local_c[4];
  if (iVar3 != 0) {
    cVar1 = *(char *)(iVar3 + -1);
    if ((cVar1 == '\0') || (cVar1 == -1)) {
      FUN_0040ef10((char *)(iVar3 + -1));
    }
    else {
      *(char *)(iVar3 + -1) = cVar1 + -1;
    }
  }
  local_c[4] = 0;
  local_c[5] = 0;
  local_c[6] = 0;
  FUN_0040ef10((undefined *)local_c);
  *(int *)((int)this + 0xc) = *(int *)((int)this + 0xc) + -1;
  *param_1 = param_2;
  return;
}



void __fastcall FUN_004032b0(int *param_1)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  int iVar3;
  
  puVar1 = *(undefined4 **)(*param_1 + 8);
  if (puVar1 == DAT_004239a4) {
    iVar3 = *(int *)(*param_1 + 4);
    if (*param_1 == *(int *)(iVar3 + 8)) {
      do {
        *param_1 = iVar3;
        iVar3 = *(int *)(iVar3 + 4);
      } while (*param_1 == *(int *)(iVar3 + 8));
    }
    if (*(int *)(*param_1 + 8) != iVar3) {
      *param_1 = iVar3;
    }
    return;
  }
  for (puVar2 = (undefined4 *)*puVar1; puVar2 != DAT_004239a4; puVar2 = (undefined4 *)*puVar2) {
    puVar1 = puVar2;
  }
  *param_1 = (int)puVar1;
  return;
}



void FUN_00403300(int *param_1)

{
  int *piVar1;
  
  if (param_1 != DAT_004239a4) {
    do {
      FUN_00403300((int *)param_1[2]);
      piVar1 = (int *)*param_1;
      FUN_00403730((int)(param_1 + 3));
      FUN_0040ef10((undefined *)param_1);
      param_1 = piVar1;
    } while (piVar1 != DAT_004239a4);
  }
  return;
}



void __fastcall FUN_00403350(int param_1)

{
  undefined4 *puVar1;
  void *pvVar2;
  int local_4;
  
  local_4 = param_1;
  puVar1 = operator_new(0x30);
  puVar1[1] = 0;
  puVar1[0xb] = 1;
  FUN_0040f272(&local_4);
  if (DAT_004239a4 == (undefined4 *)0x0) {
    DAT_004239a4 = puVar1;
    *puVar1 = 0;
    puVar1 = (undefined4 *)0x0;
    DAT_004239a4[2] = 0;
  }
  DAT_004239a0 = DAT_004239a0 + 1;
  FUN_0040f30e();
  if (puVar1 != (undefined4 *)0x0) {
    FUN_0040ef10((undefined *)puVar1);
  }
  puVar1 = DAT_004239a4;
  pvVar2 = operator_new(0x30);
  *(undefined4 **)((int)pvVar2 + 4) = puVar1;
  *(undefined4 *)((int)pvVar2 + 0x2c) = 0;
  *(void **)(param_1 + 4) = pvVar2;
  *(undefined4 *)(param_1 + 0xc) = 0;
  *(void **)pvVar2 = pvVar2;
  *(int *)(*(int *)(param_1 + 4) + 8) = *(int *)(param_1 + 4);
  return;
}



int * FUN_004033f0(int *param_1,int *param_2)

{
  int iVar1;
  int *piVar2;
  int *piVar3;
  int *piVar4;
  
  piVar4 = param_1;
  if (param_1 == DAT_004239a4) {
    *param_2 = (int)DAT_004239a4;
    return param_1;
  }
  do {
    iVar1 = piVar4[0xb];
    piVar2 = operator_new(0x30);
    piVar2[1] = (int)param_2;
    piVar2[0xb] = iVar1;
    if (param_1 == piVar4) {
      param_1 = piVar2;
    }
    piVar3 = FUN_004033f0((int *)piVar4[2],piVar2);
    piVar2[2] = (int)piVar3;
    FUN_004034a0((undefined1 *)(piVar2 + 3),(undefined1 *)(piVar4 + 3));
    *param_2 = (int)piVar2;
    piVar4 = (int *)*piVar4;
    param_2 = piVar2;
  } while (piVar4 != DAT_004239a4);
  *piVar2 = (int)DAT_004239a4;
  return param_1;
}



void FUN_00403480(undefined4 param_1,undefined4 param_2)

{
  void *pvVar1;
  
  pvVar1 = operator_new(0x30);
  *(undefined4 *)((int)pvVar1 + 4) = param_1;
  *(undefined4 *)((int)pvVar1 + 0x2c) = param_2;
  return;
}



// WARNING: Removing unreachable block (ram,0x00403519)

void __cdecl FUN_004034a0(undefined1 *param_1,undefined1 *param_2)

{
  undefined1 *this;
  undefined1 uVar1;
  uint uVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  uint uVar5;
  undefined4 *puVar6;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  puStack_8 = &LAB_00419359;
  local_c = ExceptionList;
  local_4 = 0;
  if (param_1 != (undefined1 *)0x0) {
    uVar1 = *param_2;
    ExceptionList = &local_c;
    *(undefined4 *)(param_1 + 4) = 0;
    *param_1 = uVar1;
    *(undefined4 *)(param_1 + 8) = 0;
    *(undefined4 *)(param_1 + 0xc) = 0;
    FUN_00401900(param_1,param_2,0,0xffffffff);
    this = param_1 + 0x10;
    local_4 = CONCAT31(local_4._1_3_,1);
    *this = param_2[0x10];
    *(undefined4 *)(param_1 + 0x14) = 0;
    *(undefined4 *)(param_1 + 0x18) = 0;
    *(undefined4 *)(param_1 + 0x1c) = 0;
    uVar2 = *(uint *)(param_2 + 0x18);
    if (this == param_2 + 0x10) {
      FUN_004035f0(this,uVar2,0xffffffff);
      FUN_004035f0(this,0,0);
      ExceptionList = local_c;
      return;
    }
    if (uVar2 != 0) {
      puVar3 = *(undefined4 **)(param_2 + 0x14);
      if (puVar3 == (undefined4 *)0x0) {
        puVar3 = &DAT_0041a214;
      }
      if (*(byte *)((int)puVar3 + -1) < 0xfe) {
        *(undefined4 *)(param_1 + 0x14) = 0;
        *(undefined4 *)(param_1 + 0x18) = 0;
        *(undefined4 *)(param_1 + 0x1c) = 0;
        puVar3 = *(undefined4 **)(param_2 + 0x14);
        if (puVar3 == (undefined4 *)0x0) {
          puVar3 = &DAT_0041a214;
        }
        *(undefined4 **)(param_1 + 0x14) = puVar3;
        *(undefined4 *)(param_1 + 0x18) = *(undefined4 *)(param_2 + 0x18);
        *(undefined4 *)(param_1 + 0x1c) = *(undefined4 *)(param_2 + 0x1c);
        *(char *)((int)puVar3 + -1) = *(char *)((int)puVar3 + -1) + '\x01';
        ExceptionList = local_c;
        return;
      }
    }
    uVar4 = FUN_00401570(this,uVar2,'\x01');
    if ((char)uVar4 != '\0') {
      puVar3 = &DAT_0041a214;
      if (*(undefined4 **)(param_2 + 0x14) != (undefined4 *)0x0) {
        puVar3 = *(undefined4 **)(param_2 + 0x14);
      }
      puVar6 = *(undefined4 **)(param_1 + 0x14);
      for (uVar5 = uVar2 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *puVar6 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar6 = puVar6 + 1;
      }
      for (uVar5 = uVar2 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined1 *)puVar6 = *(undefined1 *)puVar3;
        puVar3 = (undefined4 *)((int)puVar3 + 1);
        puVar6 = (undefined4 *)((int)puVar6 + 1);
      }
      FUN_00401550(this,uVar2);
    }
  }
  ExceptionList = local_c;
  return;
}



void * __thiscall FUN_004035f0(void *this,uint param_1,uint param_2)

{
  char cVar1;
  undefined1 *puVar2;
  undefined4 uVar3;
  uint uVar4;
  uint uVar5;
  undefined4 *puVar6;
  char *pcVar7;
  char *pcVar8;
  
  if (*(uint *)((int)this + 8) < param_1) {
    FUN_0040f147();
  }
  pcVar7 = *(char **)((int)this + 4);
  if (((pcVar7 != (char *)0x0) && (cVar1 = pcVar7[-1], cVar1 != '\0')) && (cVar1 != -1)) {
    pcVar7[-1] = cVar1 + -1;
    uVar4 = 0xffffffff;
    *(undefined4 *)((int)this + 4) = 0;
    *(undefined4 *)((int)this + 8) = 0;
    *(undefined4 *)((int)this + 0xc) = 0;
    pcVar8 = pcVar7;
    do {
      if (uVar4 == 0) break;
      uVar4 = uVar4 - 1;
      cVar1 = *pcVar8;
      pcVar8 = pcVar8 + 1;
    } while (cVar1 != '\0');
    uVar4 = ~uVar4 - 1;
    uVar3 = FUN_00401570(this,uVar4,'\x01');
    if ((char)uVar3 != '\0') {
      pcVar8 = *(char **)((int)this + 4);
      for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
        pcVar7 = pcVar7 + 4;
        pcVar8 = pcVar8 + 4;
      }
      for (uVar5 = uVar4 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *pcVar8 = *pcVar7;
        pcVar7 = pcVar7 + 1;
        pcVar8 = pcVar8 + 1;
      }
      FUN_00401550(this,uVar4);
    }
  }
  uVar4 = *(int *)((int)this + 8) - param_1;
  if (uVar4 < param_2) {
    param_2 = uVar4;
  }
  if (param_2 == 0) {
    return this;
  }
  puVar6 = (undefined4 *)(*(int *)((int)this + 4) + param_1);
  FUN_0040f710(puVar6,(undefined4 *)((int)puVar6 + param_2),uVar4 - param_2);
  uVar4 = *(int *)((int)this + 8) - param_2;
  if (0xfffffffd < uVar4) {
    FUN_0040ef1b();
  }
  puVar2 = *(undefined1 **)((int)this + 4);
  if (((puVar2 == (undefined1 *)0x0) || (cVar1 = puVar2[-1], cVar1 == '\0')) || (cVar1 == -1)) {
    if (uVar4 == 0) {
      if (puVar2 == (undefined1 *)0x0) {
        return this;
      }
      *(undefined4 *)((int)this + 8) = 0;
      *puVar2 = 0;
      return this;
    }
    if (uVar4 <= *(uint *)((int)this + 0xc)) goto LAB_004036f4;
  }
  else if (uVar4 == 0) {
    puVar2[-1] = cVar1 + -1;
    FUN_00401380(this,'\0');
    return this;
  }
  FUN_00401690(uVar4);
LAB_004036f4:
  *(uint *)((int)this + 8) = uVar4;
  *(undefined1 *)(uVar4 + *(int *)((int)this + 4)) = 0;
  return this;
}



undefined * __thiscall FUN_00403710(void *this,byte param_1)

{
  FUN_00403730((int)this);
  if ((param_1 & 1) != 0) {
    FUN_0040ef10(this);
  }
  return this;
}



void __fastcall FUN_00403730(int param_1)

{
  char cVar1;
  int iVar2;
  
  iVar2 = *(int *)(param_1 + 0x14);
  if (iVar2 != 0) {
    cVar1 = *(char *)(iVar2 + -1);
    if ((cVar1 == '\0') || (cVar1 == -1)) {
      FUN_0040ef10((char *)(iVar2 + -1));
    }
    else {
      *(char *)(iVar2 + -1) = cVar1 + -1;
    }
  }
  *(undefined4 *)(param_1 + 0x14) = 0;
  *(undefined4 *)(param_1 + 0x18) = 0;
  *(undefined4 *)(param_1 + 0x1c) = 0;
  iVar2 = *(int *)(param_1 + 4);
  if (iVar2 != 0) {
    cVar1 = *(char *)(iVar2 + -1);
    if ((cVar1 != '\0') && (cVar1 != -1)) {
      *(char *)(iVar2 + -1) = cVar1 + -1;
      *(undefined4 *)(param_1 + 4) = 0;
      *(undefined4 *)(param_1 + 8) = 0;
      *(undefined4 *)(param_1 + 0xc) = 0;
      return;
    }
    FUN_0040ef10((char *)(iVar2 + -1));
  }
  *(undefined4 *)(param_1 + 4) = 0;
  *(undefined4 *)(param_1 + 8) = 0;
  *(undefined4 *)(param_1 + 0xc) = 0;
  return;
}



undefined4 FUN_004037e0(void)

{
  BOOL BVar1;
  byte local_30c [260];
  byte local_208 [260];
  byte local_104 [260];
  
  FUN_0040fffb(DAT_00423a0c,local_30c,local_208,(byte *)0x0,(byte *)0x0);
  FUN_00410142(local_104,local_30c,local_208,(byte *)0x0,(byte *)0x0);
  BVar1 = SetCurrentDirectoryA((LPCSTR)local_104);
  if (BVar1 == 0) {
    MessageBoxA((HWND)0x0,s_ChangeDirectoryExeFriend_failed_004201a8,&DAT_004200cc,0);
    return 1;
  }
  return 0;
}



undefined4 FUN_00403860(void)

{
  byte bVar1;
  DWORD DVar2;
  BOOL BVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  byte *pbVar7;
  undefined4 *puVar8;
  byte *pbVar9;
  byte local_208;
  undefined4 local_207;
  byte local_104;
  byte local_103 [2];
  byte abStack_101 [257];
  
  DVar2 = GetFileAttributesA(s___config_ini_00420178);
  if (DVar2 == 0xffffffff) {
    local_208 = 0;
    puVar8 = &local_207;
    for (iVar4 = 0x40; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar8 = 0;
      puVar8 = puVar8 + 1;
    }
    *(undefined2 *)puVar8 = 0;
    *(undefined1 *)((int)puVar8 + 2) = 0;
    SHGetSpecialFolderPathA((HWND)0x0,(LPSTR)&local_208,0x1c,1);
    DVar2 = GetFileAttributesA((LPCSTR)&local_208);
    if (DVar2 == 0xffffffff) {
      MessageBoxA((HWND)0x0,s_CSIDL_LOCAL_APPDATA_failed_00420204,&DAT_004200cc,0);
      return 1;
    }
    local_104 = 0;
    pbVar7 = local_103;
    for (iVar4 = 0x40; iVar4 != 0; iVar4 = iVar4 + -1) {
      pbVar7[0] = 0;
      pbVar7[1] = 0;
      pbVar7[2] = 0;
      pbVar7[3] = 0;
      pbVar7 = pbVar7 + 4;
    }
    pbVar7[0] = 0;
    pbVar7[1] = 0;
    pbVar7[2] = 0;
    uVar5 = 0xffffffff;
    pbVar7 = &local_208;
    do {
      pbVar9 = pbVar7;
      if (uVar5 == 0) break;
      uVar5 = uVar5 - 1;
      pbVar9 = pbVar7 + 1;
      bVar1 = *pbVar7;
      pbVar7 = pbVar9;
    } while (bVar1 != 0);
    uVar5 = ~uVar5;
    pbVar7 = pbVar9 + -uVar5;
    pbVar9 = &local_104;
    for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
      *(undefined4 *)pbVar9 = *(undefined4 *)pbVar7;
      pbVar7 = pbVar7 + 4;
      pbVar9 = pbVar9 + 4;
    }
    for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
      *pbVar9 = *pbVar7;
      pbVar7 = pbVar7 + 1;
      pbVar9 = pbVar9 + 1;
    }
    FUN_00410142(&local_208,(byte *)0x0,&local_104,(byte *)s_C_EGG_004201fc,(byte *)0x0);
    CreateDirectoryA((LPCSTR)&local_208,(LPSECURITY_ATTRIBUTES)0x0);
    uVar5 = 0xffffffff;
    pbVar7 = &local_208;
    do {
      pbVar9 = pbVar7;
      if (uVar5 == 0) break;
      uVar5 = uVar5 - 1;
      pbVar9 = pbVar7 + 1;
      bVar1 = *pbVar7;
      pbVar7 = pbVar9;
    } while (bVar1 != 0);
    uVar5 = ~uVar5;
    pbVar7 = pbVar9 + -uVar5;
    pbVar9 = &local_104;
    for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
      *(undefined4 *)pbVar9 = *(undefined4 *)pbVar7;
      pbVar7 = pbVar7 + 4;
      pbVar9 = pbVar9 + 4;
    }
    for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
      *pbVar9 = *pbVar7;
      pbVar7 = pbVar7 + 1;
      pbVar9 = pbVar9 + 1;
    }
    FUN_00410142(&local_208,(byte *)0x0,&local_104,(byte *)s_tenhou_004201f4,(byte *)0x0);
    CreateDirectoryA((LPCSTR)&local_208,(LPSECURITY_ATTRIBUTES)0x0);
    uVar5 = 0xffffffff;
    pbVar7 = &local_208;
    do {
      pbVar9 = pbVar7;
      if (uVar5 == 0) break;
      uVar5 = uVar5 - 1;
      pbVar9 = pbVar7 + 1;
      bVar1 = *pbVar7;
      pbVar7 = pbVar9;
    } while (bVar1 != 0);
    uVar5 = ~uVar5;
    pbVar7 = pbVar9 + -uVar5;
    pbVar9 = &local_104;
    for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
      *(undefined4 *)pbVar9 = *(undefined4 *)pbVar7;
      pbVar7 = pbVar7 + 4;
      pbVar9 = pbVar9 + 4;
    }
    for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
      *pbVar9 = *pbVar7;
      pbVar7 = pbVar7 + 1;
      pbVar9 = pbVar9 + 1;
    }
    FUN_00410142(&local_208,(byte *)0x0,&local_104,&DAT_004201f0,(byte *)0x0);
    CreateDirectoryA((LPCSTR)&local_208,(LPSECURITY_ATTRIBUTES)0x0);
    BVar3 = SetCurrentDirectoryA((LPCSTR)&local_208);
    if (BVar3 == 0) {
      MessageBoxA((HWND)0x0,s_ChangeDirectoryToLocalAppData_fa_004201c8,&DAT_004200cc,0);
      return 1;
    }
  }
  return 0;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void __fastcall FUN_00403a30(undefined4 param_1)

{
  undefined1 local_1;
  
  local_1 = (undefined1)((uint)param_1 >> 0x18);
  DAT_004239a8 = local_1;
  DAT_004239ac = 0;
  _DAT_004239b0 = 0;
  _DAT_004239b4 = 0;
  return;
}



void FUN_00403a50(void)

{
  FUN_0040fac3(&LAB_00403a60);
  return;
}



undefined4 * FUN_00403ab0(void)

{
  undefined4 *puVar1;
  
  puVar1 = DAT_004239ac;
  if (DAT_004239ac == (undefined4 *)0x0) {
    puVar1 = &DAT_0041a214;
  }
  return puVar1;
}



void __cdecl FUN_00403ac0(HANDLE param_1,undefined4 param_2)

{
  FUN_00403ae0(0,param_1,param_2,(uint *)0x3);
  return;
}



// WARNING: Removing unreachable block (ram,0x00403c1e)

int __cdecl FUN_00403ae0(SOCKET param_1,HANDLE param_2,undefined4 param_3,uint *param_4)

{
  uint *puVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  void *in_stack_0000201c;
  char *in_stack_00002030;
  void *in_stack_00002034;
  int *in_stack_0000203c;
  undefined1 *puVar6;
  undefined1 *puVar7;
  undefined4 uStack_c;
  undefined **local_8;
  int local_4;
  
  local_4 = -1;
  local_8 = (undefined **)&LAB_00419386;
  uStack_c = ExceptionList;
  ExceptionList = &uStack_c;
  FUN_004101e0();
  if (in_stack_0000203c != (int *)0x0) {
    (**(code **)(*in_stack_0000203c + 4))(in_stack_0000203c);
  }
  local_4 = 0;
  param_1 = 0xffffffff;
  param_2 = (HANDLE)0xffffffff;
  local_8 = &PTR_FUN_0041a240;
  FUN_00406810(&param_3,in_stack_00002030,(undefined1 *)((int)&uStack_c + 3));
  iVar5 = 10;
  do {
    puVar1 = param_4;
    if (param_4 == (uint *)0x0) {
      puVar1 = &DAT_0041a214;
    }
    iVar2 = FUN_00403e40(puVar1,&param_1,&local_4,(undefined4 *)&stack0x00000000,&param_3);
    if (iVar2 != 0) {
      FUN_00401380(&param_3,'\x01');
      if (iVar2 != 0) goto LAB_00403cc1;
LAB_00403bfc:
      if (in_stack_0000203c != (int *)0x0) {
        (**(code **)(*in_stack_0000203c + 0x18))(in_stack_0000203c,0,0,4,0);
      }
      goto LAB_00403c16;
    }
    switch(local_4) {
    case 200:
      FUN_00401380(&param_3,'\x01');
      goto LAB_00403bfc;
    case 0x194:
      FUN_00401380(&param_3,'\x01');
      iVar2 = 2;
      goto LAB_00403cc1;
    }
    iVar5 = iVar5 + -1;
  } while (0 < iVar5);
  FUN_00401380(&param_3,'\x01');
  iVar2 = 0xc;
  goto LAB_00403cc1;
  while( true ) {
    if (iVar5 == -1) {
      iVar2 = WSAGetLastError();
      goto LAB_00403cc1;
    }
    puVar7 = &stack0x0000001c + iVar5;
    puVar6 = &stack0x0000001c;
    uVar3 = FUN_00406f10(in_stack_00002034);
    uVar4 = FUN_00406f10(in_stack_00002034);
    FUN_00406d10(in_stack_00002034,uVar4,uVar3,puVar6,(int)puVar7);
    if ((in_stack_0000203c != (int *)0x0) &&
       (iVar5 = (**(code **)(*in_stack_0000203c + 0x18))
                          (in_stack_0000203c,*(undefined4 *)((int)in_stack_00002034 + 8),0,5,0),
       iVar5 != 0)) break;
LAB_00403c16:
    iVar5 = recv(param_1,&stack0x0000001c,0x2000,0);
    if (iVar5 == 0) {
      if (in_stack_0000203c != (int *)0x0) {
        (**(code **)(*in_stack_0000203c + 0x18))
                  (in_stack_0000203c,*(undefined4 *)((int)in_stack_00002034 + 8),0,6,0);
      }
      iVar2 = 0;
      goto LAB_00403cc1;
    }
  }
  iVar2 = -0x7fffbffc;
LAB_00403cc1:
  local_8 = &PTR_FUN_0041a240;
  if (param_1 != 0xffffffff) {
    closesocket(param_1);
    param_1 = 0xffffffff;
  }
  if (param_2 != (HANDLE)0xffffffff) {
    CloseHandle(param_2);
    param_2 = (HANDLE)0xffffffff;
  }
  if (in_stack_0000203c != (int *)0x0) {
    (**(code **)(*in_stack_0000203c + 8))(in_stack_0000203c);
  }
  ExceptionList = in_stack_0000201c;
  return iVar2;
}



void __fastcall FUN_00403e00(undefined4 *param_1)

{
  *param_1 = &PTR_FUN_0041a240;
  if (param_1[3] != 0xffffffff) {
    closesocket(param_1[3]);
    param_1[3] = 0xffffffff;
  }
  if ((HANDLE)param_1[4] != (HANDLE)0xffffffff) {
    CloseHandle((HANDLE)param_1[4]);
    param_1[4] = 0xffffffff;
  }
  return;
}



int __cdecl
FUN_00403e40(uint *param_1,SOCKET *param_2,int *param_3,undefined4 *param_4,void *param_5)

{
  char cVar1;
  byte bVar2;
  short sVar3;
  uint uVar4;
  u_short hostshort;
  uint *puVar5;
  int iVar6;
  uint *puVar7;
  SOCKET s;
  char *pcVar8;
  hostent *phVar9;
  undefined1 *puVar10;
  undefined4 uVar11;
  byte *pbVar12;
  uint uVar13;
  void *this;
  void *extraout_ECX;
  void *extraout_ECX_00;
  void *this_00;
  void *this_01;
  void *this_02;
  void *this_03;
  byte *pbVar14;
  char *pcVar15;
  _OSVERSIONINFOA *p_Var16;
  bool bVar17;
  undefined1 local_1f5;
  uint *local_1f4;
  undefined1 local_1f0 [4];
  uint *local_1ec;
  uint local_1e8;
  undefined4 local_1e4;
  undefined1 local_1e0 [4];
  char *local_1dc;
  sockaddr local_1d0;
  undefined1 local_1c0 [4];
  int local_1bc;
  undefined4 local_1b8;
  undefined4 local_1b4;
  undefined1 local_1b0 [4];
  int local_1ac;
  _OSVERSIONINFOA local_1a0;
  CHAR local_10c [256];
  void *local_c;
  undefined1 *puStack_8;
  uint local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004193f3;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  if (*param_2 != 0xffffffff) {
    ExceptionList = &local_c;
    closesocket(*param_2);
    *param_2 = 0xffffffff;
  }
  FUN_00401380(local_1e0,'\0');
  uVar13 = 0xffffffff;
  pcVar8 = s_localhost_00420340;
  do {
    if (uVar13 == 0) break;
    uVar13 = uVar13 - 1;
    cVar1 = *pcVar8;
    pcVar8 = pcVar8 + 1;
  } while (cVar1 != '\0');
  FUN_004014b0(local_1e0,(undefined4 *)s_localhost_00420340,~uVar13 - 1);
  local_4 = 0;
  hostshort = 0x50;
  FUN_00401380(local_1c0,'\0');
  uVar13 = 0xffffffff;
  pcVar8 = &DAT_0042033c;
  do {
    if (uVar13 == 0) break;
    uVar13 = uVar13 - 1;
    cVar1 = *pcVar8;
    pcVar8 = pcVar8 + 1;
  } while (cVar1 != '\0');
  FUN_004014b0(local_1c0,(undefined4 *)&DAT_0042033c,~uVar13 - 1);
  local_4._0_1_ = 1;
  local_1f4 = param_1;
  puVar5 = FUN_00410470(param_1,&DAT_00420338);
  if (puVar5 == (uint *)0x0) {
    local_4 = (uint)local_4._1_3_ << 8;
    FUN_00401380(local_1c0,'\x01');
    local_4 = 0xffffffff;
    FUN_00401380(local_1e0,'\x01');
    iVar6 = -0x7ff8ffa9;
  }
  else {
    puVar5 = (uint *)((int)puVar5 + 3);
    cVar1 = *(char *)puVar5;
    puVar7 = puVar5;
    while (((cVar1 != '\0' && (cVar1 != '/')) && (cVar1 != ':'))) {
      puVar7 = (uint *)((int)puVar7 + 1);
      cVar1 = *(char *)puVar7;
    }
    local_1d0.sa_family = CONCAT11(local_1d0.sa_family._1_1_,local_1f5);
    local_1f4 = puVar7;
    FUN_00401380(&local_1d0,'\0');
    FUN_00406b30(&local_1d0,(undefined1 *)puVar5,(int)puVar7);
    local_4._0_1_ = 2;
    FUN_00401900(local_1e0,&local_1d0,0,0xffffffff);
    local_4 = CONCAT31(local_4._1_3_,1);
    FUN_00401380(&local_1d0,'\x01');
    if ((char)*local_1f4 == ':') {
      hostshort = FUN_00410459(this,(byte *)((int)local_1f4 + 1),(int *)&local_1f4,(void *)0xa);
    }
    if ((char)*local_1f4 == '/') {
      uVar13 = 0xffffffff;
      puVar5 = local_1f4;
      do {
        if (uVar13 == 0) break;
        uVar13 = uVar13 - 1;
        uVar4 = *puVar5;
        puVar5 = (uint *)((int)puVar5 + 1);
      } while ((char)uVar4 != '\0');
      FUN_004014b0(local_1c0,local_1f4,~uVar13 - 1);
    }
    s = socket(2,1,0);
    *param_2 = s;
    local_1f4 = (uint *)0x2710;
    setsockopt(s,0xffff,0x1005,(char *)&local_1f4,4);
    local_1f4 = (uint *)0x2710;
    setsockopt(*param_2,0xffff,0x1006,(char *)&local_1f4,4);
    if (local_1dc == (char *)0x0) {
      pcVar8 = "";
      this_00 = extraout_ECX;
    }
    else {
      FUN_00406fa0(local_1e0);
      pcVar8 = local_1dc;
      this_00 = extraout_ECX_00;
    }
    uVar13 = FUN_0041020f(this_00,(int)*pcVar8);
    if (uVar13 == 0) {
      pcVar8 = local_1dc;
      if (local_1dc == (char *)0x0) {
        pcVar8 = "";
      }
      local_1f4 = (uint *)inet_addr(pcVar8);
      phVar9 = gethostbyaddr((char *)&local_1f4,4,2);
    }
    else {
      pcVar8 = local_1dc;
      if (local_1dc == (char *)0x0) {
        pcVar8 = "";
      }
      phVar9 = gethostbyname(pcVar8);
    }
    if (phVar9 == (hostent *)0x0) {
      iVar6 = WSAGetLastError();
    }
    else {
      local_1d0.sa_family = 2;
      local_1d0.sa_data._0_2_ = htons(hostshort);
      local_1d0.sa_data[3] = '\0';
      local_1d0.sa_data[4] = '\0';
      local_1d0.sa_data[5] = '\0';
      local_1d0.sa_data[6] = '\0';
      local_1d0.sa_data[2] = '\0';
      local_1d0.sa_data[7] = '\0';
      local_1d0.sa_data[8] = '\0';
      local_1d0.sa_data[9] = '\0';
      local_1d0.sa_data[10] = '\0';
      local_1d0.sa_data[0xb] = '\0';
      local_1d0.sa_data[0xc] = '\0';
      local_1d0.sa_data[0xd] = '\0';
      sVar3 = phVar9->h_length;
      pcVar8 = *phVar9->h_addr_list;
      pcVar15 = local_1d0.sa_data + 2;
      for (uVar13 = (uint)(int)sVar3 >> 2; uVar13 != 0; uVar13 = uVar13 - 1) {
        *(undefined4 *)pcVar15 = *(undefined4 *)pcVar8;
        pcVar8 = pcVar8 + 4;
        pcVar15 = pcVar15 + 4;
      }
      for (uVar13 = (int)sVar3 & 3; uVar13 != 0; uVar13 = uVar13 - 1) {
        *pcVar15 = *pcVar8;
        pcVar8 = pcVar8 + 1;
        pcVar15 = pcVar15 + 1;
      }
      iVar6 = connect(*param_2,&local_1d0,0x10);
      if (iVar6 != -1) {
        FUN_00401380(local_1f0,'\0');
        local_4._0_1_ = 3;
        p_Var16 = &local_1a0;
        for (iVar6 = 0x25; iVar6 != 0; iVar6 = iVar6 + -1) {
          p_Var16->dwOSVersionInfoSize = 0;
          p_Var16 = (_OSVERSIONINFOA *)&p_Var16->dwMajorVersion;
        }
        local_1a0.dwOSVersionInfoSize = 0x94;
        GetVersionExA(&local_1a0);
        puVar10 = FUN_004017a0(local_1b0,&DAT_0042031c,(int)local_1c0);
        local_4._0_1_ = 4;
        puVar10 = FUN_00401be0((undefined1 *)&local_1d0,puVar10,s_HTTP_1_0_00420310);
        local_4._0_1_ = 5;
        FUN_004013d0(local_1f0,(int)puVar10,0,0xffffffff);
        local_4._0_1_ = 4;
        FUN_00401380(&local_1d0,'\x01');
        local_4._0_1_ = 3;
        FUN_00401380(local_1b0,'\x01');
        uVar13 = 0xffffffff;
        pcVar8 = s_User_Agent__Mozilla_4_0___004202f4;
        do {
          if (uVar13 == 0) break;
          uVar13 = uVar13 - 1;
          cVar1 = *pcVar8;
          pcVar8 = pcVar8 + 1;
        } while (cVar1 != '\0');
        FUN_00407020(local_1f0,(undefined4 *)s_User_Agent__Mozilla_4_0___004202f4,~uVar13 - 1);
        uVar13 = 0xffffffff;
        pcVar8 = s_compatible__004202e8;
        do {
          if (uVar13 == 0) break;
          uVar13 = uVar13 - 1;
          cVar1 = *pcVar8;
          pcVar8 = pcVar8 + 1;
        } while (cVar1 != '\0');
        FUN_00407020(local_1f0,(undefined4 *)s_compatible__004202e8,~uVar13 - 1);
        uVar13 = 0xffffffff;
        pcVar8 = s_C_EGG_TENHOU__004202d8;
        do {
          if (uVar13 == 0) break;
          uVar13 = uVar13 - 1;
          cVar1 = *pcVar8;
          pcVar8 = pcVar8 + 1;
        } while (cVar1 != '\0');
        FUN_00407020(local_1f0,(undefined4 *)s_C_EGG_TENHOU__004202d8,~uVar13 - 1);
        wsprintfA(local_10c,s_OS_d__d__d__d__004202c8,local_1a0.dwPlatformId,
                  local_1a0.dwMajorVersion,local_1a0.dwMinorVersion,local_1a0.dwBuildNumber);
        uVar13 = 0xffffffff;
        pcVar8 = local_10c;
        do {
          if (uVar13 == 0) break;
          uVar13 = uVar13 - 1;
          cVar1 = *pcVar8;
          pcVar8 = pcVar8 + 1;
        } while (cVar1 != '\0');
        FUN_00407020(local_1f0,(undefined4 *)local_10c,~uVar13 - 1);
        uVar13 = 0xffffffff;
        pcVar8 = s_http___tenhou_net___004202b0;
        do {
          if (uVar13 == 0) break;
          uVar13 = uVar13 - 1;
          cVar1 = *pcVar8;
          pcVar8 = pcVar8 + 1;
        } while (cVar1 != '\0');
        FUN_00407020(local_1f0,(undefined4 *)s_http___tenhou_net___004202b0,~uVar13 - 1);
        uVar13 = 0xffffffff;
        pcVar8 = s_Accept_Encoding__gzip__deflate_0042028c;
        do {
          if (uVar13 == 0) break;
          uVar13 = uVar13 - 1;
          cVar1 = *pcVar8;
          pcVar8 = pcVar8 + 1;
        } while (cVar1 != '\0');
        FUN_00407020(local_1f0,(undefined4 *)s_Accept_Encoding__gzip__deflate_0042028c,~uVar13 - 1);
        puVar10 = FUN_004017a0((undefined1 *)&local_1d0,s_Host__00420284,(int)local_1e0);
        local_4._0_1_ = 6;
        puVar10 = FUN_00401be0(local_1b0,puVar10,&DAT_00420280);
        local_4._0_1_ = 7;
        FUN_004013d0(local_1f0,(int)puVar10,0,0xffffffff);
        local_4._0_1_ = 6;
        FUN_00401380(local_1b0,'\x01');
        local_4 = CONCAT31(local_4._1_3_,3);
        FUN_00401380(&local_1d0,'\x01');
        uVar13 = 0xffffffff;
        pcVar8 = s_Tenhou_Client_Type__download_00420260;
        do {
          if (uVar13 == 0) break;
          uVar13 = uVar13 - 1;
          cVar1 = *pcVar8;
          pcVar8 = pcVar8 + 1;
        } while (cVar1 != '\0');
        FUN_00407020(local_1f0,(undefined4 *)s_Tenhou_Client_Type__download_00420260,~uVar13 - 1);
        uVar13 = 0xffffffff;
        pcVar8 = &DAT_00420280;
        do {
          if (uVar13 == 0) break;
          uVar13 = uVar13 - 1;
          cVar1 = *pcVar8;
          pcVar8 = pcVar8 + 1;
        } while (cVar1 != '\0');
        FUN_00407020(local_1f0,(undefined4 *)&DAT_00420280,~uVar13 - 1);
        puVar5 = local_1ec;
        if (local_1ec == (uint *)0x0) {
          puVar5 = &DAT_0041a214;
        }
        send(*param_2,(char *)puVar5,local_1e8,0);
        FUN_00401b10(local_1f0);
        uVar13 = 0xffffffff;
        if (local_1e8 != 0xffffffff) {
          uVar13 = local_1e8;
        }
        if (uVar13 != 0) {
          FUN_0040f710(local_1ec,(undefined4 *)((int)local_1ec + uVar13),local_1e8 - uVar13);
          uVar13 = local_1e8 - uVar13;
          uVar11 = FUN_00401570(local_1f0,uVar13,'\0');
          if ((char)uVar11 != '\0') {
            *(char *)((int)local_1ec + uVar13) = '\0';
            local_1e8 = uVar13;
          }
        }
        do {
          do {
            iVar6 = recv(*param_2,(char *)&local_1f4,1,0);
            if (iVar6 == -1) {
              iVar6 = WSAGetLastError();
              local_4 = CONCAT31(local_4._1_3_,1);
              FUN_00401380(local_1f0,'\x01');
              goto LAB_004044a8;
            }
            if ((iVar6 != 1) || (FUN_00406a60(local_1f0,1,(char)local_1f4), 0xfff < local_1e8))
            goto LAB_00404718;
          } while (local_1e8 < 4);
          uVar13 = local_1e8 - 4;
          if ((local_1e8 < uVar13) || (local_1ec == (uint *)0x0)) {
            pbVar12 = (byte *)&DAT_0041a214;
          }
          else {
            FUN_00406fa0(local_1f0);
            pbVar12 = (byte *)(uVar13 + (int)local_1ec);
          }
          pbVar14 = &DAT_00420258;
          do {
            bVar2 = *pbVar12;
            bVar17 = bVar2 < *pbVar14;
            if (bVar2 != *pbVar14) {
LAB_00404479:
              iVar6 = (1 - (uint)bVar17) - (uint)(bVar17 != 0);
              goto LAB_0040447e;
            }
            if (bVar2 == 0) break;
            bVar2 = pbVar12[1];
            bVar17 = bVar2 < pbVar14[1];
            if (bVar2 != pbVar14[1]) goto LAB_00404479;
            pbVar12 = pbVar12 + 2;
            pbVar14 = pbVar14 + 2;
          } while (bVar2 != 0);
          iVar6 = 0;
LAB_0040447e:
        } while (iVar6 != 0);
        if (3 < local_1e8) {
          uVar13 = local_1e8 - 4;
          if ((local_1e8 < uVar13) || (local_1ec == (uint *)0x0)) {
            pbVar12 = (byte *)&DAT_0041a214;
          }
          else {
            FUN_00406fa0(local_1f0);
            pbVar12 = (byte *)(uVar13 + (int)local_1ec);
          }
          pbVar14 = &DAT_00420258;
          do {
            bVar2 = *pbVar12;
            bVar17 = bVar2 < *pbVar14;
            if (bVar2 != *pbVar14) {
LAB_0040452c:
              iVar6 = (1 - (uint)bVar17) - (uint)(bVar17 != 0);
              goto LAB_00404531;
            }
            if (bVar2 == 0) break;
            bVar2 = pbVar12[1];
            bVar17 = bVar2 < pbVar14[1];
            if (bVar2 != pbVar14[1]) goto LAB_0040452c;
            pbVar12 = pbVar12 + 2;
            pbVar14 = pbVar14 + 2;
          } while (bVar2 != 0);
          iVar6 = 0;
LAB_00404531:
          if (iVar6 == 0) {
            puVar5 = &DAT_0041a214;
            if (local_1ec != (uint *)0x0) {
              puVar5 = local_1ec;
            }
            puVar10 = FUN_00404780(local_1b0,puVar5,s_HTTP_1_1_0042024c,&DAT_00420280);
            pbVar12 = *(byte **)(puVar10 + 4);
            if (pbVar12 == (byte *)0x0) {
              pbVar12 = (byte *)&DAT_0041a214;
            }
            iVar6 = FUN_00410459(this_01,pbVar12,(int *)0x0,(void *)0xa);
            *param_3 = iVar6;
            FUN_00401380(local_1b0,'\x01');
            if (*param_3 == 0) {
              puVar5 = local_1ec;
              if (local_1ec == (uint *)0x0) {
                puVar5 = &DAT_0041a214;
              }
              puVar10 = FUN_00404780(local_1b0,puVar5,s_HTTP_1_0_00420240,&DAT_00420280);
              pbVar12 = *(byte **)(puVar10 + 4);
              if (pbVar12 == (byte *)0x0) {
                pbVar12 = (byte *)&DAT_0041a214;
              }
              iVar6 = FUN_00410459(this_02,pbVar12,(int *)0x0,(void *)0xa);
              *param_3 = iVar6;
              FUN_00401380(local_1b0,'\x01');
            }
            puVar5 = local_1ec;
            if (local_1ec == (uint *)0x0) {
              puVar5 = &DAT_0041a214;
            }
            puVar10 = FUN_00404780(local_1b0,puVar5,s_Content_Length__0042022c,&DAT_00420280);
            pbVar12 = *(byte **)(puVar10 + 4);
            if (pbVar12 == (byte *)0x0) {
              pbVar12 = (byte *)&DAT_0041a214;
            }
            uVar11 = FUN_00410459(this_03,pbVar12,(int *)0x0,(void *)0xa);
            *param_4 = uVar11;
            FUN_00401380(local_1b0,'\x01');
            puVar5 = local_1ec;
            if (local_1ec == (uint *)0x0) {
              puVar5 = &DAT_0041a214;
            }
            puVar10 = FUN_00404780(local_1b0,puVar5,s_Location__00420220,&DAT_00420280);
            local_4 = CONCAT31(local_4._1_3_,8);
            FUN_00401900(param_5,puVar10,0,0xffffffff);
            if (local_1ac != 0) {
              cVar1 = *(char *)(local_1ac + -1);
              if ((cVar1 == '\0') || (cVar1 == -1)) {
                FUN_0040ef10((char *)(local_1ac + -1));
              }
              else {
                *(char *)(local_1ac + -1) = cVar1 + -1;
              }
            }
            if (local_1ec != (uint *)0x0) {
              cVar1 = *(char *)((int)local_1ec + -1);
              if ((cVar1 == '\0') || (cVar1 == -1)) {
                FUN_0040ef10((char *)((int)local_1ec + -1));
              }
              else {
                *(char *)((int)local_1ec + -1) = cVar1 + -1;
              }
            }
            local_1ec = (uint *)0x0;
            local_1e8 = 0;
            local_1e4 = 0;
            if (local_1bc != 0) {
              cVar1 = *(char *)(local_1bc + -1);
              if ((cVar1 == '\0') || (cVar1 == -1)) {
                FUN_0040ef10((undefined *)(local_1bc + -1));
              }
              else {
                *(char *)(local_1bc + -1) = cVar1 + -1;
              }
            }
            local_1bc = 0;
            local_1b8 = 0;
            local_1b4 = 0;
            if (local_1dc != (char *)0x0) {
              cVar1 = local_1dc[-1];
              if ((cVar1 != '\0') && (cVar1 != -1)) {
                local_1dc[-1] = cVar1 + -1;
                ExceptionList = local_c;
                return 0;
              }
              FUN_0040ef10(local_1dc + -1);
            }
            ExceptionList = local_c;
            return 0;
          }
        }
LAB_00404718:
        local_4._0_1_ = 1;
        FUN_00401380(local_1f0,'\x01');
        local_4 = (uint)local_4._1_3_ << 8;
        FUN_00401380(local_1c0,'\x01');
        local_4 = 0xffffffff;
        FUN_00401380(local_1e0,'\x01');
        ExceptionList = local_c;
        return -0x7ffefeef;
      }
      iVar6 = WSAGetLastError();
      FUN_00402040((byte *)s_connect_error__d_00420324);
    }
LAB_004044a8:
    local_4 = local_4 & 0xffffff00;
    FUN_00401380(local_1c0,'\x01');
    local_4 = 0xffffffff;
    FUN_00401380(local_1e0,'\x01');
  }
  ExceptionList = local_c;
  return iVar6;
}



undefined1 * __cdecl FUN_00404780(undefined1 *param_1,uint *param_2,char *param_3,char *param_4)

{
  char cVar1;
  bool bVar2;
  bool bVar3;
  uint *puVar4;
  uint *puVar5;
  undefined1 *puVar6;
  uint uVar7;
  undefined1 local_2c [4];
  uint local_28;
  int local_24;
  undefined1 local_1c [4];
  int local_18;
  undefined4 local_14;
  undefined4 local_10;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_0041942e;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  puVar4 = FUN_00410470(param_2,param_3);
  if (puVar4 == (uint *)0x0) {
    *(undefined4 *)(param_1 + 4) = 0;
    *(undefined4 *)(param_1 + 8) = 0;
    *(undefined4 *)(param_1 + 0xc) = 0;
    *param_1 = param_1._0_1_;
    ExceptionList = local_c;
    return param_1;
  }
  uVar7 = 0xffffffff;
  do {
    if (uVar7 == 0) break;
    uVar7 = uVar7 - 1;
    cVar1 = *param_3;
    param_3 = param_3 + 1;
  } while (cVar1 != '\0');
  puVar4 = (uint *)((int)puVar4 + (~uVar7 - 1));
  puVar5 = FUN_00410470(puVar4,param_4);
  if ((puVar5 == (uint *)0x0) || (puVar5 <= puVar4)) {
    local_1c[0] = param_1._0_1_;
    FUN_00401380(local_1c,'\0');
    bVar3 = false;
    bVar2 = true;
    puVar6 = local_1c;
    local_4 = 1;
  }
  else {
    local_2c[0] = param_1._0_1_;
    FUN_00401380(local_2c,'\0');
    FUN_00406fa0(local_2c);
    if (local_28 == 0) {
      uVar7 = 0;
    }
    else {
      uVar7 = local_24 + local_28;
    }
    FUN_00406fa0(local_2c);
    FUN_00406d10(local_2c,local_28,uVar7,(undefined1 *)puVar4,(int)puVar5);
    bVar3 = true;
    bVar2 = false;
    puVar6 = local_2c;
    local_4 = 0;
  }
  *param_1 = *puVar6;
  *(undefined4 *)(param_1 + 4) = 0;
  *(undefined4 *)(param_1 + 8) = 0;
  *(undefined4 *)(param_1 + 0xc) = 0;
  FUN_00401900(param_1,puVar6,0,0xffffffff);
  if (bVar2) {
    if (local_18 != 0) {
      cVar1 = *(char *)(local_18 + -1);
      if ((cVar1 == '\0') || (cVar1 == -1)) {
        FUN_0040ef10((char *)(local_18 + -1));
      }
      else {
        *(char *)(local_18 + -1) = cVar1 + -1;
      }
    }
    local_18 = 0;
    local_14 = 0;
    local_10 = 0;
  }
  if ((bVar3) && (local_28 != 0)) {
    cVar1 = *(char *)(local_28 - 1);
    if ((cVar1 != '\0') && (cVar1 != -1)) {
      *(char *)(local_28 - 1) = cVar1 + -1;
      ExceptionList = local_c;
      return param_1;
    }
    FUN_0040ef10((char *)(local_28 - 1));
  }
  ExceptionList = local_c;
  return param_1;
}



undefined4 * __thiscall FUN_00404940(void *this,byte param_1)

{
  *(undefined ***)this = &PTR_FUN_0041a240;
  if (*(SOCKET *)((int)this + 0xc) != 0xffffffff) {
    closesocket(*(SOCKET *)((int)this + 0xc));
    *(undefined4 *)((int)this + 0xc) = 0xffffffff;
  }
  if (*(HANDLE *)((int)this + 0x10) != (HANDLE)0xffffffff) {
    CloseHandle(*(HANDLE *)((int)this + 0x10));
    *(undefined4 *)((int)this + 0x10) = 0xffffffff;
  }
  if ((param_1 & 1) != 0) {
    FUN_0040ef10(this);
  }
  return this;
}



uint FUN_00404990(void)

{
  byte bVar1;
  char cVar2;
  int iVar3;
  char *lpText;
  byte *pbVar4;
  uint uVar5;
  undefined1 *puVar6;
  undefined4 uVar7;
  undefined1 local_2c [4];
  byte *local_28;
  undefined1 local_1c [4];
  int local_18;
  void *local_c;
  undefined1 *puStack_8;
  int local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_00419450;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  FUN_00401380(local_2c,'\0');
  local_4 = 0;
  iVar3 = FUN_00403ac0(s_http___tenhou_net_1_resinfo_rele_004203f0,local_2c);
  if (iVar3 == 0) {
    pbVar4 = local_28;
    if (local_28 == (byte *)0x0) {
      pbVar4 = (byte *)&DAT_0041a214;
    }
    iVar3 = FUN_004104f0(pbVar4,(byte *)s_BASEURL__0042037c,8);
    if (iVar3 != 0) {
      lpText = (char *)&DAT_0041a214;
      if (local_28 != (byte *)0x0) {
        lpText = (char *)local_28;
      }
      if ((byte *)lpText != (byte *)0x0) goto LAB_00404a43;
    }
    pbVar4 = local_28;
    if (local_28 == (byte *)0x0) {
      pbVar4 = (byte *)&DAT_0041a214;
    }
    iVar3 = FUN_004104f0(pbVar4 + 8,(byte *)s_http____00420374,7);
    if (iVar3 == 0) {
      puVar6 = FUN_004068d0(local_2c,local_1c,8,0xffffffff);
      local_4._0_1_ = 1;
      FUN_00401900(&DAT_004239a8,puVar6,0,0xffffffff);
      local_4 = (uint)local_4._1_3_ << 8;
      if (local_18 != 0) {
        cVar2 = *(char *)(local_18 + -1);
        if ((cVar2 == '\0') || (cVar2 == -1)) {
          FUN_0040ef10((char *)(local_18 + -1));
        }
        else {
          *(char *)(local_18 + -1) = cVar2 + -1;
        }
      }
      uVar7 = FUN_00402040((byte *)s_BASEURL__s_0042034c);
      if (local_28 != (byte *)0x0) {
        bVar1 = local_28[-1];
        if ((bVar1 != 0) && (bVar1 != 0xff)) {
          local_28[-1] = bVar1 - 1;
          ExceptionList = local_c;
          return CONCAT31((int3)((uint)uVar7 >> 8),1);
        }
        uVar7 = FUN_0040ef10(local_28 + -1);
      }
      ExceptionList = local_c;
      return CONCAT31((int3)((uint)uVar7 >> 8),1);
    }
    lpText = s_INVALID_BASEURL_HTTP_0042035c;
  }
  else {
    lpText = &DAT_00420388;
  }
LAB_00404a43:
  uVar5 = MessageBoxA((HWND)0x0,lpText,&DAT_004200cc,0);
  if (local_28 != (byte *)0x0) {
    bVar1 = local_28[-1];
    if ((bVar1 != 0) && (bVar1 != 0xff)) {
      local_28[-1] = bVar1 - 1;
      ExceptionList = local_c;
      return uVar5 & 0xffffff00;
    }
    uVar5 = FUN_0040ef10(local_28 + -1);
  }
  ExceptionList = local_c;
  return uVar5 & 0xffffff00;
}



undefined1 * __cdecl FUN_00404b60(undefined1 *param_1,LPCSTR param_2)

{
  byte bVar1;
  HANDLE hFile;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  byte *pbVar7;
  undefined4 *puVar8;
  byte *pbVar9;
  byte local_4a8 [35];
  undefined1 local_485;
  uint local_484;
  HANDLE local_480;
  int local_47c;
  undefined4 local_478;
  undefined4 local_474;
  undefined4 local_470;
  uint local_46c;
  int local_468;
  undefined1 local_464 [37];
  undefined1 auStack_43f [27];
  byte local_424;
  undefined4 local_423 [7];
  undefined1 local_404;
  undefined1 local_400 [1024];
  
  local_424 = 0;
  puVar8 = local_423;
  for (iVar4 = 8; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar8 = 0;
    puVar8 = puVar8 + 1;
  }
  local_4a8[0] = 0;
  local_4a8[1] = 0;
  local_4a8[2] = 0;
  local_4a8[3] = 0;
  local_4a8[4] = 0;
  local_4a8[5] = 0;
  local_4a8[6] = 0;
  local_4a8[7] = 0;
  local_4a8[8] = 0;
  local_4a8[9] = 0;
  local_4a8[10] = 0;
  local_4a8[0xb] = 0;
  local_4a8[0xc] = 0;
  local_4a8[0xd] = 0;
  local_4a8[0xe] = 0;
  local_4a8[0xf] = 0;
  local_480 = (HANDLE)0x0;
  hFile = CreateFileA(param_2,0x80000000,1,(LPSECURITY_ATTRIBUTES)0x0,3,0,(HANDLE)0x0);
  local_480 = hFile;
  if (hFile != (HANDLE)0xffffffff) {
    local_468 = 0;
    local_46c = 0;
    local_47c = 0x67452301;
    local_478 = 0xefcdab89;
    local_474 = 0x98badcfe;
    local_470 = 0x10325476;
    iVar4 = ReadFile(hFile,local_400,0x400,&local_484,(LPOVERLAPPED)0x0);
    uVar5 = local_484;
    while ((iVar4 != 0 && (uVar5 != 0))) {
      uVar2 = local_46c >> 3;
      local_46c = local_46c + uVar5 * 8;
      uVar2 = uVar2 & 0x3f;
      if (local_46c < uVar5 * 8) {
        local_468 = local_468 + 1;
      }
      local_468 = local_468 + (uVar5 >> 0x1d);
      uVar6 = -uVar2 + 0x40;
      local_484 = uVar5;
      if (uVar5 < uVar6) {
        uVar6 = 0;
      }
      else {
        FUN_00404e30(local_464 + uVar2,(int)local_400,uVar6);
        FUN_00404e50(&local_47c,(int)local_464);
        for (uVar2 = -uVar2 + 0x7f; uVar2 < uVar5; uVar2 = uVar2 + 0x40) {
          FUN_00404e50(&local_47c,(int)(auStack_43f + uVar2));
          uVar6 = uVar6 + 0x40;
          hFile = local_480;
        }
        uVar2 = 0;
      }
      FUN_00404e30(local_464 + uVar2,(int)(local_400 + uVar6),uVar5 - uVar6);
      iVar4 = ReadFile(hFile,local_400,0x400,&local_484,(LPOVERLAPPED)0x0);
      uVar5 = local_484;
    }
    FUN_00405760((int)local_4a8,&local_47c);
    CloseHandle(hFile);
  }
  local_4a8[0x10] = 0x30;
  local_4a8[0x11] = 0x31;
  local_4a8[0x12] = 0x32;
  local_4a8[0x13] = 0x33;
  local_4a8[0x14] = 0x34;
  local_4a8[0x15] = 0x35;
  local_4a8[0x16] = 0x36;
  local_4a8[0x17] = 0x37;
  local_4a8[0x18] = 0x38;
  local_4a8[0x19] = 0x39;
  local_4a8[0x1a] = 0x61;
  local_4a8[0x1b] = 0x62;
  local_4a8[0x1c] = 99;
  local_4a8[0x1d] = 100;
  local_4a8[0x1e] = 0x65;
  local_4a8[0x1f] = 0x66;
  iVar4 = 0;
  pbVar7 = (byte *)local_423;
  do {
    pbVar9 = local_4a8 + iVar4;
    bVar1 = local_4a8[(*pbVar9 & 0xf) + 0x10];
    iVar4 = iVar4 + 1;
    pbVar7[-1] = local_4a8[(*pbVar9 >> 4) + 0x10];
    *pbVar7 = bVar1;
    pbVar7 = pbVar7 + 2;
  } while (iVar4 < 0x10);
  *param_1 = local_485;
  uVar5 = 0xffffffff;
  local_404 = 0;
  pbVar7 = &local_424;
  do {
    if (uVar5 == 0) break;
    uVar5 = uVar5 - 1;
    bVar1 = *pbVar7;
    pbVar7 = pbVar7 + 1;
  } while (bVar1 != 0);
  uVar5 = ~uVar5 - 1;
  *(undefined4 *)(param_1 + 4) = 0;
  *(undefined4 *)(param_1 + 8) = 0;
  *(undefined4 *)(param_1 + 0xc) = 0;
  uVar3 = FUN_00401570(param_1,uVar5,'\x01');
  if ((char)uVar3 != '\0') {
    pbVar7 = &local_424;
    pbVar9 = *(byte **)(param_1 + 4);
    for (uVar2 = uVar5 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
      *(undefined4 *)pbVar9 = *(undefined4 *)pbVar7;
      pbVar7 = pbVar7 + 4;
      pbVar9 = pbVar9 + 4;
    }
    for (uVar2 = uVar5 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
      *pbVar9 = *pbVar7;
      pbVar7 = pbVar7 + 1;
      pbVar9 = pbVar9 + 1;
    }
    *(uint *)(param_1 + 8) = uVar5;
    *(undefined1 *)(uVar5 + *(int *)(param_1 + 4)) = 0;
  }
  return param_1;
}



void __cdecl FUN_00404e30(undefined1 *param_1,int param_2,int param_3)

{
  int iVar1;
  
  if (param_3 != 0) {
    iVar1 = param_2 - (int)param_1;
    do {
      *param_1 = param_1[iVar1];
      param_1 = param_1 + 1;
      param_3 = param_3 + -1;
    } while (param_3 != 0);
  }
  return;
}



void __cdecl FUN_00404e50(int *param_1,int param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  undefined2 *puVar7;
  undefined2 *puVar8;
  uint uVar9;
  int *piVar10;
  int local_40 [4];
  int local_30;
  int local_2c;
  int local_28;
  int local_24;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  int local_8;
  int local_4;
  
  puVar7 = (undefined2 *)(param_2 + 2);
  piVar10 = local_40;
  do {
    puVar8 = puVar7 + 2;
    *piVar10 = CONCAT31(CONCAT21(*puVar7,*(undefined1 *)((int)puVar7 + -1)),
                        *(undefined1 *)(puVar7 + -1));
    puVar7 = puVar8;
    piVar10 = piVar10 + 1;
  } while ((uint)((int)puVar8 + (-2 - param_2)) < 0x40);
  uVar3 = param_1[1];
  uVar5 = param_1[2];
  uVar1 = *param_1 + -0x28955b88 + (~uVar3 & param_1[3] | uVar5 & uVar3) + local_40[0];
  uVar2 = (uVar1 >> 0x19 | uVar1 * 0x80) + uVar3;
  uVar1 = param_1[3] + 0xe8c7b756U + (~uVar2 & uVar5 | uVar3 & uVar2) + local_40[1];
  uVar4 = (uVar1 >> 0x14 | uVar1 * 0x1000) + uVar2;
  uVar1 = uVar5 + 0x242070db + (~uVar4 & uVar3 | uVar4 & uVar2) + local_40[2];
  uVar6 = (uVar1 >> 0xf | uVar1 * 0x20000) + uVar4;
  uVar1 = uVar3 + 0xc1bdceee + (~uVar6 & uVar2 | uVar4 & uVar6) + local_40[3];
  uVar9 = (uVar1 * 0x400000 | uVar1 >> 10) + uVar6;
  uVar1 = uVar2 + 0xf57c0faf + (~uVar9 & uVar4 | uVar6 & uVar9) + local_30;
  uVar3 = (uVar1 >> 0x19 | uVar1 * 0x80) + uVar9;
  uVar1 = uVar4 + 0x4787c62a + (~uVar3 & uVar6 | uVar9 & uVar3) + local_2c;
  uVar5 = (uVar1 >> 0x14 | uVar1 * 0x1000) + uVar3;
  uVar1 = uVar6 + 0xa8304613 + (~uVar5 & uVar9 | uVar5 & uVar3) + local_28;
  uVar2 = (uVar1 >> 0xf | uVar1 * 0x20000) + uVar5;
  uVar1 = uVar9 + 0xfd469501 + (~uVar2 & uVar3 | uVar5 & uVar2) + local_24;
  uVar4 = (uVar1 * 0x400000 | uVar1 >> 10) + uVar2;
  uVar1 = uVar3 + 0x698098d8 + (~uVar4 & uVar5 | uVar2 & uVar4) + local_20;
  uVar3 = (uVar1 >> 0x19 | uVar1 * 0x80) + uVar4;
  uVar1 = uVar5 + 0x8b44f7af + (~uVar3 & uVar2 | uVar4 & uVar3) + local_1c;
  uVar5 = (uVar1 >> 0x14 | uVar1 * 0x1000) + uVar3;
  uVar1 = (uVar2 - 0xa44f) + (~uVar5 & uVar4 | uVar5 & uVar3) + local_18;
  uVar2 = (uVar1 >> 0xf | uVar1 * 0x20000) + uVar5;
  uVar1 = uVar4 + 0x895cd7be + (~uVar2 & uVar3 | uVar5 & uVar2) + local_14;
  uVar4 = (uVar1 * 0x400000 | uVar1 >> 10) + uVar2;
  uVar1 = uVar3 + 0x6b901122 + (~uVar4 & uVar5 | uVar2 & uVar4) + local_10;
  uVar3 = (uVar1 >> 0x19 | uVar1 * 0x80) + uVar4;
  uVar1 = uVar5 + 0xfd987193 + (~uVar3 & uVar2 | uVar4 & uVar3) + local_c;
  uVar5 = (uVar1 >> 0x14 | uVar1 * 0x1000) + uVar3;
  uVar1 = uVar2 + 0xa679438e + (~uVar5 & uVar4 | uVar5 & uVar3) + local_8;
  uVar2 = (uVar1 >> 0xf | uVar1 * 0x20000) + uVar5;
  uVar1 = uVar4 + 0x49b40821 + (~uVar2 & uVar3 | uVar5 & uVar2) + local_4;
  uVar4 = (uVar1 * 0x400000 | uVar1 >> 10) + uVar2;
  uVar1 = uVar3 + 0xf61e2562 + (~uVar5 & uVar2 | uVar5 & uVar4) + local_40[1];
  uVar3 = (uVar1 >> 0x1b | uVar1 * 0x20) + uVar4;
  uVar1 = uVar5 + 0xc040b340 + (~uVar2 & uVar4 | uVar2 & uVar3) + local_28;
  uVar5 = (uVar1 >> 0x17 | uVar1 * 0x200) + uVar3;
  uVar1 = uVar2 + 0x265e5a51 + (~uVar4 & uVar3 | uVar5 & uVar4) + local_14;
  uVar2 = (uVar1 >> 0x12 | uVar1 * 0x4000) + uVar5;
  uVar1 = uVar4 + 0xe9b6c7aa + (~uVar3 & uVar5 | uVar2 & uVar3) + local_40[0];
  uVar4 = (uVar1 * 0x100000 | uVar1 >> 0xc) + uVar2;
  uVar1 = uVar3 + 0xd62f105d + (~uVar5 & uVar2 | uVar5 & uVar4) + local_2c;
  uVar3 = (uVar1 >> 0x1b | uVar1 * 0x20) + uVar4;
  uVar1 = uVar5 + 0x2441453 + (~uVar2 & uVar4 | uVar2 & uVar3) + local_18;
  uVar5 = (uVar1 >> 0x17 | uVar1 * 0x200) + uVar3;
  uVar1 = uVar2 + 0xd8a1e681 + (~uVar4 & uVar3 | uVar5 & uVar4) + local_4;
  uVar2 = (uVar1 >> 0x12 | uVar1 * 0x4000) + uVar5;
  uVar1 = uVar4 + 0xe7d3fbc8 + (~uVar3 & uVar5 | uVar2 & uVar3) + local_30;
  uVar4 = (uVar1 * 0x100000 | uVar1 >> 0xc) + uVar2;
  uVar1 = uVar3 + 0x21e1cde6 + (~uVar5 & uVar2 | uVar5 & uVar4) + local_1c;
  uVar3 = (uVar1 >> 0x1b | uVar1 * 0x20) + uVar4;
  uVar1 = uVar5 + 0xc33707d6 + (~uVar2 & uVar4 | uVar2 & uVar3) + local_8;
  uVar5 = (uVar1 >> 0x17 | uVar1 * 0x200) + uVar3;
  uVar1 = uVar2 + 0xf4d50d87 + (~uVar4 & uVar3 | uVar5 & uVar4) + local_40[3];
  uVar2 = (uVar1 >> 0x12 | uVar1 * 0x4000) + uVar5;
  uVar1 = uVar4 + 0x455a14ed + (~uVar3 & uVar5 | uVar2 & uVar3) + local_20;
  uVar4 = (uVar1 * 0x100000 | uVar1 >> 0xc) + uVar2;
  uVar1 = uVar3 + 0xa9e3e905 + (~uVar5 & uVar2 | uVar5 & uVar4) + local_c;
  uVar3 = (uVar1 >> 0x1b | uVar1 * 0x20) + uVar4;
  uVar1 = uVar5 + 0xfcefa3f8 + (~uVar2 & uVar4 | uVar2 & uVar3) + local_40[2];
  uVar5 = (uVar1 >> 0x17 | uVar1 * 0x200) + uVar3;
  uVar1 = uVar2 + 0x676f02d9 + (~uVar4 & uVar3 | uVar5 & uVar4) + local_24;
  uVar2 = (uVar1 >> 0x12 | uVar1 * 0x4000) + uVar5;
  uVar1 = uVar4 + 0x8d2a4c8a + (~uVar3 & uVar5 | uVar2 & uVar3) + local_10;
  uVar4 = (uVar1 * 0x100000 | uVar1 >> 0xc) + uVar2;
  uVar1 = (uVar3 - 0x5c6be) + (uVar5 ^ uVar2 ^ uVar4) + local_2c;
  uVar3 = (uVar1 >> 0x1c | uVar1 * 0x10) + uVar4;
  uVar1 = uVar5 + 0x8771f681 + (uVar2 ^ uVar4 ^ uVar3) + local_20;
  uVar6 = (uVar1 >> 0x15 | uVar1 * 0x800) + uVar3;
  uVar1 = uVar2 + 0x6d9d6122 + (uVar6 ^ uVar4 ^ uVar3) + local_14;
  uVar2 = (uVar1 >> 0x10 | uVar1 * 0x10000) + uVar6;
  uVar1 = uVar4 + 0xfde5380c + (uVar6 ^ uVar2 ^ uVar3) + local_8;
  uVar5 = (uVar1 * 0x800000 | uVar1 >> 9) + uVar2;
  uVar1 = uVar3 + 0xa4beea44 + (uVar6 ^ uVar2 ^ uVar5) + local_40[1];
  uVar3 = (uVar1 >> 0x1c | uVar1 * 0x10) + uVar5;
  uVar1 = uVar6 + 0x4bdecfa9 + (uVar2 ^ uVar5 ^ uVar3) + local_30;
  uVar4 = (uVar1 >> 0x15 | uVar1 * 0x800) + uVar3;
  uVar1 = uVar2 + 0xf6bb4b60 + (uVar4 ^ uVar5 ^ uVar3) + local_24;
  uVar2 = (uVar1 >> 0x10 | uVar1 * 0x10000) + uVar4;
  uVar1 = uVar5 + 0xbebfbc70 + (uVar4 ^ uVar2 ^ uVar3) + local_18;
  uVar5 = (uVar1 * 0x800000 | uVar1 >> 9) + uVar2;
  uVar1 = uVar3 + 0x289b7ec6 + (uVar4 ^ uVar2 ^ uVar5) + local_c;
  uVar3 = (uVar1 >> 0x1c | uVar1 * 0x10) + uVar5;
  uVar1 = uVar4 + 0xeaa127fa + (uVar2 ^ uVar5 ^ uVar3) + local_40[0];
  uVar4 = (uVar1 >> 0x15 | uVar1 * 0x800) + uVar3;
  uVar1 = uVar2 + 0xd4ef3085 + (uVar4 ^ uVar5 ^ uVar3) + local_40[3];
  uVar6 = (uVar1 >> 0x10 | uVar1 * 0x10000) + uVar4;
  uVar1 = uVar5 + 0x4881d05 + (uVar4 ^ uVar6 ^ uVar3) + local_28;
  uVar5 = (uVar1 * 0x800000 | uVar1 >> 9) + uVar6;
  uVar1 = uVar3 + 0xd9d4d039 + (uVar4 ^ uVar6 ^ uVar5) + local_1c;
  uVar3 = (uVar1 >> 0x1c | uVar1 * 0x10) + uVar5;
  uVar1 = uVar4 + 0xe6db99e5 + (uVar6 ^ uVar5 ^ uVar3) + local_10;
  uVar2 = (uVar1 >> 0x15 | uVar1 * 0x800) + uVar3;
  uVar1 = uVar6 + 0x1fa27cf8 + (uVar2 ^ uVar5 ^ uVar3) + local_4;
  uVar4 = (uVar1 >> 0x10 | uVar1 * 0x10000) + uVar2;
  uVar1 = uVar5 + 0xc4ac5665 + (uVar2 ^ uVar4 ^ uVar3) + local_40[2];
  uVar5 = (uVar1 * 0x800000 | uVar1 >> 9) + uVar4;
  uVar1 = uVar3 + 0xf4292244 + ((~uVar2 | uVar5) ^ uVar4) + local_40[0];
  uVar3 = (uVar1 >> 0x1a | uVar1 * 0x40) + uVar5;
  uVar1 = uVar2 + 0x432aff97 + ((~uVar4 | uVar3) ^ uVar5) + local_24;
  uVar2 = (uVar1 >> 0x16 | uVar1 * 0x400) + uVar3;
  uVar1 = uVar4 + 0xab9423a7 + ((~uVar5 | uVar2) ^ uVar3) + local_8;
  uVar4 = (uVar1 >> 0x11 | uVar1 * 0x8000) + uVar2;
  uVar1 = uVar5 + 0xfc93a039 + ((~uVar3 | uVar4) ^ uVar2) + local_2c;
  uVar5 = (uVar1 * 0x200000 | uVar1 >> 0xb) + uVar4;
  uVar1 = uVar3 + 0x655b59c3 + ((~uVar2 | uVar5) ^ uVar4) + local_10;
  uVar3 = (uVar1 >> 0x1a | uVar1 * 0x40) + uVar5;
  uVar1 = uVar2 + 0x8f0ccc92 + ((~uVar4 | uVar3) ^ uVar5) + local_40[3];
  uVar2 = (uVar1 >> 0x16 | uVar1 * 0x400) + uVar3;
  uVar1 = (uVar4 - 0x100b83) + ((~uVar5 | uVar2) ^ uVar3) + local_18;
  uVar4 = (uVar1 >> 0x11 | uVar1 * 0x8000) + uVar2;
  uVar1 = uVar5 + 0x85845dd1 + ((~uVar3 | uVar4) ^ uVar2) + local_40[1];
  uVar5 = (uVar1 * 0x200000 | uVar1 >> 0xb) + uVar4;
  uVar1 = uVar3 + 0x6fa87e4f + ((~uVar2 | uVar5) ^ uVar4) + local_20;
  uVar3 = (uVar1 >> 0x1a | uVar1 * 0x40) + uVar5;
  uVar1 = uVar2 + 0xfe2ce6e0 + ((~uVar4 | uVar3) ^ uVar5) + local_4;
  uVar2 = (uVar1 >> 0x16 | uVar1 * 0x400) + uVar3;
  uVar1 = uVar4 + 0xa3014314 + ((~uVar5 | uVar2) ^ uVar3) + local_28;
  uVar4 = (uVar1 >> 0x11 | uVar1 * 0x8000) + uVar2;
  uVar1 = uVar5 + 0x4e0811a1 + ((~uVar3 | uVar4) ^ uVar2) + local_c;
  uVar5 = (uVar1 * 0x200000 | uVar1 >> 0xb) + uVar4;
  uVar1 = uVar3 + 0xf7537e82 + ((~uVar2 | uVar5) ^ uVar4) + local_30;
  uVar3 = (uVar1 >> 0x1a | uVar1 * 0x40) + uVar5;
  uVar1 = uVar2 + 0xbd3af235 + ((~uVar4 | uVar3) ^ uVar5) + local_14;
  uVar2 = (uVar1 >> 0x16 | uVar1 * 0x400) + uVar3;
  uVar1 = uVar4 + 0x2ad7d2bb + ((~uVar5 | uVar2) ^ uVar3) + local_40[2];
  uVar4 = (uVar1 >> 0x11 | uVar1 * 0x8000) + uVar2;
  uVar1 = uVar5 + 0xeb86d391 + ((~uVar3 | uVar4) ^ uVar2) + local_1c;
  *param_1 = uVar3 + *param_1;
  param_1[1] = (uVar1 * 0x200000 | uVar1 >> 0xb) + param_1[1] + uVar4;
  param_1[2] = param_1[2] + uVar4;
  param_1[3] = param_1[3] + uVar2;
  return;
}



void __cdecl FUN_00405760(int param_1,int *param_2)

{
  int *piVar1;
  undefined1 *puVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  int *piVar6;
  int iVar7;
  int *piVar8;
  int *piVar9;
  undefined1 auStackY_7f [19];
  undefined4 uStackY_6c;
  undefined1 local_48 [8];
  undefined1 local_40 [64];
  
  piVar9 = param_2;
  local_40[0] = 0x80;
  local_40[1] = 0;
  local_40[2] = 0;
  local_40[3] = 0;
  local_40[4] = 0;
  local_40[5] = 0;
  local_40[6] = 0;
  local_40[7] = 0;
  local_40[8] = 0;
  local_40[9] = 0;
  local_40[10] = 0;
  local_40[0xb] = 0;
  local_40[0xc] = 0;
  local_40[0xd] = 0;
  local_40[0xe] = 0;
  local_40[0xf] = 0;
  local_40[0x10] = 0;
  local_40[0x11] = 0;
  local_40[0x12] = 0;
  local_40[0x13] = 0;
  local_40[0x14] = 0;
  local_40[0x15] = 0;
  local_40[0x16] = 0;
  local_40[0x17] = 0;
  local_40[0x18] = 0;
  local_40[0x19] = 0;
  local_40[0x1a] = 0;
  local_40[0x1b] = 0;
  local_40[0x1c] = 0;
  local_40[0x1d] = 0;
  local_40[0x1e] = 0;
  local_40[0x1f] = 0;
  local_40[0x20] = 0;
  local_40[0x21] = 0;
  local_40[0x22] = 0;
  local_40[0x23] = 0;
  local_40[0x24] = 0;
  local_40[0x25] = 0;
  local_40[0x26] = 0;
  local_40[0x27] = 0;
  local_40[0x28] = 0;
  local_40[0x29] = 0;
  local_40[0x2a] = 0;
  local_40[0x2b] = 0;
  local_40[0x2c] = 0;
  local_40[0x2d] = 0;
  local_40[0x2e] = 0;
  local_40[0x2f] = 0;
  local_40[0x30] = 0;
  local_40[0x31] = 0;
  local_40[0x32] = 0;
  local_40[0x33] = 0;
  local_40[0x34] = 0;
  local_40[0x35] = 0;
  local_40[0x36] = 0;
  local_40[0x37] = 0;
  local_40[0x38] = 0;
  local_40[0x39] = 0;
  local_40[0x3a] = 0;
  local_40[0x3b] = 0;
  local_40[0x3c] = 0;
  local_40[0x3d] = 0;
  local_40[0x3e] = 0;
  local_40[0x3f] = 0;
  uVar5 = 0;
  piVar8 = param_2 + 4;
  do {
    local_48[uVar5] = (char)*piVar8;
    iVar7 = *piVar8;
    local_48[uVar5 + 1] = (char)((uint)iVar7 >> 8);
    local_48[uVar5 + 2] = (char)((uint)iVar7 >> 0x10);
    local_48[uVar5 + 3] = (char)((uint)iVar7 >> 0x18);
    uVar5 = uVar5 + 4;
    piVar8 = piVar8 + 1;
  } while (uVar5 < 8);
  iVar7 = 0x38;
  uVar5 = (uint)param_2[4] >> 3 & 0x3f;
  if (0x37 < uVar5) {
    iVar7 = 0x78;
  }
  piVar8 = (int *)(iVar7 - uVar5);
  uVar5 = param_2[4] + (int)piVar8 * 8;
  uVar3 = (uint)param_2[4] >> 3 & 0x3f;
  param_2[4] = uVar5;
  if (uVar5 < (uint)((int)piVar8 * 8)) {
    param_2[5] = param_2[5] + 1;
  }
  param_2[5] = param_2[5] + ((uint)piVar8 >> 0x1d);
  piVar6 = (int *)(-uVar3 + 0x40);
  if (piVar8 < piVar6) {
    param_2 = (int *)0x0;
  }
  else {
    piVar1 = (int *)0x0;
    if (piVar6 != (int *)0x0) {
      do {
        *(undefined1 *)(uVar3 + 0x18 + (int)param_2 + (int)piVar1) = local_40[(int)piVar1];
        piVar1 = (int *)((int)piVar1 + 1);
      } while (piVar1 < piVar6);
    }
    FUN_00404e50(param_2,(int)(param_2 + 6));
    piVar1 = (int *)(-uVar3 + 0x7f);
    param_2 = piVar6;
    if (piVar1 < piVar8) {
      do {
        FUN_00404e50(piVar9,(int)(auStackY_7f + (int)piVar1));
        piVar1 = piVar1 + 0x10;
        param_2 = param_2 + 0x10;
      } while (piVar1 < piVar8);
    }
    uVar3 = 0;
  }
  uVar5 = 0;
  if (piVar8 != param_2) {
    do {
      *(undefined1 *)((int)piVar9 + uVar5 + uVar3 + 0x18) = (local_40 + (int)param_2)[uVar5];
      uVar5 = uVar5 + 1;
    } while (uVar5 < (uint)((int)piVar8 - (int)param_2));
  }
  uVar3 = piVar9[4] + 0x40;
  uVar5 = (uint)piVar9[4] >> 3 & 0x3f;
  piVar9[4] = uVar3;
  if (uVar3 < 0x40) {
    piVar9[5] = piVar9[5] + 1;
  }
  uVar3 = -uVar5 + 0x40;
  if (uVar3 < 9) {
    uVar4 = 0;
    if (uVar3 != 0) {
      do {
        *(undefined1 *)((int)piVar9 + uVar4 + uVar5 + 0x18) = local_48[uVar4];
        uVar4 = uVar4 + 1;
      } while (uVar4 < uVar3);
    }
    FUN_00404e50(piVar9,(int)(piVar9 + 6));
    if (-uVar5 + 0x7f < 8) {
      do {
        FUN_00404e50(piVar9,(int)(local_48 + uVar3));
        uVar3 = uVar3 + 0x40;
      } while (uVar3 < 0xffffffc9);
    }
    uVar5 = 0;
  }
  else {
    uVar3 = 0;
  }
  uStackY_6c = 0x4059fd;
  FUN_00404e30((undefined1 *)(uVar5 + 0x18 + (int)piVar9),(int)(local_48 + uVar3),8 - uVar3);
  puVar2 = (undefined1 *)(param_1 + 1);
  piVar8 = piVar9;
  do {
    puVar2[-1] = (char)*piVar8;
    *puVar2 = (char)((uint)*piVar8 >> 8);
    puVar2[1] = (char)((uint)*piVar8 >> 0x10);
    puVar2[2] = (char)((uint)*piVar8 >> 0x18);
    puVar2 = puVar2 + 4;
    piVar8 = piVar8 + 1;
  } while (puVar2 + (-1 - param_1) < (undefined1 *)0x10);
  for (iVar7 = 0x16; iVar7 != 0; iVar7 = iVar7 + -1) {
    *piVar9 = 0;
    piVar9 = piVar9 + 1;
  }
  return;
}



bool __cdecl FUN_00405a50(char *param_1)

{
  char cVar1;
  undefined1 *puVar2;
  LPCSTR lpFileName;
  DWORD DVar3;
  uint uVar4;
  char *pcVar5;
  undefined1 local_2c [4];
  int local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined1 local_1c [4];
  int local_18;
  undefined4 local_14;
  undefined4 local_10;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_00419468;
  local_c = ExceptionList;
  local_2c[0] = param_1._0_1_;
  uVar4 = 0xffffffff;
  pcVar5 = param_1;
  do {
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    cVar1 = *pcVar5;
    pcVar5 = pcVar5 + 1;
  } while (cVar1 != '\0');
  local_28 = 0;
  local_24 = 0;
  local_20 = 0;
  ExceptionList = &local_c;
  FUN_004014b0(local_2c,(undefined4 *)param_1,~uVar4 - 1);
  local_4 = 0;
  puVar2 = FUN_00401be0(local_1c,local_2c,s__skipupdate_0042042c);
  lpFileName = *(LPCSTR *)(puVar2 + 4);
  if (lpFileName == (LPCSTR)0x0) {
    lpFileName = "";
  }
  DVar3 = GetFileAttributesA(lpFileName);
  if (local_18 != 0) {
    cVar1 = *(char *)(local_18 + -1);
    if ((cVar1 == '\0') || (cVar1 == -1)) {
      FUN_0040ef10((char *)(local_18 + -1));
    }
    else {
      *(char *)(local_18 + -1) = cVar1 + -1;
    }
  }
  local_18 = 0;
  local_14 = 0;
  local_10 = 0;
  local_4 = 0xffffffff;
  if (local_28 != 0) {
    cVar1 = *(char *)(local_28 + -1);
    if ((cVar1 == '\0') || (cVar1 == -1)) {
      FUN_0040ef10((char *)(local_28 + -1));
    }
    else {
      *(char *)(local_28 + -1) = cVar1 + -1;
    }
  }
  local_28 = 0;
  local_24 = 0;
  local_20 = 0;
  if (DVar3 != 0xffffffff) {
    FUN_00402040((byte *)s_SKIPUPDATE__s_0042041c);
  }
  ExceptionList = local_c;
  return DVar3 != 0xffffffff;
}



void FUN_00405b70(void)

{
  bool bVar1;
  int *piVar2;
  FILE *pFVar3;
  undefined3 extraout_var;
  uint uVar5;
  undefined3 extraout_var_00;
  LPCSTR in_stack_00001004;
  LPCSTR in_stack_00001008;
  int iVar4;
  
  FUN_004101e0();
  piVar2 = (int *)FUN_00407180(in_stack_00001004,&DAT_0042043c);
  pFVar3 = (FILE *)FUN_004107c6(in_stack_00001008,&DAT_00420438);
  if (piVar2 != (int *)0x0) {
    if (pFVar3 != (FILE *)0x0) {
      bVar1 = FUN_00407a50((int)piVar2);
      iVar4 = CONCAT31(extraout_var,bVar1);
      while (iVar4 == 0) {
        uVar5 = FUN_004076f0(piVar2,(uint *)&stack0x00000000,0x1000);
        FUN_0041065c(&stack0x00000000,1,uVar5,(int *)pFVar3);
        bVar1 = FUN_00407a50((int)piVar2);
        iVar4 = CONCAT31(extraout_var_00,bVar1);
      }
    }
    FUN_00407ad0(piVar2);
  }
  if (pFVar3 != (FILE *)0x0) {
    FUN_004105df(pFVar3);
  }
  return;
}



void __cdecl FUN_00405c10(uint param_1,char *param_2)

{
  char cVar1;
  DWORD DVar2;
  DWORD DVar3;
  
  DVar2 = GetTickCount();
  cVar1 = *param_2;
  while ((cVar1 != '\0' && (DVar3 = GetTickCount(), DVar3 - DVar2 < param_1))) {
    Sleep(1);
    cVar1 = *param_2;
  }
  return;
}



undefined4 __cdecl
FUN_00405c50(HWND param_1,undefined4 param_2,char *param_3,char *param_4,byte *param_5)

{
  byte bVar1;
  char cVar2;
  bool bVar3;
  char *pcVar4;
  undefined1 *puVar5;
  void *this;
  DWORD DVar6;
  LPCSTR lpText;
  uint uVar7;
  uint *puVar8;
  byte *pbVar9;
  undefined1 local_188 [4];
  int iStack_184;
  uint local_180;
  undefined4 uStack_17c;
  int local_178 [4];
  undefined1 local_168 [4];
  uint *local_164;
  undefined4 uStack_160;
  undefined4 uStack_15c;
  undefined1 auStack_158 [16];
  undefined1 local_148 [16];
  undefined4 auStack_138 [5];
  undefined1 local_124 [16];
  undefined4 local_114;
  char acStack_110 [260];
  void *local_c;
  undefined1 *puStack_8;
  uint local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004194e3;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  bVar3 = FUN_00405a50(param_4);
  if (!bVar3) {
    FUN_00406490(local_178,param_2,param_3);
    local_4 = 0;
    FUN_004066d0(&local_114);
    local_4._0_1_ = 1;
    FUN_00404b60(local_188,param_4);
    uVar7 = 0xffffffff;
    pbVar9 = param_5;
    do {
      if (uVar7 == 0) break;
      uVar7 = uVar7 - 1;
      bVar1 = *pbVar9;
      pbVar9 = pbVar9 + 1;
    } while (bVar1 != 0);
    local_4._0_1_ = 2;
    uVar7 = FUN_004070e0(local_188,0,local_180,param_5,~uVar7 - 1);
    if (uVar7 == 0) {
      local_4._0_1_ = 1;
      FUN_00401380(local_188,'\x01');
    }
    else {
      FUN_00401380(local_148,'\0');
      uVar7 = 0xffffffff;
      pcVar4 = param_4;
      do {
        if (uVar7 == 0) break;
        uVar7 = uVar7 - 1;
        cVar2 = *pcVar4;
        pcVar4 = pcVar4 + 1;
      } while (cVar2 != '\0');
      FUN_004014b0(local_148,(undefined4 *)param_4,~uVar7 - 1);
      puVar5 = local_148;
      local_4._0_1_ = 3;
      pcVar4 = (char *)FUN_00403ab0();
      puVar5 = FUN_004017a0(local_124,pcVar4,(int)puVar5);
      local_4._0_1_ = 4;
      FUN_00401be0(local_168,puVar5,&DAT_004204fc);
      local_4._0_1_ = 7;
      FUN_00401380(local_124,'\x01');
      local_4 = CONCAT31(local_4._1_3_,6);
      FUN_00401380(local_148,'\x01');
      FUN_00402040((byte *)s_GOING_DOWNLOAD__s_BEGIN_004204e0);
      while( true ) {
        puVar8 = local_164;
        if (local_164 == (uint *)0x0) {
          puVar8 = &DAT_0041a214;
        }
        if (&stack0x00000000 != (undefined1 *)0x178) {
          (**(code **)(local_178[0] + 4))();
        }
        this = (void *)FUN_00406090(auStack_138);
        local_4._0_1_ = 8;
        DVar6 = FUN_004060b0(this,puVar8,acStack_110,local_178);
        local_4 = CONCAT31(local_4._1_3_,6);
        FUN_00403e00(auStack_138);
        if (&stack0x00000000 != (undefined1 *)0x178) {
          (**(code **)(local_178[0] + 8))();
        }
        if ((DVar6 == 0) || (*param_3 == '\0')) break;
        ShowWindow(param_1,5);
        FUN_00405c10(10000,param_3);
      }
      FUN_00402040((byte *)s_GOING_DOWNLOAD__s_END_004204c8);
      if (*param_3 == '\0') {
        local_4._0_1_ = 2;
        FUN_00401380(local_168,'\x01');
        local_4 = CONCAT31(local_4._1_3_,1);
        FUN_00401380(local_188,'\x01');
LAB_00405fb1:
        local_4 = local_4 & 0xffffff00;
        FUN_00406780(&local_114);
        local_4 = 0xffffffff;
        FUN_004064b0(local_178);
        ExceptionList = local_c;
        return 1;
      }
      FUN_00405b70();
      puVar5 = FUN_00404b60(auStack_158,param_4);
      uVar7 = 0xffffffff;
      pbVar9 = param_5;
      do {
        if (uVar7 == 0) break;
        uVar7 = uVar7 - 1;
        bVar1 = *pbVar9;
        pbVar9 = pbVar9 + 1;
      } while (bVar1 != 0);
      local_4._0_1_ = 9;
      uVar7 = FUN_004070e0(puVar5,0,*(uint *)(puVar5 + 8),param_5,~uVar7 - 1);
      local_4._0_1_ = 6;
      FUN_00401380(auStack_158,'\x01');
      if (uVar7 != 0) {
        FUN_00401380(auStack_158,'\0');
        uVar7 = 0xffffffff;
        pcVar4 = param_4;
        do {
          if (uVar7 == 0) break;
          uVar7 = uVar7 - 1;
          cVar2 = *pcVar4;
          pcVar4 = pcVar4 + 1;
        } while (cVar2 != '\0');
        FUN_004014b0(auStack_158,(undefined4 *)param_4,~uVar7 - 1);
        local_4._0_1_ = 10;
        puVar5 = FUN_00401be0((undefined1 *)auStack_138,auStack_158,&DAT_00420440);
        lpText = *(LPCSTR *)(puVar5 + 4);
        if (lpText == (LPCSTR)0x0) {
          lpText = "";
        }
        MessageBoxA(param_1,lpText,&DAT_004200cc,0x10);
        FUN_00401380(auStack_138,'\x01');
        local_4._0_1_ = 6;
        FUN_00401380(auStack_158,'\x01');
        local_4 = CONCAT31(local_4._1_3_,2);
        FUN_00401380(local_168,'\x01');
        if (iStack_184 != 0) {
          cVar2 = *(char *)(iStack_184 + -1);
          if ((cVar2 == '\0') || (cVar2 == -1)) {
            FUN_0040ef10((char *)(iStack_184 + -1));
          }
          else {
            *(char *)(iStack_184 + -1) = cVar2 + -1;
          }
        }
        iStack_184 = 0;
        local_180 = 0;
        uStack_17c = 0;
        goto LAB_00405fb1;
      }
      if (local_164 != (uint *)0x0) {
        cVar2 = *(char *)((int)local_164 + -1);
        if ((cVar2 == '\0') || (cVar2 == -1)) {
          FUN_0040ef10((char *)((int)local_164 + -1));
        }
        else {
          *(char *)((int)local_164 + -1) = cVar2 + -1;
        }
      }
      local_164 = (uint *)0x0;
      uStack_160 = 0;
      uStack_15c = 0;
      if (iStack_184 != 0) {
        cVar2 = *(char *)(iStack_184 + -1);
        if ((cVar2 == '\0') || (cVar2 == -1)) {
          FUN_0040ef10((char *)(iStack_184 + -1));
        }
        else {
          *(char *)(iStack_184 + -1) = cVar2 + -1;
        }
      }
      iStack_184 = 0;
      local_180 = 0;
      uStack_17c = 0;
    }
    local_4 = (uint)local_4._1_3_ << 8;
    FUN_00406780(&local_114);
    local_4 = 0xffffffff;
    FUN_004064b0(local_178);
  }
  ExceptionList = local_c;
  return 0;
}



void __fastcall FUN_00406090(undefined4 *param_1)

{
  param_1[1] = 0;
  param_1[2] = 0;
  *param_1 = &PTR_FUN_0041a240;
  param_1[3] = 0xffffffff;
  param_1[4] = 0xffffffff;
  return;
}



DWORD __thiscall FUN_004060b0(void *this,uint *param_1,char *param_2,int *param_3)

{
  uint uVar1;
  int *piVar2;
  HANDLE pvVar3;
  DWORD DVar4;
  uint *nNumberOfBytesToWrite;
  BOOL BVar5;
  int iVar6;
  uint uVar7;
  
  if (*(int *)((int)this + 0x10) != -1) {
    return 1;
  }
  pvVar3 = CreateFileA(param_2,0x40000000,1,(LPSECURITY_ATTRIBUTES)0x0,2,0,(HANDLE)0x0);
  *(HANDLE *)((int)this + 0x10) = pvVar3;
  if (pvVar3 != (HANDLE)0xffffffff) {
    DVar4 = FUN_00406200(this,param_1);
    piVar2 = param_3;
    if (DVar4 != 0) {
      return DVar4;
    }
    if (param_3 != (int *)0x0) {
      (**(code **)(*param_3 + 0x18))(param_3,0,*(undefined4 *)((int)this + 8),4,0);
    }
    param_2 = operator_new(0x10000);
    uVar7 = 0;
    while( true ) {
      if (((*(uint *)((int)this + 8) != 0) && (*(uint *)((int)this + 8) <= uVar7)) ||
         (nNumberOfBytesToWrite = (uint *)recv(*(SOCKET *)((int)this + 0xc),param_2,0x10000,0),
         nNumberOfBytesToWrite == (uint *)0x0)) {
        uVar1 = *(uint *)((int)this + 8);
        if ((uVar1 != 0) && (uVar1 != uVar7)) {
          return 0x8001000f;
        }
        if (piVar2 != (int *)0x0) {
          (**(code **)(*piVar2 + 0x18))(piVar2,uVar7,uVar1,6,0);
        }
        if (param_2 != (char *)0x0) {
          FUN_0040ef10(param_2);
        }
        return 0;
      }
      if (nNumberOfBytesToWrite == (uint *)0xffffffff) {
        DVar4 = WSAGetLastError();
        return DVar4;
      }
      BVar5 = WriteFile(*(HANDLE *)((int)this + 0x10),param_2,(DWORD)nNumberOfBytesToWrite,
                        (LPDWORD)&param_1,(LPOVERLAPPED)0x0);
      if ((BVar5 == 0) || (param_1 != nNumberOfBytesToWrite)) break;
      uVar7 = uVar7 + (int)nNumberOfBytesToWrite;
      if ((piVar2 != (int *)0x0) &&
         (iVar6 = (**(code **)(*piVar2 + 0x18))(piVar2,uVar7,*(undefined4 *)((int)this + 8),5,0),
         iVar6 != 0)) {
        return 0x80004004;
      }
    }
  }
  DVar4 = GetLastError();
  return DVar4;
}



int __thiscall FUN_00406200(void *this,uint *param_1)

{
  char cVar1;
  undefined4 uVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  undefined1 local_1c [4];
  uint *local_18;
  uint local_14;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004194f8;
  local_c = ExceptionList;
  local_1c[0] = param_1._0_1_;
  ExceptionList = &local_c;
  FUN_00401380(local_1c,'\0');
  uVar5 = 0xffffffff;
  puVar3 = param_1;
  do {
    if (uVar5 == 0) break;
    uVar5 = uVar5 - 1;
    uVar6 = *puVar3;
    puVar3 = (uint *)((int)puVar3 + 1);
  } while ((char)uVar6 != '\0');
  uVar5 = ~uVar5 - 1;
  uVar2 = FUN_00401570(local_1c,uVar5,'\x01');
  if ((char)uVar2 != '\0') {
    puVar3 = local_18;
    for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
      *puVar3 = *param_1;
      param_1 = param_1 + 1;
      puVar3 = puVar3 + 1;
    }
    for (uVar6 = uVar5 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *(char *)puVar3 = (char)*param_1;
      param_1 = (uint *)((int)param_1 + 1);
      puVar3 = (uint *)((int)puVar3 + 1);
    }
    *(undefined1 *)((int)local_18 + uVar5) = 0;
    local_14 = uVar5;
  }
  local_4 = 0;
  iVar7 = 10;
  while( true ) {
    puVar3 = local_18;
    if (local_18 == (uint *)0x0) {
      puVar3 = &DAT_0041a214;
    }
    iVar4 = FUN_00403e40(puVar3,(SOCKET *)((int)this + 0xc),(int *)((int)this + 4),
                         (undefined4 *)((int)this + 8),local_1c);
    if (iVar4 != 0) break;
    switch(*(int *)((int)this + 4)) {
    case 200:
      if (local_18 != (uint *)0x0) {
        cVar1 = *(char *)((int)local_18 + -1);
        if ((cVar1 != '\0') && (cVar1 != -1)) {
          *(char *)((int)local_18 + -1) = cVar1 + -1;
          ExceptionList = local_c;
          return 0;
        }
        FUN_0040ef10((undefined *)((int)local_18 + -1));
      }
      ExceptionList = local_c;
      return 0;
    default:
      iVar7 = iVar7 + -1;
      if (iVar7 < 1) {
        if (local_18 != (uint *)0x0) {
          cVar1 = *(char *)((int)local_18 + -1);
          if ((cVar1 == '\0') || (cVar1 == -1)) {
            FUN_0040ef10((undefined *)((int)local_18 + -1));
          }
          else {
            *(char *)((int)local_18 + -1) = cVar1 + -1;
          }
        }
        ExceptionList = local_c;
        return 0xc;
      }
      break;
    case 0x194:
      if (local_18 != (uint *)0x0) {
        cVar1 = *(char *)((int)local_18 + -1);
        if ((cVar1 != '\0') && (cVar1 != -1)) {
          *(char *)((int)local_18 + -1) = cVar1 + -1;
          ExceptionList = local_c;
          return 2;
        }
        FUN_0040ef10((undefined *)((int)local_18 + -1));
      }
      ExceptionList = local_c;
      return 2;
    }
  }
  if (local_18 == (uint *)0x0) {
    ExceptionList = local_c;
    return iVar4;
  }
  cVar1 = *(char *)((int)local_18 + -1);
  if ((cVar1 != '\0') && (cVar1 != -1)) {
    *(char *)((int)local_18 + -1) = cVar1 + -1;
    ExceptionList = local_c;
    return iVar4;
  }
  FUN_0040ef10((undefined *)((int)local_18 + -1));
  ExceptionList = local_c;
  return iVar4;
}



void __thiscall FUN_00406490(void *this,undefined4 param_1,undefined4 param_2)

{
  *(undefined4 *)((int)this + 4) = param_2;
  *(undefined4 *)((int)this + 8) = param_1;
  *(undefined4 *)((int)this + 0xc) = 1;
  *(undefined ***)this = &PTR_LAB_0041a244;
  return;
}



void __fastcall FUN_004064b0(undefined4 *param_1)

{
  *param_1 = &PTR_LAB_0041a244;
  return;
}



undefined4 * __fastcall FUN_004066d0(undefined4 *param_1)

{
  DWORD DVar1;
  UINT UVar2;
  int iVar3;
  undefined4 *puVar4;
  int local_108;
  CHAR local_104;
  undefined4 local_103;
  
  local_104 = '\0';
  *param_1 = &PTR_LAB_0041a274;
  puVar4 = &local_103;
  for (iVar3 = 0x40; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  *(undefined2 *)puVar4 = 0;
  *(undefined1 *)((int)puVar4 + 2) = 0;
  DVar1 = GetTempPathA(0x104,&local_104);
  if (DVar1 == 0) {
    local_108 = MessageBoxA((HWND)0x0,s_GetTempPath_failed_00420554,&DAT_004200cc,0);
                    // WARNING: Subroutine does not return
    __CxxThrowException_8(&local_108,&DAT_0041e508);
  }
  UVar2 = GetTempFileNameA(&local_104,&DAT_00420550,0,(LPSTR)(param_1 + 1));
  if (UVar2 == 0) {
    local_108 = MessageBoxA((HWND)0x0,s_GetTempFileName_failed_00420538,&DAT_004200cc,0);
                    // WARNING: Subroutine does not return
    __CxxThrowException_8(&local_108,&DAT_0041e508);
  }
  return param_1;
}



void __fastcall FUN_00406780(undefined4 *param_1)

{
  LPCSTR lpFileName;
  DWORD DVar1;
  
  lpFileName = (LPCSTR)(param_1 + 1);
  *param_1 = &PTR_LAB_0041a274;
  DVar1 = GetFileAttributesA(lpFileName);
  if (DVar1 != 0xffffffff) {
    DeleteFileA(lpFileName);
    *lpFileName = '\0';
  }
  return;
}



undefined1 * __thiscall FUN_00406810(void *this,char *param_1,undefined1 *param_2)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  char *pcVar5;
  
  *(undefined1 *)this = *param_2;
  uVar3 = 0xffffffff;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  pcVar5 = param_1;
  do {
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    cVar1 = *pcVar5;
    pcVar5 = pcVar5 + 1;
  } while (cVar1 != '\0');
  uVar3 = ~uVar3 - 1;
  if (0xfffffffd < uVar3) {
    FUN_0040ef1b();
  }
  iVar2 = *(int *)((int)this + 4);
  if (((iVar2 == 0) || (cVar1 = *(char *)(iVar2 + -1), cVar1 == '\0')) || (cVar1 == -1)) {
    if (uVar3 == 0) {
      FUN_00401380(this,'\x01');
      return this;
    }
    if ((*(uint *)((int)this + 0xc) < 0x20) && (uVar3 <= *(uint *)((int)this + 0xc)))
    goto LAB_004068a3;
    FUN_00401380(this,'\x01');
  }
  else if (uVar3 == 0) {
    *(char *)(iVar2 + -1) = cVar1 + -1;
    FUN_00401380(this,'\0');
    return this;
  }
  FUN_00401690(uVar3);
LAB_004068a3:
  pcVar5 = *(char **)((int)this + 4);
  for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined4 *)pcVar5 = *(undefined4 *)param_1;
    param_1 = param_1 + 4;
    pcVar5 = pcVar5 + 4;
  }
  for (uVar4 = uVar3 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar5 = *param_1;
    param_1 = param_1 + 1;
    pcVar5 = pcVar5 + 1;
  }
  *(uint *)((int)this + 8) = uVar3;
  *(undefined1 *)(*(int *)((int)this + 4) + uVar3) = 0;
  return this;
}



undefined1 * __thiscall FUN_004068d0(void *this,undefined1 *param_1,uint param_2,uint param_3)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  undefined4 *puVar7;
  
  *param_1 = param_1._0_1_;
  *(undefined4 *)(param_1 + 4) = 0;
  *(undefined4 *)(param_1 + 8) = 0;
  *(undefined4 *)(param_1 + 0xc) = 0;
  if (*(uint *)((int)this + 8) < param_2) {
    FUN_0040f147();
  }
  uVar5 = *(uint *)((int)this + 8) - param_2;
  if (param_3 < uVar5) {
    uVar5 = param_3;
  }
  if (param_1 == this) {
    uVar5 = uVar5 + param_2;
    if (*(uint *)(param_1 + 8) < uVar5) {
      FUN_0040f147();
    }
    FUN_00401b10(param_1);
    iVar1 = *(int *)(param_1 + 8) - uVar5;
    iVar6 = -1;
    if (iVar1 != -1) {
      iVar6 = iVar1;
    }
    if (iVar6 != 0) {
      FUN_0040f710((undefined4 *)(*(int *)(param_1 + 4) + uVar5),
                   (undefined4 *)((int)(*(int *)(param_1 + 4) + uVar5) + iVar6),iVar1 - iVar6);
      iVar1 = *(int *)(param_1 + 8);
      uVar2 = FUN_00401570(param_1,iVar1 - iVar6,'\0');
      if ((char)uVar2 != '\0') {
        FUN_00401550(param_1,iVar1 - iVar6);
      }
    }
    FUN_00401b10(param_1);
    uVar5 = *(uint *)(param_1 + 8);
    if (uVar5 < param_2) {
      param_2 = uVar5;
    }
    if (param_2 != 0) {
      FUN_0040f710(*(undefined4 **)(param_1 + 4),
                   (undefined4 *)((int)*(undefined4 **)(param_1 + 4) + param_2),uVar5 - param_2);
      iVar6 = *(int *)(param_1 + 8);
      uVar2 = FUN_00401570(param_1,iVar6 - param_2,'\0');
      if ((char)uVar2 != '\0') {
        FUN_00401550(param_1,iVar6 - param_2);
        return param_1;
      }
    }
  }
  else {
    if ((uVar5 != 0) && (uVar5 == *(uint *)((int)this + 8))) {
      puVar3 = *(undefined4 **)((int)this + 4);
      if (puVar3 == (undefined4 *)0x0) {
        puVar3 = &DAT_0041a214;
      }
      if (*(byte *)((int)puVar3 + -1) < 0xfe) {
        FUN_00401380(param_1,'\x01');
        puVar3 = *(undefined4 **)((int)this + 4);
        if (puVar3 == (undefined4 *)0x0) {
          puVar3 = &DAT_0041a214;
        }
        *(undefined4 **)(param_1 + 4) = puVar3;
        *(undefined4 *)(param_1 + 8) = *(undefined4 *)((int)this + 8);
        *(undefined4 *)(param_1 + 0xc) = *(undefined4 *)((int)this + 0xc);
        *(char *)((int)puVar3 + -1) = *(char *)((int)puVar3 + -1) + '\x01';
        return param_1;
      }
    }
    uVar2 = FUN_00401570(param_1,uVar5,'\x01');
    if ((char)uVar2 != '\0') {
      puVar3 = *(undefined4 **)((int)this + 4);
      if (puVar3 == (undefined4 *)0x0) {
        puVar3 = &DAT_0041a214;
      }
      puVar3 = (undefined4 *)((int)puVar3 + param_2);
      puVar7 = *(undefined4 **)(param_1 + 4);
      for (uVar4 = uVar5 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
        *puVar7 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar7 = puVar7 + 1;
      }
      for (uVar4 = uVar5 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)puVar7 = *(undefined1 *)puVar3;
        puVar3 = (undefined4 *)((int)puVar3 + 1);
        puVar7 = (undefined4 *)((int)puVar7 + 1);
      }
      *(uint *)(param_1 + 8) = uVar5;
      *(undefined1 *)(*(int *)(param_1 + 4) + uVar5) = 0;
    }
  }
  return param_1;
}



void * __thiscall FUN_00406a60(void *this,uint param_1,undefined1 param_2)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  undefined4 *puVar5;
  
  if (-*(int *)((int)this + 8) - 1U <= param_1) {
    FUN_0040ef1b();
  }
  if (param_1 == 0) {
    return this;
  }
  uVar4 = param_1 + *(int *)((int)this + 8);
  if (0xfffffffd < uVar4) {
    FUN_0040ef1b();
  }
  iVar2 = *(int *)((int)this + 4);
  if (((iVar2 == 0) || (cVar1 = *(char *)(iVar2 + -1), cVar1 == '\0')) || (cVar1 == -1)) {
    if (uVar4 == 0) {
      if (iVar2 == 0) {
        return this;
      }
      FUN_00401550(this,0);
      return this;
    }
    if (uVar4 <= *(uint *)((int)this + 0xc)) goto LAB_00406ae9;
  }
  else if (uVar4 == 0) {
    *(char *)(iVar2 + -1) = cVar1 + -1;
    *(undefined4 *)((int)this + 4) = 0;
    *(undefined4 *)((int)this + 8) = 0;
    *(undefined4 *)((int)this + 0xc) = 0;
    return this;
  }
  FUN_00401690(uVar4);
LAB_00406ae9:
  puVar5 = (undefined4 *)(*(int *)((int)this + 4) + *(int *)((int)this + 8));
  for (uVar3 = param_1 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    *puVar5 = CONCAT22(CONCAT11(param_2,param_2),CONCAT11(param_2,param_2));
    puVar5 = puVar5 + 1;
  }
  for (uVar3 = param_1 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *(undefined1 *)puVar5 = param_2;
    puVar5 = (undefined4 *)((int)puVar5 + 1);
  }
  *(uint *)((int)this + 8) = uVar4;
  *(undefined1 *)(*(int *)((int)this + 4) + uVar4) = 0;
  return this;
}



void * __thiscall FUN_00406b30(void *this,undefined1 *param_1,int param_2)

{
  char cVar1;
  undefined1 uVar2;
  int iVar3;
  undefined1 *puVar4;
  uint uVar5;
  uint uVar6;
  undefined4 uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  undefined4 *puVar11;
  int local_8;
  
  uVar9 = 0;
  if (((*(int *)((int)this + 4) != 0) &&
      (cVar1 = *(char *)(*(int *)((int)this + 4) + -1), cVar1 != '\0')) && (cVar1 != -1)) {
    FUN_00401570(this,*(uint *)((int)this + 8),'\0');
  }
  if (*(int *)((int)this + 4) != 0) {
    *(undefined1 *)(*(int *)((int)this + 4) + -1) = 0xff;
  }
  iVar3 = *(int *)((int)this + 4);
  if (iVar3 == 0) {
    local_8 = 0;
  }
  else {
    local_8 = *(int *)((int)this + 8) + iVar3;
    if ((*(char *)(iVar3 + -1) != '\0') && (*(char *)(iVar3 + -1) != -1)) {
      uVar10 = *(uint *)((int)this + 8);
      uVar5 = FUN_00401b00();
      if (uVar5 < uVar10) {
        FUN_0040ef1b();
      }
      iVar3 = *(int *)((int)this + 4);
      if (((iVar3 == 0) || (cVar1 = *(char *)(iVar3 + -1), cVar1 == '\0')) || (cVar1 == -1)) {
        if (uVar10 == 0) {
          if (iVar3 != 0) {
            FUN_00401550(this,0);
          }
          goto LAB_00406bde;
        }
        if (uVar10 <= *(uint *)((int)this + 0xc)) goto LAB_00406bde;
      }
      else if (uVar10 == 0) {
        *(char *)(iVar3 + -1) = cVar1 + -1;
        FUN_00401380(this,'\0');
        goto LAB_00406bde;
      }
      FUN_00401690(uVar10);
    }
  }
LAB_00406bde:
  if (*(int *)((int)this + 4) != 0) {
    *(undefined1 *)(*(int *)((int)this + 4) + -1) = 0xff;
  }
  iVar3 = *(int *)((int)this + 4);
  FUN_00406fa0(this);
  if (iVar3 != 0) {
    uVar9 = iVar3 - *(int *)((int)this + 4);
  }
  uVar10 = param_2 - (int)param_1;
  if (local_8 == 0) {
    uVar5 = 0;
  }
  else {
    uVar5 = local_8 - iVar3;
  }
  if (*(uint *)((int)this + 8) < uVar9) {
    FUN_0040f147();
  }
  uVar8 = *(int *)((int)this + 8) - uVar9;
  if (uVar8 < uVar5) {
    uVar5 = uVar8;
  }
  if (-uVar10 - 1 <= *(int *)((int)this + 8) - uVar5) {
    FUN_0040ef1b();
  }
  FUN_00401b10(this);
  uVar8 = (*(int *)((int)this + 8) - uVar5) - uVar9;
  if (uVar10 < uVar5) {
    FUN_0040f710((undefined4 *)(*(int *)((int)this + 4) + uVar10 + uVar9),
                 (undefined4 *)(*(int *)((int)this + 4) + uVar5 + uVar9),uVar8);
  }
  if ((uVar10 != 0) || (uVar5 != 0)) {
    uVar6 = (*(int *)((int)this + 8) - uVar5) + uVar10;
    uVar7 = FUN_00401570(this,uVar6,'\0');
    if ((char)uVar7 != '\0') {
      if (uVar5 < uVar10) {
        FUN_0040f710((undefined4 *)(*(int *)((int)this + 4) + uVar10 + uVar9),
                     (undefined4 *)(*(int *)((int)this + 4) + uVar5 + uVar9),uVar8);
      }
      puVar11 = (undefined4 *)(uVar9 + *(int *)((int)this + 4));
      for (uVar5 = uVar10 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *puVar11 = 0;
        puVar11 = puVar11 + 1;
      }
      for (uVar5 = uVar10 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined1 *)puVar11 = 0;
        puVar11 = (undefined4 *)((int)puVar11 + 1);
      }
      FUN_00401550(this,uVar6);
    }
  }
  FUN_00406fa0(this);
  puVar4 = (undefined1 *)(uVar9 + *(int *)((int)this + 4));
  for (; uVar10 != 0; uVar10 = uVar10 - 1) {
    uVar2 = *param_1;
    param_1 = param_1 + 1;
    *puVar4 = uVar2;
    puVar4 = puVar4 + 1;
  }
  return this;
}



void * __thiscall FUN_00406d10(void *this,uint param_1,uint param_2,undefined1 *param_3,int param_4)

{
  char cVar1;
  undefined1 uVar2;
  char *pcVar3;
  int iVar4;
  undefined1 *puVar5;
  undefined4 uVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  char *pcVar10;
  undefined4 *puVar11;
  
  uVar7 = param_1;
  if (((*(int *)((int)this + 4) != 0) &&
      (cVar1 = *(char *)(*(int *)((int)this + 4) + -1), cVar1 != '\0')) && (cVar1 != -1)) {
    FUN_00401570(this,*(uint *)((int)this + 8),'\0');
  }
  if (*(int *)((int)this + 4) != 0) {
    *(undefined1 *)(*(int *)((int)this + 4) + -1) = 0xff;
  }
  if (param_1 == 0) {
    param_1 = 0;
  }
  else {
    param_1 = param_1 - *(int *)((int)this + 4);
  }
  uVar9 = param_4 - (int)param_3;
  if (param_2 != 0) {
    param_2 = param_2 - uVar7;
  }
  if (*(uint *)((int)this + 8) < param_1) {
    FUN_0040f147();
  }
  uVar7 = *(int *)((int)this + 8) - param_1;
  if (uVar7 < param_2) {
    param_2 = uVar7;
  }
  if (-uVar9 - 1 <= *(int *)((int)this + 8) - param_2) {
    FUN_0040ef1b();
  }
  pcVar3 = *(char **)((int)this + 4);
  if (((pcVar3 != (char *)0x0) && (pcVar3[-1] != '\0')) && (pcVar3[-1] != -1)) {
    FUN_00401380(this,'\x01');
    uVar7 = 0xffffffff;
    pcVar10 = pcVar3;
    do {
      if (uVar7 == 0) break;
      uVar7 = uVar7 - 1;
      cVar1 = *pcVar10;
      pcVar10 = pcVar10 + 1;
    } while (cVar1 != '\0');
    FUN_004014b0(this,(undefined4 *)pcVar3,~uVar7 - 1);
  }
  uVar7 = (*(int *)((int)this + 8) - param_2) - param_1;
  if (uVar9 < param_2) {
    FUN_0040f710((undefined4 *)(*(int *)((int)this + 4) + uVar9 + param_1),
                 (undefined4 *)(*(int *)((int)this + 4) + param_2 + param_1),uVar7);
  }
  if ((uVar9 != 0) || (param_2 != 0)) {
    uVar8 = (*(int *)((int)this + 8) - param_2) + uVar9;
    uVar6 = FUN_00401570(this,uVar8,'\0');
    if ((char)uVar6 != '\0') {
      if (param_2 < uVar9) {
        FUN_0040f710((undefined4 *)(*(int *)((int)this + 4) + uVar9 + param_1),
                     (undefined4 *)(*(int *)((int)this + 4) + param_2 + param_1),uVar7);
      }
      puVar11 = (undefined4 *)(param_1 + *(int *)((int)this + 4));
      for (uVar7 = uVar9 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
        *puVar11 = 0;
        puVar11 = puVar11 + 1;
      }
      for (uVar7 = uVar9 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
        *(undefined1 *)puVar11 = 0;
        puVar11 = (undefined4 *)((int)puVar11 + 1);
      }
      *(uint *)((int)this + 8) = uVar8;
      *(undefined1 *)(uVar8 + *(int *)((int)this + 4)) = 0;
    }
  }
  if (((*(int *)((int)this + 4) == 0) ||
      (cVar1 = *(char *)(*(int *)((int)this + 4) + -1), cVar1 == '\0')) || (cVar1 == -1))
  goto LAB_00406ed6;
  uVar7 = *(uint *)((int)this + 8);
  uVar8 = FUN_00401b00();
  if (uVar8 < uVar7) {
    FUN_0040ef1b();
  }
  iVar4 = *(int *)((int)this + 4);
  if (((iVar4 == 0) || (cVar1 = *(char *)(iVar4 + -1), cVar1 == '\0')) || (cVar1 == -1)) {
    if (uVar7 == 0) {
      if (iVar4 != 0) {
        FUN_00401550(this,0);
      }
      goto LAB_00406ed6;
    }
    if (uVar7 <= *(uint *)((int)this + 0xc)) goto LAB_00406ed6;
  }
  else if (uVar7 == 0) {
    *(char *)(iVar4 + -1) = cVar1 + -1;
    FUN_00401380(this,'\0');
    goto LAB_00406ed6;
  }
  FUN_00401690(uVar7);
LAB_00406ed6:
  if (*(int *)((int)this + 4) != 0) {
    *(undefined1 *)(*(int *)((int)this + 4) + -1) = 0xff;
  }
  puVar5 = (undefined1 *)(param_1 + *(int *)((int)this + 4));
  for (; uVar9 != 0; uVar9 = uVar9 - 1) {
    uVar2 = *param_3;
    param_3 = param_3 + 1;
    *puVar5 = uVar2;
    puVar5 = puVar5 + 1;
  }
  return this;
}



int __fastcall FUN_00406f10(void *param_1)

{
  char cVar1;
  uint uVar2;
  int iVar3;
  
  if (((*(int *)((int)param_1 + 4) == 0) ||
      (cVar1 = *(char *)(*(int *)((int)param_1 + 4) + -1), cVar1 == '\0')) || (cVar1 == -1))
  goto LAB_00406f77;
  uVar2 = *(uint *)((int)param_1 + 8);
  if (0xfffffffd < uVar2) {
    FUN_0040ef1b();
  }
  iVar3 = *(int *)((int)param_1 + 4);
  if (((iVar3 == 0) || (cVar1 = *(char *)(iVar3 + -1), cVar1 == '\0')) || (cVar1 == -1)) {
    if (uVar2 == 0) {
      if (iVar3 != 0) {
        FUN_00401550(param_1,0);
      }
      goto LAB_00406f77;
    }
    if (uVar2 <= *(uint *)((int)param_1 + 0xc)) goto LAB_00406f77;
  }
  else if (uVar2 == 0) {
    *(char *)(iVar3 + -1) = cVar1 + -1;
    FUN_00401380(param_1,'\0');
    goto LAB_00406f77;
  }
  FUN_00401690(uVar2);
LAB_00406f77:
  if (*(int *)((int)param_1 + 4) != 0) {
    *(undefined1 *)(*(int *)((int)param_1 + 4) + -1) = 0xff;
  }
  if (*(int *)((int)param_1 + 4) == 0) {
    return 0;
  }
  return *(int *)((int)param_1 + 8) + *(int *)((int)param_1 + 4);
}



void __fastcall FUN_00406fa0(void *param_1)

{
  char cVar1;
  uint uVar2;
  int iVar3;
  
  if (((*(int *)((int)param_1 + 4) == 0) ||
      (cVar1 = *(char *)(*(int *)((int)param_1 + 4) + -1), cVar1 == '\0')) || (cVar1 == -1))
  goto LAB_00407008;
  uVar2 = *(uint *)((int)param_1 + 8);
  if (0xfffffffd < uVar2) {
    FUN_0040ef1b();
  }
  iVar3 = *(int *)((int)param_1 + 4);
  if (((iVar3 == 0) || (cVar1 = *(char *)(iVar3 + -1), cVar1 == '\0')) || (cVar1 == -1)) {
    if (uVar2 == 0) {
      if (iVar3 != 0) {
        FUN_00401550(param_1,0);
      }
      goto LAB_00407008;
    }
    if (uVar2 <= *(uint *)((int)param_1 + 0xc)) goto LAB_00407008;
  }
  else if (uVar2 == 0) {
    *(char *)(iVar3 + -1) = cVar1 + -1;
    *(undefined4 *)((int)param_1 + 4) = 0;
    *(undefined4 *)((int)param_1 + 8) = 0;
    *(undefined4 *)((int)param_1 + 0xc) = 0;
    goto LAB_00407008;
  }
  FUN_00401690(uVar2);
LAB_00407008:
  if (*(int *)((int)param_1 + 4) != 0) {
    *(undefined1 *)(*(int *)((int)param_1 + 4) + -1) = 0xff;
  }
  return;
}



void * __thiscall FUN_00407020(void *this,undefined4 *param_1,uint param_2)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  undefined4 *puVar5;
  
  if (-*(int *)((int)this + 8) - 1U <= param_2) {
    FUN_0040ef1b();
  }
  if (param_2 == 0) {
    return this;
  }
  uVar4 = param_2 + *(int *)((int)this + 8);
  if (0xfffffffd < uVar4) {
    FUN_0040ef1b();
  }
  iVar2 = *(int *)((int)this + 4);
  if (((iVar2 == 0) || (cVar1 = *(char *)(iVar2 + -1), cVar1 == '\0')) || (cVar1 == -1)) {
    if (uVar4 == 0) {
      if (iVar2 == 0) {
        return this;
      }
      FUN_00401550(this,0);
      return this;
    }
    if (uVar4 <= *(uint *)((int)this + 0xc)) goto LAB_004070ab;
  }
  else if (uVar4 == 0) {
    *(char *)(iVar2 + -1) = cVar1 + -1;
    *(undefined4 *)((int)this + 4) = 0;
    *(undefined4 *)((int)this + 8) = 0;
    *(undefined4 *)((int)this + 0xc) = 0;
    return this;
  }
  FUN_00401690(uVar4);
LAB_004070ab:
  puVar5 = (undefined4 *)(*(int *)((int)this + 4) + *(int *)((int)this + 8));
  for (uVar3 = param_2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    *puVar5 = *param_1;
    param_1 = param_1 + 1;
    puVar5 = puVar5 + 1;
  }
  for (uVar3 = param_2 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *(undefined1 *)puVar5 = *(undefined1 *)param_1;
    param_1 = (undefined4 *)((int)param_1 + 1);
    puVar5 = (undefined4 *)((int)puVar5 + 1);
  }
  *(uint *)((int)this + 8) = uVar4;
  *(undefined1 *)(*(int *)((int)this + 4) + uVar4) = 0;
  return this;
}



uint __thiscall FUN_004070e0(void *this,uint param_1,uint param_2,byte *param_3,uint param_4)

{
  uint uVar1;
  uint uVar2;
  byte *pbVar3;
  bool bVar4;
  bool bVar5;
  
  if (*(uint *)((int)this + 8) < param_1) {
    FUN_0040f147();
  }
  uVar1 = *(int *)((int)this + 8) - param_1;
  if (uVar1 < param_2) {
    param_2 = uVar1;
  }
  uVar1 = param_2;
  if (param_4 <= param_2) {
    uVar1 = param_4;
  }
  if (*(int *)((int)this + 4) == 0) {
    pbVar3 = (byte *)0x0;
  }
  else {
    pbVar3 = (byte *)(*(int *)((int)this + 4) + param_1);
  }
  bVar4 = false;
  uVar2 = 0;
  bVar5 = true;
  do {
    if (uVar1 == 0) break;
    uVar1 = uVar1 - 1;
    bVar4 = *pbVar3 < *param_3;
    bVar5 = *pbVar3 == *param_3;
    pbVar3 = pbVar3 + 1;
    param_3 = param_3 + 1;
  } while (bVar5);
  if (!bVar5) {
    uVar2 = (1 - (uint)bVar4) - (uint)(bVar4 != 0);
  }
  if (uVar2 == 0) {
    if (param_2 < param_4) {
      return 0xffffffff;
    }
    uVar2 = (uint)(param_2 != param_4);
  }
  return uVar2;
}



void __cdecl FUN_00407180(LPCSTR param_1,char *param_2)

{
  FUN_004071a0(param_1,param_2,0xffffffff);
  return;
}



undefined4 * __cdecl FUN_004071a0(LPCSTR param_1,char *param_2,uint param_3)

{
  char cVar1;
  undefined4 *puVar2;
  undefined4 uVar3;
  char *pcVar4;
  int iVar5;
  void *pvVar6;
  DWORD *pDVar7;
  int *piVar8;
  uint uVar9;
  uint uVar10;
  char *pcVar11;
  LPCSTR pCVar12;
  int local_5c;
  char *local_58;
  int local_54;
  char local_50 [80];
  
  local_54 = -1;
  local_5c = 0;
  local_58 = param_2;
  if (param_1 == (LPCSTR)0x0) {
    return (undefined4 *)0x0;
  }
  if (param_2 == (char *)0x0) {
    return (undefined4 *)0x0;
  }
  puVar2 = _malloc(0x74);
  if (puVar2 == (undefined4 *)0x0) {
    return (undefined4 *)0x0;
  }
  puVar2[8] = 0;
  puVar2[9] = 0;
  puVar2[10] = 0;
  puVar2[0x11] = 0;
  *puVar2 = 0;
  puVar2[0x12] = 0;
  puVar2[3] = 0;
  puVar2[4] = 0;
  puVar2[1] = 0;
  puVar2[0x10] = 0;
  puVar2[0xe] = 0;
  puVar2[0xf] = 0;
  puVar2[0x19] = 0;
  puVar2[0x1a] = 0;
  puVar2[0x1b] = 0xffffffff;
  uVar3 = FUN_0040afe0(0,(uint *)0x0,0);
  puVar2[0x13] = uVar3;
  uVar9 = 0xffffffff;
  puVar2[0x14] = 0;
  puVar2[0x16] = 0;
  pCVar12 = param_1;
  do {
    if (uVar9 == 0) break;
    uVar9 = uVar9 - 1;
    cVar1 = *pCVar12;
    pCVar12 = pCVar12 + 1;
  } while (cVar1 != '\0');
  pcVar4 = _malloc(~uVar9);
  puVar2[0x15] = pcVar4;
  if (pcVar4 != (char *)0x0) {
    uVar9 = 0xffffffff;
    pCVar12 = param_1;
    do {
      pcVar11 = pCVar12;
      if (uVar9 == 0) break;
      uVar9 = uVar9 - 1;
      pcVar11 = pCVar12 + 1;
      cVar1 = *pCVar12;
      pCVar12 = pcVar11;
    } while (cVar1 != '\0');
    uVar9 = ~uVar9;
    pcVar11 = pcVar11 + -uVar9;
    for (uVar10 = uVar9 >> 2; uVar10 != 0; uVar10 = uVar10 - 1) {
      *(undefined4 *)pcVar4 = *(undefined4 *)pcVar11;
      pcVar11 = pcVar11 + 4;
      pcVar4 = pcVar4 + 4;
    }
    for (uVar9 = uVar9 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
      *pcVar4 = *pcVar11;
      pcVar11 = pcVar11 + 1;
      pcVar4 = pcVar4 + 1;
    }
    *(undefined1 *)(puVar2 + 0x17) = 0;
    param_2 = local_50;
    do {
      if (*local_58 == 'r') {
        *(undefined1 *)(puVar2 + 0x17) = 0x72;
      }
      if ((*local_58 == 'w') || (*local_58 == 'a')) {
        *(undefined1 *)(puVar2 + 0x17) = 0x77;
      }
      cVar1 = *local_58;
      if ((cVar1 < '0') || ('9' < cVar1)) {
        if (cVar1 == 'f') {
          local_5c = 1;
        }
        else if (cVar1 == 'h') {
          local_5c = 2;
        }
        else if (cVar1 == 'R') {
          local_5c = 3;
        }
        else {
          *param_2 = cVar1;
          param_2 = param_2 + 1;
        }
      }
      else {
        local_54 = cVar1 + -0x30;
      }
      local_58 = local_58 + 1;
    } while ((cVar1 != '\0') && (param_2 != &stack0x00000000));
    if (*(char *)(puVar2 + 0x17) != '\0') {
      if (*(char *)(puVar2 + 0x17) == 'w') {
        iVar5 = FUN_00409530((int)puVar2,local_54,8,-0xf,8,local_5c,s_1_2_3_00420580,0x38);
        pvVar6 = _malloc(0x4000);
        puVar2[0x12] = pvVar6;
        puVar2[3] = pvVar6;
        if (iVar5 != 0) goto LAB_0040732d;
      }
      else {
        pvVar6 = _malloc(0x4000);
        puVar2[0x11] = pvVar6;
        *puVar2 = pvVar6;
        iVar5 = FUN_00407bb0((int)puVar2,0xfffffff1,s_1_2_3_00420580,0x38);
        if (iVar5 != 0) goto LAB_0040732d;
        pvVar6 = (void *)puVar2[0x11];
      }
      if (pvVar6 != (void *)0x0) {
        puVar2[4] = 0x4000;
        pDVar7 = FUN_00410d43();
        *pDVar7 = 0;
        if ((int)param_3 < 0) {
          piVar8 = (int *)FUN_004107c6(param_1,local_50);
        }
        else {
          piVar8 = FUN_00410bdd(param_3,local_50);
        }
        puVar2[0x10] = piVar8;
        if (piVar8 != (int *)0x0) {
          if (*(char *)(puVar2 + 0x17) != 'w') {
            FUN_00407410(puVar2);
            iVar5 = FUN_00410a1e((char *)puVar2[0x10]);
            puVar2[0x18] = iVar5 - puVar2[1];
            return puVar2;
          }
          FUN_00410ba1(piVar8,(byte *)s__c_c_c_c_c_c_c_c_c_c_00420568);
          puVar2[0x18] = 10;
          return puVar2;
        }
      }
    }
  }
LAB_0040732d:
  FUN_00407630((undefined *)puVar2);
  return (undefined4 *)0x0;
}



void __cdecl FUN_00407410(undefined4 *param_1)

{
  char *pcVar1;
  DWORD *pDVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  
  uVar3 = param_1[1];
  if (uVar3 < 2) {
    if (uVar3 != 0) {
      *(undefined1 *)param_1[0x11] = *(undefined1 *)*param_1;
    }
    pDVar2 = FUN_00410d43();
    *pDVar2 = 0;
    uVar3 = FUN_00410e8f((char *)(param_1[0x11] + uVar3),1,0x4000 >> ((byte)uVar3 & 0x1f),
                         (int *)param_1[0x10]);
    if ((uVar3 == 0) && ((*(byte *)(param_1[0x10] + 0xc) & 0x20) != 0)) {
      param_1[0xe] = 0xffffffff;
    }
    uVar3 = param_1[1] + uVar3;
    param_1[1] = uVar3;
    *param_1 = param_1[0x11];
    if (uVar3 < 2) {
      param_1[0x16] = uVar3;
      return;
    }
  }
  pcVar1 = (char *)*param_1;
  if ((*pcVar1 != '\x1f') || (pcVar1[1] != -0x75)) {
    param_1[0x16] = 1;
    return;
  }
  param_1[1] = param_1[1] + -2;
  *param_1 = pcVar1 + 2;
  uVar3 = FUN_004075b0(param_1);
  uVar4 = FUN_004075b0(param_1);
  if ((uVar3 != 8) || ((uVar4 & 0xe0) != 0)) {
    param_1[0xe] = 0xfffffffd;
    return;
  }
  iVar6 = 6;
  do {
    FUN_004075b0(param_1);
    iVar6 = iVar6 + -1;
  } while (iVar6 != 0);
  if ((uVar4 & 4) != 0) {
    uVar3 = FUN_004075b0(param_1);
    uVar5 = FUN_004075b0(param_1);
    iVar6 = uVar3 + uVar5 * 0x100;
    do {
      if (iVar6 == 0) break;
      iVar6 = iVar6 + -1;
      uVar3 = FUN_004075b0(param_1);
    } while (uVar3 != 0xffffffff);
  }
  if ((uVar4 & 8) != 0) {
    uVar3 = FUN_004075b0(param_1);
    while ((uVar3 != 0 && (uVar3 != 0xffffffff))) {
      uVar3 = FUN_004075b0(param_1);
    }
  }
  if ((uVar4 & 0x10) != 0) {
    uVar3 = FUN_004075b0(param_1);
    while ((uVar3 != 0 && (uVar3 != 0xffffffff))) {
      uVar3 = FUN_004075b0(param_1);
    }
  }
  if ((uVar4 & 2) != 0) {
    iVar6 = 2;
    do {
      FUN_004075b0(param_1);
      iVar6 = iVar6 + -1;
    } while (iVar6 != 0);
  }
  param_1[0xe] = -(uint)(param_1[0xf] != 0) & 0xfffffffd;
  return;
}



uint __cdecl FUN_004075b0(undefined4 *param_1)

{
  byte bVar1;
  DWORD *pDVar2;
  uint uVar3;
  
  if (param_1[0xf] != 0) {
    return 0xffffffff;
  }
  if (param_1[1] == 0) {
    pDVar2 = FUN_00410d43();
    *pDVar2 = 0;
    uVar3 = FUN_00410e8f((char *)param_1[0x11],1,0x4000,(int *)param_1[0x10]);
    param_1[1] = uVar3;
    if (uVar3 == 0) {
      param_1[0xf] = 1;
      if ((*(byte *)(param_1[0x10] + 0xc) & 0x20) == 0) {
        return 0xffffffff;
      }
      param_1[0xe] = 0xffffffff;
      return 0xffffffff;
    }
    *param_1 = param_1[0x11];
  }
  param_1[1] = param_1[1] + -1;
  bVar1 = *(byte *)*param_1;
  *param_1 = (byte *)*param_1 + 1;
  return (uint)bVar1;
}



uint __cdecl FUN_00407630(undefined *param_1)

{
  int iVar1;
  DWORD *pDVar2;
  uint uVar3;
  
  uVar3 = 0;
  if (param_1 != (undefined *)0x0) {
    if (*(undefined **)(param_1 + 0x50) != (undefined *)0x0) {
      FUN_00410fa6(*(undefined **)(param_1 + 0x50));
    }
    if (*(int *)(param_1 + 0x1c) != 0) {
      if (param_1[0x5c] == 'w') {
        uVar3 = FUN_0040a160((int)param_1);
      }
      else if (param_1[0x5c] == 'r') {
        uVar3 = FUN_004094e0((int)param_1);
      }
    }
    if (((*(FILE **)(param_1 + 0x40) != (FILE *)0x0) &&
        (iVar1 = FUN_004105df(*(FILE **)(param_1 + 0x40)), iVar1 != 0)) &&
       (pDVar2 = FUN_00410d43(), *pDVar2 != 0x1d)) {
      uVar3 = 0xffffffff;
    }
    if ((int)*(uint *)(param_1 + 0x38) < 0) {
      uVar3 = *(uint *)(param_1 + 0x38);
    }
    if (*(undefined **)(param_1 + 0x44) != (undefined *)0x0) {
      FUN_00410fa6(*(undefined **)(param_1 + 0x44));
    }
    if (*(undefined **)(param_1 + 0x48) != (undefined *)0x0) {
      FUN_00410fa6(*(undefined **)(param_1 + 0x48));
    }
    if (*(undefined **)(param_1 + 0x54) != (undefined *)0x0) {
      FUN_00410fa6(*(undefined **)(param_1 + 0x54));
    }
    FUN_00410fa6(param_1);
    return uVar3;
  }
  return 0xfffffffe;
}



int __cdecl FUN_004076f0(int *param_1,uint *param_2,int param_3)

{
  DWORD *pDVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  undefined4 *puVar5;
  uint *puVar6;
  undefined4 *puVar7;
  
  if ((param_1 == (int *)0x0) || ((char)param_1[0x17] != 'r')) {
    return -2;
  }
  iVar3 = param_1[0xe];
  if ((iVar3 != -3) && (iVar3 != -1)) {
    if (iVar3 == 1) {
      return 0;
    }
    param_1[3] = (int)param_2;
    param_1[4] = param_3;
    if ((param_3 != 0) && (param_1[0x1b] != -1)) {
      *(char *)param_2 = (char)param_1[0x1b];
      param_2 = (uint *)((int)param_2 + 1);
      param_1[0x1b] = -1;
      param_1[3] = param_1[3] + 1;
      param_1[4] = param_1[4] + -1;
      param_1[0x1a] = param_1[0x1a] + 1;
      if (param_1[0x1c] != 0) {
        param_1[0xe] = 1;
        return 1;
      }
    }
    puVar6 = param_2;
    if (param_1[4] != 0) {
      do {
        uVar2 = param_1[1];
        if (param_1[0x16] != 0) {
          if ((uint)param_1[4] < uVar2) {
            uVar2 = param_1[4];
          }
          if (uVar2 != 0) {
            param_2 = (uint *)((int)param_2 + uVar2);
            puVar5 = (undefined4 *)*param_1;
            puVar7 = (undefined4 *)param_1[3];
            for (uVar4 = uVar2 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
              *puVar7 = *puVar5;
              puVar5 = puVar5 + 1;
              puVar7 = puVar7 + 1;
            }
            for (uVar4 = uVar2 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
              *(undefined1 *)puVar7 = *(undefined1 *)puVar5;
              puVar5 = (undefined4 *)((int)puVar5 + 1);
              puVar7 = (undefined4 *)((int)puVar7 + 1);
            }
            param_1[3] = (int)param_2;
            *param_1 = *param_1 + uVar2;
            param_1[4] = param_1[4] - uVar2;
            param_1[1] = param_1[1] - uVar2;
          }
          if (param_1[4] != 0) {
            uVar2 = FUN_00410e8f((char *)param_2,1,param_1[4],(int *)param_1[0x10]);
            param_1[4] = param_1[4] - uVar2;
          }
          iVar3 = param_3 - param_1[4];
          param_1[0x19] = param_1[0x19] + iVar3;
          param_1[0x1a] = param_1[0x1a] + iVar3;
          if (iVar3 == 0) {
            param_1[0xf] = 1;
          }
          return iVar3;
        }
        if ((uVar2 == 0) && (param_1[0xf] == 0)) {
          pDVar1 = FUN_00410d43();
          *pDVar1 = 0;
          uVar2 = FUN_00410e8f((char *)param_1[0x11],1,0x4000,(int *)param_1[0x10]);
          param_1[1] = uVar2;
          if ((uVar2 == 0) && (param_1[0xf] = 1, (*(byte *)(param_1[0x10] + 0xc) & 0x20) != 0)) {
            param_1[0xe] = -1;
            break;
          }
          *param_1 = param_1[0x11];
        }
        param_1[0x19] = param_1[0x19] + param_1[1];
        param_1[0x1a] = param_1[0x1a] + param_1[4];
        iVar3 = FUN_00407c90(param_1,0);
        param_1[0x19] = param_1[0x19] - param_1[1];
        param_1[0xe] = iVar3;
        param_1[0x1a] = param_1[0x1a] - param_1[4];
        if (iVar3 == 1) {
          iVar3 = FUN_0040afe0(param_1[0x13],puVar6,param_1[3] - (int)puVar6);
          puVar6 = (uint *)param_1[3];
          param_1[0x13] = iVar3;
          iVar3 = FUN_00407a80(param_1);
          if (iVar3 == param_1[0x13]) {
            FUN_00407a80(param_1);
            FUN_00407410(param_1);
            if (param_1[0xe] != 0) break;
            FUN_00407b50((int)param_1);
            iVar3 = FUN_0040afe0(0,(uint *)0x0,0);
            param_1[0x13] = iVar3;
          }
          else {
            param_1[0xe] = -3;
          }
        }
        if (((param_1[0xe] != 0) || (param_1[0xf] != 0)) || (param_1[4] == 0)) break;
      } while( true );
    }
    iVar3 = FUN_0040afe0(param_1[0x13],puVar6,param_1[3] - (int)puVar6);
    param_1[0x13] = iVar3;
    if ((param_3 != param_1[4]) || ((param_1[0xe] != -3 && (param_1[0xe] != -1)))) {
      return param_3 - param_1[4];
    }
  }
  return -1;
}



uint __cdecl FUN_00407980(int *param_1,int param_2)

{
  bool bVar1;
  uint uVar2;
  uint uVar3;
  
  bVar1 = false;
  if ((param_1 == (int *)0x0) || ((char)param_1[0x17] != 'w')) {
    return 0xfffffffe;
  }
  param_1[1] = 0;
  do {
    uVar3 = 0x4000 - param_1[4];
    if (uVar3 != 0) {
      uVar2 = FUN_0041065c((char *)param_1[0x12],1,uVar3,(int *)param_1[0x10]);
      if (uVar2 != uVar3) {
        param_1[0xe] = -1;
        return 0xffffffff;
      }
      param_1[4] = 0x4000;
      param_1[3] = param_1[0x12];
    }
    if (bVar1) break;
    param_1[0x1a] = param_1[0x1a] + param_1[4];
    uVar2 = FUN_004097f0(param_1,param_2);
    param_1[0xe] = uVar2;
    param_1[0x1a] = param_1[0x1a] - param_1[4];
    if ((uVar3 == 0) && (uVar2 == 0xfffffffb)) {
      param_1[0xe] = 0;
    }
    if ((param_1[4] == 0) && (param_1[0xe] != 1)) {
      bVar1 = false;
    }
    else {
      bVar1 = true;
    }
  } while ((param_1[0xe] == 0) || (param_1[0xe] == 1));
  return -(uint)(param_1[0xe] != 1) & param_1[0xe];
}



bool __cdecl FUN_00407a50(int param_1)

{
  if ((param_1 != 0) && (*(char *)(param_1 + 0x5c) == 'r')) {
    if (*(int *)(param_1 + 0x3c) != 0) {
      return true;
    }
    return *(int *)(param_1 + 0x38) == 1;
  }
  return false;
}



int __cdecl FUN_00407a80(undefined4 *param_1)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  uVar1 = FUN_004075b0(param_1);
  uVar2 = FUN_004075b0(param_1);
  uVar3 = FUN_004075b0(param_1);
  uVar4 = FUN_004075b0(param_1);
  if (uVar4 == 0xffffffff) {
    param_1[0xe] = 0xfffffffd;
  }
  return uVar4 * 0x1000000 + uVar1 + uVar2 * 0x100 + uVar3 * 0x10000;
}



uint __cdecl FUN_00407ad0(int *param_1)

{
  uint uVar1;
  
  if (param_1 == (int *)0x0) {
    return 0xfffffffe;
  }
  if ((char)param_1[0x17] == 'w') {
    uVar1 = FUN_00407980(param_1,4);
    if (uVar1 == 0) {
      FUN_00407b20((int *)param_1[0x10],param_1[0x13]);
      FUN_00407b20((int *)param_1[0x10],param_1[0x19]);
    }
  }
  uVar1 = FUN_00407630((undefined *)param_1);
  return uVar1;
}



void __cdecl FUN_00407b20(int *param_1,uint param_2)

{
  int iVar1;
  
  iVar1 = 4;
  do {
    FUN_004111c6(param_2 & 0xff,param_1);
    param_2 = param_2 >> 8;
    iVar1 = iVar1 + -1;
  } while (iVar1 != 0);
  return;
}



undefined4 __cdecl FUN_00407b50(int param_1)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  
  if ((param_1 != 0) && (puVar2 = *(undefined4 **)(param_1 + 0x1c), puVar2 != (undefined4 *)0x0)) {
    puVar2[7] = 0;
    *(undefined4 *)(param_1 + 0x14) = 0;
    *(undefined4 *)(param_1 + 8) = 0;
    *(undefined4 *)(param_1 + 0x18) = 0;
    *(undefined4 *)(param_1 + 0x30) = 1;
    *puVar2 = 0;
    puVar2[1] = 0;
    puVar2[3] = 0;
    puVar2[8] = 0;
    puVar2[10] = 0;
    puVar2[0xb] = 0;
    puVar2[0xc] = 0;
    puVar2[0xe] = 0;
    puVar2[0xf] = 0;
    puVar1 = puVar2 + 0x14c;
    puVar2[5] = 0x8000;
    puVar2[0x1b] = puVar1;
    puVar2[0x14] = puVar1;
    puVar2[0x13] = puVar1;
    return 0;
  }
  return 0xfffffffe;
}



undefined4 __cdecl FUN_00407bb0(int param_1,uint param_2,char *param_3,int param_4)

{
  int iVar1;
  undefined4 uVar2;
  
  if (((param_3 == (char *)0x0) || (*param_3 != s_1_2_3_00420580[0])) || (param_4 != 0x38)) {
    return 0xfffffffa;
  }
  if (param_1 == 0) {
    return 0xfffffffe;
  }
  *(undefined4 *)(param_1 + 0x18) = 0;
  if (*(int *)(param_1 + 0x20) == 0) {
    *(code **)(param_1 + 0x20) = FUN_0040b2f0;
    *(undefined4 *)(param_1 + 0x28) = 0;
  }
  if (*(int *)(param_1 + 0x24) == 0) {
    *(code **)(param_1 + 0x24) = FUN_0040b310;
  }
  iVar1 = (**(code **)(param_1 + 0x20))(*(undefined4 *)(param_1 + 0x28),1,0x2530);
  if (iVar1 == 0) {
    return 0xfffffffc;
  }
  *(int *)(param_1 + 0x1c) = iVar1;
  if ((int)param_2 < 0) {
    *(undefined4 *)(iVar1 + 8) = 0;
    param_2 = -param_2;
  }
  else {
    *(int *)(iVar1 + 8) = ((int)param_2 >> 4) + 1;
    if ((int)param_2 < 0x30) {
      param_2 = param_2 & 0xf;
    }
  }
  if ((7 < (int)param_2) && ((int)param_2 < 0x10)) {
    *(uint *)(iVar1 + 0x24) = param_2;
    *(undefined4 *)(iVar1 + 0x34) = 0;
    uVar2 = FUN_00407b50(param_1);
    return uVar2;
  }
  (**(code **)(param_1 + 0x24))(*(undefined4 *)(param_1 + 0x28),iVar1);
  *(undefined4 *)(param_1 + 0x1c) = 0;
  return 0xfffffffe;
}



int __cdecl FUN_00407c90(int *param_1,int param_2)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  byte bVar5;
  byte bVar6;
  byte *pbVar7;
  char cVar8;
  byte *pbVar9;
  uint *puVar10;
  uint *puVar11;
  uint *puVar12;
  uint uVar13;
  int iVar14;
  uint uVar15;
  uint uVar16;
  uint local_40;
  uint local_3c;
  uint *local_38;
  uint local_34;
  uint local_30;
  undefined1 local_2c;
  undefined1 local_2b;
  undefined1 local_2a;
  undefined1 local_29;
  uint *local_28;
  byte *local_24;
  uint local_20;
  int local_1c;
  uint local_18;
  uint local_14;
  undefined1 local_10;
  byte bStack_f;
  ushort uStack_e;
  char local_8;
  byte bStack_7;
  ushort uStack_6;
  
  if ((((param_1 != (int *)0x0) && (puVar1 = (uint *)param_1[7], puVar1 != (uint *)0x0)) &&
      (param_1[3] != 0)) && ((*param_1 != 0 || (param_1[1] == 0)))) {
    if (*puVar1 == 0xb) {
      *puVar1 = 0xc;
    }
    local_28 = (uint *)param_1[3];
    local_30 = param_1[4];
    puVar11 = (uint *)*param_1;
    uVar15 = param_1[1];
    uVar16 = puVar1[0xe];
    pbVar9 = (byte *)puVar1[0xf];
    uVar4 = *puVar1;
    local_1c = 0;
    local_20 = local_30;
    local_38 = puVar11;
    local_3c = uVar15;
    local_14 = uVar15;
    local_40 = uVar16;
    while (uVar4 < 0x1d) {
      uVar2 = uVar16;
      uVar13 = uVar15;
      switch((&switchD_00407d1a::switchdataD_00409304)[uVar4]) {
      case (undefined *)0x407d21:
        if (puVar1[2] == 0) {
          *puVar1 = 0xc;
        }
        else {
          for (; uVar16 = local_40, pbVar9 < (byte *)0x10; pbVar9 = pbVar9 + 8) {
            uVar13 = 0;
            if (uVar15 == 0) goto LAB_004091e5;
            uVar15 = uVar15 - 1;
            uVar2 = uVar2 + ((uint)(byte)*puVar11 << ((byte)pbVar9 & 0x1f));
            puVar11 = (uint *)((int)puVar11 + 1);
            local_40 = uVar2;
            local_3c = uVar15;
            local_38 = puVar11;
          }
          if (((puVar1[2] & 2) == 0) || (uVar2 != 0x8b1f)) {
            puVar1[4] = 0;
            if (puVar1[8] != 0) {
              *(undefined4 *)(puVar1[8] + 0x30) = 0xffffffff;
            }
            local_40 = uVar16;
            if (((puVar1[2] & 1) == 0) || (((uVar16 & 0xff) * 0x100 + (uVar16 >> 8)) % 0x1f != 0)) {
              param_1[6] = (int)s_incorrect_header_check_00420700;
              *puVar1 = 0x1b;
            }
            else if (((byte)uVar16 & 0xf) == 8) {
              pbVar9 = pbVar9 + -4;
              uVar16 = uVar16 >> 4;
              uVar4 = (uVar16 & 0xf) + 8;
              if (puVar1[9] < uVar4) {
                param_1[6] = (int)s_invalid_window_size_00420718;
                *puVar1 = 0x1b;
                local_40 = uVar16;
              }
              else {
                pbVar9 = (byte *)0x0;
                puVar1[5] = 1 << (sbyte)uVar4;
                uVar4 = FUN_0040bc50(0,(byte *)0x0,0);
                puVar1[6] = uVar4;
                param_1[0xc] = uVar4;
                *puVar1 = (uint)(~(byte)(uVar16 >> 8) & 2 | 9);
                local_40 = 0;
                uVar16 = 0;
              }
            }
            else {
              param_1[6] = (int)s_unknown_compression_method_0042072c;
              *puVar1 = 0x1b;
            }
          }
          else {
            pbVar9 = (byte *)0x0;
            uVar16 = FUN_0040afe0(0,(uint *)0x0,0);
            puVar1[6] = uVar16;
            local_2c = 0x1f;
            local_2b = 0x8b;
            uVar16 = FUN_0040afe0(puVar1[6],(uint *)&local_2c,2);
            puVar1[6] = uVar16;
            local_40 = 0;
            *puVar1 = 1;
            uVar16 = 0;
          }
        }
        break;
      case (undefined *)0x407e9f:
        for (; pbVar9 < (byte *)0x10; pbVar9 = pbVar9 + 8) {
          uVar13 = 0;
          if (uVar15 == 0) goto LAB_004091e5;
          uVar15 = uVar15 - 1;
          uVar2 = uVar2 + ((uint)(byte)*puVar11 << ((byte)pbVar9 & 0x1f));
          puVar11 = (uint *)((int)puVar11 + 1);
          local_40 = uVar2;
          local_3c = uVar15;
          local_38 = puVar11;
        }
        puVar1[4] = uVar2;
        if ((char)uVar2 == '\b') {
          if ((uVar2 & 0xe000) == 0) {
            if ((uint *)puVar1[8] != (uint *)0x0) {
              *(uint *)puVar1[8] = uVar2 >> 8 & 1;
            }
            if ((puVar1[4] & 0x200) != 0) {
              local_2c = (undefined1)local_40;
              local_2b = (undefined1)(local_40 >> 8);
              uVar16 = FUN_0040afe0(puVar1[6],(uint *)&local_2c,2);
              puVar1[6] = uVar16;
            }
            uVar2 = 0;
            pbVar9 = (byte *)0x0;
            *puVar1 = 2;
            do {
              uVar13 = 0;
              if (uVar15 == 0) goto LAB_004091e5;
              uVar15 = uVar15 - 1;
              bVar6 = (byte)pbVar9;
              pbVar9 = pbVar9 + 8;
              uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar6 & 0x1f));
              puVar11 = (uint *)((int)puVar11 + 1);
              local_40 = uVar2;
joined_r0x00407f45:
            } while (pbVar9 < (byte *)0x20);
            if (puVar1[8] != 0) {
              *(uint *)(puVar1[8] + 4) = uVar2;
            }
            if ((puVar1[4] & 0x200) != 0) {
              local_2c = (undefined1)local_40;
              local_29 = (undefined1)(local_40 >> 0x18);
              local_2b = (undefined1)(local_40 >> 8);
              local_2a = (undefined1)(local_40 >> 0x10);
              uVar16 = FUN_0040afe0(puVar1[6],(uint *)&local_2c,4);
              puVar1[6] = uVar16;
            }
            uVar2 = 0;
            pbVar9 = (byte *)0x0;
            *puVar1 = 3;
            do {
              uVar13 = 0;
              if (uVar15 == 0) goto LAB_004091e5;
              uVar15 = uVar15 - 1;
              bVar6 = (byte)pbVar9;
              pbVar9 = pbVar9 + 8;
              uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar6 & 0x1f));
              puVar11 = (uint *)((int)puVar11 + 1);
              local_40 = uVar2;
              local_3c = uVar15;
              local_38 = puVar11;
joined_r0x00407fbf:
            } while (pbVar9 < (byte *)0x10);
            if (puVar1[8] != 0) {
              *(uint *)(puVar1[8] + 8) = uVar2 & 0xff;
              *(uint *)(puVar1[8] + 0xc) = uVar2 >> 8;
            }
            if ((puVar1[4] & 0x200) != 0) {
              local_2c = (undefined1)local_40;
              local_2b = (undefined1)(local_40 >> 8);
              uVar16 = FUN_0040afe0(puVar1[6],(uint *)&local_2c,2);
              puVar1[6] = uVar16;
            }
            uVar16 = 0;
            pbVar9 = (byte *)0x0;
            local_40 = 0;
            *puVar1 = 4;
            goto switchD_00407d1a_caseD_40803f;
          }
          param_1[6] = (int)s_unknown_header_flags_set_004206e4;
        }
        else {
          param_1[6] = (int)s_unknown_compression_method_0042072c;
        }
        goto LAB_00409181;
      case (undefined *)0x407f42:
        goto joined_r0x00407f45;
      case (undefined *)0x407fbc:
        goto joined_r0x00407fbf;
      case (undefined *)0x40803f:
switchD_00407d1a_caseD_40803f:
        uVar2 = uVar16;
        if ((puVar1[4] & 0x400) == 0) {
          if (puVar1[8] != 0) {
            *(undefined4 *)(puVar1[8] + 0x10) = 0;
          }
        }
        else {
          for (; pbVar9 < (byte *)0x10; pbVar9 = pbVar9 + 8) {
            uVar13 = 0;
            if (uVar15 == 0) goto LAB_004091e5;
            uVar15 = uVar15 - 1;
            local_40 = uVar2 + ((uint)(byte)*puVar11 << ((byte)pbVar9 & 0x1f));
            puVar11 = (uint *)((int)puVar11 + 1);
            uVar2 = local_40;
            local_3c = uVar15;
            local_38 = puVar11;
          }
          puVar1[0x10] = uVar2;
          if (puVar1[8] != 0) {
            *(uint *)(puVar1[8] + 0x14) = uVar2;
          }
          if ((puVar1[4] & 0x200) != 0) {
            local_2c = (undefined1)local_40;
            local_2b = (undefined1)(local_40 >> 8);
            uVar16 = FUN_0040afe0(puVar1[6],(uint *)&local_2c,2);
            puVar1[6] = uVar16;
          }
          uVar16 = 0;
          pbVar9 = (byte *)0x0;
          local_40 = 0;
        }
        *puVar1 = 5;
        uVar2 = uVar16;
        uVar13 = uVar15;
      case (undefined *)0x4080ce:
        if ((puVar1[4] & 0x400) == 0) {
LAB_00408186:
          puVar1[0x10] = 0;
          *puVar1 = 6;
switchD_00407d1a_caseD_408193:
          if ((puVar1[4] & 0x800) == 0) {
            if (puVar1[8] != 0) {
              *(undefined4 *)(puVar1[8] + 0x1c) = 0;
            }
LAB_00408243:
            puVar1[0x10] = 0;
            *puVar1 = 7;
switchD_00407d1a_caseD_408250:
            if ((puVar1[4] & 0x1000) == 0) {
              if (puVar1[8] != 0) {
                *(undefined4 *)(puVar1[8] + 0x24) = 0;
              }
LAB_00408300:
              *puVar1 = 8;
              uVar15 = uVar13;
switchD_00407d1a_caseD_408306:
              if ((puVar1[4] & 0x200) != 0) {
                for (; pbVar9 < (byte *)0x10; pbVar9 = pbVar9 + 8) {
                  uVar13 = 0;
                  if (uVar15 == 0) goto LAB_004091e5;
                  uVar15 = uVar15 - 1;
                  uVar2 = uVar2 + ((uint)(byte)*puVar11 << ((byte)pbVar9 & 0x1f));
                  puVar11 = (uint *)((int)puVar11 + 1);
                  local_40 = uVar2;
                  local_3c = uVar15;
                  local_38 = puVar11;
                }
                if (uVar2 != (puVar1[6] & 0xffff)) {
                  param_1[6] = (int)s_header_crc_mismatch_004206d0;
                  goto LAB_00409181;
                }
                pbVar9 = (byte *)0x0;
                local_40 = 0;
              }
              if (puVar1[8] != 0) {
                *(uint *)(puVar1[8] + 0x2c) = (int)puVar1[4] >> 9 & 1;
                *(undefined4 *)(puVar1[8] + 0x30) = 1;
              }
              uVar16 = FUN_0040afe0(0,(uint *)0x0,0);
              puVar1[6] = uVar16;
              param_1[0xc] = uVar16;
              *puVar1 = 0xb;
              uVar16 = local_40;
              break;
            }
            if (uVar13 != 0) {
              uVar16 = 0;
              do {
                pbVar7 = (byte *)(uVar16 + (int)puVar11);
                local_24 = (byte *)(uint)*pbVar7;
                uVar16 = uVar16 + 1;
                if (((puVar1[8] != 0) && (local_18 = *(uint *)(puVar1[8] + 0x24), local_18 != 0)) &&
                   (puVar1[0x10] < *(uint *)(puVar1[8] + 0x28))) {
                  *(byte *)(local_18 + puVar1[0x10]) = *pbVar7;
                  puVar1[0x10] = puVar1[0x10] + 1;
                  uVar13 = local_3c;
                }
              } while ((local_24 != (byte *)0x0) && (uVar16 < uVar13));
              if ((puVar1[4] & 0x200) != 0) {
                uVar4 = FUN_0040afe0(puVar1[6],puVar11,uVar16);
                puVar1[6] = uVar4;
                uVar2 = local_40;
              }
              uVar13 = uVar13 - uVar16;
              puVar11 = (uint *)((int)puVar11 + uVar16);
              local_3c = uVar13;
              local_38 = puVar11;
              if (local_24 == (byte *)0x0) goto LAB_00408300;
            }
          }
          else if (uVar13 != 0) {
            uVar16 = 0;
            do {
              pbVar7 = (byte *)(uVar16 + (int)puVar11);
              local_24 = (byte *)(uint)*pbVar7;
              uVar16 = uVar16 + 1;
              if (((puVar1[8] != 0) && (local_18 = *(uint *)(puVar1[8] + 0x1c), local_18 != 0)) &&
                 (puVar1[0x10] < *(uint *)(puVar1[8] + 0x20))) {
                *(byte *)(local_18 + puVar1[0x10]) = *pbVar7;
                puVar1[0x10] = puVar1[0x10] + 1;
                uVar13 = local_3c;
              }
            } while ((local_24 != (byte *)0x0) && (uVar16 < uVar13));
            if ((puVar1[4] & 0x200) != 0) {
              uVar4 = FUN_0040afe0(puVar1[6],puVar11,uVar16);
              puVar1[6] = uVar4;
              uVar2 = local_40;
            }
            uVar13 = uVar13 - uVar16;
            puVar11 = (uint *)((int)puVar11 + uVar16);
            local_3c = uVar13;
            local_38 = puVar11;
            if (local_24 == (byte *)0x0) goto LAB_00408243;
          }
        }
        else {
          uVar16 = puVar1[0x10];
          if (uVar13 < puVar1[0x10]) {
            uVar16 = uVar13;
          }
          if (uVar16 != 0) {
            if ((puVar1[8] != 0) && (local_18 = *(uint *)(puVar1[8] + 0x10), local_18 != 0)) {
              uVar4 = *(uint *)(puVar1[8] + 0x18);
              iVar3 = *(int *)(puVar1[8] + 0x14) - puVar1[0x10];
              uVar15 = uVar16;
              if (uVar4 < uVar16 + iVar3) {
                uVar15 = uVar4 - iVar3;
              }
              puVar10 = local_38;
              puVar12 = (uint *)(iVar3 + local_18);
              for (uVar4 = uVar15 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
                *puVar12 = *puVar10;
                puVar10 = puVar10 + 1;
                puVar12 = puVar12 + 1;
              }
              for (uVar15 = uVar15 & 3; puVar11 = local_38, uVar13 = local_3c, uVar15 != 0;
                  uVar15 = uVar15 - 1) {
                *(byte *)puVar12 = (byte)*puVar10;
                puVar10 = (uint *)((int)puVar10 + 1);
                puVar12 = (uint *)((int)puVar12 + 1);
              }
            }
            if ((puVar1[4] & 0x200) != 0) {
              uVar4 = FUN_0040afe0(puVar1[6],puVar11,uVar16);
              puVar1[6] = uVar4;
              uVar2 = local_40;
            }
            uVar13 = uVar13 - uVar16;
            puVar11 = (uint *)((int)puVar11 + uVar16);
            puVar1[0x10] = puVar1[0x10] - uVar16;
            local_3c = uVar13;
            local_38 = puVar11;
          }
          if (puVar1[0x10] == 0) goto LAB_00408186;
        }
LAB_004091e5:
        param_1[3] = (int)local_28;
        param_1[4] = local_30;
        *param_1 = (int)puVar11;
        param_1[1] = uVar13;
        puVar1[0xe] = uVar2;
        puVar1[0xf] = (uint)pbVar9;
        if (((puVar1[10] == 0) && ((0x17 < (int)*puVar1 || (local_20 == param_1[4])))) ||
           (iVar3 = FUN_004093c0((int)param_1,local_20), iVar3 == 0)) {
          iVar3 = local_14 - param_1[1];
          uVar16 = local_20 - param_1[4];
          param_1[2] = param_1[2] + iVar3;
          param_1[5] = param_1[5] + uVar16;
          puVar1[7] = puVar1[7] + uVar16;
          if ((puVar1[2] != 0) && (uVar16 != 0)) {
            if (puVar1[4] == 0) {
              uVar4 = FUN_0040bc50(puVar1[6],(byte *)(param_1[3] - uVar16),uVar16);
            }
            else {
              uVar4 = FUN_0040afe0(puVar1[6],(uint *)(param_1[3] - uVar16),uVar16);
            }
            puVar1[6] = uVar4;
            param_1[0xc] = uVar4;
          }
          param_1[0xb] = (-(uint)(puVar1[1] != 0) & 0x40) + ((*puVar1 != 0xb) - 1 & 0x80) +
                         puVar1[0xf];
          if (((iVar3 != 0) || (uVar16 != 0)) && (param_2 != 4)) {
            return local_1c;
          }
          if (local_1c != 0) {
            return local_1c;
          }
          return -5;
        }
        *puVar1 = 0x1c;
switchD_00407d1a_caseD_409235:
        return -4;
      case (undefined *)0x408193:
        goto switchD_00407d1a_caseD_408193;
      case (undefined *)0x408250:
        goto switchD_00407d1a_caseD_408250;
      case (undefined *)0x408306:
        goto switchD_00407d1a_caseD_408306;
      case (undefined *)0x4083a4:
        for (; pbVar9 < (byte *)0x20; pbVar9 = pbVar9 + 8) {
          uVar13 = 0;
          if (uVar15 == 0) goto LAB_004091e5;
          uVar15 = uVar15 - 1;
          uVar2 = uVar2 + ((uint)(byte)*puVar11 << ((byte)pbVar9 & 0x1f));
          puVar11 = (uint *)((int)puVar11 + 1);
          local_40 = uVar2;
          local_3c = uVar15;
          local_38 = puVar11;
        }
        uVar16 = (uVar2 >> 0x18) +
                 ((uVar2 & 0xff) * 0x10000 + (uVar2 & 0xff00)) * 0x100 +
                 (local_40 >> 0x10 & 0xff) * 0x100;
        puVar1[6] = uVar16;
        param_1[0xc] = uVar16;
        uVar16 = 0;
        local_40 = 0;
        pbVar9 = (byte *)0x0;
        *puVar1 = 10;
      case (undefined *)0x40840e:
        if (puVar1[3] == 0) {
          param_1[3] = (int)local_28;
          param_1[1] = uVar15;
          param_1[4] = local_30;
          *param_1 = (int)puVar11;
          puVar1[0xf] = (uint)pbVar9;
          puVar1[0xe] = uVar16;
          return 2;
        }
        uVar16 = FUN_0040bc50(0,(byte *)0x0,0);
        puVar1[6] = uVar16;
        param_1[0xc] = uVar16;
        *puVar1 = 0xb;
        uVar2 = local_40;
        uVar13 = uVar15;
switchD_00407d1a_caseD_40843b:
        uVar15 = uVar13;
        if (param_2 != 5) {
switchD_00407d1a_caseD_408446:
          if (puVar1[1] == 0) {
            for (; pbVar9 < (byte *)0x3; pbVar9 = pbVar9 + 8) {
              uVar13 = 0;
              if (uVar15 == 0) goto LAB_004091e5;
              uVar15 = uVar15 - 1;
              uVar2 = uVar2 + ((uint)(byte)*puVar11 << ((byte)pbVar9 & 0x1f));
              puVar11 = (uint *)((int)puVar11 + 1);
              local_3c = uVar15;
              local_38 = puVar11;
            }
            puVar1[1] = uVar2 & 1;
            switch(uVar2 >> 1 & 3) {
            case 0:
              uVar16 = uVar2 >> 3;
              *puVar1 = 0xd;
              pbVar9 = pbVar9 + -3;
              local_40 = uVar16;
              break;
            case 1:
              FUN_00409390((int)puVar1);
              uVar16 = uVar2 >> 3;
              *puVar1 = 0x12;
              pbVar9 = pbVar9 + -3;
              local_40 = uVar16;
              break;
            case 2:
              uVar16 = uVar2 >> 3;
              *puVar1 = 0xf;
              pbVar9 = pbVar9 + -3;
              local_40 = uVar16;
              break;
            case 3:
              param_1[6] = (int)s_invalid_block_type_004206bc;
              *puVar1 = 0x1b;
            default:
              pbVar9 = pbVar9 + -3;
              uVar16 = uVar2 >> 3;
              local_40 = uVar2 >> 3;
            }
          }
          else {
            *puVar1 = 0x18;
            uVar16 = uVar2 >> (sbyte)((uint)pbVar9 & 7);
            pbVar9 = pbVar9 + -((uint)pbVar9 & 7);
            local_40 = uVar16;
          }
          break;
        }
        goto LAB_004091e5;
      case (undefined *)0x40843b:
        goto switchD_00407d1a_caseD_40843b;
      case (undefined *)0x408446:
        goto switchD_00407d1a_caseD_408446;
      case (undefined *)0x40851c:
        uVar2 = uVar16 >> (sbyte)((uint)pbVar9 & 7);
        for (pbVar9 = pbVar9 + -((uint)pbVar9 & 7); pbVar9 < (byte *)0x20; pbVar9 = pbVar9 + 8) {
          uVar13 = 0;
          if (uVar15 == 0) goto LAB_004091e5;
          uVar15 = uVar15 - 1;
          uVar2 = uVar2 + ((uint)(byte)*puVar11 << ((byte)pbVar9 & 0x1f));
          puVar11 = (uint *)((int)puVar11 + 1);
          local_3c = uVar15;
          local_38 = puVar11;
        }
        uVar16 = uVar2 & 0xffff;
        if (uVar16 == ~uVar2 >> 0x10) {
          uVar2 = 0;
          puVar1[0x10] = uVar16;
          local_40 = 0;
          pbVar9 = (byte *)0x0;
          *puVar1 = 0xe;
          uVar13 = uVar15;
          goto switchD_00407d1a_caseD_40858c;
        }
        param_1[6] = (int)s_invalid_stored_block_lengths_0042069c;
        local_40 = uVar2;
        goto LAB_00409181;
      case (undefined *)0x40858c:
switchD_00407d1a_caseD_40858c:
        local_34 = puVar1[0x10];
        uVar15 = uVar13;
        if (local_34 == 0) {
LAB_00408cd2:
          *puVar1 = 0xb;
          uVar16 = uVar2;
        }
        else {
          if (uVar13 < local_34) {
            local_34 = uVar13;
          }
          if (local_30 < local_34) {
            local_34 = local_30;
          }
          if (local_34 == 0) goto LAB_004091e5;
          puVar11 = local_38;
          puVar10 = local_28;
          for (uVar16 = local_34 >> 2; uVar16 != 0; uVar16 = uVar16 - 1) {
            *puVar10 = *puVar11;
            puVar11 = puVar11 + 1;
            puVar10 = puVar10 + 1;
          }
          for (uVar16 = local_34 & 3; uVar16 != 0; uVar16 = uVar16 - 1) {
            *(byte *)puVar10 = (byte)*puVar11;
            puVar11 = (uint *)((int)puVar11 + 1);
            puVar10 = (uint *)((int)puVar10 + 1);
          }
          uVar15 = local_3c - local_34;
          puVar11 = (uint *)((int)local_38 + local_34);
          local_30 = local_30 - local_34;
          local_28 = (uint *)((int)local_28 + local_34);
          puVar1[0x10] = puVar1[0x10] - local_34;
          uVar16 = uVar2;
          local_3c = uVar15;
          local_38 = puVar11;
        }
        break;
      case (undefined *)0x408612:
        for (; pbVar9 < (byte *)0xe; pbVar9 = pbVar9 + 8) {
          uVar13 = 0;
          if (uVar15 == 0) goto LAB_004091e5;
          uVar15 = uVar15 - 1;
          uVar2 = uVar2 + ((uint)(byte)*puVar11 << ((byte)pbVar9 & 0x1f));
          puVar11 = (uint *)((int)puVar11 + 1);
          local_3c = uVar15;
          local_38 = puVar11;
        }
        pbVar9 = pbVar9 + -0xe;
        puVar1[0x18] = (uVar2 & 0x1f) + 0x101;
        uVar16 = (uVar2 >> 5 & 0x1f) + 1;
        puVar1[0x17] = (uVar2 >> 10 & 0xf) + 4;
        uVar2 = uVar2 >> 0xe;
        puVar1[0x19] = uVar16;
        local_40 = uVar2;
        if ((puVar1[0x18] < 0x11f) && (uVar16 < 0x1f)) {
          puVar1[0x1a] = 0;
          *puVar1 = 0x10;
          goto switchD_00407d1a_caseD_408696;
        }
        param_1[6] = (int)s_too_many_length_or_distance_symb_0042065c;
LAB_00409181:
        *puVar1 = 0x1b;
        uVar16 = uVar2;
        break;
      case (undefined *)0x408696:
switchD_00407d1a_caseD_408696:
        uVar16 = uVar15;
        if (puVar1[0x1a] < puVar1[0x17]) {
          do {
            for (; uVar15 = local_3c, pbVar9 < (byte *)0x3; pbVar9 = pbVar9 + 8) {
              uVar13 = 0;
              if (uVar16 == 0) goto LAB_004091e5;
              uVar4 = *puVar11;
              uVar16 = uVar16 - 1;
              bVar6 = (byte)pbVar9;
              puVar11 = (uint *)((int)puVar11 + 1);
              uVar2 = uVar2 + ((uint)(byte)uVar4 << (bVar6 & 0x1f));
              local_3c = uVar16;
              local_38 = puVar11;
            }
            pbVar9 = pbVar9 + -3;
            local_40 = uVar2 >> 3;
            *(ushort *)
             ((int)puVar1 + (uint)*(ushort *)(&DAT_0041ab20 + puVar1[0x1a] * 2) * 2 + 0x70) =
                 (ushort)uVar2 & 7;
            uVar4 = puVar1[0x1a];
            puVar1[0x1a] = uVar4 + 1;
            local_3c = uVar15;
            uVar2 = local_40;
            uVar16 = uVar15;
          } while (uVar4 + 1 < puVar1[0x17]);
        }
        uVar16 = puVar1[0x1a];
        while (uVar16 < 0x13) {
          *(undefined2 *)
           ((int)puVar1 + (uint)*(ushort *)(&DAT_0041ab20 + puVar1[0x1a] * 2) * 2 + 0x70) = 0;
          uVar16 = puVar1[0x1a] + 1;
          puVar1[0x1a] = uVar16;
        }
        puVar1[0x13] = (uint)(puVar1 + 0x14c);
        puVar1[0x1b] = (uint)(puVar1 + 0x14c);
        puVar1[0x15] = 7;
        local_1c = FUN_0040b7b0(0,(ushort *)(puVar1 + 0x1c),0x13,(int *)(puVar1 + 0x1b),
                                puVar1 + 0x15,(ushort *)(puVar1 + 0xbc));
        uVar16 = local_40;
        if (local_1c == 0) {
          puVar1[0x1a] = 0;
          *puVar1 = 0x11;
          goto switchD_00407d1a_caseD_40879e;
        }
        param_1[6] = (int)s_invalid_code_lengths_set_00420680;
        *puVar1 = 0x1b;
        break;
      case (undefined *)0x40879e:
switchD_00407d1a_caseD_40879e:
        local_18 = puVar1[0x18] + puVar1[0x19];
        uVar2 = uVar16;
        if (puVar1[0x1a] < local_18) {
          do {
            local_24 = (byte *)((1 << ((byte)puVar1[0x15] & 0x1f)) - 1);
            uVar16 = *(uint *)(puVar1[0x13] + ((uint)local_24 & uVar2) * 4);
            local_10 = (undefined1)uVar16;
            bStack_f = (byte)(uVar16 >> 8);
            uStack_e = (ushort)(uVar16 >> 0x10);
            uVar16 = uVar16 >> 8;
            if (pbVar9 < (byte *)(uVar16 & 0xff)) {
              do {
                uVar13 = 0;
                if (uVar15 == 0) goto LAB_004091e5;
                uVar15 = uVar15 - 1;
                bVar6 = (byte)pbVar9;
                pbVar9 = pbVar9 + 8;
                uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar6 & 0x1f));
                puVar11 = (uint *)((int)puVar11 + 1);
                uVar16 = *(uint *)(puVar1[0x13] + ((uint)local_24 & uVar2) * 4);
                local_10 = (undefined1)uVar16;
                bStack_f = (byte)(uVar16 >> 8);
                uStack_e = (ushort)(uVar16 >> 0x10);
                uVar16 = uVar16 >> 8;
                local_3c = uVar15;
                local_38 = puVar11;
              } while (pbVar9 < (byte *)(uVar16 & 0xff));
            }
            bVar6 = (byte)uVar16;
            if (uStack_e < 0x10) {
              for (; pbVar9 < (byte *)(uVar16 & 0xff); pbVar9 = pbVar9 + 8) {
                uVar13 = 0;
                if (uVar15 == 0) goto LAB_004091e5;
                uVar15 = uVar15 - 1;
                uVar2 = uVar2 + ((uint)(byte)*puVar11 << ((byte)pbVar9 & 0x1f));
                puVar11 = (uint *)((int)puVar11 + 1);
                local_3c = uVar15;
                local_38 = puVar11;
              }
              pbVar9 = pbVar9 + -(int)(uVar16 & 0xff);
              local_40 = uVar2 >> (bVar6 & 0x1f);
              *(ushort *)((int)puVar1 + puVar1[0x1a] * 2 + 0x70) = uStack_e;
              puVar1[0x1a] = puVar1[0x1a] + 1;
            }
            else {
              if (uStack_e == 0x10) {
                uVar16 = uVar16 & 0xff;
                if (pbVar9 < (byte *)(uVar16 + 2)) {
                  do {
                    uVar13 = 0;
                    if (uVar15 == 0) goto LAB_004091e5;
                    uVar15 = uVar15 - 1;
                    bVar5 = (byte)pbVar9;
                    pbVar9 = pbVar9 + 8;
                    uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar5 & 0x1f));
                    puVar11 = (uint *)((int)puVar11 + 1);
                    local_3c = uVar15;
                    local_38 = puVar11;
                  } while (pbVar9 < (byte *)(uVar16 + 2));
                }
                pbVar9 = pbVar9 + -uVar16;
                local_40 = uVar2 >> (bVar6 & 0x1f);
                if (puVar1[0x1a] == 0) {
                  param_1[6] = (int)s_invalid_bit_length_repeat_00420640;
                  *puVar1 = 0x1b;
                  uVar16 = local_40;
                  break;
                }
                local_24 = (byte *)(uint)*(ushort *)((int)puVar1 + puVar1[0x1a] * 2 + 0x6e);
                uVar16 = local_40 & 3;
                local_40 = local_40 >> 2;
                iVar3 = uVar16 + 3;
                pbVar9 = pbVar9 + -2;
              }
              else {
                if (uStack_e == 0x11) {
                  if (pbVar9 < (byte *)((uVar16 & 0xff) + 3)) {
                    do {
                      uVar13 = 0;
                      if (uVar15 == 0) goto LAB_004091e5;
                      uVar15 = uVar15 - 1;
                      bVar5 = (byte)pbVar9;
                      pbVar9 = pbVar9 + 8;
                      uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar5 & 0x1f));
                      puVar11 = (uint *)((int)puVar11 + 1);
                      local_3c = uVar15;
                      local_38 = puVar11;
                    } while (pbVar9 < (byte *)((uVar16 & 0xff) + 3));
                  }
                  uVar2 = uVar2 >> (bVar6 & 0x1f);
                  iVar14 = -3;
                  iVar3 = (uVar2 & 7) + 3;
                  local_40 = uVar2 >> 3;
                }
                else {
                  if (pbVar9 < (byte *)((uVar16 & 0xff) + 7)) {
                    do {
                      uVar13 = 0;
                      if (uVar15 == 0) goto LAB_004091e5;
                      uVar15 = uVar15 - 1;
                      bVar5 = (byte)pbVar9;
                      pbVar9 = pbVar9 + 8;
                      uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar5 & 0x1f));
                      puVar11 = (uint *)((int)puVar11 + 1);
                      local_3c = uVar15;
                      local_38 = puVar11;
                    } while (pbVar9 < (byte *)((uVar16 & 0xff) + 7));
                  }
                  uVar2 = uVar2 >> (bVar6 & 0x1f);
                  iVar14 = -7;
                  iVar3 = (uVar2 & 0x7f) + 0xb;
                  local_40 = uVar2 >> 7;
                }
                local_24 = (byte *)0x0;
                pbVar9 = pbVar9 + (iVar14 - (uVar16 & 0xff));
              }
              uVar15 = local_3c;
              if (local_18 < puVar1[0x1a] + iVar3) {
                param_1[6] = (int)s_invalid_bit_length_repeat_00420640;
                *puVar1 = 0x1b;
                uVar16 = local_40;
                break;
              }
              for (; iVar3 != 0; iVar3 = iVar3 + -1) {
                *(short *)((int)puVar1 + puVar1[0x1a] * 2 + 0x70) = (short)local_24;
                puVar1[0x1a] = puVar1[0x1a] + 1;
              }
            }
            local_18 = puVar1[0x19] + puVar1[0x18];
            uVar16 = local_40;
            uVar2 = local_40;
          } while (puVar1[0x1a] < local_18);
        }
        if (*puVar1 != 0x1b) {
          puVar1[0x13] = (uint)(puVar1 + 0x14c);
          puVar1[0x1b] = (uint)(puVar1 + 0x14c);
          puVar1[0x15] = 9;
          local_1c = FUN_0040b7b0(1,(ushort *)(puVar1 + 0x1c),puVar1[0x18],(int *)(puVar1 + 0x1b),
                                  puVar1 + 0x15,(ushort *)(puVar1 + 0xbc));
          uVar16 = local_40;
          if (local_1c == 0) {
            puVar1[0x14] = puVar1[0x1b];
            puVar1[0x16] = 6;
            local_1c = FUN_0040b7b0(2,(ushort *)((int)puVar1 + puVar1[0x18] * 2 + 0x70),puVar1[0x19]
                                    ,(int *)(puVar1 + 0x1b),puVar1 + 0x16,(ushort *)(puVar1 + 0xbc))
            ;
            if (local_1c == 0) {
              *puVar1 = 0x12;
              uVar2 = local_40;
              goto switchD_00407d1a_caseD_408aeb;
            }
            param_1[6] = (int)s_invalid_distances_set_0042060c;
            *puVar1 = 0x1b;
          }
          else {
            param_1[6] = (int)s_invalid_literal_lengths_set_00420624;
            *puVar1 = 0x1b;
          }
        }
        break;
      case (undefined *)0x408aeb:
switchD_00407d1a_caseD_408aeb:
        if ((uVar15 < 6) || (local_30 < 0x102)) {
          local_24 = (byte *)((1 << ((byte)puVar1[0x15] & 0x1f)) - 1);
          uVar16 = *(uint *)(puVar1[0x13] + ((uint)local_24 & uVar2) * 4);
          local_10 = (undefined1)uVar16;
          bStack_f = (byte)(uVar16 >> 8);
          uStack_e = (ushort)(uVar16 >> 0x10);
          if (pbVar9 < (byte *)(uVar16 >> 8 & 0xff)) {
            do {
              uVar13 = 0;
              if (uVar15 == 0) goto LAB_004091e5;
              uVar15 = uVar15 - 1;
              bVar6 = (byte)pbVar9;
              pbVar9 = pbVar9 + 8;
              uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar6 & 0x1f));
              puVar11 = (uint *)((int)puVar11 + 1);
              uVar16 = *(uint *)(puVar1[0x13] + ((uint)local_24 & uVar2) * 4);
              local_10 = (undefined1)uVar16;
              bStack_f = (byte)(uVar16 >> 8);
              uStack_e = (ushort)(uVar16 >> 0x10);
              local_3c = uVar15;
              local_38 = puVar11;
            } while (pbVar9 < (byte *)(uVar16 >> 8 & 0xff));
          }
          cVar8 = (char)uVar16;
          if ((cVar8 != '\0') && ((uVar16 & 0xf0) == 0)) {
            bStack_7 = (byte)(uVar16 >> 8);
            uStack_6 = (ushort)(uVar16 >> 0x10);
            uVar4 = *(uint *)(puVar1[0x13] +
                             ((((1 << (cVar8 + bStack_f & 0x1f)) - 1U & uVar2) >> (bStack_f & 0x1f))
                             + (uint)uStack_e) * 4);
            local_10 = (undefined1)uVar4;
            bStack_f = (byte)(uVar4 >> 8);
            uStack_e = (ushort)(uVar4 >> 0x10);
            local_24 = (byte *)(uVar16 >> 8 & 0xff);
            local_8 = cVar8;
            if (pbVar9 < local_24 + (uVar4 >> 8 & 0xff)) {
              do {
                uVar13 = 0;
                if (uVar15 == 0) goto LAB_004091e5;
                uVar15 = uVar15 - 1;
                bVar6 = (byte)pbVar9;
                pbVar9 = pbVar9 + 8;
                uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar6 & 0x1f));
                puVar11 = (uint *)((int)puVar11 + 1);
                uVar16 = *(uint *)(puVar1[0x13] +
                                  ((((1 << (cVar8 + bStack_7 & 0x1f)) - 1U & uVar2) >>
                                   (bStack_7 & 0x1f)) + (uint)uStack_6) * 4);
                local_10 = (undefined1)uVar16;
                bStack_f = (byte)(uVar16 >> 8);
                uStack_e = (ushort)(uVar16 >> 0x10);
                local_3c = uVar15;
                local_38 = puVar11;
              } while (pbVar9 < local_24 + (uVar16 >> 8 & 0xff));
            }
            uVar16 = CONCAT22(uStack_e,CONCAT11(bStack_f,local_10));
            uVar2 = uVar2 >> (bStack_7 & 0x1f);
            pbVar9 = pbVar9 + -(int)local_24;
          }
          pbVar9 = pbVar9 + -(CONCAT21(uStack_e,bStack_f) & 0xff);
          uVar2 = uVar2 >> (bStack_f & 0x1f);
          puVar1[0x10] = (uint)uStack_e;
          local_40 = uVar2;
          if ((char)uVar16 == '\0') {
            *puVar1 = 0x17;
            uVar16 = uVar2;
            break;
          }
          if ((uVar16 & 0x20) == 0) {
            if ((uVar16 & 0x40) == 0) {
              *puVar1 = 0x13;
              puVar1[0x12] = uVar16 & 0xf;
              goto switchD_00407d1a_caseD_408cfe;
            }
            param_1[6] = (int)s_invalid_literal_length_code_004205f0;
            goto LAB_00409181;
          }
          goto LAB_00408cd2;
        }
        param_1[4] = local_30;
        param_1[3] = (int)local_28;
        *param_1 = (int)puVar11;
        param_1[1] = uVar15;
        puVar1[0xe] = local_40;
        puVar1[0xf] = (uint)pbVar9;
        FUN_0040b320(param_1,local_20);
        pbVar9 = (byte *)puVar1[0xf];
        local_28 = (uint *)param_1[3];
        local_30 = param_1[4];
        puVar11 = (uint *)*param_1;
        uVar15 = param_1[1];
        uVar16 = puVar1[0xe];
        local_40 = puVar1[0xe];
        local_3c = uVar15;
        local_38 = puVar11;
        break;
      case (undefined *)0x408cfe:
switchD_00407d1a_caseD_408cfe:
        pbVar7 = (byte *)puVar1[0x12];
        if (pbVar7 != (byte *)0x0) {
          if (pbVar9 < pbVar7) {
            do {
              uVar13 = 0;
              if (uVar15 == 0) goto LAB_004091e5;
              uVar15 = uVar15 - 1;
              bVar6 = (byte)pbVar9;
              pbVar7 = (byte *)puVar1[0x12];
              pbVar9 = pbVar9 + 8;
              uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar6 & 0x1f));
              puVar11 = (uint *)((int)puVar11 + 1);
              local_3c = uVar15;
              local_38 = puVar11;
            } while (pbVar9 < pbVar7);
          }
          puVar1[0x10] = puVar1[0x10] + ((1 << ((byte)pbVar7 & 0x1f)) - 1U & uVar2);
          uVar2 = uVar2 >> ((byte)puVar1[0x12] & 0x1f);
          pbVar9 = pbVar9 + -puVar1[0x12];
        }
        *puVar1 = 0x14;
      case (undefined *)0x408d50:
        local_18 = (1 << ((byte)puVar1[0x16] & 0x1f)) - 1;
        uVar16 = *(uint *)(puVar1[0x14] + (local_18 & uVar2) * 4);
        local_10 = (undefined1)uVar16;
        bStack_f = (byte)(uVar16 >> 8);
        uStack_e = (ushort)(uVar16 >> 0x10);
        if (pbVar9 < (byte *)(uVar16 >> 8 & 0xff)) {
          do {
            uVar13 = 0;
            if (uVar15 == 0) goto LAB_004091e5;
            uVar15 = uVar15 - 1;
            bVar6 = (byte)pbVar9;
            pbVar9 = pbVar9 + 8;
            uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar6 & 0x1f));
            puVar11 = (uint *)((int)puVar11 + 1);
            uVar16 = *(uint *)(puVar1[0x14] + (local_18 & uVar2) * 4);
            local_10 = (undefined1)uVar16;
            bStack_f = (byte)(uVar16 >> 8);
            uStack_e = (ushort)(uVar16 >> 0x10);
            local_3c = uVar15;
            local_38 = puVar11;
          } while (pbVar9 < (byte *)(uVar16 >> 8 & 0xff));
        }
        if ((uVar16 & 0xf0) == 0) {
          local_8 = (char)uVar16;
          bStack_7 = (byte)(uVar16 >> 8);
          uStack_6 = (ushort)(uVar16 >> 0x10);
          uVar4 = *(uint *)(puVar1[0x14] +
                           ((((1 << (local_8 + bStack_f & 0x1f)) - 1U & uVar2) >> (bStack_f & 0x1f))
                           + (uint)uStack_e) * 4);
          local_10 = (undefined1)uVar4;
          bStack_f = (byte)(uVar4 >> 8);
          uStack_e = (ushort)(uVar4 >> 0x10);
          local_24 = (byte *)(uVar16 >> 8 & 0xff);
          if (pbVar9 < local_24 + (uVar4 >> 8 & 0xff)) {
            do {
              uVar13 = 0;
              if (uVar15 == 0) goto LAB_004091e5;
              uVar15 = uVar15 - 1;
              bVar6 = (byte)pbVar9;
              pbVar9 = pbVar9 + 8;
              uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar6 & 0x1f));
              puVar11 = (uint *)((int)puVar11 + 1);
              uVar16 = *(uint *)(puVar1[0x14] +
                                ((((1 << (local_8 + bStack_7 & 0x1f)) - 1U & uVar2) >>
                                 (bStack_7 & 0x1f)) + (uint)uStack_6) * 4);
              local_10 = (undefined1)uVar16;
              bStack_f = (byte)(uVar16 >> 8);
              uStack_e = (ushort)(uVar16 >> 0x10);
              local_3c = uVar15;
              local_38 = puVar11;
            } while (pbVar9 < local_24 + (uVar16 >> 8 & 0xff));
          }
          uVar16 = CONCAT22(uStack_e,CONCAT11(bStack_f,local_10));
          uVar2 = uVar2 >> (bStack_7 & 0x1f);
          pbVar9 = pbVar9 + -(int)local_24;
        }
        uVar2 = uVar2 >> (bStack_f & 0x1f);
        pbVar9 = pbVar9 + -(CONCAT21(uStack_e,bStack_f) & 0xff);
        local_40 = uVar2;
        if ((uVar16 & 0x40) != 0) {
          param_1[6] = (int)s_invalid_distance_code_004205d8;
          goto LAB_00409181;
        }
        *puVar1 = 0x15;
        puVar1[0x11] = (uint)uStack_e;
        puVar1[0x12] = uVar16 & 0xf;
switchD_00407d1a_caseD_408ed0:
        pbVar7 = (byte *)puVar1[0x12];
        if (pbVar7 != (byte *)0x0) {
          if (pbVar9 < pbVar7) {
            do {
              uVar13 = 0;
              if (uVar15 == 0) goto LAB_004091e5;
              uVar15 = uVar15 - 1;
              bVar6 = (byte)pbVar9;
              pbVar7 = (byte *)puVar1[0x12];
              pbVar9 = pbVar9 + 8;
              uVar2 = uVar2 + ((uint)(byte)*puVar11 << (bVar6 & 0x1f));
              puVar11 = (uint *)((int)puVar11 + 1);
              local_3c = uVar15;
              local_38 = puVar11;
            } while (pbVar9 < pbVar7);
          }
          puVar1[0x11] = puVar1[0x11] + ((1 << ((byte)pbVar7 & 0x1f)) - 1U & uVar2);
          uVar2 = uVar2 >> ((byte)puVar1[0x12] & 0x1f);
          pbVar9 = pbVar9 + -puVar1[0x12];
          local_40 = uVar2;
        }
        if ((puVar1[0xb] - local_30) + local_20 < puVar1[0x11]) {
          param_1[6] = (int)s_invalid_distance_too_far_back_004205b8;
          goto LAB_00409181;
        }
        *puVar1 = 0x16;
switchD_00407d1a_caseD_408f4a:
        uVar13 = uVar15;
        if (local_30 != 0) {
          local_34 = puVar1[0x11];
          if (local_20 - local_30 < local_34) {
            local_34 = local_34 - (local_20 - local_30);
            if (puVar1[0xc] < local_34) {
              local_34 = local_34 - puVar1[0xc];
              local_24 = (byte *)((puVar1[0xd] + puVar1[10]) - local_34);
            }
            else {
              local_24 = (byte *)((puVar1[0xd] - local_34) + puVar1[0xc]);
            }
            local_18 = puVar1[0x10];
            if (local_18 < local_34) goto LAB_00408fb6;
          }
          else {
            local_24 = (byte *)((int)local_28 - local_34);
            local_18 = puVar1[0x10];
LAB_00408fb6:
            local_34 = local_18;
          }
          if (local_30 < local_34) {
            local_34 = local_30;
          }
          local_30 = local_30 - local_34;
          puVar1[0x10] = local_18 - local_34;
          do {
            *(byte *)local_28 = *local_24;
            local_28 = (uint *)((int)local_28 + 1);
            local_24 = local_24 + 1;
            local_34 = local_34 - 1;
          } while (local_34 != 0);
          uVar16 = uVar2;
          if (puVar1[0x10] == 0) {
            *puVar1 = 0x12;
          }
          break;
        }
        goto LAB_004091e5;
      case (undefined *)0x408ed0:
        goto switchD_00407d1a_caseD_408ed0;
      case (undefined *)0x408f4a:
        goto switchD_00407d1a_caseD_408f4a;
      case (undefined *)0x409012:
        if (local_30 == 0) goto LAB_004091e5;
        *(byte *)local_28 = (byte)puVar1[0x10];
        local_28 = (uint *)((int)local_28 + 1);
        local_30 = local_30 - 1;
        *puVar1 = 0x12;
        break;
      case (undefined *)0x40903c:
        if (puVar1[2] != 0) {
          for (; uVar2 = uVar16, pbVar9 < (byte *)0x20; pbVar9 = pbVar9 + 8) {
            uVar13 = 0;
            if (uVar15 == 0) goto LAB_004091e5;
            uVar15 = uVar15 - 1;
            uVar16 = uVar16 + ((uint)(byte)*puVar11 << ((byte)pbVar9 & 0x1f));
            puVar11 = (uint *)((int)puVar11 + 1);
            local_40 = uVar16;
            local_3c = uVar15;
            local_38 = puVar11;
          }
          local_20 = local_20 - local_30;
          param_1[5] = param_1[5] + local_20;
          puVar1[7] = puVar1[7] + local_20;
          if (local_20 != 0) {
            if (puVar1[4] == 0) {
              uVar16 = FUN_0040bc50(puVar1[6],(byte *)((int)local_28 - local_20),local_20);
            }
            else {
              uVar16 = FUN_0040afe0(puVar1[6],(uint *)((int)local_28 - local_20),local_20);
            }
            puVar1[6] = uVar16;
            param_1[0xc] = uVar16;
            uVar2 = local_40;
          }
          uVar16 = uVar2;
          if (puVar1[4] == 0) {
            uVar16 = ((uVar2 & 0xff) * 0x10000 + (uVar2 & 0xff00)) * 0x100 +
                     (local_40 >> 0x10 & 0xff) * 0x100 + (uVar2 >> 0x18);
            local_18 = uVar16;
          }
          local_20 = local_30;
          if (uVar16 != puVar1[6]) {
            param_1[6] = (int)s_incorrect_data_check_004205a0;
            goto LAB_00409181;
          }
          uVar16 = 0;
          pbVar9 = (byte *)0x0;
          local_40 = 0;
        }
        *puVar1 = 0x19;
      case (undefined *)0x40912e:
        if ((puVar1[2] != 0) && (uVar2 = uVar16, puVar1[4] != 0)) {
          for (; pbVar9 < (byte *)0x20; pbVar9 = pbVar9 + 8) {
            uVar13 = 0;
            if (uVar15 == 0) goto LAB_004091e5;
            uVar15 = uVar15 - 1;
            local_40 = uVar2 + ((uint)(byte)*puVar11 << ((byte)pbVar9 & 0x1f));
            puVar11 = (uint *)((int)puVar11 + 1);
            uVar2 = local_40;
            local_3c = uVar15;
            local_38 = puVar11;
          }
          if (uVar2 != puVar1[7]) {
            param_1[6] = (int)s_incorrect_length_check_00420588;
            goto LAB_00409181;
          }
          uVar16 = 0;
          pbVar9 = (byte *)0x0;
        }
        *puVar1 = 0x1a;
switchD_00407d1a_caseD_4091d3:
        local_1c = 1;
        uVar2 = uVar16;
        uVar13 = uVar15;
        goto LAB_004091e5;
      case (undefined *)0x4091d3:
        goto switchD_00407d1a_caseD_4091d3;
      case (undefined *)0x4091dd:
        local_1c = -3;
        goto LAB_004091e5;
      case (undefined *)0x409235:
        goto switchD_00407d1a_caseD_409235;
      }
      uVar4 = *puVar1;
    }
  }
  return -2;
}



void __cdecl FUN_00409390(int param_1)

{
  *(undefined **)(param_1 + 0x4c) = &DAT_0041a2a0;
  *(undefined4 *)(param_1 + 0x54) = 9;
  *(undefined **)(param_1 + 0x50) = &DAT_0041aaa0;
  *(undefined4 *)(param_1 + 0x58) = 5;
  return;
}



undefined4 __cdecl FUN_004093c0(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  
  iVar1 = *(int *)(param_1 + 0x1c);
  if (*(int *)(iVar1 + 0x34) == 0) {
    iVar2 = (**(code **)(param_1 + 0x20))
                      (*(undefined4 *)(param_1 + 0x28),
                       1 << ((byte)*(undefined4 *)(iVar1 + 0x24) & 0x1f),1);
    *(int *)(iVar1 + 0x34) = iVar2;
    if (iVar2 == 0) {
      return 1;
    }
  }
  if (*(int *)(iVar1 + 0x28) == 0) {
    *(undefined4 *)(iVar1 + 0x30) = 0;
    *(undefined4 *)(iVar1 + 0x2c) = 0;
    *(int *)(iVar1 + 0x28) = 1 << ((byte)*(undefined4 *)(iVar1 + 0x24) & 0x1f);
  }
  uVar4 = *(uint *)(iVar1 + 0x28);
  uVar3 = param_2 - *(int *)(param_1 + 0x10);
  if (uVar4 <= uVar3) {
    puVar6 = (undefined4 *)(*(int *)(param_1 + 0xc) - uVar4);
    puVar7 = *(undefined4 **)(iVar1 + 0x34);
    for (uVar3 = uVar4 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
      *puVar7 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar7 = puVar7 + 1;
    }
    for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
      *(undefined1 *)puVar7 = *(undefined1 *)puVar6;
      puVar6 = (undefined4 *)((int)puVar6 + 1);
      puVar7 = (undefined4 *)((int)puVar7 + 1);
    }
    *(undefined4 *)(iVar1 + 0x30) = 0;
    *(undefined4 *)(iVar1 + 0x2c) = *(undefined4 *)(iVar1 + 0x28);
    return 0;
  }
  uVar4 = uVar4 - *(int *)(iVar1 + 0x30);
  if (uVar3 < uVar4) {
    uVar4 = uVar3;
  }
  puVar6 = (undefined4 *)(*(int *)(param_1 + 0xc) - uVar3);
  puVar7 = (undefined4 *)(*(int *)(iVar1 + 0x34) + *(int *)(iVar1 + 0x30));
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *puVar7 = *puVar6;
    puVar6 = puVar6 + 1;
    puVar7 = puVar7 + 1;
  }
  for (uVar5 = uVar4 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined1 *)puVar7 = *(undefined1 *)puVar6;
    puVar6 = (undefined4 *)((int)puVar6 + 1);
    puVar7 = (undefined4 *)((int)puVar7 + 1);
  }
  uVar3 = uVar3 - uVar4;
  if (uVar3 != 0) {
    puVar6 = (undefined4 *)(*(int *)(param_1 + 0xc) - uVar3);
    puVar7 = *(undefined4 **)(iVar1 + 0x34);
    for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
      *puVar7 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar7 = puVar7 + 1;
    }
    for (uVar4 = uVar3 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
      *(undefined1 *)puVar7 = *(undefined1 *)puVar6;
      puVar6 = (undefined4 *)((int)puVar6 + 1);
      puVar7 = (undefined4 *)((int)puVar7 + 1);
    }
    *(uint *)(iVar1 + 0x30) = uVar3;
    *(undefined4 *)(iVar1 + 0x2c) = *(undefined4 *)(iVar1 + 0x28);
    return 0;
  }
  uVar3 = *(int *)(iVar1 + 0x30) + uVar4;
  *(uint *)(iVar1 + 0x30) = uVar3;
  if (uVar3 == *(uint *)(iVar1 + 0x28)) {
    *(undefined4 *)(iVar1 + 0x30) = 0;
  }
  if (*(uint *)(iVar1 + 0x2c) < *(uint *)(iVar1 + 0x28)) {
    *(uint *)(iVar1 + 0x2c) = *(uint *)(iVar1 + 0x2c) + uVar4;
  }
  return 0;
}



undefined4 __cdecl FUN_004094e0(int param_1)

{
  int iVar1;
  
  if (((param_1 != 0) && (*(int *)(param_1 + 0x1c) != 0)) &&
     (*(code **)(param_1 + 0x24) != (code *)0x0)) {
    iVar1 = *(int *)(*(int *)(param_1 + 0x1c) + 0x34);
    if (iVar1 != 0) {
      (**(code **)(param_1 + 0x24))(*(undefined4 *)(param_1 + 0x28),iVar1);
    }
    (**(code **)(param_1 + 0x24))(*(undefined4 *)(param_1 + 0x28),*(undefined4 *)(param_1 + 0x1c));
    *(undefined4 *)(param_1 + 0x1c) = 0;
    return 0;
  }
  return 0xfffffffe;
}



undefined4 __cdecl
FUN_00409530(int param_1,int param_2,int param_3,int param_4,int param_5,int param_6,char *param_7,
            int param_8)

{
  uint uVar1;
  int *piVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  
  iVar4 = 1;
  if (((param_7 == (char *)0x0) || (*param_7 != '1')) || (param_8 != 0x38)) {
    return 0xfffffffa;
  }
  if (param_1 != 0) {
    *(undefined4 *)(param_1 + 0x18) = 0;
    if (*(int *)(param_1 + 0x20) == 0) {
      *(code **)(param_1 + 0x20) = FUN_0040b2f0;
      *(undefined4 *)(param_1 + 0x28) = 0;
    }
    if (*(int *)(param_1 + 0x24) == 0) {
      *(code **)(param_1 + 0x24) = FUN_0040b310;
    }
    if (param_2 == -1) {
      param_2 = 6;
    }
    if (param_4 < 0) {
      iVar4 = 0;
      param_4 = -param_4;
    }
    else if (0xf < param_4) {
      iVar4 = 2;
      param_4 = param_4 + -0x10;
    }
    if (((((0 < param_5) && (param_5 < 10)) &&
         ((param_3 == 8 && ((7 < param_4 && (param_4 < 0x10)))))) && (-1 < param_2)) &&
       (((param_2 < 10 && (-1 < param_6)) && (param_6 < 5)))) {
      if (param_4 == 8) {
        param_4 = 9;
      }
      piVar2 = (int *)(**(code **)(param_1 + 0x20))(*(undefined4 *)(param_1 + 0x28),1,0x16c0);
      if (piVar2 == (int *)0x0) {
        return 0xfffffffc;
      }
      *(int **)(param_1 + 0x1c) = piVar2;
      piVar2[6] = iVar4;
      piVar2[0xc] = param_4;
      iVar5 = 1 << ((byte)param_4 & 0x1f);
      iVar4 = 1 << ((byte)(param_5 + 7) & 0x1f);
      piVar2[0x14] = param_5 + 7;
      *piVar2 = param_1;
      piVar2[7] = 0;
      piVar2[0xb] = iVar5;
      piVar2[0x13] = iVar4;
      piVar2[0x15] = iVar4 + -1;
      piVar2[0xd] = iVar5 + -1;
      piVar2[0x16] = (param_5 + 9U) / 3;
      iVar4 = (**(code **)(param_1 + 0x20))(*(undefined4 *)(param_1 + 0x28),iVar5,2);
      piVar2[0xe] = iVar4;
      iVar4 = (**(code **)(param_1 + 0x20))(*(undefined4 *)(param_1 + 0x28),piVar2[0xb],2);
      piVar2[0x10] = iVar4;
      iVar4 = (**(code **)(param_1 + 0x20))(*(undefined4 *)(param_1 + 0x28),piVar2[0x13],2);
      piVar2[0x11] = iVar4;
      iVar4 = 1 << ((char)param_5 + 6U & 0x1f);
      piVar2[0x5a7] = iVar4;
      iVar4 = (**(code **)(param_1 + 0x20))(*(undefined4 *)(param_1 + 0x28),iVar4,4);
      uVar1 = piVar2[0x5a7];
      piVar2[2] = iVar4;
      piVar2[3] = uVar1 * 4;
      if (((piVar2[0xe] != 0) && (piVar2[0x10] != 0)) && ((piVar2[0x11] != 0 && (iVar4 != 0)))) {
        *(undefined1 *)(piVar2 + 9) = 8;
        piVar2[0x5a9] = iVar4 + (uVar1 & 0xfffffffe);
        piVar2[0x5a6] = iVar4 + uVar1 * 3;
        piVar2[0x22] = param_6;
        piVar2[0x21] = param_2;
        uVar3 = FUN_00409760(param_1);
        return uVar3;
      }
      piVar2[1] = 0x29a;
      *(char **)(param_1 + 0x18) = s_insufficient_memory_00420770;
      FUN_0040a160(param_1);
      return 0xfffffffc;
    }
  }
  return 0xfffffffe;
}



undefined4 __cdecl FUN_00409760(int param_1)

{
  int iVar1;
  uint uVar2;
  
  if ((((param_1 != 0) && (iVar1 = *(int *)(param_1 + 0x1c), iVar1 != 0)) &&
      (*(int *)(param_1 + 0x20) != 0)) && (*(int *)(param_1 + 0x24) != 0)) {
    *(undefined4 *)(param_1 + 0x14) = 0;
    *(undefined4 *)(param_1 + 8) = 0;
    *(undefined4 *)(param_1 + 0x18) = 0;
    *(undefined4 *)(param_1 + 0x2c) = 2;
    *(undefined4 *)(iVar1 + 0x14) = 0;
    *(undefined4 *)(iVar1 + 0x10) = *(undefined4 *)(iVar1 + 8);
    if (*(int *)(iVar1 + 0x18) < 0) {
      *(int *)(iVar1 + 0x18) = -*(int *)(iVar1 + 0x18);
    }
    *(uint *)(iVar1 + 4) = (-(uint)(*(int *)(iVar1 + 0x18) != 0) & 0xffffffb9) + 0x71;
    if (*(int *)(iVar1 + 0x18) == 2) {
      uVar2 = FUN_0040afe0(0,(uint *)0x0,0);
    }
    else {
      uVar2 = FUN_0040bc50(0,(byte *)0x0,0);
    }
    *(uint *)(param_1 + 0x30) = uVar2;
    *(undefined4 *)(iVar1 + 0x28) = 0;
    FUN_0040bec0(iVar1);
    FUN_0040a220(iVar1);
    return 0;
  }
  return 0xfffffffe;
}



uint __cdecl FUN_004097f0(int *param_1,int param_2)

{
  char cVar1;
  undefined4 *puVar2;
  int *piVar3;
  int iVar4;
  bool bVar5;
  undefined1 uVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  undefined4 *puVar11;
  
  if ((((param_1 == (int *)0x0) || (puVar2 = (undefined4 *)param_1[7], puVar2 == (undefined4 *)0x0))
      || (4 < param_2)) || (param_2 < 0)) {
    return 0xfffffffe;
  }
  if (((param_1[3] == 0) || ((*param_1 == 0 && (param_1[1] != 0)))) ||
     ((puVar2[1] == 0x29a && (param_2 != 4)))) {
    param_1[6] = (int)s_stream_error_00420790;
    return 0xfffffffe;
  }
  if (param_1[4] == 0) {
    param_1[6] = (int)s_buffer_error_00420760;
    return 0xfffffffb;
  }
  iVar8 = puVar2[10];
  *puVar2 = param_1;
  puVar2[10] = param_2;
  if (puVar2[1] == 0x2a) {
    if (puVar2[6] == 2) {
      iVar7 = FUN_0040afe0(0,(uint *)0x0,0);
      param_1[0xc] = iVar7;
      *(undefined1 *)(puVar2[2] + puVar2[5]) = 0x1f;
      iVar7 = puVar2[5];
      puVar2[5] = iVar7 + 1;
      *(undefined1 *)(puVar2[2] + iVar7 + 1) = 0x8b;
      iVar7 = puVar2[5];
      puVar2[5] = iVar7 + 1;
      *(undefined1 *)(puVar2[2] + iVar7 + 1) = 8;
      piVar3 = (int *)puVar2[7];
      iVar7 = puVar2[5] + 1;
      puVar2[5] = iVar7;
      if (piVar3 == (int *)0x0) {
        *(undefined1 *)(puVar2[2] + iVar7) = 0;
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        *(undefined1 *)(puVar2[2] + iVar7 + 1) = 0;
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        *(undefined1 *)(puVar2[2] + iVar7 + 1) = 0;
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        *(undefined1 *)(puVar2[2] + iVar7 + 1) = 0;
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        *(undefined1 *)(puVar2[2] + iVar7 + 1) = 0;
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        if (puVar2[0x21] == 9) {
          uVar6 = 2;
        }
        else if (((int)puVar2[0x22] < 2) && (1 < (int)puVar2[0x21])) {
          uVar6 = 0;
        }
        else {
          uVar6 = 4;
        }
        *(undefined1 *)(puVar2[2] + iVar7 + 1) = uVar6;
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        *(undefined1 *)(puVar2[2] + iVar7 + 1) = 0xb;
        puVar2[1] = 0x71;
        puVar2[5] = puVar2[5] + 1;
      }
      else {
        *(byte *)(puVar2[2] + iVar7) =
             (-(piVar3[9] != 0) & 0x10U) + (-(piVar3[0xb] != 0) & 2U) + (-(piVar3[7] != 0) & 8U) +
             (-(piVar3[4] != 0) & 4U) + (*piVar3 != 0);
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        *(undefined1 *)(puVar2[2] + iVar7 + 1) = *(undefined1 *)(puVar2[7] + 4);
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        *(char *)(puVar2[2] + iVar7 + 1) = (char)((uint)*(undefined4 *)(puVar2[7] + 4) >> 8);
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        *(char *)(puVar2[2] + iVar7 + 1) = (char)((uint)*(undefined4 *)(puVar2[7] + 4) >> 0x10);
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        *(char *)(puVar2[2] + iVar7 + 1) = (char)((uint)*(undefined4 *)(puVar2[7] + 4) >> 0x18);
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        if (puVar2[0x21] == 9) {
          uVar6 = 2;
        }
        else if (((int)puVar2[0x22] < 2) && (1 < (int)puVar2[0x21])) {
          uVar6 = 0;
        }
        else {
          uVar6 = 4;
        }
        *(undefined1 *)(puVar2[2] + iVar7 + 1) = uVar6;
        iVar7 = puVar2[5];
        puVar2[5] = iVar7 + 1;
        *(undefined1 *)(puVar2[2] + iVar7 + 1) = *(undefined1 *)(puVar2[7] + 0xc);
        uVar10 = puVar2[5] + 1;
        puVar2[5] = uVar10;
        if (*(int *)(puVar2[7] + 0x10) != 0) {
          *(undefined1 *)(puVar2[2] + uVar10) = *(undefined1 *)(puVar2[7] + 0x14);
          iVar7 = puVar2[5];
          puVar2[5] = iVar7 + 1;
          *(char *)(puVar2[2] + iVar7 + 1) = (char)((uint)*(undefined4 *)(puVar2[7] + 0x14) >> 8);
          uVar10 = puVar2[5] + 1;
          puVar2[5] = uVar10;
        }
        if (*(int *)(puVar2[7] + 0x2c) != 0) {
          iVar7 = FUN_0040afe0(param_1[0xc],(uint *)puVar2[2],uVar10);
          param_1[0xc] = iVar7;
        }
        puVar2[8] = 0;
        puVar2[1] = 0x45;
      }
    }
    else {
      if (((int)puVar2[0x22] < 2) && (iVar7 = puVar2[0x21], 1 < iVar7)) {
        if (iVar7 < 6) {
          iVar7 = 1;
        }
        else {
          iVar7 = (iVar7 != 6) + 2;
        }
      }
      else {
        iVar7 = 0;
      }
      uVar10 = puVar2[0xc] * 0x1000 - 0x7800U | iVar7 << 6;
      if (puVar2[0x1b] != 0) {
        uVar10 = uVar10 | 0x20;
      }
      puVar2[1] = 0x71;
      FUN_0040a0b0((int)puVar2,(uVar10 - uVar10 % 0x1f) + 0x1f);
      if (puVar2[0x1b] != 0) {
        FUN_0040a0b0((int)puVar2,(uint)param_1[0xc] >> 0x10);
        FUN_0040a0b0((int)puVar2,param_1[0xc] & 0xffff);
      }
      uVar10 = FUN_0040bc50(0,(byte *)0x0,0);
      param_1[0xc] = uVar10;
    }
  }
  if (puVar2[1] == 0x45) {
    iVar7 = puVar2[7];
    if (*(int *)(iVar7 + 0x10) != 0) {
      uVar10 = puVar2[5];
      if ((uint)puVar2[8] < (*(uint *)(iVar7 + 0x14) & 0xffff)) {
        do {
          uVar9 = puVar2[5];
          if (uVar9 == puVar2[3]) {
            if ((*(int *)(iVar7 + 0x2c) != 0) && (uVar10 < uVar9)) {
              iVar7 = FUN_0040afe0(param_1[0xc],(uint *)(puVar2[2] + uVar10),uVar9 - uVar10);
              param_1[0xc] = iVar7;
            }
            FUN_0040a0e0((int)param_1);
            uVar9 = puVar2[5];
            uVar10 = uVar9;
            if (uVar9 == puVar2[3]) break;
          }
          *(undefined1 *)(puVar2[2] + uVar9) =
               *(undefined1 *)(*(int *)(puVar2[7] + 0x10) + puVar2[8]);
          iVar4 = puVar2[8];
          puVar2[5] = puVar2[5] + 1;
          iVar7 = puVar2[7];
          puVar2[8] = iVar4 + 1U;
        } while (iVar4 + 1U < (*(uint *)(iVar7 + 0x14) & 0xffff));
      }
      if ((*(int *)(puVar2[7] + 0x2c) != 0) && (uVar10 < (uint)puVar2[5])) {
        iVar7 = FUN_0040afe0(param_1[0xc],(uint *)(puVar2[2] + uVar10),puVar2[5] - uVar10);
        param_1[0xc] = iVar7;
      }
      if (puVar2[8] != *(int *)(puVar2[7] + 0x14)) goto LAB_00409c40;
      puVar2[8] = 0;
    }
    puVar2[1] = 0x49;
  }
LAB_00409c40:
  if (puVar2[1] == 0x49) {
    if (*(int *)(puVar2[7] + 0x1c) != 0) {
      uVar10 = puVar2[5];
      do {
        uVar9 = puVar2[5];
        if (uVar9 == puVar2[3]) {
          if ((*(int *)(puVar2[7] + 0x2c) != 0) && (uVar10 < uVar9)) {
            iVar7 = FUN_0040afe0(param_1[0xc],(uint *)(puVar2[2] + uVar10),uVar9 - uVar10);
            param_1[0xc] = iVar7;
          }
          FUN_0040a0e0((int)param_1);
          uVar9 = puVar2[5];
          uVar10 = uVar9;
          if (uVar9 == puVar2[3]) {
            bVar5 = true;
            break;
          }
        }
        cVar1 = *(char *)(*(int *)(puVar2[7] + 0x1c) + -1 + puVar2[8] + 1);
        puVar2[8] = puVar2[8] + 1;
        *(char *)(puVar2[2] + uVar9) = cVar1;
        puVar2[5] = puVar2[5] + 1;
        bVar5 = false;
        uVar9 = uVar10;
      } while (cVar1 != '\0');
      if ((*(int *)(puVar2[7] + 0x2c) != 0) && (uVar9 < (uint)puVar2[5])) {
        iVar7 = FUN_0040afe0(param_1[0xc],(uint *)(puVar2[2] + uVar9),puVar2[5] - uVar9);
        param_1[0xc] = iVar7;
      }
      if (bVar5) goto LAB_00409d02;
      puVar2[8] = 0;
    }
    puVar2[1] = 0x5b;
  }
LAB_00409d02:
  if (puVar2[1] == 0x5b) {
    if (*(int *)(puVar2[7] + 0x24) != 0) {
      uVar10 = puVar2[5];
      do {
        uVar9 = puVar2[5];
        if (uVar9 == puVar2[3]) {
          if ((*(int *)(puVar2[7] + 0x2c) != 0) && (uVar10 < uVar9)) {
            iVar7 = FUN_0040afe0(param_1[0xc],(uint *)(puVar2[2] + uVar10),uVar9 - uVar10);
            param_1[0xc] = iVar7;
          }
          FUN_0040a0e0((int)param_1);
          uVar9 = puVar2[5];
          uVar10 = uVar9;
          if (uVar9 == puVar2[3]) {
            bVar5 = true;
            break;
          }
        }
        cVar1 = *(char *)(*(int *)(puVar2[7] + 0x24) + -1 + puVar2[8] + 1);
        puVar2[8] = puVar2[8] + 1;
        *(char *)(puVar2[2] + uVar9) = cVar1;
        puVar2[5] = puVar2[5] + 1;
        bVar5 = false;
        uVar9 = uVar10;
      } while (cVar1 != '\0');
      if ((*(int *)(puVar2[7] + 0x2c) != 0) && (uVar9 < (uint)puVar2[5])) {
        iVar7 = FUN_0040afe0(param_1[0xc],(uint *)(puVar2[2] + uVar9),puVar2[5] - uVar9);
        param_1[0xc] = iVar7;
      }
      if (bVar5) goto LAB_00409dc1;
    }
    puVar2[1] = 0x67;
  }
LAB_00409dc1:
  if (puVar2[1] == 0x67) {
    if (*(int *)(puVar2[7] + 0x2c) != 0) {
      if ((uint)puVar2[3] < puVar2[5] + 2) {
        FUN_0040a0e0((int)param_1);
      }
      if ((uint)puVar2[3] < puVar2[5] + 2) goto LAB_00409e31;
      *(char *)(puVar2[2] + puVar2[5]) = (char)param_1[0xc];
      iVar7 = puVar2[5];
      puVar2[5] = iVar7 + 1;
      *(char *)(puVar2[2] + iVar7 + 1) = (char)((uint)param_1[0xc] >> 8);
      puVar2[5] = puVar2[5] + 1;
      iVar7 = FUN_0040afe0(0,(uint *)0x0,0);
      param_1[0xc] = iVar7;
    }
    puVar2[1] = 0x71;
  }
LAB_00409e31:
  if (puVar2[5] == 0) {
    if (((param_1[1] == 0) && (param_2 <= iVar8)) && (param_2 != 4)) {
      param_1[6] = (int)s_buffer_error_00420760;
      return 0xfffffffb;
    }
  }
  else {
    FUN_0040a0e0((int)param_1);
    if (param_1[4] == 0) {
      puVar2[10] = 0xffffffff;
      return 0;
    }
  }
  if (puVar2[1] == 0x29a) {
    if (param_1[1] != 0) {
      param_1[6] = (int)s_buffer_error_00420760;
      return 0xfffffffb;
    }
LAB_00409eac:
    if ((puVar2[0x1d] == 0) && ((param_2 == 0 || (puVar2[1] == 0x29a)))) goto LAB_00409f6e;
  }
  else if (param_1[1] == 0) goto LAB_00409eac;
  iVar8 = (**(code **)(&DAT_0041ab88 + puVar2[0x21] * 0xc))(puVar2,param_2);
  if ((iVar8 == 2) || (iVar8 == 3)) {
    puVar2[1] = 0x29a;
  }
  if ((iVar8 == 0) || (iVar8 == 2)) {
    if (param_1[4] == 0) {
      puVar2[10] = 0xffffffff;
    }
    return 0;
  }
  if (iVar8 == 1) {
    if (param_2 == 1) {
      FUN_0040c050((int)puVar2);
    }
    else {
      FUN_0040bfb0((int)puVar2,(undefined1 *)0x0,0,0);
      if (param_2 == 3) {
        *(undefined2 *)(puVar2[0x11] + -2 + puVar2[0x13] * 2) = 0;
        uVar10 = puVar2[0x13] * 2 - 2;
        puVar11 = (undefined4 *)puVar2[0x11];
        for (uVar9 = uVar10 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
          *puVar11 = 0;
          puVar11 = puVar11 + 1;
        }
        for (uVar10 = uVar10 & 3; uVar10 != 0; uVar10 = uVar10 - 1) {
          *(undefined1 *)puVar11 = 0;
          puVar11 = (undefined4 *)((int)puVar11 + 1);
        }
      }
    }
    FUN_0040a0e0((int)param_1);
    if (param_1[4] == 0) {
      puVar2[10] = 0xffffffff;
      return 0;
    }
  }
LAB_00409f6e:
  if (param_2 != 4) {
    return 0;
  }
  if ((int)puVar2[6] < 1) {
    return 1;
  }
  if (puVar2[6] == 2) {
    *(char *)(puVar2[2] + puVar2[5]) = (char)param_1[0xc];
    iVar8 = puVar2[5];
    puVar2[5] = iVar8 + 1;
    *(char *)(puVar2[2] + iVar8 + 1) = (char)((uint)param_1[0xc] >> 8);
    iVar8 = puVar2[5];
    puVar2[5] = iVar8 + 1;
    *(char *)(puVar2[2] + iVar8 + 1) = (char)((uint)param_1[0xc] >> 0x10);
    iVar8 = puVar2[5];
    puVar2[5] = iVar8 + 1;
    *(char *)(puVar2[2] + iVar8 + 1) = (char)((uint)param_1[0xc] >> 0x18);
    iVar8 = puVar2[5];
    puVar2[5] = iVar8 + 1;
    *(char *)(puVar2[2] + iVar8 + 1) = (char)param_1[2];
    iVar8 = puVar2[5];
    puVar2[5] = iVar8 + 1;
    *(char *)(puVar2[2] + iVar8 + 1) = (char)((uint)param_1[2] >> 8);
    iVar8 = puVar2[5];
    puVar2[5] = iVar8 + 1;
    *(char *)(puVar2[2] + iVar8 + 1) = (char)((uint)param_1[2] >> 0x10);
    iVar8 = puVar2[5];
    puVar2[5] = iVar8 + 1;
    *(char *)(puVar2[2] + iVar8 + 1) = (char)((uint)param_1[2] >> 0x18);
    puVar2[5] = puVar2[5] + 1;
  }
  else {
    FUN_0040a0b0((int)puVar2,(uint)param_1[0xc] >> 0x10);
    FUN_0040a0b0((int)puVar2,param_1[0xc] & 0xffff);
  }
  FUN_0040a0e0((int)param_1);
  if (0 < (int)puVar2[6]) {
    puVar2[6] = -puVar2[6];
  }
  return (uint)(puVar2[5] == 0);
}



void __cdecl FUN_0040a0b0(int param_1,undefined4 param_2)

{
  int iVar1;
  
  *(char *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) = (char)((uint)param_2 >> 8);
  iVar1 = *(int *)(param_1 + 0x14) + 1;
  *(int *)(param_1 + 0x14) = iVar1;
  *(char *)(*(int *)(param_1 + 8) + iVar1) = (char)param_2;
  *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
  return;
}



void __cdecl FUN_0040a0e0(int param_1)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  
  uVar3 = *(uint *)(*(int *)(param_1 + 0x1c) + 0x14);
  if (*(uint *)(param_1 + 0x10) < uVar3) {
    uVar3 = *(uint *)(param_1 + 0x10);
  }
  if (uVar3 != 0) {
    puVar4 = *(undefined4 **)(*(int *)(param_1 + 0x1c) + 0x10);
    puVar5 = *(undefined4 **)(param_1 + 0xc);
    for (uVar2 = uVar3 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    for (uVar2 = uVar3 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
      *(undefined1 *)puVar5 = *(undefined1 *)puVar4;
      puVar4 = (undefined4 *)((int)puVar4 + 1);
      puVar5 = (undefined4 *)((int)puVar5 + 1);
    }
    *(uint *)(param_1 + 0xc) = *(int *)(param_1 + 0xc) + uVar3;
    *(uint *)(*(int *)(param_1 + 0x1c) + 0x10) = *(int *)(*(int *)(param_1 + 0x1c) + 0x10) + uVar3;
    *(uint *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + uVar3;
    *(uint *)(param_1 + 0x10) = *(int *)(param_1 + 0x10) - uVar3;
    *(uint *)(*(int *)(param_1 + 0x1c) + 0x14) = *(int *)(*(int *)(param_1 + 0x1c) + 0x14) - uVar3;
    iVar1 = *(int *)(param_1 + 0x1c);
    if (*(int *)(iVar1 + 0x14) == 0) {
      *(undefined4 *)(iVar1 + 0x10) = *(undefined4 *)(iVar1 + 8);
    }
  }
  return;
}



uint __cdecl FUN_0040a160(int param_1)

{
  int iVar1;
  int iVar2;
  
  if (((param_1 != 0) && (iVar1 = *(int *)(param_1 + 0x1c), iVar1 != 0)) &&
     (((iVar2 = *(int *)(iVar1 + 4), iVar2 == 0x2a ||
       (((iVar2 == 0x45 || (iVar2 == 0x49)) || (iVar2 == 0x5b)))) ||
      (((iVar2 == 0x67 || (iVar2 == 0x71)) || (iVar2 == 0x29a)))))) {
    if (*(int *)(iVar1 + 8) != 0) {
      (**(code **)(param_1 + 0x24))(*(undefined4 *)(param_1 + 0x28),*(int *)(iVar1 + 8));
    }
    iVar1 = *(int *)(*(int *)(param_1 + 0x1c) + 0x44);
    if (iVar1 != 0) {
      (**(code **)(param_1 + 0x24))(*(undefined4 *)(param_1 + 0x28),iVar1);
    }
    iVar1 = *(int *)(*(int *)(param_1 + 0x1c) + 0x40);
    if (iVar1 != 0) {
      (**(code **)(param_1 + 0x24))(*(undefined4 *)(param_1 + 0x28),iVar1);
    }
    iVar1 = *(int *)(*(int *)(param_1 + 0x1c) + 0x38);
    if (iVar1 != 0) {
      (**(code **)(param_1 + 0x24))(*(undefined4 *)(param_1 + 0x28),iVar1);
    }
    (**(code **)(param_1 + 0x24))(*(undefined4 *)(param_1 + 0x28),*(undefined4 *)(param_1 + 0x1c));
    *(undefined4 *)(param_1 + 0x1c) = 0;
    return (iVar2 != 0x71) - 1 & 0xfffffffd;
  }
  return 0xfffffffe;
}



void __cdecl FUN_0040a220(int param_1)

{
  ushort uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  undefined4 *puVar5;
  
  *(int *)(param_1 + 0x3c) = *(int *)(param_1 + 0x2c) << 1;
  *(undefined2 *)(*(int *)(param_1 + 0x44) + -2 + *(int *)(param_1 + 0x4c) * 2) = 0;
  uVar4 = *(int *)(param_1 + 0x4c) * 2 - 2;
  puVar5 = *(undefined4 **)(param_1 + 0x44);
  for (uVar3 = uVar4 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined1 *)puVar5 = 0;
    puVar5 = (undefined4 *)((int)puVar5 + 1);
  }
  iVar2 = *(int *)(param_1 + 0x84) * 0xc;
  *(uint *)(param_1 + 0x80) = (uint)*(ushort *)(&DAT_0041ab82 + iVar2);
  *(uint *)(param_1 + 0x8c) = (uint)*(ushort *)(&DAT_0041ab80 + iVar2);
  *(uint *)(param_1 + 0x90) = (uint)*(ushort *)(&DAT_0041ab84 + iVar2);
  uVar1 = *(ushort *)(&DAT_0041ab86 + iVar2);
  *(undefined4 *)(param_1 + 0x6c) = 0;
  *(undefined4 *)(param_1 + 0x5c) = 0;
  *(undefined4 *)(param_1 + 0x74) = 0;
  *(undefined4 *)(param_1 + 0x68) = 0;
  *(undefined4 *)(param_1 + 0x48) = 0;
  *(uint *)(param_1 + 0x7c) = (uint)uVar1;
  *(undefined4 *)(param_1 + 0x78) = 2;
  *(undefined4 *)(param_1 + 0x60) = 2;
  return;
}



uint __cdecl FUN_0040a2c0(int *param_1,int param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  undefined1 *puVar6;
  uint uVar7;
  
  uVar7 = 0xffff;
  if (param_1[3] - 5U < 0xffff) {
    uVar7 = param_1[3] - 5U;
  }
  do {
    uVar5 = param_1[0x1d];
    if (uVar5 < 2) {
      FUN_0040a420(param_1);
      uVar5 = param_1[0x1d];
      if (uVar5 == 0) {
        if (param_2 == 0) {
          return 0;
        }
        iVar2 = param_1[0x17];
        if (iVar2 < 0) {
          puVar6 = (undefined1 *)0x0;
        }
        else {
          puVar6 = (undefined1 *)(param_1[0xe] + iVar2);
        }
        FUN_0040c2b0(param_1,puVar6,param_1[0x1b] - iVar2,(uint)(param_2 == 4));
        param_1[0x17] = param_1[0x1b];
        FUN_0040a0e0(*param_1);
        if (*(int *)(*param_1 + 0x10) != 0) {
          return (-(uint)(param_2 != 4) & 0xfffffffe) + 3;
        }
        return (param_2 != 4) - 1 & 2;
      }
    }
    iVar2 = param_1[0x1b];
    param_1[0x1d] = 0;
    param_1[0x1b] = iVar2 + uVar5;
    iVar3 = param_1[0x17];
    uVar4 = param_1[0x1b];
    uVar1 = iVar3 + uVar7;
    if ((iVar2 + uVar5 == 0) || (uVar1 <= uVar4)) {
      param_1[0x1b] = uVar1;
      param_1[0x1d] = uVar4 - uVar1;
      if (iVar3 < 0) {
        puVar6 = (undefined1 *)0x0;
      }
      else {
        puVar6 = (undefined1 *)(param_1[0xe] + iVar3);
      }
      FUN_0040c2b0(param_1,puVar6,uVar1 - iVar3,0);
      param_1[0x17] = param_1[0x1b];
      FUN_0040a0e0(*param_1);
      if (*(int *)(*param_1 + 0x10) == 0) {
        return 0;
      }
    }
    iVar2 = param_1[0x17];
    if (param_1[0xb] - 0x106U <= (uint)(param_1[0x1b] - iVar2)) {
      if (iVar2 < 0) {
        puVar6 = (undefined1 *)0x0;
      }
      else {
        puVar6 = (undefined1 *)(param_1[0xe] + iVar2);
      }
      FUN_0040c2b0(param_1,puVar6,param_1[0x1b] - iVar2,0);
      param_1[0x17] = param_1[0x1b];
      FUN_0040a0e0(*param_1);
      if (*(int *)(*param_1 + 0x10) == 0) {
        return 0;
      }
    }
  } while( true );
}



void __cdecl FUN_0040a420(int *param_1)

{
  uint uVar1;
  ushort uVar2;
  uint uVar3;
  ushort *puVar4;
  uint uVar5;
  undefined4 *puVar6;
  int iVar7;
  undefined4 *puVar8;
  
  uVar1 = param_1[0xb];
  while( true ) {
    uVar5 = (param_1[0xf] - param_1[0x1d]) - param_1[0x1b];
    if (param_1[0xb] + -0x106 + uVar1 <= (uint)param_1[0x1b]) {
      puVar6 = (undefined4 *)(param_1[0xe] + uVar1);
      puVar8 = (undefined4 *)param_1[0xe];
      for (uVar3 = uVar1 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
        *puVar8 = *puVar6;
        puVar6 = puVar6 + 1;
        puVar8 = puVar8 + 1;
      }
      for (uVar3 = uVar1 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
        *(undefined1 *)puVar8 = *(undefined1 *)puVar6;
        puVar6 = (undefined4 *)((int)puVar6 + 1);
        puVar8 = (undefined4 *)((int)puVar8 + 1);
      }
      param_1[0x1c] = param_1[0x1c] - uVar1;
      iVar7 = param_1[0x13];
      param_1[0x1b] = param_1[0x1b] - uVar1;
      param_1[0x17] = param_1[0x17] - uVar1;
      puVar4 = (ushort *)(param_1[0x11] + iVar7 * 2);
      do {
        puVar4 = puVar4 + -1;
        if (*puVar4 < uVar1) {
          uVar2 = 0;
        }
        else {
          uVar2 = *puVar4 - (short)uVar1;
        }
        iVar7 = iVar7 + -1;
        *puVar4 = uVar2;
      } while (iVar7 != 0);
      puVar4 = (ushort *)(param_1[0x10] + uVar1 * 2);
      uVar3 = uVar1;
      do {
        puVar4 = puVar4 + -1;
        if (*puVar4 < uVar1) {
          uVar2 = 0;
        }
        else {
          uVar2 = *puVar4 - (short)uVar1;
        }
        uVar3 = uVar3 - 1;
        *puVar4 = uVar2;
      } while (uVar3 != 0);
      uVar5 = uVar5 + uVar1;
    }
    if (((int *)*param_1)[1] == 0) break;
    uVar5 = FUN_0040a530((int *)*param_1,
                         (undefined4 *)(param_1[0x1d] + param_1[0x1b] + param_1[0xe]),uVar5);
    uVar5 = param_1[0x1d] + uVar5;
    param_1[0x1d] = uVar5;
    if (2 < uVar5) {
      uVar3 = (uint)*(byte *)(param_1[0x1b] + param_1[0xe]);
      param_1[0x12] = uVar3;
      param_1[0x12] =
           (uVar3 << ((byte)param_1[0x16] & 0x1f) ^
           (uint)((byte *)(param_1[0x1b] + param_1[0xe]))[1]) & param_1[0x15];
    }
    if (0x105 < uVar5) {
      return;
    }
    if (*(int *)(*param_1 + 4) == 0) {
      return;
    }
  }
  return;
}



uint __cdecl FUN_0040a530(int *param_1,undefined4 *param_2,uint param_3)

{
  uint uVar1;
  uint uVar2;
  undefined4 *puVar3;
  
  uVar1 = param_1[1];
  uVar2 = uVar1;
  if (param_3 < uVar1) {
    uVar2 = param_3;
  }
  if (uVar2 == 0) {
    return 0;
  }
  param_1[1] = uVar1 - uVar2;
  if (*(int *)(param_1[7] + 0x18) == 1) {
    uVar1 = FUN_0040bc50(param_1[0xc],(byte *)*param_1,uVar2);
  }
  else {
    if (*(int *)(param_1[7] + 0x18) != 2) goto LAB_0040a585;
    uVar1 = FUN_0040afe0(param_1[0xc],(uint *)*param_1,uVar2);
  }
  param_1[0xc] = uVar1;
LAB_0040a585:
  puVar3 = (undefined4 *)*param_1;
  for (uVar1 = uVar2 >> 2; uVar1 != 0; uVar1 = uVar1 - 1) {
    *param_2 = *puVar3;
    puVar3 = puVar3 + 1;
    param_2 = param_2 + 1;
  }
  for (uVar1 = uVar2 & 3; uVar1 != 0; uVar1 = uVar1 - 1) {
    *(undefined1 *)param_2 = *(undefined1 *)puVar3;
    puVar3 = (undefined4 *)((int)puVar3 + 1);
    param_2 = (undefined4 *)((int)param_2 + 1);
  }
  param_1[2] = param_1[2] + uVar2;
  *param_1 = *param_1 + uVar2;
  return uVar2;
}



void __cdecl FUN_0040afe0(uint param_1,uint *param_2,uint param_3)

{
  if (param_2 == (uint *)0x0) {
    return;
  }
  FUN_0040b000(param_1,param_2,param_3);
  return;
}



uint __cdecl FUN_0040b000(uint param_1,uint *param_2,uint param_3)

{
  uint uVar1;
  uint uVar2;
  uint *puVar3;
  
  uVar1 = ~param_1;
  for (; (param_3 != 0 && (((uint)param_2 & 3) != 0)); param_2 = (uint *)((int)param_2 + 1)) {
    uVar1 = uVar1 >> 8 ^ *(uint *)(&DAT_0041ac00 + (uVar1 & 0xff ^ (uint)(byte)*param_2) * 4);
    param_3 = param_3 - 1;
  }
  if (0x1f < param_3) {
    uVar2 = param_3 >> 5;
    puVar3 = param_2;
    do {
      uVar1 = uVar1 ^ *puVar3;
      uVar1 = *(uint *)(&DAT_0041b000 + (uVar1 >> 0x10 & 0xff) * 4) ^
              *(uint *)(&DAT_0041b400 + (uVar1 >> 8 & 0xff) * 4) ^
              *(uint *)(&DAT_0041ac00 + (uVar1 >> 0x18) * 4) ^
              *(uint *)(&DAT_0041b800 + (uVar1 & 0xff) * 4) ^ puVar3[1];
      uVar1 = *(uint *)(&DAT_0041b000 + (uVar1 >> 0x10 & 0xff) * 4) ^
              *(uint *)(&DAT_0041b400 + (uVar1 >> 8 & 0xff) * 4) ^
              *(uint *)(&DAT_0041ac00 + (uVar1 >> 0x18) * 4) ^
              *(uint *)(&DAT_0041b800 + (uVar1 & 0xff) * 4) ^ puVar3[2];
      uVar1 = *(uint *)(&DAT_0041b000 + (uVar1 >> 0x10 & 0xff) * 4) ^
              *(uint *)(&DAT_0041b400 + (uVar1 >> 8 & 0xff) * 4) ^
              *(uint *)(&DAT_0041ac00 + (uVar1 >> 0x18) * 4) ^
              *(uint *)(&DAT_0041b800 + (uVar1 & 0xff) * 4) ^ puVar3[3];
      uVar1 = *(uint *)(&DAT_0041b000 + (uVar1 >> 0x10 & 0xff) * 4) ^
              *(uint *)(&DAT_0041b400 + (uVar1 >> 8 & 0xff) * 4) ^
              *(uint *)(&DAT_0041ac00 + (uVar1 >> 0x18) * 4) ^
              *(uint *)(&DAT_0041b800 + (uVar1 & 0xff) * 4) ^ puVar3[4];
      uVar1 = *(uint *)(&DAT_0041b000 + (uVar1 >> 0x10 & 0xff) * 4) ^
              *(uint *)(&DAT_0041b400 + (uVar1 >> 8 & 0xff) * 4) ^
              *(uint *)(&DAT_0041ac00 + (uVar1 >> 0x18) * 4) ^
              *(uint *)(&DAT_0041b800 + (uVar1 & 0xff) * 4) ^ puVar3[5];
      uVar1 = *(uint *)(&DAT_0041b000 + (uVar1 >> 0x10 & 0xff) * 4) ^
              *(uint *)(&DAT_0041b400 + (uVar1 >> 8 & 0xff) * 4) ^
              *(uint *)(&DAT_0041ac00 + (uVar1 >> 0x18) * 4) ^
              *(uint *)(&DAT_0041b800 + (uVar1 & 0xff) * 4) ^ puVar3[6];
      param_2 = puVar3 + 8;
      param_3 = param_3 - 0x20;
      uVar1 = *(uint *)(&DAT_0041b000 + (uVar1 >> 0x10 & 0xff) * 4) ^
              *(uint *)(&DAT_0041b400 + (uVar1 >> 8 & 0xff) * 4) ^
              *(uint *)(&DAT_0041ac00 + (uVar1 >> 0x18) * 4) ^
              *(uint *)(&DAT_0041b800 + (uVar1 & 0xff) * 4) ^ puVar3[7];
      uVar1 = *(uint *)(&DAT_0041b000 + (uVar1 >> 0x10 & 0xff) * 4) ^
              *(uint *)(&DAT_0041b400 + (uVar1 >> 8 & 0xff) * 4) ^
              *(uint *)(&DAT_0041ac00 + (uVar1 >> 0x18) * 4) ^
              *(uint *)(&DAT_0041b800 + (uVar1 & 0xff) * 4);
      uVar2 = uVar2 - 1;
      puVar3 = param_2;
    } while (uVar2 != 0);
  }
  if (3 < param_3) {
    uVar2 = param_3 >> 2;
    do {
      uVar1 = uVar1 ^ *param_2;
      param_2 = param_2 + 1;
      param_3 = param_3 - 4;
      uVar1 = *(uint *)(&DAT_0041b000 + (uVar1 >> 0x10 & 0xff) * 4) ^
              *(uint *)(&DAT_0041b400 + (uVar1 >> 8 & 0xff) * 4) ^
              *(uint *)(&DAT_0041ac00 + (uVar1 >> 0x18) * 4) ^
              *(uint *)(&DAT_0041b800 + (uVar1 & 0xff) * 4);
      uVar2 = uVar2 - 1;
    } while (uVar2 != 0);
  }
  for (; param_3 != 0; param_3 = param_3 - 1) {
    uVar1 = uVar1 >> 8 ^ *(uint *)(&DAT_0041ac00 + (uVar1 & 0xff ^ (uint)(byte)*param_2) * 4);
    param_2 = (uint *)((int)param_2 + 1);
  }
  return ~uVar1;
}



void __cdecl FUN_0040b2f0(undefined4 param_1,int param_2,int param_3)

{
  _malloc(param_2 * param_3);
  return;
}



void __cdecl FUN_0040b310(undefined4 param_1,undefined *param_2)

{
  FUN_00410fa6(param_2);
  return;
}



void __cdecl FUN_0040b320(int *param_1,uint param_2)

{
  int iVar1;
  byte *pbVar2;
  undefined4 *puVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  uint uVar11;
  undefined1 *puVar12;
  int iVar13;
  undefined1 *puVar14;
  undefined1 *puVar15;
  byte *pbVar16;
  byte *pbVar17;
  byte *pbVar18;
  int iVar19;
  uint uVar20;
  uint uVar21;
  int iVar22;
  undefined1 *puVar23;
  undefined1 *puVar24;
  undefined1 *puVar25;
  uint uVar26;
  byte *local_44;
  undefined1 uStack_6;
  undefined1 uStack_5;
  
  puVar3 = (undefined4 *)param_1[7];
  pbVar16 = (byte *)(*param_1 + -1);
  iVar4 = param_1[4];
  pbVar2 = pbVar16 + param_1[1] + -5;
  puVar23 = (undefined1 *)(param_1[3] + -1);
  iVar13 = iVar4 - param_2;
  uVar20 = puVar3[0xf];
  iVar19 = puVar3[10];
  uVar5 = puVar3[0xb];
  uVar6 = puVar3[0xc];
  iVar22 = puVar3[0xd];
  iVar7 = puVar3[0x13];
  iVar8 = puVar3[0x14];
  uVar9 = puVar3[0x15];
  uVar10 = puVar3[0x16];
  uVar26 = puVar3[0xe];
  puVar24 = puVar23;
  local_44 = pbVar16;
LAB_0040b3b2:
  if (uVar20 < 0xf) {
    pbVar17 = pbVar16 + 1;
    pbVar18 = pbVar16 + 2;
    pbVar16 = pbVar16 + 2;
    uVar26 = uVar26 + ((uint)*pbVar17 << ((byte)uVar20 & 0x1f)) +
             ((uint)*pbVar18 << ((byte)uVar20 + 8 & 0x1f));
    uVar20 = uVar20 + 0x10;
    local_44 = pbVar16;
  }
  uVar11 = *(uint *)(iVar7 + ((1 << ((byte)uVar9 & 0x1f)) - 1U & uVar26) * 4);
  uStack_6 = (undefined1)(uVar11 >> 0x10);
  uStack_5 = (undefined1)(uVar11 >> 0x18);
  uVar26 = uVar26 >> ((byte)(uVar11 >> 8) & 0x1f);
  uVar20 = uVar20 - (uVar11 >> 8 & 0xff);
  while ((uVar11 & 0xff) != 0) {
    if ((uVar11 & 0x10) != 0) {
      param_2 = (uint)CONCAT11(uStack_5,uStack_6);
      uVar11 = uVar11 & 0xf;
      pbVar18 = pbVar16;
      if (uVar11 != 0) {
        if (uVar20 < uVar11) {
          pbVar16 = pbVar16 + 1;
          uVar26 = uVar26 + ((uint)*pbVar16 << ((byte)uVar20 & 0x1f));
          uVar20 = uVar20 + 8;
          local_44 = pbVar16;
        }
        param_2 = param_2 + ((1 << (sbyte)uVar11) - 1U & uVar26);
        uVar26 = uVar26 >> (sbyte)uVar11;
        uVar20 = uVar20 - uVar11;
        pbVar18 = pbVar16;
      }
      if (uVar20 < 0xf) {
        pbVar17 = pbVar18 + 1;
        pbVar16 = pbVar18 + 2;
        pbVar18 = pbVar18 + 2;
        uVar26 = uVar26 + ((uint)*pbVar17 << ((byte)uVar20 & 0x1f)) +
                 ((uint)*pbVar16 << ((byte)uVar20 + 8 & 0x1f));
        uVar20 = uVar20 + 0x10;
        local_44 = pbVar18;
      }
      uVar11 = *(uint *)(iVar8 + ((1 << ((byte)uVar10 & 0x1f)) - 1U & uVar26) * 4);
      uStack_6 = (undefined1)(uVar11 >> 0x10);
      uStack_5 = (undefined1)(uVar11 >> 0x18);
      uVar26 = uVar26 >> ((byte)(uVar11 >> 8) & 0x1f);
      uVar20 = uVar20 - (uVar11 >> 8 & 0xff);
      goto joined_r0x0040b4f3;
    }
    if ((uVar11 & 0x40) != 0) {
      if ((uVar11 & 0x20) == 0) {
        param_1[6] = (int)s_invalid_literal_length_code_004205f0;
        goto LAB_0040b755;
      }
      *puVar3 = 0xb;
      goto LAB_0040b75c;
    }
    uVar11 = *(uint *)(iVar7 + (((1 << ((byte)uVar11 & 0x1f)) - 1U & uVar26) +
                               (uint)CONCAT11(uStack_5,uStack_6)) * 4);
    uStack_6 = (undefined1)(uVar11 >> 0x10);
    uStack_5 = (undefined1)(uVar11 >> 0x18);
    uVar26 = uVar26 >> ((byte)(uVar11 >> 8) & 0x1f);
    uVar20 = uVar20 - (uVar11 >> 8 & 0xff);
  }
  puVar24[1] = uStack_6;
  puVar15 = puVar24 + 1;
  goto LAB_0040b706;
joined_r0x0040b4f3:
  pbVar16 = pbVar18;
  if ((uVar11 & 0x10) != 0) goto LAB_0040b53f;
  if ((uVar11 & 0x40) != 0) {
    param_1[6] = (int)s_invalid_distance_code_004205d8;
LAB_0040b755:
    *puVar3 = 0x1b;
    goto LAB_0040b75c;
  }
  uVar11 = *(uint *)(iVar8 + (((1 << ((byte)uVar11 & 0x1f)) - 1U & uVar26) +
                             (uint)CONCAT11(uStack_5,uStack_6)) * 4);
  uStack_6 = (undefined1)(uVar11 >> 0x10);
  uStack_5 = (undefined1)(uVar11 >> 0x18);
  uVar26 = uVar26 >> ((byte)(uVar11 >> 8) & 0x1f);
  uVar20 = uVar20 - (uVar11 >> 8 & 0xff);
  goto joined_r0x0040b4f3;
LAB_0040b53f:
  uVar11 = uVar11 & 0xf;
  uVar21 = uVar20;
  if (uVar20 < uVar11) {
    pbVar16 = pbVar18 + 1;
    uVar21 = uVar20 + 8;
    uVar26 = uVar26 + ((uint)*pbVar16 << ((byte)uVar20 & 0x1f));
    local_44 = pbVar16;
    if (uVar21 < uVar11) {
      pbVar16 = pbVar18 + 2;
      uVar26 = uVar26 + ((uint)*pbVar16 << ((byte)uVar21 & 0x1f));
      uVar21 = uVar20 + 0x10;
      local_44 = pbVar16;
    }
  }
  uVar20 = uVar21 - uVar11;
  uVar21 = (uint)CONCAT11(uStack_5,uStack_6) + ((1 << (sbyte)uVar11) - 1U & uVar26);
  uVar26 = uVar26 >> (sbyte)uVar11;
  if (uVar21 <= (uint)((int)puVar24 - (int)(puVar23 + iVar13))) {
    puVar14 = puVar24 + -uVar21;
    do {
      puVar25 = puVar24;
      puVar12 = puVar14;
      puVar25[1] = puVar12[1];
      puVar25[2] = puVar12[2];
      puVar15 = puVar25 + 3;
      *puVar15 = puVar12[3];
      param_2 = param_2 - 3;
      puVar14 = puVar12 + 3;
      puVar24 = puVar15;
    } while (2 < param_2);
    if (param_2 != 0) {
      puVar15 = puVar25 + 4;
      *puVar15 = puVar12[4];
      if (1 < param_2) {
        puVar15 = puVar25 + 5;
        *puVar15 = puVar12[5];
      }
    }
    goto LAB_0040b706;
  }
  uVar11 = uVar21 - ((int)puVar24 - (int)(puVar23 + iVar13));
  if (uVar5 < uVar11) {
    param_1[6] = (int)s_invalid_distance_too_far_back_004205b8;
    *puVar3 = 0x1b;
LAB_0040b75c:
    iVar19 = (int)pbVar16 - (uVar20 >> 3);
    iVar22 = uVar20 + (uVar20 >> 3) * -8;
    *param_1 = iVar19 + 1;
    param_1[3] = (int)(puVar24 + 1);
    param_1[1] = (int)(pbVar2 + (5 - iVar19));
    param_1[4] = (int)(puVar23 + iVar4 + -0x101 + (0x101 - (int)puVar24));
    puVar3[0xe] = uVar26 & (1 << ((byte)iVar22 & 0x1f)) - 1U;
    puVar3[0xf] = iVar22;
    return;
  }
  iVar1 = iVar22 + -1;
  if (uVar6 == 0) {
    puVar14 = (undefined1 *)(iVar1 + (iVar19 - uVar11));
    if (uVar11 < param_2) {
      param_2 = param_2 - uVar11;
      do {
        puVar15 = puVar14 + 1;
        puVar14 = puVar14 + 1;
        puVar24 = puVar24 + 1;
        uVar11 = uVar11 - 1;
        *puVar24 = *puVar15;
      } while (uVar11 != 0);
LAB_0040b660:
      puVar14 = puVar24 + -uVar21;
    }
  }
  else if (uVar6 < uVar11) {
    puVar14 = (undefined1 *)(iVar1 + (uVar6 - uVar11) + iVar19);
    uVar11 = uVar11 - uVar6;
    if (uVar11 < param_2) {
      param_2 = param_2 - uVar11;
      do {
        puVar15 = puVar14 + 1;
        puVar14 = puVar14 + 1;
        puVar24 = puVar24 + 1;
        uVar11 = uVar11 - 1;
        *puVar24 = *puVar15;
      } while (uVar11 != 0);
      puVar14 = (undefined1 *)(iVar22 + -1);
      if (uVar6 < param_2) {
        param_2 = param_2 - uVar6;
        uVar11 = uVar6;
        do {
          puVar15 = puVar14 + 1;
          puVar14 = puVar14 + 1;
          puVar24 = puVar24 + 1;
          uVar11 = uVar11 - 1;
          *puVar24 = *puVar15;
        } while (uVar11 != 0);
        goto LAB_0040b660;
      }
    }
  }
  else {
    puVar14 = (undefined1 *)(iVar1 + (uVar6 - uVar11));
    if (uVar11 < param_2) {
      param_2 = param_2 - uVar11;
      do {
        puVar15 = puVar14 + 1;
        puVar14 = puVar14 + 1;
        puVar24 = puVar24 + 1;
        uVar11 = uVar11 - 1;
        *puVar24 = *puVar15;
      } while (uVar11 != 0);
      goto LAB_0040b660;
    }
  }
  if (2 < param_2) {
    uVar11 = param_2 / 3;
    do {
      puVar15 = puVar14 + 2;
      puVar24[1] = puVar14[1];
      puVar14 = puVar14 + 3;
      puVar24[2] = *puVar15;
      puVar24 = puVar24 + 3;
      *puVar24 = *puVar14;
      param_2 = param_2 - 3;
      uVar11 = uVar11 - 1;
      pbVar16 = local_44;
    } while (uVar11 != 0);
  }
  puVar15 = puVar24;
  if (param_2 != 0) {
    puVar15 = puVar24 + 1;
    *puVar15 = puVar14[1];
    if (1 < param_2) {
      puVar15 = puVar24 + 2;
      *puVar15 = puVar14[2];
    }
  }
LAB_0040b706:
  puVar24 = puVar15;
  if ((pbVar2 <= pbVar16) || (puVar23 + iVar4 + -0x101 <= puVar15)) goto LAB_0040b75c;
  goto LAB_0040b3b2;
}



undefined4 __cdecl
FUN_0040b7b0(int param_1,ushort *param_2,uint param_3,int *param_4,uint *param_5,ushort *param_6)

{
  ushort uVar1;
  char cVar2;
  ushort *puVar3;
  byte bVar4;
  ushort *puVar5;
  uint uVar6;
  uint uVar7;
  byte bVar8;
  int iVar9;
  uint uVar10;
  int *piVar11;
  int iVar12;
  byte bVar13;
  uint uVar14;
  uint uVar15;
  uint uVar16;
  undefined4 local_7c;
  uint local_78;
  uint local_74;
  int local_70;
  ushort *local_68;
  uint local_64;
  int local_60;
  ushort *local_5c;
  ushort *local_58;
  uint local_54;
  uint local_50;
  ushort local_40 [15];
  ushort local_22 [17];
  
  puVar5 = local_40;
  for (iVar9 = 8; puVar3 = param_2, uVar15 = param_3, iVar9 != 0; iVar9 = iVar9 + -1) {
    puVar5[0] = 0;
    puVar5[1] = 0;
    puVar5 = puVar5 + 2;
  }
  for (; uVar15 != 0; uVar15 = uVar15 - 1) {
    local_40[*puVar3] = local_40[*puVar3] + 1;
    puVar3 = puVar3 + 1;
  }
  uVar15 = 0xf;
  puVar5 = local_22;
  do {
    if (*puVar5 != 0) break;
    uVar15 = uVar15 - 1;
    puVar5 = puVar5 + -1;
  } while (uVar15 != 0);
  local_78 = *param_5;
  if (uVar15 < *param_5) {
    local_78 = uVar15;
  }
  if (uVar15 == 0) {
    *(undefined4 *)*param_4 = 0x140;
    iVar9 = *param_4;
    *param_4 = iVar9 + 4;
    *(undefined4 *)(iVar9 + 4) = 0x140;
    *param_4 = *param_4 + 4;
    *param_5 = 1;
    return 0;
  }
  uVar10 = 1;
  puVar5 = local_40;
  do {
    puVar5 = puVar5 + 1;
    if (*puVar5 != 0) break;
    uVar10 = uVar10 + 1;
  } while (uVar10 < 0x10);
  if (local_78 < uVar10) {
    local_78 = uVar10;
  }
  iVar9 = 1;
  puVar5 = local_40;
  uVar6 = 1;
  do {
    puVar5 = puVar5 + 1;
    iVar9 = iVar9 * 2 - (uint)*puVar5;
    if (iVar9 < 0) {
      return 0xffffffff;
    }
    uVar6 = uVar6 + 1;
  } while (uVar6 < 0x10);
  uVar6 = 0;
  if ((0 < iVar9) && ((param_1 == 0 || (uVar15 != 1)))) {
    return 0xffffffff;
  }
  local_22[2] = 0;
  uVar16 = 0;
  do {
    uVar7 = uVar16 + 2;
    *(short *)((int)local_22 + uVar16 + 6) =
         *(short *)((int)local_40 + uVar16 + 2) + *(short *)((int)local_22 + uVar16 + 4);
    uVar16 = uVar7;
  } while (uVar7 < 0x1c);
  puVar5 = param_2;
  if (param_3 != 0) {
    do {
      if (*puVar5 != 0) {
        param_6[local_22[*puVar5 + 1]] = (ushort)uVar6;
        local_22[*puVar5 + 1] = local_22[*puVar5 + 1] + 1;
      }
      uVar6 = uVar6 + 1;
      puVar5 = puVar5 + 1;
    } while (uVar6 < param_3);
  }
  if (param_1 == 0) {
    local_5c = param_6;
    local_58 = param_6;
    local_60 = 0x13;
  }
  else if (param_1 == 1) {
    local_60 = 0x100;
    local_58 = (ushort *)&DAT_0041ca56;
    local_5c = (ushort *)&DAT_0041ca96;
  }
  else {
    local_58 = (ushort *)&DAT_0041ccd8;
    local_5c = (ushort *)&DAT_0041cd18;
    local_60 = -1;
  }
  uVar16 = 0;
  local_54 = 0xffffffff;
  local_70 = *param_4;
  bVar8 = (byte)local_78;
  local_64 = 1 << (bVar8 & 0x1f);
  local_74 = 0;
  uVar6 = local_64 - 1;
  if ((param_1 != 1) || (local_64 < 0x5b0)) {
    local_68 = param_6;
    local_50 = local_64;
LAB_0040b9cf:
    do {
      cVar2 = (char)uVar10;
      bVar13 = (byte)local_74;
      bVar4 = cVar2 - bVar13;
      uVar7 = (uint)*local_68;
      if ((int)uVar7 < local_60) {
        local_7c = (uint)CONCAT21(*local_68,bVar4) << 8;
      }
      else if (local_60 < (int)uVar7) {
        local_7c = CONCAT22(local_58[uVar7],CONCAT11(bVar4,(char)local_5c[uVar7]));
      }
      else {
        local_7c = CONCAT31((uint3)bVar4,0x60);
      }
      iVar9 = 1 << (cVar2 - bVar13 & 0x1f);
      piVar11 = (int *)(local_70 + ((uVar16 >> (bVar13 & 0x1f)) + local_50) * 4);
      uVar7 = local_50;
      do {
        uVar7 = uVar7 - iVar9;
        piVar11 = piVar11 + -iVar9;
        *piVar11 = local_7c;
      } while (uVar7 != 0);
      for (uVar7 = 1 << (cVar2 - 1U & 0x1f); (uVar16 & uVar7) != 0; uVar7 = uVar7 >> 1) {
      }
      if (uVar7 == 0) {
        uVar16 = 0;
      }
      else {
        uVar16 = (uVar7 - 1 & uVar16) + uVar7;
      }
      local_68 = local_68 + 1;
      local_40[uVar10] = local_40[uVar10] - 1;
      if (local_40[uVar10] == 0) {
        if (uVar10 == uVar15) {
          local_7c._0_2_ = CONCAT11(cVar2 - bVar13,0x40);
          local_7c = (uint)(ushort)local_7c;
          goto joined_r0x0040bba9;
        }
        uVar10 = (uint)param_2[*local_68];
      }
    } while ((uVar10 <= local_78) || (uVar7 = uVar6 & uVar16, uVar7 == local_54));
    if (local_74 == 0) {
      local_74 = local_78;
    }
    local_70 = local_70 + local_50 * 4;
    iVar12 = uVar10 - local_74;
    uVar14 = local_74 + iVar12;
    iVar9 = 1 << ((byte)iVar12 & 0x1f);
    if (uVar14 < uVar15) {
      puVar5 = local_40 + uVar14;
      do {
        uVar1 = *puVar5;
        if ((int)(iVar9 - (uint)uVar1) < 1) break;
        iVar12 = iVar12 + 1;
        uVar14 = uVar14 + 1;
        puVar5 = puVar5 + 1;
        iVar9 = (iVar9 - (uint)uVar1) * 2;
      } while (uVar14 < uVar15);
    }
    local_50 = 1 << ((byte)iVar12 & 0x1f);
    local_64 = local_64 + local_50;
    if ((param_1 != 1) || (local_64 < 0x5b0)) {
      *(byte *)(*param_4 + uVar7 * 4) = (byte)iVar12;
      *(byte *)(*param_4 + 1 + uVar7 * 4) = bVar8;
      *(short *)(*param_4 + 2 + uVar7 * 4) = (short)(local_70 - *param_4 >> 2);
      local_54 = uVar7;
      goto LAB_0040b9cf;
    }
  }
  return 1;
joined_r0x0040bba9:
  if (uVar16 == 0) {
LAB_0040bc0c:
    *param_4 = *param_4 + local_64 * 4;
    *param_5 = local_78;
    return 0;
  }
  if ((local_74 != 0) && ((uVar6 & uVar16) != local_54)) {
    local_7c._0_2_ = CONCAT11(bVar8,(undefined1)local_7c);
    local_7c = (uint)(ushort)local_7c;
    local_70 = *param_4;
    local_74 = 0;
    uVar10 = local_78;
  }
  *(uint *)(local_70 + (uVar16 >> ((byte)local_74 & 0x1f)) * 4) = local_7c;
  for (uVar15 = 1 << ((char)uVar10 - 1U & 0x1f); (uVar16 & uVar15) != 0; uVar15 = uVar15 >> 1) {
  }
  if (uVar15 == 0) goto LAB_0040bc0c;
  uVar16 = (uVar15 - 1 & uVar16) + uVar15;
  goto joined_r0x0040bba9;
}



uint __cdecl FUN_0040bc50(uint param_1,byte *param_2,uint param_3)

{
  byte *pbVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  int iVar17;
  int iVar18;
  uint uVar19;
  byte *pbVar20;
  uint uVar21;
  
  uVar21 = param_1 >> 0x10;
  uVar2 = param_1 & 0xffff;
  if (param_3 == 1) {
    uVar2 = uVar2 + *param_2;
    if (0xfff0 < uVar2) {
      uVar2 = uVar2 - 0xfff1;
    }
    uVar21 = uVar21 + uVar2;
    if (0xfff0 < uVar21) {
      uVar21 = uVar21 - 0xfff1;
    }
    return uVar21 << 0x10 | uVar2;
  }
  if (param_2 != (byte *)0x0) {
    if (0xf < param_3) {
      pbVar20 = param_2;
      if (0x15af < param_3) {
        param_2 = (byte *)(param_3 / 0x15b0);
        do {
          param_3 = param_3 - 0x15b0;
          iVar18 = 0x15b;
          do {
            iVar3 = uVar2 + *pbVar20;
            iVar4 = iVar3 + (uint)pbVar20[1];
            iVar5 = iVar4 + (uint)pbVar20[2];
            iVar6 = iVar5 + (uint)pbVar20[3];
            iVar7 = iVar6 + (uint)pbVar20[4];
            iVar8 = iVar7 + (uint)pbVar20[5];
            iVar9 = iVar8 + (uint)pbVar20[6];
            iVar10 = iVar9 + (uint)pbVar20[7];
            iVar11 = iVar10 + (uint)pbVar20[8];
            iVar12 = iVar11 + (uint)pbVar20[9];
            iVar13 = iVar12 + (uint)pbVar20[10];
            iVar14 = iVar13 + (uint)pbVar20[0xb];
            iVar15 = iVar14 + (uint)pbVar20[0xc];
            iVar16 = iVar15 + (uint)pbVar20[0xd];
            pbVar1 = pbVar20 + 0xf;
            iVar17 = iVar16 + (uint)pbVar20[0xe];
            pbVar20 = pbVar20 + 0x10;
            uVar2 = iVar17 + (uint)*pbVar1;
            uVar21 = uVar21 + iVar3 + iVar4 + iVar5 + iVar6 + iVar7 + iVar8 + iVar9 + iVar10 +
                     iVar11 + iVar12 + iVar13 + iVar14 + iVar15 + iVar16 + iVar17 + uVar2;
            iVar18 = iVar18 + -1;
          } while (iVar18 != 0);
          uVar2 = uVar2 % 0xfff1;
          uVar21 = uVar21 % 0xfff1;
          param_2 = param_2 + -1;
        } while (param_2 != (byte *)0x0);
      }
      if (param_3 != 0) {
        if (0xf < param_3) {
          uVar19 = param_3 >> 4;
          do {
            iVar18 = uVar2 + *pbVar20;
            iVar3 = iVar18 + (uint)pbVar20[1];
            iVar4 = iVar3 + (uint)pbVar20[2];
            iVar5 = iVar4 + (uint)pbVar20[3];
            iVar6 = iVar5 + (uint)pbVar20[4];
            iVar7 = iVar6 + (uint)pbVar20[5];
            iVar8 = iVar7 + (uint)pbVar20[6];
            iVar9 = iVar8 + (uint)pbVar20[7];
            iVar10 = iVar9 + (uint)pbVar20[8];
            iVar11 = iVar10 + (uint)pbVar20[9];
            iVar12 = iVar11 + (uint)pbVar20[10];
            iVar13 = iVar12 + (uint)pbVar20[0xb];
            iVar14 = iVar13 + (uint)pbVar20[0xc];
            iVar15 = iVar14 + (uint)pbVar20[0xd];
            iVar16 = iVar15 + (uint)pbVar20[0xe];
            param_3 = param_3 - 0x10;
            uVar2 = iVar16 + (uint)pbVar20[0xf];
            uVar21 = uVar21 + iVar18 + iVar3 + iVar4 + iVar5 + iVar6 + iVar7 + iVar8 + iVar9 +
                     iVar10 + iVar11 + iVar12 + iVar13 + iVar14 + iVar15 + iVar16 + uVar2;
            pbVar20 = pbVar20 + 0x10;
            uVar19 = uVar19 - 1;
          } while (uVar19 != 0);
        }
        for (; param_3 != 0; param_3 = param_3 - 1) {
          uVar2 = uVar2 + *pbVar20;
          pbVar20 = pbVar20 + 1;
          uVar21 = uVar21 + uVar2;
        }
        uVar2 = uVar2 % 0xfff1;
        uVar21 = uVar21 % 0xfff1;
      }
      return uVar21 << 0x10 | uVar2;
    }
    for (; param_3 != 0; param_3 = param_3 - 1) {
      uVar2 = uVar2 + *param_2;
      param_2 = param_2 + 1;
      uVar21 = uVar21 + uVar2;
    }
    if (0xfff0 < uVar2) {
      uVar2 = uVar2 - 0xfff1;
    }
    return uVar21 % 0xfff1 << 0x10 | uVar2;
  }
  return 1;
}



void __cdecl FUN_0040bec0(int param_1)

{
  FUN_0040bf30();
  *(int *)(param_1 + 0xb18) = param_1 + 0x94;
  *(int *)(param_1 + 0xb30) = param_1 + 0xa7c;
  *(undefined ***)(param_1 + 0xb20) = &PTR_DAT_004207c8;
  *(int *)(param_1 + 0xb24) = param_1 + 0x988;
  *(undefined ***)(param_1 + 0xb2c) = &PTR_DAT_004207e0;
  *(undefined **)(param_1 + 0xb38) = &DAT_004207f8;
  *(undefined2 *)(param_1 + 0x16b8) = 0;
  *(undefined4 *)(param_1 + 0x16bc) = 0;
  *(undefined4 *)(param_1 + 0x16b4) = 8;
  FUN_0040bf40(param_1);
  return;
}



void FUN_0040bf30(void)

{
  return;
}



void __cdecl FUN_0040bf40(int param_1)

{
  undefined2 *puVar1;
  int iVar2;
  
  iVar2 = 0x11e;
  puVar1 = (undefined2 *)(param_1 + 0x94);
  do {
    *puVar1 = 0;
    puVar1 = puVar1 + 2;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  puVar1 = (undefined2 *)(param_1 + 0x988);
  iVar2 = 0x1e;
  do {
    *puVar1 = 0;
    puVar1 = puVar1 + 2;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  puVar1 = (undefined2 *)(param_1 + 0xa7c);
  iVar2 = 0x13;
  do {
    *puVar1 = 0;
    puVar1 = puVar1 + 2;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  *(undefined4 *)(param_1 + 0x16ac) = 0;
  *(undefined4 *)(param_1 + 0x16a8) = 0;
  *(undefined4 *)(param_1 + 0x16b0) = 0;
  *(undefined4 *)(param_1 + 0x16a0) = 0;
  *(undefined2 *)(param_1 + 0x494) = 1;
  return;
}



void __cdecl FUN_0040bfb0(int param_1,undefined1 *param_2,int param_3,int param_4)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 0x16bc);
  if (iVar1 < 0xe) {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(param_4 << ((byte)iVar1 & 0x1f));
    *(int *)(param_1 + 0x16bc) = iVar1 + 3;
  }
  else {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(param_4 << ((byte)iVar1 & 0x1f));
    *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
         *(undefined1 *)(param_1 + 0x16b8);
    iVar1 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar1;
    *(undefined1 *)(*(int *)(param_1 + 8) + iVar1) = *(undefined1 *)(param_1 + 0x16b9);
    iVar1 = *(int *)(param_1 + 0x16bc);
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x16bc) = iVar1 + -0xd;
    *(ushort *)(param_1 + 0x16b8) = (ushort)param_4 >> (0x10U - (char)iVar1 & 0x1f);
  }
  FUN_0040d9a0(param_1,param_2,param_3,1);
  return;
}



void __cdecl FUN_0040c050(int param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 0x16bc);
  if (iVar1 < 0xe) {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(2 << ((byte)iVar1 & 0x1f));
    *(int *)(param_1 + 0x16bc) = iVar1 + 3;
  }
  else {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(2 << ((byte)iVar1 & 0x1f));
    *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
         *(undefined1 *)(param_1 + 0x16b8);
    iVar1 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar1;
    *(undefined1 *)(*(int *)(param_1 + 8) + iVar1) = *(undefined1 *)(param_1 + 0x16b9);
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
    iVar1 = *(int *)(param_1 + 0x16bc);
    *(int *)(param_1 + 0x16bc) = iVar1 + -0xd;
    *(ushort *)(param_1 + 0x16b8) = 2 >> (0x10U - (char)iVar1 & 0x1f);
  }
  iVar1 = *(int *)(param_1 + 0x16bc);
  if (iVar1 < 10) {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(0x70000 << ((byte)iVar1 & 0x1f));
    *(int *)(param_1 + 0x16bc) = iVar1 + 7;
  }
  else {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(0 << ((byte)iVar1 & 0x1f));
    *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
         *(undefined1 *)(param_1 + 0x16b8);
    iVar1 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar1;
    *(undefined1 *)(*(int *)(param_1 + 8) + iVar1) = *(undefined1 *)(param_1 + 0x16b9);
    iVar1 = *(int *)(param_1 + 0x16bc);
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x16bc) = iVar1 + -9;
    *(ushort *)(param_1 + 0x16b8) = 0 >> (0x10U - (char)iVar1 & 0x1f);
  }
  FUN_0040d890(param_1);
  iVar1 = *(int *)(param_1 + 0x16bc);
  if ((*(int *)(param_1 + 0x16b4) - iVar1) + 0xb < 9) {
    if (iVar1 < 0xe) {
      *(ushort *)(param_1 + 0x16b8) =
           *(ushort *)(param_1 + 0x16b8) | (ushort)(2 << ((byte)iVar1 & 0x1f));
      *(int *)(param_1 + 0x16bc) = iVar1 + 3;
    }
    else {
      *(ushort *)(param_1 + 0x16b8) =
           *(ushort *)(param_1 + 0x16b8) | (ushort)(2 << ((byte)iVar1 & 0x1f));
      *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
           *(undefined1 *)(param_1 + 0x16b8);
      iVar1 = *(int *)(param_1 + 0x14) + 1;
      *(int *)(param_1 + 0x14) = iVar1;
      *(undefined1 *)(*(int *)(param_1 + 8) + iVar1) = *(undefined1 *)(param_1 + 0x16b9);
      *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
      iVar1 = *(int *)(param_1 + 0x16bc);
      *(int *)(param_1 + 0x16bc) = iVar1 + -0xd;
      *(ushort *)(param_1 + 0x16b8) = 2 >> (0x10U - (char)iVar1 & 0x1f);
    }
    iVar1 = *(int *)(param_1 + 0x16bc);
    if (iVar1 < 10) {
      *(ushort *)(param_1 + 0x16b8) =
           *(ushort *)(param_1 + 0x16b8) | (ushort)(0x70000 << ((byte)iVar1 & 0x1f));
      *(int *)(param_1 + 0x16bc) = iVar1 + 7;
    }
    else {
      *(ushort *)(param_1 + 0x16b8) =
           *(ushort *)(param_1 + 0x16b8) | (ushort)(0 << ((byte)iVar1 & 0x1f));
      *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
           *(undefined1 *)(param_1 + 0x16b8);
      iVar1 = *(int *)(param_1 + 0x14) + 1;
      *(int *)(param_1 + 0x14) = iVar1;
      *(undefined1 *)(*(int *)(param_1 + 8) + iVar1) = *(undefined1 *)(param_1 + 0x16b9);
      iVar1 = *(int *)(param_1 + 0x16bc);
      *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
      *(int *)(param_1 + 0x16bc) = iVar1 + -9;
      *(ushort *)(param_1 + 0x16b8) = 0 >> (0x10U - (char)iVar1 & 0x1f);
    }
    FUN_0040d890(param_1);
  }
  *(undefined4 *)(param_1 + 0x16b4) = 7;
  return;
}



void __cdecl FUN_0040c2b0(int *param_1,undefined1 *param_2,int param_3,int param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  
  iVar3 = 0;
  if (param_1[0x21] < 1) {
    uVar4 = param_3 + 5;
  }
  else {
    if ((param_3 != 0) && (*(int *)(*param_1 + 0x2c) == 2)) {
      FUN_0040d820(param_1);
    }
    FUN_0040c4c0(param_1,param_1 + 0x2c6);
    FUN_0040c4c0(param_1,param_1 + 0x2c9);
    iVar3 = FUN_0040ca90(param_1);
    uVar5 = param_1[0x5aa] + 10U >> 3;
    uVar4 = param_1[0x5ab] + 10U >> 3;
    if (uVar5 < uVar4) goto LAB_0040c323;
  }
  uVar5 = uVar4;
LAB_0040c323:
  if ((uVar5 < param_3 + 4U) || (param_2 == (undefined1 *)0x0)) {
    if ((param_1[0x22] == 4) || (uVar4 == uVar5)) {
      iVar1 = param_1[0x5af];
      iVar3 = param_4 + 2;
      if (iVar1 < 0xe) {
        *(ushort *)(param_1 + 0x5ae) =
             *(ushort *)(param_1 + 0x5ae) | (ushort)(iVar3 << ((byte)iVar1 & 0x1f));
        param_1[0x5af] = iVar1 + 3;
      }
      else {
        *(ushort *)(param_1 + 0x5ae) =
             *(ushort *)(param_1 + 0x5ae) | (ushort)(iVar3 << ((byte)iVar1 & 0x1f));
        *(char *)(param_1[2] + param_1[5]) = (char)param_1[0x5ae];
        iVar1 = param_1[5];
        param_1[5] = iVar1 + 1;
        *(undefined1 *)(iVar1 + 1 + param_1[2]) = *(undefined1 *)((int)param_1 + 0x16b9);
        iVar1 = param_1[0x5af];
        param_1[5] = param_1[5] + 1;
        param_1[0x5af] = iVar1 + -0xd;
        *(ushort *)(param_1 + 0x5ae) = (ushort)iVar3 >> (0x10U - (char)iVar1 & 0x1f);
      }
      FUN_0040d3e0((int)param_1,0x41cea4,0x41d324);
    }
    else {
      iVar2 = param_1[0x5af];
      iVar1 = param_4 + 4;
      if (iVar2 < 0xe) {
        *(ushort *)(param_1 + 0x5ae) =
             *(ushort *)(param_1 + 0x5ae) | (ushort)(iVar1 << ((byte)iVar2 & 0x1f));
        param_1[0x5af] = iVar2 + 3;
      }
      else {
        *(ushort *)(param_1 + 0x5ae) =
             *(ushort *)(param_1 + 0x5ae) | (ushort)(iVar1 << ((byte)iVar2 & 0x1f));
        *(char *)(param_1[5] + param_1[2]) = (char)param_1[0x5ae];
        iVar2 = param_1[5];
        param_1[5] = iVar2 + 1;
        *(undefined1 *)(iVar2 + 1 + param_1[2]) = *(undefined1 *)((int)param_1 + 0x16b9);
        iVar2 = param_1[0x5af];
        param_1[5] = param_1[5] + 1;
        param_1[0x5af] = iVar2 + -0xd;
        *(ushort *)(param_1 + 0x5ae) = (ushort)iVar1 >> (0x10U - (char)iVar2 & 0x1f);
      }
      FUN_0040cbf0((int)param_1,param_1[0x2c7] + 1,param_1[0x2ca] + 1,iVar3 + 1);
      FUN_0040d3e0((int)param_1,(int)(param_1 + 0x25),(int)(param_1 + 0x262));
    }
  }
  else {
    FUN_0040bfb0((int)param_1,param_2,param_3,param_4);
  }
  FUN_0040bf40((int)param_1);
  if (param_4 != 0) {
    FUN_0040d920((int)param_1);
  }
  return;
}



void __cdecl FUN_0040c4c0(int *param_1,int *param_2)

{
  byte bVar1;
  short *psVar2;
  int *piVar3;
  byte bVar4;
  int iVar5;
  short *psVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int local_8;
  int local_4;
  
  piVar3 = param_1;
  psVar2 = (short *)*param_2;
  iVar8 = -1;
  iVar7 = *(int *)param_2[2];
  local_4 = ((int *)param_2[2])[3];
  iVar5 = 0;
  local_8 = -1;
  param_1[0x514] = 0;
  param_1[0x515] = 0x23d;
  psVar6 = psVar2;
  if (0 < local_4) {
    do {
      if (*psVar6 == 0) {
        psVar6[1] = 0;
      }
      else {
        iVar8 = param_1[0x514];
        param_1[0x514] = iVar8 + 1;
        param_1[iVar8 + 0x2d8] = iVar5;
        *(undefined1 *)(iVar5 + 0x1458 + (int)param_1) = 0;
        iVar8 = iVar5;
        local_8 = iVar5;
      }
      iVar5 = iVar5 + 1;
      psVar6 = psVar6 + 2;
    } while (iVar5 < local_4);
  }
  iVar5 = param_1[0x514];
  while (iVar5 < 2) {
    if (iVar8 < 2) {
      iVar9 = iVar8 + 1;
      iVar8 = iVar9;
    }
    else {
      iVar9 = 0;
    }
    param_1[0x514] = iVar5 + 1;
    param_1[iVar5 + 0x2d8] = iVar9;
    psVar2[iVar9 * 2] = 1;
    *(undefined1 *)((int)param_1 + iVar9 + 0x1458) = 0;
    param_1[0x5aa] = param_1[0x5aa] + -1;
    if (iVar7 != 0) {
      param_1[0x5ab] = param_1[0x5ab] - (uint)*(ushort *)(iVar7 + 2 + iVar9 * 4);
    }
    local_8 = iVar8;
    iVar5 = param_1[0x514];
  }
  param_2[1] = iVar8;
  for (iVar7 = param_1[0x514] / 2; 0 < iVar7; iVar7 = iVar7 + -1) {
    FUN_0040c700((int)param_1,(int)psVar2,iVar7);
  }
  param_1 = (int *)(psVar2 + local_4 * 2);
  do {
    iVar7 = piVar3[0x2d8];
    piVar3[0x2d8] = piVar3[piVar3[0x514] + 0x2d7];
    piVar3[0x514] = piVar3[0x514] + -1;
    FUN_0040c700((int)piVar3,(int)psVar2,1);
    iVar8 = piVar3[0x515];
    iVar5 = piVar3[0x2d8];
    piVar3[0x515] = iVar8 + -1;
    piVar3[iVar8 + 0x2d6] = iVar7;
    iVar8 = piVar3[0x515];
    piVar3[0x515] = iVar8 + -1;
    piVar3[iVar8 + 0x2d6] = iVar5;
    *(short *)param_1 = psVar2[iVar5 * 2] + psVar2[iVar7 * 2];
    bVar1 = *(byte *)((int)piVar3 + iVar7 + 0x1458);
    bVar4 = *(byte *)((int)piVar3 + iVar5 + 0x1458);
    if (bVar4 <= bVar1) {
      bVar4 = bVar1;
    }
    *(byte *)((int)piVar3 + local_4 + 0x1458) = bVar4 + 1;
    psVar2[iVar5 * 2 + 1] = (short)local_4;
    psVar2[iVar7 * 2 + 1] = (short)local_4;
    piVar3[0x2d8] = local_4;
    local_4 = local_4 + 1;
    param_1 = param_1 + 1;
    FUN_0040c700((int)piVar3,(int)psVar2,1);
  } while (1 < piVar3[0x514]);
  iVar7 = piVar3[0x515];
  piVar3[0x515] = iVar7 + -1;
  piVar3[iVar7 + 0x2d6] = piVar3[0x2d8];
  FUN_0040c7e0(piVar3,param_2);
  FUN_0040ca10(psVar2,local_8,(int)(piVar3 + 0x2cf));
  return;
}



void __cdecl FUN_0040c700(int param_1,int param_2,int param_3)

{
  ushort uVar1;
  ushort uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  bool bVar8;
  
  iVar6 = *(int *)(param_1 + 0x1450);
  iVar3 = *(int *)(param_1 + 0xb5c + param_3 * 4);
  iVar7 = param_3 * 2;
  bVar8 = SBORROW4(iVar7,iVar6);
  iVar5 = iVar7 - iVar6;
  if (iVar6 < iVar7) {
    *(int *)(param_1 + 0xb5c + param_3 * 4) = iVar3;
    return;
  }
  do {
    iVar6 = iVar7;
    if (bVar8 != iVar5 < 0) {
      iVar5 = *(int *)(param_1 + 0xb60 + iVar7 * 4);
      iVar4 = *(int *)(param_1 + 0xb5c + iVar7 * 4);
      uVar1 = *(ushort *)(param_2 + iVar5 * 4);
      uVar2 = *(ushort *)(param_2 + iVar4 * 4);
      if ((uVar1 < uVar2) ||
         ((uVar1 == uVar2 &&
          (*(byte *)(iVar5 + 0x1458 + param_1) <= *(byte *)(param_1 + 0x1458 + iVar4))))) {
        iVar6 = iVar7 + 1;
      }
    }
    iVar5 = *(int *)(param_1 + 0xb5c + iVar6 * 4);
    uVar1 = *(ushort *)(param_2 + iVar3 * 4);
    uVar2 = *(ushort *)(param_2 + iVar5 * 4);
    if (uVar1 < uVar2) break;
    if ((uVar1 == uVar2) &&
       (*(byte *)(param_1 + 0x1458 + iVar3) <= *(byte *)(iVar5 + 0x1458 + param_1))) {
      *(int *)(param_1 + 0xb5c + param_3 * 4) = iVar3;
      return;
    }
    iVar7 = iVar6 * 2;
    *(int *)(param_1 + 0xb5c + param_3 * 4) = iVar5;
    iVar4 = *(int *)(param_1 + 0x1450);
    bVar8 = SBORROW4(iVar7,iVar4);
    iVar5 = iVar7 - iVar4;
    param_3 = iVar6;
  } while (iVar5 == 0 || iVar7 < iVar4);
  *(int *)(param_1 + 0xb5c + param_3 * 4) = iVar3;
  return;
}



void __cdecl FUN_0040c7e0(int *param_1,int *param_2)

{
  ushort *puVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int *piVar7;
  uint uVar8;
  int iVar9;
  int *piVar10;
  int iVar11;
  short *psVar12;
  ushort *puVar13;
  int *piVar14;
  int *piVar15;
  uint uVar16;
  int iVar17;
  uint uVar18;
  int local_1c;
  int local_14;
  
  piVar7 = param_1;
  iVar3 = *param_2;
  iVar4 = param_2[1];
  piVar15 = (int *)param_2[2];
  iVar9 = *piVar15;
  iVar5 = piVar15[1];
  uVar18 = piVar15[4];
  iVar6 = piVar15[2];
  local_1c = 0;
  piVar15 = param_1 + 0x2cf;
  for (iVar11 = 8; iVar11 != 0; iVar11 = iVar11 + -1) {
    *piVar15 = 0;
    piVar15 = piVar15 + 1;
  }
  *(undefined2 *)(iVar3 + 2 + param_1[param_1[0x515] + 0x2d7] * 4) = 0;
  iVar11 = param_1[0x515] + 1;
  if (iVar11 < 0x23d) {
    param_1 = param_1 + param_1[0x515] + 0x2d8;
    local_14 = 0x23d - iVar11;
    piVar15 = (int *)(iVar11 + local_14);
    do {
      iVar11 = *param_1;
      uVar8 = *(ushort *)(iVar3 + 2 + (uint)*(ushort *)(iVar3 + 2 + iVar11 * 4) * 4) + 1;
      if ((int)uVar18 < (int)uVar8) {
        local_1c = local_1c + 1;
        uVar8 = uVar18;
      }
      *(short *)(iVar3 + 2 + iVar11 * 4) = (short)uVar8;
      if (iVar11 <= iVar4) {
        psVar12 = (short *)((int)piVar7 + uVar8 * 2 + 0xb3c);
        *psVar12 = *psVar12 + 1;
        iVar17 = 0;
        if (iVar6 <= iVar11) {
          iVar17 = *(int *)(iVar5 + (iVar11 - iVar6) * 4);
        }
        uVar16 = (uint)*(ushort *)(iVar3 + iVar11 * 4);
        piVar7[0x5aa] = piVar7[0x5aa] + (uVar8 + iVar17) * uVar16;
        if (iVar9 != 0) {
          piVar7[0x5ab] =
               piVar7[0x5ab] + ((uint)*(ushort *)(iVar9 + 2 + iVar11 * 4) + iVar17) * uVar16;
        }
      }
      param_1 = param_1 + 1;
      local_14 = local_14 + -1;
    } while (local_14 != 0);
    if (local_1c != 0) {
      do {
        iVar9 = uVar18 - 1;
        psVar12 = (short *)((int)piVar7 + iVar9 * 2 + 0xb3c);
        sVar2 = *(short *)((int)piVar7 + iVar9 * 2 + 0xb3c);
        while (sVar2 == 0) {
          psVar12 = psVar12 + -1;
          iVar9 = iVar9 + -1;
          sVar2 = *psVar12;
        }
        psVar12 = (short *)((int)piVar7 + iVar9 * 2 + 0xb3c);
        *psVar12 = *psVar12 + -1;
        psVar12 = (short *)((int)piVar7 + iVar9 * 2 + 0xb3e);
        *psVar12 = *psVar12 + 2;
        psVar12 = (short *)((int)piVar7 + uVar18 * 2 + 0xb3c);
        *psVar12 = *psVar12 + -1;
        local_1c = local_1c + -2;
      } while (0 < local_1c);
      if (uVar18 != 0) {
        puVar13 = (ushort *)((int)piVar7 + uVar18 * 2 + 0xb3c);
        param_2 = piVar15;
        do {
          piVar10 = (int *)(uint)*puVar13;
          if (piVar10 != (int *)0x0) {
            piVar14 = piVar7 + (int)piVar15 + 0x2d7;
            param_1 = piVar10;
            do {
              iVar9 = piVar14[-1];
              piVar15 = (int *)((int)param_2 + -1);
              piVar14 = piVar14 + -1;
              if (iVar9 <= iVar4) {
                puVar1 = (ushort *)(iVar3 + 2 + iVar9 * 4);
                uVar8 = (uint)*puVar1;
                if (uVar8 != uVar18) {
                  piVar7[0x5aa] =
                       piVar7[0x5aa] + (uVar18 - uVar8) * (uint)*(ushort *)(iVar3 + iVar9 * 4);
                  *puVar1 = (ushort)uVar18;
                }
                piVar10 = (int *)((int)param_1 - 1);
                param_1 = piVar10;
              }
              param_2 = piVar15;
            } while (piVar10 != (int *)0x0);
          }
          uVar18 = uVar18 - 1;
          puVar13 = puVar13 + -1;
        } while (uVar18 != 0);
      }
    }
  }
  return;
}



// WARNING: Type propagation algorithm not settling

void __cdecl FUN_0040ca10(undefined2 *param_1,int param_2,int param_3)

{
  ushort uVar1;
  uint uVar2;
  ushort *puVar3;
  int iVar4;
  ushort auStack_20 [16];
  
  uVar1 = 0;
  puVar3 = auStack_20;
  iVar4 = 0xf;
  do {
    puVar3 = (ushort *)((int)puVar3 + 2);
    uVar1 = (*(short *)((param_3 - (int)(auStack_20 + 1)) + (int)puVar3) + uVar1) * 2;
    iVar4 = iVar4 + -1;
    *puVar3 = uVar1;
  } while (iVar4 != 0);
  if (-1 < param_2) {
    iVar4 = param_2 + 1;
    do {
      uVar2 = (uint)(ushort)param_1[1];
      if (uVar2 != 0) {
        uVar1 = auStack_20[uVar2];
        auStack_20[uVar2] = uVar1 + 1;
        uVar2 = FUN_0040d870((uint)uVar1,uVar2);
        *param_1 = (short)uVar2;
      }
      param_1 = param_1 + 2;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  return;
}



void __cdecl FUN_0040ca90(int *param_1)

{
  int iVar1;
  
  FUN_0040cb00((int)param_1,(int)(param_1 + 0x25),param_1[0x2c7]);
  FUN_0040cb00((int)param_1,(int)(param_1 + 0x262),param_1[0x2ca]);
  FUN_0040c4c0(param_1,param_1 + 0x2cc);
  iVar1 = 0x12;
  do {
    if (*(short *)((int)param_1 + (uint)(byte)(&DAT_0041ce90)[iVar1] * 4 + 0xa7e) != 0) break;
    iVar1 = iVar1 + -1;
  } while (2 < iVar1);
  param_1[0x5aa] = param_1[0x5aa] + iVar1 * 3 + 0x11;
  return;
}



void __cdecl FUN_0040cb00(int param_1,int param_2,int param_3)

{
  short *psVar1;
  ushort uVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  ushort *puVar6;
  int iVar7;
  int iVar8;
  uint local_4;
  
  uVar2 = *(ushort *)(param_2 + 2);
  iVar8 = 0;
  local_4 = 0xffffffff;
  iVar5 = 7;
  iVar7 = 4;
  if (uVar2 == 0) {
    iVar5 = 0x8a;
    iVar7 = 3;
  }
  *(undefined2 *)(param_2 + 6 + param_3 * 4) = 0xffff;
  if (-1 < param_3) {
    puVar6 = (ushort *)(param_2 + 6);
    uVar3 = (uint)uVar2;
    param_2 = param_3 + 1;
    do {
      uVar4 = (uint)*puVar6;
      iVar8 = iVar8 + 1;
      if ((iVar5 <= iVar8) || (uVar3 != uVar4)) {
        if (iVar8 < iVar7) {
          psVar1 = (short *)(param_1 + 0xa7c + uVar3 * 4);
          *psVar1 = *psVar1 + (short)iVar8;
        }
        else if (uVar3 == 0) {
          if (iVar8 < 0xb) {
            *(short *)(param_1 + 0xac0) = *(short *)(param_1 + 0xac0) + 1;
          }
          else {
            *(short *)(param_1 + 0xac4) = *(short *)(param_1 + 0xac4) + 1;
          }
        }
        else {
          if (uVar3 != local_4) {
            psVar1 = (short *)(param_1 + 0xa7c + uVar3 * 4);
            *psVar1 = *psVar1 + 1;
          }
          *(short *)(param_1 + 0xabc) = *(short *)(param_1 + 0xabc) + 1;
        }
        iVar8 = 0;
        local_4 = uVar3;
        if (uVar4 == 0) {
          iVar5 = 0x8a;
          iVar7 = 3;
        }
        else if (uVar3 == uVar4) {
          iVar5 = 6;
          iVar7 = 3;
        }
        else {
          iVar5 = 7;
          iVar7 = 4;
        }
      }
      puVar6 = puVar6 + 2;
      param_2 = param_2 + -1;
      uVar3 = uVar4;
    } while (param_2 != 0);
  }
  return;
}



void __cdecl FUN_0040cbf0(int param_1,int param_2,int param_3,int param_4)

{
  ushort uVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = *(int *)(param_1 + 0x16bc);
  if (iVar2 < 0xc) {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(param_2 + -0x101 << ((byte)iVar2 & 0x1f));
    *(int *)(param_1 + 0x16bc) = iVar2 + 5;
  }
  else {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(param_2 + -0x101 << ((byte)iVar2 & 0x1f));
    *(undefined1 *)(*(int *)(param_1 + 0x14) + *(int *)(param_1 + 8)) =
         *(undefined1 *)(param_1 + 0x16b8);
    iVar2 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar2;
    *(undefined1 *)(iVar2 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
    iVar2 = *(int *)(param_1 + 0x16bc);
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x16bc) = iVar2 + -0xb;
    *(ushort *)(param_1 + 0x16b8) = (ushort)(param_2 + -0x101) >> (0x10U - (char)iVar2 & 0x1f);
  }
  iVar2 = *(int *)(param_1 + 0x16bc);
  if (iVar2 < 0xc) {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(param_3 + -1 << ((byte)iVar2 & 0x1f));
    *(int *)(param_1 + 0x16bc) = iVar2 + 5;
  }
  else {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(param_3 + -1 << ((byte)iVar2 & 0x1f));
    *(undefined1 *)(*(int *)(param_1 + 0x14) + *(int *)(param_1 + 8)) =
         *(undefined1 *)(param_1 + 0x16b8);
    iVar2 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar2;
    *(undefined1 *)(iVar2 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
    iVar2 = *(int *)(param_1 + 0x16bc);
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x16bc) = iVar2 + -0xb;
    *(ushort *)(param_1 + 0x16b8) = (ushort)(param_3 + -1) >> (0x10U - (char)iVar2 & 0x1f);
  }
  iVar3 = *(int *)(param_1 + 0x16bc);
  iVar2 = param_4 + -4;
  if (iVar3 < 0xd) {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar2 << ((byte)iVar3 & 0x1f));
    *(int *)(param_1 + 0x16bc) = iVar3 + 4;
  }
  else {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar2 << ((byte)iVar3 & 0x1f));
    *(undefined1 *)(*(int *)(param_1 + 0x14) + *(int *)(param_1 + 8)) =
         *(undefined1 *)(param_1 + 0x16b8);
    iVar3 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar3;
    *(undefined1 *)(iVar3 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
    iVar3 = *(int *)(param_1 + 0x16bc);
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x16bc) = iVar3 + -0xc;
    *(ushort *)(param_1 + 0x16b8) = (ushort)iVar2 >> (0x10U - (char)iVar3 & 0x1f);
  }
  iVar2 = 0;
  if (0 < param_4) {
    do {
      iVar3 = *(int *)(param_1 + 0x16bc);
      if (iVar3 < 0xe) {
        *(ushort *)(param_1 + 0x16b8) =
             *(ushort *)(param_1 + 0x16b8) |
             *(short *)(param_1 + 0xa7e + (uint)(byte)(&DAT_0041ce90)[iVar2] * 4) <<
             ((byte)iVar3 & 0x1f);
        *(int *)(param_1 + 0x16bc) = iVar3 + 3;
      }
      else {
        uVar1 = *(ushort *)(param_1 + 0xa7e + (uint)(byte)(&DAT_0041ce90)[iVar2] * 4);
        *(ushort *)(param_1 + 0x16b8) =
             *(ushort *)(param_1 + 0x16b8) | uVar1 << ((byte)iVar3 & 0x1f);
        *(undefined1 *)(*(int *)(param_1 + 0x14) + *(int *)(param_1 + 8)) =
             *(undefined1 *)(param_1 + 0x16b8);
        iVar3 = *(int *)(param_1 + 0x14) + 1;
        *(int *)(param_1 + 0x14) = iVar3;
        *(undefined1 *)(iVar3 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
        iVar3 = *(int *)(param_1 + 0x16bc);
        *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
        *(int *)(param_1 + 0x16bc) = iVar3 + -0xd;
        *(ushort *)(param_1 + 0x16b8) = uVar1 >> (0x10U - (char)iVar3 & 0x1f);
      }
      iVar2 = iVar2 + 1;
    } while (iVar2 < param_4);
  }
  FUN_0040ce60(param_1,param_1 + 0x94,(ushort *)(param_2 + -1));
  FUN_0040ce60(param_1,param_1 + 0x988,(ushort *)(param_3 + -1));
  return;
}



void __cdecl FUN_0040ce60(int param_1,int param_2,ushort *param_3)

{
  ushort uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  uint uVar8;
  uint local_8;
  int local_4;
  
  local_8 = 0xffffffff;
  iVar2 = 7;
  iVar7 = 4;
  if (*(ushort *)(param_2 + 2) == 0) {
    iVar2 = 0x8a;
    iVar7 = 3;
  }
  if (-1 < (int)param_3) {
    local_4 = (int)param_3 + 1;
    iVar3 = 0;
    uVar5 = (uint)*(ushort *)(param_2 + 2);
    param_3 = (ushort *)(param_2 + 6);
    do {
      iVar4 = iVar3 + 1;
      uVar6 = (uint)*param_3;
      if ((iVar2 <= iVar4) || (uVar5 != uVar6)) {
        if (iVar4 < iVar7) {
          do {
            iVar2 = *(int *)(param_1 + 0x16bc);
            uVar8 = (uint)*(ushort *)(param_1 + 0xa7e + uVar5 * 4);
            if ((int)(0x10 - uVar8) < iVar2) {
              uVar1 = *(ushort *)(param_1 + 0xa7c + uVar5 * 4);
              *(ushort *)(param_1 + 0x16b8) =
                   *(ushort *)(param_1 + 0x16b8) | uVar1 << ((byte)iVar2 & 0x1f);
              *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
                   *(undefined1 *)(param_1 + 0x16b8);
              iVar2 = *(int *)(param_1 + 0x14) + 1;
              *(int *)(param_1 + 0x14) = iVar2;
              *(undefined1 *)(iVar2 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
              *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
              *(ushort *)(param_1 + 0x16b8) =
                   uVar1 >> (0x10U - (char)*(int *)(param_1 + 0x16bc) & 0x1f);
              *(uint *)(param_1 + 0x16bc) = *(int *)(param_1 + 0x16bc) + -0x10 + uVar8;
            }
            else {
              *(ushort *)(param_1 + 0x16b8) =
                   *(ushort *)(param_1 + 0x16b8) |
                   *(short *)(param_1 + 0xa7c + uVar5 * 4) << ((byte)iVar2 & 0x1f);
              *(uint *)(param_1 + 0x16bc) = iVar2 + uVar8;
            }
            iVar4 = iVar4 + -1;
          } while (iVar4 != 0);
        }
        else if (uVar5 == 0) {
          if (iVar4 < 0xb) {
            iVar2 = *(int *)(param_1 + 0x16bc);
            uVar8 = (uint)*(ushort *)(param_1 + 0xac2);
            if ((int)(0x10 - uVar8) < iVar2) {
              uVar1 = *(ushort *)(param_1 + 0xac0);
              *(ushort *)(param_1 + 0x16b8) =
                   *(ushort *)(param_1 + 0x16b8) | uVar1 << ((byte)iVar2 & 0x1f);
              *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
                   *(undefined1 *)(param_1 + 0x16b8);
              iVar2 = *(int *)(param_1 + 0x14) + 1;
              *(int *)(param_1 + 0x14) = iVar2;
              *(undefined1 *)(iVar2 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
              *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
              *(ushort *)(param_1 + 0x16b8) =
                   uVar1 >> (0x10U - (char)*(int *)(param_1 + 0x16bc) & 0x1f);
              *(uint *)(param_1 + 0x16bc) = *(int *)(param_1 + 0x16bc) + -0x10 + uVar8;
            }
            else {
              *(ushort *)(param_1 + 0x16b8) =
                   *(ushort *)(param_1 + 0x16b8) |
                   *(short *)(param_1 + 0xac0) << ((byte)iVar2 & 0x1f);
              *(uint *)(param_1 + 0x16bc) = iVar2 + uVar8;
            }
            iVar2 = *(int *)(param_1 + 0x16bc);
            if (iVar2 < 0xe) {
              *(ushort *)(param_1 + 0x16b8) =
                   *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar3 + -2 << ((byte)iVar2 & 0x1f));
              iVar2 = iVar2 + 3;
LAB_0040d379:
              *(int *)(param_1 + 0x16bc) = iVar2;
            }
            else {
              *(ushort *)(param_1 + 0x16b8) =
                   *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar3 + -2 << ((byte)iVar2 & 0x1f));
              *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
                   *(undefined1 *)(param_1 + 0x16b8);
              iVar2 = *(int *)(param_1 + 0x14) + 1;
              *(int *)(param_1 + 0x14) = iVar2;
              *(undefined1 *)(iVar2 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
              iVar2 = *(int *)(param_1 + 0x16bc);
              *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
              *(int *)(param_1 + 0x16bc) = iVar2 + -0xd;
              *(ushort *)(param_1 + 0x16b8) = (ushort)(iVar3 + -2) >> (0x10U - (char)iVar2 & 0x1f);
            }
          }
          else {
            iVar2 = *(int *)(param_1 + 0x16bc);
            uVar8 = (uint)*(ushort *)(param_1 + 0xac6);
            if ((int)(0x10 - uVar8) < iVar2) {
              uVar1 = *(ushort *)(param_1 + 0xac4);
              *(ushort *)(param_1 + 0x16b8) =
                   *(ushort *)(param_1 + 0x16b8) | uVar1 << ((byte)iVar2 & 0x1f);
              *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
                   *(undefined1 *)(param_1 + 0x16b8);
              iVar2 = *(int *)(param_1 + 0x14) + 1;
              *(int *)(param_1 + 0x14) = iVar2;
              *(undefined1 *)(iVar2 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
              *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
              *(ushort *)(param_1 + 0x16b8) =
                   uVar1 >> (0x10U - (char)*(int *)(param_1 + 0x16bc) & 0x1f);
              *(uint *)(param_1 + 0x16bc) = *(int *)(param_1 + 0x16bc) + -0x10 + uVar8;
            }
            else {
              *(ushort *)(param_1 + 0x16b8) =
                   *(ushort *)(param_1 + 0x16b8) |
                   *(short *)(param_1 + 0xac4) << ((byte)iVar2 & 0x1f);
              *(uint *)(param_1 + 0x16bc) = iVar2 + uVar8;
            }
            iVar2 = *(int *)(param_1 + 0x16bc);
            if (iVar2 < 10) {
              *(ushort *)(param_1 + 0x16b8) =
                   *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar3 + -10 << ((byte)iVar2 & 0x1f));
              iVar2 = iVar2 + 7;
              goto LAB_0040d379;
            }
            *(ushort *)(param_1 + 0x16b8) =
                 *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar3 + -10 << ((byte)iVar2 & 0x1f));
            *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
                 *(undefined1 *)(param_1 + 0x16b8);
            iVar2 = *(int *)(param_1 + 0x14) + 1;
            *(int *)(param_1 + 0x14) = iVar2;
            *(undefined1 *)(iVar2 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
            iVar2 = *(int *)(param_1 + 0x16bc);
            *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
            *(int *)(param_1 + 0x16bc) = iVar2 + -9;
            *(ushort *)(param_1 + 0x16b8) = (ushort)(iVar3 + -10) >> (0x10U - (char)iVar2 & 0x1f);
          }
        }
        else {
          if (uVar5 != local_8) {
            iVar2 = *(int *)(param_1 + 0x16bc);
            uVar8 = (uint)*(ushort *)(param_1 + 0xa7e + uVar5 * 4);
            iVar4 = iVar3;
            if ((int)(0x10 - uVar8) < iVar2) {
              uVar1 = *(ushort *)(param_1 + 0xa7c + uVar5 * 4);
              *(ushort *)(param_1 + 0x16b8) =
                   *(ushort *)(param_1 + 0x16b8) | uVar1 << ((byte)iVar2 & 0x1f);
              *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
                   *(undefined1 *)(param_1 + 0x16b8);
              iVar2 = *(int *)(param_1 + 0x14) + 1;
              *(int *)(param_1 + 0x14) = iVar2;
              *(undefined1 *)(iVar2 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
              *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
              *(ushort *)(param_1 + 0x16b8) =
                   uVar1 >> (0x10U - (char)*(int *)(param_1 + 0x16bc) & 0x1f);
              *(uint *)(param_1 + 0x16bc) = *(int *)(param_1 + 0x16bc) + -0x10 + uVar8;
            }
            else {
              *(ushort *)(param_1 + 0x16b8) =
                   *(ushort *)(param_1 + 0x16b8) |
                   *(short *)(param_1 + 0xa7c + uVar5 * 4) << ((byte)iVar2 & 0x1f);
              *(uint *)(param_1 + 0x16bc) = iVar2 + uVar8;
            }
          }
          iVar2 = *(int *)(param_1 + 0x16bc);
          uVar8 = (uint)*(ushort *)(param_1 + 0xabe);
          if ((int)(0x10 - uVar8) < iVar2) {
            uVar1 = *(ushort *)(param_1 + 0xabc);
            *(ushort *)(param_1 + 0x16b8) =
                 *(ushort *)(param_1 + 0x16b8) | uVar1 << ((byte)iVar2 & 0x1f);
            *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
                 *(undefined1 *)(param_1 + 0x16b8);
            iVar2 = *(int *)(param_1 + 0x14) + 1;
            *(int *)(param_1 + 0x14) = iVar2;
            *(undefined1 *)(iVar2 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
            *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
            *(ushort *)(param_1 + 0x16b8) =
                 uVar1 >> (0x10U - (char)*(int *)(param_1 + 0x16bc) & 0x1f);
            *(uint *)(param_1 + 0x16bc) = *(int *)(param_1 + 0x16bc) + -0x10 + uVar8;
          }
          else {
            *(ushort *)(param_1 + 0x16b8) =
                 *(ushort *)(param_1 + 0x16b8) | *(short *)(param_1 + 0xabc) << ((byte)iVar2 & 0x1f)
            ;
            *(uint *)(param_1 + 0x16bc) = iVar2 + uVar8;
          }
          iVar2 = *(int *)(param_1 + 0x16bc);
          if (iVar2 < 0xf) {
            *(ushort *)(param_1 + 0x16b8) =
                 *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar4 + -3 << ((byte)iVar2 & 0x1f));
            iVar2 = iVar2 + 2;
            goto LAB_0040d379;
          }
          *(ushort *)(param_1 + 0x16b8) =
               *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar4 + -3 << ((byte)iVar2 & 0x1f));
          *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
               *(undefined1 *)(param_1 + 0x16b8);
          iVar2 = *(int *)(param_1 + 0x14) + 1;
          *(int *)(param_1 + 0x14) = iVar2;
          *(undefined1 *)(iVar2 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
          iVar2 = *(int *)(param_1 + 0x16bc);
          *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
          *(int *)(param_1 + 0x16bc) = iVar2 + -0xe;
          *(ushort *)(param_1 + 0x16b8) = (ushort)(iVar4 + -3) >> (0x10U - (char)iVar2 & 0x1f);
        }
        iVar4 = 0;
        local_8 = uVar5;
        if (uVar6 == 0) {
          iVar2 = 0x8a;
          iVar7 = 3;
        }
        else if (uVar5 == uVar6) {
          iVar2 = 6;
          iVar7 = 3;
        }
        else {
          iVar2 = 7;
          iVar7 = 4;
        }
      }
      param_3 = param_3 + 2;
      local_4 = local_4 + -1;
      iVar3 = iVar4;
      uVar5 = uVar6;
    } while (local_4 != 0);
  }
  return;
}



void __cdecl FUN_0040d3e0(int param_1,int param_2,int param_3)

{
  ushort uVar1;
  byte bVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  
  uVar3 = 0;
  if (*(int *)(param_1 + 0x16a0) != 0) {
    do {
      uVar8 = (uint)*(ushort *)(*(int *)(param_1 + 0x16a4) + uVar3 * 2);
      uVar10 = (uint)*(byte *)(uVar3 + *(int *)(param_1 + 0x1698));
      uVar3 = uVar3 + 1;
      if (uVar8 == 0) {
        uVar8 = (uint)*(ushort *)(param_2 + 2 + uVar10 * 4);
        iVar4 = *(int *)(param_1 + 0x16bc);
        if ((int)(0x10 - uVar8) < iVar4) {
          uVar1 = *(ushort *)(param_2 + uVar10 * 4);
          *(ushort *)(param_1 + 0x16b8) =
               *(ushort *)(param_1 + 0x16b8) | uVar1 << ((byte)iVar4 & 0x1f);
          *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
               *(undefined1 *)(param_1 + 0x16b8);
          iVar5 = *(int *)(param_1 + 0x14) + 1;
          *(int *)(param_1 + 0x14) = iVar5;
          *(undefined1 *)(iVar5 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
          *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
          iVar4 = *(int *)(param_1 + 0x16bc) + -0x10 + uVar8;
          *(ushort *)(param_1 + 0x16b8) = uVar1 >> (0x10U - (char)*(int *)(param_1 + 0x16bc) & 0x1f)
          ;
        }
        else {
          *(ushort *)(param_1 + 0x16b8) =
               *(ushort *)(param_1 + 0x16b8) |
               *(short *)(param_2 + uVar10 * 4) << ((byte)iVar4 & 0x1f);
          iVar4 = iVar4 + uVar8;
        }
LAB_0040d757:
        *(int *)(param_1 + 0x16bc) = iVar4;
      }
      else {
        uVar7 = (uint)(byte)(&DAT_0041d59c)[uVar10];
        uVar9 = (uint)*(ushort *)(param_2 + 0x406 + uVar7 * 4);
        bVar2 = (byte)*(int *)(param_1 + 0x16bc);
        if ((int)(0x10 - uVar9) < *(int *)(param_1 + 0x16bc)) {
          uVar1 = *(ushort *)(param_2 + 0x404 + uVar7 * 4);
          *(ushort *)(param_1 + 0x16b8) = *(ushort *)(param_1 + 0x16b8) | uVar1 << (bVar2 & 0x1f);
          *(undefined1 *)(*(int *)(param_1 + 0x14) + *(int *)(param_1 + 8)) =
               *(undefined1 *)(param_1 + 0x16b8);
          iVar5 = *(int *)(param_1 + 0x14) + 1;
          *(int *)(param_1 + 0x14) = iVar5;
          *(undefined1 *)(*(int *)(param_1 + 8) + iVar5) = *(undefined1 *)(param_1 + 0x16b9);
          *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
          *(ushort *)(param_1 + 0x16b8) = uVar1 >> (0x10U - (char)*(int *)(param_1 + 0x16bc) & 0x1f)
          ;
          *(uint *)(param_1 + 0x16bc) = *(int *)(param_1 + 0x16bc) + -0x10 + uVar9;
        }
        else {
          *(ushort *)(param_1 + 0x16b8) =
               *(ushort *)(param_1 + 0x16b8) |
               *(short *)(param_2 + 0x404 + uVar7 * 4) << (bVar2 & 0x1f);
          *(uint *)(param_1 + 0x16bc) = *(int *)(param_1 + 0x16bc) + uVar9;
        }
        iVar5 = *(int *)(&DAT_0041cd58 + uVar7 * 4);
        if (iVar5 != 0) {
          iVar6 = uVar10 - *(int *)(&DAT_0041d69c + uVar7 * 4);
          iVar4 = *(int *)(param_1 + 0x16bc);
          if (0x10 - iVar5 < iVar4) {
            *(ushort *)(param_1 + 0x16b8) =
                 *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar6 << ((byte)iVar4 & 0x1f));
            *(undefined1 *)(*(int *)(param_1 + 0x14) + *(int *)(param_1 + 8)) =
                 *(undefined1 *)(param_1 + 0x16b8);
            iVar4 = *(int *)(param_1 + 0x14) + 1;
            *(int *)(param_1 + 0x14) = iVar4;
            *(undefined1 *)(*(int *)(param_1 + 8) + iVar4) = *(undefined1 *)(param_1 + 0x16b9);
            *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
            *(ushort *)(param_1 + 0x16b8) =
                 (ushort)iVar6 >> (0x10U - (char)*(int *)(param_1 + 0x16bc) & 0x1f);
            *(int *)(param_1 + 0x16bc) = *(int *)(param_1 + 0x16bc) + -0x10 + iVar5;
          }
          else {
            *(ushort *)(param_1 + 0x16b8) =
                 *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar6 << ((byte)iVar4 & 0x1f));
            *(int *)(param_1 + 0x16bc) = iVar4 + iVar5;
          }
        }
        uVar10 = uVar8 - 1;
        if (uVar10 < 0x100) {
          bVar2 = (&DAT_0041d39b)[uVar8];
        }
        else {
          bVar2 = (&DAT_0041d49c)[uVar10 >> 7];
        }
        uVar8 = (uint)bVar2;
        iVar5 = *(int *)(param_1 + 0x16bc);
        uVar7 = (uint)*(ushort *)(param_3 + 2 + uVar8 * 4);
        if ((int)(0x10 - uVar7) < iVar5) {
          uVar1 = *(ushort *)(param_3 + uVar8 * 4);
          *(ushort *)(param_1 + 0x16b8) =
               *(ushort *)(param_1 + 0x16b8) | uVar1 << ((byte)iVar5 & 0x1f);
          *(undefined1 *)(*(int *)(param_1 + 0x14) + *(int *)(param_1 + 8)) =
               *(undefined1 *)(param_1 + 0x16b8);
          iVar5 = *(int *)(param_1 + 0x14) + 1;
          *(int *)(param_1 + 0x14) = iVar5;
          *(undefined1 *)(*(int *)(param_1 + 8) + iVar5) = *(undefined1 *)(param_1 + 0x16b9);
          *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
          *(ushort *)(param_1 + 0x16b8) = uVar1 >> (0x10U - (char)*(int *)(param_1 + 0x16bc) & 0x1f)
          ;
          *(uint *)(param_1 + 0x16bc) = *(int *)(param_1 + 0x16bc) + -0x10 + uVar7;
        }
        else {
          *(ushort *)(param_1 + 0x16b8) =
               *(ushort *)(param_1 + 0x16b8) |
               *(short *)(param_3 + uVar8 * 4) << ((byte)iVar5 & 0x1f);
          *(uint *)(param_1 + 0x16bc) = iVar5 + uVar7;
        }
        iVar5 = *(int *)(&DAT_0041cdcc + uVar8 * 4);
        if (iVar5 != 0) {
          iVar6 = uVar10 - *(int *)(&DAT_0041d710 + uVar8 * 4);
          iVar4 = *(int *)(param_1 + 0x16bc);
          if (iVar4 <= 0x10 - iVar5) {
            *(ushort *)(param_1 + 0x16b8) =
                 *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar6 << ((byte)iVar4 & 0x1f));
            iVar4 = iVar4 + iVar5;
            goto LAB_0040d757;
          }
          *(ushort *)(param_1 + 0x16b8) =
               *(ushort *)(param_1 + 0x16b8) | (ushort)(iVar6 << ((byte)iVar4 & 0x1f));
          *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
               *(undefined1 *)(param_1 + 0x16b8);
          iVar4 = *(int *)(param_1 + 0x14) + 1;
          *(int *)(param_1 + 0x14) = iVar4;
          *(undefined1 *)(iVar4 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
          iVar4 = *(int *)(param_1 + 0x16bc);
          *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
          *(int *)(param_1 + 0x16bc) = iVar4 + -0x10 + iVar5;
          *(ushort *)(param_1 + 0x16b8) = (ushort)iVar6 >> (0x10U - (char)iVar4 & 0x1f);
        }
      }
    } while (uVar3 < *(uint *)(param_1 + 0x16a0));
  }
  iVar5 = *(int *)(param_1 + 0x16bc);
  uVar3 = (uint)*(ushort *)(param_2 + 0x402);
  if ((int)(0x10 - uVar3) < iVar5) {
    uVar1 = *(ushort *)(param_2 + 0x400);
    *(ushort *)(param_1 + 0x16b8) = *(ushort *)(param_1 + 0x16b8) | uVar1 << ((byte)iVar5 & 0x1f);
    *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
         *(undefined1 *)(param_1 + 0x16b8);
    iVar5 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar5;
    *(undefined1 *)(iVar5 + *(int *)(param_1 + 8)) = *(undefined1 *)(param_1 + 0x16b9);
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
    *(ushort *)(param_1 + 0x16b8) = uVar1 >> (0x10U - (char)*(int *)(param_1 + 0x16bc) & 0x1f);
    *(uint *)(param_1 + 0x16bc) = *(int *)(param_1 + 0x16bc) + -0x10 + uVar3;
  }
  else {
    *(ushort *)(param_1 + 0x16b8) =
         *(ushort *)(param_1 + 0x16b8) | *(short *)(param_2 + 0x400) << ((byte)iVar5 & 0x1f);
    *(uint *)(param_1 + 0x16bc) = iVar5 + uVar3;
  }
  *(uint *)(param_1 + 0x16b4) = (uint)*(ushort *)(param_2 + 0x402);
  return;
}



void __cdecl FUN_0040d820(int *param_1)

{
  int iVar1;
  int *piVar2;
  
  iVar1 = 0;
  piVar2 = param_1 + 0x25;
  do {
    if ((short)*piVar2 != 0) break;
    iVar1 = iVar1 + 1;
    piVar2 = piVar2 + 1;
  } while (iVar1 < 9);
  if (iVar1 == 9) {
    iVar1 = 0xe;
    piVar2 = param_1 + 0x33;
    do {
      if ((short)*piVar2 != 0) break;
      iVar1 = iVar1 + 1;
      piVar2 = piVar2 + 1;
    } while (iVar1 < 0x20);
  }
  *(uint *)(*param_1 + 0x2c) = (uint)(iVar1 == 0x20);
  return;
}



uint __cdecl FUN_0040d870(uint param_1,int param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  
  uVar1 = 0;
  do {
    uVar2 = uVar1;
    uVar3 = param_1 & 1;
    param_1 = param_1 >> 1;
    param_2 = param_2 + -1;
    uVar1 = (uVar2 | uVar3) << 1;
  } while (0 < param_2);
  return uVar2 & 0x7fffffff | uVar3;
}



void __cdecl FUN_0040d890(int param_1)

{
  int iVar1;
  
  if (*(int *)(param_1 + 0x16bc) == 0x10) {
    *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
         *(undefined1 *)(param_1 + 0x16b8);
    iVar1 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar1;
    *(undefined1 *)(*(int *)(param_1 + 8) + iVar1) = *(undefined1 *)(param_1 + 0x16b9);
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
    *(undefined2 *)(param_1 + 0x16b8) = 0;
    *(undefined4 *)(param_1 + 0x16bc) = 0;
    return;
  }
  if (7 < *(int *)(param_1 + 0x16bc)) {
    *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
         *(undefined1 *)(param_1 + 0x16b8);
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
    *(ushort *)(param_1 + 0x16b8) = (ushort)*(byte *)(param_1 + 0x16b9);
    *(int *)(param_1 + 0x16bc) = *(int *)(param_1 + 0x16bc) + -8;
  }
  return;
}



void __cdecl FUN_0040d920(int param_1)

{
  int iVar1;
  
  if (8 < *(int *)(param_1 + 0x16bc)) {
    *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
         *(undefined1 *)(param_1 + 0x16b8);
    iVar1 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar1;
    *(undefined1 *)(*(int *)(param_1 + 8) + iVar1) = *(undefined1 *)(param_1 + 0x16b9);
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
    *(undefined2 *)(param_1 + 0x16b8) = 0;
    *(undefined4 *)(param_1 + 0x16bc) = 0;
    return;
  }
  if (0 < *(int *)(param_1 + 0x16bc)) {
    *(undefined1 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0x14)) =
         *(undefined1 *)(param_1 + 0x16b8);
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
  }
  *(undefined2 *)(param_1 + 0x16b8) = 0;
  *(undefined4 *)(param_1 + 0x16bc) = 0;
  return;
}



void __cdecl FUN_0040d9a0(int param_1,undefined1 *param_2,int param_3,int param_4)

{
  byte bVar1;
  int iVar2;
  
  FUN_0040d920(param_1);
  *(undefined4 *)(param_1 + 0x16b4) = 8;
  if (param_4 != 0) {
    *(byte *)(*(int *)(param_1 + 0x14) + *(int *)(param_1 + 8)) = (byte)param_3;
    iVar2 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar2;
    bVar1 = (byte)((uint)param_3 >> 8);
    *(byte *)(iVar2 + *(int *)(param_1 + 8)) = bVar1;
    iVar2 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar2;
    *(byte *)(iVar2 + *(int *)(param_1 + 8)) = ~(byte)param_3;
    iVar2 = *(int *)(param_1 + 0x14) + 1;
    *(int *)(param_1 + 0x14) = iVar2;
    *(byte *)(iVar2 + *(int *)(param_1 + 8)) = ~bVar1;
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
  }
  for (; param_3 != 0; param_3 = param_3 + -1) {
    *(undefined1 *)(*(int *)(param_1 + 0x14) + *(int *)(param_1 + 8)) = *param_2;
    param_2 = param_2 + 1;
    *(int *)(param_1 + 0x14) = *(int *)(param_1 + 0x14) + 1;
  }
  return;
}



undefined1 * __cdecl FUN_0040da40(undefined1 *param_1,void *param_2,char *param_3)

{
  char cVar1;
  undefined *puVar2;
  undefined1 *puVar3;
  undefined1 *puVar4;
  int iVar5;
  void *this;
  uint uVar6;
  uint uVar7;
  undefined1 *puVar8;
  uint uVar9;
  undefined1 *this_00;
  char *pcVar10;
  undefined1 local_2c [4];
  undefined1 *local_28;
  undefined1 *local_24;
  undefined4 local_20;
  undefined1 local_1c [4];
  int local_18;
  void *local_c;
  undefined1 *puStack_8;
  uint local_4;
  
  puStack_8 = &LAB_00419550;
  local_c = ExceptionList;
  uVar9 = 0;
  local_2c[0] = param_3._0_1_;
  local_28 = (undefined1 *)0x0;
  local_24 = (undefined1 *)0x0;
  local_20 = 0;
  uVar6 = 0xffffffff;
  pcVar10 = param_3;
  do {
    if (uVar6 == 0) break;
    uVar6 = uVar6 - 1;
    cVar1 = *pcVar10;
    pcVar10 = pcVar10 + 1;
  } while (cVar1 != '\0');
  uVar7 = 0xffffffff;
  pcVar10 = param_3;
  do {
    if (uVar7 == 0) break;
    uVar7 = uVar7 - 1;
    cVar1 = *pcVar10;
    pcVar10 = pcVar10 + 1;
  } while (cVar1 != '\0');
  local_4 = 1;
  ExceptionList = &local_c;
  uVar7 = FUN_0040e0b0(param_2,param_3,0,~uVar7 - 1);
  do {
    if (uVar7 == 0xffffffff) {
      if (uVar9 < *(uint *)((int)param_2 + 8)) {
        puVar4 = FUN_004068d0(param_2,local_1c,uVar9,0xffffffff);
        local_4._0_1_ = 3;
        FUN_0040e450(local_2c,local_24,1,puVar4);
        local_4 = CONCAT31(local_4._1_3_,1);
        if (local_18 != 0) {
          cVar1 = *(char *)(local_18 + -1);
          if ((cVar1 == '\0') || (cVar1 == -1)) {
            FUN_0040ef10((char *)(local_18 + -1));
          }
          else {
            *(char *)(local_18 + -1) = cVar1 + -1;
          }
        }
      }
      *param_1 = local_2c[0];
      if (local_28 == (undefined1 *)0x0) {
        iVar5 = 0;
      }
      else {
        iVar5 = (int)local_24 - (int)local_28 >> 4;
      }
      if (iVar5 < 0) {
        iVar5 = 0;
      }
      this = operator_new(iVar5 << 4);
      puVar3 = local_24;
      *(void **)(param_1 + 4) = this;
      this_00 = local_28;
      puVar8 = local_24;
      for (puVar4 = local_28; puVar4 != puVar3; puVar4 = puVar4 + 0x10) {
        local_4._1_3_ = (undefined3)(local_4 >> 8);
        local_4 = CONCAT31(local_4._1_3_,4);
        if (this != (void *)0x0) {
          FUN_0040df30(this,puVar4);
          this_00 = local_28;
          puVar8 = local_24;
        }
        this = (void *)((int)this + 0x10);
      }
      *(void **)(param_1 + 8) = this;
      *(void **)(param_1 + 0xc) = this;
      local_4 = local_4 & 0xffffff00;
      puVar2 = this_00;
      for (; this_00 != puVar8; this_00 = this_00 + 0x10) {
        FUN_00401380(this_00,'\x01');
        puVar2 = local_28;
      }
      FUN_0040ef10(puVar2);
      ExceptionList = local_c;
      return param_1;
    }
    puVar4 = FUN_004068d0(param_2,local_1c,uVar9,uVar7 - uVar9);
    local_4._0_1_ = 2;
    FUN_0040e450(local_2c,local_24,1,puVar4);
    local_4 = CONCAT31(local_4._1_3_,1);
    FUN_00401380(local_1c,'\x01');
    uVar9 = uVar7 + (~uVar6 - 1);
    uVar7 = 0xffffffff;
    pcVar10 = param_3;
    do {
      if (uVar7 == 0) break;
      uVar7 = uVar7 - 1;
      cVar1 = *pcVar10;
      pcVar10 = pcVar10 + 1;
    } while (cVar1 != '\0');
    uVar7 = FUN_0040e0b0(param_2,param_3,uVar9,~uVar7 - 1);
  } while( true );
}



undefined1 * __cdecl FUN_0040dc30(undefined1 *param_1,void *param_2,char *param_3,char *param_4)

{
  char cVar1;
  int iVar2;
  undefined1 *puVar3;
  undefined1 *puVar4;
  int *piVar5;
  uint uVar6;
  uint uVar7;
  char *pcVar8;
  undefined *this;
  undefined1 local_d1;
  uint local_d0;
  int local_cc;
  undefined1 local_c8;
  undefined1 local_c7;
  undefined1 local_c0;
  undefined4 local_b8;
  undefined4 local_b4;
  undefined1 local_b0 [4];
  undefined1 local_ac [4];
  undefined *local_a8;
  undefined *local_a4;
  undefined1 local_9c [16];
  undefined1 local_8c [16];
  undefined1 local_7c [16];
  undefined1 local_6c [16];
  undefined4 local_5c [2];
  undefined1 local_54 [8];
  undefined1 local_4c [32];
  undefined1 local_2c [32];
  void *local_c;
  undefined1 *puStack_8;
  int local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004195c6;
  local_c = ExceptionList;
  local_b8 = 0;
  ExceptionList = &local_c;
  FUN_0040da40(local_ac,param_2,param_3);
  local_c7 = local_d1;
  local_4 = 1;
  local_c8 = local_d1;
  local_c0 = 0;
  FUN_00403350((int)&local_c8);
  uVar6 = 0xffffffff;
  pcVar8 = param_4;
  do {
    if (uVar6 == 0) break;
    uVar6 = uVar6 - 1;
    cVar1 = *pcVar8;
    pcVar8 = pcVar8 + 1;
  } while (cVar1 != '\0');
  local_4._0_1_ = 2;
  local_d0 = 0;
  local_cc = 0;
  do {
    if ((local_a8 == (undefined *)0x0) || ((uint)((int)local_a4 - (int)local_a8 >> 4) <= local_d0))
    {
      param_1[1] = local_c7;
      *param_1 = local_c8;
      param_1[8] = local_c0;
      FUN_00403350((int)param_1);
      FUN_0040e240(param_1,(int)&local_c8);
      local_b8 = 1;
      local_4._0_1_ = 1;
      FUN_00402ba0(&local_c8);
      local_4 = (uint)local_4._1_3_ << 8;
      for (this = local_a8; this != local_a4; this = this + 0x10) {
        FUN_00401380(this,'\x01');
      }
      FUN_0040ef10(local_a8);
      ExceptionList = local_c;
      return param_1;
    }
    puVar4 = local_a8 + local_cc;
    uVar7 = 0xffffffff;
    pcVar8 = param_4;
    do {
      if (uVar7 == 0) break;
      uVar7 = uVar7 - 1;
      cVar1 = *pcVar8;
      pcVar8 = pcVar8 + 1;
    } while (cVar1 != '\0');
    uVar7 = FUN_0040e0b0(puVar4,param_4,0,~uVar7 - 1);
    if (uVar7 == 0xffffffff) {
      FUN_00401380(local_8c,'\0');
      local_4._0_1_ = 7;
      piVar5 = (int *)FUN_0040e330(local_2c,puVar4,local_8c);
      local_4._0_1_ = 8;
      FUN_0040e710(&local_c8,&local_b4,piVar5);
      FUN_0040e970(local_54,&local_b4,local_b0);
      local_4._0_1_ = 7;
      FUN_00403730((int)local_2c);
      local_4._0_1_ = 2;
      FUN_00401380(local_8c,'\x01');
    }
    else {
      puVar3 = FUN_004068d0(puVar4,local_6c,uVar7 + (~uVar6 - 1),0xffffffff);
      local_4._0_1_ = 3;
      puVar4 = FUN_004068d0(puVar4,local_7c,0,uVar7);
      local_4._0_1_ = 4;
      FUN_00401380(local_9c,'\0');
      local_4._0_1_ = 5;
      piVar5 = (int *)FUN_0040e330(local_4c,puVar4,local_9c);
      local_4._0_1_ = 6;
      piVar5 = (int *)FUN_0040e140(&local_c8,local_5c,piVar5);
      iVar2 = *piVar5;
      local_4._0_1_ = 5;
      FUN_00403730((int)local_4c);
      local_4._0_1_ = 4;
      FUN_00401380(local_9c,'\x01');
      FUN_00401900((void *)(iVar2 + 0x1c),puVar3,0,0xffffffff);
      local_4._0_1_ = 3;
      FUN_00401380(local_7c,'\x01');
      local_4._0_1_ = 2;
      FUN_00401380(local_6c,'\x01');
    }
    local_d0 = local_d0 + 1;
    local_cc = local_cc + 0x10;
  } while( true );
}



// WARNING: Removing unreachable block (ram,0x0040df57)

undefined1 * __thiscall FUN_0040df30(void *this,undefined1 *param_1)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  undefined4 *puVar7;
  
  *(undefined1 *)this = *param_1;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  uVar5 = *(uint *)(param_1 + 8);
  if (this == param_1) {
    if (uVar5 != 0) {
      FUN_0040f147();
    }
    FUN_00401b10(this);
    iVar1 = *(int *)((int)this + 8) - uVar5;
    iVar6 = -1;
    if (iVar1 != -1) {
      iVar6 = iVar1;
    }
    if (iVar6 != 0) {
      puVar3 = (undefined4 *)(*(int *)((int)this + 4) + uVar5);
      FUN_0040f710(puVar3,(undefined4 *)((int)puVar3 + iVar6),iVar1 - iVar6);
      uVar5 = *(int *)((int)this + 8) - iVar6;
      uVar2 = FUN_00401570(this,uVar5,'\0');
      if ((char)uVar2 != '\0') {
        FUN_00401550(this,uVar5);
      }
    }
    FUN_00401b10(this);
    return this;
  }
  if (uVar5 != 0) {
    puVar3 = *(undefined4 **)(param_1 + 4);
    if (puVar3 == (undefined4 *)0x0) {
      puVar3 = &DAT_0041a214;
    }
    if (*(byte *)((int)puVar3 + -1) < 0xfe) {
      *(undefined4 *)((int)this + 4) = 0;
      *(undefined4 *)((int)this + 8) = 0;
      *(undefined4 *)((int)this + 0xc) = 0;
      puVar3 = *(undefined4 **)(param_1 + 4);
      if (puVar3 == (undefined4 *)0x0) {
        puVar3 = &DAT_0041a214;
      }
      *(undefined4 **)((int)this + 4) = puVar3;
      *(undefined4 *)((int)this + 8) = *(undefined4 *)(param_1 + 8);
      *(undefined4 *)((int)this + 0xc) = *(undefined4 *)(param_1 + 0xc);
      *(char *)((int)puVar3 + -1) = *(char *)((int)puVar3 + -1) + '\x01';
      return this;
    }
  }
  uVar2 = FUN_00401570(this,uVar5,'\x01');
  if ((char)uVar2 != '\0') {
    puVar3 = *(undefined4 **)(param_1 + 4);
    if (puVar3 == (undefined4 *)0x0) {
      puVar3 = &DAT_0041a214;
    }
    puVar7 = *(undefined4 **)((int)this + 4);
    for (uVar4 = uVar5 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
      *puVar7 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar7 = puVar7 + 1;
    }
    for (uVar4 = uVar5 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
      *(undefined1 *)puVar7 = *(undefined1 *)puVar3;
      puVar3 = (undefined4 *)((int)puVar3 + 1);
      puVar7 = (undefined4 *)((int)puVar7 + 1);
    }
    FUN_00401550(this,uVar5);
  }
  return this;
}



void __fastcall FUN_0040e050(int param_1)

{
  char cVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar2 = *(int *)(param_1 + 8);
  for (iVar4 = *(int *)(param_1 + 4); iVar4 != iVar2; iVar4 = iVar4 + 0x10) {
    iVar3 = *(int *)(iVar4 + 4);
    if (iVar3 != 0) {
      cVar1 = *(char *)(iVar3 + -1);
      if ((cVar1 == '\0') || (cVar1 == -1)) {
        FUN_0040ef10((char *)(iVar3 + -1));
      }
      else {
        *(char *)(iVar3 + -1) = cVar1 + -1;
      }
    }
    *(undefined4 *)(iVar4 + 4) = 0;
    *(undefined4 *)(iVar4 + 8) = 0;
    *(undefined4 *)(iVar4 + 0xc) = 0;
  }
  FUN_0040ef10(*(undefined **)(param_1 + 4));
  *(undefined4 *)(param_1 + 4) = 0;
  *(undefined4 *)(param_1 + 8) = 0;
  *(undefined4 *)(param_1 + 0xc) = 0;
  return;
}



uint __thiscall FUN_0040e0b0(void *this,char *param_1,uint param_2,uint param_3)

{
  char *pcVar1;
  char *_Buf;
  uint uVar2;
  char *_MaxCount;
  char *pcVar3;
  char *pcVar4;
  bool bVar5;
  
  if ((param_3 != 0) || (*(uint *)((int)this + 8) < param_2)) {
    if ((param_2 < *(uint *)((int)this + 8)) &&
       (uVar2 = *(uint *)((int)this + 8) - param_2, param_3 <= uVar2)) {
      _MaxCount = (char *)(uVar2 + (1 - param_3));
      _Buf = (char *)(*(int *)((int)this + 4) + param_2);
      while( true ) {
        pcVar1 = _memchr(_Buf,(int)*param_1,(size_t)_MaxCount);
        if (pcVar1 == (char *)0x0) break;
        bVar5 = true;
        uVar2 = param_3;
        pcVar3 = pcVar1;
        pcVar4 = param_1;
        do {
          if (uVar2 == 0) break;
          uVar2 = uVar2 - 1;
          bVar5 = *pcVar3 == *pcVar4;
          pcVar3 = pcVar3 + 1;
          pcVar4 = pcVar4 + 1;
        } while (bVar5);
        if (bVar5) {
          return (int)pcVar1 - *(int *)((int)this + 4);
        }
        _MaxCount = _Buf + (int)(_MaxCount + (-1 - (int)pcVar1));
        _Buf = pcVar1 + 1;
      }
    }
    param_2 = 0xffffffff;
  }
  return param_2;
}



void __thiscall FUN_0040e140(void *this,undefined4 *param_1,int *param_2)

{
  int *this_00;
  uint uVar1;
  undefined4 *puVar2;
  int *piVar3;
  int *piVar4;
  bool bVar5;
  void *local_4;
  
  this_00 = param_2;
  piVar4 = *(int **)((int)this + 4);
  bVar5 = true;
  piVar3 = (int *)piVar4[1];
  local_4 = this;
  if (piVar3 != DAT_004239a4) {
    do {
      piVar4 = piVar3;
      uVar1 = FUN_0040ee90(this_00,(int)(piVar4 + 3));
      bVar5 = (int)uVar1 < 0;
      if (bVar5) {
        piVar3 = (int *)*piVar4;
      }
      else {
        piVar3 = (int *)piVar4[2];
      }
    } while (piVar3 != DAT_004239a4);
  }
  if (*(char *)((int)this + 8) != '\0') {
    puVar2 = (undefined4 *)FUN_0040e990(this,&local_4,(int)piVar3,piVar4,(undefined1 *)this_00);
    *param_1 = *puVar2;
    *(undefined1 *)(param_1 + 1) = 1;
    return;
  }
  param_2 = piVar4;
  if (bVar5) {
    if (piVar4 == (int *)**(int **)((int)this + 4)) {
      puVar2 = (undefined4 *)FUN_0040e990(this,&local_4,(int)piVar3,piVar4,(undefined1 *)this_00);
      *param_1 = *puVar2;
      *(undefined1 *)(param_1 + 1) = 1;
      return;
    }
    FUN_0040ecc0((int *)&param_2);
  }
  uVar1 = FUN_0040ee90(param_2 + 3,(int)this_00);
  if ((int)uVar1 < 0) {
    puVar2 = (undefined4 *)FUN_0040e990(this,&local_4,(int)piVar3,piVar4,(undefined1 *)this_00);
    *param_1 = *puVar2;
    *(undefined1 *)(param_1 + 1) = 1;
    return;
  }
  *param_1 = param_2;
  *(undefined1 *)(param_1 + 1) = 0;
  return;
}



void __thiscall FUN_0040e240(void *this,int param_1)

{
  int iVar1;
  undefined4 *puVar2;
  int *piVar3;
  int *piVar4;
  int *piVar5;
  int *piVar6;
  int *local_8;
  
  piVar3 = *(int **)((int)this + 4);
  piVar6 = *(int **)(*(int *)(param_1 + 4) + 4);
  piVar5 = piVar3;
  local_8 = piVar6;
  if (piVar6 != DAT_004239a4) {
    do {
      iVar1 = piVar6[0xb];
      piVar3 = operator_new(0x30);
      piVar3[0xb] = iVar1;
      piVar3[1] = (int)piVar5;
      if (local_8 == piVar6) {
        local_8 = piVar3;
      }
      piVar4 = FUN_004033f0((int *)piVar6[2],piVar3);
      piVar3[2] = (int)piVar4;
      FUN_004034a0((undefined1 *)(piVar3 + 3),(undefined1 *)(piVar6 + 3));
      *piVar5 = (int)piVar3;
      piVar6 = (int *)*piVar6;
      piVar5 = piVar3;
    } while (piVar6 != DAT_004239a4);
  }
  *piVar3 = (int)DAT_004239a4;
  *(int **)(*(int *)((int)this + 4) + 4) = local_8;
  puVar2 = *(undefined4 **)((int)this + 4);
  *(undefined4 *)((int)this + 0xc) = *(undefined4 *)(param_1 + 0xc);
  piVar3 = (int *)puVar2[1];
  if (piVar3 == DAT_004239a4) {
    *puVar2 = puVar2;
    *(int *)(*(int *)((int)this + 4) + 8) = *(int *)((int)this + 4);
    return;
  }
  for (piVar6 = (int *)*piVar3; piVar6 != DAT_004239a4; piVar6 = (int *)*piVar6) {
    piVar3 = piVar6;
  }
  *puVar2 = piVar3;
  piVar3 = *(int **)(*(int *)((int)this + 4) + 4);
  for (piVar6 = (int *)piVar3[2]; piVar6 != DAT_004239a4; piVar6 = (int *)piVar6[2]) {
    piVar3 = piVar6;
  }
  *(int **)(*(int *)((int)this + 4) + 8) = piVar3;
  return;
}



// WARNING: Removing unreachable block (ram,0x0040e371)

undefined1 * __thiscall FUN_0040e330(void *this,undefined1 *param_1,undefined1 *param_2)

{
  undefined1 uVar1;
  uint uVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  uint uVar5;
  undefined4 *puVar6;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004195d8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined1 *)this = *param_1;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  uVar2 = *(uint *)(param_1 + 8);
  if (this == param_1) {
    FUN_004035f0(this,uVar2,0xffffffff);
    FUN_004035f0(this,0,0);
  }
  else {
    if (uVar2 != 0) {
      puVar3 = *(undefined4 **)(param_1 + 4);
      if (puVar3 == (undefined4 *)0x0) {
        puVar3 = &DAT_0041a214;
      }
      if (*(byte *)((int)puVar3 + -1) < 0xfe) {
        FUN_00401380(this,'\x01');
        puVar3 = *(undefined4 **)(param_1 + 4);
        if (puVar3 == (undefined4 *)0x0) {
          puVar3 = &DAT_0041a214;
        }
        *(undefined4 **)((int)this + 4) = puVar3;
        *(undefined4 *)((int)this + 8) = *(undefined4 *)(param_1 + 8);
        *(undefined4 *)((int)this + 0xc) = *(undefined4 *)(param_1 + 0xc);
        *(char *)((int)puVar3 + -1) = *(char *)((int)puVar3 + -1) + '\x01';
        goto LAB_0040e40a;
      }
    }
    uVar4 = FUN_00401570(this,uVar2,'\x01');
    if ((char)uVar4 != '\0') {
      puVar3 = *(undefined4 **)(param_1 + 4);
      if (puVar3 == (undefined4 *)0x0) {
        puVar3 = &DAT_0041a214;
      }
      puVar6 = *(undefined4 **)((int)this + 4);
      for (uVar5 = uVar2 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *puVar6 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar6 = puVar6 + 1;
      }
      for (uVar5 = uVar2 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined1 *)puVar6 = *(undefined1 *)puVar3;
        puVar3 = (undefined4 *)((int)puVar3 + 1);
        puVar6 = (undefined4 *)((int)puVar6 + 1);
      }
      FUN_00401550(this,uVar2);
    }
  }
LAB_0040e40a:
  local_4 = 0;
  uVar1 = *param_2;
  *(undefined4 *)((int)this + 0x14) = 0;
  *(undefined1 *)((int)this + 0x10) = uVar1;
  *(undefined4 *)((int)this + 0x18) = 0;
  *(undefined4 *)((int)this + 0x1c) = 0;
  FUN_00401900((undefined1 *)((int)this + 0x10),param_2,0,0xffffffff);
  ExceptionList = local_c;
  return this;
}



void __thiscall FUN_0040e450(void *this,undefined1 *param_1,uint param_2,undefined1 *param_3)

{
  undefined1 *puVar1;
  uint uVar2;
  int iVar3;
  undefined1 *puVar4;
  undefined1 *puVar5;
  int iVar6;
  undefined1 *puVar7;
  void *this_00;
  void *pvVar8;
  
  puVar5 = *(undefined1 **)((int)this + 8);
  if ((uint)(*(int *)((int)this + 0xc) - (int)puVar5 >> 4) < param_2) {
    iVar6 = *(int *)((int)this + 4);
    if ((iVar6 == 0) || (uVar2 = (int)puVar5 - iVar6 >> 4, uVar2 <= param_2)) {
      uVar2 = param_2;
    }
    if (iVar6 == 0) {
      iVar6 = 0;
    }
    else {
      iVar6 = (int)puVar5 - iVar6 >> 4;
    }
    iVar6 = uVar2 + iVar6;
    iVar3 = iVar6;
    if (iVar6 < 0) {
      iVar3 = 0;
    }
    puVar4 = operator_new(iVar3 << 4);
    puVar5 = puVar4;
    for (puVar7 = *(undefined1 **)((int)this + 4); uVar2 = param_2, puVar1 = puVar5,
        puVar7 != param_1; puVar7 = puVar7 + 0x10) {
      FUN_0040ed20(puVar5,puVar7);
      puVar5 = puVar5 + 0x10;
    }
    for (; uVar2 != 0; uVar2 = uVar2 - 1) {
      FUN_0040ed20(puVar1,param_3);
      puVar1 = puVar1 + 0x10;
    }
    puVar5 = puVar5 + param_2 * 0x10;
    puVar7 = *(undefined1 **)((int)this + 8);
    for (; param_1 != puVar7; param_1 = param_1 + 0x10) {
      FUN_0040ed20(puVar5,param_1);
      puVar5 = puVar5 + 0x10;
    }
    pvVar8 = *(void **)((int)this + 8);
    for (this_00 = *(void **)((int)this + 4); this_00 != pvVar8;
        this_00 = (void *)((int)this_00 + 0x10)) {
      FUN_00401380(this_00,'\x01');
    }
    FUN_0040ef10(*(undefined **)((int)this + 4));
    iVar3 = *(int *)((int)this + 4);
    *(undefined1 **)((int)this + 0xc) = puVar4 + iVar6 * 0x10;
    if (iVar3 == 0) {
      *(undefined1 **)((int)this + 4) = puVar4;
      *(undefined1 **)((int)this + 8) = puVar4 + param_2 * 0x10;
      return;
    }
    *(undefined1 **)((int)this + 4) = puVar4;
    *(undefined1 **)((int)this + 8) =
         puVar4 + ((*(int *)((int)this + 8) - iVar3 >> 4) + param_2) * 0x10;
    return;
  }
  if ((uint)((int)puVar5 - (int)param_1 >> 4) < param_2) {
    puVar7 = param_1 + param_2 * 0x10;
    for (puVar4 = param_1; puVar4 != puVar5; puVar4 = puVar4 + 0x10) {
      FUN_0040ed20(puVar7,puVar4);
      puVar7 = puVar7 + 0x10;
    }
    puVar5 = *(undefined1 **)((int)this + 8);
    for (iVar6 = param_2 - ((int)puVar5 - (int)param_1 >> 4); iVar6 != 0; iVar6 = iVar6 + -1) {
      FUN_0040ed20(puVar5,param_3);
      puVar5 = puVar5 + 0x10;
    }
    puVar5 = *(undefined1 **)((int)this + 8);
    if (param_1 != puVar5) {
      do {
        FUN_00401900(param_1,param_3,0,0xffffffff);
        param_1 = param_1 + 0x10;
      } while (param_1 != puVar5);
      *(uint *)((int)this + 8) = *(int *)((int)this + 8) + param_2 * 0x10;
      return;
    }
  }
  else {
    if (param_2 == 0) {
      return;
    }
    puVar7 = puVar5;
    for (puVar4 = puVar5 + param_2 * -0x10; puVar4 != puVar5; puVar4 = puVar4 + 0x10) {
      FUN_0040ed20(puVar7,puVar4);
      puVar7 = puVar7 + 0x10;
    }
    pvVar8 = *(void **)((int)this + 8);
    puVar5 = (undefined1 *)((int)pvVar8 + param_2 * -0x10);
    while (param_1 != puVar5) {
      puVar5 = puVar5 + -0x10;
      pvVar8 = (void *)((int)pvVar8 + -0x10);
      FUN_00401900(pvVar8,puVar5,0,0xffffffff);
    }
    puVar5 = param_1 + param_2 * 0x10;
    for (; param_1 != puVar5; param_1 = param_1 + 0x10) {
      FUN_00401900(param_1,param_3,0,0xffffffff);
    }
  }
  *(int *)((int)this + 8) = *(int *)((int)this + 8) + param_2 * 0x10;
  return;
}



undefined4 * __thiscall FUN_0040e710(void *this,undefined4 *param_1,int *param_2)

{
  int *piVar1;
  int *piVar2;
  int iVar3;
  byte *pbVar4;
  uint uVar5;
  undefined4 *puVar6;
  int *piVar7;
  int *piVar8;
  bool bVar9;
  int *local_4;
  
  piVar1 = param_2;
  bVar9 = true;
  piVar7 = *(int **)((int)this + 4);
  piVar8 = (int *)piVar7[1];
  local_4 = this;
  if (piVar8 != DAT_004239a4) {
    do {
      piVar7 = piVar8;
      pbVar4 = (byte *)piVar7[4];
      if (pbVar4 == (byte *)0x0) {
        pbVar4 = (byte *)&DAT_0041a214;
      }
      uVar5 = FUN_004070e0(piVar1,0,piVar1[2],pbVar4,piVar7[5]);
      bVar9 = (int)uVar5 < 0;
      if (bVar9) {
        piVar8 = (int *)*piVar7;
      }
      else {
        piVar8 = (int *)piVar7[2];
      }
    } while (piVar8 != DAT_004239a4);
  }
  if (*(char *)((int)this + 8) != '\0') {
    param_2 = (int *)FUN_00403480(piVar7,0);
    *param_2 = (int)DAT_004239a4;
    param_2[2] = (int)DAT_004239a4;
    FUN_004034a0((undefined1 *)(param_2 + 3),(undefined1 *)piVar1);
    *(int *)((int)this + 0xc) = *(int *)((int)this + 0xc) + 1;
    if (((piVar7 == *(int **)((int)this + 4)) || (piVar8 != DAT_004239a4)) ||
       (uVar5 = FUN_0040ee90(piVar1,(int)(piVar7 + 3)), (int)uVar5 < 0)) {
      *piVar7 = (int)param_2;
      piVar8 = *(int **)((int)this + 4);
      if (piVar7 == piVar8) {
        piVar8[1] = (int)param_2;
        *(int **)(*(int *)((int)this + 4) + 8) = param_2;
      }
      else if (piVar7 == (int *)*piVar8) {
        *piVar8 = (int)param_2;
      }
    }
    else {
      piVar7[2] = (int)param_2;
      if (piVar7 == *(int **)(*(int *)((int)this + 4) + 8)) {
        *(int **)(*(int *)((int)this + 4) + 8) = param_2;
      }
    }
    piVar8 = param_2;
    piVar7 = param_2;
    if (param_2 != *(int **)(*(int *)((int)this + 4) + 4)) {
      do {
        piVar1 = (int *)piVar7[1];
        if (piVar1[0xb] != 0) break;
        piVar2 = *(int **)piVar1[1];
        if (piVar1 == piVar2) {
          iVar3 = ((undefined4 *)piVar1[1])[2];
          if (*(int *)(iVar3 + 0x2c) == 0) {
            piVar1[0xb] = 1;
            *(undefined4 *)(iVar3 + 0x2c) = 1;
            *(undefined4 *)(*(int *)(piVar7[1] + 4) + 0x2c) = 0;
            piVar7 = *(int **)(piVar7[1] + 4);
          }
          else {
            if (piVar7 == (int *)piVar1[2]) {
              FUN_0040ec00(this,(int)piVar1);
              piVar7 = piVar1;
            }
            *(undefined4 *)(piVar7[1] + 0x2c) = 1;
            *(undefined4 *)(*(int *)(piVar7[1] + 4) + 0x2c) = 0;
            FUN_0040ec60(this,*(int **)(piVar7[1] + 4));
          }
        }
        else if (piVar2[0xb] == 0) {
          piVar1[0xb] = 1;
          piVar2[0xb] = 1;
          *(undefined4 *)(*(int *)(piVar7[1] + 4) + 0x2c) = 0;
          piVar7 = *(int **)(piVar7[1] + 4);
        }
        else {
          if (piVar7 == (int *)*piVar1) {
            FUN_0040ec60(this,piVar1);
            piVar7 = piVar1;
          }
          *(undefined4 *)(piVar7[1] + 0x2c) = 1;
          *(undefined4 *)(*(int *)(piVar7[1] + 4) + 0x2c) = 0;
          FUN_0040ec00(this,*(int *)(piVar7[1] + 4));
        }
      } while (piVar7 != *(int **)(*(int *)((int)this + 4) + 4));
    }
    *(undefined4 *)(*(int *)(*(int *)((int)this + 4) + 4) + 0x2c) = 1;
    *param_1 = piVar8;
    *(undefined1 *)(param_1 + 1) = 1;
    return param_1;
  }
  local_4 = piVar7;
  if (bVar9) {
    if (piVar7 == (int *)**(int **)((int)this + 4)) goto LAB_0040e930;
    FUN_0040ecc0((int *)&local_4);
  }
  uVar5 = FUN_0040ee90(local_4 + 3,(int)piVar1);
  if (-1 < (int)uVar5) {
    param_2 = (int *)((uint)param_2 & 0xffffff00);
    FUN_0040e970(param_1,&local_4,(undefined1 *)&param_2);
    return param_1;
  }
LAB_0040e930:
  puVar6 = (undefined4 *)FUN_0040e990(this,&param_2,(int)piVar8,piVar7,(undefined1 *)piVar1);
  *param_1 = *puVar6;
  *(undefined1 *)(param_1 + 1) = 1;
  return param_1;
}



void __thiscall FUN_0040e970(void *this,undefined4 *param_1,undefined1 *param_2)

{
  *(undefined4 *)this = *param_1;
  *(undefined1 *)((int)this + 4) = *param_2;
  return;
}



void __thiscall
FUN_0040e990(void *this,undefined4 *param_1,int param_2,int *param_3,undefined1 *param_4)

{
  int iVar1;
  int *piVar2;
  int *piVar3;
  uint uVar4;
  int *piVar5;
  int *piVar6;
  int *piVar7;
  
  piVar3 = operator_new(0x30);
  piVar3[1] = (int)param_3;
  piVar3[0xb] = 0;
  *piVar3 = DAT_004239a4;
  piVar3[2] = DAT_004239a4;
  FUN_004034a0((undefined1 *)(piVar3 + 3),param_4);
  *(int *)((int)this + 0xc) = *(int *)((int)this + 0xc) + 1;
  if ((param_3 != *(int **)((int)this + 4)) && (param_2 == DAT_004239a4)) {
    uVar4 = FUN_0040ee90(param_4,(int)(param_3 + 3));
    if (-1 < (int)uVar4) {
      param_3[2] = (int)piVar3;
      if (param_3 == *(int **)(*(int *)((int)this + 4) + 8)) {
        *(int **)(*(int *)((int)this + 4) + 8) = piVar3;
      }
      goto LAB_0040ea24;
    }
  }
  *param_3 = (int)piVar3;
  piVar5 = *(int **)((int)this + 4);
  if (param_3 == piVar5) {
    piVar5[1] = (int)piVar3;
    *(int **)(*(int *)((int)this + 4) + 8) = piVar3;
  }
  else if (param_3 == (int *)*piVar5) {
    *piVar5 = (int)piVar3;
  }
LAB_0040ea24:
  piVar5 = piVar3;
  if (piVar3 != *(int **)(*(int *)((int)this + 4) + 4)) {
    do {
      piVar6 = (int *)piVar5[1];
      if (piVar6[0xb] != 0) break;
      piVar7 = *(int **)piVar6[1];
      if (piVar6 == piVar7) {
        iVar1 = ((undefined4 *)piVar6[1])[2];
        if (*(int *)(iVar1 + 0x2c) == 0) {
          piVar6[0xb] = 1;
          *(undefined4 *)(iVar1 + 0x2c) = 1;
          *(undefined4 *)(*(int *)(piVar5[1] + 4) + 0x2c) = 0;
          piVar5 = *(int **)(piVar5[1] + 4);
        }
        else {
          if (piVar5 == (int *)piVar6[2]) {
            piVar5 = (int *)piVar6[2];
            piVar6[2] = *piVar5;
            if (*piVar5 != DAT_004239a4) {
              *(int **)(*piVar5 + 4) = piVar6;
            }
            piVar5[1] = piVar6[1];
            if (piVar6 == *(int **)(*(int *)((int)this + 4) + 4)) {
              *(int **)(*(int *)((int)this + 4) + 4) = piVar5;
            }
            else {
              piVar7 = (int *)piVar6[1];
              if (piVar6 == (int *)*piVar7) {
                *piVar7 = (int)piVar5;
              }
              else {
                piVar7[2] = (int)piVar5;
              }
            }
            *piVar5 = (int)piVar6;
            piVar6[1] = (int)piVar5;
            piVar5 = piVar6;
          }
          *(undefined4 *)(piVar5[1] + 0x2c) = 1;
          *(undefined4 *)(*(int *)(piVar5[1] + 4) + 0x2c) = 0;
          piVar6 = *(int **)(piVar5[1] + 4);
          piVar7 = (int *)*piVar6;
          *piVar6 = piVar7[2];
          if (piVar7[2] != DAT_004239a4) {
            *(int **)(piVar7[2] + 4) = piVar6;
          }
          piVar7[1] = piVar6[1];
          if (piVar6 == *(int **)(*(int *)((int)this + 4) + 4)) {
            *(int **)(*(int *)((int)this + 4) + 4) = piVar7;
            piVar7[2] = (int)piVar6;
          }
          else {
            piVar2 = (int *)piVar6[1];
            if (piVar6 == (int *)piVar2[2]) {
              piVar2[2] = (int)piVar7;
              piVar7[2] = (int)piVar6;
            }
            else {
              *piVar2 = (int)piVar7;
              piVar7[2] = (int)piVar6;
            }
          }
LAB_0040ebd2:
          piVar6[1] = (int)piVar7;
        }
      }
      else {
        if (piVar7[0xb] != 0) {
          if (piVar5 == (int *)*piVar6) {
            iVar1 = *piVar6;
            *piVar6 = *(int *)(iVar1 + 8);
            if (*(int *)(iVar1 + 8) != DAT_004239a4) {
              *(int **)(*(int *)(iVar1 + 8) + 4) = piVar6;
            }
            *(int *)(iVar1 + 4) = piVar6[1];
            if (piVar6 == *(int **)(*(int *)((int)this + 4) + 4)) {
              *(int *)(*(int *)((int)this + 4) + 4) = iVar1;
            }
            else {
              piVar5 = (int *)piVar6[1];
              if (piVar6 == (int *)piVar5[2]) {
                piVar5[2] = iVar1;
              }
              else {
                *piVar5 = iVar1;
              }
            }
            *(int **)(iVar1 + 8) = piVar6;
            piVar6[1] = iVar1;
            piVar5 = piVar6;
          }
          *(undefined4 *)(piVar5[1] + 0x2c) = 1;
          *(undefined4 *)(*(int *)(piVar5[1] + 4) + 0x2c) = 0;
          piVar6 = *(int **)(piVar5[1] + 4);
          piVar7 = (int *)piVar6[2];
          piVar6[2] = *piVar7;
          if (*piVar7 != DAT_004239a4) {
            *(int **)(*piVar7 + 4) = piVar6;
          }
          piVar7[1] = piVar6[1];
          if (piVar6 == *(int **)(*(int *)((int)this + 4) + 4)) {
            *(int **)(*(int *)((int)this + 4) + 4) = piVar7;
          }
          else {
            piVar2 = (int *)piVar6[1];
            if (piVar6 == (int *)*piVar2) {
              *piVar2 = (int)piVar7;
            }
            else {
              piVar2[2] = (int)piVar7;
            }
          }
          *piVar7 = (int)piVar6;
          goto LAB_0040ebd2;
        }
        piVar6[0xb] = 1;
        piVar7[0xb] = 1;
        *(undefined4 *)(*(int *)(piVar5[1] + 4) + 0x2c) = 0;
        piVar5 = *(int **)(piVar5[1] + 4);
      }
    } while (piVar5 != *(int **)(*(int *)((int)this + 4) + 4));
  }
  *(undefined4 *)(*(int *)(*(int *)((int)this + 4) + 4) + 0x2c) = 1;
  *param_1 = piVar3;
  return;
}



void __thiscall FUN_0040ec00(void *this,int param_1)

{
  int *piVar1;
  int *piVar2;
  
  piVar1 = *(int **)(param_1 + 8);
  *(int *)(param_1 + 8) = *piVar1;
  if (*piVar1 != DAT_004239a4) {
    *(int *)(*piVar1 + 4) = param_1;
  }
  piVar1[1] = *(int *)(param_1 + 4);
  if (param_1 == *(int *)(*(int *)((int)this + 4) + 4)) {
    *(int **)(*(int *)((int)this + 4) + 4) = piVar1;
    *piVar1 = param_1;
    *(int **)(param_1 + 4) = piVar1;
    return;
  }
  piVar2 = *(int **)(param_1 + 4);
  if (param_1 == *piVar2) {
    *piVar2 = (int)piVar1;
    *piVar1 = param_1;
    *(int **)(param_1 + 4) = piVar1;
    return;
  }
  piVar2[2] = (int)piVar1;
  *piVar1 = param_1;
  *(int **)(param_1 + 4) = piVar1;
  return;
}



void __thiscall FUN_0040ec60(void *this,int *param_1)

{
  int iVar1;
  int *piVar2;
  
  iVar1 = *param_1;
  *param_1 = *(int *)(iVar1 + 8);
  if (*(int *)(iVar1 + 8) != DAT_004239a4) {
    *(int **)(*(int *)(iVar1 + 8) + 4) = param_1;
  }
  *(int *)(iVar1 + 4) = param_1[1];
  if (param_1 == *(int **)(*(int *)((int)this + 4) + 4)) {
    *(int *)(*(int *)((int)this + 4) + 4) = iVar1;
    *(int **)(iVar1 + 8) = param_1;
    param_1[1] = iVar1;
    return;
  }
  piVar2 = (int *)param_1[1];
  if (param_1 == (int *)piVar2[2]) {
    piVar2[2] = iVar1;
    *(int **)(iVar1 + 8) = param_1;
    param_1[1] = iVar1;
    return;
  }
  *piVar2 = iVar1;
  *(int **)(iVar1 + 8) = param_1;
  param_1[1] = iVar1;
  return;
}



void __fastcall FUN_0040ecc0(int *param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  
  piVar3 = (int *)*param_1;
  if ((piVar3[0xb] == 0) && (*(int **)(piVar3[1] + 4) == piVar3)) {
    *param_1 = piVar3[2];
    return;
  }
  iVar1 = *piVar3;
  if (iVar1 == DAT_004239a4) {
    piVar3 = (int *)piVar3[1];
    if (*param_1 == *piVar3) {
      do {
        *param_1 = (int)piVar3;
        piVar3 = (int *)piVar3[1];
      } while (*param_1 == *piVar3);
    }
    *param_1 = (int)piVar3;
    return;
  }
  for (iVar2 = *(int *)(iVar1 + 8); iVar2 != DAT_004239a4; iVar2 = *(int *)(iVar2 + 8)) {
    iVar1 = iVar2;
  }
  *param_1 = iVar1;
  return;
}



// WARNING: Removing unreachable block (ram,0x0040ed70)

void __cdecl FUN_0040ed20(undefined1 *param_1,undefined1 *param_2)

{
  undefined1 uVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 *puVar5;
  uint uVar6;
  int iVar7;
  undefined4 *puVar8;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  puStack_8 = &LAB_00419601;
  local_c = ExceptionList;
  local_4 = 0;
  if (param_1 != (undefined1 *)0x0) {
    uVar1 = *param_2;
    ExceptionList = &local_c;
    *(undefined4 *)(param_1 + 4) = 0;
    *param_1 = uVar1;
    *(undefined4 *)(param_1 + 8) = 0;
    *(undefined4 *)(param_1 + 0xc) = 0;
    uVar2 = *(uint *)(param_2 + 8);
    if (param_1 == param_2) {
      if (uVar2 != 0) {
        FUN_0040f147();
      }
      FUN_00401b10(param_1);
      iVar3 = *(int *)(param_1 + 8) - uVar2;
      iVar7 = -1;
      if (iVar3 != -1) {
        iVar7 = iVar3;
      }
      if (iVar7 != 0) {
        FUN_0040f710((undefined4 *)(uVar2 + *(int *)(param_1 + 4)),
                     (undefined4 *)((int)(uVar2 + *(int *)(param_1 + 4)) + iVar7),iVar3 - iVar7);
        iVar3 = *(int *)(param_1 + 8);
        uVar4 = FUN_00401570(param_1,iVar3 - iVar7,'\0');
        if ((char)uVar4 != '\0') {
          FUN_00401550(param_1,iVar3 - iVar7);
        }
      }
      FUN_00401b10(param_1);
      ExceptionList = local_c;
      return;
    }
    if (uVar2 != 0) {
      puVar5 = *(undefined4 **)(param_2 + 4);
      if (puVar5 == (undefined4 *)0x0) {
        puVar5 = &DAT_0041a214;
      }
      if (*(byte *)((int)puVar5 + -1) < 0xfe) {
        FUN_00401380(param_1,'\x01');
        puVar5 = *(undefined4 **)(param_2 + 4);
        if (puVar5 == (undefined4 *)0x0) {
          puVar5 = &DAT_0041a214;
        }
        *(undefined4 **)(param_1 + 4) = puVar5;
        *(undefined4 *)(param_1 + 8) = *(undefined4 *)(param_2 + 8);
        *(undefined4 *)(param_1 + 0xc) = *(undefined4 *)(param_2 + 0xc);
        *(char *)((int)puVar5 + -1) = *(char *)((int)puVar5 + -1) + '\x01';
        ExceptionList = local_c;
        return;
      }
    }
    uVar4 = FUN_00401570(param_1,uVar2,'\x01');
    if ((char)uVar4 != '\0') {
      puVar5 = *(undefined4 **)(param_2 + 4);
      if (puVar5 == (undefined4 *)0x0) {
        puVar5 = &DAT_0041a214;
      }
      puVar8 = *(undefined4 **)(param_1 + 4);
      for (uVar6 = uVar2 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
        *puVar8 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar8 = puVar8 + 1;
      }
      for (uVar6 = uVar2 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
        *(undefined1 *)puVar8 = *(undefined1 *)puVar5;
        puVar5 = (undefined4 *)((int)puVar5 + 1);
        puVar8 = (undefined4 *)((int)puVar8 + 1);
      }
      *(uint *)(param_1 + 8) = uVar2;
      *(undefined1 *)(*(int *)(param_1 + 4) + uVar2) = 0;
    }
  }
  ExceptionList = local_c;
  return;
}



uint __thiscall FUN_0040ee90(void *this,int param_1)

{
  uint uVar1;
  uint uVar2;
  byte *pbVar3;
  uint uVar4;
  uint uVar5;
  byte *pbVar6;
  bool bVar7;
  bool bVar8;
  
  uVar1 = *(uint *)(param_1 + 8);
  pbVar3 = *(byte **)(param_1 + 4);
  if (pbVar3 == (byte *)0x0) {
    pbVar3 = (byte *)&DAT_0041a214;
  }
  uVar2 = *(uint *)((int)this + 8);
  uVar5 = uVar2;
  if (uVar1 <= uVar2) {
    uVar5 = uVar1;
  }
  bVar7 = false;
  uVar4 = 0;
  bVar8 = true;
  pbVar6 = *(byte **)((int)this + 4);
  do {
    if (uVar5 == 0) break;
    uVar5 = uVar5 - 1;
    bVar7 = *pbVar6 < *pbVar3;
    bVar8 = *pbVar6 == *pbVar3;
    pbVar6 = pbVar6 + 1;
    pbVar3 = pbVar3 + 1;
  } while (bVar8);
  if (!bVar8) {
    uVar4 = (1 - (uint)bVar7) - (uint)(bVar7 != 0);
  }
  if (uVar4 == 0) {
    if (uVar2 < uVar1) {
      return 0xffffffff;
    }
    uVar4 = (uint)(uVar2 != uVar1);
  }
  return uVar4;
}



void __cdecl FUN_0040ef10(undefined *param_1)

{
  FUN_00410fa6(param_1);
  return;
}



void FUN_0040ef1b(void)

{
  size_t sVar1;
  int unaff_EBP;
  
  FUN_0041151c();
  *(undefined1 *)(unaff_EBP + -0x20) = *(undefined1 *)(unaff_EBP + -0xd);
  FUN_00401380((void *)(unaff_EBP + -0x20),'\0');
  sVar1 = _strlen("string too long");
  FUN_004014b0((void *)(unaff_EBP + -0x20),(undefined4 *)"string too long",sVar1);
  *(undefined4 *)(unaff_EBP + -4) = 0;
  FUN_0040ef75();
  *(undefined ***)(unaff_EBP + -0x3c) = &PTR_LAB_0041d78c;
                    // WARNING: Subroutine does not return
  __CxxThrowException_8(unaff_EBP + -0x3c,&DAT_0041e8c8);
}



undefined4 * FUN_0040ef75(void)

{
  undefined4 *this;
  undefined1 *puVar1;
  undefined4 *this_00;
  int unaff_EBP;
  
  FUN_0041151c();
  *(undefined4 **)(unaff_EBP + -0x14) = this_00;
  *(undefined **)(unaff_EBP + -0x10) = &DAT_004239b8;
  FUN_004113f1(this_00,(undefined4 *)(unaff_EBP + -0x10));
  puVar1 = *(undefined1 **)(unaff_EBP + 8);
  *(undefined4 *)(unaff_EBP + -4) = 0;
  this = this_00 + 3;
  *(undefined1 *)this = *puVar1;
  FUN_00401380(this,'\0');
  FUN_00401900(this,puVar1,0,0xffffffff);
  ExceptionList = *(void **)(unaff_EBP + -0xc);
  *this_00 = &PTR_FUN_0041d7ac;
  return this_00;
}



void FUN_0040efe6(void)

{
  undefined1 local_20 [28];
  
  FUN_0040f05c();
                    // WARNING: Subroutine does not return
  __CxxThrowException_8(local_20,&DAT_0041e938);
}



void FUN_0040f003(void)

{
  exception *this;
  int unaff_EBP;
  
  FUN_0041151c();
  *(exception **)(unaff_EBP + -0x10) = this;
  *(undefined ***)this = &PTR_FUN_0041d7ac;
  *(undefined4 *)(unaff_EBP + -4) = 0;
  FUN_00401380(this + 0xc,'\x01');
  *(undefined4 *)(unaff_EBP + -4) = 0xffffffff;
  exception::~exception(this);
  ExceptionList = *(void **)(unaff_EBP + -0xc);
  return;
}



undefined * __thiscall FUN_0040f040(void *this,byte param_1)

{
  FUN_0040f003();
  if ((param_1 & 1) != 0) {
    FUN_0040ef10(this);
  }
  return this;
}



undefined4 * FUN_0040f05c(void)

{
  undefined4 *this;
  undefined1 uVar1;
  int iVar2;
  undefined4 *this_00;
  int unaff_EBP;
  
  FUN_0041151c();
  iVar2 = *(int *)(unaff_EBP + 8);
  *(undefined4 **)(unaff_EBP + -0x10) = this_00;
  FUN_0041142e(this_00,iVar2);
  uVar1 = *(undefined1 *)(iVar2 + 0xc);
  *(undefined4 *)(unaff_EBP + -4) = 0;
  this = this_00 + 3;
  *(undefined1 *)this = uVar1;
  FUN_00401380(this,'\0');
  FUN_00401900(this,(void *)(iVar2 + 0xc),0,0xffffffff);
  ExceptionList = *(void **)(unaff_EBP + -0xc);
  *this_00 = &PTR_FUN_0041d7ac;
  return this_00;
}



void FUN_0040f0b9(void)

{
  exception *this;
  int unaff_EBP;
  
  FUN_0041151c();
  *(exception **)(unaff_EBP + -0x10) = this;
  *(undefined ***)this = &PTR_FUN_0041d7ac;
  *(undefined4 *)(unaff_EBP + -4) = 0;
  FUN_00401380(this + 0xc,'\x01');
  *(undefined4 *)(unaff_EBP + -4) = 0xffffffff;
  exception::~exception(this);
  ExceptionList = *(void **)(unaff_EBP + -0xc);
  return;
}



void FUN_0040f0f6(void)

{
  undefined4 local_20 [7];
  
  FUN_0040f12f(local_20);
                    // WARNING: Subroutine does not return
  __CxxThrowException_8(local_20,&DAT_0041e8c8);
}



undefined4 * __fastcall FUN_0040f12f(undefined4 *param_1)

{
  FUN_0040f05c();
  *param_1 = &PTR_LAB_0041d78c;
  return param_1;
}



void FUN_0040f147(void)

{
  size_t sVar1;
  int unaff_EBP;
  
  FUN_0041151c();
  *(undefined1 *)(unaff_EBP + -0x20) = *(undefined1 *)(unaff_EBP + -0xd);
  FUN_00401380((void *)(unaff_EBP + -0x20),'\0');
  sVar1 = _strlen("invalid string position");
  FUN_004014b0((void *)(unaff_EBP + -0x20),(undefined4 *)"invalid string position",sVar1);
  *(undefined4 *)(unaff_EBP + -4) = 0;
  FUN_0040ef75();
  *(undefined ***)(unaff_EBP + -0x3c) = &PTR_LAB_0041d7bc;
                    // WARNING: Subroutine does not return
  __CxxThrowException_8(unaff_EBP + -0x3c,&DAT_0041e9f0);
}



void FUN_0040f1a1(void)

{
  exception *this;
  int unaff_EBP;
  undefined4 *unaff_FS_OFFSET;
  
  FUN_0041151c();
  *(exception **)(unaff_EBP + -0x10) = this;
  *(undefined ***)this = &PTR_FUN_0041d7ac;
  *(undefined4 *)(unaff_EBP + -4) = 0;
  FUN_00401380(this + 0xc,'\x01');
  *(undefined4 *)(unaff_EBP + -4) = 0xffffffff;
  exception::~exception(this);
  *unaff_FS_OFFSET = *(undefined4 *)(unaff_EBP + -0xc);
  return;
}



void FUN_0040f1de(void)

{
  undefined4 local_20 [7];
  
  FUN_0040f217(local_20);
                    // WARNING: Subroutine does not return
  __CxxThrowException_8(local_20,&DAT_0041e9f0);
}



undefined4 * __fastcall FUN_0040f217(undefined4 *param_1)

{
  FUN_0040f05c();
  *param_1 = &PTR_LAB_0041d7bc;
  return param_1;
}



void FUN_0040f259(void)

{
  FUN_0040fac3(FUN_00401d40);
  return;
}



void FUN_0040f265(void)

{
  FUN_0040fac3(&DAT_0040f271);
  return;
}



undefined4 __fastcall FUN_0040f272(undefined4 param_1)

{
  LONG LVar1;
  
  if (DAT_004239d8 != 2) {
    if (DAT_004239d8 == 0) {
      LVar1 = InterlockedExchange(&DAT_004239d8,1);
      if (LVar1 == 0) {
        InitializeCriticalSection((LPCRITICAL_SECTION)&DAT_004239c0);
        FUN_0040fac3(&LAB_0040f2f0);
        DAT_004239d8 = 2;
        goto LAB_0040f2e4;
      }
      if (LVar1 == 2) {
        DAT_004239d8 = 2;
      }
    }
    while (DAT_004239d8 == 1) {
      Sleep(1);
    }
    if (DAT_004239d8 != 2) {
      return param_1;
    }
  }
LAB_0040f2e4:
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_004239c0);
  return param_1;
}



void FUN_0040f30e(void)

{
  if (DAT_004239d8 == 2) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_004239c0);
  }
  return;
}



void FUN_0040f323(undefined *UNRECOVERED_JUMPTABLE)

{
  ExceptionList = *(void **)ExceptionList;
                    // WARNING: Could not recover jumptable at 0x0040f34e. Too many branches
                    // WARNING: Treating indirect jump as call
  (*(code *)UNRECOVERED_JUMPTABLE)();
  return;
}



void FUN_0040f357(undefined4 param_1,undefined *UNRECOVERED_JUMPTABLE)

{
  LOCK();
  UNLOCK();
                    // WARNING: Could not recover jumptable at 0x0040f35c. Too many branches
                    // WARNING: Treating indirect jump as call
  (*(code *)UNRECOVERED_JUMPTABLE)();
  return;
}



void FUN_0040f35e(undefined4 param_1,undefined *UNRECOVERED_JUMPTABLE)

{
  LOCK();
  UNLOCK();
                    // WARNING: Could not recover jumptable at 0x0040f363. Too many branches
                    // WARNING: Treating indirect jump as call
  (*(code *)UNRECOVERED_JUMPTABLE)();
  return;
}



void FUN_0040f365(PVOID param_1,PEXCEPTION_RECORD param_2)

{
  void *pvVar1;
  
  pvVar1 = ExceptionList;
  RtlUnwind(param_1,(PVOID)0x40f38d,param_2,(PVOID)0x0);
  param_2->ExceptionFlags = param_2->ExceptionFlags & 0xfffffffd;
  *(void **)pvVar1 = ExceptionList;
  ExceptionList = pvVar1;
  return;
}



undefined4 __cdecl
FUN_0040f3b4(PEXCEPTION_RECORD param_1,PVOID param_2,DWORD param_3,undefined4 param_4)

{
  int *in_EAX;
  undefined4 uVar1;
  
  uVar1 = FUN_00411875(param_1,param_2,param_3,param_4,in_EAX,0,(PVOID)0x0,'\0');
  return uVar1;
}



undefined4 __cdecl
FUN_0040f3ea(undefined4 param_1,undefined4 param_2,undefined4 param_3,int param_4,int param_5)

{
  undefined4 uVar1;
  void *local_18;
  code *local_14;
  undefined4 local_10;
  undefined4 local_c;
  int local_8;
  
  local_10 = param_2;
  local_14 = FUN_0040f43e;
  local_8 = param_4 + 1;
  local_c = param_1;
  local_18 = ExceptionList;
  ExceptionList = &local_18;
  uVar1 = __CallSettingFrame_12(param_3,param_1,param_5);
  ExceptionList = local_18;
  return uVar1;
}



void __cdecl FUN_0040f43e(PEXCEPTION_RECORD param_1,PVOID param_2,DWORD param_3)

{
  FUN_00411875(param_1,*(PVOID *)((int)param_2 + 0xc),param_3,0,*(int **)((int)param_2 + 8),
               *(int *)((int)param_2 + 0x10),param_2,'\0');
  return;
}



undefined4 __cdecl
FUN_0040f463(undefined4 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,undefined4 param_6,undefined4 param_7)

{
  DWORD *pDVar1;
  undefined4 uVar2;
  undefined4 **ppuVar3;
  undefined4 *local_34;
  undefined4 local_30;
  undefined4 *local_2c;
  code *local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined1 *local_10;
  undefined1 *local_c;
  int local_8;
  
  local_c = &stack0xfffffffc;
  local_10 = &stack0xffffffbc;
  local_28 = FUN_0040f519;
  local_24 = param_5;
  local_20 = param_2;
  local_1c = param_6;
  local_18 = param_7;
  local_8 = 0;
  local_14 = 0x40f4eb;
  local_2c = ExceptionList;
  ExceptionList = &local_2c;
  local_34 = param_1;
  local_30 = param_3;
  ppuVar3 = &local_34;
  uVar2 = *param_1;
  pDVar1 = FUN_00412133();
  (*(code *)pDVar1[0x1a])(uVar2,ppuVar3);
  if (local_8 != 0) {
    *local_2c = *(undefined4 *)ExceptionList;
  }
  ExceptionList = local_2c;
  return 0;
}



undefined4 __cdecl FUN_0040f519(PEXCEPTION_RECORD param_1,PVOID param_2,DWORD param_3)

{
  undefined4 uVar1;
  
  if ((param_1->ExceptionFlags & 0x66) != 0) {
    *(undefined4 *)((int)param_2 + 0x24) = 1;
    return 1;
  }
  FUN_00411875(param_1,*(PVOID *)((int)param_2 + 0xc),param_3,0,*(int **)((int)param_2 + 8),
               *(int *)((int)param_2 + 0x10),*(PVOID *)((int)param_2 + 0x14),'\x01');
  if (*(int *)((int)param_2 + 0x24) == 0) {
    FUN_0040f365(param_2,param_1);
  }
                    // WARNING: Could not recover jumptable at 0x0040f583. Too many branches
                    // WARNING: Treating indirect jump as call
  uVar1 = (**(code **)((int)param_2 + 0x18))();
  return uVar1;
}



int __cdecl FUN_0040f58e(int param_1,int param_2,int param_3,uint *param_4,uint *param_5)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  
  uVar5 = *(uint *)(param_1 + 0xc);
  iVar1 = *(int *)(param_1 + 0x10);
  uVar4 = uVar5;
  uVar3 = uVar5;
  while (uVar2 = uVar4, -1 < param_2) {
    if (uVar5 == 0xffffffff) {
      FUN_0041229b();
    }
    uVar5 = uVar5 - 1;
    if (((*(int *)(iVar1 + 4 + uVar5 * 0x14) < param_3) &&
        (param_3 <= *(int *)(iVar1 + uVar5 * 0x14 + 8))) || (uVar4 = uVar2, uVar5 == 0xffffffff)) {
      param_2 = param_2 + -1;
      uVar4 = uVar5;
      uVar3 = uVar2;
    }
  }
  uVar5 = uVar5 + 1;
  *param_4 = uVar5;
  *param_5 = uVar3;
  if ((*(uint *)(param_1 + 0xc) < uVar3) || (uVar3 < uVar5)) {
    FUN_0041229b();
  }
  return iVar1 + uVar5 * 0x14;
}



// Library Function - Single Match
//  __global_unwind2
// 
// Library: Visual Studio

void __cdecl __global_unwind2(PVOID param_1)

{
  RtlUnwind(param_1,(PVOID)0x40f624,(PEXCEPTION_RECORD)0x0,(PVOID)0x0);
  return;
}



// Library Function - Single Match
//  __local_unwind2
// 
// Libraries: Visual Studio 1998 Debug, Visual Studio 1998 Release, Visual Studio 2003 Debug, Visual
// Studio 2003 Release

void __cdecl __local_unwind2(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  void *pvStack_1c;
  undefined1 *puStack_18;
  undefined4 local_14;
  int iStack_10;
  
  iStack_10 = param_1;
  puStack_18 = &LAB_0040f62c;
  pvStack_1c = ExceptionList;
  ExceptionList = &pvStack_1c;
  while( true ) {
    iVar1 = *(int *)(param_1 + 8);
    iVar2 = *(int *)(param_1 + 0xc);
    if ((iVar2 == -1) || (iVar2 == param_2)) break;
    local_14 = *(undefined4 *)(iVar1 + iVar2 * 0xc);
    *(undefined4 *)(param_1 + 0xc) = local_14;
    if (*(int *)(iVar1 + 4 + iVar2 * 0xc) == 0) {
      FUN_0040f6e2();
      (**(code **)(iVar1 + 8 + iVar2 * 0xc))();
    }
  }
  ExceptionList = pvStack_1c;
  return;
}



// Library Function - Single Match
//  __abnormal_termination
// 
// Library: Visual Studio

int __cdecl __abnormal_termination(void)

{
  int iVar1;
  
  iVar1 = 0;
  if ((*(undefined1 **)((int)ExceptionList + 4) == &LAB_0040f62c) &&
     (*(int *)((int)ExceptionList + 8) == *(int *)(*(int *)((int)ExceptionList + 0xc) + 0xc))) {
    iVar1 = 1;
  }
  return iVar1;
}



// Library Function - Single Match
//  __NLG_Notify1
// 
// Libraries: Visual Studio 2017 Debug, Visual Studio 2017 Release, Visual Studio 2019 Debug, Visual
// Studio 2019 Release

void __fastcall __NLG_Notify1(undefined4 param_1)

{
  undefined4 in_EAX;
  undefined4 unaff_EBP;
  
  DAT_004208f8 = param_1;
  DAT_004208f4 = in_EAX;
  DAT_004208fc = unaff_EBP;
  return;
}



void FUN_0040f6e2(void)

{
  undefined4 in_EAX;
  int unaff_EBP;
  
  DAT_004208f8 = *(undefined4 *)(unaff_EBP + 8);
  DAT_004208f4 = in_EAX;
  DAT_004208fc = unaff_EBP;
  return;
}



// Library Function - Single Match
//  void * __cdecl operator new(unsigned int)
// 
// Library: Visual Studio 2003 Release

void * __cdecl operator_new(uint param_1)

{
  void *pvVar1;
  
  pvVar1 = __nh_malloc(param_1,1);
  return pvVar1;
}



undefined4 * __cdecl FUN_0040f710(undefined4 *param_1,undefined4 *param_2,uint param_3)

{
  uint uVar1;
  uint uVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  
  if ((param_2 < param_1) && (param_1 < (undefined4 *)(param_3 + (int)param_2))) {
    puVar3 = (undefined4 *)((param_3 - 4) + (int)param_2);
    puVar4 = (undefined4 *)((param_3 - 4) + (int)param_1);
    if (((uint)puVar4 & 3) == 0) {
      uVar1 = param_3 >> 2;
      uVar2 = param_3 & 3;
      if (7 < uVar1) {
        for (; uVar1 != 0; uVar1 = uVar1 - 1) {
          *puVar4 = *puVar3;
          puVar3 = puVar3 + -1;
          puVar4 = puVar4 + -1;
        }
        switch(uVar2) {
        case 0:
          return param_1;
        case 2:
          goto switchD_0040f8c7_caseD_2;
        case 3:
          goto switchD_0040f8c7_caseD_3;
        }
        goto switchD_0040f8c7_caseD_1;
      }
    }
    else {
      switch(param_3) {
      case 0:
        goto switchD_0040f8c7_caseD_0;
      case 1:
        goto switchD_0040f8c7_caseD_1;
      case 2:
        goto switchD_0040f8c7_caseD_2;
      case 3:
        goto switchD_0040f8c7_caseD_3;
      default:
        uVar1 = param_3 - ((uint)puVar4 & 3);
        switch((uint)puVar4 & 3) {
        case 1:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          puVar3 = (undefined4 *)((int)puVar3 + -1);
          uVar1 = uVar1 >> 2;
          puVar4 = (undefined4 *)((int)puVar4 - 1);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return param_1;
            case 2:
              goto switchD_0040f8c7_caseD_2;
            case 3:
              goto switchD_0040f8c7_caseD_3;
            }
            goto switchD_0040f8c7_caseD_1;
          }
          break;
        case 2:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          uVar1 = uVar1 >> 2;
          *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
          puVar3 = (undefined4 *)((int)puVar3 + -2);
          puVar4 = (undefined4 *)((int)puVar4 - 2);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return param_1;
            case 2:
              goto switchD_0040f8c7_caseD_2;
            case 3:
              goto switchD_0040f8c7_caseD_3;
            }
            goto switchD_0040f8c7_caseD_1;
          }
          break;
        case 3:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
          uVar1 = uVar1 >> 2;
          *(undefined1 *)((int)puVar4 + 1) = *(undefined1 *)((int)puVar3 + 1);
          puVar3 = (undefined4 *)((int)puVar3 + -3);
          puVar4 = (undefined4 *)((int)puVar4 - 3);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return param_1;
            case 2:
              goto switchD_0040f8c7_caseD_2;
            case 3:
              goto switchD_0040f8c7_caseD_3;
            }
            goto switchD_0040f8c7_caseD_1;
          }
        }
      }
    }
    switch(uVar1) {
    case 7:
      puVar4[7 - uVar1] = puVar3[7 - uVar1];
    case 6:
      puVar4[6 - uVar1] = puVar3[6 - uVar1];
    case 5:
      puVar4[5 - uVar1] = puVar3[5 - uVar1];
    case 4:
      puVar4[4 - uVar1] = puVar3[4 - uVar1];
    case 3:
      puVar4[3 - uVar1] = puVar3[3 - uVar1];
    case 2:
      puVar4[2 - uVar1] = puVar3[2 - uVar1];
    case 1:
      puVar4[1 - uVar1] = puVar3[1 - uVar1];
      puVar3 = puVar3 + -uVar1;
      puVar4 = puVar4 + -uVar1;
    }
    switch(uVar2) {
    case 1:
switchD_0040f8c7_caseD_1:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      return param_1;
    case 2:
switchD_0040f8c7_caseD_2:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
      return param_1;
    case 3:
switchD_0040f8c7_caseD_3:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
      *(undefined1 *)((int)puVar4 + 1) = *(undefined1 *)((int)puVar3 + 1);
      return param_1;
    }
switchD_0040f8c7_caseD_0:
    return param_1;
  }
  puVar3 = param_1;
  if (((uint)param_1 & 3) == 0) {
    uVar1 = param_3 >> 2;
    uVar2 = param_3 & 3;
    if (7 < uVar1) {
      for (; uVar1 != 0; uVar1 = uVar1 - 1) {
        *puVar3 = *param_2;
        param_2 = param_2 + 1;
        puVar3 = puVar3 + 1;
      }
      switch(uVar2) {
      case 0:
        return param_1;
      case 2:
        goto switchD_0040f745_caseD_2;
      case 3:
        goto switchD_0040f745_caseD_3;
      }
      goto switchD_0040f745_caseD_1;
    }
  }
  else {
    switch(param_3) {
    case 0:
      goto switchD_0040f745_caseD_0;
    case 1:
      goto switchD_0040f745_caseD_1;
    case 2:
      goto switchD_0040f745_caseD_2;
    case 3:
      goto switchD_0040f745_caseD_3;
    default:
      uVar1 = (param_3 - 4) + ((uint)param_1 & 3);
      switch((uint)param_1 & 3) {
      case 1:
        uVar2 = uVar1 & 3;
        *(undefined1 *)param_1 = *(undefined1 *)param_2;
        *(undefined1 *)((int)param_1 + 1) = *(undefined1 *)((int)param_2 + 1);
        uVar1 = uVar1 >> 2;
        *(undefined1 *)((int)param_1 + 2) = *(undefined1 *)((int)param_2 + 2);
        param_2 = (undefined4 *)((int)param_2 + 3);
        puVar3 = (undefined4 *)((int)param_1 + 3);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *param_2;
            param_2 = param_2 + 1;
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return param_1;
          case 2:
            goto switchD_0040f745_caseD_2;
          case 3:
            goto switchD_0040f745_caseD_3;
          }
          goto switchD_0040f745_caseD_1;
        }
        break;
      case 2:
        uVar2 = uVar1 & 3;
        *(undefined1 *)param_1 = *(undefined1 *)param_2;
        uVar1 = uVar1 >> 2;
        *(undefined1 *)((int)param_1 + 1) = *(undefined1 *)((int)param_2 + 1);
        param_2 = (undefined4 *)((int)param_2 + 2);
        puVar3 = (undefined4 *)((int)param_1 + 2);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *param_2;
            param_2 = param_2 + 1;
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return param_1;
          case 2:
            goto switchD_0040f745_caseD_2;
          case 3:
            goto switchD_0040f745_caseD_3;
          }
          goto switchD_0040f745_caseD_1;
        }
        break;
      case 3:
        uVar2 = uVar1 & 3;
        *(undefined1 *)param_1 = *(undefined1 *)param_2;
        param_2 = (undefined4 *)((int)param_2 + 1);
        uVar1 = uVar1 >> 2;
        puVar3 = (undefined4 *)((int)param_1 + 1);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *param_2;
            param_2 = param_2 + 1;
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return param_1;
          case 2:
            goto switchD_0040f745_caseD_2;
          case 3:
            goto switchD_0040f745_caseD_3;
          }
          goto switchD_0040f745_caseD_1;
        }
      }
    }
  }
  switch(uVar1) {
  case 7:
    puVar3[uVar1 - 7] = param_2[uVar1 - 7];
  case 6:
    puVar3[uVar1 - 6] = param_2[uVar1 - 6];
  case 5:
    puVar3[uVar1 - 5] = param_2[uVar1 - 5];
  case 4:
    puVar3[uVar1 - 4] = param_2[uVar1 - 4];
  case 3:
    puVar3[uVar1 - 3] = param_2[uVar1 - 3];
  case 2:
    puVar3[uVar1 - 2] = param_2[uVar1 - 2];
  case 1:
    puVar3[uVar1 - 1] = param_2[uVar1 - 1];
    param_2 = param_2 + uVar1;
    puVar3 = puVar3 + uVar1;
  }
  switch(uVar2) {
  case 1:
switchD_0040f745_caseD_1:
    *(undefined1 *)puVar3 = *(undefined1 *)param_2;
    return param_1;
  case 2:
switchD_0040f745_caseD_2:
    *(undefined1 *)puVar3 = *(undefined1 *)param_2;
    *(undefined1 *)((int)puVar3 + 1) = *(undefined1 *)((int)param_2 + 1);
    return param_1;
  case 3:
switchD_0040f745_caseD_3:
    *(undefined1 *)puVar3 = *(undefined1 *)param_2;
    *(undefined1 *)((int)puVar3 + 1) = *(undefined1 *)((int)param_2 + 1);
    *(undefined1 *)((int)puVar3 + 2) = *(undefined1 *)((int)param_2 + 2);
    return param_1;
  }
switchD_0040f745_caseD_0:
  return param_1;
}



undefined4 __cdecl FUN_0040fa45(undefined4 param_1)

{
  byte *pbVar1;
  SIZE_T SVar2;
  
  FUN_0040fe3a();
  pbVar1 = (byte *)FUN_00412620(DAT_00425138);
  if (pbVar1 < DAT_00425134 + (4 - (int)DAT_00425138)) {
    SVar2 = FUN_00412620(DAT_00425138);
    pbVar1 = FUN_004122f1(DAT_00425138,(uint *)(SVar2 + 0x10));
    if (pbVar1 == (byte *)0x0) {
      param_1 = 0;
      goto LAB_0040faba;
    }
    DAT_00425134 = pbVar1 + ((int)DAT_00425134 - (int)DAT_00425138 >> 2) * 4;
    DAT_00425138 = pbVar1;
  }
  *(undefined4 *)DAT_00425134 = param_1;
  DAT_00425134 = DAT_00425134 + 4;
LAB_0040faba:
  FUN_0040fe43();
  return param_1;
}



int __cdecl FUN_0040fac3(undefined4 param_1)

{
  int iVar1;
  
  iVar1 = FUN_0040fa45(param_1);
  return (iVar1 != 0) - 1;
}



int __cdecl FUN_0040fb04(undefined1 *param_1,int param_2,byte *param_3,undefined4 *param_4)

{
  int iVar1;
  undefined1 *local_24;
  int local_20;
  undefined1 *local_1c;
  undefined4 local_18;
  
  local_1c = param_1;
  local_24 = param_1;
  local_20 = param_2;
  local_18 = 0x42;
  iVar1 = FUN_00412826((int *)&local_24,param_3,param_4);
  local_20 = local_20 + -1;
  if (local_20 < 0) {
    FUN_0041270e(0,(int *)&local_24);
  }
  else {
    *local_24 = 0;
  }
  return iVar1;
}



void __fastcall FUN_0040fb54(undefined4 *param_1)

{
  *param_1 = &type_info::vftable;
  FUN_004130b6(0x1b);
  if ((undefined *)param_1[1] != (undefined *)0x0) {
    FUN_00410fa6((undefined *)param_1[1]);
  }
  FUN_00413117(0x1b);
  return;
}



undefined4 * __thiscall FUN_0040fb7d(void *this,byte param_1)

{
  FUN_0040fb54(this);
  if ((param_1 & 1) != 0) {
    FUN_0040ef10(this);
  }
  return this;
}



// Library Function - Single Match
//  __CxxThrowException@8
// 
// Library: Visual Studio 2003 Release

void __CxxThrowException_8(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  DWORD *pDVar2;
  DWORD *pDVar3;
  DWORD local_24 [4];
  DWORD local_14;
  ULONG_PTR local_10;
  undefined4 local_c;
  undefined4 local_8;
  
  pDVar2 = &DAT_0041d7e8;
  pDVar3 = local_24;
  for (iVar1 = 8; iVar1 != 0; iVar1 = iVar1 + -1) {
    *pDVar3 = *pDVar2;
    pDVar2 = pDVar2 + 1;
    pDVar3 = pDVar3 + 1;
  }
  local_c = param_1;
  local_8 = param_2;
  RaiseException(local_24[0],local_24[1],local_14,&local_10);
  return;
}



HANDLE __cdecl FUN_0040fbd3(int param_1,SIZE_T param_2,int param_3)

{
  int *lpThreadId;
  HANDLE hThread;
  DWORD DVar1;
  
  DVar1 = 0;
  lpThreadId = FUN_00413218(1,0x74);
  if (lpThreadId != (int *)0x0) {
    FUN_00412120((int)lpThreadId);
    lpThreadId[0x12] = param_1;
    lpThreadId[0x13] = param_3;
    hThread = CreateThread((LPSECURITY_ATTRIBUTES)0x0,param_2,FUN_0040fc4a,lpThreadId,4,
                           (LPDWORD)lpThreadId);
    lpThreadId[1] = (int)hThread;
    if ((hThread != (HANDLE)0x0) && (DVar1 = ResumeThread(hThread), DVar1 != 0xffffffff)) {
      return hThread;
    }
    DVar1 = GetLastError();
  }
  FUN_00410fa6((undefined *)lpThreadId);
  if (DVar1 != 0) {
    FUN_00410cd0(DVar1);
  }
  return (HANDLE)0xffffffff;
}



undefined4 FUN_0040fc4a(undefined *param_1)

{
  undefined *puVar1;
  BOOL BVar2;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_0041d808;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  ExceptionList = &local_14;
  puVar1 = TlsGetValue(DAT_00420ac0);
  if (puVar1 != (undefined *)0x0) {
    *(undefined4 *)(puVar1 + 0x48) = *(undefined4 *)(param_1 + 0x48);
    *(undefined4 *)(puVar1 + 0x4c) = *(undefined4 *)(param_1 + 0x4c);
    FUN_0041219a(param_1);
    param_1 = puVar1;
  }
  BVar2 = TlsSetValue(DAT_00420ac0,param_1);
  if (BVar2 == 0) {
    __amsg_exit(0x10);
  }
  if (PTR_FUN_00420a9c != (undefined *)0x0) {
    (*(code *)PTR_FUN_00420a9c)();
  }
  local_8 = 0;
  (**(code **)(param_1 + 0x48))(*(undefined4 *)(param_1 + 0x4c));
  FUN_0040fd07();
  ExceptionList = local_14;
  return 0;
}



void FUN_0040fd07(void)

{
  DWORD *pDVar1;
  
  if (PTR_FUN_00420aa0 != (undefined *)0x0) {
    (*(code *)PTR_FUN_00420aa0)();
  }
  pDVar1 = FUN_00412133();
  if (pDVar1 == (DWORD *)0x0) {
    __amsg_exit(0x10);
  }
  if ((HANDLE)pDVar1[1] != (HANDLE)0xffffffff) {
    CloseHandle((HANDLE)pDVar1[1]);
  }
  FUN_0041219a((undefined *)pDVar1);
                    // WARNING: Subroutine does not return
  ExitThread(0);
}



void FUN_0040fd46(void)

{
  if (PTR_FUN_00420a98 != (undefined *)0x0) {
    (*(code *)PTR_FUN_00420a98)();
  }
  FUN_0040fe4c((undefined4 *)&DAT_00420028,(undefined4 *)&DAT_0042003c);
  FUN_0040fe4c((undefined4 *)&DAT_00420000,(undefined4 *)&DAT_00420024);
  return;
}



void __cdecl FUN_0040fd73(UINT param_1)

{
  FUN_0040fd95(param_1,0,0);
  return;
}



// Library Function - Single Match
//  __exit
// 
// Library: Visual Studio 2003 Release

void __cdecl __exit(int _Code)

{
  FUN_0040fd95(_Code,1,0);
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void __cdecl FUN_0040fd95(UINT param_1,int param_2,int param_3)

{
  HANDLE hProcess;
  undefined4 *puVar1;
  UINT uExitCode;
  
  FUN_0040fe3a();
  if (DAT_00423a1c == 1) {
    uExitCode = param_1;
    hProcess = GetCurrentProcess();
    TerminateProcess(hProcess,uExitCode);
  }
  _DAT_00423a18 = 1;
  DAT_00423a14 = (undefined1)param_3;
  if (param_2 == 0) {
    if ((DAT_00425138 != (undefined4 *)0x0) &&
       (puVar1 = (undefined4 *)(DAT_00425134 - 4), DAT_00425138 <= puVar1)) {
      do {
        if ((code *)*puVar1 != (code *)0x0) {
          (*(code *)*puVar1)();
        }
        puVar1 = puVar1 + -1;
      } while (DAT_00425138 <= puVar1);
    }
    FUN_0040fe4c((undefined4 *)&DAT_00420040,(undefined4 *)&DAT_00420048);
  }
  FUN_0040fe4c((undefined4 *)&DAT_0042004c,(undefined4 *)&DAT_00420054);
  if (param_3 == 0) {
    DAT_00423a1c = 1;
                    // WARNING: Subroutine does not return
    ExitProcess(param_1);
  }
  FUN_0040fe43();
  return;
}



void FUN_0040fe3a(void)

{
  FUN_004130b6(0xd);
  return;
}



void FUN_0040fe43(void)

{
  FUN_00413117(0xd);
  return;
}



void __cdecl FUN_0040fe4c(undefined4 *param_1,undefined4 *param_2)

{
  for (; param_1 < param_2; param_1 = param_1 + 1) {
    if ((code *)*param_1 != (code *)0x0) {
      (*(code *)*param_1)();
    }
  }
  return;
}



uint __thiscall FUN_0040fe66(void *this,byte *param_1,byte *param_2)

{
  byte *pbVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  byte *pbVar5;
  ushort uVar6;
  ushort uVar7;
  byte *local_10;
  byte *local_c;
  byte local_8;
  byte local_7;
  
  if (DAT_00424f1c == 0) {
    uVar2 = FUN_00413c00(this,param_1,param_2);
  }
  else {
    FUN_004130b6(0x19);
    local_10 = param_2 + -1;
    local_c = param_1 + -1;
    do {
      uVar2 = (uint)*param_1;
      pbVar5 = param_1 + 1;
      pbVar1 = local_c + 1;
      if ((*(byte *)((int)&DAT_00425020 + uVar2 + 1) & 4) == 0) {
        param_1 = pbVar5;
        local_c = pbVar1;
        if ((*(byte *)((int)&DAT_00425020 + uVar2 + 1) & 0x10) == 0x10) {
          uVar2 = (uint)(byte)(&DAT_00424f20)[uVar2];
        }
      }
      else if (*pbVar5 == 0) {
        uVar2 = 0;
        param_1 = pbVar5;
        local_c = pbVar1;
      }
      else {
        iVar4 = FUN_004139a4(DAT_00425124,0x200,(char *)pbVar1,2,(LPWSTR)&local_8,2,DAT_00424f04,1);
        if (iVar4 == 1) {
          uVar2 = (uint)local_8;
        }
        else {
          if (iVar4 != 2) goto LAB_0040ffca;
          uVar2 = (uint)local_8 * 0x100 + (uint)local_7;
        }
        param_1 = param_1 + 2;
        local_c = local_c + 2;
      }
      uVar3 = (uint)*param_2;
      uVar6 = (ushort)*param_2;
      pbVar5 = param_2 + 1;
      pbVar1 = local_10 + 1;
      if ((*(byte *)((int)&DAT_00425020 + uVar3 + 1) & 4) == 0) {
        param_2 = pbVar5;
        local_10 = pbVar1;
        if ((*(byte *)((int)&DAT_00425020 + uVar3 + 1) & 0x10) == 0x10) {
          uVar6 = (ushort)(byte)(&DAT_00424f20)[uVar3];
        }
      }
      else if (*pbVar5 == 0) {
        uVar6 = 0;
        param_2 = pbVar5;
        local_10 = pbVar1;
      }
      else {
        iVar4 = FUN_004139a4(DAT_00425124,0x200,(char *)pbVar1,2,(LPWSTR)&local_8,2,DAT_00424f04,1);
        if (iVar4 == 1) {
          uVar6 = (ushort)local_8;
        }
        else {
          if (iVar4 != 2) {
LAB_0040ffca:
            FUN_00413117(0x19);
            return 0x7fffffff;
          }
          uVar6 = (ushort)local_8 * 0x100 + (ushort)local_7;
        }
        param_2 = param_2 + 2;
        local_10 = local_10 + 2;
      }
      uVar7 = (ushort)uVar2;
      if (uVar7 != uVar6) {
        FUN_00413117(0x19);
        return (-(uint)(uVar6 < uVar7) & 2) - 1;
      }
    } while (uVar7 != 0);
    FUN_00413117(0x19);
    uVar2 = 0;
  }
  return uVar2;
}



void __cdecl FUN_0040fffb(byte *param_1,byte *param_2,byte *param_3,byte *param_4,byte *param_5)

{
  byte bVar1;
  size_t sVar2;
  byte *pbVar3;
  uint uVar4;
  byte *local_8;
  
  local_8 = (byte *)0x0;
  sVar2 = _strlen((char *)param_1);
  if ((sVar2 == 0) || (param_1[1] != 0x3a)) {
    if (param_2 != (byte *)0x0) {
      *param_2 = 0;
    }
  }
  else {
    if (param_2 != (byte *)0x0) {
      FUN_00413cd0(param_2,param_1,2);
      param_2[2] = 0;
    }
    param_1 = param_1 + 2;
  }
  param_2 = (byte *)0x0;
  pbVar3 = param_1;
  if (*param_1 != 0) {
    do {
      bVar1 = *pbVar3;
      if ((*(byte *)((int)&DAT_00425020 + bVar1 + 1) & 4) == 0) {
        if ((bVar1 == 0x2f) || (bVar1 == 0x5c)) {
          param_2 = pbVar3 + 1;
        }
        else if (bVar1 == 0x2e) {
          local_8 = pbVar3;
        }
      }
      else {
        pbVar3 = pbVar3 + 1;
      }
      pbVar3 = pbVar3 + 1;
    } while (*pbVar3 != 0);
    if (param_2 != (byte *)0x0) {
      if (param_3 != (byte *)0x0) {
        uVar4 = (int)param_2 - (int)param_1;
        if (0xfe < uVar4) {
          uVar4 = 0xff;
        }
        FUN_00413cd0(param_3,param_1,uVar4);
        param_3[uVar4] = 0;
      }
      goto LAB_004100c2;
    }
  }
  param_2 = param_1;
  if (param_3 != (byte *)0x0) {
    *param_3 = 0;
  }
LAB_004100c2:
  param_1 = pbVar3;
  if ((local_8 == (byte *)0x0) || (local_8 < param_2)) {
    if (param_4 != (byte *)0x0) {
      uVar4 = 0xff;
      if ((uint)((int)param_1 - (int)param_2) < 0xff) {
        uVar4 = (int)param_1 - (int)param_2;
      }
      FUN_00413cd0(param_4,param_2,uVar4);
      param_4[uVar4] = 0;
    }
    if (param_5 != (byte *)0x0) {
      *param_5 = 0;
    }
  }
  else {
    if (param_4 != (byte *)0x0) {
      uVar4 = (int)local_8 - (int)param_2;
      if (0xfe < uVar4) {
        uVar4 = 0xff;
      }
      FUN_00413cd0(param_4,param_2,uVar4);
      param_4[uVar4] = 0;
    }
    if (param_5 != (byte *)0x0) {
      uVar4 = 0xff;
      if ((uint)((int)param_1 - (int)local_8) < 0xff) {
        uVar4 = (int)param_1 - (int)local_8;
      }
      FUN_00413cd0(param_5,local_8,uVar4);
      param_5[uVar4] = 0;
    }
  }
  return;
}



void __cdecl FUN_00410142(byte *param_1,byte *param_2,byte *param_3,byte *param_4,byte *param_5)

{
  byte bVar1;
  byte *pbVar2;
  byte *pbVar3;
  
  if ((param_2 != (byte *)0x0) && (*param_2 != 0)) {
    *param_1 = *param_2;
    param_1[1] = 0x3a;
    param_1 = param_1 + 2;
  }
  if ((param_3 != (byte *)0x0) && (bVar1 = *param_3, pbVar2 = param_3, bVar1 != 0)) {
    do {
      pbVar3 = param_1;
      *pbVar3 = bVar1;
      bVar1 = pbVar2[1];
      param_1 = pbVar3 + 1;
      pbVar2 = pbVar2 + 1;
    } while (bVar1 != 0);
    pbVar2 = FUN_00413d6a(param_3,pbVar2);
    if ((*pbVar2 != 0x2f) && (*pbVar2 != 0x5c)) {
      *param_1 = 0x5c;
      param_1 = pbVar3 + 2;
    }
  }
  if (param_4 != (byte *)0x0) {
    bVar1 = *param_4;
    while (bVar1 != 0) {
      *param_1 = bVar1;
      pbVar2 = param_4 + 1;
      param_1 = param_1 + 1;
      param_4 = param_4 + 1;
      bVar1 = *pbVar2;
    }
  }
  if (param_5 == (byte *)0x0) {
    *param_1 = 0;
  }
  else {
    if ((*param_5 != 0) && (*param_5 != 0x2e)) {
      *param_1 = 0x2e;
      param_1 = param_1 + 1;
    }
    do {
      bVar1 = *param_5;
      *param_1 = bVar1;
      param_1 = param_1 + 1;
      param_5 = param_5 + 1;
    } while (bVar1 != 0);
  }
  return;
}



// WARNING: Unable to track spacebase fully for stack

void FUN_004101e0(void)

{
  uint in_EAX;
  undefined1 *puVar1;
  undefined4 unaff_retaddr;
  
  puVar1 = &stack0x00000004;
  for (; 0xfff < in_EAX; in_EAX = in_EAX - 0x1000) {
    puVar1 = puVar1 + -0x1000;
  }
  *(undefined4 *)(puVar1 + (-4 - in_EAX)) = unaff_retaddr;
  return;
}



uint __thiscall FUN_0041020f(void *this,int param_1)

{
  uint uVar1;
  
  if (1 < DAT_00420f1c) {
    uVar1 = FUN_00413dd7(this,param_1,0x103);
    return uVar1;
  }
  return *(ushort *)(PTR_DAT_00420d10 + param_1 * 2) & 0x103;
}



void __thiscall FUN_0041023d(void *this,byte *param_1,int *param_2,void *param_3)

{
  FUN_00410254(this,param_1,param_2,param_3,0);
  return;
}



void * __thiscall FUN_00410254(void *this,byte *param_1,int *param_2,void *param_3,uint param_4)

{
  byte *pbVar1;
  void *pvVar2;
  uint uVar3;
  void *pvVar4;
  uint uVar5;
  DWORD *pDVar6;
  void *this_00;
  byte bVar7;
  undefined *puVar8;
  void *local_c;
  byte *local_8;
  
  local_c = (void *)0x0;
  bVar7 = *param_1;
  pbVar1 = param_1;
  while( true ) {
    local_8 = pbVar1 + 1;
    if (DAT_00420f1c < 2) {
      uVar3 = (byte)PTR_DAT_00420d10[(uint)bVar7 * 2] & 8;
      this = PTR_DAT_00420d10;
    }
    else {
      puVar8 = (undefined *)0x8;
      uVar3 = FUN_00413dd7(this,(uint)bVar7,8);
      this = puVar8;
    }
    if (uVar3 == 0) break;
    bVar7 = *local_8;
    pbVar1 = local_8;
  }
  if (bVar7 == 0x2d) {
    param_4 = param_4 | 2;
LAB_004102af:
    bVar7 = *local_8;
    local_8 = pbVar1 + 2;
  }
  else if (bVar7 == 0x2b) goto LAB_004102af;
  if ((((int)param_3 < 0) || (param_3 == (void *)0x1)) || (0x24 < (int)param_3)) {
    if (param_2 != (int *)0x0) {
      *param_2 = (int)param_1;
    }
    return (void *)0x0;
  }
  this_00 = (void *)0x10;
  if (param_3 == (void *)0x0) {
    if (bVar7 != 0x30) {
      param_3 = (void *)0xa;
      goto LAB_00410319;
    }
    if ((*local_8 != 0x78) && (*local_8 != 0x58)) {
      param_3 = (void *)0x8;
      goto LAB_00410319;
    }
    param_3 = (void *)0x10;
  }
  if (((param_3 == (void *)0x10) && (bVar7 == 0x30)) && ((*local_8 == 0x78 || (*local_8 == 0x58))))
  {
    bVar7 = local_8[1];
    local_8 = local_8 + 2;
  }
LAB_00410319:
  pvVar4 = (void *)(0xffffffff / ZEXT48(param_3));
  do {
    uVar3 = (uint)bVar7;
    if (DAT_00420f1c < 2) {
      uVar5 = (byte)PTR_DAT_00420d10[uVar3 * 2] & 4;
    }
    else {
      pvVar2 = (void *)0x4;
      uVar5 = FUN_00413dd7(this_00,uVar3,4);
      this_00 = pvVar2;
    }
    if (uVar5 == 0) {
      if (DAT_00420f1c < 2) {
        uVar3 = *(ushort *)(PTR_DAT_00420d10 + uVar3 * 2) & 0x103;
      }
      else {
        uVar3 = FUN_00413dd7(this_00,uVar3,0x103);
      }
      if (uVar3 == 0) {
LAB_004103c5:
        local_8 = local_8 + -1;
        if ((param_4 & 8) == 0) {
          if (param_2 != (int *)0x0) {
            local_8 = param_1;
          }
          local_c = (void *)0x0;
        }
        else if (((param_4 & 4) != 0) ||
                (((param_4 & 1) == 0 &&
                 ((((param_4 & 2) != 0 && ((void *)0x80000000 < local_c)) ||
                  (((param_4 & 2) == 0 && ((void *)0x7fffffff < local_c)))))))) {
          pDVar6 = FUN_00410d43();
          *pDVar6 = 0x22;
          if ((param_4 & 1) == 0) {
            local_c = (void *)(((param_4 & 2) != 0) + 0x7fffffff);
          }
          else {
            local_c = (void *)0xffffffff;
          }
        }
        if (param_2 != (int *)0x0) {
          *param_2 = (int)local_8;
        }
        if ((param_4 & 2) == 0) {
          return local_c;
        }
        return (void *)-(int)local_c;
      }
      uVar3 = FUN_00413e4c((int)(char)bVar7);
      this_00 = (void *)(uVar3 - 0x37);
    }
    else {
      this_00 = (void *)((char)bVar7 + -0x30);
    }
    if (param_3 <= this_00) goto LAB_004103c5;
    if ((local_c < pvVar4) ||
       ((local_c == pvVar4 && (this_00 <= (void *)(0xffffffff % ZEXT48(param_3)))))) {
      local_c = (void *)((int)local_c * (int)param_3 + (int)this_00);
      param_4 = param_4 | 8;
    }
    else {
      param_4 = param_4 | 0xc;
    }
    bVar7 = *local_8;
    local_8 = local_8 + 1;
  } while( true );
}



void __thiscall FUN_00410459(void *this,byte *param_1,int *param_2,void *param_3)

{
  FUN_00410254(this,param_1,param_2,param_3,1);
  return;
}



uint * __cdecl FUN_00410470(uint *param_1,char *param_2)

{
  char *pcVar1;
  char *pcVar2;
  char cVar3;
  uint *puVar4;
  uint uVar5;
  char cVar6;
  uint uVar7;
  char *pcVar8;
  uint uVar9;
  uint *puVar10;
  
  cVar3 = *param_2;
  if (cVar3 == '\0') {
    return param_1;
  }
  if (param_2[1] == '\0') {
    while (((uint)param_1 & 3) != 0) {
      uVar5 = *param_1;
      if ((char)uVar5 == cVar3) {
        return param_1;
      }
      param_1 = (uint *)((int)param_1 + 1);
      if ((char)uVar5 == '\0') {
        return (uint *)0x0;
      }
    }
    while( true ) {
      while( true ) {
        uVar5 = *param_1;
        uVar9 = uVar5 ^ CONCAT22(CONCAT11(cVar3,cVar3),CONCAT11(cVar3,cVar3));
        uVar7 = uVar5 ^ 0xffffffff ^ uVar5 + 0x7efefeff;
        puVar10 = param_1 + 1;
        if (((uVar9 ^ 0xffffffff ^ uVar9 + 0x7efefeff) & 0x81010100) != 0) break;
        param_1 = puVar10;
        if ((uVar7 & 0x81010100) != 0) {
          if ((uVar7 & 0x1010100) != 0) {
            return (uint *)0x0;
          }
          if ((uVar5 + 0x7efefeff & 0x80000000) == 0) {
            return (uint *)0x0;
          }
        }
      }
      uVar5 = *param_1;
      if ((char)uVar5 == cVar3) {
        return param_1;
      }
      if ((char)uVar5 == '\0') {
        return (uint *)0x0;
      }
      cVar6 = (char)(uVar5 >> 8);
      if (cVar6 == cVar3) {
        return (uint *)((int)param_1 + 1);
      }
      if (cVar6 == '\0') {
        return (uint *)0x0;
      }
      cVar6 = (char)(uVar5 >> 0x10);
      if (cVar6 == cVar3) {
        return (uint *)((int)param_1 + 2);
      }
      if (cVar6 == '\0') break;
      cVar6 = (char)(uVar5 >> 0x18);
      if (cVar6 == cVar3) {
        return (uint *)((int)param_1 + 3);
      }
      param_1 = puVar10;
      if (cVar6 == '\0') {
        return (uint *)0x0;
      }
    }
    return (uint *)0x0;
  }
  do {
    cVar6 = (char)*param_1;
    do {
      while (puVar10 = param_1, param_1 = (uint *)((int)puVar10 + 1), cVar6 != cVar3) {
        if (cVar6 == '\0') {
          return (uint *)0x0;
        }
        cVar6 = *(char *)param_1;
      }
      cVar6 = *(char *)param_1;
      pcVar8 = param_2;
      puVar4 = puVar10;
    } while (cVar6 != param_2[1]);
    do {
      if (pcVar8[2] == '\0') {
        return puVar10;
      }
      if (*(char *)((int)puVar4 + 2) != pcVar8[2]) break;
      pcVar1 = pcVar8 + 3;
      if (*pcVar1 == '\0') {
        return puVar10;
      }
      pcVar2 = (char *)((int)puVar4 + 3);
      pcVar8 = pcVar8 + 2;
      puVar4 = (uint *)((int)puVar4 + 2);
    } while (*pcVar1 == *pcVar2);
  } while( true );
}



int __cdecl FUN_004104f0(byte *param_1,byte *param_2,size_t param_3)

{
  byte bVar1;
  size_t sVar2;
  int iVar3;
  uint uVar4;
  byte *pbVar5;
  byte *pbVar6;
  byte *pbVar7;
  
  if (param_3 == 0) {
    return 0;
  }
  if (DAT_00424f1c == 0) {
    iVar3 = _strncmp((char *)param_1,(char *)param_2,param_3);
    return iVar3;
  }
  FUN_004130b6(0x19);
  pbVar5 = param_2;
  sVar2 = param_3;
  do {
    param_3 = sVar2 - 1;
    param_2._0_2_ = (ushort)*param_1;
    pbVar7 = param_1 + 1;
    if ((*(byte *)((int)&DAT_00425020 + (ushort)param_2 + 1) & 4) == 0) {
LAB_00410571:
      bVar1 = *pbVar5;
      uVar4 = (uint)(ushort)bVar1;
      pbVar6 = pbVar5 + 1;
      if ((*(byte *)((int)&DAT_00425020 + (ushort)bVar1 + 1) & 4) != 0) {
        if (param_3 != 0) {
          param_3 = sVar2 - 2;
          if (*pbVar6 != 0) {
            uVar4 = (uint)CONCAT11(bVar1,*pbVar6);
            pbVar6 = pbVar5 + 2;
            goto LAB_0041059e;
          }
        }
        uVar4 = 0;
      }
    }
    else {
      if (param_3 != 0) {
        bVar1 = *pbVar7;
        if (bVar1 == 0) {
          param_2._0_2_ = 0;
        }
        else {
          pbVar7 = param_1 + 2;
          param_2._0_2_ = CONCAT11(*param_1,bVar1);
        }
        goto LAB_00410571;
      }
      uVar4 = (uint)*pbVar5;
      param_2._0_2_ = 0;
      pbVar6 = pbVar5;
      if ((*(byte *)((int)&DAT_00425020 + uVar4 + 1) & 4) != 0) {
LAB_004105d0:
        FUN_00413117(0x19);
        return 0;
      }
    }
LAB_0041059e:
    if ((ushort)param_2 != (ushort)uVar4) {
      FUN_00413117(0x19);
      return (-(uint)((ushort)uVar4 < (ushort)param_2) & 2) - 1;
    }
    if (((ushort)param_2 == 0) || (pbVar5 = pbVar6, param_1 = pbVar7, sVar2 = param_3, param_3 == 0)
       ) goto LAB_004105d0;
  } while( true );
}



undefined4 __cdecl FUN_004105df(FILE *param_1)

{
  undefined4 uVar1;
  
  uVar1 = 0xffffffff;
  if ((param_1->_flag & 0x40) == 0) {
    FUN_00414084((uint)param_1);
    uVar1 = __fclose_lk(param_1);
    FUN_004140d6((uint)param_1);
  }
  else {
    param_1->_flag = 0;
  }
  return uVar1;
}



// Library Function - Single Match
//  __fclose_lk
// 
// Library: Visual Studio 2003 Release

undefined4 __cdecl __fclose_lk(FILE *param_1)

{
  int iVar1;
  undefined4 uVar2;
  
  uVar2 = 0xffffffff;
  if ((param_1->_flag & 0x83) != 0) {
    uVar2 = FUN_004110bd((int *)param_1);
    __freebuf(param_1);
    iVar1 = FUN_00414128(param_1->_file);
    if (iVar1 < 0) {
      uVar2 = 0xffffffff;
    }
    else if (param_1->_tmpfname != (char *)0x0) {
      FUN_00410fa6(param_1->_tmpfname);
      param_1->_tmpfname = (char *)0x0;
    }
  }
  param_1->_flag = 0;
  return uVar2;
}



uint __cdecl FUN_0041065c(char *param_1,uint param_2,uint param_3,int *param_4)

{
  uint uVar1;
  
  FUN_00414084((uint)param_4);
  uVar1 = FUN_0041068b(param_1,param_2,param_3,param_4);
  FUN_004140d6((uint)param_4);
  return uVar1;
}



uint __cdecl FUN_0041068b(char *param_1,uint param_2,uint param_3,int *param_4)

{
  int *piVar1;
  int iVar2;
  int *piVar3;
  uint uVar4;
  int *piVar5;
  int *piVar6;
  int *piVar7;
  
  piVar1 = param_4;
  piVar6 = (int *)(param_2 * param_3);
  if (piVar6 == (int *)0x0) {
    param_3 = 0;
  }
  else {
    piVar5 = piVar6;
    if ((*(ushort *)(param_4 + 3) & 0x10c) == 0) {
      param_4 = (int *)0x1000;
    }
    else {
      param_4 = (int *)param_4[6];
    }
    do {
      if (((piVar1[3] & 0x108U) == 0) || (piVar7 = (int *)piVar1[1], piVar7 == (int *)0x0)) {
        if (param_4 <= piVar5) {
          if (((piVar1[3] & 0x108U) != 0) && (iVar2 = FUN_004110bd(piVar1), iVar2 != 0)) {
LAB_0041078c:
            return (uint)((int)piVar6 - (int)piVar5) / param_2;
          }
          piVar7 = piVar5;
          if (param_4 != (int *)0x0) {
            piVar7 = (int *)((int)piVar5 - (uint)piVar5 % (uint)param_4);
          }
          piVar3 = (int *)FUN_00414233(piVar1[4],param_1,(uint)piVar7);
          if ((piVar3 == (int *)0xffffffff) ||
             (piVar5 = (int *)((int)piVar5 - (int)piVar3), piVar3 < piVar7)) {
            piVar1[3] = piVar1[3] | 0x20;
            goto LAB_0041078c;
          }
          goto LAB_00410743;
        }
        uVar4 = FUN_0041270e((int)*param_1,piVar1);
        if (uVar4 == 0xffffffff) goto LAB_0041078c;
        param_1 = param_1 + 1;
        param_4 = (int *)piVar1[6];
        piVar5 = (int *)((int)piVar5 - 1);
        if ((int)param_4 < 1) {
          param_4 = (int *)0x1;
        }
      }
      else {
        piVar3 = piVar5;
        if (piVar7 <= piVar5) {
          piVar3 = piVar7;
        }
        FUN_00411540((undefined4 *)*piVar1,(undefined4 *)param_1,(uint)piVar3);
        piVar1[1] = piVar1[1] - (int)piVar3;
        *piVar1 = *piVar1 + (int)piVar3;
        piVar5 = (int *)((int)piVar5 - (int)piVar3);
LAB_00410743:
        param_1 = param_1 + (int)piVar3;
      }
    } while (piVar5 != (int *)0x0);
  }
  return param_3;
}



undefined4 * __cdecl FUN_00410795(LPCSTR param_1,char *param_2,uint param_3)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  
  puVar1 = FUN_00414593();
  if (puVar1 == (undefined4 *)0x0) {
    return (undefined4 *)0x0;
  }
  puVar2 = FUN_00414423(param_1,param_2,param_3,puVar1);
  FUN_004140d6((uint)puVar1);
  return puVar2;
}



void __cdecl FUN_004107c6(LPCSTR param_1,char *param_2)

{
  FUN_00410795(param_1,param_2,0x40);
  return;
}



void FUN_004107d9(undefined4 param_1,undefined4 param_2,int param_3,undefined *param_4)

{
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  puStack_c = &DAT_0041d818;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  local_8 = 0;
  ExceptionList = &local_14;
  while( true ) {
    param_3 = param_3 + -1;
    if (param_3 < 0) break;
    (*(code *)param_4)();
  }
  local_8 = 0xffffffff;
  FUN_00410841();
  ExceptionList = local_14;
  return;
}



void FUN_00410841(void)

{
  int unaff_EBP;
  
  if (*(int *)(unaff_EBP + -0x1c) == 0) {
    FUN_00410859(*(undefined4 *)(unaff_EBP + 8),*(undefined4 *)(unaff_EBP + 0xc),
                 *(int *)(unaff_EBP + 0x10),*(undefined **)(unaff_EBP + 0x14));
  }
  return;
}



void FUN_00410859(undefined4 param_1,undefined4 param_2,int param_3,undefined *param_4)

{
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  puStack_c = &DAT_0041d828;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  local_8 = 0;
  ExceptionList = &local_14;
  while( true ) {
    param_3 = param_3 + -1;
    if (param_3 < 0) break;
    (*(code *)param_4)();
  }
  ExceptionList = local_14;
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void entry(void)

{
  DWORD DVar1;
  int iVar2;
  byte *pbVar3;
  HMODULE pHVar4;
  UINT UVar5;
  undefined4 uVar6;
  _STARTUPINFOA local_60;
  undefined1 *local_1c;
  _EXCEPTION_POINTERS *local_18;
  void *pvStack_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_0041d838;
  puStack_10 = &LAB_004134d8;
  pvStack_14 = ExceptionList;
  local_1c = &stack0xffffff88;
  ExceptionList = &pvStack_14;
  DVar1 = GetVersion();
  _DAT_004239ec = DVar1 >> 8 & 0xff;
  _DAT_004239e8 = DVar1 & 0xff;
  _DAT_004239e4 = _DAT_004239e8 * 0x100 + _DAT_004239ec;
  _DAT_004239e0 = DVar1 >> 0x10;
  iVar2 = FUN_00414e1c(1);
  if (iVar2 == 0) {
    FUN_004109fa((undefined *)0x1c);
  }
  iVar2 = FUN_004120cc();
  if (iVar2 == 0) {
    FUN_004109fa((undefined *)0x10);
  }
  local_8 = 0;
  FUN_00414aeb();
  DAT_00425128 = GetCommandLineA();
  DAT_00423a20 = FUN_004149b9();
  FUN_0041476c();
  FUN_004146b3();
  FUN_0040fd46();
  local_60.dwFlags = 0;
  GetStartupInfoA(&local_60);
  pbVar3 = FUN_0041465b();
  uVar6 = 0;
  pHVar4 = GetModuleHandleA((LPCSTR)0x0);
  UVar5 = FUN_00401e50(pHVar4,uVar6,pbVar3);
  FUN_0040fd73(UVar5);
  FUN_00413355(local_18->ExceptionRecord->ExceptionCode,local_18);
  return;
}



// Library Function - Single Match
//  __amsg_exit
// 
// Library: Visual Studio 2003 Release

void __cdecl __amsg_exit(int param_1)

{
  if (DAT_00423a28 == 1) {
    FUN_00414e79();
  }
  FUN_00414eb2((undefined *)param_1);
  (*(code *)PTR___exit_00420920)(0xff);
  return;
}



void __cdecl FUN_004109fa(undefined *param_1)

{
  if (DAT_00423a28 == 1) {
    FUN_00414e79();
  }
  FUN_00414eb2(param_1);
                    // WARNING: Subroutine does not return
  ExitProcess(0xff);
}



int __cdecl FUN_00410a1e(char *param_1)

{
  int iVar1;
  
  FUN_00414084((uint)param_1);
  iVar1 = FUN_00410a40(param_1);
  FUN_004140d6((uint)param_1);
  return iVar1;
}



int __cdecl FUN_00410a40(char *param_1)

{
  uint uVar1;
  uint uVar2;
  byte bVar3;
  DWORD *pDVar4;
  char *pcVar5;
  DWORD DVar6;
  char *pcVar7;
  char *pcVar8;
  char *pcVar9;
  int iVar10;
  int local_c;
  DWORD local_8;
  
  pcVar8 = param_1;
  uVar1 = *(uint *)(param_1 + 0x10);
  if (*(int *)(param_1 + 4) < 0) {
    param_1[4] = '\0';
    param_1[5] = '\0';
    param_1[6] = '\0';
    param_1[7] = '\0';
  }
  local_8 = FUN_00415005(uVar1,0,1);
  if ((int)local_8 < 0) {
LAB_00410ace:
    local_c = -1;
  }
  else {
    uVar2 = *(uint *)(param_1 + 0xc);
    if ((uVar2 & 0x108) == 0) {
      return local_8 - *(int *)(param_1 + 4);
    }
    pcVar5 = *(char **)param_1;
    pcVar7 = *(char **)(param_1 + 8);
    local_c = (int)pcVar5 - (int)pcVar7;
    if ((uVar2 & 3) == 0) {
      if ((uVar2 & 0x80) == 0) {
        pDVar4 = FUN_00410d43();
        *pDVar4 = 0x16;
        goto LAB_00410ace;
      }
    }
    else {
      pcVar9 = pcVar7;
      if ((*(byte *)((&DAT_00423de0)[(int)uVar1 >> 5] + 4 + (uVar1 & 0x1f) * 0x24) & 0x80) != 0) {
        for (; pcVar9 < pcVar5; pcVar9 = pcVar9 + 1) {
          if (*pcVar9 == '\n') {
            local_c = local_c + 1;
          }
        }
      }
    }
    if (local_8 != 0) {
      if ((param_1[0xc] & 1U) != 0) {
        if (*(int *)(param_1 + 4) == 0) {
          local_c = 0;
        }
        else {
          pcVar5 = pcVar5 + (*(int *)(param_1 + 4) - (int)pcVar7);
          iVar10 = (uVar1 & 0x1f) * 0x24;
          if ((*(byte *)(iVar10 + 4 + (&DAT_00423de0)[(int)uVar1 >> 5]) & 0x80) != 0) {
            DVar6 = FUN_00415005(uVar1,0,2);
            if (DVar6 == local_8) {
              pcVar7 = *(char **)(param_1 + 8);
              pcVar9 = pcVar5 + (int)pcVar7;
              param_1 = pcVar5;
              for (; pcVar7 < pcVar9; pcVar7 = pcVar7 + 1) {
                if (*pcVar7 == '\n') {
                  param_1 = param_1 + 1;
                }
              }
              bVar3 = pcVar8[0xd] & 0x20;
            }
            else {
              FUN_00415005(uVar1,local_8,0);
              pcVar8 = (char *)0x200;
              if ((((char *)0x200 < pcVar5) || ((*(uint *)(param_1 + 0xc) & 8) == 0)) ||
                 ((*(uint *)(param_1 + 0xc) & 0x400) != 0)) {
                pcVar8 = *(char **)(param_1 + 0x18);
              }
              bVar3 = *(byte *)(iVar10 + 4 + (&DAT_00423de0)[(int)uVar1 >> 5]) & 4;
              param_1 = pcVar8;
            }
            pcVar5 = param_1;
            if (bVar3 != 0) {
              pcVar5 = param_1 + 1;
            }
          }
          param_1 = pcVar5;
          local_8 = local_8 - (int)param_1;
        }
      }
      local_c = local_c + local_8;
    }
  }
  return local_c;
}



int __cdecl FUN_00410ba1(int *param_1,byte *param_2)

{
  int iVar1;
  int iVar2;
  
  FUN_00414084((uint)param_1);
  iVar1 = FUN_004150dd(param_1);
  iVar2 = FUN_00412826(param_1,param_2,(undefined4 *)&stack0x0000000c);
  FUN_0041516a(iVar1,param_1);
  FUN_004140d6((uint)param_1);
  return iVar2;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

undefined4 * __cdecl FUN_00410bdd(uint param_1,char *param_2)

{
  char cVar1;
  bool bVar2;
  bool bVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  undefined4 *puVar7;
  
  if (((DAT_00423ee0 <= param_1) ||
      (bVar3 = true,
      (*(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + (param_1 & 0x1f) * 0x24) & 1) == 0)) ||
     (puVar7 = FUN_00414593(), puVar7 == (undefined4 *)0x0)) {
    return (undefined4 *)0x0;
  }
  cVar1 = *param_2;
  if (cVar1 != 'a') {
    if (cVar1 == 'r') {
      puVar7[3] = 1;
      goto LAB_00410c43;
    }
    if (cVar1 != 'w') {
      puVar7 = (undefined4 *)0x0;
      goto LAB_00410cbf;
    }
  }
  puVar7[3] = 2;
LAB_00410c43:
  puVar7[3] = puVar7[3] | DAT_00423be4;
  bVar2 = bVar3;
  bVar4 = false;
  bVar6 = false;
LAB_00410c53:
  bVar5 = bVar6;
  cVar1 = param_2[1];
  param_2 = param_2 + 1;
  if ((cVar1 == '\0') || (!bVar2)) goto LAB_00410cb2;
  bVar6 = bVar5;
  if (cVar1 == '+') {
    if ((puVar7[3] & 0x80) == 0) {
      puVar7[3] = puVar7[3] & 0xfffffffc | 0x80;
      goto LAB_00410c53;
    }
  }
  else {
    if (cVar1 == 'b') {
LAB_00410c93:
      bVar6 = bVar3;
      if (bVar5) goto LAB_00410ca5;
      goto LAB_00410c53;
    }
    if (cVar1 == 'c') {
      if (bVar4) goto LAB_00410ca5;
      *(byte *)((int)puVar7 + 0xd) = *(byte *)((int)puVar7 + 0xd) | 0x40;
      bVar4 = bVar3;
      goto LAB_00410c53;
    }
    if (cVar1 == 'n') {
      if (bVar4) goto LAB_00410ca5;
      *(byte *)((int)puVar7 + 0xd) = *(byte *)((int)puVar7 + 0xd) & 0xbf;
      bVar4 = bVar3;
      goto LAB_00410c53;
    }
    if (cVar1 == 't') goto LAB_00410c93;
  }
LAB_00410ca5:
  bVar2 = false;
  bVar6 = bVar5;
  goto LAB_00410c53;
LAB_00410cb2:
  _DAT_00423acc = _DAT_00423acc + 1;
  puVar7[4] = param_1;
LAB_00410cbf:
  FUN_004140d6((uint)puVar7);
  return puVar7;
}



void __cdecl FUN_00410cd0(uint param_1)

{
  DWORD *pDVar1;
  uint *puVar2;
  int iVar3;
  
  pDVar1 = FUN_00410d4c();
  iVar3 = 0;
  *pDVar1 = param_1;
  puVar2 = &DAT_00420928;
  do {
    if (param_1 == *puVar2) {
      pDVar1 = FUN_00410d43();
      *pDVar1 = *(DWORD *)(iVar3 * 8 + 0x42092c);
      return;
    }
    puVar2 = puVar2 + 2;
    iVar3 = iVar3 + 1;
  } while (puVar2 < &DAT_00420a90);
  if ((0x12 < param_1) && (param_1 < 0x25)) {
    pDVar1 = FUN_00410d43();
    *pDVar1 = 0xd;
    return;
  }
  if ((0xbb < param_1) && (param_1 < 0xcb)) {
    pDVar1 = FUN_00410d43();
    *pDVar1 = 8;
    return;
  }
  pDVar1 = FUN_00410d43();
  *pDVar1 = 0x16;
  return;
}



DWORD * FUN_00410d43(void)

{
  DWORD *pDVar1;
  
  pDVar1 = FUN_00412133();
  return pDVar1 + 2;
}



DWORD * FUN_00410d4c(void)

{
  DWORD *pDVar1;
  
  pDVar1 = FUN_00412133();
  return pDVar1 + 3;
}



// Library Function - Single Match
//  _malloc
// 
// Library: Visual Studio 2003 Release

void * __cdecl _malloc(size_t _Size)

{
  void *pvVar1;
  
  pvVar1 = __nh_malloc(_Size,DAT_00423be8);
  return pvVar1;
}



// Library Function - Single Match
//  __nh_malloc
// 
// Library: Visual Studio 2003 Release

void * __cdecl __nh_malloc(size_t _Size,int _NhFlag)

{
  void *pvVar1;
  int iVar2;
  
  if (_Size < 0xffffffe1) {
    do {
      pvVar1 = (void *)FUN_00410d93((uint *)_Size);
      if (pvVar1 != (void *)0x0) {
        return pvVar1;
      }
      if (_NhFlag == 0) {
        return (void *)0x0;
      }
      iVar2 = FUN_00415194(_Size);
    } while (iVar2 != 0);
  }
  return (void *)0x0;
}



void __cdecl FUN_00410d93(uint *param_1)

{
  int *piVar1;
  uint dwBytes;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_0041d848;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  if (DAT_00423dcc == 3) {
    ExceptionList = &local_14;
    if (param_1 <= DAT_00423dc4) {
      ExceptionList = &local_14;
      FUN_004130b6(9);
      local_8 = 0;
      piVar1 = FUN_0041554b(param_1);
      local_8 = 0xffffffff;
      FUN_00410dfa();
      if (piVar1 != (int *)0x0) {
        ExceptionList = local_14;
        return;
      }
    }
  }
  else {
    ExceptionList = &local_14;
    if (DAT_00423dcc == 2) {
      if (param_1 == (uint *)0x0) {
        dwBytes = 0x10;
      }
      else {
        dwBytes = (int)param_1 + 0xfU & 0xfffffff0;
      }
      ExceptionList = &local_14;
      if (dwBytes <= DAT_0042328c) {
        ExceptionList = &local_14;
        FUN_004130b6(9);
        local_8 = 1;
        piVar1 = FUN_00415fee(dwBytes >> 4);
        local_8 = 0xffffffff;
        FUN_00410e59();
        if (piVar1 != (int *)0x0) {
          ExceptionList = local_14;
          return;
        }
      }
      goto LAB_00410e72;
    }
  }
  if (param_1 == (uint *)0x0) {
    param_1 = (uint *)0x1;
  }
  dwBytes = (int)param_1 + 0xfU & 0xfffffff0;
LAB_00410e72:
  HeapAlloc(DAT_00423dc8,0,dwBytes);
  ExceptionList = local_14;
  return;
}



void FUN_00410dfa(void)

{
  FUN_00413117(9);
  return;
}



void FUN_00410e59(void)

{
  FUN_00413117(9);
  return;
}



uint __cdecl FUN_00410e8f(char *param_1,uint param_2,uint param_3,int *param_4)

{
  uint uVar1;
  
  FUN_00414084((uint)param_4);
  uVar1 = FUN_00410ebe(param_1,param_2,param_3,param_4);
  FUN_004140d6((uint)param_4);
  return uVar1;
}



uint __cdecl FUN_00410ebe(char *param_1,uint param_2,uint param_3,int *param_4)

{
  int *piVar1;
  char *pcVar2;
  int iVar3;
  uint uVar4;
  char *pcVar5;
  char *pcVar6;
  char *pcVar7;
  
  piVar1 = param_4;
  pcVar6 = (char *)(param_2 * param_3);
  if (pcVar6 == (char *)0x0) {
    param_3 = 0;
  }
  else {
    pcVar5 = param_1;
    param_1 = pcVar6;
    if ((*(ushort *)(param_4 + 3) & 0x10c) == 0) {
      param_4 = (int *)0x1000;
    }
    else {
      param_4 = (int *)param_4[6];
    }
    do {
      if (((*(ushort *)(piVar1 + 3) & 0x10c) == 0) ||
         (pcVar2 = (char *)piVar1[1], pcVar2 == (char *)0x0)) {
        if (param_1 < param_4) {
          uVar4 = FUN_004163c3(piVar1);
          if (uVar4 == 0xffffffff) goto LAB_00410f9a;
          *pcVar5 = (char)uVar4;
          param_4 = (int *)piVar1[6];
          pcVar5 = pcVar5 + 1;
          param_1 = param_1 + -1;
        }
        else {
          pcVar2 = param_1;
          if (param_4 != (int *)0x0) {
            pcVar2 = param_1 + -((uint)param_1 % (uint)param_4);
          }
          iVar3 = FUN_0041649f(piVar1[4],pcVar5,pcVar2);
          if (iVar3 == 0) {
            piVar1[3] = piVar1[3] | 0x10;
LAB_00410f9a:
            return (uint)((int)pcVar6 - (int)param_1) / param_2;
          }
          if (iVar3 == -1) {
            piVar1[3] = piVar1[3] | 0x20;
            goto LAB_00410f9a;
          }
          param_1 = param_1 + -iVar3;
          pcVar5 = pcVar5 + iVar3;
        }
      }
      else {
        pcVar7 = param_1;
        if (pcVar2 <= param_1) {
          pcVar7 = pcVar2;
        }
        FUN_00411540((undefined4 *)pcVar5,(undefined4 *)*piVar1,(uint)pcVar7);
        param_1 = param_1 + -(int)pcVar7;
        piVar1[1] = piVar1[1] - (int)pcVar7;
        *piVar1 = (int)(pcVar7 + *piVar1);
        pcVar5 = pcVar5 + (int)pcVar7;
      }
    } while (param_1 != (char *)0x0);
  }
  return param_3;
}



void __cdecl FUN_00410fa6(undefined *param_1)

{
  uint *puVar1;
  int local_2c;
  uint *local_28;
  uint local_24;
  uint *local_20;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_0041d860;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  if (param_1 == (undefined *)0x0) {
    return;
  }
  if (DAT_00423dcc == 3) {
    ExceptionList = &local_14;
    FUN_004130b6(9);
    local_8 = 0;
    local_20 = (uint *)FUN_004151f7((int)param_1);
    if (local_20 != (uint *)0x0) {
      FUN_00415222(local_20,(int)param_1);
    }
    local_8 = 0xffffffff;
    FUN_00411010();
    puVar1 = local_20;
  }
  else {
    ExceptionList = &local_14;
    if (DAT_00423dcc != 2) goto LAB_00411072;
    ExceptionList = &local_14;
    FUN_004130b6(9);
    local_8 = 1;
    local_28 = (uint *)FUN_00415f52(param_1,&local_2c,&local_24);
    if (local_28 != (uint *)0x0) {
      FUN_00415fa9(local_2c,local_24,(byte *)local_28);
    }
    local_8 = 0xffffffff;
    FUN_00411068();
    puVar1 = local_28;
  }
  if (puVar1 != (uint *)0x0) {
    ExceptionList = local_14;
    return;
  }
LAB_00411072:
  HeapFree(DAT_00423dc8,0,param_1);
  ExceptionList = local_14;
  return;
}



void FUN_00411010(void)

{
  FUN_00413117(9);
  return;
}



void FUN_00411068(void)

{
  FUN_00413117(9);
  return;
}



int __cdecl FUN_0041108f(int *param_1)

{
  int iVar1;
  
  iVar1 = FUN_004110bd(param_1);
  if (iVar1 != 0) {
    return -1;
  }
  if ((*(byte *)((int)param_1 + 0xd) & 0x40) != 0) {
    iVar1 = FUN_004166dd(param_1[4]);
    return -(uint)(iVar1 != 0);
  }
  return 0;
}



undefined4 __cdecl FUN_004110bd(int *param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  
  uVar2 = 0;
  if ((((byte)param_1[3] & 3) == 2) && ((param_1[3] & 0x108U) != 0)) {
    uVar3 = *param_1 - param_1[2];
    if (0 < (int)uVar3) {
      uVar1 = FUN_00414233(param_1[4],(char *)param_1[2],uVar3);
      if (uVar1 == uVar3) {
        if ((param_1[3] & 0x80U) != 0) {
          param_1[3] = param_1[3] & 0xfffffffd;
        }
      }
      else {
        param_1[3] = param_1[3] | 0x20;
        uVar2 = 0xffffffff;
      }
    }
  }
  param_1[1] = 0;
  *param_1 = param_1[2];
  return uVar2;
}



int __cdecl FUN_00411122(int param_1)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  
  iVar3 = 0;
  iVar5 = 0;
  FUN_004130b6(2);
  iVar4 = 0;
  if (0 < DAT_00424f00) {
    do {
      iVar2 = *(int *)(DAT_00423ee4 + iVar4 * 4);
      if ((iVar2 != 0) && ((*(byte *)(iVar2 + 0xc) & 0x83) != 0)) {
        FUN_004140b3(iVar4,iVar2);
        piVar1 = *(int **)(DAT_00423ee4 + iVar4 * 4);
        if ((piVar1[3] & 0x83U) != 0) {
          if (param_1 == 1) {
            iVar2 = FUN_0041108f(piVar1);
            if (iVar2 != -1) {
              iVar3 = iVar3 + 1;
            }
          }
          else if ((param_1 == 0) && ((piVar1[3] & 2U) != 0)) {
            iVar2 = FUN_0041108f(piVar1);
            if (iVar2 == -1) {
              iVar5 = -1;
            }
          }
        }
        FUN_00414105(iVar4,*(int *)(DAT_00423ee4 + iVar4 * 4));
      }
      iVar4 = iVar4 + 1;
    } while (iVar4 < DAT_00424f00);
  }
  FUN_00413117(2);
  if (param_1 != 1) {
    iVar3 = iVar5;
  }
  return iVar3;
}



uint __cdecl FUN_004111c6(uint param_1,int *param_2)

{
  int *piVar1;
  uint uVar2;
  
  FUN_00414084((uint)param_2);
  piVar1 = param_2 + 1;
  *piVar1 = *piVar1 + -1;
  if (*piVar1 < 0) {
    uVar2 = FUN_0041270e(param_1,param_2);
  }
  else {
    uVar2 = param_1 & 0xff;
    *(undefined1 *)*param_2 = (undefined1)param_1;
    *param_2 = *param_2 + 1;
  }
  FUN_004140d6((uint)param_2);
  return uVar2;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_00411201(void)

{
  void *extraout_ECX;
  
  FUN_00411219();
  _DAT_00423a58 = FUN_004168e4();
  FUN_00416894(extraout_ECX);
  return;
}



void FUN_00411218(void)

{
  return;
}



void FUN_00411219(void)

{
  PTR_FUN_0042335c = &LAB_00416967;
  PTR_FUN_00423358 = __cfltcvt;
  PTR_FUN_00423360 = __fassign;
  PTR_FUN_00423364 = FUN_0041690d;
  PTR_FUN_00423368 = &LAB_004169b5;
  PTR_FUN_0042336c = __cfltcvt;
  return;
}



uint * __cdecl FUN_00411270(uint *param_1,char param_2)

{
  uint uVar1;
  char cVar2;
  uint uVar3;
  uint uVar4;
  uint *puVar5;
  
  while (((uint)param_1 & 3) != 0) {
    uVar1 = *param_1;
    if ((char)uVar1 == param_2) {
      return param_1;
    }
    param_1 = (uint *)((int)param_1 + 1);
    if ((char)uVar1 == '\0') {
      return (uint *)0x0;
    }
  }
  while( true ) {
    while( true ) {
      uVar1 = *param_1;
      uVar4 = uVar1 ^ CONCAT22(CONCAT11(param_2,param_2),CONCAT11(param_2,param_2));
      uVar3 = uVar1 ^ 0xffffffff ^ uVar1 + 0x7efefeff;
      puVar5 = param_1 + 1;
      if (((uVar4 ^ 0xffffffff ^ uVar4 + 0x7efefeff) & 0x81010100) != 0) break;
      param_1 = puVar5;
      if ((uVar3 & 0x81010100) != 0) {
        if ((uVar3 & 0x1010100) != 0) {
          return (uint *)0x0;
        }
        if ((uVar1 + 0x7efefeff & 0x80000000) == 0) {
          return (uint *)0x0;
        }
      }
    }
    uVar1 = *param_1;
    if ((char)uVar1 == param_2) {
      return param_1;
    }
    if ((char)uVar1 == '\0') {
      return (uint *)0x0;
    }
    cVar2 = (char)(uVar1 >> 8);
    if (cVar2 == param_2) {
      return (uint *)((int)param_1 + 1);
    }
    if (cVar2 == '\0') {
      return (uint *)0x0;
    }
    cVar2 = (char)(uVar1 >> 0x10);
    if (cVar2 == param_2) {
      return (uint *)((int)param_1 + 2);
    }
    if (cVar2 == '\0') break;
    cVar2 = (char)(uVar1 >> 0x18);
    if (cVar2 == param_2) {
      return (uint *)((int)param_1 + 3);
    }
    param_1 = puVar5;
    if (cVar2 == '\0') {
      return (uint *)0x0;
    }
  }
  return (uint *)0x0;
}



// Library Function - Single Match
//  _memchr
// 
// Libraries: Visual Studio 1998 Debug, Visual Studio 1998 Release

void * __cdecl _memchr(void *_Buf,int _Val,size_t _MaxCount)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  uint *puVar4;
  char cVar5;
  uint uVar6;
  bool bVar7;
  
  if (_MaxCount == 0) {
    return (void *)0x0;
  }
  uVar6 = _Val & 0xff;
  while (((uint)_Buf & 3) != 0) {
    uVar2 = *(uint *)_Buf;
    _Buf = (void *)((int)_Buf + 1);
    if ((char)uVar2 == (char)_Val) goto LAB_004113be;
    _MaxCount = _MaxCount - 1;
    if (_MaxCount == 0) {
      return (void *)0x0;
    }
  }
  uVar2 = _MaxCount - 4;
  if (3 < _MaxCount) {
    uVar6 = uVar6 * 0x1010101;
    puVar4 = _Buf;
    do {
      _Buf = puVar4 + 1;
      if (((*puVar4 ^ uVar6 ^ 0xffffffff ^ (*puVar4 ^ uVar6) + 0x7efefeff) & 0x81010100) != 0) {
        uVar1 = *puVar4;
        cVar5 = (char)uVar6;
        if ((char)uVar1 == cVar5) {
          return puVar4;
        }
        if ((char)(uVar1 >> 8) == cVar5) {
          return (char *)((int)puVar4 + 1);
        }
        if ((char)(uVar1 >> 0x10) == cVar5) {
          return (char *)((int)puVar4 + 2);
        }
        if ((char)(uVar1 >> 0x18) == cVar5) goto LAB_004113be;
      }
      bVar7 = 3 < uVar2;
      uVar2 = uVar2 - 4;
      puVar4 = _Buf;
    } while (bVar7);
  }
  iVar3 = uVar2 + 4;
  while( true ) {
    if (iVar3 == 0) {
      return (void *)0x0;
    }
    uVar2 = *(uint *)_Buf;
    _Buf = (void *)((int)_Buf + 1);
    if ((char)uVar2 == (char)uVar6) break;
    iVar3 = iVar3 + -1;
  }
LAB_004113be:
  return (char *)((int)_Buf + -1);
}



exception * __thiscall FUN_004113d5(void *this,byte param_1)

{
  exception::~exception(this);
  if ((param_1 & 1) != 0) {
    FUN_0040ef10(this);
  }
  return this;
}



undefined4 * __thiscall FUN_004113f1(void *this,undefined4 *param_1)

{
  size_t sVar1;
  uint *puVar2;
  
  *(undefined ***)this = &PTR_FUN_0041d87c;
  sVar1 = _strlen((char *)*param_1);
  puVar2 = operator_new(sVar1 + 1);
  *(uint **)((int)this + 4) = puVar2;
  if (puVar2 != (uint *)0x0) {
    FUN_00416770(puVar2,(uint *)*param_1);
  }
  *(undefined4 *)((int)this + 8) = 1;
  return this;
}



undefined4 * __thiscall FUN_0041142e(void *this,int param_1)

{
  int iVar1;
  size_t sVar2;
  uint *puVar3;
  
  *(undefined ***)this = &PTR_FUN_0041d87c;
  iVar1 = *(int *)(param_1 + 8);
  *(int *)((int)this + 8) = iVar1;
  if (iVar1 == 0) {
    *(undefined4 *)((int)this + 4) = *(undefined4 *)(param_1 + 4);
  }
  else {
    sVar2 = _strlen(*(char **)(param_1 + 4));
    puVar3 = operator_new(sVar2 + 1);
    *(uint **)((int)this + 4) = puVar3;
    if (puVar3 != (uint *)0x0) {
      FUN_00416770(puVar3,*(uint **)(param_1 + 4));
    }
  }
  return this;
}



// Library Function - Single Match
//  public: virtual __thiscall exception::~exception(void)
// 
// Libraries: Visual Studio 2003 Release, Visual Studio 2005 Release

void __thiscall exception::~exception(exception *this)

{
  *(undefined ***)this = &PTR_FUN_0041d87c;
  if (*(int *)(this + 8) != 0) {
    FUN_0040ef10(*(undefined **)(this + 4));
  }
  return;
}



// Library Function - Single Match
//  _strlen
// 
// Libraries: Visual Studio 1998 Debug, Visual Studio 1998 Release

size_t __cdecl _strlen(char *_Str)

{
  uint uVar1;
  uint *puVar2;
  uint *puVar3;
  
  puVar2 = (uint *)_Str;
  do {
    if (((uint)puVar2 & 3) == 0) goto LAB_004114c0;
    uVar1 = *puVar2;
    puVar2 = (uint *)((int)puVar2 + 1);
  } while ((char)uVar1 != '\0');
LAB_004114f3:
  return (size_t)((int)puVar2 + (-1 - (int)_Str));
LAB_004114c0:
  do {
    do {
      puVar3 = puVar2;
      puVar2 = puVar3 + 1;
    } while (((*puVar3 ^ 0xffffffff ^ *puVar3 + 0x7efefeff) & 0x81010100) == 0);
    uVar1 = *puVar3;
    if ((char)uVar1 == '\0') {
      return (int)puVar3 - (int)_Str;
    }
    if ((char)(uVar1 >> 8) == '\0') {
      return (size_t)((int)puVar3 + (1 - (int)_Str));
    }
    if ((uVar1 & 0xff0000) == 0) {
      return (size_t)((int)puVar3 + (2 - (int)_Str));
    }
  } while ((uVar1 & 0xff000000) != 0);
  goto LAB_004114f3;
}



void FUN_0041151c(void)

{
  undefined1 auStack_c [12];
  
  ExceptionList = auStack_c;
  return;
}



undefined4 * __cdecl FUN_00411540(undefined4 *param_1,undefined4 *param_2,uint param_3)

{
  uint uVar1;
  uint uVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  
  if ((param_2 < param_1) && (param_1 < (undefined4 *)(param_3 + (int)param_2))) {
    puVar3 = (undefined4 *)((param_3 - 4) + (int)param_2);
    puVar4 = (undefined4 *)((param_3 - 4) + (int)param_1);
    if (((uint)puVar4 & 3) == 0) {
      uVar1 = param_3 >> 2;
      uVar2 = param_3 & 3;
      if (7 < uVar1) {
        for (; uVar1 != 0; uVar1 = uVar1 - 1) {
          *puVar4 = *puVar3;
          puVar3 = puVar3 + -1;
          puVar4 = puVar4 + -1;
        }
        switch(uVar2) {
        case 0:
          return param_1;
        case 2:
          goto switchD_004116f7_caseD_2;
        case 3:
          goto switchD_004116f7_caseD_3;
        }
        goto switchD_004116f7_caseD_1;
      }
    }
    else {
      switch(param_3) {
      case 0:
        goto switchD_004116f7_caseD_0;
      case 1:
        goto switchD_004116f7_caseD_1;
      case 2:
        goto switchD_004116f7_caseD_2;
      case 3:
        goto switchD_004116f7_caseD_3;
      default:
        uVar1 = param_3 - ((uint)puVar4 & 3);
        switch((uint)puVar4 & 3) {
        case 1:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          puVar3 = (undefined4 *)((int)puVar3 + -1);
          uVar1 = uVar1 >> 2;
          puVar4 = (undefined4 *)((int)puVar4 - 1);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return param_1;
            case 2:
              goto switchD_004116f7_caseD_2;
            case 3:
              goto switchD_004116f7_caseD_3;
            }
            goto switchD_004116f7_caseD_1;
          }
          break;
        case 2:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          uVar1 = uVar1 >> 2;
          *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
          puVar3 = (undefined4 *)((int)puVar3 + -2);
          puVar4 = (undefined4 *)((int)puVar4 - 2);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return param_1;
            case 2:
              goto switchD_004116f7_caseD_2;
            case 3:
              goto switchD_004116f7_caseD_3;
            }
            goto switchD_004116f7_caseD_1;
          }
          break;
        case 3:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
          uVar1 = uVar1 >> 2;
          *(undefined1 *)((int)puVar4 + 1) = *(undefined1 *)((int)puVar3 + 1);
          puVar3 = (undefined4 *)((int)puVar3 + -3);
          puVar4 = (undefined4 *)((int)puVar4 - 3);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return param_1;
            case 2:
              goto switchD_004116f7_caseD_2;
            case 3:
              goto switchD_004116f7_caseD_3;
            }
            goto switchD_004116f7_caseD_1;
          }
        }
      }
    }
    switch(uVar1) {
    case 7:
      puVar4[7 - uVar1] = puVar3[7 - uVar1];
    case 6:
      puVar4[6 - uVar1] = puVar3[6 - uVar1];
    case 5:
      puVar4[5 - uVar1] = puVar3[5 - uVar1];
    case 4:
      puVar4[4 - uVar1] = puVar3[4 - uVar1];
    case 3:
      puVar4[3 - uVar1] = puVar3[3 - uVar1];
    case 2:
      puVar4[2 - uVar1] = puVar3[2 - uVar1];
    case 1:
      puVar4[1 - uVar1] = puVar3[1 - uVar1];
      puVar3 = puVar3 + -uVar1;
      puVar4 = puVar4 + -uVar1;
    }
    switch(uVar2) {
    case 1:
switchD_004116f7_caseD_1:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      return param_1;
    case 2:
switchD_004116f7_caseD_2:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
      return param_1;
    case 3:
switchD_004116f7_caseD_3:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
      *(undefined1 *)((int)puVar4 + 1) = *(undefined1 *)((int)puVar3 + 1);
      return param_1;
    }
switchD_004116f7_caseD_0:
    return param_1;
  }
  puVar3 = param_1;
  if (((uint)param_1 & 3) == 0) {
    uVar1 = param_3 >> 2;
    uVar2 = param_3 & 3;
    if (7 < uVar1) {
      for (; uVar1 != 0; uVar1 = uVar1 - 1) {
        *puVar3 = *param_2;
        param_2 = param_2 + 1;
        puVar3 = puVar3 + 1;
      }
      switch(uVar2) {
      case 0:
        return param_1;
      case 2:
        goto switchD_00411575_caseD_2;
      case 3:
        goto switchD_00411575_caseD_3;
      }
      goto switchD_00411575_caseD_1;
    }
  }
  else {
    switch(param_3) {
    case 0:
      goto switchD_00411575_caseD_0;
    case 1:
      goto switchD_00411575_caseD_1;
    case 2:
      goto switchD_00411575_caseD_2;
    case 3:
      goto switchD_00411575_caseD_3;
    default:
      uVar1 = (param_3 - 4) + ((uint)param_1 & 3);
      switch((uint)param_1 & 3) {
      case 1:
        uVar2 = uVar1 & 3;
        *(undefined1 *)param_1 = *(undefined1 *)param_2;
        *(undefined1 *)((int)param_1 + 1) = *(undefined1 *)((int)param_2 + 1);
        uVar1 = uVar1 >> 2;
        *(undefined1 *)((int)param_1 + 2) = *(undefined1 *)((int)param_2 + 2);
        param_2 = (undefined4 *)((int)param_2 + 3);
        puVar3 = (undefined4 *)((int)param_1 + 3);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *param_2;
            param_2 = param_2 + 1;
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return param_1;
          case 2:
            goto switchD_00411575_caseD_2;
          case 3:
            goto switchD_00411575_caseD_3;
          }
          goto switchD_00411575_caseD_1;
        }
        break;
      case 2:
        uVar2 = uVar1 & 3;
        *(undefined1 *)param_1 = *(undefined1 *)param_2;
        uVar1 = uVar1 >> 2;
        *(undefined1 *)((int)param_1 + 1) = *(undefined1 *)((int)param_2 + 1);
        param_2 = (undefined4 *)((int)param_2 + 2);
        puVar3 = (undefined4 *)((int)param_1 + 2);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *param_2;
            param_2 = param_2 + 1;
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return param_1;
          case 2:
            goto switchD_00411575_caseD_2;
          case 3:
            goto switchD_00411575_caseD_3;
          }
          goto switchD_00411575_caseD_1;
        }
        break;
      case 3:
        uVar2 = uVar1 & 3;
        *(undefined1 *)param_1 = *(undefined1 *)param_2;
        param_2 = (undefined4 *)((int)param_2 + 1);
        uVar1 = uVar1 >> 2;
        puVar3 = (undefined4 *)((int)param_1 + 1);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *param_2;
            param_2 = param_2 + 1;
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return param_1;
          case 2:
            goto switchD_00411575_caseD_2;
          case 3:
            goto switchD_00411575_caseD_3;
          }
          goto switchD_00411575_caseD_1;
        }
      }
    }
  }
  switch(uVar1) {
  case 7:
    puVar3[uVar1 - 7] = param_2[uVar1 - 7];
  case 6:
    puVar3[uVar1 - 6] = param_2[uVar1 - 6];
  case 5:
    puVar3[uVar1 - 5] = param_2[uVar1 - 5];
  case 4:
    puVar3[uVar1 - 4] = param_2[uVar1 - 4];
  case 3:
    puVar3[uVar1 - 3] = param_2[uVar1 - 3];
  case 2:
    puVar3[uVar1 - 2] = param_2[uVar1 - 2];
  case 1:
    puVar3[uVar1 - 1] = param_2[uVar1 - 1];
    param_2 = param_2 + uVar1;
    puVar3 = puVar3 + uVar1;
  }
  switch(uVar2) {
  case 1:
switchD_00411575_caseD_1:
    *(undefined1 *)puVar3 = *(undefined1 *)param_2;
    return param_1;
  case 2:
switchD_00411575_caseD_2:
    *(undefined1 *)puVar3 = *(undefined1 *)param_2;
    *(undefined1 *)((int)puVar3 + 1) = *(undefined1 *)((int)param_2 + 1);
    return param_1;
  case 3:
switchD_00411575_caseD_3:
    *(undefined1 *)puVar3 = *(undefined1 *)param_2;
    *(undefined1 *)((int)puVar3 + 1) = *(undefined1 *)((int)param_2 + 1);
    *(undefined1 *)((int)puVar3 + 2) = *(undefined1 *)((int)param_2 + 2);
    return param_1;
  }
switchD_00411575_caseD_0:
  return param_1;
}



undefined4 __cdecl
FUN_00411875(PEXCEPTION_RECORD param_1,PVOID param_2,DWORD param_3,undefined4 param_4,int *param_5,
            int param_6,PVOID param_7,char param_8)

{
  code *pcVar1;
  undefined4 uVar2;
  
  if (*param_5 != 0x19930520) {
    FUN_0041229b();
  }
  if ((param_1->ExceptionFlags & 0x66) == 0) {
    if (param_5[3] != 0) {
      if (((param_1->ExceptionCode == 0xe06d7363) && (0x19930520 < param_1->ExceptionInformation[0])
          ) && (pcVar1 = *(code **)(param_1->ExceptionInformation[2] + 8), pcVar1 != (code *)0x0)) {
        uVar2 = (*pcVar1)(param_1,param_2,param_3,param_4,param_5,param_6,param_7,param_8);
        return uVar2;
      }
      FUN_00411910(param_1,param_2,param_3,param_4,(int)param_5,param_8,param_6,param_7);
    }
  }
  else if ((param_5[1] != 0) && (param_6 == 0)) {
    FUN_00411bca((int)param_2,param_4,(int)param_5,-1);
  }
  return 1;
}



void __cdecl
FUN_00411910(PEXCEPTION_RECORD param_1,PVOID param_2,DWORD param_3,undefined4 param_4,int param_5,
            char param_6,int param_7,PVOID param_8)

{
  byte *pbVar1;
  bool bVar2;
  DWORD *pDVar3;
  undefined3 extraout_var;
  int *piVar4;
  int iVar5;
  int *piVar6;
  uint local_1c;
  uint local_18;
  int local_14;
  int local_10;
  int local_c;
  uint local_8;
  
  local_18 = local_18 & 0xffffff00;
  local_14 = *(int *)((int)param_2 + 8);
  if ((local_14 < -1) || (*(int *)(param_5 + 4) <= local_14)) {
    FUN_0041229b();
  }
  if (param_1->ExceptionCode == 0xe06d7363) {
    if (((param_1->NumberParameters == 3) && (param_1->ExceptionInformation[0] == 0x19930520)) &&
       (param_1->ExceptionInformation[2] == 0)) {
      pDVar3 = FUN_00412133();
      if (pDVar3[0x1b] == 0) {
        return;
      }
      pDVar3 = FUN_00412133();
      param_1 = (PEXCEPTION_RECORD)pDVar3[0x1b];
      pDVar3 = FUN_00412133();
      param_3 = pDVar3[0x1c];
      local_18 = CONCAT31(local_18._1_3_,1);
      bVar2 = FUN_00416d33(param_1,1);
      if (CONCAT31(extraout_var,bVar2) == 0) {
        FUN_0041229b();
      }
      if (param_1->ExceptionCode != 0xe06d7363) goto LAB_00411a98;
      if (((param_1->NumberParameters == 3) && (param_1->ExceptionInformation[0] == 0x19930520)) &&
         (param_1->ExceptionInformation[2] == 0)) {
        FUN_0041229b();
      }
    }
    iVar5 = local_14;
    if (((param_1->ExceptionCode == 0xe06d7363) && (param_1->NumberParameters == 3)) &&
       (param_1->ExceptionInformation[0] == 0x19930520)) {
      piVar4 = (int *)FUN_0040f58e(param_5,param_7,local_14,&local_8,&local_1c);
      do {
        if (local_1c <= local_8) {
          if (param_6 == '\0') {
            return;
          }
          FUN_00411fec((int)param_1);
          return;
        }
        if ((*piVar4 <= iVar5) && (iVar5 <= piVar4[1])) {
          pbVar1 = (byte *)piVar4[4];
          for (local_10 = piVar4[3]; iVar5 = local_14, 0 < local_10; local_10 = local_10 + -1) {
            piVar6 = *(int **)(param_1->ExceptionInformation[2] + 0xc);
            for (local_c = *piVar6; 0 < local_c; local_c = local_c + -1) {
              piVar6 = piVar6 + 1;
              iVar5 = FUN_00411b6d(pbVar1,(byte *)*piVar6,(uint *)param_1->ExceptionInformation[2]);
              if (iVar5 != 0) {
                FUN_00411c68(param_1,param_2,param_3,param_4,param_5,pbVar1,(byte *)*piVar6,piVar4,
                             param_7,param_8);
                iVar5 = local_14;
                goto LAB_00411a78;
              }
            }
            pbVar1 = pbVar1 + 0x10;
          }
        }
LAB_00411a78:
        local_8 = local_8 + 1;
        piVar4 = piVar4 + 5;
      } while( true );
    }
  }
LAB_00411a98:
  if (param_6 == '\0') {
    FUN_00411ac3(param_1,param_2,param_3,param_4,param_5,local_14,param_7,param_8);
    return;
  }
  FUN_0041223a();
  return;
}



void __cdecl
FUN_00411ac3(PEXCEPTION_RECORD param_1,PVOID param_2,DWORD param_3,undefined4 param_4,int param_5,
            int param_6,int param_7,PVOID param_8)

{
  DWORD *pDVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  uint local_c;
  uint local_8;
  
  pDVar1 = FUN_00412133();
  if ((pDVar1[0x1a] != 0) &&
     (iVar2 = FUN_0040f463(&param_1->ExceptionCode,param_2,param_3,param_4,param_5,param_7,param_8),
     iVar2 != 0)) {
    return;
  }
  piVar3 = (int *)FUN_0040f58e(param_5,param_7,param_6,&local_8,&local_c);
  for (; local_8 < local_c; local_8 = local_8 + 1) {
    if ((*piVar3 <= param_6) && (param_6 <= piVar3[1])) {
      iVar4 = piVar3[3] * 0x10 + piVar3[4];
      iVar2 = *(int *)(iVar4 + -0xc);
      if ((iVar2 == 0) || (*(char *)(iVar2 + 8) == '\0')) {
        FUN_00411c68(param_1,param_2,param_3,param_4,param_5,(byte *)(iVar4 + -0x10),(byte *)0x0,
                     piVar3,param_7,param_8);
      }
    }
    piVar3 = piVar3 + 5;
  }
  return;
}



undefined4 __cdecl FUN_00411b6d(byte *param_1,byte *param_2,uint *param_3)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = *(int *)(param_1 + 4);
  if ((iVar1 == 0) || (*(char *)(iVar1 + 8) == '\0')) {
LAB_00411bc4:
    uVar2 = 1;
  }
  else {
    if (iVar1 == *(int *)(param_2 + 4)) {
LAB_00411b9e:
      if (((((*param_2 & 2) == 0) || ((*param_1 & 8) != 0)) &&
          (((*param_3 & 1) == 0 || ((*param_1 & 1) != 0)))) &&
         (((*param_3 & 2) == 0 || ((*param_1 & 2) != 0)))) goto LAB_00411bc4;
    }
    else {
      iVar1 = _strcmp((char *)(iVar1 + 8),(char *)(*(int *)(param_2 + 4) + 8));
      if (iVar1 == 0) goto LAB_00411b9e;
    }
    uVar2 = 0;
  }
  return uVar2;
}



void __cdecl FUN_00411bca(int param_1,undefined4 param_2,int param_3,int param_4)

{
  int iVar1;
  int iVar2;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  puStack_c = &DAT_0041d898;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  ExceptionList = &local_14;
  for (iVar2 = *(int *)(param_1 + 8); local_8 = 0xffffffff, iVar2 != param_4;
      iVar2 = *(int *)(*(int *)(param_3 + 8) + iVar2 * 8)) {
    if ((iVar2 < 0) || (*(int *)(param_3 + 4) <= iVar2)) {
      FUN_0041229b();
    }
    local_8 = 0;
    iVar1 = *(int *)(*(int *)(param_3 + 8) + 4 + iVar2 * 8);
    if (iVar1 != 0) {
      __CallSettingFrame_12(iVar1,param_1,0x103);
    }
  }
  *(int *)(param_1 + 8) = iVar2;
  ExceptionList = local_14;
  return;
}



void __cdecl
FUN_00411c68(PEXCEPTION_RECORD param_1,PVOID param_2,DWORD param_3,undefined4 param_4,int param_5,
            byte *param_6,byte *param_7,int *param_8,int param_9,PVOID param_10)

{
  undefined *UNRECOVERED_JUMPTABLE;
  
  if (param_7 != (byte *)0x0) {
    FUN_00411e28((int)param_1,(int)param_2,param_6,param_7);
  }
  if (param_10 == (PVOID)0x0) {
    param_10 = param_2;
  }
  FUN_0040f365(param_10,param_1);
  FUN_00411bca((int)param_2,param_4,param_5,*param_8);
  *(int *)((int)param_2 + 8) = param_8[1] + 1;
  UNRECOVERED_JUMPTABLE =
       (undefined *)
       FUN_00411ce3((DWORD)param_1,param_2,param_3,param_5,*(undefined4 *)(param_6 + 0xc),param_9,
                    0x100);
  if (UNRECOVERED_JUMPTABLE != (undefined *)0x0) {
    FUN_0040f323(UNRECOVERED_JUMPTABLE);
  }
  return;
}



undefined4 __cdecl
FUN_00411ce3(DWORD param_1,undefined4 param_2,DWORD param_3,undefined4 param_4,undefined4 param_5,
            int param_6,int param_7)

{
  DWORD *pDVar1;
  undefined4 uVar2;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_0041d8a8;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  ExceptionList = &local_14;
  FUN_00412133();
  FUN_00412133();
  pDVar1 = FUN_00412133();
  pDVar1[0x1b] = param_1;
  pDVar1 = FUN_00412133();
  pDVar1[0x1c] = param_3;
  local_8 = 1;
  uVar2 = FUN_0040f3ea(param_2,param_4,param_5,param_6,param_7);
  local_8 = 0xffffffff;
  FUN_00411db0();
  ExceptionList = local_14;
  return uVar2;
}



void FUN_00411db0(void)

{
  DWORD *pDVar1;
  int unaff_EBP;
  int unaff_ESI;
  int *unaff_EDI;
  
  *(undefined4 *)(unaff_ESI + -4) = *(undefined4 *)(unaff_EBP + -0x28);
  pDVar1 = FUN_00412133();
  pDVar1[0x1b] = *(DWORD *)(unaff_EBP + -0x1c);
  pDVar1 = FUN_00412133();
  pDVar1[0x1c] = *(DWORD *)(unaff_EBP + -0x20);
  if ((((*unaff_EDI == -0x1f928c9d) && (unaff_EDI[4] == 3)) && (unaff_EDI[5] == 0x19930520)) &&
     ((*(int *)(unaff_EBP + -0x24) == 0 && (*(int *)(unaff_EBP + -0x2c) != 0)))) {
    __abnormal_termination();
    FUN_00411fec((int)unaff_EDI);
  }
  return;
}



void __cdecl FUN_00411e28(int param_1,int param_2,byte *param_3,byte *param_4)

{
  int *piVar1;
  bool bVar2;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  int iVar3;
  undefined3 extraout_var_01;
  undefined3 extraout_var_02;
  undefined3 extraout_var_03;
  undefined3 extraout_var_04;
  undefined4 *puVar4;
  undefined3 extraout_var_05;
  undefined3 extraout_var_06;
  undefined3 extraout_var_07;
  uint uVar5;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  puStack_c = &DAT_0041d8c0;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  if (*(int *)(param_3 + 4) == 0) {
    return;
  }
  if (*(char *)(*(int *)(param_3 + 4) + 8) == '\0') {
    return;
  }
  if (*(int *)(param_3 + 8) == 0) {
    return;
  }
  piVar1 = (int *)(*(int *)(param_3 + 8) + 0xc + param_2);
  local_8 = 0;
  if ((*param_3 & 8) == 0) {
    if ((*param_4 & 1) == 0) {
      if (*(int *)(param_4 + 0x18) == 0) {
        ExceptionList = &local_14;
        bVar2 = FUN_00416d33(*(void **)(param_1 + 0x18),1);
        if ((CONCAT31(extraout_var_03,bVar2) != 0) &&
           (bVar2 = FUN_00416d4f(piVar1,1), CONCAT31(extraout_var_04,bVar2) != 0)) {
          uVar5 = *(uint *)(param_4 + 0x14);
          puVar4 = (undefined4 *)FUN_00412053(*(int *)(param_1 + 0x18),(int *)(param_4 + 8));
          FUN_0040f710(piVar1,puVar4,uVar5);
          ExceptionList = local_14;
          return;
        }
      }
      else {
        ExceptionList = &local_14;
        bVar2 = FUN_00416d33(*(void **)(param_1 + 0x18),1);
        if (((CONCAT31(extraout_var_05,bVar2) != 0) &&
            (bVar2 = FUN_00416d4f(piVar1,1), CONCAT31(extraout_var_06,bVar2) != 0)) &&
           (bVar2 = FUN_00416d6b(*(FARPROC *)(param_4 + 0x18)), CONCAT31(extraout_var_07,bVar2) != 0
           )) {
          if ((*param_4 & 4) != 0) {
            FUN_00412053(*(int *)(param_1 + 0x18),(int *)(param_4 + 8));
            FUN_0040f35e(piVar1,*(undefined **)(param_4 + 0x18));
            ExceptionList = local_14;
            return;
          }
          FUN_00412053(*(int *)(param_1 + 0x18),(int *)(param_4 + 8));
          FUN_0040f357(piVar1,*(undefined **)(param_4 + 0x18));
          ExceptionList = local_14;
          return;
        }
      }
    }
    else {
      ExceptionList = &local_14;
      bVar2 = FUN_00416d33(*(void **)(param_1 + 0x18),1);
      if ((CONCAT31(extraout_var_01,bVar2) != 0) &&
         (bVar2 = FUN_00416d4f(piVar1,1), CONCAT31(extraout_var_02,bVar2) != 0)) {
        FUN_0040f710(piVar1,*(undefined4 **)(param_1 + 0x18),*(uint *)(param_4 + 0x14));
        if (*(int *)(param_4 + 0x14) != 4) {
          ExceptionList = local_14;
          return;
        }
        iVar3 = *piVar1;
        if (iVar3 == 0) {
          ExceptionList = local_14;
          return;
        }
        goto LAB_00411eb6;
      }
    }
  }
  else {
    ExceptionList = &local_14;
    bVar2 = FUN_00416d33(*(void **)(param_1 + 0x18),1);
    if ((CONCAT31(extraout_var,bVar2) != 0) &&
       (bVar2 = FUN_00416d4f(piVar1,1), CONCAT31(extraout_var_00,bVar2) != 0)) {
      iVar3 = *(int *)(param_1 + 0x18);
      *piVar1 = iVar3;
LAB_00411eb6:
      iVar3 = FUN_00412053(iVar3,(int *)(param_4 + 8));
      *piVar1 = iVar3;
      ExceptionList = local_14;
      return;
    }
  }
  FUN_0041229b();
  ExceptionList = local_14;
  return;
}



void __cdecl FUN_00411fec(int param_1)

{
  undefined *UNRECOVERED_JUMPTABLE;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  puStack_c = &DAT_0041d8d0;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  if ((param_1 != 0) &&
     (UNRECOVERED_JUMPTABLE = *(undefined **)(*(int *)(param_1 + 0x1c) + 4),
     UNRECOVERED_JUMPTABLE != (undefined *)0x0)) {
    local_8 = 0;
    ExceptionList = &local_14;
    FUN_0040f357(*(undefined4 *)(param_1 + 0x18),UNRECOVERED_JUMPTABLE);
  }
  ExceptionList = local_14;
  return;
}



int __cdecl FUN_00412053(int param_1,int *param_2)

{
  int iVar1;
  int iVar2;
  
  iVar1 = param_2[1];
  iVar2 = *param_2 + param_1;
  if (-1 < iVar1) {
    iVar2 = iVar2 + *(int *)(*(int *)(iVar1 + param_1) + param_2[2]) + iVar1;
  }
  return iVar2;
}



// WARNING: Restarted to delay deadcode elimination for space: stack
// Library Function - Single Match
//  __CallSettingFrame@12
// 
// Libraries: Visual Studio 2017 Debug, Visual Studio 2017 Release, Visual Studio 2019 Debug, Visual
// Studio 2019 Release

void __CallSettingFrame_12(undefined4 param_1,undefined4 param_2,int param_3)

{
  code *pcVar1;
  
  pcVar1 = (code *)__NLG_Notify1(param_3);
  (*pcVar1)();
  if (param_3 == 0x100) {
    param_3 = 2;
  }
  __NLG_Notify1(param_3);
  return;
}



undefined4 FUN_004120cc(void)

{
  DWORD *lpTlsValue;
  BOOL BVar1;
  DWORD DVar2;
  
  FUN_0041308d();
  DAT_00420ac0 = TlsAlloc();
  if (DAT_00420ac0 != 0xffffffff) {
    lpTlsValue = (DWORD *)FUN_00413218(1,0x74);
    if (lpTlsValue != (DWORD *)0x0) {
      BVar1 = TlsSetValue(DAT_00420ac0,lpTlsValue);
      if (BVar1 != 0) {
        FUN_00412120((int)lpTlsValue);
        DVar2 = GetCurrentThreadId();
        lpTlsValue[1] = 0xffffffff;
        *lpTlsValue = DVar2;
        return 1;
      }
    }
  }
  return 0;
}



void __cdecl FUN_00412120(int param_1)

{
  *(undefined **)(param_1 + 0x50) = &DAT_00420b90;
  *(undefined4 *)(param_1 + 0x14) = 1;
  return;
}



DWORD * FUN_00412133(void)

{
  DWORD dwErrCode;
  DWORD *lpTlsValue;
  BOOL BVar1;
  DWORD DVar2;
  
  dwErrCode = GetLastError();
  lpTlsValue = TlsGetValue(DAT_00420ac0);
  if (lpTlsValue == (DWORD *)0x0) {
    lpTlsValue = (DWORD *)FUN_00413218(1,0x74);
    if (lpTlsValue != (DWORD *)0x0) {
      BVar1 = TlsSetValue(DAT_00420ac0,lpTlsValue);
      if (BVar1 != 0) {
        FUN_00412120((int)lpTlsValue);
        DVar2 = GetCurrentThreadId();
        lpTlsValue[1] = 0xffffffff;
        *lpTlsValue = DVar2;
        goto LAB_0041218e;
      }
    }
    __amsg_exit(0x10);
  }
LAB_0041218e:
  SetLastError(dwErrCode);
  return lpTlsValue;
}



void __cdecl FUN_0041219a(undefined *param_1)

{
  if (DAT_00420ac0 != 0xffffffff) {
    if ((param_1 != (undefined *)0x0) ||
       (param_1 = TlsGetValue(DAT_00420ac0), param_1 != (undefined *)0x0)) {
      if (*(undefined **)(param_1 + 0x24) != (undefined *)0x0) {
        FUN_00410fa6(*(undefined **)(param_1 + 0x24));
      }
      if (*(undefined **)(param_1 + 0x28) != (undefined *)0x0) {
        FUN_00410fa6(*(undefined **)(param_1 + 0x28));
      }
      if (*(undefined **)(param_1 + 0x30) != (undefined *)0x0) {
        FUN_00410fa6(*(undefined **)(param_1 + 0x30));
      }
      if (*(undefined **)(param_1 + 0x38) != (undefined *)0x0) {
        FUN_00410fa6(*(undefined **)(param_1 + 0x38));
      }
      if (*(undefined **)(param_1 + 0x40) != (undefined *)0x0) {
        FUN_00410fa6(*(undefined **)(param_1 + 0x40));
      }
      if (*(undefined **)(param_1 + 0x44) != (undefined *)0x0) {
        FUN_00410fa6(*(undefined **)(param_1 + 0x44));
      }
      if (*(undefined **)(param_1 + 0x50) != &DAT_00420b90) {
        FUN_00410fa6(*(undefined **)(param_1 + 0x50));
      }
      FUN_00410fa6(param_1);
    }
    TlsSetValue(DAT_00420ac0,(LPVOID)0x0);
    return;
  }
  return;
}



void FUN_0041223a(void)

{
  DWORD *pDVar1;
  void *pvStack_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  puStack_c = &DAT_0041d8e0;
  puStack_10 = &LAB_004134d8;
  pvStack_14 = ExceptionList;
  local_8 = 0;
  ExceptionList = &pvStack_14;
  pDVar1 = FUN_00412133();
  if (pDVar1[0x18] != 0) {
    local_8 = 1;
    pDVar1 = FUN_00412133();
    (*(code *)pDVar1[0x18])();
  }
  local_8 = 0xffffffff;
  FUN_00416d83();
  return;
}



void FUN_0041229b(void)

{
  void *pvStack_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  puStack_c = &DAT_0041d8f8;
  puStack_10 = &LAB_004134d8;
  pvStack_14 = ExceptionList;
  ExceptionList = &pvStack_14;
  if (PTR_FUN_00420ac4 != (undefined *)0x0) {
    local_8 = 1;
    ExceptionList = &pvStack_14;
    (*(code *)PTR_FUN_00420ac4)();
  }
  local_8 = 0xffffffff;
  FUN_0041223a();
  return;
}



byte * __cdecl FUN_004122f1(byte *param_1,uint *param_2)

{
  int iVar1;
  uint *puVar2;
  byte *pbVar3;
  int local_3c;
  uint *local_38;
  byte *local_34;
  int *local_30;
  uint *local_2c;
  byte *local_28;
  uint *local_24;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_0041d910;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  pbVar3 = (byte *)0x0;
  if (param_1 == (byte *)0x0) {
    ExceptionList = &local_14;
    pbVar3 = _malloc((size_t)param_2);
  }
  else {
    if (param_2 == (uint *)0x0) {
      ExceptionList = &local_14;
      FUN_00410fa6(param_1);
    }
    else {
      ExceptionList = &local_14;
      if (DAT_00423dcc == 3) {
        do {
          local_28 = (byte *)0x0;
          if (param_2 < (uint *)0xffffffe1) {
            FUN_004130b6(9);
            local_8 = 0;
            local_2c = (uint *)FUN_004151f7((int)param_1);
            if (local_2c != (uint *)0x0) {
              if (param_2 <= DAT_00423dc4) {
                iVar1 = FUN_00415a00(local_2c,(int)param_1,(int)param_2);
                if (iVar1 == 0) {
                  local_28 = (byte *)FUN_0041554b(param_2);
                  if (local_28 != (byte *)0x0) {
                    local_24 = (uint *)(*(int *)(param_1 + -4) - 1);
                    puVar2 = local_24;
                    if (param_2 <= local_24) {
                      puVar2 = param_2;
                    }
                    FUN_00411540((undefined4 *)local_28,(undefined4 *)param_1,(uint)puVar2);
                    local_2c = (uint *)FUN_004151f7((int)param_1);
                    FUN_00415222(local_2c,(int)param_1);
                  }
                }
                else {
                  local_28 = param_1;
                }
              }
              if (local_28 == (byte *)0x0) {
                if (param_2 == (uint *)0x0) {
                  param_2 = (uint *)0x1;
                }
                param_2 = (uint *)((int)param_2 + 0xfU & 0xfffffff0);
                local_28 = HeapAlloc(DAT_00423dc8,0,(SIZE_T)param_2);
                if (local_28 != (byte *)0x0) {
                  local_24 = (uint *)(*(int *)(param_1 + -4) - 1);
                  puVar2 = local_24;
                  if (param_2 <= local_24) {
                    puVar2 = param_2;
                  }
                  FUN_00411540((undefined4 *)local_28,(undefined4 *)param_1,(uint)puVar2);
                  FUN_00415222(local_2c,(int)param_1);
                }
              }
            }
            local_8 = 0xffffffff;
            FUN_0041247c();
            if (local_2c == (uint *)0x0) {
              if (param_2 == (uint *)0x0) {
                param_2 = (uint *)0x1;
              }
              param_2 = (uint *)((int)param_2 + 0xfU & 0xfffffff0);
              local_28 = HeapReAlloc(DAT_00423dc8,0,param_1,(SIZE_T)param_2);
            }
          }
          if (local_28 != (byte *)0x0) {
            ExceptionList = local_14;
            return local_28;
          }
          if (DAT_00423be8 == (byte *)0x0) {
            ExceptionList = local_14;
            return (byte *)0x0;
          }
          iVar1 = FUN_00415194(param_2);
        } while (iVar1 != 0);
      }
      else {
        ExceptionList = &local_14;
        if (DAT_00423dcc == 2) {
          ExceptionList = &local_14;
          if (param_2 < (uint *)0xffffffe1) {
            if (param_2 == (uint *)0x0) {
              param_2 = (uint *)0x10;
              ExceptionList = &local_14;
            }
            else {
              param_2 = (uint *)((int)param_2 + 0xfU & 0xfffffff0);
              ExceptionList = &local_14;
            }
          }
          do {
            local_28 = pbVar3;
            if (param_2 < (uint *)0xffffffe1) {
              FUN_004130b6(9);
              local_8 = 1;
              pbVar3 = (byte *)FUN_00415f52(param_1,&local_3c,(uint *)&local_30);
              local_34 = pbVar3;
              if (pbVar3 == (byte *)0x0) {
                local_28 = HeapReAlloc(DAT_00423dc8,0,param_1,(SIZE_T)param_2);
              }
              else {
                if (param_2 < DAT_0042328c) {
                  iVar1 = FUN_0041631a(local_3c,local_30,pbVar3,(uint)param_2 >> 4);
                  if (iVar1 == 0) {
                    local_28 = (byte *)FUN_00415fee((uint)param_2 >> 4);
                    if (local_28 != (byte *)0x0) {
                      local_38 = (uint *)((uint)*pbVar3 << 4);
                      puVar2 = local_38;
                      if (param_2 <= local_38) {
                        puVar2 = param_2;
                      }
                      FUN_00411540((undefined4 *)local_28,(undefined4 *)param_1,(uint)puVar2);
                      FUN_00415fa9(local_3c,(int)local_30,pbVar3);
                    }
                  }
                  else {
                    local_28 = param_1;
                  }
                }
                if ((local_28 == (byte *)0x0) &&
                   (local_28 = HeapAlloc(DAT_00423dc8,0,(SIZE_T)param_2), local_28 != (byte *)0x0))
                {
                  local_38 = (uint *)((uint)*pbVar3 << 4);
                  puVar2 = local_38;
                  if (param_2 <= local_38) {
                    puVar2 = param_2;
                  }
                  FUN_00411540((undefined4 *)local_28,(undefined4 *)param_1,(uint)puVar2);
                  FUN_00415fa9(local_3c,(int)local_30,pbVar3);
                }
              }
              local_8 = 0xffffffff;
              FUN_004125ca();
            }
            if (local_28 != pbVar3) {
              ExceptionList = local_14;
              return local_28;
            }
            if (DAT_00423be8 == pbVar3) {
              ExceptionList = local_14;
              return local_28;
            }
            iVar1 = FUN_00415194(param_2);
          } while (iVar1 != 0);
        }
        else {
          do {
            pbVar3 = (byte *)0x0;
            if (param_2 < (uint *)0xffffffe1) {
              if (param_2 == (uint *)0x0) {
                param_2 = (uint *)0x1;
              }
              param_2 = (uint *)((int)param_2 + 0xfU & 0xfffffff0);
              pbVar3 = HeapReAlloc(DAT_00423dc8,0,param_1,(SIZE_T)param_2);
            }
            if (pbVar3 != (byte *)0x0) {
              ExceptionList = local_14;
              return pbVar3;
            }
            if (DAT_00423be8 == (byte *)0x0) {
              ExceptionList = local_14;
              return (byte *)0x0;
            }
            iVar1 = FUN_00415194(param_2);
          } while (iVar1 != 0);
        }
      }
    }
    pbVar3 = (byte *)0x0;
  }
  ExceptionList = local_14;
  return pbVar3;
}



void FUN_0041247c(void)

{
  FUN_00413117(9);
  return;
}



void FUN_004125ca(void)

{
  FUN_00413117(9);
  return;
}



SIZE_T __cdecl FUN_00412620(undefined *param_1)

{
  byte *pbVar1;
  SIZE_T SVar2;
  undefined4 local_30;
  byte *local_2c;
  uint local_28;
  SIZE_T local_24;
  byte *local_20;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_0041d928;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  if (DAT_00423dcc == 3) {
    ExceptionList = &local_14;
    FUN_004130b6(9);
    local_8 = 0;
    local_20 = (byte *)FUN_004151f7((int)param_1);
    if (local_20 != (byte *)0x0) {
      local_24 = *(int *)(param_1 + -4) - 9;
    }
    SVar2 = local_24;
    local_8 = 0xffffffff;
    FUN_0041268a();
    pbVar1 = local_20;
  }
  else {
    ExceptionList = &local_14;
    if (DAT_00423dcc != 2) goto LAB_004126de;
    ExceptionList = &local_14;
    FUN_004130b6(9);
    local_8 = 1;
    local_2c = (byte *)FUN_00415f52(param_1,&local_30,&local_28);
    if (local_2c != (byte *)0x0) {
      local_24 = (uint)*local_2c << 4;
    }
    SVar2 = local_24;
    local_8 = 0xffffffff;
    FUN_00412705();
    pbVar1 = local_2c;
  }
  if (pbVar1 != (byte *)0x0) {
    ExceptionList = local_14;
    return SVar2;
  }
LAB_004126de:
  SVar2 = HeapSize(DAT_00423dc8,0,param_1);
  ExceptionList = local_14;
  return SVar2;
}



void FUN_0041268a(void)

{
  FUN_00413117(9);
  return;
}



void FUN_00412705(void)

{
  FUN_00413117(9);
  return;
}



uint __cdecl FUN_0041270e(uint param_1,int *param_2)

{
  uint uVar1;
  uint uVar2;
  char *pcVar3;
  int *piVar4;
  byte bVar5;
  undefined3 extraout_var;
  undefined *puVar6;
  int *piVar7;
  
  piVar4 = param_2;
  uVar1 = param_2[3];
  uVar2 = param_2[4];
  if (((uVar1 & 0x82) == 0) || ((uVar1 & 0x40) != 0)) {
LAB_0041281a:
    param_2[3] = uVar1 | 0x20;
  }
  else {
    if ((uVar1 & 1) != 0) {
      param_2[1] = 0;
      if ((uVar1 & 0x10) == 0) goto LAB_0041281a;
      *param_2 = param_2[2];
      param_2[3] = uVar1 & 0xfffffffe;
    }
    uVar1 = param_2[3];
    param_2[1] = 0;
    param_2 = (int *)0x0;
    piVar4[3] = uVar1 & 0xffffffef | 2;
    if (((uVar1 & 0x10c) == 0) &&
       (((piVar4 != (int *)&DAT_00420f50 && (piVar4 != (int *)&DAT_00420f70)) ||
        (bVar5 = FUN_00416dde(uVar2), CONCAT31(extraout_var,bVar5) == 0)))) {
      FUN_00416d9a(piVar4);
    }
    if ((*(ushort *)(piVar4 + 3) & 0x108) == 0) {
      piVar7 = (int *)0x1;
      param_2 = (int *)FUN_00414233(uVar2,(char *)&param_1,1);
    }
    else {
      pcVar3 = (char *)piVar4[2];
      piVar7 = (int *)(*piVar4 - (int)pcVar3);
      *piVar4 = (int)(pcVar3 + 1);
      piVar4[1] = piVar4[6] + -1;
      if ((int)piVar7 < 1) {
        if (uVar2 == 0xffffffff) {
          puVar6 = &DAT_004211b0;
        }
        else {
          puVar6 = (undefined *)((&DAT_00423de0)[(int)uVar2 >> 5] + (uVar2 & 0x1f) * 0x24);
        }
        if ((puVar6[4] & 0x20) != 0) {
          FUN_00415005(uVar2,0,2);
        }
      }
      else {
        param_2 = (int *)FUN_00414233(uVar2,pcVar3,(uint)piVar7);
      }
      *(undefined1 *)piVar4[2] = (undefined1)param_1;
    }
    if (param_2 == piVar7) {
      return param_1 & 0xff;
    }
    piVar4[3] = piVar4[3] | 0x20;
  }
  return 0xffffffff;
}



int __cdecl FUN_00412826(int *param_1,byte *param_2,undefined4 *param_3)

{
  byte *pbVar1;
  uint uVar2;
  WCHAR *pWVar3;
  undefined4 uVar4;
  short *psVar5;
  int *piVar6;
  int iVar7;
  byte bVar8;
  int iVar9;
  WCHAR *pWVar10;
  undefined1 *puVar11;
  undefined8 uVar12;
  WCHAR local_250 [255];
  undefined1 local_51;
  undefined4 local_50;
  undefined4 local_4c;
  CHAR local_48 [4];
  undefined4 local_44;
  int local_40;
  int local_3c;
  int local_38;
  WCHAR *local_34;
  int local_30;
  int local_2c;
  int local_28;
  undefined8 local_24;
  char local_1a;
  char local_19;
  int local_18;
  undefined1 *local_14;
  WCHAR *local_10;
  int local_c;
  uint local_8;
  
  local_40 = 0;
  local_14 = (undefined1 *)0x0;
  bVar8 = *param_2;
  local_18 = 0;
  local_34 = (WCHAR *)0x0;
  pbVar1 = param_2;
  do {
    if ((bVar8 == 0) || (param_2 = pbVar1 + 1, local_18 < 0)) {
      return local_18;
    }
    if (((char)bVar8 < ' ') || ('x' < (char)bVar8)) {
      uVar2 = 0;
    }
    else {
      uVar2 = (byte)(&DAT_0041d920)[(char)bVar8] & 0xf;
    }
    local_40 = (int)(char)(&DAT_0041d940)[uVar2 * 8 + local_40] >> 4;
    switch(local_40) {
    case 0:
switchD_004128a2_caseD_0:
      local_30 = 0;
      if ((PTR_DAT_00420d10[(uint)bVar8 * 2 + 1] & 0x80) != 0) {
        FUN_00412fc4((int)(char)bVar8,param_1,&local_18);
        bVar8 = *param_2;
        param_2 = pbVar1 + 2;
      }
      FUN_00412fc4((int)(char)bVar8,param_1,&local_18);
      break;
    case 1:
      local_c = -1;
      local_44 = 0;
      local_3c = 0;
      local_2c = 0;
      local_28 = 0;
      local_8 = 0;
      local_30 = 0;
      break;
    case 2:
      if (bVar8 == 0x20) {
        local_8 = local_8 | 2;
      }
      else if (bVar8 == 0x23) {
        local_8 = local_8 | 0x80;
      }
      else if (bVar8 == 0x2b) {
        local_8 = local_8 | 1;
      }
      else if (bVar8 == 0x2d) {
        local_8 = local_8 | 4;
      }
      else if (bVar8 == 0x30) {
        local_8 = local_8 | 8;
      }
      break;
    case 3:
      if (bVar8 == 0x2a) {
        local_2c = FUN_00413062((int *)&param_3);
        if (local_2c < 0) {
          local_8 = local_8 | 4;
          local_2c = -local_2c;
        }
      }
      else {
        local_2c = (char)bVar8 + -0x30 + local_2c * 10;
      }
      break;
    case 4:
      local_c = 0;
      break;
    case 5:
      if (bVar8 == 0x2a) {
        local_c = FUN_00413062((int *)&param_3);
        if (local_c < 0) {
          local_c = -1;
        }
      }
      else {
        local_c = (char)bVar8 + -0x30 + local_c * 10;
      }
      break;
    case 6:
      if (bVar8 == 0x49) {
        if ((*param_2 != 0x36) || (pbVar1[2] != 0x34)) {
          local_40 = 0;
          goto switchD_004128a2_caseD_0;
        }
        param_2 = pbVar1 + 3;
        local_8 = local_8 | 0x8000;
      }
      else if (bVar8 == 0x68) {
        local_8 = local_8 | 0x20;
      }
      else if (bVar8 == 0x6c) {
        local_8 = local_8 | 0x10;
      }
      else if (bVar8 == 0x77) {
        local_8 = local_8 | 0x800;
      }
      break;
    case 7:
      pWVar10 = local_10;
      if ((char)bVar8 < 'h') {
        if ((char)bVar8 < 'e') {
          if ((char)bVar8 < 'Y') {
            if (bVar8 != 0x58) {
              if (bVar8 == 0x43) {
                if ((local_8 & 0x830) == 0) {
                  local_8 = local_8 | 0x800;
                }
                goto LAB_00412af7;
              }
              if ((bVar8 != 0x45) && (bVar8 != 0x47)) {
                if (bVar8 == 0x53) {
                  if ((local_8 & 0x830) == 0) {
                    local_8 = local_8 | 0x800;
                  }
                  goto LAB_00412a67;
                }
                goto LAB_00412e7a;
              }
              local_44 = 1;
              bVar8 = bVar8 + 0x20;
              goto LAB_00412ac9;
            }
LAB_00412cd6:
            local_38 = 7;
LAB_00412cdd:
            local_14 = (undefined1 *)0x10;
            if ((local_8 & 0x80) != 0) {
              local_1a = '0';
              local_19 = (char)local_38 + 'Q';
              local_28 = 2;
            }
            goto LAB_00412d46;
          }
          if (bVar8 == 0x5a) {
            psVar5 = (short *)FUN_00413062((int *)&param_3);
            if ((psVar5 == (short *)0x0) ||
               (pWVar10 = *(WCHAR **)(psVar5 + 2), pWVar10 == (WCHAR *)0x0)) {
              local_10 = (WCHAR *)PTR_DAT_00420ac8;
              pWVar10 = (WCHAR *)PTR_DAT_00420ac8;
              goto LAB_00412c57;
            }
            if ((local_8 & 0x800) == 0) {
              local_30 = 0;
              local_14 = (undefined1 *)(int)*psVar5;
            }
            else {
              local_30 = 1;
              local_14 = (undefined1 *)((uint)(int)*psVar5 >> 1);
            }
          }
          else if (bVar8 == 99) {
LAB_00412af7:
            if ((local_8 & 0x810) == 0) {
              uVar4 = FUN_00413062((int *)&param_3);
              local_250[0]._0_1_ = (undefined1)uVar4;
              local_14 = (undefined1 *)0x1;
            }
            else {
              uVar4 = FUN_0041307f((int *)&param_3);
              local_14 = (undefined1 *)FUN_00416e07((LPSTR)local_250,(WCHAR)uVar4);
              if ((int)local_14 < 0) {
                local_3c = 1;
              }
            }
            pWVar10 = local_250;
          }
          else if (bVar8 == 100) goto LAB_00412d3b;
        }
        else {
LAB_00412ac9:
          local_8 = local_8 | 0x40;
          pWVar10 = local_250;
          pWVar3 = local_250;
          if (local_c < 0) {
            local_c = 6;
          }
          else if (local_c == 0) {
            if (bVar8 == 0x67) {
              local_c = 1;
            }
          }
          else {
            if (0x200 < local_c) {
              local_c = 0x200;
            }
            if ((0xa3 < local_c) &&
               (local_10 = local_250, local_34 = _malloc(local_c + 0x15d), pWVar10 = local_34,
               pWVar3 = local_34, local_34 == (WCHAR *)0x0)) {
              local_c = 0xa3;
              pWVar10 = local_250;
              pWVar3 = local_10;
            }
          }
          local_10 = pWVar3;
          local_50 = *param_3;
          local_4c = param_3[1];
          param_3 = param_3 + 2;
          (*(code *)PTR_FUN_00423358)(&local_50,pWVar10,(int)(char)bVar8,local_c,local_44);
          uVar2 = local_8 & 0x80;
          if ((uVar2 != 0) && (local_c == 0)) {
            (*(code *)PTR_FUN_00423364)(pWVar10);
          }
          if ((bVar8 == 0x67) && (uVar2 == 0)) {
            (*(code *)PTR_FUN_0042335c)(pWVar10);
          }
          if ((char)*pWVar10 == '-') {
            local_8 = local_8 | 0x100;
            pWVar10 = (WCHAR *)((int)pWVar10 + 1);
            local_10 = pWVar10;
          }
LAB_00412c57:
          local_14 = (undefined1 *)_strlen((char *)pWVar10);
          pWVar10 = local_10;
        }
LAB_00412e7a:
        local_10 = pWVar10;
        uVar2 = local_8;
        if (local_3c == 0) {
          if ((local_8 & 0x40) != 0) {
            if ((local_8 & 0x100) == 0) {
              if ((local_8 & 1) == 0) {
                if ((local_8 & 2) == 0) goto LAB_00412eb2;
                local_1a = ' ';
              }
              else {
                local_1a = '+';
              }
            }
            else {
              local_1a = '-';
            }
            local_28 = 1;
          }
LAB_00412eb2:
          iVar9 = (local_2c - local_28) - (int)local_14;
          if ((local_8 & 0xc) == 0) {
            FUN_00412ff9(0x20,iVar9,param_1,&local_18);
          }
          FUN_0041302a(&local_1a,local_28,param_1,&local_18);
          if (((uVar2 & 8) != 0) && ((uVar2 & 4) == 0)) {
            FUN_00412ff9(0x30,iVar9,param_1,&local_18);
          }
          if ((local_30 == 0) || (puVar11 = local_14, pWVar10 = local_10, (int)local_14 < 1)) {
            FUN_0041302a((char *)local_10,(int)local_14,param_1,&local_18);
          }
          else {
            do {
              puVar11 = puVar11 + -1;
              iVar7 = FUN_00416e07(local_48,*pWVar10);
              if (iVar7 < 1) break;
              FUN_0041302a(local_48,iVar7,param_1,&local_18);
              pWVar10 = pWVar10 + 1;
            } while (puVar11 != (undefined1 *)0x0);
          }
          if ((local_8 & 4) != 0) {
            FUN_00412ff9(0x20,iVar9,param_1,&local_18);
          }
        }
      }
      else {
        if (bVar8 == 0x69) {
LAB_00412d3b:
          local_8 = local_8 | 0x40;
LAB_00412d3f:
          local_14 = (undefined1 *)0xa;
LAB_00412d46:
          if ((local_8 & 0x8000) == 0) {
            if ((local_8 & 0x20) == 0) {
              if ((local_8 & 0x40) == 0) {
                uVar2 = FUN_00413062((int *)&param_3);
                local_24 = (ulonglong)uVar2;
                goto LAB_00412d99;
              }
              uVar2 = FUN_00413062((int *)&param_3);
            }
            else if ((local_8 & 0x40) == 0) {
              uVar2 = FUN_00413062((int *)&param_3);
              uVar2 = uVar2 & 0xffff;
            }
            else {
              uVar4 = FUN_00413062((int *)&param_3);
              uVar2 = (uint)(short)uVar4;
            }
            local_24 = (ulonglong)(int)uVar2;
          }
          else {
            local_24 = FUN_0041306f((int *)&param_3);
          }
LAB_00412d99:
          iVar9 = (int)(local_24 >> 0x20);
          if ((((local_8 & 0x40) != 0) && (iVar9 == 0 || (longlong)local_24 < 0)) &&
             ((longlong)local_24 < 0)) {
            local_8 = local_8 | 0x100;
            local_24 = CONCAT44(-(iVar9 + (uint)((int)local_24 != 0)),-(int)local_24);
          }
          if ((local_8 & 0x8000) == 0) {
            local_24 = local_24 & 0xffffffff;
          }
          if (local_c < 0) {
            local_c = 1;
          }
          else {
            local_8 = local_8 & 0xfffffff7;
            if (0x200 < local_c) {
              local_c = 0x200;
            }
          }
          if ((uint)local_24 == 0 && local_24._4_4_ == 0) {
            local_28 = 0;
          }
          local_10 = (WCHAR *)&local_51;
          while( true ) {
            puVar11 = local_14;
            iVar9 = local_c + -1;
            if ((local_c < 1) && ((uint)local_24 == 0 && local_24._4_4_ == 0)) break;
            uVar2 = (int)local_14 >> 0x1f;
            local_c = iVar9;
            uVar12 = __aullrem((uint)local_24,local_24._4_4_,(uint)local_14,uVar2);
            iVar9 = (int)uVar12 + 0x30;
            local_24 = __aulldiv((uint)local_24,local_24._4_4_,(uint)puVar11,uVar2);
            if (0x39 < iVar9) {
              iVar9 = iVar9 + local_38;
            }
            pWVar10 = (WCHAR *)((int)local_10 + -1);
            *(char *)local_10 = (char)iVar9;
            local_10 = pWVar10;
          }
          iVar7 = -(int)local_10;
          local_14 = &local_51 + iVar7;
          pWVar10 = (WCHAR *)((int)local_10 + 1);
          local_c = iVar9;
          if (((local_8 & 0x200) != 0) &&
             ((*(char *)pWVar10 != '0' || (local_14 == (undefined1 *)0x0)))) {
            *(char *)local_10 = '0';
            local_14 = (undefined1 *)((int)&local_50 + iVar7);
            pWVar10 = local_10;
          }
          goto LAB_00412e7a;
        }
        if (bVar8 != 0x6e) {
          if (bVar8 == 0x6f) {
            local_14 = (undefined1 *)0x8;
            if ((local_8 & 0x80) != 0) {
              local_8 = local_8 | 0x200;
            }
            goto LAB_00412d46;
          }
          if (bVar8 == 0x70) {
            local_c = 8;
            goto LAB_00412cd6;
          }
          if (bVar8 == 0x73) {
LAB_00412a67:
            iVar9 = 0x7fffffff;
            if (local_c != -1) {
              iVar9 = local_c;
            }
            pWVar3 = (WCHAR *)FUN_00413062((int *)&param_3);
            if ((local_8 & 0x810) == 0) {
              pWVar10 = pWVar3;
              if (pWVar3 == (WCHAR *)0x0) {
                pWVar3 = (WCHAR *)PTR_DAT_00420ac8;
                pWVar10 = (WCHAR *)PTR_DAT_00420ac8;
              }
              for (; (iVar9 != 0 && ((char)*pWVar3 != '\0')); pWVar3 = (WCHAR *)((int)pWVar3 + 1)) {
                iVar9 = iVar9 + -1;
              }
              local_14 = (undefined1 *)((int)pWVar3 - (int)pWVar10);
            }
            else {
              if (pWVar3 == (WCHAR *)0x0) {
                pWVar3 = (WCHAR *)PTR_DAT_00420acc;
              }
              local_30 = 1;
              for (pWVar10 = pWVar3; (iVar9 != 0 && (*pWVar10 != L'\0')); pWVar10 = pWVar10 + 1) {
                iVar9 = iVar9 + -1;
              }
              local_14 = (undefined1 *)((int)pWVar10 - (int)pWVar3 >> 1);
              pWVar10 = pWVar3;
            }
            goto LAB_00412e7a;
          }
          if (bVar8 != 0x75) {
            if (bVar8 != 0x78) goto LAB_00412e7a;
            local_38 = 0x27;
            goto LAB_00412cdd;
          }
          goto LAB_00412d3f;
        }
        piVar6 = (int *)FUN_00413062((int *)&param_3);
        if ((local_8 & 0x20) == 0) {
          *piVar6 = local_18;
        }
        else {
          *(undefined2 *)piVar6 = (undefined2)local_18;
        }
        local_3c = 1;
      }
      if (local_34 != (WCHAR *)0x0) {
        FUN_00410fa6((undefined *)local_34);
        local_34 = (WCHAR *)0x0;
      }
    }
    bVar8 = *param_2;
    pbVar1 = param_2;
  } while( true );
}



void __cdecl FUN_00412fc4(uint param_1,int *param_2,int *param_3)

{
  int *piVar1;
  uint uVar2;
  
  piVar1 = param_2 + 1;
  *piVar1 = *piVar1 + -1;
  if (*piVar1 < 0) {
    uVar2 = FUN_0041270e(param_1,param_2);
  }
  else {
    *(undefined1 *)*param_2 = (undefined1)param_1;
    *param_2 = *param_2 + 1;
    uVar2 = param_1 & 0xff;
  }
  if (uVar2 == 0xffffffff) {
    *param_3 = -1;
    return;
  }
  *param_3 = *param_3 + 1;
  return;
}



void __cdecl FUN_00412ff9(uint param_1,int param_2,int *param_3,int *param_4)

{
  do {
    if (param_2 < 1) {
      return;
    }
    param_2 = param_2 + -1;
    FUN_00412fc4(param_1,param_3,param_4);
  } while (*param_4 != -1);
  return;
}



void __cdecl FUN_0041302a(char *param_1,int param_2,int *param_3,int *param_4)

{
  char cVar1;
  
  do {
    if (param_2 < 1) {
      return;
    }
    param_2 = param_2 + -1;
    cVar1 = *param_1;
    param_1 = param_1 + 1;
    FUN_00412fc4((int)cVar1,param_3,param_4);
  } while (*param_4 != -1);
  return;
}



undefined4 __cdecl FUN_00413062(int *param_1)

{
  *param_1 = *param_1 + 4;
  return *(undefined4 *)(*param_1 + -4);
}



undefined8 __cdecl FUN_0041306f(int *param_1)

{
  *param_1 = *param_1 + 8;
  return *(undefined8 *)(*param_1 + -8);
}



undefined4 __cdecl FUN_0041307f(int *param_1)

{
  *param_1 = *param_1 + 4;
  return CONCAT22((short)((uint)*param_1 >> 0x10),*(undefined2 *)(*param_1 + -4));
}



void FUN_0041308d(void)

{
  InitializeCriticalSection((LPCRITICAL_SECTION)PTR_DAT_00420b14);
  InitializeCriticalSection((LPCRITICAL_SECTION)PTR_DAT_00420b04);
  InitializeCriticalSection((LPCRITICAL_SECTION)PTR_DAT_00420af4);
  InitializeCriticalSection((LPCRITICAL_SECTION)PTR_DAT_00420ad4);
  return;
}



void __cdecl FUN_004130b6(int param_1)

{
  int *piVar1;
  LPCRITICAL_SECTION lpCriticalSection;
  
  piVar1 = (int *)(&DAT_00420ad0 + param_1 * 4);
  if (*(int *)(&DAT_00420ad0 + param_1 * 4) == 0) {
    lpCriticalSection = _malloc(0x18);
    if (lpCriticalSection == (LPCRITICAL_SECTION)0x0) {
      __amsg_exit(0x11);
    }
    FUN_004130b6(0x11);
    if (*piVar1 == 0) {
      InitializeCriticalSection(lpCriticalSection);
      *piVar1 = (int)lpCriticalSection;
    }
    else {
      FUN_00410fa6((undefined *)lpCriticalSection);
    }
    FUN_00413117(0x11);
  }
  EnterCriticalSection((LPCRITICAL_SECTION)*piVar1);
  return;
}



void __cdecl FUN_00413117(int param_1)

{
  LeaveCriticalSection(*(LPCRITICAL_SECTION *)(&DAT_00420ad0 + param_1 * 4));
  return;
}



// Library Function - Single Match
//  _strcmp
// 
// Libraries: Visual Studio 1998 Debug, Visual Studio 1998 Release

int __cdecl _strcmp(char *_Str1,char *_Str2)

{
  undefined2 uVar1;
  undefined4 uVar2;
  byte bVar3;
  byte bVar4;
  bool bVar5;
  
  if (((uint)_Str1 & 3) != 0) {
    if (((uint)_Str1 & 1) != 0) {
      bVar4 = *_Str1;
      _Str1 = _Str1 + 1;
      bVar5 = bVar4 < (byte)*_Str2;
      if (bVar4 != *_Str2) goto LAB_00413174;
      _Str2 = _Str2 + 1;
      if (bVar4 == 0) {
        return 0;
      }
      if (((uint)_Str1 & 2) == 0) goto LAB_00413140;
    }
    uVar1 = *(undefined2 *)_Str1;
    _Str1 = _Str1 + 2;
    bVar4 = (byte)uVar1;
    bVar5 = bVar4 < (byte)*_Str2;
    if (bVar4 != *_Str2) goto LAB_00413174;
    if (bVar4 == 0) {
      return 0;
    }
    bVar4 = (byte)((ushort)uVar1 >> 8);
    bVar5 = bVar4 < (byte)_Str2[1];
    if (bVar4 != _Str2[1]) goto LAB_00413174;
    if (bVar4 == 0) {
      return 0;
    }
    _Str2 = _Str2 + 2;
  }
LAB_00413140:
  while( true ) {
    uVar2 = *(undefined4 *)_Str1;
    bVar4 = (byte)uVar2;
    bVar5 = bVar4 < (byte)*_Str2;
    if (bVar4 != *_Str2) break;
    if (bVar4 == 0) {
      return 0;
    }
    bVar4 = (byte)((uint)uVar2 >> 8);
    bVar5 = bVar4 < (byte)_Str2[1];
    if (bVar4 != _Str2[1]) break;
    if (bVar4 == 0) {
      return 0;
    }
    bVar4 = (byte)((uint)uVar2 >> 0x10);
    bVar5 = bVar4 < (byte)_Str2[2];
    if (bVar4 != _Str2[2]) break;
    bVar3 = (byte)((uint)uVar2 >> 0x18);
    if (bVar4 == 0) {
      return 0;
    }
    bVar5 = bVar3 < (byte)_Str2[3];
    if (bVar3 != _Str2[3]) break;
    _Str2 = _Str2 + 4;
    _Str1 = _Str1 + 4;
    if (bVar3 == 0) {
      return 0;
    }
  }
LAB_00413174:
  return (uint)bVar5 * -2 + 1;
}



int FUN_004131b4(int *param_1)

{
  int *piVar1;
  bool bVar2;
  int iVar3;
  undefined3 extraout_var;
  
  piVar1 = (int *)*param_1;
  if (((*piVar1 == -0x1f928c9d) && (piVar1[4] == 3)) && (piVar1[5] == 0x19930520)) {
    iVar3 = FUN_0041223a();
    return iVar3;
  }
  if ((DAT_00423ac0 != (FARPROC)0x0) &&
     (bVar2 = FUN_00416d6b(DAT_00423ac0), CONCAT31(extraout_var,bVar2) != 0)) {
    iVar3 = (*DAT_00423ac0)(param_1);
    return iVar3;
  }
  return 0;
}



int * __cdecl FUN_00413218(int param_1,int param_2)

{
  int iVar1;
  uint *puVar2;
  uint *puVar3;
  uint *_Size;
  int *local_24;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_0041d9b8;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  puVar2 = (uint *)(param_1 * param_2);
  puVar3 = puVar2;
  ExceptionList = &local_14;
  if (puVar2 < (uint *)0xffffffe1) {
    if (puVar2 == (uint *)0x0) {
      puVar3 = (uint *)0x1;
    }
    puVar3 = (uint *)((int)puVar3 + 0xfU & 0xfffffff0);
    ExceptionList = &local_14;
  }
  do {
    local_24 = (int *)0x0;
    if (puVar3 < (uint *)0xffffffe1) {
      if (DAT_00423dcc == 3) {
        if (puVar2 <= DAT_00423dc4) {
          FUN_004130b6(9);
          local_8 = 0;
          local_24 = FUN_0041554b(puVar2);
          local_8 = 0xffffffff;
          FUN_004132b1();
          _Size = puVar2;
          if (local_24 == (int *)0x0) goto LAB_00413305;
LAB_004132f4:
          _memset(local_24,0,(size_t)_Size);
        }
LAB_00413300:
        if (local_24 != (int *)0x0) {
          ExceptionList = local_14;
          return local_24;
        }
      }
      else {
        if ((DAT_00423dcc != 2) || (DAT_0042328c < puVar3)) goto LAB_00413300;
        FUN_004130b6(9);
        local_8 = 1;
        local_24 = FUN_00415fee((uint)puVar3 >> 4);
        local_8 = 0xffffffff;
        FUN_0041333a();
        _Size = puVar3;
        if (local_24 != (int *)0x0) goto LAB_004132f4;
      }
LAB_00413305:
      local_24 = HeapAlloc(DAT_00423dc8,8,(SIZE_T)puVar3);
    }
    if (local_24 != (int *)0x0) {
      ExceptionList = local_14;
      return local_24;
    }
    if (DAT_00423be8 == 0) {
      ExceptionList = local_14;
      return (int *)0x0;
    }
    iVar1 = FUN_00415194(puVar3);
    if (iVar1 == 0) {
      ExceptionList = local_14;
      return (int *)0x0;
    }
  } while( true );
}



void FUN_004132b1(void)

{
  FUN_00413117(9);
  return;
}



void FUN_0041333a(void)

{
  FUN_00413117(9);
  return;
}



LONG __cdecl FUN_00413355(int param_1,_EXCEPTION_POINTERS *param_2)

{
  code *pcVar1;
  DWORD DVar2;
  DWORD DVar3;
  DWORD *pDVar4;
  int *piVar5;
  LONG LVar6;
  int iVar7;
  int iVar8;
  
  pDVar4 = FUN_00412133();
  piVar5 = FUN_00413493(param_1,(int *)pDVar4[0x14]);
  if ((piVar5 == (int *)0x0) || (pcVar1 = (code *)piVar5[2], pcVar1 == (code *)0x0)) {
    LVar6 = UnhandledExceptionFilter(param_2);
  }
  else if (pcVar1 == (code *)0x5) {
    piVar5[2] = 0;
    LVar6 = 1;
  }
  else {
    if (pcVar1 != (code *)0x1) {
      DVar2 = pDVar4[0x15];
      pDVar4[0x15] = (DWORD)param_2;
      if (piVar5[1] == 8) {
        if (DAT_00420c08 < DAT_00420c0c + DAT_00420c08) {
          iVar7 = DAT_00420c08 * 0xc;
          iVar8 = DAT_00420c08;
          do {
            *(undefined4 *)(iVar7 + 8 + pDVar4[0x14]) = 0;
            iVar8 = iVar8 + 1;
            iVar7 = iVar7 + 0xc;
          } while (iVar8 < DAT_00420c0c + DAT_00420c08);
        }
        iVar7 = *piVar5;
        DVar3 = pDVar4[0x16];
        if (iVar7 == -0x3fffff72) {
          pDVar4[0x16] = 0x83;
        }
        else if (iVar7 == -0x3fffff70) {
          pDVar4[0x16] = 0x81;
        }
        else if (iVar7 == -0x3fffff6f) {
          pDVar4[0x16] = 0x84;
        }
        else if (iVar7 == -0x3fffff6d) {
          pDVar4[0x16] = 0x85;
        }
        else if (iVar7 == -0x3fffff73) {
          pDVar4[0x16] = 0x82;
        }
        else if (iVar7 == -0x3fffff71) {
          pDVar4[0x16] = 0x86;
        }
        else if (iVar7 == -0x3fffff6e) {
          pDVar4[0x16] = 0x8a;
        }
        (*pcVar1)(8,pDVar4[0x16]);
        pDVar4[0x16] = DVar3;
      }
      else {
        piVar5[2] = 0;
        (*pcVar1)(piVar5[1]);
      }
      pDVar4[0x15] = DVar2;
    }
    LVar6 = -1;
  }
  return LVar6;
}



int * __cdecl FUN_00413493(int param_1,int *param_2)

{
  int *piVar1;
  
  piVar1 = param_2;
  if (*param_2 != param_1) {
    do {
      piVar1 = piVar1 + 3;
      if (param_2 + DAT_00420c14 * 3 <= piVar1) break;
    } while (*piVar1 != param_1);
  }
  if ((param_2 + DAT_00420c14 * 3 <= piVar1) || (*piVar1 != param_1)) {
    piVar1 = (int *)0x0;
  }
  return piVar1;
}



void FUN_00413595(int param_1)

{
  __local_unwind2(*(int *)(param_1 + 0x18),*(int *)(param_1 + 0x1c));
  return;
}



undefined4 __cdecl FUN_004135b0(int param_1)

{
  BYTE *pBVar1;
  byte *pbVar2;
  byte bVar3;
  byte bVar4;
  UINT CodePage;
  UINT *pUVar5;
  BOOL BVar6;
  uint uVar7;
  uint uVar8;
  BYTE *pBVar9;
  int iVar10;
  byte *pbVar11;
  int iVar12;
  byte *pbVar13;
  undefined4 uVar14;
  undefined4 *puVar15;
  _cpinfo local_1c;
  uint local_8;
  
  FUN_004130b6(0x19);
  CodePage = FUN_0041375d(param_1);
  if (CodePage != DAT_00424f04) {
    if (CodePage != 0) {
      iVar12 = 0;
      pUVar5 = &DAT_00420c20;
LAB_004135ed:
      if (*pUVar5 != CodePage) goto code_r0x004135f1;
      local_8 = 0;
      puVar15 = &DAT_00425020;
      for (iVar10 = 0x40; iVar10 != 0; iVar10 = iVar10 + -1) {
        *puVar15 = 0;
        puVar15 = puVar15 + 1;
      }
      iVar12 = iVar12 * 0x30;
      *(undefined1 *)puVar15 = 0;
      pbVar13 = (byte *)(iVar12 + 0x420c30);
      do {
        bVar3 = *pbVar13;
        pbVar11 = pbVar13;
        while ((bVar3 != 0 && (bVar3 = pbVar11[1], bVar3 != 0))) {
          uVar8 = (uint)*pbVar11;
          if (uVar8 <= bVar3) {
            bVar4 = (&DAT_00420c18)[local_8];
            do {
              pbVar2 = (byte *)((int)&DAT_00425020 + uVar8 + 1);
              *pbVar2 = *pbVar2 | bVar4;
              uVar8 = uVar8 + 1;
            } while (uVar8 <= bVar3);
          }
          pbVar11 = pbVar11 + 2;
          bVar3 = *pbVar11;
        }
        local_8 = local_8 + 1;
        pbVar13 = pbVar13 + 8;
      } while (local_8 < 4);
      DAT_00424f1c = 1;
      DAT_00424f04 = CodePage;
      DAT_00425124 = FUN_004137a7(CodePage);
      DAT_00424f10 = *(undefined4 *)(iVar12 + 0x420c24);
      DAT_00424f14 = *(undefined4 *)(iVar12 + 0x420c28);
      DAT_00424f18 = *(undefined4 *)(iVar12 + 0x420c2c);
      goto LAB_00413741;
    }
    goto LAB_0041373c;
  }
  goto LAB_004135d7;
code_r0x004135f1:
  pUVar5 = pUVar5 + 0xc;
  iVar12 = iVar12 + 1;
  if ((UINT *)0x420d0f < pUVar5) goto code_r0x004135fc;
  goto LAB_004135ed;
code_r0x004135fc:
  BVar6 = GetCPInfo(CodePage,&local_1c);
  uVar8 = 1;
  if (BVar6 == 1) {
    DAT_00425124 = 0;
    puVar15 = &DAT_00425020;
    for (iVar12 = 0x40; iVar12 != 0; iVar12 = iVar12 + -1) {
      *puVar15 = 0;
      puVar15 = puVar15 + 1;
    }
    *(undefined1 *)puVar15 = 0;
    if (local_1c.MaxCharSize < 2) {
      DAT_00424f1c = 0;
      DAT_00424f04 = CodePage;
    }
    else {
      DAT_00424f04 = CodePage;
      if (local_1c.LeadByte[0] != '\0') {
        pBVar9 = local_1c.LeadByte + 1;
        do {
          bVar3 = *pBVar9;
          if (bVar3 == 0) break;
          for (uVar7 = (uint)pBVar9[-1]; uVar7 <= bVar3; uVar7 = uVar7 + 1) {
            pbVar13 = (byte *)((int)&DAT_00425020 + uVar7 + 1);
            *pbVar13 = *pbVar13 | 4;
          }
          pBVar1 = pBVar9 + 1;
          pBVar9 = pBVar9 + 2;
        } while (*pBVar1 != 0);
      }
      do {
        pbVar13 = (byte *)((int)&DAT_00425020 + uVar8 + 1);
        *pbVar13 = *pbVar13 | 8;
        uVar8 = uVar8 + 1;
      } while (uVar8 < 0xff);
      DAT_00425124 = FUN_004137a7(CodePage);
      DAT_00424f1c = 1;
    }
    DAT_00424f10 = 0;
    DAT_00424f14 = 0;
    DAT_00424f18 = 0;
  }
  else {
    if (DAT_00423ac4 == 0) {
      uVar14 = 0xffffffff;
      goto LAB_0041374e;
    }
LAB_0041373c:
    FUN_004137da();
  }
LAB_00413741:
  FUN_00413803();
LAB_004135d7:
  uVar14 = 0;
LAB_0041374e:
  FUN_00413117(0x19);
  return uVar14;
}



int __cdecl FUN_0041375d(int param_1)

{
  int iVar1;
  bool bVar2;
  
  if (param_1 == -2) {
    DAT_00423ac4 = 1;
                    // WARNING: Could not recover jumptable at 0x00413777. Too many branches
                    // WARNING: Treating indirect jump as call
    iVar1 = GetOEMCP();
    return iVar1;
  }
  if (param_1 == -3) {
    DAT_00423ac4 = 1;
                    // WARNING: Could not recover jumptable at 0x0041378c. Too many branches
                    // WARNING: Treating indirect jump as call
    iVar1 = GetACP();
    return iVar1;
  }
  bVar2 = param_1 == -4;
  if (bVar2) {
    param_1 = DAT_00423c0c;
  }
  DAT_00423ac4 = (uint)bVar2;
  return param_1;
}



undefined4 __cdecl FUN_004137a7(int param_1)

{
  if (param_1 == 0x3a4) {
    return 0x411;
  }
  if (param_1 == 0x3a8) {
    return 0x804;
  }
  if (param_1 == 0x3b5) {
    return 0x412;
  }
  if (param_1 != 0x3b6) {
    return 0;
  }
  return 0x404;
}



void FUN_004137da(void)

{
  int iVar1;
  undefined4 *puVar2;
  
  puVar2 = &DAT_00425020;
  for (iVar1 = 0x40; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined1 *)puVar2 = 0;
  DAT_00424f04 = 0;
  DAT_00424f1c = 0;
  DAT_00425124 = 0;
  DAT_00424f10 = 0;
  DAT_00424f14 = 0;
  DAT_00424f18 = 0;
  return;
}



void FUN_00413803(void)

{
  byte *pbVar1;
  BOOL BVar2;
  uint uVar3;
  char cVar4;
  uint uVar5;
  uint uVar6;
  ushort *puVar7;
  undefined1 uVar8;
  BYTE *pBVar9;
  CHAR *pCVar10;
  WORD local_518 [256];
  WCHAR local_318 [128];
  WCHAR local_218 [128];
  CHAR local_118 [256];
  _cpinfo local_18;
  
  BVar2 = GetCPInfo(DAT_00424f04,&local_18);
  if (BVar2 == 1) {
    uVar3 = 0;
    do {
      local_118[uVar3] = (CHAR)uVar3;
      uVar3 = uVar3 + 1;
    } while (uVar3 < 0x100);
    local_118[0] = ' ';
    if (local_18.LeadByte[0] != 0) {
      pBVar9 = local_18.LeadByte + 1;
      do {
        uVar3 = (uint)local_18.LeadByte[0];
        if (uVar3 <= *pBVar9) {
          uVar5 = (*pBVar9 - uVar3) + 1;
          uVar6 = uVar5 >> 2;
          pCVar10 = local_118 + uVar3;
          while (uVar6 != 0) {
            uVar6 = uVar6 - 1;
            builtin_memcpy(pCVar10,"    ",4);
            pCVar10 = pCVar10 + 4;
          }
          for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
            *pCVar10 = ' ';
            pCVar10 = pCVar10 + 1;
          }
        }
        local_18.LeadByte[0] = pBVar9[1];
        pBVar9 = pBVar9 + 2;
      } while (local_18.LeadByte[0] != 0);
    }
    FUN_00417018(1,local_118,0x100,local_518,DAT_00424f04,DAT_00425124,0);
    FUN_004139a4(DAT_00425124,0x100,local_118,0x100,local_218,0x100,DAT_00424f04,0);
    FUN_004139a4(DAT_00425124,0x200,local_118,0x100,local_318,0x100,DAT_00424f04,0);
    uVar3 = 0;
    puVar7 = local_518;
    do {
      if ((*puVar7 & 1) == 0) {
        if ((*puVar7 & 2) != 0) {
          pbVar1 = (byte *)((int)&DAT_00425020 + uVar3 + 1);
          *pbVar1 = *pbVar1 | 0x20;
          uVar8 = *(undefined1 *)((int)local_318 + uVar3);
          goto LAB_0041390f;
        }
        (&DAT_00424f20)[uVar3] = 0;
      }
      else {
        pbVar1 = (byte *)((int)&DAT_00425020 + uVar3 + 1);
        *pbVar1 = *pbVar1 | 0x10;
        uVar8 = *(undefined1 *)((int)local_218 + uVar3);
LAB_0041390f:
        (&DAT_00424f20)[uVar3] = uVar8;
      }
      uVar3 = uVar3 + 1;
      puVar7 = puVar7 + 1;
    } while (uVar3 < 0x100);
  }
  else {
    uVar3 = 0;
    do {
      if ((uVar3 < 0x41) || (0x5a < uVar3)) {
        if ((0x60 < uVar3) && (uVar3 < 0x7b)) {
          pbVar1 = (byte *)((int)&DAT_00425020 + uVar3 + 1);
          *pbVar1 = *pbVar1 | 0x20;
          cVar4 = (char)uVar3 + -0x20;
          goto LAB_00413959;
        }
        (&DAT_00424f20)[uVar3] = 0;
      }
      else {
        pbVar1 = (byte *)((int)&DAT_00425020 + uVar3 + 1);
        *pbVar1 = *pbVar1 | 0x10;
        cVar4 = (char)uVar3 + ' ';
LAB_00413959:
        (&DAT_00424f20)[uVar3] = cVar4;
      }
      uVar3 = uVar3 + 1;
    } while (uVar3 < 0x100);
  }
  return;
}



void FUN_00413988(void)

{
  if (DAT_00425130 == 0) {
    FUN_004135b0(-3);
    DAT_00425130 = 1;
  }
  return;
}



int __cdecl
FUN_004139a4(LCID param_1,uint param_2,char *param_3,int param_4,LPWSTR param_5,int param_6,
            UINT param_7,int param_8)

{
  int iVar1;
  int iVar2;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_0041d9d8;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  ExceptionList = &local_14;
  if (DAT_00423ac8 == 0) {
    ExceptionList = &local_14;
    iVar1 = LCMapStringW(0,0x100,L"",1,(LPWSTR)0x0,0);
    if (iVar1 == 0) {
      iVar1 = LCMapStringA(0,0x100,"",1,(LPSTR)0x0,0);
      if (iVar1 == 0) {
        ExceptionList = local_14;
        return 0;
      }
      DAT_00423ac8 = 2;
    }
    else {
      DAT_00423ac8 = 1;
    }
  }
  if (0 < param_4) {
    param_4 = FUN_00413bc8(param_3,param_4);
  }
  if (DAT_00423ac8 == 2) {
    iVar1 = LCMapStringA(param_1,param_2,param_3,param_4,(LPSTR)param_5,param_6);
    ExceptionList = local_14;
    return iVar1;
  }
  if (DAT_00423ac8 == 1) {
    if (param_7 == 0) {
      param_7 = DAT_00423c0c;
    }
    iVar1 = MultiByteToWideChar(param_7,(-(uint)(param_8 != 0) & 8) + 1,param_3,param_4,(LPWSTR)0x0,
                                0);
    if (iVar1 != 0) {
      local_8 = 0;
      FUN_004101e0();
      local_8 = 0xffffffff;
      if ((&stack0x00000000 != (undefined1 *)0x3c) &&
         (iVar2 = MultiByteToWideChar(param_7,1,param_3,param_4,(LPWSTR)&stack0xffffffc4,iVar1),
         iVar2 != 0)) {
        iVar2 = LCMapStringW(param_1,param_2,(LPCWSTR)&stack0xffffffc4,iVar1,(LPWSTR)0x0,0);
        if (iVar2 != 0) {
          if ((param_2 & 0x400) == 0) {
            local_8 = 1;
            FUN_004101e0();
            local_8 = 0xffffffff;
            if (&stack0x00000000 == (undefined1 *)0x3c) {
              ExceptionList = local_14;
              return 0;
            }
            iVar1 = LCMapStringW(param_1,param_2,(LPCWSTR)&stack0xffffffc4,iVar1,
                                 (LPWSTR)&stack0xffffffc4,iVar2);
            if (iVar1 == 0) {
              ExceptionList = local_14;
              return 0;
            }
            if (param_6 == 0) {
              param_6 = 0;
              param_5 = (LPWSTR)0x0;
            }
            iVar2 = WideCharToMultiByte(param_7,0x220,(LPCWSTR)&stack0xffffffc4,iVar2,(LPSTR)param_5
                                        ,param_6,(LPCSTR)0x0,(LPBOOL)0x0);
            iVar1 = iVar2;
          }
          else {
            if (param_6 == 0) {
              ExceptionList = local_14;
              return iVar2;
            }
            if (param_6 < iVar2) {
              ExceptionList = local_14;
              return 0;
            }
            iVar1 = LCMapStringW(param_1,param_2,(LPCWSTR)&stack0xffffffc4,iVar1,param_5,param_6);
          }
          if (iVar1 != 0) {
            ExceptionList = local_14;
            return iVar2;
          }
        }
      }
    }
  }
  ExceptionList = local_14;
  return 0;
}



int __cdecl FUN_00413bc8(char *param_1,int param_2)

{
  char *pcVar1;
  int iVar2;
  
  iVar2 = param_2;
  for (pcVar1 = param_1; (iVar2 != 0 && (iVar2 = iVar2 + -1, *pcVar1 != '\0')); pcVar1 = pcVar1 + 1)
  {
  }
  if (*pcVar1 != '\0') {
    return param_2;
  }
  return (int)pcVar1 - (int)param_1;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

uint __thiscall FUN_00413c00(void *this,byte *param_1,byte *param_2)

{
  bool bVar1;
  int iVar2;
  byte bVar3;
  byte bVar4;
  byte bVar5;
  char cVar6;
  void *extraout_ECX;
  void *this_00;
  void *extraout_ECX_00;
  uint uVar8;
  uint uVar9;
  uint uVar7;
  
  iVar2 = _DAT_00423da8;
  if (DAT_00423bfc == 0) {
    bVar5 = 0xff;
    do {
      do {
        cVar6 = '\0';
        if (bVar5 == 0) goto LAB_00413c4e;
        bVar5 = *param_2;
        param_2 = param_2 + 1;
        bVar4 = *param_1;
        param_1 = param_1 + 1;
      } while (bVar4 == bVar5);
      bVar3 = bVar5 + 0xbf + (-((byte)(bVar5 + 0xbf) < 0x1a) & 0x20U) + 0x41;
      bVar4 = bVar4 + 0xbf;
      bVar5 = bVar4 + (-(bVar4 < 0x1a) & 0x20U) + 0x41;
    } while (bVar5 == bVar3);
    cVar6 = (bVar5 < bVar3) * -2 + '\x01';
LAB_00413c4e:
    uVar7 = (uint)cVar6;
  }
  else {
    LOCK();
    _DAT_00423da8 = _DAT_00423da8 + 1;
    UNLOCK();
    bVar1 = 0 < DAT_00423da4;
    if (bVar1) {
      LOCK();
      UNLOCK();
      _DAT_00423da8 = iVar2;
      FUN_004130b6(0x13);
      this = extraout_ECX;
    }
    uVar9 = (uint)bVar1;
    uVar7 = 0xff;
    uVar8 = 0;
    do {
      do {
        if ((char)uVar7 == '\0') goto LAB_00413caf;
        bVar5 = *param_2;
        uVar7 = CONCAT31((int3)(uVar7 >> 8),bVar5);
        param_2 = param_2 + 1;
        bVar4 = *param_1;
        uVar8 = CONCAT31((int3)(uVar8 >> 8),bVar4);
        param_1 = param_1 + 1;
      } while (bVar5 == bVar4);
      uVar8 = FUN_004171d0(this,uVar8);
      uVar7 = FUN_004171d0(this_00,uVar7);
      this = extraout_ECX_00;
    } while ((byte)uVar8 == (byte)uVar7);
    uVar8 = (uint)((byte)uVar8 < (byte)uVar7);
    uVar7 = (1 - uVar8) - (uint)(uVar8 != 0);
LAB_00413caf:
    if (uVar9 == 0) {
      LOCK();
      _DAT_00423da8 = _DAT_00423da8 + -1;
      UNLOCK();
    }
    else {
      FUN_00413117(0x13);
    }
  }
  return uVar7;
}



byte * __cdecl FUN_00413cd0(byte *param_1,byte *param_2,uint param_3)

{
  byte bVar1;
  byte bVar2;
  uint uVar3;
  uint uVar4;
  byte *pbVar5;
  byte *pbVar6;
  
  if (DAT_00424f1c == 0) {
    param_1 = (byte *)_strncpy((char *)param_1,(char *)param_2,param_3);
  }
  else {
    FUN_004130b6(0x19);
    uVar4 = 0;
    pbVar6 = param_1;
    pbVar5 = param_1;
    if (param_3 != 0) {
      do {
        bVar1 = *param_2;
        uVar4 = param_3 - 1;
        bVar2 = *(byte *)((int)&DAT_00425020 + bVar1 + 1);
        *pbVar5 = bVar1;
        if ((bVar2 & 4) == 0) {
          pbVar6 = pbVar5 + 1;
          param_2 = param_2 + 1;
          if (bVar1 == 0) goto LAB_00413d41;
        }
        else {
          pbVar6 = pbVar5 + 1;
          if (uVar4 == 0) {
            *pbVar5 = 0;
            goto LAB_00413d41;
          }
          bVar1 = param_2[1];
          uVar4 = param_3 - 2;
          *pbVar6 = bVar1;
          pbVar6 = pbVar5 + 2;
          param_2 = param_2 + 2;
          if (bVar1 == 0) {
            *pbVar5 = 0;
            goto LAB_00413d41;
          }
        }
        param_3 = uVar4;
        pbVar5 = pbVar6;
      } while (uVar4 != 0);
      uVar4 = 0;
    }
LAB_00413d41:
    if (uVar4 != 0) {
      for (uVar3 = uVar4 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
        pbVar6[0] = 0;
        pbVar6[1] = 0;
        pbVar6[2] = 0;
        pbVar6[3] = 0;
        pbVar6 = pbVar6 + 4;
      }
      for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *pbVar6 = 0;
        pbVar6 = pbVar6 + 1;
      }
    }
    FUN_00413117(0x19);
  }
  return param_1;
}



byte * __cdecl FUN_00413d6a(byte *param_1,byte *param_2)

{
  byte *pbVar1;
  
  if (param_1 < param_2) {
    if (DAT_00424f1c == 0) {
      pbVar1 = param_2 + -1;
    }
    else {
      FUN_004130b6(0x19);
      pbVar1 = param_2 + -1;
      if ((*(byte *)((int)&DAT_00425020 + param_2[-1] + 1) & 4) == 0) {
        do {
          pbVar1 = pbVar1 + -1;
          if (pbVar1 < param_1) break;
        } while ((*(byte *)((int)&DAT_00425020 + *pbVar1 + 1) & 4) != 0);
        FUN_00413117(0x19);
        pbVar1 = param_2 + (-1 - ((int)param_2 - (int)pbVar1 & 1U));
      }
      else {
        FUN_00413117(0x19);
        pbVar1 = param_2 + -2;
      }
    }
  }
  else {
    pbVar1 = (byte *)0x0;
  }
  return pbVar1;
}



uint __thiscall FUN_00413dd7(void *this,int param_1,uint param_2)

{
  BOOL BVar1;
  int iVar2;
  undefined4 local_8;
  
  if (param_1 + 1U < 0x101) {
    param_1._2_2_ = *(ushort *)(PTR_DAT_00420d10 + param_1 * 2);
  }
  else {
    if ((PTR_DAT_00420d10[(param_1 >> 8 & 0xffU) * 2 + 1] & 0x80) == 0) {
      local_8 = CONCAT31((int3)((uint)this >> 8),(char)param_1) & 0xffff00ff;
      iVar2 = 1;
    }
    else {
      local_8._0_2_ = CONCAT11((char)param_1,(char)((uint)param_1 >> 8));
      local_8 = CONCAT22((short)((uint)this >> 0x10),(undefined2)local_8) & 0xff00ffff;
      iVar2 = 2;
    }
    BVar1 = FUN_00417018(1,(LPCSTR)&local_8,iVar2,(LPWORD)((int)&param_1 + 2),0,0,1);
    if (BVar1 == 0) {
      return 0;
    }
  }
  return param_1._2_2_ & param_2;
}



uint __cdecl FUN_00413e4c(uint param_1)

{
  void *extraout_ECX;
  bool bVar1;
  void *this;
  
  if (DAT_00423bfc == 0) {
    if ((0x60 < (int)param_1) && ((int)param_1 < 0x7b)) {
      return param_1 - 0x20;
    }
  }
  else {
    InterlockedIncrement((LONG *)&DAT_00423da8);
    bVar1 = DAT_00423da4 != 0;
    this = extraout_ECX;
    if (bVar1) {
      InterlockedDecrement((LONG *)&DAT_00423da8);
      this = (void *)0x13;
      FUN_004130b6(0x13);
    }
    param_1 = FUN_00413ebb(this,param_1);
    if (bVar1) {
      FUN_00413117(0x13);
    }
    else {
      InterlockedDecrement((LONG *)&DAT_00423da8);
    }
  }
  return param_1;
}



uint __thiscall FUN_00413ebb(void *this,uint param_1)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  void *local_8;
  
  uVar1 = param_1;
  if (DAT_00423bfc == 0) {
    if ((0x60 < (int)param_1) && ((int)param_1 < 0x7b)) {
      uVar1 = param_1 - 0x20;
    }
  }
  else {
    local_8 = this;
    if ((int)param_1 < 0x100) {
      if (DAT_00420f1c < 2) {
        uVar2 = (byte)PTR_DAT_00420d10[param_1 * 2] & 2;
      }
      else {
        uVar2 = FUN_00413dd7(this,param_1,2);
      }
      if (uVar2 == 0) {
        return uVar1;
      }
    }
    if ((PTR_DAT_00420d10[((int)uVar1 >> 8 & 0xffU) * 2 + 1] & 0x80) == 0) {
      param_1 = CONCAT31((int3)(param_1 >> 8),(char)uVar1) & 0xffff00ff;
      iVar3 = 1;
    }
    else {
      uVar2 = param_1 >> 0x10;
      param_1._0_2_ = CONCAT11((char)uVar1,(char)(uVar1 >> 8));
      param_1 = CONCAT22((short)uVar2,(undefined2)param_1) & 0xff00ffff;
      iVar3 = 2;
    }
    iVar3 = FUN_004139a4(DAT_00423bfc,0x200,(char *)&param_1,iVar3,(LPWSTR)&local_8,3,0,1);
    if (iVar3 != 0) {
      if (iVar3 == 1) {
        uVar1 = (uint)local_8 & 0xff;
      }
      else {
        uVar1 = (uint)local_8 & 0xffff;
      }
    }
  }
  return uVar1;
}



// Library Function - Single Match
//  _strncmp
// 
// Libraries: Visual Studio 1998 Debug, Visual Studio 1998 Release

int __cdecl _strncmp(char *_Str1,char *_Str2,size_t _MaxCount)

{
  char cVar1;
  char cVar2;
  size_t sVar3;
  int iVar4;
  uint uVar5;
  char *pcVar6;
  char *pcVar7;
  
  uVar5 = 0;
  sVar3 = _MaxCount;
  pcVar6 = _Str1;
  if (_MaxCount != 0) {
    do {
      if (sVar3 == 0) break;
      sVar3 = sVar3 - 1;
      cVar1 = *pcVar6;
      pcVar6 = pcVar6 + 1;
    } while (cVar1 != '\0');
    iVar4 = _MaxCount - sVar3;
    do {
      pcVar6 = _Str2;
      pcVar7 = _Str1;
      if (iVar4 == 0) break;
      iVar4 = iVar4 + -1;
      pcVar7 = _Str1 + 1;
      pcVar6 = _Str2 + 1;
      cVar2 = *_Str1;
      cVar1 = *_Str2;
      _Str2 = pcVar6;
      _Str1 = pcVar7;
    } while (cVar1 == cVar2);
    uVar5 = 0;
    if ((byte)pcVar6[-1] <= (byte)pcVar7[-1]) {
      if (pcVar6[-1] == pcVar7[-1]) {
        return 0;
      }
      uVar5 = 0xfffffffe;
    }
    uVar5 = ~uVar5;
  }
  return uVar5;
}



void __cdecl FUN_00414084(uint param_1)

{
  if ((0x420f2f < param_1) && (param_1 < 0x421191)) {
    FUN_004130b6(((int)(param_1 - 0x420f30) >> 5) + 0x1c);
    return;
  }
  EnterCriticalSection((LPCRITICAL_SECTION)(param_1 + 0x20));
  return;
}



void __cdecl FUN_004140b3(int param_1,int param_2)

{
  if (param_1 < 0x14) {
    FUN_004130b6(param_1 + 0x1c);
    return;
  }
  EnterCriticalSection((LPCRITICAL_SECTION)(param_2 + 0x20));
  return;
}



void __cdecl FUN_004140d6(uint param_1)

{
  if ((0x420f2f < param_1) && (param_1 < 0x421191)) {
    FUN_00413117(((int)(param_1 - 0x420f30) >> 5) + 0x1c);
    return;
  }
  LeaveCriticalSection((LPCRITICAL_SECTION)(param_1 + 0x20));
  return;
}



void __cdecl FUN_00414105(int param_1,int param_2)

{
  if (param_1 < 0x14) {
    FUN_00413117(param_1 + 0x1c);
    return;
  }
  LeaveCriticalSection((LPCRITICAL_SECTION)(param_2 + 0x20));
  return;
}



undefined4 __cdecl FUN_00414128(uint param_1)

{
  undefined4 uVar1;
  DWORD *pDVar2;
  
  if ((param_1 < DAT_00423ee0) &&
     ((*(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + (param_1 & 0x1f) * 0x24) & 1) != 0)) {
    FUN_0041767f(param_1);
    uVar1 = FUN_00414185(param_1);
    FUN_004176de(param_1);
    return uVar1;
  }
  pDVar2 = FUN_00410d43();
  *pDVar2 = 9;
  pDVar2 = FUN_00410d4c();
  *pDVar2 = 0;
  return 0xffffffff;
}



undefined4 __cdecl FUN_00414185(uint param_1)

{
  int iVar1;
  int iVar2;
  HANDLE hObject;
  BOOL BVar3;
  DWORD DVar4;
  undefined4 uVar5;
  
  iVar1 = FUN_0041763d(param_1);
  if (iVar1 != -1) {
    if ((param_1 == 1) || (param_1 == 2)) {
      iVar1 = FUN_0041763d(2);
      iVar2 = FUN_0041763d(1);
      if (iVar2 == iVar1) goto LAB_004141d3;
    }
    hObject = (HANDLE)FUN_0041763d(param_1);
    BVar3 = CloseHandle(hObject);
    if (BVar3 == 0) {
      DVar4 = GetLastError();
      goto LAB_004141d5;
    }
  }
LAB_004141d3:
  DVar4 = 0;
LAB_004141d5:
  FUN_004175be(param_1);
  *(undefined1 *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + (param_1 & 0x1f) * 0x24) = 0;
  if (DVar4 == 0) {
    uVar5 = 0;
  }
  else {
    FUN_00410cd0(DVar4);
    uVar5 = 0xffffffff;
  }
  return uVar5;
}



// Library Function - Single Match
//  __freebuf
// 
// Library: Visual Studio 2003 Release

void __cdecl __freebuf(FILE *_File)

{
  if (((_File->_flag & 0x83U) != 0) && ((_File->_flag & 8U) != 0)) {
    FUN_00410fa6(_File->_base);
    *(ushort *)&_File->_flag = (ushort)_File->_flag & 0xfbf7;
    _File->_ptr = (char *)0x0;
    _File->_base = (char *)0x0;
    _File->_cnt = 0;
  }
  return;
}



int __cdecl FUN_00414233(uint param_1,char *param_2,uint param_3)

{
  int iVar1;
  DWORD *pDVar2;
  
  if ((param_1 < DAT_00423ee0) &&
     ((*(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + (param_1 & 0x1f) * 0x24) & 1) != 0)) {
    FUN_0041767f(param_1);
    iVar1 = FUN_00414298(param_1,param_2,param_3);
    FUN_004176de(param_1);
    return iVar1;
  }
  pDVar2 = FUN_00410d43();
  *pDVar2 = 9;
  pDVar2 = FUN_00410d4c();
  *pDVar2 = 0;
  return -1;
}



int __cdecl FUN_00414298(DWORD param_1,char *param_2,uint param_3)

{
  int *piVar1;
  char *pcVar2;
  char cVar3;
  int iVar4;
  char *pcVar5;
  BOOL BVar6;
  DWORD *pDVar7;
  char local_418 [1028];
  int local_14;
  DWORD local_10;
  DWORD local_c;
  char *local_8;
  
  local_c = 0;
  local_14 = 0;
  if (param_3 == 0) {
LAB_004142b1:
    iVar4 = 0;
  }
  else {
    piVar1 = &DAT_00423de0 + ((int)param_1 >> 5);
    iVar4 = (param_1 & 0x1f) * 0x24;
    if ((*(byte *)(*piVar1 + 4 + iVar4) & 0x20) != 0) {
      FUN_0041506a(param_1,0,2);
    }
    if ((*(byte *)((undefined4 *)(*piVar1 + iVar4) + 1) & 0x80) == 0) {
      BVar6 = WriteFile(*(HANDLE *)(*piVar1 + iVar4),param_2,param_3,&local_10,(LPOVERLAPPED)0x0);
      if (BVar6 == 0) {
        param_1 = GetLastError();
      }
      else {
        local_c = local_10;
        param_1 = 0;
      }
LAB_00414380:
      if (local_c != 0) {
        return local_c - local_14;
      }
      if (param_1 == 0) goto LAB_004143f2;
      if (param_1 == 5) {
        pDVar7 = FUN_00410d43();
        *pDVar7 = 9;
        pDVar7 = FUN_00410d4c();
        *pDVar7 = 5;
      }
      else {
        FUN_00410cd0(param_1);
      }
    }
    else {
      local_8 = param_2;
      param_1 = 0;
      if (param_3 != 0) {
        do {
          pcVar5 = local_418;
          do {
            if (param_3 <= (uint)((int)local_8 - (int)param_2)) break;
            pcVar2 = local_8 + 1;
            cVar3 = *local_8;
            local_8 = pcVar2;
            if (cVar3 == '\n') {
              local_14 = local_14 + 1;
              *pcVar5 = '\r';
              pcVar5 = pcVar5 + 1;
            }
            *pcVar5 = cVar3;
            pcVar5 = pcVar5 + 1;
          } while ((int)pcVar5 - (int)local_418 < 0x400);
          BVar6 = WriteFile(*(HANDLE *)(*piVar1 + iVar4),local_418,(int)pcVar5 - (int)local_418,
                            &local_10,(LPOVERLAPPED)0x0);
          if (BVar6 == 0) {
            param_1 = GetLastError();
            goto LAB_00414380;
          }
          local_c = local_c + local_10;
          if (((int)local_10 < (int)pcVar5 - (int)local_418) ||
             (param_3 <= (uint)((int)local_8 - (int)param_2))) goto LAB_00414380;
        } while( true );
      }
LAB_004143f2:
      if (((*(byte *)(*piVar1 + 4 + iVar4) & 0x40) != 0) && (*param_2 == '\x1a')) goto LAB_004142b1;
      pDVar7 = FUN_00410d43();
      *pDVar7 = 0x1c;
      pDVar7 = FUN_00410d4c();
      *pDVar7 = 0;
    }
    iVar4 = -1;
  }
  return iVar4;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

undefined4 * __cdecl FUN_00414423(LPCSTR param_1,char *param_2,uint param_3,undefined4 *param_4)

{
  char cVar1;
  bool bVar2;
  bool bVar3;
  bool bVar4;
  uint uVar5;
  uint uVar6;
  
  bVar4 = false;
  bVar3 = false;
  cVar1 = *param_2;
  if (cVar1 == 'a') {
    uVar5 = 0x109;
  }
  else {
    if (cVar1 == 'r') {
      uVar5 = 0;
      uVar6 = DAT_00423be4 | 1;
      goto LAB_00414464;
    }
    if (cVar1 != 'w') {
      return (undefined4 *)0x0;
    }
    uVar5 = 0x301;
  }
  uVar6 = DAT_00423be4 | 2;
LAB_00414464:
  bVar2 = true;
LAB_00414467:
  cVar1 = param_2[1];
  param_2 = param_2 + 1;
  if ((cVar1 == '\0') || (!bVar2)) {
    uVar5 = FUN_00417700(param_1,uVar5,param_3,0x1a4);
    if ((int)uVar5 < 0) {
      return (undefined4 *)0x0;
    }
    _DAT_00423acc = _DAT_00423acc + 1;
    param_4[3] = uVar6;
    param_4[1] = 0;
    *param_4 = 0;
    param_4[2] = 0;
    param_4[7] = 0;
    param_4[4] = uVar5;
    return param_4;
  }
  if (cVar1 < 'U') {
    if (cVar1 == 'T') {
      if ((uVar5 & 0x1000) == 0) {
        uVar5 = uVar5 | 0x1000;
        goto LAB_00414467;
      }
    }
    else if (cVar1 == '+') {
      if ((uVar5 & 2) == 0) {
        uVar5 = uVar5 & 0xfffffffe | 2;
        uVar6 = uVar6 & 0xfffffffc | 0x80;
        goto LAB_00414467;
      }
    }
    else if (cVar1 == 'D') {
      if ((uVar5 & 0x40) == 0) {
        uVar5 = uVar5 | 0x40;
        goto LAB_00414467;
      }
    }
    else if (cVar1 == 'R') {
      if (!bVar3) {
        bVar3 = true;
        uVar5 = uVar5 | 0x10;
        goto LAB_00414467;
      }
    }
    else if ((cVar1 == 'S') && (!bVar3)) {
      bVar3 = true;
      uVar5 = uVar5 | 0x20;
      goto LAB_00414467;
    }
  }
  else {
    if (cVar1 == 'b') {
      if ((uVar5 & 0xc000) != 0) goto LAB_00414547;
      uVar5 = uVar5 | 0x8000;
      goto LAB_00414467;
    }
    if (cVar1 == 'c') {
      if (!bVar4) {
        bVar4 = true;
        uVar6 = uVar6 | 0x4000;
        goto LAB_00414467;
      }
    }
    else {
      if (cVar1 != 'n') {
        if ((cVar1 != 't') || ((uVar5 & 0xc000) != 0)) goto LAB_00414547;
        uVar5 = uVar5 | 0x4000;
        goto LAB_00414467;
      }
      if (!bVar4) {
        bVar4 = true;
        uVar6 = uVar6 & 0xffffbfff;
        goto LAB_00414467;
      }
    }
  }
LAB_00414547:
  bVar2 = false;
  goto LAB_00414467;
}



undefined4 * FUN_00414593(void)

{
  int iVar1;
  void *pvVar2;
  int iVar3;
  undefined4 *puVar4;
  
  puVar4 = (undefined4 *)0x0;
  FUN_004130b6(2);
  iVar3 = 0;
  if (0 < DAT_00424f00) {
    do {
      iVar1 = *(int *)(DAT_00423ee4 + iVar3 * 4);
      if (iVar1 == 0) {
        iVar3 = iVar3 * 4;
        pvVar2 = _malloc(0x38);
        *(void **)(iVar3 + DAT_00423ee4) = pvVar2;
        if (*(int *)(iVar3 + DAT_00423ee4) != 0) {
          InitializeCriticalSection((LPCRITICAL_SECTION)(*(int *)(iVar3 + DAT_00423ee4) + 0x20));
          EnterCriticalSection((LPCRITICAL_SECTION)(*(int *)(iVar3 + DAT_00423ee4) + 0x20));
          puVar4 = *(undefined4 **)(iVar3 + DAT_00423ee4);
LAB_00414637:
          if (puVar4 != (undefined4 *)0x0) {
            puVar4[4] = 0xffffffff;
            puVar4[1] = 0;
            puVar4[3] = 0;
            puVar4[2] = 0;
            *puVar4 = 0;
            puVar4[7] = 0;
          }
        }
        break;
      }
      if ((*(byte *)(iVar1 + 0xc) & 0x83) == 0) {
        FUN_004140b3(iVar3,iVar1);
        iVar1 = *(int *)(DAT_00423ee4 + iVar3 * 4);
        if ((*(byte *)(iVar1 + 0xc) & 0x83) == 0) {
          puVar4 = *(undefined4 **)(DAT_00423ee4 + iVar3 * 4);
          goto LAB_00414637;
        }
        FUN_00414105(iVar3,iVar1);
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < DAT_00424f00);
  }
  FUN_00413117(2);
  return puVar4;
}



byte * FUN_0041465b(void)

{
  byte bVar1;
  int iVar2;
  byte *pbVar3;
  byte *pbVar4;
  
  if (DAT_00425130 == 0) {
    FUN_00413988();
  }
  bVar1 = *DAT_00425128;
  pbVar4 = DAT_00425128;
  if (bVar1 == 0x22) {
    while( true ) {
      pbVar3 = pbVar4;
      bVar1 = pbVar3[1];
      pbVar4 = pbVar3 + 1;
      if ((bVar1 == 0x22) || (bVar1 == 0)) break;
      iVar2 = FUN_004179cf(bVar1);
      if (iVar2 != 0) {
        pbVar4 = pbVar3 + 2;
      }
    }
    if (*pbVar4 == 0x22) goto LAB_00414698;
  }
  else {
    while (0x20 < bVar1) {
      bVar1 = pbVar4[1];
      pbVar4 = pbVar4 + 1;
    }
  }
  for (; (*pbVar4 != 0 && (*pbVar4 < 0x21)); pbVar4 = pbVar4 + 1) {
LAB_00414698:
  }
  return pbVar4;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_004146b3(void)

{
  char cVar1;
  size_t sVar2;
  undefined4 *puVar3;
  void *pvVar4;
  int iVar5;
  uint *puVar6;
  
  if (DAT_00425130 == 0) {
    FUN_00413988();
  }
  iVar5 = 0;
  for (puVar6 = DAT_00423a20; (char)*puVar6 != '\0'; puVar6 = (uint *)((int)puVar6 + sVar2 + 1)) {
    if ((char)*puVar6 != '=') {
      iVar5 = iVar5 + 1;
    }
    sVar2 = _strlen((char *)puVar6);
  }
  puVar3 = _malloc(iVar5 * 4 + 4);
  _DAT_004239fc = puVar3;
  if (puVar3 == (undefined4 *)0x0) {
    __amsg_exit(9);
  }
  cVar1 = (char)*DAT_00423a20;
  puVar6 = DAT_00423a20;
  while (cVar1 != '\0') {
    sVar2 = _strlen((char *)puVar6);
    if ((char)*puVar6 != '=') {
      pvVar4 = _malloc(sVar2 + 1);
      *puVar3 = pvVar4;
      if (pvVar4 == (void *)0x0) {
        __amsg_exit(9);
      }
      FUN_00416770((uint *)*puVar3,puVar6);
      puVar3 = puVar3 + 1;
    }
    puVar6 = (uint *)((int)puVar6 + sVar2 + 1);
    cVar1 = (char)*puVar6;
  }
  FUN_00410fa6((undefined *)DAT_00423a20);
  DAT_00423a20 = (uint *)0x0;
  *puVar3 = 0;
  _DAT_0042512c = 1;
  return;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void FUN_0041476c(void)

{
  undefined4 *puVar1;
  byte *pbVar2;
  int local_c;
  int local_8;
  
  if (DAT_00425130 == 0) {
    FUN_00413988();
  }
  GetModuleFileNameA((HMODULE)0x0,&DAT_00423ad0,0x104);
  DAT_00423a0c = &DAT_00423ad0;
  pbVar2 = &DAT_00423ad0;
  if (*DAT_00425128 != 0) {
    pbVar2 = DAT_00425128;
  }
  FUN_00414805(pbVar2,(undefined4 *)0x0,(byte *)0x0,&local_8,&local_c);
  puVar1 = _malloc(local_c + local_8 * 4);
  if (puVar1 == (undefined4 *)0x0) {
    __amsg_exit(8);
  }
  FUN_00414805(pbVar2,puVar1,(byte *)(puVar1 + local_8),&local_8,&local_c);
  _DAT_004239f4 = puVar1;
  _DAT_004239f0 = local_8 + -1;
  return;
}



void __cdecl FUN_00414805(byte *param_1,undefined4 *param_2,byte *param_3,int *param_4,int *param_5)

{
  byte bVar1;
  bool bVar2;
  bool bVar3;
  byte *pbVar4;
  byte *pbVar5;
  uint uVar6;
  undefined4 *puVar7;
  
  *param_5 = 0;
  *param_4 = 1;
  if (param_2 != (undefined4 *)0x0) {
    *param_2 = param_3;
    param_2 = param_2 + 1;
  }
  if (*param_1 == 0x22) {
    while( true ) {
      bVar1 = param_1[1];
      pbVar4 = param_1 + 1;
      if ((bVar1 == 0x22) || (bVar1 == 0)) break;
      if (((*(byte *)((int)&DAT_00425020 + bVar1 + 1) & 4) != 0) &&
         (*param_5 = *param_5 + 1, param_3 != (byte *)0x0)) {
        *param_3 = *pbVar4;
        param_3 = param_3 + 1;
        pbVar4 = param_1 + 2;
      }
      *param_5 = *param_5 + 1;
      param_1 = pbVar4;
      if (param_3 != (byte *)0x0) {
        *param_3 = *pbVar4;
        param_3 = param_3 + 1;
      }
    }
    *param_5 = *param_5 + 1;
    if (param_3 != (byte *)0x0) {
      *param_3 = 0;
      param_3 = param_3 + 1;
    }
    if (*pbVar4 == 0x22) {
      pbVar4 = param_1 + 2;
    }
  }
  else {
    do {
      *param_5 = *param_5 + 1;
      if (param_3 != (byte *)0x0) {
        *param_3 = *param_1;
        param_3 = param_3 + 1;
      }
      bVar1 = *param_1;
      pbVar4 = param_1 + 1;
      if ((*(byte *)((int)&DAT_00425020 + bVar1 + 1) & 4) != 0) {
        *param_5 = *param_5 + 1;
        if (param_3 != (byte *)0x0) {
          *param_3 = *pbVar4;
          param_3 = param_3 + 1;
        }
        pbVar4 = param_1 + 2;
      }
      if (bVar1 == 0x20) break;
      if (bVar1 == 0) goto LAB_004148b0;
      param_1 = pbVar4;
    } while (bVar1 != 9);
    if (bVar1 == 0) {
LAB_004148b0:
      pbVar4 = pbVar4 + -1;
    }
    else if (param_3 != (byte *)0x0) {
      param_3[-1] = 0;
    }
  }
  bVar2 = false;
  puVar7 = param_2;
  while (*pbVar4 != 0) {
    for (; (*pbVar4 == 0x20 || (*pbVar4 == 9)); pbVar4 = pbVar4 + 1) {
    }
    if (*pbVar4 == 0) break;
    if (puVar7 != (undefined4 *)0x0) {
      *puVar7 = param_3;
      puVar7 = puVar7 + 1;
      param_2 = puVar7;
    }
    *param_4 = *param_4 + 1;
    while( true ) {
      bVar3 = true;
      uVar6 = 0;
      for (; *pbVar4 == 0x5c; pbVar4 = pbVar4 + 1) {
        uVar6 = uVar6 + 1;
      }
      if (*pbVar4 == 0x22) {
        pbVar5 = pbVar4;
        if ((uVar6 & 1) == 0) {
          if ((!bVar2) || (pbVar5 = pbVar4 + 1, pbVar4[1] != 0x22)) {
            bVar3 = false;
            pbVar5 = pbVar4;
          }
          bVar2 = !bVar2;
          puVar7 = param_2;
        }
        uVar6 = uVar6 >> 1;
        pbVar4 = pbVar5;
      }
      for (; uVar6 != 0; uVar6 = uVar6 - 1) {
        if (param_3 != (byte *)0x0) {
          *param_3 = 0x5c;
          param_3 = param_3 + 1;
        }
        *param_5 = *param_5 + 1;
      }
      bVar1 = *pbVar4;
      if ((bVar1 == 0) || ((!bVar2 && ((bVar1 == 0x20 || (bVar1 == 9)))))) break;
      if (bVar3) {
        if (param_3 == (byte *)0x0) {
          if ((*(byte *)((int)&DAT_00425020 + bVar1 + 1) & 4) != 0) {
            pbVar4 = pbVar4 + 1;
            *param_5 = *param_5 + 1;
          }
        }
        else {
          if ((*(byte *)((int)&DAT_00425020 + bVar1 + 1) & 4) != 0) {
            *param_3 = bVar1;
            param_3 = param_3 + 1;
            pbVar4 = pbVar4 + 1;
            *param_5 = *param_5 + 1;
          }
          *param_3 = *pbVar4;
          param_3 = param_3 + 1;
        }
        *param_5 = *param_5 + 1;
      }
      pbVar4 = pbVar4 + 1;
    }
    if (param_3 != (byte *)0x0) {
      *param_3 = 0;
      param_3 = param_3 + 1;
    }
    *param_5 = *param_5 + 1;
  }
  if (puVar7 != (undefined4 *)0x0) {
    *puVar7 = 0;
  }
  *param_4 = *param_4 + 1;
  return;
}



LPSTR FUN_004149b9(void)

{
  char cVar1;
  WCHAR WVar2;
  WCHAR *pWVar3;
  WCHAR *pWVar4;
  int iVar5;
  size_t _Size;
  LPSTR pCVar6;
  char *pcVar7;
  LPWCH lpWideCharStr;
  LPCH pCVar9;
  LPSTR local_8;
  char *pcVar8;
  
  lpWideCharStr = (LPWCH)0x0;
  pCVar9 = (LPCH)0x0;
  if (DAT_00423bd4 == 0) {
    lpWideCharStr = GetEnvironmentStringsW();
    if (lpWideCharStr != (LPWCH)0x0) {
      DAT_00423bd4 = 1;
LAB_00414a10:
      if ((lpWideCharStr == (LPWCH)0x0) &&
         (lpWideCharStr = GetEnvironmentStringsW(), lpWideCharStr == (LPWCH)0x0)) {
        return (LPSTR)0x0;
      }
      WVar2 = *lpWideCharStr;
      pWVar4 = lpWideCharStr;
      while (WVar2 != L'\0') {
        do {
          pWVar3 = pWVar4;
          pWVar4 = pWVar3 + 1;
        } while (*pWVar4 != L'\0');
        pWVar4 = pWVar3 + 2;
        WVar2 = *pWVar4;
      }
      iVar5 = ((int)pWVar4 - (int)lpWideCharStr >> 1) + 1;
      _Size = WideCharToMultiByte(0,0,lpWideCharStr,iVar5,(LPSTR)0x0,0,(LPCSTR)0x0,(LPBOOL)0x0);
      local_8 = (LPSTR)0x0;
      if (((_Size != 0) && (pCVar6 = _malloc(_Size), pCVar6 != (LPSTR)0x0)) &&
         (iVar5 = WideCharToMultiByte(0,0,lpWideCharStr,iVar5,pCVar6,_Size,(LPCSTR)0x0,(LPBOOL)0x0),
         local_8 = pCVar6, iVar5 == 0)) {
        FUN_00410fa6(pCVar6);
        local_8 = (LPSTR)0x0;
      }
      FreeEnvironmentStringsW(lpWideCharStr);
      return local_8;
    }
    pCVar9 = GetEnvironmentStrings();
    if (pCVar9 == (LPCH)0x0) {
      return (LPSTR)0x0;
    }
    DAT_00423bd4 = 2;
  }
  else {
    if (DAT_00423bd4 == 1) goto LAB_00414a10;
    if (DAT_00423bd4 != 2) {
      return (LPSTR)0x0;
    }
  }
  if ((pCVar9 == (LPCH)0x0) && (pCVar9 = GetEnvironmentStrings(), pCVar9 == (LPCH)0x0)) {
    return (LPSTR)0x0;
  }
  cVar1 = *pCVar9;
  pcVar7 = pCVar9;
  while (cVar1 != '\0') {
    do {
      pcVar8 = pcVar7;
      pcVar7 = pcVar8 + 1;
    } while (*pcVar7 != '\0');
    pcVar7 = pcVar8 + 2;
    cVar1 = *pcVar7;
  }
  pCVar6 = _malloc((size_t)(pcVar7 + (1 - (int)pCVar9)));
  if (pCVar6 == (LPSTR)0x0) {
    pCVar6 = (LPSTR)0x0;
  }
  else {
    FUN_00411540((undefined4 *)pCVar6,(undefined4 *)pCVar9,(uint)(pcVar7 + (1 - (int)pCVar9)));
  }
  FreeEnvironmentStringsA(pCVar9);
  return pCVar6;
}



void FUN_00414aeb(void)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  DWORD DVar4;
  HANDLE hFile;
  UINT *pUVar5;
  int iVar6;
  uint uVar7;
  UINT UVar8;
  UINT UVar9;
  _STARTUPINFOA local_4c;
  byte *local_8;
  
  puVar2 = _malloc(0x480);
  if (puVar2 == (undefined4 *)0x0) {
    __amsg_exit(0x1b);
  }
  DAT_00423ee0 = 0x20;
  DAT_00423de0 = puVar2;
  for (; puVar2 < DAT_00423de0 + 0x120; puVar2 = puVar2 + 9) {
    *(undefined1 *)(puVar2 + 1) = 0;
    *puVar2 = 0xffffffff;
    puVar2[2] = 0;
    *(undefined1 *)((int)puVar2 + 5) = 10;
  }
  GetStartupInfoA(&local_4c);
  if ((local_4c.cbReserved2 != 0) && ((UINT *)local_4c.lpReserved2 != (UINT *)0x0)) {
    UVar8 = *(UINT *)local_4c.lpReserved2;
    pUVar5 = (UINT *)((int)local_4c.lpReserved2 + 4);
    local_8 = (byte *)((int)pUVar5 + UVar8);
    if (0x7ff < (int)UVar8) {
      UVar8 = 0x800;
    }
    UVar9 = UVar8;
    if ((int)DAT_00423ee0 < (int)UVar8) {
      puVar2 = &DAT_00423de4;
      do {
        puVar3 = _malloc(0x480);
        UVar9 = DAT_00423ee0;
        if (puVar3 == (undefined4 *)0x0) break;
        DAT_00423ee0 = DAT_00423ee0 + 0x20;
        *puVar2 = puVar3;
        puVar1 = puVar3;
        for (; puVar3 < puVar1 + 0x120; puVar3 = puVar3 + 9) {
          *(undefined1 *)(puVar3 + 1) = 0;
          *puVar3 = 0xffffffff;
          puVar3[2] = 0;
          *(undefined1 *)((int)puVar3 + 5) = 10;
          puVar1 = (undefined4 *)*puVar2;
        }
        puVar2 = puVar2 + 1;
        UVar9 = UVar8;
      } while ((int)DAT_00423ee0 < (int)UVar8);
    }
    uVar7 = 0;
    if (0 < (int)UVar9) {
      do {
        if (((*(HANDLE *)local_8 != (HANDLE)0xffffffff) && ((*pUVar5 & 1) != 0)) &&
           (((*pUVar5 & 8) != 0 || (DVar4 = GetFileType(*(HANDLE *)local_8), DVar4 != 0)))) {
          puVar2 = (undefined4 *)((int)(&DAT_00423de0)[(int)uVar7 >> 5] + (uVar7 & 0x1f) * 0x24);
          *puVar2 = *(undefined4 *)local_8;
          *(byte *)(puVar2 + 1) = (byte)*pUVar5;
        }
        local_8 = local_8 + 4;
        uVar7 = uVar7 + 1;
        pUVar5 = (UINT *)((int)pUVar5 + 1);
      } while ((int)uVar7 < (int)UVar9);
    }
  }
  iVar6 = 0;
  do {
    puVar2 = DAT_00423de0 + iVar6 * 9;
    if (DAT_00423de0[iVar6 * 9] == -1) {
      *(undefined1 *)(puVar2 + 1) = 0x81;
      if (iVar6 == 0) {
        DVar4 = 0xfffffff6;
      }
      else {
        DVar4 = 0xfffffff5 - (iVar6 != 1);
      }
      hFile = GetStdHandle(DVar4);
      if ((hFile != (HANDLE)0xffffffff) && (DVar4 = GetFileType(hFile), DVar4 != 0)) {
        *puVar2 = hFile;
        if ((DVar4 & 0xff) != 2) {
          if ((DVar4 & 0xff) == 3) {
            *(byte *)(puVar2 + 1) = *(byte *)(puVar2 + 1) | 8;
          }
          goto LAB_00414c90;
        }
      }
      *(byte *)(puVar2 + 1) = *(byte *)(puVar2 + 1) | 0x40;
    }
    else {
      *(byte *)(puVar2 + 1) = *(byte *)(puVar2 + 1) | 0x80;
    }
LAB_00414c90:
    iVar6 = iVar6 + 1;
    if (2 < iVar6) {
      SetHandleCount(DAT_00423ee0);
      return;
    }
  } while( true );
}



void __cdecl FUN_00414ca7(undefined4 *param_1)

{
  int iVar1;
  HMODULE pHVar2;
  
  *param_1 = 0;
  pHVar2 = GetModuleHandleA((LPCSTR)0x0);
  if (((short)pHVar2->unused == 0x5a4d) && (iVar1 = pHVar2[0xf].unused, iVar1 != 0)) {
    *(undefined1 *)param_1 = *(undefined1 *)((int)&pHVar2[6].unused + iVar1 + 2);
    *(undefined1 *)((int)param_1 + 1) = *(undefined1 *)((int)&pHVar2[6].unused + iVar1 + 3);
  }
  return;
}



int FUN_00414cd4(void)

{
  char cVar1;
  byte bVar2;
  BOOL BVar3;
  DWORD DVar4;
  int iVar5;
  uint *puVar6;
  byte *pbVar7;
  char *pcVar8;
  byte *this;
  byte unaff_BL;
  undefined4 local_1230;
  char local_1a0 [260];
  DWORD local_9c;
  uint local_98;
  DWORD local_8c;
  CHAR aCStackY_18 [4];
  
  FUN_004101e0();
  local_9c = 0x94;
  BVar3 = GetVersionExA((LPOSVERSIONINFOA)&local_9c);
  if (((BVar3 == 0) || (local_8c != 2)) || (local_98 < 5)) {
    builtin_memcpy(aCStackY_18,".MA",4);
    DVar4 = GetEnvironmentVariableA("__MSVCRT_HEAP_SELECT",(LPSTR)&local_1230,0x1090);
    if (DVar4 != 0) {
      pcVar8 = (char *)&local_1230;
      while ((char)local_1230 != '\0') {
        cVar1 = *pcVar8;
        if (('`' < cVar1) && (cVar1 < '{')) {
          *pcVar8 = cVar1 + -0x20;
        }
        pcVar8 = pcVar8 + 1;
        local_1230._0_1_ = *pcVar8;
      }
      builtin_memcpy(aCStackY_18,"lMA",4);
      iVar5 = _strncmp("__GLOBAL_HEAP_SELECTED",(char *)&local_1230,0x16);
      if (iVar5 == 0) {
        puVar6 = &local_1230;
      }
      else {
        aCStackY_18[0] = -0x72;
        aCStackY_18[1] = 'M';
        aCStackY_18[2] = 'A';
        aCStackY_18[3] = '\0';
        GetModuleFileNameA((HMODULE)0x0,local_1a0,0x104);
        pcVar8 = local_1a0;
        while (local_1a0[0] != '\0') {
          cVar1 = *pcVar8;
          if (('`' < cVar1) && (cVar1 < '{')) {
            *pcVar8 = cVar1 + -0x20;
          }
          pcVar8 = pcVar8 + 1;
          local_1a0[0] = *pcVar8;
        }
        puVar6 = FUN_00410470(&local_1230,local_1a0);
      }
      if ((puVar6 != (uint *)0x0) && (puVar6 = FUN_00411270(puVar6,','), puVar6 != (uint *)0x0)) {
        pbVar7 = (byte *)((int)puVar6 + 1);
        bVar2 = *pbVar7;
        this = pbVar7;
        while (bVar2 != 0) {
          if (*this == 0x3b) {
            *this = 0;
          }
          else {
            this = this + 1;
          }
          bVar2 = *this;
        }
        aCStackY_18[0] = -0xc;
        aCStackY_18[1] = 'M';
        aCStackY_18[2] = 'A';
        aCStackY_18[3] = '\0';
        iVar5 = FUN_0041023d(this,pbVar7,(int *)0x0,(void *)0xa);
        if (iVar5 == 2) {
          return 2;
        }
        if (iVar5 == 3) {
          return 3;
        }
        if (iVar5 == 1) {
          return 1;
        }
      }
    }
    FUN_00414ca7((undefined4 *)&stack0xfffffff8);
    iVar5 = 3 - (uint)(unaff_BL < 6);
  }
  else {
    iVar5 = 1;
  }
  return iVar5;
}



undefined4 __cdecl FUN_00414e1c(int param_1)

{
  undefined **ppuVar1;
  
  DAT_00423dc8 = HeapCreate((uint)(param_1 == 0),0x1000,0);
  if (DAT_00423dc8 != (HANDLE)0x0) {
    DAT_00423dcc = FUN_00414cd4();
    if (DAT_00423dcc == 3) {
      ppuVar1 = (undefined **)FUN_004151af(0x3f8);
    }
    else {
      if (DAT_00423dcc != 2) {
        return 1;
      }
      ppuVar1 = FUN_00415cf6();
    }
    if (ppuVar1 != (undefined **)0x0) {
      return 1;
    }
    HeapDestroy(DAT_00423dc8);
  }
  return 0;
}



void FUN_00414e79(void)

{
  if ((DAT_00423a28 == 1) || ((DAT_00423a28 == 0 && (DAT_00420924 == 1)))) {
    FUN_00414eb2((undefined *)0xfc);
    if (DAT_00423bd8 != (code *)0x0) {
      (*DAT_00423bd8)();
    }
    FUN_00414eb2((undefined *)0xff);
  }
  return;
}



void __cdecl FUN_00414eb2(undefined *param_1)

{
  undefined4 *puVar1;
  undefined **ppuVar2;
  DWORD DVar3;
  size_t sVar4;
  HANDLE hFile;
  int iVar5;
  uint *_Dest;
  undefined1 auStackY_1e3 [7];
  LPCVOID lpBuffer;
  LPOVERLAPPED lpOverlapped;
  uint local_1a8 [65];
  uint local_a4 [40];
  
  iVar5 = 0;
  ppuVar2 = (undefined **)&DAT_004211d8;
  do {
    if (param_1 == *ppuVar2) break;
    ppuVar2 = ppuVar2 + 2;
    iVar5 = iVar5 + 1;
  } while (ppuVar2 < &PTR_LOOP_00421268);
  if (param_1 == (undefined *)(&DAT_004211d8)[iVar5 * 2]) {
    if ((DAT_00423a28 == 1) || ((DAT_00423a28 == 0 && (DAT_00420924 == 1)))) {
      ppuVar2 = &param_1;
      puVar1 = (undefined4 *)(iVar5 * 8 + 0x4211dc);
      lpOverlapped = (LPOVERLAPPED)0x0;
      sVar4 = _strlen((char *)*puVar1);
      lpBuffer = (LPCVOID)*puVar1;
      hFile = GetStdHandle(0xfffffff4);
      WriteFile(hFile,lpBuffer,sVar4,(LPDWORD)ppuVar2,lpOverlapped);
    }
    else if (param_1 != (undefined *)0xfc) {
      DVar3 = GetModuleFileNameA((HMODULE)0x0,(LPSTR)local_1a8,0x104);
      if (DVar3 == 0) {
        FUN_00416770(local_1a8,(uint *)"<program name unknown>");
      }
      _Dest = local_1a8;
      sVar4 = _strlen((char *)local_1a8);
      if (0x3c < sVar4 + 1) {
        sVar4 = _strlen((char *)local_1a8);
        _Dest = (uint *)(auStackY_1e3 + sVar4);
        _strncpy((char *)_Dest,"...",3);
      }
      FUN_00416770(local_a4,(uint *)"Runtime Error!\n\nProgram: ");
      FUN_00416780(local_a4,_Dest);
      FUN_00416780(local_a4,(uint *)&DAT_0041dcd0);
      FUN_00416780(local_a4,*(uint **)(iVar5 * 8 + 0x4211dc));
      auStackY_1e3._3_4_ = 0x414fd6;
      FUN_00417a11(local_a4,"Microsoft Visual C++ Runtime Library",0x12010);
    }
  }
  return;
}



DWORD __cdecl FUN_00415005(uint param_1,LONG param_2,DWORD param_3)

{
  DWORD DVar1;
  DWORD *pDVar2;
  
  if ((param_1 < DAT_00423ee0) &&
     ((*(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + (param_1 & 0x1f) * 0x24) & 1) != 0)) {
    FUN_0041767f(param_1);
    DVar1 = FUN_0041506a(param_1,param_2,param_3);
    FUN_004176de(param_1);
    return DVar1;
  }
  pDVar2 = FUN_00410d43();
  *pDVar2 = 9;
  pDVar2 = FUN_00410d4c();
  *pDVar2 = 0;
  return 0xffffffff;
}



DWORD __cdecl FUN_0041506a(uint param_1,LONG param_2,DWORD param_3)

{
  byte *pbVar1;
  HANDLE hFile;
  DWORD *pDVar2;
  DWORD DVar3;
  uint uVar4;
  
  hFile = (HANDLE)FUN_0041763d(param_1);
  if (hFile == (HANDLE)0xffffffff) {
    pDVar2 = FUN_00410d43();
    *pDVar2 = 9;
  }
  else {
    DVar3 = SetFilePointer(hFile,param_2,(PLONG)0x0,param_3);
    if (DVar3 == 0xffffffff) {
      uVar4 = GetLastError();
    }
    else {
      uVar4 = 0;
    }
    if (uVar4 == 0) {
      pbVar1 = (byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + (param_1 & 0x1f) * 0x24);
      *pbVar1 = *pbVar1 & 0xfd;
      return DVar3;
    }
    FUN_00410cd0(uVar4);
  }
  return 0xffffffff;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

undefined4 __cdecl FUN_004150dd(undefined4 *param_1)

{
  undefined4 uVar1;
  byte bVar2;
  undefined3 extraout_var;
  int iVar3;
  void *pvVar4;
  
  bVar2 = FUN_00416dde(param_1[4]);
  if (CONCAT31(extraout_var,bVar2) == 0) {
    return 0;
  }
  if (param_1 == (undefined4 *)&DAT_00420f50) {
    iVar3 = 0;
  }
  else {
    if (param_1 != (undefined4 *)&DAT_00420f70) {
      return 0;
    }
    iVar3 = 1;
  }
  _DAT_00423acc = _DAT_00423acc + 1;
  if ((*(ushort *)(param_1 + 3) & 0x10c) != 0) {
    return 0;
  }
  if ((&DAT_00423bdc)[iVar3] == 0) {
    pvVar4 = _malloc(0x1000);
    (&DAT_00423bdc)[iVar3] = pvVar4;
    if (pvVar4 == (void *)0x0) {
      param_1[2] = param_1 + 5;
      *param_1 = param_1 + 5;
      param_1[6] = 2;
      param_1[1] = 2;
      goto LAB_00415159;
    }
  }
  uVar1 = (&DAT_00423bdc)[iVar3];
  param_1[6] = 0x1000;
  param_1[2] = uVar1;
  *param_1 = uVar1;
  param_1[1] = 0x1000;
LAB_00415159:
  *(ushort *)(param_1 + 3) = *(ushort *)(param_1 + 3) | 0x1102;
  return 1;
}



void __cdecl FUN_0041516a(int param_1,int *param_2)

{
  if ((param_1 != 0) && ((*(byte *)((int)param_2 + 0xd) & 0x10) != 0)) {
    FUN_004110bd(param_2);
    *(byte *)((int)param_2 + 0xd) = *(byte *)((int)param_2 + 0xd) & 0xee;
    param_2[6] = 0;
    *param_2 = 0;
    param_2[2] = 0;
  }
  return;
}



undefined4 __cdecl FUN_00415194(undefined4 param_1)

{
  int iVar1;
  
  if (DAT_00423bec != (code *)0x0) {
    iVar1 = (*DAT_00423bec)(param_1);
    if (iVar1 != 0) {
      return 1;
    }
  }
  return 0;
}



undefined4 __cdecl FUN_004151af(undefined4 param_1)

{
  DAT_00423dc0 = HeapAlloc(DAT_00423dc8,0,0x140);
  if (DAT_00423dc0 == (LPVOID)0x0) {
    return 0;
  }
  DAT_00423db8 = 0;
  DAT_00423dbc = 0;
  DAT_00423db4 = DAT_00423dc0;
  DAT_00423dc4 = param_1;
  DAT_00423dac = 0x10;
  return 1;
}



uint __cdecl FUN_004151f7(int param_1)

{
  uint uVar1;
  
  uVar1 = DAT_00423dc0;
  while( true ) {
    if (DAT_00423dc0 + DAT_00423dbc * 0x14 <= uVar1) {
      return 0;
    }
    if ((uint)(param_1 - *(int *)(uVar1 + 0xc)) < 0x100000) break;
    uVar1 = uVar1 + 0x14;
  }
  return uVar1;
}



void __cdecl FUN_00415222(uint *param_1,int param_2)

{
  char *pcVar1;
  uint *puVar2;
  int *piVar3;
  char cVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  byte bVar8;
  uint uVar9;
  uint *puVar10;
  uint *puVar11;
  uint *puVar12;
  uint uVar13;
  uint uVar14;
  uint local_8;
  
  uVar5 = param_1[4];
  puVar12 = (uint *)(param_2 + -4);
  uVar14 = param_2 - param_1[3] >> 0xf;
  piVar3 = (int *)(uVar14 * 0x204 + 0x144 + uVar5);
  uVar13 = *puVar12;
  local_8 = uVar13 - 1;
  if ((local_8 & 1) == 0) {
    uVar6 = *(uint *)(local_8 + (int)puVar12);
    uVar7 = *(uint *)(param_2 + -8);
    if ((uVar6 & 1) == 0) {
      uVar9 = ((int)uVar6 >> 4) - 1;
      if (0x3f < uVar9) {
        uVar9 = 0x3f;
      }
      if (*(int *)((int)puVar12 + uVar13 + 3) == *(int *)((int)puVar12 + uVar13 + 7)) {
        if (uVar9 < 0x20) {
          pcVar1 = (char *)(uVar9 + 4 + uVar5);
          uVar9 = ~(0x80000000U >> ((byte)uVar9 & 0x1f));
          puVar10 = (uint *)(uVar5 + 0x44 + uVar14 * 4);
          *puVar10 = *puVar10 & uVar9;
          *pcVar1 = *pcVar1 + -1;
          if (*pcVar1 == '\0') {
            *param_1 = *param_1 & uVar9;
          }
        }
        else {
          pcVar1 = (char *)(uVar9 + 4 + uVar5);
          uVar9 = ~(0x80000000U >> ((byte)uVar9 - 0x20 & 0x1f));
          puVar10 = (uint *)(uVar5 + 0xc4 + uVar14 * 4);
          *puVar10 = *puVar10 & uVar9;
          *pcVar1 = *pcVar1 + -1;
          if (*pcVar1 == '\0') {
            param_1[1] = param_1[1] & uVar9;
          }
        }
      }
      local_8 = local_8 + uVar6;
      *(undefined4 *)(*(int *)((int)puVar12 + uVar13 + 7) + 4) =
           *(undefined4 *)((int)puVar12 + uVar13 + 3);
      *(undefined4 *)(*(int *)((int)puVar12 + uVar13 + 3) + 8) =
           *(undefined4 *)((int)puVar12 + uVar13 + 7);
    }
    puVar10 = (uint *)(((int)local_8 >> 4) - 1);
    if ((uint *)0x3f < puVar10) {
      puVar10 = (uint *)0x3f;
    }
    puVar11 = param_1;
    if ((uVar7 & 1) == 0) {
      puVar12 = (uint *)((int)puVar12 - uVar7);
      puVar11 = (uint *)(((int)uVar7 >> 4) - 1);
      if ((uint *)0x3f < puVar11) {
        puVar11 = (uint *)0x3f;
      }
      local_8 = local_8 + uVar7;
      puVar10 = (uint *)(((int)local_8 >> 4) - 1);
      if ((uint *)0x3f < puVar10) {
        puVar10 = (uint *)0x3f;
      }
      if (puVar11 != puVar10) {
        if (puVar12[1] == puVar12[2]) {
          if (puVar11 < (uint *)0x20) {
            uVar13 = ~(0x80000000U >> ((byte)puVar11 & 0x1f));
            puVar2 = (uint *)(uVar5 + 0x44 + uVar14 * 4);
            *puVar2 = *puVar2 & uVar13;
            pcVar1 = (char *)((int)puVar11 + uVar5 + 4);
            *pcVar1 = *pcVar1 + -1;
            if (*pcVar1 == '\0') {
              *param_1 = *param_1 & uVar13;
            }
          }
          else {
            uVar13 = ~(0x80000000U >> ((byte)puVar11 - 0x20 & 0x1f));
            puVar2 = (uint *)(uVar5 + 0xc4 + uVar14 * 4);
            *puVar2 = *puVar2 & uVar13;
            pcVar1 = (char *)((int)puVar11 + uVar5 + 4);
            *pcVar1 = *pcVar1 + -1;
            if (*pcVar1 == '\0') {
              param_1[1] = param_1[1] & uVar13;
            }
          }
        }
        *(uint *)(puVar12[2] + 4) = puVar12[1];
        *(uint *)(puVar12[1] + 8) = puVar12[2];
      }
    }
    if (((uVar7 & 1) != 0) || (puVar11 != puVar10)) {
      puVar12[1] = piVar3[(int)puVar10 * 2 + 1];
      puVar12[2] = (uint)(piVar3 + (int)puVar10 * 2);
      (piVar3 + (int)puVar10 * 2)[1] = (int)puVar12;
      *(uint **)(puVar12[1] + 8) = puVar12;
      if (puVar12[1] == puVar12[2]) {
        cVar4 = *(char *)((int)puVar10 + uVar5 + 4);
        *(char *)((int)puVar10 + uVar5 + 4) = cVar4 + '\x01';
        bVar8 = (byte)puVar10;
        if (puVar10 < (uint *)0x20) {
          if (cVar4 == '\0') {
            *param_1 = *param_1 | 0x80000000U >> (bVar8 & 0x1f);
          }
          puVar10 = (uint *)(uVar5 + 0x44 + uVar14 * 4);
          *puVar10 = *puVar10 | 0x80000000U >> (bVar8 & 0x1f);
        }
        else {
          if (cVar4 == '\0') {
            param_1[1] = param_1[1] | 0x80000000U >> (bVar8 - 0x20 & 0x1f);
          }
          puVar10 = (uint *)(uVar5 + 0xc4 + uVar14 * 4);
          *puVar10 = *puVar10 | 0x80000000U >> (bVar8 - 0x20 & 0x1f);
        }
      }
    }
    *puVar12 = local_8;
    *(uint *)((local_8 - 4) + (int)puVar12) = local_8;
    *piVar3 = *piVar3 + -1;
    if (*piVar3 == 0) {
      if (DAT_00423db8 != (uint *)0x0) {
        VirtualFree((LPVOID)(DAT_00423db0 * 0x8000 + DAT_00423db8[3]),0x8000,0x4000);
        DAT_00423db8[2] = DAT_00423db8[2] | 0x80000000U >> ((byte)DAT_00423db0 & 0x1f);
        *(undefined4 *)(DAT_00423db8[4] + 0xc4 + DAT_00423db0 * 4) = 0;
        *(char *)(DAT_00423db8[4] + 0x43) = *(char *)(DAT_00423db8[4] + 0x43) + -1;
        if (*(char *)(DAT_00423db8[4] + 0x43) == '\0') {
          DAT_00423db8[1] = DAT_00423db8[1] & 0xfffffffe;
        }
        if (DAT_00423db8[2] == 0xffffffff) {
          VirtualFree((LPVOID)DAT_00423db8[3],0,0x8000);
          HeapFree(DAT_00423dc8,0,(LPVOID)DAT_00423db8[4]);
          FUN_0040f710(DAT_00423db8,DAT_00423db8 + 5,
                       (DAT_00423dbc * 0x14 - (int)DAT_00423db8) + -0x14 + DAT_00423dc0);
          DAT_00423dbc = DAT_00423dbc + -1;
          if (DAT_00423db8 < param_1) {
            param_1 = param_1 + -5;
          }
          DAT_00423db4 = DAT_00423dc0;
        }
      }
      DAT_00423db8 = param_1;
      DAT_00423db0 = uVar14;
    }
  }
  return;
}



int * __cdecl FUN_0041554b(uint *param_1)

{
  char *pcVar1;
  int *piVar2;
  char cVar3;
  int *piVar4;
  byte bVar5;
  uint uVar6;
  int iVar7;
  uint *puVar8;
  int iVar9;
  int *piVar10;
  uint *puVar11;
  uint *puVar12;
  uint uVar13;
  int iVar14;
  uint local_10;
  uint local_c;
  int local_8;
  
  puVar8 = DAT_00423dc0 + DAT_00423dbc * 5;
  uVar6 = (int)param_1 + 0x17U & 0xfffffff0;
  iVar7 = ((int)((int)param_1 + 0x17U) >> 4) + -1;
  bVar5 = (byte)iVar7;
  param_1 = DAT_00423db4;
  if (iVar7 < 0x20) {
    local_10 = 0xffffffff >> (bVar5 & 0x1f);
    local_c = 0xffffffff;
  }
  else {
    local_c = 0xffffffff >> (bVar5 - 0x20 & 0x1f);
    local_10 = 0;
  }
  for (; (param_1 < puVar8 && ((param_1[1] & local_c) == 0 && (*param_1 & local_10) == 0));
      param_1 = param_1 + 5) {
  }
  puVar11 = DAT_00423dc0;
  if (param_1 == puVar8) {
    for (; (puVar11 < DAT_00423db4 && ((puVar11[1] & local_c) == 0 && (*puVar11 & local_10) == 0));
        puVar11 = puVar11 + 5) {
    }
    param_1 = puVar11;
    if (puVar11 == DAT_00423db4) {
      for (; (puVar11 < puVar8 && (puVar11[2] == 0)); puVar11 = puVar11 + 5) {
      }
      puVar12 = DAT_00423dc0;
      param_1 = puVar11;
      if (puVar11 == puVar8) {
        for (; (puVar12 < DAT_00423db4 && (puVar12[2] == 0)); puVar12 = puVar12 + 5) {
        }
        param_1 = puVar12;
        if ((puVar12 == DAT_00423db4) && (param_1 = FUN_00415854(), param_1 == (uint *)0x0)) {
          return (int *)0x0;
        }
      }
      iVar7 = FUN_00415905((int)param_1);
      *(int *)param_1[4] = iVar7;
      if (*(int *)param_1[4] == -1) {
        return (int *)0x0;
      }
    }
  }
  piVar4 = (int *)param_1[4];
  local_8 = *piVar4;
  if ((local_8 == -1) ||
     ((piVar4[local_8 + 0x31] & local_c) == 0 && (piVar4[local_8 + 0x11] & local_10) == 0)) {
    local_8 = 0;
    puVar8 = (uint *)(piVar4 + 0x11);
    if ((piVar4[0x31] & local_c) == 0 && (piVar4[0x11] & local_10) == 0) {
      do {
        puVar11 = puVar8 + 0x21;
        local_8 = local_8 + 1;
        puVar8 = puVar8 + 1;
      } while ((*puVar11 & local_c) == 0 && (local_10 & *puVar8) == 0);
    }
  }
  iVar7 = 0;
  piVar2 = piVar4 + local_8 * 0x81 + 0x51;
  local_10 = piVar4[local_8 + 0x11] & local_10;
  if (local_10 == 0) {
    local_10 = piVar4[local_8 + 0x31] & local_c;
    iVar7 = 0x20;
  }
  for (; -1 < (int)local_10; local_10 = local_10 << 1) {
    iVar7 = iVar7 + 1;
  }
  piVar10 = (int *)piVar2[iVar7 * 2 + 1];
  iVar9 = *piVar10 - uVar6;
  iVar14 = (iVar9 >> 4) + -1;
  if (0x3f < iVar14) {
    iVar14 = 0x3f;
  }
  DAT_00423db4 = param_1;
  if (iVar14 != iVar7) {
    if (piVar10[1] == piVar10[2]) {
      if (iVar7 < 0x20) {
        pcVar1 = (char *)((int)piVar4 + iVar7 + 4);
        uVar13 = ~(0x80000000U >> ((byte)iVar7 & 0x1f));
        piVar4[local_8 + 0x11] = uVar13 & piVar4[local_8 + 0x11];
        *pcVar1 = *pcVar1 + -1;
        if (*pcVar1 == '\0') {
          *param_1 = *param_1 & uVar13;
        }
      }
      else {
        pcVar1 = (char *)((int)piVar4 + iVar7 + 4);
        uVar13 = ~(0x80000000U >> ((byte)iVar7 - 0x20 & 0x1f));
        piVar4[local_8 + 0x31] = piVar4[local_8 + 0x31] & uVar13;
        *pcVar1 = *pcVar1 + -1;
        if (*pcVar1 == '\0') {
          param_1[1] = param_1[1] & uVar13;
        }
      }
    }
    *(int *)(piVar10[2] + 4) = piVar10[1];
    *(int *)(piVar10[1] + 8) = piVar10[2];
    if (iVar9 == 0) goto LAB_00415811;
    piVar10[1] = piVar2[iVar14 * 2 + 1];
    piVar10[2] = (int)(piVar2 + iVar14 * 2);
    (piVar2 + iVar14 * 2)[1] = (int)piVar10;
    *(int **)(piVar10[1] + 8) = piVar10;
    if (piVar10[1] == piVar10[2]) {
      cVar3 = *(char *)(iVar14 + 4 + (int)piVar4);
      bVar5 = (byte)iVar14;
      if (iVar14 < 0x20) {
        *(char *)(iVar14 + 4 + (int)piVar4) = cVar3 + '\x01';
        if (cVar3 == '\0') {
          *param_1 = *param_1 | 0x80000000U >> (bVar5 & 0x1f);
        }
        piVar4[local_8 + 0x11] = piVar4[local_8 + 0x11] | 0x80000000U >> (bVar5 & 0x1f);
      }
      else {
        *(char *)(iVar14 + 4 + (int)piVar4) = cVar3 + '\x01';
        if (cVar3 == '\0') {
          param_1[1] = param_1[1] | 0x80000000U >> (bVar5 - 0x20 & 0x1f);
        }
        piVar4[local_8 + 0x31] = piVar4[local_8 + 0x31] | 0x80000000U >> (bVar5 - 0x20 & 0x1f);
      }
    }
  }
  if (iVar9 != 0) {
    *piVar10 = iVar9;
    *(int *)(iVar9 + -4 + (int)piVar10) = iVar9;
  }
LAB_00415811:
  piVar10 = (int *)((int)piVar10 + iVar9);
  *piVar10 = uVar6 + 1;
  *(uint *)((int)piVar10 + (uVar6 - 4)) = uVar6 + 1;
  iVar7 = *piVar2;
  *piVar2 = iVar7 + 1;
  if (((iVar7 == 0) && (param_1 == DAT_00423db8)) && (local_8 == DAT_00423db0)) {
    DAT_00423db8 = (uint *)0x0;
  }
  *piVar4 = local_8;
  return piVar10 + 1;
}



undefined4 * FUN_00415854(void)

{
  undefined4 *puVar1;
  LPVOID pvVar2;
  
  if (DAT_00423dbc == DAT_00423dac) {
    pvVar2 = HeapReAlloc(DAT_00423dc8,0,DAT_00423dc0,(DAT_00423dac * 5 + 0x50) * 4);
    if (pvVar2 == (LPVOID)0x0) {
      return (undefined4 *)0x0;
    }
    DAT_00423dac = DAT_00423dac + 0x10;
    DAT_00423dc0 = pvVar2;
  }
  puVar1 = (undefined4 *)((int)DAT_00423dc0 + DAT_00423dbc * 0x14);
  pvVar2 = HeapAlloc(DAT_00423dc8,8,0x41c4);
  puVar1[4] = pvVar2;
  if (pvVar2 != (LPVOID)0x0) {
    pvVar2 = VirtualAlloc((LPVOID)0x0,0x100000,0x2000,4);
    puVar1[3] = pvVar2;
    if (pvVar2 != (LPVOID)0x0) {
      puVar1[2] = 0xffffffff;
      *puVar1 = 0;
      puVar1[1] = 0;
      DAT_00423dbc = DAT_00423dbc + 1;
      *(undefined4 *)puVar1[4] = 0xffffffff;
      return puVar1;
    }
    HeapFree(DAT_00423dc8,0,(LPVOID)puVar1[4]);
  }
  return (undefined4 *)0x0;
}



int __cdecl FUN_00415905(int param_1)

{
  int *piVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  LPVOID pvVar6;
  int *piVar7;
  int iVar8;
  int iVar9;
  int *lpAddress;
  
  iVar3 = *(int *)(param_1 + 0x10);
  iVar9 = 0;
  for (iVar4 = *(int *)(param_1 + 8); -1 < iVar4; iVar4 = iVar4 << 1) {
    iVar9 = iVar9 + 1;
  }
  iVar8 = 0x3f;
  iVar4 = iVar9 * 0x204 + 0x144 + iVar3;
  iVar5 = iVar4;
  do {
    *(int *)(iVar5 + 8) = iVar5;
    *(int *)(iVar5 + 4) = iVar5;
    iVar5 = iVar5 + 8;
    iVar8 = iVar8 + -1;
  } while (iVar8 != 0);
  lpAddress = (int *)(iVar9 * 0x8000 + *(int *)(param_1 + 0xc));
  pvVar6 = VirtualAlloc(lpAddress,0x8000,0x1000,4);
  if (pvVar6 == (LPVOID)0x0) {
    iVar9 = -1;
  }
  else {
    if (lpAddress <= lpAddress + 0x1c00) {
      piVar7 = lpAddress + 4;
      do {
        piVar7[-2] = -1;
        piVar7[0x3fb] = -1;
        piVar7[-1] = 0xff0;
        *piVar7 = (int)(piVar7 + 0x3ff);
        piVar7[1] = (int)(piVar7 + -0x401);
        piVar7[0x3fa] = 0xff0;
        piVar1 = piVar7 + 0x3fc;
        piVar7 = piVar7 + 0x400;
      } while (piVar1 <= lpAddress + 0x1c00);
    }
    *(int **)(iVar4 + 0x1fc) = lpAddress + 3;
    lpAddress[5] = iVar4 + 0x1f8;
    *(int **)(iVar4 + 0x200) = lpAddress + 0x1c03;
    lpAddress[0x1c04] = iVar4 + 0x1f8;
    *(undefined4 *)(iVar3 + 0x44 + iVar9 * 4) = 0;
    *(undefined4 *)(iVar3 + 0xc4 + iVar9 * 4) = 1;
    cVar2 = *(char *)(iVar3 + 0x43);
    *(char *)(iVar3 + 0x43) = cVar2 + '\x01';
    if (cVar2 == '\0') {
      *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 1;
    }
    *(uint *)(param_1 + 8) = *(uint *)(param_1 + 8) & ~(0x80000000U >> ((byte)iVar9 & 0x1f));
  }
  return iVar9;
}



undefined4 __cdecl FUN_00415a00(uint *param_1,int param_2,int param_3)

{
  char *pcVar1;
  int *piVar2;
  int iVar3;
  char cVar4;
  uint uVar5;
  int iVar6;
  uint *puVar7;
  byte bVar8;
  int iVar9;
  uint uVar10;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  uint local_c;
  
  uVar5 = param_1[4];
  uVar12 = param_3 + 0x17U & 0xfffffff0;
  uVar10 = param_2 - param_1[3] >> 0xf;
  iVar3 = uVar10 * 0x204 + 0x144 + uVar5;
  iVar6 = *(int *)(param_2 + -4);
  iVar9 = iVar6 + -1;
  uVar13 = *(uint *)(iVar6 + -5 + param_2);
  iVar6 = iVar6 + -5 + param_2;
  if (iVar9 < (int)uVar12) {
    if (((uVar13 & 1) != 0) || ((int)(uVar13 + iVar9) < (int)uVar12)) {
      return 0;
    }
    local_c = ((int)uVar13 >> 4) - 1;
    if (0x3f < local_c) {
      local_c = 0x3f;
    }
    if (*(int *)(iVar6 + 4) == *(int *)(iVar6 + 8)) {
      if (local_c < 0x20) {
        pcVar1 = (char *)(local_c + 4 + uVar5);
        uVar11 = ~(0x80000000U >> ((byte)local_c & 0x1f));
        puVar7 = (uint *)(uVar5 + 0x44 + uVar10 * 4);
        *puVar7 = *puVar7 & uVar11;
        *pcVar1 = *pcVar1 + -1;
        if (*pcVar1 == '\0') {
          *param_1 = *param_1 & uVar11;
        }
      }
      else {
        pcVar1 = (char *)(local_c + 4 + uVar5);
        uVar11 = ~(0x80000000U >> ((byte)local_c - 0x20 & 0x1f));
        puVar7 = (uint *)(uVar5 + 0xc4 + uVar10 * 4);
        *puVar7 = *puVar7 & uVar11;
        *pcVar1 = *pcVar1 + -1;
        if (*pcVar1 == '\0') {
          param_1[1] = param_1[1] & uVar11;
        }
      }
    }
    *(undefined4 *)(*(int *)(iVar6 + 8) + 4) = *(undefined4 *)(iVar6 + 4);
    *(undefined4 *)(*(int *)(iVar6 + 4) + 8) = *(undefined4 *)(iVar6 + 8);
    iVar6 = uVar13 + (iVar9 - uVar12);
    if (0 < iVar6) {
      uVar13 = (iVar6 >> 4) - 1;
      iVar9 = param_2 + -4 + uVar12;
      if (0x3f < uVar13) {
        uVar13 = 0x3f;
      }
      iVar3 = iVar3 + uVar13 * 8;
      *(undefined4 *)(iVar9 + 4) = *(undefined4 *)(iVar3 + 4);
      *(int *)(iVar9 + 8) = iVar3;
      *(int *)(iVar3 + 4) = iVar9;
      *(int *)(*(int *)(iVar9 + 4) + 8) = iVar9;
      if (*(int *)(iVar9 + 4) == *(int *)(iVar9 + 8)) {
        cVar4 = *(char *)(uVar13 + 4 + uVar5);
        *(char *)(uVar13 + 4 + uVar5) = cVar4 + '\x01';
        bVar8 = (byte)uVar13;
        if (uVar13 < 0x20) {
          if (cVar4 == '\0') {
            *param_1 = *param_1 | 0x80000000U >> (bVar8 & 0x1f);
          }
          puVar7 = (uint *)(uVar5 + 0x44 + uVar10 * 4);
        }
        else {
          if (cVar4 == '\0') {
            param_1[1] = param_1[1] | 0x80000000U >> (bVar8 - 0x20 & 0x1f);
          }
          puVar7 = (uint *)(uVar5 + 0xc4 + uVar10 * 4);
          bVar8 = bVar8 - 0x20;
        }
        *puVar7 = *puVar7 | 0x80000000U >> (bVar8 & 0x1f);
      }
      piVar2 = (int *)(param_2 + -4 + uVar12);
      *piVar2 = iVar6;
      *(int *)(iVar6 + -4 + (int)piVar2) = iVar6;
    }
    *(uint *)(param_2 + -4) = uVar12 + 1;
    *(uint *)(param_2 + -8 + uVar12) = uVar12 + 1;
  }
  else if ((int)uVar12 < iVar9) {
    param_3 = iVar9 - uVar12;
    *(uint *)(param_2 + -4) = uVar12 + 1;
    piVar2 = (int *)(param_2 + -4 + uVar12);
    uVar11 = (param_3 >> 4) - 1;
    piVar2[-1] = uVar12 + 1;
    if (0x3f < uVar11) {
      uVar11 = 0x3f;
    }
    if ((uVar13 & 1) == 0) {
      uVar12 = ((int)uVar13 >> 4) - 1;
      if (0x3f < uVar12) {
        uVar12 = 0x3f;
      }
      if (*(int *)(iVar6 + 4) == *(int *)(iVar6 + 8)) {
        if (uVar12 < 0x20) {
          pcVar1 = (char *)(uVar12 + 4 + uVar5);
          uVar12 = ~(0x80000000U >> ((byte)uVar12 & 0x1f));
          puVar7 = (uint *)(uVar5 + 0x44 + uVar10 * 4);
          *puVar7 = *puVar7 & uVar12;
          *pcVar1 = *pcVar1 + -1;
          if (*pcVar1 == '\0') {
            *param_1 = *param_1 & uVar12;
          }
        }
        else {
          pcVar1 = (char *)(uVar12 + 4 + uVar5);
          uVar12 = ~(0x80000000U >> ((byte)uVar12 - 0x20 & 0x1f));
          puVar7 = (uint *)(uVar5 + 0xc4 + uVar10 * 4);
          *puVar7 = *puVar7 & uVar12;
          *pcVar1 = *pcVar1 + -1;
          if (*pcVar1 == '\0') {
            param_1[1] = param_1[1] & uVar12;
          }
        }
      }
      *(undefined4 *)(*(int *)(iVar6 + 8) + 4) = *(undefined4 *)(iVar6 + 4);
      *(undefined4 *)(*(int *)(iVar6 + 4) + 8) = *(undefined4 *)(iVar6 + 8);
      param_3 = param_3 + uVar13;
      uVar11 = (param_3 >> 4) - 1;
      if (0x3f < uVar11) {
        uVar11 = 0x3f;
      }
    }
    iVar6 = iVar3 + uVar11 * 8;
    piVar2[1] = *(int *)(iVar3 + 4 + uVar11 * 8);
    piVar2[2] = iVar6;
    *(int **)(iVar6 + 4) = piVar2;
    *(int **)(piVar2[1] + 8) = piVar2;
    if (piVar2[1] == piVar2[2]) {
      cVar4 = *(char *)(uVar11 + 4 + uVar5);
      *(char *)(uVar11 + 4 + uVar5) = cVar4 + '\x01';
      bVar8 = (byte)uVar11;
      if (uVar11 < 0x20) {
        if (cVar4 == '\0') {
          *param_1 = *param_1 | 0x80000000U >> (bVar8 & 0x1f);
        }
        puVar7 = (uint *)(uVar5 + 0x44 + uVar10 * 4);
      }
      else {
        if (cVar4 == '\0') {
          param_1[1] = param_1[1] | 0x80000000U >> (bVar8 - 0x20 & 0x1f);
        }
        puVar7 = (uint *)(uVar5 + 0xc4 + uVar10 * 4);
        bVar8 = bVar8 - 0x20;
      }
      *puVar7 = *puVar7 | 0x80000000U >> (bVar8 & 0x1f);
    }
    *piVar2 = param_3;
    *(int *)(param_3 + -4 + (int)piVar2) = param_3;
  }
  return 1;
}



undefined ** FUN_00415cf6(void)

{
  bool bVar1;
  int *lpAddress;
  LPVOID pvVar2;
  undefined **ppuVar3;
  int iVar4;
  undefined **lpMem;
  
  if (DAT_00421278 == -1) {
    lpMem = &PTR_LOOP_00421268;
  }
  else {
    lpMem = HeapAlloc(DAT_00423dc8,0,0x2020);
    if (lpMem == (undefined **)0x0) {
      return (undefined **)0x0;
    }
  }
  lpAddress = VirtualAlloc((LPVOID)0x0,0x400000,0x2000,4);
  if (lpAddress != (int *)0x0) {
    pvVar2 = VirtualAlloc(lpAddress,0x10000,0x1000,4);
    if (pvVar2 != (LPVOID)0x0) {
      if (lpMem == &PTR_LOOP_00421268) {
        if (PTR_LOOP_00421268 == (undefined *)0x0) {
          PTR_LOOP_00421268 = (undefined *)&PTR_LOOP_00421268;
        }
        if (PTR_LOOP_0042126c == (undefined *)0x0) {
          PTR_LOOP_0042126c = (undefined *)&PTR_LOOP_00421268;
        }
      }
      else {
        *lpMem = (undefined *)&PTR_LOOP_00421268;
        lpMem[1] = PTR_LOOP_0042126c;
        PTR_LOOP_0042126c = (undefined *)lpMem;
        *(undefined ***)lpMem[1] = lpMem;
      }
      lpMem[5] = (undefined *)(lpAddress + 0x100000);
      ppuVar3 = lpMem + 6;
      lpMem[3] = (undefined *)(lpMem + 0x26);
      lpMem[4] = (undefined *)lpAddress;
      lpMem[2] = (undefined *)ppuVar3;
      iVar4 = 0;
      do {
        bVar1 = 0xf < iVar4;
        iVar4 = iVar4 + 1;
        *ppuVar3 = (undefined *)((bVar1 - 1 & 0xf1) - 1);
        ppuVar3[1] = (undefined *)0xf1;
        ppuVar3 = ppuVar3 + 2;
      } while (iVar4 < 0x400);
      _memset(lpAddress,0,0x10000);
      for (; lpAddress < lpMem[4] + 0x10000; lpAddress = lpAddress + 0x400) {
        *(undefined1 *)(lpAddress + 0x3e) = 0xff;
        *lpAddress = (int)(lpAddress + 2);
        lpAddress[1] = 0xf0;
      }
      return lpMem;
    }
    VirtualFree(lpAddress,0,0x8000);
  }
  if (lpMem != &PTR_LOOP_00421268) {
    HeapFree(DAT_00423dc8,0,lpMem);
  }
  return (undefined **)0x0;
}



void __cdecl FUN_00415e3a(undefined **param_1)

{
  VirtualFree(param_1[4],0,0x8000);
  if ((undefined **)PTR_LOOP_00423288 == param_1) {
    PTR_LOOP_00423288 = param_1[1];
  }
  if (param_1 != &PTR_LOOP_00421268) {
    *(undefined **)param_1[1] = *param_1;
    *(undefined **)(*param_1 + 4) = param_1[1];
    HeapFree(DAT_00423dc8,0,param_1);
    return;
  }
  DAT_00421278 = 0xffffffff;
  return;
}



void __cdecl FUN_00415e90(int param_1)

{
  BOOL BVar1;
  undefined **ppuVar2;
  int iVar3;
  undefined **ppuVar4;
  undefined **ppuVar5;
  int local_8;
  
  ppuVar4 = (undefined **)PTR_LOOP_0042126c;
  do {
    ppuVar5 = ppuVar4;
    if (ppuVar4[4] != (undefined *)0xffffffff) {
      local_8 = 0;
      ppuVar5 = ppuVar4 + 0x804;
      iVar3 = 0x3ff000;
      do {
        if (*ppuVar5 == (undefined *)0xf0) {
          BVar1 = VirtualFree(ppuVar4[4] + iVar3,0x1000,0x4000);
          if (BVar1 != 0) {
            *ppuVar5 = (undefined *)0xffffffff;
            DAT_00423bf0 = DAT_00423bf0 + -1;
            if (((undefined **)ppuVar4[3] == (undefined **)0x0) || (ppuVar5 < ppuVar4[3])) {
              ppuVar4[3] = (undefined *)ppuVar5;
            }
            local_8 = local_8 + 1;
            param_1 = param_1 + -1;
            if (param_1 == 0) break;
          }
        }
        iVar3 = iVar3 + -0x1000;
        ppuVar5 = ppuVar5 + -2;
      } while (-1 < iVar3);
      ppuVar5 = (undefined **)ppuVar4[1];
      if ((local_8 != 0) && (ppuVar4[6] == (undefined *)0xffffffff)) {
        ppuVar2 = ppuVar4 + 8;
        iVar3 = 1;
        do {
          if (*ppuVar2 != (undefined *)0xffffffff) break;
          iVar3 = iVar3 + 1;
          ppuVar2 = ppuVar2 + 2;
        } while (iVar3 < 0x400);
        if (iVar3 == 0x400) {
          FUN_00415e3a(ppuVar4);
        }
      }
    }
    if ((ppuVar5 == (undefined **)PTR_LOOP_0042126c) || (ppuVar4 = ppuVar5, param_1 < 1)) {
      return;
    }
  } while( true );
}



int __cdecl FUN_00415f52(undefined *param_1,undefined4 *param_2,uint *param_3)

{
  undefined **ppuVar1;
  uint uVar2;
  
  ppuVar1 = &PTR_LOOP_00421268;
  while ((param_1 <= ppuVar1[4] || (ppuVar1[5] <= param_1))) {
    ppuVar1 = (undefined **)*ppuVar1;
    if (ppuVar1 == &PTR_LOOP_00421268) {
      return 0;
    }
  }
  if (((uint)param_1 & 0xf) != 0) {
    return 0;
  }
  if (((uint)param_1 & 0xfff) < 0x100) {
    return 0;
  }
  *param_2 = ppuVar1;
  uVar2 = (uint)param_1 & 0xfffff000;
  *param_3 = uVar2;
  return ((int)(param_1 + (-0x100 - uVar2)) >> 4) + 8 + uVar2;
}



void __cdecl FUN_00415fa9(int param_1,int param_2,byte *param_3)

{
  int *piVar1;
  
  piVar1 = (int *)(param_1 + 0x18 + (param_2 - *(int *)(param_1 + 0x10) >> 0xc) * 8);
  *piVar1 = *piVar1 + (uint)*param_3;
  *param_3 = 0;
  piVar1[1] = 0xf1;
  if ((*piVar1 == 0xf0) && (DAT_00423bf0 = DAT_00423bf0 + 1, DAT_00423bf0 == 0x20)) {
    FUN_00415e90(0x10);
  }
  return;
}



// WARNING: Type propagation algorithm not settling

int * __cdecl FUN_00415fee(uint param_1)

{
  uint *puVar1;
  undefined **ppuVar2;
  undefined *puVar3;
  int *piVar4;
  int *piVar5;
  undefined **ppuVar6;
  int *piVar7;
  uint *puVar8;
  undefined **ppuVar9;
  int local_8;
  
  piVar7 = (int *)PTR_LOOP_00423288;
  do {
    if (piVar7[4] != -1) {
      puVar8 = (uint *)piVar7[2];
      piVar4 = (int *)(((int)puVar8 + (-0x18 - (int)piVar7) >> 3) * 0x1000 + piVar7[4]);
      if (puVar8 < piVar7 + 0x806) {
        do {
          if (((int)param_1 <= (int)*puVar8) && (param_1 < puVar8[1])) {
            piVar5 = (int *)FUN_004161f6(piVar4,*puVar8,param_1);
            if (piVar5 != (int *)0x0) goto LAB_004160b9;
            puVar8[1] = param_1;
          }
          puVar8 = puVar8 + 2;
          piVar4 = piVar4 + 0x400;
        } while (puVar8 < piVar7 + 0x806);
      }
      puVar1 = (uint *)piVar7[2];
      piVar4 = (int *)piVar7[4];
      for (puVar8 = (uint *)(piVar7 + 6); puVar8 < puVar1; puVar8 = puVar8 + 2) {
        if (((int)param_1 <= (int)*puVar8) && (param_1 < puVar8[1])) {
          piVar5 = (int *)FUN_004161f6(piVar4,*puVar8,param_1);
          if (piVar5 != (int *)0x0) {
LAB_004160b9:
            PTR_LOOP_00423288 = (undefined *)piVar7;
            *puVar8 = *puVar8 - param_1;
            piVar7[2] = (int)puVar8;
            return piVar5;
          }
          puVar8[1] = param_1;
        }
        piVar4 = piVar4 + 0x400;
      }
    }
    piVar7 = (int *)*piVar7;
    if (piVar7 == (int *)PTR_LOOP_00423288) {
      ppuVar9 = &PTR_LOOP_00421268;
      while ((ppuVar9[4] == (undefined *)0xffffffff || (ppuVar9[3] == (undefined *)0x0))) {
        ppuVar9 = (undefined **)*ppuVar9;
        if (ppuVar9 == &PTR_LOOP_00421268) {
          ppuVar9 = FUN_00415cf6();
          if (ppuVar9 == (undefined **)0x0) {
            return (int *)0x0;
          }
          piVar7 = (int *)ppuVar9[4];
          *(char *)(piVar7 + 2) = (char)param_1;
          PTR_LOOP_00423288 = (undefined *)ppuVar9;
          *piVar7 = (int)piVar7 + param_1 + 8;
          piVar7[1] = 0xf0 - param_1;
          ppuVar9[6] = ppuVar9[6] + -(param_1 & 0xff);
          return piVar7 + 0x40;
        }
      }
      ppuVar2 = (undefined **)ppuVar9[3];
      local_8 = 0;
      piVar7 = (int *)(ppuVar9[4] + ((int)ppuVar2 + (-0x18 - (int)ppuVar9) >> 3) * 0x1000);
      puVar3 = *ppuVar2;
      ppuVar6 = ppuVar2;
      for (; (puVar3 == (undefined *)0xffffffff && (local_8 < 0x10)); local_8 = local_8 + 1) {
        ppuVar6 = ppuVar6 + 2;
        puVar3 = *ppuVar6;
      }
      piVar4 = VirtualAlloc(piVar7,local_8 << 0xc,0x1000,4);
      if (piVar4 != piVar7) {
        return (int *)0x0;
      }
      _memset(piVar7,local_8 << 0xc,0);
      ppuVar6 = ppuVar2;
      if (0 < local_8) {
        piVar4 = piVar7 + 1;
        do {
          *(undefined1 *)(piVar4 + 0x3d) = 0xff;
          piVar4[-1] = (int)(piVar4 + 1);
          *piVar4 = 0xf0;
          *ppuVar6 = (undefined *)0xf0;
          ppuVar6[1] = (undefined *)0xf1;
          piVar4 = piVar4 + 0x400;
          ppuVar6 = ppuVar6 + 2;
          local_8 = local_8 + -1;
        } while (local_8 != 0);
      }
      for (; (ppuVar6 < ppuVar9 + 0x806 && (*ppuVar6 != (undefined *)0xffffffff));
          ppuVar6 = ppuVar6 + 2) {
      }
      PTR_LOOP_00423288 = (undefined *)ppuVar9;
      ppuVar9[3] = (undefined *)(-(uint)(ppuVar6 < ppuVar9 + 0x806) & (uint)ppuVar6);
      *(char *)(piVar7 + 2) = (char)param_1;
      ppuVar9[2] = (undefined *)ppuVar2;
      *ppuVar2 = *ppuVar2 + -param_1;
      piVar7[1] = piVar7[1] - param_1;
      *piVar7 = (int)piVar7 + param_1 + 8;
      return piVar7 + 0x40;
    }
  } while( true );
}



int __cdecl FUN_004161f6(int *param_1,uint param_2,uint param_3)

{
  byte *pbVar1;
  byte *pbVar2;
  byte bVar3;
  byte *pbVar4;
  uint uVar5;
  byte *pbVar6;
  
  pbVar2 = (byte *)*param_1;
  pbVar1 = (byte *)(param_1 + 0x3e);
  bVar3 = (byte)param_3;
  if ((uint)param_1[1] < param_3) {
    pbVar6 = pbVar2;
    if (pbVar2[param_1[1]] != 0) {
      pbVar6 = pbVar2 + param_1[1];
    }
    while( true ) {
      while( true ) {
        if (pbVar1 <= pbVar6 + param_3) {
          pbVar6 = (byte *)(param_1 + 2);
          while( true ) {
            while( true ) {
              if (pbVar2 <= pbVar6) {
                return 0;
              }
              if (pbVar1 <= pbVar6 + param_3) {
                return 0;
              }
              if (*pbVar6 == 0) break;
              pbVar6 = pbVar6 + *pbVar6;
            }
            uVar5 = 1;
            pbVar4 = pbVar6;
            while (pbVar4 = pbVar4 + 1, *pbVar4 == 0) {
              uVar5 = uVar5 + 1;
            }
            if (param_3 <= uVar5) break;
            param_2 = param_2 - uVar5;
            pbVar6 = pbVar4;
            if (param_2 < param_3) {
              return 0;
            }
          }
          if (pbVar6 + param_3 < pbVar1) {
            *param_1 = (int)(pbVar6 + param_3);
            param_1[1] = uVar5 - param_3;
          }
          else {
            param_1[1] = 0;
            *param_1 = (int)(param_1 + 2);
          }
          *pbVar6 = bVar3;
          pbVar2 = pbVar6 + 8;
          goto LAB_00416309;
        }
        if (*pbVar6 == 0) break;
        pbVar6 = pbVar6 + *pbVar6;
      }
      uVar5 = 1;
      pbVar4 = pbVar6;
      while (pbVar4 = pbVar4 + 1, *pbVar4 == 0) {
        uVar5 = uVar5 + 1;
      }
      if (param_3 <= uVar5) break;
      if (pbVar6 == pbVar2) {
        param_1[1] = uVar5;
        pbVar6 = pbVar4;
      }
      else {
        param_2 = param_2 - uVar5;
        pbVar6 = pbVar4;
        if (param_2 < param_3) {
          return 0;
        }
      }
    }
    if (pbVar6 + param_3 < pbVar1) {
      *param_1 = (int)(pbVar6 + param_3);
      param_1[1] = uVar5 - param_3;
    }
    else {
      param_1[1] = 0;
      *param_1 = (int)(param_1 + 2);
    }
    *pbVar6 = bVar3;
    pbVar2 = pbVar6 + 8;
  }
  else {
    *pbVar2 = bVar3;
    if (pbVar2 + param_3 < pbVar1) {
      *param_1 = *param_1 + param_3;
      param_1[1] = param_1[1] - param_3;
    }
    else {
      param_1[1] = 0;
      *param_1 = (int)(param_1 + 2);
    }
    pbVar2 = pbVar2 + 8;
  }
LAB_00416309:
  return (int)pbVar2 * 0x10 + (int)param_1 * -0xf;
}



undefined4 __cdecl FUN_0041631a(int param_1,int *param_2,byte *param_3,uint param_4)

{
  byte *pbVar1;
  int *piVar2;
  byte bVar3;
  byte *pbVar4;
  int iVar5;
  uint uVar6;
  
  uVar6 = (uint)*param_3;
  piVar2 = (int *)(param_1 + 0x18 + ((int)param_2 - *(int *)(param_1 + 0x10) >> 0xc) * 8);
  if (param_4 < uVar6) {
    *param_3 = (byte)param_4;
    *piVar2 = *piVar2 + (uVar6 - param_4);
    piVar2[1] = 0xf1;
  }
  else {
    if (param_4 <= uVar6) {
      return 0;
    }
    pbVar1 = param_3 + param_4;
    if (param_2 + 0x3e < pbVar1) {
      return 0;
    }
    for (pbVar4 = param_3 + uVar6; (pbVar4 < pbVar1 && (*pbVar4 == 0)); pbVar4 = pbVar4 + 1) {
    }
    if (pbVar4 != pbVar1) {
      return 0;
    }
    *param_3 = (byte)param_4;
    if ((param_3 <= (byte *)*param_2) && ((byte *)*param_2 < pbVar1)) {
      if (pbVar1 < param_2 + 0x3e) {
        iVar5 = 0;
        *param_2 = (int)pbVar1;
        bVar3 = *pbVar1;
        while (bVar3 == 0) {
          iVar5 = iVar5 + 1;
          bVar3 = pbVar1[iVar5];
        }
        param_2[1] = iVar5;
      }
      else {
        param_2[1] = 0;
        *param_2 = (int)(param_2 + 2);
      }
    }
    *piVar2 = *piVar2 + (uVar6 - param_4);
  }
  return 1;
}



uint __cdecl FUN_004163c3(undefined4 *param_1)

{
  byte bVar1;
  uint uVar2;
  int iVar3;
  undefined *puVar4;
  
  uVar2 = param_1[3];
  if (((uVar2 & 0x83) != 0) && ((uVar2 & 0x40) == 0)) {
    if ((uVar2 & 2) == 0) {
      param_1[3] = uVar2 | 1;
      if ((uVar2 & 0x10c) == 0) {
        FUN_00416d9a(param_1);
      }
      else {
        *param_1 = param_1[2];
      }
      iVar3 = FUN_0041649f(param_1[4],(char *)param_1[2],(char *)param_1[6]);
      param_1[1] = iVar3;
      if ((iVar3 != 0) && (iVar3 != -1)) {
        if ((param_1[3] & 0x82) == 0) {
          uVar2 = param_1[4];
          if (uVar2 == 0xffffffff) {
            puVar4 = &DAT_004211b0;
          }
          else {
            puVar4 = (undefined *)((&DAT_00423de0)[(int)uVar2 >> 5] + (uVar2 & 0x1f) * 0x24);
          }
          if ((puVar4[4] & 0x82) == 0x82) {
            param_1[3] = param_1[3] | 0x2000;
          }
        }
        if (((param_1[6] == 0x200) && ((param_1[3] & 8) != 0)) && ((param_1[3] & 0x400) == 0)) {
          param_1[6] = 0x1000;
        }
        param_1[1] = iVar3 + -1;
        bVar1 = *(byte *)*param_1;
        *param_1 = (byte *)*param_1 + 1;
        return (uint)bVar1;
      }
      param_1[3] = param_1[3] | (-(uint)(iVar3 != 0) & 0x10) + 0x10;
      param_1[1] = 0;
    }
    else {
      param_1[3] = uVar2 | 0x20;
    }
  }
  return 0xffffffff;
}



int __cdecl FUN_0041649f(uint param_1,char *param_2,char *param_3)

{
  int iVar1;
  DWORD *pDVar2;
  
  if ((param_1 < DAT_00423ee0) &&
     ((*(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + (param_1 & 0x1f) * 0x24) & 1) != 0)) {
    FUN_0041767f(param_1);
    iVar1 = FUN_00416504(param_1,param_2,param_3);
    FUN_004176de(param_1);
    return iVar1;
  }
  pDVar2 = FUN_00410d43();
  *pDVar2 = 9;
  pDVar2 = FUN_00410d4c();
  *pDVar2 = 0;
  return -1;
}



int __cdecl FUN_00416504(uint param_1,char *param_2,char *param_3)

{
  int *piVar1;
  byte *pbVar2;
  char cVar3;
  byte bVar4;
  BOOL BVar5;
  DWORD DVar6;
  DWORD *pDVar7;
  char *pcVar8;
  int iVar9;
  DWORD local_10;
  char *local_c;
  char local_5;
  
  local_c = (char *)0x0;
  if (param_3 != (char *)0x0) {
    piVar1 = &DAT_00423de0 + ((int)param_1 >> 5);
    iVar9 = (param_1 & 0x1f) * 0x24;
    bVar4 = *(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + iVar9 + 4);
    if ((bVar4 & 2) == 0) {
      pcVar8 = param_2;
      if (((bVar4 & 0x48) != 0) &&
         (cVar3 = *(char *)((&DAT_00423de0)[(int)param_1 >> 5] + iVar9 + 5), cVar3 != '\n')) {
        param_3 = param_3 + -1;
        *param_2 = cVar3;
        pcVar8 = param_2 + 1;
        local_c = (char *)0x1;
        *(undefined1 *)(*piVar1 + 5 + iVar9) = 10;
      }
      BVar5 = ReadFile(*(HANDLE *)(*piVar1 + iVar9),pcVar8,(DWORD)param_3,&local_10,
                       (LPOVERLAPPED)0x0);
      if (BVar5 == 0) {
        DVar6 = GetLastError();
        if (DVar6 == 5) {
          pDVar7 = FUN_00410d43();
          *pDVar7 = 9;
          pDVar7 = FUN_00410d4c();
          *pDVar7 = 5;
        }
        else {
          if (DVar6 == 0x6d) {
            return 0;
          }
          FUN_00410cd0(DVar6);
        }
        return -1;
      }
      bVar4 = *(byte *)(*piVar1 + 4 + iVar9);
      if ((bVar4 & 0x80) == 0) {
        return (int)local_c + local_10;
      }
      if ((local_10 == 0) || (*param_2 != '\n')) {
        bVar4 = bVar4 & 0xfb;
      }
      else {
        bVar4 = bVar4 | 4;
      }
      *(byte *)(*piVar1 + 4 + iVar9) = bVar4;
      param_3 = param_2;
      local_c = param_2 + (int)local_c + local_10;
      pcVar8 = param_2;
      if (param_2 < local_c) {
        do {
          cVar3 = *param_3;
          if (cVar3 == '\x1a') {
            pbVar2 = (byte *)(*piVar1 + 4 + iVar9);
            bVar4 = *pbVar2;
            if ((bVar4 & 0x40) == 0) {
              *pbVar2 = bVar4 | 2;
            }
            break;
          }
          if (cVar3 == '\r') {
            if (param_3 < local_c + -1) {
              if (param_3[1] == '\n') {
                param_3 = param_3 + 2;
                goto LAB_0041668f;
              }
              *pcVar8 = '\r';
              pcVar8 = pcVar8 + 1;
              param_3 = param_3 + 1;
            }
            else {
              param_3 = param_3 + 1;
              BVar5 = ReadFile(*(HANDLE *)(*piVar1 + iVar9),&local_5,1,&local_10,(LPOVERLAPPED)0x0);
              if (((BVar5 == 0) && (DVar6 = GetLastError(), DVar6 != 0)) || (local_10 == 0)) {
LAB_004166a9:
                *pcVar8 = '\r';
LAB_004166ac:
                pcVar8 = pcVar8 + 1;
              }
              else if ((*(byte *)(*piVar1 + 4 + iVar9) & 0x48) == 0) {
                if ((pcVar8 == param_2) && (local_5 == '\n')) {
LAB_0041668f:
                  *pcVar8 = '\n';
                  goto LAB_004166ac;
                }
                FUN_0041506a(param_1,-1,1);
                if (local_5 != '\n') goto LAB_004166a9;
              }
              else {
                if (local_5 == '\n') goto LAB_0041668f;
                *pcVar8 = '\r';
                pcVar8 = pcVar8 + 1;
                *(char *)(*piVar1 + 5 + iVar9) = local_5;
              }
            }
          }
          else {
            *pcVar8 = cVar3;
            pcVar8 = pcVar8 + 1;
            param_3 = param_3 + 1;
          }
        } while (param_3 < local_c);
      }
      return (int)pcVar8 - (int)param_2;
    }
  }
  return 0;
}



undefined4 __cdecl FUN_004166dd(uint param_1)

{
  HANDLE hFile;
  BOOL BVar1;
  DWORD DVar2;
  DWORD *pDVar3;
  int iVar4;
  undefined4 uVar5;
  
  if (DAT_00423ee0 <= param_1) {
LAB_0041675e:
    pDVar3 = FUN_00410d43();
    *pDVar3 = 9;
    return 0xffffffff;
  }
  iVar4 = (param_1 & 0x1f) * 0x24;
  if ((*(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + iVar4) & 1) == 0) goto LAB_0041675e;
  FUN_0041767f(param_1);
  if ((*(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + iVar4) & 1) != 0) {
    hFile = (HANDLE)FUN_0041763d(param_1);
    BVar1 = FlushFileBuffers(hFile);
    if (BVar1 == 0) {
      DVar2 = GetLastError();
    }
    else {
      DVar2 = 0;
    }
    uVar5 = 0;
    if (DVar2 == 0) goto LAB_00416753;
    pDVar3 = FUN_00410d4c();
    *pDVar3 = DVar2;
  }
  pDVar3 = FUN_00410d43();
  *pDVar3 = 9;
  uVar5 = 0xffffffff;
LAB_00416753:
  FUN_004176de(param_1);
  return uVar5;
}



uint * __cdecl FUN_00416770(uint *param_1,uint *param_2)

{
  byte bVar1;
  uint uVar2;
  uint uVar3;
  uint *puVar4;
  
  puVar4 = param_1;
  while (((uint)param_2 & 3) != 0) {
    bVar1 = (byte)*param_2;
    uVar3 = (uint)bVar1;
    param_2 = (uint *)((int)param_2 + 1);
    if (bVar1 == 0) goto LAB_00416858;
    *(byte *)puVar4 = bVar1;
    puVar4 = (uint *)((int)puVar4 + 1);
  }
  do {
    uVar2 = *param_2;
    uVar3 = *param_2;
    param_2 = param_2 + 1;
    if (((uVar2 ^ 0xffffffff ^ uVar2 + 0x7efefeff) & 0x81010100) != 0) {
      if ((char)uVar3 == '\0') {
LAB_00416858:
        *(byte *)puVar4 = (byte)uVar3;
        return param_1;
      }
      if ((char)(uVar3 >> 8) == '\0') {
        *(short *)puVar4 = (short)uVar3;
        return param_1;
      }
      if ((uVar3 & 0xff0000) == 0) {
        *(short *)puVar4 = (short)uVar3;
        *(byte *)((int)puVar4 + 2) = 0;
        return param_1;
      }
      if ((uVar3 & 0xff000000) == 0) {
        *puVar4 = uVar3;
        return param_1;
      }
    }
    *puVar4 = uVar3;
    puVar4 = puVar4 + 1;
  } while( true );
}



uint * __cdecl FUN_00416780(uint *param_1,uint *param_2)

{
  byte bVar1;
  uint uVar2;
  uint *puVar3;
  uint uVar4;
  uint *puVar5;
  
  puVar3 = param_1;
  do {
    if (((uint)puVar3 & 3) == 0) goto LAB_0041679c;
    uVar4 = *puVar3;
    puVar3 = (uint *)((int)puVar3 + 1);
  } while ((byte)uVar4 != 0);
  goto LAB_004167cf;
  while( true ) {
    if ((uVar4 & 0xff0000) == 0) {
      puVar5 = (uint *)((int)puVar5 + 2);
      goto joined_r0x004167eb;
    }
    if ((uVar4 & 0xff000000) == 0) break;
LAB_0041679c:
    do {
      puVar5 = puVar3;
      puVar3 = puVar5 + 1;
    } while (((*puVar5 ^ 0xffffffff ^ *puVar5 + 0x7efefeff) & 0x81010100) == 0);
    uVar4 = *puVar5;
    if ((char)uVar4 == '\0') goto joined_r0x004167eb;
    if ((char)(uVar4 >> 8) == '\0') {
      puVar5 = (uint *)((int)puVar5 + 1);
      goto joined_r0x004167eb;
    }
  }
LAB_004167cf:
  puVar5 = (uint *)((int)puVar3 + -1);
joined_r0x004167eb:
  do {
    if (((uint)param_2 & 3) == 0) {
      do {
        uVar2 = *param_2;
        uVar4 = *param_2;
        param_2 = param_2 + 1;
        if (((uVar2 ^ 0xffffffff ^ uVar2 + 0x7efefeff) & 0x81010100) != 0) {
          if ((char)uVar4 == '\0') {
LAB_00416858:
            *(byte *)puVar5 = (byte)uVar4;
            return param_1;
          }
          if ((char)(uVar4 >> 8) == '\0') {
            *(short *)puVar5 = (short)uVar4;
            return param_1;
          }
          if ((uVar4 & 0xff0000) == 0) {
            *(short *)puVar5 = (short)uVar4;
            *(byte *)((int)puVar5 + 2) = 0;
            return param_1;
          }
          if ((uVar4 & 0xff000000) == 0) {
            *puVar5 = uVar4;
            return param_1;
          }
        }
        *puVar5 = uVar4;
        puVar5 = puVar5 + 1;
      } while( true );
    }
    bVar1 = (byte)*param_2;
    uVar4 = (uint)bVar1;
    param_2 = (uint *)((int)param_2 + 1);
    if (bVar1 == 0) goto LAB_00416858;
    *(byte *)puVar5 = bVar1;
    puVar5 = (uint *)((int)puVar5 + 1);
  } while( true );
}



void __fastcall FUN_00416894(void *param_1)

{
  FUN_0041838c(param_1,0x10000,0x30000);
  return;
}



// WARNING: Removing unreachable block (ram,0x004168db)

undefined4 FUN_004168a6(void)

{
  return 0;
}



void FUN_004168e4(void)

{
  HMODULE hModule;
  FARPROC pFVar1;
  
  hModule = GetModuleHandleA("KERNEL32");
  if (hModule != (HMODULE)0x0) {
    pFVar1 = GetProcAddress(hModule,"IsProcessorFeaturePresent");
    if (pFVar1 != (FARPROC)0x0) {
      (*pFVar1)(0);
      return;
    }
  }
  FUN_004168a6();
  return;
}



void __cdecl FUN_0041690d(char *param_1)

{
  char cVar1;
  char cVar2;
  undefined *this;
  uint uVar3;
  undefined *puVar4;
  
  this = (undefined *)(int)*param_1;
  uVar3 = FUN_00417161((uint)this);
  if (uVar3 != 0x65) {
    do {
      param_1 = param_1 + 1;
      if (DAT_00420f1c < 2) {
        uVar3 = (byte)PTR_DAT_00420d10[*param_1 * 2] & 4;
        this = PTR_DAT_00420d10;
      }
      else {
        puVar4 = (undefined *)0x4;
        uVar3 = FUN_00413dd7(this,(int)*param_1,4);
        this = puVar4;
      }
    } while (uVar3 != 0);
  }
  cVar2 = *param_1;
  *param_1 = DAT_00420f20;
  do {
    param_1 = param_1 + 1;
    cVar1 = *param_1;
    *param_1 = cVar2;
    cVar2 = cVar1;
  } while (*param_1 != '\0');
  return;
}



// Library Function - Single Match
//  __fassign
// 
// Library: Visual Studio 2003 Release

void __cdecl __fassign(int flag,char *argument,char *number)

{
  void *in_ECX;
  void *local_c;
  void *local_8;
  
  if (flag != 0) {
    local_c = in_ECX;
    local_8 = in_ECX;
    FUN_00417e2c(in_ECX,(uint *)&local_c,(byte *)number);
    *(void **)argument = local_c;
    *(void **)(argument + 4) = local_8;
    return;
  }
  FUN_00417e59(in_ECX,(uint *)&number,(byte *)number);
  *(char **)argument = number;
  return;
}



undefined1 * __cdecl FUN_00416a0b(undefined8 *param_1,undefined1 *param_2,int param_3,int param_4)

{
  uint local_2c [6];
  int local_14 [4];
  
  FUN_00418534((int)*param_1,(int)((ulonglong)*param_1 >> 0x20),local_14,local_2c);
  FUN_004184bd(param_2 + (uint)(0 < param_3) + (uint)(local_14[0] == 0x2d),param_3 + 1,(int)local_14
              );
  FUN_00416a6c(param_2,param_3,param_4,local_14,'\0');
  return param_2;
}



undefined1 * __cdecl
FUN_00416a6c(undefined1 *param_1,int param_2,int param_3,int *param_4,char param_5)

{
  undefined1 *puVar1;
  undefined1 *puVar2;
  uint *puVar3;
  int iVar4;
  
  if (param_5 != '\0') {
    FUN_00416d0e(param_1 + (*param_4 == 0x2d),(uint)(0 < param_2));
  }
  puVar1 = param_1;
  if (*param_4 == 0x2d) {
    *param_1 = 0x2d;
    puVar1 = param_1 + 1;
  }
  puVar2 = puVar1;
  if (0 < param_2) {
    puVar2 = puVar1 + 1;
    *puVar1 = puVar1[1];
    *puVar2 = DAT_00420f20;
  }
  puVar3 = FUN_00416770((uint *)(puVar2 + param_2 + (uint)(param_5 == '\0')),(uint *)"e+000");
  if (param_3 != 0) {
    *(undefined1 *)puVar3 = 0x45;
  }
  if (*(char *)param_4[3] != '0') {
    iVar4 = param_4[1] + -1;
    if (iVar4 < 0) {
      iVar4 = -iVar4;
      *(undefined1 *)((int)puVar3 + 1) = 0x2d;
    }
    if (99 < iVar4) {
      *(char *)((int)puVar3 + 2) = *(char *)((int)puVar3 + 2) + (char)(iVar4 / 100);
      iVar4 = iVar4 % 100;
    }
    if (9 < iVar4) {
      *(char *)((int)puVar3 + 3) = *(char *)((int)puVar3 + 3) + (char)(iVar4 / 10);
      iVar4 = iVar4 % 10;
    }
    *(char *)(puVar3 + 1) = (char)puVar3[1] + (char)iVar4;
  }
  return param_1;
}



char * __cdecl FUN_00416b2e(undefined8 *param_1,char *param_2,size_t param_3)

{
  uint local_2c [6];
  int local_14;
  int local_10;
  
  FUN_00418534((int)*param_1,(int)((ulonglong)*param_1 >> 0x20),&local_14,local_2c);
  FUN_004184bd(param_2 + (local_14 == 0x2d),local_10 + param_3,(int)&local_14);
  FUN_00416b83(param_2,param_3,&local_14,'\0');
  return param_2;
}



char * __cdecl FUN_00416b83(char *param_1,size_t param_2,int *param_3,char param_4)

{
  int iVar1;
  int iVar2;
  char *pcVar3;
  
  iVar1 = param_3[1];
  if ((param_4 != '\0') && (iVar1 - 1U == param_2)) {
    iVar2 = *param_3;
    param_1[(uint)(iVar2 == 0x2d) + (iVar1 - 1U)] = '0';
    (param_1 + (uint)(iVar2 == 0x2d) + (iVar1 - 1U))[1] = '\0';
  }
  pcVar3 = param_1;
  if (*param_3 == 0x2d) {
    *param_1 = '-';
    pcVar3 = param_1 + 1;
  }
  if (param_3[1] < 1) {
    FUN_00416d0e(pcVar3,1);
    *pcVar3 = '0';
    pcVar3 = pcVar3 + 1;
  }
  else {
    pcVar3 = pcVar3 + param_3[1];
  }
  if (0 < (int)param_2) {
    FUN_00416d0e(pcVar3,1);
    *pcVar3 = DAT_00420f20;
    iVar1 = param_3[1];
    if (iVar1 < 0) {
      if ((param_4 != '\0') || (-iVar1 <= (int)param_2)) {
        param_2 = -iVar1;
      }
      FUN_00416d0e(pcVar3 + 1,param_2);
      _memset(pcVar3 + 1,0x30,param_2);
    }
  }
  return param_1;
}



void __cdecl FUN_00416c2a(undefined8 *param_1,char *param_2,size_t param_3,int param_4)

{
  int iVar1;
  char *pcVar2;
  char *pcVar3;
  uint local_2c [6];
  int local_14;
  int local_10;
  
  FUN_00418534((int)*param_1,(int)((ulonglong)*param_1 >> 0x20),&local_14,local_2c);
  iVar1 = local_10 + -1;
  FUN_004184bd(param_2 + (local_14 == 0x2d),param_3,(int)&local_14);
  local_10 = local_10 + -1;
  if ((local_10 < -4) || ((int)param_3 <= local_10)) {
    FUN_00416a6c(param_2,param_3,param_4,&local_14,'\x01');
  }
  else {
    pcVar2 = param_2 + (local_14 == 0x2d);
    if (iVar1 < local_10) {
      do {
        pcVar3 = pcVar2;
        pcVar2 = pcVar3 + 1;
      } while (*pcVar3 != '\0');
      pcVar3[-1] = '\0';
    }
    FUN_00416b83(param_2,param_3,&local_14,'\x01');
  }
  return;
}



// Library Function - Single Match
//  __cfltcvt
// 
// Library: Visual Studio 2003 Release

errno_t __cdecl
__cfltcvt(double *arg,char *buffer,size_t sizeInBytes,int format,int precision,int caps)

{
  char *pcVar1;
  undefined1 *puVar2;
  
  if ((sizeInBytes == 0x65) || (sizeInBytes == 0x45)) {
    puVar2 = FUN_00416a0b(arg,buffer,format,precision);
  }
  else {
    if (sizeInBytes == 0x66) {
      pcVar1 = FUN_00416b2e(arg,buffer,format);
      return (errno_t)pcVar1;
    }
    puVar2 = (undefined1 *)FUN_00416c2a(arg,buffer,format,precision);
  }
  return (errno_t)puVar2;
}



void __cdecl FUN_00416d0e(char *param_1,int param_2)

{
  size_t sVar1;
  
  if (param_2 != 0) {
    sVar1 = _strlen(param_1);
    FUN_0040f710((undefined4 *)(param_1 + param_2),(undefined4 *)param_1,sVar1 + 1);
  }
  return;
}



bool __cdecl FUN_00416d33(void *param_1,UINT_PTR param_2)

{
  BOOL BVar1;
  
  BVar1 = IsBadReadPtr(param_1,param_2);
  return BVar1 == 0;
}



bool __cdecl FUN_00416d4f(LPVOID param_1,UINT_PTR param_2)

{
  BOOL BVar1;
  
  BVar1 = IsBadWritePtr(param_1,param_2);
  return BVar1 == 0;
}



bool __cdecl FUN_00416d6b(FARPROC param_1)

{
  BOOL BVar1;
  
  BVar1 = IsBadCodePtr(param_1);
  return BVar1 == 0;
}



void FUN_00416d83(void)

{
  FUN_00414eb2((undefined *)0xa);
  FUN_0041864f((DWORD *)0x16);
                    // WARNING: Subroutine does not return
  __exit(3);
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

void __cdecl FUN_00416d9a(undefined4 *param_1)

{
  void *pvVar1;
  
  _DAT_00423acc = _DAT_00423acc + 1;
  pvVar1 = _malloc(0x1000);
  param_1[2] = pvVar1;
  if (pvVar1 == (void *)0x0) {
    param_1[3] = param_1[3] | 4;
    param_1[2] = param_1 + 5;
    param_1[6] = 2;
  }
  else {
    param_1[3] = param_1[3] | 8;
    param_1[6] = 0x1000;
  }
  param_1[1] = 0;
  *param_1 = param_1[2];
  return;
}



byte __cdecl FUN_00416dde(uint param_1)

{
  if (DAT_00423ee0 <= param_1) {
    return 0;
  }
  return *(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + (param_1 & 0x1f) * 0x24) & 0x40;
}



int __cdecl FUN_00416e07(LPSTR param_1,WCHAR param_2)

{
  int iVar1;
  bool bVar2;
  
  InterlockedIncrement((LONG *)&DAT_00423da8);
  bVar2 = DAT_00423da4 != 0;
  if (bVar2) {
    InterlockedDecrement((LONG *)&DAT_00423da8);
    FUN_004130b6(0x13);
  }
  iVar1 = FUN_00416e60(param_1,param_2);
  if (bVar2) {
    FUN_00413117(0x13);
  }
  else {
    InterlockedDecrement((LONG *)&DAT_00423da8);
  }
  return iVar1;
}



int __cdecl FUN_00416e60(LPSTR param_1,WCHAR param_2)

{
  LPSTR lpMultiByteStr;
  int iVar1;
  DWORD *pDVar2;
  
  lpMultiByteStr = param_1;
  if (param_1 == (LPSTR)0x0) {
    return 0;
  }
  if (DAT_00423bfc == 0) {
    if ((ushort)param_2 < 0x100) {
      *param_1 = (CHAR)param_2;
      return 1;
    }
  }
  else {
    param_1 = (LPSTR)0x0;
    iVar1 = WideCharToMultiByte(DAT_00423c0c,0x220,&param_2,1,lpMultiByteStr,DAT_00420f1c,
                                (LPCSTR)0x0,(LPBOOL)&param_1);
    if ((iVar1 != 0) && (param_1 == (LPSTR)0x0)) {
      return iVar1;
    }
  }
  pDVar2 = FUN_00410d43();
  *pDVar2 = 0x2a;
  return -1;
}



// Library Function - Single Match
//  __aulldiv
// 
// Library: Visual Studio

undefined8 __aulldiv(uint param_1,uint param_2,uint param_3,uint param_4)

{
  ulonglong uVar1;
  longlong lVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar6;
  
  uVar9 = param_1;
  uVar6 = param_4;
  uVar7 = param_2;
  uVar3 = param_3;
  if (param_4 == 0) {
    uVar3 = param_2 / param_3;
    iVar4 = (int)(((ulonglong)param_2 % (ulonglong)param_3 << 0x20 | (ulonglong)param_1) /
                 (ulonglong)param_3);
  }
  else {
    do {
      uVar5 = uVar6 >> 1;
      uVar3 = (uint)(CONCAT14((uVar6 & 1) != 0,uVar3) >> 1);
      uVar8 = uVar7 >> 1;
      uVar9 = (uint)(CONCAT14((uVar7 & 1) != 0,uVar9) >> 1);
      uVar6 = uVar5;
      uVar7 = uVar8;
    } while (uVar5 != 0);
    uVar1 = CONCAT44(uVar8,uVar9) / (ulonglong)uVar3;
    iVar4 = (int)uVar1;
    lVar2 = (ulonglong)param_3 * (uVar1 & 0xffffffff);
    uVar3 = (uint)((ulonglong)lVar2 >> 0x20);
    uVar9 = uVar3 + iVar4 * param_4;
    if (((CARRY4(uVar3,iVar4 * param_4)) || (param_2 < uVar9)) ||
       ((param_2 <= uVar9 && (param_1 < (uint)lVar2)))) {
      iVar4 = iVar4 + -1;
    }
    uVar3 = 0;
  }
  return CONCAT44(uVar3,iVar4);
}



// Library Function - Single Match
//  __aullrem
// 
// Library: Visual Studio

undefined8 __aullrem(uint param_1,uint param_2,uint param_3,uint param_4)

{
  ulonglong uVar1;
  longlong lVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  bool bVar11;
  
  uVar4 = param_1;
  uVar9 = param_4;
  uVar10 = param_2;
  uVar3 = param_3;
  if (param_4 == 0) {
    iVar6 = (int)(((ulonglong)param_2 % (ulonglong)param_3 << 0x20 | (ulonglong)param_1) %
                 (ulonglong)param_3);
    iVar7 = 0;
  }
  else {
    do {
      uVar5 = uVar9 >> 1;
      uVar3 = (uint)(CONCAT14((uVar9 & 1) != 0,uVar3) >> 1);
      uVar8 = uVar10 >> 1;
      uVar4 = (uint)(CONCAT14((uVar10 & 1) != 0,uVar4) >> 1);
      uVar9 = uVar5;
      uVar10 = uVar8;
    } while (uVar5 != 0);
    uVar1 = CONCAT44(uVar8,uVar4) / (ulonglong)uVar3;
    uVar3 = (int)uVar1 * param_4;
    lVar2 = (uVar1 & 0xffffffff) * (ulonglong)param_3;
    uVar9 = (uint)((ulonglong)lVar2 >> 0x20);
    uVar4 = (uint)lVar2;
    uVar10 = uVar9 + uVar3;
    if (((CARRY4(uVar9,uVar3)) || (param_2 < uVar10)) || ((param_2 <= uVar10 && (param_1 < uVar4))))
    {
      bVar11 = uVar4 < param_3;
      uVar4 = uVar4 - param_3;
      uVar10 = (uVar10 - param_4) - (uint)bVar11;
    }
    iVar6 = -(uVar4 - param_1);
    iVar7 = -(uint)(uVar4 - param_1 != 0) - ((uVar10 - param_2) - (uint)(uVar4 < param_1));
  }
  return CONCAT44(iVar7,iVar6);
}



// Library Function - Single Match
//  _memset
// 
// Libraries: Visual Studio 1998 Debug, Visual Studio 1998 Release

void * __cdecl _memset(void *_Dst,int _Val,size_t _Size)

{
  uint uVar1;
  uint uVar2;
  size_t sVar3;
  uint *puVar4;
  
  if (_Size == 0) {
    return _Dst;
  }
  uVar1 = _Val & 0xff;
  puVar4 = _Dst;
  if (3 < _Size) {
    uVar2 = -(int)_Dst & 3;
    sVar3 = _Size;
    if (uVar2 != 0) {
      sVar3 = _Size - uVar2;
      do {
        *(undefined1 *)puVar4 = (undefined1)_Val;
        puVar4 = (uint *)((int)puVar4 + 1);
        uVar2 = uVar2 - 1;
      } while (uVar2 != 0);
    }
    uVar1 = uVar1 * 0x1010101;
    _Size = sVar3 & 3;
    uVar2 = sVar3 >> 2;
    if (uVar2 != 0) {
      for (; uVar2 != 0; uVar2 = uVar2 - 1) {
        *puVar4 = uVar1;
        puVar4 = puVar4 + 1;
      }
      if (_Size == 0) {
        return _Dst;
      }
    }
  }
  do {
    *(char *)puVar4 = (char)uVar1;
    puVar4 = (uint *)((int)puVar4 + 1);
    _Size = _Size - 1;
  } while (_Size != 0);
  return _Dst;
}



BOOL __cdecl
FUN_00417018(DWORD param_1,LPCSTR param_2,int param_3,LPWORD param_4,UINT param_5,LCID param_6,
            int param_7)

{
  undefined1 *puVar1;
  BOOL BVar2;
  int iVar3;
  WORD local_20 [2];
  undefined1 *local_1c;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &DAT_0041e098;
  puStack_10 = &LAB_004134d8;
  local_14 = ExceptionList;
  local_1c = &stack0xffffffc8;
  iVar3 = DAT_00423c14;
  ExceptionList = &local_14;
  puVar1 = &stack0xffffffc8;
  if (DAT_00423c14 == 0) {
    ExceptionList = &local_14;
    BVar2 = GetStringTypeW(1,L"",1,local_20);
    iVar3 = 1;
    puVar1 = local_1c;
    if (BVar2 == 0) {
      BVar2 = GetStringTypeA(0,1,"",1,local_20);
      if (BVar2 == 0) {
        ExceptionList = local_14;
        return 0;
      }
      iVar3 = 2;
      puVar1 = local_1c;
    }
  }
  local_1c = puVar1;
  DAT_00423c14 = iVar3;
  if (DAT_00423c14 != 2) {
    if (DAT_00423c14 == 1) {
      if (param_5 == 0) {
        param_5 = DAT_00423c0c;
      }
      iVar3 = MultiByteToWideChar(param_5,(-(uint)(param_7 != 0) & 8) + 1,param_2,param_3,
                                  (LPWSTR)0x0,0);
      if (iVar3 != 0) {
        local_8 = 0;
        FUN_004101e0();
        local_1c = &stack0xffffffc8;
        _memset(&stack0xffffffc8,0,iVar3 * 2);
        local_8 = 0xffffffff;
        if ((&stack0x00000000 != (undefined1 *)0x38) &&
           (iVar3 = MultiByteToWideChar(param_5,1,param_2,param_3,(LPWSTR)&stack0xffffffc8,iVar3),
           iVar3 != 0)) {
          BVar2 = GetStringTypeW(param_1,(LPCWSTR)&stack0xffffffc8,iVar3,param_4);
          ExceptionList = local_14;
          return BVar2;
        }
      }
    }
    ExceptionList = local_14;
    return 0;
  }
  if (param_6 == 0) {
    param_6 = DAT_00423bfc;
  }
  BVar2 = GetStringTypeA(param_6,param_1,param_2,param_3,param_4);
  ExceptionList = local_14;
  return BVar2;
}



uint __cdecl FUN_00417161(uint param_1)

{
  void *extraout_ECX;
  bool bVar1;
  void *this;
  
  if (DAT_00423bfc == 0) {
    if ((0x40 < (int)param_1) && ((int)param_1 < 0x5b)) {
      return param_1 + 0x20;
    }
  }
  else {
    InterlockedIncrement((LONG *)&DAT_00423da8);
    bVar1 = DAT_00423da4 != 0;
    this = extraout_ECX;
    if (bVar1) {
      InterlockedDecrement((LONG *)&DAT_00423da8);
      this = (void *)0x13;
      FUN_004130b6(0x13);
    }
    param_1 = FUN_004171d0(this,param_1);
    if (bVar1) {
      FUN_00413117(0x13);
    }
    else {
      InterlockedDecrement((LONG *)&DAT_00423da8);
    }
  }
  return param_1;
}



uint __thiscall FUN_004171d0(void *this,uint param_1)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  void *local_8;
  
  uVar1 = param_1;
  if (DAT_00423bfc == 0) {
    if ((0x40 < (int)param_1) && ((int)param_1 < 0x5b)) {
      uVar1 = param_1 + 0x20;
    }
  }
  else {
    iVar3 = 1;
    local_8 = this;
    if ((int)param_1 < 0x100) {
      if (DAT_00420f1c < 2) {
        uVar2 = (byte)PTR_DAT_00420d10[param_1 * 2] & 1;
      }
      else {
        uVar2 = FUN_00413dd7(this,param_1,1);
      }
      if (uVar2 == 0) {
        return uVar1;
      }
    }
    if ((PTR_DAT_00420d10[((int)uVar1 >> 8 & 0xffU) * 2 + 1] & 0x80) == 0) {
      param_1 = CONCAT31((int3)(param_1 >> 8),(char)uVar1) & 0xffff00ff;
    }
    else {
      uVar2 = param_1 >> 0x10;
      param_1._0_2_ = CONCAT11((char)uVar1,(char)(uVar1 >> 8));
      param_1 = CONCAT22((short)uVar2,(undefined2)param_1) & 0xff00ffff;
      iVar3 = 2;
    }
    iVar3 = FUN_004139a4(DAT_00423bfc,0x100,(char *)&param_1,iVar3,(LPWSTR)&local_8,3,0,1);
    if (iVar3 != 0) {
      if (iVar3 == 1) {
        uVar1 = (uint)local_8 & 0xff;
      }
      else {
        uVar1 = (uint)local_8 & 0xffff;
      }
    }
  }
  return uVar1;
}



// Library Function - Single Match
//  _strncpy
// 
// Libraries: Visual Studio 1998 Debug, Visual Studio 1998 Release

char * __cdecl _strncpy(char *_Dest,char *_Source,size_t _Count)

{
  uint uVar1;
  uint uVar2;
  char cVar3;
  uint uVar4;
  uint *puVar5;
  
  if (_Count == 0) {
    return _Dest;
  }
  puVar5 = (uint *)_Dest;
  if (((uint)_Source & 3) != 0) {
    while( true ) {
      uVar4 = *(uint *)_Source;
      _Source = (char *)((int)_Source + 1);
      *(char *)puVar5 = (char)uVar4;
      puVar5 = (uint *)((int)puVar5 + 1);
      _Count = _Count - 1;
      if (_Count == 0) {
        return _Dest;
      }
      if ((char)uVar4 == '\0') break;
      if (((uint)_Source & 3) == 0) {
        uVar4 = _Count >> 2;
        goto joined_r0x004172de;
      }
    }
    do {
      if (((uint)puVar5 & 3) == 0) {
        uVar4 = _Count >> 2;
        cVar3 = '\0';
        if (uVar4 == 0) goto LAB_0041731b;
        goto LAB_00417389;
      }
      *(char *)puVar5 = '\0';
      puVar5 = (uint *)((int)puVar5 + 1);
      _Count = _Count - 1;
    } while (_Count != 0);
    return _Dest;
  }
  uVar4 = _Count >> 2;
  if (uVar4 != 0) {
    do {
      uVar1 = *(uint *)_Source;
      uVar2 = *(uint *)_Source;
      _Source = (char *)((int)_Source + 4);
      if (((uVar1 ^ 0xffffffff ^ uVar1 + 0x7efefeff) & 0x81010100) != 0) {
        if ((char)uVar2 == '\0') {
          *puVar5 = 0;
joined_r0x00417385:
          while( true ) {
            uVar4 = uVar4 - 1;
            puVar5 = puVar5 + 1;
            if (uVar4 == 0) break;
LAB_00417389:
            *puVar5 = 0;
          }
          cVar3 = '\0';
          _Count = _Count & 3;
          if (_Count != 0) goto LAB_0041731b;
          return _Dest;
        }
        if ((char)(uVar2 >> 8) == '\0') {
          *puVar5 = uVar2 & 0xff;
          goto joined_r0x00417385;
        }
        if ((uVar2 & 0xff0000) == 0) {
          *puVar5 = uVar2 & 0xffff;
          goto joined_r0x00417385;
        }
        if ((uVar2 & 0xff000000) == 0) {
          *puVar5 = uVar2;
          goto joined_r0x00417385;
        }
      }
      *puVar5 = uVar2;
      puVar5 = puVar5 + 1;
      uVar4 = uVar4 - 1;
joined_r0x004172de:
    } while (uVar4 != 0);
    _Count = _Count & 3;
    if (_Count == 0) {
      return _Dest;
    }
  }
  do {
    cVar3 = (char)*(uint *)_Source;
    _Source = (char *)((int)_Source + 1);
    *(char *)puVar5 = cVar3;
    puVar5 = (uint *)((int)puVar5 + 1);
    if (cVar3 == '\0') {
      while (_Count = _Count - 1, _Count != 0) {
LAB_0041731b:
        *(char *)puVar5 = cVar3;
        puVar5 = (uint *)((int)puVar5 + 1);
      }
      return _Dest;
    }
    _Count = _Count - 1;
  } while (_Count != 0);
  return _Dest;
}



uint FUN_0041741f(void)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  int *piVar3;
  uint uVar4;
  int local_8;
  int local_4;
  
  uVar4 = 0xffffffff;
  FUN_004130b6(0x12);
  local_8 = 0;
  local_4 = 0;
  piVar3 = &DAT_00423de0;
  while (puVar2 = (undefined4 *)*piVar3, puVar1 = puVar2, puVar2 != (undefined4 *)0x0) {
    for (; puVar2 < puVar1 + 0x120; puVar2 = puVar2 + 9) {
      if ((*(byte *)(puVar2 + 1) & 1) == 0) {
        if (puVar2[2] == 0) {
          FUN_004130b6(0x11);
          if (puVar2[2] == 0) {
            InitializeCriticalSection((LPCRITICAL_SECTION)(puVar2 + 3));
            puVar2[2] = puVar2[2] + 1;
          }
          FUN_00413117(0x11);
        }
        EnterCriticalSection((LPCRITICAL_SECTION)(puVar2 + 3));
        if ((*(byte *)(puVar2 + 1) & 1) == 0) {
          *puVar2 = 0xffffffff;
          uVar4 = ((int)puVar2 - *piVar3) / 0x24 + local_4;
          if (uVar4 != 0xffffffff) goto LAB_00417531;
          break;
        }
        LeaveCriticalSection((LPCRITICAL_SECTION)(puVar2 + 3));
      }
      puVar1 = (undefined4 *)*piVar3;
    }
    local_4 = local_4 + 0x20;
    piVar3 = piVar3 + 1;
    local_8 = local_8 + 1;
    if (0x423edf < (int)piVar3) goto LAB_00417531;
  }
  puVar2 = _malloc(0x480);
  if (puVar2 != (undefined4 *)0x0) {
    DAT_00423ee0 = DAT_00423ee0 + 0x20;
    (&DAT_00423de0)[local_8] = puVar2;
    puVar1 = puVar2;
    for (; puVar2 < puVar1 + 0x120; puVar2 = puVar2 + 9) {
      *(undefined1 *)(puVar2 + 1) = 0;
      *puVar2 = 0xffffffff;
      puVar2[2] = 0;
      *(undefined1 *)((int)puVar2 + 5) = 10;
      puVar1 = (undefined4 *)(&DAT_00423de0)[local_8];
    }
    uVar4 = local_8 << 5;
    FUN_0041767f(uVar4);
  }
LAB_00417531:
  FUN_00413117(0x12);
  return uVar4;
}



undefined4 __cdecl FUN_00417542(uint param_1,HANDLE param_2)

{
  DWORD *pDVar1;
  int iVar2;
  DWORD nStdHandle;
  
  if (param_1 < DAT_00423ee0) {
    iVar2 = (param_1 & 0x1f) * 0x24;
    if (*(int *)((&DAT_00423de0)[(int)param_1 >> 5] + iVar2) == -1) {
      if (DAT_00420924 == 1) {
        if (param_1 == 0) {
          nStdHandle = 0xfffffff6;
        }
        else if (param_1 == 1) {
          nStdHandle = 0xfffffff5;
        }
        else {
          if (param_1 != 2) goto LAB_0041759b;
          nStdHandle = 0xfffffff4;
        }
        SetStdHandle(nStdHandle,param_2);
      }
LAB_0041759b:
      *(HANDLE *)((&DAT_00423de0)[(int)param_1 >> 5] + iVar2) = param_2;
      return 0;
    }
  }
  pDVar1 = FUN_00410d43();
  *pDVar1 = 9;
  pDVar1 = FUN_00410d4c();
  *pDVar1 = 0;
  return 0xffffffff;
}



undefined4 __cdecl FUN_004175be(uint param_1)

{
  int *piVar1;
  DWORD *pDVar2;
  int iVar3;
  DWORD nStdHandle;
  
  if (param_1 < DAT_00423ee0) {
    iVar3 = (param_1 & 0x1f) * 0x24;
    piVar1 = (int *)((&DAT_00423de0)[(int)param_1 >> 5] + iVar3);
    if (((*(byte *)(piVar1 + 1) & 1) != 0) && (*piVar1 != -1)) {
      if (DAT_00420924 == 1) {
        if (param_1 == 0) {
          nStdHandle = 0xfffffff6;
        }
        else if (param_1 == 1) {
          nStdHandle = 0xfffffff5;
        }
        else {
          if (param_1 != 2) goto LAB_0041761a;
          nStdHandle = 0xfffffff4;
        }
        SetStdHandle(nStdHandle,(HANDLE)0x0);
      }
LAB_0041761a:
      *(undefined4 *)((&DAT_00423de0)[(int)param_1 >> 5] + iVar3) = 0xffffffff;
      return 0;
    }
  }
  pDVar2 = FUN_00410d43();
  *pDVar2 = 9;
  pDVar2 = FUN_00410d4c();
  *pDVar2 = 0;
  return 0xffffffff;
}



undefined4 __cdecl FUN_0041763d(uint param_1)

{
  DWORD *pDVar1;
  
  if ((param_1 < DAT_00423ee0) &&
     ((*(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + (param_1 & 0x1f) * 0x24) & 1) != 0)) {
    return *(undefined4 *)((&DAT_00423de0)[(int)param_1 >> 5] + (param_1 & 0x1f) * 0x24);
  }
  pDVar1 = FUN_00410d43();
  *pDVar1 = 9;
  pDVar1 = FUN_00410d4c();
  *pDVar1 = 0;
  return 0xffffffff;
}



void __cdecl FUN_0041767f(uint param_1)

{
  int iVar1;
  int iVar2;
  
  iVar2 = (param_1 & 0x1f) * 0x24;
  iVar1 = (&DAT_00423de0)[(int)param_1 >> 5] + iVar2;
  if (*(int *)(iVar1 + 8) == 0) {
    FUN_004130b6(0x11);
    if (*(int *)(iVar1 + 8) == 0) {
      InitializeCriticalSection((LPCRITICAL_SECTION)(iVar1 + 0xc));
      *(int *)(iVar1 + 8) = *(int *)(iVar1 + 8) + 1;
    }
    FUN_00413117(0x11);
  }
  EnterCriticalSection((LPCRITICAL_SECTION)((&DAT_00423de0)[(int)param_1 >> 5] + 0xc + iVar2));
  return;
}



void __cdecl FUN_004176de(uint param_1)

{
  LeaveCriticalSection
            ((LPCRITICAL_SECTION)
             ((&DAT_00423de0)[(int)param_1 >> 5] + 0xc + (param_1 & 0x1f) * 0x24));
  return;
}



uint __cdecl FUN_00417700(LPCSTR param_1,uint param_2,uint param_3,uint param_4)

{
  byte *pbVar1;
  uint uVar2;
  uint uVar3;
  DWORD *pDVar4;
  HANDLE hFile;
  DWORD DVar5;
  int iVar6;
  int iVar7;
  bool bVar8;
  _SECURITY_ATTRIBUTES local_20;
  DWORD local_14;
  DWORD local_10;
  DWORD local_c;
  byte local_5;
  
  bVar8 = (param_2 & 0x80) == 0;
  local_20.nLength = 0xc;
  local_20.lpSecurityDescriptor = (LPVOID)0x0;
  if (bVar8) {
    local_5 = 0;
  }
  else {
    local_5 = 0x10;
  }
  local_20.bInheritHandle = (BOOL)bVar8;
  if (((param_2 & 0x8000) == 0) && (((param_2 & 0x4000) != 0 || (DAT_00423ca8 != 0x8000)))) {
    local_5 = local_5 | 0x80;
  }
  uVar2 = param_2 & 3;
  if (uVar2 == 0) {
    local_10 = 0x80000000;
  }
  else if (uVar2 == 1) {
    local_10 = 0x40000000;
  }
  else {
    if (uVar2 != 2) goto LAB_00417804;
    local_10 = 0xc0000000;
  }
  if (param_3 == 0x10) {
    local_14 = 0;
  }
  else if (param_3 == 0x20) {
    local_14 = 1;
  }
  else if (param_3 == 0x30) {
    local_14 = 2;
  }
  else {
    if (param_3 != 0x40) goto LAB_00417804;
    local_14 = 3;
  }
  uVar2 = param_2 & 0x700;
  if (uVar2 < 0x401) {
    if ((uVar2 == 0x400) || (uVar2 == 0)) {
      local_c = 3;
    }
    else if (uVar2 == 0x100) {
      local_c = 4;
    }
    else {
      if (uVar2 == 0x200) goto LAB_0041781e;
      if (uVar2 != 0x300) goto LAB_00417804;
      local_c = 2;
    }
  }
  else {
    if (uVar2 != 0x500) {
      if (uVar2 == 0x600) {
LAB_0041781e:
        local_c = 5;
        goto LAB_0041782e;
      }
      if (uVar2 != 0x700) {
LAB_00417804:
        pDVar4 = FUN_00410d43();
        *pDVar4 = 0x16;
        pDVar4 = FUN_00410d4c();
        *pDVar4 = 0;
        return 0xffffffff;
      }
    }
    local_c = 1;
  }
LAB_0041782e:
  uVar2 = 0x80;
  if (((param_2 & 0x100) != 0) && ((~DAT_004239dc & param_4 & 0x80) == 0)) {
    uVar2 = 1;
  }
  if ((param_2 & 0x40) != 0) {
    uVar2 = uVar2 | 0x4000000;
    local_10 = CONCAT13(local_10._3_1_,0x10000);
  }
  if ((param_2 & 0x1000) != 0) {
    uVar2 = uVar2 | 0x100;
  }
  if ((param_2 & 0x20) == 0) {
    if ((param_2 & 0x10) != 0) {
      uVar2 = uVar2 | 0x10000000;
    }
  }
  else {
    uVar2 = uVar2 | 0x8000000;
  }
  uVar3 = FUN_0041741f();
  if (uVar3 == 0xffffffff) {
    pDVar4 = FUN_00410d43();
    *pDVar4 = 0x18;
    pDVar4 = FUN_00410d4c();
    *pDVar4 = 0;
    return 0xffffffff;
  }
  hFile = CreateFileA(param_1,local_10,local_14,&local_20,local_c,uVar2,(HANDLE)0x0);
  if (hFile != (HANDLE)0xffffffff) {
    DVar5 = GetFileType(hFile);
    if (DVar5 != 0) {
      if (DVar5 == 2) {
        local_5 = local_5 | 0x40;
      }
      else if (DVar5 == 3) {
        local_5 = local_5 | 8;
      }
      FUN_00417542(uVar3,hFile);
      iVar7 = (uVar3 & 0x1f) * 0x24;
      param_1._3_1_ = local_5 & 0x48;
      *(byte *)((&DAT_00423de0)[(int)uVar3 >> 5] + 4 + iVar7) = local_5 | 1;
      if ((((local_5 & 0x48) == 0) && ((local_5 & 0x80) != 0)) && ((param_2 & 2) != 0)) {
        local_14 = FUN_0041506a(uVar3,-1,2);
        if (local_14 == 0xffffffff) {
          pDVar4 = FUN_00410d4c();
          if (*pDVar4 == 0x83) goto LAB_004179a8;
        }
        else {
          param_3 = param_3 & 0xffffff;
          iVar6 = FUN_00416504(uVar3,(char *)((int)&param_3 + 3),(char *)0x1);
          if ((((iVar6 != 0) || (param_3._3_1_ != '\x1a')) ||
              (iVar6 = FUN_0041888a(uVar3,local_14), iVar6 != -1)) &&
             (DVar5 = FUN_0041506a(uVar3,0,0), DVar5 != 0xffffffff)) goto LAB_004179a8;
        }
        FUN_00414128(uVar3);
        uVar2 = 0xffffffff;
      }
      else {
LAB_004179a8:
        uVar2 = uVar3;
        if ((param_1._3_1_ == 0) && ((param_2 & 8) != 0)) {
          pbVar1 = (byte *)((&DAT_00423de0)[(int)uVar3 >> 5] + 4 + iVar7);
          *pbVar1 = *pbVar1 | 0x20;
        }
      }
      goto LAB_004179c1;
    }
    CloseHandle(hFile);
  }
  DVar5 = GetLastError();
  FUN_00410cd0(DVar5);
  uVar2 = 0xffffffff;
LAB_004179c1:
  FUN_004176de(uVar3);
  return uVar2;
}



void __cdecl FUN_004179cf(byte param_1)

{
  FUN_004179e0(param_1,0,4);
  return;
}



undefined4 __cdecl FUN_004179e0(byte param_1,uint param_2,byte param_3)

{
  uint uVar1;
  
  if ((*(byte *)((int)&DAT_00425020 + param_1 + 1) & param_3) == 0) {
    if (param_2 == 0) {
      uVar1 = 0;
    }
    else {
      uVar1 = *(ushort *)(&DAT_00420d1a + (uint)param_1 * 2) & param_2;
    }
    if (uVar1 == 0) {
      return 0;
    }
  }
  return 1;
}



int __cdecl FUN_00417a11(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  HMODULE hModule;
  int iVar1;
  
  iVar1 = 0;
  if (DAT_00423c24 == (FARPROC)0x0) {
    hModule = LoadLibraryA("user32.dll");
    if (hModule != (HMODULE)0x0) {
      DAT_00423c24 = GetProcAddress(hModule,"MessageBoxA");
      if (DAT_00423c24 != (FARPROC)0x0) {
        DAT_00423c28 = GetProcAddress(hModule,"GetActiveWindow");
        DAT_00423c2c = GetProcAddress(hModule,"GetLastActivePopup");
        goto LAB_00417a60;
      }
    }
    iVar1 = 0;
  }
  else {
LAB_00417a60:
    if (DAT_00423c28 != (FARPROC)0x0) {
      iVar1 = (*DAT_00423c28)();
      if ((iVar1 != 0) && (DAT_00423c2c != (FARPROC)0x0)) {
        iVar1 = (*DAT_00423c2c)(iVar1);
      }
    }
    iVar1 = (*DAT_00423c24)(iVar1,param_1,param_2,param_3);
  }
  return iVar1;
}



undefined4 __cdecl FUN_00417a9a(int param_1,int param_2)

{
  int *piVar1;
  int iVar2;
  
  if ((*(uint *)(param_1 + (param_2 / 0x20) * 4) & ~(-1 << (0x1fU - (char)(param_2 % 0x20) & 0x1f)))
      != 0) {
    return 0;
  }
  iVar2 = param_2 / 0x20 + 1;
  if (iVar2 < 3) {
    piVar1 = (int *)(param_1 + iVar2 * 4);
    do {
      if (*piVar1 != 0) {
        return 0;
      }
      iVar2 = iVar2 + 1;
      piVar1 = piVar1 + 1;
    } while (iVar2 < 3);
  }
  return 1;
}



void __cdecl FUN_00417ae3(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  uint *puVar3;
  
  puVar3 = (uint *)(param_1 + (param_2 / 0x20) * 4);
  iVar1 = FUN_004189af(*puVar3,1 << (0x1fU - (char)(param_2 % 0x20) & 0x1f),puVar3);
  iVar2 = param_2 / 0x20 + -1;
  if (-1 < iVar2) {
    puVar3 = (uint *)(param_1 + iVar2 * 4);
    do {
      if (iVar1 == 0) {
        return;
      }
      iVar1 = FUN_004189af(*puVar3,1,puVar3);
      iVar2 = iVar2 + -1;
      puVar3 = puVar3 + -1;
    } while (-1 < iVar2);
  }
  return;
}



undefined4 __cdecl FUN_00417b39(int param_1,int param_2)

{
  uint *puVar1;
  int iVar2;
  byte bVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 local_8;
  
  local_8 = 0;
  puVar1 = (uint *)(param_1 + (param_2 / 0x20) * 4);
  bVar3 = 0x1f - (char)(param_2 % 0x20);
  if (((*puVar1 & 1 << (bVar3 & 0x1f)) != 0) &&
     (iVar2 = FUN_00417a9a(param_1,param_2 + 1), iVar2 == 0)) {
    local_8 = FUN_00417ae3(param_1,param_2 + -1);
  }
  *puVar1 = *puVar1 & -1 << (bVar3 & 0x1f);
  iVar2 = param_2 / 0x20 + 1;
  if (iVar2 < 3) {
    puVar5 = (undefined4 *)(param_1 + iVar2 * 4);
    for (iVar4 = 3 - iVar2; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
  }
  return local_8;
}



void __cdecl FUN_00417bc5(int param_1,undefined4 *param_2)

{
  int iVar1;
  int iVar2;
  
  iVar1 = param_1 - (int)param_2;
  iVar2 = 3;
  do {
    *(undefined4 *)(iVar1 + (int)param_2) = *param_2;
    param_2 = param_2 + 1;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}



void __cdecl FUN_00417be0(undefined4 *param_1)

{
  *param_1 = 0;
  param_1[1] = 0;
  param_1[2] = 0;
  return;
}



undefined4 __cdecl FUN_00417bec(int *param_1)

{
  int iVar1;
  
  iVar1 = 0;
  do {
    if (*param_1 != 0) {
      return 0;
    }
    iVar1 = iVar1 + 1;
    param_1 = param_1 + 1;
  } while (iVar1 < 3);
  return 1;
}



void __cdecl FUN_00417c07(uint *param_1,uint param_2)

{
  uint uVar1;
  int iVar2;
  byte bVar3;
  int iVar4;
  int iVar5;
  uint *puVar6;
  int local_8;
  
  local_8 = 3;
  iVar2 = (int)param_2 / 0x20;
  iVar5 = (int)param_2 % 0x20;
  param_2 = 0;
  bVar3 = (byte)iVar5;
  puVar6 = param_1;
  do {
    uVar1 = *puVar6;
    *puVar6 = uVar1 >> (bVar3 & 0x1f) | param_2;
    puVar6 = puVar6 + 1;
    param_2 = (uVar1 & ~(-1 << (bVar3 & 0x1f))) << (0x20 - bVar3 & 0x1f);
    local_8 = local_8 + -1;
  } while (local_8 != 0);
  iVar5 = 2;
  iVar4 = 8;
  do {
    if (iVar5 < iVar2) {
      *(undefined4 *)(iVar4 + (int)param_1) = 0;
    }
    else {
      *(undefined4 *)(iVar4 + (int)param_1) = *(undefined4 *)(iVar4 + iVar2 * -4 + (int)param_1);
    }
    iVar5 = iVar5 + -1;
    iVar4 = iVar4 + -4;
  } while (-1 < iVar4);
  return;
}



undefined4 __cdecl FUN_00417c94(ushort *param_1,uint *param_2,int *param_3)

{
  ushort uVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  undefined4 uVar5;
  undefined4 local_1c [3];
  uint local_10;
  uint local_c;
  int local_8;
  
  uVar1 = param_1[5];
  local_10 = *(uint *)(param_1 + 3);
  local_c = *(uint *)(param_1 + 1);
  uVar3 = uVar1 & 0x7fff;
  iVar4 = uVar3 - 0x3fff;
  local_8 = (uint)*param_1 << 0x10;
  if (iVar4 == -0x3fff) {
    iVar4 = 0;
    iVar2 = FUN_00417bec((int *)&local_10);
    if (iVar2 != 0) {
LAB_00417dc0:
      uVar5 = 0;
      goto LAB_00417dc2;
    }
    FUN_00417be0(&local_10);
  }
  else {
    FUN_00417bc5((int)local_1c,&local_10);
    iVar2 = FUN_00417b39((int)&local_10,param_3[2]);
    if (iVar2 != 0) {
      iVar4 = uVar3 - 0x3ffe;
    }
    iVar2 = param_3[1];
    if (iVar4 < iVar2 - param_3[2]) {
      FUN_00417be0(&local_10);
    }
    else {
      if (iVar2 < iVar4) {
        if (*param_3 <= iVar4) {
          FUN_00417be0(&local_10);
          local_10 = local_10 | 0x80000000;
          FUN_00417c07(&local_10,param_3[3]);
          iVar4 = param_3[5] + *param_3;
          uVar5 = 1;
          goto LAB_00417dc2;
        }
        local_10 = local_10 & 0x7fffffff;
        iVar4 = param_3[5] + iVar4;
        FUN_00417c07(&local_10,param_3[3]);
        goto LAB_00417dc0;
      }
      FUN_00417bc5((int)&local_10,local_1c);
      FUN_00417c07(&local_10,iVar2 - iVar4);
      FUN_00417b39((int)&local_10,param_3[2]);
      FUN_00417c07(&local_10,param_3[3] + 1);
    }
  }
  iVar4 = 0;
  uVar5 = 2;
LAB_00417dc2:
  local_10 = iVar4 << (0x1fU - (char)param_3[3] & 0x1f) |
             -(uint)((uVar1 & 0x8000) != 0) & 0x80000000 | local_10;
  if (param_3[4] == 0x40) {
    param_2[1] = local_10;
    *param_2 = local_c;
  }
  else if (param_3[4] == 0x20) {
    *param_2 = local_10;
  }
  return uVar5;
}



void __cdecl FUN_00417e00(ushort *param_1,uint *param_2)

{
  FUN_00417c94(param_1,param_2,(int *)&DAT_00423480);
  return;
}



void __cdecl FUN_00417e16(ushort *param_1,uint *param_2)

{
  FUN_00417c94(param_1,param_2,(int *)&DAT_00423498);
  return;
}



void __thiscall FUN_00417e2c(void *this,uint *param_1,byte *param_2)

{
  ushort local_10 [6];
  
  FUN_00417e86(this,local_10,(int *)&param_2,param_2,0,0,0,0);
  FUN_00417e00(local_10,param_1);
  return;
}



void __thiscall FUN_00417e59(void *this,uint *param_1,byte *param_2)

{
  ushort local_10 [6];
  
  FUN_00417e86(this,local_10,(int *)&param_2,param_2,0,0,0,0);
  FUN_00417e16(local_10,param_1);
  return;
}



undefined4 __thiscall
FUN_00417e86(void *this,ushort *param_1,int *param_2,byte *param_3,int param_4,int param_5,
            int param_6,int param_7)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  char *pcVar4;
  int iVar5;
  byte bVar6;
  byte *pbVar7;
  byte *pbVar8;
  int iVar9;
  byte *pbVar10;
  char local_60 [23];
  char local_49;
  ushort local_44;
  undefined2 uStack_42;
  undefined2 uStack_40;
  byte *local_3e;
  ushort local_3a;
  int local_34;
  int local_30;
  undefined4 local_2c;
  int local_28;
  int local_24;
  byte *local_20;
  int local_1c;
  undefined4 local_18;
  int local_14;
  char *local_10;
  int local_c;
  uint local_8;
  
  local_10 = local_60;
  local_2c = 0;
  local_1c = 1;
  local_8 = 0;
  local_14 = 0;
  local_28 = 0;
  local_24 = 0;
  local_30 = 0;
  local_34 = 0;
  local_20 = (byte *)0x0;
  local_c = 0;
  local_18 = 0;
  pbVar8 = param_3;
  while( true ) {
    bVar6 = *pbVar8;
    this = (void *)CONCAT31((int3)((uint)this >> 8),bVar6);
    if ((((bVar6 != 0x20) && (bVar6 != 9)) && (bVar6 != 10)) && (bVar6 != 0xd)) break;
    pbVar8 = pbVar8 + 1;
  }
  iVar1 = 4;
  iVar9 = 0;
  iVar5 = local_14;
LAB_00417edd:
  local_14 = iVar5;
  pbVar7 = pbVar8;
  iVar5 = 1;
  bVar6 = *pbVar7;
  pbVar8 = pbVar7 + 1;
  iVar2 = local_14;
  switch(iVar9) {
  case 0:
    if (('0' < (char)bVar6) && ((char)bVar6 < ':')) {
LAB_00417efa:
      local_14 = iVar2;
      iVar9 = 3;
      goto LAB_0041811f;
    }
    if (bVar6 == DAT_00420f20) goto LAB_00417f09;
    if (bVar6 == 0x2b) {
      local_2c = 0;
      iVar9 = 2;
      iVar5 = local_14;
    }
    else if (bVar6 == 0x2d) {
      local_2c = 0x8000;
      iVar9 = 2;
      iVar5 = local_14;
    }
    else {
      iVar9 = iVar5;
      iVar5 = local_14;
      if (bVar6 != 0x30) goto LAB_004181f9;
    }
    goto LAB_00417edd;
  case 1:
    local_14 = 1;
    if (('0' < (char)bVar6) && (iVar2 = iVar5, (char)bVar6 < ':')) goto LAB_00417efa;
    iVar9 = iVar1;
    if (bVar6 != DAT_00420f20) {
      iVar9 = iVar5;
      if ((bVar6 == 0x2b) || (iVar9 = local_14, bVar6 == 0x2d)) goto LAB_00417f8e;
      iVar9 = iVar5;
      local_14 = iVar5;
      if (bVar6 != 0x30) goto LAB_00417f67;
    }
    goto LAB_00417edd;
  case 2:
    if (('0' < (char)bVar6) && ((char)bVar6 < ':')) goto LAB_00417efa;
    if (bVar6 == DAT_00420f20) {
LAB_00417f09:
      iVar9 = 5;
      iVar5 = local_14;
    }
    else {
      iVar9 = iVar5;
      pbVar7 = param_3;
      iVar5 = local_14;
      if (bVar6 != 0x30) goto LAB_004181fe;
    }
    goto LAB_00417edd;
  case 3:
    local_14 = iVar5;
    while( true ) {
      if (DAT_00420f1c < 2) {
        uVar3 = (byte)PTR_DAT_00420d10[(uint)bVar6 * 2] & 4;
        this = PTR_DAT_00420d10;
      }
      else {
        pbVar7 = (byte *)0x4;
        uVar3 = FUN_00413dd7(this,(uint)bVar6,4);
        this = pbVar7;
      }
      if (uVar3 == 0) break;
      if (local_8 < 0x19) {
        local_8 = local_8 + 1;
        pcVar4 = local_10 + 1;
        *local_10 = bVar6 - 0x30;
        local_10 = pcVar4;
      }
      else {
        local_c = local_c + 1;
      }
      bVar6 = *pbVar8;
      pbVar8 = pbVar8 + 1;
    }
    iVar9 = iVar1;
    iVar5 = local_14;
    if (bVar6 != DAT_00420f20) goto LAB_0041807b;
    goto LAB_00417edd;
  case 4:
    local_14 = 1;
    local_28 = 1;
    iVar9 = iVar5;
    if (local_8 == 0) {
      while (iVar5 = local_28, iVar9 = local_14, bVar6 == 0x30) {
        local_c = local_c + -1;
        bVar6 = *pbVar8;
        pbVar8 = pbVar8 + 1;
      }
    }
    while( true ) {
      local_14 = iVar9;
      local_28 = iVar5;
      if (DAT_00420f1c < 2) {
        uVar3 = (byte)PTR_DAT_00420d10[(uint)bVar6 * 2] & 4;
        this = PTR_DAT_00420d10;
      }
      else {
        pbVar7 = (byte *)0x4;
        uVar3 = FUN_00413dd7(this,(uint)bVar6,4);
        this = pbVar7;
      }
      if (uVar3 == 0) break;
      if (local_8 < 0x19) {
        local_8 = local_8 + 1;
        local_c = local_c + -1;
        pcVar4 = local_10 + 1;
        *local_10 = bVar6 - 0x30;
        local_10 = pcVar4;
      }
      bVar6 = *pbVar8;
      pbVar8 = pbVar8 + 1;
      iVar5 = local_28;
      iVar9 = local_14;
    }
LAB_0041807b:
    iVar9 = local_14;
    if ((bVar6 == 0x2b) || (bVar6 == 0x2d)) {
LAB_00417f8e:
      local_14 = iVar9;
      iVar9 = 0xb;
      pbVar8 = pbVar8 + -1;
      iVar5 = local_14;
    }
    else {
LAB_00417f67:
      if (((char)bVar6 < 'D') ||
         (('E' < (char)bVar6 && (((char)bVar6 < 'd' || ('e' < (char)bVar6)))))) goto LAB_004181f9;
      iVar9 = 6;
      iVar5 = local_14;
    }
    goto LAB_00417edd;
  case 5:
    local_28 = iVar5;
    if (DAT_00420f1c < 2) {
      uVar3 = (byte)PTR_DAT_00420d10[(uint)bVar6 * 2] & 4;
      this = PTR_DAT_00420d10;
    }
    else {
      pbVar7 = (byte *)0x4;
      uVar3 = FUN_00413dd7(this,(uint)bVar6,4);
      this = pbVar7;
    }
    iVar9 = iVar1;
    pbVar7 = param_3;
    if (uVar3 != 0) goto LAB_0041811f;
    goto LAB_004181fe;
  case 6:
    pbVar7 = pbVar7 + -1;
    this = pbVar7;
    param_3 = pbVar7;
    if (((char)bVar6 < '1') || ('9' < (char)bVar6)) {
      if (bVar6 == 0x2b) goto LAB_00418154;
      if (bVar6 == 0x2d) goto LAB_00418148;
      if (bVar6 != 0x30) goto LAB_004181fe;
LAB_004180ed:
      iVar9 = 8;
      iVar5 = local_14;
      goto LAB_00417edd;
    }
    break;
  case 7:
    if (((char)bVar6 < '1') || ('9' < (char)bVar6)) {
      pbVar7 = param_3;
      if (bVar6 == 0x30) goto LAB_004180ed;
      goto LAB_004181fe;
    }
    break;
  case 8:
    local_24 = 1;
    while (bVar6 == 0x30) {
      bVar6 = *pbVar8;
      pbVar8 = pbVar8 + 1;
    }
    if (((char)bVar6 < '1') || ('9' < (char)bVar6)) goto LAB_004181f9;
    break;
  case 9:
    local_24 = 1;
    pbVar7 = (byte *)0x0;
    goto LAB_0041817f;
  default:
    goto switchD_00417ee9_caseD_a;
  case 0xb:
    if (param_7 != 0) {
      if (bVar6 == 0x2b) {
LAB_00418154:
        iVar9 = 7;
        this = pbVar7;
        param_3 = pbVar7;
        iVar5 = local_14;
      }
      else {
        param_3 = pbVar7;
        if (bVar6 != 0x2d) goto LAB_004181fe;
LAB_00418148:
        local_1c = -1;
        iVar9 = 7;
        this = pbVar7;
        param_3 = pbVar7;
        iVar5 = local_14;
      }
      goto LAB_00417edd;
    }
    iVar9 = 10;
    pbVar8 = pbVar7;
switchD_00417ee9_caseD_a:
    pbVar7 = pbVar8;
    iVar5 = local_14;
    if (iVar9 != 10) goto LAB_00417edd;
    goto LAB_004181fe;
  }
  iVar9 = 9;
LAB_0041811f:
  pbVar8 = pbVar8 + -1;
  iVar5 = local_14;
  goto LAB_00417edd;
LAB_0041817f:
  if (DAT_00420f1c < 2) {
    uVar3 = (byte)PTR_DAT_00420d10[(uint)bVar6 * 2] & 4;
    this = PTR_DAT_00420d10;
  }
  else {
    pbVar10 = (byte *)0x4;
    uVar3 = FUN_00413dd7(this,(uint)bVar6,4);
    this = pbVar10;
  }
  if (uVar3 == 0) goto LAB_004181c9;
  this = (void *)(int)(char)bVar6;
  pbVar7 = (byte *)((int)this + (int)pbVar7 * 10 + -0x30);
  if (0x1450 < (int)pbVar7) goto LAB_004181c1;
  bVar6 = *pbVar8;
  pbVar8 = pbVar8 + 1;
  goto LAB_0041817f;
LAB_004181c1:
  pbVar7 = (byte *)0x1451;
LAB_004181c9:
  while( true ) {
    local_20 = pbVar7;
    if (DAT_00420f1c < 2) {
      uVar3 = (byte)PTR_DAT_00420d10[(uint)bVar6 * 2] & 4;
      this = PTR_DAT_00420d10;
    }
    else {
      pbVar7 = (byte *)0x4;
      uVar3 = FUN_00413dd7(this,(uint)bVar6,4);
      this = pbVar7;
    }
    if (uVar3 == 0) break;
    bVar6 = *pbVar8;
    pbVar8 = pbVar8 + 1;
    pbVar7 = local_20;
  }
LAB_004181f9:
  pbVar7 = pbVar8 + -1;
LAB_004181fe:
  *param_2 = (int)pbVar7;
  if (local_14 == 0) {
    local_44 = 0;
    local_3a = 0;
    local_3e = (byte *)0x0;
    param_3 = (byte *)0x0;
    local_18 = 4;
    goto LAB_0041830c;
  }
  pcVar4 = local_10;
  if (0x18 < local_8) {
    if ('\x04' < local_49) {
      local_49 = local_49 + '\x01';
    }
    local_8 = 0x18;
    local_c = local_c + 1;
    pcVar4 = local_10 + -1;
  }
  if (local_8 == 0) {
    local_44 = 0;
    local_3a = 0;
    local_3e = (byte *)0x0;
    param_3 = (byte *)0x0;
  }
  else {
    while (pcVar4 = pcVar4 + -1, *pcVar4 == '\0') {
      local_8 = local_8 - 1;
      local_c = local_c + 1;
    }
    FUN_00418a89(local_60,local_8,(uint *)&local_44);
    pbVar8 = local_20;
    if (local_1c < 0) {
      pbVar8 = (byte *)-(int)local_20;
    }
    pbVar8 = pbVar8 + local_c;
    if (local_24 == 0) {
      pbVar8 = pbVar8 + param_5;
    }
    if (local_28 == 0) {
      pbVar8 = pbVar8 + -param_6;
    }
    if ((int)pbVar8 < 0x1451) {
      if (-0x1451 < (int)pbVar8) {
        FUN_00418d70((int *)&local_44,(uint)pbVar8,param_4);
        param_3 = (byte *)CONCAT22(uStack_40,uStack_42);
        goto LAB_00418291;
      }
      local_34 = 1;
    }
    else {
      local_30 = 1;
    }
    local_3a = (ushort)param_3;
    local_3e = param_3;
    local_44 = local_3a;
  }
LAB_00418291:
  if (local_30 == 0) {
    if (local_34 != 0) {
      local_44 = 0;
      local_3a = 0;
      local_3e = (byte *)0x0;
      param_3 = (byte *)0x0;
      local_18 = 1;
    }
  }
  else {
    param_3 = (byte *)0x0;
    local_3a = 0x7fff;
    local_3e = (byte *)0x80000000;
    local_44 = 0;
    local_18 = 2;
  }
LAB_0041830c:
  *(byte **)(param_1 + 3) = local_3e;
  *(byte **)(param_1 + 1) = param_3;
  param_1[5] = local_3a | (ushort)local_2c;
  *param_1 = local_44;
  return local_18;
}



uint __thiscall FUN_00418357(void *this,uint param_1,uint param_2)

{
  uint uVar1;
  undefined2 in_FPUControlWord;
  undefined4 local_8;
  
  local_8 = CONCAT22((short)((uint)this >> 0x10),in_FPUControlWord);
  uVar1 = FUN_004183a2(local_8);
  uVar1 = uVar1 & ~param_2 | param_1 & param_2;
  FUN_00418434(uVar1);
  return uVar1;
}



void __thiscall FUN_0041838c(void *this,uint param_1,uint param_2)

{
  FUN_00418357(this,param_1,param_2 & 0xfff7ffff);
  return;
}



uint __cdecl FUN_004183a2(uint param_1)

{
  uint uVar1;
  uint uVar2;
  
  uVar1 = 0;
  if ((param_1 & 1) != 0) {
    uVar1 = 0x10;
  }
  if ((param_1 & 4) != 0) {
    uVar1 = uVar1 | 8;
  }
  if ((param_1 & 8) != 0) {
    uVar1 = uVar1 | 4;
  }
  if ((param_1 & 0x10) != 0) {
    uVar1 = uVar1 | 2;
  }
  if ((param_1 & 0x20) != 0) {
    uVar1 = uVar1 | 1;
  }
  if ((param_1 & 2) != 0) {
    uVar1 = uVar1 | 0x80000;
  }
  uVar2 = param_1 & 0xc00;
  if (uVar2 != 0) {
    if (uVar2 == 0x400) {
      uVar1 = uVar1 | 0x100;
    }
    else if (uVar2 == 0x800) {
      uVar1 = uVar1 | 0x200;
    }
    else if (uVar2 == 0xc00) {
      uVar1 = uVar1 | 0x300;
    }
  }
  if ((param_1 & 0x300) == 0) {
    uVar1 = uVar1 | 0x20000;
  }
  else if ((param_1 & 0x300) == 0x200) {
    uVar1 = uVar1 | 0x10000;
  }
  if ((param_1 & 0x1000) != 0) {
    uVar1 = uVar1 | 0x40000;
  }
  return uVar1;
}



uint __cdecl FUN_00418434(uint param_1)

{
  uint uVar1;
  uint uVar2;
  
  uVar1 = (uint)((param_1 & 0x10) != 0);
  if ((param_1 & 8) != 0) {
    uVar1 = uVar1 | 4;
  }
  if ((param_1 & 4) != 0) {
    uVar1 = uVar1 | 8;
  }
  if ((param_1 & 2) != 0) {
    uVar1 = uVar1 | 0x10;
  }
  if ((param_1 & 1) != 0) {
    uVar1 = uVar1 | 0x20;
  }
  if ((param_1 & 0x80000) != 0) {
    uVar1 = uVar1 | 2;
  }
  uVar2 = param_1 & 0x300;
  if (uVar2 != 0) {
    if (uVar2 == 0x100) {
      uVar1 = uVar1 | 0x400;
    }
    else if (uVar2 == 0x200) {
      uVar1 = uVar1 | 0x800;
    }
    else if (uVar2 == 0x300) {
      uVar1 = uVar1 | 0xc00;
    }
  }
  if ((param_1 & 0x30000) == 0) {
    uVar1 = uVar1 | 0x300;
  }
  else if ((param_1 & 0x30000) == 0x10000) {
    uVar1 = uVar1 | 0x200;
  }
  if ((param_1 & 0x40000) != 0) {
    uVar1 = uVar1 | 0x1000;
  }
  return uVar1;
}



void __cdecl FUN_004184bd(char *param_1,int param_2,int param_3)

{
  char *_Str;
  char *pcVar1;
  char *pcVar2;
  size_t sVar3;
  char *pcVar4;
  char cVar5;
  
  pcVar1 = param_1;
  pcVar4 = *(char **)(param_3 + 0xc);
  _Str = param_1 + 1;
  *param_1 = '0';
  pcVar2 = _Str;
  if (0 < param_2) {
    param_1 = (char *)param_2;
    param_2 = 0;
    do {
      cVar5 = *pcVar4;
      if (cVar5 == '\0') {
        cVar5 = '0';
      }
      else {
        pcVar4 = pcVar4 + 1;
      }
      *pcVar2 = cVar5;
      pcVar2 = pcVar2 + 1;
      param_1 = param_1 + -1;
    } while (param_1 != (char *)0x0);
  }
  *pcVar2 = '\0';
  if ((-1 < param_2) && ('4' < *pcVar4)) {
    while (pcVar2 = pcVar2 + -1, *pcVar2 == '9') {
      *pcVar2 = '0';
    }
    *pcVar2 = *pcVar2 + '\x01';
  }
  if (*pcVar1 == '1') {
    *(int *)(param_3 + 4) = *(int *)(param_3 + 4) + 1;
  }
  else {
    sVar3 = _strlen(_Str);
    FUN_0040f710((undefined4 *)pcVar1,(undefined4 *)_Str,sVar3 + 1);
  }
  return;
}



int * __cdecl FUN_00418534(undefined4 param_1,undefined4 param_2,int *param_3,uint *param_4)

{
  int *piVar1;
  uint *puVar2;
  int iVar3;
  undefined4 in_stack_ffffffbc;
  undefined2 uVar4;
  short local_2c;
  char local_2a;
  uint local_28 [6];
  uint local_10;
  uint uStack_c;
  undefined2 uStack_8;
  
  uVar4 = (undefined2)((uint)in_stack_ffffffbc >> 0x10);
  FUN_00418590(&local_10,&param_1);
  iVar3 = FUN_00418dec(local_10,uStack_c,CONCAT22(uVar4,uStack_8),0x11,0,&local_2c);
  puVar2 = param_4;
  piVar1 = param_3;
  param_3[2] = iVar3;
  *param_3 = (int)local_2a;
  param_3[1] = (int)local_2c;
  FUN_00416770(param_4,local_28);
  piVar1[3] = (int)puVar2;
  return piVar1;
}



void __cdecl FUN_00418590(uint *param_1,uint *param_2)

{
  ushort uVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  uint local_8;
  
  uVar1 = *(ushort *)((int)param_2 + 6);
  uVar3 = (uVar1 & 0x7ff0) >> 4;
  uVar2 = *param_2;
  local_8 = 0x80000000;
  if (uVar3 == 0) {
    if (((param_2[1] & 0xfffff) == 0) && (uVar2 == 0)) {
      param_1[1] = 0;
      *param_1 = 0;
      *(undefined2 *)(param_1 + 2) = 0;
      return;
    }
    iVar4 = 0x3c01;
    local_8 = 0;
  }
  else if (uVar3 == 0x7ff) {
    iVar4 = 0x7fff;
  }
  else {
    iVar4 = uVar3 + 0x3c00;
  }
  local_8 = uVar2 >> 0x15 | (param_2[1] & 0xfffff) << 0xb | local_8;
  param_1[1] = local_8;
  *param_1 = uVar2 << 0xb;
  while ((local_8 & 0x80000000) == 0) {
    local_8 = *param_1 >> 0x1f | local_8 * 2;
    *param_1 = *param_1 * 2;
    param_1[1] = local_8;
    iVar4 = iVar4 + 0xffff;
  }
  *(ushort *)(param_1 + 2) = uVar1 & 0x8000 | (ushort)iVar4;
  return;
}



void FUN_00418646(void)

{
  __amsg_exit(2);
  return;
}



undefined4 __cdecl FUN_0041864f(DWORD *param_1)

{
  bool bVar1;
  DWORD *pDVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  code *pcVar6;
  undefined4 *puVar7;
  DWORD local_10;
  DWORD local_c;
  
  bVar1 = false;
  if (param_1 == (DWORD *)0x2) {
    puVar7 = &DAT_00423c30;
    pcVar6 = DAT_00423c30;
LAB_004186d5:
    bVar1 = true;
    FUN_004130b6(1);
    pDVar2 = param_1;
  }
  else {
    if (((param_1 != (DWORD *)0x4) && (param_1 != (DWORD *)0x8)) && (param_1 != (DWORD *)0xb)) {
      if (param_1 == (DWORD *)0xf) {
        puVar7 = &DAT_00423c3c;
        pcVar6 = DAT_00423c3c;
      }
      else if (param_1 == (DWORD *)0x15) {
        puVar7 = &DAT_00423c34;
        pcVar6 = DAT_00423c34;
      }
      else {
        if (param_1 != (DWORD *)0x16) {
          return 0xffffffff;
        }
        puVar7 = &DAT_00423c38;
        pcVar6 = DAT_00423c38;
      }
      goto LAB_004186d5;
    }
    pDVar2 = FUN_00412133();
    uVar3 = FUN_004187d1((int)param_1,pDVar2[0x14]);
    puVar7 = (undefined4 *)(uVar3 + 8);
    pcVar6 = (code *)*puVar7;
  }
  if (pcVar6 == (code *)0x1) {
    if (!bVar1) {
      return 0;
    }
    FUN_00413117(1);
    return 0;
  }
  if (pcVar6 == (code *)0x0) {
    if (bVar1) {
      FUN_00413117(1);
    }
                    // WARNING: Subroutine does not return
    __exit(3);
  }
  if (((param_1 == (DWORD *)0x8) || (param_1 == (DWORD *)0xb)) || (param_1 == (DWORD *)0x4)) {
    local_c = pDVar2[0x15];
    pDVar2[0x15] = 0;
    if (param_1 == (DWORD *)0x8) {
      local_10 = pDVar2[0x16];
      pDVar2[0x16] = 0x8c;
      goto LAB_00418749;
    }
  }
  else {
LAB_00418749:
    if (param_1 == (DWORD *)0x8) {
      if (DAT_00420c08 < DAT_00420c0c + DAT_00420c08) {
        iVar4 = DAT_00420c08 * 0xc;
        iVar5 = DAT_00420c08;
        do {
          iVar4 = iVar4 + 0xc;
          *(undefined4 *)((pDVar2[0x14] - 4) + iVar4) = 0;
          iVar5 = iVar5 + 1;
        } while (iVar5 < DAT_00420c0c + DAT_00420c08);
      }
      goto LAB_00418787;
    }
  }
  *puVar7 = 0;
LAB_00418787:
  if (bVar1) {
    FUN_00413117(1);
  }
  if (param_1 == (DWORD *)0x8) {
    (*pcVar6)(8,pDVar2[0x16]);
  }
  else {
    (*pcVar6)(param_1);
    if ((param_1 != (DWORD *)0xb) && (param_1 != (DWORD *)0x4)) {
      return 0;
    }
  }
  pDVar2[0x15] = local_c;
  if (param_1 == (DWORD *)0x8) {
    pDVar2[0x16] = local_10;
  }
  return 0;
}



uint __cdecl FUN_004187d1(int param_1,uint param_2)

{
  int *piVar1;
  uint uVar2;
  uint uVar3;
  
  uVar2 = param_2;
  if (*(int *)(param_2 + 4) != param_1) {
    uVar3 = param_2;
    do {
      uVar2 = uVar3 + 0xc;
      if (param_2 + DAT_00420c14 * 0xc <= uVar2) break;
      piVar1 = (int *)(uVar3 + 0x10);
      uVar3 = uVar2;
    } while (*piVar1 != param_1);
  }
  if ((param_2 + DAT_00420c14 * 0xc <= uVar2) || (*(int *)(uVar2 + 4) != param_1)) {
    uVar2 = 0;
  }
  return uVar2;
}



int __cdecl FUN_00418810(byte *param_1,byte *param_2)

{
  byte bVar1;
  int iVar2;
  byte abStack_28 [32];
  
  abStack_28[0x1c] = 0;
  abStack_28[0x1d] = 0;
  abStack_28[0x1e] = 0;
  abStack_28[0x1f] = 0;
  abStack_28[0x18] = 0;
  abStack_28[0x19] = 0;
  abStack_28[0x1a] = 0;
  abStack_28[0x1b] = 0;
  abStack_28[0x14] = 0;
  abStack_28[0x15] = 0;
  abStack_28[0x16] = 0;
  abStack_28[0x17] = 0;
  abStack_28[0x10] = 0;
  abStack_28[0x11] = 0;
  abStack_28[0x12] = 0;
  abStack_28[0x13] = 0;
  abStack_28[0xc] = 0;
  abStack_28[0xd] = 0;
  abStack_28[0xe] = 0;
  abStack_28[0xf] = 0;
  abStack_28[8] = 0;
  abStack_28[9] = 0;
  abStack_28[10] = 0;
  abStack_28[0xb] = 0;
  abStack_28[4] = 0;
  abStack_28[5] = 0;
  abStack_28[6] = 0;
  abStack_28[7] = 0;
  abStack_28[0] = 0;
  abStack_28[1] = 0;
  abStack_28[2] = 0;
  abStack_28[3] = 0;
  while( true ) {
    bVar1 = *param_2;
    if (bVar1 == 0) break;
    param_2 = param_2 + 1;
    abStack_28[(int)(uint)bVar1 >> 3] = abStack_28[(int)(uint)bVar1 >> 3] | '\x01' << (bVar1 & 7);
  }
  iVar2 = -1;
  do {
    iVar2 = iVar2 + 1;
    bVar1 = *param_1;
    if (bVar1 == 0) {
      return iVar2;
    }
    param_1 = param_1 + 1;
  } while ((abStack_28[(int)(uint)bVar1 >> 3] >> (bVar1 & 7) & 1) == 0);
  return iVar2;
}



byte * __cdecl FUN_00418850(byte *param_1,byte *param_2)

{
  byte bVar1;
  byte *pbVar2;
  byte abStack_28 [32];
  
  abStack_28[0x1c] = 0;
  abStack_28[0x1d] = 0;
  abStack_28[0x1e] = 0;
  abStack_28[0x1f] = 0;
  abStack_28[0x18] = 0;
  abStack_28[0x19] = 0;
  abStack_28[0x1a] = 0;
  abStack_28[0x1b] = 0;
  abStack_28[0x14] = 0;
  abStack_28[0x15] = 0;
  abStack_28[0x16] = 0;
  abStack_28[0x17] = 0;
  abStack_28[0x10] = 0;
  abStack_28[0x11] = 0;
  abStack_28[0x12] = 0;
  abStack_28[0x13] = 0;
  abStack_28[0xc] = 0;
  abStack_28[0xd] = 0;
  abStack_28[0xe] = 0;
  abStack_28[0xf] = 0;
  abStack_28[8] = 0;
  abStack_28[9] = 0;
  abStack_28[10] = 0;
  abStack_28[0xb] = 0;
  abStack_28[4] = 0;
  abStack_28[5] = 0;
  abStack_28[6] = 0;
  abStack_28[7] = 0;
  abStack_28[0] = 0;
  abStack_28[1] = 0;
  abStack_28[2] = 0;
  abStack_28[3] = 0;
  while( true ) {
    bVar1 = *param_2;
    if (bVar1 == 0) break;
    param_2 = param_2 + 1;
    abStack_28[(int)(uint)bVar1 >> 3] = abStack_28[(int)(uint)bVar1 >> 3] | '\x01' << (bVar1 & 7);
  }
  do {
    pbVar2 = param_1;
    bVar1 = *pbVar2;
    if (bVar1 == 0) {
      return (byte *)0x0;
    }
    param_1 = pbVar2 + 1;
  } while ((abStack_28[(int)(uint)bVar1 >> 3] >> (bVar1 & 7) & 1) == 0);
  return pbVar2;
}



int __cdecl FUN_0041888a(uint param_1,int param_2)

{
  DWORD DVar1;
  DWORD DVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  DWORD *pDVar6;
  HANDLE hFile;
  BOOL BVar7;
  int iVar8;
  uint uVar9;
  char local_1008 [4064];
  undefined4 uStackY_28;
  
  FUN_004101e0();
  iVar8 = 0;
  DVar1 = FUN_0041506a(param_1,0,1);
  if ((DVar1 == 0xffffffff) || (DVar2 = FUN_0041506a(param_1,0,2), DVar2 == 0xffffffff)) {
    iVar8 = -1;
  }
  else {
    uVar9 = param_2 - DVar2;
    if ((int)uVar9 < 1) {
      if ((int)uVar9 < 0) {
        FUN_0041506a(param_1,param_2,0);
        hFile = (HANDLE)FUN_0041763d(param_1);
        BVar7 = SetEndOfFile(hFile);
        iVar8 = (BVar7 != 0) - 1;
        if (iVar8 == -1) {
          pDVar6 = FUN_00410d43();
          *pDVar6 = 0xd;
          DVar2 = GetLastError();
          pDVar6 = FUN_00410d4c();
          *pDVar6 = DVar2;
        }
      }
    }
    else {
      _memset(local_1008,0,0x1000);
      uStackY_28 = 0x4188f7;
      iVar3 = FUN_00419181(param_1,0x8000);
      do {
        uVar4 = 0x1000;
        if ((int)uVar9 < 0x1000) {
          uVar4 = uVar9;
        }
        iVar5 = FUN_00414298(param_1,local_1008,uVar4);
        if (iVar5 == -1) {
          pDVar6 = FUN_00410d4c();
          if (*pDVar6 == 5) {
            pDVar6 = FUN_00410d43();
            *pDVar6 = 0xd;
          }
          iVar8 = -1;
          break;
        }
        uVar9 = uVar9 - iVar5;
      } while (0 < (int)uVar9);
      FUN_00419181(param_1,iVar3);
    }
    FUN_0041506a(param_1,DVar1,0);
  }
  return iVar8;
}



undefined4 __cdecl FUN_004189af(uint param_1,uint param_2,uint *param_3)

{
  uint uVar1;
  undefined4 uVar2;
  
  uVar2 = 0;
  uVar1 = param_1 + param_2;
  if ((uVar1 < param_1) || (uVar1 < param_2)) {
    uVar2 = 1;
  }
  *param_3 = uVar1;
  return uVar2;
}



// Library Function - Single Match
//  ___add_12
// 
// Library: Visual Studio 2003 Release

void __cdecl ___add_12(uint *param_1,uint *param_2)

{
  int iVar1;
  
  iVar1 = FUN_004189af(*param_1,*param_2,param_1);
  if (iVar1 != 0) {
    iVar1 = FUN_004189af(param_1[1],1,param_1 + 1);
    if (iVar1 != 0) {
      param_1[2] = param_1[2] + 1;
    }
  }
  iVar1 = FUN_004189af(param_1[1],param_2[1],param_1 + 1);
  if (iVar1 != 0) {
    param_1[2] = param_1[2] + 1;
  }
  FUN_004189af(param_1[2],param_2[2],param_1 + 2);
  return;
}



void __cdecl FUN_00418a2e(uint *param_1)

{
  uint uVar1;
  uint uVar2;
  
  uVar1 = *param_1;
  uVar2 = param_1[1];
  *param_1 = uVar1 * 2;
  param_1[1] = uVar2 * 2 | uVar1 >> 0x1f;
  param_1[2] = param_1[2] << 1 | uVar2 >> 0x1f;
  return;
}



void __cdecl FUN_00418a5c(uint *param_1)

{
  uint uVar1;
  
  uVar1 = param_1[1];
  param_1[1] = uVar1 >> 1 | param_1[2] << 0x1f;
  param_1[2] = param_1[2] >> 1;
  *param_1 = *param_1 >> 1 | uVar1 << 0x1f;
  return;
}



void __cdecl FUN_00418a89(char *param_1,int param_2,uint *param_3)

{
  uint *puVar1;
  uint local_14;
  uint local_10;
  uint local_c;
  int local_8;
  
  puVar1 = param_3;
  local_8 = 0x404e;
  *param_3 = 0;
  param_3[1] = 0;
  param_3[2] = 0;
  if (param_2 != 0) {
    param_3 = (uint *)param_2;
    do {
      local_14 = *puVar1;
      local_10 = puVar1[1];
      local_c = puVar1[2];
      FUN_00418a2e(puVar1);
      FUN_00418a2e(puVar1);
      ___add_12(puVar1,&local_14);
      FUN_00418a2e(puVar1);
      local_10 = 0;
      local_c = 0;
      local_14 = (uint)*param_1;
      ___add_12(puVar1,&local_14);
      param_1 = param_1 + 1;
      param_3 = (uint *)((int)param_3 + -1);
    } while (param_3 != (uint *)0x0);
  }
  while (puVar1[2] == 0) {
    puVar1[2] = puVar1[1] >> 0x10;
    local_8 = local_8 + 0xfff0;
    puVar1[1] = *puVar1 >> 0x10 | puVar1[1] << 0x10;
    *puVar1 = *puVar1 << 0x10;
  }
  while ((puVar1[2] & 0x8000) == 0) {
    FUN_00418a2e(puVar1);
    local_8 = local_8 + 0xffff;
  }
  *(undefined2 *)((int)puVar1 + 10) = (undefined2)local_8;
  return;
}



void __cdecl FUN_00418b50(int *param_1,int *param_2)

{
  int *piVar1;
  short sVar2;
  int iVar3;
  int *piVar4;
  int *piVar5;
  ushort uVar6;
  uint uVar7;
  int iVar8;
  ushort uVar9;
  uint uVar10;
  ushort uVar11;
  byte local_28;
  undefined1 uStack_27;
  undefined2 uStack_26;
  short local_24;
  undefined2 uStack_22;
  undefined2 local_20;
  undefined1 uStack_1e;
  byte bStack_1d;
  int *local_1c;
  int local_18;
  int local_14;
  ushort *local_10;
  ushort *local_c;
  short *local_8;
  
  piVar5 = param_2;
  piVar4 = param_1;
  local_18 = 0;
  local_28 = 0;
  uStack_27 = 0;
  uStack_26 = 0;
  local_24 = 0;
  uStack_22 = 0;
  local_20 = 0;
  uStack_1e = 0;
  bStack_1d = 0;
  uVar7 = *(ushort *)((int)param_1 + 10) & 0x7fff;
  uVar10 = *(ushort *)((int)param_2 + 10) & 0x7fff;
  uVar11 = (*(ushort *)((int)param_2 + 10) ^ *(ushort *)((int)param_1 + 10)) & 0x8000;
  uVar6 = (ushort)uVar7;
  piVar1 = (int *)(uVar10 + uVar7);
  if (((uVar6 < 0x7fff) && (uVar9 = (ushort)uVar10, uVar9 < 0x7fff)) && ((ushort)piVar1 < 0xbffe)) {
    if ((ushort)piVar1 < 0x3fc0) {
LAB_00418bf3:
      piVar4[2] = 0;
      piVar4[1] = 0;
      *piVar4 = 0;
      return;
    }
    if (((uVar6 != 0) || (piVar1 = (int *)((int)piVar1 + 1), (param_1[2] & 0x7fffffffU) != 0)) ||
       ((uVar6 = 0, param_1[1] != 0 || (*param_1 != 0)))) {
      param_1 = piVar1;
      if (((uVar9 == 0) && (param_1 = (int *)((int)param_1 + 1), (param_2[2] & 0x7fffffffU) == 0))
         && ((param_2[1] == 0 && (*param_2 == 0)))) goto LAB_00418bf3;
      local_14 = 0;
      local_8 = &local_24;
      param_2 = (int *)0x5;
      do {
        if (0 < (int)param_2) {
          local_c = (ushort *)(local_14 * 2 + (int)piVar4);
          local_10 = (ushort *)(piVar5 + 2);
          local_1c = param_2;
          do {
            iVar8 = FUN_004189af(*(uint *)(local_8 + -2),(uint)*local_c * (uint)*local_10,
                                 (uint *)(local_8 + -2));
            if (iVar8 != 0) {
              *local_8 = *local_8 + 1;
            }
            local_c = local_c + 1;
            local_10 = local_10 + -1;
            local_1c = (int *)((int)local_1c + -1);
          } while (local_1c != (int *)0x0);
        }
        local_8 = local_8 + 1;
        local_14 = local_14 + 1;
        param_2 = (int *)((int)param_2 + -1);
      } while (0 < (int)param_2);
      param_1 = (int *)((int)param_1 + 0xc002);
      if ((short)(ushort)param_1 < 1) {
LAB_00418ca7:
        param_1._0_2_ = (ushort)param_1 - 1;
        if ((short)(ushort)param_1 < 0) {
          iVar8 = -(int)(short)(ushort)param_1;
          param_1._0_2_ = (ushort)param_1 + (short)iVar8;
          do {
            if ((local_28 & 1) != 0) {
              local_18 = local_18 + 1;
            }
            FUN_00418a5c((uint *)&local_28);
            iVar8 = iVar8 + -1;
          } while (iVar8 != 0);
          if (local_18 != 0) {
            local_28 = local_28 | 1;
          }
        }
      }
      else {
        do {
          if ((bStack_1d & 0x80) != 0) break;
          FUN_00418a2e((uint *)&local_28);
          param_1 = (int *)((int)param_1 + 0xffff);
        } while (0 < (short)(ushort)param_1);
        if ((short)(ushort)param_1 < 1) goto LAB_00418ca7;
      }
      if ((0x8000 < CONCAT11(uStack_27,local_28)) ||
         (sVar2 = CONCAT11(bStack_1d,uStack_1e), iVar3 = CONCAT22(local_20,uStack_22),
         iVar8 = CONCAT22(local_24,uStack_26),
         (CONCAT22(uStack_26,CONCAT11(uStack_27,local_28)) & 0x1ffff) == 0x18000)) {
        if (CONCAT22(local_24,uStack_26) == -1) {
          iVar8 = 0;
          if (CONCAT22(local_20,uStack_22) == -1) {
            if (CONCAT11(bStack_1d,uStack_1e) == -1) {
              param_1._0_2_ = (ushort)param_1 + 1;
              sVar2 = -0x8000;
              iVar3 = 0;
              iVar8 = 0;
            }
            else {
              sVar2 = CONCAT11(bStack_1d,uStack_1e) + 1;
              iVar3 = 0;
              iVar8 = 0;
            }
          }
          else {
            sVar2 = CONCAT11(bStack_1d,uStack_1e);
            iVar3 = CONCAT22(local_20,uStack_22) + 1;
          }
        }
        else {
          iVar8 = CONCAT22(local_24,uStack_26) + 1;
          sVar2 = CONCAT11(bStack_1d,uStack_1e);
          iVar3 = CONCAT22(local_20,uStack_22);
        }
      }
      local_24 = (short)((uint)iVar8 >> 0x10);
      uStack_26 = (undefined2)iVar8;
      local_20 = (undefined2)((uint)iVar3 >> 0x10);
      uStack_22 = (undefined2)iVar3;
      bStack_1d = (byte)((ushort)sVar2 >> 8);
      uStack_1e = (undefined1)sVar2;
      if (0x7ffe < (ushort)param_1) goto LAB_00418d50;
      uVar6 = (ushort)param_1 | uVar11;
      *(undefined2 *)piVar4 = uStack_26;
      *(uint *)((int)piVar4 + 2) = CONCAT22(uStack_22,local_24);
      *(uint *)((int)piVar4 + 6) = CONCAT13(bStack_1d,CONCAT12(uStack_1e,local_20));
    }
    *(ushort *)((int)piVar4 + 10) = uVar6;
  }
  else {
LAB_00418d50:
    piVar4[1] = 0;
    *piVar4 = 0;
    piVar4[2] = (-(uint)(uVar11 != 0) & 0x80000000) + 0x7fff8000;
  }
  return;
}



void __cdecl FUN_00418d70(int *param_1,uint param_2,int param_3)

{
  uint uVar1;
  uint uVar2;
  undefined **ppuVar3;
  undefined **ppuVar4;
  undefined2 local_10;
  undefined4 local_e;
  undefined2 uStack_a;
  undefined *puStack_8;
  
  ppuVar3 = &PTR_DAT_00423550;
  if (param_2 != 0) {
    if ((int)param_2 < 0) {
      param_2 = -param_2;
      ppuVar3 = (undefined **)0x4236b0;
    }
    if (param_3 == 0) {
      *(undefined2 *)param_1 = 0;
    }
    while (param_2 != 0) {
      ppuVar3 = ppuVar3 + 0x15;
      uVar1 = (int)param_2 >> 3;
      uVar2 = param_2 & 7;
      param_2 = uVar1;
      if (uVar2 != 0) {
        ppuVar4 = ppuVar3 + uVar2 * 3;
        if (0x7fff < *(ushort *)(ppuVar3 + uVar2 * 3)) {
          local_10 = SUB42(*ppuVar4,0);
          local_e._0_2_ = (undefined2)((uint)*ppuVar4 >> 0x10);
          local_e._2_2_ = SUB42(ppuVar4[1],0);
          uStack_a = (undefined2)((uint)ppuVar4[1] >> 0x10);
          puStack_8 = ppuVar4[2];
          local_e = CONCAT22(local_e._2_2_,(undefined2)local_e) + -1;
          ppuVar4 = (undefined **)&local_10;
        }
        FUN_00418b50(param_1,(int *)ppuVar4);
      }
    }
  }
  return;
}



undefined4 __cdecl
FUN_00418dec(uint param_1,uint param_2,uint param_3,int param_4,byte param_5,short *param_6)

{
  short *psVar1;
  uint uVar2;
  short *psVar3;
  char cVar4;
  uint uVar5;
  short *psVar6;
  short *psVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  char *pcVar11;
  undefined1 local_20;
  undefined1 local_1f;
  undefined1 local_1e;
  undefined1 local_1d;
  undefined1 local_1c;
  undefined1 local_1b;
  undefined1 local_1a;
  undefined1 local_19;
  undefined1 local_18;
  undefined1 local_17;
  undefined1 local_16;
  undefined1 local_15;
  undefined2 local_14;
  undefined4 local_12;
  undefined4 local_e;
  undefined1 local_a;
  char cStack_9;
  undefined4 local_8;
  
  psVar3 = param_6;
  uVar5 = param_3 & 0x7fff;
  local_20 = 0xcc;
  local_1f = 0xcc;
  local_1e = 0xcc;
  local_1d = 0xcc;
  local_1c = 0xcc;
  local_1b = 0xcc;
  local_1a = 0xcc;
  local_19 = 0xcc;
  local_18 = 0xcc;
  local_17 = 0xcc;
  local_16 = 0xfb;
  local_15 = 0x3f;
  local_8 = 1;
  if ((param_3 & 0x8000) == 0) {
    *(undefined1 *)(param_6 + 1) = 0x20;
  }
  else {
    *(undefined1 *)(param_6 + 1) = 0x2d;
  }
  if ((((short)uVar5 != 0) || (param_2 != 0)) || (param_1 != 0)) {
    if ((short)uVar5 == 0x7fff) {
      *param_6 = 1;
      if (((param_2 == 0x80000000) && (param_1 == 0)) || ((param_2 & 0x40000000) != 0)) {
        if (((param_3 & 0x8000) == 0) || (param_2 != 0xc0000000)) {
          if ((param_2 != 0x80000000) || (param_1 != 0)) goto LAB_00418ee1;
          pcVar11 = "1#INF";
        }
        else {
          if (param_1 != 0) {
LAB_00418ee1:
            pcVar11 = "1#QNAN";
            goto LAB_00418ee6;
          }
          pcVar11 = "1#IND";
        }
        FUN_00416770((uint *)(param_6 + 2),(uint *)pcVar11);
        *(undefined1 *)((int)psVar3 + 3) = 5;
      }
      else {
        pcVar11 = "1#SNAN";
LAB_00418ee6:
        FUN_00416770((uint *)(param_6 + 2),(uint *)pcVar11);
        *(undefined1 *)((int)psVar3 + 3) = 6;
      }
      return 0;
    }
    local_14 = 0;
    local_a = (undefined1)uVar5;
    cStack_9 = (char)(uVar5 >> 8);
    sVar8 = (short)(((uVar5 >> 8) + (param_2 >> 0x18) * 2) * 0x4d + -0x134312f4 + uVar5 * 0x4d10 >>
                   0x10);
    local_e = param_2;
    local_12 = param_1;
    FUN_00418d70((int *)&local_14,-(int)sVar8,1);
    if (0x3ffe < CONCAT11(cStack_9,local_a)) {
      sVar8 = sVar8 + 1;
      FUN_00418b50((int *)&local_14,(int *)&local_20);
    }
    *psVar3 = sVar8;
    iVar10 = param_4;
    if (((param_5 & 1) == 0) || (iVar10 = param_4 + sVar8, 0 < param_4 + sVar8)) {
      if (0x15 < iVar10) {
        iVar10 = 0x15;
      }
      iVar9 = CONCAT11(cStack_9,local_a) - 0x3ffe;
      local_a = 0;
      cStack_9 = '\0';
      param_6 = (short *)0x8;
      do {
        FUN_00418a2e((uint *)&local_14);
        param_6 = (short *)((int)param_6 + -1);
      } while (param_6 != (short *)0x0);
      if (iVar9 < 0) {
        param_6 = (short *)0x0;
        for (uVar5 = -iVar9 & 0xff; uVar5 != 0; uVar5 = uVar5 - 1) {
          FUN_00418a5c((uint *)&local_14);
        }
      }
      param_4 = iVar10 + 1;
      psVar6 = psVar3 + 2;
      param_6 = psVar6;
      uVar5 = local_12;
      uVar2 = local_e;
      if (0 < param_4) {
        do {
          local_e._2_2_ = (undefined2)(uVar2 >> 0x10);
          local_e._0_2_ = (undefined2)uVar2;
          local_12._2_2_ = (undefined2)(uVar5 >> 0x10);
          local_12._0_2_ = (undefined2)uVar5;
          param_1 = CONCAT22((undefined2)local_12,local_14);
          param_2 = CONCAT22((undefined2)local_e,local_12._2_2_);
          param_3 = CONCAT13(cStack_9,CONCAT12(local_a,local_e._2_2_));
          local_12 = uVar5;
          local_e = uVar2;
          FUN_00418a2e((uint *)&local_14);
          FUN_00418a2e((uint *)&local_14);
          ___add_12((uint *)&local_14,&param_1);
          FUN_00418a2e((uint *)&local_14);
          cVar4 = cStack_9;
          cStack_9 = '\0';
          psVar6 = (short *)((int)param_6 + 1);
          param_4 = param_4 + -1;
          *(char *)param_6 = cVar4 + '0';
          param_6 = psVar6;
          uVar5 = local_12;
          uVar2 = local_e;
        } while (param_4 != 0);
      }
      psVar7 = psVar6 + -1;
      psVar1 = psVar3 + 2;
      if ('4' < *(char *)((int)psVar6 + -1)) {
        for (; psVar1 <= psVar7; psVar7 = (short *)((int)psVar7 + -1)) {
          if ((char)*psVar7 != '9') {
            if (psVar1 <= psVar7) goto LAB_0041903e;
            break;
          }
          *(char *)psVar7 = '0';
        }
        psVar7 = (short *)((int)psVar7 + 1);
        *psVar3 = *psVar3 + 1;
LAB_0041903e:
        *(char *)psVar7 = (char)*psVar7 + '\x01';
LAB_00419040:
        cVar4 = ((char)psVar7 - (char)psVar3) + -3;
        *(char *)((int)psVar3 + 3) = cVar4;
        *(undefined1 *)(cVar4 + 4 + (int)psVar3) = 0;
        return local_8;
      }
      for (; psVar1 <= psVar7; psVar7 = (short *)((int)psVar7 + -1)) {
        if ((char)*psVar7 != '0') {
          if (psVar1 <= psVar7) goto LAB_00419040;
          break;
        }
      }
      *psVar3 = 0;
      *(undefined1 *)(psVar3 + 1) = 0x20;
      *(undefined1 *)((int)psVar3 + 3) = 1;
      *(char *)psVar1 = '0';
      goto LAB_00419076;
    }
  }
  *psVar3 = 0;
  *(undefined1 *)(psVar3 + 1) = 0x20;
  *(undefined1 *)((int)psVar3 + 3) = 1;
  *(undefined1 *)(psVar3 + 2) = 0x30;
LAB_00419076:
  *(undefined1 *)((int)psVar3 + 5) = 0;
  return 1;
}



// WARNING: Globals starting with '_' overlap smaller symbols at the same address

undefined4 __cdecl FUN_00419080(byte *param_1,char *param_2,void *param_3)

{
  char cVar1;
  int iVar2;
  byte bVar3;
  ushort uVar4;
  uint uVar5;
  undefined4 uVar6;
  void *this;
  uint uVar7;
  bool bVar8;
  uint uVar9;
  
  iVar2 = _DAT_00423da8;
  uVar6 = 0;
  if (param_3 != (void *)0x0) {
    if (DAT_00423bfc == 0) {
      do {
        bVar3 = *param_1;
        cVar1 = *param_2;
        uVar4 = CONCAT11(bVar3,cVar1);
        if (bVar3 == 0) break;
        uVar4 = CONCAT11(bVar3,cVar1);
        uVar7 = (uint)uVar4;
        if (cVar1 == '\0') break;
        param_1 = param_1 + 1;
        param_2 = param_2 + 1;
        if ((0x40 < bVar3) && (bVar3 < 0x5b)) {
          uVar7 = (uint)CONCAT11(bVar3 + 0x20,cVar1);
        }
        uVar4 = (ushort)uVar7;
        bVar3 = (byte)uVar7;
        if ((0x40 < bVar3) && (bVar3 < 0x5b)) {
          uVar4 = (ushort)CONCAT31((int3)(uVar7 >> 8),bVar3 + 0x20);
        }
        bVar3 = (byte)(uVar4 >> 8);
        bVar8 = bVar3 < (byte)uVar4;
        if (bVar3 != (byte)uVar4) goto LAB_004190df;
        param_3 = (void *)((int)param_3 + -1);
      } while (param_3 != (void *)0x0);
      uVar6 = 0;
      bVar3 = (byte)(uVar4 >> 8);
      bVar8 = bVar3 < (byte)uVar4;
      if (bVar3 != (byte)uVar4) {
LAB_004190df:
        uVar6 = 0xffffffff;
        if (!bVar8) {
          uVar6 = 1;
        }
      }
    }
    else {
      LOCK();
      _DAT_00423da8 = _DAT_00423da8 + 1;
      UNLOCK();
      bVar8 = 0 < DAT_00423da4;
      if (bVar8) {
        LOCK();
        UNLOCK();
        _DAT_00423da8 = iVar2;
        FUN_004130b6(0x13);
      }
      uVar9 = (uint)bVar8;
      uVar5 = 0;
      uVar7 = 0;
      do {
        uVar5 = CONCAT31((int3)(uVar5 >> 8),*param_1);
        uVar7 = CONCAT31((int3)(uVar7 >> 8),*param_2);
        if ((uVar5 == 0) || (uVar7 == 0)) break;
        param_1 = param_1 + 1;
        param_2 = param_2 + 1;
        uVar7 = FUN_004171d0(param_3,uVar7);
        uVar5 = FUN_004171d0(this,uVar5);
        bVar8 = uVar5 < uVar7;
        if (uVar5 != uVar7) goto LAB_00419155;
        param_3 = (void *)((int)param_3 + -1);
      } while (param_3 != (void *)0x0);
      uVar6 = 0;
      bVar8 = uVar5 < uVar7;
      if (uVar5 != uVar7) {
LAB_00419155:
        uVar6 = 0xffffffff;
        if (!bVar8) {
          uVar6 = 1;
        }
      }
      if (uVar9 == 0) {
        LOCK();
        _DAT_00423da8 = _DAT_00423da8 + -1;
        UNLOCK();
      }
      else {
        FUN_00413117(0x13);
      }
    }
  }
  return uVar6;
}



int __cdecl FUN_00419181(uint param_1,int param_2)

{
  byte bVar1;
  DWORD *pDVar2;
  byte bVar3;
  
  bVar1 = *(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + (param_1 & 0x1f) * 0x24);
  if (param_2 == 0x8000) {
    bVar3 = bVar1 & 0x7f;
  }
  else {
    if (param_2 != 0x4000) {
      pDVar2 = FUN_00410d43();
      *pDVar2 = 0x16;
      return -1;
    }
    bVar3 = bVar1 | 0x80;
  }
  *(byte *)((&DAT_00423de0)[(int)param_1 >> 5] + 4 + (param_1 & 0x1f) * 0x24) = bVar3;
  return (-(uint)((bVar1 & 0x80) != 0) & 0xffffc000) + 0x8000;
}



int WSAStartup(WORD wVersionRequired,LPWSADATA lpWSAData)

{
  int iVar1;
  
                    // WARNING: Could not recover jumptable at 0x004191f0. Too many branches
                    // WARNING: Treating indirect jump as call
  iVar1 = WSAStartup(wVersionRequired,lpWSAData);
  return iVar1;
}



int WSACleanup(void)

{
  int iVar1;
  
                    // WARNING: Could not recover jumptable at 0x004191f6. Too many branches
                    // WARNING: Treating indirect jump as call
  iVar1 = WSACleanup();
  return iVar1;
}



int WSACancelBlockingCall(void)

{
  int iVar1;
  
                    // WARNING: Could not recover jumptable at 0x004191fc. Too many branches
                    // WARNING: Treating indirect jump as call
  iVar1 = WSACancelBlockingCall();
  return iVar1;
}



int send(SOCKET s,char *buf,int len,int flags)

{
  int iVar1;
  
                    // WARNING: Could not recover jumptable at 0x00419202. Too many branches
                    // WARNING: Treating indirect jump as call
  iVar1 = send(s,buf,len,flags);
  return iVar1;
}



int closesocket(SOCKET s)

{
  int iVar1;
  
                    // WARNING: Could not recover jumptable at 0x00419208. Too many branches
                    // WARNING: Treating indirect jump as call
  iVar1 = closesocket(s);
  return iVar1;
}



int connect(SOCKET s,sockaddr *name,int namelen)

{
  int iVar1;
  
                    // WARNING: Could not recover jumptable at 0x0041920e. Too many branches
                    // WARNING: Treating indirect jump as call
  iVar1 = connect(s,name,namelen);
  return iVar1;
}



u_long htonl(u_long hostlong)

{
  u_long uVar1;
  
                    // WARNING: Could not recover jumptable at 0x00419214. Too many branches
                    // WARNING: Treating indirect jump as call
  uVar1 = htonl(hostlong);
  return uVar1;
}



u_short htons(u_short hostshort)

{
  u_short uVar1;
  
                    // WARNING: Could not recover jumptable at 0x0041921a. Too many branches
                    // WARNING: Treating indirect jump as call
  uVar1 = htons(hostshort);
  return uVar1;
}



SOCKET socket(int af,int type,int protocol)

{
  SOCKET SVar1;
  
                    // WARNING: Could not recover jumptable at 0x00419220. Too many branches
                    // WARNING: Treating indirect jump as call
  SVar1 = socket(af,type,protocol);
  return SVar1;
}



int WSAGetLastError(void)

{
  int iVar1;
  
                    // WARNING: Could not recover jumptable at 0x00419226. Too many branches
                    // WARNING: Treating indirect jump as call
  iVar1 = WSAGetLastError();
  return iVar1;
}



int recv(SOCKET s,char *buf,int len,int flags)

{
  int iVar1;
  
                    // WARNING: Could not recover jumptable at 0x0041922c. Too many branches
                    // WARNING: Treating indirect jump as call
  iVar1 = recv(s,buf,len,flags);
  return iVar1;
}



hostent * gethostbyaddr(char *addr,int len,int type)

{
  hostent *phVar1;
  
                    // WARNING: Could not recover jumptable at 0x00419232. Too many branches
                    // WARNING: Treating indirect jump as call
  phVar1 = gethostbyaddr(addr,len,type);
  return phVar1;
}



ulong inet_addr(char *cp)

{
  ulong uVar1;
  
                    // WARNING: Could not recover jumptable at 0x00419238. Too many branches
                    // WARNING: Treating indirect jump as call
  uVar1 = inet_addr(cp);
  return uVar1;
}



hostent * gethostbyname(char *name)

{
  hostent *phVar1;
  
                    // WARNING: Could not recover jumptable at 0x0041923e. Too many branches
                    // WARNING: Treating indirect jump as call
  phVar1 = gethostbyname(name);
  return phVar1;
}



int setsockopt(SOCKET s,int level,int optname,char *optval,int optlen)

{
  int iVar1;
  
                    // WARNING: Could not recover jumptable at 0x00419244. Too many branches
                    // WARNING: Treating indirect jump as call
  iVar1 = setsockopt(s,level,optname,optval,optlen);
  return iVar1;
}



void RtlUnwind(PVOID TargetFrame,PVOID TargetIp,PEXCEPTION_RECORD ExceptionRecord,PVOID ReturnValue)

{
                    // WARNING: Could not recover jumptable at 0x0041924a. Too many branches
                    // WARNING: Treating indirect jump as call
  RtlUnwind(TargetFrame,TargetIp,ExceptionRecord,ReturnValue);
  return;
}



void Unwind_00419250(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0xb0);
  return;
}



void Unwind_0041925b(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0xc0);
  return;
}



void Unwind_00419266(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x84);
  return;
}



void Unwind_00419271(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x74);
  return;
}



void Unwind_00419279(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0xc0);
  return;
}



void Unwind_00419284(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x84);
  return;
}



void Unwind_004192b0(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1c);
  return;
}



void Unwind_004192d0(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1c);
  return;
}



void Unwind_004192f0(void)

{
  int unaff_EBP;
  
  FUN_00402aa0((void *)(unaff_EBP + -0x168));
  return;
}



void Unwind_004192fb(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x150);
  return;
}



void Unwind_00419306(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x130);
  return;
}



void Unwind_00419311(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x140);
  return;
}



void Unwind_0041931c(void)

{
  int unaff_EBP;
  
  FUN_00402aa0((void *)(unaff_EBP + -0x120));
  return;
}



void Unwind_00419340(void)

{
  FUN_00401d40();
  return;
}



void Unwind_00419351(void)

{
  int unaff_EBP;
  
  FUN_00401340(*(int *)(unaff_EBP + -0x10));
  return;
}



void Unwind_00419370(void)

{
  int unaff_EBP;
  
  FUN_00403e00((undefined4 *)(unaff_EBP + -0x2030));
  return;
}



void Unwind_0041937b(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x201c);
  return;
}



void Unwind_00419390(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1e0);
  return;
}



void Unwind_0041939b(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1c0);
  return;
}



void Unwind_004193a6(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1d0);
  return;
}



void Unwind_004193b1(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1f0);
  return;
}



void Unwind_004193bc(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1b0);
  return;
}



void Unwind_004193c7(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1d0);
  return;
}



void Unwind_004193d2(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1d0);
  return;
}



void Unwind_004193dd(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1b0);
  return;
}



void Unwind_004193e8(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1b0);
  return;
}



void Unwind_00419400(void)

{
  int unaff_EBP;
  
  if ((*(uint *)(unaff_EBP + -0x30) & 2) != 0) {
    FUN_00401340(unaff_EBP + -0x2c);
    return;
  }
  return;
}



void Unwind_00419417(void)

{
  int unaff_EBP;
  
  if ((*(uint *)(unaff_EBP + -0x30) & 4) != 0) {
    FUN_00401340(unaff_EBP + -0x1c);
    return;
  }
  return;
}



void Unwind_00419440(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x2c);
  return;
}



void Unwind_00419448(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1c);
  return;
}



void Unwind_00419460(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x2c);
  return;
}



void Unwind_00419480(void)

{
  int unaff_EBP;
  
  FUN_004064b0((undefined4 *)(unaff_EBP + -0x178));
  return;
}



void Unwind_0041948b(void)

{
  int unaff_EBP;
  
  FUN_00406780((undefined4 *)(unaff_EBP + -0x114));
  return;
}



void Unwind_00419496(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x188);
  return;
}



void Unwind_004194a1(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x148);
  return;
}



void Unwind_004194ac(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x124);
  return;
}



void Unwind_004194b7(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x168);
  return;
}



void Unwind_004194c2(void)

{
  int unaff_EBP;
  
  FUN_00403e00((undefined4 *)(unaff_EBP + -0x138));
  return;
}



void Unwind_004194cd(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x158);
  return;
}



void Unwind_004194d8(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x158);
  return;
}



void Unwind_004194f0(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1c);
  return;
}



void Unwind_00419510(void)

{
  int unaff_EBP;
  
  FUN_0040e050(unaff_EBP + -0x2c);
  return;
}



void Unwind_00419518(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1c);
  return;
}



void Unwind_00419520(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x1c);
  return;
}



void Unwind_00419528(void)

{
  FUN_00401d40();
  return;
}



void Unwind_00419539(void)

{
  int unaff_EBP;
  
  if ((*(uint *)(unaff_EBP + -0x30) & 1) != 0) {
    FUN_0040e050(*(int *)(unaff_EBP + 4));
    return;
  }
  return;
}



void Unwind_00419560(void)

{
  int unaff_EBP;
  
  FUN_0040e050(unaff_EBP + -0xac);
  return;
}



void Unwind_0041956b(void)

{
  int unaff_EBP;
  
  FUN_00402aa0((void *)(unaff_EBP + -200));
  return;
}



void Unwind_00419576(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x6c);
  return;
}



void Unwind_0041957e(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x7c);
  return;
}



void Unwind_00419586(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x9c);
  return;
}



void Unwind_00419591(void)

{
  int unaff_EBP;
  
  FUN_00403730(unaff_EBP + -0x4c);
  return;
}



void Unwind_00419599(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x8c);
  return;
}



void Unwind_004195a4(void)

{
  int unaff_EBP;
  
  FUN_00403730(unaff_EBP + -0x2c);
  return;
}



void Unwind_004195ac(void)

{
  int unaff_EBP;
  
  if ((*(uint *)(unaff_EBP + -0xb8) & 1) != 0) {
    FUN_00402aa0(*(void **)(unaff_EBP + 4));
    return;
  }
  return;
}



void Unwind_004195d0(void)

{
  int unaff_EBP;
  
  FUN_00401340(*(int *)(unaff_EBP + -0x10));
  return;
}



void Unwind_004195f0(void)

{
  FUN_00401d40();
  return;
}



void Unwind_0041960c(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x20);
  return;
}



void Unwind_00419620(void)

{
  int unaff_EBP;
  
  exception::~exception(*(exception **)(unaff_EBP + -0x14));
  return;
}



void Unwind_00419634(void)

{
  int unaff_EBP;
  
  exception::~exception(*(exception **)(unaff_EBP + -0x10));
  return;
}



void Unwind_00419648(void)

{
  int unaff_EBP;
  
  exception::~exception(*(exception **)(unaff_EBP + -0x10));
  return;
}



void Unwind_0041965c(void)

{
  int unaff_EBP;
  
  exception::~exception(*(exception **)(unaff_EBP + -0x10));
  return;
}



void Unwind_00419670(void)

{
  int unaff_EBP;
  
  FUN_00401340(unaff_EBP + -0x20);
  return;
}



void Unwind_00419684(void)

{
  int unaff_EBP;
  
  exception::~exception(*(exception **)(unaff_EBP + -0x10));
  return;
}


