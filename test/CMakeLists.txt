SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SF1RENGINE_ROOT}/testbin")
INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/core/
  ${CMAKE_CURRENT_SOURCE_DIR}

  ${izenelib_INCLUDE_DIRS}
  )

CONFIGURE_FILE(
  TestResources.h.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/TestResources.h
  ESCAPE_QUOTES @ONLY
  )

ADD_SUBDIRECTORY(document-manager)
ADD_SUBDIRECTORY(mining-manager)
ADD_SUBDIRECTORY(ranking-manager)
ADD_SUBDIRECTORY(query-manager) 
ADD_SUBDIRECTORY(log-manager) 
ADD_SUBDIRECTORY(license-manager) 
ADD_SUBDIRECTORY(configuration-manager) 
ADD_SUBDIRECTORY(recommend-manager)
ADD_SUBDIRECTORY(common)
ADD_SUBDIRECTORY(product-manager)
