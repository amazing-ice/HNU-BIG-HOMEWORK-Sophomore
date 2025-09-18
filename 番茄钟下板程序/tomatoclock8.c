#include "STC15F2K60S2.H"
#include "sys.H"
#include "displayer.H"
#include "key.h"
#include "IR.h"
#include "beep.h"
#include "adc.h"
#include "music.h"
#include "M24C02.h"
#include "DS1302.h"
#include "oled.h"
#include "uart1.h"
#include "guanshanjiu.h"
#include <string.h>


code unsigned long SysClock = 11059200;

struct_DS1302_RTC xdata time;


code char decode_table[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f,
    0x40, 0x00  
};

// ????
enum {
    MODE_NORMAL = 0,
    MODE_SET_TIME,
    MODE_COUNTDOWN,
    MODE_LED_CONTROL
};

// ??????
enum {
    CURSOR_HOUR = 0,
    CURSOR_MINUTE,
    CURSOR_SECOND
};

// ?????? (??)
enum {
    CMD_STOP = 1,
    CMD_START_TIMER,
    CMD_LED_CONTROL
};

// ?????PC????????
enum {
    CMD_SET_POMODORO_PC = 0,
    CMD_UNLOCK_PC = 1,
    CMD_LOCK_PC = 2,
    CMD_UPDATE_TIME_PC = 4
};

// ????
unsigned char xdata time_set[3] = {0, 25, 0};
unsigned char xdata current_time[3] = {0, 0, 0};
unsigned char xdata cursor_pos = CURSOR_SECOND;
unsigned char xdata function_mode = MODE_NORMAL;
unsigned char xdata blink_counter = 0;
unsigned char xdata inactivity_timer = 0;
unsigned char xdata countdown_active = 0;
unsigned char xdata led_selection = 0;
unsigned char xdata ir_tx_buf[5] = {CMD_START_TIMER, 0, 0, 0, 0};
unsigned char xdata uart1_rx_buf[5]; // ????
bit blink_state = 0;
bit countdown_paused = 0;
bit led_control_active = 0;
unsigned char xdata data_buf[5];

unsigned char xdata current_song_index = 0;
bit music_playing = 0;
bit music_paused = 0;
// ???????

// ??????
void update_display();
void handle_key1();
void handle_key2();
void handle_nav(unsigned char nav_key);
void enter_setting_mode();
void exit_setting_mode();
void start_countdown();
void stop_countdown();
void update_countdown();
void save_settings();
void load_settings();
void beep_confirm();
void beep_error();
void beep_countdown_end();
void send_tomato_command();
void enter_led_control_mode();
void exit_led_control_mode();
void update_led_display();
void send_led_command();
void send_data_to_pc(unsigned char cmd, unsigned char d1, unsigned char d2, unsigned char d3, unsigned char d4); // ????
void delay_ms(unsigned int ms); // ????
void uart_rx_callback(void);   // ????


// ????????
/*void delay_ms(unsigned int ms)
{
    unsigned int i, j;
    for (i = ms; i > 0; i--)
        for (j = 800; j > 0; j--);
}*/
extern void delay_ms(unsigned int ms);

// ?????????? (????1)
void send_data_to_pc(unsigned char cmd, unsigned char d1, unsigned char d2, unsigned char d3, unsigned char d4)
{

    data_buf[0] = cmd;
    data_buf[1] = d1;
    data_buf[2] = d2;
    data_buf[3] = d3;
    data_buf[4] = d4;
    Uart1Print(data_buf, 5);
}

void Rop_callback(){
	if(GetADC().Rop<10)
		SetBeep(1200,150);
	else if(GetADC().Rop>100){
		SetBeep(2000,150);
	}
}

