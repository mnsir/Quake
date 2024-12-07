
#include <common/progs.h>
#include "pr_comp.h" // defs shared with qcc

struct globalvars_t
{
    int pad[28];
    int self;
    int other;
    int world;
    float time;
    float frametime;
    float force_retouch;
    string_t mapname;
    float deathmatch;
    float coop;
    float teamplay;
    float serverflags;
    float total_secrets;
    float total_monsters;
    float found_secrets;
    float killed_monsters;
    float parm1;
    float parm2;
    float parm3;
    float parm4;
    float parm5;
    float parm6;
    float parm7;
    float parm8;
    float parm9;
    float parm10;
    float parm11;
    float parm12;
    float parm13;
    float parm14;
    float parm15;
    float parm16;
    vec3_t v_forward;
    vec3_t v_up;
    vec3_t v_right;
    float trace_allsolid;
    float trace_startsolid;
    float trace_fraction;
    vec3_t trace_endpos;
    vec3_t trace_plane_normal;
    float trace_plane_dist;
    int trace_ent;
    float trace_inopen;
    float trace_inwater;
    int msg_entity;
    func_t main;
    func_t StartFrame;
    func_t PlayerPreThink;
    func_t PlayerPostThink;
    func_t ClientKill;
    func_t ClientConnect;
    func_t PutClientInServer;
    func_t ClientDisconnect;
    func_t SetNewParms;
    func_t SetChangeParms;
};

struct alignas(4) entvars_t
{
    float modelindex;
    vec3_t absmin;
    vec3_t absmax;
    float ltime;
    float movetype;
    float solid;
    vec3_t origin;
    vec3_t oldorigin;
    vec3_t velocity;
    vec3_t angles;
    vec3_t avelocity;
    vec3_t punchangle;
    string_t classname;
    string_t model;
    float frame;
    float skin;
    float effects;
    vec3_t mins;
    vec3_t maxs;
    vec3_t size;
    func_t touch;
    func_t use;
    func_t think;
    func_t blocked;
    float nextthink;
    int groundentity;
    float health;
    float frags;
    float weapon;
    string_t weaponmodel;
    float weaponframe;
    float currentammo;
    float ammo_shells;
    float ammo_nails;
    float ammo_rockets;
    float ammo_cells;
    float items;
    float takedamage;
    int chain;
    float deadflag;
    vec3_t view_ofs;
    float button0;
    float button1;
    float button2;
    float impulse;
    float fixangle;
    vec3_t v_angle;
    float idealpitch;
    string_t netname;
    int enemy;
    float flags;
    float colormap;
    float team;
    float max_health;
    float teleport_time;
    float armortype;
    float armorvalue;
    float waterlevel;
    float watertype;
    float ideal_yaw;
    float yaw_speed;
    int aiment;
    int goalentity;
    float spawnflags;
    string_t target;
    string_t targetname;
    float dmg_take;
    float dmg_save;
    int dmg_inflictor;
    int owner;
    vec3_t movedir;
    string_t message;
    float sounds;
    string_t noise;
    string_t noise1;
    string_t noise2;
    string_t noise3;
    string_t wad;
    string_t map;
    float worldtype;
    string_t killtarget;
    float light_lev;
    float style;
    func_t th_stand;
    func_t th_walk;
    func_t th_run;
    func_t th_missile;
    func_t th_melee;
    func_t th_pain;
    func_t th_die;
    int /*entity_t*/ oldenemy;
    float speed;
    float lefty;
    float search_time;
    float attack_state;
    float walkframe;
    float attack_finished;
    float pain_finished;
    float invincible_finished;
    float invisible_finished;
    float super_damage_finished;
    float radsuit_finished;
    float invincible_time;
    float invincible_sound;
    float invisible_time;
    float invisible_sound;
    float super_time;
    float super_sound;
    float rad_time;
    float fly_sound;
    float axhitme;
    float show_hostile;
    float jump_flag;
    float swim_flag;
    float air_finished;
    float bubble_count;
    string_t deathtype;
    string_t mdl;
    vec3_t mangle;
    float t_length;
    float t_width;
    vec3_t dest;
    vec3_t dest1;
    vec3_t dest2;
    float wait;
    float delay;
    int /*entity_t*/ trigger_field;
    string_t noise4;
    float pausetime;
    int /*entity_t*/ movetarget;
    float aflag;
    float dmg;
    float cnt;
    func_t think1;
    vec3_t finaldest;
    vec3_t finalangle;
    float count;
    float lip;
    float state;
    vec3_t pos1;
    vec3_t pos2;
    float height;
    float waitmin;
    float waitmax;
    float distance;
    float volume;
    float healamount;
    float healtype;
    float hit_z;
    float dmgtime;
    float inpain;
};

union eval_t
{
    string_t string;
    float _float;
    float vector[3];
    func_t function;
    int _int;
    int edict;
};

struct edict_t
{
    bool free;
    link_t area; // linked to a division node or leaf

    int num_leafs;
    std::array<short, 16> leafnums;

    entity_state_t baseline;

    float freetime; // sv.time when the object was freed
    entvars_t v; // C exported fields from progs
    // other fields from progs come immediately after
};
#define EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)

//============================================================================

extern float * pr_globals; // same as pr_global_struct

//============================================================================

void PR_Init();

void PR_ExecuteProgram(func_t fnum);
void PR_LoadProgs();

void PR_Profile_f();

edict_t * ED_Alloc();
void ED_Free(edict_t * ed);

void ED_Print(edict_t * ed);
void ED_Write(FILE * f, edict_t * ed);
char * ED_ParseEdict(char * data, edict_t * ent);

void ED_WriteGlobals(FILE * f);
void ED_ParseGlobals(char * data);

void ED_LoadFromFile(char * data);

#define EDICT_TO_PROG(e) ((byte *)e - (byte *)sv.edicts)
#define PROG_TO_EDICT(e) ((edict_t *)((byte *)sv.edicts + e))

//============================================================================

#define G_FLOAT(o) (pr_globals[o])
#define G_INT(o) (*(int *)&pr_globals[o])
#define G_EDICT(o) ((edict_t *)((byte *)sv.edicts+ *(int *)&pr_globals[o]))
#define G_EDICTNUM(o) std::distance(sv.edicts, G_EDICT(o))
#define G_VECTOR(o) (&pr_globals[o])
#define G_STRING(o) (Progs::FromStringOffset(*(string_t *)&pr_globals[o]))
#define G_FUNCTION(o) (*(func_t *)&pr_globals[o])

extern int pr_argc;

extern bool pr_trace;
extern const Progs::dfunction_t * pr_xfunction;
extern int pr_xstatement;

void PR_RunError(char * error, ...);

void ED_PrintEdicts();
void ED_PrintNum(int ent);

eval_t * GetEdictFieldValue(edict_t * ed, std::string_view field);

namespace Progs
{
    string_t ToStringOffset(char* str);
    char* FromStringOffset(string_t offset);

    using BuiltIn = void(*)();
    std::span<const BuiltIn> GetBuiltIns();

    ::globalvars_t& GetGlobalStruct();
}
