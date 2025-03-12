cmake_minimum_required (VERSION 3.20)

project (DeltaImage VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(DeltaImage
	main.cpp
	Differentiate.cpp
	Filter.cpp
)

include(FetchContent)

FetchContent_Declare(
	STBI
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG "master"
)
FetchContent_MakeAvailable(STBI)

include(FetchContent)

FetchContent_Declare(JSON URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(JSON)

target_link_libraries(DeltaImage PRIVATE nlohmann_json::nlohmann_json)

target_include_directories(DeltaImage
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/stbi-src
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/json-src/include
)

target_link_directories(DeltaImage
    PUBLIC ${CMAKE_BINARY_DIR}/_deps/stbi-src
)