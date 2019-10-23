cmake_minimum_required(VERSION 3.5)
set(PIM_VERSION "5.12.40")

project(KSMTP VERSION ${PIM_VERSION})

set(CMAKE_CXX_STANDARD 14)

# ECM setup
set(KF5_MIN_VERSION "5.63.0")

find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

include(GenerateExportHeader)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)

include(ECMSetupVersion)
include(FeatureSummary)
include(ECMQtDeclareLoggingCategory)



set(KSMTP_LIB_VERSION ${PIM_VERSION})
set(QT_REQUIRED_VERSION "5.11.0")
ecm_setup_version(PROJECT VARIABLE_PREFIX KSMTP
    VERSION_HEADER ${KSMTP_BINARY_DIR}/ksmtp_version.h
    PACKAGE_VERSION_FILE ${KSMTP_BINARY_DIR}/KPimSMTPConfigVersion.cmake
    SOVERSION 5
)


############ Find packages ############
find_package(Qt5Network ${QT_REQUIRED_VERSION} REQUIRED)

find_package(KF5CoreAddons ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5I18n ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5KIO ${KF5_MIN_VERSION} CONFIG REQUIRED)

find_package(Sasl2)
set_package_properties(Sasl2 PROPERTIES TYPE REQUIRED)

########### CMake Config Files ############
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KPimSMTP")

configure_package_config_file(
    "${KSMTP_SOURCE_DIR}/KPimSMTPConfig.cmake.in"
    "${KSMTP_BINARY_DIR}/KPimSMTPConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

add_definitions(-DTRANSLATION_DOMAIN=\"libksmtp5\")
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
   add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)
   add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x060000)
endif()

add_definitions(-DQT_NO_FOREACH)



########### Targets ###########
add_subdirectory(src)

if (BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(tests)
endif ()

########### Install Files ############
install(FILES
    "${KSMTP_BINARY_DIR}/KPimSMTPConfig.cmake"
    "${KSMTP_BINARY_DIR}/KPimSMTPConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(EXPORT KPimSMTPTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KPimSMTPTargets.cmake NAMESPACE KPim::)

install(FILES
    ${KSMTP_BINARY_DIR}/ksmtp_version.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim COMPONENT Devel
)

install(FILES ksmtp.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
