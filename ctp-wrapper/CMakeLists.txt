cmake_minimum_required(VERSION 3.23)
project(wrapper)

set(CMAKE_CXX_STANDARD 17)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")

if (MSVC)
    message(STATUS "Building for windows")
    # from openctp: https://github.com/openctp/openctp
    add_library(thostmduserapi_se_tts SHARED IMPORTED)
    set_property(TARGET thostmduserapi_se_tts PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/v672/win64/thostmduserapi_se.dll")
    set_property(TARGET thostmduserapi_se_tts PROPERTY IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/v672/win64/thostmduserapi_se.lib")
    target_include_directories(thostmduserapi_se_tts INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/v672/win64")

    add_library(thosttraderapi_se_tts SHARED IMPORTED)
    set_property(TARGET thosttraderapi_se_tts PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/v672/win64/thosttraderapi_se.dll")
    set_property(TARGET thosttraderapi_se_tts PROPERTY IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/v672/win64/thosttraderapi_se.lib")
    target_include_directories(thosttraderapi_se_tts INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/v672/win64")
else ()
    message(STATUS "Building for linux")
    # from 上期技术: http://www.sfit.com.cn/5_2_DocumentDown_2.htm
    add_library(thostmduserapi_se_sq SHARED IMPORTED)
    set_property(TARGET thostmduserapi_se_sq PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/v672/linux64/thostmduserapi_se.so")
    target_include_directories(thostmduserapi_se_sq INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/v672/linux")
    add_library(thosttraderapi_se_sq SHARED IMPORTED)
    set_property(TARGET thosttraderapi_se_sq PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/v672/linux64/thosttraderapi_se.so")
    target_include_directories(thosttraderapi_se_sq INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/v672/linux")
endif ()


find_package(fmt CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(Boost 1.86.0 REQUIRED)
set(SOURCES config.cpp events.cpp hq.cpp)

add_library(ctp_wrap STATIC ${SOURCES})
target_include_directories(ctp_wrap PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
if (BUILD_TTS)
    message(STATUS "Comiple with tts")
    target_link_libraries(ctp_wrap PUBLIC
            spdlog::spdlog fmt::fmt-header-only spdlog::spdlog_header_only
            yaml-cpp::yaml-cpp
            absl::node_hash_map
            lueing_common
            thostmduserapi_se_tts thosttraderapi_se_tts)

    if (MSVC)
        add_custom_command(TARGET ctp_wrap POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/lib/v672/win64/thostmduserapi_se.dll"
                "${CMAKE_CURRENT_SOURCE_DIR}/lib/v672/win64/thosttraderapi_se.dll"
                $<TARGET_FILE_DIR:ctp_wrap>)
    endif ()
elseif (BUILD_SQ)
    message(STATUS "Comiple with sq")
    target_link_libraries(ctp_wrap PUBLIC
            spdlog::spdlog fmt::fmt-header-only spdlog::spdlog_header_only
            yaml-cpp::yaml-cpp
            absl::node_hash_map
            lueing_common
            thostmduserapi_se_sq thosttraderapi_se_sq)
endif ()


if (BUILD_TTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_executable(events_test events.cpp events_test.cpp)
    target_include_directories(events_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(events_test PRIVATE
            absl::node_hash_map
            GTest::gtest_main)

    add_executable(config_test config.cpp config_test.cpp)
    target_include_directories(config_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(config_test PUBLIC yaml-cpp::yaml-cpp lueing_common thostmduserapi_se_tts GTest::gtest_main)

    add_executable(hq_test hq_test.cpp)
    target_link_libraries(hq_test PUBLIC ctp_wrap GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(events_test config_test hq_test)
endif ()