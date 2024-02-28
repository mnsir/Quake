#define _CRT_SECURE_NO_WARNINGS
#include "keys.h"

#include "dll.h"

#include <stdexcept>
#include <cstring>
#include <format>
#include <string>
#include <array>

/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

/*

key up events are sent even if in console mode

*/

//typedef enum { key_game, key_console, key_message, key_menu } keydest_t;

struct Qwe
{
    Qwe()
    {
    }

    void SetEditLine(std::string_view cmd)
    {
        lines[edit_line] = std::format("{} ", cmd);
    }

    void AppendChar(char ch)
    {
        if (lines[edit_line].size() < MAXCMDLINE - 1)
        {
            lines[edit_line] += ch;
        }
    }

    void ClearAnyTyping()
    {
        lines[edit_line].clear();
    }

    void Prev()
    {
        do
        {
            history_line = (history_line - 1) & 31;
        } while (history_line != edit_line && lines[history_line].empty());
        if (history_line == edit_line)
            history_line = (edit_line + 1) & 31;

        lines[edit_line] = lines[history_line];
    }

    void Next()
    {
        if (history_line != edit_line)
        {
            do
            {
                history_line = (history_line + 1) & 31;
            } while (history_line != edit_line && lines[history_line].empty());
            if (history_line == edit_line)
            {
                ClearAnyTyping();
            }
            else
            {
                lines[edit_line] = lines[history_line];
            }
        }
    }

    void Pop()
    {
        if (!lines[edit_line].empty())
        {
            lines[edit_line].pop_back();
        }
    }

    std::string_view GetEditLine() const
    {
        return lines[edit_line];
    }

    void Flush()
    {
        edit_line = (edit_line + 1) & 31;
        history_line = edit_line;
        ClearAnyTyping();
    }

    static constexpr size_t MAXCMDLINE = 0xFF;
    std::array<std::string, 32> lines;
    size_t edit_line = 0;
    size_t history_line = 0;
};

Qwe qwe;

int shift_down = false;
int key_lastpress;

/*keydest_t*/int key_dest;

int key_count; // incremented every key event

std::array<std::string, 256> keybindings;
bool consolekeys[256]; // if true, can't be rebound while in console
bool menubound[256]; // if true, can't be rebound while in menu
int keyshift[256]; // key to map to if shift held down in console
int key_repeats[256]; // if > 1, it is autorepeating
bool keydown[256];

typedef struct
{
    const char * name;
    int keynum;
} keyname_t;

keyname_t keynames[] =
{
    {"TAB", K_TAB},
    {"ENTER", K_ENTER},
    {"ESCAPE", K_ESCAPE},
    {"SPACE", K_SPACE},
    {"BACKSPACE", K_BACKSPACE},
    {"UPARROW", K_UPARROW},
    {"DOWNARROW", K_DOWNARROW},
    {"LEFTARROW", K_LEFTARROW},
    {"RIGHTARROW", K_RIGHTARROW},

    {"ALT", K_ALT},
    {"CTRL", K_CTRL},
    {"SHIFT", K_SHIFT},

    {"F1", K_F1},
    {"F2", K_F2},
    {"F3", K_F3},
    {"F4", K_F4},
    {"F5", K_F5},
    {"F6", K_F6},
    {"F7", K_F7},
    {"F8", K_F8},
    {"F9", K_F9},
    {"F10", K_F10},
    {"F11", K_F11},
    {"F12", K_F12},

    {"INS", K_INS},
    {"DEL", K_DEL},
    {"PGDN", K_PGDN},
    {"PGUP", K_PGUP},
    {"HOME", K_HOME},
    {"END", K_END},

    {"MOUSE1", K_MOUSE1},
    {"MOUSE2", K_MOUSE2},
    {"MOUSE3", K_MOUSE3},

    {"JOY1", K_JOY1},
    {"JOY2", K_JOY2},
    {"JOY3", K_JOY3},
    {"JOY4", K_JOY4},

    {"AUX1", K_AUX1},
    {"AUX2", K_AUX2},
    {"AUX3", K_AUX3},
    {"AUX4", K_AUX4},
    {"AUX5", K_AUX5},
    {"AUX6", K_AUX6},
    {"AUX7", K_AUX7},
    {"AUX8", K_AUX8},
    {"AUX9", K_AUX9},
    {"AUX10", K_AUX10},
    {"AUX11", K_AUX11},
    {"AUX12", K_AUX12},
    {"AUX13", K_AUX13},
    {"AUX14", K_AUX14},
    {"AUX15", K_AUX15},
    {"AUX16", K_AUX16},
    {"AUX17", K_AUX17},
    {"AUX18", K_AUX18},
    {"AUX19", K_AUX19},
    {"AUX20", K_AUX20},
    {"AUX21", K_AUX21},
    {"AUX22", K_AUX22},
    {"AUX23", K_AUX23},
    {"AUX24", K_AUX24},
    {"AUX25", K_AUX25},
    {"AUX26", K_AUX26},
    {"AUX27", K_AUX27},
    {"AUX28", K_AUX28},
    {"AUX29", K_AUX29},
    {"AUX30", K_AUX30},
    {"AUX31", K_AUX31},
    {"AUX32", K_AUX32},

    {"PAUSE", K_PAUSE},

    {"MWHEELUP", K_MWHEELUP},
    {"MWHEELDOWN", K_MWHEELDOWN},

    {"SEMICOLON", ';'}, // because a raw semicolon seperates commands

    {nullptr, 0}
};

