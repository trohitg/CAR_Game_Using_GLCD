//#include<reg51.h>
#include <at89c5131.h>
#include<stdlib.h>


/* Configure the data bus and Control bus as per the hardware connection
   Dtatus bus is connected to P20:P27 and control bus P00:P04*/
#define GlcdDataBus  P2

sbit CS_BAR = P1^4;									// Chip Select for the ADC
# define OBS_LEN 4   // change this value to change the length of the obstacle
# define OBS_NUM 3
# define bull_num 20
# define BULL_LEN 2

sbit RS  = P0^0;
sbit RW  = P0^1;
sbit EN  = P0^2;
sbit CS1 = P0^3;
sbit CS2 = P0^4;

/// for accelerometer 

bit transmit_completed= 0;					// To check if spi data transmit is complete

unsigned int adcVal = 0;
unsigned char serial_data;
unsigned char data_save_high;
unsigned char data_save_low;

unsigned int i ,j, d ,a , b;
unsigned char pos_x; // car position	(along 64px)
unsigned char pos_y; // car position y (along 128px)
unsigned char x_inst;

unsigned int counter;
unsigned char brk;


void Board_Init();
void System_Init();
void Glcd_Init();
void INT0_Init(void);
void SPI_Init();
void delay(unsigned int cnt);
void Glcd_SelectPage0();
void Glcd_selectPage10();
void Glcd_SelectPage1();
void Glcd_CmdWrite(char cmd);
void Glcd_DataWrite(char dat);
void clrscr(void);
void clr_baseline(void);
void clr_topline(void);
void display_car(void);
void delay_ms(unsigned int delay);
void advance_obst(unsigned int i);
void advance_bullet(unsigned int i);
void obst_update(unsigned int i);
void bull_update(unsigned int i);
void init_spawns(unsigned int i);
void init_bullet(unsigned int i);
void game_over();
void test_touch(unsigned int i);
void blast(unsigned int i,unsigned int j);
int take_accData(void);
void clr_car();
void clr_advance_obst(unsigned int i);
void clr_obs(unsigned int i);
void run_cycle();

/* 5x7 Font including 1 space to display HELLO WORLD 
char H[]={0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00};
char E[]={0x7F, 0x49, 0x49, 0x49, 0x41, 0x00};
char L[]={0x7F, 0x40, 0x40, 0x40, 0x40, 0x00};
char O[]={0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00};

char W[]={0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00};
char R[]={0x7F, 0x09, 0x19, 0x29, 0x46, 0x00};
char D[]={0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00};
*/

struct obstacle
{
	unsigned char  pos_y;
	unsigned char pos_x;
	char spawned;
}obstacles[OBS_NUM];


struct bullet
{
	
	unsigned char pos_x;
	unsigned char pos_y;
	char spawned;

}bullet[bull_num];

void clr_topline(void)
{	
	Glcd_SelectPage1();
	for (a=0xb8;a<=0xbf;a++)
	{
		Glcd_CmdWrite(a);
		Glcd_CmdWrite(0x40+128-BULL_LEN);
		for (b=0x00;b<=BULL_LEN;b++)
		{
				Glcd_DataWrite(0x00);
		}
	}
}

void advance_bullet(unsigned int i)
{		
	  Glcd_CmdWrite(bullet[i].pos_x);
		Glcd_CmdWrite(0x40+bullet[i].pos_y%64);
		Glcd_DataWrite(0xff);
		Glcd_CmdWrite(0x40+(bullet[i].pos_y-BULL_LEN)%64);
		Glcd_DataWrite(0x00);
}

void bull_update(unsigned int i)
{
	if(bullet[i].pos_y>64 && bullet[i].pos_y-BULL_LEN<63)
	{
		Glcd_SelectPage1();
		Glcd_CmdWrite(bullet[i].pos_x);
		Glcd_CmdWrite(0x40+(bullet[i].pos_y%64));
		Glcd_DataWrite(0xff);
		Glcd_SelectPage0();
		Glcd_CmdWrite(bullet[i].pos_x);
		Glcd_CmdWrite(0x40+(bullet[i].pos_y+OBS_LEN)%64);
		Glcd_DataWrite(0x00);
	}
	else if(bullet[i].pos_y<64)
	{
		Glcd_SelectPage0();
		advance_bullet(i);
	}
	else
	{
		Glcd_SelectPage1();
		advance_bullet(i);
	}
}

