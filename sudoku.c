#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.3"

unsigned int FAILED = 0;

void print_bits(unsigned long x)
{
    int i;
    for(i=0;i<8*sizeof(unsigned long);i++)
    {
        printf("%d",(x>>63)>0);
        x<<=1;
    }
    printf("\n");
}

unsigned int count_bits(unsigned long v) // count the number of bits set in v
{
    unsigned int c; // c accumulates the total bits set in v
    for (c = 0; v; c++)
    {
      v &= v - 1; // clear the least significant bit set
    }
    return c;
}


struct sudoku{
    unsigned int * table;
    unsigned long * brow;
    unsigned long * bcolumn;
    unsigned long * bblock;
    int size;
    int blocksize;
    int status; //0 unknown, 1 solved, 2 impossible
};

struct sudoku * create(int blocksize)
{
    struct sudoku * s = malloc(sizeof(struct sudoku));
    s->blocksize = blocksize;
    s->size = blocksize*blocksize;
    s->table   = malloc(sizeof(unsigned int ) * s->size * s->size);
    s->brow    = malloc(sizeof(unsigned long) * s->size);
    s->bcolumn = malloc(sizeof(unsigned long) * s->size);
    s->bblock  = malloc(sizeof(unsigned long) * s->size);
    s->status = 0;
    return s;
}

struct sudoku * copy(struct sudoku *s)
{
    struct sudoku * c = create(s->blocksize);
    c->blocksize = s->blocksize ;
    c->size = s->size ;
    int i;
    for(i=0;i<s->size*s->size;i++)
        c->table[i] = s->table[i] ;
    for(i=0;i<s->size;i++)
        c->brow[i] = s->brow[i] ;
    for(i=0;i<s->size;i++)
        c->bcolumn[i] = s->bcolumn[i] ;
    for(i=0;i<s->size;i++)
        c->bblock[i] = s->bblock[i] ;
    c->status = s->status ;
    return c;
}

void copy_into(struct sudoku * dest, struct sudoku * src)
{
    dest->blocksize = src->blocksize ;
    dest->size = src->size ;
    int i;
    memcpy(dest->table,src->table,src->size*src->size*sizeof(unsigned int));
    memcpy(dest->brow,src->brow,src->size*sizeof(unsigned long));
    memcpy(dest->bcolumn,src->bcolumn,src->size*sizeof(unsigned long));
    memcpy(dest->bblock,src->bblock,src->size*sizeof(unsigned long));
    dest->status = src->status ;
}

void destroy(struct sudoku * s)
{
    free(s->table);
    free(s->brow);
    free(s->bcolumn);
    free(s->bblock);
    free(s);
}

void setup(struct sudoku * s)
{
    int i,j;
    //row
    for(i=0;i<s->size;i++)
    {
        s->brow[i]=(1<<(s->size))-1;
        for(j=0;j<s->size;j++)
        {
            if(s->table[j+i*s->size])
                s->brow[i]&=~(1<<(s->table[j+i*s->size]-1));
        }
//        printf("row %d = ",i);
//        print_bits(s->brow[i]);
    }
    //column
    for(j=0;j<s->size;j++)
    {
        s->bcolumn[j]=(1<<(s->size))-1;
        for(i=0;i<s->size;i++)
        {
            if(s->table[j+i*s->size])
                s->bcolumn[j]&=~(1<<(s->table[j+i*s->size]-1));
        }
//        printf("column %d = ",j);
//        print_bits(s->bcolumn[j]);
    }
    //block
    for(i=0;i<s->size;i++)
        s->bblock[i]=(1<<(s->size))-1;

    for(i=0;i<s->size;i++)
    {
        for(j=0;j<s->size;j++)
        {
            if(s->table[j+i*s->size])
                s->bblock[j/s->blocksize + (i/s->blocksize)*s->blocksize]&=~(1<<(s->table[j+i*s->size]-1));
        }
    }
//    for(i=0;i<s->size;i++)
//    {
//        printf("block %d = ",i);
//        print_bits(s->bblock[i]);
//    }
}

void print_sudoku(struct sudoku * s)
{
    int i,j;
    for(i=0;i<s->size;i++)
    {
        printf("\033[47;30m");
        for(j=0;j<s->size;j++)
        {
            printf(" %2d ",s->table[j+i*s->size]);
            if(j%s->blocksize==s->blocksize-1)printf("||");
        }
        printf("\033[m");
        printf("\n");
        printf("\033[47;30m");
        if(i%s->blocksize==s->blocksize-1)
        {
            int k;
            for(k=0;k<4*s->size+2*s->blocksize;k++)printf("|");
            printf("\033[m");
            printf("\n");
        }
    }
}

int bit_position(unsigned long x)
{
    int c = 0;
    while(x)
    {
        c++;
        x>>=1;
    }
    return c;
}

