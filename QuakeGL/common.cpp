
// common.c -- misc functions used in client and server

#include "quakedef.h"
#include <common/pak.h>
#include <string_view>
#include <map>

#undef max
#define NUM_SAFE_ARGVS 7

static char * largv[MAX_NUM_ARGVS + NUM_SAFE_ARGVS + 1];
static char * argvdummy = (char*)" ";

static char * safeargvs[NUM_SAFE_ARGVS] =
{(char*)"-stdvid", (char*)"-nolan", (char*)"-nosound", (char*)"-nocdaudio", (char*)"-nojoy", (char*)"-nomouse", (char*)"-dibonly"};

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
    byte * buf;

    buf = (byte*)SZ_GetSpace(1);
    buf[0] = c;
}

void sizebuf_t::MSG_WriteByte(int c)
{
    byte * buf;

    buf = (byte*)SZ_GetSpace(1);
    buf[0] = c;
}

void sizebuf_t::MSG_WriteShort(int c)
{
    byte * buf;

    buf = (byte*)SZ_GetSpace(2);
    buf[0] = c & 0xff;
    buf[1] = c >> 8;
}

void sizebuf_t::MSG_WriteLong(int c)
{
    byte * buf;

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

void sizebuf_t::MSG_WriteString(const char * s)
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

void * sizebuf_t::SZ_GetSpace(int length)
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

void sizebuf_t::SZ_Write(const void * data, int length)
{
    std::memcpy(SZ_GetSpace(length), data, length);
}

void sizebuf_t::SZ_Print(const char * data)
{
    int len = std::strlen(data) + 1;

    // byte * cast to keep VC++ happy
    if (data[cursize - 1])
        std::memcpy((byte *)SZ_GetSpace(len), data, len); // no trailing 0
    else
        std::memcpy((byte *)SZ_GetSpace(len - 1) - 1, data, len); // write over trailing 0
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

char com_cachedir[MAX_OSPATH];
char com_gamedir[MAX_OSPATH];

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

std::span<unsigned char> COM_LoadFile(const char * path)
{
    using namespace std::string_view_literals;
    const static std::map<std::string_view, std::span<unsigned char>> m = {
    { "gfx.wad"sv, pak::gfx_wad() },
    { "sound/ambience/buzz1.wav"sv, pak::sound::ambience::buzz1_wav() },
    { "sound/ambience/comp1.wav"sv, pak::sound::ambience::comp1_wav() },
    { "sound/ambience/drip1.wav"sv, pak::sound::ambience::drip1_wav() },
    { "sound/ambience/drone6.wav"sv, pak::sound::ambience::drone6_wav() },
    { "sound/ambience/fire1.wav"sv, pak::sound::ambience::fire1_wav() },
    { "sound/ambience/fl_hum1.wav"sv, pak::sound::ambience::fl_hum1_wav() },
    { "sound/ambience/hum1.wav"sv, pak::sound::ambience::hum1_wav() },
    { "sound/ambience/suck1.wav"sv, pak::sound::ambience::suck1_wav() },
    { "sound/ambience/swamp1.wav"sv, pak::sound::ambience::swamp1_wav() },
    { "sound/ambience/swamp2.wav"sv, pak::sound::ambience::swamp2_wav() },
    { "sound/ambience/thunder1.wav"sv, pak::sound::ambience::thunder1_wav() },
    { "sound/ambience/water1.wav"sv, pak::sound::ambience::water1_wav() },
    { "sound/ambience/wind2.wav"sv, pak::sound::ambience::wind2_wav() },
    { "sound/ambience/windfly.wav"sv, pak::sound::ambience::windfly_wav() },
    { "sound/boss1/death.wav"sv, pak::sound::boss1::death_wav() },
    { "sound/boss1/out1.wav"sv, pak::sound::boss1::out1_wav() },
    { "sound/boss1/pain.wav"sv, pak::sound::boss1::pain_wav() },
    { "sound/boss1/sight1.wav"sv, pak::sound::boss1::sight1_wav() },
    { "sound/boss1/throw.wav"sv, pak::sound::boss1::throw_wav() },
    { "sound/buttons/airbut1.wav"sv, pak::sound::buttons::airbut1_wav() },
    { "sound/buttons/switch02.wav"sv, pak::sound::buttons::switch02_wav() },
    { "sound/buttons/switch04.wav"sv, pak::sound::buttons::switch04_wav() },
    { "sound/buttons/switch21.wav"sv, pak::sound::buttons::switch21_wav() },
    { "sound/demon/ddeath.wav"sv, pak::sound::demon::ddeath_wav() },
    { "sound/demon/dhit2.wav"sv, pak::sound::demon::dhit2_wav() },
    { "sound/demon/djump.wav"sv, pak::sound::demon::djump_wav() },
    { "sound/demon/dland2.wav"sv, pak::sound::demon::dland2_wav() },
    { "sound/demon/dpain1.wav"sv, pak::sound::demon::dpain1_wav() },
    { "sound/demon/idle1.wav"sv, pak::sound::demon::idle1_wav() },
    { "sound/demon/sight2.wav"sv, pak::sound::demon::sight2_wav() },
    { "sound/dog/dattack1.wav"sv, pak::sound::dog::dattack1_wav() },
    { "sound/dog/ddeath.wav"sv, pak::sound::dog::ddeath_wav() },
    { "sound/dog/dpain1.wav"sv, pak::sound::dog::dpain1_wav() },
    { "sound/dog/dsight.wav"sv, pak::sound::dog::dsight_wav() },
    { "sound/dog/idle.wav"sv, pak::sound::dog::idle_wav() },
    { "sound/doors/airdoor1.wav"sv, pak::sound::doors::airdoor1_wav() },
    { "sound/doors/airdoor2.wav"sv, pak::sound::doors::airdoor2_wav() },
    { "sound/doors/basesec1.wav"sv, pak::sound::doors::basesec1_wav() },
    { "sound/doors/basesec2.wav"sv, pak::sound::doors::basesec2_wav() },
    { "sound/doors/basetry.wav"sv, pak::sound::doors::basetry_wav() },
    { "sound/doors/baseuse.wav"sv, pak::sound::doors::baseuse_wav() },
    { "sound/doors/ddoor1.wav"sv, pak::sound::doors::ddoor1_wav() },
    { "sound/doors/ddoor2.wav"sv, pak::sound::doors::ddoor2_wav() },
    { "sound/doors/doormv1.wav"sv, pak::sound::doors::doormv1_wav() },
    { "sound/doors/drclos4.wav"sv, pak::sound::doors::drclos4_wav() },
    { "sound/doors/hydro1.wav"sv, pak::sound::doors::hydro1_wav() },
    { "sound/doors/hydro2.wav"sv, pak::sound::doors::hydro2_wav() },
    { "sound/doors/latch2.wav"sv, pak::sound::doors::latch2_wav() },
    { "sound/doors/medtry.wav"sv, pak::sound::doors::medtry_wav() },
    { "sound/doors/meduse.wav"sv, pak::sound::doors::meduse_wav() },
    { "sound/doors/runetry.wav"sv, pak::sound::doors::runetry_wav() },
    { "sound/doors/runeuse.wav"sv, pak::sound::doors::runeuse_wav() },
    { "sound/doors/stndr1.wav"sv, pak::sound::doors::stndr1_wav() },
    { "sound/doors/stndr2.wav"sv, pak::sound::doors::stndr2_wav() },
    { "sound/doors/winch2.wav"sv, pak::sound::doors::winch2_wav() },
    { "sound/hknight/hit.wav"sv, pak::sound::hknight::hit_wav() },
    { "sound/items/armor1.wav"sv, pak::sound::items::armor1_wav() },
    { "sound/items/damage.wav"sv, pak::sound::items::damage_wav() },
    { "sound/items/damage2.wav"sv, pak::sound::items::damage2_wav() },
    { "sound/items/damage3.wav"sv, pak::sound::items::damage3_wav() },
    { "sound/items/health1.wav"sv, pak::sound::items::health1_wav() },
    { "sound/items/inv1.wav"sv, pak::sound::items::inv1_wav() },
    { "sound/items/inv2.wav"sv, pak::sound::items::inv2_wav() },
    { "sound/items/inv3.wav"sv, pak::sound::items::inv3_wav() },
    { "sound/items/itembk2.wav"sv, pak::sound::items::itembk2_wav() },
    { "sound/items/protect.wav"sv, pak::sound::items::protect_wav() },
    { "sound/items/protect2.wav"sv, pak::sound::items::protect2_wav() },
    { "sound/items/protect3.wav"sv, pak::sound::items::protect3_wav() },
    { "sound/items/r_item1.wav"sv, pak::sound::items::r_item1_wav() },
    { "sound/items/r_item2.wav"sv, pak::sound::items::r_item2_wav() },
    { "sound/items/suit.wav"sv, pak::sound::items::suit_wav() },
    { "sound/items/suit2.wav"sv, pak::sound::items::suit2_wav() },
    { "sound/knight/idle.wav"sv, pak::sound::knight::idle_wav() },
    { "sound/knight/kdeath.wav"sv, pak::sound::knight::kdeath_wav() },
    { "sound/knight/khurt.wav"sv, pak::sound::knight::khurt_wav() },
    { "sound/knight/ksight.wav"sv, pak::sound::knight::ksight_wav() },
    { "sound/knight/sword1.wav"sv, pak::sound::knight::sword1_wav() },
    { "sound/knight/sword2.wav"sv, pak::sound::knight::sword2_wav() },
    { "sound/misc/h2ohit1.wav"sv, pak::sound::misc::h2ohit1_wav() },
    { "sound/misc/medkey.wav"sv, pak::sound::misc::medkey_wav() },
    { "sound/misc/menu1.wav"sv, pak::sound::misc::menu1_wav() },
    { "sound/misc/menu2.wav"sv, pak::sound::misc::menu2_wav() },
    { "sound/misc/menu3.wav"sv, pak::sound::misc::menu3_wav() },
    { "sound/misc/null.wav"sv, pak::sound::misc::null_wav() },
    { "sound/misc/outwater.wav"sv, pak::sound::misc::outwater_wav() },
    { "sound/misc/power.wav"sv, pak::sound::misc::power_wav() },
    { "sound/misc/r_tele1.wav"sv, pak::sound::misc::r_tele1_wav() },
    { "sound/misc/r_tele2.wav"sv, pak::sound::misc::r_tele2_wav() },
    { "sound/misc/r_tele3.wav"sv, pak::sound::misc::r_tele3_wav() },
    { "sound/misc/r_tele4.wav"sv, pak::sound::misc::r_tele4_wav() },
    { "sound/misc/r_tele5.wav"sv, pak::sound::misc::r_tele5_wav() },
    { "sound/misc/runekey.wav"sv, pak::sound::misc::runekey_wav() },
    { "sound/misc/secret.wav"sv, pak::sound::misc::secret_wav() },
    { "sound/misc/talk.wav"sv, pak::sound::misc::talk_wav() },
    { "sound/misc/trigger1.wav"sv, pak::sound::misc::trigger1_wav() },
    { "sound/misc/water1.wav"sv, pak::sound::misc::water1_wav() },
    { "sound/misc/water2.wav"sv, pak::sound::misc::water2_wav() },
    { "sound/ogre/ogdrag.wav"sv, pak::sound::ogre::ogdrag_wav() },
    { "sound/ogre/ogdth.wav"sv, pak::sound::ogre::ogdth_wav() },
    { "sound/ogre/ogidle.wav"sv, pak::sound::ogre::ogidle_wav() },
    { "sound/ogre/ogidle2.wav"sv, pak::sound::ogre::ogidle2_wav() },
    { "sound/ogre/ogpain1.wav"sv, pak::sound::ogre::ogpain1_wav() },
    { "sound/ogre/ogsawatk.wav"sv, pak::sound::ogre::ogsawatk_wav() },
    { "sound/ogre/ogwake.wav"sv, pak::sound::ogre::ogwake_wav() },
    { "sound/plats/medplat1.wav"sv, pak::sound::plats::medplat1_wav() },
    { "sound/plats/medplat2.wav"sv, pak::sound::plats::medplat2_wav() },
    { "sound/plats/plat1.wav"sv, pak::sound::plats::plat1_wav() },
    { "sound/plats/plat2.wav"sv, pak::sound::plats::plat2_wav() },
    { "sound/plats/train1.wav"sv, pak::sound::plats::train1_wav() },
    { "sound/plats/train2.wav"sv, pak::sound::plats::train2_wav() },
    { "sound/player/axhit1.wav"sv, pak::sound::player::axhit1_wav() },
    { "sound/player/axhit2.wav"sv, pak::sound::player::axhit2_wav() },
    { "sound/player/death1.wav"sv, pak::sound::player::death1_wav() },
    { "sound/player/death2.wav"sv, pak::sound::player::death2_wav() },
    { "sound/player/death3.wav"sv, pak::sound::player::death3_wav() },
    { "sound/player/death4.wav"sv, pak::sound::player::death4_wav() },
    { "sound/player/death5.wav"sv, pak::sound::player::death5_wav() },
    { "sound/player/drown1.wav"sv, pak::sound::player::drown1_wav() },
    { "sound/player/drown2.wav"sv, pak::sound::player::drown2_wav() },
    { "sound/player/gasp1.wav"sv, pak::sound::player::gasp1_wav() },
    { "sound/player/gasp2.wav"sv, pak::sound::player::gasp2_wav() },
    { "sound/player/gib.wav"sv, pak::sound::player::gib_wav() },
    { "sound/player/h2odeath.wav"sv, pak::sound::player::h2odeath_wav() },
    { "sound/player/h2ojump.wav"sv, pak::sound::player::h2ojump_wav() },
    { "sound/player/inh2o.wav"sv, pak::sound::player::inh2o_wav() },
    { "sound/player/inlava.wav"sv, pak::sound::player::inlava_wav() },
    { "sound/player/land.wav"sv, pak::sound::player::land_wav() },
    { "sound/player/land2.wav"sv, pak::sound::player::land2_wav() },
    { "sound/player/lburn1.wav"sv, pak::sound::player::lburn1_wav() },
    { "sound/player/lburn2.wav"sv, pak::sound::player::lburn2_wav() },
    { "sound/player/pain1.wav"sv, pak::sound::player::pain1_wav() },
    { "sound/player/pain2.wav"sv, pak::sound::player::pain2_wav() },
    { "sound/player/pain3.wav"sv, pak::sound::player::pain3_wav() },
    { "sound/player/pain4.wav"sv, pak::sound::player::pain4_wav() },
    { "sound/player/pain5.wav"sv, pak::sound::player::pain5_wav() },
    { "sound/player/pain6.wav"sv, pak::sound::player::pain6_wav() },
    { "sound/player/plyrjmp8.wav"sv, pak::sound::player::plyrjmp8_wav() },
    { "sound/player/slimbrn2.wav"sv, pak::sound::player::slimbrn2_wav() },
    { "sound/player/teledth1.wav"sv, pak::sound::player::teledth1_wav() },
    { "sound/player/tornoff2.wav"sv, pak::sound::player::tornoff2_wav() },
    { "sound/player/udeath.wav"sv, pak::sound::player::udeath_wav() },
    { "sound/shambler/melee1.wav"sv, pak::sound::shambler::melee1_wav() },
    { "sound/shambler/melee2.wav"sv, pak::sound::shambler::melee2_wav() },
    { "sound/shambler/sattck1.wav"sv, pak::sound::shambler::sattck1_wav() },
    { "sound/shambler/sboom.wav"sv, pak::sound::shambler::sboom_wav() },
    { "sound/shambler/sdeath.wav"sv, pak::sound::shambler::sdeath_wav() },
    { "sound/shambler/shurt2.wav"sv, pak::sound::shambler::shurt2_wav() },
    { "sound/shambler/sidle.wav"sv, pak::sound::shambler::sidle_wav() },
    { "sound/shambler/smack.wav"sv, pak::sound::shambler::smack_wav() },
    { "sound/shambler/ssight.wav"sv, pak::sound::shambler::ssight_wav() },
    { "sound/soldier/death1.wav"sv, pak::sound::soldier::death1_wav() },
    { "sound/soldier/idle.wav"sv, pak::sound::soldier::idle_wav() },
    { "sound/soldier/pain1.wav"sv, pak::sound::soldier::pain1_wav() },
    { "sound/soldier/pain2.wav"sv, pak::sound::soldier::pain2_wav() },
    { "sound/soldier/sattck1.wav"sv, pak::sound::soldier::sattck1_wav() },
    { "sound/soldier/sight1.wav"sv, pak::sound::soldier::sight1_wav() },
    { "sound/weapons/ax1.wav"sv, pak::sound::weapons::ax1_wav() },
    { "sound/weapons/bounce.wav"sv, pak::sound::weapons::bounce_wav() },
    { "sound/weapons/grenade.wav"sv, pak::sound::weapons::grenade_wav() },
    { "sound/weapons/guncock.wav"sv, pak::sound::weapons::guncock_wav() },
    { "sound/weapons/lhit.wav"sv, pak::sound::weapons::lhit_wav() },
    { "sound/weapons/lock4.wav"sv, pak::sound::weapons::lock4_wav() },
    { "sound/weapons/lstart.wav"sv, pak::sound::weapons::lstart_wav() },
    { "sound/weapons/pkup.wav"sv, pak::sound::weapons::pkup_wav() },
    { "sound/weapons/r_exp3.wav"sv, pak::sound::weapons::r_exp3_wav() },
    { "sound/weapons/ric1.wav"sv, pak::sound::weapons::ric1_wav() },
    { "sound/weapons/ric2.wav"sv, pak::sound::weapons::ric2_wav() },
    { "sound/weapons/ric3.wav"sv, pak::sound::weapons::ric3_wav() },
    { "sound/weapons/rocket1i.wav"sv, pak::sound::weapons::rocket1i_wav() },
    { "sound/weapons/sgun1.wav"sv, pak::sound::weapons::sgun1_wav() },
    { "sound/weapons/shotgn2.wav"sv, pak::sound::weapons::shotgn2_wav() },
    { "sound/weapons/spike2.wav"sv, pak::sound::weapons::spike2_wav() },
    { "sound/weapons/tink1.wav"sv, pak::sound::weapons::tink1_wav() },
    { "sound/wizard/hit.wav"sv, pak::sound::wizard::hit_wav() },
    { "sound/wizard/wattack.wav"sv, pak::sound::wizard::wattack_wav() },
    { "sound/wizard/wdeath.wav"sv, pak::sound::wizard::wdeath_wav() },
    { "sound/wizard/widle1.wav"sv, pak::sound::wizard::widle1_wav() },
    { "sound/wizard/widle2.wav"sv, pak::sound::wizard::widle2_wav() },
    { "sound/wizard/wpain.wav"sv, pak::sound::wizard::wpain_wav() },
    { "sound/wizard/wsight.wav"sv, pak::sound::wizard::wsight_wav() },
    { "sound/zombie/idle_w2.wav"sv, pak::sound::zombie::idle_w2_wav() },
    { "sound/zombie/z_fall.wav"sv, pak::sound::zombie::z_fall_wav() },
    { "sound/zombie/z_gib.wav"sv, pak::sound::zombie::z_gib_wav() },
    { "sound/zombie/z_hit.wav"sv, pak::sound::zombie::z_hit_wav() },
    { "sound/zombie/z_idle.wav"sv, pak::sound::zombie::z_idle_wav() },
    { "sound/zombie/z_idle1.wav"sv, pak::sound::zombie::z_idle1_wav() },
    { "sound/zombie/z_miss.wav"sv, pak::sound::zombie::z_miss_wav() },
    { "sound/zombie/z_pain.wav"sv, pak::sound::zombie::z_pain_wav() },
    { "sound/zombie/z_pain1.wav"sv, pak::sound::zombie::z_pain1_wav() },
    { "sound/zombie/z_shot1.wav"sv, pak::sound::zombie::z_shot1_wav() },

    { "config.cfg"sv, pak::default_cfg() },
    { "autoexec.cfg"sv, pak::default_cfg() },

    { "default.cfg"sv, pak::default_cfg() },
    { "demo1.dem"sv, pak::demo1_dem() },
    { "demo2.dem"sv, pak::demo2_dem() },
    { "demo3.dem"sv, pak::demo3_dem() },
    { "end1.bin"sv, pak::end1_bin() },
    { "gfx.wad"sv, pak::gfx_wad() },
    { "progs.dat"sv, pak::progs_dat() },
    { "quake.rc"sv, pak::quake_rc() },

    { "maps/b_batt0.bsp"sv, pak::maps::b_batt0_bsp() },
    { "maps/b_batt1.bsp"sv, pak::maps::b_batt1_bsp() },
    { "maps/b_bh10.bsp"sv, pak::maps::b_bh10_bsp() },
    { "maps/b_bh100.bsp"sv, pak::maps::b_bh100_bsp() },
    { "maps/b_bh25.bsp"sv, pak::maps::b_bh25_bsp() },
    { "maps/b_explob.bsp"sv, pak::maps::b_explob_bsp() },
    { "maps/b_nail0.bsp"sv, pak::maps::b_nail0_bsp() },
    { "maps/b_nail1.bsp"sv, pak::maps::b_nail1_bsp() },
    { "maps/b_rock0.bsp"sv, pak::maps::b_rock0_bsp() },
    { "maps/b_rock1.bsp"sv, pak::maps::b_rock1_bsp() },
    { "maps/b_shell0.bsp"sv, pak::maps::b_shell0_bsp() },
    { "maps/b_shell1.bsp"sv, pak::maps::b_shell1_bsp() },
    { "maps/e1m1.bsp"sv, pak::maps::e1m1_bsp() },
    { "maps/e1m2.bsp"sv, pak::maps::e1m2_bsp() },
    { "maps/e1m3.bsp"sv, pak::maps::e1m3_bsp() },
    { "maps/e1m4.bsp"sv, pak::maps::e1m4_bsp() },
    { "maps/e1m5.bsp"sv, pak::maps::e1m5_bsp() },
    { "maps/e1m6.bsp"sv, pak::maps::e1m6_bsp() },
    { "maps/e1m7.bsp"sv, pak::maps::e1m7_bsp() },
    { "maps/e1m8.bsp"sv, pak::maps::e1m8_bsp() },
    { "maps/start.bsp"sv, pak::maps::start_bsp() },

    { "progs/armor.mdl"sv, pak::progs::armor_mdl() },
    { "progs/backpack.mdl"sv, pak::progs::backpack_mdl() },
    { "progs/bolt.mdl"sv, pak::progs::bolt_mdl() },
    { "progs/bolt2.mdl"sv, pak::progs::bolt2_mdl() },
    { "progs/bolt3.mdl"sv, pak::progs::bolt3_mdl() },
    { "progs/boss.mdl"sv, pak::progs::boss_mdl() },
    { "progs/demon.mdl"sv, pak::progs::demon_mdl() },
    { "progs/dog.mdl"sv, pak::progs::dog_mdl() },
    { "progs/end1.mdl"sv, pak::progs::end1_mdl() },
    { "progs/eyes.mdl"sv, pak::progs::eyes_mdl() },
    { "progs/flame.mdl"sv, pak::progs::flame_mdl() },
    { "progs/flame2.mdl"sv, pak::progs::flame2_mdl() },
    { "progs/g_light.mdl"sv, pak::progs::g_light_mdl() },
    { "progs/g_nail.mdl"sv, pak::progs::g_nail_mdl() },
    { "progs/g_nail2.mdl"sv, pak::progs::g_nail2_mdl() },
    { "progs/g_rock.mdl"sv, pak::progs::g_rock_mdl() },
    { "progs/g_rock2.mdl"sv, pak::progs::g_rock2_mdl() },
    { "progs/g_shot.mdl"sv, pak::progs::g_shot_mdl() },
    { "progs/gib1.mdl"sv, pak::progs::gib1_mdl() },
    { "progs/gib2.mdl"sv, pak::progs::gib2_mdl() },
    { "progs/gib3.mdl"sv, pak::progs::gib3_mdl() },
    { "progs/grenade.mdl"sv, pak::progs::grenade_mdl() },
    { "progs/h_demon.mdl"sv, pak::progs::h_demon_mdl() },
    { "progs/h_dog.mdl"sv, pak::progs::h_dog_mdl() },
    { "progs/h_guard.mdl"sv, pak::progs::h_guard_mdl() },
    { "progs/h_knight.mdl"sv, pak::progs::h_knight_mdl() },
    { "progs/h_ogre.mdl"sv, pak::progs::h_ogre_mdl() },
    { "progs/h_player.mdl"sv, pak::progs::h_player_mdl() },
    { "progs/h_shams.mdl"sv, pak::progs::h_shams_mdl() },
    { "progs/h_wizard.mdl"sv, pak::progs::h_wizard_mdl() },
    { "progs/h_zombie.mdl"sv, pak::progs::h_zombie_mdl() },
    { "progs/invisibl.mdl"sv, pak::progs::invisibl_mdl() },
    { "progs/invulner.mdl"sv, pak::progs::invulner_mdl() },
    { "progs/knight.mdl"sv, pak::progs::knight_mdl() },
    { "progs/lavaball.mdl"sv, pak::progs::lavaball_mdl() },
    { "progs/m_g_key.mdl"sv, pak::progs::m_g_key_mdl() },
    { "progs/m_s_key.mdl"sv, pak::progs::m_s_key_mdl() },
    { "progs/missile.mdl"sv, pak::progs::missile_mdl() },
    { "progs/ogre.mdl"sv, pak::progs::ogre_mdl() },
    { "progs/player.mdl"sv, pak::progs::player_mdl() },
    { "progs/quaddama.mdl"sv, pak::progs::quaddama_mdl() },
    { "progs/s_bubble.spr"sv, pak::progs::s_bubble_spr() },
    { "progs/s_explod.spr"sv, pak::progs::s_explod_spr() },
    { "progs/s_light.mdl"sv, pak::progs::s_light_mdl() },
    { "progs/s_light.spr"sv, pak::progs::s_light_spr() },
    { "progs/s_spike.mdl"sv, pak::progs::s_spike_mdl() },
    { "progs/shambler.mdl"sv, pak::progs::shambler_mdl() },
    { "progs/soldier.mdl"sv, pak::progs::soldier_mdl() },
    { "progs/spike.mdl"sv, pak::progs::spike_mdl() },
    { "progs/suit.mdl"sv, pak::progs::suit_mdl() },
    { "progs/v_axe.mdl"sv, pak::progs::v_axe_mdl() },
    { "progs/v_light.mdl"sv, pak::progs::v_light_mdl() },
    { "progs/v_nail.mdl"sv, pak::progs::v_nail_mdl() },
    { "progs/v_nail2.mdl"sv, pak::progs::v_nail2_mdl() },
    { "progs/v_rock.mdl"sv, pak::progs::v_rock_mdl() },
    { "progs/v_rock2.mdl"sv, pak::progs::v_rock2_mdl() },
    { "progs/v_shot.mdl"sv, pak::progs::v_shot_mdl() },
    { "progs/v_shot2.mdl"sv, pak::progs::v_shot2_mdl() },
    { "progs/w_g_key.mdl"sv, pak::progs::w_g_key_mdl() },
    { "progs/w_s_key.mdl"sv, pak::progs::w_s_key_mdl() },
    { "progs/w_spike.mdl"sv, pak::progs::w_spike_mdl() },
    { "progs/wizard.mdl"sv, pak::progs::wizard_mdl() },
    { "progs/zom_gib.mdl"sv, pak::progs::zom_gib_mdl() },
    { "progs/zombie.mdl"sv, pak::progs::zombie_mdl() },

    };

    std::span<unsigned char> res;
    if (auto it = m.find(path); it != m.end())
        res = it->second;

    return res;
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
    strcpy(com_gamedir, dir);
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
}
