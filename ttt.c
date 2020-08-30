#ifndef F_CPU
#define F_CPU 16000000L //Sets the CPU top 8 Megahertz
#endif

#include <util/delay.h>
#include <avr/io.h>
#include <stdlib.h>

#define ONSWITCH PD0 //On switch pin number
#define Turn_On_Debug PORTB |= 0x10
#define Turn_Off_Debug PORTB &= 0xEF

#define DEBOUNCE_TIME 25 //Used to make sure the button was supposed to be pressed
#define LOCK_INPUT_TIME 100 //Used so it will give a delay before pressing the button again
#define DELAY_FOR_SNAKE 50
#define FAST 0
#define SLOW 1
#define p1 -1
#define p2 1

//Declaring the functions
void init_ports_mcu();
void checkOnState();
void clearBoard(unsigned char reset);
unsigned char Button_State();
void turnOnLight(unsigned char index, unsigned char color, unsigned char speed);
void turnOffLight(unsigned char index, unsigned char color, unsigned char speed);
void computeMove();
unsigned char secondBest(char sm);
char minimax(unsigned char depth, char player);
void checkWinner(unsigned char w);
unsigned char evaluate(unsigned char isCPU);
void checkFull();
void Winner(unsigned char three[]);
void BestOf3Winner(unsigned char color);
void Draw();

//Global Variables used
unsigned char ON, Color, Turn, CPU, p1Wins = 0, p2Wins = 0, CPU_firstMove = 1;

//Global variable to keep track of the CPUMoves gone
int CPUMoves = 0;

//Global arrays used
unsigned char board[9];
unsigned char greenBit[9] = {PC5, PC2, PC1, PC7, PA7, PD7, PD1, PD4, PD6};
unsigned char blueBit[9] = {PC4, PC3, PC0, PC6, PA6, PA5, PD2, PD3, PD5};
unsigned char wins[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};

int main(void){

	init_ports_mcu(); //Sets the digital i/o
	Turn = 0;
	Color = 1;
	CPU = 1;
	
	while(1){

		//Checks switch state and runs each turn
		while(ON){
			checkOnState();
							
			//Calculates the best turn and moves for the CPU
			if(CPU && Turn){
				computeMove();
				checkWinner(evaluate(0));
				Turn = (Turn == 1) ? 0 : 1; //Switches the turn
				CPUMoves++;
			}

			//Gets the buttons state and sets the light if it isn't null
			else{
				unsigned char state = Button_State();
				if(state != 15){
					turnOnLight(state, Turn == Color, SLOW);	
					checkWinner(evaluate(0));	
					Turn = (Turn == 1) ? 0 : 1; //Switches the turn	
				}
			}
			checkFull();				
		}

		//Clears the board and checks when switch gets put back on
		checkOnState();
	}
	return 0;
}

unsigned char Button_State(){
	//Mask used to check what buttons are pressed
	unsigned char mask = 0x01;

	for(unsigned char i = 0; i < 9; i++){

		//Reset the mask for different port
		if(i == 5)
			mask = 0x01;
			
		//Checks if a button was pressed in PORTA with debounce time
		if(i < 5){
			if(!(PINA & mask)){
				_delay_ms(DEBOUNCE_TIME);
				if(!(PINA & mask)){
					if(board[i] == 0){
						board[i] = (Turn == 0) ? p1 : p2;
						return i;
					}
				}
			}
		}

		//Checks if a button was pressed in PORTB with debounce time
		else{
			if(!(PINB & mask)){
				_delay_ms(DEBOUNCE_TIME);
				if(!(PINB & mask)){
					if(board[i] == 0){
						board[i] = (Turn == 0) ? p1 : p2;
						return i;
					}
				}
			}
		}
		mask = mask * 2; //Increments the mask
	}
	return 15;
} 
void computeMove(){
	char bestVal = -2;
	char bm = -1, sm = -1;
	
	for(unsigned char i = 0; i < 9; i++){
		if(board[i] == 0){
			board[i] = p2;
			char moveVal = -minimax(0, p1);
			board[i] = 0;
			if(moveVal > bestVal){
				sm = bm;
				bestVal = moveVal;
				bm = i;
			}
		}
	}	
	if(secondBest(sm)){
		board[sm] = p2;
		turnOnLight(sm, Turn == Color, SLOW);
	}
	else{
		board[bm] = p2;	
		turnOnLight(bm, Turn == Color, SLOW);
	}
	CPU_firstMove = 0;
}
char minimax(unsigned char depth, char player){
	char winner = evaluate(1);
	if(winner != 0) return winner * player;
	
	char move = -1;
	char score = -2;
	for(unsigned char i = 0; i < 9; i++){
		if(board[i] == 0){
			board[i] = player;
			char thisScore = -minimax(depth+1, player*-1);
			if(thisScore > score){
				move = i;
				score = thisScore;
			}
			board[i] = 0;
		}
	}
	if(move == -1)
		Turn_On_Debug;
	else
		Turn_Off_Debug;
	if(move == -1) return 0;
	return score;
}
unsigned char secondBest(char sm){
	if(sm != -1 && board[sm] == 0){
		if((CPU_firstMove && CPUMoves % 2 == 0) || CPUMoves % 8 ==0){
			return 1;
		}
	}
	return 0;
}
void checkWinner(unsigned char w){
	if(w != 15){
		Winner(wins[w]);
	}
}
unsigned char evaluate(unsigned char isCPU){
	for(unsigned char i = 0; i < 8; ++i){
		if(board[wins[i][0]] != 0 && board[wins[i][0]] == board[wins[i][1]] && board[wins[i][0]] == board[wins[i][2]]){
			if(isCPU)
				return board[wins[i][2]];
			return i;
		}
	}
	if(isCPU)
		return 0;
	return 15;
}
void checkFull(){
	for(unsigned char i = 0; i < 9; i++){
		if(board[i] == 0)
			return;
	}
	Draw();
}
void turnOnLight(unsigned char index, unsigned char color, unsigned char speed){
	
	//Checks if light should be green then sets specific pin in either PORTA, PORTC, or PORTD
	if(color){
		if(index < 4)
			PORTC &= ~(1 << greenBit[index]);
		else if(index == 4)
			PORTA &= ~(1 << greenBit[index]);
		else
			PORTD &= ~(1 << greenBit[index]);
	}

	//Light should be blue then sets specific pin in either PORTA, PORTC, or PORTD
	else{
		if(index < 4)
			PORTC &= ~(1 << blueBit[index]);
		else if(index == 4 || index == 5)
			PORTA &= ~(1 << blueBit[index]);
		else
			PORTD &= ~(1 << blueBit[index]);
	}
	
	//Locks inputs so there is a hesitantion on next reading
	if(speed)
		_delay_ms(LOCK_INPUT_TIME); 
	else
		_delay_ms(DELAY_FOR_SNAKE);
}

