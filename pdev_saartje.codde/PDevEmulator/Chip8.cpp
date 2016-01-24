#include "Chip8.h"

bool Chip8::Initialize(const char *path)
{
	for (int i = 0; i < 4096; i++)
	{
		memoryBuffer[i] = 0;
	}

	for (int i = 0; i < 16; i++)
	{
		reg[i] = 0;
		stack[i] = 0;
		keys[i] = 0;
	}

	for (int i = 0; i < 2048; i++)
	{
		display[i] = 0;
	}

	regI = 0;
	regPC = 0x200;
	stackPointer = 0;
	delayTimer = 0;
	soundTimer = 0;
	keyPress = 0;

	LoadFile(path);
	if (file == NULL) return 0;

	// - Fixes for two compatibilty problems -
	int checkSum = 0;

	for (int i = 512; i < (4096 - 512); i++)
	{
		checkSum += memoryBuffer[i];
	}
	if (checkSum == 19434) // CONNECT4
	{
		incrementRegI = false; // The increment should not be there for “connect 4” to work
	}
	if (checkSum == 40068) // BLITZ
	{
		ignorePixel = true; // Disabled pixel wrapping, pixels should be ignored for “blitz” to work
	}

	return 1;
}

void Chip8::LoadFile(const char *path)
{
	// FONT
	unsigned char chip8_fontset[80] =
	{
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};
	for (int i = 0; i < 80; i++)
	{
		memoryBuffer[i] = chip8_fontset[i];
	}

	unsigned char SuperFont[160] =
	{
		0xFF, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, // 0
		0x18, 0x78, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0xFF, 0xFF, // 1
		0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, // 2
		0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 3
		0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, // 4
		0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 5
		0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, // 6
		0xFF, 0xFF, 0x03, 0x03, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x18, // 7
		0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, // 8
		0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 9
		0x7E, 0xFF, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xC3, // A
		0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, // B
		0x3C, 0xFF, 0xC3, 0xC0, 0xC0, 0xC0, 0xC0, 0xC3, 0xFF, 0x3C, // C
		0xFC, 0xFE, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFE, 0xFC, // D
		0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, // E
		0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0, // F
	};

	//std::FILE* binaryFile;
	errno_t err = fopen_s(&file, path, "r");
	if (file != NULL)
	{
		std::fread(&memoryBuffer[0x200], sizeof U8, 4096, file);
	}
}

