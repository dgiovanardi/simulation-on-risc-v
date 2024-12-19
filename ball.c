/*
    ball.c for Simulation on RISC-V
    Copyright (C) 2024  Daniele Giovanardi   daniele.giovanardi@madenetwork.it

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
On a Debian-based Linux distribution install Clang:
sudo apt install -y clang

Then compile ball.c with Clang:
clang --target=riscv32 -march=rv32i -static -S -fno-addrsig ball.c


DON'T FORGET TO CUT THE FOLLOWING LINES FROM ball.s (in .text section):
	.attribute	4, 16
	.attribute	5, "rv32i2p0"


Parameters for assembler compiler:
(https://riscvasm.lucasteske.dev/#)

__heap_size   = 0x200;
__stack_size  = 0x800;

MEMORY
{
  ROM (rwx) : ORIGIN = 0x00000000, LENGTH = 0x01000
  RAM (rwx) : ORIGIN = 0x00001000, LENGTH = 0x01000
}


Parameters for debugger:
  - set memory size to 2000 (hex)
  - set program counter to 0
  - set stack pointer to 1A40 where:
                         1000h (RAM origin)
                       +   40h (variables)
                       +  200h (heap size)
                       +  800h (stack size)


Video controller port:
 - 0x1b00 (short)   If not 0 => Ball pos to be updated
 - 0x1b02 (short)   Ball left position
 - 0x1b04 (short)   Ball top position

*/

//---------------------------------------------------------------------------
// Constants (set in Init() function)
//---------------------------------------------------------------------------
unsigned int LcgState;  // Random number generator seed

int kScale;             // Float-point emulation: bits to shift
int kSpeedGear;         // Movement speed
int kBallRadius;        // Ball radius (on view port)
int kViewPortW;         // View port width
int kViewPortH;         // View port height
int kFieldW;            // View port width padding to prevent ball cropping (i.e. ball movement range)
int kFieldH;            // View port height padding
int kSegmentPad[3];     // (.data segment padding for stack pointer alignment)
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Variables
//---------------------------------------------------------------------------
enum TSide : int { sideTop=0, sideRight, sideBottom, sideLeft };

int ReboundPoint;       // Side coordinate (X or Y) where ball rebound
int ReboundSide;        // Side where ball rebound
int scBallPosX;         // Center of the ball (emulated float precision)
int scBallPosY;         // Center of the ball (emulated float precision)
int scBallDelta;        // X increment for every Y step on sides Left/Right
                        // Y increment for every X step on sides Top/Bottom
                        // (emulated float precision)
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Functions declaration
//---------------------------------------------------------------------------

// Float-point emulation
int  ScaleUp  (int AIntValue);     // Integer to be right shifted
int  ScaleDown(int AScaledValue);  // Scaled value to be left shifted
int  ScaleDrop(int AScaledValue);  // Drop "decimal" part of scaled value

void Init();       // Init constants and initial var values
void Tick();       // Move ball
void NewRebound(); // Calculate new rebound target and delta (X or Y) on path
int  RandomX();    // Generate new target on X axis (top/bottom side)
int  RandomY();    // Generate new target on X axis (top/bottom side)
unsigned int LcgRandom(unsigned int *state); // Random number generator
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Implementation
//---------------------------------------------------------------------------

int main()
{
short *pToBeUpdated = (short *)0x1b00;
short *pBallLeft    = (short *)0x1b02;
short *pBallTop     = (short *)0x1b04;


		Init();
		
		while(1)
		{
        Tick();
        
				*pToBeUpdated = 1;
				*pBallLeft    = ScaleDown(scBallPosX);
        *pBallTop     = ScaleDown(scBallPosY);
    }
}
//---------------------------------------------------------------------------

int ScaleUp(int AIntValue)
{
    return AIntValue << kScale;
}
//---------------------------------------------------------------------------

int ScaleDown(int AScaledValue)
{
    return AScaledValue >> kScale;
}
//---------------------------------------------------------------------------

int ScaleDrop(int AScaledValue)
{
    return (AScaledValue >> kScale) << kScale;
}
//---------------------------------------------------------------------------

