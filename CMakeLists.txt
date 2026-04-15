cmake_minimum_required(VERSION 3.13)
project(qtsparkle)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()

include(CMakePackageConfigHelpers)
include(cmake/Translations.cmake)

option(BUILD_STATIC_LIBS "Static build" OFF)
option(BUILD_SHARED_LIBS "Shared build" ON)

if(MSVC)
  set(CMAKE_C_STANDARD 99)
else()
  set(CMAKE_C_STANDARD 11)
endif()

set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  list(APPEND COMPILE_OPTIONS -MP -W4 -wd4702)
else()
  list(APPEND COMPILE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -Wunused
    -Wshadow
    -Wundef
    -Wuninitialized
    -Wredundant-decls
    -Wcast-align
    -Winit-self
    -Wmissing-include-dirs
    -Wmissing-declarations
    -Wstrict-overflow=2
    -Wunused-parameter
    -Wformat=2
    -Wdisabled-optimization
    $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
    $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
  )
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND COMPILE_OPTIONS -Wno-unused-command-line-argument)
  endif()
endif()

add_compile_options(${COMPILE_OPTIONS})

set(QT_VERSION_MAJOR 6)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED Core Network Widgets LinguistTools)
set(QT_LIBRARIES Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets)
set(QT_LUPDATE_EXECUTABLE Qt${QT_VERSION_MAJOR}::lupdate)
set(QT_LRELEASE_EXECUTABLE Qt${QT_VERSION_MAJOR}::lrelease)
set(QT_SUFFIX "-qt${QT_VERSION_MAJOR}")

add_definitions(
  -DQT_STRICT_ITERATORS
  -DQT_NO_CAST_FROM_BYTEARRAY
  -DQT_USE_QSTRINGBUILDER
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_TO_ASCII
  -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
  -DQT_NO_FOREACH
  -DQT_ASCII_CAST_WARNINGS
  -DQT_NO_CAST_FROM_ASCII
  -DQT_NO_KEYWORDS
  -DQT_NO_SIGNALS_SLOTS_KEYWORDS
)

message("- Building with Qt ${QT_VERSION_MAJOR}")

add_subdirectory(src)
add_subdirectory(exampleapp)

install(EXPORT ${PROJECT_NAME}${QT_SUFFIX}Targets
  NAMESPACE ${PROJECT_NAME}${QT_SUFFIX}::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}${QT_SUFFIX}
)

configure_package_config_file(${PROJECT_NAME}Config.cmake.in ${PROJECT_NAME}${QT_SUFFIX}Config.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}${QT_SUFFIX})

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}${QT_SUFFIX}Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  FRAMEWORK DESTINATION ${FRAMEWORK_INSTALL_DIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${QT_SUFFIX}Config.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}${QT_SUFFIX})
