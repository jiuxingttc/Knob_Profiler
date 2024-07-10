#include <cstdlib>
#include <stdio.h>
using namespace std;

int func0();
int func1();
int func2(bool);
int func3();
void func4(int);

int func0(){
    int g=0;
    int v0 = 0;
    for(int v1=0,v2=10;v0 < 100 && v1<100000;v0++,v1++,v2--){
        g+=v0+v1+v2;
    }
    return g;
}
int func1(){
    int i=0,g=0;
    while(i++<1000){
        g+=i;
        func3();
    }
    return g;
}
int func2(bool flag){
    bool flag2 = flag;
    if(flag2){
        int i=0,g=0;
        while(i++<1000){
            g+=i;
            func3();
        }
        return g;
    }
   int i=0,g=0;
    while(i++<1000){
        g+=i;
    }
    return g;
}
int func3(){
    int i=0,g=0;
    while(i++<1000){
        g+=i;
    }
    return g;
}
void func4(int t){
    int iterations = 1000;
    double t2 = t + 1.2;
    printf("Number of iterations = %d|%lf\n", iterations, t2);
    while(iterations--){
        if(iterations%100==0){
            printf("iteration No. %d\n", 1000-iterations);
        }
        func1();
        if(t2>5.0){
            func2(false);
        }else{
            func2(true);
        }
    }
}
int main(int argc, char *argv[]){
    int conf1 = atoll(argv[1]);
    bool conf2 = true;
    func4(conf1);
    func4(conf2);
    return 0;
}
