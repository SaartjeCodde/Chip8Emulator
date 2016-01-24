#pragma once

#include <iostream>
#include <Windows.h>
#include <vector>

#include <glad\glad.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

typedef unsigned char U8;
typedef unsigned short U16;

class Chip8
{
public:	
	bool Initialize(const char *path = "../c8games/SAARTJE");
	void LoadFile(const char *path = "../c8games/SAARTJE");
	void Tick();
	void Draw();
	void Keypress(U8 k, int action);

	FILE *file;
	std::vector<U8> textureVector;
	U8 delayTimer = 0;
	U8 soundTimer = 0;

private:
	U8 memoryBuffer[4096] = { 0 };
	U8 reg[16] = { 0 }; // Registers; reg[x] = VX, reg[y] = VY
	U8 keys[16] = { 0 }; // 16 possible keys in CHIP-8 game
	U8 keyPress = 0;

	U16 regI = 0;
	U16 regPC = 0x200; // Program counter (program starts at 0x200)
	U16 opcode;
	U16 stack[16] = { 0 }; // Stack to hold subroutine data
	U16 stackPointer = 0;
	U16 display[64 * 32] = { 0 };

	// - Fixes for two compatibilty problems -
	// For fixing problem n°1: 0xFX55 and 0xFX65 can either not modify register I, or increment it by X + 1) 
	// The increment is required for “animal race” to work, but should not be there for “connect 4” to work.
	bool incrementRegI = true;
	// For fixing problem n°2: 0xDXYN can either ignore pixels that fall outside the screen, or wrap around)
	// Pixels should be ignored for “blitz” to work, and wrapped around for “vers” to work.
	bool ignorePixel = false;
};