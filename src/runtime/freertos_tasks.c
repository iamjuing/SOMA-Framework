// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include "FreeRTOS/tasks.c"

#ifndef SOMA_FREERTOS_CONFIG_INCLUDED
#error "FreeRTOS must use the FreeRTOSConfig.h supplied by SOMA."
#endif

static void prvSelectTaskFromList( List_t* pxList,
                                   eTaskState eState,
                                   UBaseType_t uxLastTCBNumber,
                                   BaseType_t xHasLastTCBNumber,
                                   TCB_t** ppxSelectedTCB,
                                   eTaskState* peSelectedTaskState )
{
    ListItem_t* pxListItem = listGET_HEAD_ENTRY( pxList );

    while( pxListItem != listGET_END_MARKER( pxList ) )
    {
        TCB_t* const pxTCB = listGET_LIST_ITEM_OWNER( pxListItem );

        if( ( ( xHasLastTCBNumber == pdFALSE ) ||
              ( pxTCB->uxTCBNumber > uxLastTCBNumber ) ) &&
            ( ( *ppxSelectedTCB == NULL ) ||
              ( pxTCB->uxTCBNumber < ( *ppxSelectedTCB )->uxTCBNumber ) ) )
        {
            *ppxSelectedTCB = pxTCB;
            *peSelectedTaskState = eState;
        }

        pxListItem = listGET_NEXT( pxListItem );
    }
}

static TCB_t* prvSelectNextActiveTask( UBaseType_t uxLastTCBNumber,
                                       BaseType_t xHasLastTCBNumber,
                                       eTaskState* peSelectedTaskState )
{
    TCB_t* pxSelectedTCB = NULL;
    UBaseType_t uxQueue = configMAX_PRIORITIES;

    do
    {
        --uxQueue;
        prvSelectTaskFromList( &( pxReadyTasksLists[ uxQueue ] ),
                               eReady,
                               uxLastTCBNumber,
                               xHasLastTCBNumber,
                               &pxSelectedTCB,
                               peSelectedTaskState );
    } while( uxQueue > ( UBaseType_t ) tskIDLE_PRIORITY );

    prvSelectTaskFromList( ( List_t* ) pxDelayedTaskList,
                           eBlocked,
                           uxLastTCBNumber,
                           xHasLastTCBNumber,
                           &pxSelectedTCB,
                           peSelectedTaskState );
    prvSelectTaskFromList( ( List_t* ) pxOverflowDelayedTaskList,
                           eBlocked,
                           uxLastTCBNumber,
                           xHasLastTCBNumber,
                           &pxSelectedTCB,
                           peSelectedTaskState );

    #if ( INCLUDE_vTaskSuspend == 1 )
    {
        prvSelectTaskFromList( &xSuspendedTaskList,
                               eSuspended,
                               uxLastTCBNumber,
                               xHasLastTCBNumber,
                               &pxSelectedTCB,
                               peSelectedTaskState );
    }
    #endif

    return pxSelectedTCB;
}

static UBaseType_t prvGetActiveTaskCount( void )
{
    UBaseType_t uxTaskCount = 0u;
    UBaseType_t uxQueue = configMAX_PRIORITIES;

    do
    {
        --uxQueue;
        uxTaskCount += listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ uxQueue ] ) );
    } while( uxQueue > ( UBaseType_t ) tskIDLE_PRIORITY );

    uxTaskCount += listCURRENT_LIST_LENGTH( ( List_t* ) pxDelayedTaskList );
    uxTaskCount += listCURRENT_LIST_LENGTH( ( List_t* ) pxOverflowDelayedTaskList );

    #if ( INCLUDE_vTaskSuspend == 1 )
    {
        uxTaskCount += listCURRENT_LIST_LENGTH( &xSuspendedTaskList );
    }
    #endif

    return uxTaskCount;
}

static configSTACK_DEPTH_TYPE prvGetTaskStackDepth( const TCB_t* pxTCB )
{
    ptrdiff_t stack_depth = pxTCB->pxStack - pxTCB->pxEndOfStack;

    if( stack_depth < 0 )
    {
        stack_depth = ~stack_depth;
        stack_depth += 1;
    }

    stack_depth += 1; // pxEndOfStack points to the last valid stack position

    return ( configSTACK_DEPTH_TYPE ) stack_depth;
}

