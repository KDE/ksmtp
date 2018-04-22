set(ksmtp_SRCS
    job.cpp
    loginjob.cpp
    sendjob.cpp
    session.cpp
    sessionthread.cpp
    sessionuiproxy.cpp
)

ecm_qt_declare_logging_category(ksmtp_SRCS HEADER ksmtp_debug.h IDENTIFIER KSMTP_LOG CATEGORY_NAME org.kde.pim.ksmtp)

add_library(KPimSMTP ${ksmtp_SRCS})

generate_export_header(KPimSMTP BASE_NAME ksmtp)

add_library(KPim::SMTP ALIAS KPimSMTP)
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
    Qt5::Network
    Sasl2::Sasl2
    ${extra_LIBS}
)

set_target_properties(KPimSMTP PROPERTIES
    VERSION ${KSMTP_VERSION_STRING}
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

ecm_generate_pri_file(BASE_NAME KSMTP LIB_NAME KPimSMTP DEPS "" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR}/KPim/KSMTP)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
