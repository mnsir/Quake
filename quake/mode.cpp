#include "mode.h"
#include "args.h"


enum class Mode
{
    Standard,
    Rogue,
    Hipnotic,
};

Mode mode_ = Mode::Standard;


void Mode_Init()
{

    if (Args_GetIndex("-rogue"))
    {
        mode_ = Mode::Rogue;
    }
    else if (Args_GetIndex("-hipnotic"))
    {
        mode_ = Mode::Hipnotic;
    }
}


int Mode_IsStandard() { return mode_ == Mode::Standard; }
int Mode_IsRogue() { return mode_ == Mode::Rogue; }
int Mode_IsHipnotic() { return mode_ == Mode::Hipnotic; }