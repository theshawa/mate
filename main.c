#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

typedef enum errorType {
  ERR_OK,
  ERR_SOURCE,
  ERR_SYNTAX
} ErrorType;

typedef struct error {
  ErrorType type;
  char* msg;
  int row, col;
} Error;

const Error ok = {
  ERR_OK,
  NULL,
  0,0
};

Error err = ok;

void print_error(){
  int pos = 0;
  switch(err.type){
  case ERR_SOURCE:
    printf("source");
    break;
  case ERR_SYNTAX:
    printf("sytax");
    pos = 1;
    errno = 1;
    break;
  default:
    printf("unknown");
    break;
  }
  if(pos) printf(" error at line %d, col %d:\n\"%s\" (%d)\n",err.row,err.col,err.msg,errno);
  else printf(" error: \"%s\" (%d)\n",err.msg,errno);
}

void exit_with_error(ErrorType type, char* msg){
  err.type = type;
  err.msg = msg;
  print_error();
  exit(errno);
}

/* typedef enum tokenType { */
/*   TKT_OP='o',       // Operators */
/*   TKT_LIT_ID='l',  // Identifiers */
/*   TKT_LIT_STR='s',  // String literals */
/*   TKT_LIT_FLOAT='f' // Float literals */
/*   TKT_LIT_INT='i' // Int literals */
/*   TKT_KW='k',  // Keywords */
/*   TKT_PUNC='p',     // Punctuation (),;{}[] */
/* } TokenType; */

typedef enum tokenType {
  TKT_OP_ARITHM=1,
  TKT_OP_COMP,
  TKT_OP_ASS,
  TKT_OP_RNG,
  TKT_OP_TER,
  TKT_OP_ACC,
  TKT_OP_LOG,
  TKT_ID,
  TKT_LIT_STR,
  TKT_LIT_INT,
  TKT_LIT_FLT,
  TKT_KW,
  TKT_TYPE,
  TKT_PUNC
} TokenType;

typedef struct token {
  TokenType type;
  char* value;
  int value_len;
  char* desc;
  struct token* next;
  int row,col;
} Token;

Token* new_token(TokenType type, char* value,size_t value_len,char* desc,int row, int col){
  Token* token = malloc(sizeof(Token));
  token->type = type;
  token->value = value;
  token->value_len = value_len;
  token->desc = desc;
  token->row = row;
  token->col = col;
  token->next = NULL;
  return token;
}

void print_token(Token* token){
  if(!token) printf("NULL\n");
  else {
    printf("%s %d:%d -> \"%.*s\"\n",
	   token->type==TKT_KW ? "kw":
	   token->type==TKT_TYPE ? "type" :
	   token->type==TKT_PUNC ? "punc" :
	   token->type==TKT_OP_ARITHM ? "op_arith" :
	   token->type==TKT_OP_ACC ? "op_acc" :
	   token->type==TKT_OP_ASS ? "op_ass" :
	   token->type==TKT_OP_COMP ? "op_comp" :
	   token->type==TKT_OP_LOG ? "op_log" :
	   token->type==TKT_OP_RNG ? "op_rng" :
	   token->type==TKT_OP_TER  ? "op_ter":
	   token->desc,
	   token->row,token->col,token->value_len,token->value
	   );

  }
}

void print_tokens(Token* token){
  while(token){
    print_token(token);
    token = token->next;
  }
}

void free_tokens(Token* tokens){
  Token* next = tokens;
  while(next){
    Token* current = next;
    next = current->next;
    if(current->type==TKT_PUNC) free(current->desc);
    free(current);
  }
}

size_t get_file_size(FILE* file){
  fpos_t original_pos;
  if(fgetpos(file,&original_pos)!=0){
    exit_with_error(ERR_SOURCE,"fgetpos() failed");
  }
  if(fseek(file,0,SEEK_END)!=0){
    exit_with_error(ERR_SOURCE,"fseek(SEEK_END) failed");
  }
  size_t len = ftell(file);
  if(fsetpos(file,&original_pos)!=0){
    exit_with_error(ERR_SOURCE,"fsetpos() failed");
  }
  return len;
}

char* read_file_contents(char* path){
  FILE* file = fopen(path,"rb");
  if(!file){
    exit_with_error(ERR_SOURCE,"file does not exists");
  }
  size_t file_size = get_file_size(file);
  char* contents = malloc(file_size+1);
  char* writer = contents;
  size_t bytes_read = 0;
  while(bytes_read<file_size){
    size_t current_bytes_read = fread(writer,1,file_size - bytes_read,file);
    if(ferror(file)){
      free(contents);
      exit_with_error(ERR_SOURCE,"error while reading file");
    }
    bytes_read = current_bytes_read;
    writer+=current_bytes_read;
    if(feof(file)) break;
  }
  *writer = '\0';
  return contents;
}

