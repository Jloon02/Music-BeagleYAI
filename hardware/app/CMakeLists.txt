# Build the app which supports ALSA, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(music_board ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(music_board LINK_PRIVATE hal)
target_link_libraries(music_board LINK_PRIVATE lcd)
target_link_libraries(music_board LINK_PRIVATE lgpio)
# ---------------- NEW! ----------------
# ALSA support
find_package(ALSA REQUIRED)
target_link_libraries(music_board LINK_PRIVATE asound)

# PTHREAD support
find_package(Threads REQUIRED)
target_link_libraries(music_board LINK_PRIVATE Threads::Threads)

# Copy executable to final location (change `music_board` to project name as needed)
add_custom_command(TARGET music_board POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:music_board>"
     "~/cmpt433/public/myApps/music_board" 
  COMMENT "Copying ARM executable to public NFS directory")

add_custom_command(TARGET music_board POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:music_board>"
     "~/cmpt433/public/myApps/music_board" 
  COMMENT "Copying ARM executable to public NFS directory")

add_custom_command(TARGET music_board POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
     "${CMAKE_SOURCE_DIR}/wave-files"
     "~/cmpt433/public/myApps/wave-files" 
  COMMAND chmod -R u+w "~/cmpt433/public/myApps/wave-files"
  COMMAND chmod 777 "~/cmpt433/public/myApps/wave-files"
  COMMENT "Copying wave-files to public NFS directory and giving permissions")


  # Copy the Server folder to NFS
# add_custom_command(TARGET music_board POST_BUILD 
# COMMAND "${CMAKE_COMMAND}" -E copy_directory
#    "${CMAKE_SOURCE_DIR}/as3-server"
#    "~/cmpt433/public/myApps/musicboard-server-copy" 
# COMMENT "Copying WAVE files to public NFS directory")