//Checks the switch state
void checkOnState(){
	if(PIND & (1 << ONSWITCH))
		ON = 1;
	else{
		ON = 0;
		Turn = 0;
		clearBoard(1);
	}
}

void Winner(unsigned char three[]){
	
	unsigned char color = (board[three[0]] == p2) & Color;
	
	if(!Turn)
		p1Wins++;
	else
		p2Wins++;
		
	turnOffLight(three[0], color, SLOW);
	turnOffLight(three[1], color, SLOW);
	turnOffLight(three[2], color, SLOW);
	
	_delay_ms(LOCK_INPUT_TIME);
	for(unsigned char i = 0; i < 2; i++){
		for(unsigned char j = 0; j < 6; j++){
			checkOnState();
			if(!ON)
				break;
			if(j < 3)
				turnOnLight(three[j], color, FAST);
			else
				turnOffLight(three[j-3], color, SLOW);
		}
		if(!ON)
			break;
		_delay_ms(LOCK_INPUT_TIME);
	}
	if(p1Wins == 2 || p2Wins == 2)
		BestOf3Winner(color);
	clearBoard(0);
}
void Draw(){
	unsigned char snake[9] = {8, 7, 6, 3, 4, 5, 2, 1, 0};
		
	_delay_ms(200);
	for(unsigned char i = 0; i < 9; i++){
		checkOnState();
		if(!ON)
			break;
		turnOffLight(snake[i], 0, FAST);
		turnOffLight(snake[i], 1, FAST);
	}
	clearBoard(0);
}
void BestOf3Winner(unsigned char color){
	unsigned char snake[9] = {0, 1, 2, 5, 4, 3, 6, 7, 8};
		
	_delay_ms(200);
	for(unsigned char i = 0; i < 9; i++){
		checkOnState();
		if(!ON)
			break;
		turnOffLight(snake[i], !color, FAST);
		turnOnLight(snake[i], color, FAST);
	}
	p1Wins=0;
	p2Wins=0;
	_delay_ms(1000);
}
void turnOffLight(unsigned char index, unsigned char color, unsigned char speed){

	//Checks if light was green then sets specific pin in either PORTA, PORTC, or PORTD
	if(color){
		if(index < 4)
			PORTC |= (1 << greenBit[index]);
		else if(index == 4)
			PORTA |= (1 << greenBit[index]);
		else
			PORTD |= (1 << greenBit[index]);
	}

	//Light was blue then sets specific pin in either PORTA, PORTC, or PORTD
	else{
		if(index < 4)
			PORTC |= (1 << blueBit[index]);
		else if(index == 4 || index == 5)
			PORTA |= (1 << blueBit[index]);
		else
			PORTD |= (1 << blueBit[index]);
	}
	
	//Locks inputs so there is a hesitantion on next reading
	if(speed)
		_delay_ms(DELAY_FOR_SNAKE); 
	else
		_delay_ms(DELAY_FOR_SNAKE/2);
}

//Disable all the lights
void clearBoard(unsigned char reset){

	//Clears all the ports
	PORTA = 0xFF;
	PORTC = 0xFF;
	PORTD = 0xFE;
	PORTB &= 0xEF;

	//Reset the buttons that have been pressed
	for(unsigned char i = 0; i < 9; i++){
		board[i] = 0;
	}
	if(reset){
		p1Wins = 0;
		p2Wins = 0;
	}
	CPU_firstMove = 1;
}

//Sets the ports and directions
void init_ports_mcu(){

	//Sets the pin directions in Ports (0 input, 1 output)
	DDRC = 0xFF;
	DDRA = 0xF0;
	DDRD = 0xFE;
	DDRB = 0xF0;

	//Sets the initial value (0 LOW, 1 HIGH)
	PORTC = 0xFF;
	PORTA = 0xFF;
	PORTD = 0xFE;
	PORTB = 0x0F;

	//Disable JTAG twice for regular PORTC i/o
	MCUCSR |= (1 << JTD);
	MCUCSR |= (1 << JTD);
}
