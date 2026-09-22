cmake_minimum_required(VERSION 3.16)

project(anytime_golf_amigaos4 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)

find_package(Bullet REQUIRED)

file(GLOB ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../../code/engine/*.cpp"
)
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../../code/game/Classes/*.cpp"
)
file(GLOB PVRT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../../code/lib/pvrt/*.cpp"
)

list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/(RudeRegistry(CF|Win|Symbian)|SoundEngine|MikSound)\\.cpp$")
list(FILTER GAME_SOURCES EXCLUDE REGEX "/(RBEffectsMgr|RBTButton|RBTTitle|SoundProvider|SoundStream)\\.cpp$")
list(FILTER PVRT_SOURCES EXCLUDE REGEX "/PVRTPrint3D\\.cpp$")
list(APPEND PVRT_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/../../code/lib/pvrt/OGLES/PVRTTextureAPI.cpp"
)

add_executable(golf_amigaos4
    main.cpp
    gthread_stub.c
    ${ENGINE_SOURCES}
    ${GAME_SOURCES}
    ${PVRT_SOURCES}
)

target_compile_definitions(golf_amigaos4 PRIVATE
    RUDE_AMIGAOS4
    RUDE_IPAD=1
    RUDE_DEBUG
    NO_RUDETWEAKER
)

target_include_directories(golf_amigaos4 PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/compat"
    "${CMAKE_CURRENT_LIST_DIR}/../../code/engine"
    "${CMAKE_CURRENT_LIST_DIR}/../../code/game/Classes"
    "${CMAKE_CURRENT_LIST_DIR}/../../code/lib/bullet/src"
    "${CMAKE_CURRENT_LIST_DIR}/../../code/lib/bullet/src/LinearMath"
    "${CMAKE_CURRENT_LIST_DIR}/../../code/lib/bullet/src/BulletCollision"
    "${CMAKE_CURRENT_LIST_DIR}/../../code/lib/bullet/src/BulletDynamics"
)

target_link_libraries(golf_amigaos4 PRIVATE
    SDL2_gl4es
    SDL2_image::SDL2_image
    gl4es
    GLU_gl4es
    Bullet::Dynamics
    Bullet::Collision
    Bullet::LinearMath
)

set_target_properties(golf_amigaos4 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_custom_command(TARGET golf_amigaos4 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_LIST_DIR}/../../code/game/Data"
        "$<TARGET_FILE_DIR:golf_amigaos4>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_LIST_DIR}/../../code/game/Resources-iPad"
        "$<TARGET_FILE_DIR:golf_amigaos4>/data/Resources-iPad"
)
