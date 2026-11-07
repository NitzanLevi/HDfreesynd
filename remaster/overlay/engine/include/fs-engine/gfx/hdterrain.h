#pragma once
#include <cstdint>
void hdSetBase(unsigned char* pixels, int width, int height);
void hdClear();
void hdInvalidate(int offset);
void hdStamp(int tile, int x, int y, const unsigned char* originalBottomUp);
void hdStampPixels(int x,int y,int w,int h,int stride,const unsigned char* source,bool flipped=false,bool doubled=false);
void hdStampBackgroundRegion(int x,int y,int w,int h,const unsigned char* source,int stride);
void hdCompose(const unsigned char* pixels, const uint32_t* palette, uint32_t* output);
void hdToggle();
