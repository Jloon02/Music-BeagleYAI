# Build the app, using the HAL

include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(music_board ${MY_SOURCES})

# Make use of the libraries
target_link_libraries(music_board LINK_PRIVATE hal)
target_link_libraries(music_board LINK_PRIVATE lcd)
target_link_libraries(music_board LINK_PRIVATE lgpio)

# Copy executable to final location
add_custom_command(TARGET music_board POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:music_board>"
     "~/cmpt433/public/myApps/music_board" 
  COMMENT "Copying ARM executable to public NFS directory")
