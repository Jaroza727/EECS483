/* File: codegen.cc
 * ----------------
 * Implementation for the CodeGenerator class. The methods don't do anything
 * too fancy, mostly just create objects of the various Tac instruction
 * classes and append them to the list.
 */

#include "codegen.h"
#include <string.h>
#include "tac.h"
#include "ast_decl.h"
#include "mips.h"
#include "hashtable.h"
#include <iostream>
#include <stack>

CodeGenerator::CodeGenerator()
{
  code = new List<Instruction*>();
}

char *CodeGenerator::NewLabel()
{
  static int nextLabelNum = 0;
  char temp[10];
  sprintf(temp, "_L%d", nextLabelNum++);
  return strdup(temp);
}


Location *CodeGenerator::GenTempVariable()
{
  static int nextTempNum;
  char temp[10];
  Location *result = NULL;
  sprintf(temp, "_tmp%d", nextTempNum++);
  result = new Location(Segment::fpRelative, fn->GetOffset(), temp);
  fn->UpdateOffset();
  return result;
}

 
Location *CodeGenerator::GenLoadConstant(int value)
{
  Location *result = GenTempVariable();
  code->Append(new LoadConstant(result, value));
  return result;
}

Location *CodeGenerator::GenLoadConstant(const char *s)
{
  Location *result = GenTempVariable();
  code->Append(new LoadStringConstant(result, s));
  return result;
} 

Location *CodeGenerator::GenLoadLabel(const char *label)
{
  Location *result = GenTempVariable();
  code->Append(new LoadLabel(result, label));
  return result;
} 


void CodeGenerator::GenAssign(Location *dst, Location *src)
{
  code->Append(new Assign(dst, src));
}


Location *CodeGenerator::GenLoad(Location *ref, int offset)
{
  Location *result = GenTempVariable();
  code->Append(new Load(result, ref, offset));
  return result;
}

void CodeGenerator::GenStore(Location *dst,Location *src, int offset)
{
  code->Append(new Store(dst, src, offset));
}


Location *CodeGenerator::GenBinaryOp(const char *opName, Location *op1,
						     Location *op2)
{
  Location *result = GenTempVariable();
  code->Append(new BinaryOp(BinaryOp::OpCodeForName(opName), result, op1, op2));
  return result;
}


void CodeGenerator::GenLabel(const char *label)
{
  code->Append(new Label(label));
}

void CodeGenerator::GenIfZ(Location *test, const char *label)
{
  code->Append(new IfZ(test, label));
}

void CodeGenerator::GenGoto(const char *label)
{
  code->Append(new Goto(label));
}

void CodeGenerator::GenReturn(Location *val)
{
  code->Append(new Return(val));
}


BeginFunc *CodeGenerator::GenBeginFunc(FnDecl *f)
{
  BeginFunc *result = new BeginFunc(f->GetFormals());
  code->Append(result);
  fn = f;
  return result;
}

void CodeGenerator::GenEndFunc()
{
  code->Append(new EndFunc());
}

void CodeGenerator::GenPushParam(Location *param)
{
  code->Append(new PushParam(param));
}

void CodeGenerator::GenPopParams(int numBytesOfParams)
{
  Assert(numBytesOfParams >= 0 && numBytesOfParams % VarSize == 0); // sanity check
  if (numBytesOfParams > 0)
    code->Append(new PopParams(numBytesOfParams));
}

Location *CodeGenerator::GenLCall(const char *label, bool fnHasReturnValue)
{
  Location *result = fnHasReturnValue ? GenTempVariable() : NULL;
  code->Append(new LCall(label, result));
  return result;
}

Location *CodeGenerator::GenACall(Location *fnAddr, bool fnHasReturnValue)
{
  Location *result = fnHasReturnValue ? GenTempVariable() : NULL;
  code->Append(new ACall(fnAddr, result));
  return result;
}
 
 
static struct _builtin {
  const char *label;
  int numArgs;
  bool hasReturn;
} builtins[] =
 {{"_Alloc", 1, true},
  {"_ReadLine", 0, true},
  {"_ReadInteger", 0, true},
  {"_StringEqual", 2, true},
  {"_PrintInt", 1, false},
  {"_PrintString", 1, false},
  {"_PrintBool", 1, false},
  {"_Halt", 0, false}};

