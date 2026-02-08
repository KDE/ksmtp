# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(logintest logintest.cpp)
target_link_libraries(
    logintest
    KPim6SMTP
    Qt::Core
    KF6::KIOWidgets
)
