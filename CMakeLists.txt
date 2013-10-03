########################################################
# cmake file for CMSPixelDecoder
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
########################################################

option(BUILD_TOOLS "Build the CMSPixelDecoder testing tools" OFF)
option(BUILD_D2H "Build the d2h data conversion tools" OFF)

# Library Version
SET(CMSPIXELDECODER_LIB_VERSION "3.1.0")
SET(CMSPIXELDECODER_LIB_SOVERSION "3")

SET(GCC_COMPILE_FLAGS "-Wall -Wextra -g -Wno-deprecated -ansi -pedantic -Wno-long-long")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}" )

SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fno-inline -fdiagnostics-show-option -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef" CACHE STRING "Debug options." FORCE )
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -Wall"  CACHE STRING "Relwithdebinfo options." FORCE )

# Set the correct build type and allow command line options:
IF(!CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Relwithdebinfo)
  #SET(CMAKE_BUILD_TYPE Debug)
ENDIF(!CMAKE_BUILD_TYPE)

SET(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

MESSAGE( STATUS "Build Type: " ${CMAKE_BUILD_TYPE} )
MESSAGE( STATUS "Installing to: " ${PROJECT_SOURCE_DIR} )

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )


# Preparing the build of the shared decoder library
SET(cmspixeldecoder_source_files CMSPixelDecoder.cc CMSPixelDecoder.h)

ADD_LIBRARY(CMSPixelDecoder SHARED ${cmspixeldecoder_source_files})
SET (DEC_LIB ${DEC_LIB} CMSPixelDecoder)

SET_TARGET_PROPERTIES( CMSPixelDecoder  PROPERTIES
  VERSION ${CMSPIXELDECODER_LIB_VERSION}
  SOVERSION ${CMSPIXELDECODER_LIB_SOVERSION}
  )

MESSAGE( STATUS "Building CMSPixelDecoder Library." )


# Add the executables
IF(BUILD_TOOLS)
  ADD_EXECUTABLE (unit_tests unit_tests.cc)
  TARGET_LINK_LIBRARIES (unit_tests  ${DEC_LIB})

  ADD_EXECUTABLE (stat_tool stat_tool.cc)
  TARGET_LINK_LIBRARIES (stat_tool ${DEC_LIB})

  ADD_EXECUTABLE (mc_background mc_background.cc)
  TARGET_LINK_LIBRARIES (mc_background ${DEC_LIB})

  ADD_EXECUTABLE (tentative_dec tentative_dec.cc)
  TARGET_LINK_LIBRARIES (tentative_dec ${DEC_LIB})

  MESSAGE( STATUS "Building CMSPixelDecoder tools." )
ELSE(BUILD_TOOLS)
  MESSAGE( STATUS "NOT building CMSPixelDecoder tools." )
ENDIF(BUILD_TOOLS)

IF(BUILD_D2H)
  FIND_PACKAGE(ROOT)
  IF(ROOT_FOUND)
    MESSAGE (STATUS "Building d2h - data2histograms.")
    INCLUDE_DIRECTORIES(${ROOT_INCLUDE_DIR})
    ADD_EXECUTABLE (d2h d2h.cc)
    TARGET_LINK_LIBRARIES (d2h ${DEC_LIB} ${ROOT_LIBRARIES})
  ELSE(ROOT_FOUND)
    MESSAGE (STATUS "NOT building d2h - ROOT not found.")
  ENDIF(ROOT_FOUND)
ELSE(BUILD_D2H)
  MESSAGE( STATUS "NOT building d2h - data2histograms." )
ENDIF(BUILD_D2H)


# Unit tests:
#ADD_TEST (DecodingUnitTests unit_tests SUMMARY)
#SET_TESTS_PROPERTIES (DecodingUnitTests
#  PROPERTIES 
#  PASS_REGULAR_EXPRESSION "Usage:.*number"
#  )
