/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
/* Xilinx includes. */
#include "xil_printf.h"
#include "xparameters.h"
#include "xsysmon.h"
#include "ps7_init.h"

#define TIMER_ID	1
#define DELAY_10_SECONDS	10000UL
#define DELAY_1_SECOND		1000UL
#define TIMER_CHECK_THRESHOLD	9
/*-----------------------------------------------------------*/

/* The tasks as described at the top of this file. */
static void XadcTask( void *pvParameters );
static void ReaderTask( void *pvParameters );
static void vTimerCallback( TimerHandle_t pxTimer );
/*-----------------------------------------------------------*/

/* The queue used by the tasks, as described at the top of this
file. */
static TaskHandle_t H_XadcTask;
static TaskHandle_t H_ReaderTask;
static QueueHandle_t xQueue = NULL;
static TimerHandle_t xTimer = NULL;

XSysMon XAdcInst;
long RxtaskCntr = 0;


int main( void )
{
	ps7_init();
	ps7_post_config();
	const TickType_t x10seconds = pdMS_TO_TICKS( DELAY_10_SECONDS );

	xil_printf( "Hello from Freertos example main\r\n" );

	/* Create the two tasks.  The Tx task is given a lower priority than the
	Rx task, so the Rx task will leave the Blocked state and pre-empt the Tx
	task as soon as the Tx task places an item in the queue. */
	xTaskCreate( 	XadcTask, 					/* The function that implements the task. */
					( const char * ) "Tx", 		/* Text name for the task, provided to assist debugging only. */
					configMINIMAL_STACK_SIZE, 	/* The stack allocated to the task. */
					NULL, 						/* The task parameter is not used, so set to NULL. */
					tskIDLE_PRIORITY,			/* The task runs at the idle priority. */
					&H_XadcTask );

	xTaskCreate( ReaderTask,
				 ( const char * ) "Rx",
				 configMINIMAL_STACK_SIZE,
				 NULL,
				 tskIDLE_PRIORITY + 1,
				 &H_ReaderTask );

	/* Create the queue used by the tasks.  The Rx task has a higher priority
	than the Tx task, so will preempt the Tx task and remove values from the
	queue as soon as the Tx task writes to the queue - therefore the queue can
	never have more than one item in it. */
	xQueue = xQueueCreate( 	10,						/* Space in the queue. */
							sizeof( int ) );	/* Each space in the queue is large enough to hold a uint32_t. */

	/* Check the queue was created. */
	configASSERT( xQueue );

	/* Create a timer with a timer expiry of 10 seconds. The timer would expire
	 after 10 seconds and the timer call back would get called. In the timer call back
	 checks are done to ensure that the tasks have been running properly till then.
	 The tasks are deleted in the timer call back and a message is printed to convey that
	 the example has run successfully.
	 The timer expiry is set to 10 seconds and the timer set to not auto reload. */
	xTimer = xTimerCreate( (const char *) "Timer",
							x10seconds,
							pdFALSE,
							(void *) TIMER_ID,
							vTimerCallback);
	/* Check the timer was created. */
	configASSERT( xTimer );

	/* start the timer with a block time of 0 ticks. This means as soon
	   as the schedule starts the timer will start running and will expire after
	   10 seconds */
	xTimerStart( xTimer, 0 );

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	/*Not reached*/
	for( ;; );
}


/*-----------------------------------------------------------*/
static void XadcTask( void *pvParameters )
{
const TickType_t x1second = pdMS_TO_TICKS( DELAY_1_SECOND );

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

/*-----------------------------------------------------------*/
static void ReaderTask( void *pvParameters )
{
int Temp_100 = 0;

	for( ;; )
	{
		/* Block to wait for data arriving on the queue. */
		xQueueReceive( 	xQueue,				/* The queue being read. */
						&Temp_100,	/* Data is read into this address. */
						portMAX_DELAY );	/* Wait without a timeout for data. */

		/* Print the received data. */
		xil_printf("Received temperature: %d.%02d °C\r\n",
           Temp_100 / 100,
           Temp_100 % 100);
		RxtaskCntr++;
	}
}

/*-----------------------------------------------------------*/
static void vTimerCallback( TimerHandle_t pxTimer )
{
	long lTimerId;
	configASSERT( pxTimer );

	lTimerId = ( long ) pvTimerGetTimerID( pxTimer );

	if (lTimerId != TIMER_ID) {
		xil_printf("FreeRTOS Hello World Example FAILED");
	}

	/* If the RxtaskCntr is updated every time the Rx task is called. The
	 Rx task is called every time the Tx task sends a message. The Tx task
	 sends a message every 1 second.
	 The timer expires after 10 seconds. We expect the RxtaskCntr to at least
	 have a value of 9 (TIMER_CHECK_THRESHOLD) when the timer expires. */
	if (RxtaskCntr >= TIMER_CHECK_THRESHOLD) {
		xil_printf("Successfully ran FreeRTOS Hello World Example");
	} else {
		xil_printf("FreeRTOS Hello World Example FAILED");
	}

	vTaskDelete( H_XadcTask );
	vTaskDelete( H_ReaderTask );
}

