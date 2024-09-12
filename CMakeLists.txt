# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(PIM_VERSION "6.2.2")

project(KSMTP VERSION ${PIM_VERSION})


# ECM setup
set(KF_MIN_VERSION "6.3.0")
set(QT_REQUIRED_VERSION "6.6.0")
find_package(ECM ${KF_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMGenerateExportHeader)
include(ECMGenerateHeaders)

include(ECMSetupVersion)
include(FeatureSummary)
include(KDEGitCommitHooks)
include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)
include(ECMFeatureSummary)

include(ECMAddQch)


option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")


set(KSMTP_LIB_VERSION ${PIM_VERSION})

ecm_setup_version(PROJECT VARIABLE_PREFIX KSMTP
    VERSION_HEADER ${KSMTP_BINARY_DIR}/src/ksmtp_version.h
    PACKAGE_VERSION_FILE ${KSMTP_BINARY_DIR}/KPim6SMTPConfigVersion.cmake
    SOVERSION 6
)


############ Find packages ############
find_package(Qt6Network ${QT_REQUIRED_VERSION} REQUIRED)
find_package(KF6CoreAddons ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6I18n ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6KIO ${KF_MIN_VERSION} CONFIG REQUIRED)

find_package(Sasl2)
set_package_properties(Sasl2 PROPERTIES TYPE REQUIRED)

########### CMake Config Files ############

add_definitions(-DQT_NO_CONTEXTLESS_CONNECT)
add_definitions(-DTRANSLATION_DOMAIN=\"libksmtp6\")
ecm_set_disabled_deprecation_versions(QT 6.7.2 KF 6.4.0)

option(USE_UNITY_CMAKE_SUPPORT "Use UNITY cmake support (speedup compile time)" OFF)

set(COMPILE_WITH_UNITY_CMAKE_SUPPORT OFF)
if(USE_UNITY_CMAKE_SUPPORT)
    set(COMPILE_WITH_UNITY_CMAKE_SUPPORT ON)
endif()


########### Targets ###########
add_subdirectory(src)

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KPim6SMTP")
if(BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KPim6SMTP_QCH
        FILE KPim6SMTPQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
)
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KPim6SMTPQchTargets.cmake\")")
endif()

configure_package_config_file(
    "${KSMTP_SOURCE_DIR}/KPimSMTPConfig.cmake.in"
    "${KSMTP_BINARY_DIR}/KPim6SMTPConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

if(BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(tests)
endif()

########### Install Files ############
install(FILES
    "${KSMTP_BINARY_DIR}/KPim6SMTPConfig.cmake"
    "${KSMTP_BINARY_DIR}/KPim6SMTPConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(EXPORT KPim6SMTPTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KPim6SMTPTargets.cmake NAMESPACE KPim6::)

install(FILES
    ${KSMTP_BINARY_DIR}/src/ksmtp_version.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/KSMTP COMPONENT Devel
)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
ki18n_install(po)
ecm_feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
