cmake_minimum_required(VERSION 3.20)
project(optigrab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(OPTIGRAB_BUILD_TESTS "Build unit tests" ON)

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
)
FetchContent_MakeAvailable(Catch2)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCDIO REQUIRED libcdio)
pkg_check_modules(LIBCDIO_CDDA REQUIRED libcdio_cdda)
pkg_check_modules(LIBCDIO_PARANOIA REQUIRED libcdio_paranoia)

add_library(optigrab_lib
  src/domain/Session.cpp
  src/domain/TrackRange.cpp
  src/services/Filename.cpp
  src/services/RipService.cpp
  src/cli/Tokenizer.cpp
  src/cli/History.cpp
  src/cli/LineReader.cpp
  src/cli/CommandHandler.cpp
  src/cli/commands/ListCommand.cpp
  src/cli/commands/SelectCommand.cpp
  src/cli/commands/DetailCommand.cpp
  src/cli/commands/SetCommand.cpp
  src/cli/commands/RipCommand.cpp
  src/cli/commands/HelpCommand.cpp
  src/cli/commands/ExitCommand.cpp
  src/cli/commands/ClsCommand.cpp
  src/adapters/linux/LinuxDriveEnumerator.cpp
  src/adapters/libcdio/LibcdioTocReader.cpp
  src/adapters/libcdio/LibcdioParanoiaExtractor.cpp
  src/adapters/cdparanoia/CdparanoiaExtractor.cpp
  src/adapters/ffmpeg/FfmpegExtractor.cpp
  src/adapters/ffmpeg/FfmpegEncoder.cpp
  src/adapters/null/NullMetadataProvider.cpp
  src/adapters/manual/ManualMetadataProvider.cpp
  src/app/CompositionRoot.cpp
  src/app/Application.cpp
)

add_library(optigrab::lib ALIAS optigrab_lib)

target_include_directories(optigrab_lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${LIBCDIO_INCLUDE_DIRS}
    ${LIBCDIO_CDDA_INCLUDE_DIRS}
    ${LIBCDIO_PARANOIA_INCLUDE_DIRS}
)

target_link_libraries(optigrab_lib
  PUBLIC
    ${LIBCDIO_LIBRARIES}
    ${LIBCDIO_CDDA_LIBRARIES}
    ${LIBCDIO_PARANOIA_LIBRARIES}
)

target_compile_options(optigrab_lib PRIVATE
  -Wall -Wextra -Wpedantic
  ${LIBCDIO_CFLAGS_OTHER}
  ${LIBCDIO_CDDA_CFLAGS_OTHER}
  ${LIBCDIO_PARANOIA_CFLAGS_OTHER}
)

target_link_directories(optigrab_lib PUBLIC
  ${LIBCDIO_LIBRARY_DIRS}
  ${LIBCDIO_CDDA_LIBRARY_DIRS}
  ${LIBCDIO_PARANOIA_LIBRARY_DIRS}
)

add_executable(optigrab src/main.cpp)
target_link_libraries(optigrab PRIVATE optigrab::lib)

if(OPTIGRAB_BUILD_TESTS)
  enable_testing()
  add_executable(optigrab_tests
    tests/TokenizerTest.cpp
    tests/TrackRangeTest.cpp
    tests/FilenameTest.cpp
    tests/SessionTest.cpp
    tests/CommandHandlerTest.cpp
    tests/RipServiceTest.cpp
    tests/FakeAdaptersTest.cpp
    tests/HistoryTest.cpp
  )
  target_link_libraries(optigrab_tests PRIVATE optigrab::lib Catch2::Catch2WithMain)
  target_include_directories(optigrab_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  include(CTest)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(Catch)
  catch_discover_tests(optigrab_tests)
endif()

install(TARGETS optigrab RUNTIME DESTINATION bin)
