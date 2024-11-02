#pragma once
#include <span>

namespace pak
{
    std::span<unsigned char> default_cfg();
    std::span<unsigned char> demo1_dem();
    std::span<unsigned char> demo2_dem();
    std::span<unsigned char> demo3_dem();
    std::span<unsigned char> end1_bin();
    std::span<unsigned char> gfx_wad();
    std::span<unsigned char> progs_dat();
    std::span<unsigned char> quake_rc();
    namespace gfx
    {
        std::span<unsigned char> bigbox_lmp();
        std::span<unsigned char> box_bl_lmp();
        std::span<unsigned char> box_bm_lmp();
        std::span<unsigned char> box_br_lmp();
        std::span<unsigned char> box_ml_lmp();
        std::span<unsigned char> box_mm_lmp();
        std::span<unsigned char> box_mm2_lmp();
        std::span<unsigned char> box_mr_lmp();
        std::span<unsigned char> box_tl_lmp();
        std::span<unsigned char> box_tm_lmp();
        std::span<unsigned char> box_tr_lmp();
        std::span<unsigned char> colormap_lmp();
        std::span<unsigned char> complete_lmp();
        std::span<unsigned char> conback_lmp();
        std::span<unsigned char> dim_drct_lmp();
        std::span<unsigned char> dim_ipx_lmp();
        std::span<unsigned char> dim_modm_lmp();
        std::span<unsigned char> dim_mult_lmp();
        std::span<unsigned char> dim_tcp_lmp();
        std::span<unsigned char> finale_lmp();
        std::span<unsigned char> help0_lmp();
        std::span<unsigned char> help1_lmp();
        std::span<unsigned char> help2_lmp();
        std::span<unsigned char> help3_lmp();
        std::span<unsigned char> help4_lmp();
        std::span<unsigned char> help5_lmp();
        std::span<unsigned char> inter_lmp();
        std::span<unsigned char> loading_lmp();
        std::span<unsigned char> mainmenu_lmp();
        std::span<unsigned char> menudot1_lmp();
        std::span<unsigned char> menudot2_lmp();
        std::span<unsigned char> menudot3_lmp();
        std::span<unsigned char> menudot4_lmp();
        std::span<unsigned char> menudot5_lmp();
        std::span<unsigned char> menudot6_lmp();
        std::span<unsigned char> menuplyr_lmp();
        std::span<unsigned char> mp_menu_lmp();
        std::span<unsigned char> netmen1_lmp();
        std::span<unsigned char> netmen2_lmp();
        std::span<unsigned char> netmen3_lmp();
        std::span<unsigned char> netmen4_lmp();
        std::span<unsigned char> netmen5_lmp();
        std::span<unsigned char> p_load_lmp();
        std::span<unsigned char> p_multi_lmp();
        std::span<unsigned char> p_option_lmp();
        std::span<unsigned char> p_save_lmp();
        std::span<unsigned char> palette_lmp();
        std::span<unsigned char> pause_lmp();
        std::span<unsigned char> qplaque_lmp();
        std::span<unsigned char> ranking_lmp();
        std::span<unsigned char> sell_lmp();
        std::span<unsigned char> sp_menu_lmp();
        std::span<unsigned char> ttl_cstm_lmp();
        std::span<unsigned char> ttl_main_lmp();
        std::span<unsigned char> ttl_sgl_lmp();
        std::span<unsigned char> vidmodes_lmp();
    }
    namespace maps
    {
        std::span<unsigned char> b_batt0_bsp();
        std::span<unsigned char> b_batt1_bsp();
        std::span<unsigned char> b_bh10_bsp();
        std::span<unsigned char> b_bh100_bsp();
        std::span<unsigned char> b_bh25_bsp();
        std::span<unsigned char> b_explob_bsp();
        std::span<unsigned char> b_nail0_bsp();
        std::span<unsigned char> b_nail1_bsp();
        std::span<unsigned char> b_rock0_bsp();
        std::span<unsigned char> b_rock1_bsp();
        std::span<unsigned char> b_shell0_bsp();
        std::span<unsigned char> b_shell1_bsp();
        std::span<unsigned char> e1m1_bsp();
        std::span<unsigned char> e1m2_bsp();
        std::span<unsigned char> e1m3_bsp();
        std::span<unsigned char> e1m4_bsp();
        std::span<unsigned char> e1m5_bsp();
        std::span<unsigned char> e1m6_bsp();
        std::span<unsigned char> e1m7_bsp();
        std::span<unsigned char> e1m8_bsp();
        std::span<unsigned char> start_bsp();
    }
    namespace progs
    {
        std::span<unsigned char> armor_mdl();
        std::span<unsigned char> backpack_mdl();
        std::span<unsigned char> bolt_mdl();
        std::span<unsigned char> bolt2_mdl();
        std::span<unsigned char> bolt3_mdl();
        std::span<unsigned char> boss_mdl();
        std::span<unsigned char> demon_mdl();
        std::span<unsigned char> dog_mdl();
        std::span<unsigned char> end1_mdl();
        std::span<unsigned char> eyes_mdl();
        std::span<unsigned char> flame_mdl();
        std::span<unsigned char> flame2_mdl();
        std::span<unsigned char> g_light_mdl();
        std::span<unsigned char> g_nail_mdl();
        std::span<unsigned char> g_nail2_mdl();
        std::span<unsigned char> g_rock_mdl();
        std::span<unsigned char> g_rock2_mdl();
        std::span<unsigned char> g_shot_mdl();
        std::span<unsigned char> gib1_mdl();
        std::span<unsigned char> gib2_mdl();
        std::span<unsigned char> gib3_mdl();
        std::span<unsigned char> grenade_mdl();
        std::span<unsigned char> h_demon_mdl();
        std::span<unsigned char> h_dog_mdl();
        std::span<unsigned char> h_guard_mdl();
        std::span<unsigned char> h_knight_mdl();
        std::span<unsigned char> h_ogre_mdl();
        std::span<unsigned char> h_player_mdl();
        std::span<unsigned char> h_shams_mdl();
        std::span<unsigned char> h_wizard_mdl();
        std::span<unsigned char> h_zombie_mdl();
        std::span<unsigned char> invisibl_mdl();
        std::span<unsigned char> invulner_mdl();
        std::span<unsigned char> knight_mdl();
        std::span<unsigned char> lavaball_mdl();
        std::span<unsigned char> m_g_key_mdl();
        std::span<unsigned char> m_s_key_mdl();
        std::span<unsigned char> missile_mdl();
        std::span<unsigned char> ogre_mdl();
        std::span<unsigned char> player_mdl();
        std::span<unsigned char> quaddama_mdl();
        std::span<unsigned char> s_bubble_spr();
        std::span<unsigned char> s_explod_spr();
        std::span<unsigned char> s_light_mdl();
        std::span<unsigned char> s_light_spr();
        std::span<unsigned char> s_spike_mdl();
        std::span<unsigned char> shambler_mdl();
        std::span<unsigned char> soldier_mdl();
        std::span<unsigned char> spike_mdl();
        std::span<unsigned char> suit_mdl();
        std::span<unsigned char> v_axe_mdl();
        std::span<unsigned char> v_light_mdl();
        std::span<unsigned char> v_nail_mdl();
        std::span<unsigned char> v_nail2_mdl();
        std::span<unsigned char> v_rock_mdl();
        std::span<unsigned char> v_rock2_mdl();
        std::span<unsigned char> v_shot_mdl();
        std::span<unsigned char> v_shot2_mdl();
        std::span<unsigned char> w_g_key_mdl();
        std::span<unsigned char> w_s_key_mdl();
        std::span<unsigned char> w_spike_mdl();
        std::span<unsigned char> wizard_mdl();
        std::span<unsigned char> zom_gib_mdl();
        std::span<unsigned char> zombie_mdl();
    }
    namespace sound
    {
        namespace ambience
        {
            std::span<unsigned char> buzz1_wav();
            std::span<unsigned char> comp1_wav();
            std::span<unsigned char> drip1_wav();
            std::span<unsigned char> drone6_wav();
            std::span<unsigned char> fire1_wav();
            std::span<unsigned char> fl_hum1_wav();
            std::span<unsigned char> hum1_wav();
            std::span<unsigned char> suck1_wav();
            std::span<unsigned char> swamp1_wav();
            std::span<unsigned char> swamp2_wav();
            std::span<unsigned char> thunder1_wav();
            std::span<unsigned char> water1_wav();
            std::span<unsigned char> wind2_wav();
            std::span<unsigned char> windfly_wav();
        }
        namespace boss1
        {
            std::span<unsigned char> death_wav();
            std::span<unsigned char> out1_wav();
            std::span<unsigned char> pain_wav();
            std::span<unsigned char> sight1_wav();
            std::span<unsigned char> throw_wav();
        }
        namespace buttons
        {
            std::span<unsigned char> airbut1_wav();
            std::span<unsigned char> switch02_wav();
            std::span<unsigned char> switch04_wav();
            std::span<unsigned char> switch21_wav();
        }
        namespace demon
        {
            std::span<unsigned char> ddeath_wav();
            std::span<unsigned char> dhit2_wav();
            std::span<unsigned char> djump_wav();
            std::span<unsigned char> dland2_wav();
            std::span<unsigned char> dpain1_wav();
            std::span<unsigned char> idle1_wav();
            std::span<unsigned char> sight2_wav();
        }
        namespace dog
        {
            std::span<unsigned char> dattack1_wav();
            std::span<unsigned char> ddeath_wav();
            std::span<unsigned char> dpain1_wav();
            std::span<unsigned char> dsight_wav();
            std::span<unsigned char> idle_wav();
        }
        namespace doors
        {
            std::span<unsigned char> airdoor1_wav();
            std::span<unsigned char> airdoor2_wav();
            std::span<unsigned char> basesec1_wav();
            std::span<unsigned char> basesec2_wav();
            std::span<unsigned char> basetry_wav();
            std::span<unsigned char> baseuse_wav();
            std::span<unsigned char> ddoor1_wav();
            std::span<unsigned char> ddoor2_wav();
            std::span<unsigned char> doormv1_wav();
            std::span<unsigned char> drclos4_wav();
            std::span<unsigned char> hydro1_wav();
            std::span<unsigned char> hydro2_wav();
            std::span<unsigned char> latch2_wav();
            std::span<unsigned char> medtry_wav();
            std::span<unsigned char> meduse_wav();
            std::span<unsigned char> runetry_wav();
            std::span<unsigned char> runeuse_wav();
            std::span<unsigned char> stndr1_wav();
            std::span<unsigned char> stndr2_wav();
            std::span<unsigned char> winch2_wav();
        }
        namespace hknight
        {
            std::span<unsigned char> hit_wav();
        }
        namespace items
        {
            std::span<unsigned char> armor1_wav();
            std::span<unsigned char> damage_wav();
            std::span<unsigned char> damage2_wav();
            std::span<unsigned char> damage3_wav();
            std::span<unsigned char> health1_wav();
            std::span<unsigned char> inv1_wav();
            std::span<unsigned char> inv2_wav();
            std::span<unsigned char> inv3_wav();
            std::span<unsigned char> itembk2_wav();
            std::span<unsigned char> protect_wav();
            std::span<unsigned char> protect2_wav();
            std::span<unsigned char> protect3_wav();
            std::span<unsigned char> r_item1_wav();
            std::span<unsigned char> r_item2_wav();
            std::span<unsigned char> suit_wav();
            std::span<unsigned char> suit2_wav();
        }
        namespace knight
        {
            std::span<unsigned char> idle_wav();
            std::span<unsigned char> kdeath_wav();
            std::span<unsigned char> khurt_wav();
            std::span<unsigned char> ksight_wav();
            std::span<unsigned char> sword1_wav();
            std::span<unsigned char> sword2_wav();
        }
        namespace misc
        {
            std::span<unsigned char> h2ohit1_wav();
            std::span<unsigned char> medkey_wav();
            std::span<unsigned char> menu1_wav();
            std::span<unsigned char> menu2_wav();
            std::span<unsigned char> menu3_wav();
            std::span<unsigned char> null_wav();
            std::span<unsigned char> outwater_wav();
            std::span<unsigned char> power_wav();
            std::span<unsigned char> r_tele1_wav();
            std::span<unsigned char> r_tele2_wav();
            std::span<unsigned char> r_tele3_wav();
            std::span<unsigned char> r_tele4_wav();
            std::span<unsigned char> r_tele5_wav();
            std::span<unsigned char> runekey_wav();
            std::span<unsigned char> secret_wav();
            std::span<unsigned char> talk_wav();
            std::span<unsigned char> trigger1_wav();
            std::span<unsigned char> water1_wav();
            std::span<unsigned char> water2_wav();
        }
        namespace ogre
        {
            std::span<unsigned char> ogdrag_wav();
            std::span<unsigned char> ogdth_wav();
            std::span<unsigned char> ogidle_wav();
            std::span<unsigned char> ogidle2_wav();
            std::span<unsigned char> ogpain1_wav();
            std::span<unsigned char> ogsawatk_wav();
            std::span<unsigned char> ogwake_wav();
        }
        namespace plats
        {
            std::span<unsigned char> medplat1_wav();
            std::span<unsigned char> medplat2_wav();
            std::span<unsigned char> plat1_wav();
            std::span<unsigned char> plat2_wav();
            std::span<unsigned char> train1_wav();
            std::span<unsigned char> train2_wav();
        }
        namespace player
        {
            std::span<unsigned char> axhit1_wav();
            std::span<unsigned char> axhit2_wav();
            std::span<unsigned char> death1_wav();
            std::span<unsigned char> death2_wav();
            std::span<unsigned char> death3_wav();
            std::span<unsigned char> death4_wav();
            std::span<unsigned char> death5_wav();
            std::span<unsigned char> drown1_wav();
            std::span<unsigned char> drown2_wav();
            std::span<unsigned char> gasp1_wav();
            std::span<unsigned char> gasp2_wav();
            std::span<unsigned char> gib_wav();
            std::span<unsigned char> h2odeath_wav();
            std::span<unsigned char> h2ojump_wav();
            std::span<unsigned char> inh2o_wav();
            std::span<unsigned char> inlava_wav();
            std::span<unsigned char> land_wav();
            std::span<unsigned char> land2_wav();
            std::span<unsigned char> lburn1_wav();
            std::span<unsigned char> lburn2_wav();
            std::span<unsigned char> pain1_wav();
            std::span<unsigned char> pain2_wav();
            std::span<unsigned char> pain3_wav();
            std::span<unsigned char> pain4_wav();
            std::span<unsigned char> pain5_wav();
            std::span<unsigned char> pain6_wav();
            std::span<unsigned char> plyrjmp8_wav();
            std::span<unsigned char> slimbrn2_wav();
            std::span<unsigned char> teledth1_wav();
            std::span<unsigned char> tornoff2_wav();
            std::span<unsigned char> udeath_wav();
        }
        namespace shambler
        {
            std::span<unsigned char> melee1_wav();
            std::span<unsigned char> melee2_wav();
            std::span<unsigned char> sattck1_wav();
            std::span<unsigned char> sboom_wav();
            std::span<unsigned char> sdeath_wav();
            std::span<unsigned char> shurt2_wav();
            std::span<unsigned char> sidle_wav();
            std::span<unsigned char> smack_wav();
            std::span<unsigned char> ssight_wav();
        }
        namespace soldier
        {
            std::span<unsigned char> death1_wav();
            std::span<unsigned char> idle_wav();
            std::span<unsigned char> pain1_wav();
            std::span<unsigned char> pain2_wav();
            std::span<unsigned char> sattck1_wav();
            std::span<unsigned char> sight1_wav();
        }
        namespace weapons
        {
            std::span<unsigned char> ax1_wav();
            std::span<unsigned char> bounce_wav();
            std::span<unsigned char> grenade_wav();
            std::span<unsigned char> guncock_wav();
            std::span<unsigned char> lhit_wav();
            std::span<unsigned char> lock4_wav();
            std::span<unsigned char> lstart_wav();
            std::span<unsigned char> pkup_wav();
            std::span<unsigned char> r_exp3_wav();
            std::span<unsigned char> ric1_wav();
            std::span<unsigned char> ric2_wav();
            std::span<unsigned char> ric3_wav();
            std::span<unsigned char> rocket1i_wav();
            std::span<unsigned char> sgun1_wav();
            std::span<unsigned char> shotgn2_wav();
            std::span<unsigned char> spike2_wav();
            std::span<unsigned char> tink1_wav();
        }
        namespace wizard
        {
            std::span<unsigned char> hit_wav();
            std::span<unsigned char> wattack_wav();
            std::span<unsigned char> wdeath_wav();
            std::span<unsigned char> widle1_wav();
            std::span<unsigned char> widle2_wav();
            std::span<unsigned char> wpain_wav();
            std::span<unsigned char> wsight_wav();
        }
        namespace zombie
        {
            std::span<unsigned char> idle_w2_wav();
            std::span<unsigned char> z_fall_wav();
            std::span<unsigned char> z_gib_wav();
            std::span<unsigned char> z_hit_wav();
            std::span<unsigned char> z_idle_wav();
            std::span<unsigned char> z_idle1_wav();
            std::span<unsigned char> z_miss_wav();
            std::span<unsigned char> z_pain_wav();
            std::span<unsigned char> z_pain1_wav();
            std::span<unsigned char> z_shot1_wav();
        }
    }
}
