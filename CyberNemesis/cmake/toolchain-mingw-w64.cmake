# Cross-compilar el .exe de Windows desde Linux con MinGW-w64.
#   cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-win
#   cmake --build build-win --target package_game
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CN_MINGW_PREFIX x86_64-w64-mingw32)
set(CMAKE_C_COMPILER   ${CN_MINGW_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${CN_MINGW_PREFIX}-g++)
set(CMAKE_RC_COMPILER  ${CN_MINGW_PREFIX}-windres)
set(CMAKE_FIND_ROOT_PATH /usr/${CN_MINGW_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