Location *CodeGenerator::GenBuiltInCall(BuiltIn bn, Location *arg1, Location *arg2)
{
  Assert(bn >= 0 && bn < NumBuiltIns);
  struct _builtin *b = &builtins[bn];
  Location *result = NULL;

  if (b->hasReturn) result = GenTempVariable();
                // verify appropriate number of non-NULL arguments given
  Assert((b->numArgs == 0 && !arg1 && !arg2)
	|| (b->numArgs == 1 && arg1 && !arg2)
	|| (b->numArgs == 2 && arg1 && arg2));
  if (arg2) code->Append(new PushParam(arg2));
  if (arg1) code->Append(new PushParam(arg1));
  code->Append(new LCall(b->label, result));
  GenPopParams(VarSize*b->numArgs);
  return result;
}


void CodeGenerator::GenVTable(const char *className, List<const char *> *methodLabels)
{
  code->Append(new VTable(className, methodLabels));
}


void CodeGenerator::DoFinalCodeGen()
{
  // Register allocation
  BuildCFG();
  LiveVariableAnalysis();
  BuildInterferenceGraph();
  ColorInterferenceGraph();

  if (IsDebugOn("tac")) { // if debug don't translate to mips, just print Tac
    for (int i = 0; i < code->NumElements(); i++)
      code->Nth(i)->Print();
  } else {
    Mips mips;
    mips.EmitPreamble();
    for (int i = 0; i < code->NumElements(); i++)
      code->Nth(i)->Emit(&mips);
  }
}

void CodeGenerator::BuildCFG()
{
  // build label to Instruction hashtable
  Hashtable<Instruction*> labelToTac;
  for (int i = 0; i < code->NumElements() - 1; i++)
  {
    if (auto labelTac = dynamic_cast<Label*>(code->Nth(i)))
    {
      labelToTac.Enter(labelTac->GetLabel(), code->Nth(i+1));
    }
  }

  // std::cout << "Debug begin..." << std::endl;
  for (int i = 0; i < code->NumElements() - 1; i++)
  {
    auto tac = code->Nth(i);
    // tac->Print();
    if (dynamic_cast<EndFunc*>(code->Nth(i)))
    {
      continue;
    }
    else if (auto ifZTac = dynamic_cast<IfZ*>(tac))
    {
      auto jumpToTac = labelToTac.Lookup(ifZTac->GetLabel());
      ifZTac->next.Append(jumpToTac);
      ifZTac->next.Append(code->Nth(i+1));
      jumpToTac->previous.Append(ifZTac);
      code->Nth(i+1)->previous.Append(ifZTac);
    }
    else if (auto gotoTac = dynamic_cast<Goto*>(tac))
    {
      auto jumpToTac = labelToTac.Lookup(gotoTac->GetLabel());
      gotoTac->next.Append(jumpToTac);
      jumpToTac->previous.Append(gotoTac);
    }
    else
    {
      tac->next.Append(code->Nth(i+1));
      code->Nth(i+1)->previous.Append(tac);
    }

    // std::cout << ">>> Next:" << std::endl;
    // for (int j = 0; j < tac->next.NumElements(); j++)
    // {
    //   tac->next.Nth(j)->Print();
    // }
    // std::cout << ">>> Next end." << std::endl;
  }
  // std::cout << "Debug end" << std::endl;
}

void CodeGenerator::LiveVariableAnalysis()
{
  bool changed = true;
  while (changed)
  {
    changed = false;
    for (int i = code->NumElements() - 1; i >= 0; i--)
    {
      auto tac = code->Nth(i);

      LiveVars *newLiveVarsOut = new LiveVars;
      for (int j = 0; j < tac->next.NumElements(); j++) {
        auto nextLiveVars = tac->next.Nth(j)->liveVarsIn;
        newLiveVarsOut->insert(nextLiveVars->begin(), nextLiveVars->end());
      }

      if (*newLiveVarsOut != *(tac->liveVarsOut))
      {
        changed = true;
      }
      tac->liveVarsOut = newLiveVarsOut;
      *(tac->liveVarsIn) = *newLiveVarsOut;
      auto gens = tac->GetGenVars();
      auto kills = tac->GetKillVars();
      for (auto killedLoc : *(kills))
        tac->liveVarsIn->erase(killedLoc);
      tac->liveVarsIn->insert(gens->begin(), gens->end());
    }
  }

  // debug output

  // for (int i = 0; i < code->NumElements(); i++)
  // {
  //   auto tac = code->Nth(i);
  //   tac->Print();

  //   // std::cout << "Kills: ";
  //   // auto kills = tac->GetKillVars();
  //   // for (auto loc : *(kills))
  //   //   std::cout << loc->GetName() << " ";
  //   // std::cout << std::endl;

  //   // std::cout << "Gens: ";
  //   // auto gens = tac->GetGenVars();
  //   // for (auto loc : *(gens))
  //   //   std::cout << loc->GetName() << " ";
  //   // std::cout << std::endl;

  //   // std::cout << "IN set: ";
  //   // for (auto loc : *(tac->liveVarsIn))
  //   //   std::cout << loc->GetName() << " ";
  //   // std::cout << std::endl;
  //   // std::cout << "Out set: ";
  //   // for (auto loc : *(tac->liveVarsOut))
  //   //   std::cout << loc->GetName() << " ";
  //   // std::cout << std::endl;
  // }
}