/*
==============================================================================

 LINE TYPING INTO THE CONSOLE

==============================================================================
*/


/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
void Key_Console(int key)
{
    if (key == K_ENTER)
    {
        auto line = qwe.GetEditLine();
        auto fmt = std::format("{}\n", line); // no prompt
        dll.Cbuf_AddText(fmt.c_str());
        fmt = std::format("]{}\n", line); // prompt
        dll.Lib_Con_Printf(fmt.c_str());

        qwe.Flush();

        if (dll.CL_IsStateDisconnected())
            dll.SCR_UpdateScreen(); // force an update, because the command
        // may take some time
        return;
    }

    if (key == K_TAB)
    { // command completion
        auto line = qwe.GetEditLine();
        const char * cmd = dll.Cmd_CompleteCommand(line.data());
        if (!cmd)
            cmd = dll.Cvar_CompleteVariable(line.data());
        if (cmd)
        {
            qwe.SetEditLine(cmd);
            return;
        }
    }

    if (key == K_BACKSPACE || key == K_LEFTARROW)
    {
        qwe.Pop();
        return;
    }

    if (key == K_UPARROW)
    {
        qwe.Prev();
        return;
    }

    if (key == K_DOWNARROW)
    {
        qwe.Next();
        return;
    }

    if (key == K_PGUP || key == K_MWHEELUP)
    {
        dll.Con_SetBackScroll(dll.Con_GetBackScroll() + 2);
        if (dll.Con_GetBackScroll() > dll.Con_GetTotalLines() - (dll.VID_GetHeight() >> 3) - 1)
            dll.Con_SetBackScroll(dll.Con_GetTotalLines() - (dll.VID_GetHeight() >> 3) - 1);
        return;
    }

    if (key == K_PGDN || key == K_MWHEELDOWN)
    {
        dll.Con_SetBackScroll(dll.Con_GetBackScroll() - 2);
        if (dll.Con_GetBackScroll() < 0)
            dll.Con_SetBackScroll(0);
        return;
    }

    if (key == K_HOME)
    {
        dll.Con_SetBackScroll(dll.Con_GetTotalLines() - (dll.VID_GetHeight() >> 3) - 1);
        return;
    }

    if (key == K_END)
    {
        dll.Con_SetBackScroll(0);
        return;
    }

    if (key < 32 || key > 127)
        return; // non printable

    qwe.AppendChar(key);
}

//============================================================================

char chat_buffer[32];
bool team_message = false;

void Key_Message(int key)
{
    static int chat_bufferlen = 0;

    if (key == K_ENTER)
    {
        if (team_message)
            dll.Cbuf_AddText("say_team \"");
        else
            dll.Cbuf_AddText("say \"");
        dll.Cbuf_AddText(chat_buffer);
        dll.Cbuf_AddText("\"\n");

        Key_SetDest(key_game);
        chat_bufferlen = 0;
        chat_buffer[0] = 0;
        return;
    }

    if (key == K_ESCAPE)
    {
        Key_SetDest(key_game);
        chat_bufferlen = 0;
        chat_buffer[0] = 0;
        return;
    }

    if (key < 32 || key > 127)
        return; // non printable

    if (key == K_BACKSPACE)
    {
        if (chat_bufferlen)
        {
            chat_bufferlen--;
            chat_buffer[chat_bufferlen] = 0;
        }
        return;
    }

    if (chat_bufferlen == 31)
        return; // all full

    chat_buffer[chat_bufferlen++] = key;
    chat_buffer[chat_bufferlen] = 0;
}

