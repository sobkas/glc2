SET(CLIENT_DIR "${CMAKE_SOURCE_DIR}/src/client")
SET(COMMON_DIR "${CMAKE_SOURCE_DIR}/src/common")

SET(GLC2_CLIENT_SRC
    ${CLIENT_DIR}/main.c
    ${CLIENT_DIR}/client.c
    ${CLIENT_DIR}/elfhacks.c
    ${CLIENT_DIR}/hook.c
    ${CLIENT_DIR}/opengl.c
    ${CLIENT_DIR}/x11.c
    ${CLIENT_DIR}/event.c
    ${CLIENT_DIR}/log.c
    ${COMMON_DIR}/packetstream.c)

ADD_DEFINITIONS(-DGLC2_CLIENT_SO_NAME="libglc2-client.so")

SET(CMAKE_C_FLAGS "${BASE_C_FLAGS} -Wall -Wextra -Wno-missing-field-initializers -fvisibility=hidden")
INCLUDE_DIRECTORIES(${COMMON_DIR})

ADD_LIBRARY(glc2_client SHARED ${GLC2_CLIENT_SRC})
TARGET_LINK_LIBRARIES(glc2_client GL dl X11 Xxf86vm)
SET_TARGET_PROPERTIES(glc2_client PROPERTIES
    OUTPUT_NAME glc2-client
    VERSION ${GLC2_CLIENT_VERSION}
    SOVERSION ${GLC2_CLIENT_VERSION})


IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
    IF(NOT DISABLE_32BIT_BUILD)
        ADD_SUBDIRECTORY(i386)
    ENDIF()

    IF(EXISTS "${CMAKE_INSTALL_PREFIX}/lib/x86_64-linux-gnu")
        INSTALL(TARGETS glc2_client DESTINATION lib/x86_64-linux-gnu)
    ELSEIF(EXISTS "${CMAKE_INSTALL_PREFIX}/lib64")
        INSTALL(TARGETS glc2_client DESTINATION lib64)
    ELSE()
        INSTALL(TARGETS glc2_client DESTINATION lib)
    ENDIF()
ELSE()
    IF(EXISTS "${CMAKE_INSTALL_PREFIX}/lib/i386-linux-gnu")
        INSTALL(TARGETS glc2_client DESTINATION lib/i386-linux-gnu)
    ELSEIF(EXISTS "${CMAKE_INSTALL_PREFIX}/lib32")
        INSTALL(TARGETS glc2_client DESTINATION lib32)
    ELSE()
        INSTALL(TARGETS glc2_client DESTINATION lib)
    ENDIF()
ENDIF()

