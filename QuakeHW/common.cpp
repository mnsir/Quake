
// common.c -- misc functions used in client and server

#include "quakedef.h"

#define NUM_SAFE_ARGVS 7

static char * largv[MAX_NUM_ARGVS + NUM_SAFE_ARGVS + 1];
static char * argvdummy = (char*)" ";

static char * safeargvs[NUM_SAFE_ARGVS] =
{ (char*)"-stdvid", (char*)"-nolan", (char*)"-nosound", (char*)"-nocdaudio", (char*)"-nojoy", (char*)"-nomouse", (char*)"-dibonly"};

cvar_t registered = {(char*)"registered", (char*)"0"};
cvar_t cmdline = {(char*)"cmdline", (char*)"0", false, true};

int static_registered = 1; // only for startup check, then set

bool msg_suppress_1 = 0;

void COM_InitFilesystem();

char com_token[1024];
int com_argc;
char ** com_argv;

#define CMDLINE_LENGTH 256
char com_cmdline[CMDLINE_LENGTH];

/*


All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories. The sys_* files pass this to host_init in quakeparms_t->basedir. This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory. The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to. This can be overridden with the "-game" command line parameter. The game directory can never be changed while quake is executing. This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, especially over ISDN / T1 lines. If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.



FIXME:
The file "parms.txt" will be read out of the game directory and appended to the current command line arguments to allow different games to initialize startup parms differently. This could be used to add a "-sspeed 22050" for the high quality sound edition. Because they are added at the end, they will not override an explicit setting on the original command line.

*/

//============================================================================


// ClearLink is used for new headnodes
void ClearLink(link_t * l)
{
    l->prev = l->next = l;
}

void RemoveLink(link_t * l)
{
    l->next->prev = l->prev;
    l->prev->next = l->next;
}

void InsertLinkBefore(link_t * l, link_t * before)
{
    l->next = before;
    l->prev = before->prev;
    l->prev->next = l;
    l->next->prev = l;
}
void InsertLinkAfter(link_t * l, link_t * after)
{
    l->next = after->next;
    l->prev = after;
    l->prev->next = l;
    l->next->prev = l;
}

