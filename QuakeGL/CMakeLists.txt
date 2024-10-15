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

  "cd_win.cpp"
  "chase.cpp"
  "cl_demo.cpp"
  "cl_input.cpp"
  "cl_main.cpp"
  "cl_parse.cpp"
  "cl_tent.cpp"
  "cmd.cpp"
  "common.cpp"
  "conproc.cpp"
  "console.cpp"
  "crc.cpp"
  "cvar.cpp"
  "gl_draw.cpp"
  "gl_mesh.cpp"
  "gl_model.cpp"
  "gl_refrag.cpp"
  "gl_rlight.cpp"
  "gl_rmain.cpp"
  "gl_rmisc.cpp"
  "gl_rsurf.cpp"
  "gl_screen.cpp"
  "gl_vidnt.cpp"
  "gl_warp.cpp"
  "host_cmd.cpp"
  "host.cpp"
  "in_win.cpp"
  "keys.cpp"
  "mathlib.cpp"
  "menu.cpp"
  "net_dgrm.cpp"
  "net_loop.cpp"
  "net_main.cpp"
  "net_vcr.cpp"
  "net_win.cpp"
  "net_wins.cpp"
  "net_wipx.cpp"
  "pr_cmds.cpp"
  "pr_edict.cpp"
  "pr_exec.cpp"
  "r_part.cpp"
  "sbar.cpp"
  "snd_dma.cpp"
  "snd_mem.cpp"
  "snd_mix.cpp"
  "snd_win.cpp"
  "sv_main.cpp"
  "sv_move.cpp"
  "sv_phys.cpp"
  "sv_user.cpp"
  "sys_win.cpp"
  "view.cpp"
  "wad.cpp"
  "world.cpp"
  "zone.cpp"
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
