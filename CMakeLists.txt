cmake_minimum_required(VERSION 3.19)
project(omp-argon2 LANGUAGES C CXX VERSION 2.0.0)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(deps/omp-sdk)

include_directories(
    .
    deps
    deps/omp-sdk/include
    deps/pawn/source
    deps/pawn/source/linux
    deps/pawn-natives
)

add_definitions(
    -DHAVE_STDINT_H=1
    -DPAWN_CELL_SIZE=32
)

add_library(${PROJECT_NAME} SHARED
    main.cpp
    natives.cpp
    argon2-component.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE OMP-SDK)

if(WIN32)
    find_package(unofficial-argon2 CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::argon2::libargon2 bcrypt)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE argon2)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")