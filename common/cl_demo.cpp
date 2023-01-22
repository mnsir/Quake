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
along with this_ program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "cl_demo.h"

#include "client.h"
#include "cl_main.h"
#include "cmd.h"
#include "console.h"
#include "host.h"
#include "protocol.h"
#include "sys.h"


void CL_FinishTimeDemo(void);

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
void CL_StopPlayback(void)
{
	if (!cls.demoplayback)
		return;

	fclose(cls.demofile);
	cls.demoplayback = false;
	cls.demofile = NULL;
	cls.state = ca_disconnected;

	if (cls.timedemo)
		CL_FinishTimeDemo();
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
void CL_WriteDemoMessage(void)
{
	int len;
	float f;

	len = LittleLong(net_message.cursize);
	fwrite(&len, 4, 1, cls.demofile);
	for (int i = 0; i < 3; i++)
	{
		f = LittleFloat(cl.viewangles[i]);
		fwrite(&f, 4, 1, cls.demofile);
	}
	fwrite(net_message.data, net_message.cursize, 1, cls.demofile);
	fflush(cls.demofile);
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
int CL_GetMessage(void)
{
	int r;
	float f;

	if (cls.demoplayback)
	{
		// decide if it is time to grab the next message 
		if (cls.signon == SIGNONS) // allways grab until fully connected
		{
			if (cls.timedemo)
			{
				if (host_framecount == cls.td_lastframe)
					return 0; // allready read this_ frame's message
				cls.td_lastframe = host_framecount;
				// if this_ is the second frame, grab the real td_starttime
				// so the bogus time on the first frame doesn't count
				if (host_framecount == cls.td_startframe + 1)
					cls.td_starttime = realtime;
			}
			else if (/* cl.time > 0 && */ cl.time <= cl.mtime[0])
			{
				return 0; // don't need another message yet
			}
		}

		// get the next message
		fread(&net_message.cursize, 4, 1, cls.demofile);
		VectorCopy(cl.mviewangles[0], cl.mviewangles[1]);
		for (int i = 0; i < 3; i++)
		{
			r = fread(&f, 4, 1, cls.demofile);
			cl.mviewangles[0][i] = LittleFloat(f);
		}

		net_message.cursize = LittleLong(net_message.cursize);
		if (net_message.cursize > MAX_MSGLEN)
			Sys_Error((char*)"Demo message > MAX_MSGLEN");
		r = fread(net_message.data, net_message.cursize, 1, cls.demofile);
		if (r != 1)
		{
			CL_StopPlayback();
			return 0;
		}

		return 1;
	}

	while (true)
	{
		r = NET_GetMessage(cls.netcon);

		if (r != 1 && r != 2)
			return r;

		// discard nop keepalive message
		if (net_message.cursize == 1 && net_message.data[0] == svc_nop)
			Con_Printf((char*)"<-- server to client keepalive\n");
		else
			break;
	}

	if (cls.demorecording)
		CL_WriteDemoMessage();

	return r;
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f(void)
{
	if (cmd_source != src_command)
		return;

	if (!cls.demorecording)
	{
		Con_Printf((char*)"Not recording a demo.\n");
		return;
	}

	// write a disconnect message to the demo file
	SZ_Clear(&net_message);
	MSG_WriteByte(&net_message, svc_disconnect);
	CL_WriteDemoMessage();

	// finish up
	fclose(cls.demofile);
	cls.demofile = NULL;
	cls.demorecording = false;
	Con_Printf((char*)"Completed demo\n");
}

/*
====================
CL_Record_f

record <demoname> <map> [cd track]
====================
*/
void CL_Record_f(void)
{
	char name[MAX_OSPATH];
	int track;

	if (cmd_source != src_command)
		return;

	int c = Cmd_Argc();
	if (c != 2 && c != 3 && c != 4)
	{
		Con_Printf((char*)"record <demoname> [<map> [cd track]]\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_Printf((char*)"Relative pathnames are not allowed.\n");
		return;
	}

	if (c == 2 && cls.state == ca_connected)
	{
		Con_Printf(
			(char*)"Can not record - already connected to server\nClient demo recording must be started before connecting\n");
		return;
	}

	// write the forced cd track number, or -1
	if (c == 4)
	{
		track = atoi(Cmd_Argv(3));
		Con_Printf((char*)"Forcing CD track to %i\n", cls.forcetrack);
	}
	else
		track = -1;

	sprintf(name, (char*)"%s/%s", com_gamedir, Cmd_Argv(1));

	//
	// start the map up
	//
	if (c > 2)
		Cmd_ExecuteString(va((char*)"map %s", Cmd_Argv(2)), src_command);

	//
	// open the demo file
	//
	COM_DefaultExtension(name, (char*)".dem");

	Con_Printf((char*)"recording to %s.\n", name);
	cls.demofile = fopen(name, "wb");
	if (!cls.demofile)
	{
		Con_Printf((char*)"ERROR: couldn't open.\n");
		return;
	}

	cls.forcetrack = track;
	fprintf(cls.demofile, (char*)"%i\n", cls.forcetrack);

	cls.demorecording = true;
}


/*
====================
CL_PlayDemo_f

play [demoname]
====================
*/
void CL_PlayDemo_f(void)
{
	char name[256];
	int c;
	bool neg = false;

	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf((char*)"play <demoname> : plays a demo\n");
		return;
	}

	//
	// disconnect from server
	//
	CL_Disconnect();

	//
	// open the demo file
	//
	strcpy(name, Cmd_Argv(1));
	COM_DefaultExtension(name, (char*)".dem");

	Con_Printf((char*)"Playing demo from %s.\n", name);
	COM_FOpenFile(name, &cls.demofile);
	if (!cls.demofile)
	{
		Con_Printf((char*)"ERROR: couldn't open.\n");
		cls.demonum = -1; // stop demo loop
		return;
	}

	cls.demoplayback = true;
	cls.state = ca_connected;
	cls.forcetrack = 0;

	while ((c = getc(cls.demofile)) != '\n')
		if (c == '-')
			neg = true;
		else
			cls.forcetrack = cls.forcetrack * 10 + (c - '0');

	if (neg)
		cls.forcetrack = -cls.forcetrack;
	// ZOID, fscanf is evil
	// fscanf (cls.demofile, (char*)"%i\n", &cls.forcetrack);
}

/*
====================
CL_FinishTimeDemo

====================
*/
void CL_FinishTimeDemo(void)
{
	cls.timedemo = false;

	// the first frame didn't count
	int frames = (host_framecount - cls.td_startframe) - 1;
	float time = realtime - cls.td_starttime;
	if (!time)
		time = 1;
	Con_Printf((char*)"%i frames %5.1f seconds %5.1f fps\n", frames, time, frames / time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f(void)
{
	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf((char*)"timedemo <demoname> : gets demo speeds\n");
		return;
	}

	CL_PlayDemo_f();

	// cls.td_starttime will be grabbed at the second frame of the demo, so
	// all the loading time doesn't get counted

	cls.timedemo = true;
	cls.td_startframe = host_framecount;
	cls.td_lastframe = -1; // get a new_ message this_ frame
}
