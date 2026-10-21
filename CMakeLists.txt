cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)

project(qt_prime VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Protobuf Widgets Network WebSockets)
find_package(libsodium REQUIRED)

qt_standard_project_setup(REQUIRES 6.10)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(appqt_prime
    main.cpp
)

qt_add_protobuf(my_messages_lib
    PROTO_FILES
        message.proto
)

qt_add_qml_module(appqt_prime
    URI qt_prime
    QML_FILES
        Main.qml
        SOURCES client.h client.cpp
        SOURCES sslwebsocket.h sslwebsocket.cpp
        QML_FILES ChatScreen.qml
        QML_FILES ChatsScreen.qml
        QML_FILES LoginScreen.qml
        QML_FILES RegisterScreen.qml
        QML_FILES SearchScreen.qml
        QML_FILES SettingsScreen.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appqt_prime PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appqt_prime
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appqt_prime
    PRIVATE Qt6::Quick
    Qt6::Protobuf
    Qt6::Network
    Qt6::WebSockets
    libsodium::libsodium
    my_messages_lib
)

if (ANDROID)
    include(FetchContent)
    FetchContent_Declare(
        android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)

    add_android_openssl_libraries(appqt_prime)
endif()

install(TARGETS appqt_prime
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
