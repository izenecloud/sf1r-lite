INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/core/
  ${CMAKE_SOURCE_DIR}/bundles/
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/common

  ${idmlib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${izenejma_INCLUDE_DIRS}  
  ${izenelib_INCLUDE_DIRS}
  ${iise_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  ${xml2_INCLUDE_DIRS}
  ${sqlite3_INCLUDE_DIRS}
  )

ADD_EXECUTABLE(LicenseFileGenerator
    license-file-generator/LicenseFileGenerator.cpp
    )

TARGET_LINK_LIBRARIES(LicenseFileGenerator
  sf1r_configuration_manager
  sf1r_la_manager
  sf1r_query_manager
  sf1r_log_manager
  sf1r_license_manager
  sf1r_common
  ${idmlib_LIBRARIES}
  ${imllib_LIBRARIES}
  ${ilplib_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}  
  ${izenelib_LIBRARIES}
  ${xml2_LIBRARIES}

  #external
  ${Boost_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${sqlite3_LIBRARIES}
  ${MYSQL_LIBRARIES}
)

