/***********************************************************************/
/*  Program Name: 3-asm_pass1_u.c                                      */
/*  This program is the part of SIC/XE assembler Pass 1.	  		   */
/*  The program only identify the symbol, opcode and operand 		   */
/*  of a line of the asm file. The program do not build the            */
/*  SYMTAB.			                                               	   */
/*  2019.12.13                                                         */
/*  2021.03.26 Process error: format 1 & 2 instruction use + 		   */
/***********************************************************************/
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "2-optable.c"

/* Public variables and functions */
#define	ADDR_SIMPLE			0x01
#define	ADDR_IMMEDIATE		0x02
#define	ADDR_INDIRECT		0x04
#define	ADDR_INDEX			0x08

#define	LINE_EOF			(-1)
#define	LINE_COMMENT		(-2)
#define	LINE_ERROR			(0)
#define	LINE_CORRECT		(1)

#define MAX_SYMTAB			1000
#define MAX_LINE			10000

typedef struct
{
	char		symbol[LEN_SYMBOL];
	char		op[LEN_SYMBOL];
	char		operand1[LEN_SYMBOL];
	char		operand2[LEN_SYMBOL];
	unsigned	code;
	unsigned	fmt;
	unsigned	addressing;	
	unsigned	locctr;
} LINE;

int process_line(LINE *line);
/* return LINE_EOF, LINE_COMMENT, LINE_ERROR, LINE_CORRECT and Instruction information in *line*/

/* Private variable and function */

void init_LINE(LINE *line)
{
	line->symbol[0] = '\0';
	line->op[0] = '\0';
	line->operand1[0] = '\0';
	line->operand2[0] = '\0';
	line->code = 0x0;
	line->fmt = 0x0;
	line->addressing = ADDR_SIMPLE;
	line->locctr = 0x0;
}

