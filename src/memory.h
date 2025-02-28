#ifndef MEMORY_H
#define MEMORY_H
#include "isadefines.h"

#define LINE_SIZE 8

#define blocked_error    0xff

typedef struct memory_line
{
    byte status = blocked_error;
    word data[LINE_SIZE];
} memory_line;

class Memory{
    private:
        byte blocked = false;
        byte cooldown = 0;
        memory_line memory_data[2000]; // 2000 * 8 * 4 = 65kb
        memory_line buffer;
        memory_line empty;

        void block(){
            blocked = true;
            cooldown = 100;
        }

    public:
        Memory(){}

        void cycle(){
            printf("[Memory] Cycle\n");
            cooldown -= 1;
            if (cooldown == 0) blocked = false;
        }

        // load instructions
        byte buffer_address(word line){
            if (blocked) return blocked_error;
            printf("[Memory] Loading line %u\n to buffer\n", line);
            buffer.status = 0;
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
            printf("[Memory] storing byte line %u index %u offset %u data %u\n", line, index, offset, data);
            byte byte_pos = (3 - offset) << 3;
            word mask = 0xFF << byte_pos;
            memory_data[line].data[index] &= ~mask;
            memory_data[line].data[index] |= data << byte_pos;
            block();
            return 0;
        }
        
        byte store_word(word line, word index, word data){
            if (blocked) return blocked_error;
            printf("[Memory] storing word line %u index %u data %u\n", line, index, data);
            memory_data[line].data[index] = data;
            block();
            return 0;
        }
};

class Bus {
    private:
        Memory ram;

        word address_to_line(word address){
            return address >> 5;
        }
        
        word address_to_index(word address){
            return (address >> 2) & 0b111;
        }

        word address_to_offset(word address){
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

};


#define cache_wait      0b1000
#define cache_fetch     0b0100
#define cache_miss      0b0010
#define cache_hit       0b0001

typedef struct cache_line
{
    byte status = cache_hit;
    word data;
} cache_line;

#define valid_bit       ((1 << 31))
#define update_bit      ((1 << 30)) 
#define tag_bit_mask    ((1 << 22) - 1)
// move bus here  
class Cache_lv1{
    private:
        byte blocked = false;
        byte cooldown = 0;
        byte replace = 0;
        word cache_memory[64];  // 16 blocks * 4 words
        word cache_tags[64];    // 31: Valid bit | 30: Update bit
        word buffer_addr_load = 0;
        word buffer_addr_store = 0;
        
        bool byte_mode;
        word byte_offset;
        byte byte_data;
        cache_line buffer;
        Bus bus;

        void block(){
            blocked = true;
            cooldown = 1;
        }

        void unblock(){
            blocked = false;
            cooldown = 0;
        }

        void write_word(byte index, word tag, word data){
            cache_memory[index + replace] = data;
            cache_tags[index + replace] = tag | valid_bit | update_bit;
            replace = (replace + 1) % 4;
        }

        void write_byte(byte index, byte offset, word tag, byte data){
            byte byte_pos = (3 - offset) << 3;
            word mask = 0xFF << byte_pos;
            cache_memory[index + replace] &= ~mask;
            cache_memory[index + replace] |= data << byte_pos;
            cache_tags[index + replace] = tag | valid_bit | update_bit;
            replace = (replace + 1) % 4;
        }

        word address_to_tag(word address){
            return address >> 6;
        }

        byte address_to_index(word address){
            return ((address >> 2) & 0b1111) << 2;
        }

        byte address_to_offset(word address){
            return address & 0b11;
        }

        word data_to_address(word tag, byte index, byte offset){
            return (tag << 6) | index | offset;
        }

    public:
        Cache_lv1() {
            for (int i = 0; i < 32; i++)
                cache_tags[i] = 0;
        };

