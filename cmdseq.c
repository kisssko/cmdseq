
/*
 * cmdseq.c
 *
 *  Created on: Sep 10, 2018
 *      Author: a.gurov
 */

#include <stdio.h>

#include <platform/test_event_codes.h>
#include <platform/devices.h>

#include <platform/arch/ppc/ppc_476fp_config.h>
#include <platform/arch/ppc/ppc_476fp_lib_c.h>

#include <rumboot/io.h>
#include <rumboot/timer.h>
#include <rumboot/printf.h>
#include <rumboot/platform.h>

#include <platform/oi10/platform/cmdseq.h>

#define CMDSEQ_STACK_DEPTH      0x10

/* Lib functions aliases */
#define _GET_UPTIME              rumboot_platform_get_uptime

#define _MEM_WR(BASE,ADDR,DATA) iowrite32 ((DATA),(uint32_t)((BASE) + (ADDR)))
#define _MEM_RD(BASE,ADDR)      ioread32  (       (uint32_t)((BASE) + (ADDR)))

#define _PUSH(VAL)              *--SP = (uint32_t)(VAL)
#define _POP()                  *SP++

/* (B)ase address, (O)ffset, (D)ata, (R)esult */
#define RD_SET(B,O,D,R)         (R = _MEM_RD((B), (O)) |  (D))
#define RD_CLR(B,O,D,R)         (R = _MEM_RD((B), (O)) & ~(D))
#define RD_INV(B,O,D,R)         (R = _MEM_RD((B), (O)) ^  (D))