void init_bullet(unsigned int i)
{
	bullet[i].pos_x = 0xb8+pos_x;
	bullet[i].pos_y = 0x1a+BULL_LEN;
	bullet[i].spawned = 's';
}

void clr_advance_obst(unsigned int i)
{
		Glcd_CmdWrite(obstacles[i].pos_x);
		Glcd_CmdWrite(0x40+obstacles[i].pos_y%64);
		for(j=0;j<OBS_LEN;j++)
		Glcd_DataWrite(0x00);
}

void clr_obs(unsigned int i)
{
	if(obstacles[i].pos_y<64 && obstacles[i].pos_y+OBS_LEN>63)
	{
		Glcd_SelectPage0();
		Glcd_CmdWrite(obstacles[i].pos_x);
		Glcd_CmdWrite(0x40+63);
		for(j=0;j<64-obstacles[i].pos_y;j++)
		Glcd_DataWrite(0x00);
		Glcd_SelectPage1();
		Glcd_CmdWrite(obstacles[i].pos_x);
		Glcd_CmdWrite(0x40);
		for(j=0;j<OBS_LEN+obstacles[i].pos_y-64;j++)
		Glcd_DataWrite(0x00);
	}
	else if(obstacles[i].pos_y<64)
	{
		Glcd_SelectPage0();
		clr_advance_obst(i);
	}
	else
	{
		Glcd_SelectPage1();
		clr_advance_obst(i);
	}	
}

void blast(unsigned int i,unsigned int j)
{
	if(bullet[i].spawned == 's'){
		if(bullet[i].pos_y == obstacles[j].pos_y)
		{
			if(bullet[i].pos_x == obstacles[j].pos_x)
			{
			obstacles[j].spawned = 'n';
			clr_obs(j);
			}
		}
 	}
}


void SPI_Init()
{
	CS_BAR = 1;	                  	// DISABLE ADC SLAVE SELECT-CS 
	SPCON |= 0x20;               	 	// P1.1(SSBAR) is available as standard I/O pin 
	SPCON |= 0x01;                	// Fclk Periph/4 AND Fclk Periph=12MHz ,HENCE SCK IE. BAUD RATE=3000KHz 
	SPCON |= 0x10;               	 	// Master mode 
	SPCON &= ~0x08;               	// CPOL=0; transmit mode example|| SCK is 0 at idle state
	SPCON |= 0x04;                	// CPHA=1; transmit mode example 
	IEN1 |= 0x04;                	 	// enable spi interrupt 
	EA=1;                         	// enable interrupts 
	SPCON |= 0x40;                	// run spi;ENABLE SPI INTERFACE SPEN= 1 
}

/* local function to generate delay */
void delay(unsigned int cnt)
{

    for(d=0;d<cnt;d++);
}

void Glcd_SelectPage0() // CS1=1, CS2=0
{
    CS1 = 1;
    CS2 = 0;
}

void Glcd_selectPage10()
{
	CS1 = 1;
	CS2 = 1;
}

void Glcd_SelectPage1() // CS1=0, CS1=1
{
    CS1 = 0;
    CS2 = 1;
}

/* Function to send the command to LCD */
void Glcd_CmdWrite(char cmd)
{
    GlcdDataBus = cmd;   //Send the Command 
    RS = 0;              // Send LOW pulse on RS pin for selecting Command register
    RW = 0;              // Send LOW pulse on RW pin for Write operation
    EN = 1;              // Generate a High-to-low pulse on EN pin
    delay(10);
    EN = 0;

    // delay(1000);
}

/* Function to send the data to LCD */
void Glcd_DataWrite(char dat)
{
    GlcdDataBus = dat;   //Send the data on DataBus
    RS = 1;              // Send HIGH pulse on RS pin for selecting data register
    RW = 0;              // Send LOW pulse on RW pin for Write operation
    EN = 1;              // Generate a High-to-low pulse on EN pin
    delay(10);
    EN = 0;
    // delay(1000);
}

