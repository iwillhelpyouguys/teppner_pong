/*
 * Copyright (c) 2009-2012 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include "pong.h"

// Funktionsprototypen
XGpio 		GpioOutput;					// LED
XScuGic 	InterruptController; 		// Interrupt-Controller-Instanz
XScuGic 	IntcInstance;				/* Interrupt Controller Instance */
XScuTimer 	TimerInstance;				// Timer-Instanz
XGpio 		GpioBtn;					// Buttons

player 		spieler[] = {{0},{0}};

ball		spielBall = {0};

int timerValue = 0xA5DC; // 0xF5DC
int punkte1 = 4;
int punkte2 = 4;

void initializeColortoVGAController(){ //set R G B Black to slvreg 1 2 3 4
	Xil_Out32(XPAR_VGA_0_S00_AXI_BASEADDR+4 , 0xf00);
	Xil_Out32(XPAR_VGA_0_S00_AXI_BASEADDR+8 , 0x0f0);
	Xil_Out32(XPAR_VGA_0_S00_AXI_BASEADDR+12 , 0x00f);
	Xil_Out32(XPAR_VGA_0_S00_AXI_BASEADDR+16 , 0x000);
}

void zeichnePunkt(int x, int y, int col)
{
	//Xil_Out32(XPAR_VGA_0_S00_AXI_BASEADDR+ y * x , col);
	Xil_Out32(XPAR_VGA_0_S00_AXI_BASEADDR, col | (640*y+x)<<2 );
}

void zeichneLinie(int x1, int y1, int x2, int y2, int col) {
   int dy = y2 - y1;
    int dx = x2 - x1;
    int stepx, stepy;

    if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
    if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
    dy <<= 1;        // dy is now 2*dy
    dx <<= 1;        // dx is now 2*dx

    zeichnePunkt(x1,y1, col);
    if (dx > dy)
    {
        int fraction = dy - (dx >> 1);  // same as 2*dy - dx
        while (x1 != x2)
        {
           if (fraction >= 0)
           {
               y1 += stepy;
               fraction -= dx;          // same as fraction -= 2*dx
           }
           x1 += stepx;
           fraction += dy;              // same as fraction -= 2*dy
           zeichnePunkt(x1, y1, col);
        }
     } else {
        int fraction = dx - (dy >> 1);
        while (y1 != y2) {
           if (fraction >= 0) {
               x1 += stepx;
               fraction -= dy;
           }
           y1 += stepy;
           fraction += dx;
           zeichnePunkt(x1, y1, col);
        }
     }
}

void loescheAlles() {
	int x,y;
	for (x = 0; x < SCREENSIZE_X; x++) {
		for (y = 0; y < SCREENSIZE_Y; y++) {
			zeichnePunkt(x, y, 1);
		}
	}
//	for(x=0; x < (640 * 480); x++)
//	{
//		Xil_Out32(XPAR_VGA_0_S00_AXI_BASEADDR, 1 | x<<2);
//	}
}

void build_background()
{

}

void zeichneBall(int x, int y, int col) {
	//int i,j;
	zeichnePunkt(x,y, col);
	zeichnePunkt(x-1,y, col);
	zeichnePunkt(x+1,y, col);
	zeichnePunkt(x,y-1, col);
	zeichnePunkt(x,y+1, col);
//	for (j=BALLSIZE+y ; j<= y; j--)
//	{
//		for (i=1 ; i<=BALLSIZE; i++)
//		{
//		zeichnePunkt(x-i,y, col);
//		zeichnePunkt(x+i,y, col);
//		zeichnePunkt(x,y-i, col);
//		zeichnePunkt(x,y+i, col);
//
//		}
//	}

}

void zeichneBallNeu()
{
	zeichneBall(spielBall.posOldX, spielBall.posOldY, 1);
	zeichneBall(spielBall.posX, spielBall.posY, 2);
}

void zeichneBoard()
{
	int k; // Board Stärke
	int y = 450; // Y-Position
	for(k = 0; k < 5; k++)
	{
		zeichneLinie( spieler[0].posOldX-BOARDSIZE/2, y+k, spieler[0].posOldX+BOARDSIZE/2, y+k, 1);
		zeichneLinie( spieler[0].posX-BOARDSIZE/2, y+k, spieler[0].posX+BOARDSIZE/2, y+k, 2);
	}

	y = 10; // Y-Position
	for(k = 0; k < 5; k++)
	{
		zeichneLinie( spieler[1].posOldX-BOARDSIZE/2, y+k, spieler[1].posOldX+BOARDSIZE/2, y+k, 1);
		zeichneLinie( spieler[1].posX-BOARDSIZE/2, y+k, spieler[1].posX+BOARDSIZE/2, y+k, 2);
	}
}

