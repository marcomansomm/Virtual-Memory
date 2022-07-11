#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>

#define QTD_PAGE 256
#define QTD_FRAME 128
#define TAM_MAX_PAGETABLE 256
#define TAM_MAX_PHYSICAL_MEMORY 128
#define TAM_THREADS 16

typedef struct virtual_addres{
  int page_number;
  int offset;
}Virtual;

Virtual virtual_address;

typedef struct real_address{
  int frame_number;
  int offset;
}Real;

Real real_address;

typedef struct node{
  int frame_number;
  int validation;
}elemPageTable;

typedef struct nodeTLB{
  int page_number;
  int frame_number;
  unsigned long clockLast;
}elemTLB;

typedef struct Memory{
  signed char buffer[256];
  int validation;
  unsigned long clockLast;
}Memory;

elemTLB *TLB;
Memory *physicalMemory;
elemPageTable *pageTable;
int pageFault = 0;
int TLBmiss = 0;
int TLBhit = 0; 
char instruction;
int frameSubstituido = 0;
int contador = 0;
int TLBsubstituido = 0;
int contadorTLB = 0;
FILE *arq_frame;
FILE *arq_correct;
int FIFOmemory = 0;
int LRUmemory = 0;
int FIFOTLB = 0;
int LRUTLB = 0;
unsigned long clk = 0;
int achou = 0;


int binaryToDecimal(int bit[]);
void decimalToBinary(int address);
void addPageTable(int number_frame);
void allocationFrameToPhysicalMemoryFIFO(int page_number_dec, int offset_dec, int address);
void allocationFrameToPhysicalMemoryLRU(int page_number_dec, int offset_dec, int address);
void check_pageTable(int address);
void readFrame(int address);
void atualizarPageTable(int frame);
void checkTLB(int address);
void addTLB();
void addTLBLRU();
void addTimer();
void addTimerTLB();
void checkTLBThreads(int id);

int main(int argc, char *argv[]) {
  
  int address, i = 0;
  pthread_t threads[TAM_THREADS];

  physicalMemory = malloc(sizeof(Memory) * QTD_FRAME);
  pageTable = malloc(sizeof(elemPageTable) * QTD_PAGE);
  TLB = malloc(sizeof(elemTLB) * 16);

  for(int i = 0; i<16; i++){
    TLB[i].frame_number = -1;
    TLB[i].page_number = -1;
    TLB[i].clockLast = 0;
  }

  for(int i = 0; i<256; i++){
    pageTable[i].frame_number = -1;
    pageTable[i].validation = 0;
  }

  for(int i = 0; i<128; i++){
    physicalMemory[i].validation = 0;
    physicalMemory[i].clockLast = 0;
  }
  
  FILE *arq_address;
  arq_correct = fopen("correct.txt", "w");
  arq_address = fopen(argv[1], "r");

  if(arq_address == NULL){
    printf("O arquivo não foi encontrado ou está com o formato errado\n");
    exit(1);
  }
  if(argc != 4){
    printf("Insira novamente!\nOs valores devem ser -> ./vm address.txt fifo fifo\n");
    exit(1);
  }
  if(strcmp(argv[2], "fifo") && strcmp(argv[2], "lru")){
    printf("Comando Invalido");
  }
  if(strcmp(argv[3], "fifo") && strcmp(argv[3], "lru")){
    printf("Comando Invalido");
  }
  if (!strcmp(argv[2], "fifo")){
    FIFOmemory = 1;
    LRUmemory = 0;
  }
  if(!strcmp(argv[2], "lru")){
    LRUmemory = 1;
    FIFOmemory = 0;
  }
  if(!strcmp(argv[3], "fifo")){
    FIFOTLB = 1;
    LRUTLB = 0;
  }
  if(!strcmp(argv[3], "lru")){
    FIFOTLB = 0;
    LRUTLB = 1;
  }

  while((fscanf(arq_address, "%d", &address)) != EOF){

    decimalToBinary(address);

    for(int j = 0; j<TAM_THREADS; j++){
		  pthread_create(&(threads[j]), NULL, checkTLBThreads, j);
	  }
    
    for(int j = 0; j<TAM_THREADS; j++){
		  pthread_join(threads[j], NULL);
	  }
    //checkTLB(address); // checar se o valor existe na pageTable

    if(achou == 1){
      readFrame(address);
    } else if (achou == 0) {
      check_pageTable(address);
    }

    achou = 0;

    i++;
    clk++;
  }
  fclose(arq_address);
  fprintf(arq_correct, "Number of Translated Addresses = %d", i);
  fprintf(arq_correct, "\nPage Faults = %d \nPage Fault Rate = %.3f", pageFault, (double)pageFault / i);
  fprintf(arq_correct, "\nTLB Hit = %d\nTLB Hit Rate = %.3f", TLBhit, (double)TLBhit / i);
  fclose(arq_correct);
  
  
  return 0;
}

