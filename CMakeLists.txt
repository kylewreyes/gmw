cmake_minimum_required(VERSION 3.10)

# project details
project(GMWApp VERSION 1.0)
set(PARTICIPANT_EXEC_NAME participant)
set(LIBRARY_NAME gmw_app_lib)
set(LIBRARY_NAME_SHARED gmw_app_lib_shared)

# packages
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
include(Boost)
# include(Cryptopp)
include(CTest)
include(Doctest)
include(Documentation)
include(Warnings)
include(Curses)

# add shared libraries
set(SOURCES_SHARED
  src-shared/circuit.cxx
  src-shared/messages.cxx
  src-shared/logger.cxx
  src-shared/util.cxx)
add_library(${LIBRARY_NAME_SHARED} ${SOURCES_SHARED})
target_include_directories(${LIBRARY_NAME_SHARED} PUBLIC ${PROJECT_SOURCE_DIR}/include-shared)
target_link_libraries(${LIBRARY_NAME_SHARED} PUBLIC doctest)
target_link_libraries(${LIBRARY_NAME_SHARED} PRIVATE cryptopp)
# target_link_libraries(${LIBRARY_NAME_SHARED} PRIVATE cryptopp-shared)
target_link_libraries(${LIBRARY_NAME_SHARED} PRIVATE ${Boost_LIBRARIES})
target_link_libraries(${LIBRARY_NAME_SHARED} PRIVATE ${CURSES_LIBRARIES})

# add student libraries
set(SOURCES
  src/pkg/peer_link.cxx
  src/drivers/cli_driver.cxx
  src/drivers/crypto_driver.cxx
  src/drivers/network_driver.cxx
  src/drivers/ot_driver.cxx
  src/drivers/share_driver.cxx)
add_library(${LIBRARY_NAME} ${SOURCES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include-shared ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${LIBRARY_NAME} PRIVATE ${LIBRARY_NAME_SHARED})

# add participant executable
add_executable(${PARTICIPANT_EXEC_NAME} src/cmd/participant.cxx)
target_link_libraries(${PARTICIPANT_EXEC_NAME} PRIVATE ${LIBRARY_NAME})

# properties
set_target_properties(
  ${LIBRARY_NAME}
  ${PARTICIPANT_EXEC_NAME}
    PROPERTIES
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED YES
      CXX_EXTENSIONS YES
)

# add tests
add_subdirectory(test)
add_custom_target(check ./test.sh)
