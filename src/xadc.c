#include "xsysmon.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "sleep.h"

XSysMon XAdcInst;

int main(void) {
    XSysMon_Config *ConfigPtr;
    int Status;
    u16 TempRaw;
    float TempC;

    ConfigPtr = XSysMon_LookupConfig(0x43C00000);
    if (!ConfigPtr) {
        xil_printf("Lookup failed\r\n");
        return XST_FAILURE;
    }

    Status = XSysMon_CfgInitialize(&XAdcInst, ConfigPtr, ConfigPtr->BaseAddress);
    if (Status != XST_SUCCESS) {
        xil_printf("Init failed\r\n");
        return XST_FAILURE;
    }

    // Set sequencer mode and enable temperature
    XSysMon_SetSequencerMode(&XAdcInst, XSM_SEQ_MODE_SINGCHAN);
    XSysMon_SetSeqChEnables(&XAdcInst, XSM_SEQ_CH_TEMP);

    while (1) {

        TempRaw = XSysMon_GetAdcData(&XAdcInst, XSM_CH_TEMP);
        TempC = XSysMon_RawToTemperature(TempRaw);

        int temp_times_100 = (int)(TempC * 100);
		xil_printf("Temperature: %d.%02d °C\r\n",
           temp_times_100 / 100,
           temp_times_100 % 100);

        sleep(1);
    }

    return XST_SUCCESS;
}

