#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
int main(int argc,char **argv){if(argc!=4)return 2;for(int i=1;i<4;++i){void *h=dlopen(argv[i],RTLD_NOW|RTLD_GLOBAL);if(!h){fprintf(stderr,"%s\n",dlerror());return 1;}if(i==3){uint32_t(*version)(void)=dlsym(h,"obs_module_ver");if(!version||!dlsym(h,"obs_module_load")||!dlsym(h,"obs_module_unload"))return 3;printf("PASS: modulo caricato con le librerie ufficiali OBS; API %u.%u.%u\n",version()>>24,(version()>>16)&255,version()&65535);}}return 0;}
