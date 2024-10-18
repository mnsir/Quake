
// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"
#include "r_local.h"

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

byte palette[] =
"\x00\x00\x00\x0f\x0f\x0f\x1f\x1f\x1f\x2f\x2f\x2f\x3f\x3f\x3f\x4b\x4b\x4b\x5b\x5b\x5b\x6b\x6b\x6b"
"\x7b\x7b\x7b\x8b\x8b\x8b\x9b\x9b\x9b\xab\xab\xab\xbb\xbb\xbb\xcb\xcb\xcb\xdb\xdb\xdb\xeb\xeb\xeb"
"\x0f\x0b\x07\x17\x0f\x0b\x1f\x17\x0b\x27\x1b\x0f\x2f\x23\x13\x37\x2b\x17\x3f\x2f\x17\x4b\x37\x1b"
"\x53\x3b\x1b\x5b\x43\x1f\x63\x4b\x1f\x6b\x53\x1f\x73\x57\x1f\x7b\x5f\x23\x83\x67\x23\x8f\x6f\x23"
"\x0b\x0b\x0f\x13\x13\x1b\x1b\x1b\x27\x27\x27\x33\x2f\x2f\x3f\x37\x37\x4b\x3f\x3f\x57\x47\x47\x67"
"\x4f\x4f\x73\x5b\x5b\x7f\x63\x63\x8b\x6b\x6b\x97\x73\x73\xa3\x7b\x7b\xaf\x83\x83\xbb\x8b\x8b\xcb"
"\x00\x00\x00\x07\x07\x00\x0b\x0b\x00\x13\x13\x00\x1b\x1b\x00\x23\x23\x00\x2b\x2b\x07\x2f\x2f\x07"
"\x37\x37\x07\x3f\x3f\x07\x47\x47\x07\x4b\x4b\x0b\x53\x53\x0b\x5b\x5b\x0b\x63\x63\x0b\x6b\x6b\x0f"
"\x07\x00\x00\x0f\x00\x00\x17\x00\x00\x1f\x00\x00\x27\x00\x00\x2f\x00\x00\x37\x00\x00\x3f\x00\x00"
"\x47\x00\x00\x4f\x00\x00\x57\x00\x00\x5f\x00\x00\x67\x00\x00\x6f\x00\x00\x77\x00\x00\x7f\x00\x00"
"\x13\x13\x00\x1b\x1b\x00\x23\x23\x00\x2f\x2b\x00\x37\x2f\x00\x43\x37\x00\x4b\x3b\x07\x57\x43\x07"
"\x5f\x47\x07\x6b\x4b\x0b\x77\x53\x0f\x83\x57\x13\x8b\x5b\x13\x97\x5f\x1b\xa3\x63\x1f\xaf\x67\x23"
"\x23\x13\x07\x2f\x17\x0b\x3b\x1f\x0f\x4b\x23\x13\x57\x2b\x17\x63\x2f\x1f\x73\x37\x23\x7f\x3b\x2b"
"\x8f\x43\x33\x9f\x4f\x33\xaf\x63\x2f\xbf\x77\x2f\xcf\x8f\x2b\xdf\xab\x27\xef\xcb\x1f\xff\xf3\x1b"
"\x0b\x07\x00\x1b\x13\x00\x2b\x23\x0f\x37\x2b\x13\x47\x33\x1b\x53\x37\x23\x63\x3f\x2b\x6f\x47\x33"
"\x7f\x53\x3f\x8b\x5f\x47\x9b\x6b\x53\xa7\x7b\x5f\xb7\x87\x6b\xc3\x93\x7b\xd3\xa3\x8b\xe3\xb3\x97"
"\xab\x8b\xa3\x9f\x7f\x97\x93\x73\x87\x8b\x67\x7b\x7f\x5b\x6f\x77\x53\x63\x6b\x4b\x57\x5f\x3f\x4b"
"\x57\x37\x43\x4b\x2f\x37\x43\x27\x2f\x37\x1f\x23\x2b\x17\x1b\x23\x13\x13\x17\x0b\x0b\x0f\x07\x07"
"\xbb\x73\x9f\xaf\x6b\x8f\xa3\x5f\x83\x97\x57\x77\x8b\x4f\x6b\x7f\x4b\x5f\x73\x43\x53\x6b\x3b\x4b"
"\x5f\x33\x3f\x53\x2b\x37\x47\x23\x2b\x3b\x1f\x23\x2f\x17\x1b\x23\x13\x13\x17\x0b\x0b\x0f\x07\x07"
"\xdb\xc3\xbb\xcb\xb3\xa7\xbf\xa3\x9b\xaf\x97\x8b\xa3\x87\x7b\x97\x7b\x6f\x87\x6f\x5f\x7b\x63\x53"
"\x6b\x57\x47\x5f\x4b\x3b\x53\x3f\x33\x43\x33\x27\x37\x2b\x1f\x27\x1f\x17\x1b\x13\x0f\x0f\x0b\x07"
"\x6f\x83\x7b\x67\x7b\x6f\x5f\x73\x67\x57\x6b\x5f\x4f\x63\x57\x47\x5b\x4f\x3f\x53\x47\x37\x4b\x3f"
"\x2f\x43\x37\x2b\x3b\x2f\x23\x33\x27\x1f\x2b\x1f\x17\x23\x17\x0f\x1b\x13\x0b\x13\x0b\x07\x0b\x07"
"\xff\xf3\x1b\xef\xdf\x17\xdb\xcb\x13\xcb\xb7\x0f\xbb\xa7\x0f\xab\x97\x0b\x9b\x83\x07\x8b\x73\x07"
"\x7b\x63\x07\x6b\x53\x00\x5b\x47\x00\x4b\x37\x00\x3b\x2b\x00\x2b\x1f\x00\x1b\x0f\x00\x0b\x07\x00"
"\x00\x00\xff\x0b\x0b\xef\x13\x13\xdf\x1b\x1b\xcf\x23\x23\xbf\x2b\x2b\xaf\x2f\x2f\x9f\x2f\x2f\x8f"
"\x2f\x2f\x7f\x2f\x2f\x6f\x2f\x2f\x5f\x2b\x2b\x4f\x23\x23\x3f\x1b\x1b\x2f\x13\x13\x1f\x0b\x0b\x0f"
"\x2b\x00\x00\x3b\x00\x00\x4b\x07\x00\x5f\x07\x00\x6f\x0f\x00\x7f\x17\x07\x93\x1f\x07\xa3\x27\x0b"
"\xb7\x33\x0f\xc3\x4b\x1b\xcf\x63\x2b\xdb\x7f\x3b\xe3\x97\x4f\xe7\xab\x5f\xef\xbf\x77\xf7\xd3\x8b"
"\xa7\x7b\x3b\xb7\x9b\x37\xc7\xc3\x37\xe7\xe3\x57\x7f\xbf\xff\xab\xe7\xff\xd7\xff\xff\x67\x00\x00"
"\x8b\x00\x00\xb3\x00\x00\xd7\x00\x00\xff\x00\x00\xff\xf3\x93\xff\xf7\xc7\xff\xff\xff\x9f\x5b\x53"
;

