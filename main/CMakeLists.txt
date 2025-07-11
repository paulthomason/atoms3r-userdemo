file(GLOB_RECURSE APP_SRCS
    ./*.c
    ./*.cpp
)

idf_component_register(SRCS ${APP_SRCS}
                    INCLUDE_DIRS ".")

include(gen_single_bin)
