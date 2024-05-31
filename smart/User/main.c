#include "stm32f10x.h"
#include "stm32f1_rc522.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f1_delay.h"
#include "stm32f10x_i2c.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"
#include "stm32f10x_flash.h"
#include "i2c.h"
#include "i2c_lcd.h"


#define FLASH_PAGE_START_ADDRESS  0x0801FC00  // Ð?a ch? b?t d?u c?a trang Flash (1 KB cu?i cùng)
#define FLASH_PAGE_SIZE           1024        // Kích thu?c c?a trang Flash
#define UART_STATUS_REGISTER 			0x1000
#define UART_DATA_REGISTER   			0x1001
#define UART_TX_READY        (1 << 0)
#define NUM_CARDS 10
#define CARD_SIZE 5



uint16_t received_data;												// bien chua du lieu nhan duoc tu may tinh
uint32_t i,j;
uchar status;  																// Bien trang thai cua RFID
uchar str[MAX_LEN]; 													// MAX_LEN = 16
uchar serNum[5];															// mang chua id cua the		
// The master va the thanh vien
uchar Master_Card[5] = {0x53, 0x08, 0x35, 0x14, 0x7A};
uchar Member_Cards[10][5];

uint8_t num_members = 0;
uint8_t data_ready = 0;													// co kiem tra du lieu lua chon
uint8_t password[4] = {'0','0','0','0'};


//Funtion in programing
void TIM2_Config(void);
void GPIO_Config(void);
char readKeypad(void);
void Flash_Write(uint32_t address, uint8_t *data, uint32_t length);
void Flash_Read(uint32_t address, uint8_t *data, uint32_t length);
void UART_SendString(const char *str);
uint8_t Is_Master_Card(uchar *card_data);
uint8_t Is_Member_Card(uchar *card_data);
void Add_Member_Card(void);
void Remove_Member_Card(void);
void Handle_Card(uchar *card_data);
void Initialize_Member_Cards(void);
uint8_t CheckPass(uint8_t passwordInput[], uint8_t compare[]);
uint8_t CheckChangePass(uint8_t passwordInput[]);
void changePassword();
void home_open_lcd();
void LCD_screen(char str1[],char str2[]);
void Unlock();
void open_door();
// M?ng ký t? d?i di?n cho các phím trên bàn phím ma tr?n
char keymap[4][3] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}
};

void delay_milis(uint16_t ms);

void delay_milis(uint16_t ms) {
	int i = 0;
	while (i < ms * SystemCoreClock /1000) {
		__nop();
		i++;
	}
}


char readKeypad(void) {
    
		uint8_t row,col;
    // Ð?t t?t c? các hàng v? m?c cao
    GPIO_SetBits(GPIOA, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3);

    for (row = 0; row < 4; row++) {
        // Ð?t hàng hi?n t?i v? m?c th?p
        GPIO_ResetBits(GPIOA, (GPIO_Pin_0 << row));

        for (col = 0; col < 3; col++) {
            // Ki?m tra c?t hi?n t?i
            if (!GPIO_ReadInputDataBit(GPIOA, (GPIO_Pin_4 << col))) {
                // Ð?i cho d?n khi phím du?c th? ra (debouncing)
                while (!GPIO_ReadInputDataBit(GPIOA, (GPIO_Pin_4 << col)));
                // Ð?t l?i hàng hi?n t?i v? m?c cao tru?c khi thoát vòng l?p
                GPIO_SetBits(GPIOA, GPIO_Pin_0 << row);
                return keymap[row][col];
            }
        }

        // Ð?t l?i hàng hi?n t?i v? m?c cao
        GPIO_SetBits(GPIOA, GPIO_Pin_0 << row);
    }

    return 0;
}

