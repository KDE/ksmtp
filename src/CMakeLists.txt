# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_library(KPim6SMTP)
add_library(KPim6::SMTP ALIAS KPim6SMTP)
target_sources(
    KPim6SMTP
    PRIVATE
        job.cpp
        loginjob.cpp
        sendjob.cpp
        session.cpp
        sessionthread.cpp
        sessionuiproxy.cpp
        loginjob.h
        sessionthread_p.h
        session_p.h
        job.h
        serverresponse_p.h
        session.h
        sessionuiproxy.h
        job_p.h
        sendjob.h
)

ecm_qt_declare_logging_category(KPim6SMTP HEADER ksmtp_debug.h
        IDENTIFIER KSMTP_LOG CATEGORY_NAME org.kde.pim.ksmtp
        DESCRIPTION "ksmtp (ksmtp)" EXPORT KSMTP
)

if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim6SMTP PROPERTIES UNITY_BUILD ON)
endif()

ecm_generate_export_header(KPim6SMTP
    BASE_NAME ksmtp
    VERSION ${KSMTP_VERSION}
    DEPRECATED_BASE_VERSION 0
    USE_VERSION_HEADER
)
target_include_directories(
    KPim6SMTP
    INTERFACE
        "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim6/KSMTP;${KDE_INSTALL_INCLUDEDIR}/KPim6/KSMTP/ksmtp>"
)

if(WIN32)
    set(extra_LIBS ws2_32)
endif()

target_link_libraries(
    KPim6SMTP
    PUBLIC KF6::CoreAddons
    PRIVATE KF6::I18n KF6::KIOCore Qt::Network Sasl2::Sasl2 ${extra_LIBS}
)

set_target_properties(
    KPim6SMTP
    PROPERTIES
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

install(
    TARGETS KPim6SMTP
    EXPORT KPim6SMTPTargets
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/ksmtp_export.h ${KSMTP_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/KSMTP/ksmtp
)

install(
    FILES ${KSMTP_CamelCase_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/KSMTP/KSMTP
)
ecm_qt_install_logging_categories(EXPORT KSMTP FILE ksmtp.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