static BaseType_t prvGetNextActiveTaskStatus( TaskStatus_t* pxTaskStatus,
                                              char pcTaskName[ configMAX_TASK_NAME_LEN ],
                                              configSTACK_DEPTH_TYPE* puxStackDepth,
                                              UBaseType_t* puxLastTaskNumber,
                                              BaseType_t* pxHasLastTaskNumber,
                                              BaseType_t xGetFreeStackSpace )
{
    TCB_t* pxSelectedTCB = NULL;
    eTaskState eSelectedTaskState = eInvalid;
    BaseType_t xTaskFound = pdFALSE;
    UBaseType_t uxNameIndex;

    configASSERT( pxTaskStatus != NULL );
    configASSERT( pcTaskName != NULL );
    configASSERT( puxLastTaskNumber != NULL );
    configASSERT( pxHasLastTaskNumber != NULL );

    vTaskSuspendAll();
    {
        pxSelectedTCB = prvSelectNextActiveTask( *puxLastTaskNumber,
                                                 *pxHasLastTaskNumber,
                                                 &eSelectedTaskState );

        if( pxSelectedTCB != NULL )
        {
            vTaskGetInfo( ( TaskHandle_t ) pxSelectedTCB,
                          pxTaskStatus,
                          xGetFreeStackSpace,
                          eSelectedTaskState );

            for( uxNameIndex = 0u;
                 uxNameIndex < ( UBaseType_t ) configMAX_TASK_NAME_LEN;
                 ++uxNameIndex )
            {
                pcTaskName[ uxNameIndex ] = pxSelectedTCB->pcTaskName[ uxNameIndex ];
            }

            pcTaskName[ configMAX_TASK_NAME_LEN - 1u ] = '\0';
            pxTaskStatus->pcTaskName = pcTaskName;

            if( puxStackDepth != NULL )
            {
                *puxStackDepth = prvGetTaskStackDepth( pxSelectedTCB );
            }

            *puxLastTaskNumber = pxSelectedTCB->uxTCBNumber;
            *pxHasLastTaskNumber = pdTRUE;
            xTaskFound = pdTRUE;
        }
    }
    ( void ) xTaskResumeAll();

    return xTaskFound;
}

/**
 * @brief Returns the number of active tasks without including deleted tasks
 *        waiting for idle-task cleanup.
 */
UBaseType_t uxTaskGetActiveTaskCount( void )
{
    UBaseType_t uxTaskCount;

    vTaskSuspendAll();
    {
        uxTaskCount = prvGetActiveTaskCount();
    }
    ( void ) xTaskResumeAll();

    return uxTaskCount;
}

/**
 * @brief Obtains one active task record while the scheduler is briefly locked.
 *
 * @p puxLastTaskNumber and @p pxHasLastTaskNumber form an input/output
 * cursor.  The separate valid flag allows task number zero to be collected.
 * The task name is copied into caller-owned storage before the scheduler is
 * resumed, so it remains valid if the task is deleted afterward.
 * The record deliberately omits stack high-water collection, so this has
 * bounded scheduler suspension independent of task stack size.
 */
BaseType_t xTaskGetNextSystemState( TaskStatus_t* pxTaskStatus,
                                    char pcTaskName[ configMAX_TASK_NAME_LEN ],
                                    UBaseType_t* puxLastTaskNumber,
                                    BaseType_t* pxHasLastTaskNumber )
{
    return prvGetNextActiveTaskStatus( pxTaskStatus,
                                       pcTaskName,
                                       NULL,
                                       puxLastTaskNumber,
                                       pxHasLastTaskNumber,
                                       pdFALSE );
}

/**
 * @brief Obtains one active task stack record while the scheduler is locked.
 *
 * The task name and stack depth are copied into caller-owned storage before
 * the scheduler is resumed. Stack high-water collection uses FreeRTOS's
 * existing exact fill-pattern scan and therefore extends the lock only for
 * the selected task.
 */
BaseType_t xTaskGetNextStackProfile( TaskStatus_t* pxTaskStatus,
                                     char pcTaskName[ configMAX_TASK_NAME_LEN ],
                                     configSTACK_DEPTH_TYPE* puxStackDepth,
                                     UBaseType_t* puxLastTaskNumber,
                                     BaseType_t* pxHasLastTaskNumber )
{
    configASSERT( puxStackDepth != NULL );

    return prvGetNextActiveTaskStatus( pxTaskStatus,
                                       pcTaskName,
                                       puxStackDepth,
                                       puxLastTaskNumber,
                                       pxHasLastTaskNumber,
                                       pdTRUE );
}

/**
 * @brief Reads the platform run-time counter without taking a system snapshot.
 */
configRUN_TIME_COUNTER_TYPE ulTaskGetTotalRunTime( void )
{
    configRUN_TIME_COUNTER_TYPE ulRunTime = 0u;

    #if ( configGENERATE_RUN_TIME_STATS == 1 )
    {
        #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
        portALT_GET_RUN_TIME_COUNTER_VALUE( ulRunTime );
        #else
        ulRunTime = ( configRUN_TIME_COUNTER_TYPE ) portGET_RUN_TIME_COUNTER_VALUE();
        #endif
    }
    #endif

    return ulRunTime;
}
