#ifndef CPU_H
#define CPU_H
#include "isadefines.h"
#include "memory.h"

class CPU {
    private:
        Cache_lv1 cache1;
        int cycle_count = 0;
    public:
    
    CPU() {};

    void cycle(){
        cycle_count += 1;
        cache1.cycle();
    }

    int get_cycle(){
        return cycle_count;
    }

    // load
    // store
    // show cycles

};

#endif