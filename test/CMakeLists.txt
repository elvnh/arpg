set(TEST_EXECUTABLE_NAME ${PROJECT_NAME}-tests)
set(TEST_RUNNER_CODEGEN_TARGET test_runner_codegen)

set(PARENT_SOURCE_DIR ${CMAKE_SOURCE_DIR})

set(TEST_ROOT_DIR ${PARENT_SOURCE_DIR}/test)
set(TEST_RUNNER_SRC_FILE_NAME ${CMAKE_BINARY_DIR}/test_runner.c)

add_executable(${TEST_EXECUTABLE_NAME} ${TEST_RUNNER_SRC_FILE_NAME})
add_dependencies(${TEST_EXECUTABLE_NAME} ${TEST_RUNNER_CODEGEN_TARGET})

target_include_directories(
  ${TEST_EXECUTABLE_NAME}
  PRIVATE
  include
  ${PARENT_SOURCE_DIR}/src
)

target_link_libraries(
  ${TEST_EXECUTABLE_NAME}
  PRIVATE
  ${BASE_LIB_NAME}
  ${GAME_LIB_NAME}
)

add_custom_target(
  ${TEST_RUNNER_CODEGEN_TARGET}
  BYPRODUCTS ${TEST_RUNNER_SRC_FILE_NAME}
  COMMAND python3 ${TEST_ROOT_DIR}/generate_test_runner.py
          -o ${TEST_RUNNER_SRC_FILE_NAME}
          `find ${TEST_ROOT_DIR}/src -name \*.c -print`
)
