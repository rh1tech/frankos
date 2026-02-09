/*-----------------------------------------------------------------------*/
/* OS Dependent Functions for FatFs (FreeRTOS version)                    */
/*-----------------------------------------------------------------------*/

#include "ff.h"
#include "FreeRTOS.h"
#include "semphr.h"

#if FF_FS_REENTRANT   /* Mutual exclusion enabled */

/*-----------------------------------------------------------------------*/
/* Create a Synchronization Object                                       */
/*-----------------------------------------------------------------------*/

int ff_cre_syncobj (BYTE vol, FF_SYNC_t *sobj)
{
    (void)vol; // том можно использовать при желании, но обычно не нужно

    SemaphoreHandle_t m = xSemaphoreCreateMutex();
    if (m == NULL) {
        return 0;
    }

    *sobj = m;
    return 1;
}


/*-----------------------------------------------------------------------*/
/* Delete a Synchronization Object                                       */
/*-----------------------------------------------------------------------*/

int ff_del_syncobj (FF_SYNC_t sobj)
{
    if (sobj != NULL) {
        vSemaphoreDelete(sobj);
    }
    return 1;
}


/*-----------------------------------------------------------------------*/
/* Request Grant to Access the Volume                                    */
/*-----------------------------------------------------------------------*/

int ff_req_grant (FF_SYNC_t sobj)
{
    if (sobj == NULL)
        return 0;

    if (xSemaphoreTake(sobj, FF_FS_TIMEOUT) == pdTRUE)
        return 1;

    return 0; // timeout
}


/*-----------------------------------------------------------------------*/
/* Release Grant to Access the Volume                                    */
/*-----------------------------------------------------------------------*/

void ff_rel_grant (FF_SYNC_t sobj)
{
    if (sobj != NULL) {
        xSemaphoreGive(sobj);
    }
}

#endif /* FF_FS_REENTRANT */
