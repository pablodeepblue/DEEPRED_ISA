#include <iostream>
#include <string>
#include <cctype>

#include "cpu.h"

Cache_lv1 cache1;
word buff_index;
word buff_offset;
word buff_mask;
word cycle_count;

word getvalue(){
    std::string data;
    std::cin >> data;
    if (data.substr(0, 2) == "0x") return std::stoi(data.substr(2), 0, 16);
    else return std::stoi(data, 0, 10);
}

// change to cache
void load(word data){
    if (cache1.buffer_address(data) == blocked_error) std::cout << "[" << data << "] not loaded to buffer : blocked" << std::endl;
    else {
        buff_index = (data >> 2) & 0b111;
        buff_offset = 0;
        buff_mask = 0xFFFFFFFF;
        std::cout << "["<< data << "] loading to buffer" << std::endl;
    }
}

// change to cache
void loadbyte(word data){
    if (cache1.buffer_address(data) == blocked_error) std::cout << "[" << data << "] not loaded to buffer : blocked" << std::endl;
    else {
        buff_index = (data >> 2) & 0b111;
        buff_offset = (3 - (data & 0b11)) << 3;
        buff_mask = 0xFF;
        std::cout << "["<< data << "] loading to buffer" << std::endl;
    }
}

// change to cache
void buff(){
    if (cache1.buffer_status()) {
        cache_line a  = cache1.load_buffer();
        std::cout << "buffered index " << buff_index << std::endl;
        std::cout << "buffered offset " << buff_offset << std::endl;
        std::cout << "final extracted val " << ((a.data >> buff_offset) & buff_mask) << std::endl;
    }
    else std::cout << "Buffer not loaded yet" << std::endl;
}

void store(word data){
    word add = getvalue();
    if (cache1.store_word(add, data) == blocked_error) std::cout << data << " store to [" << add << "] blocked" << std::endl;
    else std::cout << data << " store to [" << add << "] buffered" << std::endl;
}

void storebyte(word data){
    word add = getvalue();
    if (cache1.store_byte(add, data) == blocked_error) std::cout << data << " store to [" << add << "] blocked" << std::endl;
    else std::cout << data << " store to [" << add << "] buffered" << std::endl;
}

void cycle(word data){
    cycle_count += data;
    for (word i = 0; i < data; i++){
        cache1.cycle();
    }
        
}

int main(){
    bool flag = true;
    while (flag)
    {
        std::string instruction;
        word data;
        std::cout << "\nCycle: " << cycle_count << std::endl;
        printf(">> ");
        std::cin >> instruction;
        for(std::size_t i=0;i < instruction.length();i++) instruction[i] = tolower(instruction[i]);
        if (instruction == "lda") { 
            data = getvalue(); 
            load(data);
        }
        else if (instruction == "ldb") { 
            data = getvalue(); 
            loadbyte(data);
        }
        else if (instruction == "buff") { 
            buff();
        }
        else if ((instruction == "str") | (instruction == "sta")) { 
            data = getvalue();
            store(data);
        }
        else if ((instruction == "stb")) { 
            data = getvalue();
            storebyte(data);
        }
        else if (instruction == "cycle") { 
            data = getvalue();
            cycle(data);
        }
        else if (instruction == "hlt") 
            flag = false;
        std::cin.sync();
        cycle(1);
    }  
}