#pragma once

namespace pak
{
    unsigned char* default_cfg();
    unsigned char* demo1_dem();
    unsigned char* demo2_dem();
    unsigned char* demo3_dem();
    unsigned char* end1_bin();
    unsigned char* gfx_wad();
    unsigned char* progs_dat();
    unsigned char* quake_rc();
    namespace gfx
    {
        unsigned char* bigbox_lmp();
        unsigned char* box_bl_lmp();
        unsigned char* box_bm_lmp();
        unsigned char* box_br_lmp();
        unsigned char* box_ml_lmp();
        unsigned char* box_mm_lmp();
        unsigned char* box_mm2_lmp();
        unsigned char* box_mr_lmp();
        unsigned char* box_tl_lmp();
        unsigned char* box_tm_lmp();
        unsigned char* box_tr_lmp();
        unsigned char* colormap_lmp();
        unsigned char* complete_lmp();
        unsigned char* conback_lmp();
        unsigned char* dim_drct_lmp();
        unsigned char* dim_ipx_lmp();
        unsigned char* dim_modm_lmp();
        unsigned char* dim_mult_lmp();
        unsigned char* dim_tcp_lmp();
        unsigned char* finale_lmp();
        unsigned char* help0_lmp();
        unsigned char* help1_lmp();
        unsigned char* help2_lmp();
        unsigned char* help3_lmp();
        unsigned char* help4_lmp();
        unsigned char* help5_lmp();
        unsigned char* inter_lmp();
        unsigned char* loading_lmp();
        unsigned char* mainmenu_lmp();
        unsigned char* menudot1_lmp();
        unsigned char* menudot2_lmp();
        unsigned char* menudot3_lmp();
        unsigned char* menudot4_lmp();
        unsigned char* menudot5_lmp();
        unsigned char* menudot6_lmp();
        unsigned char* menuplyr_lmp();
        unsigned char* mp_menu_lmp();
        unsigned char* netmen1_lmp();
        unsigned char* netmen2_lmp();
        unsigned char* netmen3_lmp();
        unsigned char* netmen4_lmp();
        unsigned char* netmen5_lmp();
        unsigned char* p_load_lmp();
        unsigned char* p_multi_lmp();
        unsigned char* p_option_lmp();
        unsigned char* p_save_lmp();
        unsigned char* palette_lmp();
        unsigned char* pause_lmp();
        unsigned char* qplaque_lmp();
        unsigned char* ranking_lmp();
        unsigned char* sell_lmp();
        unsigned char* sp_menu_lmp();
        unsigned char* ttl_cstm_lmp();
        unsigned char* ttl_main_lmp();
        unsigned char* ttl_sgl_lmp();
        unsigned char* vidmodes_lmp();
    }
    namespace maps
    {
        unsigned char* b_batt0_bsp();
        unsigned char* b_batt1_bsp();
        unsigned char* b_bh10_bsp();
        unsigned char* b_bh100_bsp();
        unsigned char* b_bh25_bsp();
        unsigned char* b_explob_bsp();
        unsigned char* b_nail0_bsp();
        unsigned char* b_nail1_bsp();
        unsigned char* b_rock0_bsp();
        unsigned char* b_rock1_bsp();
        unsigned char* b_shell0_bsp();
        unsigned char* b_shell1_bsp();
        unsigned char* e1m1_bsp();
        unsigned char* e1m2_bsp();
        unsigned char* e1m3_bsp();
        unsigned char* e1m4_bsp();
        unsigned char* e1m5_bsp();
        unsigned char* e1m6_bsp();
        unsigned char* e1m7_bsp();
        unsigned char* e1m8_bsp();
        unsigned char* start_bsp();
    }
    namespace progs
    {
        unsigned char* armor_mdl();
        unsigned char* backpack_mdl();
        unsigned char* bolt_mdl();
        unsigned char* bolt2_mdl();
        unsigned char* bolt3_mdl();
        unsigned char* boss_mdl();
        unsigned char* demon_mdl();
        unsigned char* dog_mdl();
        unsigned char* end1_mdl();
        unsigned char* eyes_mdl();
        unsigned char* flame_mdl();
        unsigned char* flame2_mdl();
        unsigned char* g_light_mdl();
        unsigned char* g_nail_mdl();
        unsigned char* g_nail2_mdl();
        unsigned char* g_rock_mdl();
        unsigned char* g_rock2_mdl();
        unsigned char* g_shot_mdl();
        unsigned char* gib1_mdl();
        unsigned char* gib2_mdl();
        unsigned char* gib3_mdl();
        unsigned char* grenade_mdl();
        unsigned char* h_demon_mdl();
        unsigned char* h_dog_mdl();
        unsigned char* h_guard_mdl();
        unsigned char* h_knight_mdl();
        unsigned char* h_ogre_mdl();
        unsigned char* h_player_mdl();
        unsigned char* h_shams_mdl();
        unsigned char* h_wizard_mdl();
        unsigned char* h_zombie_mdl();
        unsigned char* invisibl_mdl();
        unsigned char* invulner_mdl();
        unsigned char* knight_mdl();
        unsigned char* lavaball_mdl();
        unsigned char* m_g_key_mdl();
        unsigned char* m_s_key_mdl();
        unsigned char* missile_mdl();
        unsigned char* ogre_mdl();
        unsigned char* player_mdl();
        unsigned char* quaddama_mdl();
        unsigned char* s_bubble_spr();
        unsigned char* s_explod_spr();
        unsigned char* s_light_mdl();
        unsigned char* s_light_spr();
        unsigned char* s_spike_mdl();
        unsigned char* shambler_mdl();
        unsigned char* soldier_mdl();
        unsigned char* spike_mdl();
        unsigned char* suit_mdl();
        unsigned char* v_axe_mdl();
        unsigned char* v_light_mdl();
        unsigned char* v_nail_mdl();
        unsigned char* v_nail2_mdl();
        unsigned char* v_rock_mdl();
        unsigned char* v_rock2_mdl();
        unsigned char* v_shot_mdl();
        unsigned char* v_shot2_mdl();
        unsigned char* w_g_key_mdl();
        unsigned char* w_s_key_mdl();
        unsigned char* w_spike_mdl();
        unsigned char* wizard_mdl();
        unsigned char* zom_gib_mdl();
        unsigned char* zombie_mdl();
    }
    namespace sound
    {
        namespace ambience
        {
            unsigned char* buzz1_wav();
            unsigned char* comp1_wav();
            unsigned char* drip1_wav();
            unsigned char* drone6_wav();
            unsigned char* fire1_wav();
            unsigned char* fl_hum1_wav();
            unsigned char* hum1_wav();
            unsigned char* suck1_wav();
            unsigned char* swamp1_wav();
            unsigned char* swamp2_wav();
            unsigned char* thunder1_wav();
            unsigned char* water1_wav();
            unsigned char* wind2_wav();
            unsigned char* windfly_wav();
        }
        namespace boss1
        {
            unsigned char* death_wav();
            unsigned char* out1_wav();
            unsigned char* pain_wav();
            unsigned char* sight1_wav();
            unsigned char* throw_wav();
        }
        namespace buttons
        {
            unsigned char* airbut1_wav();
            unsigned char* switch02_wav();
            unsigned char* switch04_wav();
            unsigned char* switch21_wav();
        }
        namespace demon
        {
            unsigned char* ddeath_wav();
            unsigned char* dhit2_wav();
            unsigned char* djump_wav();
            unsigned char* dland2_wav();
            unsigned char* dpain1_wav();
            unsigned char* idle1_wav();
            unsigned char* sight2_wav();
        }
        namespace dog
        {
            unsigned char* dattack1_wav();
            unsigned char* ddeath_wav();
            unsigned char* dpain1_wav();
            unsigned char* dsight_wav();
            unsigned char* idle_wav();
        }
        namespace doors
        {
            unsigned char* airdoor1_wav();
            unsigned char* airdoor2_wav();
            unsigned char* basesec1_wav();
            unsigned char* basesec2_wav();
            unsigned char* basetry_wav();
            unsigned char* baseuse_wav();
            unsigned char* ddoor1_wav();
            unsigned char* ddoor2_wav();
            unsigned char* doormv1_wav();
            unsigned char* drclos4_wav();
            unsigned char* hydro1_wav();
            unsigned char* hydro2_wav();
            unsigned char* latch2_wav();
            unsigned char* medtry_wav();
            unsigned char* meduse_wav();
            unsigned char* runetry_wav();
            unsigned char* runeuse_wav();
            unsigned char* stndr1_wav();
            unsigned char* stndr2_wav();
            unsigned char* winch2_wav();
        }
        namespace hknight
        {
            unsigned char* hit_wav();
        }
        namespace items
        {
            unsigned char* armor1_wav();
            unsigned char* damage_wav();
            unsigned char* damage2_wav();
            unsigned char* damage3_wav();
            unsigned char* health1_wav();
            unsigned char* inv1_wav();
            unsigned char* inv2_wav();
            unsigned char* inv3_wav();
            unsigned char* itembk2_wav();
            unsigned char* protect_wav();
            unsigned char* protect2_wav();
            unsigned char* protect3_wav();
            unsigned char* r_item1_wav();
            unsigned char* r_item2_wav();
            unsigned char* suit_wav();
            unsigned char* suit2_wav();
        }
        namespace knight
        {
            unsigned char* idle_wav();
            unsigned char* kdeath_wav();
            unsigned char* khurt_wav();
            unsigned char* ksight_wav();
            unsigned char* sword1_wav();
            unsigned char* sword2_wav();
        }
        namespace misc
        {
            unsigned char* h2ohit1_wav();
            unsigned char* medkey_wav();
            unsigned char* menu1_wav();
            unsigned char* menu2_wav();
            unsigned char* menu3_wav();
            unsigned char* null_wav();
            unsigned char* outwater_wav();
            unsigned char* power_wav();
            unsigned char* r_tele1_wav();
            unsigned char* r_tele2_wav();
            unsigned char* r_tele3_wav();
            unsigned char* r_tele4_wav();
            unsigned char* r_tele5_wav();
            unsigned char* runekey_wav();
            unsigned char* secret_wav();
            unsigned char* talk_wav();
            unsigned char* trigger1_wav();
            unsigned char* water1_wav();
            unsigned char* water2_wav();
        }
        namespace ogre
        {
            unsigned char* ogdrag_wav();
            unsigned char* ogdth_wav();
            unsigned char* ogidle_wav();
            unsigned char* ogidle2_wav();
            unsigned char* ogpain1_wav();
            unsigned char* ogsawatk_wav();
            unsigned char* ogwake_wav();
        }
        namespace plats
        {
            unsigned char* medplat1_wav();
            unsigned char* medplat2_wav();
            unsigned char* plat1_wav();
            unsigned char* plat2_wav();
            unsigned char* train1_wav();
            unsigned char* train2_wav();
        }
        namespace player
        {
            unsigned char* axhit1_wav();
            unsigned char* axhit2_wav();
            unsigned char* death1_wav();
            unsigned char* death2_wav();
            unsigned char* death3_wav();
            unsigned char* death4_wav();
            unsigned char* death5_wav();
            unsigned char* drown1_wav();
            unsigned char* drown2_wav();
            unsigned char* gasp1_wav();
            unsigned char* gasp2_wav();
            unsigned char* gib_wav();
            unsigned char* h2odeath_wav();
            unsigned char* h2ojump_wav();
            unsigned char* inh2o_wav();
            unsigned char* inlava_wav();
            unsigned char* land_wav();
            unsigned char* land2_wav();
            unsigned char* lburn1_wav();
            unsigned char* lburn2_wav();
            unsigned char* pain1_wav();
            unsigned char* pain2_wav();
            unsigned char* pain3_wav();
            unsigned char* pain4_wav();
            unsigned char* pain5_wav();
            unsigned char* pain6_wav();
            unsigned char* plyrjmp8_wav();
            unsigned char* slimbrn2_wav();
            unsigned char* teledth1_wav();
            unsigned char* tornoff2_wav();
            unsigned char* udeath_wav();
        }
        namespace shambler
        {
            unsigned char* melee1_wav();
            unsigned char* melee2_wav();
            unsigned char* sattck1_wav();
            unsigned char* sboom_wav();
            unsigned char* sdeath_wav();
            unsigned char* shurt2_wav();
            unsigned char* sidle_wav();
            unsigned char* smack_wav();
            unsigned char* ssight_wav();
        }
        namespace soldier
        {
            unsigned char* death1_wav();
            unsigned char* idle_wav();
            unsigned char* pain1_wav();
            unsigned char* pain2_wav();
            unsigned char* sattck1_wav();
            unsigned char* sight1_wav();
        }
        namespace weapons
        {
            unsigned char* ax1_wav();
            unsigned char* bounce_wav();
            unsigned char* grenade_wav();
            unsigned char* guncock_wav();
            unsigned char* lhit_wav();
            unsigned char* lock4_wav();
            unsigned char* lstart_wav();
            unsigned char* pkup_wav();
            unsigned char* r_exp3_wav();
            unsigned char* ric1_wav();
            unsigned char* ric2_wav();
            unsigned char* ric3_wav();
            unsigned char* rocket1i_wav();
            unsigned char* sgun1_wav();
            unsigned char* shotgn2_wav();
            unsigned char* spike2_wav();
            unsigned char* tink1_wav();
        }
        namespace wizard
        {
            unsigned char* hit_wav();
            unsigned char* wattack_wav();
            unsigned char* wdeath_wav();
            unsigned char* widle1_wav();
            unsigned char* widle2_wav();
            unsigned char* wpain_wav();
            unsigned char* wsight_wav();
        }
        namespace zombie
        {
            unsigned char* idle_w2_wav();
            unsigned char* z_fall_wav();
            unsigned char* z_gib_wav();
            unsigned char* z_hit_wav();
            unsigned char* z_idle_wav();
            unsigned char* z_idle1_wav();
            unsigned char* z_miss_wav();
            unsigned char* z_pain_wav();
            unsigned char* z_pain1_wav();
            unsigned char* z_shot1_wav();
        }
    }
}
