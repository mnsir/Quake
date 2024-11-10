

#include "quakedef.h"

#include <common/time.h>
#include <common/progs.h>

using namespace std::string_view_literals;

extern cvar_t pausable;

int current_skill;

void Mod_Print();

/*
==================
Host_Quit_f
==================
*/

extern void M_Menu_Quit_f();

void Host_Quit_f()
{
    if (key_dest != key_console && cls.state != ca_dedicated)
    {
        M_Menu_Quit_f();
        return;
    }
    CL_Disconnect();
    Host_ShutdownServer(false);

    Sys_Quit();
}


/*
==================
Host_Status_f
==================
*/
void Host_Status_f()
{
    client_t * client;
    int seconds;
    int minutes;
    int hours = 0;
    int j;
    void (*print) (char * fmt, ...);

    if (cmd_source == src_command)
    {
        if (!sv.active)
        {
            Cmd_ForwardToServer();
            return;
        }
        print = Con_Printf;
    }
    else
        print = SV_ClientPrintf;

    print((char*)"host: %s\n", Cvar_VariableString((char*)"hostname"));
    print((char*)"version: %4.2f\n", VERSION);
    if (tcpipAvailable)
        print((char*)"tcp/ip: %s\n", my_tcpip_address);
    if (ipxAvailable)
        print((char*)"ipx: %s\n", my_ipx_address);
    print((char*)"map: %s\n", sv.name);
    print((char*)"players: %i active (%i max)\n\n", net_activeconnections, svs.maxclients);
    for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
    {
        if (!client->active)
            continue;
        seconds = (int)(net_time - client->netconnection->connecttime);
        minutes = seconds / 60;
        if (minutes)
        {
            seconds -= (minutes * 60);
            hours = minutes / 60;
            if (hours)
                minutes -= (hours * 60);
        }
        else
            hours = 0;
        print((char*)"#%-2u %-16.16s %3i %2i:%02i:%02i\n", j + 1, client->name, (int)client->edict->v.frags, hours, minutes, seconds);
        print((char*)" %s\n", client->netconnection->address);
    }
}


