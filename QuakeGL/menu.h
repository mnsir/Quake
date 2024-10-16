

//
// the net drivers should just set the apropriate bits in m_activenet,
// instead of having the menu code look through their internal tables
//
#define MNET_IPX 1
#define MNET_TCP 2

enum MState { m_none, m_main, m_singleplayer, m_load, m_save, m_multiplayer, m_setup, m_net, m_options, m_video, m_keys, m_help, m_quit, m_serialconfig, m_modemconfig, m_lanconfig, m_gameoptions, m_search, m_slist };

extern int m_activenet;

//
// menus
//
void M_Init();
void M_Keydown(int key);
void M_Draw();
void M_ToggleMenu_f();


