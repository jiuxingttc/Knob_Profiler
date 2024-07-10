#ifndef CLASS_EXAMPLE_H
#define CLASS_EXAMPLE_H

class MyClass{
    public:
        MyClass():_vv(1),_my_var(1){}
        int foo(int b);
    private:
        int _vv;
        int _my_var;
};
#endif