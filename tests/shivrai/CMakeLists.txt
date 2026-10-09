cmake_minimum_required(VERSION 3.15)
project(shivrai_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Repo root: 2 levels up (shivrai → tests → shivrai-hon-core)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

message(STATUS "REPO_ROOT = ${REPO_ROOT}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${REPO_ROOT}/src
)

set(SHIVRAI_SOURCES
    ${REPO_ROOT}/src/shivrai/identity/identity.cpp
    ${REPO_ROOT}/src/shivrai/interop/message.cpp
    ${REPO_ROOT}/src/shivrai/assets/rwa.cpp
    ${REPO_ROOT}/src/shivrai/gaming/assets.cpp
    ${REPO_ROOT}/src/shivrai/fintech/payment.cpp
    ${REPO_ROOT}/src/shivrai/assets/token_ledger.cpp
)

add_executable(test_identity test_identity.cpp ${SHIVRAI_SOURCES})
add_executable(test_interop  test_interop.cpp  ${SHIVRAI_SOURCES})
add_executable(test_rwa      test_rwa.cpp      ${SHIVRAI_SOURCES})
add_executable(test_gaming   test_gaming.cpp   ${SHIVRAI_SOURCES})
add_executable(test_payment  test_payment.cpp  ${SHIVRAI_SOURCES})
add_executable(test_concurrency test_concurrency.cpp ${SHIVRAI_SOURCES})

enable_testing()
add_test(NAME test_identity COMMAND test_identity)
add_test(NAME test_interop  COMMAND test_interop)
add_test(NAME test_rwa      COMMAND test_rwa)
add_test(NAME test_gaming   COMMAND test_gaming)
add_test(NAME test_payment  COMMAND test_payment)
add_test(NAME test_concurrency COMMAND test_concurrency)