// ????????????
void uart_rx_callback()
{
    // ???????,???1???,??????“???”
		
		unsigned char	received_cmd = uart1_rx_buf[0];

    // ????????? PC ?????“??/??”?? (CMD_UNLOCK_PC ??? 1)
    if (received_cmd == CMD_UNLOCK_PC)
    {	
        
			// ?????,????????????????
				SetBeep(800, 50);
				delay_ms(100); // ????????
        SetBeep(800, 50);
        if (function_mode == MODE_COUNTDOWN)
        {
            // ???,?????????? stop_countdown() ??
						
            stop_countdown();
						
        }
    }
		else if (received_cmd == CMD_LED_CONTROL){
				SetBeep(800, 50);
				delay_ms(100); // ????????
        SetBeep(800, 50);
				if (uart1_rx_buf[1]>=10){
					OLED_Clear();
					OLED_ShowCHinese(10, 2, 52);
					OLED_ShowCHinese(25, 2, 53);
					OLED_ShowCHinese(40, 2, 59);
					OLED_ShowCHinese(55, 2, 60);
					OLED_ShowCHinese(70, 2, 61);
					OLED_ShowCHinese(10, 4, 68);
					OLED_ShowCHinese(25, 4, 69);
					OLED_ShowCHinese(40, 4, 62);
					OLED_ShowCHinese(55, 4, 63);
					OLED_ShowCHinese(70, 4, 31);
					OLED_ShowCHinese(85, 4, uart1_rx_buf[1]+54);
					Seg7Print(uart1_rx_buf[2]/10, uart1_rx_buf[2]%10, 10, uart1_rx_buf[3]/10,
										uart1_rx_buf[3]%10, 10, uart1_rx_buf[4]/10, uart1_rx_buf[4]%10);
				}
				else if (uart1_rx_buf[1]==2){
					OLED_Clear();
					OLED_ShowCHinese(10, 2, 95);
					OLED_ShowCHinese(25, 2, 62);
					OLED_ShowCHinese(40, 2, 96);
					OLED_ShowCHinese(55, 2, 97);
					OLED_ShowCHinese(70, 2, 98);
					OLED_ShowCHinese(85, 2, 99);
					OLED_ShowCHinese(10, 4, 76);
					OLED_ShowCHinese(25, 4, 77);
					OLED_ShowCHinese(40, 4, 78);
					OLED_ShowCHinese(55, 4, uart1_rx_buf[2] + 78);
					OLED_ShowCHinese(70, 4, 91);
					if(uart1_rx_buf[3]==1){OLED_ShowCHinese(85, 4, 92);OLED_ShowCHinese(100, 4, 94);}
					else if(uart1_rx_buf[3]==20){OLED_ShowCHinese(85, 4, 80);OLED_ShowCHinese(100, 4, 88);}
					else if(uart1_rx_buf[3]==30){OLED_ShowCHinese(85, 4, 81);OLED_ShowCHinese(100, 4, 88);}
					else if(uart1_rx_buf[3]==11){OLED_ShowCHinese(85, 4, 88);OLED_ShowCHinese(100, 4, 94);}
					else if(uart1_rx_buf[3]==21){OLED_ShowCHinese(85, 4, 93);OLED_ShowCHinese(100, 4, 94);}
				  else if(uart1_rx_buf[3]>=2&&uart1_rx_buf[3]<=10){OLED_ShowCHinese(85, 4, 92);OLED_ShowCHinese(100, 4,88+uart1_rx_buf[3]-10 );}//x+(x-10)
					else if(uart1_rx_buf[3]>=12&&uart1_rx_buf[3]<=19){OLED_ShowCHinese(85, 4, 88);OLED_ShowCHinese(100, 4, uart1_rx_buf[3]%10+78);}
					else if(uart1_rx_buf[3]>=22&&uart1_rx_buf[3]<=29){OLED_ShowCHinese(85, 4, 93);OLED_ShowCHinese(100, 4,uart1_rx_buf[3]%10+78 );}
					time = RTC_Read();
					Seg7Print(2, 0, 2, 5, time.month/10, time.month%10, time.day/10, time.day%10);
				}
		}
			
    
    // ????????PC?????,??????? else if
    // else if (received_cmd == ANOTHER_CMD_FROM_PC) { ... }
}

