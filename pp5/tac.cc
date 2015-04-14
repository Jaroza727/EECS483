/* File: tac.cc
 * ------------
 * Implementation of Location class and Instruction class/subclasses.
 */
  
#include "tac.h"
#include "mips.h"
#include <string.h>
#include <deque>

Location::Location(Segment s, int o, const char *name) :
  variableName(strdup(name)), segment(s), offset(o), reg(Mips::zero) {}

Instruction::Instruction()
{
  liveVarsIn = new LiveVars;
  liveVarsOut = new LiveVars;
}

void Instruction::Print() {
  printf("\t%s ;", printed);
  printf("\n");
}

void Instruction::Emit(Mips *mips) {
  if (*printed)
    mips->Emit("# %s", printed);   // emit TAC as comment into assembly
  EmitSpecific(mips);
}

LiveVars* Instruction::FilterGlobalVars(LiveVars* liveVars)
{
  LiveVars* result = new LiveVars;
  for (auto var : *liveVars)
  {
    if (var->GetSegment() == fpRelative)
    {
      result->insert(var);
    }
  }
  return result;
}



LoadConstant::LoadConstant(Location *d, int v)
  : dst(d), val(v) {
  Assert(dst != NULL);
  sprintf(printed, "%s = %d", dst->GetName(), val);
}
void LoadConstant::EmitSpecific(Mips *mips) {
  mips->EmitLoadConstant(dst, val);
}

LiveVars* LoadConstant::GetKillVars()
{
  return FilterGlobalVars(new LiveVars {dst});
}



LoadStringConstant::LoadStringConstant(Location *d, const char *s)
  : dst(d) {
  Assert(dst != NULL && s != NULL);
  const char *quote = (*s == '"') ? "" : "\"";
  str = new char[strlen(s) + 2*strlen(quote) + 1];
  sprintf(str, "%s%s%s", quote, s, quote);
  quote = (strlen(str) > 50) ? "...\"" : "";
  sprintf(printed, "%s = %.50s%s", dst->GetName(), str, quote);
}
void LoadStringConstant::EmitSpecific(Mips *mips) {
  mips->EmitLoadStringConstant(dst, str);
}

LiveVars* LoadStringConstant::GetKillVars()
{
  return FilterGlobalVars(new LiveVars {dst});
}
     


LoadLabel::LoadLabel(Location *d, const char *l)
  : dst(d), label(strdup(l)) {
  Assert(dst != NULL && label != NULL);
  sprintf(printed, "%s = %s", dst->GetName(), label);
}
void LoadLabel::EmitSpecific(Mips *mips) {
  mips->EmitLoadLabel(dst, label);
}

LiveVars* LoadLabel::GetKillVars()
{
  return FilterGlobalVars(new LiveVars {dst});
}



Assign::Assign(Location *d, Location *s)
  : dst(d), src(s) {
  Assert(dst != NULL && src != NULL);
  sprintf(printed, "%s = %s", dst->GetName(), src->GetName());
}
void Assign::EmitSpecific(Mips *mips) {
  mips->EmitCopy(dst, src);
}

LiveVars* Assign::GetKillVars()
{
  return FilterGlobalVars(new LiveVars {dst});
}

LiveVars* Assign::GetGenVars()
{
  return FilterGlobalVars(new LiveVars {src});
}




Load::Load(Location *d, Location *s, int off)
  : dst(d), src(s), offset(off) {
  Assert(dst != NULL && src != NULL);
  if (offset) 
    sprintf(printed, "%s = *(%s + %d)", dst->GetName(), src->GetName(), offset);
  else
    sprintf(printed, "%s = *(%s)", dst->GetName(), src->GetName());
}
void Load::EmitSpecific(Mips *mips) {
  mips->EmitLoad(dst, src, offset);
}

LiveVars* Load::GetGenVars()
{
  return FilterGlobalVars(new LiveVars {src});
}

LiveVars* Load::GetKillVars()
{
  return FilterGlobalVars(new LiveVars {dst});
}



Store::Store(Location *d, Location *s, int off)
  : dst(d), src(s), offset(off) {
  Assert(dst != NULL && src != NULL);
  if (offset)
    sprintf(printed, "*(%s + %d) = %s", dst->GetName(), offset, src->GetName());
  else
    sprintf(printed, "*(%s) = %s", dst->GetName(), src->GetName());
}
void Store::EmitSpecific(Mips *mips) {
  mips->EmitStore(dst, src, offset);
}

LiveVars* Store::GetGenVars()
{
  return FilterGlobalVars(new LiveVars {dst, src});
}

 
const char * const BinaryOp::opName[Mips::NumOps]  = {"+", "-", "*", "/", "%", "==", "<", "&&", "||"};;

Mips::OpCode BinaryOp::OpCodeForName(const char *name) {
  for (int i = 0; i < Mips::NumOps; i++) 
    if (opName[i] && !strcmp(opName[i], name))
	return (Mips::OpCode)i;
  Failure("Unrecognized Tac operator: '%s'\n", name);
  return Mips::Add; // can't get here, but compiler doesn't know that
}

BinaryOp::BinaryOp(Mips::OpCode c, Location *d, Location *o1, Location *o2)
  : code(c), dst(d), op1(o1), op2(o2) {
  Assert(dst != NULL && op1 != NULL && op2 != NULL);
  Assert(code >= 0 && code < Mips::NumOps);
  sprintf(printed, "%s = %s %s %s", dst->GetName(), op1->GetName(), opName[code], op2->GetName());
}
void BinaryOp::EmitSpecific(Mips *mips) {	  
  mips->EmitBinaryOp(code, dst, op1, op2);
}

