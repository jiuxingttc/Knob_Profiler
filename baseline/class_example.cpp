#include <cstdlib>
#include <stdio.h>
#include "class_example.h"
using namespace std;

int MyClass::foo(int b){
    int var = this->_my_var + b;
    if(var > 1){
        for(int k=0; k<10000;++k){
            if(k%2==0) this->_vv += 1;
            else _vv -= 1;
        }
    }
    return var;
}

int main(int argc, char** argv){
    MyClass obj;
    int v = obj.foo(2);
    printf("%d\n", v);
}