void newGame() {
	spielBall.posOldX = spielBall.posX;
	spielBall.posOldY = spielBall.posY;
	spielBall.posX = SCREENSIZE_X/2;
	spielBall.posY = SCREENSIZE_Y/2;

	spieler[0].posOldX = spieler[0].posX;
	spieler[1].posOldX = spieler[1].posX;
	zeichneBoard();

	zeichneBallNeu();
}

void setup() {
	spieler[0].posOldX = spieler[0].posX;
	spieler[1].posOldX = spieler[1].posX;
	spieler[0].posX = SCREENSIZE_X/2;
	spieler[1].posX = SCREENSIZE_X/2;
	spielBall.posOldX = spielBall.posX;
	spielBall.posOldY = spielBall.posY;
	spielBall.posX = SCREENSIZE_X/2;
	spielBall.posY = SCREENSIZE_Y/2;
	spielBall.stepX = 1;
	spielBall.stepY = -1;

	XScuTimer_Stop(&TimerInstance);
	//timerValue = 0xA5DC;
	timerValue = 0x55DC;
	XScuTimer_LoadTimer(&TimerInstance, timerValue);
	XScuTimer_Start(&TimerInstance);

	loescheAlles();
}

// interrupt handler for right
void right_InterruptHandler()
{
	spieler[0].posOldX = spieler[0].posX;

	//printf("right posX: %d  \r\n", spieler.posX);
	if (spieler[0].posX+10+BOARDSIZE/2 > SCREENSIZE_X) {
		spieler[0].posX = SCREENSIZE_X-BOARDSIZE/2;
	}
	else{
		spieler[0].posX += 10;
	}

	zeichneBoard();
}

// interrupt handler for left
void left_InterruptHandler()
{

	spieler[0].posOldX = spieler[0].posX;
	printf("left posX: %d  \r\n", spieler[0].posX);
	if (spieler[0].posX-10-BOARDSIZE/2 < 0) {
		spieler[0].posX = BOARDSIZE/2;

	}
	else{
		spieler[0].posX -= 10;
	}

	zeichneBoard();
}

// interrupt handler for right player2
void right_InterruptHandler2()
{
	spieler[1].posOldX = spieler[1].posX;

	//printf("right posX: %d  \r\n", spieler.posX);
	if (spieler[1].posX+10+BOARDSIZE/2 > SCREENSIZE_X) {
		spieler[1].posX = SCREENSIZE_X-BOARDSIZE/2;
	}
	else{
		spieler[1].posX += 10;
	}

	zeichneBoard();
}

// interrupt handler for left  player2
void left_InterruptHandler2()
{

	spieler[1].posOldX = spieler[1].posX;
	printf("left posX: %d  \r\n", spieler[1].posX);
	if (spieler[1].posX-10-BOARDSIZE/2 < 0) {
		spieler[1].posX = BOARDSIZE/2;

	}
	else{
		spieler[1].posX -= 10;
	}

	zeichneBoard();
}


void showLives(){
	int output = 0;
	switch(spieler[1].lives) {
		case 4:
			output = 240;
			break;
		case 3:
			output = 224;
			break;
		case 2:
			output = 192;
			break;
		case 1:
			output = 128;
			break;
		default:
			output = 0;
			break;
	}
	switch(spieler[0].lives) {
		case 4:
			output += 15;
			break;
		case 3:
			output += 14;
			break;
		case 2:
			output += 12;
			break;
		case 1:
			output += 8;
			break;
		default:
			output += 0;
			break;
	}
	printf("Punkte: %d \r\n",output);

	XGpio_DiscreteWrite(&GpioOutput, 1, output);
	//XGpio_DiscreteWrite(&GpioOutput, 1, 240);
}

