#include <cstring>

void f(const char* s)
{
    char b[42];
    std::strcpy(b, s);
}

int main()
{
    f("test");
}
