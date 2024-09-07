//---------------------------------------------------------------------------
#ifndef InterpretatorH
#define InterpretatorH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
//---------------------------------------------------------------------------

class EInterpError : public Exception {
public:
  int errcode; // код ошибки
  int pos; // позиция в script-e где произошла ошибка
  EInterpError(int _errcode, int _pos);
  EInterpError(AnsiString _msg, int _pos);
};

Variant RunScript(AnsiString script);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