static void TimerIntHandler(void *CallBackRef) {
	//int led=0x0;

	// Bewege Ball
	spielBall.posOldX = spielBall.posX;
	spielBall.posOldY = spielBall.posY;

	if(spielBall.posX > SCREENSIZE_X) {
		// rechter Rand, spiegeln
		print("rechter Rand, spiegeln \r\n");
		spielBall.stepX = spielBall.stepX * -1;
	}else if(spielBall.posX < 0) {
		// linker Rand, spiegeln
		print("linker Rand, spiegeln \r\n");
		spielBall.stepX = spielBall.stepX * -1;
	}else if(spielBall.posY < 0) {
		// oberer Rand, PUNKT
		print("oberer Rand, PUNKT und Neustart! \r\n");

/*
		int i;
		//int led=0x0;
		punkte1--;*/

		spieler[0].lives--;
		if(spieler[0].lives == 0) {
			spieler[0].lives = 4;
			spieler[1].lives = 4;
		}
		setup();
		newGame();
		showLives();

	}else if(spielBall.posY+1 >= 450 && spielBall.posY+1 <= 455 ) {
		// Board höhe
		if(spielBall.posX > spieler[0].posOldX-BOARDSIZE/2 && spielBall.posX < spieler[0].posOldX+BOARDSIZE/2) {
			print("Schläger getroffen, spiegeln \r\n");
			spielBall.stepY = spielBall.stepY * -1;
			XScuTimer_Stop(&TimerInstance);
			timerValue = timerValue/1.2;
			XScuTimer_LoadTimer(&TimerInstance, timerValue);
			XScuTimer_Start(&TimerInstance);
		}

	}else if(spielBall.posY-1 >= 10 && spielBall.posY-1 <= 15) {
		// Board höhe
		if(spielBall.posX > spieler[1].posOldX-BOARDSIZE/2 && spielBall.posX < spieler[1].posOldX+BOARDSIZE/2) {
			print("Schläger2 getroffen, spiegeln \r\n");
			spielBall.stepY = spielBall.stepY * -1;
			XScuTimer_Stop(&TimerInstance);
			timerValue = timerValue/1.2;
			XScuTimer_LoadTimer(&TimerInstance, timerValue);
			XScuTimer_Start(&TimerInstance);
		}
	}else if(spielBall.posY == SCREENSIZE_Y) {
		// unterer Rand, PUNKT
		print("unterer Rand, PUNKT und Neustart! \r\n");
		/*setup();
		newGame();*/

		spieler[1].lives--;
		if(spieler[1].lives == 0) {
			spieler[0].lives = 4;
			spieler[1].lives = 4;
		}
		setup();
		newGame();
		showLives();
/*
		int i;
		//int led=0x0;
		punkte2--;
		for (i = 4 ; i < punkte2+4; i++)
		{
			printf("i=%d \r\n",i);
			led = led | 1<<i;
		}
		XGpio_DiscreteWrite(&GpioOutput, 1, led);*/
	}

	zeichneBoard();

	spielBall.posX = spielBall.posX + spielBall.stepX;
	spielBall.posY = spielBall.posY + spielBall.stepY;

	zeichneBallNeu();

	/*XScuTimer *TimerInstancePtr = (XScuTimer *) CallBackRef;
	XScuTimer_ClearInterruptStatus(TimerInstancePtr);*/
}


