#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------token------------
typedef enum {
	TK_RESERVED, // symbol
	TK_EQ, // ==
	TK_NQ, // !=
	TK_LE, // <=
	TK_GE, // >=
	TK_NUM,		 // number token
	TK_EOF,		 // end of input token
} TokenKind;

typedef struct Token Token;

// token type
struct Token {
	TokenKind kind;
	Token *next;
	int val;
	char *str;
	int len;
};

// now focused token
Token *token;

// ----------Node--------------
typedef enum {
	ND_ADD, // +
	ND_SUB, // -
	ND_MUL, // *
	ND_DIV, // /
	ND_NUM, // number
} NodeKind;

typedef struct Node Node;

struct Node {
	NodeKind kind;
	Node *lhs;
	Node *rhs;
	int val;
};

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *term();

// error report function
// like printf
char *user_input;

void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, ""); // pos white space
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// if next token is ok symbol, progress one token and return true
// otherwise false
bool consume(char *op) {
	if (token->kind != TK_RESERVED ||
		strlen(op) != token->len ||
		memcmp(token->str, op, token->len))
		return false;
	token = token->next;
	return true;
}

void expect(char *op) {
	if (token->kind != TK_RESERVED ||
		strlen(op) != token->len ||
		memcmp(token->str, op, token->len))
		error_at(token->str, "'%c' is unexpected", op);
	token = token->next;
}

// next token is number, progress one token
// report error
int expect_number() {
	if (token->kind != TK_NUM)
		error_at(token->str, "Not a number");
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof() {
	return token->kind ==TK_EOF;
}

// --------------debug_util----------------
void debug_show(Token *cur) {
	Token *target = cur;
	for(;;){
		if (target == NULL)break;
		printf("kind %d 0==NUM, 1==other\n", target->kind != TK_NUM);
		printf("str is %s\n", target->str);
		printf("val is %d\n", target->val);
		printf("length is %d\n", target->len);
		printf("-----------------\n");
		target = target->next;
	}
}

// -------------token-----------------------

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	tok->len = len;
	cur->next = tok;
	return tok;
}

// return tokenized
Token *tokenize(char *p) {
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while (*p) {
		// whitespace skip
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (strncmp(p, "==", 2) == 0) {
			cur = new_token(TK_EQ, cur, p, 2);
			p+=2;
			continue;
		}
		// if ()//wip
		if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>') {
			cur = new_token(TK_RESERVED, cur, p++, 1);
			continue;
		}

		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, -1/* wrong? */);
			cur->val = strtol(p, &p, 10);
			continue;
		}

		error_at(token->str, "can't tokenize");
	}

	new_token(TK_EOF, cur, p, -2 /* wrong? */);
	return head.next;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1,sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_NUM;
	node->val = val;
	return node;
}

Node *expr() {
	Node *node = equality();
	
	for(;;) {
		if (consume("+"))
			node =  new_node(ND_ADD, node, mul());
		else if (consume("-"))
			node = new_node(ND_SUB, node, mul());
		else
			return node;
	}
}

Node *equality() {
	Node *node = relational();
	return node;
}

Node *relational() {
	Node *node = add();
	return node;
}

Node *add() {
	Node *node = mul();
	for(;;) {
		if (consume("+"))
			node =  new_node(ND_ADD, node, mul());
		else if (consume("-"))
			node = new_node(ND_SUB, node, mul());
		else
			return node;
	}
}

Node *mul() {
	Node *node = unary();

	for(;;) {
		if (consume("*"))
			node = new_node(ND_MUL, node, unary());
		else if (consume("/"))
			node = new_node(ND_DIV, node, unary());
		else
			return node;
	}
}

Node *unary() {
	if (consume("+"))
		return term();
	if (consume("-"))
		return new_node(ND_SUB, new_node_num(0), term());
	return term();
}

Node *term() {
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	}

	return new_node_num(expect_number());
}

void gen(Node *node) {
	if (node->kind == ND_NUM) {
		printf("  push %d\n", node->val);
		return;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("  pop rdi\n");
	printf("  pop rax\n");

	switch (node->kind) {
	case ND_ADD:
		printf("  add rax, rdi\n");
		break;
	case ND_SUB:
		printf("  sub rax, rdi\n");
		break;
	case ND_MUL:
		printf("  imul rax, rdi\n");
		break;
	case ND_DIV:
		printf("  cqo\n");
		printf("  idiv rdi\n");
	}
	printf("  push rax\n");
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "number of argument is not true.\n");
		return 1;
	}

	// tokenize
	user_input = argv[1];
	token = tokenize(user_input);
	
	// DEBUG_BEGIN
	//debug_show(token);
	//printf("----------------------\n");
	// DEBUG_END
	
	Node *node = expr();
	// assembly front part
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	gen(node);

	printf("  pop rax\n");
	printf("  ret\n");
	return 0;
}
