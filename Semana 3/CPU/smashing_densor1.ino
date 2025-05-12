#define PINCLK 3

// Módulos de memória
word MemoriaDados[32];
word MemoriaCodigo[64] = {0x0181, 0x0000, 0x0004, 0x0100,
                          0x0005, 0x0000, 0x0006, 0x0003,
                          0x0104, 0x0006, 0x018F, 0x0004,
                          0x0005, 0x0001, 0x0005, 0x0004,
                          0x00C0, 0x0123};

word EPROM[32] = {0xF10, 0xF10, 0xD10, 0xC13, 0xC08, 0x812, 0x400,
                  0xC00, 0xC80, 0xC40, 0xC80, 0xC40, 0xC80, 0xC40,
                  0xC80, 0xC40, 0xC04, 0xC20, 0xC04, 0xC20, 0xC04, 
                  0xC20, 0xC04, 0xC20, 0xC11, 0xC11, 0xC11, 0xC11,
                  0xC11, 0xC11, 0xC11, 0xC11};

// Memória de Dados
/*
  0x00 - x
  0x01 - i
*/

// for(i = 0, x = 0, i < 16; i++) { x += i; }
/*
  MOV V, 1						0x0181		        
  ADD V, A ; i++				0x0000			     
  MOV A, V ; A = i				0x0004
  MOV R, 0 ; R = @x				0x0100		      
  MOV V, @R ; V = x 			0x0005		    
  ADD V, A ; x += i				0x0000	     
  MOV @R, V ; x = x				0x0006		      
  MOV V, A ; V = i				0x0003		      
  MOV R, 1 ; R = @i				0x0104		     
  MOV @R, V ; i = i				0x0006		      						    
  MOV V, 15 ; V = 15			0x018F			 
  MOV A, V ; A = 15             0x0004                    
  MOV V, @R ; V = i             0x0005    
  SUB V, A ; i - 15				0x0001
  MOV V, @R ; V = i				0x0005
  MOV A, V ; A = i				0x0004
  JC -16 ; Se i < 15 loop		0x00C0					        
  JMP 17 ; HALT					0x0123
*/

// Entradas e saídas dos registos
int DPC, QPC;
boolean DC, QC;
boolean DZ, QZ;
int DV, QV;
int DA, QA;
int DR, QR;

// Saídas circuito combinatório do MC
boolean JC, JZ;

// Saídas Módulo de Controlo
boolean WR, RD, EnC, EnZ, BIT_PC0, BIT_JC0, EnV, EnA, EnR, A_1, A_0;

// Saídas de multiplexers, somadores e ALU
int Y1, Y2, Y3, Y4, ALU_S;

word MC_Address;

/*
  V - Registo 7 Bits
  R - Registo 5 Bits
  A - Registo 7 Bits
  PC - Registo 6 Bits
*/

void registo1Bit(boolean D, boolean En, boolean *Q) {
  if(En)
    *Q = D;
}

void registo7Bits(int D, boolean En, int *Q) {
  if(En)
    *Q = D;
}

void registo5Bits(int D, boolean En, int *Q) {
  if(En)
    *Q = D;
}

void registo6Bits(int D, boolean En, int *Q) {
  if(En)
    *Q = D;
}

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

void ALU_7Bits(bool sel1, bool sel0, int A, int B, boolean *C, boolean *Z, int *S) {
  
  /*
    ADD - 00
    SUB - 01
    NOT - 10
  */

  if(!sel1 & !sel0) {
    *S = A + B;
  } else if(!sel1 & sel0) {
    *S = A - B;
  } else if(sel1 & !sel0) {
    *S = ~A;
  }

  *C = *S & 0x80;
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
  registo6Bits(DPC, 1, &QPC);
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

  MC_Address = ((MemoriaCodigo[QPC] & 0x0180) >> 4) + (MemoriaCodigo[QPC] & 0x0007);

  WR = bitRead(EPROM[MC_Address], 11);
  RD = bitRead(EPROM[MC_Address], 10);
  EnC = bitRead(EPROM[MC_Address], 9);
  EnZ = bitRead(EPROM[MC_Address], 8);
  JC = bitRead(EPROM[MC_Address], 7);
  JZ = bitRead(EPROM[MC_Address], 6);
  BIT_JC0 = bitRead(EPROM[MC_Address], 5);
  EnV = bitRead(EPROM[MC_Address], 4);
  EnA = bitRead(EPROM[MC_Address], 3);
  EnR = bitRead(EPROM[MC_Address], 2);
  A_1 = bitRead(EPROM[MC_Address], 1);
  A_0 = bitRead(EPROM[MC_Address], 0);
  
  BIT_PC0 = (JC & QC) || (JZ & QZ);

}
 
void circuito() {

  ALU_7Bits(MemoriaCodigo[QPC] & 0x0002, MemoriaCodigo[QPC] & 0x0001, QV, QA, &DC, &DZ, &ALU_S);
  DPC = Y3;
  int rel5 = (MemoriaCodigo[QPC] & 0x07C) >> 2;
  if(bitRead(rel5, 4) == 1)
    rel5 |= 0xFFE0;
  mux_2x1(BIT_PC0, 1, rel5, &Y1);
  somador6Bits(Y1, QPC, &Y2);
  mux_2x1(BIT_JC0, Y2, (MemoriaCodigo[QPC] & 0x07E) >> 1, &Y3);
  mux_4x2(A_1, A_0, ALU_S, MemoriaCodigo[QPC] & 0x07F, MemoriaDados[QR], QA, &Y4);
  if(bitRead(Y4, 6) == 1)
    rel5 |= 0xFF80;
  DV = Y4;
  DA = QV;
  DR = (MemoriaCodigo[QPC] & 0x07C) >> 2;

  if(!WR) {
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

void clkInterrupt() {
  if (digitalRead(2) == LOW) { 
    risingCLK();
  } else {                     
    fallingCLK();
  }
}

void setup() {
  Serial.begin(9600);
  attachInterrupt(0, clkInterrupt, CHANGE);
  interrupts();
}

void loop() {  
  clockGenerator(PINCLK, 1);
  moduloControlo();
  circuito();
}