int main() {
    print("PONG started\n\r");

	spieler[0].lives = 4;
	spieler[1].lives = 4;

	int Status;
	XScuGic_Config *GicConfig;
	XScuTimer_Config *TimerConfigPtr;

	init_platform();			// Platform initialisieren
	Xil_ExceptionInit(); // Exception-Handling des Prozessors initialisiert
	initializeColortoVGAController();
	//Test Funktionen
//	loescheAlles();
//	zeichneBall(300,300,2);
//	zeichneBoard();

	//while(1);
/*
	// Init Button
	Status = XGpio_Initialize(&GpioBtn, XPAR_AXI_GPIO_1_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		print("Fehler: Buttons initialisiert\r\n");
		return XST_FAILURE;
	}
	XGpio_SetDataDirection(&GpioBtn, 1, 0xFFFFFFFF);*/

	// initialize LED's as outputs
	Status = XGpio_Initialize(&GpioOutput, XPAR_AXI_GPIO_0_DEVICE_ID);
	if (Status != XST_SUCCESS)
	{
		print("Fehler: LED's initialisiert\r\n");
		return XST_FAILURE;
	}
	XGpio_SetDataDirection(&GpioOutput, 1, 0x0);
	XGpio_DiscreteWrite(&GpioOutput, 1, 0xFFFFFFFF);

	// Interrupt-Controler initialisieren
	GicConfig = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
	if (NULL == GicConfig) {
		print("Fehler: InterruptController Lookup Config\r\n");
		return XST_FAILURE;
	}
	Status = XScuGic_CfgInitialize(&InterruptController, GicConfig, GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS)
	{
		print("Fehler: InterruptController initialisiert\r\n");
		return XST_FAILURE;
	}

	// Interrupt-Controler für Timer
	TimerConfigPtr = XScuTimer_LookupConfig(XPAR_XSCUTIMER_0_DEVICE_ID);
	if (NULL == TimerConfigPtr) {
		print("Fehler: TimerConfigPtr XScuTimer_LookupConfig\r\n");
		return XST_FAILURE;
	}
	Status = XScuTimer_CfgInitialize(&TimerInstance, TimerConfigPtr, TimerConfigPtr->BaseAddr);
	if (Status != XST_SUCCESS) {
		print("SCU Timer Interrupt XScuTimer_CfgInitialize Failed\r\n");
		return XST_FAILURE;
	}

	// globaler Interrupt-Handler an den GIC angebunden
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
								(Xil_ExceptionHandler) XScuGic_InterruptHandler,
								(void *) &InterruptController);

	// Timer-Interrupt-Controler an globalen Interrupt-Controler hängen
	Status = XScuGic_Connect(	&InterruptController,
					XPAR_SCUTIMER_INTR,
					(Xil_ExceptionHandler)TimerIntHandler,
					(void *) &InterruptController);
	if (Status != XST_SUCCESS) {
			print("Fehler: Timer-Interrupt initialisieren\r\n");
			return Status;
	}

	// links Interrupt
	Status = XScuGic_Connect(	&InterruptController,
					XPAR_FABRIC_MYIP_0_RIGHT_INTERRUPT_INTR,
					(Xil_ExceptionHandler) left_InterruptHandler,
					(void *) &InterruptController);
	if (Status != XST_SUCCESS) {
		print("Fehler: left_InterruptHandler initialisieren\r\n");
		return XST_FAILURE;
	}

	// links Interrupt 2
		Status = XScuGic_Connect(	&InterruptController,
						XPAR_FABRIC_MYIP_1_RIGHT_INTERRUPT_INTR,
						(Xil_ExceptionHandler) left_InterruptHandler2,
						(void *) &InterruptController);
		if (Status != XST_SUCCESS) {
			print("Fehler: left_InterruptHandler initialisieren\r\n");
			return XST_FAILURE;
		}

	// rechts Interreupt
	Status = XScuGic_Connect(	&InterruptController,
					XPAR_FABRIC_MYIP_0_LEFT_INTERRUPT_INTR,
					(Xil_ExceptionHandler) right_InterruptHandler,
					(void *) &InterruptController);
	if (Status != XST_SUCCESS) {
		print("Fehler: right_InterruptHandler initialisiert\r\n");
		return XST_FAILURE;
	}

	// rechts Interreupt
		Status = XScuGic_Connect(	&InterruptController,
						XPAR_FABRIC_MYIP_1_LEFT_INTERRUPT_INTR,
						(Xil_ExceptionHandler) right_InterruptHandler2,
						(void *) &InterruptController);
		if (Status != XST_SUCCESS) {
			print("Fehler: right_InterruptHandler initialisiert\r\n");
			return XST_FAILURE;
		}

	// enable interrupts for my user interrupts
	XScuGic_Enable(&InterruptController, XPAR_SCUTIMER_INTR);
	XScuGic_Enable(&InterruptController, XPAR_FABRIC_MYIP_0_LEFT_INTERRUPT_INTR);
	XScuGic_Enable(&InterruptController, XPAR_FABRIC_MYIP_0_RIGHT_INTERRUPT_INTR);

	//Interrupt for Player2
	XScuGic_Enable(&InterruptController, XPAR_FABRIC_MYIP_1_LEFT_INTERRUPT_INTR);
	XScuGic_Enable(&InterruptController, XPAR_FABRIC_MYIP_1_RIGHT_INTERRUPT_INTR);

	// enable interrupts
	Xil_ExceptionEnable();
	XScuTimer_EnableInterrupt(&TimerInstance);
	XScuTimer_EnableAutoReload(&TimerInstance);
	XScuTimer_SetPrescaler(&TimerInstance, 0xFF);

	XScuGic_SetPriorityTriggerType(&InterruptController, XPAR_SCUTIMER_INTR, 20, 3);
	XScuGic_SetPriorityTriggerType(&InterruptController, XPAR_FABRIC_MYIP_0_LEFT_INTERRUPT_INTR, 200, 3);
	XScuGic_SetPriorityTriggerType(&InterruptController, XPAR_FABRIC_MYIP_0_RIGHT_INTERRUPT_INTR, 200, 3);

	//Interrupt for Player2
	XScuGic_SetPriorityTriggerType(&InterruptController, XPAR_FABRIC_MYIP_1_LEFT_INTERRUPT_INTR, 200, 3);
	XScuGic_SetPriorityTriggerType(&InterruptController, XPAR_FABRIC_MYIP_1_RIGHT_INTERRUPT_INTR, 200, 3);

	//XScuTimer_Stop(&TimerInstance);

	setup();

	newGame();
	print("new game \r\n");

	printf("Timer started \n");
	//XScuTimer_Start(&TimerInstance);

	print("before loop \r\n");
	XScuTimer_LoadTimer(&TimerInstance, timerValue);
	XScuTimer_Start(&TimerInstance);
	while(1)
	{
		/*usleep(500);
		//print("in loop \r\n");
		XScuTimer_Stop(&TimerInstance);
		XScuTimer_LoadTimer(&TimerInstance, TIMER_LOAD_VALUE);
		XScuTimer_Start(&TimerInstance);*/
	}

	printf("Beenden\n");
	XScuTimer_Stop(&TimerInstance);

	return 0;
}

