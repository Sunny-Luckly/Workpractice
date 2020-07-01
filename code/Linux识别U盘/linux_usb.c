
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

        /* ���ӽ����зſ�SIGINT�ź� */ 
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

//�鿴�ļ����ļ����Ƿ����
//����ֵ:TRUE:����,FALSE:������
bool isFileExist(const char *FileName)
{
	if(FileName == NULL )return FALSE;
	if(access(FileName,F_OK)==0)  // access(filename, 0) == 0 //���ڷ�0��-1
	{
		return TRUE;
	}
	return FALSE;
}


//�½�һ�ļ���
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


//ɾ��һ�ļ�
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
   // �� �½���Ŀ¼ɾ�� ֻ��ɾ���ǿյ�Ŀ¼
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
	if(0 == result) // ���سɹ�
    {	
    	ISmountsucc = TRUE;
        if(isFileExist(UPDATA_DIR0) != 1) // U����û��probe����ļ����򴴽�һ��
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
   /* ��Դ�ļ� */
   printf("--open src----\r\n");
   if((from_fd=open(from,O_RDONLY))==-1)   /*open file readonly,����-1��ʾ�������򷵻��ļ�������*/
   {
     printf(stderr,"Open %s Error:%s\n",from,strerror(errno));
     //exit(1);
	 return -1;
   }

   /* ����Ŀ���ļ� */
   /* ʹ����O_CREATѡ��-�����ļ�,open()������Ҫ��3������,
      mode=S_IRUSR|S_IWUSR��ʾS_IRUSR �û����Զ� S_IWUSR �û�����д*/
      printf("--open des----\r\n");
   if((to_fd=open(to,O_WRONLY|O_CREAT|O_TRUNC,0777))==-1) 
   {
        printf("Open %s Error:%s\n",to,strerror(errno));
        //exit(1);
        close(from_fd);
        return -1;
   }
   
   /* ���´�����һ������Ŀ����ļ��Ĵ��� */
   while(BytesRead=read(from_fd,buffer,BUFFER_SIZE))
   {
     /* һ�������Ĵ������� */
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
            /* һ�������������� */
            if((BytesWrite==-1)&&(errno!=EINTR))
            {
		 		err=-1;
                break;
            }
            /* д�������ж����ֽ� */
            else if(BytesWrite==BytesRead) 
            {
		 		err=0;
                break;
            }
            /* ֻд��һ����,����д */
            else if(BytesWrite>0)
            {
                ptr+=BytesWrite;
                BytesRead-=BytesWrite;
            }
       }
       
       /* д��ʱ�������������� */
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

	if((flag_usbismount==1)&&isFileExist(UPDATA_DIR0))// ������Ŀ¼�����򷵻�
	{
        unameFlag = 0;
		return ;
	}

	for(i=0;i<26;i++)   //unameFlag  ���������ЩU��������������Щֻ��һ������ һ���Ҳ��ϣ���������������һ��
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
	
	if(flag_usb == 1)//���USB�Ƿ����
	{
		UMountUSB();// ж��0�ɹ� 1ʧ��MountUsb();  �ػ�����ǰҪ��ж�أ�����������¹���

		usleep(2000);
        if(unameFlag)
        {
            flag_usbismount = MountUsb(uname1[i]); // 1�ɹ� 0ʧ��
        }
		else
		{
            flag_usbismount = MountUsb(uname[i]); // 1�ɹ� 0ʧ��
		}
	
		if(flag_usbismount == 1)
		{
			usleep(50);  
		}
		else
		{
			UMountUSB();// ж��0�ɹ� 1ʧ��
			flag_usbismount = 0;
            unameFlag = 1;
		}
	}
}

