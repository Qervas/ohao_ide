cmake_minimum_required(VERSION 3.16)
project(ohao-ide VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets 
    Core 
    Gui 
    Network
    WebEngineCore
    WebEngineWidgets
    Pdf
    PdfWidgets
)
qt_standard_project_setup()

# Automatically find all source files recursively
file(GLOB_RECURSE PROJECT_SOURCES
    src/*.cpp
    src/*.h
)

qt_add_executable(ohao-ide
    ${PROJECT_SOURCES}
)

# Add include directories
target_include_directories(ohao-ide PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ohao-ide PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets
    Qt6::Pdf
    Qt6::PdfWidgets
)

install(TARGETS ohao-ide
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


# Install documentation files
install(FILES
    LICENSE
    README.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

install(FILES resources.qrc
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
)