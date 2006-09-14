/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2004 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2004, 2006 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libiec/init.c \n
** \author Spiro Trikaliotis \n
** \version $Id: init.c,v 1.12 2006-09-14 19:15:17 strik Exp $ \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Initialize the IEC bus
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

/*! timeout values */

IEC_TIMEOUTS libiec_global_timeouts;

/*! Read timeout values from the registry */
#define READ_TIMEOUT_VALUE(_what_, _default_) \
    do { \
        libiec_global_timeouts._what_ = _default_; \
        if (HKey) cbm_registry_read_ulong(*HKey, L#_what_, &libiec_global_timeouts._what_); \
    } while (0)


/*! \internal \brief Initialize the global timeout template

 \param HKey
   Pointer to a handle with holds a registry key from which
   the timeout values are to be read.

   If this is NULL, no access to the registry is performed.
*/
static VOID
cbmiec_timeouts_init(IN PHANDLE HKey)
{
    HANDLE hKey;

    // reset related

    READ_TIMEOUT_VALUE(T_holdreset,                             100000); // 100 ms
    READ_TIMEOUT_VALUE(T_afterreset,                           2000000); // = 2 s

    // wait for listener related

    READ_TIMEOUT_VALUE(T_WaitForListener_Granu_T_H,                 10); // us

    // receive related

    READ_TIMEOUT_VALUE(T_1_RECV_WAIT_CLK_LOW_DATA_READY_GRANU,      20); // us
    READ_TIMEOUT_VALUE(T_2_Times,                                   80); //! x T_2, \todo 40
    READ_TIMEOUT_VALUE(T_2_RECV_WAIT_CLK_HIGH_T_NE,                 10); // us
    READ_TIMEOUT_VALUE(T_3_RECV_EOI_RECOGNIZED,                     70); // us
    READ_TIMEOUT_VALUE(T_4_Times,                                  200); //! x T_4, \todo 100
    READ_TIMEOUT_VALUE(T_4_RECV_WAIT_CLK_HIGH_AFTER_EOI_GRANU,      20); // us
    READ_TIMEOUT_VALUE(T_5_Times,                                 5000); // 500 //! x T_5, \todo Was 200
    READ_TIMEOUT_VALUE(T_5_RECV_BIT_WAIT_CLK_HIGH,                   1); // 10  //! us  \todo Was 10
    READ_TIMEOUT_VALUE(T_6_Times,                                  100); // x T_6
    READ_TIMEOUT_VALUE(T_6_RECV_BIT_WAIT_CLK_LOW,                   20); // us
    READ_TIMEOUT_VALUE(T_7_RECV_INTER_BYTE_DELAY,                   70); // us

    // IEC_WAIT related

    READ_TIMEOUT_VALUE(T_8_IEC_WAIT_LONG_DELAY,                     20); // us
    READ_TIMEOUT_VALUE(T_8_IEC_WAIT_SHORT_DELAY,                    10); // us

    // send related 

    READ_TIMEOUT_VALUE(T_9_Times,                                  100); // x T_9
    READ_TIMEOUT_VALUE(T_9_SEND_WAIT_DEVICES_T_AT,                  10); // us
    READ_TIMEOUT_VALUE(T_10_SEND_BEFORE_1ST_BYTE,                   20); // us
    READ_TIMEOUT_VALUE(T_11_SEND_BEFORE_BYTE_DELAY,                 50); // us
    READ_TIMEOUT_VALUE(T_12_SEND_AFTER_BYTE_DELAY,                 100); // us
    READ_TIMEOUT_VALUE(T_13_SEND_TURN_AROUND_LISTENER_TALKER_T_TK,  20); // us
    READ_TIMEOUT_VALUE(T_14_SEND_AT_END_DELAY,                     100); // us

    // sendbyte related:

    READ_TIMEOUT_VALUE(T_15_SEND_BEFORE_BIT_DELAY_T_S,              70); // us
    READ_TIMEOUT_VALUE(T_16_SEND_BIT_TIME_T_V,                      20); // us
    READ_TIMEOUT_VALUE(T_17_Times,                                  20); // x T_17
    READ_TIMEOUT_VALUE(T_17_SEND_FRAME_HANDSHAKE_T_F,              100); // us

    // parallel burst related

    READ_TIMEOUT_VALUE(T_PARALLEL_BURST_READ_BYTE_HANDSHAKED,   300000); // us
    READ_TIMEOUT_VALUE(T_PARALLEL_BURST_WRITE_BYTE_HANDSHAKED,  300000); // us
}
#undef READ_TIMEOUT_VALUE

/*! \brief Check if the cable works at all

 This function tries to find out if there is a cable connected,
 and if the type given is the right one.

 \param Pdx
   Pointer to the device extension.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.

 \todo
   Do a more sophisticated test
*/
static NTSTATUS
cbmiec_testcable(PDEVICE_EXTENSION Pdx) 
{
    NTSTATUS ntStatus = STATUS_PORT_DISCONNECTED;
    UCHAR ch;

    FUNC_ENTER();

//    CBMIEC_SET(PP_RESET_OUT|PP_ATN_OUT|PP_CLK_OUT|PP_DATA_OUT);

    /* check if the state of all lines is correct */

    ch = READ_PORT_UCHAR(IN_PORT);

    DBG_PRINT((DBG_PREFIX "############ Status: out: $%02x, in: $%02x ($%02x ^ $%02x)",
        READ_PORT_UCHAR(OUT_PORT), ch, ch ^ Pdx->IecOutEor, Pdx->IecOutEor));

#define READ(_x) ((((READ_PORT_UCHAR(OUT_PORT) ^ Pdx->IecOutEor)) & (_x)) ? 1 : 0)

#define SHOW(_x, _y) 
    // DBG_PRINT((DBG_PREFIX "CBMIEC_GET(" #_x ") = $%02x, READ(" #_y ") = $%02x", CBMIEC_GET(_x), READ(_y) ));

    do {
SHOW(PP_ATN_IN, PP_ATN_OUT);
        if (CBMIEC_GET(PP_ATN_IN) != READ(PP_ATN_OUT))
        {
            DBG_PRINT((DBG_PREFIX "ATN IN != ATN OUT"));
            break;
        }

SHOW(PP_CLK_IN, PP_CLK_OUT);
        if (CBMIEC_GET(PP_CLK_IN) != READ(PP_CLK_OUT))
        {
            DBG_PRINT((DBG_PREFIX "CLOCK IN != CLOCK OUT"));
            break;
        }

SHOW(PP_DATA_IN, PP_DATA_OUT);
        if (CBMIEC_GET(PP_DATA_IN) != READ(PP_DATA_OUT))
        {
            DBG_PRINT((DBG_PREFIX "DATA IN != DATA OUT"));
            break;
        }

SHOW(PP_RESET_IN, PP_RESET_OUT);
        if (CBMIEC_GET(PP_RESET_IN) != READ(PP_RESET_OUT))
        {
            DBG_PRINT((DBG_PREFIX "RESET IN != RESET OUT"));
            break;
        }

#undef SHOW

        /* if either ATN or RESET is set, we can play with them
         * in order to perform an even better test.
         */

        if (CBMIEC_GET(PP_ATN_IN))
        {
            CBMIEC_RELEASE(PP_ATN_OUT);

            if (CBMIEC_GET(PP_ATN_IN) != READ(PP_ATN_OUT))
            {
                DBG_PRINT((DBG_PREFIX "ATN still set, although we unset it!"));
                break;
            }
        }

        if (CBMIEC_GET(PP_RESET_IN))
        {
            CBMIEC_RELEASE(PP_RESET_OUT);

            if (CBMIEC_GET(PP_RESET_IN) != READ(PP_RESET_OUT))
            {
                DBG_PRINT((DBG_PREFIX "RESET still set, although we unset it!"));
                break;
            }
        }

        ntStatus = STATUS_SUCCESS;

    } while (0);

#undef READ

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Determine the type of cable (XA1541/XM1541) on the IEC bus

 This function tries to determine the type of cable with which
 the IEC bus is connected to the PC's parallel port. Afterwards,
 some variables in the device extension are initialized to reflect
 the type.

 \param Pdx
   Pointer to the device extension.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
static NTSTATUS
cbmiec_checkcable(PDEVICE_EXTENSION Pdx) 
{
    NTSTATUS ntStatus;
    const wchar_t *msgAuto = L"";
    const wchar_t *msgCable;
    UCHAR in, out;

    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "*****************" ));
    DBG_PRINT((DBG_PREFIX "cbmiec_checkcable" ));
    DBG_PRINT((DBG_PREFIX "*****************" ));

/*! \todo Do a more sophisticated test for the cable */

    switch (Pdx->IecCableUserSet)
    {
    case IEC_CABLETYPE_XM:
        /* FALL THROUGH */

    case IEC_CABLETYPE_XA:

        /* the user specified a cable type, use this 
         * without questioning
         */

        Pdx->IecCable = Pdx->IecCableUserSet;
        break;

    default:
        in = CBMIEC_GET(PP_ATN_IN);
        out = (READ_PORT_UCHAR(OUT_PORT) & PP_ATN_OUT) ? 1 : 0;
        Pdx->IecCable = (in != out) ? IEC_CABLETYPE_XA : IEC_CABLETYPE_XM;
        msgAuto = L" (auto)";
        break;
    }

    switch (Pdx->IecCable)
    {
    case IEC_CABLETYPE_XM:
        Pdx->IecOutEor = 0xc4;
        msgCable = L"passive (XM1541)";
        break;

    case IEC_CABLETYPE_XA:
        Pdx->IecOutEor = 0xcb;
        msgCable = L"active (XA1541)";
        break;
    }

    Pdx->IecInEor = 0x80;

    /* remember the current state of the output bits */

    Pdx->IecOutBits = (READ_PORT_UCHAR(OUT_PORT) ^ Pdx->IecOutEor) 
                      & (PP_DATA_OUT|PP_CLK_OUT|PP_ATN_OUT|PP_RESET_OUT);

    /* Now, test if the cable really works */

DBG_PRINT((DBG_PREFIX "using %ws cable%ws", msgCable, msgAuto)); // @@@@TODO

    ntStatus = cbmiec_testcable(Pdx);

    if (NT_SUCCESS(ntStatus))
    {
        DBG_SUCCESS((DBG_PREFIX "using %ws cable%ws",
            msgCable, msgAuto));

        LogErrorString(Pdx->Fdo, CBM_IEC_INIT, msgCable, msgAuto);
    }
    else
    {
        DBG_ERROR((DBG_PREFIX "could not validate that the cable "
                   "used is really a %ws cable%ws",
                   msgCable, msgAuto));

        LogErrorString(Pdx->Fdo, CBM_IEC_INIT_FAIL, msgCable, msgAuto);
    }

/*
    if (Pdx->IecOutBits & PP_RESET_OUT)
    {
       cbmiec_reset(Pdx);
    }
*/

    DBG_PRINT((DBG_PREFIX "*********************" ));
    DBG_PRINT((DBG_PREFIX "end cbmiec_checkcable" ));
    DBG_PRINT((DBG_PREFIX "*********************" ));

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}


