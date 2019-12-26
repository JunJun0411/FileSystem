//
// Simple FIle System
// 제출년도 2019년, 운영체제 과목
// Student Name : 박준형	
// Student Number : B511074
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
/* optional */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/***********/

#include "sfs_types.h"
#include "sfs_func.h"
#include "sfs_disk.h"
#include "sfs.h"

void dump_directory();

/* BIT operation Macros */
/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
struct bitMap{
	unsigned char bM[SFS_BLOCKSIZE];
};
struct indirect{
	u_int32_t sfi_direct[128];
};

static struct sfs_super spb;	// superblock
static struct sfs_dir sd_cwd = { SFS_NOINO }; // current working directory

void error_message(const char *message, const char *path, int error_code) {
	switch (error_code) {
	case -1:
		printf("%s: %s: No such file or directory\n",message, path); return;
	case -2:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -3:
		printf("%s: %s: Directory full\n",message, path); return;
	case -4:
		printf("%s: %s: No block available\n",message, path); return;
	case -5:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -6:
		printf("%s: %s: Already exists\n",message, path); return;
	case -7:
		printf("%s: %s: Directory not empty\n",message, path); return;
	case -8:
		printf("%s: %s: Invalid argument\n",message, path); return;
	case -9:
		printf("%s: %s: Is a directory\n",message, path); return;
	case -10:
		printf("%s: %s: Is not a file\n",message, path); return;
	case -11:
		printf("%s: input file size exceeds the max file size\n",message); return;
	case -12:
		printf("%s: can't open %s input file\n",message, path); return;
	default:
		printf("unknown error code\n");
		return;
	}
}

void sfs_mount(const char* path)
{
	if( sd_cwd.sfd_ino !=  SFS_NOINO )
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}

	printf("Disk image: %s\n", path);

	disk_open(path);
	disk_read( &spb, SFS_SB_LOCATION );

	printf("Superblock magic: %x\n", spb.sp_magic);

	assert( spb.sp_magic == SFS_MAGIC );
	
	printf("Number of blocks: %d\n", spb.sp_nblocks);
	printf("Volume name: %s\n", spb.sp_volname);
	printf("%s, mounted\n", spb.sp_volname);
	
	sd_cwd.sfd_ino = 1;		//init at root
	sd_cwd.sfd_name[0] = '/';
	sd_cwd.sfd_name[1] = '\0';
}

void sfs_umount() {

	if( sd_cwd.sfd_ino !=  SFS_NOINO )
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}
}
int bitMapFind(int bitMapCount, int Max){
	int i, j, k, re;
	struct bitMap bitM;
    //비트맵에 0 찾기
    for(i=2; i< 2+bitMapCount; i++){
        disk_read(&bitM, i);
        for(j =0; j<SFS_BLOCKSIZE; j++){
            if(bitM.bM[j] == 255) continue;
			else{
				for(k=0; k<8; k++){
					if(BIT_CHECK(bitM.bM[j],k)==0) {
						// 정상적인 block을 찾은경우
						if ( (re =(((i-2)*4096)+(j*8)+(k))) <= Max )
							return re;
						// Max값인 최대 Block수보다 큰경우 즉 꽉찬경우
						else return -1;
					}
				}
			}
        }
    }
	return -1;
}
void bitMapWrite(int Num){
    int i, j, k;
    i = (Num/4096) + 2;
    j = (Num%4096)/8;
    k = (Num%4096)%8;
    struct bitMap bitM;
	bzero(&bitM, sizeof(struct bitMap));
    disk_read(&bitM, i);
	BIT_SET(bitM.bM[j],k);
	// 0 -> 1 기록한 비트맵을 disk에 수정사항을 write해주고 탈출
	disk_write(&bitM, i);
}
void bitMapClear(int Num){
	int i, j, k;
	i = (Num/4096) + 2;
	j = (Num%4096)/8;
	k = (Num%4096)%8;
    struct bitMap bitM;
	bzero(&bitM, sizeof(struct bitMap));
 
	disk_read(&bitM, i);
    BIT_CLEAR(bitM.bM[j],k);
    disk_write(&bitM, i);
}