/*
============================================================================

 LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_strncasecmp(const char * s1, const char * s2, int n)
{
    int c1, c2;

    while (1)
    {
        c1 = *s1++;
        c2 = *s2++;

        if (!n--)
            return 0; // strings are equal until end point

        if (c1 != c2)
        {
            if (c1 >= 'a' && c1 <= 'z')
                c1 -= ('a' - 'A');
            if (c2 >= 'a' && c2 <= 'z')
                c2 -= ('a' - 'A');
            if (c1 != c2)
                return -1; // strings not equal
        }
        if (!c1)
            return 0; // strings are equal
        // s1++;
        // s2++;
    }

    return -1;
}

int Q_strcasecmp(const char * s1, const char * s2)
{
    return Q_strncasecmp(s1, s2, 99999);
}

void sizebuf_t::MSG_WriteChar(int c)
{
    byte* buf;

    buf = (byte*)SZ_GetSpace(1);
    buf[0] = c;
}

void sizebuf_t::MSG_WriteByte(int c)
{
    byte* buf;

    buf = (byte*)SZ_GetSpace(1);
    buf[0] = c;
}

void sizebuf_t::MSG_WriteShort(int c)
{
    byte* buf;

    buf = (byte*)SZ_GetSpace(2);
    buf[0] = c & 0xff;
    buf[1] = c >> 8;
}

void sizebuf_t::MSG_WriteLong(int c)
{
    byte* buf;

    buf = (byte*)SZ_GetSpace(4);
    buf[0] = c & 0xff;
    buf[1] = (c >> 8) & 0xff;
    buf[2] = (c >> 16) & 0xff;
    buf[3] = c >> 24;
}

void sizebuf_t::MSG_WriteFloat(float f)
{
    union
    {
        float f;
        int l;
    } dat;


    dat.f = f;
    dat.l = dat.l;

    SZ_Write(&dat.l, 4);
}

void sizebuf_t::MSG_WriteString(const char* s)
{
    if (!s)
        SZ_Write("", 1);
    else
        SZ_Write(s, std::strlen(s) + 1);
}

void sizebuf_t::MSG_WriteCoord(float f)
{
    MSG_WriteShort((int)(f * 8));
}

void sizebuf_t::MSG_WriteAngle(float f)
{
    MSG_WriteByte(((int)f * 256 / 360) & 255);
}

void Msg::BeginReading()
{
    Msg::readcount = 0;
    Msg::badread = false;
}

// returns -1 and sets Msg::badread if no more characters are available
int Msg::ReadChar(const sizebuf_t& buf)
{
    if (Msg::readcount + 1 > buf.cursize)
    {
        Msg::badread = true;
        return -1;
    }

    int c = (signed char)buf.data[Msg::readcount];
    Msg::readcount++;

    return c;
}

int Msg::ReadByte(const sizebuf_t& buf)
{
    if (Msg::readcount + 1 > buf.cursize)
    {
        Msg::badread = true;
        return -1;
    }

    int c = (unsigned char)buf.data[Msg::readcount];
    Msg::readcount++;

    return c;
}

int Msg::ReadShort(const sizebuf_t& buf)
{
    if (Msg::readcount + 2 > buf.cursize)
    {
        Msg::badread = true;
        return -1;
    }

    int c = (short)(buf.data[Msg::readcount] + (buf.data[Msg::readcount + 1] << 8));

    Msg::readcount += 2;

    return c;
}

int Msg::ReadLong(const sizebuf_t& buf)
{
    if (Msg::readcount + 4 > buf.cursize)
    {
        Msg::badread = true;
        return -1;
    }

    int c = buf.data[Msg::readcount]
        + (buf.data[Msg::readcount + 1] << 8)
        + (buf.data[Msg::readcount + 2] << 16)
        + (buf.data[Msg::readcount + 3] << 24);

    Msg::readcount += 4;

    return c;
}

float Msg::ReadFloat(const sizebuf_t& buf)
{
    union
    {
        byte b[4];
        float f;
        int l;
    } dat;

    dat.b[0] = buf.data[Msg::readcount];
    dat.b[1] = buf.data[Msg::readcount + 1];
    dat.b[2] = buf.data[Msg::readcount + 2];
    dat.b[3] = buf.data[Msg::readcount + 3];
    Msg::readcount += 4;

    return dat.f;
}

const char* Msg::ReadString(const sizebuf_t& buf)
{
    static char string[2048];
    int l = 0;
    do
    {
        int c = Msg::ReadChar(buf);
        if (c == -1 || c == 0)
            break;
        string[l] = c;
        l++;
    } while (l < sizeof(string) - 1);

    string[l] = 0;

    return string;
}

float Msg::ReadCoord(const sizebuf_t& buf)
{
    return Msg::ReadShort(buf) * (1.0 / 8);
}

float Msg::ReadAngle(const sizebuf_t& buf)
{
    return Msg::ReadChar(buf) * (360.0 / 256);
}

//===========================================================================

void sizebuf_t::SZ_Alloc(int startsize)
{
    startsize = std::max(startsize, 256);
    data = (byte*)Hunk_AllocName(startsize, (char*)"sizebuf");
    maxsize = startsize;
    cursize = 0;
}


void sizebuf_t::SZ_Free()
{
    cursize = 0;
}

void sizebuf_t::SZ_Clear()
{
    cursize = 0;
}

void* sizebuf_t::SZ_GetSpace(int length)
{
    if (cursize + length > maxsize)
    {
        if (!allowoverflow_)
            Sys_Error((char*)"SZ_GetSpace: overflow without allowoverflow set");

        if (length > maxsize)
            Sys_Error((char*)"SZ_GetSpace: %i is > full buffer size", length);

        overflowed_ = true;
        Con_Printf("SZ_GetSpace: overflow");
        SZ_Clear();
    }

    void* data1 = data + cursize;
    cursize += length;

    return data1;
}

void sizebuf_t::SZ_Write(const void* data, int length)
{
    std::memcpy(SZ_GetSpace(length), data, length);
}

void sizebuf_t::SZ_Print(const char* data)
{
    int len = std::strlen(data) + 1;

    // byte * cast to keep VC++ happy
    if (data[cursize - 1])
        std::memcpy((byte*)SZ_GetSpace(len), data, len); // no trailing 0
    else
        std::memcpy((byte*)SZ_GetSpace(len - 1) - 1, data, len); // write over trailing 0
}


//============================================================================


/*
============
COM_SkipPath
============
*/
char * COM_SkipPath(char * pathname)
{
    char * last;

    last = pathname;
    while (*pathname)
    {
        if (*pathname == '/')
            last = pathname + 1;
        pathname++;
    }
    return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension(char * in, char * out)
{
    while (*in && *in != '.')
        *out++ = *in++;
    *out = 0;
}

/*
============
COM_FileExtension
============
*/
char * COM_FileExtension(char * in)
{
    static char exten[8];
    int i;

    while (*in && *in != '.')
        in++;
    if (!*in)
        return (char*)"";
    in++;
    for (i = 0; i < 7 && *in; i++, in++)
        exten[i] = *in;
    exten[i] = 0;
    return exten;
}

/*
============
COM_FileBase
============
*/
void COM_FileBase(char * in, char * out)
{
    char * s, * s2;

    s = in + strlen(in) - 1;

    while (s != in && *s != '.')
        s--;

    for (s2 = s; *s2 && *s2 != '/'; s2--)
        ;

    if (s - s2 < 2)
        strcpy(out, "?model?");
    else
    {
        s--;
        strncpy(out, s2 + 1, s - s2);
        out[s - s2] = 0;
    }
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension(char * path, char * extension)
{
    char * src;
    //
    // if path doesn't have a .EXT, append extension
    // (extension should include the .)
    //
    src = path + strlen(path) - 1;

    while (*src != '/' && src != path)
    {
        if (*src == '.')
            return; // it has an extension
        src--;
    }

    strcat(path, extension);
}


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
const char * COM_Parse(const char * data)
{
    int c;
    int len;

    len = 0;
    com_token[0] = 0;

    if (!data)
        return NULL;

    // skip whitespace
skipwhite:
    while ((c = *data) <= ' ')
    {
        if (c == 0)
            return NULL; // end of file;
        data++;
    }

    // skip // comments
    if (c == '/' && data[1] == '/')
    {
        while (*data && *data != '\n')
            data++;
        goto skipwhite;
    }


    // handle quoted strings specially
    if (c == '\"')
    {
        data++;
        while (1)
        {
            c = *data++;
            if (c == '\"' || !c)
            {
                com_token[len] = 0;
                return data;
            }
            com_token[len] = c;
            len++;
        }
    }

    // parse single characters
    if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ':')
    {
        com_token[len] = c;
        len++;
        com_token[len] = 0;
        return data + 1;
    }

    // parse a regular word
    do
    {
        com_token[len] = c;
        data++;
        len++;
        c = *data;
        if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ':')
            break;
    } while (c > 32);

    com_token[len] = 0;
    return data;
}


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParm(char * parm)
{
    int i;

    for (i = 1; i < com_argc; i++)
    {
        if (!com_argv[i])
            continue; // NEXTSTEP sometimes clears appkit vars.
        if (!std::strcmp(parm, com_argv[i]))
            return i;
    }

    return 0;
}

