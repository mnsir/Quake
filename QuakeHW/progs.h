
#include <common/progs.h>
#include "pr_comp.h" // defs shared with qcc

typedef struct
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
} globalvars_t;

typedef struct
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
} entvars_t;

#define PROGHEADER_CRC 5927

typedef union eval_s
{
    string_t string;
    float _float;
    float vector[3];
    func_t function;
    int _int;
    int edict;
} eval_t;

#define MAX_ENT_LEAFS 16
typedef struct edict_s
{
    bool free;
    link_t area; // linked to a division node or leaf

    int num_leafs;
    short leafnums[MAX_ENT_LEAFS];

    entity_state_t baseline;

    float freetime; // sv.time when the object was freed
    entvars_t v; // C exported fields from progs
    // other fields from progs come immediately after
} edict_t;
#define EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)

//============================================================================

extern dfunction_t * pr_functions;
extern char * pr_strings;
extern dstatement_t * pr_statements;
extern globalvars_t * pr_global_struct;
extern float * pr_globals; // same as pr_global_struct
namespace Progs
{
    constexpr int edict_size = entityfields * 4 + sizeof(edict_t) - sizeof(entvars_t);
}
//============================================================================

void PR_Init();

void PR_ExecuteProgram(func_t fnum);
void PR_LoadProgs();

void PR_Profile_f();

edict_t * ED_Alloc();
void ED_Free(edict_t * ed);

char * ED_NewString(char * string);
// returns a copy of the string allocated from the server's string heap

void ED_Print(edict_t * ed);
void ED_Write(FILE * f, edict_t * ed);
char * ED_ParseEdict(char * data, edict_t * ent);

void ED_WriteGlobals(FILE * f);
void ED_ParseGlobals(const char * data);

void ED_LoadFromFile(char * data);

//define EDICT_NUM(n) ((edict_t *)(sv.edicts+ (n)*pr_edict_size))
//define NUM_FOR_EDICT(e) (((byte *)(e) - sv.edicts)/pr_edict_size)

edict_t * EDICT_NUM(int n);
int NUM_FOR_EDICT(edict_t * e);
edict_t* NEXT_EDICT(edict_t* e);

#define EDICT_TO_PROG(e) ((byte *)e - (byte *)sv.edicts)
#define PROG_TO_EDICT(e) ((edict_t *)((byte *)sv.edicts + e))

//============================================================================

#define G_FLOAT(o) (pr_globals[o])
#define G_INT(o) (*(int *)&pr_globals[o])
#define G_EDICT(o) ((edict_t *)((byte *)sv.edicts+ *(int *)&pr_globals[o]))
#define G_EDICTNUM(o) NUM_FOR_EDICT(G_EDICT(o))
#define G_VECTOR(o) (&pr_globals[o])
#define G_STRING(o) (pr_strings + *(string_t *)&pr_globals[o])
#define G_FUNCTION(o) (*(func_t *)&pr_globals[o])

#define E_FLOAT(e,o) (((float*)&e->v)[o])
#define E_INT(e,o) (*(int *)&((float*)&e->v)[o])
#define E_VECTOR(e,o) (&((float*)&e->v)[o])
#define E_STRING(e,o) (pr_strings + *(string_t *)&((float*)&e->v)[o])

extern int type_size[8];

typedef void (*builtin_t) ();
extern builtin_t * pr_builtins;
extern int pr_numbuiltins;

extern int pr_argc;

extern bool pr_trace;
extern dfunction_t * pr_xfunction;
extern int pr_xstatement;

void PR_RunError(char * error, ...);

void ED_PrintEdicts();
void ED_PrintNum(int ent);

eval_t * GetEdictFieldValue(edict_t * ed, char * field);