void Flash_Write(uint32_t address, uint8_t *data, uint32_t length) {
		uint32_t i;
		uint16_t halfword;
    FLASH_Unlock();  // Mo khóa Flash de ghi
		

    // Xóa trang Flash tru?c khi ghi
    FLASH_ErasePage(address);
			// Xóa trang Flash trc khi ghi
    FLASH_ErasePage(FLASH_PAGE_START_ADDRESS);

    for (i = 0; i < length / 2; i++) {
        halfword = (data[i * 2 + 1] << 8) | data[i * 2];
        FLASH_ProgramHalfWord(address + i * 2, halfword);
    }

    //  length là le, ghi byte cuoi cùng vào m?t d?a ch? halfword cu?i cùng
    if (length % 2 != 0) {
        halfword = data[length - 1];
        FLASH_ProgramHalfWord(FLASH_PAGE_START_ADDRESS + length - 1, halfword);
    }

    FLASH_Lock();  // Khóa Flash sau khi ghi
}

void Flash_Read(uint32_t address, uint8_t *data, uint32_t length) {
    uint32_t i;
    uint16_t *flash_data = (uint16_t *)address;

    for (i = 0; i < length / 2; i++) {
        data[i * 2] = (uint8_t)flash_data[i] & 0xFF;          // L?y byte th?p
        data[i * 2 + 1] = (uint8_t)(flash_data[i] >> 8);      // L?y byte cao
    }

    // N?u length là s? l?, d?c byte cu?i cùng
    if (length % 2 != 0) {
        data[length - 1] = (uint8_t)flash_data[length / 2] & 0xFF;   // L?y byte th?p c?a halfword cu?i cùng
    }
}



void UART_SendString(const char *str) {
    
	while (*str) {
        // Doi den khi TXE (Transmit Data Register Empty) duoc thiet lap
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        
        // Gui ky tu qua UART1
        USART_SendData(USART1, *str++);
        
        // Ký t? xu?ng dòng
        //if (*(str - 1) == '\n') {
            //while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
            //USART_SendData(USART1, '\r');
        ////}
    }
}

// Ham de kiem tra xem mot the co phai la the master hay khong
uint8_t Is_Master_Card(uchar *card_data) {
    return memcmp(card_data, Master_Card, sizeof(Master_Card)) == 0;
}

// Ham de kiem tra xem mot the co phai la the thanh vien hay khong
uint8_t Is_Member_Card(uchar *card_data) {
    uint8_t i;
    for (i = 0; i < num_members; ++i) {
        if (memcmp(card_data, Member_Cards[i], sizeof(Member_Cards[i])) == 0) {
            return 1; // La the thanh vien
        }
    }
    return 0; // Khong phai the thanh vien
}

void Add_Member_Card(void) {
    char buffer[3];
    uint8_t i;
		uint8_t already_exists;					// co kiemtra the da ton tai chua
    // Kiem tra xem toi da chua
    if (num_members >= 10) {
        UART_SendString("Cannot add more member cards. Maximum limit reached.\n");
				LCD_screen("Full The!!!", "Khong the them");
        return;
    }

    // Ð?i ngu?i dùng dua th? m?i vào
    UART_SendString("Please present the new member card...\n");
		LCD_screen("Dua The vao", "");
    while (1) {
        uint8_t status = MFRC522_Request(PICC_REQIDL, str);
        if (status == MI_OK) {
            UART_SendString("Card detected: ");
            for (i = 0; i < 5; ++i) {
                sprintf(buffer, "%02X", str[i]);
                UART_SendString(buffer);
                UART_SendString(" ");
            }
            UART_SendString("\n");
            status = MFRC522_Anticoll(str);
            if (status == MI_OK) {
                memcpy(serNum, str, 5);
                UART_SendString("Card ID: ");
                for (i = 0; i < 5; ++i) {
                    sprintf(buffer, "%02X", serNum[i]);
                    UART_SendString(buffer);
                    UART_SendString(" ");
                }
                UART_SendString("\n");
                // Ki?m tra xem ID th? m?i dã t?n t?i trong danh sách thành viên chua
                already_exists = 0;
                for (i = 0; i < num_members; ++i) {
                    if (memcmp(Member_Cards[i], serNum, sizeof(serNum)) == 0) {
                        already_exists = 1;
                        break;
                    }
                }
                
                if (already_exists) {
										UART_SendString("This member card is already added.\n");
										LCD_screen("The da them","");
								}	
                    
                 else {
                    // Luu ID vào danh sách thành viên và tang bi?n d?m
                    memcpy(Member_Cards[num_members], str, sizeof(str));
                    num_members++;
										Flash_Write(FLASH_PAGE_START_ADDRESS, (uint8_t *)Member_Cards, sizeof(Member_Cards));
										//Add_Card_To_Flash(serNum, sizeof(serNum));
                    UART_SendString("Member card added successfully.\n");
										LCD_screen("Them the","Thanh Cong");
                }
								
                break; // Thoát kh?i vòng l?p sau khi thêm th? thành công ho?c g?p th? dã t?n t?i
            }
        }
    }
}

