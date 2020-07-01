
#ifndef WIN32
#include <usb_log.h>
#include <includes.h>
#include "lock.h"
#include <sys/mount.h>
#include <sys/un.h> 
#include <linux/types.h> 
#include <linux/netlink.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/vfs.h> 
#include <netinet/in.h> 
#include <Language.h>
#include <logbookdata.h>
#include "trenddata.h"
#include <stdbool.h>
#include <VersionInfoDialog.h>
#include <errno.h>

#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


int flag_usbismount = 0;
char flag_usb=0;
static bool ISmountsucc = FALSE;
extern char LogVersion[24];
extern char LogSysCfgSN[16];
extern char LogSysCfgPWD[6];

int my_system(char* cmd) 
{ 
    int status = 0; 
    pid_t pid; 

    if ((pid = vfork()) <0) 
    {  
        status = -1; 
    } 
    else if (pid==0) 
    { 
        const char *new_argv[4]; 
        struct sigaction sa_cld; 
        sa_cld.sa_handler = SIG_DFL; 
        sa_cld.sa_flags = 0; 

        /* 在子进程中放开SIGINT信号 */ 
        sigemptyset(&sa_cld.sa_mask); 
        sigaction (SIGINT, &sa_cld, NULL); 
        sigaction (SIGQUIT, &sa_cld, NULL); 
      
        new_argv[0] = "sh"; 
        new_argv[1] = "-c"; 
        new_argv[2] = cmd; 
        new_argv[3] = NULL; 

        //execl("/bin/sh","sh","-c" ,cmd,(char *)0);
        if (execve("/bin/sh",(char *const *) new_argv, NULL) <0) 
        { 
            printf("fail to execve %s! errno: %d\n",cmd); 
            exit(1); 
        } 
        else 
        { 
            exit(0); 
        } 
    } 
    else 
    { 
        waitpid(pid,&status,0); 
    } 
    return status; 
}

//查看文件或文件夹是否存在
//返回值:TRUE:存在,FALSE:不存在
bool isFileExist(const char *FileName)
{
	if(FileName == NULL )return FALSE;
	if(access(FileName,F_OK)==0)  // access(filename, 0) == 0 //存在返0否返-1
	{
		return TRUE;
	}
	return FALSE;
}


//新建一文件夹
int CreateFolder(const char *directory)
{
	char dire[50];
	memset(dire,0x0,sizeof(dire));
	strcpy(dire,directory);
	if(mkdir(dire,0777)==-1)
	{
		return -1;
	}
	return 0;
}


//删除一文件
int RemoveFile(char *filename)
{
    if(filename == NULL)return -1;
 
    if(remove(filename) == -1)
    {
        return -1;
    }
    return 0;
}

int UMountUSB()
{
   // 将 新建的目录删除 只能删除非空的目录
    if(!umount(UPDATA_DIR1))
    {
		printf(" umount  %s success \n",UPDATA_DIR1);
		return 0;
	}
	
	return 1;
}

