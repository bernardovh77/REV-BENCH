#include "thermal.h"
#include <stdio.h>
#include <time.h>
int main(void)
{
    const double y[]={.1,.001,.899};rb_tw_pair s;double check=0;
    const clock_t start=clock();
    for(int repeat=0;repeat<100;repeat++) {
        if(rb_tw_prepare(&s,.003,y,900,747.384892289,650,300,37.699111843,7.791149781))return 2;
        for(int i=0;i<1000;i++)if(rb_tw_step(&s,.001))return 3;
        check+=s.energy;
    }
    const double elapsed=(double)(clock()-start)/CLOCKS_PER_SEC;
    printf("{\"steps\":100000,\"cpu_s\":%.17g,\"ns_per_call\":%.17g,\"bytes\":%zu,\"checksum\":%.17g}\n",elapsed,elapsed*1e4,sizeof s,check);
    return 0;
}
