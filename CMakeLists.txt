cmake_minimum_required(VERSION 3.10)

# set the project name
project(CmakeTest)
include(FindPkgConfig)
pkg_check_modules(LIBMYSQLCLIENT REQUIRED mysqlclient)


# include dirs
include_directories(
    include
    ${LIBMYSQLCLIENT_INCLUDE_DIRS}
    ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
    # ${GLPK_INCLUDE_DIRS}
    /home/hems/glpk-4.65-built/src
)

link_directories(${LIBMYSQLCLIENT_LIBRARY_DIRS})

# add the executable
add_executable(GHEMS src/GHEMS.cpp src/new2D.cpp src/SQLFunction.cpp)
target_link_libraries(GHEMS 
${LIBMYSQLCLIENT_LIBRARIES} 
/home/hems/glpk-4.65-built/src/.libs/libglpk.a
)