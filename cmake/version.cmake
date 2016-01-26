FUNCTION(GENERATE_VERSION VERSION_FILE)
  
  MESSAGE( STATUS "CMAKE_SOURCE_DIR:         " ${CMAKE_SOURCE_DIR} )
  MESSAGE( STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR} )
  MESSAGE( STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )

  EXECUTE_PROCESS(
  	COMMAND git describe --exact-match --tags HEAD
  	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	RESULT_VARIABLE IS_TAG
  	OUTPUT_STRIP_TRAILING_WHITESPACE
  	ERROR_QUIET
  )

  IF(IS_TAG EQUAL 0)
  	SET(${PROJECTNAME}_VERSION_TWEAK "")
  ELSE()
  	EXECUTE_PROCESS(
  	  COMMAND git rev-list HEAD --count
  	  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  	  OUTPUT_VARIABLE GIT_REV_COUNT
  	  OUTPUT_STRIP_TRAILING_WHITESPACE
  	  ERROR_QUIET
  	)
  	IF(GIT_REV_COUNT STREQUAL "")
  		SET(${PROJECTNAME}_VERSION_TWEAK "-unknown")
  	ELSE()
  		SET(${PROJECTNAME}_VERSION_TWEAK "-${GIT_REV_COUNT}")
  	ENDIF()
  ENDIF()
  
  EXECUTE_PROCESS(
    COMMAND git diff --shortstat
    COMMAND tail -n1
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DIRTY
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  
  IF(GIT_DIRTY STREQUAL "")
  	SET(${PROJECTNAME}_VERSION_DIRTY "")
  ELSE()
  	SET(${PROJECTNAME}_VERSION_DIRTY "+dirty")
  ENDIF()

  IF((NOT (GIT_DIRTY STREQUAL "")) AND (IS_TAG EQUAL 0))
    MESSAGE( FATAL_ERROR "Never build a release from dirty tag!" )
  ENDIF()
  
  SET(CJET_LAST ${${PROJECTNAME}_VERSION_TWEAK}${${PROJECTNAME}_VERSION_DIRTY} PARENT_SCOPE)

ENDFUNCTION(GENERATE_VERSION)

