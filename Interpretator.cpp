//---------------------------------------------------------------------------
#include "Interpretator.h"
#include <Classes.hpp>
//#include <Math.h>
#include <Math.hpp>
#include <Dialogs.hpp>
//---------------------------------------------------------------------------
#define sDecSepar       '.'
#define sUnd            '_'
#define sApp            '\''
#define sIf             "����"
#define sThen           "�����"
#define sElse           "�����"
#define sEndIf          "���������"
#define sWhile          "����"
#define sCycle          "����"
#define sEndCycle       "����������"
#define sShowVal        "��������"
#define sResult         "���������"
#define sAnd            "�"
#define sOr             "���"
#define sNot            "��"
#define sNE             "<>"
#define sLE             "<="
#define sGE             ">="
#define sEQ             "="
#define sLT             "<"
#define sGT             ">"
#define sPlus           "+"
#define sMinus          "-"
#define sMult           "*"
#define sDiv            "/"
#define sPower          "^"
#define sOpenParen      "("
#define sClosParen      ")"
#define sSemicol        ";"

#define sInt            "���"
#define sAbs            "���"

AnsiString _script;

int GetRowFromPos(AnsiString text, int pos)
{
  int res = 1;
  for (int i= 1; i <= pos; i++)
    if (text.SubString(i, 2) == "\r\n")
      res++;
  return res;
}

typedef enum {
    // ����������, ������, �����
    tokVar, tokString, tokNumber,
    // �������
    tokInt, tokAbs, tokShowVal,
    // ����������������� �����
    tokIf, tokThen, tokElse, tokEndIf,
    tokWhile, tokCycle, tokEndCycle,
    // ���������
    tokOr, tokAnd, tokNot,
    tokEQ, tokNE, tokLE, tokGE, tokL, tokG,
    tokPlus, tokMinus, tokMult, tokDiv, tokPower,
    tokOpenParen, tokClosParen, tokSemicol,
    // �����������, �����
    tokComment, tokEnd
} TToken;

AnsiString sToken[tokEnd + 1] = {
    "", "", "",
    sInt, sAbs, sShowVal, sIf, sThen, sElse, sEndIf, sWhile, sCycle, sEndCycle,
    sOr, sAnd, sNot, sEQ, sNE, sLE, sGE, sLT, sGT, sPlus, sMinus, sMult, sDiv, sPower,
    sOpenParen, sClosParen, sSemicol};

AnsiString sError[20] = {
    "��������� ����������� ������",
    "����������� ����������",
    "�������� ����� ������",
    "�������� ����� �����������",
    "���������� ������",
    "�������� �������� ��� ����� � �������",
    "������������ ��������",
    "��������� �������� ������������",
    "����������� ��������",
    (AnsiString)"� �������� ��������� ��� ������������������ ����� \"" + sThen + "\"",
    (AnsiString)"� �������� ��������� ��� ������������������ ����� \"" + sEndIf + "\"",
    (AnsiString)"� ��������� ����� ��� ������������������ ����� \"" + sCycle + "\"",
    (AnsiString)"� ��������� ����� ��� ������������������ ����� \"" + sEndCycle + "\"",
    "��������� ���������",
    "",
    "",
    "",
    "��������� ����������� ������",
    "",
    ""
  };

EInterpError::EInterpError(int _errcode, int _pos) :
  errcode(_errcode),
  pos(_pos),
  Exception("������ " + IntToStr(GetRowFromPos(_script, pos)) + "\r\n" + sError[_errcode])
{
}

EInterpError::EInterpError(AnsiString _msg, int _pos) :
  errcode(-1), // System exception
  pos(_pos),
  Exception("������ " + IntToStr(GetRowFromPos(_script, pos)) + "\r\n" + _msg)
{
}
  
int opprior[tokPower - tokOr + 1] = {1, 1, 2, 3, 3, 3, 3, 3, 3, 4, 4, 5, 5, 6};

int OpPrior(TToken oper)
{
  return opprior[oper - tokOr];
}

typedef struct {
    AnsiString Name;
    Variant Value;
} TVar;

TList* Vars; // ����������
int posUk; // ������� �������� ���������
//TToken
int CurToken;
AnsiString CurStr;

