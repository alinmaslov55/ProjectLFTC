#include "quick.h"

int i;

int max(int x,int y){
if(x<y){
return y;
}
else{
return x;
}
}

int main(){
i=0;
while(i<10){
puti(max(i,5));
i=i+1;
}
puts("PI=");
putr(3.14159);
return 0;
}
