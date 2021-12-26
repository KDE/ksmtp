# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(logintest logintest.cpp)
target_link_libraries(logintest KPimSMTP Qt${QT_MAJOR_VERSION}::Core KF5::KIOWidgets)
