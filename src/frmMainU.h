/*
    Simulation on RISC-V
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
#ifndef frmMainUH
#define frmMainUH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Grids.hpp>
#include <Vcl.ExtCtrls.hpp>
//---------------------------------------------------------------------------
#include "EmulatorU.h"
//---------------------------------------------------------------------------

class TfrmMain : public TForm
{
__published:	// IDE-managed Components
    TButton *btnLoadAsm;
    TMemo *memoOutput;
    TMemo *SourceContent;
    TStringGrid *DebInsn;
    TStringGrid *RegDump;
    TButton *btnRun;
    TButton *btnStop;
    TButton *btnStep;
    TButton *btnGoTo;
    TEdit *editGoTo;
    TPanel *ViewPort;
    TPanel *Ball;
    TEdit *editPC;
    TLabel *Label1;
    TLabel *Label2;
    TEdit *editStack;
    TEdit *editTextEnd;
    TLabel *Label3;
    TLabel *Label6;
    TEdit *editExecBlockSize;
    TLabel *Label7;
    TEdit *editMemSize;
    TEdit *Edit2;
    TLabel *Label8;
    TLabel *Label9;
    TEdit *editTextStart;
    TTimer *TimerStep;
    TButton *btnReset;
    TEdit *editExecBlockInterval;
    TLabel *Label10;
    TEdit *editCurPC;
    TLabel *Label11;
    TEdit *editRunAt;
    TButton *btnRunAt;
    TStringGrid *DebMemory;
    TMemo *Memo1;
    TLabel *Label12;
    TEdit *editMemWatch;
    void __fastcall btnLoadAsmClick(TObject *Sender);
    void __fastcall btnRunClick(TObject *Sender);
    void __fastcall btnStopClick(TObject *Sender);
    void __fastcall editPCKeyPress(TObject *Sender, System::WideChar &Key);
    void __fastcall TimerStepTimer(TObject *Sender);
    void __fastcall btnStepClick(TObject *Sender);
    void __fastcall btnResetClick(TObject *Sender);
    void __fastcall btnGoToClick(TObject *Sender);
    void __fastcall btnRunAtClick(TObject *Sender);
private:	// User declarations

    enum ProgramState {
        stateRunning,
        stateStopping,
        stateStopped
    };

    enum Ports : int {
        portsVideo = 0x1b00
    };

    typedef struct {
        short ToBeUpdated; // +0
        short BallLeft;    // +2
        short BallTop;     // +4
    } TVideoPort;


    RiscV_RV32I     FRiscV_CPU;     // CPU
    ProgramState    FState;         // RISC-V program running state
    char           *FpDebuggerMem;  // Memory for debugger comparison (same of RISC-V)
    char           *FpRiscVMem;     // Memory for RISC-V processor (ROM + RAM)
    int             FcRiscVMem;     // Memory size
    unsigned long   FBreakpoint;    // Breakpoint set by "Run At" button

    int     ConvertToInt(String AHex);
    String  ConvertToString(long AValue);
    void    RefreshDebug();
    void    RedrawMemory();
    void    RedrawMemoryRow(int ARow);
    void    UpdateVideo(TVideoPort *ANewValues);

    void    Run();

    __property char *RiscVMem = { read = FpRiscVMem };

public:		// User declarations
    __fastcall TfrmMain(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmMain *frmMain;
//---------------------------------------------------------------------------
#endif
