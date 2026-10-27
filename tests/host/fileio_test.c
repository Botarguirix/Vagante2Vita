/* Thin import-table fixture; opens real stdio streams and spies on fflush. */
#include "win32.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <io.h>
uint8_t *guest_base;
uint32_t last_error;
extern const ApiEntry win32_fileio[];
static FILE *files[4096],*read_cache;
static char stdio_buffer[8192];
static int writing[4096],cache_busy,flushes,fail_flush,null_flushes;
int __real_fflush(FILE *);
int __wrap_fflush(FILE *stream) {
    /* The CRT itself calls fflush(NULL) at process exit; count it separately. */
    if(!stream) {++null_flushes;return __real_fflush(stream);}
    ++flushes;return fail_flush?EOF:__real_fflush(stream);
}
void *handle_get(uint32_t h,int type) {
    if(type!=H_FILE || h<0x1000 || ((h-0x1000)&3)) return NULL;
    unsigned slot=(h-0x1000)/4; return slot<4096?files[slot]:NULL;
}
static void open_original(Cpu *c,int wide) {
    char path[256]; uint32_t src=ARG(c,0),access=ARG(c,1),disposition=ARG(c,4);
    assert(ARG(c,2)==0x33 && ARG(c,5)==0x80); /* arguments survive delegation */
    if(wide) { unsigned i=0;for(;rd16(src+2*i);i++){assert(i<255);path[i]=(char)rd16(src+2*i);}path[i]=0; }
    else { assert(strlen(STR(src))<sizeof path);strcpy(path,STR(src)); }
    int write=(access&0x40000000u)!=0;FILE *f;
    if(!write && read_cache && !cache_busy) { f=read_cache;rewind(f);cache_busy=1; }
    else {
        f=fopen(path,write?(disposition==3?"r+b":"w+b"):"rb");
        if(f) setvbuf(f,stdio_buffer,_IOFBF,sizeof stdio_buffer);
        if(f&&!write) {read_cache=f;cache_busy=1;}
    }
    uint32_t h=UINT32_MAX;
    if(f) for(unsigned i=1;i<4096;i++) if(!files[i]) {files[i]=f;writing[i]=write;h=0x1000+4*i;break;}
    ret_stdcall(c,h,7);
}
static void original_a(Cpu *c){open_original(c,0);}
static void original_w(Cpu *c){open_original(c,1);}
static void original_write(Cpu *c) {
    uint32_t h=ARG(c,0),n=ARG(c,2);FILE *f=handle_get(h,H_FILE);
    unsigned slot=h>=0x1000?(h-0x1000)/4:4096;
    if(f) n=slot<4096&&writing[slot]?(uint32_t)fwrite(G(ARG(c,1)),1,n,f):0;
    if(ARG(c,3)) wr32(ARG(c,3),n);
    ret_stdcall(c,1,5);
}
static void original_close(Cpu *c) {
    uint32_t h=ARG(c,0);FILE *f=handle_get(h,H_FILE);
    if(f) { unsigned slot=(h-0x1000)/4; files[slot]=NULL; writing[slot]=0;
        if(f==read_cache) cache_busy=0;else fclose(f);
    }
    ret_stdcall(c,1,1);
}
const ApiEntry win32_threads[]={{NULL,NULL}};
const ApiEntry win32_kernel32[]={{"CreateFileA",original_a},{"CreateFileW",original_w},
    {"WriteFile",original_write},{"CloseHandle",original_close},{NULL,NULL}};
static GuestFn find(const char *name) {
    const ApiEntry *entry=win32_fileio;
    for(;entry->name;entry++) if(!strcmp(entry->name,name)) return entry->fn;
    abort();
}
static uint32_t open_guest(const char *path,uint32_t access,int wide) {
    Cpu c={0};c.esp.d=0x100;
    strcpy(STR(0x500),path);for(unsigned i=0;i<=strlen(path);i++)wr16(0x700+i*2,(unsigned char)path[i]);
    wr32(0x104,wide?0x700:0x500);wr32(0x108,access);wr32(0x10c,0x33);
    wr32(0x110,0);wr32(0x114,2);wr32(0x118,0x80);wr32(0x11c,0);
    find(wide?"CreateFileW":"CreateFileA")(&c);assert(c.esp.d==0x120);return c.eax.d;
}
static uint32_t write_guest(uint32_t handle,const char *text,int delegate_only) {
    Cpu c={0};c.esp.d=0x100;strcpy(STR(0x1000),text);
    wr32(0x104,handle);wr32(0x108,0x1000);wr32(0x10c,(uint32_t)strlen(text));wr32(0x110,0x2000);wr32(0x114,0);
    int null_before=null_flushes;
    if(delegate_only) original_write(&c);else find("WriteFile")(&c);
    assert(null_flushes==null_before);
    assert(c.esp.d==0x118);return c.eax.d;
}
static void close_guest(uint32_t h) {
    Cpu c={0};c.esp.d=0x100;wr32(0x104,h);find("CloseHandle")(&c);assert(c.esp.d==0x108);
}
static void visible(const char *path,const char *expected) {
    FILE *f=fopen(path,"rb");assert(f);char text[1024]={0};size_t n=fread(text,1,sizeof text,f);fclose(f);
    assert(n==strlen(expected)&&!memcmp(text,expected,n));
}
int main(int argc,char **argv) {
    guest_base=calloc(1,0x4000);assert(guest_base);
    if(argc==2) {
        int old=!strcmp(argv[1],"--child-buffered");
        uint32_t h=open_guest(old?"child-before.log":"child-after.log",0x40000000u,0);
        assert(h!=UINT32_MAX);assert(write_guest(h,"last line survives forced exit\n",old));
        _write(1,"READY\n",6);
        char byte; _read(0,&byte,1); /* parent keeps stdin open, then terminates us */
        abort();
    }
    uint32_t h=open_guest("write-a.log",0xc0000000u,0);assert(h!=UINT32_MAX);
    assert(write_guest(h,"first\n",0));assert(flushes==1 && rd32(0x2000)==6);visible("write-a.log","first\n");
    assert(write_guest(h,"last",0));assert(flushes==2);visible("write-a.log","first\nlast");
    assert(write_guest(h,"",0));assert(flushes==3);close_guest(h);
    h=open_guest("write-w.log",0x40000000u,1);assert(write_guest(h,"wide",0));assert(flushes==4);visible("write-w.log","wide");
    fail_flush=1;assert(!write_guest(h," failure",0) && last_error==29 && rd32(0x2000)==8);
    fail_flush=0;assert(write_guest(h,"",0));visible("write-w.log","wide failure");close_guest(h);
    int count=flushes;
    FILE *seed=fopen("read.dat","wb");assert(seed);fputs("cached input",seed);fclose(seed);
    uint32_t r=open_guest("read.dat",0x80000000u,0);assert(r==h);
    FILE *cached=handle_get(r,H_FILE);assert(fgetc(cached)=='c');
    assert(write_guest(r,"",0) && flushes==count);close_guest(r);
    r=open_guest("read.dat",0x80000000u,1);assert(handle_get(r,H_FILE)==cached && fgetc(cached)=='c');
    assert(write_guest(r,"ignored",0) && !rd32(0x2000) && flushes==count);close_guest(r);
    assert(write_guest(r,"closed",0) && flushes==count);
    assert(write_guest(0,"stdout",0) && write_guest(UINT32_MAX,"invalid",0) && flushes==count);
    fclose(read_cache);free(guest_base);
    puts("fileio: A/W, write-through, read reuse, handle reuse, stdcall, no fflush(NULL), error propagation OK");
}
