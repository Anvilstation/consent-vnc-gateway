cmake_minimum_required(VERSION 3.16)

project(ConsentVncGateway VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network DBus Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network DBus Sql)

set(PROJECT_SOURCES
    src/main.cpp
    src/config/AppConfig.cpp
    src/config/AppConfig.h
    src/domain/Models.cpp
    src/domain/Models.h
    src/application/Interfaces.h
    src/application/AccessCoordinator.cpp
    src/application/AccessCoordinator.h
    src/infrastructure/LocalHostIdentityProvider.cpp
    src/infrastructure/LocalHostIdentityProvider.h
    src/infrastructure/SqliteSessionRepository.cpp
    src/infrastructure/SqliteSessionRepository.h
    src/infrastructure/SystemVncController.cpp
    src/infrastructure/SystemVncController.h
    src/presentation/QtConsentPrompt.cpp
    src/presentation/QtConsentPrompt.h
    src/transport/GatewayAdaptor.cpp
    src/transport/GatewayAdaptor.h
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(consent-vnc-gateway MANUAL_FINALIZATION ${PROJECT_SOURCES})
else()
    add_executable(consent-vnc-gateway ${PROJECT_SOURCES})
endif()

target_include_directories(consent-vnc-gateway PRIVATE src)
target_link_libraries(consent-vnc-gateway PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Sql
)

add_custom_command(TARGET consent-vnc-gateway POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/config
            $<TARGET_FILE_DIR:consent-vnc-gateway>/config
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(consent-vnc-gateway)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    add_executable(access-coordinator-tests
        tests/AccessCoordinatorTest.cpp
        src/domain/Models.cpp
        src/domain/Models.h
        src/application/Interfaces.h
        src/application/AccessCoordinator.cpp
        src/application/AccessCoordinator.h
    )
    target_include_directories(access-coordinator-tests PRIVATE src)
    target_link_libraries(access-coordinator-tests PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME access-coordinator-tests COMMAND access-coordinator-tests)
endif()