// Key1, Key2 ????
void key_callback() {
    unsigned char xdata key_act;
    inactivity_timer = 0;
    
    key_act = GetKeyAct(enumKey1);
    if (key_act == enumKeyPress) {
        handle_key1();
    }
    
    key_act = GetKeyAct(enumKey2);
    if (key_act == enumKeyPress) {
        handle_key2();
    }
}

// ?????
void nav_callback() {
    unsigned char xdata act;
    inactivity_timer = 0;
    
    act = GetAdcNavAct(enumAdcNavKeyUp);
    if (act == enumKeyPress) {
        if (function_mode == MODE_NORMAL) {
            // ??:??/????
            if (music_playing) {
                if (music_paused) {
                    SetPlayerMode(enumModePlay); // ????
                    music_paused = 0;
                    beep_confirm();
                    OLED_Clear();
                    OLED_ShowCHinese(10, 2, 70); // ?
                    OLED_ShowCHinese(25, 2, 71); // ?
                    OLED_ShowCHinese(40, 2, 43); // ?
									  OLED_ShowCHinese(10, 5, 73); // ?
                    OLED_ShowCHinese(25, 5, 74); // ?
                    OLED_ShowCHinese(40, 5, 75); // ?
                }
            } else {
                // ????????
                SetMusic(84,0xFE,guanshan,sizeof(guanshan),enumMscNull);
                SetPlayerMode(enumModePlay);
                music_playing = 1;
                music_paused = 0;
                beep_confirm();
                OLED_Clear();
                OLED_ShowCHinese(10, 2, 70); // ?
                OLED_ShowCHinese(25, 2, 71); // ?
                OLED_ShowCHinese(40, 2, 43); // ?
							  OLED_ShowCHinese(10, 5, 73); // ?
                OLED_ShowCHinese(25, 5, 74); // ?
                OLED_ShowCHinese(40, 5, 75); // ?
            }
        } else {
            handle_nav(enumAdcNavKeyUp);
        }
    }
    
    act = GetAdcNavAct(enumAdcNavKeyDown);
    if (act == enumKeyPress) {
        if (function_mode == MODE_NORMAL) {
            // ??:????
            if (music_playing && !music_paused) {
                SetPlayerMode(enumModePause);
                music_paused = 1;
                beep_confirm();
                OLED_Clear();
                OLED_ShowCHinese(10, 2, 72); // ?
                OLED_ShowCHinese(25, 2, 32); // ?
                OLED_ShowCHinese(40, 2, 43); // ?
            }
        } else {
            handle_nav(enumAdcNavKeyDown);
        }
    }
    
    act = GetAdcNavAct(enumAdcNavKeyLeft);
    if (act == enumKeyPress) 
     {
            handle_nav(enumAdcNavKeyLeft);
        }
    
    
    act = GetAdcNavAct(enumAdcNavKeyRight);
    if (act == enumKeyPress) {
       
            handle_nav(enumAdcNavKeyRight);
        
    }
    
    act = GetAdcNavAct(enumAdcNavKeyCenter);
    if (act == enumKeyPress) handle_nav(enumAdcNavKeyCenter);
}

void enter_led_control_mode() {
    function_mode = MODE_LED_CONTROL;
    led_control_active = 1;
    led_selection = 0;
    LedPrint(0x01);
    beep_confirm();
    update_led_display();
    
    OLED_Clear();
	  OLED_ShowChar(10, 2, 'A', 16);
	  OLED_ShowChar(25, 2, 'I', 16);
    OLED_ShowCHinese(40, 2, 47);
    OLED_ShowCHinese(55, 2, 48);
    OLED_ShowCHinese(70, 2, 49);
}

void exit_led_control_mode() {
    function_mode = MODE_NORMAL;
    led_control_active = 0;
    LedPrint(0x00);
    //beep_confirm();
    update_display();
}

