/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2001-2004 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file install/i_opencbm.c \n
** \author Spiro Trikaliotis \n
** \version $Id: i_opencbm.c,v 1.10.2.1 2007-03-11 13:46:01 strik Exp $ \n
** \n
** \brief Functions for accessing the driver
**
****************************************************************/

/*! The name of the executable */
#ifndef DBG_PROGNAME
    #define DBG_PROGNAME "INSTCBM.EXE"
#endif // #ifndef DBG_PROGNAME

/*! Mark: We are in user-space (for debug.h) */
// #define DBG_USERMODE

// Include the functionality of the DLL into the install
// Application

#if 1 // ---- @SRT 

#include "../lib/WINBUILD/i_opencbm.c"

#else

#include "windows.h"
#include "windowsx.h"

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#ifndef DBG_PROGNAME
    #define DBG_PROGNAME "OPENCBM.DLL"
#endif // #ifndef DBG_PROGNAME

#include "debug.h"

#include <winioctl.h>
#include "cbmioctl.h"

#include <stdlib.h>
#include <stddef.h>

#include "i_opencbm.h"

#include "version.h"
#include "archlib.h"

#endif


/*! \brief Tell the driver to update its settings

 This function tells the driver to update its settings
 after they have been changed.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 If cbm_driver_open() did not succeed, it is illegal to 
 call cbm_driver_close().
*/

VOID
CbmInstallUpdate(VOID)
{
    CBM_FILE HandleDevice;
/* @@@@@@@
    FUNC_ENTER();

    if (cbmarch_driver_open(&HandleDevice, 0) == 0)
    {
        cbm_ioctl(HandleDevice, CBMCTRL(UPDATE), NULL, 0, NULL, 0);
        cbmarch_driver_close(HandleDevice);
    }
/* @@@@@@@
*/
    FUNC_LEAVE();
}

/*! \brief Tell the driver to update its settings

 This function tells the driver to update its settings
 after they have been changed.

 \param HandleDevice  
   A CBM_FILE which contains the file handle of the driver.

 If cbm_driver_open() did not succeed, it is illegal to 
 call cbm_driver_close().
*/

BOOL
CbmTestIrq(VOID)
{
    CBM_FILE HandleDevice;
    BOOL error;

    FUNC_ENTER();

/* @@@@@@@
    if (cbmarch_driver_open(&HandleDevice, 0) == 0)
    {
        DBG_PRINT((DBG_PREFIX "Issuing CBMCTRL_TEST_IRQ"));
        error = cbm_ioctl(HandleDevice, CBMCTRL(TEST_IRQ), NULL, 0, NULL, 0) ? FALSE : TRUE;
        DBG_PRINT((DBG_PREFIX "Returning from CBMCTRL_TEST_IRQ"));
        cbmarch_driver_close(HandleDevice);
    }

    FUNC_LEAVE_BOOL(error);
/* @@@@@@@
*/
    FUNC_LEAVE_BOOL(FALSE);
}

#if DBG

#include <stdio.h>

/*! \brief Output contents of the debugging buffer

 This function outputs the contents of the kernel-mode
 debugging buffer to the screen.

 This function is for use of the installation routines only!
*/

VOID
CbmOutputDebuggingBuffer(VOID)
{
#define OUTPUT_BUFFER_LEN 0x20000
    CHAR *buffer;

    CBM_FILE HandleDevice;

    FUNC_ENTER();

    buffer = malloc(OUTPUT_BUFFER_LEN);

/* @@@@@@@
    if (buffer && cbmarch_driver_open(&HandleDevice, 0) == 0)
    {
        PCHAR p = buffer;
        PCHAR endLine;

        cbm_ioctl(HandleDevice, CBMCTRL(I_READDBG), NULL, 0, buffer, OUTPUT_BUFFER_LEN);
        cbmarch_driver_close(HandleDevice);

        printf("Output of the debugging buffer:\n\n");

        do {

            endLine = strchr(p, 13);
            if (endLine)
            {
                *endLine = 0;
            }
            printf("%s", p);

            p = endLine + 1;

        } while (endLine);
    }
/* @@@@@@@
*/

    if (buffer)
    {
        free(buffer);
    }

    FUNC_LEAVE();
}

#endif // #if DBG
