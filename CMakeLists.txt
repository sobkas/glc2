CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(glc2)

SET(GLC2_CLIENT_VERSION 0.1) # client shared object
SET(GLC2_SERVER_VERSION 0.1) # server shared object
SET(GLC2_VERSION 0.1) # the glc2 executable

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake_modules")

SET(BASE_C_FLAGS "${CMAKE_C_FLAGS}")

IF(NOT DEFINED CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Debug")
ENDIF()

IF(NOT DEFINED DISABLE_32BIT_BUILD)
    SET(DISABLE_32BIT_BUILD "No")
ENDIF()

ADD_SUBDIRECTORY(src/client)
ADD_SUBDIRECTORY(src/server)
ADD_SUBDIRECTORY(src/glc2)
