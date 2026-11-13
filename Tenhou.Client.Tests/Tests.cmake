# Included by the single native CMake project in Tenhou.Client.
set(client_checks_directory "${CMAKE_CURRENT_LIST_DIR}/src")
add_executable(tenhou_protocol_check "${client_checks_directory}/client_protocol_check.cpp")
target_link_libraries(tenhou_protocol_check PRIVATE tenhou_network)
add_executable(tenhou_variant_ui_check "${client_checks_directory}/variant_ui_check.cpp" src/variant_table_view.cpp)
target_link_libraries(tenhou_variant_ui_check PRIVATE tenhou_network gdi32)
add_executable(tenhou_resource_check "${client_checks_directory}/resource_check.cpp" src/background.cpp
    "${asset_generated_directory}/client_assets.rc")
target_link_libraries(tenhou_resource_check PRIVATE tenhou_table comctl32)
add_dependencies(tenhou_resource_check tenhou_assets)
foreach(check tenhou_protocol_check tenhou_variant_ui_check tenhou_resource_check)
    target_compile_options(${check} PRIVATE /W4 /WX /utf-8 /EHsc)
endforeach()
add_custom_target(tenhou_client_checks DEPENDS tenhou_protocol_check tenhou_variant_ui_check tenhou_resource_check)
enable_testing()
add_test(NAME client_resources COMMAND tenhou_resource_check
    "${TENHOU_OUTPUT_DIRECTORY}/resources" "${CMAKE_CURRENT_SOURCE_DIR}/assets")
