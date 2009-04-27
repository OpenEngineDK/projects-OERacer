
# Project source code list
SET( OERACER_SOURCES
  # Add all the cpp source files here
  main.cpp
)

# todo get rid of this!@#!
# Include if using SDL under Mac OS X
IF(APPLE)
  SET(OERACER_SOURCES ${OERACER_SOURCES} ${SDL_MAIN_FOR_MAC})
ENDIF(APPLE)

# Project executable
ADD_EXECUTABLE(OERacer ${OERACER_SOURCES})

# Project dependencies
TARGET_LINK_LIBRARIES(OERacer
  OpenEngine_Utils
  # Extensions
  Extensions_SetupHelpers
  Extensions_FixedTimeStepPhysics
  Extensions_GenericHandlers
)