        void cycle(){
            printf("[Cache] Cycling\n");
            cooldown -= 1;
            if (cooldown == 0) blocked = false;
            write_through();
            if (buffer.status == cache_fetch && bus.buffer_address(buffer_addr_load) != blocked_error){
                buffer.status = cache_wait;
                cooldown = 255;
            }
            else if (buffer.status == cache_wait && bus.buffer_status()) {
                word tag = address_to_tag(buffer_addr_load);
                byte index = address_to_index(buffer_addr_load);
                word data = bus.load_buffer().data[(buffer_addr_load >> 2) & 0b111];
                if (byte_mode){
                    byte byte_pos = (3 - byte_offset) << 3;
                    word mask = 0xFF << byte_pos;
                    data &= ~mask;
                    data |= byte_data << byte_pos;
                    byte_mode = false;
                } else {  
                    buffer.data = data;
                }
                buffer.status = cache_hit;
                write_word(index, tag, data);
                unblock();
            }
            bus.cycle();
        }

        byte check_for_tag(word address){
            word tag = address_to_tag(address);
            byte index = address_to_index(address);
            for (int i = 0; i < 4; i++) 
                if ((cache_tags[index + i] & tag_bit_mask) == tag ) return cache_hit;
            return cache_miss;
        }

        // load instructions
        byte buffer_address(word address){
            if (blocked) return blocked_error;
            printf("[Cache] Buffering 0x%X\n", address);
            word tag = address_to_tag(address);
            byte index = address_to_index(address);
            for (int i = 0; i < 4; i++) 
                if ((cache_tags[index + i] & tag_bit_mask) == tag ){
                    buffer.status = 0;
                    buffer.data = cache_memory[index + i];
                    block();
                    return cache_hit;
                }
            buffer_addr_load = address;
            buffer.status = cache_fetch;
            cooldown = 255;
            blocked = true;
            return cache_miss;
        }

        byte buffer_status(){
            return (!blocked);
        }

        cache_line load_buffer(){
            if (blocked) return cache_line{};
            else return buffer;
        }

        // store
        byte store_word(word address, word data){
            if (blocked) return blocked_error;
            printf("[Cache] Storing %u (word) at 0x%X\n", data, (address >> 2) << 2);
            word tag = address_to_tag(address);
            byte index = address_to_index(address);
            for(int i = 0; i < 4; i++)
                if ( ( !( cache_tags[index + i] & valid_bit ) ) || ( ( cache_tags[index + i] & tag_bit_mask ) == tag ) ){
                    cache_tags[index + i] = tag | valid_bit | update_bit;
                    cache_memory[index + i] = data;
                    return cache_hit;
                }
            if ((cache_tags[index + replace] & valid_bit) && (cache_tags[index + replace] & update_bit)){
                buffer_addr_store = data_to_address(cache_tags[index + replace], index, 0);
                buffer.status = cache_miss;
                buffer.data = cache_memory[index + replace];
                blocked = true;
                cooldown = 255;
            }
            else{
                block();
            }
            write_word(index + replace, tag, data);
            return cache_miss;
        }

        byte store_byte(word address, byte data){
            if (blocked) return blocked_error;
            printf("[Cache] Storing %u (byte) at 0x%X\n", data, address);
            if (check_for_tag(address) == cache_miss){
                byte_offset = address_to_offset(address);
                byte_data = data;
                byte_mode = true;
                return buffer_address(address);
            }
            else{
                word index = address_to_index(address);
                word tag = address_to_tag(address);
                for(int i = 0; i < 4; i++) if ((cache_tags[index + i] & tag_bit_mask) == tag ){
                    byte byte_pos = (3 - address_to_offset(address)) << 3;
                    word mask = 0xFF << byte_pos;
                    cache_memory[index + i] &= ~mask;
                    cache_memory[index + i] |= data << byte_pos;
                    cache_tags[index + i] = tag | valid_bit | update_bit;
                }
                return cache_hit;
            }
        }

        void write_through(){
            printf("[Cache] Attempting writethrough\n");
            if (buffer.status == cache_miss) {
                if (bus.store_word(buffer_addr_store, buffer.data) != blocked_error){
                    buffer.status = cache_hit;
                    blocked = false;
                    cooldown = 0;
                    return;
                }
                blocked = true;
                cooldown = 100;
            }
            else for (int i = 0; i < 32; i++)
                if((cache_tags[i] & valid_bit) && (cache_tags[i] & update_bit)){
                    word addr = data_to_address(cache_tags[i], i, 0);
                    if (bus.store_word(addr, cache_memory[i]) != blocked_error) cache_tags[i] ^= update_bit;
                    return;
                }
            return;
        }

};
#endif