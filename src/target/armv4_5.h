/***************************************************************************
 *   Copyright (C) 2005 by Dominic Rath                                    *
 *   Dominic.Rath@gmx.de                                                   *
 *                                                                         *
 *   Copyright (C) 2008 by Spencer Oliver                                  *
 *   spen@spen-soft.co.uk                                                  *
 *                                                                         *
 *   Copyright (C) 2009 by Øyvind Harboe                                   *
 *   oyvind.harboe@zylin.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef ARMV4_5_H
#define ARMV4_5_H

#include <target/target.h>
#include <helper/command.h>


/**
 * These numbers match the five low bits of the *PSR registers on
 * "classic ARM" processors, which build on the ARMv4 processor
 * modes and register set.
 */
enum arm_mode {
	ARM_MODE_USR = 16,
	ARM_MODE_FIQ = 17,
	ARM_MODE_IRQ = 18,
	ARM_MODE_SVC = 19,
	ARM_MODE_ABT = 23,
	ARM_MODE_MON = 26,
	ARM_MODE_UND = 27,
	ARM_MODE_SYS = 31,
	ARM_MODE_ANY = -1
};

const char *arm_mode_name(unsigned psr_mode);
bool is_arm_mode(unsigned psr_mode);

/** The PSR "T" and "J" bits define the mode of "classic ARM" cores. */
enum arm_state {
	ARM_STATE_ARM,
	ARM_STATE_THUMB,
	ARM_STATE_JAZELLE,
	ARM_STATE_THUMB_EE,
};

extern const char *arm_state_strings[];

/* OBSOLETE, DO NOT USE IN NEW CODE!  The "number" of an arm_mode is an
 * index into the armv4_5_core_reg_map array.  Its remaining users are
 * remnants which could as easily walk * the register cache directly as
 * use the expensive ARMV4_5_CORE_REG_MODE() macro.
 */
int arm_mode_to_number(enum arm_mode mode);
enum arm_mode armv4_5_number_to_mode(int number);

extern const int armv4_5_core_reg_map[8][17];

#define ARMV4_5_CORE_REG_MODE(cache, mode, num) \
		cache->reg_list[armv4_5_core_reg_map[arm_mode_to_number(mode)][num]]

/* offset into armv4_5 core register cache -- OBSOLETE, DO NOT USE! */
enum { ARMV4_5_CPSR = 31, };

#define ARM_COMMON_MAGIC 0x0A450A45

/**
 * Represents a generic ARM core, with standard application registers.
 *
 * There are sixteen application registers (including PC, SP, LR) and a PSR.
 * Cortex-M series cores do not support as many core states or shadowed
 * registers as traditional ARM cores, and only support Thumb2 instructions.
 */
struct arm
{
	int common_magic;
	struct reg_cache *core_cache;

	/** Handle to the CPSR; valid in all core modes. */
	struct reg *cpsr;

	/** Handle to the SPSR; valid only in core modes with an SPSR. */
	struct reg *spsr;

	/** Support for arm_reg_current() */
	const int *map;

	/**
	 * Indicates what registers are in the ARM state core register set.
	 * ARM_MODE_ANY indicates the standard set of 37 registers,
	 * seen on for example ARM7TDMI cores.  ARM_MODE_MON indicates three
	 * more registers are shadowed, for "Secure Monitor" mode.
	 */
	enum arm_mode core_type;

	/** Record the current core mode: SVC, USR, or some other mode. */
	enum arm_mode core_mode;

	/** Record the current core state: ARM, Thumb, or otherwise. */
	enum arm_state core_state;

	/** Flag reporting unavailability of the BKPT instruction. */
	bool is_armv4;

	/** Flag reporting whether semihosting is active. */
	bool is_semihosting;

	/** Value to be returned by semihosting SYS_ERRNO request. */
	int semihosting_errno;

	/** Backpointer to the target. */
	struct target *target;

	/** Handle for the debug module, if one is present. */
	struct arm_dpm *dpm;

	/** Handle for the Embedded Trace Module, if one is present. */
	struct etm_context *etm;

	/* FIXME all these methods should take "struct arm *" not target */

	/** Retrieve all core registers, for display. */
	int (*full_context)(struct target *target);

	/** Retrieve a single core register. */
	int (*read_core_reg)(struct target *target, struct reg *reg,
			int num, enum arm_mode mode);
	int (*write_core_reg)(struct target *target, struct reg *reg,
			int num, enum arm_mode mode, uint32_t value);

	/** Read coprocessor register.  */
	int (*mrc)(struct target *target, int cpnum,
			uint32_t op1, uint32_t op2,
			uint32_t CRn, uint32_t CRm,
			uint32_t *value);

