#include "kernel_api.h"
#include "fs.h"

// get the end of bss segment from linker
extern unsigned char _end;
extern unsigned int t;	
/**
 * Find a file in root directory entries
 */

unsigned int fat16_getcluster(char *fn,struct dev* sd_num)
{

    bpb_t *bpb=(bpb_t*)(&_end+ sd_num->record);/*DBR*/
    fatdir_t *dir=(fatdir_t*)(&_end+2048);
    unsigned int root_sec, s;
    // find the root directory's LBA
    root_sec=((bpb->spf16)*bpb->nf)+bpb->rsc;
    s = (bpb->nr0 + (bpb->nr1 << 8)) * sizeof(fatdir_t);
    // add partition LBA
    root_sec+= sd_num->partitionlba;  
    // load the root directory
    if(kservice_dev_read(1, root_sec, (unsigned char*)dir, s/512+1)) {
        // iterate on each entry and check if it's the one we're looking for
        for(;dir->name[0]!='U';dir++) {
            // is it a valid entry?
            if(dir->name[0]==0xE5 || dir->attr[0]==0xF) continue;
            // filename match?
            if(!memcmp(dir->name,fn,8)) {
                return ((unsigned int)dir->ch)<<16|dir->cl;
            }
        }

    } else {
        kservice_uart_write("ERROR: Unable to load root directory\n");
    }
    return 0;
}

/**
 * Read a file into memory
 */
char *fat16_readfile( int cluster,struct dev* sd_num)
{
    // BIOS Parameter Block
    bpb_t *bpb=(bpb_t*)(&_end+sd_num->record);
    // File allocation tables. We choose between FAT16 and FAT32 dynamically
    unsigned int *fat32=(unsigned int*)(&_end + (512*3-sd_num->record) + bpb->rsc*512);/*reserved: bpb->rsc*/
    unsigned short *fat16=(unsigned short*)fat32;
    // unsigned short *fat16=(unsigned short*)fat32;
    // Data pointers
    unsigned int data_sec, s;
    unsigned char *data, *ptr;
    // find the LBA of the first data sector
    data_sec = ((bpb->spf16)*bpb->nf)+bpb->rsc;
    s = (bpb->nr0 + (bpb->nr1 << 8)) * sizeof(fatdir_t);

    data_sec+=(s+511)>>9;//fat16
    // add partition LBA
    data_sec+= sd_num->partitionlba;
    // dump important properties
    // load FAT table

    s = kservice_dev_read(1, sd_num->partitionlba+1,(unsigned char*)(&_end+2048),(bpb->spf16)+bpb->rsc);
   
    // end of FAT in memory
    data = ptr = (unsigned char*)(&_end+2048+s);
    // iterate on cluster chain
    while(cluster>1 && cluster<0xFFF8) {
	// load all sectors in a cluster
        kservice_dev_read(1, (cluster-2)*bpb -> spc + data_sec, ptr ,bpb->spc);
	// move pointer, sector per cluster * bytes per sector
        ptr+=bpb->spc*(bpb->bps0 + (bpb->bps1 << 8));
        // get the next cluster in chain
        cluster=fat16[cluster];
    }
    return (char*)data;
}
void fat16_read_directory(void* nope, struct dev* sd_num)
{ 
    bpb_t *bpb=(bpb_t*)(&_end+sd_num->record);
    unsigned int root_sec, s;
    // find the root directory's LBA
    root_sec=((bpb->spf16)*bpb->nf)+bpb->rsc;
    s = (bpb->nr0 + (bpb->nr1 << 8));
    s *= sizeof(fatdir_t);
    // add partition LBA
    root_sec+=sd_num->partitionlba;
    // load the root directory
    kservice_dev_read(1, root_sec,(unsigned char*)(&_end+2048),s/512+1);
    unsigned long addr = (unsigned long)(&_end+2048);
    fat_listdirectory((unsigned int*)(&_end+(addr-(unsigned long)&_end)));
}
