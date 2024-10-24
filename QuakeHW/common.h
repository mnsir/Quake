
// comndef.h -- general definitions

#if !defined BYTE_DEFINED
typedef unsigned char byte;
#define BYTE_DEFINED 1
#endif

#include <stdbool.h>


//============================================================================

struct sizebuf_t
{
    void SZ_Alloc(int startsize);
    void SZ_Free();
    void SZ_Clear();
    void* SZ_GetSpace(int length);
    void SZ_Write(const void* data, int length);
    void SZ_Print(const char* data); // strcats onto the sizebuf

    void MSG_WriteChar(int c);
    void MSG_WriteByte(int c);
    void MSG_WriteShort(int c);
    void MSG_WriteLong(int c);
    void MSG_WriteFloat(float f);
    void MSG_WriteString(const char* s);
    void MSG_WriteCoord(float f);
    void MSG_WriteAngle(float f);

    void AllowOverflow() { allowoverflow_ = true; }
    bool IsOverflowed() const { return overflowed_; }
    void ResetOverflowed() { overflowed_ = false; }

private:
    bool allowoverflow_ = false; // if false, do a Sys_Error
    bool overflowed_ = false; // set to true if the buffer size failed

public:
    byte* data = nullptr;
    int maxsize = 0;
    int cursize = 0;
};

//============================================================================

typedef struct link_s
{
    struct link_s * prev, * next;
} link_t;


void ClearLink(link_t * l);
void RemoveLink(link_t * l);
void InsertLinkBefore(link_t * l, link_t * before);
void InsertLinkAfter(link_t * l, link_t * after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (int)&(((t *)0)->m)))

//============================================================================

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((short)0x7fff)
#define Q_MAXINT ((int)0x7fffffff)
#define Q_MAXLONG ((int)0x7fffffff)
#define Q_MAXFLOAT ((int)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((short)0x8000)
#define Q_MININT ((int)0x80000000)
#define Q_MINLONG ((int)0x80000000)
#define Q_MINFLOAT ((int)0x7fffffff)

//============================================================================

struct Msg
{
    static inline int readcount = 0;
    static inline bool badread = false; // set if a read goes beyond end of message

    static void BeginReading();
    static int ReadChar(const sizebuf_t& buf);
    static int ReadByte(const sizebuf_t& buf);
    static int ReadShort(const sizebuf_t& buf);
    static int ReadLong(const sizebuf_t& buf);
    static float ReadFloat(const sizebuf_t& buf);
    static const char* ReadString(const sizebuf_t& buf);

    static float ReadCoord(const sizebuf_t& buf);
    static float ReadAngle(const sizebuf_t& buf);
};

//============================================================================

int Q_strcasecmp(const char * s1, const char * s2);
int Q_strncasecmp(const char * s1, const char * s2, int n);

//============================================================================

extern char com_token[1024];
extern bool com_eof;

const char * COM_Parse(const char * data);


extern int com_argc;
extern char ** com_argv;

int COM_CheckParm(char * parm);
void COM_Init(char * path);
void COM_InitArgv(int argc, char ** argv);

char * COM_SkipPath(char * pathname);
void COM_StripExtension(char * in, char * out);
void COM_FileBase(char * in, char * out);
void COM_DefaultExtension(char * path, char * extension);

char * va(const char * format, ...);
// does a varargs printf into a temp buffer


//============================================================================

extern int com_filesize;
struct cache_user_s;

extern char com_gamedir[MAX_OSPATH];

void COM_WriteFile(char * filename, void * data, int len);
int COM_OpenFile(char * filename, int * hndl);
int COM_FOpenFile(char * filename, FILE ** file);
void COM_CloseFile(int h);

byte * COM_LoadStackFile(char * path, void * buffer, int bufsize);
byte * COM_LoadTempFile(char * path);
byte * COM_LoadHunkFile(char * path);
void COM_LoadCacheFile(char * path, struct cache_user_s * cu);


extern struct cvar_s registered;

constexpr bool rogue = false;    // TODO must be checked only one game param // COM_CheckParm((char*)"-rogue")
constexpr bool hipnotic = false; // TODO must be checked only one game param // COM_CheckParm((char*)"-hipnotic")
constexpr bool standard_quake = !rogue && !hipnotic;
