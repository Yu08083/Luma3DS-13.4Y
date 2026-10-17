#pragma once

#include "utils.h"
#include "kernel.h"
#include "svc.h"


typedef enum ProcessOp
{
    PROCESSOP_GET_ALL_HANDLES,

    PROCESSOP_SET_MMU_TO_RWX,

    PROCESSOP_GET_ON_MEMORY_CHANGE_EVENT,
    PROCESSOP_SIGNAL_ON_EXIT,
    PROCESSOP_GET_PA_FROM_VA,

    PROCESSOP_SCHEDULE_THREADS,
} ProcessOp;

Result  ControlProcess(Handle process, ProcessOp op, u32 varg2, u32 varg3);
