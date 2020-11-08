#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <signal.h>
#include "cat.h"
#include "logo.h"
#include "./fonts/font.h"

/* configurations */
#define SSD1306_I2C_DEV   0x3C
#define S_WIDTH 128
#define S_HEIGHT 64
#define S_PAGES (S_HEIGHT/8)

/* cat */
#define CAT_WIDTH 70
#define CAT_HEIGHT 6
#define CAT_MOVE 4
#define NUM_FRAMES (S_WIDTH/CAT_MOVE)
#define CAT_Y_LOC 1

/* logo */
#define LOGO_WIDTH 56
#define LOGO_HEIGHT 6

/* font */
#define FONT_WIDTH 5
#define FONT_HEIGHT 1

/* others */
#define DEBUG 0

/* ssd1306 APIs */
void ssd1306_command(int i2c_fd,uint8_t cmd){
    uint8_t buffer[2];
    buffer[0]=(0<<7)|(0<<6);//Co = 0, D/C# = 0
    buffer[1]=cmd;

    if(write(i2c_fd,buffer,2)!=2){
        printf("i2c write failed!\n");
    }
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

/* update display */
void update_full(int i2c_fd, uint8_t* data){
    ssd1306_command(i2c_fd, 0x20); // addressing mode
    ssd1306_command(i2c_fd, 0x0); // horizontal addressing mode
    
    ssd1306_command(i2c_fd, 0x21); // set column start/end address
    ssd1306_command(i2c_fd, 0x0); 
    ssd1306_command(i2c_fd, S_WIDTH - 1);

    ssd1306_command(i2c_fd, 0x22); // set page start/end address
    ssd1306_command(i2c_fd, 0x0); 
    ssd1306_command(i2c_fd, S_PAGES - 1);

    ssd1306_data(i2c_fd, data, S_WIDTH * S_PAGES);
}

void update_area(int i2c_fd, const uint8_t* data, int x, int y, int x_len, int y_len){
    ssd1306_command(i2c_fd, 0x20); // addressing mode
    ssd1306_command(i2c_fd, 0x0); // horizontal addressing mode
    
    ssd1306_command(i2c_fd, 0x21); // set column start/end address
    ssd1306_command(i2c_fd, x); 
    ssd1306_command(i2c_fd, x + x_len - 1);

    ssd1306_command(i2c_fd, 0x22); // set page start/end address
    ssd1306_command(i2c_fd, y); 
    ssd1306_command(i2c_fd, y + y_len - 1);

    ssd1306_data(i2c_fd, data, x_len * y_len);

}

void update_area_x_wrap(int i2c_fd, const uint8_t* data, int x, int y, int x_len, int y_len){
    if(x + x_len <= S_WIDTH) update_area(i2c_fd, data, x, y, x_len, y_len);
    else {
        int part1_len = S_WIDTH - x;
        int part2_len = x_len - part1_len;

        uint8_t* part1_buf = (uint8_t *)malloc(part1_len * y_len);
        uint8_t* part2_buf = (uint8_t *)malloc(part2_len * y_len);
        
        for(int x = 0; x < part1_len; x++){
            for(int y = 0; y < y_len; y++){
                part1_buf[part1_len * y + x] = data[x_len * y + x];
            }
        }

        for(int x = 0; x < part2_len; x++){
            for(int y = 0; y < y_len; y++){
                part2_buf[part2_len * y + x] = data[x_len * y + part1_len + x];
            }
        }

        update_area(i2c_fd, part1_buf, x, y, part1_len, y_len);
        update_area(i2c_fd, part2_buf, 0, y, part2_len, y_len);
        
        free(part1_buf);
        free(part2_buf);
    }
}

void update_bitwise_vertical(int i2c_fd, const uint8_t* data, int bitwise_delay_in_microsec,
                             int x, int y, int x_len, int y_len){
    uint8_t* tmp = (uint8_t*)malloc(x_len * y_len);

    // bitwise update from above
    for(int _y = 0; _y < y_len; _y++){
        for(int bit = 0; bit < 8; bit++){
            int mask = (2<<bit) - 1;
            // original data is masked from LSB
            for(int _x = 0; _x < x_len; _x++){
                tmp[x_len * _y + _x] = (data[x_len * _y + _x]) & mask;
            }        
            update_area(i2c_fd, tmp + x_len * _y, x, _y, x_len, 1);
            usleep(bitwise_delay_in_microsec);
        }
    }

    free(tmp);
}


/* write fonts */
void write_char(int i2c_fd, char c, int x, int y){
    if(c < ' '){
        c = ' ';
    }

    update_area(i2c_fd, font[c - ' '], x, y, FONT_WIDTH, FONT_HEIGHT); 
}

void write_str(int i2c_fd, char* str, int x, int y){
    char c;
    while(c = *str++){
        write_char(i2c_fd, c, x, y);
        x += FONT_WIDTH;
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
    ssd1306_command(i2c_fd,0x81);//Set Contrast Control
    ssd1306_command(i2c_fd,0x7F);//0:min, 0xFF:max
    ssd1306_command(i2c_fd,0xA4);//Disable Entire Display On
    ssd1306_command(i2c_fd,0xA6);//Set Normal Display
    ssd1306_command(i2c_fd,0xD5);//Set OscFrequency
    ssd1306_command(i2c_fd,0x80);
    ssd1306_command(i2c_fd,0x8D);//Enable charge pump regulator
    ssd1306_command(i2c_fd,0x14);
    ssd1306_command(i2c_fd,0xAF);//Display ON
}

int i2c_fd;
uint8_t *data_s;
int with_robot;

void cat_walking(int sig){
    static int i = 0;
    const unsigned char * cats[4] = {cat1, cat2, cat3, cat4};

    // fixed frame rate
    int ROBOT = with_robot? 10: 0;
    data_s = (uint8_t*)malloc((CAT_WIDTH + CAT_MOVE + ROBOT) * CAT_HEIGHT);
    
    // update data
    for(int y = 0; y < CAT_HEIGHT; y++){
        for(int x = 0; x < CAT_MOVE; x++){
            data_s[(CAT_WIDTH + CAT_MOVE + ROBOT) * y + x] = 0x0;
        }

        // frame by frame update
        for(int x = 0; x < CAT_WIDTH; x++){
            data_s[(CAT_WIDTH + CAT_MOVE + ROBOT) * y + (x + CAT_MOVE)] = 
                                cats[(i/CAT_MOVE)%4][CAT_WIDTH * y + x];
        }

        for(int x = 0; x < ROBOT; x++){
            data_s[(CAT_WIDTH + CAT_MOVE + ROBOT) * y + (x + CAT_WIDTH + CAT_MOVE)] = 0x0;
        }
    }

    
    // display robot
    if(with_robot){
        data_s[(CAT_WIDTH + CAT_MOVE + ROBOT) * (CAT_HEIGHT-1) + (CAT_WIDTH + ROBOT + CAT_MOVE - 1)] = 0x03;
        data_s[(CAT_WIDTH + CAT_MOVE + ROBOT) * (CAT_HEIGHT-1) + (CAT_WIDTH + ROBOT + CAT_MOVE - 2)] = 0x0B;
        data_s[(CAT_WIDTH + CAT_MOVE + ROBOT) * (CAT_HEIGHT-1) + (CAT_WIDTH + ROBOT + CAT_MOVE - 3)] = 0x1F;
        data_s[(CAT_WIDTH + CAT_MOVE + ROBOT) * (CAT_HEIGHT-1) + (CAT_WIDTH + ROBOT + CAT_MOVE - 4)] = 0x0B;
        data_s[(CAT_WIDTH + CAT_MOVE + ROBOT) * (CAT_HEIGHT-1) + (CAT_WIDTH + ROBOT + CAT_MOVE - 5)] = 0x03;
    }

    // update data to display
    update_area_x_wrap(i2c_fd, data_s, i, CAT_Y_LOC, CAT_WIDTH + CAT_MOVE + ROBOT, CAT_HEIGHT);
    free(data_s);
    
    i+=CAT_MOVE;
    if(i >= S_WIDTH) i = 0;
}


int main(){
    /* init hardware */ 
    i2c_fd =open("/dev/i2c-1",O_RDWR);
    
    if(i2c_fd <0){
        printf("err opening device\n");
        return -1;
    }
    
    if(ioctl(i2c_fd,I2C_SLAVE,SSD1306_I2C_DEV)<0){
        printf("err setting i2c slave address\n");
        return -1;
    }
    
    ssd1306_Init(i2c_fd);

    /* initialize with black background */
    uint8_t* data = (uint8_t*) malloc (S_WIDTH * S_PAGES);
    memset(data, 0, sizeof(uint8_t) * S_WIDTH * S_PAGES);
    update_full(i2c_fd, data);
    printf("Background Initialized with zeros\n");
    
    /* Loading: cat walking */
    int interval = 200000;
    with_robot = 0;
    signal(SIGALRM, cat_walking);
    ualarm(interval, interval); // SIGALRM generated with interval
    int TIME_REMAIN = 10 * 1000000 / interval; // x seconds to finish loading
    
    // write string
    write_str(i2c_fd, " LOADING... ", 34, S_PAGES - 1);
    
    while(TIME_REMAIN--){
        sleep(1);
    }
    ualarm(0, 0); // disable signal
    
    /* message */
    // black background again 
    sleep(1);
    update_full(i2c_fd, data);
    write_str(i2c_fd, "ENJOY!", 49, S_PAGES/2);

    /* Loading: cat running */
    sleep(1);
    update_full(i2c_fd, data);
    interval = 50000;
    with_robot = 1;
    signal(SIGALRM, cat_walking);
    ualarm(interval, interval); // SIGALRM generated with interval
    TIME_REMAIN = 10 * 1000000 / interval; // x seconds to finish loading
    
    while(TIME_REMAIN--){
        sleep(1);
    }
    ualarm(0, 0); // disable signal

    /* update logo */

    // black background again 
    update_full(i2c_fd, data);

    // prepare dataset
    uint8_t* data_s = (uint8_t*)malloc((LOGO_WIDTH) * LOGO_HEIGHT);
    for(int y = 0; y < LOGO_HEIGHT; y++){
        for(int x = 0; x < LOGO_WIDTH; x++){
            data_s[(LOGO_WIDTH) * y + (x)] = logo[LOGO_WIDTH * y + x];
        }
    }
    
    printf("data prepared\n");
    
    if(DEBUG){
        printf("printing data to display...\n");
        for(int i = 0; i < LOGO_HEIGHT; i++){
            for(int j = 0; j < LOGO_WIDTH; j++){
                printf("%d ", data_s[LOGO_WIDTH * i + j]);
            }
            printf("\n");
        }
    }

    // bitwise update from the above
    int delay_us = (int)(0.05 * (1000 * 1000));
    update_bitwise_vertical(i2c_fd, data_s, delay_us, 36, 0, LOGO_WIDTH, LOGO_HEIGHT);

    /* write team name */
    // write string
    sleep(1);
    write_str(i2c_fd, "The Butlers.", 34, S_PAGES - 1);

    /* wrap up */
    free(data);
    close(i2c_fd);
    
    return 0;
}
