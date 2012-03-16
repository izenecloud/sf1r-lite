INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/core/
    )

SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
    COMPONENTS unit_test_framework)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
    INCLUDE_DIRECTORIES(
        ${Boost_INCLUDE_DIRS}
        )
    ADD_EXECUTABLE(t_license_manager
        Runner.cpp
        t_LicenseClient.cpp
        t_LicenseEncryptor.cpp
        t_LicenseTool.cpp
        t_LicenseRequestFileGenerator.cpp
        )
    TARGET_LINK_LIBRARIES(t_license_manager
        sf1r_license_manager
        sf1r_log_manager
        sf1r_common
        sf1r_query_manager
        sf1r_la_manager
        sf1r_configuration_manager
        sf1r_product_manager
        sf1r_document_manager
        sf1r_node_manager
        sf1r_search_manager
        sf1r_mining_manager
        sf1r_ranking_manager
        sf1r_index_manager
        sf1r_directory_manager
        ${Glog_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ilplib_LIBRARIES}
        ${wisekma_LIBRARIES}
        ${izenecma_LIBRARIES}
        ${izenejma_LIBRARIES}
        ${idmlib_LIBRARIES}
        ${imllib_LIBRARIES}
        ${izenelib_LIBRARIES}
        ${xml2_LIBRARIES}
        sqlite3
        ${MYSQL_LIBRARIES}
        ${TokyoCabinet_LIBRARIES}
        ${ImageMagick_LIBRARIES}
        )
    SET_TARGET_PROPERTIES(t_license_manager
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
        ${SF1RENGINE_ROOT}/testbin
        )
    ADD_TEST(license_manager
        "${SF1RENGINE_ROOT}/testbin/t_license_manager"
        )
ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)

