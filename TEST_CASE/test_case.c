#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc,char **argv){
	char buf[500]={0,};
	int i,j,use;
	char in_disk[91][5];
	char tmp[2];
	char list[3][100] = { "DISK1.img","DISK2.img","DISKFull.img"};
	char command_list[10][6] = {"cd","ls","0","touch","rmdir","rm","mv",0,};
	char *p;
	FILE *fp;
	int top = 2;
	int max = 40;
	if(argc < 2){
		printf("test_case <test_case_name> <number of random test> [all]\n");
		return 0;
	}
	use = 3;
	for(i=0;i<91;i++){
		bzero(buf,sizeof(buf));
		strcat(buf,"m");
		sprintf(tmp,"%d",i);
		strcat(buf,tmp);
		strncpy(in_disk[i],buf,strlen(buf)+1);
	}
	max = atoi(argv[2]);
	if(argc == 4){
		if(!strcmp(argv[3],"all"))
			use = 7;
	}
	p = argv[1];
	p = p+5;
	strcpy(command_list[top],p);
	for(i=0;i<3;i++){
		if(i >= 0){
			srand(time(NULL));
			bzero(buf,sizeof(buf));
			sprintf(buf,"test_random%d",i+1);
			fp = fopen(buf,"w");
			fprintf(fp,"mount %s\n",list[i]);
			for(j=0;j<max;j++)
				fprintf(fp,"%s %s\n",command_list[rand()%use],in_disk[rand()%91]);
			fprintf(fp,"fsck\nexit\n");
			fclose(fp);
			strcpy(argv[1],buf);
		}
		system("re");
		bzero(buf,sizeof(buf));
		printf("%s\n",list[i]);
		strcat(buf,"sfs < ");
		strcat(buf,argv[1]);
		strcat(buf," > output1.txt");
		system(buf);
		printf("%s \n",buf);
		bzero(buf,sizeof(buf));
		strcat(buf,"cp ");
		strcat(buf,list[i]);
		strcat(buf," DISK_te.img");
		system(buf);
		system("re");
		bzero(buf,sizeof(buf));
		strcat(buf,"mysfs < ");
		strcat(buf,argv[1]);
		strcat(buf," > output2.txt");
		system(buf);
		printf("%s \n",buf);
		bzero(buf,sizeof(buf));
		system("diff output1.txt output2.txt");
		strcat(buf,"cmp ");
		strcat(buf,list[i]);
		strcat(buf," DISK_te.img");
		system(buf);
	}
	return 0 ;
}
