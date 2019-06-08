project(cweb_epoll_file)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
MESSAGE(STATUS "#########################")

# ADD_DEFINITIONS(-g -Wno-deprecated )
SET(CMAKE_BUILD_TYPE Debug)
ADD_DEFINITIONS( -g -Wall )
file(GLOB ALL_CXX_SRCS
./*.c
)

#SET(EXTRA_INC_DIR /home/nginx/boost_1_60_0)


INCLUDE_DIRECTORIES(
#${EXTRA_INC_DIR}
)


LINK_DIRECTORIES(
#${EXTRA_INC_DIR}/stage/lib
)

set(ALL_LIB  pthread
#        libboost_locale.a
#        libboost_regex.a
#        libboost_system.a
#        libboost_filesystem.a
        )

add_executable(${PROJECT_NAME} ${ALL_CXX_SRCS} )

target_link_libraries(${PROJECT_NAME} ${ALL_LIB})