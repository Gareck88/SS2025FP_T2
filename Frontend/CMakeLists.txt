cmake_minimum_required(VERSION 3.16)

project(AudioTranskriptor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt-Komponenten finden
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Network Concurrent Sql Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Concurrent Sql Multimedia)

# Platform-spezifische Quellcodedateien
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
    maccapturethread.h
    maccapturethread.cpp
)

# Hauptquellcodedateien
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
)

# Platform-spezifische Konfiguration
if(WIN32)
    qt_add_executable(AudioTranskriptor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${WIN_SOURCES}
    )
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PULSEAUDIO REQUIRED libpulse-simple)
    include_directories(PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})

    qt_add_executable(AudioTranskriptor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${LINUX_SOURCES}
    )
elseif(APPLE)
    # Architekturerkennung
    execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCHITECTURE OUTPUT_STRIP_TRAILING_WHITESPACE)

    # Alle möglichen Suchpfade
    set(SEARCH_PATHS_ARM64
        "/opt/homebrew/opt/postgresql@14/lib/libpq.dylib"
        "/opt/homebrew/lib/libpq.dylib"
        "/opt/homebrew/opt/libpq/lib/libpq.dylib"
    )

    set(SEARCH_PATHS_X86_64
        "/usr/local/opt/postgresql@14/lib/libpq.dylib"
        "/usr/local/lib/libpq.dylib"
        "/usr/local/opt/libpq/lib/libpq.dylib"
    )

    # Durchsuchung aller Pfade
    if(ARCHITECTURE STREQUAL "arm64")
        foreach(PATH IN LISTS SEARCH_PATHS_ARM64)
            if(EXISTS "${PATH}")
                set(PQ_LIB "${PATH}")
                break()
            endif()
        endforeach()
        set(QT_PLUGINS_DIR "/opt/homebrew/lib/qt/plugins")
    else()
        foreach(PATH IN LISTS SEARCH_PATHS_X86_64)
            if(EXISTS "${PATH}")
                set(PQ_LIB "${PATH}")
                break()
            endif()
        endforeach()
        set(QT_PLUGINS_DIR "/usr/local/lib/qt/plugins")
    endif()

    # Header-Pfad ableiten
    if(PQ_LIB MATCHES "postgresql@14")
        string(REPLACE "/lib/libpq.dylib" "/include" PQ_INCLUDE_DIR "${PQ_LIB}")
    else()
        string(REPLACE "/lib/libpq.dylib" "/include" PQ_INCLUDE_DIR "${PQ_LIB}")
    endif()

    if(NOT PQ_LIB)
        message(FATAL_ERROR "libpq.dylib nicht gefunden in:\n"
                "Apple Silicon: ${SEARCH_PATHS_ARM64}\n"
                "Intel: ${SEARCH_PATHS_X86_64}\n\n"
                "Installation prüfen mit:\n"
                "1. brew install postgresql@14\n"
                "2. ODER brew install libpq\n"
                "3. brew link --force libpq")
    else()
        message(STATUS "Gefundene PostgreSQL-Bibliothek: ${PQ_LIB}")
        message(STATUS "Verwendete Include-Pfade: ${PQ_INCLUDE_DIR}")
    endif()

    include_directories(${PQ_INCLUDE_DIR})

    qt_add_executable(AudioTranskriptor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${MAC_SOURCES}
    )

    target_link_libraries(AudioTranskriptor PRIVATE "${PQ_LIB}")

    # Qt SQL-Treiber konfigurieren
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    execute_process(
        COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_PLUGINS
        OUTPUT_VARIABLE QT_PLUGINS_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Treiber und Abhängigkeiten ins Bundle kopieren
    add_custom_command(TARGET AudioTranskriptor POST_BUILD
        # Verzeichnisse erstellen
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AudioTranskriptor>/../Frameworks"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AudioTranskriptor>/../PlugIns/sqldrivers"

        # Bibliotheken kopieren
        COMMAND ${CMAKE_COMMAND} -E copy
            "${PQ_LIB}"
            "$<TARGET_FILE_DIR:AudioTranskriptor>/../Frameworks/libpq.dylib"

        # Treiber kopieren
        COMMAND ${CMAKE_COMMAND} -E copy
            "${QT_PLUGINS_DIR}/sqldrivers/libqsqlpsql.dylib"
            "$<TARGET_FILE_DIR:AudioTranskriptor>/../PlugIns/sqldrivers/"

        # RPATH setzen
        COMMAND install_name_tool -add_rpath "@loader_path/../Frameworks"
            "$<TARGET_FILE_DIR:AudioTranskriptor>/../PlugIns/sqldrivers/libqsqlpsql.dylib"

        COMMENT "Kopiere PostgreSQL-Komponenten ins App-Bundle"
    )
endif()

# Python-Umgebung kopieren
set(PYTHON_SRC_DIR "${CMAKE_SOURCE_DIR}/python")
set(PYTHON_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/python")
add_custom_command(
    TARGET AudioTranskriptor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PYTHON_SRC_DIR}" "${PYTHON_DST_DIR}"
    COMMENT "Kopiere Python-Skripte"
)

# Haupt-Bibliotheken verlinken
target_link_libraries(AudioTranskriptor PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Multimedia
)

# Platform-spezifische Bibliotheken
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
endif()

# macOS-Bundle-Einstellungen
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

# Installationsziele
include(GNUInstallDirs)
install(TARGETS AudioTranskriptor
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AudioTranskriptor)
endif()
