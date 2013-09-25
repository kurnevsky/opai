cmake_minimum_required(VERSION 2.8)

project(OPAI)

find_package(OpenMP REQUIRED)
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
else()
  message(SEND_ERROR "Failed to find OpenMP.")
  return()
endif()

find_package(Boost COMPONENTS random REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
else()
  message(SEND_ERROR "Failed to find Boost.")
  return()
endif()

add_executable(opai bot.cpp minimax.cpp position_estimate.cpp uct.cpp mtdf.cpp main.cpp)

add_definitions("-std=c++11")

add_definitions("-Ofast")

add_definitions("-flto")

add_definitions("-march=native")

add_definitions("-Wall")

add_definitions("-Wcast-qual")

add_definitions("-Wextra")

add_definitions("-Wshadow")

add_definitions("-pedantic")

add_definitions("-DLINUX")

add_definitions("-DENVIRONMENT_64")
