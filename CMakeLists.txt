cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project("TriCNES-cpp")

add_subdirectory("ZipLib")

# Add source to this project's executable.
add_executable(${PROJECT_NAME} "src/TriCNES/Emulator.cpp" "src/TriCNES-cpp.cpp" )

add_executable(${PROJECT_NAME}-benchmark "src/TriCNES/Emulator.cpp" "src/TriCNES-cpp-benchmark.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

if (MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /Zc:__cplusplus)
endif()

# TODO: Add tests and install targets if needed.

list(PREPEND CMAKE_PREFIX_PATH "C:/SDL/SDL3-3.4.10/")
list(PREPEND CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64/lib/cmake/")
find_package(SDL3 REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Gui Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Gui Qt6::Widgets)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PRIVATE ZipLib)