void Remove_Member_Card(void) {
		uint8_t i;
		char buffer[3];
		uint8_t found_index;
    // Ki?m tra xem danh sách th? thành viên có th? nào không
    if (num_members == 0) {
        UART_SendString("No member cards to remove.\n");
				LCD_screen("Khong co the", "De xoa");
        return;
    }

    // Nh?p ID c?a th? c?n xóa
    UART_SendString("Enter ID of member card to remove (5 bytes in hexadecimal format): \n");
		LCD_screen("Dua The vao","");
    while (1) {
        uint8_t status = MFRC522_Request(PICC_REQIDL, str);
        if (status == MI_OK) {
            UART_SendString("Card detected: ");
            
           // Hien thi chuoi hex cua mang `str`
            for (i = 0; i < 5; ++i) {
                sprintf(buffer, "%02X", str[i]);
                UART_SendString(buffer);
                UART_SendString(" ");
            }
            UART_SendString("\n");
            status = MFRC522_Anticoll(str);
            if (status == MI_OK) {
                memcpy(serNum, str, 5);
                UART_SendString("Card ID: ");
                
                // Hien thi chuoi hex cua mang `serNum`
                for (i = 0; i < 5; ++i) {
                    sprintf(buffer, "%02X", serNum[i]);
                    UART_SendString(buffer);
                    UART_SendString(" ");
                }
                UART_SendString("\n");
                found_index = 11;
                for (i = 0; i < num_members; ++i) {
                     if (memcmp(Member_Cards[i], serNum, sizeof(serNum)) == 0) {
												found_index = i;
												UART_SendString("Member card found successfully.\n");
												break;
                    }
                }

                if (found_index != 11) {
                    // Di chuy?n các th? phía sau th? c?n xóa lên tru?c
                    for (i = found_index; i < num_members - 1; ++i) {
													memcpy(Member_Cards[i], Member_Cards[i + 1], CARD_SIZE);
										}
// Xóa d? li?u cu?i cùng trong m?ng thành viên
										memset(Member_Cards[num_members - 1], 0, CARD_SIZE);
                    num_members--;
										 Flash_Write(FLASH_PAGE_START_ADDRESS, (uint8_t *)Member_Cards, sizeof(Member_Cards));
                    UART_SendString("Member card removed successfully.\n");
										LCD_screen("Xoa Thanh Cong","");
                } else {
                    UART_SendString("Member card not found in the list.\n");
										LCD_screen("The chua luu","");
                }
                break; // Thoát kh?i vòng l?p sau khi xóa th?
            }
        }
    }
		
}


