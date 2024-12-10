#include "progs_globals.h"

#include "progs.h"

#include <cstddef>


static_assert(offsetof(Progs::Globals, self) == 28 * sizeof(int));
static_assert(offsetof(Progs::Globals, main) == 82 * sizeof(int));
static_assert(offsetof(Progs::Globals, AS_MELEE) == 370 * sizeof(int));
static_assert(offsetof(Progs::Globals, makevectors) == 447 * sizeof(int));
static_assert(offsetof(Progs::Globals, precache_sound) == 464 * sizeof(int));
static_assert(offsetof(Progs::Globals, cvar_set) == 505 * sizeof(int));
static_assert(offsetof(Progs::Globals, SetMovedir) == 523 * sizeof(int));
static_assert(offsetof(Progs::Globals, InitTrigger) == 559 * sizeof(int));
static_assert(offsetof(Progs::Globals, vdestdelta) == 582 * sizeof(int));
static_assert(offsetof(Progs::Globals, ldmg_1) == 922 * sizeof(int));
static_assert(offsetof(Progs::Globals, targ_5) == 1164 * sizeof(int));
static_assert(offsetof(Progs::Globals, DropBackpack) == 2520 * sizeof(int));
static_assert(offsetof(Progs::Globals, etemp) == 5456 * sizeof(int));
static_assert(offsetof(Progs::Globals, monster_oldone) == 11451 * sizeof(int));

static_assert(sizeof(Progs::Globals) == 11471 * sizeof(int));

Progs::Globals& Progs::GetGlobalStruct()
{
    static Progs::Globals inst;
    return inst;
}