void Init()
{
    LcgState     = 0x55AA55AA;

    kSpeedGear   = 2;   // Not upscaled yet
    kBallRadius  = 10;
    kViewPortW   = 367;
    kViewPortH   = 223;

    // 1-bit sign + X-bits to fit int part + (32-X)-bits as "decimal"
         if( kViewPortW > 65535 ) kScale= 8;
    else if( kViewPortW > 32767 ) kScale= 15;
    else if( kViewPortW > 16383 ) kScale= 16;
    else if( kViewPortW >  8191 ) kScale= 17;
    else if( kViewPortW >  4095 ) kScale= 18;
    else if( kViewPortW >  2047 ) kScale= 19;
    else if( kViewPortW >  1023 ) kScale= 20;
    else if( kViewPortW >   511 ) kScale= 21;
    else if( kViewPortW >   255 ) kScale= 22;
    else if( kViewPortW >   127 ) kScale= 23;

    kFieldW      = kViewPortW - (kBallRadius*2);
    kFieldH      = kViewPortH - (kBallRadius*2);

    ReboundSide  = sideLeft;
    scBallPosX   = 0;
    scBallPosY   = ScaleUp((LcgRandom(&LcgState) % (kFieldH-100)) + 50);
    NewRebound();
}
//---------------------------------------------------------------------------

void Tick()
{
    switch (ReboundSide)
    {
        case sideTop:
            if (scBallPosY <= 0) // Touch!
                NewRebound();
            else
            {
                scBallPosX += scBallDelta * kSpeedGear;
                scBallPosY -= ScaleUp(1)  * kSpeedGear;
            }
            break;

        case sideRight:
            if (scBallPosX >= ScaleUp(kFieldW-1)) // Touch!
                NewRebound();
            else
            {
                scBallPosX += ScaleUp(1)  * kSpeedGear;
                scBallPosY += scBallDelta * kSpeedGear;
            }
            break;

        case sideBottom:
            if (scBallPosY >= ScaleUp((kFieldH-1))) // Touch!
                NewRebound();
            else
            {
                scBallPosX -= scBallDelta * kSpeedGear;
                scBallPosY += ScaleUp(1)  * kSpeedGear;
            }
            break;

        case sideLeft:
            if (scBallPosX <= 0) // Touch!
                NewRebound();
            else
            {
                scBallPosX -= ScaleUp(1)  * kSpeedGear;
                scBallPosY -= scBallDelta * kSpeedGear;
            }
            break;
    }
}
//---------------------------------------------------------------------------

void NewRebound()
{
    switch (ReboundSide)
    {
        case sideTop:    // from Top to Right
            ReboundSide  = sideRight;
            scBallPosX   = ScaleDrop(scBallPosX);   // Drop "decimals"
            scBallPosY   = 0;
            ReboundPoint = RandomY();               // Random point on right side (Y)
            scBallDelta  = ScaleUp(ReboundPoint) / (kFieldW - ScaleDown(scBallPosX)); // Y delta
            break;

        case sideRight:  // from Right to Bottom
            ReboundSide  = sideBottom;
            scBallPosX   = ScaleUp  (kFieldW);
            scBallPosY   = ScaleDrop(scBallPosY);   // Drop "decimals"
            ReboundPoint = RandomX();               // Random point on bottom side (X)
            scBallDelta  = ScaleUp(ReboundPoint) / (kFieldH - ScaleDown(scBallPosY)); // X delta
            break;

        case sideBottom: // from Bottom to Left
            ReboundSide  = sideLeft;
            scBallPosX   = ScaleDrop(scBallPosX);   // Drop "decimals"
            scBallPosY   = ScaleUp  (kFieldH);
            ReboundPoint = RandomY();               // Random point on left side (Y)
            scBallDelta  = ScaleUp(ReboundPoint) / ScaleDown(scBallPosX); // Y delta
            break;

        case sideLeft:   // from Left to Top
            ReboundSide  = sideTop;
            scBallPosX   = 0;
            scBallPosY   = ScaleDrop(scBallPosY);   // Drop "decimals"
            ReboundPoint = RandomX();               // Random point on top side (X)
            scBallDelta  = ScaleUp(ReboundPoint) / ScaleDown(scBallPosY); // X delta
            break;
    }
}
//---------------------------------------------------------------------------

int RandomX()
{
    return (LcgRandom(&LcgState) % (kFieldW-100)) + 50; // Not too near the edge (at least 50px far)
}
//---------------------------------------------------------------------------

int RandomY()
{
    return (LcgRandom(&LcgState) % (kFieldH-100)) + 50; // Not too near the edge (at least 50px far)
}
//---------------------------------------------------------------------------

