#include <cstdlib>
#include <stdio.h>
using namespace std;

int gloab_var = 1;

class MyClass{
    public:
        MyClass():_vv(1),_my_var(1){}
        
        int foo(int b){
            int var = this->_my_var + b;
            if(var > 1){
                for(int k=0; k<10000;++k){
                    if(k%2==0){
                        _vv += 1;
                    }else{
                        _vv -= 1;
                    }
                }
            }
            if(gloab_var == 1){
                for(int k=0;k<100;++k){
                    _vv += 1;
                }
            }
            return var;
        }
    private:
        int _vv;
        int _my_var;
};

int main(int argc, char** argv){
    MyClass obj;
    int v = obj.foo(2);
    printf("v = %d\n", v);
}