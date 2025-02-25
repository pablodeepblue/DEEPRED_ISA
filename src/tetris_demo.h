#ifndef TETRIS_DEMO_H
#define TETRIS_DEMO_H
#include "isadefines.h"
#include "ppu.h"
#include <stdlib.h>
#include <stdio.h>
#include <SDL3/SDL.h>

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
    ppu.pallets_b[2] = 0x2A;
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

int valid(int16_t tetrimino[], byte r[], byte &block, byte &x, byte &y){
    for (int i = 0; i < 4; i ++){
        byte y_offset = y + tetrimino[block*12 + i*3];
        byte x_offset = x + tetrimino[block*12 + i*3 + 2];
        if (x_offset > 13) {
            return 0;
        }
        if (x_offset > 9) {
            return 0;
        }
        byte x_index = x_offset>>3;
        //printf("%u %u %u\n", x, y_offset, r[x_offset]);
        if (y_offset == r[x_offset]) {
            return 0;
        }
        if (y_offset > r[x_offset]) {
            return 0;
        }
    }
    return 1;
}

void ppu_tetris_update_square(PPU &ppu, int16_t tetrimino[], byte r[], byte &block, byte &x, byte &y, byte &l){
    for (int i = 0; i < 4; i ++){
        //printf("%02x\n", x);
        //y offset, tile ID, x offset per mino per orientatio

        byte y_offset = y + tetrimino[block*12 + i*3];
        byte x_offset = x + tetrimino[block*12 + i*3 + 2];
        if (x_offset > 13) {
            x += 1;
            i = 0;
            continue;
        }
        if (x_offset > 9) {
            x -= 1;
            i = 0;
            continue;
        }
        byte x_index = x_offset>>3;
        //printf("%u %u %u\n", x, y_offset, r[x_offset]);
        if (y_offset == r[x_offset]) {
            y -= 1;
            i = 0;
            l = 1;
            continue;
        }
        if (y_offset > r[x_offset]) {
            l = 1;
            continue;
        }
        byte tile_ID =  tetrimino[block*12 + i*3 + 1];
        ppu.OAM[i*4 + 0] = 0x30 + ( ( y_offset) << 3);
        ppu.OAM[i*4 + 1] = tile_ID;
        ppu.OAM[i*4 + 2] = 0;
        ppu.OAM[i*4 + 3] = 0x60 + ( (x_offset) << 3);

    }
}
    

void ppu_tetris_update_background(PPU &ppu, int16_t tetrimino[],  byte r[], byte &block, byte x, byte y){
    for (int i = 0; i < 4; i++){

        int _y = y + tetrimino[block*12 + i*3];
        int _tile = ppu.OAM[i*4+1];
        int _x = x + tetrimino[block*12 + i*3 + 2];
        if (_y < r[_x]) r[_x] = _y;
        ppu.nametable1[(_y + 6)*32 + _x + 12] = _tile;
    } 
}

