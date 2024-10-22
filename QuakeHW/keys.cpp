
#include "quakedef.h"

/*

key up events are sent even if in console mode

*/


#define MAXCMDLINE 256
char key_lines[32][MAXCMDLINE];
int key_linepos;
int shift_down = false;
int key_lastpress;

int edit_line = 0;
int history_line = 0;

keydest_t key_dest;

int key_count; // incremented every key event

char * keybindings[256];
bool consolekeys[256]; // if true, can't be rebound while in console
bool menubound[256]; // if true, can't be rebound while in menu
int keyshift[256]; // key to map to if shift held down in console
int key_repeats[256]; // if > 1, it is autorepeating
bool keydown[256];

typedef struct
{
    char * name;
    int keynum;
} keyname_t;

keyname_t keynames[] =
{
    {(char*)"TAB", K_TAB},
    {(char*)"ENTER", K_ENTER},
    {(char*)"ESCAPE", K_ESCAPE},
    {(char*)"SPACE", K_SPACE},
    {(char*)"BACKSPACE", K_BACKSPACE},
    {(char*)"UPARROW", K_UPARROW},
    {(char*)"DOWNARROW", K_DOWNARROW},
    {(char*)"LEFTARROW", K_LEFTARROW},
    {(char*)"RIGHTARROW", K_RIGHTARROW},

    {(char*)"ALT", K_ALT},
    {(char*)"CTRL", K_CTRL},
    {(char*)"SHIFT", K_SHIFT},

    {(char*)"F1", K_F1},
    {(char*)"F2", K_F2},
    {(char*)"F3", K_F3},
    {(char*)"F4", K_F4},
    {(char*)"F5", K_F5},
    {(char*)"F6", K_F6},
    {(char*)"F7", K_F7},
    {(char*)"F8", K_F8},
    {(char*)"F9", K_F9},
    {(char*)"F10", K_F10},
    {(char*)"F11", K_F11},
    {(char*)"F12", K_F12},

    {(char*)"INS", K_INS},
    {(char*)"DEL", K_DEL},
    {(char*)"PGDN", K_PGDN},
    {(char*)"PGUP", K_PGUP},
    {(char*)"HOME", K_HOME},
    {(char*)"END", K_END},

    {(char*)"MOUSE1", K_MOUSE1},
    {(char*)"MOUSE2", K_MOUSE2},
    {(char*)"MOUSE3", K_MOUSE3},

    {(char*)"JOY1", K_JOY1},
    {(char*)"JOY2", K_JOY2},
    {(char*)"JOY3", K_JOY3},
    {(char*)"JOY4", K_JOY4},

    {(char*)"AUX1", K_AUX1},
    {(char*)"AUX2", K_AUX2},
    {(char*)"AUX3", K_AUX3},
    {(char*)"AUX4", K_AUX4},
    {(char*)"AUX5", K_AUX5},
    {(char*)"AUX6", K_AUX6},
    {(char*)"AUX7", K_AUX7},
    {(char*)"AUX8", K_AUX8},
    {(char*)"AUX9", K_AUX9},
    {(char*)"AUX10", K_AUX10},
    {(char*)"AUX11", K_AUX11},
    {(char*)"AUX12", K_AUX12},
    {(char*)"AUX13", K_AUX13},
    {(char*)"AUX14", K_AUX14},
    {(char*)"AUX15", K_AUX15},
    {(char*)"AUX16", K_AUX16},
    {(char*)"AUX17", K_AUX17},
    {(char*)"AUX18", K_AUX18},
    {(char*)"AUX19", K_AUX19},
    {(char*)"AUX20", K_AUX20},
    {(char*)"AUX21", K_AUX21},
    {(char*)"AUX22", K_AUX22},
    {(char*)"AUX23", K_AUX23},
    {(char*)"AUX24", K_AUX24},
    {(char*)"AUX25", K_AUX25},
    {(char*)"AUX26", K_AUX26},
    {(char*)"AUX27", K_AUX27},
    {(char*)"AUX28", K_AUX28},
    {(char*)"AUX29", K_AUX29},
    {(char*)"AUX30", K_AUX30},
    {(char*)"AUX31", K_AUX31},
    {(char*)"AUX32", K_AUX32},

    {(char*)"PAUSE", K_PAUSE},

    {(char*)"MWHEELUP", K_MWHEELUP},
    {(char*)"MWHEELDOWN", K_MWHEELDOWN},

    {(char*)"SEMICOLON", ';'}, // because a raw semicolon seperates commands

    {NULL, 0}
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
    char * cmd;

    if (key == K_ENTER)
    {
        Cbuf_AddText(key_lines[edit_line] + 1); // skip the >
        Cbuf_AddText("\n");
        Con_Printf("%s\n", key_lines[edit_line]);
        edit_line = (edit_line + 1) & 31;
        history_line = edit_line;
        key_lines[edit_line][0] = ']';
        key_linepos = 1;
        if (cls.state == ca_disconnected)
            SCR_UpdateScreen(); // force an update, because the command
        // may take some time
        return;
    }

    if (key == K_TAB)
    { // command completion
        cmd = Cmd_CompleteCommand(key_lines[edit_line] + 1);
        if (!cmd)
            cmd = Cvar_CompleteVariable(key_lines[edit_line] + 1);
        if (cmd)
        {
            std::strcpy(key_lines[edit_line] + 1, cmd);
            key_linepos = std::strlen(cmd) + 1;
            key_lines[edit_line][key_linepos] = ' ';
            key_linepos++;
            key_lines[edit_line][key_linepos] = 0;
            return;
        }
    }

    if (key == K_BACKSPACE || key == K_LEFTARROW)
    {
        if (key_linepos > 1)
            key_linepos--;
        return;
    }

    if (key == K_UPARROW)
    {
        do
        {
            history_line = (history_line - 1) & 31;
        } while (history_line != edit_line
                 && !key_lines[history_line][1]);
        if (history_line == edit_line)
            history_line = (edit_line + 1) & 31;
        std::strcpy(key_lines[edit_line], key_lines[history_line]);
        key_linepos = std::strlen(key_lines[edit_line]);
        return;
    }

    if (key == K_DOWNARROW)
    {
        if (history_line == edit_line) return;
        do
        {
            history_line = (history_line + 1) & 31;
        } while (history_line != edit_line
                && !key_lines[history_line][1]);
        if (history_line == edit_line)
        {
            key_lines[edit_line][0] = ']';
            key_linepos = 1;
        }
        else
        {
            std::strcpy(key_lines[edit_line], key_lines[history_line]);
            key_linepos = std::strlen(key_lines[edit_line]);
        }
        return;
    }

    if (key == K_PGUP || key == K_MWHEELUP)
    {
        con_backscroll += 2;
        if (con_backscroll > con_totallines - (vid.height >> 3) - 1)
            con_backscroll = con_totallines - (vid.height >> 3) - 1;
        return;
    }

    if (key == K_PGDN || key == K_MWHEELDOWN)
    {
        con_backscroll -= 2;
        if (con_backscroll < 0)
            con_backscroll = 0;
        return;
    }

    if (key == K_HOME)
    {
        con_backscroll = con_totallines - (vid.height >> 3) - 1;
        return;
    }

    if (key == K_END)
    {
        con_backscroll = 0;
        return;
    }

    if (key < 32 || key > 127)
        return; // non printable

    if (key_linepos < MAXCMDLINE - 1)
    {
        key_lines[edit_line][key_linepos] = key;
        key_linepos++;
        key_lines[edit_line][key_linepos] = 0;
    }

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
            Cbuf_AddText("say_team \"");
        else
            Cbuf_AddText("say \"");
        Cbuf_AddText(chat_buffer);
        Cbuf_AddText("\"\n");

        key_dest = key_game;
        chat_bufferlen = 0;
        chat_buffer[0] = 0;
        return;
    }

    if (key == K_ESCAPE)
    {
        key_dest = key_game;
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
int Key_StringToKeynum(char * str)
{
    keyname_t * kn;

    if (!str || !str[0])
        return -1;
    if (!str[1])
        return str[0];

    for (kn = keynames; kn->name; kn++)
    {
        if (!Q_strcasecmp(str, kn->name))
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
char * Key_KeynumToString(int keynum)
{
    keyname_t * kn;
    static char tinystr[2];

    if (keynum == -1)
        return (char*)"<KEY NOT FOUND>";
    if (keynum > 32 && keynum < 127)
    { // printable ascii
        tinystr[0] = keynum;
        tinystr[1] = 0;
        return tinystr;
    }

    for (kn = keynames; kn->name; kn++)
        if (keynum == kn->keynum)
            return kn->name;

    return (char*)"<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding(int keynum, char * binding)
{
    char * new_;
    int l;

    if (keynum == -1)
        return;

    // free old bindings
    if (keybindings[keynum])
    {
        Z_Free(keybindings[keynum]);
        keybindings[keynum] = NULL;
    }

    // allocate memory for new binding
    l = std::strlen(binding);
    new_ = (char*)Z_Malloc(l + 1);
    std::strcpy(new_, binding);
    new_[l] = 0;
    keybindings[keynum] = new_;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f()
{
    int b;

    if (Cmd_Argc() != 2)
    {
        Con_Printf("unbind <key> : remove commands from a key\n");
        return;
    }

    b = Key_StringToKeynum(Cmd_Argv(1));
    if (b == -1)
    {
        Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
        return;
    }

    Key_SetBinding(b, (char*)"");
}

void Key_Unbindall_f()
{
    int i;

    for (i = 0; i < 256; i++)
        if (keybindings[i])
            Key_SetBinding(i, (char*)"");
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

    c = Cmd_Argc();

    if (c != 2 && c != 3)
    {
        Con_Printf("bind <key> [command] : attach a command to a key\n");
        return;
    }
    b = Key_StringToKeynum(Cmd_Argv(1));
    if (b == -1)
    {
        Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
        return;
    }

    if (c == 2)
    {
        if (keybindings[b])
            Con_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1), keybindings[b]);
        else
            Con_Printf("\"%s\" is not bound\n", Cmd_Argv(1));
        return;
    }

    // copy the rest of the command line
    cmd[0] = 0; // start out with a null string
    for (i = 2; i < c; i++)
    {
        if (i > 2)
            strcat(cmd, " ");
        strcat(cmd, Cmd_Argv(i));
    }

    Key_SetBinding(b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings(FILE * f)
{
    int i;

    for (i = 0; i < 256; i++)
        if (keybindings[i])
            if (*keybindings[i])
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

    for (i = 0; i < 32; i++)
    {
        key_lines[i][0] = ']';
        key_lines[i][1] = 0;
    }
    key_linepos = 1;

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
    Cmd_AddCommand((char*)"bind", Key_Bind_f);
    Cmd_AddCommand((char*)"unbind", Key_Unbind_f);
    Cmd_AddCommand((char*)"unbindall", Key_Unbindall_f);


}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Key_Event(int key, bool down)
{
    char * kb;
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

        if (key >= 200 && !keybindings[key])
            Con_Printf("%s is unbound, hit F4 to set.\n", Key_KeynumToString(key));
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
            M_Keydown(key);
            break;
        case key_game:
        case key_console:
            M_ToggleMenu_f();
            break;
        default:
            Sys_Error((char*)"Bad key_dest");
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
        kb = keybindings[key];
        if (kb && kb[0] == '+')
        {
            sprintf(cmd, "-%s %i\n", kb + 1, key);
            Cbuf_AddText(cmd);
        }
        if (keyshift[key] != key)
        {
            kb = keybindings[keyshift[key]];
            if (kb && kb[0] == '+')
            {
                sprintf(cmd, "-%s %i\n", kb + 1, key);
                Cbuf_AddText(cmd);
            }
        }
        return;
    }

    //
    // during demo playback, most keys bring up the main menu
    //
    if (cls.demoplayback && down && consolekeys[key] && key_dest == key_game)
    {
        M_ToggleMenu_f();
        return;
    }

    //
    // if not a consolekey, send to the interpreter no matter what mode is
    //
    if ((key_dest == key_menu && menubound[key])
        || (key_dest == key_console && !consolekeys[key])
        || (key_dest == key_game && (!con_forcedup || !consolekeys[key])))
    {
        kb = keybindings[key];
        if (kb)
        {
            if (kb[0] == '+')
            { // button commands add keynum as a parm
                sprintf(cmd, "%s %i\n", kb, key);
                Cbuf_AddText(cmd);
            }
            else
            {
                Cbuf_AddText(kb);
                Cbuf_AddText("\n");
            }
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
        M_Keydown(key);
        break;

    case key_game:
    case key_console:
        Key_Console(key);
        break;
    default:
        Sys_Error((char*)"Bad key_dest");
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

