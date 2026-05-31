cmake_minimum_required(VERSION 3.16)

project(qjs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Target: QuickJS engine (static), name `quickjs`, alias `qjs::quickjs`
include(qjs_quickjs)

# Target: C++ wrapper (static), name `qjs`, alias `qjs::qjs`
add_library(qjs STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/engine/engine.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/engine/context.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/vm.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/error.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/module/installer.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/module/loader.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/module/module_api.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/promise/promise.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/promise/bridge.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/binding/value.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/binding/value_impl.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/binding/call_context.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/binding/object_builder.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/binding/js_conv.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/binding/module_binding.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/binding/native_function_class.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/resolver/file_resolver.cc"
)

target_include_directories(qjs PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_link_libraries(qjs PUBLIC quickjs)

add_library(qjs::qjs ALIAS qjs)
add_library(qjs::quickjs ALIAS quickjs)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(QJS_BUILD_TESTS "Build qjs unit tests (GoogleTest)" ON)
else()
    option(QJS_BUILD_TESTS "Build qjs unit tests (GoogleTest)" OFF)
endif()

if(QJS_BUILD_TESTS)
    include(qjs_tests)
endif()
