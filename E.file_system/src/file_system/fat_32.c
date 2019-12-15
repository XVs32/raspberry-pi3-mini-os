/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
//https://www.twblogs.net/a/5b7c43132b71770a43da21e2
#include "sd.h"
#include "mm.h"
#include "mini_uart.h"
#include "printf.h"
#include "fs.h" 
#include "fat.h"
// get the end of bss segment from linker
extern unsigned char _end;
extern unsigned char _start_;
/**
 * Find a file in root directory entries
 */
unsigned int fat32_getcluster(void* nope, char *fn,struct dev* sd_num)
{
    bpb_t *bpb=(bpb_t*)(&_end+ sd_num->dbr);/*DBR*/
    fatdir_t *dir_32=(fatdir_t*)(&_end+2048);
    unsigned int root_sec, s;
    // find the root directory's LBA
    root_sec=((bpb->spf32)*bpb->nf)+bpb->rsc;
    s = (bpb->nr0 + (bpb->nr1 << 8)) * sizeof(fatdir_t);
    root_sec+=(bpb->rc-2)*bpb->spc;
    // add partition LBA
    root_sec+= sd_num->partitionlba;  //root_sec+=partition[0].partitionlba;
    // load the root directory
    if(kservice_dev_read(1, root_sec,(unsigned char*)dir_32,s/512+1)) {
        // iterate on each entry and check if it's the one we're looking for
        for(;dir_32->name[0]!='U';dir_32++) {
            // is it a valid entry?
            if(dir_32->name[0]==0xE5 || dir_32->attr[0]==0xF) continue;
            // filename match?
            if(!memcmp(dir_32->name,fn,8)) {
                return ((unsigned int)dir_32->ch)<<16|dir_32->cl;
            }
        }
        //uart_puts("ERROR: file not found\n");
    } else {
        uart_puts("ERROR: Unable to load root directory\n");
    }
    return 0;
}

/**
 * Read a file into memory
 */
int sect = 0;
/*need a real and buf*/
openfile* fat32_readfile(void* nope, int cluster,struct dev* sd_num)
{
    openfile *ret;
    openfile tmp;
    ret = &tmp;
    // BIOS Parameter Block
    bpb_t *bpb=(bpb_t*)(&_end+sd_num->dbr);
    // File allocation tables. We choose between FAT16 and FAT32 dynamically
    unsigned int *fat32=(unsigned int*)(&_start_ + sd_num-> fat_table_start - 512 + bpb->rsc*512);/*reserved: bpb->rsc*/
    // Data pointers
    unsigned int data_sec, s;
    unsigned char *data, *ptr;
    // find the LBA of the first data sector
    data_sec = ((bpb->spf32)*bpb->nf)+bpb->rsc;
    //s = (bpb->nr0 + (bpb->nr1 << 8)) * sizeof(fatdir_t);
    // add partition LBA
    data_sec+= sd_num->partitionlba;

    // end of FAT in memory

    //test file size
    int test_size = 0;
    int tmp_cluster = cluster;
    while(tmp_cluster>1 && tmp_cluster<0xFFF8) {
	test_size++;
	tmp_cluster=fat32[tmp_cluster];
    }	
    struct mm_info page = allocate_kernel_page(test_size*(bpb->spc*(bpb->bps0 + (bpb->bps1 << 8))));/*improve*//*buff*/
    struct mm_info phy_page = allocate_kernel_page(4096);
    data = ptr = (unsigned char *)page.start;
    // iterate on cluster chain
    int iterate = 0;
    while(cluster>1 && cluster<0xFFF8) {
	((unsigned long *)phy_page.start)[iterate++] = (unsigned long)(cluster-2)*bpb->spc+data_sec;
	// load all sectors in a cluster
	if(!kservice_dev_read(1, (cluster-2)*bpb -> spc + data_sec, ptr ,bpb->spc)){
		printf("Unable to read SD card!");
		return NULL;
	}

	// move pointer, sector per cluster * bytes per sector
        ptr+=bpb->spc*(bpb->bps0 + (bpb->bps1 << 8));
        // get the next cluster in chain
        cluster=fat32[cluster];
    }

    ret->phy_addr = (unsigned char *)phy_page.start;
    ret->log_addr = data;
    return ret;
}

