#ifndef PPU_H
#define PPU_H
#include "isadefines.h"

#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>

#define FLIP_VERTICAL 64
#define FLIP_HORIZONTAL 128

// Note: in the orignal nes, the program has to wait for interupts to render
// but since we control the cycles, we can assum the PPU is always available to be over written

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
    bool H = 0;
    halfword x_scroll = 0;
    halfword y_scroll = 0;

    // OAM formata
    // y-cord: byte as pixel value. anything greater than 224 is not rendered. 0 if top, 223 for bottom. >224 is not rendered (offscreen)
    // tile: tile from currently selected tiles (left or right). Only one table can be used per render.
    // attributes: B[0:1] select one of the foreground pallets,  B[6] flip vertically, B[7] flip horizontally
    // x-cord: byte as pixel value. Edges not ignored

    // nametable format: Each byte points to a location in tiles_l (update in future to allow both)

    void setup_sdl(){
        SDL_Init(SDL_INIT_VIDEO);
        char s[] = {'P', 'P', 'U', '_', 'O', 'U', 'T','_',  ppu_counter++, '\0'};
        SDL_CreateWindowAndRenderer(s, WIDTH * SCALE, HEIGHT*SCALE, 0, &window, &renderer);
        SDL_SetRenderScale(renderer, SCALE, SCALE); // 4 times larger than it should be
        SDL_SetWindowResizable(window, true);
    }

    PPU(){
        // will not reset PPU arrays except OAM, this is job of programer
        for (int i = 0; i < 256; i++) OAM[i] = 0;
        setup_sdl();
    }

    void render_point(int bits, byte pallet_[], int x, int y, bool background){
        if(background | bits)
        {
            byte *add = &pallet[3*pallet_[bits]];
            SDL_SetRenderDrawColor(renderer, add[0], add[1], add[2], SDL_ALPHA_OPAQUE);
            SDL_RenderPoint(renderer, x, y);
        }
    }

    void output_tile_sprite(byte chr_rom[], byte pallet[], byte tile_n, int upper_right_x, int upper_right_y, bool horizontal_fliped, bool vertical_flip){
        int base = tile_n * 16;
        for (int y = 0; y < 8; y++)
        {
            int y_out = horizontal_fliped ? upper_right_y + 7 - y : upper_right_y + y;
            byte lower = chr_rom[base + y];
            byte upper = chr_rom[base + y + 8];
            for(int x = 7; x >= 0; x-=1){
                int x_out = vertical_flip ? upper_right_x + x : upper_right_x + (7-x);
                byte u = (upper & (0b1 << x) ?  0b10 : 0b00);
                byte l = (lower & (0b1 << x) ?  0b01 : 0b00);
                // 0 1 2 3
                render_point(u + l, pallet, x_out, y_out, true);   
            }
        }
    }

    void output_tile_background(byte chr_rom[], byte pallet[], byte tile_n, int upper_right_x, int upper_right_y){
        //printf("%02x ", tile_n);
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
                render_point(u + l, pallet, upper_right_x + (7-x), upper_right_y + y, true);
                
                //printf("%X", (((upper >> x) & 1) << 1) + (lower >> x) & 1);
            }
            //printf("\n");
        }
    }

    void output_tiles(){
        int t = 0;
        for(int y = 0; y < HEIGHT; y+=8)
        {
            for (int x = 0; x < WIDTH; x+=8)
            {
                if (t > 960) return ;
                output_tile_background(tiles_l, pallets_b, t, x, y);
                t++;
            }
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

    void output_background(){
        x_scroll %= 512;
        int t = 0;
        int x_mod = x_scroll & 0b111;
        for(int y = 0; y < HEIGHT; y+=8)
        {
            for (int x = 0; x < WIDTH; x+=8)
            {
                if (t > 960) return ;
                nametable_adjuster(t, x, y);
                t++;
            }
            if(x_mod) nametable_edge(t, y);
        }
        
    }

    void output_sprites(){
        for(int i = 0; i < 256; i+=4){
            byte y = OAM[i];
            if (y == 0) continue;
            byte x = OAM[i+3];
            byte tile = OAM[i+1];
            byte pallet = OAM[i+2] & 0b11;
            byte flip_vertically = (OAM[i+2] & FLIP_VERTICAL ? 1 : 0);
            byte flip_horizontally = (OAM[i+2] & FLIP_HORIZONTAL ? 1 : 0);
            output_tile_sprite(tiles_l, &pallets_b[4*pallet], tile, x, y, flip_horizontally, flip_vertically);
            //attributes: B[0:1] select one of the foreground pallets,  B[6] flip vertically, B[7] flip horizontally
        }
    }

    void output(){
        output_background();
        output_sprites();
    }
};

#endif