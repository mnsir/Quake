#define _CRT_SECURE_NO_WARNINGS
#include "keys.h"

#include "dll.h"

#include <stdexcept>
#include <cstring>
#include <format>
#include <string>
#include <array>
#include <list>

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

class CommandLineHistory
{
public:
	CommandLineHistory()
		: recent({ "" }) // "" need to repeat the behavior
	{
	}

	void SetEditLine(std::string cmd) noexcept { std::swap(edit_line, cmd); }

	void TryPushBack(char ch) noexcept
	{
		if (edit_line.size() < edit_line_max_size)
		{
			try
			{
				edit_line.reserve(edit_line.size() + 1); // may throw
				edit_line.push_back(ch);
			}
			catch (const std::exception& e)
			{
			}
		}
	}

	void ClearAnyTyping() noexcept { edit_line.clear(); }

	void TryPopBack() noexcept
	{
		if (!edit_line.empty())
			edit_line.pop_back();
	}

	const std::string& GetEditLine() const noexcept { return edit_line; }

	void Flush() noexcept
	{
		try
		{
			recent.push_back(std::string()); // may throw
			if (recent.size() > recent_max_size)
				recent.pop_front();
			std::swap(recent.back(), edit_line);
			it_recent = recent.end();
		}
		catch (const std::exception& e)
		{
		}
	}

	void Prev()
	{
		if (auto it = it_recent; it != recent.begin())
		{
			it--;
			edit_line = *it; // may throw
			it_recent = it;
		}
	}

	void Next()
	{
		if (auto it = it_recent; it != recent.end())
		{
			it++;
			if (it != recent.end())
				edit_line = *it; // may throw
			else
				edit_line.clear();

			it_recent = it;
		}
	}

private:
	static constexpr size_t edit_line_max_size = 0xFF;
	static constexpr size_t recent_max_size = 0x1F;

	std::list<std::string> recent;
	decltype(recent)::const_iterator it_recent = recent.end();

	std::string edit_line;
};

CommandLineHistory commandLineHistory;

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

namespace {

	using namespace std::string_view_literals;

	auto keynames = std::to_array({
			std::pair{"TAB"sv, K_TAB},
			std::pair{"ENTER"sv, K_ENTER},
			std::pair{"ESCAPE"sv, K_ESCAPE},
			std::pair{"SPACE"sv, K_SPACE},
			std::pair{"BACKSPACE"sv, K_BACKSPACE},
			std::pair{"UPARROW"sv, K_UPARROW},
			std::pair{"DOWNARROW"sv, K_DOWNARROW},
			std::pair{"LEFTARROW"sv, K_LEFTARROW},
			std::pair{"RIGHTARROW"sv, K_RIGHTARROW},
			std::pair{"ALT"sv, K_ALT},
			std::pair{"CTRL"sv, K_CTRL},
			std::pair{"SHIFT"sv, K_SHIFT},
			std::pair{"F1"sv, K_F1},
			std::pair{"F2"sv, K_F2},
			std::pair{"F3"sv, K_F3},
			std::pair{"F4"sv, K_F4},
			std::pair{"F5"sv, K_F5},
			std::pair{"F6"sv, K_F6},
			std::pair{"F7"sv, K_F7},
			std::pair{"F8"sv, K_F8},
			std::pair{"F9"sv, K_F9},
			std::pair{"F10"sv, K_F10},
			std::pair{"F11"sv, K_F11},
			std::pair{"F12"sv, K_F12},
			std::pair{"INS"sv, K_INS},
			std::pair{"DEL"sv, K_DEL},
			std::pair{"PGDN"sv, K_PGDN},
			std::pair{"PGUP"sv, K_PGUP},
			std::pair{"HOME"sv, K_HOME},
			std::pair{"END"sv, K_END},
			std::pair{"MOUSE1"sv, K_MOUSE1},
			std::pair{"MOUSE2"sv, K_MOUSE2},
			std::pair{"MOUSE3"sv, K_MOUSE3},
			std::pair{"JOY1"sv, K_JOY1},
			std::pair{"JOY2"sv, K_JOY2},
			std::pair{"JOY3"sv, K_JOY3},
			std::pair{"JOY4"sv, K_JOY4},
			std::pair{"AUX1"sv, K_AUX1},
			std::pair{"AUX2"sv, K_AUX2},
			std::pair{"AUX3"sv, K_AUX3},
			std::pair{"AUX4"sv, K_AUX4},
			std::pair{"AUX5"sv, K_AUX5},
			std::pair{"AUX6"sv, K_AUX6},
			std::pair{"AUX7"sv, K_AUX7},
			std::pair{"AUX8"sv, K_AUX8},
			std::pair{"AUX9"sv, K_AUX9},
			std::pair{"AUX10"sv, K_AUX10},
			std::pair{"AUX11"sv, K_AUX11},
			std::pair{"AUX12"sv, K_AUX12},
			std::pair{"AUX13"sv, K_AUX13},
			std::pair{"AUX14"sv, K_AUX14},
			std::pair{"AUX15"sv, K_AUX15},
			std::pair{"AUX16"sv, K_AUX16},
			std::pair{"AUX17"sv, K_AUX17},
			std::pair{"AUX18"sv, K_AUX18},
			std::pair{"AUX19"sv, K_AUX19},
			std::pair{"AUX20"sv, K_AUX20},
			std::pair{"AUX21"sv, K_AUX21},
			std::pair{"AUX22"sv, K_AUX22},
			std::pair{"AUX23"sv, K_AUX23},
			std::pair{"AUX24"sv, K_AUX24},
			std::pair{"AUX25"sv, K_AUX25},
			std::pair{"AUX26"sv, K_AUX26},
			std::pair{"AUX27"sv, K_AUX27},
			std::pair{"AUX28"sv, K_AUX28},
			std::pair{"AUX29"sv, K_AUX29},
			std::pair{"AUX30"sv, K_AUX30},
			std::pair{"AUX31"sv, K_AUX31},
			std::pair{"AUX32"sv, K_AUX32},
			std::pair{"PAUSE"sv, K_PAUSE},
			std::pair{"MWHEELUP"sv, K_MWHEELUP},
			std::pair{"MWHEELDOWN"sv, K_MWHEELDOWN},
			std::pair{"SEMICOLON"sv, static_cast<int>(';')}, // because a raw semicolon seperates commands
		});
}
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
		auto&& line = commandLineHistory.GetEditLine();
		auto fmt = std::format("{}\n", line); // no prompt
		dll.Cbuf_AddText(fmt.c_str());
		fmt = std::format("]{}\n", line); // prompt
		dll.Lib_Con_Printf(fmt.c_str());

		commandLineHistory.Flush();

		if (dll.CL_IsStateDisconnected())
			dll.SCR_UpdateScreen(); // force an update, because the command
		// may take some time
		return;
	}

	if (key == K_TAB)
	{ // command completion
		auto&& line = commandLineHistory.GetEditLine();
		const char* cmd = dll.Cmd_CompleteCommand(line.data());
		if (!cmd)
			cmd = dll.Cvar_CompleteVariable(line.data());
		if (cmd)
		{
			commandLineHistory.SetEditLine(std::format("{} ", cmd));
			return;
		}
	}

	if (key == K_BACKSPACE || key == K_LEFTARROW)
	{
		commandLineHistory.TryPopBack();
		return;
	}

	if (key == K_UPARROW)
	{
		commandLineHistory.Prev();
		return;
	}

	if (key == K_DOWNARROW)
	{
		commandLineHistory.Next();
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

	commandLineHistory.TryPushBack(key);
}