Variant GetVar(AnsiString VarName)
{
  int i;
  for (int i = 0; i < Vars->Count; i++)
  {
    TVar* var = (TVar*)Vars->Items[i];
    if (var->Name == VarName)
      return var->Value;
  }
  throw EInterpError(1, posUk - VarName.Length());
}

void SetVar(AnsiString VarName, Variant Value)
{
  int i;
  TVar* v;
  // ���� ���������� � ������ VarName
  for (int i = 0; i < Vars->Count; i++)
  {
    v = (TVar*) Vars->Items[i];
    if (v->Name == VarName)
    {
      v->Value = Value;
      return;
    }
  }
  v = new TVar; // ���� �� ������� - ������� �����
  v->Name = VarName;
  v->Value = Value;
  Vars->Add(v);
}

bool IsAlpha(char c)
{
  return ((c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='�' && c<='�') || (c>='�' && c<='�'));
}

bool IsCipher(char c)
{
  return (c>='0' && c<='9');
}
  
bool IsDelim(char c)
{
  return (c=='\r' || c=='\n' || c==' ' || c=='\t');
}

AnsiString UpperCaseRus(AnsiString s)
{
  for (int i= 1; i <= s.Length(); i++)
  {
    char& c = s[i];
    if (c>='a' && c<='z')
      c += ('A' - 'a');
    else
    if (c>='�' && c<='�')
      c += ('�' - '�');
  }
  return s;
}

AnsiString ReadWord()
{
  AnsiString res = "";
  while (posUk <= _script.Length())
  {
    char c = _script[posUk];
    if (!IsAlpha(c) && !IsCipher(c) && c != sUnd) break;
    res += c;
    posUk++;
  }
  return UpperCaseRus(res);
}

AnsiString ReadString()
{
  AnsiString res = sApp;
  do {
    if (++posUk > _script.Length() || _script[posUk] == '\r' || _script[posUk] == '\n')
      throw EInterpError(2, posUk); // �������� ����� ������
    res += _script[posUk];
  } while (_script[posUk] != sApp);
  posUk++;
  return res;
}

AnsiString ReadNumber()
{
  AnsiString res = "";
  while (posUk <= _script.Length() && (IsCipher(_script[posUk]) || _script[posUk] == sDecSepar))
    res += _script[posUk++];
  return res;
}

AnsiString ReadLongComment()
{
  AnsiString res = "{";
  do {
    if (++posUk > _script.Length())
      throw EInterpError(3, posUk); // �������� ����� �����������
    res += _script[posUk];
  } while (_script[posUk] != '}');
  posUk++;
  return res;
}

AnsiString ReadShortComment()
{
  AnsiString res = "";
  while (posUk <= _script.Length() && _script[posUk] != '\r' && _script[posUk] != '\n')
    res += _script[posUk++];
  return res;
}

// ������ �� script-a ������� � CurToken � ������� ������� � ���������
// posUk �� ������ ��������� �������
void ReadTok()
{
  while (posUk <= _script.Length() && IsDelim(_script[posUk]))
    posUk++; // ���������� �����������

  if (posUk > _script.Length())
  {
    CurToken = tokEnd; // ����� script-a
    CurStr = "";
    return;
  }
  else
  { // ��� �� �� ���� ����� ��������. �����, ��������� ��� �������
    for (CurToken = tokInt; CurToken <= tokSemicol; CurToken++)
      if (sToken[CurToken] == UpperCaseRus(_script.SubString(posUk, sToken[CurToken].Length())))
      {
        CurStr = sToken[CurToken];
        posUk += CurStr.Length();
        return;
      }
  }

  if (IsAlpha(_script[posUk]))
  {
    CurToken = tokVar;
    CurStr = ReadWord();
  }
  else
  if (IsCipher(_script[posUk]))
  {
    CurToken = tokNumber;
    CurStr = ReadNumber(); // ������ �����
  }
  else
  if (_script[posUk] == sApp)
  {
    CurToken = tokString;
    CurStr = ReadString(); // ������ ������
  }
  else
  if (_script[posUk] == '{')
  {
    CurToken = tokComment;
    CurStr = ReadLongComment(); // ������ ������������� �����������
  }
  else
  if (_script.SubString(posUk, 2) == "//")
  {
    CurToken = tokComment;
    CurStr = ReadShortComment(); // ������ ������������ �����������
  }
  else
    throw EInterpError(4, posUk); // ���������� ������
}