void Chip8::Tick()
{
	opcode = (memoryBuffer[regPC] << 8) | (memoryBuffer[regPC + 1]); // Bitwise shift of 8 bits to the left then OR it with the next byte of memory
	U16 address = opcode & 0x0FFF;

	regPC += 2; // CHIP-8 commands are 2 bytes

	U8 x = (opcode & 0x0F00) >> 8;
	U8 y = (opcode & 0x00F0) >> 4;

	switch (opcode & 0xF000)
	{
	case 0x0000:
		switch (opcode & 0x00FF)
		{
		case 0x00C0:
			std::cout << "SCHIP-8 /case 0x00CN" << std::endl;
			// SCHIP-8; Scroll display N lines down
			break;
		case 0x00E0:
			//std::cout << "case 0x00E0" << std::endl;
			// Clear the screen
			for (int i = 0; i < 32 * 64; i++)
			{
				display[i] = 0;
			}
			break;
		case 0x00EE:
			//std::cout << "case 0x00EE" << std::endl;
			// Return from a subroutine
			--stackPointer; // remove the top stack
			regPC = stack[stackPointer]; // set regPC to the previous stack				
			break;
		case 0x00FB:
			std::cout << "SCHIP-8 /case 0x00FB" << std::endl;
			// Scroll display 4 pixels RIGHT
			break;
		case 0x00FC:
			std::cout << "SCHIP-8 /case 0x00FC" << std::endl;
			// Scroll display 4 pixels LEFT
			break;
		case 0x00FD:
			std::cout << "SCHIP-8 /case 0x00FD" << std::endl;
			// Exit CHIP interpreter
			break;
		case 0x00FE:
			std::cout << "SCHIP-8 /case 0x00FE" << std::endl;
			// Disable extended screen mode
			break;
		case 0x00FF:
			std::cout << "SCHIP-8 /case 0x00FF" << std::endl;
			// Enable extended screen mode for full-screen graphics
			break;
		}
		break;
	case 0x1000:
		//std::cout << "case 0x1000" << std::endl;
		// Jump to address NNN
		regPC = (opcode & 0x0FFF);
		break;
	case 0x2000:
		//std::cout << "case 0x2000" << std::endl;
		// Execute subroutine starting at address NNN
		stack[stackPointer] = regPC;
		++stackPointer;
		regPC = (opcode & 0x0FFF);
		break;
	case 0x3000:
		//std::cout << "case 0x3000" << std::endl;
		// Skip the following instruction if the value of register VX equals NN
		if (reg[x] == (opcode & 0x00FF))
		{
			regPC += 2;
		}
		break;
	case 0x4000:
		//std::cout << "case 0x4000" << std::endl;
		// Skip the following instruction if the value of register VX is not equal to NN
		if (reg[x] != (opcode & 0x00FF))
		{
			regPC += 2;
		}
		break;
	case 0x5000:
		//std::cout << "case 0x5000" << std::endl;
		// Skip the following instruction if the value of register VX is equal to the value of register VY
		if (reg[x] == reg[y])
		{
			regPC += 2;
		}
		break;
	case 0x6000:
		//std::cout << "case 0x6000" << std::endl;
		// Store number NN in register VX
		reg[x] = (opcode & 0x00FF);
		break;
	case 0x7000:
		//std::cout << "case 0x7000" << std::endl;
		// Add the value NN to register VX
		reg[x] += (opcode & 0x00FF);
		break;
	case 0x8000:
		switch (opcode & 0x000F)
		{
		case 0x0000:
			//std::cout << "case 0x0000" << std::endl;
			// Store the value of register VY in register VX
			reg[x] = reg[y];
			break;
		case 0x0001:
			//std::cout << "case 0x0001" << std::endl;
			// Set VX to VX OR VY
			reg[x] |= reg[y];
			break;
		case 0x0002:
			//std::cout << "case 0x0002" << std::endl;
			// Set VX to VX AND VY
			reg[x] &= reg[y];
			break;
		case 0x0003:
			//std::cout << "case 0x0003" << std::endl;
			// Set VX to VX XOR VY
			reg[x] ^= reg[y];
			break;
		case 0x0004:
			//std::cout << "case 0x0004" << std::endl;
			// Add the value of register VY to register VX
			// Set VF to 1 if a carry occurs
			// Set VF to 0 if a carry does not occur
			if ((reg[x] + reg[y]) > 255)
			{
				reg[0x000F] = 1;
			}
			else
			{
				reg[0x000F] = 0;
			}
			reg[x] += reg[y];
			break;
		case 0x0005:
			//std::cout << "case 0x0005" << std::endl;
			// Subtract the value of register VY from register VX
			// Set VF to 0 if a borrow occurs
			// Set VF to 1 if a borrow does not occur
			if (reg[y] > reg[x])
			{
				reg[0xF] = 0;
			}
			else
			{
				reg[0xF] = 1;
			}
			reg[x] -= reg[y];
			break;
		case 0x0006:
			//std::cout << "case 0x0006" << std::endl;
			// Store the value of register VY shifted right one bit in register VX
			// Set register VF to the least significant bit prior to the shift
			reg[0xF] = reg[x] & 0x1;
			reg[x] >>= 1;
			break;
		case 0x0007:
			//std::cout << "case 0x0007" << std::endl;
			// Set register VX to the value of VY minus VX
			// Set VF to 0 if a borrow occurs
			// Set VF to 1 if a borrow does not occur
			if (reg[y] < reg[x])
			{
				reg[0xF] = 0;
			}
			else
			{
				reg[0xF] = 1;
			}
			reg[x] = reg[y] - reg[x];
			break;
		case 0x000E:
			//std::cout << "case 0x000E" << std::endl;
			// Store the value of register VY shifted left one bit in register VX
			// Set register VF to the most significant bit prior to the shift
			reg[0xF] = reg[x] >> 7;
			reg[x] <<= 1;
			break;
		}
		break;
	case 0x9000:
		//std::cout << "case 0x9000" << std::endl;
		// Skip the following instruction if the value of register VX is not equal to the value of register VY
		if (reg[x] != reg[y])
		{
			regPC += 2;
		}
		break;
	case 0xA000:
		//std::cout << "case 0xA000" << std::endl;
		// Store memory address NNN in register I
		regI = address;
		break;
	case 0xB000:
		//std::cout << "case 0xB000" << std::endl;
		// Jump to address NNN + V0
		regPC = address + reg[0];
		break;
	case 0xC000:
		//std::cout << "case 0xC000" << std::endl;
		// Set VX to a random number with a mask of NN
		reg[x] = (rand() % 255) & (opcode & 0x00FF);
		break;
	case 0xD000:
	{
		//std::cout << "0xD000" << std::endl;
		// Draw
		U16 X = reg[x];
		U16 Y = reg[y];
		U16 height = opcode & 0x000F;
		U16 pixel;

		reg[0xF] = 0; // reset register
		for (int yPos = 0; yPos < height; ++yPos) // loop over each row
		{
			pixel = memoryBuffer[regI + yPos]; // fetch pixel value from memory starting at position regI
			for (int xPos = 0; xPos < 8; ++xPos) // loop over 8 bits of one row
			{
				if ((pixel & (0x80 >> xPos)) != 0) // check if current pixel is set to 1
				{
					int pixelPosition = (X + xPos) + ((Y + yPos) * 64);
					// For fixing problem n°2: 0xDXYN can either ignore pixels that fall outside the screen, or wrap around
					if (pixelPosition < 0 && !ignorePixel)
					{
						pixelPosition = 2047; // -> Wraps to the last pixel
					}
					int index = 1;
					if (pixelPosition > 2047 && ignorePixel)
					{
						continue;
					}
					while (pixelPosition > 2047)
					{
						pixelPosition = (X + xPos) + (((Y - index++) + yPos) * 64);
					}
					
					if (display[pixelPosition] == 1) // check if pixel on display is set to 0,
					{
						reg[0xF] = 1; // if it is, register collision by setting register
					}
					display[pixelPosition] ^= 1; // set pixel value, using xor
				}
			}
		}
	}
	break;
	case 0xE000:
		switch (opcode & 0x000F)
		{
		case 0x000E:
			//std::cout << "case 0x000E" << std::endl;
			// Skip the following instruction if the key currently stored in register VX is pressed
			if (keys[reg[x]] != 0)
			{
				regPC += 2;
			}
			break;
		case 0x0001:
			//std::cout << "case 0x0001" << std::endl;
			// Skip the following instruction if the key currently stored in register VX is not pressed
			if (keys[reg[x]] == 0)
			{
				regPC += 2;
			}
			break;
		}
		break;
	case 0xF000:
		switch (opcode & 0x00FF)
		{
		case 0x0007:
			//std::cout << "case 0x0007" << std::endl;					
			// Store the current value of the delay timer in register VX
			reg[x] = delayTimer;
			break;
		case 0x000A:
			//std::cout << "case 0x000A" << std::endl;					
			// Wait for a keypress and store the result in register VX
			keyPress = 0;
			for (int i = 0; i < 16; i++)
			{
				if (keys[i] != 0)
				{
					reg[x] = i;
					keys[i] = 0;
					keyPress = 1;
				}
			}
			if (!keyPress)
			{
				regPC -= 2; // When there's no keypress received, return  
			}
			break;
		case 0x0015:
			//std::cout << "case 0x0015" << std::endl;					
			// Set the delay timer to the value of register VX
			delayTimer = reg[x];
			break;
		case 0x0018:
			//std::cout << "case 0x0018" << std::endl;					
			// Set the sound timer to the value of register VX
			soundTimer = reg[x];
			break;
		case 0x001E:
			//std::cout << "case 0x001E" << std::endl;					
			// Add the value stored in register VX to register I
			if (regI + reg[x] > 0xFFF) // for the carry
			{
				reg[0xF] = 1; // ex. 5 + 5, 2 digits, reg[0xF] = 1
			}
			else
			{
				reg[0xF] = 0;
			}
			regI += reg[x];
			break;
		case 0x0029:
			//std::cout << "case 0x0029" << std::endl;	
			// Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
			regI = reg[x] * 5;
			break;
		case 0x0033:
			//std::cout << "case 0x0033" << std::endl;					
			// Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
			memoryBuffer[regI] = reg[x] / 100;
			memoryBuffer[regI + 1] = (reg[x] / 10) % 10;
			memoryBuffer[regI + 2] = (reg[x] % 100) % 10;
			break;
		case 0x0030:
			std::cout << "SCHIP-8 /case 0x0030" << std::endl;
			// Point to I to 10-byte font sprite for digit VX (0..9)
			break;
		case 0x0055:
			//std::cout << "case 0x0055" << std::endl;					
			// Store the values of registers V0 to VX inclusive in memory starting at address I
			// I is set to I + X + 1 after operation
			for (int i = 0; i <= x; i++)
			{
				memoryBuffer[regI + i] = reg[i];
			}
			// For fixing problem n°1: 0xFX55 and 0xFX65 can either not modify register I, or increment it by X + 1
			if (incrementRegI)
			{
				regI += x + 1;
			}
			break;
		case 0x0065:
			//std::cout << "case 0x0065" << std::endl;
			// Fill registers V0 to VX inclusive with the values stored in memory starting at address I
			// I is set to I + X + 1 after operation
			for (int i = 0; i <= x; i++)
			{
				reg[i] = memoryBuffer[regI + i];
			}
			// For fixing problem n°1: 0xFX55 and 0xFX65 can either not modify register I, or increment it by X + 1
			if (incrementRegI)
			{
				regI += x + 1;
			}
			break;
		case 0x075:
			std::cout << "SCHIP-8 /case 0x075" << std::endl;
			// STORE V0..VX in RPL user flags (x <= 7)
			break;
		case 0x085:
			std::cout << "SCHIP-8 /case 0x085" << std::endl;
			// READ V0..VX in RPL user flags (x <= 7)
			break;
		}
		break;
	}
}

