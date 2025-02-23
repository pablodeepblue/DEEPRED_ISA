#include "cpudefines.h"

#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <vector>

byte nes_file[1000000];
long file_size;

const int WIDTH = 256;
const int HEIGHT = 224;
const int SCALE = 4;

byte pallet[] = {
    124, 124, 124, 0, 0, 252, 0, 0, 188, 68, 40, 188, 148, 0, 132, 168, 0, 32, 168, 16, 0, 136, 20, 0, 80, 48, 0, 0, 120, 0, 0, 104, 0, 0, 88, 0, 0, 64, 88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 188, 188, 188, 0, 120, 248, 0, 88, 248, 104, 68, 252, 216, 0, 204, 228, 0, 88, 248, 56, 0, 228, 92, 16, 172, 124, 0, 0, 184, 0, 0, 168, 0, 0, 168, 68, 0, 136, 136, 0, 0, 0, 0, 0, 0, 0, 0, 0, 248, 248, 248, 60, 188, 252, 104, 136, 252, 152, 120, 248, 248, 120, 248, 248, 88, 152, 248, 120, 88, 252, 160, 68, 248, 184, 0, 184, 248, 24, 88, 216, 84, 88, 248, 152, 0, 232, 216, 120, 120, 120, 0, 0, 0, 0, 0, 0, 252, 252, 252, 164, 228, 252, 184, 184, 248, 216, 184, 248, 248, 184, 248, 248, 164, 192, 240, 208, 176, 252, 224, 168, 248, 216, 120, 216, 248, 120, 184, 248, 184, 184, 248, 216, 0, 252, 252, 248, 216, 248, 0, 0, 0, 0, 0, 0
};


char ppu_counter = '0';

class PPU {
    public:
    byte OAM[256]; // 64 sprites, 4 bytes each (y-position tile attributes x-position)
    byte pallets_b[16]; // 4 background (nametable) pallets 4 bytes each (color 1, color 2, color 3, color 4)
    byte pallets_f[16]; // 4 foreground (OAM) pallets 4 bytes each (color 1, color 2, color 3, color 4)
    byte nametable1[960]; // 32 x 30 tile lables, 1 bytes each
    byte atributestable1[64]; // pallets for each tile, 2 bits each
    byte nametable2[960]; // 32 x 30 tile lables, 1 bytes each
    byte atributestable2[64]; // pallets for each tile, 2 bits each
    byte tiles_l[4096]; // 16x16 8x8 tiles, 16 bytes each
    byte tiles_r[4096]; // 16x16 8x8 tiles, 16 bytes each
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool H = 1;
    halfword x_scroll = 0;
    halfword y_scroll = 0;

    // OAM formata
    // y-cord: byte as pixel value. anything greater than 224 is not rendered. 0 if top, 223 for bottom. >224 is not rendered (offscreen)
    // tile: tile from currently selected tiles (left or right). Only one table can be used per render.
    // attributes: B[0:1] select one of the foreground pallets,  B[6] flip vertically, B[7] flip horizontally



    void setup_sdl(){
        SDL_Init(SDL_INIT_VIDEO);
        char s[] = {'P', 'P', 'U', '_', 'O', 'U', 'T','_',  ppu_counter++, '\0'};
        SDL_CreateWindowAndRenderer(s, WIDTH * SCALE, HEIGHT*SCALE, 0, &window, &renderer);
        SDL_SetRenderScale(renderer, SCALE, SCALE); // 4 times larger than it should be
        SDL_SetWindowResizable(window, true);
    }

    PPU(){
        // will not reset PPU arrays except OAM, this is job of coder
        for (int i = 0; i < 256; i++) OAM[i] = 0;
        setup_sdl();
    }

    void set_mask_color(int bits, byte pallet_[]){
        byte *add = &pallet[3*pallet_[bits]];
        SDL_SetRenderDrawColor(renderer, add[0], add[1], add[2], SDL_ALPHA_OPAQUE);
    }

