
// comndef.h -- general definitions

#if !defined BYTE_DEFINED
typedef unsigned char byte;
#define BYTE_DEFINED 1
#endif

#include <stdbool.h>


//============================================================================

typedef struct sizebuf_s
{
    bool allowoverflow; // if false, do a Sys_Error
    bool overflowed; // set to true if the buffer size failed
    byte * data;
    int maxsize = 0;
    int cursize = 0;
} sizebuf_t;

void SZ_Alloc(sizebuf_t * buf, int startsize);
void SZ_Free(sizebuf_t * buf);
void SZ_Clear(sizebuf_t * buf);
void * SZ_GetSpace(sizebuf_t * buf, int length);
void SZ_Write(sizebuf_t * buf, void * data, int length);
void SZ_Print(sizebuf_t * buf, char * data); // strcats onto the sizebuf

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

void MSG_WriteChar(sizebuf_t * sb, int c);
void MSG_WriteByte(sizebuf_t * sb, int c);
void MSG_WriteShort(sizebuf_t * sb, int c);
void MSG_WriteLong(sizebuf_t * sb, int c);
void MSG_WriteFloat(sizebuf_t * sb, float f);
void MSG_WriteString(sizebuf_t * sb, char * s);
void MSG_WriteCoord(sizebuf_t * sb, float f);
void MSG_WriteAngle(sizebuf_t * sb, float f);

extern int msg_readcount;
extern bool msg_badread; // set if a read goes beyond end of message

void MSG_BeginReading();
int MSG_ReadChar();
int MSG_ReadByte();
int MSG_ReadShort();
int MSG_ReadLong();
float MSG_ReadFloat();
char * MSG_ReadString();

float MSG_ReadCoord();
float MSG_ReadAngle();

//============================================================================

int Q_strcasecmp(char * s1, char * s2);
int Q_strncasecmp(char * s1, char * s2, int n);

//============================================================================

extern char com_token[1024];
extern bool com_eof;

char * COM_Parse(char * data);


extern int com_argc;
extern char ** com_argv;

int COM_CheckParm(char * parm);
void COM_Init(char * path);
void COM_InitArgv(int argc, char ** argv);

char * COM_SkipPath(char * pathname);
void COM_StripExtension(char * in, char * out);
void COM_FileBase(char * in, char * out);
void COM_DefaultExtension(char * path, char * extension);

char * va(char * format, ...);
// does a varargs printf into a temp buffer


//============================================================================

struct cache_user_s;

extern char com_gamedir[MAX_OSPATH];

void COM_WriteFile(char * filename, void * data, int len);

std::span<unsigned char> COM_LoadFile(const char* path);


extern struct cvar_s registered;

constexpr bool rogue = false;    // TODO must be checked only one game param // COM_CheckParm((char*)"-rogue")
constexpr bool hipnotic = false; // TODO must be checked only one game param // COM_CheckParm((char*)"-hipnotic")
constexpr bool standard_quake = !rogue && !hipnotic;