quakeparms_t host_parms;

bool host_initialized; // true if into command execution

double host_frametime;
double host_time;
double realtime; // without any filtering or bounding
double oldrealtime; // last frame run
int host_framecount;

int host_hunklevel;

client_t * host_client; // current client

jmp_buf host_abortserver;

byte * host_basepal;
byte * host_colormap;

cvar_t host_framerate = {(char*)"host_framerate", (char*)"0"}; // set for slow motion
cvar_t host_speeds = {(char*)"host_speeds", (char*)"0"}; // set for running times

cvar_t sys_ticrate = {(char*)"sys_ticrate", (char*)"0.05"};
cvar_t serverprofile = {(char*)"serverprofile", (char*)"0"};

cvar_t fraglimit = {(char*)"fraglimit", (char*)"0", false, true};
cvar_t timelimit = {(char*)"timelimit", (char*)"0", false, true};
cvar_t teamplay = {(char*)"teamplay", (char*)"0", false, true};

cvar_t samelevel = {(char*)"samelevel", (char*)"0"};
cvar_t noexit = {(char*)"noexit", (char*)"0", false, true};

cvar_t developer = {(char*)"developer", (char*)"0"};

cvar_t skill = {(char*)"skill", (char*)"1"}; // 0 - 3
cvar_t deathmatch = {(char*)"deathmatch", (char*)"0"}; // 0, 1, or 2
cvar_t coop = {(char*)"coop", (char*)"0"}; // 0 or 1

cvar_t pausable = {(char*)"pausable", (char*)"1"};

