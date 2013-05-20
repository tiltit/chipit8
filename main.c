
/**
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *	
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include "SDL/SDL.h"
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>


/* Chip8 Fontset */
uint8_t ch8_fontset[80] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
  0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
  0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
  0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
  0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
  0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
  0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
  0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
  0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
  0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
  0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
  0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
  0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
  0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
  0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
  0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

uint16_t	ch8_opcode;			/* Opcode */
uint8_t		ch8_memory[4096];	/* Chip8 memory */
uint8_t		ch8_V[16];			/* CPU Registers */
uint16_t	ch8_I;				/* Index register */
uint16_t	ch8_pc;				/* Program counter */

/**
 *	Chip8 memory map
 *
 *	0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
 *	0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
 *	0x200-0xFFF - Program ROM and work RAM
 */

uint8_t		ch8_gfx[64*32];		/* Chip8 display */
uint8_t		ch8_delay_timer;	/* Delay timer */
uint8_t		ch8_sound_timer;	/* Sound timer */
uint16_t	ch8_stack[16];		/* Chip8 Stack */
uint16_t	ch8_sp;				/* Stack pointer */
uint8_t		ch8_keys[16];		/* Chip8 keys */


const float game_time_updatelength = 16; //Time between updates (ms)
float game_time_update_last;

float game_time()
{
	return SDL_GetTicks();
}

void ch8_init()
{
	uint16_t i;
	ch8_pc = 0x200;
	ch8_opcode = 0;
	ch8_I = 0;
	ch8_sp = 0;
	// Clear the display
	for(i=0;i!=64*32;++i)
		ch8_gfx[i]=0;
	// Clear the stack
	for(i=0;i!=16;++i)
		ch8_stack[i]=0;
	// Clear registers
	for(i=0;i!=16;++i)
		ch8_V[i]=0;
	// Clear memory
	for(i=0;i!=4096;++i)
		ch8_memory[i]=0;
	// Copy the fontset into memory
	for(i=0;i!=80;++i)
		ch8_memory[i]=ch8_fontset[i];

	ch8_delay_timer = 0;
	ch8_sound_timer = 0;
}

int ch8_load(char *filename)
{
	
	int i;
	FILE *file;
	uint64_t file_length;
	uint8_t *buffer;


	file = fopen(filename, "rb");
	if(!file) {
		printf("?Unable to open file");
		return 1;
	}
	// Get file length
	fseek(file, 0, SEEK_END);
	file_length = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Allocate memory
	buffer = (uint8_t *)malloc(file_length+1);
	if(!buffer) {
		fprintf(stderr, "%s\n", "Memory error" );
		close(file);
		return 1;
	}

	// Read the content into buffer
	fread(buffer, file_length, 1, file);
	fclose(file);

	// Copy content of file into chip8 memory
	for(i=0;i!=file_length;++i) {
		ch8_memory[i+512] = buffer[i];
		printf("%X", buffer[i]);
	}
	printf("\n");
	free(buffer);
	return 0;
}