int process_line(LINE *line)
/* return LINE_EOF, LINE_COMMENT, LINE_ERROR, LINE_CORRECT */
{
	char		buf[LEN_SYMBOL];
	int			c;
	int			state;
	int			ret;
	Instruction	*op;
	
	c = ASM_token(buf);		/* get the first token of a line */
	if(c == EOF)
		return LINE_EOF;
	else if((c == 1) && (buf[0] == '\n'))	/* blank line */
		return LINE_COMMENT;
	else if((c == 1) && (buf[0] == '.'))	/* a comment line */
	{
		do
		{
			c = ASM_token(buf);
		} while((c != EOF) && (buf[0] != '\n'));
		return LINE_COMMENT;
	}
	else
	{
		init_LINE(line);
		ret = LINE_ERROR;
		state = 0;
		while(state < 8)
		{
			switch(state)
			{
				case 0:
				case 1:
				case 2:
					op = is_opcode(buf);
					if((state < 2) && (buf[0] == '+'))	/* + */
					{
						line->fmt = FMT4;
						state = 2;
					}
					else	if(op != NULL)	/* INSTRUCTION */
					{
						strcpy(line->op, op->op);
						line->code = op->code;
						state = 3;
						if(line->fmt != FMT4)
						{
							line->fmt = op->fmt & (FMT1 | FMT2 | FMT3);
						}
						else if((line->fmt == FMT4) && ((op->fmt & FMT4) == 0)) /* INSTRUCTION is FMT1 or FMT 2*/
						{	/* ERROR 20210326 added */
							printf("ERROR at token %s, %s cannot use format 4 \n", buf, buf);
							ret = LINE_ERROR;
							state = 7;		/* skip following tokens in the line */
						}
					}				
					else	if(state == 0)	/* SYMBOL */
					{
						strcpy(line->symbol, buf);
						state = 1;
					}
					else		/* ERROR */
					{
						printf("ERROR at token %s\n", buf);
						ret = LINE_ERROR;
						state = 7;		/* skip following tokens in the line */
					}
					break;	
				case 3:
					if(line->fmt == FMT1 || line->code == 0x4C)	/* no operand needed */
					{
						if(c == EOF || buf[0] == '\n')
						{
							ret = LINE_CORRECT;
							state = 8;
						}
						else		/* COMMENT */
						{
							ret = LINE_CORRECT;
							state = 7;
						}
					}
					else
					{
						if(c == EOF || buf[0] == '\n')
						{
							ret = LINE_ERROR;
							state = 8;
						}
						else	if(buf[0] == '@' || buf[0] == '#')
						{
							line->addressing = (buf[0] == '#') ? ADDR_IMMEDIATE : ADDR_INDIRECT;
							state = 4;
						}
						else	/* get a symbol */
						{
							op = is_opcode(buf);
							if(op != NULL)
							{
								printf("Operand1 cannot be a reserved word\n");
								ret = LINE_ERROR;
								state = 7; 		/* skip following tokens in the line */
							}
							else
							{
								strcpy(line->operand1, buf);
								state = 5;
							}
						}
					}			
					break;		
				case 4:
					op = is_opcode(buf);
					if(op != NULL)
					{
						printf("Operand1 cannot be a reserved word\n");
						ret = LINE_ERROR;
						state = 7;		/* skip following tokens in the line */
					}
					else
					{
						strcpy(line->operand1, buf);
						state = 5;
					}
					break;
				case 5:
					if(c == EOF || buf[0] == '\n')
					{
						ret = LINE_CORRECT;
						state = 8;
					}
					else if(buf[0] == ',')
					{
						state = 6;
					}
					else	/* COMMENT */
					{
						ret = LINE_CORRECT;
						state = 7;		/* skip following tokens in the line */
					}
					break;
				case 6:
					if(c == EOF || buf[0] == '\n')
					{
						ret = LINE_ERROR;
						state = 8;
					}
					else	/* get a symbol */
					{
						op = is_opcode(buf);
						if(op != NULL)
						{
							printf("Operand2 cannot be a reserved word\n");
							ret = LINE_ERROR;
							state = 7;		/* skip following tokens in the line */
						}
						else
						{
							if(line->fmt == FMT2)
							{
								strcpy(line->operand2, buf);
								ret = LINE_CORRECT;
								state = 7;
							}
							else if((c == 1) && (buf[0] == 'x' || buf[0] == 'X'))
							{
								line->addressing = line->addressing | ADDR_INDEX;
								ret = LINE_CORRECT;
								state = 7;		/* skip following tokens in the line */
							}
							else
							{
								printf("Operand2 exists only if format 2  is used\n");
								ret = LINE_ERROR;
								state = 7;		/* skip following tokens in the line */
							}
						}
					}
					break;
				case 7:	/* skip tokens until '\n' || EOF */
					if(c == EOF || buf[0] =='\n')
						state = 8;
					break;										
			}
			if(state < 8)
				c = ASM_token(buf);  /* get the next token */
		}
		return ret;
	}
}


