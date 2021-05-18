
add_library(KPimSMTP)
add_library(KPim::SMTP ALIAS KPimSMTP)
target_sources(KPimSMTP PRIVATE
    job.cpp
    loginjob.cpp
    sendjob.cpp
    session.cpp
    sessionthread.cpp
    sessionuiproxy.cpp
)

ecm_qt_declare_logging_category(KPimSMTP HEADER ksmtp_debug.h
        IDENTIFIER KSMTP_LOG CATEGORY_NAME org.kde.pim.ksmtp
        DESCRIPTION "ksmtp (ksmtp)" EXPORT KSMTP
        )

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPimSMTP PROPERTIES UNITY_BUILD ON)
endif()

generate_export_header(KPimSMTP BASE_NAME ksmtp)
target_include_directories(KPimSMTP INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim/KSMTP;${KDE_INSTALL_INCLUDEDIR}/KPim/KSMTP/ksmtp>")

if (WIN32)
    set(extra_LIBS ws2_32)
endif()


target_link_libraries(KPimSMTP
PUBLIC
    KF5::CoreAddons
PRIVATE
    KF5::I18n
    KF5::KIOCore
    Qt::Network
    Sasl2::Sasl2
    ${extra_LIBS}
)

set_target_properties(KPimSMTP PROPERTIES
    VERSION ${KSMTP_VERSION}
    SOVERSION ${KSMTP_SOVERSION}
    EXPORT_NAME SMTP
)

ecm_generate_headers(KSMTP_CamelCase_HEADERS
    HEADER_NAMES
    Job
    LoginJob
    SendJob
    Session
    SessionUiProxy
    PREFIX KSMTP

    REQUIRED_HEADERS KSMTP_HEADERS
)

install(TARGETS KPimSMTP EXPORT KPimSMTPTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ksmtp_export.h
    ${KSMTP_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim/KSMTP/ksmtp
)

install(FILES
    ${KSMTP_CamelCase_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim/KSMTP/KSMTP
)
ecm_qt_install_logging_categories(EXPORT KSMTP FILE ksmtp.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

ecm_generate_pri_file(BASE_NAME KSMTP LIB_NAME KPimSMTP DEPS "" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR}/KPim/KSMTP)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
