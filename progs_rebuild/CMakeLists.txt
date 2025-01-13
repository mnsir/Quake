add_executable(progs_rebuild
    "main.cpp"
)

target_link_libraries(progs_rebuild
  PRIVATE
  quake::common
)