void ReadToken()
{
  do ReadTok(); while (CurToken == tokComment); // ���������� �����������
}

// Oper tokOr..tokPow
Variant DoOper(TToken Oper, Variant Operand)
{
  switch (Oper) {
    case tokNot         : return ((int)Operand)? 0 : 1;
    case tokMinus       : return -Operand;
  }
}

Variant DoOper(Variant Operand1, TToken Oper, Variant Operand2)
{
  switch (Oper) {
    case tokAnd         :  return ((int)Operand1 && (int)Operand2)? 1 : 0;
    case tokOr          :  return ((int)Operand1 || (int)Operand2)? 1 : 0;
    case tokNE          :  return (Operand1 != Operand2);
    case tokLE          :  return (Operand1 <= Operand2);
    case tokGE          :  return (Operand1 >= Operand2);
    case tokEQ          :  return (Operand1 == Operand2);
    case tokL           :  return (Operand1 <  Operand2);
    case tokG           :  return (Operand1 >  Operand2);
    case tokPlus        :  return  Operand1 + Operand2;
    case tokMinus       :  return  Operand1 - Operand2;
    case tokMult        :  return  Operand1 * Operand2;
    case tokDiv         :  return  Operand1 / Operand2;
    case tokPower       :  return  (abs(Operand2 - (int)Operand2) <= 1E-10)?
                               (double)IntPower((double)Operand1, (int)Operand2) :
                               (double)Power((double)Operand1, (double)Operand2);
  }
}
//-------------------------------------------------

Variant GetValue(int Prior);
Variant EvalExpression();
//-------------------------------------------------
                        
Variant GetFuncResult(TToken Func)
{
  Variant res;
  ReadToken();
  if (CurToken != tokOpenParen)
    throw EInterpError(0, posUk - CurStr.Length()); // missing (
  switch (Func) {
    case tokInt: res = (int)EvalExpression(); break;
    case tokAbs: res = abs(EvalExpression()); break;
  }
  if (CurToken != tokClosParen)
    throw EInterpError(17, posUk - CurStr.Length()); // missing )
  return res;  
}
//-------------------------------------------------

// ��������� ���������, ��������� �� �������� � ����������� �� ������ Prior,
// � ������� ������� � ��������� ��������� �������
Variant GetValue(int Prior)
{
  TToken Oper;
  Variant res;

  ReadToken();
  switch (CurToken)
  {
    case tokMinus:;
    case tokNot:
    {
      Oper = CurToken;
      //if OpPrior(Oper) >= Prior then
      res = DoOper(Oper, GetValue(OpPrior(Oper)));
      //else
        //raise EInterpError.Create(6,posUk-length(CurStr)); // unexpected operation
      break;  
    }
    case tokOpenParen:
    {
      res = GetValue(0);
      if (CurToken != tokClosParen)
        throw EInterpError(17, posUk - CurStr.Length()); // missing )
      ReadToken();
      break;
    }
    case tokVar:
    {
      res = GetVar(CurStr);
      ReadToken();
      break;
    }
    case tokNumber:
    {
      res = (double) StrToFloat(CurStr);
      ReadToken();
      break;
    }
    case tokString:
    {
      res = CurStr.SubString(2, CurStr.Length()-2);
      ReadToken();
      break;
    }
    case tokInt:;
    case tokAbs: // ������ � case ��� ���������� ???
    {
      res = GetFuncResult(CurToken);
      ReadToken();
      break;
    }
    default:
      throw EInterpError(13, posUk - CurStr.Length()); // expected operand
  }

  while (CurToken >= tokOr && CurToken <= tokPower && CurToken != tokNot
            && OpPrior(CurToken) > Prior)
  {
    Oper = CurToken;
    res = DoOper(res, Oper, GetValue(OpPrior(Oper)));
  }

  return res;
}
//-------------------------------------------------

Variant EvalExpression()
{
  Variant res = GetValue(0);
/*  if not (CurToken in [tokIf..tokShowVal,tokSemicol,tokEnd]) then
    raise EInterpError.Create(5,posUk-length(CurStr)) // missing operator or semicolon*/
  return res;  
}

