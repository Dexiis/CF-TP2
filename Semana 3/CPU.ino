#define PINCLK 3

// Módulos de memória
int MemoriaDados[32];
int MemoriaCodigo[64] = {0, 0x201, 0x100, 0x080, 0x380, 0x300, 0x100,
                         0x301, 0x000, 0x384, 0x301, 0x20F, 0x080,
                         0x300, 0x180, 0x300, 0x080, 0x342, 0x3A5};
                         

// Memória de Dados
/*
  0x00 - x
  0x01 - i
*/

// Código
/*
  init
  MOV V, 1								        
  ADD V, A ; i++							     
  MOV A, V ; A = i						     
  MOV R, 0 ; R = @x						      
  MOV V, @R ; V = x 						    
  ADD V, A ; x += i						     
  MOV @R, V ; x = x						      
  MOV V, A ; V = i						      
  MOV R, 1 ; R = @i						     
  MOV @R, V ; i = i						      						    
  MOV V, 15 ; V = 15						 
  MOV A, V ; A = 15                                 
  MOV V, @R ; V = i                 
  SUB V, A ; i - 15	
  MOV V, @R ; V = i
  MOV A, V ; A = i
  JC -16 ; Se i < 15 loop							        
  JMP 18 ; HALT
*/

// Select Step
boolean step = false;

// Registos
byte DPC, QPC;
boolean DC, QC;
boolean DZ, QZ;
byte DV, QV;
byte DA, QA;
byte DR, QR;

// Saídas circuito combinatório do MC
boolean JMP, JC, JZ;

// Saídas Módulo de Controlo
boolean WR, RD, EnC, EnZ, BIT_PC0, BIT_JC0, EnV, EnA, EnR, A_1, A_0;

// Saídas de multiplexers, somadores e ALU
int Y1, Y2, Y3, Y4, ALU_S;

/*
  V - Registo 7 Bits
  R - Registo 5 Bits
  A - Registo 7 Bits
  PC - Registo 6 Bits
*/

// Bits de dependência
boolean D9, D8, D7, D1, D0;

// Funções dos registos
void registo1Bit(boolean D, boolean En, boolean *Q) {
  if(En)
    *Q = D;
}

void registo7Bits(byte D, boolean En, byte *Q) {
  if(En)
    *Q = D;
}

void registo5Bits(byte D, boolean En, byte *Q) {
  if(En)
    *Q = D;
}

void registo6Bits(byte D, boolean En, byte *Q) {
  if(En)
    *Q = D;
}

// Multiplexers
void mux_2x1(boolean sel, int A, int B, int *S) {
  *S = sel ? B : A;
}

void mux_4x2(boolean sel1, boolean sel0, int A, int B, int C, int D, int *S) {
  if(!sel1 & !sel0) {
    *S = A;
  } else if(!sel1 & sel0) {
    *S = B;
  } else if(sel1 & !sel0) {
    *S = C;
  } else {
    *S = D;
  }
}

void somador6Bits(int A, int B, int *S) {
  *S = A + B;
  *S = *S & 0x3F;
}

// ALU
void ALU_7Bits(bool sel1, bool sel0, int A, int B, boolean *C, boolean *Z, int *S) {
  /*
    ADD - 10
    SUB - 11
    NOT - 01
  */

  if(sel1 & !sel0) {
    *S = A + B;
  } else if(sel1 & sel0) {
    *S = A - B;
  } else if(!sel1 & sel0) {
    *S = ~A;
  }

  *C = *S & 0x80;
  *S = *S & 0x7F; // Colocar S a 7 Bits
  *Z = *S == 0;

}

void clockGenerator(byte pino, float f) {
  static bool state = LOW;
  static unsigned long t1, t0;
  t1 = millis();
  if (t1 - t0 >= 500. / f) {
    digitalWrite(pino, state = !state);
    t0 = t1;
  }
}

void risingCLK() {
  registo6Bits(DPC, true, &QPC);
}

void fallingCLK() {
  registo7Bits(DV, EnV, &QV);
  registo7Bits(DA, EnA, &QA);
  registo5Bits(DR, EnR, &QR);
  registo1Bit(DC, EnC, &QC);
  registo1Bit(DZ, EnZ, &QZ);
  printValues();
}

void moduloControlo() {

  // Circuito combinatório de salto
  JMP = (D9 && D8 && D7 && D0);
  JC = (D9 && D8 && !D7 && D1 && !D0);
  JZ = (D9 && D8 && !D7 && D1 && D0);

  BIT_PC0 = (JC & QC) || (JZ & QZ);
  BIT_JC0 = JMP;

  WR = (D9 && D8 && !D7 && !D1 && D0);
  RD = (D9 && D8 && !D7 && !D1 && !D0);
  EnC = (!D9 && D8);
  EnZ = (!D9 && D8) || (D9 && !D8 && D7);
  EnV = (!D9 && !D8 && !D7) || (!D9 && D8) || (D9 && !D8) || (D9 && D8 && !D7 && !D1 && !D0);
  EnA = (!D9 && !D8 && D7);
  EnR = (D9 && D8 && D7 && !D0);
  A_1 = (!D9 && !D8 && !D7) || (D9 && D8 && !D7 && !D1 && !D0);
  A_0 = (!D9 && !D8 && !D7) || (D9 && !D8 && !D7);

}
 
