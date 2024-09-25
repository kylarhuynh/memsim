#define _GNU_SOURCE

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<unistd.h>
#include<time.h>
#include<sys/types.h>
#include<sys/wait.h>

/*define the pagesize*/
#define PAGESIZE 4096;

int init = 0;
void *startofheap = NULL;

typedef unsigned char BYTE;
typedef struct chunkhead{
    unsigned int size;
    unsigned int info; /*0 if it is free, 1 if it is not free*/
    unsigned char *next,*prev;
}chunkhead;

/*function declarations*/
unsigned char *mymalloc(unsigned int);
void myfree(unsigned char*);
void analyze();

chunkhead *chunkhead0;
chunkhead *current;

int main(int argc, char const *argv[]){
    clock_t start;
    clock_t end;
    double timediff;
    int i;
    unsigned char *a,*b,*c,*d,*e;
    analyze();
    a = mymalloc(4000);
    b = mymalloc(4000);
    c = mymalloc(4000);
    d = mymalloc(4000);
    e = mymalloc(4000);
    myfree(e);
    myfree(d);
    analyze();

    // BYTE *a[100];
    // start = clock();
    // analyze();/*50% points*/
    // for(i=0;i<100;i++)
    // a[i]= mymalloc(1000);
    // for(i=0;i<90;i++)
    // myfree(a[i]);
    // analyze(); /*50% of points if this is correct*/
    // myfree(a[95]);
    // a[95] = mymalloc(1000);
    // analyze();/*25% points, this new chunk should fill the smaller free one*/
    // /*(best fit)*/
    // for(i=90;i<100;i++)
    // myfree(a[i]);
    // analyze();/* 25% should be an empty heap now with the start address */
    // /*from the program start*/
    // end = clock();
    // timediff = (double)(end - start) / CLOCKS_PER_SEC;
    // printf("Result Time: %f seconds\n", timediff);
    // return 0;
}

unsigned char *mymalloc(unsigned int size){
    chunkhead *last;
    chunkhead *new;
    chunkhead *bestfit = NULL;
    void *newptr = NULL;
    if(size > 0){
        if(size % 4096 !=  0){
            size = size + (4096 - (size % 4096));
        }
        if(init == 0){
            startofheap = sbrk(size);
            chunkhead0 = (chunkhead *)startofheap;
            chunkhead0->size = size;
            chunkhead0->info = 1;
            chunkhead0->next = NULL;
            chunkhead0->prev = NULL;
            init += 1;
            return startofheap;
        }
        else{
            current = chunkhead0;
            while(current != NULL){
                if(current->next == NULL){
                    last = current;
                }
                if(current-> info == 0 && current->size == size){
                    current->info = 1;
                    return (unsigned char *)current;
                }
                if(current->info == 0 && current->size >= size){
                    /* here is where i find best fit */
                    if(bestfit == NULL){
                        bestfit = current;
                    }
                    else{
                        if(current->size < bestfit->size){
                            bestfit = current;
                        }
                    }    
                }
            current = (chunkhead*)current->next;
            }
            if(bestfit == NULL){
                current = last;
                newptr = sbrk(size);
                new = (chunkhead *)newptr;
                new->size = size;
                new->info = 1;
                new->next = current->next;
                new->prev = (unsigned char *)current;
                current->next = (unsigned char*)new;
                return newptr;
            }
            else{ /*split memory*/
                new = (chunkhead *)((unsigned char *)bestfit + size);
                new->size = bestfit->size - size;   
                new->info = 0;
                new->next = bestfit->next;             
                new->prev = (unsigned char *)bestfit;

                if(bestfit->next != NULL){
                    ((chunkhead *)bestfit->next)->prev = (unsigned char *)new;
                }

                bestfit->size = size;
                bestfit->info = 1;
                bestfit->next = (unsigned char *)new;
                return (unsigned char *)bestfit;
            }
        }
    }
}

void myfree(unsigned char *address){
    current = (chunkhead *)(address);
    current->info = 0;

    /*merge with previous chunk*/
    if(current->prev != NULL && ((chunkhead *)current->prev)->info == 0){
        ((chunkhead *)current->prev)->size = ((chunkhead *)current->prev)->size + current->size;
        ((chunkhead *)current->prev)->next = current->next;
        if(current->next != NULL){
            ((chunkhead *)current->next)->prev = current->prev;
        }
        current = ((chunkhead *)current->prev);
    }

    /*merge with next chunk*/
    if(current->next != NULL && ((chunkhead *)current->next)->info == 0){
        current->size = ((chunkhead *)current->next)->size + current->size;
        current->next = ((chunkhead *)current->next)->next;
        if(current->next != NULL){
            ((chunkhead *)current->next)->prev = (unsigned char *)current;
        }
    }    

    /*move back program break when last chunk freed*/
    if(current->next == NULL){
        if(current->prev != NULL){
            current = (chunkhead *)current->prev;
            sbrk(-(((chunkhead *)current->next)->size));
            current->next = NULL;
        }
        else{
            sbrk(-(current->size));
            chunkhead0 = NULL;
            startofheap = NULL;
            init = 0;
        }
    }
}

void analyze(){
    int no;
    chunkhead* ch;
    printf("\n--------------------------------------------------------------\n");
    if(!startofheap){
        printf("no heap, program break on address: %x\n",sbrk(0));
        return;
    }
    ch = (chunkhead*)startofheap;
    for (no=0; ch; ch = (chunkhead*)ch->next,no++){
        printf("%d | current addr: %x |", no, ch);
        printf("size: %d | ", ch->size);
        printf("info: %d | ", ch->info);
        printf("next: %x | ", ch->next);
        printf("prev: %x", ch->prev);
        printf(" \n");
    }
    printf("program break on address: %x\n",sbrk(0));
}
