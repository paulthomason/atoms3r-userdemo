cmake_minimum_required(VERSION 3.5.0)
project(asset_pool_gen VERSION 0.1.0 LANGUAGES C CXX)

add_definitions(-DPLATFORM_BUILD_DESKTOP)

# Mooncake 
add_subdirectory(../components/mooncake)

# App layer
file(GLOB_RECURSE APP_LAYER_SRCS
    ../main/utils/assets/*.c
    ../main/utils/assets/*.cpp
)
set(APP_LAYER_INCS
    ../main/utils/assets
)

add_library(app_layer ${APP_LAYER_SRCS})
target_include_directories(app_layer PUBLIC ${APP_LAYER_INCS})
target_link_libraries(app_layer PUBLIC mooncake)

add_executable(asset_pool_gen main.cpp)
target_link_libraries(asset_pool_gen PUBLIC app_layer)