void decimalToBinary(int address){
  int binario[32], page_number_bin[8], offset_bin[8];
  for(int i = 31; i >= 0; i--){
    if(address % 2 == 0){
      address /= 2;
      binario[i] = 0;
    } else {
      address /= 2;
      binario[i] = 1;
    }
  }
  
  for(int k =0, j = 16; j < 24; j++){
    page_number_bin[k] = binario[j];
    k++;
  }
  virtual_address.page_number = binaryToDecimal(page_number_bin);

  for(int k =0, j = 24; j < 32; j++){
    offset_bin[k] = binario[j];
    k++;
  }
  virtual_address.offset = binaryToDecimal(offset_bin);
  
}

int binaryToDecimal(int bit[]){
    int total = 0, j = 0;
    for(int i = 7; i >= 0; i--){
      total += bit[j] * pow(2, i);
      j++;
    }
    
    return total;
}

void checkTLBThreads(int id){
  if(TLB[id].page_number == virtual_address.page_number){
    TLBhit++;
    achou = 1;
  }
  //printf("Thread: %d /", id);
  //printf("TLB: %d %d %d\n", TLB[id].page_number, TLB[id].frame_number, TLB[id].clockLast);
}

void addTimer(){
  int pageNumber = virtual_address.page_number;
  int frame;
  frame = pageTable[pageNumber].frame_number;
  physicalMemory[frame].clockLast = clk; 
}

void addTimerTLB(){
  for(int i = 0; i<16; i++){
    if(TLB[i].page_number == virtual_address.page_number){
      TLB[i].clockLast = clk;
    }
  } 
}

void checkTLB(int address){
  int variavel = 0;
    for(int i = 0; i<16; i++){
        if(TLB[i].page_number == virtual_address.page_number){
          TLBhit++;
          variavel = 1;
          readFrame(address);
          break;
        }
    }
    if(variavel == 0){
      check_pageTable(address);
    }
}



void addTLB(int frame){
    if(TLBsubstituido == 16){
        TLBsubstituido = 0;
    }
    if(TLB[contadorTLB].page_number == -1 && contadorTLB < 16){
        TLB[contadorTLB].page_number = virtual_address.page_number;
        TLB[contadorTLB].frame_number = frame;
        contadorTLB++;
    } else if(FIFOTLB == 1 && contadorTLB == 16 && TLB[TLBsubstituido].page_number != -1){
        TLB[TLBsubstituido].page_number = virtual_address.page_number;
        TLB[TLBsubstituido].frame_number = frame;
        TLBsubstituido++;
    }
}


void addTLBLRU(int frame){
    if(TLB[contadorTLB].page_number == -1 && contadorTLB < 16){
        TLB[contadorTLB].page_number = virtual_address.page_number;
        TLB[contadorTLB].frame_number = frame;
        TLB[contadorTLB].clockLast = clk;
        contadorTLB++;
    } else if(contadorTLB == 16 && TLB[TLBsubstituido].page_number != -1){
      unsigned long clockInicial = 9999999;
      for(int i = 0; i < 16; i++){
        if(TLB[i].clockLast < clockInicial){
          clockInicial = TLB[i].clockLast;
          TLBsubstituido = i;
        }
      }
      TLB[TLBsubstituido].page_number = virtual_address.page_number;
      TLB[TLBsubstituido].frame_number = frame;
      TLB[TLBsubstituido].clockLast = clk;
  } 
}

void allocationFrameToPhysicalMemoryFIFO(int pageNumber, int offset, int address){
  pageFault++;

  // ler o arquivo BACKING_STORE.bin
  arq_frame = fopen("BACKING_STORE.bin", "rb");

    if(arq_frame != NULL){
      //procurar a instrução no frame
      fseek(arq_frame, virtual_address.page_number * 256, SEEK_SET);// pegar a pagina inteira
      
    } else {
      printf("\nO arquivo BACKING_STORE.bin N existe\n");
    }

    if(frameSubstituido == 128){
        frameSubstituido = 0;
    }


    if(physicalMemory[contador].validation == 0 && contador < 128){//caso a memoria não esteja sendo utilizada
      fread(physicalMemory[contador].buffer, sizeof(signed char), 256, arq_frame);//copiar a instrução do buffer
      fclose(arq_frame);
      physicalMemory[contador].validation = 1;
      addPageTable(contador);//adicionar na pageTable

      if(FIFOTLB){
        addTLB(contador);
      } else if (LRUTLB){
        addTLBLRU(contador);
      }

      int physicalAddress = 256 * pageTable[virtual_address.page_number].frame_number + virtual_address.offset;
      instruction = physicalMemory[pageTable[virtual_address.page_number].frame_number].buffer[virtual_address.offset];//pegar a instrução com o offset
      fprintf(arq_correct, "Virtual address: %d Physical address: %d Value: %d\n", address, physicalAddress, instruction);
      contador++;
    } else if(FIFOmemory = 1 && contador == 128 && physicalMemory[frameSubstituido].validation == 1){
        fread(physicalMemory[frameSubstituido].buffer, sizeof(signed char), 256, arq_frame);
        fclose(arq_frame);
        atualizarPageTable(frameSubstituido);

        if(FIFOTLB){
          addTLB(contador);
        } else if (LRUTLB){
          addTLBLRU(contador);
        }

        frameSubstituido++;
        int physicalAddress = 256 * pageTable[virtual_address.page_number].frame_number + virtual_address.offset;
        instruction = physicalMemory[pageTable[virtual_address.page_number].frame_number].buffer[virtual_address.offset];//pegar a instrução com o offset
        fprintf(arq_correct, "Virtual address: %d Physical address: %d Value: %d\n", address, physicalAddress, instruction);
        
    }
}

