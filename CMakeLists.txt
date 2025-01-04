cmake_minimum_required(VERSION 3.16)
project(ohao_IDE VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# set debug on
set(CMAKE_BUILD_TYPE Debug)

# Include GNUInstallDirs for standard installation directories
include(GNUInstallDirs)

# Make CUPS optional
find_package(Cups)
if(CUPS_FOUND)
    add_definitions(-DWITH_CUPS)
endif()

find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Widgets
    Gui
    PrintSupport
    Pdf
    PdfWidgets
    Network
)

# Create the executable with resources
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/mainwindow.cpp
    src/codeeditor.cpp
    src/projecttree.cpp
    src/filepreview.cpp
    src/terminal.cpp
    src/cpphighlighter.cpp
    resources.qrc
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::PrintSupport
    Qt6::Pdf
    Qt6::PdfWidgets
    Qt6::Network
)

# Installation rules with proper destinations
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install documentation files
install(FILES 
    LICENSE 
    README.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)