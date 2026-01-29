/*
 * SPDX-FileCopyrightText: 2020-2024 The Apache Software Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileContributor: 2024-2025 Espressif Systems (Shanghai) CO LTD
 */

/****************************************************************************
 * Stub implementations for PTPD when PTP is not supported
 ****************************************************************************/

#include "ptpd.h"

/* Start PTP daemon - stub implementation */
int ptpd_start(FAR const char *interface)
{
    (void)interface;
    return -1; /* Not supported */
}

/* Stop PTP daemon - stub implementation */
int ptpd_stop(int pid)
{
    (void)pid;
    return -1; /* Not supported */
}

/* Get PTP daemon status - stub implementation */
int ptpd_status(int pid, FAR struct ptpd_status_s *status)
{
    (void)pid;
    (void)status;
    return -1; /* Not supported */
}