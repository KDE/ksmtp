# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_library(KPim${KF_MAJOR_VERSION}SMTP)
add_library(KPim${KF_MAJOR_VERSION}::SMTP ALIAS KPim${KF_MAJOR_VERSION}SMTP)
add_library(KPim${KF_MAJOR_VERSION}::SMTP ALIAS KPim${KF_MAJOR_VERSION}SMTP)
target_sources(KPim${KF_MAJOR_VERSION}SMTP PRIVATE
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

ecm_qt_declare_logging_category(KPim${KF_MAJOR_VERSION}SMTP HEADER ksmtp_debug.h
        IDENTIFIER KSMTP_LOG CATEGORY_NAME org.kde.pim.ksmtp
        DESCRIPTION "ksmtp (ksmtp)" EXPORT KSMTP
        )

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim${KF_MAJOR_VERSION}SMTP PROPERTIES UNITY_BUILD ON)
endif()

generate_export_header(KPim${KF_MAJOR_VERSION}SMTP BASE_NAME ksmtp)
target_include_directories(KPim${KF_MAJOR_VERSION}SMTP INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/KSMTP;${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/KSMTP/ksmtp>")

if (WIN32)
    set(extra_LIBS ws2_32)
endif()


target_link_libraries(KPim${KF_MAJOR_VERSION}SMTP
PUBLIC
    KF${KF_MAJOR_VERSION}::CoreAddons
PRIVATE
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::KIOCore
    Qt::Network
    Sasl2::Sasl2
    ${extra_LIBS}
)

set_target_properties(KPim${KF_MAJOR_VERSION}SMTP PROPERTIES
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

install(TARGETS KPim${KF_MAJOR_VERSION}SMTP EXPORT KPim${KF_MAJOR_VERSION}SMTPTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ksmtp_export.h
    ${KSMTP_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/KSMTP/ksmtp
)

install(FILES
    ${KSMTP_CamelCase_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/KSMTP/KSMTP
)
ecm_qt_install_logging_categories(EXPORT KSMTP FILE ksmtp.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

if (BUILD_QCH)
    ecm_add_qch(
        KPimSMTP_QCH
        NAME KPimSMTP
        BASE_NAME KPimSMTP
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${KSMTP_HEADERS}
        #MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        #IMAGE_DIRS "${CMAKE_SOURCE_DIR}/docs/pics"
        LINK_QCHS
            Qt${QT_MAJOR_VERSION}Core_QCH
            Qt${QT_MAJOR_VERSION}Gui_QCH
            Qt${QT_MAJOR_VERSION}Widgets_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            KSMTP_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()

ecm_generate_pri_file(BASE_NAME KSMTP LIB_NAME KPim${KF_MAJOR_VERSION}SMTP DEPS "" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR}/KPim/KSMTP)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