#define append_token				\
  if(!token_ite) {				\
    tokens = token;				\
    token_ite = token;				\
  }else {					\
    token_ite->next = token;			\
    token_ite = token;				\
  }						

#define kw_matcher(kw) if(strcmp(word,(kw))==0) return (kw)
char* is_keyword(char* word){
  kw_matcher("def");
  kw_matcher("true");
  kw_matcher("false");
  kw_matcher("if");
  kw_matcher("then");
  kw_matcher("elseif");
  kw_matcher("endif");
  kw_matcher("while");
  kw_matcher("foreach");
  kw_matcher("in");
  kw_matcher("loop");
  kw_matcher("endloop");
  kw_matcher("nil");
  return NULL;
}

char* is_type(char* word){
  kw_matcher("int");
  kw_matcher("double");
  kw_matcher("float");
  kw_matcher("string");
  kw_matcher("bool");
  kw_matcher("array");
  kw_matcher("map");
  return NULL;
}

const char* PUNCTUATIONS = "()[];,";
const char* OPERATORS = "+-*/%&|^~<>=!.?:";

Token* lex(char* contents){
  // TODO
  Token* tokens = NULL;
  char* current_char = contents;
  Token* token_ite = tokens;
  int row=1,col=1;
  while(1){
    // skip whitespace
    while(isspace(*current_char)) {
      if(*current_char=='\n'){
	row++;
	col=1;
      }else col++;
      current_char++;      
    }
    if(*current_char=='"' || *current_char=='\''){
      // string literal
      char matcher = *current_char;
      current_char++; // pass first matcher
      char *lit_ite = current_char;
      int row_start = row,col_start = col;
      while(*lit_ite!=matcher && *lit_ite!='\0'){
	if(*lit_ite=='\n'){
	  row++;
	  col=1;
	}else col++;
	lit_ite++;
      }
      if(*lit_ite!=matcher){
	err.row = row_start;
	err.col = col_start;
	free_tokens(tokens);
	exit_with_error(ERR_SYNTAX, "unable to find the end of string literal");
      }
      Token* token = new_token(TKT_LIT_STR, current_char, lit_ite-current_char,"str", row_start, col_start);
      append_token;
      lit_ite++; // pass last matcher
      col++;
      current_char = lit_ite;
    }else if(*current_char=='#'){
      // comment
      while(*current_char!='\n' && *current_char!='\0'){
	current_char++;
	col++;
      }
      if(*current_char=='\n'){
	row++;
	col=1;
	current_char++; // pass \n
      }
    }else if(isdigit(*current_char)){
      // int/float literal
      int row_start = row,col_start = col;
      char* lit_start = current_char;
      while(isdigit(*current_char)){
	current_char++;
	col++;
      }
      // float literal
      if(*current_char=='.' && isdigit(*(current_char+1))){
        current_char++; // pass .
	col++;
	while(isdigit(*current_char)){
	  current_char++;
	  col++;
	}
	Token *token = new_token(TKT_LIT_FLT, lit_start, current_char-lit_start, "flt", row_start, col_start);
	append_token;
      }else{
	Token *token = new_token(TKT_LIT_INT, lit_start, current_char-lit_start, "int", row_start, col_start);
	append_token;
      }
    }else if(isalpha(*current_char)){
      // identifiers
      int row_start = row,col_start=col;
      char* lit_start = current_char;
      while(isalpha(*current_char) || isdigit(*current_char) || *current_char=='_'){
	current_char++;
	col++;
      }
      int value_len = current_char-lit_start;
      char *value = malloc(value_len+1);
      strncpy(value, lit_start, value_len);
      value[value_len] = '\0';
      Token *token;
      char* desc;
      if(desc = is_keyword(value)){
	token = new_token(TKT_KW, lit_start,value_len, desc, row_start, col_start);
      }else if(desc = is_type(value)){
	token = new_token(TKT_TYPE, lit_start, value_len, desc, row_start, col_start);
      }else{
	token = new_token(TKT_ID, lit_start, value_len, "id", row_start, col_start);
      }
      append_token;
      free(value);
    }else if(strchr(PUNCTUATIONS,*current_char) && *current_char!='\0'){
      // punctuations
      char *desc = malloc(2);
      desc[0] = *current_char;
      desc[1] ='\0';
      Token* token = new_token(TKT_PUNC, current_char, 1, desc, row, col);
      append_token;
      current_char++;
      col++;
    }else if(strchr(OPERATORS, *current_char) && *current_char!='\0'){
      // operators
      char* desc;
      TokenType type;
      if(*current_char=='+'){
	// += | +
	if(*(current_char+1)=='='){
	  desc = "+=";
	  type=TKT_OP_ASS;
	}else{
	  desc = "+";
	  type=TKT_OP_ARITHM;
	}
      }else if(*current_char=='-'){
	// -= | -
	if(*(current_char+1)=='='){
	  desc = "-=";
	  type=TKT_OP_ASS;
	}else{
	  desc = "-";
	  type=TKT_OP_ARITHM;
	}
      }else if(*current_char=='^'){
	// ^= | ^
	if(*(current_char+1)=='='){
	  desc = "^=";
	  type=TKT_OP_ASS;
	}else{
	  desc = "^";
	  type=TKT_OP_ARITHM;
	}
      }else if(*current_char=='!'){
	// != | !
	if(*(current_char+1)=='='){
	  desc = "!=";
	  type=TKT_OP_COMP;
	}else{
	  desc = "!";
	  type=TKT_OP_LOG;
	}
      }else if(*current_char=='*'){
	// *= | **= | ** | *
        switch(*(current_char+1)){
	  case '=':
	    desc = "*=";
	    type=TKT_OP_ASS;
	    break;  
	  case '*':
	    if(*(current_char+2)=='='){
	      desc="**=";
	      type=TKT_OP_ASS;
	    }else{
	      desc = "**";
	      type=TKT_OP_ARITHM;
	    }
	    break;
	  default:
	    desc = "*";
	    type = TKT_OP_ARITHM;
	  }
      }else if(*current_char=='/'){
	// /= | //= | // | /
	switch(*(current_char+1)){
	case '=':
	  desc = "/=";
	  type=TKT_OP_ASS;
	  break;  
	case '/':
	  if(*(current_char+2)=='='){
	    desc="//=";
	    type=TKT_OP_ASS;
	  }else{
	    desc = "//";
	    type=TKT_OP_ARITHM;
	  }
	  break;
	default:
	  desc = "/";
	  type = TKT_OP_ARITHM;
	}
      }else if(*current_char=='&'){
	// && | &= | &
	if(*(current_char+1)=='&'){
	  desc = "&&";
	  type = TKT_OP_LOG;
	}
	else if(*(current_char+1)=='='){
	  desc="&=";
	  type=TKT_OP_ASS;
	}
	else{
	  desc="&";
	  type=TKT_OP_ARITHM;
	}
      }else if(*current_char=='|'){
	// || / |= / |
	if(*(current_char+1)=='|'){
	  desc = "||";
	  type = TKT_OP_LOG;
	}
	else if(*(current_char+1)=='='){
	  desc="|=";
	  type = TKT_OP_ASS;
	}
	else{
	  desc="|";
	  type = TKT_OP_ARITHM;
	}
      }else if(*current_char=='='){
	// = | ==
	if(*(current_char+1)=='='){
	  desc = "==";
	  type=TKT_OP_COMP;
	}else{
	  desc = "=";
	  type=TKT_OP_ASS;
	}
      }else if(*current_char=='<'){
	// <<= | << | <= | <
	if(*(current_char+1)=='<'){
	  if(*(current_char+2)=='='){
	    desc="<<=";
	    type=TKT_OP_ASS;
	  }else{
	    desc="<<";
	    type=TKT_OP_ARITHM;
	  }
	}else if(*(current_char+1)=='='){
	  desc="<=";
	  type=TKT_OP_COMP;
	}else{
	  desc="<";
	  type=TKT_OP_COMP;
	}
      }else if(*current_char=='>'){
	// >>= | >> | >= | >
	if(*(current_char+1)=='>'){
	  if(*(current_char+2)=='='){
	    desc=">>=";
	    type=TKT_OP_ASS;
	  }else{
	    desc=">>";
	    type=TKT_OP_ARITHM;
	  }
	}else if(*(current_char+1)=='='){
	  desc=">=";
	  type=TKT_OP_COMP;
	}else{
	  desc=">";
	  type=TKT_OP_COMP;
	}
      }else if(*current_char=='.'){
	// . | ..
	if(*(current_char+1)=='.'){
	  desc="..";
	  type=TKT_OP_RNG;
	}else{
	  desc=".";
	  type=TKT_OP_ASS;
	}
      }else if(*current_char=='~'){
	// ~
	desc="~";
	type=TKT_OP_ARITHM;
      }else if(*current_char=='?'){
	// ?
	desc="?";
	type=TKT_OP_TER;
      }else if(*current_char==':'){
	// :
	desc=":";
	type=TKT_OP_TER;
      }
      Token* token = new_token(type, current_char, strlen(desc), desc, row, col);
      append_token;
      col+=strlen(desc);
      current_char+=strlen(desc);
    }else if(*current_char=='\0'){
      // EOF
      break;
    } else{
      printf("UNKNOWN CHARACTER: %c\n",*current_char);
      if(*current_char=='\n'){
	row++; 
	col=1; 
      }else col++; 
      current_char++;
    }
  }
  
  return tokens;
}

int main(){  
  char *contents = read_file_contents("test.mate");
  
  // printf("CONTENTS:\n%s\n",contents);
  Token* tokens = lex(contents);
  print_tokens(tokens);

  free_tokens(tokens);
  free(contents);
  return 0;
}