void CodeGenerator::BuildInterferenceGraph()
{
  InterferenceGraph* currentGraph = nullptr;
  for (int i = 0; i < code->NumElements(); i++)
  {
    auto tac = code->Nth(i);
    if (auto beginFuncTac = dynamic_cast<BeginFunc*> (tac))
    {
      currentGraph = &(beginFuncTac->interferenceGraph);
    }
    if (currentGraph)
    {
      for (auto fromTac : *(tac->liveVarsIn))
      {
        (*currentGraph)[fromTac] = {};
        for (auto toTac : *(tac->liveVarsIn))
        {
          if (fromTac != toTac)
          {
            (*currentGraph)[fromTac].insert(toTac);
          }
        }
      }

      for (auto killTac : *(tac->GetKillVars()))
      {
        (*currentGraph)[killTac] = {};
        for (auto inTac : *(tac->liveVarsIn))
        {
          if (killTac != inTac)
          {
            (*currentGraph)[killTac].insert(inTac);
            (*currentGraph)[inTac].insert(killTac);
          }
        }
      }
    }

    // debug output
    // tac->Print();
    // if (auto beginFuncTac = dynamic_cast<EndFunc*> (tac))
    // {
    //   std::cout << ">>> InferenceGraph begin: " << std::endl;
    //   for (auto& kv : *currentGraph)
    //   {
    //     std::cout << kv.first->GetName() << " -> ";
    //     for (auto toTac : kv.second)
    //     {
    //       std::cout << toTac->GetName() << " ";
    //     }
    //     std::cout << std::endl;
    //   }
    //   std::cout << ">>> InferenceGraph end." << std::endl;
    // }
  }
}

void CodeGenerator::ColorInterferenceGraph()
{
  InterferenceGraph* currentGraph = nullptr;
  for (int i = 0; i < code->NumElements(); i++)
  {
    auto tac = code->Nth(i);
    if (auto beginFuncTac = dynamic_cast<BeginFunc*> (tac))
    {
      currentGraph = &(beginFuncTac->interferenceGraph);
      std::stack<Location*> removedNodes;
      InterferenceGraph removedEdges;
      while (!currentGraph->empty())
      {
        Location* maxNode = nullptr;
        int maxNodeSize = -1;
        for (auto& kv : *currentGraph)
        {
          if ( ((int) kv.second.size()) > maxNodeSize)
          {
            maxNode = kv.first;
            maxNodeSize = (int) kv.second.size();
          }
        }
        Assert(maxNode);
        removedNodes.push(maxNode);
        removedEdges[maxNode] = (*currentGraph)[maxNode];
        currentGraph->erase(maxNode);
        for (auto& kv : *currentGraph)
        {
          kv.second.erase(maxNode);
        }
      }

      while (!removedNodes.empty())
      {
        auto node = removedNodes.top();
        removedNodes.pop();
        if ( ((int) removedEdges[node].size()) >= Mips::NumGeneralPurposeRegs)
        {
          node->SetRegister(Mips::zero);
        }
        else
        {
          std::set<Mips::Register> generalPurposeRegs
            = {Mips::t0, Mips::t1, Mips::t2, Mips::t3, Mips::t4, Mips::t5, Mips::t6, 
               Mips::t7, Mips::t8, Mips::t9, Mips::s0, Mips::s1, Mips::s2, Mips::s3,
               Mips::s4, Mips::s5, Mips::s6, Mips::s7};
          for (auto toNode : removedEdges[node])
          {
            generalPurposeRegs.erase(toNode->GetRegister());
          }
          node->SetRegister(*(generalPurposeRegs.begin()));
        }
        removedEdges.erase(node);
      }
    }
  }

  // debug output
  // for (int i = 0; i < code->NumElements(); i++)
  // {
  //   auto tac = code->Nth(i);
  //   tac->Print();
  //   std::cout << "IN set: ";
  //   for (auto loc : *(tac->liveVarsIn))
  //     std::cout << loc->GetName() << ":" << Mips().regs[loc->GetRegister()].name << " ";
  //   std::cout << std::endl;
  //   std::cout << "Out set: ";
  //   for (auto loc : *(tac->liveVarsOut))
  //     std::cout << loc->GetName() << ":" << Mips().regs[loc->GetRegister()].name << " ";
  //   std::cout << std::endl;
  //   std::cout << "----------------" << std::endl;
  // }
}

