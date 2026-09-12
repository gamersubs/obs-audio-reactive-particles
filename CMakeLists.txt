cmake_minimum_required(VERSION 3.28...3.30)
project(obs-audio-reactive-particles VERSION 0.1.0)

option(ENABLE_FRONTEND_API "Use obs-frontend-api" OFF)

find_package(libobs REQUIRED)
add_library(obs-audio-reactive-particles MODULE
    src/plugin-main.cpp
    src/particle-source.cpp
)

target_compile_features(obs-audio-reactive-particles PRIVATE cxx_std_17)
target_link_libraries(obs-audio-reactive-particles PRIVATE OBS::libobs)

if(MSVC)
    target_compile_options(obs-audio-reactive-particles PRIVATE /W4 /permissive-)
endif()

set_target_properties(obs-audio-reactive-particles PROPERTIES OUTPUT_NAME "obs-audio-reactive-particles")

install(TARGETS obs-audio-reactive-particles
    LIBRARY DESTINATION obs-plugins/64bit
    RUNTIME DESTINATION obs-plugins/64bit
)
install(DIRECTORY data/ DESTINATION data/obs-plugins/audio-reactive-particles)