//============================================================================

std::string chat_buffer;
bool team_message = false;

void Key_Message(int key)
{
	constexpr size_t chat_buffer_max_size = 31;

	if (key == K_ENTER)
	{
		std::array<char, 64> buf;
		if (team_message)
			*std::format_to(buf.data(), "say_team \"{}\"\n", chat_buffer) = '\0';
		else
			*std::format_to(buf.data(), "say \"{}\"\n", chat_buffer) = '\0';
		dll.Cbuf_AddText(buf.data());

		Key_SetDest(key_game);
		chat_buffer.clear();
		return;
	}

	if (key == K_ESCAPE)
	{
		Key_SetDest(key_game);
		chat_buffer.clear();
		return;
	}

	if (key < 32 || key > 127)
		return; // non printable

	if (key == K_BACKSPACE)
	{
		if (!chat_buffer.empty())
			chat_buffer.pop_back();
		return;
	}

	if (chat_buffer.size() < chat_buffer_max_size)
	{
		chat_buffer.reserve(chat_buffer.size() + 1); // may throw
		chat_buffer.push_back(key);
	}
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
int Key_StringToKeynum(const char* str)
{
	if (!str || !str[0])
		return -1;
	if (!str[1])
		return str[0];
	
	auto&& str_ = std::string(str);
	for (auto&& ch : str_)
		ch = std::toupper(ch);

	if (auto it = std::ranges::find(keynames, str_, &decltype(keynames)::value_type::first); it != keynames.end())
		return it->second;

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
const char* Key_KeynumToString(int keynum)
{
	static char tinystr[2];

	if (keynum == -1)
		return "<KEY NOT FOUND>";
	if (keynum > 32 && keynum < 127)
	{ // printable ascii
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}

	if (auto it = std::ranges::find(keynames, keynum, &decltype(keynames)::value_type::second); it != keynames.end())
		return it->first.data();

	return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding(int keynum, const char* binding)
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
	for (auto&& keybinding : keybindings)
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
void Key_WriteBindings(void* f_)
{
	FILE* f = reinterpret_cast<FILE*>(f_);
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
		auto&& kb = keybindings[key];
		if (kb[0] == '+')
		{
			sprintf(cmd, "-%s %i\n", kb.c_str() + 1, key);
			dll.Cbuf_AddText(cmd);
		}
		if (keyshift[key] != key)
		{
			auto&& kb = keybindings[keyshift[key]];
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
		auto&& kb = keybindings[key];
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
const char* Key_GetBinding(int i) { return keybindings[i].c_str(); }
int Key_GetLastPress() { return key_lastpress; }
int Key_GetCount() { return key_count; }
void Key_SetCount(int val) { key_count = val; }
const char* Key_GetChatBuffer() { return chat_buffer.c_str(); }
void Key_SetTeamMessage(int val) { team_message = val; }

const char* Key_GetEditLine() { return commandLineHistory.GetEditLine().c_str(); }
int Key_Get_LinePos() { return commandLineHistory.GetEditLine().size(); }
void Key_ClearAnyTyping()
{
	commandLineHistory.ClearAnyTyping();
}