void Handle_Card(uchar *card_data) {
    uint8_t option;
		char buffer[3];
		char buffer1[100];
		// ban phim
		char key;
    if (Is_Master_Card(card_data)) {
        UART_SendString("Master Card detected!\n");
        UART_SendString("1. Them the\n 2. Xoa the\n 3. Thoat\n Chon tu chon (1/2/3): \n");
				LCD_screen("Master Card","");
				Delay_ms(500);
				LCD_screen("1.Them  2.Xoa ","3. Thoat");
        while (1) {
						key = readKeypad();
						if(key != 0) {
            option = key;
            // Capture newline character
            switch (option) {
                case '1':
                    Add_Member_Card();
                    break;
                case '2':
                    Remove_Member_Card();
                    break;
                case '3':
                    UART_SendString("Thoat khoi che do quan ly the.\n");
										LCD_screen(" Welcome Home", "Pass:");
                    return;
                default:
										sprintf(buffer1,"%d",option);
										UART_SendString(buffer1);
                    UART_SendString("Lua chon khong hop le. Vui long chon lai.\n");
                    UART_SendString("Chon tu chon (1/2/3):\n");
										LCD_screen("Khong hop le","Chon 1/2/3");
                    return;//continue;
            }
            // In danh sach the thanh vien sau moi thao tac
            UART_SendString("Danh sach the thanh vien:\n");
            for (i = 0; i < num_members; ++i) {
                UART_SendString("The ");
                sprintf(buffer, "%02d: ", i + 1);
                UART_SendString(buffer);
                for (j = 0; j < 5; ++j) {
                    sprintf(buffer, "%02X", Member_Cards[i][j]);
                    UART_SendString(buffer);
                    UART_SendString(" ");
                }
                UART_SendString("\n");
            }
            UART_SendString("1. Them the\n 2. Xoa the\n 3. Thoat\n Chon tu chon (1/2/3): \n");
						LCD_screen("1.Them  2.Xoa ","3. Thoat");
					}
				}
			}
				else if (Is_Member_Card(card_data)) {
        //UART_SendString("Member Card detected!\n");
				UART_SendString("Open!\n");
				//LCD_screen("  OPEN DOOR","");
				//Unlock();
					open_door();
				UART_SendString("Close!\n");
				}	
				else {
				LCD_screen(" Unknown card","");
				UART_SendString("UNKNOWN\n");
				Delay_ms(1000);
				UART_SendString("Release\n");
        
				}
}




void Initialize_Member_Cards(void) {
    uint8_t flash_data[sizeof(Member_Cards)];
		uint8_t i; 
    Flash_Read(FLASH_PAGE_START_ADDRESS, flash_data, sizeof(flash_data));
    memcpy(Member_Cards, flash_data, sizeof(flash_data));
    num_members = 0;
		

    // Ki?m tra xem có bao nhiêu th? dã du?c luu tr? h?p l?
    for (i = 0; i < 10; ++i) {
        if (memcmp(Member_Cards[i], "\0\0\0\0\0", 5) != 0) {
            num_members++;
        }
    }
}


uint8_t CheckPass(uint8_t passwordInput[], uint8_t compare[]) {
	uint8_t i = 0;
	for (i = 0; i< 4; i++) {
		if(passwordInput[i] != compare[i]) {
			return 0;
		}
	}
	return 1;
}

uint8_t CheckChangePass(uint8_t passwordInput[]) {
	uint8_t i = 0;
	for (i = 0; i< 4; i++) {
		if(passwordInput[i] != '*') {
			return 0;
		}
	}
	return 1;
}


