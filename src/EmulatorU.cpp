/*
    RISC-V RV32I Emulator
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

//---------------------------------------------------------------------------
#pragma hdrstop
#include "EmulatorU.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

   RISC-V base class

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

RiscV::RiscV()
{
    FpMemory = NULL;
    FcMemory = 0;
    FminText = 0;
    FmaxText = 0;

    memset(FReg, 0, sizeof(FReg));
}
//---------------------------------------------------------------------------

unsigned long RiscV::getRegister( int AIndex )
{
    if (!AIndex) // zero
        return 0;
    else
        return FReg[AIndex];
}
//---------------------------------------------------------------------------

void RiscV::setRegister( int AIndex, unsigned long AValue )
{
    if (AIndex) // zero
        FReg[AIndex] = AValue;
}
//---------------------------------------------------------------------------

unsigned long RiscV::getInstruction()
{
    return *(unsigned long *)(FpMemory + PC);
}
//---------------------------------------------------------------------------

// To be used only for data or I/O ports R/W
char * RiscV::getMemory(unsigned long AAddress)
{
    if (AAddress > FcMemory)
        throw Exception("Segmentation fault");

    if (AAddress >= FminText && AAddress < FmaxText)
        throw Exception("Access to .text segment");

    return FpMemory + AAddress;
}
//---------------------------------------------------------------------------

void RiscV::Load
(
    char         *ApMemory,
    unsigned long AcMemory,
    unsigned long AInitialPC,
    unsigned long AStackPointer,
    unsigned long ATextSegmentStart,
    unsigned long ATextSegmentEnd
)
{
    FpMemory = ApMemory;
    FcMemory = AcMemory;
    FminText = ATextSegmentStart;
    FmaxText = ATextSegmentEnd;
    FPC      = AInitialPC;
    Reg[sp]  = AStackPointer;
}
//---------------------------------------------------------------------------

void RiscV::Reset(unsigned long AInitialPC, unsigned long AStackPointer)
{
    memset(FReg, 0, sizeof(FReg));

    FPC      = AInitialPC;
    Reg[sp]  = AStackPointer;
}
//---------------------------------------------------------------------------

void RiscV::GoTo(unsigned long APC)
{
    if (!FpMemory || !FcMemory)
        throw Exception("Program non loaded");

    if (APC < FminText || FPC >= FmaxText)
        throw Exception("Invalid offset");

    FPC = APC;
}
//---------------------------------------------------------------------------

void RiscV::Step()
{
    if (!FpMemory || !FcMemory)
        throw Exception("Program non loaded");

    if (FPC < FminText || FPC >= FmaxText)
        throw Exception("Segmentation fault");

    Process();
    FPC += sizeof(long);
}
//---------------------------------------------------------------------------

void RiscV::Process()
{
unsigned long iInstruction = Instruction;
String hInstruction;

    hInstruction.SetLength(sizeof(unsigned long)*2);
    BinToHex( &iInstruction, hInstruction.c_str(), sizeof(unsigned long) );

    throw Exception( String("Illegal instruction at PC ") + PC + " (" + hInstruction + ")" );
}
//---------------------------------------------------------------------------


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

   RISC-V RV32I implementation

 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool RiscV_RV32I::Decode()
{
    switch(Instruction & 0x7F)
    {
        case R_type:        DecodeFunct_7();  Execute_R     ();  break;
        case I_bits_type:   DecodeImm_I  ();  Execute_I_bits();  break;
        case I_load_type:   DecodeImm_I  ();  Execute_I_load();  break;
        case S_type:        DecodeImm_S  ();  Execute_S     ();  break;
        case B_type:        DecodeImm_B  ();  Execute_B     ();  break;
        case lui:           DecodeImm_U  ();  Execute_lui   ();  break;
        case auipc:         DecodeImm_U  ();  Execute_auipc ();  break;
        case jal:           DecodeImm_J  ();  Execute_jal   ();  break;
        case jalr:          DecodeImm_I  ();  Execute_jalr  ();  break;
        case ecall_ebreak:  DecodeImm_U  ();  Execute_ecall_ebreak();  break;
        case fence:                           Execute_fence ();  break;
        default:
            return false;
    }
    return true;
}
//---------------------------------------------------------------------------

void RiscV_RV32I::Process()
{
    if( !Decode() )            // true => Instruction decoded successfully (for the current arch)
        inherited::Process();  //         and ready to be executed
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// RV32I decoders
//---------------------------------------------------------------------------

void RiscV_RV32I::DecodeFunct_7()
{
// InsnFunct_7: funct7 rs2 rs1 funct3 rd opcode
union
{
    unsigned long packed;
    struct
    {
        unsigned opcode:7; // 6..0
        unsigned rd    :5; // 11..7
        unsigned funct3:3; // 14..12
        unsigned rs1   :5; // 19..15
        unsigned rs2   :5; // 24..20
        unsigned funct7:7; // 31..25
    } str;
} InsnFunct_7;



    InsnFunct_7.packed = Instruction;

    Ffunct  = (InsnFunct_7.str.funct7 << 3) | InsnFunct_7.str.funct3;
    Frs1    =  InsnFunct_7.str.rs1;
    Frs2    =  InsnFunct_7.str.rs2;
    Frd     =  InsnFunct_7.str.rd;
    Fopcode =  InsnFunct_7.str.opcode;   // Extracted but unused
}
//---------------------------------------------------------------------------

void RiscV_RV32I::DecodeImm_I()
{
// InsnImm_I: imm[12] rs1 funct3 rd opcode
union
{
    unsigned long packed;
    struct
    {
        unsigned opcode:7;  // 6..0
        unsigned rd    :5;  // 11..7
        unsigned funct3:3;  // 14..12
        unsigned rs1   :5;  // 19..15
        unsigned imm   :12; // 31..20
    } str;
} InsnImm_I;



    InsnImm_I.packed = Instruction;

    Fimm    = InsnImm_I.str.imm; // 12 bits
    Frs1    = InsnImm_I.str.rs1;
    Frs2    = -1;                     // Illegal value (only for debugging purposes)
    Ffunct  = InsnImm_I.str.funct3;
    Frd     = InsnImm_I.str.rd;
    Fopcode = InsnImm_I.str.opcode;   // Extracted but unused

    // Immediate sing fixup
    if (Fimm & 0x800)   // Sign bit (minus)
        Fimm |= ~0xFFF; // 1-bitwise OR on 31..12
}
//---------------------------------------------------------------------------

void RiscV_RV32I::DecodeImm_S()
{
// InsnImm_S: imm[11:5] rs2 rs1 funct3 imm[4:0] opcode
union
{
    unsigned long packed;
    struct
    {
        unsigned opcode :7;  // 6..0
        unsigned imm4_0 :5;  // 11..7
        unsigned funct3 :3;  // 14..12
        unsigned rs1    :5;  // 19..15
        unsigned rs2    :5;  // 24..20
        unsigned imm11_5:7;  // 31..25
    } str;
} InsnImm_S;



    InsnImm_S.packed = Instruction;

    Fimm    = (InsnImm_S.str.imm11_5<<5) | InsnImm_S.str.imm4_0; // 7 + 5 = 12 bits
    Frs1    = InsnImm_S.str.rs1;
    Frs2    = InsnImm_S.str.rs2;
    Ffunct  = InsnImm_S.str.funct3;
    Frd     = -1;                     // Illegal value (only for debugging purposes)
    Fopcode = InsnImm_S.str.opcode;   // Extracted but unused

    // Immediate sing fixup
    if (Fimm & 0x800)   // Sign bit (minus)
        Fimm |= ~0xFFF; // 1-bitwise OR on 31..12
}
//---------------------------------------------------------------------------

void RiscV_RV32I::DecodeImm_B()
{
// InsnImm_B: imm[12] imm[10:5] rs2 rs1 funct3 imm[4:1] imm[11] opcode
// Note: Immediate is always even (i.e. bit0 always set to 0)
union
{
    unsigned long packed;
    struct
    {
        unsigned opcode :7; // 6..0
        unsigned imm11  :1; // 7
        unsigned imm4_1 :4; // 11..8
        unsigned funct3 :3; // 14..12
        unsigned rs1    :5; // 19..15
        unsigned rs2    :5; // 24..20
        unsigned imm10_5:6; // 30..25
        unsigned imm12  :1; // 31
    } str;
} InsnImm_B;



    InsnImm_B.packed = Instruction;

    Fimm    = (InsnImm_B.str.imm12<<12) | (InsnImm_B.str.imm11<<11) | (InsnImm_B.str.imm10_5<<5) | (InsnImm_B.str.imm4_1<<1);
    Frs1    = InsnImm_B.str.rs1;
    Frs2    = InsnImm_B.str.rs2;
    Ffunct  = InsnImm_B.str.funct3;
    Frd     = -1;                     // Illegal value (only for debugging purposes)
    Fopcode = InsnImm_B.str.opcode;   // Extracted but unused

    // Immediate sing fixup
    if (Fimm & 0x1000)   // Minus
        Fimm |= ~0x1FFF; // 1-OR on 31..13
}
//---------------------------------------------------------------------------

void RiscV_RV32I::DecodeImm_U()
{
// InsnImm_U: imm[20] rd opcode
union
{
    unsigned long packed;
    struct
    {
        unsigned opcode:7;  // 6..0
        unsigned rd    :5;  // 11..7
        unsigned imm   :20; // 31..12
    } str;
} InsnImm_U;



    InsnImm_U.packed = Instruction;

    Fimm    = InsnImm_U.str.imm;
    Frs1    = -1;                     // Illegal value (only for debugging purposes)
    Frs2    = -1;                     // Illegal value (only for debugging purposes)
    Ffunct  = -1;                     // Illegal value (only for debugging purposes)
    Frd     = InsnImm_U.str.rd;
    Fopcode = InsnImm_U.str.opcode;   // Extracted but unused

    // Immediate sing fixup
    if (Fimm & 0x80000)   // Minus
        Fimm |= ~0xFFFFF; // 1-bitwise OR on 31..20
}
//---------------------------------------------------------------------------

void RiscV_RV32I::DecodeImm_J()
{
// InsnImm_J: imm[20] imm[10:1] imm[11] imm[19:12] rd opcode
// Note: Immediate is always even (i.e. bit0 always set to 0)
union
{
    unsigned long packed;
    struct
    {
        unsigned opcode  :7;  // 6..0
        unsigned rd      :5;  // 11..7
        unsigned imm19_12:8;  // 19..12
        unsigned imm11   :1;  // 20
        unsigned imm10_1 :10; // 30..21
        unsigned imm20   :1;  // 31
    } str;
} InsnImm_J;


    InsnImm_J.packed = Instruction;

    Fimm    = (InsnImm_J.str.imm20<<20) | (InsnImm_J.str.imm19_12<<12) | (InsnImm_J.str.imm11<<11) | (InsnImm_J.str.imm10_1<<1);
    Frs1    = -1;                     // Illegal value (only for debugging purposes)
    Frs2    = -1;                     // Illegal value (only for debugging purposes)
    Ffunct  = -1;                     // Illegal value (only for debugging purposes)
    Frd     = InsnImm_J.str.rd;
    Fopcode = InsnImm_J.str.opcode;   // Extracted but unused

    // Immediate sing fixup
    if (Fimm & 0x100000)   // Minus
        Fimm |= ~0x1FFFFF; // 1-OR on 31..21
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Illegal function
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_IllegalFunction()
{
unsigned int   iInstruction = Instruction;
UnicodeString  hInstruction;
unsigned short iFunct       = funct;
UnicodeString  hFunct;

    hInstruction.SetLength(8);
    BinToHex( &iInstruction, hInstruction.c_str(), hInstruction.Length()*2 );

    hFunct.SetLength(4);
    BinToHex( &iFunct, hFunct.c_str(), hFunct.Length()*2 );

    throw Exception( UnicodeString("RiscV_RV32I::Execute_IllegalFunction - Illegal funct ") + hFunct + " at PC " + hInstruction );
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// RV32I executors
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_R()
{
    switch (funct)
    {
        case R_add:     Reg[rd] = (long)Reg[rs1] +  (long)Reg[rs2];    break; // signed op
        case R_mul:     Reg[rd] = (long)Reg[rs1] *  (long)Reg[rs2];    break; // signed op
        case R_sub:     Reg[rd] = (long)Reg[rs1] -  (long)Reg[rs2];    break; // signed op
        case R_slt:     Reg[rd] = (long)Reg[rs1] <  (long)Reg[rs2];    break; // signed op
        case R_sltu:    Reg[rd] =       Reg[rs1] <        Reg[rs2];    break; // unsigned op
        case R_and:     Reg[rd] =       Reg[rs1] &        Reg[rs2];    break;
        case R_or:      Reg[rd] =       Reg[rs1] |        Reg[rs2];    break;
        case R_xor:     Reg[rd] =       Reg[rs1] ^        Reg[rs2];    break;

        case R_ssl:     Reg[rd] =       Reg[rs1] <<       Reg[rs2];    break;
        case R_srl:     Reg[rd] =       Reg[rs1] >>       Reg[rs2];    break; // unsigned op
        case R_sra:     Reg[rd] = (long)Reg[rs1] >>       Reg[rs2];    break; // signed op

        case R_mulh:    Reg[rd] = ( (int64_t)(long)Reg[rs1] *  (int64_t)(long)Reg[rs2]) >> 32;    break; // signed op
        case R_mulhu:   Reg[rd] = ((uint64_t)      Reg[rs1] * (uint64_t)      Reg[rs2]) >> 32;    break; // signed op
        case R_mulhsu:  Reg[rd] = ( (int64_t)(long)Reg[rs1] * (uint64_t)      Reg[rs2]) >> 32;    break; // signed op


        case R_div:
            if( !Reg[rs2] )
                Reg[rd] = -1;       // Division by 0 returns -1
            else if((Reg[rs1] & 0x80000000) && (long)Reg[rs2] == -1)
                Reg[rd] = Reg[rs1]; // Division overflow returns source reg.
            else
                Reg[rd] = (long)Reg[rs1] / (long)Reg[rs2];
            break;

        case R_divu:
            if( !Reg[rs2] )
                Reg[rd] = ~0UL;
            else
                Reg[rd] = Reg[rs1] / Reg[rs2];
            break;


        case R_rem:
            if( !Reg[rs2] )
                Reg[rd] = Reg[rs1]; // Reminder overflow returns 0
            else if((Reg[rs1] & 0x80000000) && (long)Reg[rs2] == -1)
                Reg[rd] = 0;        // Reminder overflow returns 0
            else
                Reg[rd] = (long)Reg[rs1] % (long)Reg[rs2];
            break;

        case R_remu:
            if( !Reg[rs2] )
                Reg[rd] = Reg[rs1];
            else
                Reg[rd] = Reg[rs1] % Reg[rs2];
            break;

        default:
            Execute_IllegalFunction();
    }
}
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_I_bits()
{
    switch (funct)
    {
        case I_addi:        Reg[rd] = Reg[rs1] + imm;     break;
        case I_xori:        Reg[rd] = Reg[rs1] ^ imm;     break;
        case I_ori:         Reg[rd] = Reg[rs1] | imm;     break;
        case I_andi:        Reg[rd] = Reg[rs1] & imm;     break;
        case I_slli:        Reg[rd] = Reg[rs1] << (imm & 0x1F); break;  // (imm & 0x1F) = shamt
        case I_srli_srai:                                                 // shamt
                 if ( (imm & 0xFE0) == 0x400) Reg[rd] = ((long)Reg[rs1]) >> (imm & 0x1F);  // imm[11:5] = 0x20 => srai
            else if (!(imm & 0xFE0))          Reg[rd] =        Reg[rs1]  >> (imm & 0x1F);  // imm[11:5] = 0x00 => srli
            else
                Execute_IllegalFunction();
            break;
        case I_slti:        Reg[rd] = (long)Reg[rs1] <                imm;  break;
        case I_sltiu:       Reg[rd] =       Reg[rs1] < (unsigned long)imm;  break;
        default:
            Execute_IllegalFunction();
            break;
    }
}
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_I_load()
{
// Memory pointer is signed char so no sign extension needed
// RISC-V is little-endian arch so no byte swap needed
    switch( funct )
    {
        case I_lb:    Reg[rd] =                    *Memory[Reg[rs1] + imm];   break;
        case I_lh:    Reg[rd] =          *(short *)(Memory[Reg[rs1] + imm]);  break;
        case I_lw:    Reg[rd] =           *(long *)(Memory[Reg[rs1] + imm]);  break;
        case I_lbu:   Reg[rd] =  *(unsigned char *)(Memory[Reg[rs1] + imm]);  break;
        case I_lhu:   Reg[rd] = *(unsigned short *)(Memory[Reg[rs1] + imm]);  break;

        default:
            Execute_IllegalFunction();
    }
}
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_S()
{
// Memory pointer is signed char so no sign extension needed
// RISC-V is little-endian arch so no byte swap needed
    switch( funct )
    {
        case S_sb:              *Memory[Reg[rs1] + imm]  = (char) Reg[rs2];   break;
        case S_sh:    *(short *)(Memory[Reg[rs1] + imm]) = (short)Reg[rs2];   break;
        case S_sw:     *(long *)(Memory[Reg[rs1] + imm]) = (long) Reg[rs2];   break;
        default:
            Execute_IllegalFunction();
    }
}
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_B()
{
    switch( funct )
    {
        case B_beq:   if (Reg[rs1] == Reg[rs2]) FPC += imm - sizeof(long);   break; // Unsigned comp.
        case B_bne:   if (Reg[rs1] != Reg[rs2]) FPC += imm - sizeof(long);   break; // Unsigned comp.
        case B_blt:   if ( ((long)Reg[rs1]) <  ((long)Reg[rs2]) ) FPC += imm - sizeof(long);   break; // Signed comp.

        case B_bge:   if ( ((long)Reg[rs1]) >= ((long)Reg[rs2]) ) FPC += imm - sizeof(long);   break; // Signed comp.
        case B_bltu:  if (Reg[rs1] <  Reg[rs2]) FPC += imm - sizeof(long);   break; // Unsigned comp.
        case B_bgeu:  if (Reg[rs1] >= Reg[rs2]) FPC += imm - sizeof(long);   break; // Unsigned comp.

        default:
            Execute_IllegalFunction();
    }
}
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_lui()
{
    Reg[rd] = imm << 12; // Signed op
}
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_auipc()
{
    Reg[rd] = PC + (imm << 12);
}
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_jal()
{
    Reg[rd] = PC + sizeof(long);
    FPC += imm - sizeof(long); // -sizeof(long) => expects PC increment
}
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_jalr()
{
    if( funct ) // funct must be 0x0
        Execute_IllegalFunction();

    Reg[rd] = PC + sizeof(long);
    FPC  = ( (((long)Reg[rs1]) + imm) & ~0x1 ) - sizeof(long); // -sizeof(long) => expects PC increment
}
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_ecall_ebreak()
{
    // Nothing to do
}
//---------------------------------------------------------------------------

void RiscV_RV32I::Execute_fence()
{
    // Nothing to do
}
//---------------------------------------------------------------------------
