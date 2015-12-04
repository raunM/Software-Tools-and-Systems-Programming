#include <stdio.h>
#include <getopt.h>
#include <ctype.h>

int totLines=0, totWords=0, totBytes=0;

/*Count lines, words and bytes given file*/
void process(FILE *f, int lineFlag, int wordFlag, int bytesFlag){
	int c = 0, numLines = 0, numWords = 0, numBytes = 0;
	int space;
	//get each character from file and increment counts accordingly
	for(; (c = fgetc(f)) != EOF; numBytes++){
		if (isspace(c)){
			if(c == '\n'){
				numLines++;
				space = 1;
			}
			space = 1;
		}
		else if (space){
			numWords++;
			space = 0;
		}
	}
	//no options are provided, default case
	if(!wordFlag && !lineFlag && !bytesFlag){
		printf("%d  %d  %d  ", numLines, numWords, numBytes);
	}
	else{
		if(lineFlag){
			printf("%d  ", numLines);
		}
		if(wordFlag){
			printf("%d  ", numWords);
		}
		if(bytesFlag){
			printf("%d  ", numBytes);
		}
	}
	totLines+=numLines, totWords+=numWords, totBytes+=numBytes;
}

int main(int argc, char **argv){
	int lineFlag=0, wordFlag=0, bytesFlag=0, fcount=0;
	int opts;
	FILE *fp;
	//parse options
	while ((opts = getopt(argc, argv, "lwc")) != -1){
		switch(opts){
		case 'l':
			lineFlag = 1;
			break;
		case 'w':
			wordFlag = 1;
			break;
		case 'c':
			bytesFlag = 1;
			break;
		case '?':
			fprintf (stderr, "Unknown option character `\\x%x'.\n",
			optopt);
			return 1;
		}
	}
	//assume stdin if there are no arguments
	if(!(argv[optind])){
		process(stdin, lineFlag, wordFlag, bytesFlag);
		printf("\n");
	}
	else{
		for (; optind < argc; optind++){
			if((fp=fopen(argv[optind], "r")) != NULL){
				process(fp, lineFlag, wordFlag, bytesFlag);
				printf("%s\n", argv[optind]);
				fcount++;
				fclose(fp);
			}
			else{
				printf("ERROR: Unable to open %s\n", argv[optind]);
				perror(argv[optind]);
				return -2;
			}
		}
		//if more than one file show totals
		if(fcount > 1){
			if(!wordFlag && !lineFlag && !bytesFlag){
				printf("%d  %d  %d %s\n", totLines, totWords, totBytes, "total");
			}
			else{
				if(lineFlag){
					printf("%d  ", totLines);
				}
				if(wordFlag){
					printf("%d  ", totWords);
				}
				if(bytesFlag){
					printf("%d  ", totBytes);
				}
				printf("%s\n", "total");
			}
		}
	}
	return 0;
}