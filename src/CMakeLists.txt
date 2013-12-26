set(libki18n_SRCS
    klocalizedstring.cpp
    kcatalog.cpp
    kuitmarkup.cpp
    common_helpers.cpp
)

add_library(KF5I18n ${libki18n_SRCS})
generate_export_header(KF5I18n BASE_NAME KI18n)
add_library(KF5::I18n ALIAS KF5I18n)

target_link_libraries(KF5I18n PUBLIC Qt5::Core)

set_target_properties(KF5I18n PROPERTIES VERSION   ${KI18N_VERSION_STRING}
                                         SOVERSION ${KI18N_SOVERSION}
                                         EXPORT_NAME I18n
)

install(TARGETS KF5I18n EXPORT KF5I18nTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    klocalizedstring.h
    kuitmarkup.h
    kuitsetup.h
    ${CMAKE_CURRENT_BINARY_DIR}/ki18n_export.h
    DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel
)

include(ECMGenerateHeaders)
ecm_generate_headers(
    KLocalizedString
    KuitSetup
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/KI18n
        DESTINATION  ${INCLUDE_INSTALL_DIR} COMPONENT Devel)

### ktranscript plugin

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ktranscript.lut.h
    COMMAND ${PERL_EXECUTABLE} ${KJS_CREATE_HASH_TABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/ktranscript.cpp
            > ${CMAKE_CURRENT_BINARY_DIR}/ktranscript.lut.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ktranscript.cpp
)
set_property(
    SOURCE ktranscript.cpp
    APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ktranscript.lut.h
)

set(ktranscript_LIB_SRCS
    ktranscript.cpp
    common_helpers.cpp
)

add_library(ktranscript MODULE ${ktranscript_LIB_SRCS})
set_target_properties(ktranscript PROPERTIES PREFIX "") # remove lib prefix
target_link_libraries(ktranscript PRIVATE KF5::JS Qt5::Core)

install(TARGETS ktranscript DESTINATION ${PLUGIN_INSTALL_DIR})
