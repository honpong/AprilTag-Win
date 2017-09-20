#include "tm2_outcall.h"
#include "uendpoint.h"
#include "uplatformapi.h"
#include "usb_definitions.h"
#include "usb.h"
#include "usbpump_application_rtems_api.h"
#include "usbpump_proto_vsc2_api.h"
#include "usbpump_vsc2app.h"
#include "usbpumpapi.h"
#include "usbpumpdebug.h"
#include "usbpumplib.h"

#include <DrvRegUtilsDefines.h>
#include <mv_types.h>
#include <stdio.h>

#include "mv_trace.h"

extern USBPUMP_VSC2APP_CONTEXT * pSelf;

static bool last_read_state[USBPUMP_VSC2APP_NUM_EP_IN];
static bool last_write_state[USBPUMP_VSC2APP_NUM_EP_OUT];

int usb_init()
{
    int i, status;
    for(i = 0; i < USBPUMP_VSC2APP_NUM_EP_IN; i++) {
        // semaphores are created with initial value of 0, so we block
        // until someone (the completion handle) releases
        status = rtems_semaphore_create(
                rtems_build_name('I', 'N', '_', '0' + i), 0,
                RTEMS_SIMPLE_BINARY_SEMAPHORE, 0, &pSelf->semReadId[i]);
        if(status != RTEMS_SUCCESSFUL) {
            return 1;
        }
    }
    for(i = 0; i < USBPUMP_VSC2APP_NUM_EP_OUT; i++) {
        status = rtems_semaphore_create(
                rtems_build_name('O', 'U', 'T', '0' + i), 0,
                RTEMS_SIMPLE_BINARY_SEMAPHORE, 0, &pSelf->semWriteId[i]);
        if(status != RTEMS_SUCCESSFUL) {
            return 1;
        }
    }
    return 0;
}

static USBPUMP_PROTO_VSC2_EVENT_FN UsbPumpVscAppI_Event;

static USBPUMP_PROTO_VSC2_SETUP_VALIDATE_FN UsbPumpVscAppI_SetupValidate;

static USBPUMP_PROTO_VSC2_SETUP_PROCESS_FN UsbPumpVscAppI_SetupProcess;

static VOID UsbPumpVscAppI_StartRead(USBPUMP_VSC2APP_CONTEXT * pSelf,
                                     __TMS_UINT32              endPoint);

static VOID UsbPumpVscAppI_StartWrite(USBPUMP_VSC2APP_CONTEXT * pSelf,
                                      __TMS_UINT32              endPoint);

static UBUFIODONEFN UsbPumpVscAppI_ReadTransferDone;

static UBUFIODONEFN UsbPumpVscAppI_WriteTransferDone;

/****************************************************************************\
|
|	Read-only data.
|
|	If program is to be ROM-able, these must all be tagged read-only
|	using the ROM storage class; they may be global.
|
\****************************************************************************/

CONST USBPUMP_PROTO_VSC2_OUTCALL gk_UsbPumpProtoVsc2_OutCall =
        USBPUMP_PROTO_VSC2_OUTCALL_INIT_V1(UsbPumpVscAppI_Event,
                                           UsbPumpVscAppI_SetupValidate,
                                           UsbPumpVscAppI_SetupProcess);

/****************************************************************************\
|
|	VARIABLES:
|
|	If program is to be ROM-able, these must be initialized
|	using the BSS keyword.  (This allows for compilers that require
|	every variable to have an initializer.)  Note that only those
|	variables owned by this module should be declared here, using the BSS
|	keyword; this allows for linkers that dislike multiple declarations
|	of objects.
|
\****************************************************************************/

/*

Name:	UsbPumpVscAppI_Event

Function:
	Deliver vsc protocol event to the registered client

Definition:
	VOID
	UsbPumpVscAppI_Event(
		VOID *				ClientHandle,
		USBPUMP_PROTO_VSC2_EVENT	Event,
		CONST VOID *			pEventInfo
		);

Description:
	This is vsc protocol driver out-call function.  It will be called
	by vsc protocol driver to deliver vsc protocol event to the
	registered client.

Returns:
	No explicit result.

*/

