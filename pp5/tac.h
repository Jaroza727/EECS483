/* File: tac.h
 * -----------
 * This module contains the Instruction class (and its various
 * subclasses) that represent Tac instructions and the Location
 * class used for operands to those instructions.
 *
 * Each instruction is mostly just a little struct with a
 * few fields, but each responds polymorphically to the methods
 * Print and Emit, the first is used to print liveout the TAC form of
 * the instruction (helpful when debugging) and the second to
 * convert to the appropriate MIPS assembly.
 *
 * The operands to each instruction are of Location class.
 * A Location object is a simple representation of where a variable
 * exists at runtime, i.e. whether it is on the stack or global
 * segment and at what offset relative to the current fp or gp.
 *
 * You may need to make changes/extensions to these classes
 * if you are working on IR optimization.

 */

#ifndef _H_tac
#define _H_tac

#include "list.h" // for VTable
#include "mips.h"
#include <set>
#include <cstring>
#include <map>


    // A Location object is used to identify the operands to the
    // various TAC instructions. A Location is either fp or gp
    // relative (depending on whether livein stack or global segemnt)
    // and has an offset relative to the base of that segment.
    // For example, a declaration for integer num as the first local
    // variable livein a function would be assigned a Location object
    // with name "num", segment fpRelative, and offset -8. 
 
typedef enum {fpRelative, gpRelative} Segment;


class Location
{
  protected:
    const char *variableName;
    Segment segment;
    int offset;

    // The register allocated to this location.
    // A "zero" indicates that no register has been allocated.
    Mips::Register reg;

	  
  public:
    Location(Segment seg, int offset, const char *name);

    const char *GetName()           { return variableName; }
    Segment GetSegment()            { return segment; }
    int GetOffset()                 { return offset; }
    void SetRegister(Mips::Register r)    { reg = r; }
    Mips::Register GetRegister()          { return reg; }

};

struct CompareLocationPtr {
  bool operator() (Location* lhs, Location* rhs)
  {
    if (strcmp(lhs->GetName(), rhs->GetName()) != 0) {
      return strcmp(lhs->GetName(), rhs->GetName()) < 0;
    } else if (lhs->GetSegment() != rhs->GetSegment()) {
      return lhs->GetSegment() < rhs->GetSegment();
    } else {
      return lhs->GetOffset() < rhs->GetOffset();
    }
  }
};

using LiveVars = std::set<Location*, CompareLocationPtr>;
using InterferenceGraph = std::map<Location*, std::set<Location*, CompareLocationPtr>, CompareLocationPtr>;


  // base class from which all Tac instructions derived
  // has the interface for the 2 polymorphic messages: Print & Emit
  
class Instruction {
    protected:
        char printed[128];
	  
    public:
  Instruction();
	virtual void Print();
	virtual void EmitSpecific(Mips *mips) = 0;
	virtual void Emit(Mips *mips);
  virtual LiveVars* GetGenVars() { return new LiveVars; }
  virtual LiveVars* GetKillVars() { return new LiveVars; }

  List<Instruction*> previous; // previous instructions
  List<Instruction*> next; // next instructions
  LiveVars* liveVarsIn; // live variables at the beginning of the instruction
  LiveVars* liveVarsOut; // live variables at the end of the instruction
};

  
  
  // for convenience, the instruction classes are listed here.
  // the interfaces for the classes follows below
  
  class LoadConstant;
  class LoadStringConstant;
  class LoadLabel;
  class Assign;
  class Load;
  class Store;
  class BinaryOp;
  class Label;
  class Goto;
  class IfZ;
  class BeginFunc;
  class EndFunc;
  class Return;
  class PushParam;
  class RemoveParams;
  class LCall;
  class ACall;
  class VTable;




class LoadConstant: public Instruction {
    Location *dst;
    int val;
  public:
    LoadConstant(Location *dst, int val);
    void EmitSpecific(Mips *mips);
    LiveVars* GetKillVars() override;
};

