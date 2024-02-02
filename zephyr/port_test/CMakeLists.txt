# Zephyr port ztest application

cmake_minimum_required(VERSION 3.13.1)

get_filename_component(REPO_ROOT ../../ ABSOLUTE)
list(APPEND ZEPHYR_EXTRA_MODULES ${REPO_ROOT})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_sources(app PRIVATE src/main.c)
