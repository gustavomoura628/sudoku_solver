#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define VERSION "0.4"

unsigned int FAILED = 0;

struct timespec stopwatch_start()
{
    struct timespec time_start;
    timespec_get(&time_start,TIME_UTC);
    return time_start;
}

double stopwatch_stop(struct timespec time_start)
{
    struct timespec time_end;
    timespec_get(&time_end,TIME_UTC);

    // calculate elapsed time
    double time_difference = time_end.tv_sec - time_start.tv_sec + (time_end.tv_nsec - time_start.tv_nsec)/(double)1000000000;
    return time_difference;
}

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

static const unsigned char BitsSetTable256[256] = 
{
# define B2(n) n, n+1, n+1, n+2
# define B4(n) B2(n), B2(n+1), B2(n+1),B2(n+2)
# define B6(n) B4(n), B4(n+1), B4(n+1),B4(n+2)
    B6(0), B6(1), B6(1), B6(2)
};

unsigned int count_bits(unsigned long v)
{
    unsigned int c;
        // Option 2:
    unsigned char * p = (unsigned char *) &v;
    c = BitsSetTable256[p[0]] + 
        BitsSetTable256[p[1]] + 
        BitsSetTable256[p[2]] +	
        BitsSetTable256[p[3]] +	
        BitsSetTable256[p[4]] +	
        BitsSetTable256[p[5]] +	
        BitsSetTable256[p[6]] +	
        BitsSetTable256[p[7]];
    return c;
}
//unsigned int count_bits(unsigned long v) // count the number of bits set in v
//{
//unsigned int c; // c accumulates the total bits set in v
//for (c = 0; v; c++)
//{
//  v &= v - 1; // clear the least significant bit set
//  counter++;
//}
//return c;
//}


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
    c->blocksize = s->blocksize;
    c->size = s->size;
    int i;
    for(i=0;i<s->size*s->size;i++)
        c->table[i] = s->table[i];
    for(i=0;i<s->size;i++)
        c->brow[i] = s->brow[i];
    for(i=0;i<s->size;i++)
        c->bcolumn[i] = s->bcolumn[i];
    for(i=0;i<s->size;i++)
        c->bblock[i] = s->bblock[i];
    c->status = s->status;
    return c;
}

void copy_into(struct sudoku * dest, struct sudoku * src)
{
    dest->blocksize = src->blocksize;
    dest->size = src->size;
    int i;
    memcpy(dest->table,src->table,src->size*src->size*sizeof(unsigned int));
    memcpy(dest->brow,src->brow,src->size*sizeof(unsigned long));
    memcpy(dest->bcolumn,src->bcolumn,src->size*sizeof(unsigned long));
    memcpy(dest->bblock,src->bblock,src->size*sizeof(unsigned long));
    dest->status = src->status;
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
            {
                long unsigned before = s->brow[i];
                s->brow[i]&=~(1<<(s->table[j+i*s->size]-1));
                // if constraints dont change it means that the number was there before
                if(before == s->brow[i])
                {
                    fprintf(stderr, "Invalid input: invalid row %d\n",i);
                    s->status = 2;
                    return;
                }
            }
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
            {
                long unsigned before = s->bcolumn[j];
                s->bcolumn[j]&=~(1<<(s->table[j+i*s->size]-1));
                // if constraints dont change this means number was there before
                if(before == s->bcolumn[j])
                {
                    fprintf(stderr, "Invalid input: invalid column %d\n",j);
                    s->status = 2;
                    return;
                }
            }
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
            {
                long unsigned before = s->bblock[j/s->blocksize + (i/s->blocksize)*s->blocksize];
                s->bblock[j/s->blocksize + (i/s->blocksize)*s->blocksize]&=~(1<<(s->table[j+i*s->size]-1));
                // if constraints dont change this means number was there before
                if(before == s->bblock[j/s->blocksize + (i/s->blocksize)*s->blocksize])
                {
                    fprintf(stderr, "Invalid input: invalid block %d\n", j/s->blocksize + (i/s->blocksize)*s->blocksize);
                    s->status = 2;
                    return;
                }
            }
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
            printf("%2d ",s->table[j+i*s->size]);
            if(j%s->blocksize==s->blocksize-1 && j<s->size-1)printf("\033[40;30m||\033[47;30m");
        }
        printf("\033[m");
        printf("\n");
        if(i%s->blocksize==s->blocksize-1 && i < s->size-1)
        {
            int k;
            printf("\033[40;30m");
            for(k=0;k<3*s->size+2*(s->blocksize-1);k++)printf("|");
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

void branch(struct sudoku * s, int index)
{
    //print_sudoku(s);
    int i,j;
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
        int min_index;
        int min_bits = 999999;
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
                    {
                        has_impossible_value=1;
                    }
                    else if(nbits == 1)
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
                    else
                    {
                        if(nbits < min_bits)
                        {
                            min_bits = nbits;
                            min_index = j+i*s->size;
                        }
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
            branch(s, min_index);
            return;
        }
    }
}

struct sudoku * read_sudoku_from_stdin()
{
    int blocksize;
    printf("Enter blocksize: ");
    scanf("%d",&blocksize);
    struct sudoku * s = create(blocksize);
    int i;
    printf("Enter sudoku table: ");
    for(i=0;i<s->size*s->size;i++)
    {
        scanf("%u",&s->table[i]);
        if(s->table[i] > s->size)
        {
            fprintf(stderr, "Invalid board input: %u\n", s->table[i]);
            exit(-1);
        }
    }
    printf("\n");
    return s;
}

int main()
{
    //setup
    struct sudoku * s = read_sudoku_from_stdin();

    printf("\nSudoku to solve:\n");
    print_sudoku(s);

    setup(s);
    if(s->status != 0)
    {
        printf("Invalid board\n");
        return -1;
    }

    // stopwatch
    struct timespec solve_stopwatch = stopwatch_start();

    // solve
    solve(s);

    double solve_time = stopwatch_stop(solve_stopwatch);

    // display results
    printf("\n");
    if(s->status == 1)
    {
        printf("Solved:\n");
        print_sudoku(s);
    }
    else
    {
        printf("Board is impossible to solve\n");
    }

    printf("Number of boards explored = %u\n",FAILED);
    printf("Elapsed time = %.3fs\n",solve_time);

    // cleanup
    destroy(s);
}
