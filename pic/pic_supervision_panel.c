sbit LCD_RS at RE2_bit;
sbit LCD_EN at RE1_bit;
sbit LCD_D7 at RD7_bit;
sbit LCD_D6 at RD6_bit;
sbit LCD_D5 at RD5_bit;
sbit LCD_D4 at RD4_bit;

sbit LCD_RS_Direction at TRISE2_bit;
sbit LCD_EN_Direction at TRISE1_bit;
sbit LCD_D7_Direction at TRISD7_bit;
sbit LCD_D6_Direction at TRISD6_bit;
sbit LCD_D5_Direction at TRISD5_bit;
sbit LCD_D4_Direction at TRISD4_bit;

#define MODE_IDLE 0
#define MODE_PASSWORD 1
#define MODE_MENU 2
#define MODE_MONITOR 3
#define MODE_THRESHOLD 4

int mode = MODE_IDLE;
int systemState = 0; //ativo/inativo
int selectedOption = 0;// 0=default 1=temp, 2=luz, 3=nivel
int menuType = 0;

char cPassw[5] = {'\0'};
unsigned int idx_passw = 0;

int thresholdAD;
float thresholdValue;
#define TEMP_MIN 20.0
#define TEMP_MAX 30.0
#define LUZ_MIN 0
#define LUZ_MAX 1023
#define NIVEL_MIN 0
#define NIVEL_MAX 60

unsigned char ucHora, ucMinuto, ucSegundo;
unsigned char ucDia, ucMes, ucAno, ucDia_Semana;

char buffer[8];

void ShowIdleScreen();
void EnterPasswordMode();
void ShowPasswordScreen();
void HandlePasswordKey(int digit);
void AddDigit(int digit);

void EnterMenuMode(int dest);
void HandleMenuKey(int digit);
void EnterMonitor(int option);
void UpdateMonitor(int option);
void ExitToIdle();

int ReadKeypad();
int RequestValue(char cmd);
int ReadUART();

void EnterThreshold(int option);
void UpdateThresholdDisplay(int option);
void SendThreshold(int option);

void Grava_RTC();
void Leitura_RTC();
void Converte_BCD(unsigned char linha, unsigned char coluna, unsigned char valor);

void main() {
  int digit;

  TRISE = 0; //lcd EN e RS

  TRISD = 0;
  Lcd_Init();
  Lcd_Cmd(_LCD_CURSOR_OFF);
  Lcd_Cmd(_LCD_CLEAR);

  UART1_Init(9600);
  TRISD = 255;
  PORTD = 255; //botões em nível alto (não pressionado)

  TRISB = 0x38; //botões RB3, RB4 e RB5 como entrada e pinos do teclado matricial como saída
  PORTB = 255;

  TRISA0_bit = 1; //analog0 (trimpot)
  ADCON1 = 0x0E;
  ADC_Init();
  CMCON = 0b00000111;

  Grava_RTC();

  while(1){
    //botão de senha
    if(PORTB.RB3 == 0 && mode == MODE_IDLE){
      EnterPasswordMode();
      Delay_ms(300);
    }

    //botão de menu
    if(PORTB.RB4 == 0 && mode == MODE_IDLE && systemState == 1){
      EnterMenuMode(MODE_MONITOR);
      Delay_ms(300);
    }

    //botão para definir limite
    if(PORTB.RB5 == 0 && mode == MODE_IDLE && systemState == 1){
      EnterMenuMode(MODE_THRESHOLD);
      Delay_ms(300);
    }

    switch(mode){
      case MODE_IDLE:
        ShowIdleScreen();
        Delay_ms(500);
        break;

      case MODE_PASSWORD:
        digit = ReadKeypad();
        if(digit != -99){
          HandlePasswordKey(digit);
          Delay_ms(300);
        }
        break;

      case MODE_MENU:
        digit = ReadKeypad();
        if(digit != -99){
          HandleMenuKey(digit);
          Delay_ms(300);
        }
        break;

      case MODE_MONITOR:
        digit = ReadKeypad();
        if(digit == -1){
          ExitToIdle();
        } else {
          UpdateMonitor(selectedOption);
          Delay_ms(500);
        }
        break;

      case MODE_THRESHOLD:
        digit = ReadKeypad();
        if(digit == -2){
          SendThreshold(selectedOption);
          ExitToIdle();
        } else {
          UpdateThresholdDisplay(selectedOption);
          Delay_ms(300);
        }
        break;

      default: break;
    }
  }
}

