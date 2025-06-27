

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS})

find_package(ROOT 6.24 CONFIG REQUIRED)

root_generate_dictionary(G__Hit Hit.h LINKDEF HitLinkDef.h)

add_library(SIGANA SHARED Hit.cxx G__Hit.cxx)
target_include_directories(SIGANA PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS})

target_link_libraries(SIGANA)

install(TARGETS SIGANA DESTINATION lib)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libHit_rdict.pcm DESTINATION lib)

#Add execuatable and link to the libraries
add_executable(analyzeSiglent analyzeSiglent.cxx)
target_link_libraries(analyzeSiglent LINK_PUBLIC SIGANA ${ROOT_LIBRARIES})

install(TARGETS analyzeSiglent DESTINATION bin)

add_executable(SigRoot SigRoot.cxx)
target_link_libraries(SigRoot LINK_PUBLIC SIGANA ${ROOT_LIBRARIES})

install(TARGETS SigRoot DESTINATION bin)

#install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/*.h
#        DESTINATION .
#        COMPONENT install
#        )