void changePassword() {
	uint8_t key;
	uint8_t passwordInput[4];
	uint8_t newPassword[4];
	uint8_t confirmPass[4];
	uint8_t invalid = 0;
	char buf[2];
	int i = 0;
	int same = 1;
	UART_SendString("Nhap Pass cu\n");
	LCD_screen("Nhap Pass cu:","");
	
	for (i = 0; i < 4 ;i++) {
		key = readKeypad();
		if(!key) {
			i--;
		} else {
			sprintf(buf,"%c",key);
			UART_SendString(buf);
			passwordInput[i] = key;
			LCD_Write_Chr('*');
			Delay_ms(200);
		}
	}
	UART_SendString("\n");
	if (CheckPass(passwordInput,password)) {
		LCD_screen("Nhap Pass moi:", "Pass:");
		UART_SendString("Nhap Pass moi\n");
		for (i = 0; i < 4; i++) {
			key = readKeypad();
			if(!key) {
				i--;
			} else {
				if (key == '#' || key == '*') {
					invalid = 1;
					break;
				} else {
					newPassword[i] = key;
					sprintf(buf,"%c",key);
					UART_SendString(buf);
					LCD_Write_Chr(key);
					Delay_ms(200);
				}
			}
		}
		if(invalid == 0) {
			/*
			char buf1[5];
			int len = 0;
			UART_SendString("\nMat khau moi se la ");
			for (i = 0; i< 4; i++) {
				len += sprintf(buf1+len,"%c",newPassword[i]);
			}
			UART_SendString(buf1);
			UART_SendString("\nBan co dong y doi\n");
			key = readKeypad();
			while(key != '*' && key != '#') {
				key = readKeypad();
			}
			if(key == '*') {
				for (i =0; i< 4; i++) {
					password[i] = newPassword[i];
				}
				UART_SendString("\nDoi thanh cong\n");
				UART_SendString("Mat khau moi la ");
				UART_SendString(buf1);
				UART_SendString("\n");
			} else if (key == '#') {
				UART_SendString("Chua doi mat khau\n");
				changePassword();
			}
			*/
			UART_SendString("\n");
			LCD_screen("Confirm Pass:","");
			for (i = 0; i < 4 ;i++) {
				key = readKeypad();
				if(!key) {
					i--;
				} else {
					sprintf(buf,"%c",key);
					UART_SendString(buf);
					confirmPass[i] = key;
					LCD_Write_Chr(key);
					Delay_ms(20);
				}
			}
			if (CheckPass(newPassword, confirmPass)) {
				for (i =0; i< 4; i++) {
					password[i] = newPassword[i];
				}
				LCD_screen("  Doi Pass"," Thanh cong");
				LCD_screen("  Welcome Home", "Pass:");
				UART_SendString("\n");
				return;
			} else {
				LCD_screen("Sai Confirm Pass","Doi Pass Fail");
				LCD_screen("  Welcome Home", "Pass:");
				return;
			}
		} else {
			UART_SendString("Thoat doi mat khau\n");
			//Delay_ms(500);
			LCD_screen("ERROR: Pass not","use * #");
			Delay_ms(500);
			LCD_screen("  Welcome Home", "Pass:");
			return;
		}
	} else {
		UART_SendString("\nSai mat khau\n");
		LCD_screen(" Wrong PassWord","");
		LCD_screen("  Welcome Home", "Pass:");
		return;
		//changePassword();
	}	
}



// Function LCD 

void home_open_lcd() {
	I2C_LCD_Clear();
	I2C_LCD_Puts("  OPEN DOOR");
}


void LCD_screen(char str1[],char str2[]) {
	
	I2C_LCD_Clear();
	I2C_LCD_Puts(str1);
	I2C_LCD_NewLine();
	I2C_LCD_Puts(str2);
	Delay_ms(1000);
	
}

void Unlock() {
	//Ham mo khoa servo
	TIM1->CCR1 = 2500;
	//Delay_ms(2000);
	delay_milis(2000);

	// Ð?t xung cho góc 90 d? (1.5ms pulse width)
	TIM1->CCR1 = 1500;
	//Delay_ms(1000);
	delay_milis(2000);
}


