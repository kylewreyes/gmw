cmake_minimum_required(VERSION 3.10)

# project details
project(GMWApp VERSION 1.0)
set(PARTICIPANT_EXEC_NAME participant)
set(OTTEST_EXEC_NAME ot_test)
set(LIBRARY_NAME gmw_app_lib)
set(LIBRARY_NAME_SHARED gmw_app_lib_shared)
# set(LIBRARY_NAME_TA yaos_app_lib_ta)

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
  src/drivers/cli_driver.cxx
  src/drivers/crypto_driver.cxx
  src/drivers/network_driver.cxx
  src/drivers/share_driver.cxx
  src/drivers/ot_driver.cxx)
add_library(${LIBRARY_NAME} ${SOURCES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include-shared ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${LIBRARY_NAME} PRIVATE ${LIBRARY_NAME_SHARED})

<<<<<<< HEAD
# add ta libraries
if ( "$ENV{CS1515_TA_MODE}" STREQUAL "on" )
  set(SOURCES_TA
    src-ta/pkg/garbler.cxx
    src-ta/pkg/evaluator.cxx
    src-ta/drivers/cli_driver.cxx
    src-ta/drivers/crypto_driver.cxx
    src-ta/drivers/network_driver.cxx
    src-ta/drivers/ot_driver.cxx)
  add_library(${LIBRARY_NAME_TA} ${SOURCES_TA})
  target_include_directories(${LIBRARY_NAME_TA} PUBLIC ${PROJECT_SOURCE_DIR}/include-shared ${PROJECT_SOURCE_DIR}/include-ta)
  target_link_libraries(${LIBRARY_NAME_TA} PRIVATE ${LIBRARY_NAME_SHARED})
endif()

# add garbler executables
# if ( "$ENV{CS1515_TA_MODE}" STREQUAL "on" )
#   add_executable(${GARBLER_EXEC_NAME} src-ta/cmd/garbler.cxx)
#   target_link_libraries(${GARBLER_EXEC_NAME} PRIVATE ${LIBRARY_NAME_TA})
# else()
#   add_executable(${GARBLER_EXEC_NAME} src/cmd/garbler.cxx)
#   target_link_libraries(${GARBLER_EXEC_NAME} PRIVATE ${LIBRARY_NAME})
# endif()

# add evaluator executables
if ( "$ENV{CS1515_TA_MODE}" STREQUAL "on" )
  add_executable(${EVALUATOR_EXEC_NAME} src-ta/cmd/participant.cxx)
  target_link_libraries(${EVALUATOR_EXEC_NAME} PRIVATE ${LIBRARY_NAME_TA})
else()
  add_executable(${EVALUATOR_EXEC_NAME} src/cmd/participanet.cxx)
  target_link_libraries(${EVALUATOR_EXEC_NAME} PRIVATE ${LIBRARY_NAME})
endif()
=======
# add participant executable
add_executable(${PARTICIPANT_EXEC_NAME} src/cmd/participant.cxx)
target_link_libraries(${PARTICIPANT_EXEC_NAME} PRIVATE ${LIBRARY_NAME})
>>>>>>> b56b47e (Fix bugs in CMake and NetworkDriver)

# add ot test executables
add_executable(${OTTEST_EXEC_NAME} src/cmd/ot_test.cxx)
target_link_libraries(${OTTEST_EXEC_NAME} PRIVATE ${LIBRARY_NAME})

# properties
set_target_properties(
  ${LIBRARY_NAME}
  ${PARTICIPANT_EXEC_NAME}
  ${OTTEST_EXEC_NAME}
    PROPERTIES
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED YES
      CXX_EXTENSIONS YES
)

# add tests
add_subdirectory(test)
add_custom_target(check ./test.sh)