// TECLADO
int ReadKeypad(){
  int digit = -99;

  PORTB.RB0 = 0;
  Delay_ms(1);
  if (PORTD.RD3 == 0) digit = 1;
  else if (PORTD.RD2 == 0) digit = 4;
  else if (PORTD.RD1 == 0) digit = 7;
  else if (PORTD.RD0 == 0) digit = -1; // <-
  PORTB.RB0 = 1;

  PORTB.RB1 = 0;
  Delay_ms(1);
  if (PORTD.RD3 == 0) digit = 2;
  else if (PORTD.RD2 == 0) digit = 5;
  else if (PORTD.RD1 == 0) digit = 8;
  else if (PORTD.RD0 == 0) digit = 0;
  PORTB.RB1 = 1;

  PORTB.RB2 = 0;
  Delay_ms(1);
  if (PORTD.RD3 == 0) digit = 3;
  else if (PORTD.RD2 == 0) digit = 6;
  else if (PORTD.RD1 == 0) digit = 9;
  else if (PORTD.RD0 == 0) digit = -2; // ->
  PORTB.RB2 = 1;

  return digit;
}

// IDLE
void ShowIdleScreen(){
  Leitura_RTC();
  TRISD=0X0F;

  if(systemState)
      Lcd_Out(1,1,"ATIVO      ");
  else
      Lcd_Out(1,1,"INATIVO    ");

  Converte_BCD(1,10,ucHora);
  Lcd_Chr_CP(':');
  Converte_BCD(1,13,ucMinuto);

  Lcd_Out(2,1,"Data:");
  Converte_BCD(2,7,ucDia);
  Lcd_Chr_CP('/');
  Converte_BCD(2,10,ucMes);
  Lcd_Chr_CP('/');
  Converte_BCD(2,13,ucAno);
  Lcd_Out(2,15,"  ");

  TRISD=255;
}

void ExitToIdle(){
  mode = MODE_IDLE;
  selectedOption = 0; //zera opção escolhida para a próxima seleção no menu
  TRISD=0X0F;
  Lcd_Cmd(_LCD_CLEAR);
  TRISD=255;
}

// SENHA
void EnterPasswordMode(){
  mode = MODE_PASSWORD;
  idx_passw = 0;
  cPassw[0] = '\0';
  ShowPasswordScreen();
}

void ShowPasswordScreen(){
  TRISD=0X0F;
  Lcd_Cmd(_LCD_CLEAR);
  Lcd_Out(1,1,"SENHA: ");
  Lcd_Out_Cp(cPassw);
  TRISD=255;
}

void AddDigit(int digit){
  if(idx_passw > 0 && digit == -1){
    cPassw[--idx_passw] = '\0';
  }
  else if(digit >= 0 && digit <= 9 && idx_passw < 4){
    cPassw[idx_passw++] = digit + '0';
    cPassw[idx_passw] = '\0';
  }
}

void HandlePasswordKey(int digit){
  if(digit == -2){ //enter
    if(idx_passw == 4){
      TRISD=0X0F;
      Lcd_Cmd(_LCD_CLEAR);

      if(strcmp(cPassw, "1234") == 0){
        systemState = !systemState;
        UART1_Write(systemState ? 'X' : 'x');
        Lcd_Out(1,1,"SISTEMA");
        Lcd_Out(2,1, systemState ? "ATIVO" : "INATIVO");
      } else {
        Lcd_Out(1,1,"SENHA");
        Lcd_Out(2,1,"INCORRETA");
      }

      TRISD=255;
      Delay_ms(2000);
      ExitToIdle();
    }
  } else {
    AddDigit(digit);
    ShowPasswordScreen();
  }
}

