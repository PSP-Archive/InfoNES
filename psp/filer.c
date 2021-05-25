#include <stdio.h>
#include <string.h>
#include "pg.h"
#include "zlibInterface.h"

struct dirent files[MAX_ENTRY];
struct dirent *sortfiles[MAX_ENTRY];
int nfiles;

struct dirent z_files[MAX_ENTRY];
struct dirent *z_sortfiles[MAX_ENTRY];
int z_nfiles;

typedef struct
{
	unsigned long color[4];
} SETTING;
enum{
	DEF_COLOR0=0x9063,
	DEF_COLOR1=RGB(85,85,95),
	DEF_COLOR2=RGB(105,105,115),
	DEF_COLOR3=0xffff,
};
SETTING setting;

char path_inzip[MAX_PATH];

int stricmp(const char *str1, const char *str2)
{
	char c1, c2;
	for(;;){
		c1 = *str1;
		if(c1>=0x61 && c1<=0x7A) c1-=0x20;
		c2 = *str2;
		if(c2>=0x61 && c2<=0x7A) c2-=0x20;
		
		if(c1!=c2)
			return 1;
		else if(c1==0)
			return 0;
		
		str1++; str2++;
	}
}

void rin_frame(const char *msg0, const char *msg1)
{
	//if(bBitmap)
	//	pgBitBlt(0,0,480,272,1,bgBitmap);
	//else
		pgFillvram(setting.color[0]);
	mh_print(345, 0, "  ■ InfoNES Ver0.95 ■", setting.color[1]);
	// メッセージなど
	if(msg0) mh_print(17, 14, msg0, setting.color[2]);
	pgDrawFrame(17,25,463,248,setting.color[1]);
	pgDrawFrame(18,26,462,247,setting.color[1]);
	// 操作説明
	if(msg1) mh_print(17, 252, msg1, setting.color[2]);
}

