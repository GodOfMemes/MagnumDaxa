
# Ensure Daxa is available
# NOTE: This is temporary, and should really be consumed from the official vcpkg port of Daxa.
# Daxa is in active development, so for the sake of the tutorial, we're using a newer version
# of Daxa than is available through vcpkg directly.
#[[
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../lib/Daxa/CMakeLists.txt")
    find_package(Git REQUIRED)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../lib")
    execute_process(COMMAND ${GIT_EXECUTABLE} clone https://github.com/Ipotrick/Daxa
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../lib"
        COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND ${GIT_EXECUTABLE} checkout 26d1a108ead92fb4960e34bb011ffd1be94ee84b
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../lib/Daxa"
        COMMAND_ERROR_IS_FATAL ANY)
endif()]]

# If the user has set a toolchain file, we'll want to chainload it via vcpkg
if(NOT (CMAKE_TOOLCHAIN_FILE MATCHES "/scripts/buildsystems/vcpkg.cmake") AND DEFINED CMAKE_TOOLCHAIN_FILE)
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}" CACHE UNINITIALIZED "")
endif()

# Check if vcpkg is installed globally. Otherwise, clone vcpkg
if(EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    file(TO_CMAKE_PATH $ENV{VCPKG_ROOT} VCPKG_ROOT)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
else()
    find_package(Git REQUIRED)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
        execute_process(COMMAND ${GIT_EXECUTABLE} clone https://github.com/Microsoft/vcpkg
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMAND_ERROR_IS_FATAL ANY)
    else()
        #[[execute_process(COMMAND ${GIT_EXECUTABLE} pull
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/vcpkg"
            COMMAND_ERROR_IS_FATAL ANY)]]
    endif()
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()