static VOID UsbPumpVscAppI_Event(VOID *                   ClientHandle,
                                 USBPUMP_PROTO_VSC2_EVENT Event,
                                 CONST VOID * pEventInfo)
{
    USBPUMP_VSC2APP_CONTEXT * CONST pSelf = ClientHandle;

    USBPUMP_UNREFERENCED_PARAMETER(pEventInfo);

    switch(Event) {
        case USBPUMP_PROTO_VSC2_EVENT_INTERFACE_UP: {
            TTUSB_PLATFORM_PRINTF((pSelf->pPlatform, UDMASK_ANY,
                                   " UsbPumpVscAppI_Event: interface up.\n"));
            printf("\nIfc up\n");
            pSelf->fInterfaceUp = TRUE;
        } break;

        case USBPUMP_PROTO_VSC2_EVENT_INTERFACE_DOWN: {
            TTUSB_PLATFORM_PRINTF((pSelf->pPlatform, UDMASK_ANY,
                                   " UsbPumpVscAppI_Event: interface down.\n"));

            printf("\nIfc down\n");
            pSelf->fInterfaceUp = FALSE;
        } break;

        case USBPUMP_PROTO_VSC2_EVENT_RESUME:
            printf("\nIfc resume\n");
            break;

        case USBPUMP_PROTO_VSC2_EVENT_SUSPEND:
            /* Need to notify client */
            printf("\nIfc suspend\n");
            break;

        default:
            break;
    }
}

/*

Name:	UsbPumpVscAppI_SetupValidate

Function:
	Validate vsc control request to the registered client.

Definition:
	USBPUMP_PROTO_VSC2_SETUP_STATUS
	UsbPumpVscAppI_SetupValidate(
		VOID *		ClientHandle,
		CONST USETUP *	pSetup
		);

Description:
	This is vsc protocol driver out-call function.  It will be called by
	vsc protocol driver when receive vendor specific request from host.
	Client provided USBPUMP_PROTO_VSC2_SETUP_VALIDATE_FN() should validate
	vendor specific request.  If client can accept this control request,
	client should return USBPUMP_PROTO_VSC2_SETUP_STATUS_ACCEPTED.  If this
	control request is unknown (can't accept this control request),
	client should return USBPUMP_PROTO_VSC2_SETUP_STATUS_NOT_CLAIMED.
	If client knows this request but wants to reject this request, client
	should return USBPUMP_PROTO_VSC2_SETUP_STATUS_REJECTED.

	If client accepted this vendor specific control request, client will
	get USBPUMP_PROTO_VSC2_SETUP_PROCESS_FN() callback to process accepted
	control request.

	If client returns USBPUMP_PROTO_VSC2_SETUP_STATUS_REJECTED, vsc protocol
	will send STALL.

	If client returns USBPUMP_PROTO_VSC2_SETUP_STATUS_NOT_CLAIMED, protocol
	do nothing for this vendor specific command.

Returns:
	USBPUMP_PROTO_VSC2_SETUP_STATUS

*/

static USBPUMP_PROTO_VSC2_SETUP_STATUS UsbPumpVscAppI_SetupValidate(
        VOID * ClientHandle, CONST USETUP * pSetup)
{
    USBPUMP_VSC2APP_CONTEXT * CONST pSelf = ClientHandle;
    USBPUMP_PROTO_VSC2_SETUP_STATUS Status;

    pSelf->fAcceptSetup = FALSE;
    Status              = USBPUMP_PROTO_VSC2_SETUP_STATUS_NOT_CLAIMED;

    if(pSetup->uc_bmRequestType == USB_bmRequestType_HVDEV) {
        pSelf->fAcceptSetup = TRUE;
        Status              = USBPUMP_PROTO_VSC2_SETUP_STATUS_ACCEPTED;
        switch(pSetup->uc_bRequest) {
            case CONTROL_MESSAGE_START:
            case CONTROL_MESSAGE_STOP_AND_RESET:
            case CONTROL_MESSAGE_USB_RESET:
                break;
            default:
                pSelf->fAcceptSetup = FALSE;
                Status              = USBPUMP_PROTO_VSC2_SETUP_STATUS_REJECTED;
                break;
        }
    }
    else if(pSetup->uc_bmRequestType == USB_bmRequestType_DVDEV) {
        if(pSetup->uc_bRequest == 0) {
            pSelf->fAcceptSetup = TRUE;
            Status              = USBPUMP_PROTO_VSC2_SETUP_STATUS_ACCEPTED;
        }
        else {
            Status = USBPUMP_PROTO_VSC2_SETUP_STATUS_REJECTED;
        }
    }

    return Status;
}

