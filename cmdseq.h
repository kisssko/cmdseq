/*
 * cmdseq.h
 *
 *  Created on: Sep 10, 2018
 *      Author: a.gurov
 */

#ifndef CMDSEQ_H_
#define CMDSEQ_H_

#include <stdint.h>
#include <string.h>

#include <platform/common_macros/common_macros.h>

/* --- Command sequencer's definitions --- */

typedef uint32_t U32;

typedef struct
{
    uint16_t    cmnd;           /* Command       */
    uint16_t    addr;           /* Address/extra */
    uint32_t    data;           /* Data          */
} seq_cmd_t;

BEGIN_ENUM( seq_flag_t )
    DECLARE_ENUM_VAL( SEQ_FLAG_OKAY_i,  0 )
    DECLARE_ENUM_VAL( SEQ_FLAG_EQ_i,    1 )
    DECLARE_ENUM_VAL( SEQ_FLAG_SKIP_i,  7 )
END_ENUM( seq_flag_t );


#define DCR_ADDR_FLAG               0x8000

#define YES                         ( !0)
#define NO                          (!!0)
#define NONE                        0x00000000
#define NOT_USED                    NONE

#define CMDSEQ_DEBUG_PRINT          NO

#define BIT1(N)                     (1 << (N))
#define BIT0(N)                     (0 << (N))
#define BYTENV(N,V)                 (((V) & 0xFF) << ((N) << 3))
#define SIZE32(N)                   ((N) / sizeof(uint32_t))
#define DCRADDR(ADDR)               (DCR_ADDR_FLAG | (ADDR))

#define SEQ_FLAG_OKAY               BIT1(SEQ_FLAG_OKAY_i)
#define SEQ_FLAG_EQ                 BIT1(SEQ_FLAG_EQ_i)
#define SEQ_FLAG_SKIP               BIT1(SEQ_FLAG_SKIP_i)

/* Command sequencer's opcodes */
#define SEQCMD_NOOP                 0x8000  /* No operation         */
#define SEQCMD_JUMP                 0x8100  /* Jump to sequence     */
#define SEQCMD_CALL                 0x8200  /* Call subsequence     */
#define SEQCMD_RETURN               0x8300  /* Return from subseq   */
#define SEQCMD_SETBUF               0x8400  /* Set buffer           */
#define SEQCMD_WRITED               0x8500  /* Write data           */
#define SEQCMD_WRITEB               0x8600  /* Write from buffer    */
#define SEQCMD_WRITEP               0x8700  /* Write from pointer   */
#define SEQCMD_COPY                 0x8800  /* Copy data (r/w)      */
#define SEQCMD_READB                0x8900  /* Read to buffer       */
#define SEQCMD_READP                0x8A00  /* Read to pointer      */
#define SEQCMD_READM                0x8B00  /* Read with mask       */
#define SEQCMD_SETBIT               0x8C00  /* Set bit              */
#define SEQCMD_CLRBIT               0x8D00  /* Clear bit            */
#define SEQCMD_INVBIT               0x8E00  /* Inverse bit          */
#define SEQCMD_CALLF                0x8F00  /* Call C/Asm function  */
#define SEQCMD_WAITM                0x9100  /* Wait milliseconds    */
#define SEQCMD_WAITU                0x9200  /* Wait microseconds    */
#define SEQCMD_WAITI                0x9300  /* Wait interrupt       */
#define SEQCMD_WAIT0                0x9400  /* Wait bit(s) set to 0 */
#define SEQCMD_WAIT1                0x9500  /* Wait bit(s) set to 1 */
#define SEQCMD_IFEQ                 0x9C00  /* Execute if equal     */
#define SEQCMD_IFNE                 0x9D00  /* Execute if not equal */
#define SEQCMD_IFEQAT               0x9E00  /* Execute if equal     */
#define SEQCMD_IFNEAT               0x9F00  /* Execute if not equal */
#define SEQCMD_PRINTV               0xED00  /* Print value at addr  */
#define SEQCMD_PRINTB               0xEE00  /* Print value in buf   */
#define SEQCMD_PRINT                0xEF00  /* Print notification   */
#define SEQCMD_ENDIF                0xFE00  /* End of condition     */
#define SEQCMD_END                  0xFFFF  /* End of sequence      */

