cmake_minimum_required(VERSION 3.16)
project(BasicSpanner CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# Automatically handle Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Define source files
file(GLOB_RECURSE SOURCES "src/**/*.cpp")
file(GLOB_RECURSE HEADERS "src/**/*.h")

# Create executable
add_executable(BasicSpanner
    src/main.cpp
    ${SOURCES}
    ${HEADERS}
)

# Create test executable
file(GLOB CORE_SOURCES "src/core/*.cpp")

# Create verification executable only if the source file exists
if (EXISTS "${CMAKE_SOURCE_DIR}/verify_basic_network.cpp")
  add_executable(verify_basic_network
      verify_basic_network.cpp
      ${CORE_SOURCES}
  )
endif()

# Link libraries
target_link_libraries(BasicSpanner
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
)



if (TARGET verify_basic_network)
  target_link_libraries(verify_basic_network
      Qt6::Core
      Qt6::Widgets
      Qt6::Charts
  )
endif()

# Include directories
target_include_directories(BasicSpanner PRIVATE
    src/core
    src/gui
    src/gui/panels
)



if (TARGET verify_basic_network)
  target_include_directories(verify_basic_network PRIVATE
      src/core
      src/gui
      src/gui/panels
  )
endif()

# Set properties for different build types
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(BasicSpanner PRIVATE DEBUG_BUILD)
endif()

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    if(MSVC)
        # MSVC compiler options
        target_compile_options(BasicSpanner PRIVATE
            /W3          # Warning level 3
            /permissive- # Disable non-conforming code
            /Zc:__cplusplus # Enable correct __cplusplus macro
            /EHsc        # Exception handling
        )
        
        # Set subsystem to console for proper output
        set_target_properties(BasicSpanner PROPERTIES
            WIN32_EXECUTABLE FALSE
        )
    endif()
    
    # Copy Qt DLLs to output directory
    add_custom_command(TARGET BasicSpanner POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Qt6::Core>
        $<TARGET_FILE:Qt6::Widgets>
        $<TARGET_FILE_DIR:BasicSpanner>
    )
    
    # Windows deployment
    find_program(QT_WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(QT_WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET BasicSpanner POST_BUILD
            COMMAND ${QT_WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:BasicSpanner>
            COMMENT "Deploying Qt libraries")
    endif()
endif()

# Install rules
install(TARGETS BasicSpanner
    RUNTIME DESTINATION bin
) 