short ShortSwap(short l)
{
    byte b1, b2;

    b1 = l & 255;
    b2 = (l >> 8) & 255;

    return (b1 << 8) + b2;
}

/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
void COM_CheckRegistered()
{
    Cvar_Set((char*)"cmdline", com_cmdline);
    Cvar_Set((char*)"registered", (char*)"1");
    static_registered = 1;
    Con_Printf("Playing registered version.\n");
}


void COM_Path_f();


/*
================
COM_InitArgv
================
*/
void COM_InitArgv(int argc, char ** argv)
{
    bool safe;
    int i, j, n;

    // reconstitute the command line for the cmdline externally visible cvar
    n = 0;

    for (j = 0; (j < MAX_NUM_ARGVS) && (j < argc); j++)
    {
        i = 0;

        while ((n < (CMDLINE_LENGTH - 1)) && argv[j][i])
        {
            com_cmdline[n++] = argv[j][i++];
        }

        if (n < (CMDLINE_LENGTH - 1))
            com_cmdline[n++] = ' ';
        else
            break;
    }

    com_cmdline[n] = 0;

    safe = false;

    for (com_argc = 0; (com_argc < MAX_NUM_ARGVS) && (com_argc < argc);
         com_argc++)
    {
        largv[com_argc] = argv[com_argc];
        if (!std::strcmp("-safe", argv[com_argc]))
            safe = true;
    }

    if (safe)
    {
        // force all the safe-mode switches. Note that we reserved extra space in
        // case we need to add these, so we don't need an overflow check
        for (i = 0; i < NUM_SAFE_ARGVS; i++)
        {
            largv[com_argc] = safeargvs[i];
            com_argc++;
        }
    }

    largv[com_argc] = argvdummy;
    com_argv = largv;
}