void update_led_display() {
    char xdata disp_buf[8];
    char i;
    for (i = 0; i < 8; i++) {
        disp_buf[i] = (i == 7) ? led_selection : 10;
    }
		if (led_selection == 1){
		OLED_Clear();
    OLED_ShowCHinese(10, 2, 52);
    OLED_ShowCHinese(25, 2, 53);
    OLED_ShowCHinese(40, 2, 62);
		OLED_ShowCHinese(55, 2, 63);}
		else if (led_selection == 2){
		OLED_Clear();
    OLED_ShowCHinese(10, 2, 52);
    OLED_ShowCHinese(25, 2, 53);
    OLED_ShowCHinese(40, 2, 97);
		OLED_ShowCHinese(55, 2, 98);}
		
    Seg7Print(disp_buf[0], disp_buf[1], disp_buf[2], disp_buf[3],
              disp_buf[4], disp_buf[5], disp_buf[6], disp_buf[7]);
}

void send_led_command() {
    ir_tx_buf[0] = CMD_LED_CONTROL;
    ir_tx_buf[1] = led_selection;
    ir_tx_buf[2] = 0;
    ir_tx_buf[3] = 0;
    ir_tx_buf[4] = 0;
    
    if (IrPrint(ir_tx_buf, 5) == enumIrTxOK) {
        beep_confirm();
        OLED_Clear();
        OLED_ShowCHinese(10, 2, 50);
        OLED_ShowCHinese(25, 2, 51);
        OLED_ShowCHinese(40, 2, 52);
        OLED_ShowCHinese(55, 2, 53);
        OLED_ShowCHinese(70, 2, 54);
        OLED_ShowCHinese(10, 4, 55);
        OLED_ShowCHinese(25, 4, 56);
        OLED_ShowCHinese(40, 4, 57);
        OLED_ShowCHinese(55, 4, 58);
        delay_ms(100);
    } else {
        beep_error();
    }
		if (IrPrint(ir_tx_buf, 5) == enumIrTxOK) {
        beep_confirm();
        OLED_Clear();
        OLED_ShowCHinese(10, 2, 50);
        OLED_ShowCHinese(25, 2, 51);
        OLED_ShowCHinese(40, 2, 52);
        OLED_ShowCHinese(55, 2, 53);
        OLED_ShowCHinese(70, 2, 54);
        OLED_ShowCHinese(10, 4, 55);
        OLED_ShowCHinese(25, 4, 56);
        OLED_ShowCHinese(40, 4, 57);
        OLED_ShowCHinese(55, 4, 58);
        delay_ms(100);
    } else {
        beep_error();
    }
		Uart1Print(ir_tx_buf, 5);
}

void timer_1s_callback() {
    if (function_mode == MODE_COUNTDOWN && countdown_active && !countdown_paused) {
        update_countdown();
    }
    
    if (countdown_active && current_time[0] == 0 && current_time[1] == 0 && current_time[2] == 0) {
        stop_countdown();
        beep_countdown_end();
        OLED_Clear();
        OLED_ShowCHinese(10, 2, 18);
        OLED_ShowCHinese(25, 2, 19);
        OLED_ShowCHinese(40, 2, 34);
			  OLED_ShowCHinese(10, 5, 11);
        OLED_ShowCHinese(25, 5, 12);
        OLED_ShowCHinese(40, 5, 13);
        OLED_ShowCHinese(55, 5, 32);
        OLED_ShowCHinese(70, 5, 33);
    }
		   if (music_playing && GetPlayerMode() == enumModeStop) {
        music_playing = 0;
        music_paused = 0;
        OLED_Clear();
        OLED_ShowCHinese(10, 2, 64); // ?
        OLED_ShowCHinese(25, 2, 65); // ?
        OLED_ShowCHinese(40, 2, 70); // ?
        OLED_ShowCHinese(55, 2, 71); // ?
        OLED_ShowCHinese(70, 2, 72); // ?
        OLED_ShowCHinese(85, 2, 73); // ?
    }
}


void timer_100ms_callback() {
    if (function_mode == MODE_SET_TIME) {
        blink_counter++;
        if (blink_counter >= 5) {
            blink_counter = 0;
            blink_state = !blink_state;
            update_display();
        }
    }
    
    if (function_mode == MODE_SET_TIME) {
        inactivity_timer++;
        if (inactivity_timer >= 300) {
            // exit_setting_mode();
        }
    }
}