void sfs_touch(const char* path){
	int flag,a,b,t;
	// 같은 이름 있으면 에러
	if(findName(path, &t, &t)){
        error_message("touch", path, -6);
        return;
    }
    // 꽉찬 경우 에러처리
    if((flag = findEntry(&a, &b))==0){
        error_message("touch", path, -3);
        return;
    }

    int MaxBlock = spb.sp_nblocks;
    int bitMapCount = MaxBlock/SFS_BLOCKBITS;
    if(spb.sp_nblocks%SFS_BLOCKBITS>0) bitMapCount++;
    int freeBlock, freeBlock1, freeBlock2;

    struct sfs_inode si;
    struct sfs_dir sd[SFS_DENTRYPERBLOCK];
    disk_read( &si, sd_cwd.sfd_ino );
	disk_read( &sd, si.sfi_direct[a]);

    //for consistency
    assert( si.sfi_type == SFS_TYPE_DIR );
	
	// direct[a]의 sd[] dir읽어오기
    disk_read(&sd, si.sfi_direct[a]);
    if((freeBlock = bitMapFind(bitMapCount, MaxBlock))==-1){
        error_message("touch", path, -4);
        return;
    }
    bitMapWrite(freeBlock);
	struct sfs_dir sd1[SFS_DENTRYPERBLOCK];
    bzero(&sd1, SFS_BLOCKSIZE);

	// 새로 direct 할당 필요한 경우 (블록할당 +1)
	if(flag == 2){
		//비트맵 free블록번호(ex, 16) 받아서 direct[a]= 16 넣어준다. -> write(si) 필요
        assert(si.sfi_direct[a] == 0); //0이 아니라면 에러
        //direct[a] entry에 freeBlock번호 부여
        si.sfi_direct[a] = freeBlock;
		
        // 새 freeBlock 번호 받기
        if((freeBlock1 = bitMapFind(bitMapCount, MaxBlock))==-1){
            error_message("touch", path, -4);
            bitMapClear(freeBlock);
            return;
        }
        bitMapWrite(freeBlock1);
        sd1[0].sfd_ino = freeBlock1;
        strncpy(sd1[0].sfd_name,path ,SFS_NAMELEN);
		//16번 블록에 디렉토리 구조체 초기화시켜 만들고
        //0번째 entry에 ino번호, 이름 지정  넣어준다.
        disk_write(&sd1, freeBlock);
	}
	
	// 빈 entry발견한 경우
	else if(flag == 1){
		//b번쨰 entry에 ino번호, 이름 지정 -> disk_write(sd)필요
        sd[b].sfd_ino = freeBlock;
        strncpy(sd[b].sfd_name,path ,SFS_NAMELEN);
        disk_write(&sd, si.sfi_direct[a]);
	}
	si.sfi_size += sizeof(struct sfs_dir);
    disk_write(&si, sd_cwd.sfd_ino);

    //inode 구조체 만들어서 size, type, direct[0]에 freeblock 받아서 넣고 디스크쓰기
    struct sfs_inode newnode;
    bzero(&newnode, sizeof(struct sfs_inode));
	newnode.sfi_type = SFS_TYPE_FILE;
	newnode.sfi_size = 0;
	if(flag == 2) freeBlock = sd1[0].sfd_ino;
	disk_write(&newnode, freeBlock);
}