void TIM1_Config();
void open_door() {
	TIM1->CCR1 = 2500;
	delay_milis(3);
	I2C_LCD_Clear();
	I2C_LCD_Puts("   OPEN DOOR");
	UART_SendString("OPEN!\n");
	delay_milis(2000);
	TIM1->CCR1 = 1500;
	delay_milis(5);
	UART_SendString("CLOSE\n");
}
int main(void) {
    // Khoi tao cac thiet bi va chuc nang
		uint32_t i,j;
		int count = 0;
		uint8_t passwordInput[4] = {'0','0','0','1'};
    char buffer[256]; 
		int len = 0;
		char so[2];
		char key;
    Delay_Init();
    UART1_Config();
		// dc servo
		SystemInit(); // Kh?i t?o h? th?ng
    GPIO_Config(); // C?u hình GPIO
    TIM2_Config(); // C?u hình Timer 2
		TIM1_Config();
    MFRC522_Init();
		Initialize_Member_Cards();
		I2C_LCD_Init();
		I2C_LCD_BackLight(1);
		LCD_screen("  Welcome Home", "Pass:");
	
		// In ra id cua card quan ly
    UART_SendString("Master Card ID: ");
    for (i = 0; i < sizeof(Master_Card); ++i) {
        sprintf(buffer, "%02X ", Master_Card[i]);
        UART_SendString(buffer);
    }
		UART_SendString("\n");
		
		
/////// Vong lap chinh
    while (1) {
			if (count == 4) {
				if(CheckPass(passwordInput, password)) {
					UART_SendString("\nMat khau dung\n");
					//LCD_screen("  OPEN_DOOR","");
					//Unlock();
					//Delay_ms(2000);
					open_door();
					
					LCD_screen("  Welcome Home", "Pass:");
					
					//ham Mo Cua viet o day
				} else if(CheckChangePass(passwordInput)){
					changePassword();
				}	else {
					UART_SendString("MAT KHAU SAI\n");
					LCD_screen(" Wrong Pass","");
					Delay_ms(1000);
					UART_SendString("Release\n");
					LCD_screen("  Welcome Home", "Pass:");
					
				}
				count = 0;
			}
		
			// nut bam
			
			key = readKeypad();
			if(key) {
				//sprintf(so,"%c",key);
				//UART_SendString(so);
				passwordInput[count] = key;
				LCD_Write_Chr('*');
				count++;
			}
			
			// the ?
        status = MFRC522_Request(PICC_REQIDL, str);
        if (status == MI_OK) {
					count = 0; 
					UART_SendString("Card detected: ");
            
           // Hien thi chuoi hex cua mang `str`
            for (i = 0; i < 5; ++i) {
                sprintf(buffer, "%02X", str[i]);
                UART_SendString(buffer);
                UART_SendString(" ");
            }
            UART_SendString("\n");
            status = MFRC522_Anticoll(str);
            if (status == MI_OK) {
                memcpy(serNum, str, 5);
								
                // Xu ly the
                Handle_Card(serNum);
							
								UART_SendString("Card ID: ");
                for (i = 0; i < 5; ++i) {
                    sprintf(buffer, "%02X", serNum[i]);
                    UART_SendString(buffer);
                    UART_SendString(" ");
                }
                UART_SendString("\n");
								LCD_screen("  Welcome Home", "Pass:");
							
            }
					}
				}
				
		
}

// cau hinh chan Pa cho servo
void GPIO_Config(void)
{		
		GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    
    /*GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);*/
		// cau hinh ban phim


    // C?u hình các chân hàng (R1-R4) làm output
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // C?u hình các chân c?t (C1-C3) làm input Pull-Up
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void TIM2_Config(void)
{
		TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
	
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    

    // C?u hình Timer 2
    TIM_TimeBaseStructure.TIM_Period = 19999;
    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    // C?u hình PWM cho Timer 2 Channel 1
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 1500; // Giá tr? ban d?u
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);

    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);

    TIM_Cmd(TIM2, ENABLE);
}

void TIM1_Config(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    // Kích ho?t clock cho TIM1 và GPIOA
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA, ENABLE);

    // C?u hình chân PA8 (TIM1_CH1) làm d?u ra thay th?
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // C?u hình Timer 1
    TIM_TimeBaseStructure.TIM_Period = 19999; // Period cho 20ms (50Hz)
    TIM_TimeBaseStructure.TIM_Prescaler = 71; // Prescaler cho 1MHz clock (72MHz / (71+1))
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    // C?u hình PWM cho Timer 1 Channel 1
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable; // Ch? c?n thi?t cho TIM1 và TIM8
    TIM_OCInitStructure.TIM_Pulse = 1500; // Giá tr? ban d?u cho 1.5ms (v? trí trung bình c?a servo)
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High; // Ch? c?n thi?t cho TIM1 và TIM8
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset; // Ch? c?n thi?t cho TIM1 và TIM8
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);

    // Kích ho?t PWM outputs và Timer 1
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}