void ch8_emulate_cycle() 
{
	uint8_t vx, vy;
	uint16_t x, y, i, j;
	uint8_t line;
	bool test;

	// Fetch opcode
	ch8_opcode = ch8_memory[ch8_pc] << 8;
	ch8_opcode |= ch8_memory[ch8_pc+1];
	vx = (ch8_opcode & 0x0F00) >> 8;
	vy = (ch8_opcode & 0x00F0) >> 4;
	
	printf("%X\t", ch8_opcode);
	// Decode opcode and excute opcodes
	switch(ch8_opcode & 0xF000)
	{
		case 0x000:
			switch(ch8_opcode & 0x000F)
			{
				case 0x0000:
					// 00E0: Clears the screen
					for(i=0;i!=64*32;++i)
						ch8_gfx[i] = 0;
					ch8_pc+=2;
				break;

				case 0x000E:
					// 00EE: Retrun from a subroutine
					ch8_pc = ch8_stack[--ch8_sp];
					ch8_pc += 2;
				break;

				default:
					printf("Unknown opcode [0x0000]: 0x%X\n", ch8_opcode);
			}
		break;

		case 0x1000:
			// Jumps to address 0x1NNN
			printf("Jump to: %d", ch8_opcode & 0x0FFF);
			ch8_pc = (ch8_opcode & 0x0FFF);
		break;

		case 0x2000:
			// Calls subroutine at 0x2NNN.
			ch8_stack[ch8_sp] = ch8_pc;
			ch8_sp++;
			ch8_pc = (ch8_opcode & 0x0FFF);
			printf("Call subroutine at: %d", (ch8_opcode & 0x0FFF));
		break;

		case 0x3000:
			// Opcode 3XNN
			// Skips the next instruction if VX equals NN.
			printf("if (%d=%d) pc+=4", ch8_opcode & 0x00FF, ch8_V[vx]);
			if(ch8_V[vx] == (ch8_opcode & 0x00FF))
				ch8_pc+=4;
			else
				ch8_pc+=2;
		break;

		case 0x4000:
			// Opcode 4XNN
			// Skips the next instruction if VX doesn't equals NN.
			if(ch8_V[vx] != (ch8_opcode & 0x00FF))
				ch8_pc+=4;
			else
				ch8_pc+=2;
		break;

		case 0x5000:
			// Opcode 5XY0
			// Skips the next instruction if VX equals VY.
			if(ch8_V[vx] == ch8_V[vy])
				ch8_pc+=4;
			else
				ch8_pc+=2;
		break;

		case 0x6000:
			// 6XNN : Sets VX to NN.
			ch8_V[vx] = (ch8_opcode & 0x00FF);
			printf("V[%d] = %d",vx, (ch8_opcode & 0x00FF));
			ch8_pc+=2;
		break;

		case 0x7000:
			// 7XNN : Adds NN to VX.
			printf("V[%d]=%d + %d = %d", vx, ch8_V[vx], (ch8_opcode & 0x00FF), (ch8_V[vx] + (ch8_opcode & 0x00FF)) );
			ch8_V[vx] += (ch8_opcode & 0x00FF);

			ch8_pc+=2;
		break;

		case 0x8000:
			switch(ch8_opcode & 0x000F)
			{
				case 0x000:
					// 8XY0: Sets VX to the value of VY.
					printf("ch8_V[%d]=%d",vx, ch8_V[vy]);
					ch8_V[vx] = ch8_V[vy];
					ch8_pc+=2;
				break;

				case 0x001:
					// 8XY1: Sets VX to VX or VY.
					ch8_V[vx] |= ch8_V[vy];
					ch8_pc+=2;
				break;

				case 0x002:
					// 8XY2: Sets VX to VX and VY.
					ch8_V[vx] &= ch8_V[vy];
					ch8_pc+=2;
				break;

				case 0x003:
					// 8XY3: Sets VX to VX xor VY.
					ch8_V[vx] ^= ch8_V[vy];
					ch8_pc+=2;
				break;

				case 0x004:
					// 8XY4: Adds VY to VX. 
					// VF is set to 1 when there's a carry, and to 0 when there isn't.

					if((ch8_V[vy] + ch8_V[vx]) > 255 )
						ch8_V[0xF] = 1;
					else
						ch8_V[0xF] = 0;

					printf("V[%d]+V[%d]=%d | V[F]=%d",ch8_V[vx], ch8_V[vy], (uint8_t)(ch8_V[vx] + ch8_V[vy]), ch8_V[0xF]);
					ch8_V[vx] += ch8_V[vy];
					ch8_pc+=2;
				break;

				case 0x005:
					// 8XY5: VY is subtracted from VX. 
					// VF is set to 0 when there's a borrow, and 1 when there isn't.
					if( ch8_V[vy] > ch8_V[vx] )
						ch8_V[0xF] = 0;
					else
						ch8_V[0xF] = 1;

					ch8_V[vx] -= ch8_V[vy];
					ch8_pc+=2;
				break;

				case 0x006:
					// 8XY6: Shifts VX right by one. 
					// VF is set to the value of the least significant bit of VX before the shift.
					printf("Shift V[%d] right by one | V[F] = %d", vx, ch8_V[0xF]);
					ch8_V[0xF] = ch8_V[vx] & 0b00000001;
					ch8_V[vx] >>= 1;
					ch8_pc+=2;
				break;

				case 0x007:
					// 8XY7: Sets VX to VY minus VX.
					// VF is set to 0 when there's a borrow, and 1 when there isn't.
					if(ch8_V[vx] > ch8_V[vy])
						ch8_V[0xF] = 0;
					else
						ch8_V[0xF] = 1;

					ch8_V[vx] = ch8_V[vy] - ch8_V[vx];
					ch8_pc+=2;
				break;

				case 0x00E:
					// 8XYE: Shifts VX left by one. 
					// VF is set to the value of the most significant bit of VX before the shift.
					ch8_V[0xF] = ((ch8_V[vx] & 0b10000000) >> 7);
					ch8_V[vx] <<= 1;
					ch8_pc+=2;
				break;
				default: printf("Unknown opcode [0x0800]: %X\n", ch8_opcode);
			}
		break;

		case 0x9000:
			// 9XY0: Skips the next instruction if VX doesn't equal VY.
			if(ch8_V[vx] != ch8_V[vy])
				ch8_pc += 4;
			else
				ch8_pc += 2;
		break;

		case 0xA000: 
			// ANNN: Sets I to the address NNN
			ch8_I = (ch8_opcode & 0x0FFF);
			printf("I = %d", (ch8_opcode & 0x0FFF));
			ch8_pc+=2;
		break;

		case 0xB000:
			// BNNN: Jumps to the address NNN plus V0.
			ch8_pc = (ch8_opcode & 0x0FFF) + ch8_V[0];
		break;

		case 0xC000:
			// CXNN: Sets VX to a random number and NN.
			ch8_V[vx] = (rand()%255) & (ch8_opcode & 0x00FF);
			ch8_pc+=2;
		break;

		case 0xD000:
			// DXYN: Draws a sprite at coordinate (VX, VY) 
			// that has a width of 8 pixels and a height of N pixels. 
			// Each row of 8 pixels is read as bit-coded (with the most 
			// significant bit of each byte displayed on the left) 
			// starting from memory location I; I value doesn't change after 
			// the execution of this instruction. As described above, 
			// VF is set to 1 if any screen pixels are flipped from set 
			// to unset when the sprite is drawn, and to 0 if that doesn't happen.
			
			ch8_V[0xF] = 0;
			x = ch8_V[vx];
			y = ch8_V[vy];
			printf("Draw x=%d : y=%d\n",x, y);
			
			for(i=0;i!=(ch8_opcode & 0x000F);++i) {
				line = ch8_memory[ch8_I + i];
				for(j=0;j!=8;++j) {
					if( ((ch8_gfx[((y+i)*64)+x+j] > 0) && (line & (0x80>>j))) > 0 ) {
						ch8_V[0xF] = 1;
					}
					if( ( line & (0x80>>j) ) > 0) {
						ch8_gfx[((y+i)*64)+x+j] ^= 1;
						printf("*");
					} else {
						printf("_");
					}
				}
				printf("\n");
			}
			
			ch8_pc+=2;
		break;

		case 0xE000:
			switch(ch8_opcode & 0x00FF) {
				case 0x009E:
					// EX9E: Skips the next instruction if the key stored in VX is pressed.
					printf("Check if key is pressed");
					if(ch8_keys[ch8_V[vx]] > 0)
						ch8_pc += 4;
					else
						ch8_pc += 2;
				break;

				case 0x00A1:
					// EXA1: Skips the next instruction if the key stored in VX isn't pressed.
					printf("Check if key isn't pressed");
					if(ch8_keys[ch8_V[vx]] == 0)
						ch8_pc += 4;
					else
						ch8_pc += 2;
				break;

				default: printf("Unknown opcode [0xE000]: %X\n", ch8_opcode);
			}
		break;

		case 0xF000:
			switch(ch8_opcode & 0x00FF) {
				case 0x0007:
					// FX07: Sets VX to the value of the delay timer.
					ch8_V[vx] = ch8_delay_timer;
					ch8_pc += 2;
				break;

				case 0x000A:
					// FX0A: A key press is awaited, and then stored in VX.
					test = false;
					for(i=0;i!=16;++i) {
						if(ch8_keys[i] > 0) {
							test = true;
							ch8_V[vx] = i;
						}
					}
					if(test==true)
						ch8_pc +=2;
				break;

				case 0x0015:
					// FX15: Sets the delay timer to VX.
					ch8_delay_timer = ch8_V[vx];
					ch8_pc += 2;
				break;

				case 0x0018:
					// FX18: Sets the sound timer to VX.
					ch8_sound_timer = ch8_V[vx];
					ch8_pc +=2;
				break;

				case 0x001E:
					// FX1E: Adds VX to I.

					// VF is set to 1 when range overflow (I+VX>0xFFF), and 0 when there isn't.
					// This is undocumented feature of the Chip-8 and used by Spacefight 2019! game.
					if((ch8_I + ch8_V[vx]) > 0xFFF)
						ch8_V[0xF] = 1;
					else
						ch8_V[0xF] = 0;

					ch8_I += ch8_V[vx];
					printf("i=%d V=%d", ch8_I, ch8_V[0xF]);
					ch8_pc += 2;
				break;

				case 0x0029:
					// FX29: Sets I to the location of the sprite for the character in VX. 
					// Characters 0-F (in hexadecimal) are represented by a 4x5 font.
					ch8_I = ch8_V[vx] * 5;
					ch8_pc += 2;
				break;

				case 0x0033:
					
					// FX33: Stores the Binary-coded decimal representation of VX, 
					// with the most significant of three digits at the address in I, 
					// the middle digit at I plus 1, and the least significant digit at I plus 2. 
					// (In other words, take the decimal representation of VX, 
					// place the hundreds digit in memory at location in I, the tens digit at location I+1, 
					// and the ones digit at location I+2.)

					ch8_memory[ch8_I] = ch8_V[vx] / 100;
  					ch8_memory[ch8_I + 1] = (ch8_V[vx] / 10) % 10;
  					ch8_memory[ch8_I + 2] = (ch8_V[vx] % 100) % 10;
  					ch8_pc += 2;
				break;

				case 0x0055:
					printf("V0 to VX=%d memory=", ch8_V[vx]);
					// Stores V0 to VX in memory starting at address I.
					for(i=0;i<=vx;++i) {
						ch8_memory[ch8_I+i] = ch8_V[i];
						printf("{%d, %d}", i, ch8_V[i]);
					}
					// On the original interpreter, when the operation is done, I=I+X+1.
					ch8_I += ch8_V[vx]+1;
					ch8_pc += 2;
				break;

				case 0x0065:
					// Fills V0 to VX with values from memory starting at address I.
					printf("V0 to VX=%d", ch8_V[vx]);
					for(i=0;i<=vx;++i) {
						ch8_V[i] = ch8_memory[ch8_I+i];
						printf("{%d, %d}", i, ch8_V[i]);
					}
					// On the original interpreter, when the operation is done, I=I+X+1.
					ch8_I += ch8_V[vx] + 1;
					ch8_pc += 2;
				break;

				default: printf("Unknown opcode [0xF000]: %X\n", ch8_opcode);
			}
		break;

		default:
			printf("Unknown opcode: 0x%X\n", ch8_opcode );
	}

	// Update timers
	if(ch8_delay_timer > 0)
		ch8_delay_timer--;

	if(ch8_sound_timer > 0) {
		if(ch8_sound_timer==1)
			printf("BEEP! \007 \n");
		ch8_sound_timer--;
	}
	printf("\n");
}

