cmake_minimum_required(VERSION 3.10)

project(qalculate-helper CXX)

IF(EXISTS "${PROJECT_SOURCE_DIR}/.build_env")
  message(STATUS "loading .build_env")
  file(STRINGS "${PROJECT_SOURCE_DIR}/.build_env" CONFIG REGEX "^[ ]*[A-Za-z0-9_]+[ ]*=")
  list(TRANSFORM CONFIG STRIP)
  list(TRANSFORM CONFIG REPLACE "([^=]+)=[ ]*(.*)" "set(\\1 \"\\2\")\n")
  cmake_language(EVAL CODE ${CONFIG})
ENDIF()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wformat -ansi -O3 -D_FORTIFY_SOURCE=2 -fPIE -pie -fstack-protector-strong")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
IF(NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z muldefs -Wl,-z relro -Wl,-z now")
ENDIF()

include_directories(include/)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBQALCULATE REQUIRED libqalculate)
pkg_check_modules(MPFR REQUIRED mpfr)
pkg_check_modules(GMP REQUIRED gmp)
pkg_check_modules(XML2 REQUIRED libxml-2.0)
pkg_check_modules(CURL REQUIRED libcurl)

include_directories(
  ${LIBQALCULATE_INCLUDE_DIRS}
  ${MPFR_INCLUDE_DIRS}
  ${GMP_INCLUDE_DIRS}
  ${XML2_INCLUDE_DIRS}
  ${CURL_INCLUDE_DIRS}
)

IF(APPLE)
  find_library (ICUUC_LIBRARY icucore)
ELSE()
  find_library(ICUUC_LIBRARY icuuc)
ENDIF()

add_executable(qalculate-helper
  src/security_util.cpp
  src/exchange_update_exception.cpp
  src/qalculate_exception.cpp
  src/qalculate-helper.cpp
  src/timeout_exception.cpp
)

target_link_libraries(qalculate-helper
  ${LIBQALCULATE_LINK_LIBRARIES}
  ${MPFR_LINK_LIBRARIES}
  ${GMP_LINK_LIBRARIES}
  ${ICUUC_LINK_LIBRARIES}
  ${XML2_LINK_LIBRARIES}
  ${CURL_LINK_LIBRARIES}
  ${PTHREAD_LINK_LIBRARIES}
)

IF(UID)
  message(STATUS "Building with setuid")
  find_library(CAPNG_LIBRARY cap-ng)
  target_link_libraries(qalculate-helper ${CAPNG_LIBRARY})
  target_compile_definitions(qalculate-helper PUBLIC UID=${UID})
ENDIF()

IF(SECCOMP)
  message(STATUS "Building with seccomp")
  find_library(SECCOMP_LIBRARY seccomp)
  target_link_libraries(qalculate-helper ${SECCOMP_LIBRARY})
  target_compile_definitions(qalculate-helper PUBLIC "SECCOMP")

  IF (SECCOMP_ALLOW_CLONE)
    target_compile_definitions(qalculate-helper PUBLIC "SECCOMP_ALLOW_CLONE")
  ENDIF()
ENDIF()

IF(SKIP_DEFANG)
  target_compile_definitions(qalculate-helper PUBLIC "SKIP_DEFANG")
ENDIF()

add_custom_command(TARGET qalculate-helper POST_BUILD
  COMMAND ${CMAKE_STRIP} $<TARGET_FILE:qalculate-helper>
)