	/** Write coprocessor register.  */
	int (*mcr)(struct target *target, int cpnum,
			uint32_t op1, uint32_t op2,
			uint32_t CRn, uint32_t CRm,
			uint32_t value);

	void *arch_info;
};

/** Convert target handle to generic ARM target state handle. */
static inline struct arm *target_to_arm(struct target *target)
{
	return target->arch_info;
}

static inline bool is_arm(struct arm *arm)
{
	return arm && arm->common_magic == ARM_COMMON_MAGIC;
}

struct arm_algorithm
{
	int common_magic;

	enum arm_mode core_mode;
	enum arm_state core_state;
};

struct arm_reg
{
	int num;
	enum arm_mode mode;
	struct target *target;
	struct arm *armv4_5_common;
	uint32_t value;
};

struct reg_cache *arm_build_reg_cache(struct target *target, struct arm *arm);

int armv4_5_arch_state(struct target *target);
int armv4_5_get_gdb_reg_list(struct target *target,
		struct reg **reg_list[], int *reg_list_size);

extern const struct command_registration arm_command_handlers[];

int armv4_5_init_arch_info(struct target *target, struct arm *armv4_5);

int armv4_5_run_algorithm(struct target *target,
		int num_mem_params, struct mem_param *mem_params,
		int num_reg_params, struct reg_param *reg_params,
		uint32_t entry_point, uint32_t exit_point,
		int timeout_ms, void *arch_info);

int arm_checksum_memory(struct target *target,
		uint32_t address, uint32_t count, uint32_t *checksum);
int arm_blank_check_memory(struct target *target,
		uint32_t address, uint32_t count, uint32_t *blank);

void arm_set_cpsr(struct arm *arm, uint32_t cpsr);
struct reg *arm_reg_current(struct arm *arm, unsigned regnum);

extern struct reg arm_gdb_dummy_fp_reg;
extern struct reg arm_gdb_dummy_fps_reg;

/* ARM mode instructions
 */

/* Store multiple increment after
 * Rn: base register
 * List: for each bit in list: store register
 * S: in priviledged mode: store user-mode registers
 * W = 1: update the base register. W = 0: leave the base register untouched
 */
#define ARMV4_5_STMIA(Rn, List, S, W)	(0xe8800000 | ((S) << 22) | ((W) << 21) | ((Rn) << 16) | (List))

/* Load multiple increment after
 * Rn: base register
 * List: for each bit in list: store register
 * S: in priviledged mode: store user-mode registers
 * W = 1: update the base register. W = 0: leave the base register untouched
 */
#define ARMV4_5_LDMIA(Rn, List, S, W)	(0xe8900000 | ((S) << 22) | ((W) << 21) | ((Rn) << 16) | (List))

/* MOV r8, r8 */
#define ARMV4_5_NOP					(0xe1a08008)

/* Move PSR to general purpose register
 * R = 1: SPSR R = 0: CPSR
 * Rn: target register
 */
#define ARMV4_5_MRS(Rn, R)			(0xe10f0000 | ((R) << 22) | ((Rn) << 12))

/* Store register
 * Rd: register to store
 * Rn: base register
 */
#define ARMV4_5_STR(Rd, Rn)			(0xe5800000 | ((Rd) << 12) | ((Rn) << 16))

/* Load register
 * Rd: register to load
 * Rn: base register
 */
#define ARMV4_5_LDR(Rd, Rn)			(0xe5900000 | ((Rd) << 12) | ((Rn) << 16))

/* Move general purpose register to PSR
 * R = 1: SPSR R = 0: CPSR
 * Field: Field mask
 * 1: control field 2: extension field 4: status field 8: flags field
 * Rm: source register
 */
#define ARMV4_5_MSR_GP(Rm, Field, R)	(0xe120f000 | (Rm) | ((Field) << 16) | ((R) << 22))
#define ARMV4_5_MSR_IM(Im, Rotate, Field, R)	(0xe320f000 | (Im)  | ((Rotate) << 8) | ((Field) << 16) | ((R) << 22))

/* Load Register Halfword Immediate Post-Index
 * Rd: register to load
 * Rn: base register
 */
#define ARMV4_5_LDRH_IP(Rd, Rn)	(0xe0d000b2 | ((Rd) << 12) | ((Rn) << 16))

/* Load Register Byte Immediate Post-Index
 * Rd: register to load
 * Rn: base register
 */
#define ARMV4_5_LDRB_IP(Rd, Rn)	(0xe4d00001 | ((Rd) << 12) | ((Rn) << 16))

/* Store register Halfword Immediate Post-Index
 * Rd: register to store
 * Rn: base register
 */
#define ARMV4_5_STRH_IP(Rd, Rn)	(0xe0c000b2 | ((Rd) << 12) | ((Rn) << 16))

