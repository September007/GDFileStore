cmake_minimum_required(VERSION 3.20.0)
# Enable C++20
set(CMAKE_CXX_STANDARD 20)
# Set C++20 directories
#add_compile_options(-isystem /usr/include/c++/9)
# vcpkg <--
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()
#set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH};$ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_DEFAULT_TRIPLET}/share)
if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
    message("set vcpkg as env says $ENV{VCPKG_DEFAULT_TRIPLET}")
    set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
else()
    message("set vcpkg as cache says ${VCPKG_TARGET_TRIPLET}")
endif()

# parse regex_path
function( parse_regex_path return_set regex_path)
    set(regex_path ${regex_path} ${ARGN})
    file(GLOB SourceFiles ${regex_path})
    set(${return_set} )
    foreach(whom ${SourceFiles})
        STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME  ${whom})
        set(${return_set} ${${return_set}} ${whom})
    endforeach()
    set(${return_set} ${${return_set}} CACHE STRING "seems like only cache varible can stay alive out of function scope" )
endfunction()
 
#add_compile_options(-lm)
option(ASAN_ENABLED "Build this target with AddressSanitizer" ON)
## disable this
#if(ASAN_ENABLED)
#  if(MSVC)
#    add_compile_options( /fsanitize=address )
#  else()
#    add_compile_options(-fsanitize=address )
#    add_link_options( -fsanitize=address)
#  endif()
#endif()
option(test_on "will build tests" ON)

Project(GDFileStore)
parse_regex_path(lib_sources "./src/GD*.cpp" "./src/deps/*.cpp")
add_library(GDFileStore ${lib_sources})
# these definitions is for future
add_compile_definitions(GDVERSION=1 LOCAL_VERSION=1 MIGRATED_VERSION=2)
include_directories( ./test ./src ./src/deps "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
message("add include vcpkg dirs for intelli: $ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
# import gtest for test
find_package(GTest CONFIG REQUIRED)
# find_package(Boost REQUIRED  ) replace boost::format with fmt
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED) 
find_package(RocksDB CONFIG REQUIRED) 
find_package(rpclib CONFIG REQUIRED) 
find_package(benchmark CONFIG REQUIRED)
#find_package(concurrencpp CONFIG REQUIRED)    
#find_package(HPX CONFIG REQUIRED)
# head only
find_package(nlohmann_json CONFIG REQUIRED)

#target_link_libraries(main PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
set (dependencis 
            PUBLIC  GTest::gtest 
            PRIVATE  spdlog::spdlog_header_only RocksDB::rocksdb rpclib::rpc
#                       concurrencpp::concurrencpp
                        benchmark::benchmark benchmark::benchmark_main
)
if(test_on)
#
    add_compile_definitions(TEST_ON)
    parse_regex_path(tests_path "./test/*.cpp")
   # message("tests:${tests_path}")
    foreach(test ${tests_path})
        STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" TestName  ${test})
        # avoid empty test_main
        if(${TestName} MATCHES "test_main")
            continue()
        endif()
        add_executable(${TestName} ${test} "./test/test_main.cpp" )
        target_link_libraries(${TestName} ${dependencis} GDFileStore)
    endforeach()
    # 
    list(LENGTH tests_path tests_count)
    if(tests_count GREATER 1)# with test_main
        add_executable(all_tests ${tests_path} "./test/test_main.cpp" "test/test_main.cpp"  )
        target_link_libraries(all_tests ${dependencis} GDFileStore)
    endif()
endif()

add_executable(syn_asyn ./test/benchmark/syn_asyn.cpp ${lib})
target_link_libraries(syn_asyn ${dependencis} GDFileStore)

add_executable(syn_asyn_native ./test/native/syn_asyn_native.cpp ${lib})
target_link_libraries(syn_asyn_native ${dependencis} GDFileStore)