LiveVars* BinaryOp::GetGenVars()
{
  return FilterGlobalVars(new LiveVars {op1, op2});
}

LiveVars* BinaryOp::GetKillVars()
{
  return FilterGlobalVars(new LiveVars {dst});
}



Label::Label(const char *l) : label(strdup(l)) {
  Assert(label != NULL);
  *printed = '\0';
}
void Label::Print() {
  printf("%s:\n", label);
}
void Label::EmitSpecific(Mips *mips) {
  mips->EmitLabel(label);
}


 
Goto::Goto(const char *l) : label(strdup(l)) {
  Assert(label != NULL);
  sprintf(printed, "Goto %s", label);
}
void Goto::EmitSpecific(Mips *mips) {	  
  mips->EmitGoto(label);
}


IfZ::IfZ(Location *te, const char *l)
   : test(te), label(strdup(l)) {
  Assert(test != NULL && label != NULL);
  sprintf(printed, "IfZ %s Goto %s", test->GetName(), label);
}
void IfZ::EmitSpecific(Mips *mips) {	  
  mips->EmitIfZ(test, label);
}

LiveVars* IfZ::GetGenVars()
{
  return FilterGlobalVars(new LiveVars {test});
}



BeginFunc::BeginFunc(List<Location*> *f) {
  sprintf(printed,"BeginFunc (unassigned)");
  frameSize = -555; // used as sentinel to recognized unassigned value
  formals = f;
}
void BeginFunc::SetFrameSize(int numBytesForAllLocalsAndTemps) {
  frameSize = numBytesForAllLocalsAndTemps; 
  sprintf(printed,"BeginFunc %d", frameSize);
}
void BeginFunc::EmitSpecific(Mips *mips) {
  mips->EmitBeginFunction(frameSize);
  /* pp5: need to load all parameters to the allocated registers.
   */
  for (int i = 0; i < formals->NumElements(); i++)
  {
    auto loc = formals->Nth(i);
    if (loc->GetRegister())
      mips->FillRegister(loc, loc->GetRegister());
  }
}


EndFunc::EndFunc() : Instruction() {
  sprintf(printed, "EndFunc");
}
void EndFunc::EmitSpecific(Mips *mips) {
  mips->EmitEndFunction();
  mips->ClearRegister();
}


 
Return::Return(Location *v) : val(v) {
  sprintf(printed, "Return %s", val? val->GetName() : "");
}
void Return::EmitSpecific(Mips *mips) {	  
  mips->EmitReturn(val);
}

LiveVars* Return::GetGenVars()
{
  return FilterGlobalVars(new LiveVars {val});
}


PushParam::PushParam(Location *p)
  :  param(p) {
  Assert(param != NULL);
  sprintf(printed, "PushParam %s", param->GetName());
}
void PushParam::EmitSpecific(Mips *mips) {
  mips->EmitParam(param);
} 

LiveVars* PushParam::GetGenVars()
{
  return FilterGlobalVars(new LiveVars {param});
}



PopParams::PopParams(int nb)
  :  numBytes(nb) {
  sprintf(printed, "PopParams %d", numBytes);
}
void PopParams::EmitSpecific(Mips *mips) {
  mips->EmitPopParams(numBytes);
} 


void FnCall::EmitSpecific(Mips *mips) {
  /* pp5: need to save registers before a function call
   * and restore them back after the call.
   */
  for (auto var : *liveVarsIn)
  {
    if (var->GetRegister())
    {
      mips->SpillRegister(var, var->GetRegister());
    }
  }
  EmitCall(mips);
  for (auto var : *liveVarsIn)
  {
    if (var->GetRegister())
    {
      mips->FillRegister(var, var->GetRegister());
    }
  }
}


LCall::LCall(const char *l, Location *d)
  :  label(strdup(l)), dst(d) {
  sprintf(printed, "%s%sLCall %s", dst? dst->GetName(): "", dst?" = ":"", label);
}
void LCall::EmitCall(Mips *mips) {
  mips->EmitLCall(dst, label);
}

LiveVars* LCall::GetKillVars()
{
  if (dst)
    return FilterGlobalVars(new LiveVars {dst});
  else
    return new LiveVars;
}


ACall::ACall(Location *ma, Location *d)
  : dst(d), methodAddr(ma) {
  Assert(methodAddr != NULL);
  sprintf(printed, "%s%sACall %s", dst? dst->GetName(): "", dst?" = ":"",
	    methodAddr->GetName());
}
void ACall::EmitCall(Mips *mips) {
  mips->EmitACall(dst, methodAddr);
} 

LiveVars* ACall::GetKillVars()
{
  if (dst)
    return FilterGlobalVars(new LiveVars {dst});
  else
    return new LiveVars;
}



VTable::VTable(const char *l, List<const char *> *m)
  : methodLabels(m), label(strdup(l)) {
  Assert(methodLabels != NULL && label != NULL);
  sprintf(printed, "VTable for class %s", l);
}

void VTable::Print() {
  printf("VTable %s =\n", label);
  for (int i = 0; i < methodLabels->NumElements(); i++) 
    printf("\t%s,\n", methodLabels->Nth(i));
  printf("; \n"); 
}
void VTable::EmitSpecific(Mips *mips) {
  mips->EmitVTable(label, methodLabels);
}


