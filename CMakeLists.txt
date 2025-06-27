cmake_minimum_required(VERSION 3.10)

# set the project name
project(SIGANA VERSION 1.0)

message("===============  ${PROJECT_NAME}  ==============")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(ROOT REQUIRED)

set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_STANDARD 17)


message("")
message("SIGANA will be installed in : ${CMAKE_INSTALL_PREFIX}")
message("===========================================================")

add_subdirectory(ANA)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/config
       DESTINATION .
       COMPONENT install
       )

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/macros
       DESTINATION .
       COMPONENT install
       )

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scripts
       DESTINATION .
       COMPONENT install
       )


#Create thisSiglent.sh
include(thisSiglent)