/*! \brief Cleanup the IEC bus

 This function cleans the IEC bus immediately before it is released.

 \param Pdx
   Pointer to the device extension.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_cleanup(IN PDEVICE_EXTENSION Pdx)
{
    FUNC_ENTER();

    if (!Pdx->DoNotReleaseBus)
    {
        cbmiec_release_bus(Pdx);
    }

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/*! \brief Set the type of the IEC cable

 This function sets the type of the IEC cable.

 \param Pdx
   Pointer to the device extension.

 \param CableType
   The type of the cable.
*/
NTSTATUS
cbmiec_set_cabletype(IN PDEVICE_EXTENSION Pdx, IN IEC_CABLETYPE CableType)
{
    FUNC_ENTER();

    Pdx->IecCableUserSet = CableType;
    cbmiec_checkcable(Pdx);

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/* */
static VOID
cbm_check_irq_availability(IN PDEVICE_EXTENSION Pdx)
{
    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "*"));

    //
    // If we got an interrupt, test if it works
    //

    if (Pdx->ParallelPortAllocatedInterrupt)
    {
        DBG_PRINT((DBG_PREFIX "* Allocated interrupt"));

        if (!NT_SUCCESS(cbmiec_test_irq(Pdx, NULL, 0)))
        {
            //
            // Interrupt does not work, free it again.
            //

            LogErrorOnly(Pdx->Fdo, CBM_IRQ_DOES_NOT_WORK);

//            DBG_PRINT((DBG_PREFIX "* Interrupt does not work, release it again"));

//            ParPortFreeInterrupt(Pdx);
        }

    }

    FUNC_LEAVE();
}