//menu
void EnterMenuMode(int dest){
  mode = MODE_MENU;
  menuType = dest;
  TRISD=0X0F;
  Lcd_Cmd(_LCD_CLEAR);
  Lcd_Out(1,1,"1-Temp 2-Luz");
  Lcd_Out(2,1,"3-Nivel  <- Sair");
  TRISD=255;
}

void HandleMenuKey(int digit){
  if(digit == -1){
    ExitToIdle();
    return;
  }
  if(digit >= 1 && digit <= 3 && menuType == MODE_MONITOR){
    EnterMonitor(digit);
  }
  if(digit >= 1 && digit <= 3 && menuType == MODE_THRESHOLD){
    EnterThreshold(digit);
  }
}

void EnterMonitor(int option){
  char *text;

  switch(option){
    case 1: text = "TEMPERATURA:"; break;
    case 2: text = "LUMINOSIDADE:"; break;
    case 3: text = "NIVEL:";        break;
    default: return;
  }

  selectedOption = option;
  mode = MODE_MONITOR;

  TRISD=0X0F;
  Lcd_Cmd(_LCD_CLEAR);
  Lcd_Out(1,1, text);
  TRISD=255;
}

void UpdateMonitor(int option){
  char cmd;
  char *unit;

  switch(option){
    case 1: cmd = 'T'; unit = " C";  break;
    case 2: cmd = 'L'; unit = "";    break;
    case 3: cmd = 'N'; unit = " cm"; break;
    default: return;
  }

  TRISD=0X0F;

  if(RequestValue(cmd) == 1){
    Lcd_Out(2,1, buffer);
    Lcd_Out_Cp(unit);
    Lcd_Out_Cp("        ");
  } else {
    Lcd_Out(2,1,"Sem resposta   ");
  }

  TRISD=255;
}

// UART->Arduino

int RequestValue(char cmd){
  UART1_Write(cmd);
  return ReadUART();
}

int ReadUART(){
  int b_idx = 0;
  char c;

  while(1){
    if(UART1_Data_Ready()){
      c = UART1_Read();
      if(c == '\n'){
        buffer[b_idx] = '\0';
        return 1;
      } else if(b_idx < 7){
        buffer[b_idx++] = c;
      } else {
        buffer[b_idx] = '\0';
        return -1;
      }
    }
  }
}

//THRESHOLD
void EnterThreshold(int option){
  char *text;

  switch(option){
    case 1: text = "TEMP LIMITE:";  break;
    case 2: text = "LUZ LIMITE:";   break;
    case 3: text = "NIVEL LIMITE:"; break;
    default: return;
  }

  selectedOption = option;
  mode = MODE_THRESHOLD;

  TRISD=0X0F;
  Lcd_Cmd(_LCD_CLEAR);
  Lcd_Out(1,1, text);
  TRISD=255;
}

void UpdateThresholdDisplay(int option){
  char thresholdStr[8];

  thresholdAD = ADC_Read(0);

  switch(option){
    case 1:
      thresholdValue = TEMP_MIN+((float)thresholdAD/1023.0)*(TEMP_MAX-TEMP_MIN);
      break;

    case 2:
      thresholdValue = LUZ_MIN+((float)thresholdAD/1023.0)*(LUZ_MAX - LUZ_MIN);
      break;

    case 3:
      thresholdValue = NIVEL_MIN+((float)thresholdAD/1023.0)*(NIVEL_MAX - NIVEL_MIN);
      break;
  }

  TRISD=0X0F;

  if(option == 1){
    FloatToStr(thresholdValue, thresholdStr);
    Ltrim(thresholdStr); //FloatToStr retorna string justificada à direita
    thresholdStr[5] = '\0'; //cortar para duas casas decimais
    Lcd_Out(2,1, thresholdStr);
    Lcd_Out_Cp(" C              ");
  } else {
    IntToStr(thresholdValue, thresholdStr);
    Ltrim(thresholdStr);
    Lcd_Out(2,1, thresholdStr);
    Lcd_Out_Cp("                ");
  }

  TRISD=255;
}