void update_display() {
    char xdata disp_buf[8];
    
    if (function_mode == MODE_NORMAL) {
        disp_buf[0] = time_set[0] / 10;
        disp_buf[1] = time_set[0] % 10;
        disp_buf[2] = 10;
        disp_buf[3] = time_set[1] / 10;
        disp_buf[4] = time_set[1] % 10;
        disp_buf[5] = 10;
        disp_buf[6] = time_set[2] / 10;
        disp_buf[7] = time_set[2] % 10;
    } 
    else if (function_mode == MODE_SET_TIME) {
        disp_buf[0] = (blink_state || cursor_pos != CURSOR_HOUR) ? time_set[0] / 10 : 11;
        disp_buf[1] = (blink_state || cursor_pos != CURSOR_HOUR) ? time_set[0] % 10 : 11;
        disp_buf[2] = 10;
        disp_buf[3] = (blink_state || cursor_pos != CURSOR_MINUTE) ? time_set[1] / 10 : 11;
        disp_buf[4] = (blink_state || cursor_pos != CURSOR_MINUTE) ? time_set[1] % 10 : 11;
        disp_buf[5] = 10;
        disp_buf[6] = (blink_state || cursor_pos != CURSOR_SECOND) ? time_set[2] / 10 : 11;
        disp_buf[7] = (blink_state || cursor_pos != CURSOR_SECOND) ? time_set[2] % 10 : 11;
    }
    else if (function_mode == MODE_COUNTDOWN) {
        disp_buf[0] = current_time[0] / 10;
        disp_buf[1] = current_time[0] % 10;
        disp_buf[2] = 10;
        disp_buf[3] = current_time[1] / 10;
        disp_buf[4] = current_time[1] % 10;
        disp_buf[5] = 10;
        disp_buf[6] = current_time[2] / 10;
        disp_buf[7] = current_time[2] % 10;
    }
    else if (function_mode == MODE_LED_CONTROL) {
        update_led_display();
        return;
    }
    
    Seg7Print(disp_buf[0], disp_buf[1], disp_buf[2], disp_buf[3],
              disp_buf[4], disp_buf[5], disp_buf[6], disp_buf[7]);
}

// ??Key1??(??/?????)
void handle_key1() {
    // ????/???????(????/LED????)
    if (function_mode == MODE_SET_TIME || function_mode == MODE_LED_CONTROL) return;
    
    if (function_mode == MODE_NORMAL) {
        // ????:?????
        start_countdown();
        OLED_Clear();
        OLED_ShowCHinese(10, 2, 11);
        OLED_ShowCHinese(25, 2, 12);
        OLED_ShowCHinese(40, 2, 13);
        OLED_ShowCHinese(55, 2, 14);
        OLED_ShowCHinese(70, 2, 15);
        OLED_Clear();
        OLED_ShowCHinese(10, 2, 11);
        OLED_ShowCHinese(25, 2, 12);
        OLED_ShowCHinese(40, 2, 13);
        OLED_ShowCHinese(55, 2, 41);
        OLED_ShowCHinese(70, 2, 42);
        OLED_ShowCHinese(85, 2, 43);
    } 
    else if (function_mode == MODE_COUNTDOWN) {
        // ?????:????/??
        if (countdown_paused) {
            // ?????:??????
            ir_tx_buf[0] = CMD_START_TIMER;
            ir_tx_buf[1] = current_time[0];  // ????
            ir_tx_buf[2] = current_time[1];  // ????
            ir_tx_buf[3] = current_time[2];  // ????
            ir_tx_buf[4] = 0;
            Uart1Print(ir_tx_buf, 5);
            
            countdown_paused = 0;
            LedPrint(0x04); // LED3?(????)
            beep_confirm();
            OLED_Clear();
            OLED_ShowCHinese(10, 2, 11);
            OLED_ShowCHinese(25, 2, 12);
            OLED_ShowCHinese(40, 2, 13);
            OLED_ShowCHinese(55, 2, 14);
            OLED_ShowCHinese(70, 2, 15);
            OLED_Clear();
            OLED_ShowCHinese(10, 2, 11);
            OLED_ShowCHinese(25, 2, 12);
            OLED_ShowCHinese(40, 2, 13);
            OLED_ShowCHinese(55, 2, 41);
            OLED_ShowCHinese(70, 2, 42);
            OLED_ShowCHinese(85, 2, 43);
        } else {
            // ?????:??????
            ir_tx_buf[0] = CMD_STOP;
            ir_tx_buf[1] = current_time[0];  // ????
            ir_tx_buf[2] = current_time[1];  // ????
            ir_tx_buf[3] = current_time[2];  // ????
            ir_tx_buf[4] = 0;
            Uart1Print(ir_tx_buf, 5);
            
            countdown_paused = 1;
            LedPrint(0x08); // LED4?(????)
            beep_confirm();
            OLED_Clear();
            OLED_ShowCHinese(10, 2, 11);
            OLED_ShowCHinese(25, 2, 12);
            OLED_ShowCHinese(40, 2, 13);
            OLED_ShowCHinese(55, 2, 32);
            OLED_ShowCHinese(70, 2, 33);
        }
    }
}