void sfs_cd(const char* path)
{
	if(path == NULL){
	    sd_cwd.sfd_ino = 1;     //init at root
	    sd_cwd.sfd_name[0] = '/';
	    sd_cwd.sfd_name[1] = '\0';
		return;
	}
	if(!strncmp(path,"." , SFS_NAMELEN)) return;
	int a,b;
	// 이름 찾고 없으면 에러
	if(!findName(path, &a, &b)){
		error_message("cd",path, -1);
		return;
	}
	struct sfs_inode si, ni, bi;
	struct sfs_dir sd[SFS_DENTRYPERBLOCK], nd[SFS_DENTRYPERBLOCK], bd[SFS_DENTRYPERBLOCK];
	bzero(&si, sizeof(struct sfs_inode));
	bzero(&ni, sizeof(struct sfs_inode));
	bzero(&bi, sizeof(struct sfs_inode));
	bzero(&sd, SFS_DENTRYPERBLOCK*sizeof(struct sfs_dir));
	bzero(&nd, SFS_DENTRYPERBLOCK*sizeof(struct sfs_dir));
	bzero(&bd, SFS_DENTRYPERBLOCK*sizeof(struct sfs_dir));
	disk_read(&si, sd_cwd.sfd_ino);
	disk_read(&sd, si.sfi_direct[a]);
	
	disk_read(&ni, sd[b].sfd_ino);
	if(sd[b].sfd_ino==1) {
		sfs_cd(NULL);
		return;
	}
	if(ni.sfi_type != SFS_TYPE_DIR){
		error_message("cd",path, -2);
		return;
	}
	// cd .. 입력한 경우
	if(!strncmp(path,"..",SFS_NAMELEN)){
		int i,j;
		disk_read(&nd, ni.sfi_direct[0]);
		disk_read(&bi, nd[1].sfd_ino);
		if(strncmp(nd[1].sfd_name, "..",SFS_NAMELEN)) printf("cd: error");
		for(i=0;i<15;i++){
			if(bi.sfi_direct[i]!=0){
				disk_read(&bd, bi.sfi_direct[i]);
				for(j=0;j<SFS_DENTRYPERBLOCK;j++){
					if(bd[j].sfd_ino == sd[b].sfd_ino){
						strncpy(sd_cwd.sfd_name, bd[j].sfd_name ,SFS_NAMELEN);
						sd_cwd.sfd_ino = sd[b].sfd_ino;
						return;
					}
				}
			}
		}		
	}
	sd_cwd.sfd_ino = sd[b].sfd_ino;
	strncpy(sd_cwd.sfd_name, path ,SFS_NAMELEN);
}

void sfs_ls(const char* path) //Error 처리 필요
{
	int i, j, ino;
	char flag = '0';
	//buffer for disk read
	struct sfs_inode lsInode, temp;
	struct sfs_dir lsDir[SFS_DENTRYPERBLOCK];
	//block access
	disk_read(&lsInode , sd_cwd.sfd_ino);
	
	for(i=0; i<15; i++){
		if(lsInode.sfi_direct[i] != 0){
			disk_read(&lsDir, lsInode.sfi_direct[i]);
			for(j=0; j< SFS_DENTRYPERBLOCK; j++){
				// ls만 쳤을 경우 path X
				if(path == NULL){
					flag='1';
					if((ino = lsDir[j].sfd_ino) != 0){
						printf("%s", lsDir[j].sfd_name); 
						disk_read(&temp , ino);
						if(temp.sfi_type == SFS_TYPE_DIR) printf("/");
						printf("\t");
					}
				}
				// path 파일이름 있는경우
				else{
					if((lsDir[j].sfd_ino!=0) && !strncmp(path, lsDir[j].sfd_name, SFS_NAMELEN)){
						//file 존재하면
						flag='1';
						disk_read(&lsInode, lsDir[j].sfd_ino);
						//만약 디렉토리가 아닌 path를 ls한 경우
						if(lsInode.sfi_type == SFS_TYPE_FILE){ 
							printf("%s", path);
						}
						else{
							for(i=0; i<15; i++){
								if(lsInode.sfi_direct[i] != 0){
									disk_read(&lsDir, lsInode.sfi_direct[i]);
								    for(j=0; j< SFS_DENTRYPERBLOCK; j++){
										if((ino = lsDir[j].sfd_ino) != 0){
							               	printf("%s", lsDir[j].sfd_name);
							            	disk_read(&temp , ino);
							                if(temp.sfi_type == 2) printf("/");
							                printf("\t");
										}
									}
								}
							}
						}
                    }
					else if(i!=14) continue;
				}	
			}
		}
	}
	// path와 같은 이름의  디렉토리 존재 하지 않았다면 출력
	if(flag == '0'){
		error_message("ls", path, -1);
		return;
	}
	printf("\n");
}

// 빈 entry찾아주는 함수 flag = '0': 꽉참 '1': entry발견 '2': 새로 할당필요
int findEntry(int *a, int *b){
	int i, j, di;
	char check='0';
	struct sfs_inode si;
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];
	disk_read(&si, sd_cwd.sfd_ino);
	
	for(i=0; i<15; i++){
		// i번째 있다면 확인
		if(si.sfi_direct[i] !=0){
			disk_read(&sd, si.sfi_direct[i]);
			for(j=0; j<SFS_DENTRYPERBLOCK; j++){
				// 비어있는 entry 찾으면
				if(sd[j].sfd_ino==0){
					*a = i;
					*b = j;
					return 1;
				}
				
			}
		}
		// 안쓰는 direct발견시 저장
		else if(si.sfi_direct[i] == 0 && check=='0'){
			di = i;	
			check = '1';
		}
	}
	// 꽉차거나 빈 direct가 있는경우
	if(check =='1'){
		*a = di;
		return 2;
	}
	// 모든 direct & entry 꽉찬경우
	else{
		return 0;
	}

}

