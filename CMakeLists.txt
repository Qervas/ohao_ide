cmake_minimum_required(VERSION 3.16)
project(ohao_IDE VERSION 1.0.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Pdf PdfWidgets WebEngineCore WebEngineWidgets Gui PrintSupport)

# Automatically find all source and header files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h" "src/*.hpp")

# Print found files for debugging
message("Found source files: ${SOURCES}")
message("Found header files: ${HEADERS}")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Pdf
    Qt6::PdfWidgets
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets
    Qt6::Gui
    Qt6::PrintSupport
)

# Add include directories for Qt
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6Network_INCLUDE_DIRS}
    ${Qt6Pdf_INCLUDE_DIRS}
    ${Qt6PdfWidgets_INCLUDE_DIRS}
    ${Qt6WebEngineCore_INCLUDE_DIRS}
    ${Qt6WebEngineWidgets_INCLUDE_DIRS}
    ${Qt6Gui_INCLUDE_DIRS}
    ${Qt6PrintSupport_INCLUDE_DIRS}
)