# Virtual Memory Manager

- Aluno : Marcoantonio Manso De Melo Santos
- Periodo : 3     
- Turma : A
- Professor : Erico Souza Teixeira 

## Linguagens utilizadas:
<img src="https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white" width="50">
<img src="https://img.shields.io/badge/-Makefile-orange" width="70">

&nbsp;

## 1. Implementação 2 : Memoria Virtual
---
```
Este projeto consiste em escrever um programa que traduz endereços lógicos para físicos para um espaço de endereço virtual de tamanho 2^16 = 65.536 bytes. Seu programa lerá um arquivo contendo endereços lógicos e, usando um TLB e uma tabela de páginas, traduzirá cada endereço lógico para seu endereço físico correspondente e emitirá o valor do byte armazenado no endereço físico traduzido. E tambem usará os metodos de substituição FIFO e LRU, tanto na TLB quanto na PageTable.
```
&nbsp;

---
## 2. Como Compilar
```
$ make
```
### Com esse comando o codigo é compilado e cria um arquivo binário.
---
## 3. Como Rodar o codigo
&nbsp;
### 1ª Digite o nome do binário que nesse caso será -> ./vm 
### 2ª Digite o nome do arquivo que será passado os endereços logicos.
### 3ª Digite o algoritmo de substituição que será aplicado na memoria fisica -> fifo
### 4ª Digite o algoritmo de substituição que será aplicado na TLB -> fifo
&nbsp;
```
$ ./vm (nome do arquivo txt) (metodo de substituicao memoria fisica) (metodo de substituicao TLB)
```
## Exemplo:
&nbsp;
```
$ ./vm addresses.txt fifo lru
```
&nbsp;

---
## 4. Como Remover o codigo
```
$ make clean
```
### Com esse comando o codigo depois de compilado, ele remove o arquivo binário do seu diretorio.