// clear  the whole screen
void clrscr(void)
{	
	Glcd_selectPage10();
	for (a=0xb8;a<=0xbf;a++)
	{
		Glcd_CmdWrite(a);
		Glcd_CmdWrite(0x40);
		for (b=0x00;b<=0x3f;b++)
		{
				Glcd_DataWrite(0x00);
		}
	}
}

void clr_baseline(void)
{

	Glcd_SelectPage0();
	for (a=0xb8;a<=0xbf;a++)
	{
		Glcd_CmdWrite(a);
		Glcd_CmdWrite(0x40);
		for (b=0x00;b<=OBS_LEN;b++)
		{
				Glcd_DataWrite(0x00);
		}
	}
	
}

void display_car(void)
{
	    Glcd_SelectPage0();
			Glcd_CmdWrite(0xb8+pos_x%8);
			Glcd_CmdWrite(0x40+pos_y);
			Glcd_DataWrite(0x66);
			Glcd_DataWrite(0xff);
			Glcd_DataWrite(0x7e);
			Glcd_DataWrite(0x3c);
			Glcd_DataWrite(0x18);
			Glcd_DataWrite(0x18);
			Glcd_DataWrite(0x3c);
			Glcd_DataWrite(0x18);
}

void delay_ms(unsigned int delay)
{
	while(delay>0)
	{
		for(d=0;d<382;d++);
		delay--;
	}
}

// structure for the obstacles 
// these characteristics are shared by all the obstacles



void advance_obst(unsigned int i)
{
		Glcd_CmdWrite(obstacles[i].pos_x);
		Glcd_CmdWrite(0x40+obstacles[i].pos_y%64);
		Glcd_DataWrite(0xff);
		Glcd_CmdWrite(0x40+(obstacles[i].pos_y+OBS_LEN)%64);
		Glcd_DataWrite(0x00);
}


// updating the position of the obstacles
// this will make sure that the transition from the two pages is smooth
void obst_update(unsigned int i)
{
	if(obstacles[i].pos_y<64 && obstacles[i].pos_y+OBS_LEN>63)
	{
		Glcd_SelectPage0();
		Glcd_CmdWrite(obstacles[i].pos_x);
		Glcd_CmdWrite(0x40+(obstacles[i].pos_y%64));
		Glcd_DataWrite(0xff);
		Glcd_SelectPage1();
		Glcd_CmdWrite(obstacles[i].pos_x);
		Glcd_CmdWrite(0x40+(obstacles[i].pos_y+OBS_LEN)%64);
		Glcd_DataWrite(0x00);
	}
	else if(obstacles[i].pos_y<64)
	{
		Glcd_SelectPage0();
		advance_obst(i);
	}
	else
	{
		Glcd_SelectPage1();
		advance_obst(i);
	}
}

// initalizing the values of the obstacles
void init_spawns(unsigned int i)
{
	obstacles[i].pos_x = 0xb8+rand()%8;
	obstacles[i].pos_y = 128-OBS_LEN;
	obstacles[i].spawned = 's';
	
}

void game_over()
{
		clrscr();
		brk =0;
}

void test_touch(unsigned int i)
{
	if(pos_x+1 == obstacles[i].pos_x-183)
	{
		Glcd_SelectPage0();
		Glcd_CmdWrite(obstacles[i].pos_x);
		Glcd_CmdWrite(0x41);
		Glcd_DataWrite(pos_x+1);
		Glcd_CmdWrite(0x42);
		Glcd_DataWrite(obstacles[i].pos_x-0xb7);
		if((pos_y+7 > obstacles[i].pos_y) && (pos_y < obstacles[i].pos_y))
		{
			game_over();
		}
	}
}