    void output_tile_background(byte chr_rom[], byte pallet[], int tile_n, int upper_right_x, int upper_right_y){
        int base = tile_n * 16;
        for (int y = 0; y < 8; y++)
        {
            if (upper_right_y + y > HEIGHT) continue;
            byte lower = chr_rom[base + y];
            byte upper = chr_rom[base + y + 8];
            for(int x = 7; x >= 0; x-=1){
                if (upper_right_x + (7-x) > WIDTH) continue;
                byte u = (upper & (0b1 << x) ?  0b10 : 0b00);
                byte l = (lower & (0b1 << x) ?  0b01 : 0b00);
                set_mask_color(u + l, pallet);
                SDL_RenderPoint(renderer, upper_right_x + (7-x), upper_right_y + y);
                //printf("%X", (((upper >> x) & 1) << 1) + (lower >> x) & 1);
            }
            //printf("\n");
        }
    }

    void output_name_table_1(){
        int t = 0;
        for(int y = 0; y < HEIGHT; y+=8)
        {
            for (int x = 0; x < WIDTH; x+=8)
            {
                if (t > 960) return ;
                output_tile_background(tiles_l, 0, nametable1[t],x, y);
                printf("%02x ", nametable1[t]);
                t++;
            }
            printf("\n");
        }
        
    }

    byte * table_to_pallet(byte atributestable[], int tableidx){
        int x = tableidx & 0b11111;
        int y = tableidx >> 5;
        int index = (x / 4) + (y / 128)*8;
        int bits = ( atributestable[index] >> ((x & 0b10) + 4 - (y & 0b10) * 2) ) & 0b11;
        return &pallets_b[4*bits];
    }


    void nametable_adjuster(int t, int x, int y){
        int x_mod = x_scroll & 0b111;
        int tile_ = (x_scroll >> 3);
        int offset_x_ = (t & 0b11111) + tile_;
        int offset_y_ = (t >> 5) << 5;
        if (offset_x_ >= 64) output_tile_background(tiles_l, table_to_pallet(atributestable1, offset_y_ + offset_x_ - 64), nametable1[offset_y_ + offset_x_ - 64], x - x_mod, y);
        else if (offset_x_ >= 32) output_tile_background(tiles_l, table_to_pallet(atributestable2, offset_y_ + offset_x_ - 32), nametable2[offset_y_ + offset_x_ - 32], x - x_mod, y);
        else output_tile_background(tiles_l, table_to_pallet(atributestable2,offset_y_ + offset_x_), nametable1[offset_y_ + offset_x_], x - x_mod, y);
        
    }

    void nametable_edge(int t, int y){
        int x_mod = x_scroll & 0b111;
        int tile_ = (x_scroll >> 3) + 1;
        int offset_x_ = 0b11111 + tile_;
        int offset_y_ = (t >> 5) << 5;
        if (offset_x_ >= 64) output_tile_background(tiles_l, table_to_pallet(atributestable1, offset_y_ + offset_x_ - 96), nametable1[offset_y_ + offset_x_ - 96], WIDTH - x_mod, y);
        else if (offset_x_ >= 32) output_tile_background(tiles_l, table_to_pallet(atributestable2, offset_y_ + offset_x_ - 64), nametable2[offset_y_ + offset_x_ - 64], WIDTH - x_mod, y);
        else output_tile_background(tiles_l, table_to_pallet(atributestable1, offset_y_ + offset_x_ - 32), nametable1[offset_y_ + offset_x_ - 32], WIDTH - x_mod, y);
    }

    void output(){
        x_scroll %= 512;
        int t = 0;
        int x_mod = x_scroll & 0b111;
        for(int y = 0; y < HEIGHT; y+=8)
        {
            for (int x = 0; x < WIDTH; x+=8)
            {
                if (t > 960) return ;
                nametable_adjuster(t, x, y);
                //printf("%02x ", nametable_adjuster(t));
                t++;
            }
            if(x_mod) nametable_edge(t, y);
            //printf("\n");
        }
        
    }

};

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
    if(loadfile(".\\game_nametable_fixed.bin", ppu.nametable1, 960, 0)) return 1;
    if(loadfile(".\\chr_rom.bin", ppu.tiles_l, 4096, 12288)) return 1;
    SDL_Event event;

    for (int i = 0; i < 4 * 64 * 8; i++)
    {
        ppu.x_scroll = i;
        ppu.output();
        SDL_RenderPresent(ppu.renderer);
        if (SDL_PollEvent(&event)) if (event.type == SDL_EVENT_QUIT) break; // to allow for window to close
        //SDL_Delay(100);  
    }
    return 0;
}


int main(int argc, char *argv[]){
    return example_ppu_usage();
}