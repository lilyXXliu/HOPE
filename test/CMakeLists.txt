find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

function (add_unit_test file_name)
  add_executable(${file_name} ${file_name}.cpp)
  target_link_libraries(${file_name} gtest)
  add_test(NAME ${file_name}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${file_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

add_unit_test(test_artdic)
add_unit_test(test_artdic_node)
add_unit_test(test_single_char_encoder)
add_unit_test(test_double_char_encoder)
add_unit_test(test_ngram_encoder)
add_unit_test(test_alm_encoder)
add_unit_test(test_almimproved_encoder)
add_unit_test(test_array_3gram_dict)
add_unit_test(test_array_4gram_dict)
