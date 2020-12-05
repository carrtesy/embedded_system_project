#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
int main(int argc,char* argv[]){
        int opt, gpio13, gpio19, gpio26, read;
        gpio13 = 0;
        gpio19 = 0;
        gpio26 = 0;
        read = 0;

        printf("INIT\n");
        while((opt =getopt(argc,argv,"r:g:b:k"))!=-1){
                switch(opt){
                        case'r':
                                gpio26 = atoi(optarg);
                                break;
                        case'g':
                                gpio19 = atoi(optarg);
                                break;
                        case'b':
                                gpio13 = atoi(optarg);
                                break;
                        case'k':
                                read = 1;
                                break;
                        default:
                                return -1;
                }
        }


        printf("READ\n");
        printf("optargs: 26- %d, 19 -%d, 13- %d\n", gpio26, gpio19, gpio13);
        int fd,res;
        fd=open("/dev/rpikey",O_RDWR);
        if( fd < 0 ){
                printf("Can't open device\n");
                return -1;
        }
        if( read ){
                uint32_t values[2];
                res = ioctl(fd,100,values);
                printf("value 20:%d 21:%d\n",values[0],values[1]);
        } else{
                uint32_t values[3];
                values[0]=gpio13;
                values[1]=gpio19;
                values[2]=gpio26;
                res =ioctl(fd,101,values);
        }
        close(fd);
        return 0;
}