//------------------------------------------------------------
//  ��� ProcessOperatorSomething �������� � ������� ������� �
//  ��������� ���� ������� ����� ���������
//------------------------------------------------------------

void ProcessShowVal()
{
  ShowMessage(EvalExpression()); // ������� ���������, ������� � ������� �������
}

void ProcessGiving()
{
  AnsiString VarName = CurStr;
  ReadToken();
  if (CurToken != tokEQ)
    throw EInterpError(7, posUk - CurStr.Length()); // ��������� �������� ������������
  SetVar(VarName, EvalExpression());
}

void Interpretate();

void ProcessCondition()
{
  int posThen, posElse, posEndIf, CountIf;
  Variant ValExpr;

  // Starting after IF
  ValExpr = EvalExpression();

  if (CurToken != tokThen)
    throw EInterpError(9, posUk - CurStr.Length()); // �������� �������� Then

  // ������ posUk ����� sThen
  posThen = posUk;
  CountIf = 1;

  do {
    ReadToken();
    switch (CurToken)
    {
      case tokIf        :     CountIf++; break;
      case tokElse      :     if (CountIf == 1) posElse = posUk; break; // ����� Else
      case tokEndIf     :     CountIf--; break;
      case tokEnd       :     throw EInterpError(10, posUk); // �������� EndIf
    }
  } while (CountIf != 0);

  posEndIf = posUk; // ����� EndIf

  posUk = (ValExpr)? posThen : posElse;

  ReadToken();
  Interpretate();

  // ����� �� ���������
  posUk = posEndIf;
  ReadToken();
}

void ProcessCycle()
{
  // Starting after WHILE
  int posExpr = posUk; // ������ ���������

  Variant ValExpr = EvalExpression();

  if (CurToken != tokCycle)
    throw EInterpError(11, posUk -CurStr.Length()); // �������� �������� Cycle

  // ������ posUk ����� sCycle
  int posCycle = posUk;
  int CountWhile = 1;

  // ������ While � EndCycle
  do {
    ReadToken();
    switch (CurToken)
    {
      case tokCycle       :     CountWhile++; break;
      case tokEndCycle    :     CountWhile--; break;
      case tokEnd         :     throw EInterpError(12, posUk); // �������� EndCycle
    }
  } while (CountWhile != 0);

  int posEndCycle = posUk; // ����� EndCycle

  while (ValExpr)
  {
    posUk = posCycle;
    ReadToken();
    Interpretate();
    posUk = posExpr;
    ValExpr = EvalExpression();
  }

  // ����� �� ���������
  posUk = posEndCycle;
  ReadToken();
}

void Interpretate() // ��������� � ������� �������
{
  while (true)
  {
    switch (CurToken)
    {
      case tokVar:
        ProcessGiving();
        break;
      case tokShowVal:
        ProcessShowVal();
        break;
      case tokIf:
        ProcessCondition();
        break;
      case tokWhile:
        ProcessCycle();
        break;
      case tokSemicol:
        ReadToken();
        break;
      default:
        return;
    }
  }
}
 
Variant RunScript(AnsiString script)
{
  Variant res;
  Vars = new TList;
  SetVar(sResult, "");
  _script = script;

  __try
  {

    posUk = 1;
    ReadToken();

    try
    {
      Interpretate();
      if (CurToken != tokEnd)
        throw EInterpError(8, posUk - CurStr.Length()); // ����������� ��������
    }
    catch (const EInterpError &e)
    {
//      throw;
      ShowMessage(e.Message);
//      Exception* e2 = new Exception("");
//      throw Exception("hfdjhfjd");//e2;
    }
    catch (const Exception& e) // Delphi exception
    {
//      throw EInterpError(e.Message, posUk - CurStr.Length());
      ShowMessage(e.Message);
    }

    res = GetVar(sResult);

  }
  __finally
  {
    for (int i = 0; i < Vars->Count; i++)
      delete (TVar*) (Vars->Items[i]);
    delete Vars;
//    ShowMessage("fhdsjfhdjs");
  }

  return res;
}
//---------------------------------------------------------------------------

