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
#ifndef EmulatorUH
#define EmulatorUH
//---------------------------------------------------------------------------
#include <classes.hpp>
//---------------------------------------------------------------------------

class RiscV
{
public:
    enum Mode {
        User       = 0,
        Supervisor = 1,
        Machine    = 3,
        Invalid    = 0xff
    };

    enum RegAbi {
        zero = 0,                                 // Zero constant                      x0
        ra,                                       // Return address                     x1
        sp,                                       // Stack pointer                      x2
        gp,                                       // Global pointer                     x3
        tp,                                       // Thread pointer                     x4
        t0, t1, t2,                               // Temporaries 0-2                    x5-x7
        s0,                                       // Saved register 0 / Frame pointer   x8
        s1,                                       // Saved register 1                   x9
        a0, a1,                                   // Function args 2-7 / Ret. values    x10-x11
        a2, a3, a4, a5, a6, a7,                   // Function arguments 2-7             x12-x17
        s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, // Saved registers 2-11               x18-x27
        t3, t4, t5, t6,                           // Temporaries 3-6                    x28-x31
        fp = s0                                   // Saved register 0 / Frame pointer   x8 (alias)
    };

private:
    char           *FpMemory;
    unsigned long   FcMemory;
    unsigned long   FminText;
    unsigned long   FmaxText;

protected:
    unsigned long   FPC;
    unsigned long   FReg[32];  // FReg[0] unused (zero reg.)

    virtual     void Process();

                void SetPC(unsigned long ANewPC);

       unsigned long getRegister(int AIndex);
                void setRegister(int AIndex, unsigned long AValue);

       unsigned long getInstruction();
               char *getMemory(unsigned long AAddress); // MUST NOT BE INSIDE .text SEGMENT!

    __property unsigned long Reg[int Index] = { read=getRegister, write=setRegister };

public:
    RiscV();

    void Load (char *ApMemory, unsigned long AcMemory, unsigned long AInitialPC, unsigned long AStackPointer, unsigned long ATextSegmentStart, unsigned long ATextSegmentEnd);
    void Reset(unsigned long AInitialPC, unsigned long AStackPointer);
    void GoTo (unsigned long APC);
    void Step ();

    __property unsigned long Registers[int Index] = { read=getRegister };
    __property unsigned long PC                   = { read=FPC };
    __property unsigned long Instruction          = { read=getInstruction };
    __property         char *Memory[unsigned long Address] = { read=getMemory };
};
//---------------------------------------------------------------------------



/*                                 3                    2 2   2 1   1 1      1 1
RV32I                              1                    5 4   0 9   5 4      2 1   7 6      0
+---------------------------------+----------------------+-----+-----+--------+-----+--------+
| R-type (Register / register)    | funct7               | rs2 | rs1 | funct3 | rd  | opcode | 0110011 0x33      DecodeFunct_7
| I-type (Immediate - Bits)       | imm[11:0]                  | rs1 | funct3 | rd  | opcode | 0010011 0x13      DecodeImm_I
| I-type (Immediate - Load)       | imm[11:0]                  | rs1 | funct3 | rd  | opcode | 0000011 0x03      DecodeImm_I
| S-type (Store)                  | imm[11:5+4:0]        | rs2 | rs1 | funct3 | imm | opcode | 0100011 0x23      DecodeImm_S
| B-type (Branch)                 | imm[12+10:5+4:1+11]  | rs2 | rs1 | funct3 | imm | opcode | 1100011 0x63      DecodeImm_B
| U-type (Upper immediate)        | imm[31:12]                                | rd  | opcode | 0110111 0x37 lui / 0010111 0x17 auipc     DecodeImm_U
| J-type (Jump) - Only jal        | imm[20+10:1+11+19:12]                     | rd  | opcode | 1101111 0x6F      DecodeImm_J
| jalr                            | imm[11:0]                  | rs1 | funct3 | rd  | opcode | 1100111 0x67      DecodeImm_I
| ecall / ebreak                  | imm[31:30]                                | rd  | opcode | 1110011 0x73      DecodeImm_U
| fence                           |                                           | rd  | opcode | 0001111 0x0f      <nop>
+---------------------------------+----------------------+-----+-----+--------+-----+--------+
*/
class RiscV_RV32I : public RiscV
{
    typedef RiscV inherited;


    enum OpCodeType {
        R_type      = 0x33,
        I_bits_type = 0x13,
        I_load_type = 0x03,
        S_type      = 0x23,
        B_type      = 0x63,
        lui         = 0x37,
        auipc       = 0x17,
        jal         = 0x6f,
        jalr        = 0x67,
        ecall_ebreak= 0x73,
        fence       = 0x0f
    };
    enum OpCode_R {   // funct7   funct3
        R_add       = 0x000 >>1 | 0x0,  // add+mul+sub 0
        R_mul       = 0x010 >>1 | 0x0,
        R_sub       = 0x200 >>1 | 0x0,