class LoadStringConstant: public Instruction {
    Location *dst;
    char *str;
  public:
    LoadStringConstant(Location *dst, const char *s);
    void EmitSpecific(Mips *mips);
    LiveVars* GetKillVars() override;
};
    
class LoadLabel: public Instruction {
    Location *dst;
    const char *label;
  public:
    LoadLabel(Location *dst, const char *label);
    void EmitSpecific(Mips *mips);
    LiveVars* GetKillVars() override;
};

class Assign: public Instruction {
    Location *dst, *src;
  public:
    Assign(Location *dst, Location *src);
    void EmitSpecific(Mips *mips);
    LiveVars* GetGenVars() override;
    LiveVars* GetKillVars() override;
};

class Load: public Instruction {
    Location *dst, *src;
    int offset;
  public:
    Load(Location *dst, Location *src, int offset = 0);
    void EmitSpecific(Mips *mips);
    LiveVars* GetKillVars() override;
    LiveVars* GetGenVars() override;
};

class Store: public Instruction {
    Location *dst, *src;
    int offset;
  public:
    Store(Location *d, Location *s, int offset = 0);
    void EmitSpecific(Mips *mips);
    LiveVars* GetGenVars() override;
};

class BinaryOp: public Instruction {

  public:
    static const char * const opName[Mips::NumOps];
    static Mips::OpCode OpCodeForName(const char *name);
    
  protected:
    Mips::OpCode code;
    Location *dst, *op1, *op2;
  public:
    BinaryOp(Mips::OpCode c, Location *dst, Location *op1, Location *op2);
    void EmitSpecific(Mips *mips);
    LiveVars* GetGenVars() override;
    LiveVars* GetKillVars() override;
};

class Label: public Instruction {
    const char *label;
  public:
    Label(const char *label);
    void Print();
    void EmitSpecific(Mips *mips);
    const char *GetLabel() { return label; }
};

class Goto: public Instruction {
    const char *label;
  public:
    Goto(const char *label);
    void EmitSpecific(Mips *mips);
    const char *GetLabel() { return label; }
};

class IfZ: public Instruction {
    Location *test;
    const char *label;
  public:
    IfZ(Location *test, const char *label);
    void EmitSpecific(Mips *mips);
    const char *GetLabel() { return label; }
    LiveVars* GetGenVars() override;
};

class BeginFunc: public Instruction {
    int frameSize;
    List<Location*> *formals;
  public:
    BeginFunc(List<Location*> *f);
    // used to backpatch the instruction with frame size once known
    void SetFrameSize(int numBytesForAllLocalsAndTemps);
    void EmitSpecific(Mips *mips);

    InterferenceGraph interferenceGraph;
};

class EndFunc: public Instruction {
  public:
    EndFunc();
    void EmitSpecific(Mips *mips);
};

class Return: public Instruction {
    Location *val;
  public:
    Return(Location *val);
    void EmitSpecific(Mips *mips);
    LiveVars* GetGenVars() override;
};   

class PushParam: public Instruction {
    Location *param;
  public:
    PushParam(Location *param);
    void EmitSpecific(Mips *mips);
    LiveVars* GetGenVars() override;
}; 

class PopParams: public Instruction {
    int numBytes;
  public:
    PopParams(int numBytesOfParamsToRemove);
    void EmitSpecific(Mips *mips);
}; 

class FnCall: public Instruction {
  public:
    void EmitSpecific(Mips *mips);
    virtual void EmitCall(Mips *mips) = 0;
};

class LCall: public FnCall {
    const char *label;
    Location *dst;
  public:
    LCall(const char *labe, Location *result);
    void EmitCall(Mips *mips) override;
    LiveVars* GetKillVars() override;
};

class ACall: public FnCall {
    Location *dst, *methodAddr;
  public:
    ACall(Location *meth, Location *result);
    void EmitCall(Mips *mips) override;
    LiveVars* GetKillVars() override;
};

class VTable: public Instruction {
    List<const char *> *methodLabels;
    const char *label;
 public:
    VTable(const char *labelForTable, List<const char *> *methodLabels);
    void Print();
    void EmitSpecific(Mips *mips);
};


#endif
