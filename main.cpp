#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

int main(int argc, char *argv[])
{
    std::string str1 = "Hello World!";
    printf(str1.c_str());
    return 0;
}