void handle_key2() {
    if (function_mode == MODE_LED_CONTROL) {
        exit_led_control_mode();
        return;
    }
    
    if (function_mode == MODE_COUNTDOWN) {
        stop_countdown();
        beep_confirm();
			  OLED_Clear();
        OLED_ShowCHinese(10, 2, 16);
        OLED_ShowCHinese(25, 2, 17);
        OLED_ShowCHinese(40, 2, 18);
        OLED_ShowCHinese(55, 2, 19);
    } 
    else if (function_mode == MODE_NORMAL) {
        enter_setting_mode();
			  OLED_Clear();
        OLED_ShowCHinese(10, 2, 16);
        OLED_ShowCHinese(25, 2, 17);
        OLED_ShowCHinese(40, 2, 18);
        OLED_ShowCHinese(55, 2, 19);
        OLED_ShowCHinese(85, 2, 46);
    } 
    else if (function_mode == MODE_SET_TIME) {
        exit_setting_mode();
			  OLED_Clear();
        OLED_ShowCHinese(10, 2, 16);
        OLED_ShowCHinese(25, 2, 17);
        OLED_ShowCHinese(40, 2, 35);
        OLED_ShowCHinese(55, 2, 36);
    }
}

void handle_nav(unsigned char nav_key) {
    if (function_mode == MODE_SET_TIME) {
        switch (nav_key) {
            case enumAdcNavKeyLeft:
                cursor_pos = (cursor_pos + 2) % 3;
								OLED_Clear();
                OLED_ShowCHinese(10, 2, 16);
                OLED_ShowCHinese(25, 2, 17);
                OLED_ShowCHinese(40, 2, 18);
                OLED_ShowCHinese(55, 2, 19); // ????
                // ????????(?/?/?)
                if (cursor_pos == CURSOR_HOUR) OLED_ShowCHinese(85, 2, 44);
                else if (cursor_pos == CURSOR_MINUTE) OLED_ShowCHinese(85, 2, 45);
                else if (cursor_pos == CURSOR_SECOND) OLED_ShowCHinese(85, 2, 46);
                beep_confirm();
                break;
            case enumAdcNavKeyRight:
                cursor_pos = (cursor_pos + 1) % 3;
								OLED_Clear();
                OLED_ShowCHinese(10, 2, 16);
                OLED_ShowCHinese(25, 2, 17);
                OLED_ShowCHinese(40, 2, 18);
                OLED_ShowCHinese(55, 2, 19); // ????
                // ????????(?/?/?)
                if (cursor_pos == CURSOR_HOUR) OLED_ShowCHinese(85, 2, 44);
                else if (cursor_pos == CURSOR_MINUTE) OLED_ShowCHinese(85, 2, 45);
                else if (cursor_pos == CURSOR_SECOND) OLED_ShowCHinese(85, 2, 46);
                beep_confirm();
                break;
            case enumAdcNavKeyUp:
                time_set[cursor_pos]++;
                if (cursor_pos == CURSOR_HOUR && time_set[0] > 23) time_set[0] = 0;
                if ((cursor_pos == CURSOR_MINUTE || cursor_pos == CURSOR_SECOND) && time_set[cursor_pos] > 59) {
                    time_set[cursor_pos] = 0;
                }
                beep_confirm();
                break;
            case enumAdcNavKeyDown:
                if (time_set[cursor_pos] == 0) {
                    if (cursor_pos == CURSOR_HOUR) time_set[0] = 23;
                    else time_set[cursor_pos] = 59;
                } else {
                    time_set[cursor_pos]--;
                }
                beep_confirm();
                break;
            case enumAdcNavKeyCenter:
                exit_setting_mode();
                break;
        }
        send_data_to_pc(CMD_UPDATE_TIME_PC, time_set[0], time_set[1], time_set[2], 0);
        update_display();
    }
    else if (function_mode == MODE_LED_CONTROL) {
        switch (nav_key) {
            case enumAdcNavKeyRight:
                led_selection = (led_selection == 0) ? 7 : led_selection - 1;
                LedPrint(1 << led_selection);
                update_led_display();
                beep_confirm();
                break;
            case enumAdcNavKeyLeft:
                led_selection = (led_selection + 1) % 8;
                LedPrint(1 << led_selection);
                update_led_display();
                beep_confirm();
                break;
            case enumAdcNavKeyCenter:
                send_led_command();
                exit_led_control_mode();
                break;
            default:
                break;
        }
    }
    else if (function_mode == MODE_NORMAL) {
        if (nav_key == enumAdcNavKeyCenter) {
            enter_led_control_mode();
        }
    }
}

