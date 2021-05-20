cmake_minimum_required(VERSION 3.10)

# set the project name
set (CMAKE_CXX_STANDARD 17)
project(CmakeTest)
include(FindPkgConfig)
pkg_check_modules(LIBMYSQLCLIENT REQUIRED mysqlclient)


# include dirs
include_directories(
    include
    ${LIBMYSQLCLIENT_INCLUDE_DIRS}
    ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
    # ${GLPK_INCLUDE_DIRS}
    # /home/hems/glpk-4.65/src
)

link_directories(${LIBMYSQLCLIENT_LIBRARY_DIRS})

# add the executable
# executable include 3 demand response by 5 household CEMS
add_executable(LHEMS src/LHEMS.cpp src/new2D.cpp src/SQLFunction.cpp src/scheduling_parameter.cpp)
target_link_libraries(LHEMS 
${LIBMYSQLCLIENT_LIBRARIES} 
/usr/local/lib/libglpk.a
)
add_executable(GHEMS src/GHEMS.cpp src/new2D.cpp src/SQLFunction.cpp src/scheduling_parameter.cpp)
target_link_libraries(GHEMS 
${LIBMYSQLCLIENT_LIBRARIES} 
/usr/local/lib/libglpk.a
)

# executable by 50 household CEMS
add_executable(fifty_LHEMS src_fifty/LHEMS.cpp src/new2D.cpp src/SQLFunction.cpp src/scheduling_parameter.cpp src_fifty/fifty_LHEMS_function.cpp)
target_link_libraries(fifty_LHEMS 
${LIBMYSQLCLIENT_LIBRARIES} 
/usr/local/lib/libglpk.a
)
add_executable(fifty_GHEMS src_fifty/GHEMS.cpp src/new2D.cpp src/SQLFunction.cpp src/scheduling_parameter.cpp)
target_link_libraries(fifty_GHEMS 
${LIBMYSQLCLIENT_LIBRARIES} 
/usr/local/lib/libglpk.a
)