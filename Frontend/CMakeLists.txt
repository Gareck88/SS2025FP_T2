cmake_minimum_required(VERSION 3.16)

project(AudioTranskriptor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Network Concurrent Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Concurrent Sql)

# Findet die PostgreSQL-Bibliothek (sollte für Win, Linux & Mac funktionieren)
find_package(PostgreSQL REQUIRED)

set(LINUX_SOURCES
    pulsecapturethread.h
    pulsecapturethread.cpp
)

set(WIN_SOURCES
    wincapturethread.h
    wincapturethread.cpp
    ringbuffer.h
)

set(MAC_SOURCES
)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    settingswizard.h
    settingswizard.cpp
    capturethread.h
    capturethread.cpp
    audiofactory.h
    audiofactory.cpp
    wavwriterthread.h
    wavwriterthread.cpp
    transcription.h
    transcription.cpp
    speakereditordialog.h
    speakereditordialog.cpp
    texteditordialog.h
    texteditordialog.cpp
    filemanager.h
    filemanager.cpp
    asrprocessmanager.h
    asrprocessmanager.cpp
    transcriptpdfexporter.h
    transcriptpdfexporter.cpp
    taggeneratormanager.h
    taggeneratormanager.cpp
    installationdialog.h
    installationdialog.cpp
    pythonenvironmentmanager.h
    pythonenvironmentmanager.cpp
    databasemanager.h
    databasemanager.cpp
    searchdialog.h
    searchdialog.cpp
    multisearchdialog.h
    multisearchdialog.cpp
)


if(WIN32)
    qt_add_executable(AudioTranskriptor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${WIN_SOURCES}
    )
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    # libpulse-simple
    pkg_check_modules(PULSEAUDIO REQUIRED libpulse-simple)

    include_directories(PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})

    qt_add_executable(AudioTranskriptor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${LINUX_SOURCES}
    )
elseif(APPLE)
    # Use Homebrew's PostgreSQL@14 path
    set(LIBPQ_ROOT "/opt/homebrew/lib/postgresql@14")

    qt_add_executable(AudioTranskriptor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${MAC_SOURCES}
    )

    # Include libpq headers if needed (adjust if headers are elsewhere)
    # include_directories("/opt/homebrew/include")

    # Link the libpq library
    # target_link_directories(AudioTranskriptor PRIVATE ${LIBPQ_ROOT})
    # target_link_libraries(AudioTranskriptor PRIVATE pq)

endif()


# Python-Umgebung einrichten
set(PYTHON_SRC_DIR "${CMAKE_SOURCE_DIR}/python")
set(PYTHON_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/python")

add_custom_command(
    TARGET AudioTranskriptor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PYTHON_SRC_DIR}" "${PYTHON_DST_DIR}"
    COMMENT "Kopiere python/ nach ${PYTHON_DST_DIR}"
)

target_link_libraries(AudioTranskriptor PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Sql
)

# Verlinkt die PostgreSQL-Bibliothek
target_link_libraries(AudioTranskriptor PRIVATE PostgreSQL::PostgreSQL)

if(WIN32)
    target_link_libraries(AudioTranskriptor PRIVATE
        ole32
        uuid
        winmm
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(AudioTranskriptor PRIVATE
        ${PULSEAUDIO_LIBRARIES}
    )
elseif(APPLE)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.AudioTranskriptor)
endif()

set_target_properties(AudioTranskriptor PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS AudioTranskriptor
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AudioTranskriptor)
endif()