int MountUsb(char *p)
{
	int i=0,result=0,flag=1;
	char syscmd[50];
	
	if(isFileExist(UPDATA_DIR1) != 1) 
    {
    	CreateFolder(UPDATA_DIR1); 
    }

    //printf("p = %s \n",p);
	//"mount -t vfat /dev/sda /tmp/usb"
	//sprintf(syscmd, "mount -t vfat %s %s",p,UPDATA_DIR1);
	//result = my_system(syscmd);
	result = mount(p,"/media/usb-sda1","vfat",0,0); // "FAT"
	if(0 == result) // 挂载成功
    {	
    	ISmountsucc = TRUE;
        if(isFileExist(UPDATA_DIR0) != 1) // U盘中没有probe这个文件夹则创建一个
        {
        	if(!CreateFolder(UPDATA_DIR0))
        	{
				printf("create %s  success \n",UPDATA_DIR0);
        	}
        }
		printf("  mount  %s is successful \n",p);
		flag = 0;				
        return 1;
    }
	if(flag)
    {
    	//printf(" mount is failed \n");
    	ISmountsucc = FALSE;
        umount("/media/usb-sda1");
    }
    return 0;
}
#if 1
#define BUFFER_SIZE 1024
int CopyFile(char *from,char *to)
{
   int from_fd,to_fd;
   int BytesRead,BytesWrite;
   char buffer[BUFFER_SIZE];
   char *ptr;
   int err=0;
   
	printf("COPY ::%s To %s\n",from,to);
   /* 打开源文件 */
   printf("--open src----\r\n");
   if((from_fd=open(from,O_RDONLY))==-1)   /*open file readonly,返回-1表示出错，否则返回文件描述符*/
   {
     printf(stderr,"Open %s Error:%s\n",from,strerror(errno));
     //exit(1);
	 return -1;
   }

   /* 创建目的文件 */
   /* 使用了O_CREAT选项-创建文件,open()函数需要第3个参数,
      mode=S_IRUSR|S_IWUSR表示S_IRUSR 用户可以读 S_IWUSR 用户可以写*/
      printf("--open des----\r\n");
   if((to_fd=open(to,O_WRONLY|O_CREAT|O_TRUNC,0777))==-1) 
   {
        printf("Open %s Error:%s\n",to,strerror(errno));
        //exit(1);
        close(from_fd);
        return -1;
   }
   
   /* 以下代码是一个经典的拷贝文件的代码 */
   while(BytesRead=read(from_fd,buffer,BUFFER_SIZE))
   {
     /* 一个致命的错误发生了 */
     if((BytesRead==-1)&&(errno!=EINTR)) 
     {
		 err=-1;
         break;
     }
     else if(BytesRead>0)
     {
       ptr=buffer;
       while(BytesWrite=write(to_fd,ptr,BytesRead))
       {
            /* 一个致命错误发生了 */
            if((BytesWrite==-1)&&(errno!=EINTR))
            {
		 		err=-1;
                break;
            }
            /* 写完了所有读的字节 */
            else if(BytesWrite==BytesRead) 
            {
		 		err=0;
                break;
            }
            /* 只写了一部分,继续写 */
            else if(BytesWrite>0)
            {
                ptr+=BytesWrite;
                BytesRead-=BytesWrite;
            }
       }
       
       /* 写的时候发生的致命错误 */
       if(BytesWrite==-1)
       {
		   err=-1;
           break;
       }
     }
   }
   close(from_fd);
   close(to_fd);
   sync();

   return err;
}
#endif

void check_usb(void)
{
	char uname[16][10]={"/dev/sda","/dev/sdb","/dev/sdc","/dev/sdd","/dev/sde","/dev/sdf","/dev/sdg","/dev/sdh","/dev/sdi","/dev/sdj","/dev/sdk","/dev/sdl","/dev/sdm","/dev/sdn","/dev/sdo","/dev/sdp"};
	char uname1[16][10]={"/dev/sda1","/dev/sdb1","/dev/sdc1","/dev/sdd1","/dev/sde1","/dev/sdf1","/dev/sdg1","/dev/sdh1","/dev/sdi1","/dev/sdj1","/dev/sdk1","/dev/sdl1","/dev/sdm1","/dev/sdn1","/dev/sdo1","/dev/sdp1"};
    int i=0;
	static char save_dev[10]={0},unameFlag=0;

	if((flag_usbismount==1)&&isFileExist(UPDATA_DIR0))// 挂上且目录存在则返回
	{
        unameFlag = 0;
		return ;
	}

	for(i=0;i<26;i++)   //unameFlag  用来解决有些U盘有两个分区有些只有一个分区 一个挂不上，则跳过挂载另外一个
	{
		if(isFileExist(uname1[i]) && unameFlag)
        {
             //printf("uname1 %s\n",uname1[i]);
			 flag_usb = 1;
			 flag_usbismount = 0;
			 strcpy(save_dev,uname[i]);
			 break;
        }
		else if(isFileExist(uname[i]))
		{
             //printf("uname %s\n",uname[i]);
			 flag_usb = 1;
			 flag_usbismount = 0;
			 unameFlag = 0;
			 strcpy(save_dev,uname[i]);
			 break;
		}
		else
		{
			 flag_usb = 0;
			 flag_usbismount = 0;
			 ISmountsucc = 0;
             usleep(2000);
		}
	}
	
	if(flag_usb == 1)//检测USB是否插上
	{
		UMountUSB();// 卸载0成功 1失败MountUsb();  关机操作前要先卸载，开机后才重新挂载

		usleep(2000);
        if(unameFlag)
        {
            flag_usbismount = MountUsb(uname1[i]); // 1成功 0失败
        }
		else
		{
            flag_usbismount = MountUsb(uname[i]); // 1成功 0失败
		}
	
		if(flag_usbismount == 1)
		{
			usleep(50);  
		}
		else
		{
			UMountUSB();// 卸载0成功 1失败
			flag_usbismount = 0;
            unameFlag = 1;
		}
	}
}

