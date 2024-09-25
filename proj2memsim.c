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
#define PAGESIZE 4096

int init = 0;
void *startofheap = NULL;

typedef unsigned char BYTE;
typedef struct chunkhead{
    unsigned int size;
    unsigned int info; /* 0 if it is free, 1 if it is not free */
    unsigned char *next, *prev;
} chunkhead;

/* Function declarations */
unsigned char *mymalloc(unsigned int);
void myfree(unsigned char*);
void analyze();

chunkhead *chunkhead0;
chunkhead *current;

int main(int argc, char const *argv[]){
    unsigned char *a, *b, *c, *d, *e;
    
    analyze();
    a = mymalloc(4000);
    b = mymalloc(4000);
    c = mymalloc(4000);
    d = mymalloc(4000);
    e = mymalloc(4000);
    myfree(e);
    myfree(d);
    analyze();
    
    return 0;
}

unsigned char *mymalloc(unsigned int size) {
    chunkhead *last;
    chunkhead *new;
    chunkhead *bestfit = NULL;
    void *newptr = NULL;

    /* Adjust size to page size boundary */
    if (size > 0) {
        if (size % PAGESIZE != 0) {
            size += (PAGESIZE - (size % PAGESIZE));
        }

        /* If first time allocation */
        if (init == 0) {
            startofheap = sbrk(size);
            chunkhead0 = (chunkhead *)startofheap;
            chunkhead0->size = size;
            chunkhead0->info = 1;
            chunkhead0->next = NULL;
            chunkhead0->prev = NULL;
            init += 1;
            return (unsigned char *)(chunkhead0 + 1); /* Return memory after the chunkhead */
        } 
        else {
            current = chunkhead0;
            while (current != NULL) {
                /* Track the last chunk */
                if (current->next == NULL) {
                    last = current;
                }
                /* Perfect fit */
                if (current->info == 0 && current->size == size) {
                    current->info = 1;
                    return (unsigned char *)(current + 1);
                }
                /* Find best fit chunk */
                if (current->info == 0 && current->size >= size) {
                    if (bestfit == NULL || current->size < bestfit->size) {
                        bestfit = current;
                    }
                }
                current = (chunkhead *)current->next;
            }

            /* No best fit found, allocate new chunk */
            if (bestfit == NULL) {
                current = last;
                newptr = sbrk(size);
                new = (chunkhead *)newptr;
                new->size = size;
                new->info = 1;
                new->next = current->next;
                new->prev = (unsigned char *)current;
                current->next = (unsigned char *)new;
                return (unsigned char *)(new + 1); /* Return memory after the chunkhead */
            } 
            else {
                /* Split the best-fit chunk */
                if (bestfit->size > size + sizeof(chunkhead)) {
                    new = (chunkhead *)((unsigned char *)bestfit + sizeof(chunkhead) + size);
                    new->size = bestfit->size - size - sizeof(chunkhead);
                    new->info = 0;
                    new->next = bestfit->next;
                    new->prev = (unsigned char *)bestfit;

                    if (bestfit->next != NULL) {
                        ((chunkhead *)bestfit->next)->prev = (unsigned char *)new;
                    }

                    bestfit->next = (unsigned char *)new;
                    bestfit->size = size;
                }
                bestfit->info = 1;
                return (unsigned char *)(bestfit + 1);
            }
        }
    }
    return NULL; /* In case size is 0 or invalid */
}

void myfree(unsigned char *address) {
    current = (chunkhead *)(address - sizeof(chunkhead)); /* Go back to chunkhead */
    current->info = 0;

    /* Merge with previous chunk if free */
    if (current->prev != NULL && ((chunkhead *)current->prev)->info == 0) {
        ((chunkhead *)current->prev)->size += current->size + sizeof(chunkhead);
        ((chunkhead *)current->prev)->next = current->next;
        if (current->next != NULL) {
            ((chunkhead *)current->next)->prev = current->prev;
        }
        current = (chunkhead *)current->prev;
    }

    /* Merge with next chunk if free */
    if (current->next != NULL && ((chunkhead *)current->next)->info == 0) {
        current->size += ((chunkhead *)current->next)->size + sizeof(chunkhead);
        current->next = ((chunkhead *)current->next)->next;
        if (current->next != NULL) {
            ((chunkhead *)current->next)->prev = (unsigned char *)current;
        }
    }

    /* Move program break back if last chunk */
    if (current->next == NULL) {
        if (current->prev != NULL) {
            current = (chunkhead *)current->prev;
            sbrk(-(current->size + sizeof(chunkhead)));
            current->next = NULL;
        } else {
            sbrk(-(current->size + sizeof(chunkhead)));
            chunkhead0 = NULL;
            startofheap = NULL;
            init = 0;
        }
    }
}

void analyze() {
    int no;
    chunkhead *ch;

    printf("\n--------------------------------------------------------------\n");
    if (!startofheap) {
        printf("No heap, program break at: %p\n", sbrk(0));
        return;
    }

    ch = (chunkhead *)startofheap;
    for (no = 0; ch; ch = (chunkhead *)ch->next, no++) {
        printf("%d | current addr: %p | ", no, (void *)ch);
        printf("size: %d | ", ch->size);
        printf("info: %d | ", ch->info);
        printf("next: %p | ", (void *)ch->next);
        printf("prev: %p", (void *)ch->prev);
        printf("\n");
    }
    printf("Program break at: %p\n", sbrk(0));
}