/*
================
COM_Init
================
*/
void COM_Init(char * basedir)
{
    Cvar_RegisterVariable(&registered);
    Cvar_RegisterVariable(&cmdline);
    Cmd_AddCommand((char*)"path", COM_Path_f);

    COM_InitFilesystem();
    COM_CheckRegistered();
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char * va(const char * format, ...)
{
    va_list argptr;
    static char string[1024];

    va_start(argptr, format);
    vsprintf(string, format, argptr);
    va_end(argptr);

    return string;
}


/// just for debugging
int memsearch(byte * start, int count, int search)
{
    int i;

    for (i = 0; i < count; i++)
        if (start[i] == search)
            return i;
    return -1;
}

/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

int com_filesize;


//
// in memory
//

typedef struct
{
    char name[MAX_QPATH];
    int filepos, filelen;
} packfile_t;

typedef struct pack_s
{
    char filename[MAX_OSPATH];
    int handle;
    int numfiles;
    packfile_t * files;
} pack_t;

//
// on disk
//
typedef struct
{
    char name[56];
    int filepos, filelen;
} dpackfile_t;

typedef struct
{
    char id[4];
    int dirofs;
    int dirlen;
} dpackheader_t;

#define MAX_FILES_IN_PACK 2048

char com_cachedir[MAX_OSPATH];
char com_gamedir[MAX_OSPATH];

typedef struct searchpath_s
{
    char filename[MAX_OSPATH];
    pack_t * pack; // only one of filename / pack will be used
    struct searchpath_s * next;
} searchpath_t;

searchpath_t * com_searchpaths;

/*
============
COM_Path_f

============
*/
void COM_Path_f()
{
    searchpath_t * s;

    Con_Printf("Current search path:\n");
    for (s = com_searchpaths; s; s = s->next)
    {
        if (s->pack)
        {
            Con_Printf("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
        }
        else
            Con_Printf("%s\n", s->filename);
    }
}

/*
============
COM_WriteFile

The filename will be prefixed by the current game directory
============
*/
void COM_WriteFile(char * filename, void * data, int len)
{
    int handle;
    char name[MAX_OSPATH];

    sprintf(name, "%s/%s", com_gamedir, filename);

    handle = Sys_FileOpenWrite(name);
    if (handle == -1)
    {
        Sys_Printf((char*)"COM_WriteFile: failed on %s\n", name);
        return;
    }

    Sys_Printf((char*)"COM_WriteFile: %s\n", name);
    Sys_FileWrite(handle, data, len);
    Sys_FileClose(handle);
}


/*
============
COM_CreatePath

Only used for CopyFile
============
*/
void COM_CreatePath(char * path)
{
    char * ofs;

    for (ofs = path + 1; *ofs; ofs++)
    {
        if (*ofs == '/')
        { // create the directory
            *ofs = 0;
            Sys_mkdir(path);
            *ofs = '/';
        }
    }
}


/*
===========
COM_CopyFile

Copies a file over from the net to the local cache, creating any directories
needed. This is for the convenience of developers using ISDN from home.
===========
*/
void COM_CopyFile(char * netpath, char * cachepath)
{
    int in, out;
    int remaining, count;
    char buf[4096];

    remaining = Sys_FileOpenRead(netpath, &in);
    COM_CreatePath(cachepath); // create directories up to the cache file
    out = Sys_FileOpenWrite(cachepath);

    while (remaining)
    {
        if (remaining < sizeof(buf))
            count = remaining;
        else
            count = sizeof(buf);
        Sys_FileRead(in, buf, count);
        Sys_FileWrite(out, buf, count);
        remaining -= count;
    }

    Sys_FileClose(in);
    Sys_FileClose(out);
}

/*
===========
COM_FindFile

Finds the file in the search path.
Sets com_filesize and one of handle or file
===========
*/
int COM_FindFile(char * filename, int * handle, FILE ** file)
{
    searchpath_t * search;
    char netpath[MAX_OSPATH];
    char cachepath[MAX_OSPATH];
    pack_t * pak;
    int i;
    int findtime, cachetime;

    if (file && handle)
        Sys_Error((char*)"COM_FindFile: both handle and file set");
    if (!file && !handle)
        Sys_Error((char*)"COM_FindFile: neither handle or file set");

    //
    // search through the path, one element at a time
    //
    search = com_searchpaths;
    
    for (; search; search = search->next)
    {
        // is the element a pak file?
        if (search->pack)
        {
            // look through all the pak file elements
            pak = search->pack;
            for (i = 0; i < pak->numfiles; i++)
                if (!strcmp(pak->files[i].name, filename))
                { // found it!
                    Sys_Printf((char*)"PackFile: %s : %s\n", pak->filename, filename);
                    if (handle)
                    {
                        *handle = pak->handle;
                        Sys_FileSeek(pak->handle, pak->files[i].filepos);
                    }
                    else
                    { // open a new file on the pakfile
                        *file = fopen(pak->filename, "rb");
                        if (*file)
                            fseek(*file, pak->files[i].filepos, SEEK_SET);
                    }
                    com_filesize = pak->files[i].filelen;
                    return com_filesize;
                }
        }
        else
        {
            // check a file in the directory tree
            if (!static_registered)
            { // if not a registered version, don't ever go beyond base
                if (strchr(filename, '/') || strchr(filename, '\\'))
                    continue;
            }

            sprintf(netpath, "%s/%s", search->filename, filename);

            findtime = Sys_FileTime(netpath);
            if (findtime == -1)
                continue;

            // see if the file needs to be updated in the cache
            if (!com_cachedir[0])
                strcpy(cachepath, netpath);
            else
            {
                if ((strlen(netpath) < 2) || (netpath[1] != ':'))
                    sprintf(cachepath, "%s%s", com_cachedir, netpath);
                else
                    sprintf(cachepath, "%s%s", com_cachedir, netpath + 2);

                cachetime = Sys_FileTime(cachepath);

                if (cachetime < findtime)
                    COM_CopyFile(netpath, cachepath);
                strcpy(netpath, cachepath);
            }

            Sys_Printf((char*)"FindFile: %s\n", netpath);
            com_filesize = Sys_FileOpenRead(netpath, &i);
            if (handle)
                *handle = i;
            else
            {
                Sys_FileClose(i);
                *file = fopen(netpath, "rb");
            }
            return com_filesize;
        }

    }

    Sys_Printf((char*)"FindFile: can't find %s\n", filename);

    if (handle)
        *handle = -1;
    else
        *file = NULL;
    com_filesize = -1;
    return -1;
}


/*
===========
COM_OpenFile

filename never has a leading slash, but may contain directory walks
returns a handle and a length
it may actually be inside a pak file
===========
*/
int COM_OpenFile(char * filename, int * handle)
{
    return COM_FindFile(filename, handle, NULL);
}

/*
===========
COM_FOpenFile

If the requested file is inside a packfile, a new FILE * will be opened
into the file.
===========
*/
int COM_FOpenFile(char * filename, FILE ** file)
{
    return COM_FindFile(filename, NULL, file);
}

/*
============
COM_CloseFile

If it is a pak file handle, don't really close it
============
*/
void COM_CloseFile(int h)
{
    searchpath_t * s;

    for (s = com_searchpaths; s; s = s->next)
        if (s->pack && s->pack->handle == h)
            return;

    Sys_FileClose(h);
}


/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Allways appends a 0 byte.
============
*/
cache_user_t * loadcache;
byte * loadbuf;
int loadsize;
byte * COM_LoadFile(char * path, int usehunk)
{
    int h;
    byte * buf;
    char base[32];
    int len;

    buf = NULL; // quiet compiler warning

    // look for it in the filesystem or pack files
    len = COM_OpenFile(path, &h);
    if (h == -1)
        return NULL;

    // extract the filename base name for hunk tag
    COM_FileBase(path, base);

    if (usehunk == 1)
        buf = (byte*)Hunk_AllocName(len + 1, base);
    else if (usehunk == 2)
        buf = (byte*)Hunk_TempAlloc(len + 1);
    else if (usehunk == 0)
        buf = (byte*)Z_Malloc(len + 1);
    else if (usehunk == 4)
    {
        if (len + 1 > loadsize)
            buf = (byte*)Hunk_TempAlloc(len + 1);
        else
            buf = loadbuf;
    }
    else
        Sys_Error((char*)"COM_LoadFile: bad usehunk");

    if (!buf)
        Sys_Error((char*)"COM_LoadFile: not enough space for %s", path);

    ((byte *)buf)[len] = 0;

    Draw_BeginDisc();
    Sys_FileRead(h, buf, len);
    COM_CloseFile(h);
    Draw_EndDisc();

    return buf;
}

byte * COM_LoadHunkFile(char * path)
{
    return COM_LoadFile(path, 1);
}

byte * COM_LoadTempFile(char * path)
{
    return COM_LoadFile(path, 2);
}

// uses temp hunk if larger than bufsize
byte * COM_LoadStackFile(char * path, void * buffer, int bufsize)
{
    byte * buf;

    loadbuf = (byte *)buffer;
    loadsize = bufsize;
    buf = COM_LoadFile(path, 4);

    return buf;
}

/*
=================
COM_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/
pack_t * COM_LoadPackFile(char * packfile)
{
    dpackheader_t header;
    int i;
    packfile_t * newfiles;
    int numpackfiles;
    pack_t * pack;
    int packhandle;
    dpackfile_t info[MAX_FILES_IN_PACK];

    if (Sys_FileOpenRead(packfile, &packhandle) == -1)
    {
        // Con_Printf ("Couldn't open %s\n", packfile);
        return NULL;
    }
    Sys_FileRead(packhandle, (void *)&header, sizeof(header));
    if (header.id[0] != 'P' || header.id[1] != 'A'
        || header.id[2] != 'C' || header.id[3] != 'K')
        Sys_Error((char*)"%s is not a packfile", packfile);
    header.dirofs = header.dirofs;
    header.dirlen = header.dirlen;

    numpackfiles = header.dirlen / sizeof(dpackfile_t);

    if (numpackfiles > MAX_FILES_IN_PACK)
        Sys_Error((char*)"%s has %i files", packfile, numpackfiles);

    newfiles = (packfile_t*)Hunk_AllocName(numpackfiles * sizeof(packfile_t), (char*)"packfile");

    Sys_FileSeek(packhandle, header.dirofs);
    Sys_FileRead(packhandle, (void *)info, header.dirlen);

    // parse the directory
    for (i = 0; i < numpackfiles; i++)
    {
        strcpy(newfiles[i].name, info[i].name);
        newfiles[i].filepos = info[i].filepos;
        newfiles[i].filelen = info[i].filelen;
    }

    pack = (pack_t*)Hunk_Alloc(sizeof(pack_t));
    strcpy(pack->filename, packfile);
    pack->handle = packhandle;
    pack->numfiles = numpackfiles;
    pack->files = newfiles;

    Con_Printf("Added packfile %s (%i files)\n", packfile, numpackfiles);
    return pack;
}


/*
================
COM_AddGameDirectory

Sets com_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ...
================
*/
void COM_AddGameDirectory(char * dir)
{
    int i;
    searchpath_t * search;
    pack_t * pak;
    char pakfile[MAX_OSPATH];

    strcpy(com_gamedir, dir);

    //
    // add the directory to the search path
    //
    search = (searchpath_t*)Hunk_Alloc(sizeof(searchpath_t));
    strcpy(search->filename, dir);
    search->next = com_searchpaths;
    com_searchpaths = search;

    //
    // add any pak files in the format pak0.pak pak1.pak, ...
    //
    for (i = 0; ; i++)
    {
        sprintf(pakfile, "%s/pak%i.pak", dir, i);
        pak = COM_LoadPackFile(pakfile);
        if (!pak)
            break;
        search = (searchpath_t*)Hunk_Alloc(sizeof(searchpath_t));
        search->pack = pak;
        search->next = com_searchpaths;
        com_searchpaths = search;
    }

    //
    // add the contents of the parms.txt file to the end of the command line
    //

}

/*
================
COM_InitFilesystem
================
*/
void COM_InitFilesystem()
{
    int i, j;
    char basedir[MAX_OSPATH];
    searchpath_t * search;

    //
    // -basedir <path>
    // Overrides the system supplied base directory (under GAMENAME)
    //
    i = COM_CheckParm((char*)"-basedir");
    if (i && i < com_argc - 1)
        strcpy(basedir, com_argv[i + 1]);
    else
        strcpy(basedir, host_parms.basedir);

    j = strlen(basedir);

    if (j > 0)
    {
        if ((basedir[j - 1] == '\\') || (basedir[j - 1] == '/'))
            basedir[j - 1] = 0;
    }

    //
    // -cachedir <path>
    // Overrides the system supplied cache directory (NULL or /qcache)
    // -cachedir - will disable caching.
    //
    i = COM_CheckParm((char*)"-cachedir");
    if (i && i < com_argc - 1)
    {
        if (com_argv[i + 1][0] == '-')
            com_cachedir[0] = 0;
        else
            strcpy(com_cachedir, com_argv[i + 1]);
    }
    else if (host_parms.cachedir)
        strcpy(com_cachedir, host_parms.cachedir);
    else
        com_cachedir[0] = 0;

    //
    // start up with GAMENAME by default (id1)
    //
    COM_AddGameDirectory(va("%s/" GAMENAME, basedir));

    if constexpr (rogue)
        COM_AddGameDirectory(va("%s/rogue", basedir));
    if constexpr (hipnotic)
        COM_AddGameDirectory(va("%s/hipnotic", basedir));

    //
    // -game <gamedir>
    // Adds basedir/gamedir as an override game
    //
    i = COM_CheckParm((char*)"-game");
    if (i && i < com_argc - 1)
    {
        COM_AddGameDirectory(va("%s/%s", basedir, com_argv[i + 1]));
    }

    //
    // -path <dir or packfile> [<dir or packfile>] ...
    // Fully specifies the exact serach path, overriding the generated one
    //
    i = COM_CheckParm((char*)"-path");
    if (i)
    {
        com_searchpaths = NULL;
        while (++i < com_argc)
        {
            if (!com_argv[i] || com_argv[i][0] == '+' || com_argv[i][0] == '-')
                break;

            search = (searchpath_t*)Hunk_Alloc(sizeof(searchpath_t));
            if (!strcmp(COM_FileExtension(com_argv[i]), "pak"))
            {
                search->pack = COM_LoadPackFile(com_argv[i]);
                if (!search->pack)
                    Sys_Error((char*)"Couldn't load packfile: %s", com_argv[i]);
            }
            else
                strcpy(search->filename, com_argv[i]);
            search->next = com_searchpaths;
            com_searchpaths = search;
        }
    }
}


