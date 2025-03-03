#ifndef MEMORY_H
#define MEMORY_H
#include "isadefines.h"
#include <stdio.h>

#define LINE_SIZE 8
#define MAXMEMORY 65000

#define blocked_error   0xff
#define cache_wait      0b1000
#define cache_fetch     0b0100
#define cache_miss      0b0010
#define cache_hit       0b0001

#define valid_bit       0x80000000
#define update_bit      0x40000000
#define tag_bit_mask    0x003fffff

constexpr word memorysize = MAXMEMORY/(LINE_SIZE*4) + 1;
typedef struct memory_line {
    word data[LINE_SIZE];
} memory_line;

typedef struct cache_line
{
    bool valid = 0;
    bool update = 0;
    word tag = 0;
    word data[LINE_SIZE];
} cache_line;


class Memory{
    private:
        byte blocked = false;
        byte cooldown = 0;
        memory_line memory_data[memorysize]; // 2000 * 8 * 4 = 65kb
        memory_line buffer;

        constexpr void block(){
            blocked = true;
            cooldown = 100;
        }

    public:
        Memory(){}

        void cycle(){
            //printf("[Memory] Cycle\n");
            cooldown -= 1;
            if (cooldown == 0) blocked = false;
        }

        // load instructions
        byte buffer_address(word line){
            if (blocked) return blocked_error;
            for (int i = 0; i < LINE_SIZE; i++) buffer.data[i] = memory_data[line].data[i];
            block();
            return 0;
        }

        byte buffer_status(){
            return (!blocked);
        }

        memory_line load_buffer(){
            if (blocked) return memory_line{};
            else return buffer;
        }

        //store instructions
        byte store_byte(word line, word index, word offset, byte data){
            if (blocked) return blocked_error;
            //printf("[Memory] storing byte line %u index %u offset %u data %u\n", line, index, offset, data);
            word byte_pos = (3 - offset) << 3;
            word mask = 0xFF << byte_pos;
            memory_data[line].data[index] &= ~mask;
            memory_data[line].data[index] |= data << byte_pos;
            block();
            return 0;
        }
        
        byte store_word(word line, word index, word data){
            if (blocked) return blocked_error;
            //printf("[Memory] storing word line %u index %u data %u\n", line, index, data);
            memory_data[line].data[index] = data;
            block();
            return 0;
        }

        byte store_line(word line, cache_line data){
            if (blocked) return blocked_error;
            printf("[Memory] storing line %u\n", line);
            for (int i = 0; i < LINE_SIZE; i++) 
                memory_data[line].data[i] = data.data[i];
            return 0;
        }
};

class Bus {
    private:
        Memory ram;

        constexpr word address_to_line(word address){
            return address >> 5;
        }
        
        constexpr word address_to_index(word address){
            return (address >> 2) & 0b111;
        }

        constexpr word address_to_offset(word address){
            return address & 0b11;
        }
    
    public:
        Bus(){}

        void sdl_startup(){ }
        
        // cycle
        void cycle(){
            ram.cycle();
        }
        
        // load
        byte buffer_address(word address){
            return ram.buffer_address(address_to_line(address));
        }

        byte buffer_status(){
            return ram.buffer_status();
        }

        memory_line load_buffer(){
            return ram.load_buffer();
        }

        // store
        byte store_byte(word address, byte data){
            return ram.store_byte(address_to_line(address), address_to_index(address), address_to_offset(address), data);
        }

        byte store_word(word address, word data){
            return ram.store_word(address_to_line(address), address_to_index(address), data);
        }

        byte store_line(word address, cache_line data){
            return ram.store_line(address_to_line(address), data);
        }

};

// move bus here  
class Cache_lv1{
    private:
        byte blocked = false;
        byte status = cache_hit;
        byte cooldown = 0;
        cache_line cache_memory[16];
        memory_line buffer;
        Bus bus;
        
        word buffer_addr_load;
        word store_data;
        bool byte_mode;
        bool store;

        cache_line buffer_miss;

        void block(){
            blocked = true;
            cooldown = 1;
        }

        void unblock(){
            blocked = false;
            cooldown = 0;
        }

        constexpr word address_to_tag(word address){
            return address >> 9;
        }

        constexpr byte address_to_index(word address){
            return (address & 0b111100000) >> 5;
        }

        constexpr byte address_to_offset_word(word address){
            return (address >> 2) & 0b111;
        }

        constexpr byte address_to_offset_byte(word address){
            return address & 0b11;
        }

        inline void buff(word address){
            buffer_addr_load = address;
            status = cache_fetch;
            cooldown = 255;
        }

        inline void set_load(word address){
            buff(address);
            blocked = true;
            store = false;
            byte_mode = false;
        }

        inline void set_store_word(word address, word data){
            buff(address);
            blocked = true;
            store = true;
            byte_mode = false;
            store_data = data;
        }

        inline void set_store_byte(word address, byte data){
            buff(address);
            blocked = true;
            store = true;
            byte_mode = true;
            store_data = data;
        }

