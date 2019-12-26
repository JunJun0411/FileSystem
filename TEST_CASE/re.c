#include <stdio.h>
#include <stdlib.h>

void main(){

	char string[100] = {"cd ..\ncp DISK1.img testdir\ncp DISK2.img testdir\ncp DISKFull.img testdir\ncd testdir"};
	system(string);
	return;
}
