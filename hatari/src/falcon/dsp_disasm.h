/*
	DSP M56001 emulation
	Disassembler

	(C) 2003-2008 ARAnyM developer team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software Foundation,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA
*/

#ifndef DSP_DISASM_H
#define DSP_DISASM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DSP_TRACE_MODE,
	DSP_DISASM_MODE
} dsp_trace_disasm_t;

/* Functions */
extern void dsp56k_disasm_init(void);
extern uint16_t dsp56k_disasm(dsp_trace_disasm_t value, FILE *fp);
extern const char* dsp56k_getInstructionText(void);

/* Registers change */
extern void dsp56k_disasm_reg_save(void);
extern void dsp56k_disasm_reg_compare(FILE *fp);

#ifdef __cplusplus
}
#endif

#endif /* DSP_DISASM_H */