//============================================================================


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string. Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum(const char * str)
{
    keyname_t * kn;

    if (!str || !str[0])
        return -1;
    if (!str[1])
        return str[0];

    for (kn = keynames; kn->name; kn++)
    {
        if (!_stricmp(str, kn->name))
            return kn->keynum;
    }
    return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
const char * Key_KeynumToString(int keynum)
{
    keyname_t * kn;
    static char tinystr[2];

    if (keynum == -1)
        return "<KEY NOT FOUND>";
    if (keynum > 32 && keynum < 127)
    { // printable ascii
        tinystr[0] = keynum;
        tinystr[1] = 0;
        return tinystr;
    }

    for (kn = keynames; kn->name; kn++)
        if (keynum == kn->keynum)
            return kn->name;

    return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding(int keynum, const char * binding)
{
    if (keynum == -1)
        return;

    keybindings[keynum] = binding;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f()
{
    int b;

    if (dll.Cmd_Argc() != 2)
    {
        dll.Lib_Con_Printf("unbind <key> : remove commands from a key\n");
        return;
    }

    b = Key_StringToKeynum(dll.Cmd_Argv(1));
    if (b == -1)
    {
        auto fmt = std::format("\"{}\" isn't a valid key\n", dll.Cmd_Argv(1));
        dll.Lib_Con_Printf(fmt.c_str());
        return;
    }

    Key_SetBinding(b, "");
}

void Key_Unbindall_f()
{
    for (auto && keybinding : keybindings)
        keybinding.clear();
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f()
{
    int i, c, b;
    char cmd[1024];

    c = dll.Cmd_Argc();

    if (c != 2 && c != 3)
    {
        dll.Lib_Con_Printf("bind <key> [command] : attach a command to a key\n");
        return;
    }
    b = Key_StringToKeynum(dll.Cmd_Argv(1));
    if (b == -1)
    {
        auto fmt = std::format("\"{}\" isn't a valid key\n", dll.Cmd_Argv(1));
        dll.Lib_Con_Printf(fmt.c_str());
        return;
    }

    if (c == 2)
    {
        auto fmt = std::format("\"{}\" = \"{}\"\n", dll.Cmd_Argv(1), keybindings[b]);
        dll.Lib_Con_Printf(fmt.c_str());
        return;
    }

    // copy the rest of the command line
    cmd[0] = 0; // start out with a null string
    for (i = 2; i < c; i++)
    {
        if (i > 2)
            strcat(cmd, " ");
        strcat(cmd, dll.Cmd_Argv(i));
    }

    Key_SetBinding(b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings(void * f_)
{
    FILE * f = reinterpret_cast<FILE *>(f_);
    for (size_t i = 0; i < keybindings.size(); ++i)
        if (!keybindings[i].empty())
            fprintf(f, "bind \"%s\" \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
}


/*
===================
Key_Init
===================
*/
void Key_Init()
{
    int i;

    //
    // init ascii characters in console mode
    //
    for (i = 32; i < 128; i++)
        consolekeys[i] = true;
    consolekeys[K_ENTER] = true;
    consolekeys[K_TAB] = true;
    consolekeys[K_LEFTARROW] = true;
    consolekeys[K_RIGHTARROW] = true;
    consolekeys[K_UPARROW] = true;
    consolekeys[K_DOWNARROW] = true;
    consolekeys[K_BACKSPACE] = true;
    consolekeys[K_PGUP] = true;
    consolekeys[K_PGDN] = true;
    consolekeys[K_SHIFT] = true;
    consolekeys[K_MWHEELUP] = true;
    consolekeys[K_MWHEELDOWN] = true;
    consolekeys['`'] = false;
    consolekeys['~'] = false;

    for (i = 0; i < 256; i++)
        keyshift[i] = i;
    for (i = 'a'; i <= 'z'; i++)
        keyshift[i] = i - 'a' + 'A';
    keyshift['1'] = '!';
    keyshift['2'] = '@';
    keyshift['3'] = '#';
    keyshift['4'] = '$';
    keyshift['5'] = '%';
    keyshift['6'] = '^';
    keyshift['7'] = '&';
    keyshift['8'] = '*';
    keyshift['9'] = '(';
    keyshift['0'] = ')';
    keyshift['-'] = '_';
    keyshift['='] = '+';
    keyshift[','] = '<';
    keyshift['.'] = '>';
    keyshift['/'] = '?';
    keyshift[';'] = ':';
    keyshift['\''] = '"';
    keyshift['['] = '{';
    keyshift[']'] = '}';
    keyshift['`'] = '~';
    keyshift['\\'] = '|';

    menubound[K_ESCAPE] = true;
    for (i = 0; i < 12; i++)
        menubound[K_F1 + i] = true;

    //
    // register our functions
    //
    dll.Cmd_AddCommand("bind", Key_Bind_f);
    dll.Cmd_AddCommand("unbind", Key_Unbind_f);
    dll.Cmd_AddCommand("unbindall", Key_Unbindall_f);


}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Key_Event(int key, int down_)
{
    bool down = static_cast<bool>(down_);
    char cmd[1024];

    keydown[key] = down;

    if (!down)
        key_repeats[key] = 0;

    key_lastpress = key;
    key_count++;
    if (key_count <= 0)
    {
        return; // just catching keys for Con_NotifyBox
    }

    // update auto-repeat status
    if (down)
    {
        key_repeats[key]++;
        if (key != K_BACKSPACE && key != K_PAUSE && key_repeats[key] > 1)
        {
            return; // ignore most autorepeats
        }

        if (key >= 200)
        {
            auto fmt = std::format("{} is unbound, hit F4 to set.\n", Key_KeynumToString(key));
            dll.Lib_Con_Printf(fmt.c_str());
        }
    }

    if (key == K_SHIFT)
        shift_down = down;

    //
    // handle escape specialy, so the user can never unbind it
    //
    if (key == K_ESCAPE)
    {
        if (!down)
            return;
        switch (key_dest)
        {
        case key_message:
            Key_Message(key);
            break;
        case key_menu:
            dll.M_Keydown(key);
            break;
        case key_game:
        case key_console:
            dll.Lib_M_ToggleMenu_f();
            break;
        default:
            throw std::runtime_error("Bad key_dest");
        }
        return;
    }

    //
    // key up events only generate commands if the game key binding is
    // a button command (leading + sign). These will occur even in console mode,
    // to keep the character from continuing an action started before a console
    // switch. Button commands include the kenum as a parameter, so multiple
    // downs can be matched with ups
    //
    if (!down)
    {
        auto && kb = keybindings[key];
        if (kb[0] == '+')
        {
            sprintf(cmd, "-%s %i\n", kb.c_str() + 1, key);
            dll.Cbuf_AddText(cmd);
        }
        if (keyshift[key] != key)
        {
            auto && kb = keybindings[keyshift[key]];
            if (kb[0] == '+')
            {
                sprintf(cmd, "-%s %i\n", kb.c_str() + 1, key);
                dll.Cbuf_AddText(cmd);
            }
        }
        return;
    }

    //
    // during demo playback, most keys bring up the main menu
    //
    if (dll.CL_IsDemoPlayBack() && down && consolekeys[key] && Key_GetDest() == key_game)
    {
        dll.Lib_M_ToggleMenu_f();
        return;
    }

    //
    // if not a consolekey, send to the interpreter no matter what mode is
    //
    if ((Key_GetDest() == key_menu && menubound[key])
        || (Key_GetDest() == key_console && !consolekeys[key])
        || (Key_GetDest() == key_game && (!dll.Con_IsForcedUp() || !consolekeys[key])))
    {
        auto && kb = keybindings[key];
        if (kb[0] == '+')
        { // button commands add keynum as a parm
            sprintf(cmd, "%s %i\n", kb.c_str(), key);
            dll.Cbuf_AddText(cmd);
        }
        else
        {
            dll.Cbuf_AddText(kb.c_str());
            dll.Cbuf_AddText("\n");
        }
        return;
    }

    if (!down)
        return; // other systems only care about key down events

    if (shift_down)
    {
        key = keyshift[key];
    }

    switch (key_dest)
    {
    case key_message:
        Key_Message(key);
        break;
    case key_menu:
        dll.M_Keydown(key);
        break;

    case key_game:
    case key_console:
        Key_Console(key);
        break;
    default:
        throw std::runtime_error("Bad key_dest");
    }
}


/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates()
{
    int i;

    for (i = 0; i < 256; i++)
    {
        keydown[i] = false;
        key_repeats[i] = 0;
    }
}


int Key_GetDest() { return key_dest; }
void Key_SetDest(int val) { key_dest = val; }
const char * Key_GetBinding(int i) { return keybindings[i].c_str(); }
int Key_GetLastPress() { return key_lastpress; }
int Key_GetCount() { return key_count; }
void Key_SetCount(int val) { key_count = val; }
const char * Key_GetChatBuffer() { return chat_buffer; }
void Key_SetTeamMessage(int val) { team_message = val; }

const char * Key_GetEditLine() { return qwe.GetEditLine().data(); }
int Key_Get_LinePos() { return qwe.GetEditLine().size(); }
void Key_ClearAnyTyping()
{
    qwe.ClearAnyTyping();
}