/* Command sequencer's general low level macro-commands */
#define CMD_TERMINATE()             {SEQCMD_END,      (NONE),(U32) (NONE   )}
#define CMD_RETURN()                {SEQCMD_RETURN,   (NONE),(U32) (NONE   )},
#define CMD_JUMPI(BASE,I)           {SEQCMD_JUMP,     (NONE),(U32)&(BASE[I])},
#define CMD_JUMP(BASE)              {SEQCMD_JUMP,     (NONE),(U32)&(BASE[0])},
#define CMD_CALLI(BASE,I)           {SEQCMD_CALL,     (NONE),(U32)&(BASE[I])},
#define CMD_CALL(BASE)              {SEQCMD_CALL,     (NONE),(U32)&(BASE[0])},
#define CMD_CALLFUNC(ADDR)          {SEQCMD_CALLF,    (NONE),(U32) (ADDR   )},
#define CMD_SETBASE(ADDR,FLAG)      {SEQCMD_SETBASE,  (FLAG),(U32) (ADDR   )},
#define CMD_SETBUF(DATA)            {SEQCMD_SETBUF,   (NONE),(U32) (DATA   )},
#define CMD_WRITE_DATA(ADDR,DATA)   {SEQCMD_WRITED,   (ADDR),(U32) (DATA   )},
#define CMD_WRITE_FROM(ADDR,DATA)   {SEQCMD_WRITEP,   (ADDR),(U32)&(DATA   )},
#define CMD_IF_EQ(ADDR,DATA)        {SEQCMD_IFEQ  ,   (ADDR),(U32) (DATA   )},
#define CMD_IF_NE(ADDR,DATA)        {SEQCMD_IFNE  ,   (ADDR),(U32) (DATA   )},
#define CMD_IF_EQ_AT(ADDR,DATA)     {SEQCMD_IFEQAT,   (ADDR),(U32)&(DATA   )},
#define CMD_IF_NE_AT(ADDR,DATA)     {SEQCMD_IFNEAT,   (ADDR),(U32)&(DATA   )},
#define CMD_READ_TO(ADDR,PTR)       {SEQCMD_READP,    (ADDR),(U32)&(DATA   )},
#define CMD_WAIT_INT(TIME)          {SEQCMD_WAITI,    (NONE),(U32) (TIME   )},
#define CMD_WAIT_MSLEEP(TIME)       {SEQCMD_WAITM,    (NONE),(U32) (TIME   )},
#define CMD_WAIT_USLEEP(TIME)       {SEQCMD_WAITU,    (NONE),(U32) (TIME   )},
#define CMD_WAIT_BIT0(ADDR,B,TIME)  {SEQCMD_WAIT0|(B),(ADDR),(U32) (TIME   )},
#define CMD_WAIT_BIT1(ADDR,B,TIME)  {SEQCMD_WAIT1|(B),(ADDR),(U32) (TIME   )},
#define CMD_SETBIT(ADDR,DATA)       {SEQCMD_SETBIT,   (ADDR),(U32) (DATA   )},
#define CMD_CLRBIT(ADDR,DATA)       {SEQCMD_CLRBIT,   (ADDR),(U32) (DATA   )},
#define CMD_INVBIT(ADDR,DATA)       {SEQCMD_INVBIT,   (ADDR),(U32) (DATA   )},
#define CMD_PRINTV(ADDR,DATA)       {SEQCMD_PRINTV,   (ADDR),(U32) (DATA   )},
#define CMD_PRINTB(DATA)            {SEQCMD_PRINTB,   (NONE),(U32) (DATA   )},
#define CMD_PRINT(DATA)             {SEQCMD_PRINT,    (NONE),(U32) (DATA   )},

#define DEF_CMDSEQ(NAME) seq_cmd_t NAME[] = {
#define END_CMDSEQ  CMD_TERMINATE()};

uint32_t cmdseq_exec(seq_cmd_t *seq, uint32_t mem_base, uint32_t dcr_base);


#endif /* CMDSEQ_H_ */