/*

Name:	UsbPumpVscAppI_SetupProcess

Function:
	Process vsc control request.

Definition:
	BOOL
	UsbPumpVscAppI_SetupProcess(
		VOID *		ClientHandle,
		CONST USETUP *	pSetup,
		VOID *		pBuffer,
		UINT16		nBuffer
		);

Description:
	This is vsc protocol driver out-call function.  It will be called by
	vsc protocol driver to process vendor specific control request.

	If direction of vendor request is from host to device, vsc protocol
	receives data from host and protocol passes received data thru pBuffer
	and nBuffer.  Client processes control data in the pBuffer and client
	should send reply using USBPUMP_PROTO_VSC2_CONTROL_REPLY_FN().

	If direction of vendor request is from device to host, vsc protocol
	provide data buffer (pBuffer and nBuffer).  Client processes control
	request and copies data to the pBuffer. Client should send reply using
	USBPUMP_PROTO_VSC2_CONTROL_REPLY_FN().

	If this process function returns FALSE, the VSC protocol driver will
	send STALL for this setup packet.  If it returns TRUE, client should
	handle this setup packet.

Returns:
	TRUE if client process this setup packet.  FALSE if client doesn't
	want to process this setup packet and want to send STALL.

*/

static BOOL UsbPumpVscAppI_SetupProcess(VOID * ClientHandle,
                                        CONST USETUP * pSetup, VOID * pBuffer,
                                        UINT16 nBuffer)
{
    USBPUMP_VSC2APP_CONTEXT * CONST pSelf = ClientHandle;

    if(!pSelf->fAcceptSetup) return FALSE;

    pSelf->fAcceptSetup = FALSE;

    if(pSetup->uc_bmRequestType == USB_bmRequestType_HVDEV) {
        pSelf->fAcceptSetup = TRUE;
        switch(pSetup->uc_bRequest) {
            case CONTROL_MESSAGE_START:
                printf("CONTROL_MESSAGE_START\n");
                startReplay();
                break;
            case CONTROL_MESSAGE_STOP_AND_RESET:
                printf("CONTROL_MESSAGE_STOP_AND_RESET\n");
                UsbPump_Rtems_DataPump_RestartDevice(50);
                stopReplay();
                break;
            case CONTROL_MESSAGE_USB_RESET:
                printf("CONTROL_MESSAGE_USB_RESET\n");
                SET_REG_WORD(CPR_MAS_RESET_ADR, 0x00);
                break;
            default:
                pSelf->fAcceptSetup = FALSE;
                break;
        }
        /* pSelf->pControlBuffer has received data from host. */
        TTUSB_PLATFORM_PRINTF((pSelf->pPlatform, UDMASK_ANY,
                               " UsbPumpVscApp_Setup: Received %d bytes:"
                               " %02x %02x %02x %02x ...\n",
                               nBuffer, ((UINT8 *)pBuffer)[0],
                               ((UINT8 *)pBuffer)[1], ((UINT8 *)pBuffer)[2],
                               ((UINT8 *)pBuffer)[3]));

        /* Send control status data */
        (*pSelf->InCall.Vsc.pControlReplyFn)(pSelf->hSession, pBuffer, 0);
    }
    else if(pSetup->uc_bmRequestType == USB_bmRequestType_DVDEV) {
        UINT16 Size;

        /* Send control data -- just copy SETUP packet */
        Size                    = sizeof(*pSetup);
        if(Size > nBuffer) Size = nBuffer;

        UHIL_cpybuf(pBuffer, pSetup, Size);

        (*pSelf->InCall.Vsc.pControlReplyFn)(pSelf->hSession, pBuffer, Size);
    }
    else {
        /* Send reply... here just STALL */
        (*pSelf->InCall.Vsc.pControlReplyFn)(pSelf->hSession, NULL, 0);
    }

    return TRUE;
}

