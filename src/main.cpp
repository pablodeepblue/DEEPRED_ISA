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

    // xmin 96 xmax 
    //60 61  
    //62 63
    ppu.output_tiles();

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

//20 x 10

uint32_t r[5];

void ppu_tetris_update_square(PPU &ppu, int &x, int &y){
    if(x < 0) x = 0;
    if(x > 64) x = 64;
    if(y < 0) y = 0;
    if(y > 144) y = 144;
    ppu.OAM[0] = 48 + y;
    ppu.OAM[1] = 0x7B;
    ppu.OAM[2] = 0;
    ppu.OAM[3] = (96 + x);

    ppu.OAM[4] = 48 + y;
    ppu.OAM[5] = 0x7B;
    ppu.OAM[6] = 0;
    ppu.OAM[7] = (104 + x);

    ppu.OAM[8] = 48 + 8 + y;
    ppu.OAM[9] = 0x7B;
    ppu.OAM[10] = 0;
    ppu.OAM[11] = (96 + x);

    ppu.OAM[12] = 48 + 8 + y;
    ppu.OAM[13] = 0x7B;
    ppu.OAM[14] = 0;
    ppu.OAM[15] = (104 + x);
}
    

void ppu_tetris_update_background(PPU &ppu, int sprite_num){
    for (int i = 0; i < sprite_num*4; i+=4){
        int y = ppu.OAM[i] / 8;
        int tile = ppu.OAM[i+1];
        int x = ppu.OAM[i+3] / 8;
        ppu.nametable1[y*32 + x] = tile;
    } 
}

int ppu_tetris(){
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

    for (int i = 0; i < 5; i++) r[i] = 0;


    // tetris box x_min 96 x_max 168
    //            y_min
    // 0x7D white block
    // 0x7C grey block
    // 0x7B white-gray
    // tetris space: 10 tiles wide 20 tiles high
    int x = WIDTH/2;
    int y = 0;

    ppu_tetris_update_square(ppu, x, y);

    if(loadfile(".\\game_nametable_fixed.bin", ppu.nametable1, 960, 0)) return 1;
    if(loadfile(".\\chr_rom.bin", ppu.tiles_l, 4096, 12288)) return 1;
    SDL_Event event;

    bool f = true;
    uint32_t ticks = SDL_GetTicks();
    uint32_t cticks = ticks;
    
    while (f)
    {
        printf("%u\n", x);
        cticks = SDL_GetTicks();
        if (cticks - ticks> 1000){
            //y += 8;
            //ticks = cticks;
            //ppu_tetris_update_square(ppu, x, y);
        }
        if (y == 144) 
        {
            ppu_tetris_update_background(ppu, 4);
            y = 0;
        }
        ppu.output();
        SDL_RenderPresent(ppu.renderer);
        if (SDL_PollEvent(&event))
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            f = false;
        case SDL_EVENT_KEY_DOWN:
            switch (event.key.key) {
                case SDLK_RIGHT:
                    x += 8;
                    ppu_tetris_update_square(ppu, x, y);
                    break;
                case SDLK_LEFT:
                    x -= 8;
                    ppu_tetris_update_square(ppu, x, y);
                    break;
                case SDLK_DOWN:
                    y += 8;
                    ppu_tetris_update_square(ppu, x, y);
                    break;
            }
        default:
            break;
        }
        // to allow for window to close
        //SDL_Delay(100);  
    }
    return 0;
}


int main(int argc, char *argv[]){
    return ppu_tetris();
}