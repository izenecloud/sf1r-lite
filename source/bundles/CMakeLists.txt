INCLUDE_DIRECTORIES(
  include

  ${CMAKE_SOURCE_DIR}/core

  ${izenelib_INCLUDE_DIRS}
  ${wiselib_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${idmlib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
  ${iise_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  )


#FILE(GLOB document_SRC
#  "document-manager/*.cpp"
#  )

#ADD_LIBRARY(sf1v5_document_manager ${document_SRC})

#INSTALL(TARGETS 
#    sf1v5_document_manager 
#    LIBRARY DESTINATION lib
#    ARCHIVE DESTINATION lib
#    COMPONENT sf1r_libraries)