void enter_setting_mode() {
    function_mode = MODE_SET_TIME;
    cursor_pos = CURSOR_SECOND;
    blink_state = 1;
    blink_counter = 0;
    inactivity_timer = 0;
    LedPrint(0x02);
    beep_confirm();
    update_display();
    send_data_to_pc(CMD_LOCK_PC, time_set[0], time_set[1], time_set[2], 0);
}

void exit_setting_mode() {
    function_mode = MODE_NORMAL;
    save_settings();
    LedPrint(0x00);
    beep_confirm();
    update_display();
}

void start_countdown() {
    current_time[0] = time_set[0];
    current_time[1] = time_set[1];
    current_time[2] = time_set[2];
    
    function_mode = MODE_COUNTDOWN;
    countdown_active = 1;
    countdown_paused = 0;
    
    send_tomato_command();
    send_data_to_pc(CMD_SET_POMODORO_PC, current_time[0], current_time[1], current_time[2], 0);
    
    LedPrint(0x04);
    beep_confirm();
    update_display();
}

void update_countdown() {
    if (current_time[2] > 0) {
        current_time[2]--;
    } else {
        if (current_time[1] > 0) {
            current_time[1]--;
            current_time[2] = 59;
        } else {
            if (current_time[0] > 0) {
                current_time[0]--;
                current_time[1] = 59;
                current_time[2] = 59;
            }
        }
    }
    update_display();
    
    if (current_time[1] == 0 && current_time[2] == 0 && current_time[0] > 0) {
        SetBeep(1000, 50);
    }
}

void stop_countdown() {
    function_mode = MODE_NORMAL;
    countdown_active = 0;
    countdown_paused = 0;
    
    ir_tx_buf[0] = CMD_STOP;
    IrPrint(ir_tx_buf, 5);
    send_data_to_pc(CMD_UNLOCK_PC, 0, 0, 0, 0);
    
    LedPrint(0x00);
    update_display();
}

void send_tomato_command() {
    ir_tx_buf[0] = CMD_START_TIMER;
    ir_tx_buf[1] = time_set[0];
    ir_tx_buf[2] = time_set[1];
    ir_tx_buf[3] = time_set[2];
    ir_tx_buf[4] = 0;
    
    if (IrPrint(ir_tx_buf, 5) == enumIrTxOK) {
        SetBeep(1200, 20);
        SetBeep(1500, 20);
    } else {
        beep_error();
    }
}

