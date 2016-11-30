cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

#===============IFCOPENSHELL SETTINGS==================
find_package(Boost REQUIRED)
find_package(IFCOPENSHELL REQUIRED)
if(NOT ${IFCOPENSHELL_FOUND})
	message(FATAL_ERROR "Cannot find IFCOpenShell")
endif()

include_directories(${Boost_INCLUDE_DIRS} ${IFCOPENSHELL_INCLUDE_DIR})
add_executable(IfcImprover main.cpp)
target_link_libraries(IfcImprover ${IFCOPENSHELL_PARSERLIB})