uint32_t cmdseq_exec(seq_cmd_t *seq, uint32_t mem_base, uint32_t dcr_base)
{
    DEF_CMDSEQ(___terminate_seq)
    END_CMDSEQ
    /* DCR still not supported */
    volatile
    uint32_t   buf      = 0,
               tmp      = 0,
               flg      = 0;  /* Flags */
    uint32_t  *SP;  /* Stack pointer */
    seq_cmd_t *op;
    uint32_t   stack[CMDSEQ_STACK_DEPTH];
    SP =      &stack[CMDSEQ_STACK_DEPTH];
    *(--SP) = (uint32_t)___terminate_seq;   /* PUSH END */
    for(op = seq; op->cmnd != SEQCMD_END; op++)
    {
        if(flg & SEQ_FLAG_SKIP)
            if((op->cmnd & 0xFF00) != SEQCMD_ENDIF)
                continue;
#if CMDSEQ_DEBUG_PRINT
        rumboot_printf("EXEC AT: 0x%X, CMD=0x%X, ADDR=0x%X, DATA=0x%X\n",
                (uint32_t)op, (uint32_t)(op->cmnd),
                (uint32_t)(op->addr), op->data);
#endif
        switch(op->cmnd & 0xFF00)
        {
        case SEQCMD_NOOP:
            break;
        case SEQCMD_SETBUF:
            buf = (uint32_t)(op->data);
        case SEQCMD_CALL:
            _PUSH(op);
        case SEQCMD_JUMP:
            op = ((seq_cmd_t*)(op->data)) - 1;
            break;
        case SEQCMD_RETURN:
            op = (seq_cmd_t*)_POP();
            break;
        case SEQCMD_CALLF:
            buf = ((uint32_t(*)(uint32_t))op->data)(buf);
            break;
        case SEQCMD_WRITED:     /* Write data           */
            _MEM_WR(mem_base, (uint32_t)(op->addr), (buf = op->data));
            break;
        case SEQCMD_WRITEB:     /* Write from buffer    */
            _MEM_WR(mem_base, (uint32_t)(op->addr), buf);
            break;
        case SEQCMD_WRITEP:     /* Write from pointer   */
            _MEM_WR(mem_base, (uint32_t)(op->addr),
                    *(uint32_t*)(op->data));
            break;
        case SEQCMD_COPY:       /* Copy data (r/w)      */
            _MEM_WR(mem_base, (uint32_t)(op->addr),
                    (buf = _MEM_RD(mem_base, op->data)));
            break;
        case SEQCMD_READB:      /* Read to buffer       */
            buf = _MEM_RD(mem_base, (uint32_t)(op->addr));
            break;
        case SEQCMD_READP:      /* Read to buffer       */
            (*(uint32_t*)(op->data)) = _MEM_RD(mem_base, (uint32_t)(op->addr));
            break;
        case SEQCMD_READM:      /* Read with mask       */
            buf = _MEM_RD(mem_base, (uint32_t)(op->addr)) & op->data;
            break;
        case SEQCMD_SETBIT:     /* Set bit              */
            _MEM_WR(mem_base, (uint32_t)(op->addr),
                    RD_SET(mem_base, buf, op->addr, op->data));
        case SEQCMD_CLRBIT:     /* Clear bit            */
            _MEM_WR(mem_base, (uint32_t)(op->addr),
                    RD_CLR(mem_base, buf, op->addr, op->data));
        case SEQCMD_INVBIT:     /* Inverse bit          */
            _MEM_WR(mem_base, (uint32_t)(op->addr),
                    RD_INV(mem_base, buf, op->addr, op->data));
            break;
        case SEQCMD_WAITM:      /* Wait milliseconds    */
            mdelay(op->data);
            break;
        case SEQCMD_WAITU:      /* Wait microseconds    */
            udelay(op->data);
            break;
        case SEQCMD_WAITI:      /* Wait interrupt       */
            /* TODO: Implement interrupt wait here. */
            udelay(op->data);
            break;
        case SEQCMD_WAIT0:      /* Wait bit(s) set to 0 */
            tmp = _GET_UPTIME();
            while
            (
                ((_GET_UPTIME() - tmp) < (op->data))
                    &&
                !(~(_MEM_RD(mem_base, (uint32_t)(op->addr)))
                        & ~BIT1((op->cmnd) & 0xFF))
            );
            if(!((_GET_UPTIME() - tmp) < (op->data)))
                rumboot_printf("SEQCMD_WAIT0 TIMEOUT!\n");
            break;
        case SEQCMD_WAIT1:      /* Wait bit(s) set to 1 */
            tmp = _GET_UPTIME();
            while
            (
                ((_GET_UPTIME() - tmp) < (op->data))
                    &&
                !(_MEM_RD(mem_base, (uint32_t)(op->addr))
                        & BIT1((op->cmnd) & 0xFF))
            );
            if(!((_GET_UPTIME() - tmp) < (op->data)))
                rumboot_printf("SEQCMD_WAIT1 TIMEOUT!\n");
            break;
        case SEQCMD_IFEQ:
            flg = ((buf = _MEM_RD(mem_base, op->data)) == (op->data))
                    << SEQ_FLAG_OKAY_i;
            if( (flg & SEQ_FLAG_OKAY)) flg |= SEQ_FLAG_EQ;
            if(!(flg & SEQ_FLAG_OKAY)) flg |= SEQ_FLAG_SKIP;
            break;
        case SEQCMD_IFNE:
            flg = ((buf = _MEM_RD(mem_base, op->data)) != (op->data))
                    << SEQ_FLAG_OKAY_i;
            if(!(flg & SEQ_FLAG_OKAY)) flg |= SEQ_FLAG_EQ;
            if(!(flg & SEQ_FLAG_OKAY)) flg |= SEQ_FLAG_SKIP;
            break;
        case SEQCMD_IFEQAT:
            flg = ((buf = _MEM_RD(mem_base, op->data))
                    == (*(uint32_t*)(op->data))) << SEQ_FLAG_OKAY_i;
            if( (flg & SEQ_FLAG_OKAY)) flg |= SEQ_FLAG_EQ;
            if(!(flg & SEQ_FLAG_OKAY)) flg |= SEQ_FLAG_SKIP;
            break;
        case SEQCMD_IFNEAT:
            flg = ((buf = _MEM_RD(mem_base, op->data))
                    != (*(uint32_t*)(op->data))) << SEQ_FLAG_OKAY_i;
            if(!(flg & SEQ_FLAG_OKAY)) flg |= SEQ_FLAG_EQ;
            if(!(flg & SEQ_FLAG_OKAY)) flg |= SEQ_FLAG_SKIP;
            break;
        case SEQCMD_PRINTB:     /* Print value in buf   */
            rumboot_printf((char*)(op->data), buf);
            break;
        case SEQCMD_PRINT:      /* Print notification   */
            rumboot_printf("SDIO: %s\n", (char*)(op->data));
            break;
        default: break;
        }
    }
    return buf;
}

