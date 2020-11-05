#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#define SSD1306_I2C_DEV   0x3C
void ssd1306_command(int i2c_fd,uint8_t cmd){
    uint8_t buffer[2];
    buffer[0]=(0<<7)|(0<<6);//Co = 0, D/C# = 0
    buffer[1]=cmd;

    if(write(i2c_fd,buffer,2)!=2){
        printf("i2c write failed!\n");
    }
}

void ssd1306_Init(int i2c_fd){
    ssd1306_command(i2c_fd,0xA8);//Set Mux Ratio
    ssd1306_command(i2c_fd,0x3f);
    ssd1306_command(i2c_fd,0xD3);//Set Display Offset
    ssd1306_command(i2c_fd,0x00);
    ssd1306_command(i2c_fd,0x40);//Set Display Start Line
    ssd1306_command(i2c_fd,0xA0);//Set Segment re-map //0xA1 for vertical inversion
    ssd1306_command(i2c_fd,0xC0);//Set COM Output Scan Direction //0xC8 for horizontal inversion
    ssd1306_command(i2c_fd,0xDA);//Set COM Pins hardware configuration
    ssd1306_command(i2c_fd,0x12);//Manual says 0x2, but 0x12 is required
    ssd1306_command(i2c_fd,0x81);//Set Contrast Controlssd1306_command(i2c_fd,0x7F);//0:min, 0xFF:max
    ssd1306_command(i2c_fd,0xA4);//Disable Entire Display On
    ssd1306_command(i2c_fd,0xA6);//Set Normal Display
    ssd1306_command(i2c_fd,0xD5);//Set OscFrequency
    ssd1306_command(i2c_fd,0x80);
    ssd1306_command(i2c_fd,0x8D);//Enable charge pump regulator
    ssd1306_command(i2c_fd,0x14);
    ssd1306_command(i2c_fd,0xAF);//Display ON
}

void ssd1306_data(int i2c_fd, const uint8_t* data, size_t size){
    uint8_t* buffer = (uint8_t*) malloc(size+1);
    
    buffer[0] = (0<<7) | (1<<6); // Co = 0, D/C# = 1
    memcpy(buffer+1, data, size);
    
    if(write(i2c_fd, buffer, size+1) != size+1){
        printf("i2c write failed\n");
    }

    free(buffer);
}

int main(){
    int i2c_fd =open("/dev/i2c-1",O_RDWR);
    if(i2c_fd <0){
        printf("err opening device\n");
        return -1;
    }
    
    if(ioctl(i2c_fd,I2C_SLAVE,SSD1306_I2C_DEV)<0){
        printf("err setting i2c slave address\n");
        return -1;
    }
    
    ssd1306_Init(i2c_fd);

    uint8_t data[3];
    data[0] = 0xFF;
    data[1] = 0x0F;
    data[2] = 0x00;

    while(1){
        ssd1306_data(i2c_fd, data, 3);
        usleep(100000);
    }

    close(i2c_fd);
    return 0;
}
