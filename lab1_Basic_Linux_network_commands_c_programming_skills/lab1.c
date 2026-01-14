#include <time.h>
#include <stdio.h>

int main(int argc, char * argv[]) {
    clock_t start, end;
    double cpu_time_used;
    start = clock();
    
    FILE *fp;
    if (argc != 3) {
        printf("invalid number of arguments. terminated");
    }
    FILE * fpi = fopen(argv[1], "r");
    FILE * fpo = fopen(argv[2], "w");

    if (fpi == NULL || fpo == NULL) {
        printf("unable to open file(s). terminated");
    }

    char buf[100];

    while (fread(&buf,1,sizeof(buf),fpi) > 0) {
        fwrite(&buf,1, sizeof(buf),fpo);
    }

    fclose(fpi);
    fclose(fpo);

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("CPU time used: %f", cpu_time_used);
}


	