int main( int argc, char* argv[] )
{
	if(argc<2) {
		printf("\nUsage: %s program.ch8\n\n", argv[0]);
		return 0;
	}
	/* Init random number generator */
	srand(time(NULL));

	/* initialize SDL */
	SDL_Init(SDL_INIT_VIDEO);

	/* Set the title bar */
	SDL_WM_SetCaption("Chipit8", "Chipit8, Chip8 interpreter");

	/* Create Window */
	SDL_Surface* screen = SDL_SetVideoMode(640, 480, 32, 0);

	/* Draw the background */
	SDL_Rect rect = {0,0,640,480};
	SDL_FillRect(screen, &rect, 0x00FFFFFF);
	SDL_UpdateRect(screen, 0, 0, 0, 0);

	/* Event structure */
	SDL_Event event;

	/* Initialize Chip8 */
	ch8_init();

	/* Load Chip8 program */
	if(ch8_load(argv[1])) {
		printf("%s\n", "Error loading program file");
		return 1;
	}

	/* Start Game loop */
	int gameover = 0;
	while (!gameover)
	{
		if ((game_time()-game_time_update_last)>game_time_updatelength)
    	{
			/* look for an event */
			if (SDL_PollEvent(&event)) {
				/* an event was found */
				switch (event.type) {
					/* close button clicked */
					case SDL_QUIT:
						gameover = 1;
						break;

					/* Handle the input */
						/**
						 * Keypad layout
						 * 
						 * 123C -> 1234
						 * 456D -> qwer
						 * 789E -> asdf
						 * A0BF -> zxcv
						 */
					case SDL_KEYDOWN:
						switch (event.key.keysym.sym) {
							case 49:  ch8_keys[0x1] = 1; break;	/* Key 1 */
							case 50:  ch8_keys[0x2] = 1; break;	/* Key 2 */
							case 51:  ch8_keys[0x3] = 1; break;	/* Key 3 */
							case 52:  ch8_keys[0xC] = 1; break;	/* Key 4 */
							case 113: ch8_keys[0x4] = 1; break;	/* Key q */
							case 119: ch8_keys[0x5] = 1; break;	/* Key w */
							case 101: ch8_keys[0x6] = 1; break;	/* Key e */
							case 114: ch8_keys[0xD] = 1; break;	/* Key r */
							case 97:  ch8_keys[0x7] = 1; break;	/* Key a */
							case 115: ch8_keys[0x8] = 1; break;	/* Key s */
							case 100: ch8_keys[0x9] = 1; break;	/* Key d */
							case 102: ch8_keys[0xE] = 1; break;	/* Key f */
							case 122: ch8_keys[0xA] = 1; break;	/* Key z */
							case 120: ch8_keys[0x0] = 1; break;	/* Key x */
							case 99:  ch8_keys[0xB] = 1; break;	/* Key c */
							case 118: ch8_keys[0xF] = 1; break;	/* Key v */

							case SDLK_ESCAPE: gameover = 1; break;
						}
					break;

					case SDL_KEYUP:
						switch (event.key.keysym.sym) {
							case 49:  ch8_keys[0x1] = 0; break;	/* Key 1 */
							case 50:  ch8_keys[0x2] = 0; break;	/* Key 2 */
							case 51:  ch8_keys[0x3] = 0; break;	/* Key 3 */
							case 52:  ch8_keys[0xC] = 0; break;	/* Key 4 */
							case 113: ch8_keys[0x4] = 0; break;	/* Key q */
							case 119: ch8_keys[0x5] = 0; break;	/* Key w */
							case 101: ch8_keys[0x6] = 0; break;	/* Key e */
							case 114: ch8_keys[0xD] = 0; break;	/* Key r */
							case 97:  ch8_keys[0x7] = 0; break;	/* Key a */
							case 115: ch8_keys[0x8] = 0; break;	/* Key s */
							case 100: ch8_keys[0x9] = 0; break;	/* Key d */
							case 102: ch8_keys[0xE] = 0; break;	/* Key f */
							case 122: ch8_keys[0xA] = 0; break;	/* Key z */
							case 120: ch8_keys[0x0] = 0; break;	/* Key x */
							case 99:  ch8_keys[0xB] = 0; break;	/* Key c */
							case 118: ch8_keys[0xF] = 0; break;	/* Key v */
						}
					break;
				}
			}

			ch8_emulate_cycle();

			/* Draw the Chip8 display to the surface */
			uint32_t *surface = (uint32_t*)screen->pixels;
			int line_offset, line;
			for(line=0;line!=320;++line) {
				for(line_offset=0;line_offset!=640;++line_offset) {
					if(ch8_gfx[((line/10)*64)+(line_offset/10)])
						*((surface + ((line+80)*640)) + line_offset) = 0x0000FF00;
					else
						*((surface + ((line+80)*640)) + line_offset) = 0x00;
				}
			}

			/* Update the screen */
			SDL_UpdateRect(screen, 0, 80, 640, 400);
			game_time_update_last = game_time();
    	}
	}

	/* Cleanup SDL */
	SDL_Quit();

	return 0;
}