void circuito() {

  ALU_7Bits(D8, D7, QV, QA, &DC, &DZ, &ALU_S);
  DPC = Y3;
  int rel5 = (MemoriaCodigo[QPC] & 0x07C) >> 2;
  if(bitRead(rel5, 4) == 1) {
    rel5 -= 32;
  }
  mux_2x1(BIT_PC0, 1, rel5, &Y1);
  somador6Bits(Y1, QPC, &Y2);
  mux_2x1(BIT_JC0, Y2, (MemoriaCodigo[QPC] & 0x07E) >> 1, &Y3);
  mux_4x2(A_1, A_0, ALU_S, MemoriaCodigo[QPC] & 0x07F, MemoriaDados[QR], QA, &Y4);
  DV = Y4;
  DA = QV;
  DR = (MemoriaCodigo[QPC] & 0x07C) >> 2;

  if(WR) {
    MemoriaDados[QR] = QV;
  }

}

void printValues() {
  Serial.println();

  Serial.print("PC  = "); Serial.println(QPC);
  Serial.print("A   = "); Serial.println(QA);
  Serial.print("V   = "); Serial.println(QV);
  Serial.print("R   = "); Serial.println(QR);
  Serial.print("C   = "); Serial.println(QC);
  Serial.print("Z   = "); Serial.println(QZ);

  Serial.println();
  
  Serial.print("x = "); Serial.println(MemoriaDados[0]);
  Serial.print("i = "); Serial.println(MemoriaDados[1]);

  Serial.println();
  Serial.print("MemoriaCodigo[PC] = "); Serial.println(MemoriaCodigo[QPC]);
  
  Serial.println("========================");
}

void debugValues() {
  Serial.println();
  Serial.println("==== Estado Atual ====");
  
  // Registradores principais
  Serial.print("PC  = "); Serial.println(QPC);
  Serial.print("A   = "); Serial.println(QA);
  Serial.print("V   = "); Serial.println(QV);
  Serial.print("R   = "); Serial.println(QR);
  Serial.print("C   = "); Serial.println(QC);
  Serial.print("Z   = "); Serial.println(QZ);

  Serial.println();
  Serial.println("==== Memória de Dados ====");

  Serial.print("0x00 = "); Serial.println(MemoriaDados[0]);
  Serial.print("0x01 = "); Serial.println(MemoriaDados[1]);
  Serial.print("0x02 = "); Serial.println(MemoriaDados[2]);
  Serial.print("0x03 = "); Serial.println(MemoriaDados[3]);

  Serial.println();

  // Sinais do circuito combinatório de salto
  Serial.println("=== Sinais de Salto ===");
  Serial.print("JMP = "); Serial.println(JMP);
  Serial.print("JC  = "); Serial.println(JC);
  Serial.print("JZ  = "); Serial.println(JZ);

  Serial.println();

  // Sinais de Controle
  Serial.println("=== Sinais de Controle ===");
  Serial.print("WR      = "); Serial.println(WR);
  Serial.print("RD      = "); Serial.println(RD);
  Serial.print("EnC     = "); Serial.println(EnC);
  Serial.print("EnZ     = "); Serial.println(EnZ);
  Serial.print("BIT_PC0 = "); Serial.println(BIT_PC0);
  Serial.print("BIT_JC0 = "); Serial.println(BIT_JC0);
  Serial.print("EnV     = "); Serial.println(EnV);
  Serial.print("EnA     = "); Serial.println(EnA);
  Serial.print("EnR     = "); Serial.println(EnR);
  Serial.print("A_1     = "); Serial.println(A_1);
  Serial.print("A_0     = "); Serial.println(A_0);

  Serial.println();

  // Saídas dos multiplexers, ALU e somadores
  Serial.println("=== Dados Internos ===");
  Serial.print("Y1    = "); Serial.println(Y1);
  Serial.print("Y2    = "); Serial.println(Y2);
  Serial.print("Y3    = "); Serial.println(Y3);
  Serial.print("Y4    = "); Serial.println(Y4);
  Serial.print("ALU_S = "); Serial.println(ALU_S);

  Serial.println();

  // Bits da instrução atual
  Serial.println("=== Bits da Instrução ===");
  Serial.print("D9 = "); Serial.println(D9);
  Serial.print("D8 = "); Serial.println(D8);
  Serial.print("D7 = "); Serial.println(D7);
  Serial.print("D1 = "); Serial.println(D1);
  Serial.print("D0 = "); Serial.println(D0);
  
  Serial.println();

  // Instrução atual na memória de código
  Serial.println("=== Instrução Atual ===");
  Serial.print("MemoriaCodigo[PC] = "); Serial.println(MemoriaCodigo[QPC]);
  
  Serial.println("========================");
}

void clkInterrupt() {
  if (digitalRead(2) == HIGH) { 
    risingCLK();
  } else {                     
    fallingCLK();
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT_PULLUP);

  attachInterrupt(0, clkInterrupt, CHANGE);
  interrupts();
}

void loop() {

  D9 = bitRead(MemoriaCodigo[QPC], 9);
  D8 = bitRead(MemoriaCodigo[QPC], 8);
  D7 = bitRead(MemoriaCodigo[QPC], 7);
  D1 = bitRead(MemoriaCodigo[QPC], 1);
  D0 = bitRead(MemoriaCodigo[QPC], 0);
  
  digitalWrite(3, analogRead(A0) < 10);
  
  moduloControlo();
  circuito();
  if(!step)
    clockGenerator(PINCLK, 1);
}