int take_accData(void)
{
	
	CS_BAR = 0;                 // enable ADC as slave		 
	SPDAT= 0x01;								// Write start bit to start ADC 
	while(!transmit_completed);	// wait end of transmition;TILL SPIF = 1 i.e. MSB of SPSTA
	transmit_completed = 0;    	// clear software transfert flag 
	
	SPDAT= 0x80;				// 80H written to start ADC CH0 single ended sampling,refer ADC datasheet
	while(!transmit_completed);	// wait end of transmition 
	data_save_high = serial_data & 0x03 ;  
	transmit_completed = 0;    	// clear software transfer flag 
			
	SPDAT= 0x00;								// 
	while(!transmit_completed);	// wait end of transmition 
	data_save_low = serial_data;
	transmit_completed = 0;    	// clear software transfer flag 
	CS_BAR = 1;                	// disable ADC as slave
	
	adcVal = (data_save_high <<8) + (data_save_low);
	
	// 319 - 405
	// 364
	adcVal -= 319;
	
	for(a=0;a<8;a++)
	{
		if(adcVal/10 <= a)
		{
			return a;
		}
	}
	return 0;
}

void clr_car()
{
	  Glcd_SelectPage0();
		Glcd_CmdWrite(0xb8+pos_x%8);
		Glcd_CmdWrite(0x40+pos_y);
		Glcd_DataWrite(0x00);
		Glcd_DataWrite(0x00);
		Glcd_DataWrite(0x00);
		Glcd_DataWrite(0x00);
		Glcd_DataWrite(0x00);
		Glcd_DataWrite(0x00);
		Glcd_DataWrite(0x00);
		Glcd_DataWrite(0x00);

}

void run_cycle(){
				for(i=0;i<OBS_NUM;i++)
			{
				if(obstacles[i].spawned == 's')
				{
					obst_update(i);
					test_touch(i);
					for(j=0;j<bull_num;j++)
					{
						blast(j,i);
					}
					obstacles[i].pos_y -= 1;
				}
				else if(counter > i*100)
					init_spawns(i);
				if(obstacles[i].pos_y <=0 && obstacles[i].spawned=='s')
				{
					obstacles[i].spawned='n';
					obstacles[i].pos_y=128-OBS_LEN;
					clr_baseline();
				}
			}

			for(i = 0;j<bull_num;j++)
			{
				if(bullet[i].spawned == 's')
				{
					bull_update(i);
					bullet[i].pos_y += 1;
				}
				if(bullet[i].pos_y >128 && bullet[i].spawned=='s')
				{
					bullet[i].spawned='n';
					bullet[i].pos_y=0;
					clr_topline();
				}
		}
}

void INT0_Init(void)
{
	IT0 = 1;
	EX0 = 1;
	EA = 1;
}

int main()
{
	
	P3 = 0X00;											// Make Port 3 output 
	P1 &= 0xEF;											// Make P1 Pin4-7 output
	P0 &= 0xF0;
	
	SPI_Init();
	INT0_Init();
	init_spawns(0);
	
	
	counter = 0;
    /* Select the Page0/Page1 and Turn on the GLCD */
    Glcd_SelectPage0();
    Glcd_CmdWrite(0x3f);
    Glcd_SelectPage1();
    Glcd_CmdWrite(0x3f);
    delay(10);

    /* Select the Page0/Page1 and Enable the GLCD */
    Glcd_SelectPage0();
    Glcd_CmdWrite(0xc0);
    Glcd_SelectPage1();
    Glcd_CmdWrite(0xc0);
    delay(10);
	
	clrscr();
	
	delay(100);

	pos_x =0;
	pos_y =0x1a;
	display_car();

		
	brk =1;
    while(brk==1)
		{
			clr_car();
			
			pos_x= take_accData();

			display_car();
			
			if(counter<OBS_NUM*100)
				counter++;
			
			run_cycle();
			
			delay_ms(10);
		}
	delay_ms(1000);
	while(1);
}


void extINT0(void)  interrupt 0
{

	for (a = 0;a<bull_num;a++)
	{
		if(bullet[a].spawned != 's')
		{
			bullet[a].spawned = 's';
			break;
		}
	}
}

void it_SPI(void) interrupt 9 /* interrupt address is 0x004B, (Address -3)/8 = interrupt no.*/
{
	switch	( SPSTA )         /* read and clear spi status register */
	{
		case 0x80:	
			serial_data=SPDAT;   /* read receive data */
      transmit_completed=1;/* set software flag */
 		break;

		case 0x10:
         /* put here for mode fault tasking */	
		break;
	
		case 0x40:
         /* put here for overrun tasking */	
		break;
	}
}