/*! \brief Initialize the IEC bus

 This function initializes the IEC bus itself, and sets some
 variables in the device extension. It has to be called before
 any other IEC function is called.

 \param Pdx
   Pointer to the device extension.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_init(IN PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    // Initialize the event which is used to wake up the
    // task in wait_for_listener()

    DBG_IRQL( == PASSIVE_LEVEL);
    KeInitializeEvent(&Pdx->EventWaitForListener, SynchronizationEvent, FALSE);

#ifdef USE_DPC

    // Initialize the DPC object which will be used for waking
    // up cbmiec_wait_for_listener() later

    DBG_IRQL( == PASSIVE_LEVEL)
    IoInitializeDpcRequest(Pdx->Fdo, cbmiec_dpc);

#endif // #ifdef USE_DPC

    ntStatus = cbmiec_checkcable(Pdx);

    if (!NT_SUCCESS(ntStatus)) {
        FUNC_LEAVE_NTSTATUS(ntStatus);
    }

    Pdx->IecBusy = FALSE;

    if (!Pdx->DoNotReleaseBus)
    {
        CBMIEC_RELEASE(PP_RESET_OUT | PP_DATA_OUT | PP_ATN_OUT | PP_LP_BIDIR | PP_LP_IRQ);
        CBMIEC_SET(PP_CLK_OUT);
        cbm_check_irq_availability(Pdx);
    }

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/*! \brief Initialization for libiec which are global in nature

 This function initializes libiec.

 \param HKey
   Pointer to a handle with holds a registry key.
   If this is NULL, no access to the registry is performed.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_global_init(IN PHANDLE HKey)
{
    FUNC_ENTER();

    //Initialize the timeout values

    cbmiec_timeouts_init(HKey);

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}
