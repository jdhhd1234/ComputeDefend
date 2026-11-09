
cmake_minimum_required(VERSION 3.21)

project(ComputerDefender
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# CPR 라이브러리 설정
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS_SSL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.12.0
    GIT_SHALLOW TRUE
)

# nlohmann/json 라이브러리
set(JSON_BuildTests OFF CACHE INTERNAL "")

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)

# cpr은 libpsl을 Meson으로 빌드한다. PATH에 cl.exe가 있으면 Meson은 개발자 환경이
# 이미 설정됐다고 판단해 VS 환경 활성화를 건너뛰고, INCLUDE가 없어 C1034로 실패한다.
# 이번 configure 동안만 MSVC bin을 PATH에서 빼서 Meson이 vswhere로 환경을 잡게 한다.
if(MSVC AND NOT DEFINED ENV{INCLUDE})
    file(TO_CMAKE_PATH "$ENV{PATH}" _cd_path_entries)
    list(FILTER _cd_path_entries EXCLUDE REGEX "/VC/Tools/MSVC/")
    string(REPLACE "/" "\\" _cd_native_path "${_cd_path_entries}")
    set(ENV{PATH} "${_cd_native_path}")
endif()

# 라이브러리 자동 다운로드 및 연결 준비
FetchContent_MakeAvailable(cpr json)

add_executable(ComputerDefender
    src/main.cpp
    src/main.hpp
)

target_compile_features(ComputerDefender
    PRIVATE cxx_std_20
)

target_link_libraries(ComputerDefender
    PRIVATE
        cpr::cpr
        nlohmann_json::nlohmann_json
)

if(WIN32)
    target_compile_definitions(ComputerDefender
        PRIVATE UNICODE _UNICODE
    )

    # cpr, curl, zlib DLL을 exe 옆에 복사한다. 없으면 실행 즉시 0xC0000135로 종료된다.
    add_custom_command(TARGET ComputerDefender POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:ComputerDefender>
            $<TARGET_FILE_DIR:ComputerDefender>
        COMMAND_EXPAND_LISTS
    )
endif()