void save_settings() {
    NVM_Write(0, time_set[0]);
    NVM_Write(1, time_set[1]);
    NVM_Write(2, time_set[2]);
}

void load_settings() {
    unsigned char hour = NVM_Read(0);
    unsigned char minute = NVM_Read(1);
    unsigned char second = NVM_Read(2);
    
    if (hour <= 23 && minute <= 59 && second <= 59) {
        time_set[0] = hour;
        time_set[1] = minute;
        time_set[2] = second;
    }
    
    // OLED startup sequence from your code
    OLED_ShowCHinese(25, 2, 37);
    OLED_ShowCHinese(40, 2, 38);
    OLED_ShowCHinese(55, 2, 39);
    OLED_ShowCHinese(70, 2, 40);
    delay_ms(1500); // Changed from delay() to delay_ms()
    OLED_Clear();
    OLED_ShowCHinese(10, 2, 0);
    OLED_ShowCHinese(25, 2, 1);
    OLED_ShowCHinese(40, 2, 2);
    OLED_ShowCHinese(55, 2, 3);
    OLED_ShowCHinese(10, 5, 4);
    OLED_ShowCHinese(25, 5, 5);
    OLED_ShowCHinese(40, 5, 6);
    OLED_ShowCHinese(55, 5, 7);
    OLED_ShowCHinese(70, 5, 8);
    OLED_ShowCHinese(85, 5, 9);
    OLED_ShowCHinese(100, 5, 10);
    delay_ms(1500); // Changed from delay() to delay_ms()
    OLED_Clear();
    OLED_ShowCHinese(10, 2, 28);
    OLED_ShowCHinese(25, 2, 29);
    OLED_ShowCHinese(40, 2, 30);
    OLED_ShowCHinese(55, 2, 31);
    OLED_ShowCHinese(70, 2, 20);
    OLED_ShowCHinese(85, 2, 21);
    OLED_ShowCHinese(10, 5, 22);
    OLED_ShowCHinese(25, 5, 23);
    OLED_ShowCHinese(40, 5, 24);
    OLED_ShowCHinese(70, 5, 25);
    OLED_ShowCHinese(85, 5, 27);
    OLED_ShowCHinese(100, 5, 26);
}

void beep_confirm() {
    SetBeep(1000, 10);
}

void beep_error() {
    SetBeep(500, 10);
    SetBeep(300, 10);
}

void beep_countdown_end() {
    SetBeep(1500, 100);
    SetBeep(1200, 100);
    SetBeep(1500, 100);
    LedPrint(0x0F);
}

void main() {
    // DS1302 Init
    time.year = 2025; time.day = 18; time.month = 9;
    time.hour = 1; time.minute = 18; time.second = 0;
    DS1302Init(time);
		// RTC_Write(time);
    
    // Hardware Init
    Uart1Init(9600);
    KeyInit();
    DisplayerInit();
	  MusicPlayerInit();
    BeepInit();
    IrInit(NEC_R05d);
    AdcInit(ADCexpEXT);
    OLED_Init();
    OLED_Clear();
    
    // Load Settings
    load_settings();
    
    // Register Callbacks
    SetEventCallBack(enumEventKey, key_callback);
    SetEventCallBack(enumEventNav, nav_callback);
	  SetEventCallBack(enumEventXADC, Rop_callback);//ÎÂ¶È
    SetEventCallBack(enumEventSys100mS, timer_100ms_callback);
    SetEventCallBack(enumEventSys1S, timer_1s_callback);
    SetEventCallBack(enumEventUart1Rxd, uart_rx_callback);
    
    // Setup Receivers
    // Setup Receivers
// ???????,????????????? 5 ???
		SetUart1Rxd(uart1_rx_buf, 5, NULL, 20); 
		SetIrRxd(ir_tx_buf, 5);
    
    // System Core Init
    MySTC_Init();
    
    // Initial State
    update_display();
    LedPrint(0x00);
    
    // Handshake with PC
    delay_ms(1500);
    Uart1Print("RDY\n", 4);
    beep_confirm();
		delay_ms(500);
    while(1) {
        MySTC_OS();
    }
}