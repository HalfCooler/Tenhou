# Optional frozen oracle for server migration comparisons; never linked into production.
set(rule_reference_directory "${CMAKE_CURRENT_LIST_DIR}")
add_executable(tenhou_rule_oracle "${rule_reference_directory}/native_rule_oracle.cpp"
    "${rule_reference_directory}/native-riichi-reference/common.cpp"
    "${rule_reference_directory}/native-riichi-reference/scoring.cpp"
    "${rule_reference_directory}/native-riichi-reference/game.cpp")
target_include_directories(tenhou_rule_oracle PRIVATE "${rule_reference_directory}/native-riichi-reference")
target_compile_options(tenhou_rule_oracle PRIVATE /W4 /WX /utf-8 /EHsc)
target_compile_definitions(tenhou_rule_oracle PRIVATE NOMINMAX)