/*

Name:	UsbPumpVscAppI_StartRead

Function:
	Start loopback

Definition:
	VOID
	UsbPumpVscAppI_StartRead(
		USBPUMP_VSC2APP_CONTEXT *	pSelf
		);

Description:
	This function starts data loopback operations.

Returns:
	No explicit result.

*/

static VOID UsbPumpVscAppI_StartRead(USBPUMP_VSC2APP_CONTEXT * pSelf,
                                     __TMS_UINT32              endPoint)
{
    UBUFQE *                         pQeOut;
    USBPUMP_VSC2APP_REQUEST *        pRequest;
    USBPUMP_PROTO_VSC2_STREAM_HANDLE hStreamOut;
    UNUSED(pRequest);

    hStreamOut = pSelf->hStreamOut[endPoint];
    if((pQeOut = UsbGetQe(&pSelf->pFreeQeHeadOut)) == NULL) {
        printf("No free UBUFQE\n");
    }
    else {
        USBPUMP_VSC2APP_REQUEST * CONST pRequest = __TMS_CONTAINER_OF(
                pQeOut, USBPUMP_VSC2APP_REQUEST, Vsc.Qe.UbufqeLegacy);
        UBUFQE_FLAGS Flags;
        //Flags = UBUFQEFLAG_SHORTCOMPLETE;
        Flags = UBUFQEFLAG_DEFINITE_LENGTH;

        UsbPumpProtoVsc2Request_PrepareLegacy(
                &pRequest->Vsc, hStreamOut, pSelf->rBuff[endPoint],
                NULL,                          /* hBuffer */
                pSelf->rSize[endPoint], Flags, /* TransferFlags */
                UsbPumpVscAppI_ReadTransferDone, pSelf);

        /* submit request to send loopback data */
        //printf("submit read request on EP %d\n", endPoint+1);
        (*pSelf->InCall.Vsc.pSubmitRequestFn)(pSelf->hSession, &pRequest->Vsc);
    }
}

static VOID UsbPumpVscAppI_StartWrite(USBPUMP_VSC2APP_CONTEXT * pSelf,
                                      __TMS_UINT32              endPoint)
{
    UBUFQE *                               pQeIn;
    USBPUMP_VSC2APP_REQUEST *              pRequestIn;
    __TMS_USBPUMP_PROTO_VSC2_STREAM_HANDLE hStreamIn;
    UNUSED(pRequestIn);

    hStreamIn = pSelf->hStreamIn[endPoint];

    if((pQeIn = UsbGetQe(&pSelf->pFreeQeHeadIn)) == NULL) {
        printf("No free UBUFQE\n");
    }
    else {
        USBPUMP_VSC2APP_REQUEST * CONST pRequestIn = __TMS_CONTAINER_OF(
                pQeIn, USBPUMP_VSC2APP_REQUEST, Vsc.Qe.UbufqeLegacy);
        UBUFQE_FLAGS Flags;
        Flags = pRequestIn->Vsc.Qe.UbufqeLegacy.uqe_flags &
                UBUFQEFLAG_POSTBREAK;

        UsbPumpProtoVsc2Request_PrepareLegacy(
                &pRequestIn->Vsc, hStreamIn, pSelf->wBuff[endPoint],
                NULL,                          /* hBuffer */
                pSelf->wSize[endPoint], Flags, /* TransferFlags */
                UsbPumpVscAppI_WriteTransferDone, pSelf);

        /* submit request to send loopback data */
        //printf("submit write request on EP %d\n", endPoint+1);
        (*pSelf->InCall.Vsc.pSubmitRequestFn)(pSelf->hSession,
                                              &pRequestIn->Vsc);
    }
}