int main(int argc, char *argv[])
{
	int			i, j, c, line_count, limit_line,end_line;
	char		buf[LEN_SYMBOL], filename[6];
	LINE		line[MAX_LINE];

	typedef struct
	{
		char sym[LEN_SYMBOL];
		int LOCCTR;
	}SYM_tab;
	
	SYM_tab symtab[MAX_SYMTAB];
	int symtab_count = 0;

	int modification_record[MAX_LINE];
	int modify_count = 0;

	int starting_address = 0, LOCCTR = 0, header_address = 0;
	int program_length;

	int base_address = 0, LDB_address = 0;
	int base_stat = 0;

	if(argc < 2)
	{
		printf("Usage: %s fname.asm\n", argv[0]);
	}
	else
	{
		if(ASM_open(argv[1]) == NULL)
			printf("File not found!!\n");
		else
		{
			for(i = 0; (c = process_line(&(line[i]))) != LINE_EOF && line[i].code != OP_END; i++){
				if(line[i].code == OP_START && i == 0){
					starting_address = atoi(line[i].operand1);
					LOCCTR = starting_address;
					header_address = starting_address;
					line[i].locctr = LOCCTR;
				}
				else {
					if(c == LINE_ERROR || c == LINE_COMMENT)
						i--;
					else{
						line[i].locctr = LOCCTR;
						if(line[i].symbol[0] != '\0'){
							int sym_err = 0;
							for(j = 0;j < symtab_count;j++){
								if(strcmp(symtab[j].sym, line[i].symbol) == 0)
									sym_err = 1;
							}
							if(!sym_err){
								strcpy(symtab[symtab_count].sym, line[i].symbol);
								symtab[symtab_count].LOCCTR = LOCCTR;
								symtab_count++;
							}
						}
						if(line[i].code == OP_WORD)
							LOCCTR += 3;
						else if(line[i].code == OP_RESW)
							LOCCTR += atoi(line[i].operand1) * 3;
						else if(line[i].code == OP_RESB)
							LOCCTR += atoi(line[i].operand1);
						else if(line[i].code == OP_BYTE){
							if(line[i].operand1[0] == 'C')
								LOCCTR += strlen(line[i].operand1) - 3;
							else if(line[i].operand1[0] == 'X')
								LOCCTR += (int)ceil((strlen(line[i].operand1) - 3) / 2.0);
						}
						else if(line[i].fmt >= FMT1 && line[i].fmt <= FMT4){
							switch(line[i].fmt){
								case FMT1:
									LOCCTR += 1;
									break;
								case FMT2:
									LOCCTR += 2;
									break;
								case FMT3:
									LOCCTR += 3;
									break;
								case FMT4:
									LOCCTR += 4;
									break;
							}
						}
					}
				}
			}
			line[i].locctr = LOCCTR;
			line_count = i + 1;
			program_length = LOCCTR - starting_address;
			ASM_close();
			for(i = 0; i < line_count; i++){
				printf("%06X\t%-8s\t%c%-8s\t%c%s%c %s\n",
						line[i].locctr, 
						line[i].symbol, 
						line[i].fmt == FMT4?'+':'\0', 
						line[i].op, 
						line[i].addressing==ADDR_IMMEDIATE?'#':line[i].addressing==ADDR_INDIRECT?'@':'\0', 
						line[i].operand1, 
						(line[i].operand2[0] == '\0')?' ':',',
						line[i].operand2);
				if (line[i].code == OP_END){
					printf("\n\nProgram Length : %X\n\n\n", program_length);
					for (j = 0; j < symtab_count; j++){
						printf("%-8s : %06X\n", symtab[j].sym, symtab[j].LOCCTR);
						if (strcmp(line[i].operand1, symtab[j].sym) == 0)
							header_address = symtab[j].LOCCTR;
					}
				}
			}
			for(i = 0; line[0].symbol[i] != '\0' && i < 6; i++)
				filename[i] = line[0].symbol[i];
			filename[i] = '\0';
			printf("\n\nObject Program :\n\n");
			printf("H%-6s%06X%06X\n", filename, starting_address, program_length);
			for (i = 1; i < line_count;){
				if(line[i].code == OP_END)
					break;
				limit_line = i;
				while(line[limit_line + 1].locctr - line[i].locctr <= 30 && line[limit_line].code != OP_RESB && line[limit_line].code != OP_RESW)
					limit_line++;
				end_line = limit_line;
				while(line[limit_line].code == OP_RESB || line[limit_line].code == OP_RESW)
					limit_line++;
				printf("T%06X%02X", line[i].locctr, line[end_line].locctr - line[i].locctr);
				for(; i < limit_line; i++){
					if(line[i].code == 0x68)
					{
						base_stat = 1;
						if(!isdigit(line[i].operand1[0]) && line[i].addressing == ADDR_IMMEDIATE)
						{
							for(j = 0; j < symtab_count; j++){
								if(strcmp(line[i].operand1, symtab[j].sym) == 0)
									break; 	
							}
							LDB_address = symtab[j].LOCCTR;
							if(LDB_address == base_address)
								base_stat = 2;
						}
					}
					if(line[i].fmt == FMT1)
						printf("%02X",line[i].code);
					else if(line[i].fmt == FMT2){
						printf("%02X", line[i].code);
						printf("%d", 
								strcmp(line[i].operand1,"X")==0?1:
								strcmp(line[i].operand1,"L")==2?1:
								strcmp(line[i].operand1,"B")==0?3:
								strcmp(line[i].operand1,"S")==0?4:
								strcmp(line[i].operand1,"T")==0?5:
								strcmp(line[i].operand1,"F")==0?6:
								strcmp(line[i].operand1,"PC")==0?8:
								strcmp(line[i].operand1,"SW")==0?9:0);
						printf("%d", 
								strcmp(line[i].operand2,"X")==0?1:
								strcmp(line[i].operand2,"L")==2?1:
								strcmp(line[i].operand2,"B")==0?3:
								strcmp(line[i].operand2,"S")==0?4:
								strcmp(line[i].operand2,"T")==0?5:
								strcmp(line[i].operand2,"F")==0?6:
								strcmp(line[i].operand2,"PC")==0?8:
								strcmp(line[i].operand2,"SW")==0?9:0);
					}
					else if(line[i].fmt == FMT3){
						printf("%02X", line[i].code + (line[i].addressing==ADDR_IMMEDIATE?1:line[i].addressing==ADDR_INDIRECT?2:3));
						if(line[i].code == 0x4C)
							printf("0000");
						else{
							if(isdigit(line[i].operand1[0]))
								printf("%X%03X", (line[i].addressing==ADDR_INDEX?8:0), atoi(line[i].operand1));
							else{
								for(j = 0; j < symtab_count; j++){
									if(strcmp(line[i].operand1, symtab[j].sym) == 0)
										break;
								}
								int pc_range = symtab[j].LOCCTR - line[i + 1].locctr;
								if(pc_range >= -2048 && pc_range <= 2047)
									printf("%X%03X", (line[i].addressing>=ADDR_INDEX?10:2), (symtab[j].LOCCTR - line[i + 1].locctr + 4096) % 4096);
								else if(base_stat == 2 && symtab[j].LOCCTR - LDB_address >= 0 && symtab[j].LOCCTR - LDB_address <= 4095)
									printf("%X%03X", (line[i].addressing>=ADDR_INDEX?12:4), symtab[j].LOCCTR - LDB_address);
							}
						}
					}
					else if(line[i].fmt == FMT4){
						printf("%02X", line[i].code + (line[i].addressing==ADDR_IMMEDIATE?1:line[i].addressing==ADDR_INDIRECT?2:3));
						if(line[i].code == 0x4C)
							printf("000000");
						else{
							printf("%X",(line[i].addressing==ADDR_INDEX?9:1));
							if(isdigit(line[i].operand1[0]))
								printf("%05X", atoi(line[i].operand1));
							else{
								for(j = 0; j < symtab_count; j++){
									if(strcmp(line[i].operand1, symtab[j].sym) == 0)
										break;
								}
								modification_record[modify_count] = line[i].locctr - starting_address + 1;
								modify_count++;
								printf("%05X", symtab[j].LOCCTR);
							}
						}
					}
					else if(line[i].code == OP_BYTE){
						if(line[i].operand1[0] == 'C')
							for(j = 2; j < line[i + 1].locctr - line[i].locctr + 2; j++)
								printf("%02X",line[i].operand1[j]);
						else if(line[i].operand1[0] == 'X')
							for(j = 2; j < (line[i + 1].locctr - line[i].locctr) * 2 + 2; j++)
								printf("%c",line[i].operand1[j]);
					}
					else if(line[i].code == OP_WORD)
						printf("%06X", atoi(line[i].operand1));
					else if(line[i].code == OP_BASE){
						for(j = 0; j < symtab_count; j++){
								if(strcmp(line[i].operand1, symtab[j].sym) == 0)
									break;
						}
						base_address = symtab[j].LOCCTR;
						base_stat = 1;
						if(base_address == LDB_address)
							base_stat = 2;
					}
					else if(line[i].code == OP_NOBASE)
						base_stat = 0;
				}
				puts("");
			}
			for (i = 0; i < modify_count; i++)
				printf("M%06X%02X\n", modification_record[i], 5);
			printf("E%06X\n", header_address);
		}
	}
}
