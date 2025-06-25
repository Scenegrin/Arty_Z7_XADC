#include "xsysmon.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "sleep.h"

XSysMon XAdcInst;

static void XadcTask( void *pvParameters ) {
    XSysMon_Config *ConfigPtr;
    int Status;
    u16 TempRaw;
    float TempC;

    ConfigPtr = XSysMon_LookupConfig(0x43C00000);
    if (!ConfigPtr) {
        xil_printf("Lookup failed\r\n");
        return;
    }

    Status = XSysMon_CfgInitialize(&XAdcInst, ConfigPtr, ConfigPtr->BaseAddress);
    if (Status != XST_SUCCESS) {
        xil_printf("Init failed\r\n");
        return;
    }

    // Set sequencer mode and enable temperature
    XSysMon_SetSequencerMode(&XAdcInst, XSM_SEQ_MODE_SINGCHAN);
    XSysMon_SetSeqChEnables(&XAdcInst, XSM_SEQ_CH_TEMP);

    while (1) {

        TempRaw = XSysMon_GetAdcData(&XAdcInst, XSM_CH_TEMP);
        TempC = XSysMon_RawToTemperature(TempRaw);

        int temp_times_100 = (int)(TempC * 100);
	xQueueSend( xQueue,			/* The queue being written to. */
					&temp_times_100, /* The address of the data being sent. */
					0UL );			/* The block time. */

        vTaskDelay( x1second );
    }

    return;
}

