# CMakeList.txt for HAL
#   Build a library (`hal`) which exposes the header files as "hal/*.h"
#   Use header as: #include "hal/button.h"

include_directories(hal/include)
file(GLOB MY_SOURCES "src/*.c")

add_library(hal STATIC ${MY_SOURCES})

# My original file changed
# Include gpiod headers
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)

# Link libgpiod to the hal library
target_link_libraries(hal PRIVATE ${GPIOD_LIBRARIES})
target_link_libraries(hal LINK_PRIVATE lcd)
target_link_libraries(hal LINK_PRIVATE lgpio)

# Include directories for gpiod
target_include_directories(hal PRIVATE ${GPIOD_INCLUDE_DIRS})

# Expose public headers of HAL
target_include_directories(hal PUBLIC include)