void solve(struct sudoku * s);

void branch(struct sudoku * s)
{
    //print_sudoku(s);
    int i,j;
    int index;
    unsigned int min_bits = 999999;

    //finding index
    for(i=0;i<s->size;i++)
    {
        for(j=0;j<s->size;j++)
        {
            if(!s->table[j+i*s->size])
            {
                unsigned long available =   s->brow[i]
                                          & s->bcolumn[j]
                                          & s->bblock[(j/s->blocksize)+(i/s->blocksize)*s->blocksize];

                int nbits = count_bits(available);

                if(nbits < min_bits)
                {
                    index = j+i*s->size;
                    min_bits = nbits;
                }
            }
        }
    }

    j=index%s->size;
    i=index/s->size;
    //printf("Best place to branch = (%d,%d) = ",i,j);
    unsigned long available =   s->brow[i]
                              & s->bcolumn[j]
                              & s->bblock[(j/s->blocksize)+(i/s->blocksize)*s->blocksize];
    //print_bits(available);

    int k=1;
    struct sudoku * c = copy(s);
    while(available)
    {
        if(available&1)
        {
            copy_into(c,s);

            unsigned long choice = (1<<(k-1));

            c->brow[i]&=~choice;
            c->bcolumn[j]&=~choice;
            c->bblock[(j/c->blocksize)+(i/c->blocksize)*c->blocksize]&=~choice;
            c->table[j+i*c->size]=bit_position(choice);

            //printf("trying %d\n",k);
            solve(c);

            if(c->status==1)
            {
                //printf("Solved!\n");
                copy_into(s,c);
                destroy(c);
                return;
            }
            //printf("nope\n");

        }

        available>>=1;
        k++;
    }

    //finished loop -> did not find feasible board
    destroy(c);
    s->status = 2;
    return;
}

void solve(struct sudoku * s)
{
    //printf("Solving...\n");
    //print_sudoku(s);
    int i,j;
    while(1)
    {
        int has_ambiguous_value = 0;
        int updated_something = 0;
        int has_impossible_value = 0;
        for(i=0;i<s->size;i++)
        {
            for(j=0;j<s->size;j++)
            {
                if(!s->table[j+i*s->size])
                {
                    has_ambiguous_value = 1;

                    unsigned long available =   s->brow[i]
                                              & s->bcolumn[j]
                                              & s->bblock[(j/s->blocksize)+(i/s->blocksize)*s->blocksize];

                    int nbits = count_bits(available);

                    if(nbits==0)
                        has_impossible_value=1;

                    if(nbits == 1)
                    {
                        //printf("row =    ");
                        //print_bits(s->brow[i]);
                        //printf("column = ");
                        //print_bits(s->bcolumn[j]);
                        //printf("block =  ");
                        //print_bits(s->bblock[(j/s->blocksize)+(i/s->blocksize)*s->blocksize]);
                        //printf("Setting %d,%d to %d\n",i,j,bit_position(available));

                        updated_something = 1;
                        s->brow[i]&=~available;
                        s->bcolumn[j]&=~available;
                        s->bblock[(j/s->blocksize)+(i/s->blocksize)*s->blocksize]&=~available;
                        s->table[j+i*s->size]=bit_position(available);

                        //track failed attempts
                        //printf("\033[0;0H\033[47;30m");
//                        printf("\033[0;0H");
//                        print_sudoku(s);
//                        printf("Failed %20d times\n",FAILED);
                        //printf("\033[m");
                    }
                }
            }
        }
        if(has_impossible_value)
        {
            FAILED++;
            s->status=2;
            return;
        }
        else if(!has_ambiguous_value)
        {
            s->status=1;
            return;
        }
        else if(!updated_something)
        {
            branch(s);
            return;
        }
    }
}

int testsudoku[9*9] = {
0,0,3,0,2,0,6,0,0,
9,0,0,3,0,5,0,0,0,
0,0,1,8,0,6,4,0,0,
0,0,8,1,0,2,9,0,0,
7,0,0,0,0,0,0,0,0,
0,0,6,7,0,8,2,0,0,
0,0,2,6,0,0,5,0,0,
8,0,0,2,0,3,0,0,0,
0,0,5,0,1,0,3,0,0
};

struct sudoku * read_sudoku_from_stdin()
{
    int blocksize;
    printf("Enter blocksize: ");
    scanf("%d",&blocksize);
    struct sudoku * s = create(blocksize);
    int i;
    printf("Enter sudoku table: ");
    for(i=0;i<s->size*s->size;i++)
        scanf("%u",&s->table[i]);
    return s;
}

int main()
{
    struct sudoku * s = read_sudoku_from_stdin();

    setup(s);
    //print_sudoku(s);
    solve(s);
    printf("\n");
    print_sudoku(s);
    printf("Number of boards explored = %u\n",FAILED);
    destroy(s);
}
