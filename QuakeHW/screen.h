
// screen.h

void SCR_Init();

void SCR_UpdateScreen();


void SCR_SizeUp();
void SCR_SizeDown();
void SCR_BringDownConsole();
void SCR_CenterPrint(char * str);

void SCR_BeginLoadingPlaque();
void SCR_EndLoadingPlaque();

int SCR_ModalMessage(char * text);

extern float scr_con_current;
extern float scr_conlines; // lines of console to display

extern int scr_fullupdate; // set to 0 to force full redraw
extern int sb_lines;

extern int clearnotify; // set to 0 whenever notify text is drawn
extern bool scr_disabled_for_loading;
extern bool scr_skipupdate;

extern cvar_t scr_viewsize;

extern cvar_t scr_viewsize;

// only the refresh window will be updated unless these variables are flagged 
extern int scr_copytop;
extern int scr_copyeverything;

extern bool block_drawing;

void SCR_UpdateWholeScreen();