// 같은 이름 있는지 확인 
int findName(const char* path, int *a, int *b){
	int i, j, di;
    char check='0';
    struct sfs_inode si;
    struct sfs_dir sd[SFS_DENTRYPERBLOCK];
    disk_read(&si, sd_cwd.sfd_ino);

    for(i=0; i<15; i++){
		if(si.sfi_direct[i] != 0){
			disk_read(&sd, si.sfi_direct[i]);
			for(j=0; j<SFS_DENTRYPERBLOCK; j++){
				if((sd[j].sfd_ino != 0) && (!strncmp(sd[j].sfd_name, path, SFS_NAMELEN))){
					*a = i;
					*b = j;
					return 1;
				}	
			}
		}
	}
	// 못찾은 경우
	return 0;
}

void sfs_mkdir(const char* org_path) 
{
	int flag,a,b;
    //skeleton implementation
	int MaxBlock = spb.sp_nblocks;
	int bitMapCount = MaxBlock/SFS_BLOCKBITS;
	if(spb.sp_nblocks%SFS_BLOCKBITS>0) bitMapCount++;
	int freeBlock, freeBlock1, freeBlock2;

    struct sfs_inode si;
    disk_read( &si, sd_cwd.sfd_ino );

    //for consistency
    assert( si.sfi_type == SFS_TYPE_DIR );
	
	//buffer for disk read
    struct sfs_dir sd[SFS_DENTRYPERBLOCK];
	// 동일 이름 있으면 에러처리
	int i,j,k;
	if(findName(org_path, &i, &j)){
		error_message("mkdir", org_path, -6);
		return;
	}
	// 꽉찬 경우 에러처리
	if((flag = findEntry(&a, &b))==0){
		error_message("mkdir", org_path, -3);
		return;
	}
	// direct[a]의 sd[] dir읽어오기
	disk_read(&sd, si.sfi_direct[a]);
	if((freeBlock = bitMapFind(bitMapCount, MaxBlock))==-1){
		error_message("mkdir", org_path, -4);
		return;
	}
	bitMapWrite(freeBlock);
	
	struct sfs_dir sd1[SFS_DENTRYPERBLOCK];
	bzero(&sd1, SFS_DENTRYPERBLOCK*sizeof(struct sfs_dir));

	// 새로 dir할당 해줘야하는 경우
	if(flag == 2){
		//비트맵 free블록번호(ex, 16) 받아서 direct[a]= 16 넣어준다. -> write(si) 필요
		assert(si.sfi_direct[a] == 0); //0이 아니라면 에러
		//direct[a] entry에 freeBlock번호 부여
		si.sfi_direct[a] = freeBlock;
		
		// 새 freeBlock 번호 받기
		if((freeBlock1 = bitMapFind(bitMapCount, MaxBlock))==-1){
			error_message("mkdir", org_path, -4);
			bitMapClear(freeBlock);
			return;
		}
		bitMapWrite(freeBlock1);
		sd1[0].sfd_ino = freeBlock1;
		strncpy(sd1[0].sfd_name,org_path,SFS_NAMELEN);
		//16번 블록에 디렉토리 구조체 초기화시켜 만들고 
		//0번째 entry에 ino번호, 이름 지정  넣어준다 .
		disk_write(&sd1, freeBlock);
	}
	// 정상 발견한 경우
	else if(flag == 1){
		//b번쨰 entry에 ino번호, 이름 지정 -> disk_write(sd)필요
		sd[b].sfd_ino = freeBlock;
		strncpy(sd[b].sfd_name,org_path ,SFS_NAMELEN);
		disk_write(&sd, si.sfi_direct[a]);
	}
	si.sfi_size += sizeof(struct sfs_dir);
	disk_write(&si, sd_cwd.sfd_ino);
	
	//inode 구조체 만들어서 size, type, direct[0]에 freeblock 받아서 넣고 디스크쓰기
	struct sfs_inode newnode;
	bzero(&newnode, sizeof(struct sfs_inode));
	newnode.sfi_size = 2*sizeof(struct sfs_dir);
	newnode.sfi_type = SFS_TYPE_DIR;
	if((freeBlock2 = bitMapFind(bitMapCount, MaxBlock))==-1){
		error_message("mkdir", org_path, -4);
		bitMapClear(freeBlock);
		if(flag==2) bitMapClear(freeBlock1);
		return;
	}
	bitMapWrite(freeBlock2);
	newnode.sfi_direct[0] = freeBlock2;
	if(flag == 2) freeBlock = sd1[0].sfd_ino;		
	disk_write(&newnode, freeBlock);
	
	// dir . .. 두개 넣은 배열 만들어서 freeBlock2번 블록에 할당
	struct sfs_dir newdir[SFS_DENTRYPERBLOCK];
	bzero(&newdir, SFS_DENTRYPERBLOCK*sizeof(struct sfs_dir));
	newdir[0].sfd_ino = freeBlock;
	strncpy(newdir[0].sfd_name, ".",SFS_NAMELEN);
	newdir[1].sfd_ino = sd_cwd.sfd_ino;
	strncpy(newdir[1].sfd_name, "..",SFS_NAMELEN);
	disk_write(&newdir, freeBlock2);

}

