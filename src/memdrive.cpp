#include <iostream>
#include <string>
#include <string.h>
#include <cctype>
#include <type_traits>

#include "cpu.h"

template <typename T>
class  Simulator{
    public:
    T cache1;
    word buff_index;
    word buff_offset;
    word buff_mask;
    word cycle_count =0 ;

    word getvalue(){
        std::string data;
        std::cin >> data;
        if (data.substr(0, 2) == "0x") return std::stoi(data.substr(2), 0, 16);
        else return std::stoi(data, 0, 10);
    }

    void load(word data){
        if (cache1.buffer_address(data) == blocked_error) std::cout << "[" << data << "] not loaded to buffer : blocked" << std::endl;
        else {
            buff_index = (data >> 2) & 0b111;
            std::cout << "["<< data << "] loading to buffer" << std::endl;
            std::cout << "["<< buff_index << "] index" << std::endl;
        }
    }

    void buff(){
        if (cache1.buffer_status()) {
            memory_line a  = cache1.load_buffer();
            std::cout << "final extracted val " << a.data[buff_index] << std::endl;
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
};

template <typename T>
void cache_mode(T &sim){
    bool flag = true; 
    while (flag)
    {
        std::string instruction;
        word data;
        std::cout << "Cycle: " << sim.cycle_count << std::endl;
        printf(">> ");
        std::cin >> instruction;
        for(std::size_t i=0;i < instruction.length();i++) instruction[i] = tolower(instruction[i]);
        if (instruction == "lda") { 
            data = sim.getvalue(); 
            sim.load(data);
        }
        else if (instruction == "buff") { 
            sim.buff();
        }
        else if ((instruction == "str") | (instruction == "sta")) { 
            data = sim.getvalue();
            sim.store(data);
        }
        else if ((instruction == "stb")) { 
            data = sim.getvalue();
            sim.storebyte(data);
        }
        else if (instruction == "cycle") { 
            data = sim.getvalue();
            sim.cycle(data);
        }
        else if (instruction == "hlt") 
            flag = false;
        std::cin.sync();
        sim.cycle(1);
    }  
}

int main(int argc, char* argv[]){
    if (argc > 1 && 0 == strcmp(argv[1], "cache")){
        printf("Cache mode\n");
        Simulator <Cache_lv1> sim;
        cache_mode(sim);
    }
    else{
        printf("Mem mode\n");
        Simulator <Bus> sim;
        cache_mode(sim);
    }
    return 0;
}