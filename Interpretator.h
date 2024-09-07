//---------------------------------------------------------------------------
#ifndef InterpretatorH
#define InterpretatorH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
//---------------------------------------------------------------------------

class EInterpError : public Exception {
public:
  int errcode; // ��� ������
  int pos; // ������� � script-e ��� ��������� ������
  EInterpError(int _errcode, int _pos);
  EInterpError(AnsiString _msg, int _pos);
};

Variant RunScript(AnsiString script);
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
