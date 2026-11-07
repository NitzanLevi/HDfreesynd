#include "fs-engine/gfx/hdterrain.h"
#include <SDL_image.h>
#include <array>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <unordered_map>
namespace {
struct Material { bool tried=false; bool menu=false; bool customReference=false;std::array<uint32_t,256> reference{};SDL_Surface* image=nullptr; };
struct Sample { int64_t tile=-1; short x=0,y=0,w=64,h=48; unsigned char expected=0;bool flipped=false; };
std::unordered_map<uint64_t,Material> materials;
std::vector<Sample> samples;
int width=640,height=400;
bool enabled=true;
int64_t fullBackground=-1;
std::vector<unsigned char> backgroundIndices;
std::array<uint32_t,256> gameReference{},menuReference{};
bool referenceLoaded=false;
void loadReference(){if(referenceLoaded)return;referenceLoaded=true;for(auto entry:{std::make_pair("hd/palette-game.rgba",&gameReference),std::make_pair("hd/palette-menu.rgba",&menuReference)}){FILE* f=std::fopen(entry.first,"rb");if(f){std::fread(entry.second->data(),4,256,f);std::fclose(f);}}}
SDL_Surface* material(uint64_t id,int w=64,int h=48) {
 auto& m=materials[id]; if(m.tried)return m.image; m.tried=true;
 char name[80];
 if(id<256)std::snprintf(name,sizeof(name),"hd/tiles/%03u.png",(unsigned)id);
 else std::snprintf(name,sizeof(name),"hd/sprites/%08x.png",(unsigned)(id-256));
 SDL_Surface* loaded=IMG_Load(name);if(!loaded)return nullptr;
 if(id>=256){char kind[90];std::snprintf(kind,sizeof(kind),"hd/sprites/%08x.kind",(unsigned)(id-256));FILE* f=std::fopen(kind,"rb");if(f){m.menu=std::fgetc(f)=='M';std::fclose(f);}std::snprintf(kind,sizeof(kind),"hd/sprites/%08x.rgba",(unsigned)(id-256));f=std::fopen(kind,"rb");if(f){m.customReference=std::fread(m.reference.data(),4,256,f)==256;std::fclose(f);}}
 m.image=SDL_ConvertSurfaceFormat(loaded,SDL_PIXELFORMAT_ARGB8888,0);
 SDL_FreeSurface(loaded);
 if(m.image && (m.image->w<w*2 || m.image->h<h*2)) {SDL_FreeSurface(m.image);m.image=nullptr;}
 if(m.image){std::printf("HD image %llu: %dx%d, original footprint %dx%d\n",(unsigned long long)id,m.image->w,m.image->h,w,h);std::fflush(stdout);FILE* log=std::fopen("hd-loaded-assets.log","a");if(log){std::fprintf(log,"%llu %dx%d world=%dx%d\n",(unsigned long long)id,m.image->w,m.image->h,w,h);std::fclose(log);}}
 return m.image;
}
}
void hdSetBase(unsigned char*,int w,int h){width=w;height=h;samples.resize(w*h);backgroundIndices.resize(w*h);enabled=std::getenv("SYNDICATE_HD_OFF")==nullptr;}
void hdClear(){std::fill(samples.begin(),samples.end(),Sample{});fullBackground=-1;}
void hdInvalidate(int offset){if(offset>=0 && offset<(int)samples.size())samples[offset].tile=-1;}
void hdToggle(){enabled=!enabled;std::printf("HD terrain: %s\n",enabled?"on":"off");}
void hdStamp(int tile,int x,int y,const unsigned char* original) {
 auto* image=material(tile);
 for(int yy=std::max(y,0);yy<std::min(y+48,height);++yy)
 for(int xx=std::max(x,0);xx<std::min(x+64,width);++xx){
  const auto c=original[(47-(yy-y))*64+(xx-x)];if(c==255)continue;
  auto& s=samples[yy*width+xx];s.tile=image?tile:-1;s.x=xx-x;s.y=yy-y;s.expected=c;s.w=64;s.h=48;s.flipped=false;
 }
}
void hdStampPixels(int x,int y,int w,int h,int stride,const unsigned char* source,bool flipped,bool doubled){
 if(w<=0||h<=0||!source)return;
 if(!stride)stride=w;
 uint32_t hash=2166136261u;
 for(unsigned char c:{(unsigned char)w,(unsigned char)(w>>8),(unsigned char)h,(unsigned char)(h>>8)})hash=(hash^c)*16777619u;
 for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++)hash=(hash^source[yy*stride+xx])*16777619u;
 const int scale=doubled?2:1,tw=w*scale,th=h*scale;
 const uint64_t key=(uint64_t)hash+256;auto* image=material(key,w,h);
 if(x==0&&y==0&&tw==width&&th==height){fullBackground=image?(int64_t)key:-1;if(image)for(int yy=0;yy<height;yy++)for(int xx=0;xx<width;xx++)backgroundIndices[yy*width+xx]=source[(yy/scale)*stride+xx/scale];}
 if(!image)return;
 for(int yy=std::max(y,0);yy<std::min(y+th,height);yy++)for(int xx=std::max(x,0);xx<std::min(x+tw,width);xx++){
  const int sx=(xx-x)/scale,sy=(yy-y)/scale;const unsigned char c=source[sy*stride+(flipped?w-1-sx:sx)];if(c==255)continue;
  auto& s=samples[yy*width+xx];s.tile=key;s.x=xx-x;s.y=yy-y;s.w=tw;s.h=th;s.expected=c;s.flipped=flipped;
 }
}
void hdStampBackgroundRegion(int x,int y,int w,int h,const unsigned char* source,int stride){
 if(stride!=width||!source)return;uint32_t hash=2166136261u;
 for(unsigned char c:{(unsigned char)width,(unsigned char)(width>>8),(unsigned char)height,(unsigned char)(height>>8)})hash=(hash^c)*16777619u;
 for(int i=0;i<width*height;i++)hash=(hash^source[i])*16777619u;
 const uint64_t key=(uint64_t)hash+256;auto* image=material(key,width,height);if(!image)return;
 for(int yy=std::max(y,0);yy<std::min(y+h,height);yy++)for(int xx=std::max(x,0);xx<std::min(x+w,width);xx++){
  auto& s=samples[yy*width+xx];s.tile=key;s.x=xx;s.y=yy;s.w=width;s.h=height;s.expected=source[yy*width+xx];s.flipped=false;
 }
}
namespace {
std::vector<unsigned char> movie2x, movie4x;
int movieFrameNumber=0;
void enlargeEdges(const unsigned char *src, int w, int h, std::vector<unsigned char>& dst) {
    dst.resize(4*w*h);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
        const int i=y*w+x, j=(2*y)*(2*w)+2*x;
        const auto center=src[i], up=src[y?i-w:i], down=src[y+1<h?i+w:i];
        const auto left=src[x?i-1:i], right=src[x+1<w?i+1:i];
        const bool edge=up!=down && left!=right;
        dst[j]=edge && left==up?left:center;
        dst[j+1]=edge && up==right?right:center;
        dst[j+2*w]=edge && left==down?left:center;
        dst[j+2*w+1]=edge && down==right?right:center;
    }
}
}
void hdIntroFrame(const unsigned char* src,int w,int h) {
    if(!src || w*2!=width || h*2!=height)return;
    ++movieFrameNumber;
    enlargeEdges(src,w,h,movie2x);
    enlargeEdges(movie2x.data(),2*w,2*h,movie4x);
    fullBackground=-1;
    for(int y=0;y<height;y++)for(int x=0;x<width;x++) {
        auto& sample=samples[y*width+x];
        sample.tile=-2;
        sample.expected=src[(y/2)*w+x/2];
    }
}
void hdCompose(const unsigned char* pixels,const uint32_t* palette,uint32_t* output){
 loadReference();
 for(int y=0;y<height;y++)for(int x=0;x<width;x++){
  const int i=y*width+x;auto s=samples[i];const uint32_t base=palette[pixels[i]];
  if(enabled && s.tile==-2 && s.expected==pixels[i]){
   for(int dy=0;dy<2;dy++)for(int dx=0;dx<2;dx++){
    int at=(y*2+dy)*width*2+x*2+dx;
    output[at]=palette[movie4x[at]];
   }
   continue;
  }
  if(enabled && (s.tile<0||s.expected!=pixels[i]) && fullBackground>=0 && backgroundIndices[i]==pixels[i]){s.tile=fullBackground;s.x=x;s.y=y;s.w=width;s.h=height;s.expected=pixels[i];s.flipped=false;}
  auto* current=enabled && s.tile>=0 && s.expected==pixels[i]?&materials[s.tile]:nullptr;
  auto* image=current?current->image:nullptr;
  const auto& reference=current && current->customReference?current->reference:(current && current->menu?menuReference:gameReference);
  const uint32_t ref=reference[s.expected];
  for(int dy=0;dy<2;dy++)for(int dx=0;dx<2;dx++){
   uint32_t color=base;
   if(image){
    int sx=(s.x*2+dx)*image->w/(s.w*2),sy=(s.y*2+dy)*image->h/(s.h*2);
    if(s.flipped)sx=image->w-1-sx;
    uint32_t c=*(uint32_t*)((unsigned char*)image->pixels+sy*image->pitch+sx*4);
    if(ref && ref!=base){uint32_t adjusted=c&0xff000000u;for(int shift:{0,8,16})adjusted|=(uint32_t)std::clamp((int)((c>>shift)&255)+(int)((base>>shift)&255)-(int)((ref>>shift)&255),0,255)<<shift;c=adjusted;}
    const int a=c>>24;
    if(a==255)color=c;
    else if(a){color=0xff000000;for(int shift:{0,8,16})color|=((((c>>shift)&255)*a+((base>>shift)&255)*(255-a))/255)<<shift;}
   }
   output[(y*2+dy)*(width*2)+x*2+dx]=color;
  }
 }
 static int lastProof=0;
 if(std::getenv("SYNDICATE_INTRO_PROOF") && movieFrameNumber!=lastProof &&
    (movieFrameNumber==300 || movieFrameNumber==997)){
    SDL_Surface* frame=SDL_CreateRGBSurfaceFrom(output,width*2,height*2,32,width*8,0xff0000,0xff00,0xff,0xff000000);
    if(frame){char filename[64];std::snprintf(filename,sizeof(filename),"intro-frame-%d.bmp",movieFrameNumber);SDL_SaveBMP(frame,filename);SDL_FreeSurface(frame);}
    lastProof=movieFrameNumber;
 }

}
