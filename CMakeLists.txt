cmake_minimum_required(VERSION 3.10)

project(qalculate-helper CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wformat -ansi -O3 -D_FORTIFY_SOURCE=2 -fPIE -pie -fstack-protector-strong")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
IF(NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z muldefs  -Wl,-z relro -Wl,-z now")
ENDIF()

include_directories(include/)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBQALCULATE REQUIRED libqalculate)
pkg_check_modules(MPFR REQUIRED mpfr)
pkg_check_modules(GMP REQUIRED gmp)
pkg_check_modules(XML2 REQUIRED libxml-2.0)
pkg_check_modules(CURL REQUIRED libcurl)

pkg_check_modules(CAPNG cap-ng)
pkg_check_modules(SECCOMP seccomp)

include_directories(
  ${LIBQALCULATE_INCLUDE_DIRS}
  ${MPFR_INCLUDE_DIRS}
  ${GMP_INCLUDE_DIRS}
  ${XML2_INCLUDE_DIRS}
  ${CURL_INCLUDE_DIRS}
)

find_library(MPFR_LIBRARY mpfr)
find_library(GMP_LIBRARY gmp)
IF(APPLE)
  find_library (ICUUC_LIBRARY icucore)
ELSE()
  find_library(ICUUC_LIBRARY icuuc)
ENDIF()
find_library(XML2_LIBRARY xml2)
find_library(CURL_LIBRARY curl)
find_library(PTHREAD_LIBRARY pthread)
find_library(CAPNG_LIBRARY cap-ng)
find_library(SECCOMP_LIBRARY seccomp)

add_executable(qalculate-helper
  src/security_util.cpp
  src/exchange_update_exception.cpp
  src/qalculate_exception.cpp
  src/qalculate-helper.cpp
  src/timeout_exception.cpp
)

target_link_libraries(qalculate-helper
  ${LIBQALCULATE_LINK_LIBRARIES}
  ${MPFR_LIBRARY}
  ${GMP_LIBRARY}
  ${ICUUC_LIBRARY}
  ${XML2_LIBRARY}
  ${CURL_LIBRARY}
  ${PTHREAD_LIBRARY}
)

IF(SETUID)
  message(STATUS "Building with setuid")
  target_link_libraries(qalculate-helper ${CAPNG_LIBRARY})
  target_compile_definitions(qalculate-helper PUBLIC "UID")
ENDIF()

IF(SECCOMP)
  message(STATUS "Building with seccomp")
  target_link_libraries(qalculate-helper ${SECCOMP_LIBRARY})
  target_compile_definitions(qalculate-helper PUBLIC "SECCOMP")

  IF (SECCOMP_ALLOW_CLONE)
    target_compile_definitions(qalculate-helper PUBLIC "SECCOMP_ALLOW_CLONE")
  ENDIF()
ENDIF()

add_custom_command(TARGET qalculate-helper POST_BUILD
  COMMAND ${CMAKE_STRIP} $<TARGET_FILE:qalculate-helper>
)