int ppu_tetris(){
    PPU ppu;
    ppu.x_scroll = 0;
    for (int i = 0; i < 960; i++) ppu.nametable2[i] = 0;
    for (int i = 0; i <  64; i++) ppu.atributestable1[i] = 0;
    for (int i = 0; i <  64; i++) ppu.atributestable2[i] = 0;
    ppu.pallets_b[0] = 0x0F;
    ppu.pallets_b[1] = 0x1C;
    ppu.pallets_b[2] = 0x00;
    ppu.pallets_b[3] = 0x30;

    ppu.pallets_f[0] = 0x0F;
    ppu.pallets_f[1] = 0x1C;
    ppu.pallets_f[2] = 0x00;
    ppu.pallets_f[3] = 0x30;

    // tetris box x_min 96 x_max 168
    //            y_min
    // 0x7D white block
    // 0x7C grey block
    // 0x7B white-gray
    // tetris space: 10 tiles wide 20 tiles high
    // for blocks
    // mark highest block

    byte tile1 = 0x7B;
    byte tile2 = 0x7C;
    byte tile3 = 0x7D;
    byte tileHidden = 0xFF;

    

    //byte rows[10];

    byte tUp = 0x00;
    byte tRight = 0x01;
    byte tDown = 0x02;
    byte tLeft = 0x03;

    byte jLeft  = 0x04;
    byte jUp  = 0x05;
    byte jRight = 0x06;
    byte jDown = 0x07;

    byte zHori = 0x08;
    byte zVert = 0x08;

    byte oFixed = 0x0A;

    byte sHoriz = 0x0B;
    byte sVert = 0x0C;

    byte lRight = 0x0D;
    byte lDown= 0x0E;
    byte lLeft = 0x0F;
    byte lUp = 0x10;
    
    byte iVert = 0x11;
    byte iHoriz = 0x12;

    byte tHidden = 0x13;

    int16_t tetrimino[] = {
        0, tile1,-1, 0, tile1, 0, 0, tile1, 1,-1, tile1, 0,
        -1, tile1, 0, 0, tile1, 0, 0, tile1, 1, 1, tile1, 0,
         0, tile1,-1, 0, tile1, 0, 0, tile1, 1, 1, tile1, 0,
        -1, tile1, 0, 0, tile1,-1, 0, tile1, 0, 1, tile1, 0,
        -1, tile3, 0, 0, tile3, 0, 1, tile3,-1, 1, tile3, 0,
        -1, tile3,-1, 0, tile3,-1, 0, tile3, 0, 0, tile3, 1,
        -1, tile3, 0,-1, tile3, 1, 0, tile3, 0, 1, tile3, 0,
         0, tile3,-1, 0, tile3, 0, 0, tile3, 1, 1, tile3, 1,
         0, tile2,-1, 0, tile2, 0, 1, tile2, 0, 1, tile2, 1,
        -1, tile2, 1, 0, tile2, 0, 0, tile2, 1, 1, tile2, 0,
         0, tile1,-1, 0, tile1, 0, 1, tile1,-1, 1, tile1, 0,
         0, tile3, 0, 0, tile3, 1, 1, tile3,-1, 1, tile3, 0,
        -1, tile3, 0, 0, tile3, 0, 0, tile3, 1, 1, tile3, 1,
        -1, tile2, 0, 0, tile2, 0, 1, tile2, 0, 1, tile2, 1,
         0, tile2,-1, 0, tile2, 0, 0, tile2, 1, 1, tile2,-1,
        -1, tile2,-1,-1, tile2, 0, 0, tile2, 0, 1, tile2, 0,
        -1, tile2, 1, 0, tile2,-1, 0, tile2, 0, 0, tile2, 1,
        -2, tile1, 0,-1, tile1, 0, 0, tile1, 0, 1, tile1, 0,
         0, tile1,-2, 0, tile1,-1, 0, tile1, 0, 0, tile1, 1,
         0, tileHidden, 0, 0, tileHidden, 0, 0, tileHidden, 0, 0, tileHidden, 0
    };

    byte rotate_table[] = {
        tLeft, tRight,
        tUp, tDown,
        tRight, tLeft,
        tDown, tUp,
        jDown, jUp,  
        jLeft, jRight,
        jUp, jDown,
        jRight, jLeft,
        zVert, zVert,
        zHori, zHori,
        oFixed, oFixed,
        sVert, sVert,
        sHoriz, sHoriz,
        lUp, lDown,   
        lRight, lLeft,
        lDown, lUp,   
        lLeft, lRight,
        iHoriz, iHoriz,
        iVert, iVert, 
    };

    byte valid_start[] = {
        tDown,
        jDown,
        iHoriz,
        oFixed,
        sHoriz,
        lDown,
        iHoriz,
    };

    byte r[10] = {20, 20, 20, 20, 20, 20, 20, 20, 20, 20,};
    byte block = tDown;
    byte x = 5;
    byte y = 0;
    byte l = 0;

    ppu_tetris_update_square(ppu, tetrimino, r, block, x, y, l);

    if(loadfile(".\\game_nametable_fixed.bin", ppu.nametable1, 960, 0)) return 1;
    if(loadfile(".\\chr_rom.bin", ppu.tiles_l, 4096, 12288)) return 1;
    SDL_Event event;

    bool f = true;
    uint32_t ticks = SDL_GetTicks();
    uint32_t cticks = ticks;
    byte t;

    while (f)
    {
        if (block == tHidden){
            if (SDL_PollEvent(&event))
            if (event.type == SDL_EVENT_QUIT) f = false;
            l = 0;
            x = 5;
            y = 0;
            //block = valid_start[SDL_rand(7)];
            continue;
        }
        if (SDL_PollEvent(&event))
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            f = false;
            break;
        case SDL_EVENT_KEY_DOWN:
            switch (event.key.key)
            {
            case SDLK_DOWN:
                y += 1;
                break;
            case SDLK_LEFT:
                x -= 1;
                break;
            case SDLK_RIGHT:
                x += 1;
                break;
            case SDLK_X:
                t = rotate_table[(block<<1) ];
                if (valid(tetrimino, r, t, x, y)) block = t;
                break;
            case SDLK_C:
                t = rotate_table[(block<<1) + 1];
                if (valid(tetrimino, r, t, x, y)) block = t;
                break;
            default:
                break;
            }
        default:
            break;
        }
        cticks = SDL_GetTicks();
        if (cticks - ticks >= 1000){
            y += 1;
            ticks = cticks;
        }
        ppu_tetris_update_square(ppu, tetrimino, r, block, x, y, l);
        if (l){
            ppu_tetris_update_background(ppu, tetrimino, r,block, x, y);
            block = tHidden;
        }
        
        ppu.output();
        SDL_RenderPresent(ppu.renderer);
        // to allow for window to close
        //SDL_Delay(100);  
    }
    return 0;
}

#endif