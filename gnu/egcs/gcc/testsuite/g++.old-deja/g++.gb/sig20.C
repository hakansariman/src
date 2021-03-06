// Special g++ Options: -fhandle-signatures
// GROUPS passed gb sigptr initialization virtual
// Test initialization of signature pointer with object from virtual class.

extern "C"
{
  int printf (char *, ...);
}

class C
{
public:
  virtual char * f (void) { return "FA"; }
  virtual char * g (int)  { return "IL"; }
};

class D : public C
{
public:
  char * f (void) { return "PA"; }
  char * g (int)  { return "SS"; }
};

signature S
{
  char * f (void);
  char * g (int);
};

int main (void)
{
  C * p = new D;
  S * q = p;

  printf ("%s%s\n", q->f (), q->g (0));

  return 0;
}