void sfs_rmdir(const char* org_path) 
{
	int i, j, k;
    int a, b;
	
	// .를 path로 지정한 경우
	if(!strncmp(org_path, ".", SFS_NAMELEN)){
		error_message("rmdir", org_path, -8);
		return;
	}
	
	// ..를 path로 지정한 경우
	if(!strncmp(org_path, "..", SFS_NAMELEN)){
		error_message("rmdir", org_path, -7);
		return;
	}

	// 같은 이름 파일 찾기
    if(!findName(org_path, &a, &b)){
        error_message("rmdir", org_path, -1);
        return;
    }
    struct sfs_inode si, ni;
    struct sfs_dir sd[SFS_DENTRYPERBLOCK];
    disk_read(&si, sd_cwd.sfd_ino);
    disk_read(&sd, si.sfi_direct[a]);

    disk_read(&ni, sd[b].sfd_ino);
    
	// 지우고자 하는 파일이 디렉토리가 아닌 경우
    if(ni.sfi_type != SFS_TYPE_DIR){
        error_message("rmdir", org_path, -2);
        return;
    }
	
	// 빈 디렉토리가 아닌 경우
	if(ni.sfi_size != (2 * sizeof(struct sfs_dir))){
		error_message("rmdir", org_path, -7);
        return;
	}
	
	// 해당 inode의 direct[i] 데이터 블락 릴리즈
    for(i=0; i<15; i++){
        if(ni.sfi_direct[i] != 0){
            //해당 block번호 비트맵에서 clear
            bitMapClear(ni.sfi_direct[i]);
			// direct[i]=0? 해줘야하는지 의문
        }
    }
    // cwd entry 하나 지워줘야함
    bitMapClear(sd[b].sfd_ino);

	int count=0;
	for(k=0; k<8; k++)
		if(sd[k].sfd_ino!=0) count++;
	// 빈 디렉토리가 된 경우
    if(count == 0){
        bitMapClear(si.sfi_direct[a]);
    }
	// 아직 남아있는 entry 존재하는 경우
	else{
		sd[b].sfd_ino = SFS_NOINO;
		disk_write(&sd, si.sfi_direct[a]);
	}
    si.sfi_size -= sizeof(struct sfs_dir);
    disk_write(&si, sd_cwd.sfd_ino);
}

void sfs_mv(const char* src_name, const char* dst_name) 
{
	int a, b;
	// 에러처리 1. dst가 이미 존재하는 경우
	if(findName(dst_name, &a, &b)){
		error_message("mv", dst_name, -6);
		return;
	}
	// 에러처리 2. src 존재하지 않는경우
	if(!findName(src_name, &a, &b)){
		error_message("mv", src_name, -1);
		return;
	}

	struct sfs_inode si;
	disk_read( &si, sd_cwd.sfd_ino );
	//buffer for disk read
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];
	disk_read( &sd, si.sfi_direct[a]);

	//for consistency
	//assert(sd[b].sfd_name == src_name);
	strcpy(sd[b].sfd_name, dst_name);

	disk_write(&sd, si.sfi_direct[a]);
}

