#include <iostream>
#include <fstream>
#include <vector>

#include "isadefines.h"
#include "ppu.h"

byte nes_file[1000000];
long file_size;

class Bus {

};

class CPU {
    public:
    Bus bus;
};

int loadfile(const char filename[], byte array[], size_t len, int offset){
    // Open the file in binary mode
    FILE *file = fopen(filename, "rb");
    
    // Check if the file opened successfully
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Move to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file); // Move back to the beginning of the file

    // Create an array to store the file's bytes
    unsigned char *buffer = (unsigned char *)malloc(size);
    if (buffer == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return 1;
    }

    // Read the file into the buffer
    size_t bytesRead = fread(buffer, 1, size, file);
    if (bytesRead != size) {
        perror("Error reading file");
        free(buffer);
        fclose(file);
        return 1;
    }

    // Successfully read, output the array content (example: print as hex)
    for (size_t i = 0; i < len; i++) {
        array[i] = buffer[i + offset]; // Print each byte in hexadecimal
    }

    // Clean up and close the file
    free(buffer);
    fclose(file);
    return 0;
}

int example_ppu_usage(){
    PPU ppu;
    ppu.x_scroll = 0;
    for (int i = 0; i < 960; i++) ppu.nametable2[i] = 0;
    for (int i = 0; i < 64; i++) ppu.atributestable1[i] = 0;
    for (int i = 0; i < 64; i++) ppu.atributestable2[i] = 0;
    ppu.pallets_b[0] = 0x0F;
    ppu.pallets_b[1] = 0x1C;
    ppu.pallets_b[2] = 0x00;
    ppu.pallets_b[3] = 0x30;

    ppu.pallets_f[0] = 0x0F;
    ppu.pallets_f[1] = 0x1C;
    ppu.pallets_f[2] = 0x00;
    ppu.pallets_f[3] = 0x30;

    ppu.OAM[0] = HEIGHT/2;
    ppu.OAM[1] = 0xF;
    ppu.OAM[2] = FLIP_VERTICAL | FLIP_HORIZONTAL;
    ppu.OAM[3] = WIDTH/2;

    if(loadfile(".\\game_nametable_fixed.bin", ppu.nametable1, 960, 0)) return 1;
    if(loadfile(".\\chr_rom.bin", ppu.tiles_l, 4096, 12288)) return 1;
    SDL_Event event;

    int i = 0;
    bool f = true;
    while (f)
    {
        ppu.x_scroll = i;
        ppu.output();
        SDL_RenderPresent(ppu.renderer);
        if (SDL_PollEvent(&event))
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            f = false;
        case SDL_EVENT_MOUSE_WHEEL:
            i += event.wheel.y;
            if ( i < 0 ) i += 2*WIDTH;
            break;
        default:
            break;
        }
        // to allow for window to close
        //SDL_Delay(100);  
    }
    return 0;
}


int main(int argc, char *argv[]){
    return example_ppu_usage();
}