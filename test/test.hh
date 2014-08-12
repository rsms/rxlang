
#define A(expr) do { \
  if ((expr) == false) { \
    fprintf(stdout, "%s: Assertion failed: (" #expr ") at " __FILE__ ":%d\n", \
            __FUNCTION__, __LINE__); \
    exit(9); \
  } \
} while(0)
