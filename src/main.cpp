#include <iostream>
#include <fstream>
#include <vector>

#include "isadefines.h"
#include "tetris_demo.h"
#include "ppu.h"

byte nes_file[1000000];
long file_size;

#define valid_bit_mask  ((1 << 31))
#define update_bit_mask ((1 << 30)) 
#define tag_bit_mask    ((1 << 27) - 1 )
#define allocation_error 0xff


class Memory{
    public:
    byte load_byte(word address){}
    halfword load_halfword(word address){}
    word load_word(word address){}

    byte store_byte(word address, byte store){}
    byte store_halfword(word address, halfword store){}
    byte store_word(word address, word store){}
};

class Cache_lv1{
    // 8 blocks * 4 words
    word cache_memory[32];
    // 0: Valid bit 1: Update bit
    word cache_tags[32];

    Cache_lv1(){
        for (int i = 0; i < 32; i++){
            cache_memory[i] = 0;
            cache_tags[i] = 0;
        }
    }

    byte data_loc(word tag, byte set){
        set <<= 2;
        for (int i = 0; i < 4; i++) if(cache_memory[set + i] == tag) return set + i;
        return allocation_error;
    }

    byte load_byte(byte location, byte offset){}
    halfword load_halfword(byte location, byte offset){}
    word load_word(byte location, byte offset){}

    byte store_byte(byte location, byte offset, byte store){}
    byte store_halfword(byte location, byte offset, halfword store){}
    byte store_word(byte location, byte offset, word store){}

    void write_through(Memory &memory){
        for (int i = 0; i < 32; i++){
            if (cache_tags[i] & update_bit_mask){
                word address = ((cache_tags[i] & tag_bit_mask) << 5) | ((i >> 2) << 2); 
                if (memory.store_word(address, cache_memory[i]) != allocation_error) cache_tags[i] ^= update_bit_mask;
                else break;
            }
        }
    }
};

class Bus {
    Memory ram;
    byte load_byte(byte location, byte offset){}
    halfword load_halfword(byte location, byte offset){}
    word load_word(byte location, byte offset){}

    byte store_byte(byte location, byte offset, byte store){}
    byte store_halfword(byte location, byte offset, halfword store){}
    byte store_word(byte location, byte offset, word store){}
};

class CPU {
    public:
    Bus bus;
    Cache_lv1 cache1;
};

// allow unaligned data?

int main(int argc, char *argv[]){
    return 0;
}