unsigned int LcgRandom(unsigned int *state)
{
// Precomputed parameters for Schrage's method
// https://en.wikipedia.org/wiki/Lehmer_random_number_generator
const unsigned int M = 0x7fffffff;
const unsigned int A = 48271;
const unsigned int Q = M / A;       // 44488
const unsigned int R = M % A;       //  3399

	unsigned int div = *state / Q;	// max: M / Q = A = 48,271
	unsigned int rem = *state % Q;	// max: Q - 1     = 44,487

	unsigned int s = rem * A;	    // max: 44,487 * 48,271 = 2,147,431,977 = 0x7fff3629
	unsigned int t = div * R;	    // max: 48,271 *  3,399 =   164,073,129
	unsigned int result = s - t;

	if (result < 0)
		result += M;

	return *state = result;
}
//---------------------------------------------------------------------------


// Functions adapted from muldi3.S and div.S at:
// https://github.com/gcc-mirror/gcc/blob/master/libgcc/config/riscv/muldi3.S
// https://github.com/gcc-mirror/gcc/blob/master/libgcc/config/riscv/div.S

void StdlibFunctions()
{
__asm("__mulsi3:                \n\t\
  mv     a2, a0                 \n\t\
  li     a0, 0                  \n\t\
.mulsi3_L1:                     \n\t\
  andi   a3, a1, 1              \n\t\
  beqz   a3, .mulsi3_L2         \n\t\
  add    a0, a0, a2             \n\t\
.mulsi3_L2:                     \n\t\
  srli   a1, a1, 1              \n\t\
  slli   a2, a2, 1              \n\t\
  bnez   a1, .mulsi3_L1         \n\t\
  ret                           \n\t\
                                \n\t\
                                \n\t\
__divsi3:                       \n\t\
  bltz  a0, .divsi3_L10         \n\t\
  bltz  a1, .divsi3_L11         \n\t\
                                \n\t\
                                \n\t\
__udivsi3:                      \n\t\
  mv    a2, a1                  \n\t\
  mv    a1, a0                  \n\t\
  li    a0, -1                  \n\t\
  beqz  a2, .udivsi3_L5         \n\t\
  li    a3, 1                   \n\t\
  bgeu  a2, a1, .udivsi3_L2     \n\t\
.udivsi3_L1:                    \n\t\
  blez  a2, .udivsi3_L2         \n\t\
  slli  a2, a2, 1               \n\t\
  slli  a3, a3, 1               \n\t\
  bgtu  a1, a2, .udivsi3_L1     \n\t\
.udivsi3_L2:                    \n\t\
  li    a0, 0                   \n\t\
.udivsi3_L3:                    \n\t\
  bltu  a1, a2, .udivsi3_L4     \n\t\
  sub   a1, a1, a2              \n\t\
  or    a0, a0, a3              \n\t\
.udivsi3_L4:                    \n\t\
  srli  a3, a3, 1               \n\t\
  srli  a2, a2, 1               \n\t\
  bnez  a3, .udivsi3_L3         \n\t\
.udivsi3_L5:                    \n\t\
  ret                           \n\t\
                                \n\t\
                                \n\t\
__umodsi3:                      \n\t\
  move  t0, ra                  \n\t\
  jal   __udivsi3               \n\t\
  move  a0, a1                  \n\t\
  jr    t0                      \n\t\
                                \n\t\
                                \n\t\
.divsi3_L10:                    \n\t\
  neg   a0, a0                  \n\t\
  bgtz  a1, .divsi3_L12         \n\t\
  neg   a1, a1                  \n\t\
  j     __udivsi3               \n\t\
.divsi3_L11:                    \n\t\
  neg   a1, a1                  \n\t\
.divsi3_L12:                    \n\t\
  move  t0, ra                  \n\t\
  jal   __udivsi3               \n\t\
  neg   a0, a0                  \n\t\
  jr    t0                      \n\t\
                                \n\t\
                                \n\t\
__modsi3:                       \n\t\
  move   t0, ra                 \n\t\
  bltz   a1, .modsi3_L31        \n\t\
  bltz   a0, .modsi3_L32        \n\t\
.modsi3_L30:                    \n\t\
  jal    __udivsi3              \n\t\
  move   a0, a1                 \n\t\
  jr     t0                     \n\t\
.modsi3_L31:                    \n\t\
  neg    a1, a1                 \n\t\
  bgez   a0, .modsi3_L30        \n\t\
.modsi3_L32:                    \n\t\
  neg    a0, a0                 \n\t\
  jal    __udivsi3              \n\t\
  neg    a0, a1                 \n\t\
  jr     t0");
}
