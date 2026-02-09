typedef void (*vApplicationMallocFailedHookPtr)( size_t );

vApplicationMallocFailedHookPtr getApplicationMallocFailedHookPtr();
void setApplicationMallocFailedHookPtr(vApplicationMallocFailedHookPtr ptr);

typedef void (*vApplicationStackOverflowHookPtr)( TaskHandle_t pxTask, char *pcTaskName );

vApplicationStackOverflowHookPtr getApplicationStackOverflowHookPtr();
void setApplicationStackOverflowHookPtr(vApplicationStackOverflowHookPtr ptr);
