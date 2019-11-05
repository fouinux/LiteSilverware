#include "osd.h"
#include "drv_serial.h"
#include "drv_time.h"
#include "math.h"
#include <stdlib.h>

#define AETR  ((-0.65f > rx[Yaw]) && (0.3f < rx[Throttle]) && (0.7f > rx[Throttle]) && (0.5f < rx[Pitch]) && (-0.3f < rx[Roll]) && (0.3f > rx[Roll]))
#define TAER  ((-0.65f > rx[Yaw]) && (-0.3f < rx[Pitch]) && (0.3f > rx[Pitch]) && (0.7f < rx[Roll]) && (0.7f > rx[Throttle]) && (0.3f < rx[Throttle]))
#define POLYGEN 0xd5


extern void flash_load( void);
extern void flash_save( void);
extern void flash_hard_coded_pid_identifier(void);

extern unsigned char osd_data[12];
extern char aux[16];
extern unsigned int osd_count;
extern unsigned int vol;
extern unsigned int cur;
extern float electricCur;
extern float rx[4];
extern float pidkp[PIDNUMBER];  
extern float pidki[PIDNUMBER];	
extern float pidkd[PIDNUMBER];
extern int number_of_increments[3][3];
extern unsigned long lastlooptime;

unsigned char powerlevel = 0;
unsigned char channel = 0;
unsigned char powerleveltmp = 0;
unsigned char channeltmp = 0;
unsigned char initmotor =0;
unsigned char mode = 0;
unsigned char sa_flag = 0;
unsigned char aetr_or_taer=0;
unsigned char showcase = 0;
unsigned char rx_switch = 1;

char motorDir[4] = {1,0,0,1};

unsigned char main_version = 1;
unsigned char modify_version = 0;
char down_flag = 0;
char up_flag = 0;
char right_flag = 0;
char left_flag = 0;

menu_list setMenu,setMenuHead;
menu_list pidMenu,pidMenuHead;
menu_list motorMenu,motorMenuHead;
menu_list receiverMenu,receiverMenuHead;
menu_list smartaudioMenu,smartaudioMenuHead;
menu_list currentMenu;

#ifdef Lite_OSD

