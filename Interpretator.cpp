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
#define sIf             "ЕСЛИ"
#define sThen           "ТОГДА"
#define sElse           "ИНАЧЕ"
#define sEndIf          "КОНЕЦЕСЛИ"
#define sWhile          "ПОКА"
#define sCycle          "ЦИКЛ"
#define sEndCycle       "КОНЕЦЦИКЛА"
#define sShowVal        "СООБЩИТЬ"
#define sResult         "РЕЗУЛЬТАТ"
#define sAnd            "И"
#define sOr             "ИЛИ"
#define sNot            "НЕ"
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

#define sInt            "ЦЕЛ"
#define sAbs            "АБС"

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
    // переменная, строка, число
    tokVar, tokString, tokNumber,
    // функции
    tokInt, tokAbs, tokShowVal,
    // зарезервированные слова
    tokIf, tokThen, tokElse, tokEndIf,
    tokWhile, tokCycle, tokEndCycle,
    // операторы
    tokOr, tokAnd, tokNot,
    tokEQ, tokNE, tokLE, tokGE, tokL, tokG,
    tokPlus, tokMinus, tokMult, tokDiv, tokPower,
    tokOpenParen, tokClosParen, tokSemicol,
    // комментарий, конец
    tokComment, tokEnd
} TToken;

AnsiString sToken[tokEnd + 1] = {
    "", "", "",
    sInt, sAbs, sShowVal, sIf, sThen, sElse, sEndIf, sWhile, sCycle, sEndCycle,
    sOr, sAnd, sNot, sEQ, sNE, sLE, sGE, sLT, sGT, sPlus, sMinus, sMult, sDiv, sPower,
    sOpenParen, sClosParen, sSemicol};

AnsiString sError[20] = {
    "Ожидается открывающая скобка",
    "Неизвестная переменная",
    "Пропущен конец строки",
    "Пропущен конец комментария",
    "Неожиданый символ",
    "Пропущен оператор или точка с запятой",
    "Недопустимая операция",
    "Ожидается оператор присваивания",
    "Неожиданный оператор",
    (AnsiString)"В условном операторе нет зарезервированного слова \"" + sThen + "\"",
    (AnsiString)"В условном операторе нет зарезервированного слова \"" + sEndIf + "\"",
    (AnsiString)"В операторе цикла нет зарезервированного слова \"" + sCycle + "\"",
    (AnsiString)"В операторе цикла нет зарезервированного слова \"" + sEndCycle + "\"",
    "Ожидается выражение",
    "",
    "",
    "",
    "Ожидается закрывающая скобка",
    "",
    ""
  };

EInterpError::EInterpError(int _errcode, int _pos) :
  errcode(_errcode),
  pos(_pos),
  Exception("Строка " + IntToStr(GetRowFromPos(_script, pos)) + "\r\n" + sError[_errcode])
{
}

EInterpError::EInterpError(AnsiString _msg, int _pos) :
  errcode(-1), // System exception
  pos(_pos),
  Exception("Строка " + IntToStr(GetRowFromPos(_script, pos)) + "\r\n" + _msg)
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

TList* Vars; // Переменные
int posUk; // позиция текущего указателя
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
  // Ищем переменную с именем VarName
  for (int i = 0; i < Vars->Count; i++)
  {
    v = (TVar*) Vars->Items[i];
    if (v->Name == VarName)
    {
      v->Value = Value;
      return;
    }
  }
  v = new TVar; // Если не найдена - создаем новую
  v->Name = VarName;
  v->Value = Value;
  Vars->Add(v);
}

bool IsAlpha(char c)
{
  return ((c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='А' && c<='Я') || (c>='а' && c<='я'));
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
    if (c>='а' && c<='я')
      c += ('А' - 'а');
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
      throw EInterpError(2, posUk); // пропущен конец строки
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
      throw EInterpError(3, posUk); // пропущен конец комментария
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

// Читает из script-a лексему в CurToken с текущей позиции и переводит
// posUk на начало следующей лексемы
void ReadTok()
{
  while (posUk <= _script.Length() && IsDelim(_script[posUk]))
    posUk++; // пропустить разделители

  if (posUk > _script.Length())
  {
    CurToken = tokEnd; // конец script-a
    CurStr = "";
    return;
  }
  else
  { // нет ли на этом месте зарезерв. слова, оператора или функции
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
    CurStr = ReadNumber(); // Читать число
  }
  else
  if (_script[posUk] == sApp)
  {
    CurToken = tokString;
    CurStr = ReadString(); // Читать строку
  }
  else
  if (_script[posUk] == '{')
  {
    CurToken = tokComment;
    CurStr = ReadLongComment(); // Читать многострочный комментарий
  }
  else
  if (_script.SubString(posUk, 2) == "//")
  {
    CurToken = tokComment;
    CurStr = ReadShortComment(); // Читать однострочный комментарий
  }
  else
    throw EInterpError(4, posUk); // Неожиданый символ
}

void ReadToken()
{
  do ReadTok(); while (CurToken == tokComment); // Пропускаем комментарии
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

// Вычисляет выражение, состоящее из операций с приоритетом не меньше Prior,
// с текущей позиции и считывает следующую лексему
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
    case tokAbs: // почему в case нет диапазонов ???
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
//  Все ProcessOperatorSomething начинают с текущей позиции и
//  считывают одну лексему после оператора
//------------------------------------------------------------

void ProcessShowVal()
{
  ShowMessage(EvalExpression()); // Считаем выражение, начиная с текущей позиции
}

void ProcessGiving()
{
  AnsiString VarName = CurStr;
  ReadToken();
  if (CurToken != tokEQ)
    throw EInterpError(7, posUk - CurStr.Length()); // Ожидается оператор присваивания
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
    throw EInterpError(9, posUk - CurStr.Length()); // Пропущен оператор Then

  // Теперь posUk после sThen
  posThen = posUk;
  CountIf = 1;

  do {
    ReadToken();
    switch (CurToken)
    {
      case tokIf        :     CountIf++; break;
      case tokElse      :     if (CountIf == 1) posElse = posUk; break; // После Else
      case tokEndIf     :     CountIf--; break;
      case tokEnd       :     throw EInterpError(10, posUk); // Пропущен EndIf
    }
  } while (CountIf != 0);

  posEndIf = posUk; // После EndIf

  posUk = (ValExpr)? posThen : posElse;

  ReadToken();
  Interpretate();

  // Выход из оператора
  posUk = posEndIf;
  ReadToken();
}

void ProcessCycle()
{
  // Starting after WHILE
  int posExpr = posUk; // Начало выражения

  Variant ValExpr = EvalExpression();

  if (CurToken != tokCycle)
    throw EInterpError(11, posUk -CurStr.Length()); // Пропущен оператор Cycle

  // Теперь posUk после sCycle
  int posCycle = posUk;
  int CountWhile = 1;

  // Искать While и EndCycle
  do {
    ReadToken();
    switch (CurToken)
    {
      case tokCycle       :     CountWhile++; break;
      case tokEndCycle    :     CountWhile--; break;
      case tokEnd         :     throw EInterpError(12, posUk); // Пропущен EndCycle
    }
  } while (CountWhile != 0);

  int posEndCycle = posUk; // после EndCycle

  while (ValExpr)
  {
    posUk = posCycle;
    ReadToken();
    Interpretate();
    posUk = posExpr;
    ValExpr = EvalExpression();
  }

  // Выход из оператора
  posUk = posEndCycle;
  ReadToken();
}

void Interpretate() // выполняет с текущей позиции
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
        throw EInterpError(8, posUk - CurStr.Length()); // Неожиданный оператор
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

