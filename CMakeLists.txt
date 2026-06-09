cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("TriCNES-cpp")


# Add source to this project's executable.
add_executable(${PROJECT_NAME} "src/TriCNES/Emulator.cpp" "src/TriCNES-cpp.cpp" )

add_executable(${PROJECT_NAME}-benchmark "src/TriCNES/Emulator.cpp" "src/TriCNES-cpp-benchmark.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TriCNES-cpp PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.


list(PREPEND CMAKE_PREFIX_PATH "C:/SDL/SDL2-2.32.10/")
find_package(SDL2 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)