void Chip8::Draw()
{
	textureVector.clear();

	for (int y = 0; y < 32; ++y)
	{
		for (int x = 0; x < 64; ++x)
		{
			if (display[x + y * 64] == 1)
			{
				textureVector.push_back(255);
				textureVector.push_back(255);
				textureVector.push_back(255);
			}
			else
			{
				textureVector.push_back(0);
				textureVector.push_back(0);
				textureVector.push_back(0);
			}
		}
	}
}

void Chip8::Keypress(U8 k, int action)
{
	// Action press = 1; release = 0, repeat = 2
	if (action == GLFW_REPEAT) return;
	if (k == '1') keys[0x1] = action;
	else if (k == '2') keys[0x2] = action;
	else if (k == '3') keys[0x3] = action;
	else if (k == '4') keys[0xC] = action;

	else if (k == 'Q') keys[0x4] = action;
	else if (k == 'W') keys[0x5] = action;
	else if (k == 'E') keys[0x6] = action;
	else if (k == 'R') keys[0xD] = action;

	else if (k == 'A') keys[0x7] = action;
	else if (k == 'S') keys[0x8] = action;
	else if (k == 'D') keys[0x9] = action;
	else if (k == 'F') keys[0xE] = action;

	else if (k == 'Z') keys[0xA] = action;
	else if (k == 'X') keys[0x0] = action;
	else if (k == 'C') keys[0xB] = action;
	else if (k == 'V') keys[0xF] = action;
}