void allocationFrameToPhysicalMemoryLRU(int page_number_dec, int offset_dec, int address){
  pageFault++;

  // ler o arquivo BACKING_STORE.bin
  arq_frame = fopen("BACKING_STORE.bin", "rb");

  if(arq_frame != NULL){
      //procurar a instrução no frame
    fseek(arq_frame, virtual_address.page_number * 256, SEEK_SET);// pegar a pagina inteira
      
  } else {
    printf("\nO arquivo BACKING_STORE.bin N existe\n");
  }

  if(frameSubstituido == 128){
      frameSubstituido = 0;
  }


  if(physicalMemory[contador].validation == 0 && contador < 128){//caso a memoria não esteja sendo utilizada
    fread(physicalMemory[contador].buffer, sizeof(signed char), 256, arq_frame);//copiar a instrução do buffer
    fclose(arq_frame);
    physicalMemory[contador].validation = 1;
    physicalMemory[contador].clockLast = clk;
    addPageTable(contador);//adicionar na pageTable

    if(FIFOTLB){
      addTLB(contador);
    } else if (LRUTLB){
      addTLBLRU(contador);
    }
    
    int physicalAddress = 256 * pageTable[virtual_address.page_number].frame_number + virtual_address.offset;
    instruction = physicalMemory[pageTable[virtual_address.page_number].frame_number].buffer[virtual_address.offset];//pegar a instrução com o offset
    fprintf(arq_correct, "Virtual address: %d Physical address: %d Value: %d\n", address, physicalAddress, instruction);
    contador++;
  } else if(contador == 128 && physicalMemory[frameSubstituido].validation == 1){
    unsigned long clockInicial = 999999;
    for(int i = 0; i<128;i++){
      if(physicalMemory[i].clockLast < clockInicial){
        clockInicial = physicalMemory[i].clockLast;
        frameSubstituido = i;
      }
    }
    physicalMemory[frameSubstituido].clockLast = clk;
    fread(physicalMemory[frameSubstituido].buffer, sizeof(signed char), 256, arq_frame);
    fclose(arq_frame);
    atualizarPageTable(frameSubstituido);

    if(FIFOTLB){
      addTLB(contador);
    } else if (LRUTLB){
      addTLBLRU(contador);
    }

    int physicalAddress = 256 * pageTable[virtual_address.page_number].frame_number + virtual_address.offset;
    instruction = physicalMemory[pageTable[virtual_address.page_number].frame_number].buffer[virtual_address.offset];//pegar a instrução com o offset
    fprintf(arq_correct, "Virtual address: %d Physical address: %d Value: %d\n", address, physicalAddress, instruction);
  }
}

void addPageTable(int number_frame){
  pageTable[virtual_address.page_number].frame_number = number_frame;
  pageTable[virtual_address.page_number].validation = 1;
}

void atualizarPageTable(int frame){
  int i;
  for(i = 0; i<256; i++){
    if(pageTable[i].frame_number == frame){
      pageTable[i].validation = 0;
      break;
    }
  }
  pageTable[i].frame_number = -1;//zerando a variavel que saiu da memoria fisica
  //adicionar a memoria fisica a nova pagina
  pageTable[virtual_address.page_number].frame_number = frame;
  pageTable[virtual_address.page_number].validation = 1;

}

void check_pageTable(int address){
  TLBmiss++;
  if(pageTable[virtual_address.page_number].validation == 0){
    if(FIFOmemory){
      allocationFrameToPhysicalMemoryFIFO(virtual_address.page_number, virtual_address.offset, address);
    } else if (LRUmemory){
      allocationFrameToPhysicalMemoryLRU(virtual_address.page_number, virtual_address.offset, address);
    }
  } else {
    readFrame(address);
    
    if(FIFOTLB){
      addTLB(contador);
    } else if (LRUTLB){
      addTLBLRU(contador);
    }
  
  }
}

void readFrame(int address){
  addTimer();
  addTimerTLB();
  int physicalAddress = 256 * pageTable[virtual_address.page_number].frame_number + virtual_address.offset;
  instruction = physicalMemory[pageTable[virtual_address.page_number].frame_number].buffer[virtual_address.offset];//pegar a instrução com o offset
  fprintf(arq_correct, "Virtual address: %d Physical address: %d Value: %d\n", address, physicalAddress, instruction);
  address = 0;
}