cvar_t temp1 = {(char*)"temp1", (char*)"0"};


/*
================
Host_EndGame
================
*/
void Host_EndGame(char * message, ...)
{
    va_list argptr;
    char string[1024];

    va_start(argptr, message);
    vsprintf(string, message, argptr);
    va_end(argptr);
    Con_DPrintf((char*)"Host_EndGame: %s\n", string);

    if (sv.active)
        Host_ShutdownServer(false);

    if (cls.state == ca_dedicated)
        Sys_Error((char*)"Host_EndGame: %s\n", string); // dedicated servers exit

    if (cls.demonum != -1)
        CL_NextDemo();
    else
        CL_Disconnect();

    longjmp(host_abortserver, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error(char * error, ...)
{
    va_list argptr;
    char string[1024];
    static bool inerror = false;

    if (inerror)
        Sys_Error((char*)"Host_Error: recursively entered");
    inerror = true;

    SCR_EndLoadingPlaque(); // reenable screen updates

    va_start(argptr, error);
    vsprintf(string, error, argptr);
    va_end(argptr);
    Con_Printf((char*)"Host_Error: %s\n", string);

    if (sv.active)
        Host_ShutdownServer(false);

    if (cls.state == ca_dedicated)
        Sys_Error((char*)"Host_Error: %s\n", string); // dedicated servers exit

    CL_Disconnect();
    cls.demonum = -1;

    inerror = false;

    longjmp(host_abortserver, 1);
}

/*
================
Host_FindMaxClients
================
*/
void Host_FindMaxClients()
{
    int i;

    svs.maxclients = 1;

    i = COM_CheckParm((char*)"-dedicated");
    if (i)
    {
        cls.state = ca_dedicated;
        if (i != (com_argc - 1))
        {
            svs.maxclients = std::atoi(com_argv[i + 1]);
        }
        else
            svs.maxclients = 8;
    }
    else
        cls.state = ca_disconnected;

    i = COM_CheckParm((char*)"-listen");
    if (i)
    {
        if (cls.state == ca_dedicated)
            Sys_Error((char*)"Only one of -dedicated or -listen can be specified");
        if (i != (com_argc - 1))
            svs.maxclients = std::atoi(com_argv[i + 1]);
        else
            svs.maxclients = 8;
    }
    if (svs.maxclients < 1)
        svs.maxclients = 8;
    else if (svs.maxclients > MAX_SCOREBOARD)
        svs.maxclients = MAX_SCOREBOARD;

    svs.maxclientslimit = svs.maxclients;
    if (svs.maxclientslimit < 4)
        svs.maxclientslimit = 4;
    svs.clients = (client_t*)Hunk_AllocName(svs.maxclientslimit * sizeof(client_t), (char*)"clients");

    if (svs.maxclients > 1)
        Cvar_SetValue((char*)"deathmatch", 1.0);
    else
        Cvar_SetValue((char*)"deathmatch", 0.0);
}


/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal()
{
    Host_InitCommands();

    Cvar_RegisterVariable(&host_framerate);
    Cvar_RegisterVariable(&host_speeds);

    Cvar_RegisterVariable(&sys_ticrate);
    Cvar_RegisterVariable(&serverprofile);

    Cvar_RegisterVariable(&fraglimit);
    Cvar_RegisterVariable(&timelimit);
    Cvar_RegisterVariable(&teamplay);
    Cvar_RegisterVariable(&samelevel);
    Cvar_RegisterVariable(&noexit);
    Cvar_RegisterVariable(&skill);
    Cvar_RegisterVariable(&developer);
    Cvar_RegisterVariable(&deathmatch);
    Cvar_RegisterVariable(&coop);

    Cvar_RegisterVariable(&pausable);

    Cvar_RegisterVariable(&temp1);

    Host_FindMaxClients();

    host_time = 1.0; // so a think at time 0 won't get called
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration()
{
    FILE * f;

    // dedicated servers initialize the host but don't parse and set the
    // config.cfg cvars
    if (host_initialized & !isDedicated)
    {
        f = fopen(va((char*)"%s/config.cfg", com_gamedir), "w");
        if (!f)
        {
            Con_Printf((char*)"Couldn't write config.cfg.\n");
            return;
        }

        Key_WriteBindings(f);
        Cvar_WriteVariables(f);

        fclose(f);
    }
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf(char * fmt, ...)
{
    va_list argptr;
    char string[1024];

    va_start(argptr, fmt);
    vsprintf(string, fmt, argptr);
    va_end(argptr);

    MSG_WriteByte(&host_client->message, svc_print);
    MSG_WriteString(&host_client->message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf(char * fmt, ...)
{
    va_list argptr;
    char string[1024];
    int i;

    va_start(argptr, fmt);
    vsprintf(string, fmt, argptr);
    va_end(argptr);

    for (i = 0; i < svs.maxclients; i++)
        if (svs.clients[i].active && svs.clients[i].spawned)
        {
            MSG_WriteByte(&svs.clients[i].message, svc_print);
            MSG_WriteString(&svs.clients[i].message, string);
        }
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands(char * fmt, ...)
{
    va_list argptr;
    char string[1024];

    va_start(argptr, fmt);
    vsprintf(string, fmt, argptr);
    va_end(argptr);

    MSG_WriteByte(&host_client->message, svc_stufftext);
    MSG_WriteString(&host_client->message, string);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void SV_DropClient(bool crash)
{
    int saveSelf;
    int i;
    client_t * client;

    if (!crash)
    {
        // send any final messages (don't check for errors)
        if (NET_CanSendMessage(host_client->netconnection))
        {
            MSG_WriteByte(&host_client->message, svc_disconnect);
            NET_SendMessage(host_client->netconnection, &host_client->message);
        }

        if (host_client->edict && host_client->spawned)
        {
            // call the prog function for removing a client
            // this will set the body to a dead frame, among other things
            saveSelf = pr_global_struct->self;
            pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
            PR_ExecuteProgram(pr_global_struct->ClientDisconnect);
            pr_global_struct->self = saveSelf;
        }

        Sys_Printf((char*)"Client %s removed\n", host_client->name);
    }

    // break the net connection
    NET_Close(host_client->netconnection);
    host_client->netconnection = NULL;

    // free the client (the body stays around)
    host_client->active = false;
    host_client->name[0] = 0;
    host_client->old_frags = -999999;
    net_activeconnections--;

    // send notification to all clients
    for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
    {
        if (!client->active)
            continue;
        MSG_WriteByte(&client->message, svc_updatename);
        MSG_WriteByte(&client->message, host_client - svs.clients);
        MSG_WriteString(&client->message, (char*)"");
        MSG_WriteByte(&client->message, svc_updatefrags);
        MSG_WriteByte(&client->message, host_client - svs.clients);
        MSG_WriteShort(&client->message, 0);
        MSG_WriteByte(&client->message, svc_updatecolors);
        MSG_WriteByte(&client->message, host_client - svs.clients);
        MSG_WriteByte(&client->message, 0);
    }
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer(bool crash)
{
    int i;
    int count;
    sizebuf_t buf;
    char message[4];
    double start;

    if (!sv.active)
        return;

    sv.active = false;

    // stop all client sounds immediately
    if (cls.state == ca_connected)
        CL_Disconnect();

    // flush any pending messages - like the score!!!
    start = Sys_FloatTime();
    do
    {
        count = 0;
        for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
        {
            if (host_client->active && host_client->message.cursize)
            {
                if (NET_CanSendMessage(host_client->netconnection))
                {
                    NET_SendMessage(host_client->netconnection, &host_client->message);
                    SZ_Clear(&host_client->message);
                }
                else
                {
                    NET_GetMessage(host_client->netconnection);
                    count++;
                }
            }
        }
        if ((Sys_FloatTime() - start) > 3.0)
            break;
    } while (count);

    // make sure all the clients know we're disconnecting
    buf.data = (byte*)message;
    buf.maxsize = 4;
    buf.cursize = 0;
    MSG_WriteByte(&buf, svc_disconnect);
    count = NET_SendToAll(&buf, 5);
    if (count)
        Con_Printf((char*)"Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count);

    for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
        if (host_client->active)
            SV_DropClient(crash);

    //
    // clear structures
    //
    memset(&sv, 0, sizeof(sv));
    memset(svs.clients, 0, svs.maxclientslimit * sizeof(client_t));
}


/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory()
{
    Con_DPrintf((char*)"Clearing memory\n");
    D_FlushCaches();
    Mod_ClearAll();
    if (host_hunklevel)
        Hunk_FreeToLowMark(host_hunklevel);

    cls.signon = 0;
    memset(&sv, 0, sizeof(sv));
    memset(&cl, 0, sizeof(cl));
}


//============================================================================


/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
bool Host_FilterTime(float time)
{
    realtime += time;

    if (!cls.timedemo && realtime - oldrealtime < 1.0 / 72.0)
        return false; // framerate is too high

    host_frametime = realtime - oldrealtime;
    oldrealtime = realtime;

    if (host_framerate.value > 0)
        host_frametime = host_framerate.value;
    else
    { // don't allow really long or short frames
        if (host_frametime > 0.1)
            host_frametime = 0.1;
        if (host_frametime < 0.001)
            host_frametime = 0.001;
    }

    return true;
}


/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands()
{
    char * cmd;

    while (1)
    {
        cmd = Sys_ConsoleInput();
        if (!cmd)
            break;
        Cbuf_AddText(cmd);
    }
}

void Host_ServerFrame()
{
    // run the world state 
    pr_global_struct->frametime = host_frametime;

    // set the time and clear the general datagram
    SV_ClearDatagram();

    // check for new clients
    SV_CheckForNewClients();

    // read client messages
    SV_RunClients();

    // move things around and think
    // always pause in single player if in console or menus
    if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game))
        SV_Physics();

    // send all messages to the clients
    SV_SendClientMessages();
}

/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame(float time)
{
    static double time1 = 0;
    static double time2 = 0;
    static double time3 = 0;
    int pass1, pass2, pass3;

    if (setjmp(host_abortserver))
        return; // something bad happened, or the server disconnected

    // keep the random time dependent
    rand();

    // decide the simulation time
    if (!Host_FilterTime(time))
        return; // don't run too fast, or packets will flood out

    // get new key events
    Sys_SendKeyEvents();

    // allow mice or other external controllers to add commands
    IN_Commands();

    // process console commands
    Cbuf_Execute();

    NET_Poll();

    // if running the server locally, make intentions now
    if (sv.active)
        CL_SendCmd();

    //-------------------
    //
    // server operations
    //
    //-------------------

    // check for commands typed to the host
    Host_GetConsoleCommands();

    if (sv.active)
        Host_ServerFrame();

    //-------------------
    //
    // client operations
    //
    //-------------------

    // if running the server remotely, send intentions now after
    // the incoming messages have been read
    if (!sv.active)
        CL_SendCmd();

    host_time += host_frametime;

    // fetch results from server
    if (cls.state == ca_connected)
    {
        CL_ReadFromServer();
    }

    // update video
    if (host_speeds.value)
        time1 = Sys_FloatTime();

    SCR_UpdateScreen();

    if (host_speeds.value)
        time2 = Sys_FloatTime();

    // update audio
    if (cls.signon == SIGNONS)
    {
        S_Update(r_origin, vpn, vright, vup);
        CL_DecayLights();
    }
    else
        S_Update(vec3_origin, vec3_origin, vec3_origin, vec3_origin);

    CDAudio_Update();

    if (host_speeds.value)
    {
        pass1 = (time1 - time3) * 1000;
        time3 = Sys_FloatTime();
        pass2 = (time2 - time1) * 1000;
        pass3 = (time3 - time2) * 1000;
        Con_Printf((char*)"%3i tot %3i server %3i gfx %3i snd\n",
                   pass1 + pass2 + pass3, pass1, pass2, pass3);
    }

    host_framecount++;
}

void Host_Frame(float time)
{
    double time1, time2;
    static double timetotal;
    static int timecount;
    int i, c, m;

    if (!serverprofile.value)
    {
        _Host_Frame(time);
        return;
    }

    time1 = Sys_FloatTime();
    _Host_Frame(time);
    time2 = Sys_FloatTime();

    timetotal += time2 - time1;
    timecount++;

    if (timecount < 1000)
        return;

    m = timetotal * 1000 / timecount;
    timecount = 0;
    timetotal = 0;
    c = 0;
    for (i = 0; i < svs.maxclients; i++)
    {
        if (svs.clients[i].active)
            c++;
    }

    Con_Printf((char*)"serverprofile: %2i clients %2i msec\n", c, m);
}

//============================================================================


extern int vcrFile;
#define VCR_SIGNATURE 0x56435231
// "VCR1"

void Host_InitVCR(char* arg0)
{
    int i, len, n;
    char * p;

    if (COM_CheckParm((char*)"-playback"))
    {
        if (com_argc != 2)
            Sys_Error((char*)"No other parameters allowed with -playback\n");

        Sys_FileOpenRead((char*)"quake.vcr", &vcrFile);
        if (vcrFile == -1)
            Sys_Error((char*)"playback file not found\n");

        Sys_FileRead(vcrFile, &i, sizeof(int));
        if (i != VCR_SIGNATURE)
            Sys_Error((char*)"Invalid signature in vcr file\n");

        Sys_FileRead(vcrFile, &com_argc, sizeof(int));
        com_argv = (char**)malloc(com_argc * sizeof(char *));
        com_argv[0] = arg0;
        for (i = 0; i < com_argc; i++)
        {
            Sys_FileRead(vcrFile, &len, sizeof(int));
            p = (char*)malloc(len);
            Sys_FileRead(vcrFile, p, len);
            com_argv[i + 1] = p;
        }
        com_argc++; /* add one for arg[0] */
        host_parms.argc = com_argc;
        host_parms.argv = com_argv;
    }

    if ((n = COM_CheckParm((char*)"-record")) != 0)
    {
        vcrFile = Sys_FileOpenWrite((char*)"quake.vcr");

        i = VCR_SIGNATURE;
        Sys_FileWrite(vcrFile, &i, sizeof(int));
        i = com_argc - 1;
        Sys_FileWrite(vcrFile, &i, sizeof(int));
        for (i = 1; i < com_argc; i++)
        {
            if (i == n)
            {
                len = 10;
                Sys_FileWrite(vcrFile, &len, sizeof(int));
                Sys_FileWrite(vcrFile, (char*)"-playback", len);
                continue;
            }
            len = std::strlen(com_argv[i]) + 1;
            Sys_FileWrite(vcrFile, &len, sizeof(int));
            Sys_FileWrite(vcrFile, com_argv[i], len);
        }
    }

}

/*
====================
Host_Init
====================
*/
void Host_Init(const quakeparms_t& parms)
{
    host_parms = std::move(parms);
    
    if (host_parms.mem.size() < minimum_memory)
        Sys_Error((char*)"Only %4.1f megs of memory available, can't execute game", host_parms.mem.size() / (float)0x100000);

    com_argc = host_parms.argc;
    com_argv = host_parms.argv;

    Memory_Init(host_parms.mem.data(), host_parms.mem.size());
    Cbuf_Init();
    Cmd_Init();
    V_Init();
    Chase_Init();
    Host_InitVCR(host_parms.argv[0]);
    COM_Init(host_parms.basedir);
    Host_InitLocal();
    W_LoadWadFile((char*)"gfx.wad");
    Key_Init();
    Con_Init();
    M_Init();
    PR_Init();
    Mod_Init();
    NET_Init();
    SV_Init();

    Con_Printf((char*)"Exe: " __TIME__ " " __DATE__ "\n");
    Con_Printf((char*)"%4.1f megabyte heap\n", host_parms.mem.size() / (1024 * 1024.0));

    R_InitTextures(); // needed even for dedicated servers

    if (cls.state != ca_dedicated)
    {
        host_basepal = palette;
        host_colormap = (byte *)COM_LoadHunkFile((char*)"gfx/colormap.lmp");
        if (!host_colormap)
            Sys_Error((char*)"Couldn't load gfx/colormap.lmp");

        VID_Init(host_basepal);

        Draw_Init();
        SCR_Init();
        R_Init();

#ifdef GLQUAKE
        // FIXME: doesn't use the new one-window approach yet
        S_Init();
#endif

        CDAudio_Init();
        Sbar_Init();
        CL_Init();
        IN_Init();
    }

    Cbuf_InsertText((char*)"exec quake.rc\n");

    Hunk_AllocName(0, (char*)"-HOST_HUNKLEVEL-");
    host_hunklevel = Hunk_LowMark();

    host_initialized = true;

    Sys_Printf((char*)"========Quake Initialized=========\n");
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error. It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown()
{
    static bool isdown = false;

    if (isdown)
    {
        printf("recursive shutdown\n");
        return;
    }
    isdown = true;

    // keep Con_Printf from trying to update the screen
    scr_disabled_for_loading = true;

    Host_WriteConfiguration();

    CDAudio_Shutdown();
    NET_Shutdown();
    S_Shutdown();
    IN_Shutdown();

    if (cls.state != ca_dedicated)
    {
        VID_Shutdown();
    }
}

