cmake_minimum_required(VERSION 3.16)
project(csmpi)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)

add_library(csmpi SHARED)

target_sources(csmpi PRIVATE src/csmpi.cpp src/configuration.cpp src/runtime.cpp src/callstack.cpp)
target_include_directories(csmpi PRIVATE include include/csmpi include/external)

# Boost
find_package(Boost 1.70.0 REQUIRED mpi serialization)
target_include_directories(csmpi PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(csmpi PRIVATE Boost::mpi)
target_link_libraries(csmpi PRIVATE Boost::serialization)

# Libunwind (Hardcoded until I can get FindLibunwind.cmake working)
#find_package(Libunwind REQUIRED)
#target_include_directories(csmpi PRIVATE ${LIBUNWIND_INCLUDE_DIR})
target_include_directories(csmpi PRIVATE /home/dchapp/repos/spack/opt/spack/linux-rhel7-power9le/gcc-9.2.0/libunwind-1.2.1-ym25jg4lvtqbouwatk2tdz7bapjsklcb/include/)
target_link_libraries(csmpi PRIVATE /home/dchapp/repos/spack/opt/spack/linux-rhel7-power9le/gcc-9.2.0/libunwind-1.2.1-ym25jg4lvtqbouwatk2tdz7bapjsklcb/lib64/libunwind.so)