void SendThreshold(int option){
  char cmd;
  char thresholdStr[8];

  switch(option){
    case 1: cmd = 'A'; break; //temperatura
    case 2: cmd = 'B'; break; //luminosidade
    case 3: cmd = 'C'; break; //nivel
    default: return;
  }

  if(option == 1){
    FloatToStr(thresholdValue, thresholdStr);
    thresholdStr[5] = '\0';
  } else {
    IntToStr(thresholdValue, thresholdStr);
  }

  Ltrim(thresholdStr);

  UART1_Write(cmd);
  UART1_Write_Text(thresholdStr);
  UART1_Write('\n');
}

// RTC
void Grava_RTC(){
   I2C1_Init(100000);     // Inicializa I2C com frequência de 100KHz
   I2C1_Start();          // Inicializa a comunicação I2C
   I2C1_Wr(0xD0);         // End. fixo para DS1307: 1101000X, onde x = 0   para gravação
   I2C1_Wr(0);            // End. onde começa a programação do relógio, end. dos segundos
   I2C1_Wr(0x00);         // Inicializa com 00 segundos - em BCD
   I2C1_Wr(0x18);         // Inicializa com 00 minutos - em BCD
   I2C1_Wr(0x14);         // Inicializa com 00 horas formato 24 horas - em BCD
   I2C1_Wr(0x05);         // Inicializa com quinta - em BCD
   I2C1_Wr(0x10);         // Inicializa com dia 16 - em BCD
   I2C1_Wr(0x07);         // Inicializa com mês 05 - em BCD
   I2C1_Wr(0x26);         // Inicializa com ano 19 - em BCD
   I2C1_Stop();           // Finaliza comunicação I2C
}

void Leitura_RTC() {          // Rotina de leitura do DS1307
   I2C1_Start();              // Inicializa comunicação I2C
   I2C1_Wr(0xD0);             // End. fixo para DS1307: 1101000X, onde x = 0   para gravação
   I2C1_Wr(0);                // End. onde começa a programação do relógio, end. dos segundos
   I2C1_Repeated_Start();     // Gera no bit de start para acessar outro endereço
   I2C1_Wr(0xD1);             // End. fixo para DS1307: 1101000X, onde x=1   para leitura
   ucSegundo = I2C1_Rd(1);            // Lê o primeiro byte(segundos) - em BCD
   ucMinuto = I2C1_Rd(1);             // Lê o segundo byte(minutos) - em BCD
   ucHora = I2C1_Rd(1);               // Lê o terceiro byte(horas) - em BCD
   ucDia_Semana = I2C1_Rd(1);          // Lê o quarto byte(dia da semana) - em BCD
   ucDia = I2C1_Rd(1);          // Lê o quinto byte(dia) - em BCD
   ucMes = I2C1_Rd(1);          // Lê o sexto byte(mes) - em BCD
   ucAno = I2C1_Rd(0);          // Lê o sétimo byte(ano) - em BCD
   I2C1_Stop();               // Finaliza comunicação I2C
}

// Rotina de conversão de dados de BCD para caractere
void Converte_BCD(unsigned char ucLinha, unsigned char ucColuna,unsigned char ucValor) {
  unsigned char ucValor1, ucValor2;

  ucValor1 = (ucValor >> 4  ) + '0';    // Converte o primeiro nibble em BCD (4 bits MSB) para char
  Lcd_Chr(ucLinha,ucColuna,ucValor1);   // Escreve caractere no LCD
  ucValor2 = (ucValor & 0x0F) + '0';    // Converte o segundo nibble em BCD (4 bits LSB) para char
  Lcd_Chr_CP(ucValor2);                 // Escreve caractere no LCD
}