/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f()
{
    if (cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (pr_global_struct->deathmatch && !host_client->privileged)
        return;

    sv_player->v.flags = (int)sv_player->v.flags ^ FL_GODMODE;
    if (!((int)sv_player->v.flags & FL_GODMODE))
        SV_ClientPrintf((char*)"godmode OFF\n");
    else
        SV_ClientPrintf((char*)"godmode ON\n");
}

void Host_Notarget_f()
{
    if (cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (pr_global_struct->deathmatch && !host_client->privileged)
        return;

    sv_player->v.flags = (int)sv_player->v.flags ^ FL_NOTARGET;
    if (!((int)sv_player->v.flags & FL_NOTARGET))
        SV_ClientPrintf((char*)"notarget OFF\n");
    else
        SV_ClientPrintf((char*)"notarget ON\n");
}

bool noclip_anglehack;

void Host_Noclip_f()
{
    if (cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (pr_global_struct->deathmatch && !host_client->privileged)
        return;

    if (sv_player->v.movetype != MOVETYPE_NOCLIP)
    {
        noclip_anglehack = true;
        sv_player->v.movetype = MOVETYPE_NOCLIP;
        SV_ClientPrintf((char*)"noclip ON\n");
    }
    else
    {
        noclip_anglehack = false;
        sv_player->v.movetype = MOVETYPE_WALK;
        SV_ClientPrintf((char*)"noclip OFF\n");
    }
}

/*
==================
Host_Fly_f

Sets client to flymode
==================
*/
void Host_Fly_f()
{
    if (cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (pr_global_struct->deathmatch && !host_client->privileged)
        return;

    if (sv_player->v.movetype != MOVETYPE_FLY)
    {
        sv_player->v.movetype = MOVETYPE_FLY;
        SV_ClientPrintf((char*)"flymode ON\n");
    }
    else
    {
        sv_player->v.movetype = MOVETYPE_WALK;
        SV_ClientPrintf((char*)"flymode OFF\n");
    }
}


/*
==================
Host_Ping_f

==================
*/
void Host_Ping_f()
{
    int i, j;
    float total;
    client_t * client;

    if (cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    SV_ClientPrintf((char*)"Client ping times:\n");
    for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
    {
        if (!client->active)
            continue;
        total = 0;
        for (j = 0; j < NUM_PING_TIMES; j++)
            total += client->ping_times[j];
        total /= NUM_PING_TIMES;
        SV_ClientPrintf((char*)"%4i %s\n", (int)(total * 1000), client->name);
    }
}

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/


/*
======================
Host_Map_f

handle a
map <servername>
command from the console. Active clients are kicked off.
======================
*/
void Host_Map_f()
{
    int i;
    char name[MAX_QPATH];

    if (cmd_source != src_command)
        return;

    cls.demonum = -1; // stop demo loop in case this fails

    CL_Disconnect();
    Host_ShutdownServer(false);

    key_dest = key_game; // remove console or menu
    SCR_BeginLoadingPlaque();

    cls.mapstring[0] = 0;
    for (i = 0; i < Cmd_Argc(); i++)
    {
        strcat(cls.mapstring, Cmd_Argv(i));
        strcat(cls.mapstring, " ");
    }
    strcat(cls.mapstring, "\n");

    svs.serverflags = 0; // haven't completed an episode yet
    strcpy(name, Cmd_Argv(1));

    SV_SpawnServer(name);

    if (!sv.active)
        return;

    if (cls.state != ca_dedicated)
    {
        strcpy(cls.spawnparms, "");

        for (i = 2; i < Cmd_Argc(); i++)
        {
            strcat(cls.spawnparms, Cmd_Argv(i));
            strcat(cls.spawnparms, " ");
        }

        Cmd_ExecuteString((char*)"connect local", src_command);
    }
}

/*
==================
Host_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
void Host_Changelevel_f()
{
    char level[MAX_QPATH];

    if (Cmd_Argc() != 2)
    {
        Con_Printf((char*)"changelevel <levelname> : continue game on a new level\n");
        return;
    }
    if (!sv.active || cls.demoplayback)
    {
        Con_Printf((char*)"Only the server may changelevel\n");
        return;
    }
    SV_SaveSpawnparms();
    strcpy(level, Cmd_Argv(1));
    SV_SpawnServer(level);
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f()
{
    char mapname[MAX_QPATH];

    if (cls.demoplayback || !sv.active)
        return;

    if (cmd_source != src_command)
        return;
    strcpy(mapname, sv.name); // must copy out, because it gets cleared
    // in sv_spawnserver
    SV_SpawnServer(mapname);
}

/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
void Host_Reconnect_f()
{
    SCR_BeginLoadingPlaque();
    cls.signon = 0; // need new connection messages
}

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/
void Host_Connect_f()
{
    char name[MAX_QPATH];

    cls.demonum = -1; // stop demo loop in case this fails
    if (cls.demoplayback)
    {
        CL_StopPlayback();
        CL_Disconnect();
    }
    strcpy(name, Cmd_Argv(1));
    CL_EstablishConnection(name);
    Host_Reconnect_f();
}


/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

#define SAVEGAME_VERSION 5

/*
===============
Host_SavegameComment

Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current
===============
*/
void Host_SavegameComment(char * text)
{
    int i;
    char kills[20];

    for (i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
        text[i] = ' ';
    memcpy(text, cl.levelname, strlen(cl.levelname));
    sprintf(kills, "kills:%3i/%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
    memcpy(text + 22, kills, strlen(kills));
    // convert space to _ to make stdio happy
    for (i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
        if (text[i] == ' ')
            text[i] = '_';
    text[SAVEGAME_COMMENT_LENGTH] = '\0';
}


/*
===============
Host_Savegame_f
===============
*/
void Host_Savegame_f()
{
    char name[256];
    FILE * f;
    int i;
    char comment[SAVEGAME_COMMENT_LENGTH + 1];

    if (cmd_source != src_command)
        return;

    if (!sv.active)
    {
        Con_Printf((char*)"Not playing a local game.\n");
        return;
    }

    if (cl.intermission)
    {
        Con_Printf((char*)"Can't save in intermission.\n");
        return;
    }

    if (svs.maxclients != 1)
    {
        Con_Printf((char*)"Can't save multiplayer games.\n");
        return;
    }

    if (Cmd_Argc() != 2)
    {
        Con_Printf((char*)"save <savename> : save a game\n");
        return;
    }

    if (strstr(Cmd_Argv(1), ".."))
    {
        Con_Printf((char*)"Relative pathnames are not allowed.\n");
        return;
    }

    for (i = 0; i < svs.maxclients; i++)
    {
        if (svs.clients[i].active && (svs.clients[i].edict->v.health <= 0))
        {
            Con_Printf((char*)"Can't savegame with a dead player\n");
            return;
        }
    }

    sprintf(name, "%s/%s", com_gamedir, Cmd_Argv(1));
    COM_DefaultExtension(name, (char*)".sav");

    Con_Printf((char*)"Saving game to %s...\n", name);
    f = fopen(name, "w");
    if (!f)
    {
        Con_Printf((char*)"ERROR: couldn't open.\n");
        return;
    }

    fprintf(f, "%i\n", SAVEGAME_VERSION);
    Host_SavegameComment(comment);
    fprintf(f, "%s\n", comment);
    for (i = 0; i < NUM_SPAWN_PARMS; i++)
        fprintf(f, "%f\n", svs.clients->spawn_parms[i]);
    fprintf(f, "%d\n", current_skill);
    fprintf(f, "%s\n", sv.name);
    fprintf(f, "%f\n", sv.time);

    // write the light styles

    for (i = 0; i < MAX_LIGHTSTYLES; i++)
    {
        if (sv.lightstyles[i])
            fprintf(f, "%s\n", sv.lightstyles[i]);
        else
            fprintf(f, "m\n");
    }


    ED_WriteGlobals(f);
    for (i = 0; i < sv.num_edicts; i++)
    {
        ED_Write(f, EDICT_NUM(i));
        fflush(f);
    }
    fclose(f);
    Con_Printf((char*)"done.\n");
}


/*
===============
Host_Loadgame_f
===============
*/
void Host_Loadgame_f()
{
    char name[MAX_OSPATH];
    FILE * f;
    char mapname[MAX_QPATH];
    float time, tfloat;
    char str[32768], * start;
    int i, r;
    edict_t * ent;
    int entnum;
    int version;
    float spawn_parms[NUM_SPAWN_PARMS];

    if (cmd_source != src_command)
        return;

    if (Cmd_Argc() != 2)
    {
        Con_Printf((char*)"load <savename> : load a game\n");
        return;
    }

    cls.demonum = -1; // stop demo loop in case this fails

    sprintf(name, "%s/%s", com_gamedir, Cmd_Argv(1));
    COM_DefaultExtension(name, (char*)".sav");

    // we can't call SCR_BeginLoadingPlaque, because too much stack space has
    // been used. The menu calls it before stuffing loadgame command
    // SCR_BeginLoadingPlaque ();

    Con_Printf((char*)"Loading game from %s...\n", name);
    f = fopen(name, "r");
    if (!f)
    {
        Con_Printf((char*)"ERROR: couldn't open.\n");
        return;
    }

    fscanf(f, "%i\n", &version);
    if (version != SAVEGAME_VERSION)
    {
        fclose(f);
        Con_Printf((char*)"Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
        return;
    }
    fscanf(f, "%s\n", str);
    for (i = 0; i < NUM_SPAWN_PARMS; i++)
        fscanf(f, "%f\n", &spawn_parms[i]);
    // this silliness is so we can load 1.06 save files, which have float skill values
    fscanf(f, "%f\n", &tfloat);
    current_skill = (int)(tfloat + 0.1);
    Cvar_SetValue((char*)"skill", (float)current_skill);

    fscanf(f, "%s\n", mapname);
    fscanf(f, "%f\n", &time);

    CL_Disconnect_f();

    SV_SpawnServer(mapname);
    if (!sv.active)
    {
        Con_Printf((char*)"Couldn't load map\n");
        return;
    }
    sv.paused = true; // pause until all clients connect
    sv.loadgame = true;

    // load the light styles

    for (i = 0; i < MAX_LIGHTSTYLES; i++)
    {
        fscanf(f, "%s\n", str);
        sv.lightstyles[i] = (char*)Hunk_Alloc(strlen(str) + 1);
        strcpy(sv.lightstyles[i], str);
    }

    // load the edicts out of the savegame file
    entnum = -1; // -1 is the globals
    while (!feof(f))
    {
        for (i = 0; i < sizeof(str) - 1; i++)
        {
            r = fgetc(f);
            if (r == EOF || !r)
                break;
            str[i] = r;
            if (r == '}')
            {
                i++;
                break;
            }
        }
        if (i == sizeof(str) - 1)
            Sys_Error((char*)"Loadgame buffer overflow");
        str[i] = 0;
        start = str;
        start = COM_Parse(str);
        if (!com_token[0])
            break; // end of file
        if (strcmp(com_token, "{"))
            Sys_Error((char*)"First token isn't a brace");

        if (entnum == -1)
        { // parse the global vars
            ED_ParseGlobals(start);
        }
        else
        { // parse an edict

            ent = EDICT_NUM(entnum);
            memset(&ent->v, 0, Progs::entityfields * 4);
            ent->free = false;
            ED_ParseEdict(start, ent);

            // link it into the bsp tree
            if (!ent->free)
                SV_LinkEdict(ent, false);
        }

        entnum++;
    }

    sv.num_edicts = entnum;
    sv.time = time;

    fclose(f);

    for (i = 0; i < NUM_SPAWN_PARMS; i++)
        svs.clients->spawn_parms[i] = spawn_parms[i];

    if (cls.state != ca_dedicated)
    {
        CL_EstablishConnection((char*)"local");
        Host_Reconnect_f();
    }
}

//============================================================================

/*
======================
Host_Name_f
======================
*/
void Host_Name_f()
{
    char * newName;

    if (Cmd_Argc() == 1)
    {
        Con_Printf((char*)"\"name\" is \"%s\"\n", cl_name.string);
        return;
    }
    if (Cmd_Argc() == 2)
        newName = Cmd_Argv(1);
    else
        newName = Cmd_Args();
    newName[15] = 0;

    if (cmd_source == src_command)
    {
        if (std::strcmp(cl_name.string, newName) == 0)
            return;
        Cvar_Set((char*)"_cl_name", newName);
        if (cls.state == ca_connected)
            Cmd_ForwardToServer();
        return;
    }

    if (host_client->name[0] && strcmp(host_client->name, "unconnected"))
        if (std::strcmp(host_client->name, newName) != 0)
            Con_Printf((char*)"%s renamed to %s\n", host_client->name, newName);
    std::strcpy(host_client->name, newName);
    host_client->edict->v.netname = host_client->name - pr_strings;

    // send notification to all clients

    MSG_WriteByte(&sv.reliable_datagram, svc_updatename);
    MSG_WriteByte(&sv.reliable_datagram, host_client - svs.clients);
    MSG_WriteString(&sv.reliable_datagram, host_client->name);
}


void Host_Version_f()
{
    Con_Printf((char*)"Version %4.2f\n", VERSION);
    Con_Printf((char*)"Exe: " __TIME__ " " __DATE__ "\n");
}

void Host_Say(bool teamonly)
{
    client_t * client;
    client_t * save;
    int j;
    char * p;
    char text[64];
    bool fromServer = false;

    if (cmd_source == src_command)
    {
        if (cls.state == ca_dedicated)
        {
            fromServer = true;
            teamonly = false;
        }
        else
        {
            Cmd_ForwardToServer();
            return;
        }
    }

    if (Cmd_Argc() < 2)
        return;

    save = host_client;

    p = Cmd_Args();
    // remove quotes if present
    if (*p == '"')
    {
        p++;
        p[std::strlen(p) - 1] = 0;
    }

    // turn on color set 1
    if (!fromServer)
        sprintf(text, "%c%s: ", 1, save->name);
    else
        sprintf(text, "%c<%s> ", 1, hostname.string);

    j = sizeof(text) - 2 - std::strlen(text); // -2 for /n and null terminator
    if (std::strlen(p) > j)
        p[j] = 0;

    strcat(text, p);
    strcat(text, "\n");

    for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
    {
        if (!client || !client->active || !client->spawned)
            continue;
        if (teamplay.value && teamonly && client->edict->v.team != save->edict->v.team)
            continue;
        host_client = client;
        SV_ClientPrintf((char*)"%s", text);
    }
    host_client = save;

    Sys_Printf((char*)"%s", &text[1]);
}


void Host_Say_f()
{
    Host_Say(false);
}


void Host_Say_Team_f()
{
    Host_Say(true);
}


void Host_Tell_f()
{
    client_t * client;
    client_t * save;
    int j;
    char * p;
    char text[64];

    if (cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (Cmd_Argc() < 3)
        return;

    std::strcpy(text, host_client->name);
    std::strcat(text, ": ");

    p = Cmd_Args();

    // remove quotes if present
    if (*p == '"')
    {
        p++;
        p[std::strlen(p) - 1] = 0;
    }

    // check length & truncate if necessary
    j = sizeof(text) - 2 - std::strlen(text); // -2 for /n and null terminator
    if (std::strlen(p) > j)
        p[j] = 0;

    strcat(text, p);
    strcat(text, "\n");

    save = host_client;
    for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
    {
        if (!client->active || !client->spawned)
            continue;
        if (Q_strcasecmp(client->name, Cmd_Argv(1)))
            continue;
        host_client = client;
        SV_ClientPrintf((char*)"%s", text);
        break;
    }
    host_client = save;
}


/*
==================
Host_Color_f
==================
*/
void Host_Color_f()
{
    int top, bottom;
    int playercolor;

    if (Cmd_Argc() == 1)
    {
        Con_Printf((char*)"\"color\" is \"%i %i\"\n", ((int)cl_color.value) >> 4, ((int)cl_color.value) & 0x0f);
        Con_Printf((char*)"color <0-13> [0-13]\n");
        return;
    }

    if (Cmd_Argc() == 2)
        top = bottom = atoi(Cmd_Argv(1));
    else
    {
        top = atoi(Cmd_Argv(1));
        bottom = atoi(Cmd_Argv(2));
    }

    top &= 15;
    if (top > 13)
        top = 13;
    bottom &= 15;
    if (bottom > 13)
        bottom = 13;

    playercolor = top * 16 + bottom;

    if (cmd_source == src_command)
    {
        Cvar_SetValue((char*)"_cl_color", playercolor);
        if (cls.state == ca_connected)
            Cmd_ForwardToServer();
        return;
    }

    host_client->colors = playercolor;
    host_client->edict->v.team = bottom + 1;

    // send notification to all clients
    MSG_WriteByte(&sv.reliable_datagram, svc_updatecolors);
    MSG_WriteByte(&sv.reliable_datagram, host_client - svs.clients);
    MSG_WriteByte(&sv.reliable_datagram, host_client->colors);
}

/*
==================
Host_Kill_f
==================
*/
void Host_Kill_f()
{
    if (cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (sv_player->v.health <= 0)
    {
        SV_ClientPrintf((char*)"Can't suicide -- allready dead!\n");
        return;
    }

    pr_global_struct->time = sv.time;
    pr_global_struct->self = EDICT_TO_PROG(sv_player);
    PR_ExecuteProgram(pr_global_struct->ClientKill);
}


/*
==================
Host_Pause_f
==================
*/
void Host_Pause_f()
{

    if (cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }
    if (!pausable.value)
        SV_ClientPrintf((char*)"Pause not allowed.\n");
    else
    {
        sv.paused ^= 1;

        if (sv.paused)
        {
            SV_BroadcastPrintf((char*)"%s paused the game\n", pr_strings + sv_player->v.netname);
        }
        else
        {
            SV_BroadcastPrintf((char*)"%s unpaused the game\n", pr_strings + sv_player->v.netname);
        }

        // send notification to all clients
        MSG_WriteByte(&sv.reliable_datagram, svc_setpause);
        MSG_WriteByte(&sv.reliable_datagram, sv.paused);
    }
}

//===========================================================================


/*
==================
Host_PreSpawn_f
==================
*/
void Host_PreSpawn_f()
{
    if (cmd_source == src_command)
    {
        Con_Printf((char*)"prespawn is not valid from the console\n");
        return;
    }

    if (host_client->spawned)
    {
        Con_Printf((char*)"prespawn not valid -- allready spawned\n");
        return;
    }

    SZ_Write(&host_client->message, sv.signon.data, sv.signon.cursize);
    MSG_WriteByte(&host_client->message, svc_signonnum);
    MSG_WriteByte(&host_client->message, 2);
    host_client->sendsignon = true;
}

/*
==================
Host_Spawn_f
==================
*/
void Host_Spawn_f()
{
    int i;
    client_t * client;
    edict_t * ent;

    if (cmd_source == src_command)
    {
        Con_Printf((char*)"spawn is not valid from the console\n");
        return;
    }

    if (host_client->spawned)
    {
        Con_Printf((char*)"Spawn not valid -- allready spawned\n");
        return;
    }

    // run the entrance script
    if (sv.loadgame)
    { // loaded games are fully inited allready
    // if this is the last client to be connected, unpause
        sv.paused = false;
    }
    else
    {
        // set up the edict
        ent = host_client->edict;

        memset(&ent->v, 0, Progs::entityfields * 4);
        ent->v.colormap = NUM_FOR_EDICT(ent);
        ent->v.team = (host_client->colors & 15) + 1;
        ent->v.netname = host_client->name - pr_strings;

        // copy spawn parms out of the client_t

        for (i = 0; i < NUM_SPAWN_PARMS; i++)
            (&pr_global_struct->parm1)[i] = host_client->spawn_parms[i];

        // call the spawn function

        pr_global_struct->time = sv.time;
        pr_global_struct->self = EDICT_TO_PROG(sv_player);
        PR_ExecuteProgram(pr_global_struct->ClientConnect);

        if ((Sys_FloatTime() - host_client->netconnection->connecttime) <= sv.time)
            Sys_Printf((char*)"%s entered the game\n", host_client->name);

        PR_ExecuteProgram(pr_global_struct->PutClientInServer);
    }


    // send all current names, colors, and frag counts
    SZ_Clear(&host_client->message);

    // send time of update
    MSG_WriteByte(&host_client->message, svc_time);
    MSG_WriteFloat(&host_client->message, sv.time);

    for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
    {
        MSG_WriteByte(&host_client->message, svc_updatename);
        MSG_WriteByte(&host_client->message, i);
        MSG_WriteString(&host_client->message, client->name);
        MSG_WriteByte(&host_client->message, svc_updatefrags);
        MSG_WriteByte(&host_client->message, i);
        MSG_WriteShort(&host_client->message, client->old_frags);
        MSG_WriteByte(&host_client->message, svc_updatecolors);
        MSG_WriteByte(&host_client->message, i);
        MSG_WriteByte(&host_client->message, client->colors);
    }

    // send all current light styles
    for (i = 0; i < MAX_LIGHTSTYLES; i++)
    {
        MSG_WriteByte(&host_client->message, svc_lightstyle);
        MSG_WriteByte(&host_client->message, (char)i);
        MSG_WriteString(&host_client->message, sv.lightstyles[i]);
    }

    //
    // send some stats
    //
    MSG_WriteByte(&host_client->message, svc_updatestat);
    MSG_WriteByte(&host_client->message, STAT_TOTALSECRETS);
    MSG_WriteLong(&host_client->message, pr_global_struct->total_secrets);

    MSG_WriteByte(&host_client->message, svc_updatestat);
    MSG_WriteByte(&host_client->message, STAT_TOTALMONSTERS);
    MSG_WriteLong(&host_client->message, pr_global_struct->total_monsters);

    MSG_WriteByte(&host_client->message, svc_updatestat);
    MSG_WriteByte(&host_client->message, STAT_SECRETS);
    MSG_WriteLong(&host_client->message, pr_global_struct->found_secrets);

    MSG_WriteByte(&host_client->message, svc_updatestat);
    MSG_WriteByte(&host_client->message, STAT_MONSTERS);
    MSG_WriteLong(&host_client->message, pr_global_struct->killed_monsters);


    //
    // send a fixangle
    // Never send a roll angle, because savegames can catch the server
    // in a state where it is expecting the client to correct the angle
    // and it won't happen if the game was just loaded, so you wind up
    // with a permanent head tilt
    ent = EDICT_NUM(1 + (host_client - svs.clients));
    MSG_WriteByte(&host_client->message, svc_setangle);
    for (i = 0; i < 2; i++)
        MSG_WriteAngle(&host_client->message, ent->v.angles[i]);
    MSG_WriteAngle(&host_client->message, 0);

    SV_WriteClientdataToMessage(sv_player, &host_client->message);

    MSG_WriteByte(&host_client->message, svc_signonnum);
    MSG_WriteByte(&host_client->message, 3);
    host_client->sendsignon = true;
}

/*
==================
Host_Begin_f
==================
*/
void Host_Begin_f()
{
    if (cmd_source == src_command)
    {
        Con_Printf((char*)"begin is not valid from the console\n");
        return;
    }

    host_client->spawned = true;
}

//===========================================================================


/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f()
{
    char * who;
    char * message = NULL;
    client_t * save;
    int i;
    bool byNumber = false;

    if (cmd_source == src_command)
    {
        if (!sv.active)
        {
            Cmd_ForwardToServer();
            return;
        }
    }
    else if (pr_global_struct->deathmatch && !host_client->privileged)
        return;

    save = host_client;

    if (Cmd_Argc() > 2 && std::strcmp(Cmd_Argv(1), "#") == 0)
    {
        i = std::atof(Cmd_Argv(2)) - 1;
        if (i < 0 || i >= svs.maxclients)
            return;
        if (!svs.clients[i].active)
            return;
        host_client = &svs.clients[i];
        byNumber = true;
    }
    else
    {
        for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
        {
            if (!host_client->active)
                continue;
            if (Q_strcasecmp(host_client->name, Cmd_Argv(1)) == 0)
                break;
        }
    }

    if (i < svs.maxclients)
    {
        if (cmd_source == src_command)
            if (cls.state == ca_dedicated)
                who = (char*)"Console";
            else
                who = cl_name.string;
        else
            who = save->name;

        // can't kick yourself!
        if (host_client == save)
            return;

        if (Cmd_Argc() > 2)
        {
            message = COM_Parse(Cmd_Args());
            if (byNumber)
            {
                message++; // skip the #
                while (*message == ' ') // skip white space
                    message++;
                message += std::strlen(Cmd_Argv(2)); // skip the number
            }
            while (*message && *message == ' ')
                message++;
        }
        if (message)
            SV_ClientPrintf((char*)"Kicked by %s: %s\n", who, message);
        else
            SV_ClientPrintf((char*)"Kicked by %s\n", who);
        SV_DropClient(false);
    }

    host_client = save;
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

/*
==================
Host_Give_f
==================
*/
void Host_Give_f()
{
    char * t;
    int v;
    eval_t * val;

    if (cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (pr_global_struct->deathmatch && !host_client->privileged)
        return;

    t = Cmd_Argv(1);
    v = atoi(Cmd_Argv(2));

    switch (t[0])
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        // MED 01/04/97 added hipnotic give stuff
        if constexpr (hipnotic)
        {
            if (t[0] == '6')
            {
                if (t[1] == 'a')
                    sv_player->v.items = (int)sv_player->v.items | HIT_PROXIMITY_GUN;
                else
                    sv_player->v.items = (int)sv_player->v.items | IT_GRENADE_LAUNCHER;
            }
            else if (t[0] == '9')
                sv_player->v.items = (int)sv_player->v.items | HIT_LASER_CANNON;
            else if (t[0] == '0')
                sv_player->v.items = (int)sv_player->v.items | HIT_MJOLNIR;
            else if (t[0] >= '2')
                sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
        }
        else
        {
            if (t[0] >= '2')
                sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
        }
        break;

    case 's':
        if constexpr (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_shells1"sv);
            if (val)
                val->_float = v;
        }

        sv_player->v.ammo_shells = v;
        break;
    case 'n':
        if constexpr (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_nails1"sv);
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon <= IT_LIGHTNING)
                    sv_player->v.ammo_nails = v;
            }
        }
        else
        {
            sv_player->v.ammo_nails = v;
        }
        break;
    case 'l':
        if constexpr (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_lava_nails"sv);
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon > IT_LIGHTNING)
                    sv_player->v.ammo_nails = v;
            }
        }
        break;
    case 'r':
        if constexpr (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_rockets1"sv);
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon <= IT_LIGHTNING)
                    sv_player->v.ammo_rockets = v;
            }
        }
        else
        {
            sv_player->v.ammo_rockets = v;
        }
        break;
    case 'm':
        if constexpr (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_multi_rockets"sv);
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon > IT_LIGHTNING)
                    sv_player->v.ammo_rockets = v;
            }
        }
        break;
    case 'h':
        sv_player->v.health = v;
        break;
    case 'c':
        if constexpr (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_cells1"sv);
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon <= IT_LIGHTNING)
                    sv_player->v.ammo_cells = v;
            }
        }
        else
        {
            sv_player->v.ammo_cells = v;
        }
        break;
    case 'p':
        if constexpr (rogue)
        {
            val = GetEdictFieldValue(sv_player, "ammo_plasma"sv);
            if (val)
            {
                val->_float = v;
                if (sv_player->v.weapon > IT_LIGHTNING)
                    sv_player->v.ammo_cells = v;
            }
        }
        break;
    }
}

edict_t * FindViewthing()
{
    int i;
    edict_t * e;

    for (i = 0; i < sv.num_edicts; i++)
    {
        e = EDICT_NUM(i);
        if (!strcmp(pr_strings + e->v.classname, "viewthing"))
            return e;
    }
    Con_Printf((char*)"No viewthing on map\n");
    return NULL;
}

/*
==================
Host_Viewmodel_f
==================
*/
void Host_Viewmodel_f()
{
    edict_t * e;
    model_t * m;

    e = FindViewthing();
    if (!e)
        return;

    m = Mod_ForName(Cmd_Argv(1), false);
    if (!m)
    {
        Con_Printf((char*)"Can't load %s\n", Cmd_Argv(1));
        return;
    }

    e->v.frame = 0;
    cl.model_precache[(int)e->v.modelindex] = m;
}

/*
==================
Host_Viewframe_f
==================
*/
void Host_Viewframe_f()
{
    edict_t * e;
    int f;
    model_t * m;

    e = FindViewthing();
    if (!e)
        return;
    m = cl.model_precache[(int)e->v.modelindex];

    f = atoi(Cmd_Argv(1));
    if (f >= m->numframes)
        f = m->numframes - 1;

    e->v.frame = f;
}


void PrintFrameName(model_t * m, int frame)
{
    aliashdr_t * hdr;
    maliasframedesc_t * pframedesc;

    hdr = (aliashdr_t *)Mod_Extradata(m);
    if (!hdr)
        return;
    pframedesc = &hdr->frames[frame];

    Con_Printf((char*)"frame %i: %s\n", frame, pframedesc->name);
}

/*
==================
Host_Viewnext_f
==================
*/
void Host_Viewnext_f()
{
    edict_t * e;
    model_t * m;

    e = FindViewthing();
    if (!e)
        return;
    m = cl.model_precache[(int)e->v.modelindex];

    e->v.frame = e->v.frame + 1;
    if (e->v.frame >= m->numframes)
        e->v.frame = m->numframes - 1;

    PrintFrameName(m, e->v.frame);
}

/*
==================
Host_Viewprev_f
==================
*/
void Host_Viewprev_f()
{
    edict_t * e;
    model_t * m;

    e = FindViewthing();
    if (!e)
        return;

    m = cl.model_precache[(int)e->v.modelindex];

    e->v.frame = e->v.frame - 1;
    if (e->v.frame < 0)
        e->v.frame = 0;

    PrintFrameName(m, e->v.frame);
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f()
{
    int i, c;

    if (cls.state == ca_dedicated)
    {
        if (!sv.active)
            Cbuf_AddText((char*)"map start\n");
        return;
    }

    c = Cmd_Argc() - 1;
    if (c > MAX_DEMOS)
    {
        Con_Printf((char*)"Max %i demos in demoloop\n", MAX_DEMOS);
        c = MAX_DEMOS;
    }
    Con_Printf((char*)"%i demo(s) in loop\n", c);

    for (i = 1; i < c + 1; i++)
        strncpy(cls.demos[i - 1], Cmd_Argv(i), sizeof(cls.demos[0]) - 1);

    if (!sv.active && cls.demonum != -1 && !cls.demoplayback)
    {
        cls.demonum = 0;
        CL_NextDemo();
    }
    else
        cls.demonum = -1;
}


/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f()
{
    if (cls.state == ca_dedicated)
        return;
    if (cls.demonum == -1)
        cls.demonum = 1;
    CL_Disconnect_f();
    CL_NextDemo();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f()
{
    if (cls.state == ca_dedicated)
        return;
    if (!cls.demoplayback)
        return;
    CL_StopPlayback();
    CL_Disconnect();
}

//=============================================================================

/*
==================
Host_InitCommands
==================
*/
void Host_InitCommands()
{
    Cmd_AddCommand((char*)"status", Host_Status_f);
    Cmd_AddCommand((char*)"quit", Host_Quit_f);
    Cmd_AddCommand((char*)"god", Host_God_f);
    Cmd_AddCommand((char*)"notarget", Host_Notarget_f);
    Cmd_AddCommand((char*)"fly", Host_Fly_f);
    Cmd_AddCommand((char*)"map", Host_Map_f);
    Cmd_AddCommand((char*)"restart", Host_Restart_f);
    Cmd_AddCommand((char*)"changelevel", Host_Changelevel_f);
    Cmd_AddCommand((char*)"connect", Host_Connect_f);
    Cmd_AddCommand((char*)"reconnect", Host_Reconnect_f);
    Cmd_AddCommand((char*)"name", Host_Name_f);
    Cmd_AddCommand((char*)"noclip", Host_Noclip_f);
    Cmd_AddCommand((char*)"version", Host_Version_f);
    Cmd_AddCommand((char*)"say", Host_Say_f);
    Cmd_AddCommand((char*)"say_team", Host_Say_Team_f);
    Cmd_AddCommand((char*)"tell", Host_Tell_f);
    Cmd_AddCommand((char*)"color", Host_Color_f);
    Cmd_AddCommand((char*)"kill", Host_Kill_f);
    Cmd_AddCommand((char*)"pause", Host_Pause_f);
    Cmd_AddCommand((char*)"spawn", Host_Spawn_f);
    Cmd_AddCommand((char*)"begin", Host_Begin_f);
    Cmd_AddCommand((char*)"prespawn", Host_PreSpawn_f);
    Cmd_AddCommand((char*)"kick", Host_Kick_f);
    Cmd_AddCommand((char*)"ping", Host_Ping_f);
    Cmd_AddCommand((char*)"load", Host_Loadgame_f);
    Cmd_AddCommand((char*)"save", Host_Savegame_f);
    Cmd_AddCommand((char*)"give", Host_Give_f);

    Cmd_AddCommand((char*)"startdemos", Host_Startdemos_f);
    Cmd_AddCommand((char*)"demos", Host_Demos_f);
    Cmd_AddCommand((char*)"stopdemo", Host_Stopdemo_f);

    Cmd_AddCommand((char*)"viewmodel", Host_Viewmodel_f);
    Cmd_AddCommand((char*)"viewframe", Host_Viewframe_f);
    Cmd_AddCommand((char*)"viewnext", Host_Viewnext_f);
    Cmd_AddCommand((char*)"viewprev", Host_Viewprev_f);

    Cmd_AddCommand((char*)"mcache", Mod_Print);
}