// 拡張子管理用
enum {
	EXT_NES,
	EXT_UNKNOWN
};
const struct {
	char *szExt;
	int nExtId;
} stExtentions[] = {
 "nes",EXT_NES,
 NULL, EXT_UNKNOWN
};
int getExtId(const char *szFilePath) {
	char *pszExt;
	int i;
	if((pszExt = strrchr(szFilePath, '.'))) {
		pszExt++;
		for (i = 0; stExtentions[i].nExtId != EXT_UNKNOWN; i++) {
			if (!stricmp(stExtentions[i].szExt,pszExt)) {
				return stExtentions[i].nExtId;
			}
		}
	}
	return EXT_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////
// クイックソート
// AC add start
void SJISCopy(struct dirent *a, unsigned char *file)
{
	unsigned char ca;
	int i;
	int len=strlen(a->name);
	
	for(i=0;i<=len;i++){
		ca = a->name[i];
		if (((0x81 <= ca)&&(ca <= 0x9f))
		|| ((0xe0 <= ca)&&(ca <= 0xef))){
			file[i++] = ca;
			file[i] = a->name[i];
		}
		else{
			if(ca>='a' && ca<='z') ca-=0x20;
			file[i] = ca;
		}
	}

}
int cmpFile(struct dirent *a, struct dirent *b)
{
    unsigned char file1[0x108];
    unsigned char file2[0x108];
	unsigned char ca, cb;
	int i, n, ret;

	if(a->type==b->type){
		SJISCopy(a, file1);
		SJISCopy(b, file2);
		n=strlen((char*)file1);
		for(i=0; i<=n; i++){
			ca=file1[i]; cb=file2[i];
			ret = ca-cb;
			if(ret!=0) return ret;
		}
		return 0;
	}
	
	if(a->type & TYPE_DIR)	return -1;
	else					return 1;
}
// AC add end

void sort_files(struct dirent **a, int left, int right) {
	struct dirent *tmp, *pivot;
	int i, p;
	
	if (left < right) {
		pivot = a[left];
		p = left;
		for (i=left+1; i<=right; i++) {
			if (cmpFile(a[i],pivot)<0){
				p=p+1;
				tmp=a[p];
				a[p]=a[i];
				a[i]=tmp;
			}
		}
		a[left] = a[p];
		a[p] = pivot;
		sort_files(a, left, p-1);
		sort_files(a, p+1, right);
	}
}
////////////////////////////////////////////////////////////////////////

void getDir(const char *path) {
	int fd, b=0;
	char *p;
	
	nfiles = 0;
	
	if(strcmp(path,"ms0:/")){
		strcpy(files[nfiles].name,"..");
		files[nfiles].type = TYPE_DIR;
		sortfiles[nfiles] = files + nfiles;
		nfiles++;
		b=1;
	}
	
	fd = sceIoDopen(path);
	while(nfiles<MAX_ENTRY){
		memset(&files[nfiles], 0x00, sizeof(struct dirent));
		if(sceIoDread(fd, &files[nfiles])<=0) break;
		if(files[nfiles].name[0] == '.') continue;
		if(files[nfiles].type == TYPE_DIR){
			strcat(files[nfiles].name, "/");
			sortfiles[nfiles] = files + nfiles;
			nfiles++;
		}else if(getExtId(files[nfiles].name) != EXT_UNKNOWN){
			sortfiles[nfiles] = files + nfiles;
			nfiles++;
		}
	}
	sceIoDclose(fd);
	if(b)
		sort_files(sortfiles+1, 0, nfiles-2);
	else
		sort_files(sortfiles, 0, nfiles-1);
}

char filer_msg[256]={0};
char lastpath[256];
int getFilePath(char *out)
{
	setting.color[0] = DEF_COLOR0;
	setting.color[1] = DEF_COLOR1;
	setting.color[2] = DEF_COLOR2;
	setting.color[3] = DEF_COLOR3;
	
	unsigned long color;
	static int sel=0;
	int top, rows=21, x, y, h, i, len, up=0, inzip=0, oldDirType;
	char path[MAX_PATH], oldDir[MAX_NAME], tmp[MAX_PATH*2], *p;
	
	top = sel-3;
	strcpy(path, lastpath);
	path_inzip[0]=0;
	
	getDir(path);
	for(;;){
		readpad();
		if(new_pad)
			filer_msg[0]=0;
		if(new_pad & CTRL_CIRCLE){
			if(sortfiles[sel]->type == TYPE_DIR){
				if(!strcmp(sortfiles[sel]->name,"..")){
					up=1;
				}else{
					if(inzip){
						strcat(path_inzip,sortfiles[sel]->name);
						getZipDir(path_inzip);
					}else{
						strcat(path,sortfiles[sel]->name);
						getDir(path);
					}
					sel=0;
				}
			}else{
				if(!inzip){
					strcpy(tmp,path);
					strcat(tmp,sortfiles[sel]->name);
					break;
				}else
					break;
			}
		}else if(new_pad & CTRL_CROSS){
			return 0;
		}else if(new_pad & CTRL_TRIANGLE){
			up=1;
		}else if(new_pad & CTRL_UP){
			sel--;
		}else if(new_pad & CTRL_DOWN){
			sel++;
		}else if(new_pad & CTRL_LEFT){
			sel-=rows/2;
		}else if(new_pad & CTRL_RIGHT){
			sel+=rows/2;
		}
		
		if(up){
			oldDir[0]=0;
			oldDirType = TYPE_DIR;
			if(inzip){
				if(path_inzip[0]==0){
					oldDirType = TYPE_FILE;
					inzip=0;
				}else{
					path_inzip[strlen(path_inzip)-1]=0;
					p=strrchr(path_inzip,'/');
					if(p) p++;
					else  p=path_inzip;
					strcpy(oldDir,p);
					strcat(oldDir,"/");
					*p=0;
					getZipDir(path_inzip);
					sel=0;
				}
			}
			if(strcmp(path,"ms0:/") && !inzip){
				if(oldDirType==TYPE_DIR)
					path[strlen(path)-1]=0;
				p=strrchr(path,'/')+1;
				strcpy(oldDir,p);
				if(oldDirType==TYPE_DIR)
					strcat(oldDir,"/");
				*p=0;
				getDir(path);
				sel=0;
			}
			for(i=0; i<nfiles; i++) {
				if(oldDirType==sortfiles[i]->type
				&& !strcmp(oldDir, sortfiles[i]->name)) {
					sel=i;
					top=sel-3;
					break;
				}
			}
			up=0;
		}
		
		if(top > nfiles-rows)	top=nfiles-rows;
		if(top < 0)				top=0;
		if(sel >= nfiles)		sel=nfiles-1;
		if(sel < 0)				sel=0;
		if(sel >= top+rows)		top=sel-rows+1;
		if(sel < top)			top=sel;
		
		if(inzip){
			strcpy(tmp,strrchr(path,'/')+1);
			strcat(tmp,":/");
			strcat(tmp,path_inzip);
			rin_frame(tmp,"○：OK  ×：Cancel  △：UP");
		}else
			rin_frame(filer_msg[0]?filer_msg:path,"○：OK  ×：Cancel  △：UP   SELECT：Remove");
		
		// スクロールバー
		if(nfiles > rows){
			h = 219;
			pgDrawFrame(445,25,446,248,setting.color[1]);
			pgFillBox(448, h*top/nfiles + 27,
				460, h*(top+rows)/nfiles + 27,setting.color[1]);
		}
		
		x=28; y=32;
		for(i=0; i<rows; i++){
			if(top+i >= nfiles) break;
			if(top+i == sel) color = setting.color[2];
			else			 color = setting.color[3];
			mh_print(x, y, sortfiles[top+i]->name, color);
			y+=10;
		}
		
		pgScreenFlipV();
	}
	
	strcpy(out, path);
	strcat(inzip?path_inzip:out, sortfiles[sel]->name);
	strcpy(lastpath, path);
	return 1;
}