/* Store register Byte Immediate Post-Index
 * Rd: register to store
 * Rn: base register
 */
#define ARMV4_5_STRB_IP(Rd, Rn)	(0xe4c00001 | ((Rd) << 12) | ((Rn) << 16))

/* Branch (and Link)
 * Im: Branch target (left-shifted by 2 bits, added to PC)
 * L: 1: branch and link 0: branch only
 */
#define ARMV4_5_B(Im, L) (0xea000000 | (Im) | ((L) << 24))

/* Branch and exchange (ARM state)
 * Rm: register holding branch target address
 */
#define ARMV4_5_BX(Rm) (0xe12fff10 | (Rm))

/* Move to ARM register from coprocessor
 * CP: Coprocessor number
 * op1: Coprocessor opcode
 * Rd: destination register
 * CRn: first coprocessor operand
 * CRm: second coprocessor operand
 * op2: Second coprocessor opcode
 */
#define ARMV4_5_MRC(CP, op1, Rd, CRn, CRm, op2) (0xee100010 | (CRm) | ((op2) << 5) | ((CP) << 8) | ((Rd) << 12) | ((CRn) << 16) | ((op1) << 21))

/* Move to coprocessor from ARM register
 * CP: Coprocessor number
 * op1: Coprocessor opcode
 * Rd: destination register
 * CRn: first coprocessor operand
 * CRm: second coprocessor operand
 * op2: Second coprocessor opcode
 */
#define ARMV4_5_MCR(CP, op1, Rd, CRn, CRm, op2) (0xee000010 | (CRm) | ((op2) << 5) | ((CP) << 8) | ((Rd) << 12) | ((CRn) << 16) | ((op1) << 21))

/* Breakpoint instruction (ARMv5)
 * Im: 16-bit immediate
 */
#define ARMV5_BKPT(Im) (0xe1200070 | ((Im & 0xfff0) << 8) | (Im & 0xf))


/* Thumb mode instructions
 */

/* Store register (Thumb mode)
 * Rd: source register
 * Rn: base register
 */
#define ARMV4_5_T_STR(Rd, Rn)	((0x6000 | (Rd) | ((Rn) << 3)) | ((0x6000 | (Rd) | ((Rn) << 3)) << 16))

/* Load register (Thumb state)
 * Rd: destination register
 * Rn: base register
 */
#define ARMV4_5_T_LDR(Rd, Rn)	((0x6800 | ((Rn) << 3) | (Rd)) | ((0x6800 | ((Rn) << 3) | (Rd)) << 16))

/* Load multiple (Thumb state)
 * Rn: base register
 * List: for each bit in list: store register
 */
#define ARMV4_5_T_LDMIA(Rn, List) ((0xc800 | ((Rn) << 8) | (List)) | ((0xc800 | ((Rn) << 8) | List) << 16))

/* Load register with PC relative addressing
 * Rd: register to load
 */
#define ARMV4_5_T_LDR_PCREL(Rd)	((0x4800 | ((Rd) << 8)) | ((0x4800 | ((Rd) << 8)) << 16))

/* Move hi register (Thumb mode)
 * Rd: destination register
 * Rm: source register
 */
#define ARMV4_5_T_MOV(Rd, Rm)	((0x4600 | ((Rd) & 0x7) | (((Rd) & 0x8) << 4) | (((Rm) & 0x7) << 3) | (((Rm) & 0x8) << 3)) | ((0x4600 | ((Rd) & 0x7) | (((Rd) & 0x8) << 4) | (((Rm) & 0x7) << 3) | (((Rm) & 0x8) << 3)) << 16))

/* No operation (Thumb mode)
 */
#define ARMV4_5_T_NOP	(0x46c0 | (0x46c0 << 16))

/* Move immediate to register (Thumb state)
 * Rd: destination register
 * Im: 8-bit immediate value
 */
#define ARMV4_5_T_MOV_IM(Rd, Im)	((0x2000 | ((Rd) << 8) | (Im)) | ((0x2000 | ((Rd) << 8) | (Im)) << 16))

/* Branch and Exchange
 * Rm: register containing branch target
 */
#define ARMV4_5_T_BX(Rm)		((0x4700 | ((Rm) << 3)) | ((0x4700 | ((Rm) << 3)) << 16))

/* Branch (Thumb state)
 * Imm: Branch target
 */
#define ARMV4_5_T_B(Imm)	((0xe000 | (Imm)) | ((0xe000 | (Imm)) << 16))

/* Breakpoint instruction (ARMv5) (Thumb state)
 * Im: 8-bit immediate
 */
#define ARMV5_T_BKPT(Im) ((0xbe00 | Im) | ((0xbe00 | Im) << 16))

#endif /* ARMV4_5_H */