void sfs_rm(const char* path) 
{
	int i, j, k;
	int a, b;
	// 같은 이름 파일 찾기
	if(!findName(path, &a, &b)){
		error_message("rm", path, -1);
		return;
	}
	struct sfs_inode si, ni;
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];
	bzero(&si, sizeof(struct sfs_inode));
	bzero(&ni, sizeof(struct sfs_inode));
	bzero(&sd, SFS_DENTRYPERBLOCK*sizeof(struct sfs_dir));
	
	disk_read(&si, sd_cwd.sfd_ino);
	disk_read(&sd, si.sfi_direct[a]);

	disk_read(&ni, sd[b].sfd_ino);
	// 지우고자 하는 파일이 디렉토리인 경우
	if(ni.sfi_type == SFS_TYPE_DIR){
		error_message("rm", path, -9);
		return;
	}
	else if(ni.sfi_type != SFS_TYPE_FILE){
		error_message("rm", path, -10);
		return;
	}
	
	// 해당 inode의 direct[i] 데이터 블락 릴리즈
	for(i=0; i<15; i++){
		if(ni.sfi_direct[i] != 0){
			//해당 block번호 비트맵에서 clear
			bitMapClear(ni.sfi_direct[i]);
		}
	}
	// indirect가 있다면
	if(ni.sfi_indirect != 0) {
		struct indirect id;
		bzero(&id, sizeof(struct indirect));
		disk_read(&id, ni.sfi_indirect);
		for(j=0; j<128; j++){
			if(id.sfi_direct[j] != 0)
				bitMapClear(id.sfi_direct[j]);
		}
		bitMapClear(ni.sfi_indirect);
	}

	// 해당 디렉토리 블럭 free
	bitMapClear(sd[b].sfd_ino);

	// 빈 entry라면 하나 지워주자
	int count=0;
	for(k=0; k<8; k++){
		if(sd[k].sfd_ino !=0) {
			count++;
			break;
		}
	}

	// 빈 디렉토리일 경우
	if(count == 0){
		bitMapClear(si.sfi_direct[a]);
	}
	// 디렉토리가 비어있지 않은경우
	else{
		sd[b].sfd_ino = SFS_NOINO;
		disk_write(&sd, si.sfi_direct[a]);
	}

	// startinode 수정해서 디스크쓰기
	si.sfi_size -= sizeof(struct sfs_dir);
	disk_write(&si, sd_cwd.sfd_ino);
}