static VOID UsbPumpVscAppI_ReadTransferDone(UDEVICE * pDevice, UENDPOINT * pUep,
                                            UBUFQE * pQe)
{
    USBPUMP_VSC2APP_REQUEST * CONST pRequest = __TMS_CONTAINER_OF(
            pQe, USBPUMP_VSC2APP_REQUEST, Vsc.Qe.UbufqeLegacy);
    USBPUMP_VSC2APP_CONTEXT * CONST pSelf = pQe->uqe_doneinfo;

    __TMS_UINT32 endPoint =
            pUep->uep_hh.uephh_pPipe->upipe_bEndpointAddress;  //endpoint number

    USBPUMP_UNREFERENCED_PARAMETER(pDevice);
    USBPUMP_UNREFERENCED_PARAMETER(pUep);

    if(pQe->uqe_status != USTAT_OK) {
        last_read_state[endPoint - 1] = false;
        TTUSB_PLATFORM_PRINTF((pSelf->pPlatform, UDMASK_ERRORS,
                               "?UsbPumpVscAppI_ReadTransferDone:", pRequest,
                               UsbPumpStatus_Name(pQe->uqe_status),
                               pQe->uqe_status));
    }
    else
        last_read_state[endPoint - 1] = true;

    //printf("actually read %lu bytes\n", pQe->uqe_bufars);
    //printf("Read : Before release EP: %d\n", endPoint);
    rtems_semaphore_release(pSelf->semReadId[endPoint - 1]);

    UsbPutQe(&pSelf->pFreeQeHeadOut, &pRequest->Vsc.Qe.UbufqeLegacy);
    //printf("Read done on EP %d\n", endPoint);
}

static VOID UsbPumpVscAppI_WriteTransferDone(UDEVICE *   pDevice,
                                             UENDPOINT * pUep, UBUFQE * pQe)
{
    USBPUMP_VSC2APP_REQUEST * CONST pRequest = __TMS_CONTAINER_OF(
            pQe, USBPUMP_VSC2APP_REQUEST, Vsc.Qe.UbufqeLegacy);
    USBPUMP_VSC2APP_CONTEXT * CONST pSelf = pQe->uqe_doneinfo;

    __TMS_UINT32 endPoint = pUep->uep_hh.uephh_pPipe->upipe_bEndpointAddress %
                            0x8;  //endpoint number

    USBPUMP_UNREFERENCED_PARAMETER(pDevice);
    USBPUMP_UNREFERENCED_PARAMETER(pUep);

    if(pQe->uqe_status != USTAT_OK) {
        last_write_state[endPoint - 1] = false;
        TTUSB_PLATFORM_PRINTF((pSelf->pPlatform, UDMASK_ERRORS,
                               "?UsbPumpVscAppI_ReadTransferDone:", pRequest,
                               UsbPumpStatus_Name(pQe->uqe_status),
                               pQe->uqe_status));
    }
    else
        last_write_state[endPoint - 1] = true;

    rtems_semaphore_release(pSelf->semWriteId[endPoint - 1]);

    UsbPutQe(&pSelf->pFreeQeHeadIn, &pRequest->Vsc.Qe.UbufqeLegacy);
    //printf("Write done on EP %d\n", endPoint);
}

// Read/Write API
bool usb_blocking_read(uint32_t endpoint, uint8_t * buffer, uint32_t size)
{
    //printf("start read to %p of %u bytes\n", buffer, size);
    pSelf->rBuff[endpoint] = (char *)buffer;
    pSelf->rSize[endpoint] = size;
    UsbPumpVscAppI_StartRead(pSelf, endpoint);
    //blocking read
    //printf("obtaining semaphore %d\n", endpoint);
    rtems_semaphore_obtain(pSelf->semReadId[endpoint], RTEMS_WAIT, 0);
    return last_read_state[endpoint];
    //printf("got it\n");
}

bool usb_blocking_write(uint32_t endpoint, uint8_t * buffer, uint32_t size)
{
    //printf("start write to %p of %u bytes\n", buffer, size);
    pSelf->wBuff[endpoint] = (char *)buffer;
    pSelf->wSize[endpoint] = size;
    UsbPumpVscAppI_StartWrite(pSelf, endpoint);
    //blocking write
    //printf("obtaining semaphore %d\n", endpoint);
    rtems_semaphore_obtain(pSelf->semWriteId[endpoint], RTEMS_WAIT, 0);
    //printf("got it\n");
    return last_write_state[endpoint];
}

/**** end of vscapp_outcall.c ****/
