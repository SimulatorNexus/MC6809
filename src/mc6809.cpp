/*
 * mc6809.cpp  -  part of MC6809
 *
 * (c)2021 elmerucr
 */

#include "mc6809.hpp"
#include <cstdio>

/*
 * Constructor arranges references ar and br to right positions in
 * dr register. Currently assumes host system is LITTLE ENDIAN.
 */
mc6809::mc6809(bus_read r, bus_write w) : ac(*(((uint8_t *)&dr)+1)), br(*((uint8_t *)&dr))
{
	read_8 = (bus_read)r;
	write_8 = (bus_write)w;

	cc = 0b00000000;

	/*
	 * When NFI pins are not (yet) assigned, there needs to be a
	 * decent starting value (true).
	 */
	default_pin = true;
	nmi_line = &default_pin;
	firq_line = &default_pin;
	irq_line = &default_pin;
	old_nmi_line = true;

	cycles = 0;

	index_regs[0b00] = &xr;
	index_regs[0b01] = &yr;
	index_regs[0b10] = &us;
	index_regs[0b11] = &sp;

	breakpoint = NULL;
	breakpoint = new bool[65536];
	clear_breakpoints();
}

mc6809::~mc6809()
{
	delete breakpoint;
}

void mc6809::reset()
{
	/*
	 * For 6800 compatibility, direct page register defaults to
	 * zero after a reset.
	 */
	dp = 0x00;

	/*
	 * firq and irq masked after reset
	 */
	cc |= (I_FLAG | F_FLAG);

	/*
	 * After reset, nmi is fully disabled. Only after a first write
	 * to the system stackpointer enabled.
	 */
	nmi_blocked = true;

	/*
	 * Load program counter from vector
	 */
	pc = 0;
	pc = ((*read_8)(VECTOR_RESET)) << 8;
	pc |= (*read_8)(VECTOR_RESET+1);
}

bool mc6809::run(int16_t desired_cycles, int32_t *consumed_cycles)
{
	cycle_saldo += desired_cycles;

	bool breakpoint_reached = false;

	*consumed_cycles = 0;

	/*
	 * This loop runs always at least one instruction. If an irq or nmi is
	 * triggered, that operation is run instead.
	 */
	do {
		uint32_t old_cycles = cycles;

		uint8_t opcode = (*read_8)(pc++);
		cycles += cycles_page1[opcode];

		bool am_legal;

		uint16_t effective_address = (this->*addressing_modes_page1[opcode])(&am_legal);
		(this->*opcodes_page1[opcode])(effective_address);

		*consumed_cycles += (cycles - old_cycles);
		breakpoint_reached = breakpoint[pc];
	} while ((!breakpoint_reached) && (*consumed_cycles < cycle_saldo));

	old_nmi_line = *nmi_line;

	cycle_saldo -= *consumed_cycles;

	return breakpoint_reached;
}

/*
 *  pc  dp ac br  xr   yr   us   sp  efhinzvc  N F I
 * c000 00 01:ae 0000 d0d0 0000 0ffc -*-*---- 11 1 1
 */
void mc6809::status(char *text_buffer)
{
	sprintf(text_buffer, " pc  dp ac br  xr   yr   us   sp  efhinzvc  N F I\n"
			"%04x %02x %02x:%02x "
			"%04x %04x %04x %04x "
			"%c%c%c%c%c%c%c%c "
			"%c%c %c %c\n",
			pc, dp, ac, br,
			xr, yr, us, sp,
			cc & E_FLAG ? '*' : '-',
			cc & F_FLAG ? '*' : '-',
			cc & H_FLAG ? '*' : '-',
			cc & I_FLAG ? '*' : '-',
			cc & N_FLAG ? '*' : '-',
			cc & Z_FLAG ? '*' : '-',
			cc & V_FLAG ? '*' : '-',
			cc & C_FLAG ? '*' : '-',
			old_nmi_line ? '1' : '0',
			*nmi_line ? '1' : '0',
			*firq_line ? '1' : '0',
			*irq_line ? '1' : '0');
}


void mc6809::toggle_breakpoint(uint16_t address)
{
	breakpoint[address]  = !breakpoint[address];
}

void mc6809::clear_breakpoints()
{
	for (int i=0; i<65536; i++) {
		breakpoint[i] = false;
	}
}