        constexpr word data_to_address(word tag, byte index, byte offset_word, byte offset_byte){
            return (tag << 9) | (index << 5) | (offset_word << 2) | offset_byte;
        }

        inline void from_cache_to_mem(cache_line &from, memory_line &to){
            for (int i = 0; i < LINE_SIZE; i++) to.data[i] = from.data[i];
        }

        void write_word(word tag, byte index, byte offset_word, word data){
            cache_memory[index].update = true;
            cache_memory[index].valid = true;
            cache_memory[index].tag = tag;
            cache_memory[index].data[offset_word] = data;
        }

        void write_byte(word tag, byte index, byte offset_word, byte offset_byte, byte data){
            cache_memory[index].update = true;
            cache_memory[index].valid = true;
            cache_memory[index].tag = tag;
            word byte_pos = (3 - offset_byte) << 3;
            word mask = 0xFF << byte_pos;
            cache_memory[index].data[offset_word] &= ~mask;
            cache_memory[index].data[offset_word] |= data << byte_pos;
        }

        void flush(){
            for (int j = 0; j < 16; j++){
                for (int i = 0; i < LINE_SIZE; i++) printf("0x%x ",cache_memory[j].data[i] );
                printf("\n");
            }
            printf("\n");
                
        }

    public:
        void cycle(){
            bus.cycle();
            cooldown -= 1;
            if (cooldown == 0) blocked = false;
            if (status == cache_fetch && bus.buffer_address(buffer_addr_load) != blocked_error){
                status = cache_wait;
                cooldown = 255;
            }
            else if (status == cache_wait && bus.buffer_status()) {
                status = cache_hit;
                unblock();
                word tag = address_to_tag(buffer_addr_load);
                byte index = address_to_index(buffer_addr_load);
                memory_line data = bus.load_buffer();
                //buffer_miss
                if (cache_memory[index].valid && cache_memory[index].update){
                    buffer_miss.tag = tag;
                    for (int i = 0; i < LINE_SIZE; i++)
                        buffer_miss.data[i] = cache_memory[index].data[i];
                    status = cache_miss;
                    cooldown = 255;
                }
                cache_memory[index].tag = tag;
                cache_memory[index].valid = 1;
                for (int i = 0; i < LINE_SIZE; i++)
                    cache_memory[index].data[i] = data.data[i];
                if (store){
                    byte offset_word = address_to_offset_word(buffer_addr_load);
                    if(byte_mode){
                        byte offset_byte = address_to_offset_word(buffer_addr_load);
                        write_byte(tag, index, offset_word, offset_byte, store_data);
                    }
                    else{
                        write_word(tag, index, offset_word, store_data);
                    }
                }
                else{
                    from_cache_to_mem(cache_memory[index], buffer);
                }
            }
            write_through();
            //flush();
        }
        
        // load
        byte buffer_address(word address){
            if (blocked) return blocked_error;
            word tag = address_to_tag(address);
            byte index = address_to_index(address);
            if (cache_memory[index].tag == tag){
                from_cache_to_mem(cache_memory[index], buffer);
                block();
                return cache_hit;
            }
            set_load(address);
            return cache_miss;
        }

        byte buffer_status(){
            return (!blocked);
        }

        memory_line load_buffer(){
            if (blocked) return memory_line{};
            else return buffer;
        }

        // store
        byte store_byte(word address, byte data){
            if (blocked) return blocked_error;
            word tag = address_to_tag(address);
            byte index = address_to_index(address);
            if (cache_memory[index].tag == tag && cache_memory[index].valid){
                status = cache_hit;
                byte offset_word = address_to_offset_word(address);
                byte offset_byte = address_to_offset_byte(address);
                write_byte(tag, index, offset_word, offset_byte, data);
                block();
                return cache_hit;
            }
            set_store_byte(address, data);
            return cache_miss;
        }
        
        byte store_word(word address, word data){
            if (blocked) return blocked_error;
            word tag = address_to_tag(address);
            byte index = address_to_index(address);
            if (cache_memory[index].tag == tag && cache_memory[index].valid){
                status = cache_hit;
                byte offset_word = address_to_offset_word(address);
                write_word(tag, index, offset_word, data);
                block();
                return cache_hit;
            }
            set_store_word(address, data);
            return cache_miss;
        }

        constexpr void write_through(){
            if (status == cache_miss) {
                word address = data_to_address(buffer_miss.tag, 0, 0, 0);
                if (bus.store_line(address, buffer_miss) != blocked_error){
                    status = cache_hit;
                    blocked = false;
                    cooldown = 0;
                    return;
                }
                blocked = true;
                cooldown = 100;
                return;
            }
            for (int i = 0; i < LINE_SIZE; i++){
                if (cache_memory[i].valid && cache_memory[i].update){
                    cache_memory[i].update = 0;
                    word address = data_to_address(cache_memory[i].tag, 0, 0, 0);
                    bus.store_line(address, cache_memory[i]);
                }
            }
            return;
        }

};
#endif