static uint8_t CRC8(unsigned char *data, const int8_t len)
{
    uint8_t crc = 0; /* start with 0 so first byte can be 'xored' in */
    uint8_t currByte;

    for (int i = 0 ; i < len ; i++) {
        currByte = data[i];

        crc ^= currByte; /* XOR-in the next input byte */

        for (int i = 0; i < 8; i++) {
            if ((crc & 0x80) != 0) {
                crc = (uint8_t)((crc << 1) ^ POLYGEN);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void getIndex()
{
    if((rx[Pitch] < -0.6f) && (down_flag == 1))
    {
        currentMenu = currentMenu->next;
        down_flag = 0;
    }		
    
    if((rx[Pitch] > 0.6f) && (up_flag == 1))
    {
        currentMenu = currentMenu->prior;
        up_flag = 0;
    }
    
    if((rx[Pitch]) < 0.6f && (rx[Pitch] > -0.6f))
    {
        up_flag = 1;
        down_flag = 1;
    }
    if((rx[Roll]) < 0.6f && (rx[Roll] > -0.6f))
    {
        right_flag = 1;
        left_flag = 1;
    }
}

void osd_setting()
{
    switch(showcase)
    {
        case 0:
            if(AETR || TAER)
            {
                showcase = 1;
                unsigned char i = 0;
                for(i=0; i<3; i++)
                {
                    pidMenu->fvalue = pidkp[i];
                    pidMenu = pidMenu->next;
                }
                
                for(i=0; i<3; i++)
                {
                    pidMenu->fvalue = pidki[i];
                    pidMenu = pidMenu->next;
                }
                
                for(i=0; i<3; i++)
                {
                    pidMenu->fvalue = pidkd[i];
                    pidMenu = pidMenu->next;
                }
                
                for(i=0; i<4; i++)
                {
                    motorMenu->uvalue = motorDir[i];
                    motorMenu = motorMenu->next;
                }
                pidMenu = pidMenuHead;
                motorMenu = motorMenuHead;
                channeltmp = channel;
                powerleveltmp = powerlevel;
            }
            if(osd_count == 200)
            {
                osd_data[0] = 0x0f;
                osd_data[0] |=showcase << 4;
                osd_data[1] = aux[CHAN_5];
                osd_data[2] = 0;
                osd_data[3] = vol >> 8;
                osd_data[4] = vol & 0xFF;
                osd_data[5] = rx_switch;
                
                osd_data[6] = 0;
                osd_data[6] = (aux[CHAN_6] << 0) | (aux[CHAN_7] << 1) | (aux[CHAN_8] << 2);
   
                osd_data[7] = (!aux[LEVELMODE] && aux[RACEMODE]);
                osd_data[8] = cur >> 8;
                osd_data[9] = cur & 0xFF;
                osd_data[10] = 0;
                osd_data[11] = 0;
                for (uint8_t i = 0; i < 11; i++)
                    osd_data[11] += osd_data[i];  
                
                UART2_DMA_Send();
                osd_count = 0;
            }
            break;
            
        case 1:
            getIndex();
            if((rx[Roll] > 0.6f) && right_flag == 1)
            {
                switch(currentMenu->index)
                {
                    case 0:
                        currentMenu = pidMenuHead;
                        showcase = 2;
                        break;
                    case 1:
                       currentMenu = motorMenuHead;
                       showcase = 3;
                       break; 
                    case 2:
                       currentMenu = receiverMenuHead;
                       showcase = 4;
                       break; 
                    case 3:
                       currentMenu = smartaudioMenuHead;
                       showcase = 5;
                       break; 
                    case 4:
                        showcase =0;
                        currentMenu = setMenuHead;
                        down_flag = 0;
                        up_flag = 0;
                        
                        extern void flash_save( void);
                        extern void flash_load( void);
						flash_save( );
                        flash_load( );
                    
                        extern int number_of_increments[3][3];
                        for( int i = 0 ; i < 3 ; i++)
                            for( int j = 0 ; j < 3 ; j++)
                                number_of_increments[i][j] = 0;
                        
                        extern unsigned long lastlooptime;
                        lastlooptime = gettime();
                        break;
                    case 5:
                        showcase =0;
                        currentMenu = setMenuHead;
                        down_flag = 0;
                        up_flag = 0;
                        break;
                }
                right_flag = 0;
            }
            if(osd_count == 200)
            {
                osd_data[0] =0x0f;
                osd_data[0] |=showcase << 4;
                osd_data[1] = currentMenu->index;
                osd_data[2] = main_version;
                osd_data[3] = modify_version;
                osd_data[4] = 0;
                osd_data[5] = 0;
                osd_data[6] = 0;
                osd_data[7] = 0;
                osd_data[8] = 0;
                osd_data[9] = 0;
                osd_data[10] = 0;
                osd_data[11] = 0;
                for (uint8_t i = 0; i < 11; i++)
                    osd_data[11] += osd_data[i];  
                
                UART2_DMA_Send();
                osd_count = 0;
            }   
            break;
        
        case 2:
            getIndex();
            
            if((rx[Roll] > 0.6f) && right_flag == 1)
            {
                if(currentMenu->index <9)
                {
                    int a;
                    currentMenu->fvalue += 0.01f;

                    pidMenu = pidMenuHead;
                    for(a=0;a<3;a++)
                    {
                        pidkp[a] = pidMenu->fvalue;
                        pidMenu = pidMenu->next;
                    }
                    
                    for(a=0;a<3;a++)
                    {
                        pidki[a] = pidMenu->fvalue;
                        pidMenu = pidMenu->next;
                    }
                    
                    for(a=0;a<3;a++)
                    {
                        pidkd[a] = pidMenu->fvalue;
                        pidMenu = pidMenu->next;
                    }
                }
                else{
                    showcase = 1;
                    pidMenu = pidMenuHead;
                    currentMenu = setMenuHead;
                }
                right_flag = 0;
            }
            if((rx[Roll] < -0.6f) && left_flag == 1)
            {
                if(currentMenu->index <9)
                {
                    int a;
                    currentMenu->fvalue -= 0.01f;
                    if(currentMenu->fvalue <=0)
                    {
                        currentMenu->fvalue = 0;
                    }
                    
                    pidMenu = pidMenuHead;
                    for(a=0;a<3;a++)
                    {
                        pidkp[a] = pidMenu->fvalue;
                        pidMenu = pidMenu->next;
                    }
                    
                    for(a=0;a<3;a++)
                    {
                        pidki[a] = pidMenu->fvalue;
                        pidMenu = pidMenu->next;
                    }
                    
                    for(a=0;a<3;a++)
                    {
                        pidkd[a] = pidMenu->fvalue;
                        pidMenu = pidMenu->next;
                    }
                }
                left_flag = 0;
            }
            if(osd_count == 200)
            {
                osd_data[0] =0x0f;
                osd_data[0] |=showcase << 4;
                osd_data[1] = currentMenu->index;
                osd_data[2] = round(pidkp[0]*100);
                osd_data[3] = round(pidkp[1]*100);
                osd_data[4] = round(pidkp[2]*100);
                osd_data[5] = round(pidki[0]*100);
                osd_data[6] = round(pidki[1]*100);
                osd_data[7] = round(pidki[2]*100);
                osd_data[8] = round(pidkd[0]*100);
                osd_data[9] = round(pidkd[1]*100);
                osd_data[10] = round(pidkd[2]*100);
                osd_data[11] = 0;
                for (uint8_t i = 0; i < 11; i++)
                    osd_data[11] += osd_data[i];  
                
                UART2_DMA_Send();
                osd_count = 0;
            } 
            break;
        
        case 3:            
            getIndex();
        
            if((rx[Roll] > 0.6f) && right_flag == 1)
            {
                if(currentMenu->index <4)
                {
                    currentMenu->uvalue = !currentMenu->uvalue;
                    motorDir[currentMenu->index] = currentMenu->uvalue;
                }
                else{
                    showcase = 1;
                    motorMenu = motorMenuHead;
                    currentMenu = setMenuHead;
                } 
                right_flag = 0;
            }
        
            if(osd_count == 200)
            {
                osd_data[0] =0x0f;
                osd_data[0] |=showcase << 4;
                osd_data[1] = currentMenu->index;
                osd_data[2] = motorDir[0] | (motorDir[1] <<1) | (motorDir[2] << 2) | (motorDir[3] <<3);
                osd_data[3] = 0;
                osd_data[4] = 0;
                osd_data[5] = 0;
                osd_data[6] = 0;
                osd_data[7] = 0;
                osd_data[8] = 0;
                osd_data[9] = 0;
                osd_data[10] = 0;
                osd_data[11] = 0;
                for (uint8_t i = 0; i < 11; i++)
                    osd_data[11] += osd_data[i];  
                
                UART2_DMA_Send();
                osd_count = 0;
            }
            break;
        
        case 4:
            getIndex();
            
            if((rx[Roll] > 0.6f) && right_flag == 1)
            {
                if(aux[LEVELMODE])
                {
                    if(currentMenu->index ==0)
                    {
                        aetr_or_taer = !aetr_or_taer;
                    }
                    else
                    {
                        showcase = 1;
                        currentMenu = setMenuHead;
                    }
                }
                right_flag = 0;
            }
            if(osd_count == 200)
            {
                osd_data[0] =0x0f;
                osd_data[0] |=showcase << 4;
                osd_data[1] = currentMenu->index;
                
                osd_data[2]  = 0;
                if(rx[0]> -0.2f && rx[0] <0.2f)
                {
                    osd_data[2] |= 0x1<<0;
                }
                else if(rx[0] > 0.4f)
                {
                    osd_data[2] |= 0x2<<0;
                }
                else if(rx[0] < -0.4f)
                {
                    osd_data[2] |= 0x0<<0;
                }
                
                if(rx[1]> -0.2f && rx[1] <0.2f)
                {
                    osd_data[2] |= 0x1<<2;
                }
                else if(rx[1]>= 0.2f)
                {
                    osd_data[2] |= 0x2<<2;
                }
                else 
                {
                    osd_data[2] |= 0x0<<2;
                }
                
                if(rx[2]> -0.2f && rx[2] <0.2f)
                {
                    osd_data[2] |= 0x1<<4;
                }
                else if(rx[2]>= 0.2f)
                {
                    osd_data[2] |= 0x2<<4;
                }
                else 
                {
                    osd_data[2] |= 0x0<<4;
                }
                
                if(rx[3]> 0.4f && rx[3] <0.6f)
                {
                    osd_data[2] |= 0x1<<6;
                }
                else if(rx[3]>= 0.6f)
                {
                    osd_data[2] |= 0x2<<6;
                }
                else 
                {
                    osd_data[2] |= 0x0<<6;
                }

                osd_data[3] = aux[CHAN_5] ;
                osd_data[4] = aux[CHAN_6] ;
                osd_data[5] = aux[CHAN_7] ;
                osd_data[6] = aux[CHAN_8] ;
                osd_data[7] = aetr_or_taer;
                osd_data[8] = 0;
                osd_data[9] = 0;
                osd_data[10] = 0;
                osd_data[11] = 0;
                for (uint8_t i = 0; i < 11; i++)
                    osd_data[11] += osd_data[i];  
                
                UART2_DMA_Send();
                osd_count = 0;
            }
            break;
        
        case 5:
            getIndex();
            
            if((rx[Roll] > 0.6f) && right_flag == 1)
            {
                switch(currentMenu->index)
                {
                    case 0:   
                        channeltmp++;
                        if(channeltmp > 39)
                            channeltmp = 0;
                        sa_flag = 1;
                        break;
                    
                    case 1:
                        powerleveltmp++;
                        if(powerleveltmp >1)
                            powerleveltmp = 0;
                        sa_flag = 2;
                        break;
                    
                    case 2:
                        if(sa_flag == 2)
                        {   
                            powerlevel = powerleveltmp;
                            sa_flag = 0;
                            osd_data[0] =0xAA;
                            osd_data[1] = 0x55;
                            osd_data[2] = 0x05;
                            osd_data[3] = 0x01;
                            osd_data[4] = powerleveltmp;
                            osd_data[5] = CRC8(osd_data,5);
                            osd_data[6] = 0;
                            osd_data[7] = 0;
                            osd_data[8] = 0;
                            osd_data[9] = 0;
                            osd_data[10] = 0;
                            osd_data[11] = 0;
                        }
                        if(sa_flag == 1)
                        {
                            channel = channeltmp;
                            sa_flag = 0;
                            osd_data[0] =0xAA;
                            osd_data[1] = 0x55;
                            osd_data[2] = 0x07;
                            osd_data[3] = 0x01;
                            osd_data[4] = channeltmp;
                            osd_data[5] = CRC8(osd_data,5);
                            osd_data[6] = 0;
                            osd_data[7] = 0;
                            osd_data[8] = 0;
                            osd_data[9] = 0;
                            osd_data[10] = 0;
                            osd_data[11] = 0;
                        }
                        flash_save();
                        extern unsigned long lastlooptime;
                        lastlooptime = gettime();
                        UART2_DMA_Send();
                        break;
                    
                    case 3:
                        showcase = 1;
                        currentMenu = setMenuHead;
                        break;
                    
                    default:
                        break;
                }
                right_flag = 0;
            }
            if((rx[Roll] < -0.6f) && left_flag == 1)
            {
                switch(currentMenu->index)
                {
                    case 0:
                        if(channeltmp == 0)
                        {
                            channeltmp = 40;
                        }
                        channeltmp --;
                        sa_flag = 1;
                        break;
  
                    default:
                        break;
                }
                left_flag = 0;
            }
            
            if(osd_count == 200)
            {
                osd_data[0] =0x0f;
                osd_data[0] |=showcase << 4;
                osd_data[1] = currentMenu->index;
                osd_data[2] = channel;
                osd_data[3] = powerlevel;
                osd_data[4] = 0;
                osd_data[5] = channeltmp;
                osd_data[6] = powerleveltmp;
                osd_data[7] = 0;
                osd_data[8] = 0;
                osd_data[9] = 0;
                osd_data[10] = 0;
                osd_data[11] = 0;
                for (uint8_t i = 0; i < 11; i++)
                    osd_data[11] += osd_data[i];  
                
                UART2_DMA_Send();
                osd_count = 0;
            }
            break;
        default:
            break;
    }
}

menu_list createMenu(char len,char item)
{
    char i = 0;
    menu_list pTail = NULL,p_new = NULL;
    menu_list pHead = (menu_list)malloc(sizeof(menu_node));
    if(NULL == pHead)
    {
        return 0;
    }
    
    pHead->index = 0;
    pHead->item = item;
    pHead->prior = pHead;
    pHead->next = pHead;
    pTail = pHead;
    
    for(i=1; i<len+1; i++)
    {
        p_new = (menu_list)malloc(sizeof(menu_node));
        if(NULL == p_new)
        {
            return 0;
        }
        p_new->index = i;
        p_new->item = item;
        p_new->prior = pTail;
        p_new->next = pHead;
        pTail->next = p_new;
        pHead->prior = p_new;
        pTail = p_new;
    }
    return pHead;  
}

void osdMenuInit(void)
{
    setMenu = createMenu(5,0);
    setMenuHead = setMenu;
    
    pidMenu = createMenu(9,1);
    pidMenuHead = pidMenu;
    
    motorMenu = createMenu(4,2);
    motorMenuHead = motorMenu;
    
    receiverMenu = createMenu(1,3);
    receiverMenuHead = receiverMenu;
    
    smartaudioMenu = createMenu(3,4);
    smartaudioMenuHead = smartaudioMenu;
    
    currentMenu = setMenu;    
}


#endif

