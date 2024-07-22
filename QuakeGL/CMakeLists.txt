cmake_minimum_required(VERSION 3.26.0 FATAL_ERROR)

set(this_target "QuakeGL")

project(${this_target})

set(${this_target}_sources
  #"pch.h"
  
  "bspfile.h"
  "cdaudio.h"
  "client.h"
  "cmd.h"
  "common.h"
  "conproc.h"
  "console.h"
  "crc.h"
  "cvar.h"
  "draw.h"
  "gl_model.h"
  "glquake.h"
  "input.h"
  "keys.h"
  "mathlib.h"
  "menu.h"
  "model.h"
  "modelgen.h"
  "net_dgrm.h"
  "net_loop.h"
  "net_vcr.h"
  "net_wins.h"
  "net_wipx.h"
  "net.h"
  "pr_comp.h"
  "progs.h"
  "protocol.h"
  "quakedef.h"
  "render.h"
  "resource.h"
  "sbar.h"
  "screen.h"
  "server.h"
  "sound.h"
  "spritegn.h"
  "sys.h"
  "vid.h"
  "view.h"
  "wad.h"
  "winquake.h"
  "world.h"
  "zone.h"

  "cd_win.c"
  "chase.c"
  "cl_demo.c"
  "cl_input.c"
  "cl_main.c"
  "cl_parse.c"
  "cl_tent.c"
  "cmd.c"
  "common.c"
  "conproc.c"
  "console.c"
  "crc.c"
  "cvar.c"
  "gl_draw.c"
  "gl_mesh.c"
  "gl_model.c"
  "gl_refrag.c"
  "gl_rlight.c"
  "gl_rmain.c"
  "gl_rmisc.c"
  "gl_rsurf.c"
  "gl_screen.c"
  "gl_vidnt.c"
  "gl_warp.c"
  "host_cmd.c"
  "host.c"
  "in_win.c"
  "keys.c"
  "mathlib.c"
  "menu.c"
  "net_dgrm.c"
  "net_loop.c"
  "net_main.c"
  "net_vcr.c"
  "net_win.c"
  "net_wins.c"
  "net_wipx.c"
  "pr_cmds.c"
  "pr_edict.c"
  "pr_exec.c"
  "r_part.c"
  "sbar.c"
  "snd_dma.c"
  "snd_mem.c"
  "snd_mix.c"
  "snd_win.c"
  "sv_main.c"
  "sv_move.c"
  "sv_phys.c"
  "sv_user.c"
  "sys_win.c"
  "view.c"
  "wad.c"
  "world.c"
  "zone.c"
)

add_executable(${this_target} WIN32
  ${${this_target}_sources}
)
# "-startwindowed",
#    //"-playback",
#    //"-startwindowed -record",
#    "-cachedir -width 800 -height 600 -bpp 32 -window -no8bit"

add_compile_definitions(${this_target} PRIVATE GLQUAKE)
#dxguid.lib;winmm.lib;wsock32.lib;opengl32.lib;comctl32.
target_link_libraries(${this_target}
  PRIVATE
  OpenGL::GL
  dxguid
  winmm
  wsock32
  comctl32
)

#add_library(quake::gl ALIAS ${this_target})

#target_precompile_headers(${this_target} PRIVATE pch.h)