void sfs_cpin(const char* local_path, const char* path) 
{
	unsigned char buf[512];
    bzero(&buf, SFS_BLOCKSIZE);
	int fd;

    // 파일오픈
    if( (fd = open(path,O_RDONLY))<0 ){
        error_message("cpin", path, -12);
		return;
    }
	int sz_file = lseek(fd, 0, SEEK_END);
	lseek(fd,0,SEEK_SET);
	if(sz_file> SFS_BLOCKSIZE*(SFS_NDIRECT+SFS_DBPERIDB)){
		error_message("cpin", path, -11);
		return;
	}
	int flag;
	int a, b, t;
    int i, j;
    // local_path 파일이 존재하는 경우
    if(findName(local_path, &t, &t)){
        error_message("cpin", local_path, -6);
        return;
    }
	//디렉토리가 꽉 찬경우
	if((flag = findEntry(&a, &b))==0){
        error_message("cpin", path, -3);
        return;
    }
	int MaxBlock = spb.sp_nblocks;
    int bitMapCount = MaxBlock/SFS_BLOCKBITS;
    if(spb.sp_nblocks%SFS_BLOCKBITS>0) bitMapCount++;
    int freeBlock, freeBlock1, freeBlock2, freeBlock3;

    struct sfs_inode si;
    struct sfs_dir sd[SFS_DENTRYPERBLOCK];
    disk_read( &si, sd_cwd.sfd_ino );
    disk_read( &sd, si.sfi_direct[a]);

    //for consistency
    assert( si.sfi_type == SFS_TYPE_DIR );

    // direct[a]의 sd[] dir읽어오기
    disk_read(&sd, si.sfi_direct[a]);
    if((freeBlock = bitMapFind(bitMapCount, MaxBlock))==-1){
        error_message("cpin", local_path, -4);
        return;
    }
	bitMapWrite(freeBlock);

    struct sfs_dir sd1[SFS_DENTRYPERBLOCK];
    bzero(&sd1, SFS_BLOCKSIZE);
	
	if(flag == 2){
        //비트맵 free블록번호(ex, 16) 받아서 direct[a]= 16 넣어준다. -> write(si) 필요
        assert(si.sfi_direct[a] == 0); //0이 아니라면 에러
        //direct[a] entry에 freeBlock번호 부여
        si.sfi_direct[a] = freeBlock;

        // 새 freeBlock 번호 받기
        if((freeBlock1 = bitMapFind(bitMapCount, MaxBlock))==-1){
            error_message("cpin", local_path, -4);
            bitMapClear(freeBlock);
            return;
        }
        bitMapWrite(freeBlock1);
        sd1[0].sfd_ino = freeBlock1;
        strncpy(sd1[0].sfd_name, local_path ,SFS_NAMELEN);
        //16번 블록에 디렉토리 구조체 초기화시켜 만들고
        //0번째 entry에 ino번호, 이름 지정  넣어준다.
        disk_write(&sd1, freeBlock);
    }
	// 빈 entry발견한 경우
    else if(flag == 1){
        //b번쨰 entry에 ino번호, 이름 지정 -> disk_write(sd)필요
        sd[b].sfd_ino = freeBlock;
        strncpy(sd[b].sfd_name,local_path ,SFS_NAMELEN);
        disk_write(&sd, si.sfi_direct[a]);
    }
	
	//inode 구조체 만들어서 size, type, direct[0]에 freeblock 받아서 넣고 디>스크쓰기
	struct sfs_inode newnode;
    bzero(&newnode, sizeof(struct sfs_inode));
    newnode.sfi_type = SFS_TYPE_FILE;
    newnode.sfi_size = 0;
	if(flag ==2) freeBlock = freeBlock1;
    disk_write(&newnode, freeBlock);
	struct indirect id;
	bzero(&id, sizeof(struct indirect));
	
	i=0;
	j=1;
	int n;
	while((n = read(fd, buf, SFS_BLOCKSIZE))>0){
		if(i==0){	
			// 첫 데이터 블럭 write
			if((freeBlock2 = bitMapFind(bitMapCount, MaxBlock))==-1){
    		    error_message("cpin", local_path, -4);
        		bitMapClear(freeBlock);
        		if(flag==2) bitMapClear(freeBlock1);
       			return;
    		}
    		bitMapWrite(freeBlock2);
    		newnode.sfi_direct[i] = freeBlock2;
			newnode.sfi_size += n;
			disk_write(&buf, newnode.sfi_direct[i]);
			//1개의 데이터블록이라도 복사한 경우
			si.sfi_size += sizeof(struct sfs_dir);
			disk_write(&si, sd_cwd.sfd_ino);
    		disk_write(&newnode, freeBlock);
		}
		else if(i<15){
			if((freeBlock3 = bitMapFind(bitMapCount, MaxBlock))==-1){
				error_message("cpin", local_path, -4);
				return;
			}
			bitMapWrite(freeBlock3);
			newnode.sfi_direct[i]=freeBlock3;
			newnode.sfi_size += n;
			disk_write(&buf, newnode.sfi_direct[i]);
    		disk_write(&newnode, freeBlock);
		}
		// indirect 사용
		else if(i==15){
			if((freeBlock2 = bitMapFind(bitMapCount, MaxBlock))==-1){
                error_message("cpin", local_path, -4);
                return;
            }
			bitMapWrite(freeBlock2);
			newnode.sfi_indirect = freeBlock2;
			
			if((freeBlock3 = bitMapFind(bitMapCount, MaxBlock))==-1){
                bitMapClear(freeBlock2);
				error_message("cpin", local_path, -4);
                return;
            }
            bitMapWrite(freeBlock3);
			id.sfi_direct[0] = freeBlock3;
			disk_write(&buf, id.sfi_direct[0]);
			disk_write(&id, freeBlock2);
			newnode.sfi_size += n;
    		disk_write(&newnode, freeBlock);
		}
		// indirect 블록 1개이상일 경우
		else{
			if((freeBlock3 = bitMapFind(bitMapCount, MaxBlock))==-1){
                error_message("cpin", local_path, -4);
                return;
            }
            bitMapWrite(freeBlock3);
			id.sfi_direct[j] = freeBlock3;
			disk_write(&buf, id.sfi_direct[j++]);
			disk_write(&id, newnode.sfi_indirect);
			newnode.sfi_size += n;
    		disk_write(&newnode, freeBlock);
			//꽉찬 경우
			if(j==128){
				break;
			}
		}
		//꽉찬 경우
		if(i++ == 142){
			break;
		}
	}
	if(n==0) return;
	//n<0일경우 에러처리
}