openfile* fat32_readbuf(void* nope, int cluster,struct dev* sd_num, void* blk)
{
    openfile *ret;
    openfile tmp;
    ret = &tmp;
    // BIOS Parameter Block
    bpb_t *bpb=(bpb_t*)(&_end+sd_num->dbr);
    // File allocation tables. We choose between FAT16 and FAT32 dynamically
    unsigned int *fat32=(unsigned int*)(&_start_ + sd_num-> fat_table_start - 512 + bpb->rsc*512);/*reserved: bpb->rsc*/
    // Data pointers
    unsigned int data_sec, s;
    unsigned char *data, *ptr;
    // find the LBA of the first data sector
    data_sec = ((bpb->spf32)*bpb->nf)+bpb->rsc;
    //s = (bpb->nr0 + (bpb->nr1 << 8)) * sizeof(fatdir_t);
    // add partition LBA
    data_sec+= sd_num->partitionlba;
    
    if(blk == NULL){
	    struct mm_info page = allocate_kernel_page((bpb->spc*(bpb->bps0 + (bpb->bps1 << 8))));/*improve*//*buff*/
            data = ptr = (unsigned char *)page.start;
	    struct mm_info phy_page = allocate_kernel_page(4096);
	    

	    int iterate = 0;

	    if(cluster>1 && cluster<0xFFF8) {
		// load all sectors in a cluster

		((unsigned long *)phy_page.start)[iterate++] = (unsigned long)(cluster-2)*bpb->spc+data_sec;

		if(!kservice_dev_read(1, (cluster-2)*bpb -> spc + data_sec, ptr ,bpb->spc)){
			printf("Unable to read SD card!");
			return NULL;
		}
		// get the next cluster in chain
		cluster=fat32[cluster];       
	    }else{
		printf("Cluster is dirty.\n\r");
		return NULL;
	    }

	    while(cluster>1 && cluster<0xFFF8) {
		// load all sectors in a cluster
		((unsigned long *)phy_page.start)[iterate++] = (unsigned long)(cluster-2)*bpb->spc+data_sec;
		// get the next cluster in chain
		cluster=fat32[cluster];
	    }
	    ret->phy_addr = (unsigned char *)phy_page.start;
	    ret->log_addr = data;
    }else{
	    data = ptr = (unsigned char *)blk;
	    if(!kservice_dev_read(1, cluster, ptr ,bpb->spc)){
			printf("Unable to read SD card!");
			return NULL;
	    }
	    ret->phy_addr = NULL;
	    ret->log_addr = data;
    }
    return ret;
}


openfile* fat32_read_directory(void* nope, struct dev* sd_num)
{
    openfile* ret;
    bpb_t *bpb=(bpb_t*)(&_end + sd_num->dbr);
    ret = fat32_readfile(0,bpb->rc, sd_num);
    //sd_num->directory = fat_addr;/*improve*/
    //data_dump((char *)(&_start_ + (unsigned int)(ret->log_addr)),256);
    //fat_listdirectory(&_end+(sd_num->directory_addr.log_addr - (unsigned int)&_end));
    return ret;
}

/*succeed=0 or fail=1*/
int* fat32_writebuf(void* nope, struct dev* sd_num,char* phy,int filesize, unsigned long log)
{ 
    bpb_t *bpb=(bpb_t*)(&_end+sd_num->dbr);
    int clustersize = bpb->spc*(bpb->bps0 + (bpb->bps1 << 8));
    if(filesize*512 > clustersize){
	
    }else{
	/*writefile*/
    	sdTransferBlocks (phy, filesize, log , 1);
    }
    

    return NULL;   
}

/*succeed=0 or fail=1*/
int* fat32_writedir(void* nope, char* phy_dir,char* log_dir)
{ 
   
    /*writedirectory*/
    sdTransferBlocks (phy_dir, 1, log_dir , 1);
    return NULL;   
}
