
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")

ADD_LIBRARY(glc2_client_i386 SHARED ${GLC2_CLIENT_SRC})
SET_TARGET_PROPERTIES(glc2_client_i386 PROPERTIES
    OUTPUT_NAME glc2-client
    VERSION ${GLC2_CLIENT_VERSION}
    SOVERSION ${GLC2_CLIENT_VERSION})

IF(EXISTS "${CMAKE_INSTALL_PREFIX}/lib/i386-linux-gnu")
    INSTALL(TARGETS glc2_client_i386 DESTINATION lib/i386-linux-gnu)
ELSEIF(EXISTS "${CMAKE_INSTALL_PREFIX}/lib32")
    INSTALL(TARGETS glc2_client_i386 DESTINATION lib32)
ELSE()
    INSTALL(TARGETS glc2_client_i386 DESTINATION lib)
ENDIF()