        R_ssl       = 0x000 >>1 | 0x1,  // ssl+mulh    1
        R_mulh      = 0x010 >>1 | 0x1,

        R_slt       = 0x000 >>1 | 0x2,  // slt+mulhsu  2
        R_mulhsu    = 0x010 >>1 | 0x2,

        R_sltu      = 0x000 >>1 | 0x3,  // sltu+mulhu  3
        R_mulhu     = 0x010 >>1 | 0x3,

        R_xor       = 0x000 >>1 | 0x4,  // xor+div     4
        R_div       = 0x010 >>1 | 0x4,

        R_srl       = 0x000 >>1 | 0x5,  // sr          5
        R_divu      = 0x010 >>1 | 0x5,
        R_sra       = 0x200 >>1 | 0x5,

        R_or        = 0x000 >>1 | 0x6,  // or+rem      6
        R_rem       = 0x010 >>1 | 0x6,

        R_and       = 0x000 >>1 | 0x7,  // and+remu    7
        R_remu      = 0x010 >>1 | 0x7
    };

    enum OpCode_I_load {
        I_lb        = 0x0,      // rd = M[rs1+imm][0:7]
        I_lh        = 0x1,      // rd = M[rs1+imm][0:15]
        I_lw        = 0x2,      // rd = M[rs1+imm][0:31]
        I_lbu       = 0x4,      // rd = M[rs1+imm][0:7]  zero-extends
        I_lhu       = 0x5       // rd = M[rs1+imm][0:15] zero-extends
    };
    enum OpCode_I_bits {
        I_addi      = 0x0,      // rd = rs1 + imm
        I_xori      = 0x4,      // rd = rs1 ˆ imm
        I_ori       = 0x6,      // rd = rs1 | imm
        I_andi      = 0x7,      // rd = rs1 & imm
        I_slli      = 0x1,      // imm[5:11]=0x00 rd = rs1 << imm[0:4]
        I_srli_srai = 0x5,      // imm[5:11]=0x00 rd = rs1 >> imm[0:4] / imm[5:11]=0x20 rd = rs1 >> imm[0:4] msb-extends
        I_slti      = 0x2,      // rd = (rs1 < imm)?1:0
        I_sltiu     = 0x3       // rd = (rs1 < imm)?1:0 zero-extends
    };
    enum OpCode_S {
        S_sb        = 0x0,      // M[rs1+imm][0:7]  = rs2[0:7]
        S_sh        = 0x1,      // M[rs1+imm][0:15] = rs2[0:15]
        S_sw        = 0x2       // M[rs1+imm][0:31] = rs2[0:31]
    };

    enum OpCode_B {
        B_beq       = 0x0,      // if(rs1 == rs2) PC += imm
        B_bne       = 0x1,      // if(rs1 != rs2) PC += imm
        B_blt       = 0x4,      // if(rs1 <  rs2) PC += imm
        B_bge       = 0x5,      // if(rs1 >= rs2) PC += imm
        B_bltu      = 0x6,      // if(rs1 <  rs2) PC += imm zero-extends
        B_bgeu      = 0x7       // if(rs1 >= rs2) PC += imm zero-extends
    };

private:
    int Fimm;
    int Ffunct;  // funct7 + funct3 = 10-bits function code
    int Frs1;
    int Frs2;
    int Frd;
    int Fopcode; // Exctracted but unused

    void DecodeFunct_7 ();
    void DecodeImm_I   ();
    void DecodeImm_S   ();
    void DecodeImm_B   ();
    void DecodeImm_U   ();
    void DecodeImm_J   ();

    void Execute_R     ();
    void Execute_I_bits();
    void Execute_I_load();
    void Execute_S     ();
    void Execute_B     ();
    void Execute_lui   ();
    void Execute_auipc ();
    void Execute_jal   ();
    void Execute_jalr  ();
    void Execute_ecall_ebreak();
    void Execute_fence ();

    bool Decode();
    void Execute();

    void Execute_IllegalFunction();

    __property  int imm   = { read=Fimm   };
    __property  int funct = { read=Ffunct };
    __property  int rs1   = { read=Frs1   };
    __property  int rs2   = { read=Frs2   };
    __property  int rd    = { read=Frd    };

protected:
    virtual void Process();

public:
    RiscV_RV32I() : RiscV() {}
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