void sfs_cpout(const char* local_path, const char* path) 
{
	int a,b;
	int i,j,k;
	// local_path 파일이 존재하지 않는 경우
	if(!findName(local_path, &a, &b)){
		error_message("cpout", local_path, -1);
		return;
	}

	unsigned char buf[512];
	bzero(&buf, SFS_BLOCKSIZE);
	int fd;

	// 파일오픈
	if( (fd = open(path, O_RDWR | O_CREAT | O_EXCL ,0700))<0 ){
		error_message("cpout", path, -6);
		return;
	}
	struct sfs_inode si, ni;
    struct sfs_dir sd[SFS_DENTRYPERBLOCK];
    bzero(&si, sizeof(struct sfs_inode));
    bzero(&ni, sizeof(struct sfs_inode));
    bzero(&sd, SFS_DENTRYPERBLOCK*sizeof(struct sfs_dir));

    disk_read(&si, sd_cwd.sfd_ino);
    disk_read(&sd, si.sfi_direct[a]);

    disk_read(&ni, sd[b].sfd_ino);
	if(ni.sfi_type != SFS_TYPE_FILE){
        error_message("cpout", local_path, -10);
        return;
    }
	int sz_file = ni.sfi_size;
	int n=0;
	// direct[i]에 있는 파일 블록들 write
	for(i=0; i<15; i++){
		if(ni.sfi_direct[i]!=0){
			disk_read(&buf, ni.sfi_direct[i]);
			n += write(fd, buf, MIN(SFS_BLOCKSIZE, (sz_file - n)));
		}
	}
	// indirect에 있는 파일 블록들 write
	if(ni.sfi_indirect != 0) {
        struct indirect id;
        bzero(&id, sizeof(struct indirect));
        disk_read(&id, ni.sfi_indirect);
        for(j=0; j<128; j++){
            if(id.sfi_direct[j] != 0)
				disk_read(&buf, ni.sfi_direct[j]);
				n += write(fd, buf, MIN(sizeof(buf),(sz_file - n)));
        }
    }
	close(fd);
}

void dump_inode(struct sfs_inode inode) {
	int i;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];

	printf("size %d type %d direct ", inode.sfi_size, inode.sfi_type);
	for(i=0; i < SFS_NDIRECT; i++) {
		printf(" %d ", inode.sfi_direct[i]);
	}
	printf(" indirect %d",inode.sfi_indirect);
	printf("\n");

	if (inode.sfi_type == SFS_TYPE_DIR) {
		for(i=0; i < SFS_NDIRECT; i++) {
			if (inode.sfi_direct[i] == 0) break;
			disk_read(dir_entry, inode.sfi_direct[i]);
			dump_directory(dir_entry);
		}
	}

}

void dump_directory(struct sfs_dir dir_entry[]) {
	int i;
	struct sfs_inode inode;
	for(i=0; i < SFS_DENTRYPERBLOCK;i++) {
		printf("%d %s\n",dir_entry[i].sfd_ino, dir_entry[i].sfd_name);
		disk_read(&inode,dir_entry[i].sfd_ino);
		if (inode.sfi_type == SFS_TYPE_FILE) {
			printf("\t");
			dump_inode(inode);
		}
	}
}

void sfs_dump() {
	// dump the current directory structure
	struct sfs_inode c_inode;

	disk_read(&c_inode, sd_cwd.sfd_ino);
	printf("cwd inode %d name %s\n",sd_cwd.sfd_ino,sd_cwd.sfd_name);
	dump_inode(c_inode);
	printf("\n");

}
