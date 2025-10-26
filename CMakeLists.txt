cmake_minimum_required(VERSION 3.16)
project(orderbook-sim LANGUAGES CXX)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCHMARKS "Build benchmark target" ON)
option(ENABLE_ASAN "Enable AddressSanitizer (Debug-on)" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(orderbook
            src/order_book.cpp
            src/price_level.cpp
            )

target_include_directories(orderbook
                            PUBLIC ${CMAKE_SOURCE_DIR}/include
                            )

if(MSVC)
    target_compile_options(orderbook PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(orderbook PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

# AddressSanitizer (Debug only)
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE MATCHES "Debug")
  if(MSVC)
    message(WARNING "ASan not configured for MSVC in this template.")
  else()
    target_compile_options(orderbook PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(orderbook PRIVATE -fsanitize=address)
  endif()
endif()

# ---- Demo executable -------------------------------------------------------
# Minimal CLI / playground (you can add later)
add_executable(ob_demo
    src/main.cpp
)
target_link_libraries(ob_demo PRIVATE orderbook)

# ---- Tests -----------------------------------------------------------------
include(CTest)  # enables ctest
if(BUILD_TESTS)
  add_executable(test_basic
      tests/test_basic.cpp
  )
  target_link_libraries(test_basic PRIVATE orderbook)

  add_test(NAME basic COMMAND test_basic)
endif()

# ---- Benchmarks ------------------------------------------------------------
if(BUILD_BENCHMARKS)
  add_executable(ob_bench
      bench/bench_driver.cpp
  )
  target_link_libraries(ob_bench PRIVATE orderbook)
endif()

# ---- Install (optional) ----------------------------------------------------
# install(TARGETS orderbook ob_demo ob_bench test_basic
#         RUNTIME DESTINATION bin
#         ARCHIVE DESTINATION lib
#         LIBRARY DESTINATION lib)
# install(DIRECTORY include/ DESTINATION include)