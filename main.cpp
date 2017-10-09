#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define bool int
#define true 1
#define false 0

#define norw 13       /* 保留字个数 */
#define txmax 100     /* 符号表容量 */
#define nmax 14       /* 数字的最大位数 */
#define al 10         /* 标识符的最大长度 */
#define maxerr 30     /* 允许的最多错误数 */
#define amax 2048     /* 地址上界*/
#define levmax 3      /* 最大允许过程嵌套声明层数*/
#define cxmax 200     /* 最多的虚拟机代码数 */
#define stacksize 500 /* 运行时数据栈元素最多为500个 */

/* 符号 */
enum symbol {
    nul,         ident,     number,     plus,      minus,
    times,       slash,     eql,        neq,       lss,
    leq,         gtr,       geq,        lparen,    rparen,
    semicolon,   becomes,   ellipsis,   varsym,    funcsym,
    ifsym,       elsesym,   whilesym,   readsym,   printsym,
    callsym,     forsym,    insym,      lbrace,    rbrace,
    period
};
#define symnum 31

/* 符号表中的类型 */
enum object {
    variable,
    function,
};

/* 虚拟机代码指令 */
enum fct {
    lit,     opr,     lod,
    sto,     cal,     ini,
    jmp,     jpc,
};
#define fctnum 8

/* 虚拟机代码结构 */
struct instruction
{
    enum fct f; /* 虚拟机代码指令 */
    int l;      /* 引用层与声明层的层次差 */
    int a;      /* 根据f的不同而不同 */
};

bool listswitch ;   /* 显示虚拟机代码与否 */
bool tableswitch ;  /* 显示符号表与否 */
char ch;            /* 存放当前读取的字符，getch 使用 */
enum symbol sym;    /* 当前的符号 */
char id[al+1];      /* 当前ident，多出的一个字节用于存放0 */
int num;            /* 当前number */
int cc, ll;         /* getch使用的计数器，cc表示当前字符(ch)的位置 */
int cx;             /* 虚拟机代码指针, 取值范围[0, cxmax-1]*/
char line[80];      /* 读取行缓冲区 */
char a[al+1];       /* 临时符号，多出的一个字节用于存放0 */
struct instruction code[cxmax]; /* 存放虚拟机代码的数组 */
char word[norw][al];        /* 保留字 */
enum symbol wsym[norw];     /* 保留字对应的符号值 */
enum symbol ssym[256];      /* 单字符的符号值 */
char mnemonic[fctnum][5];   /* 虚拟机代码指令名称 */
bool declbegsys[symnum];    /* 表示声明开始的符号集合 */
bool statbegsys[symnum];    /* 表示语句开始的符号集合 */
bool facbegsys[symnum];     /* 表示因子开始的符号集合 */

/* 符号表结构 */
struct tablestruct
{
    char name[al];	    /* 名字 */
    enum object kind;	/* 类型：const，var或procedure */
    int val;            /* 数值，仅const使用 */
    int level;          /* 所处层，仅const不使用 */
    int adr;            /* 地址，仅const不使用 */
    int size;           /* 需要分配的数据区空间, 仅procedure使用 */
};

struct tablestruct table[txmax]; /* 符号表 */

FILE* fin;      /* 输入源文件 */
FILE* ftable;	/* 输出符号表 */
FILE* fcode;    /* 输出虚拟机代码 */
FILE* foutput;  /* 输出文件及出错示意（如有错）、各行对应的生成代码首地址（如无错） */
FILE* fresult;  /* 输出执行结果 */
char fname[al];
int err;        /* 错误计数器 */
int inset(int e, bool* s);
int addset(bool* sr, bool* s1, bool* s2, int n);
int subset(bool* sr, bool* s1, bool* s2, int n);
int mulset(bool* sr, bool* s1, bool* s2, int n);
void getsym();
void getch();
void init();
void listall();
void error(int n);

void factor(bool* fsys, int* ptx, int lev);
void term(bool* fsys, int* ptx, int lev);
void condition(bool* fsys, int* ptx, int lev);
void expression(bool* fsys, int* ptx, int lev);
void if_stat(bool* fsys, int* ptx, int lev);
void while_stat(bool* fsys, int* ptx, int lev);
void read_stat(bool* fsys, int* ptx, int lev);
void print_stat(bool* fsys, int* ptx, int lev);
void assign_stat(bool* fsys, int* ptx, int lev);
void for_stat(bool* fsys, int* ptx, int lev);
void call_stat(bool* fsys, int* ptx, int lev);
void program(int lev, int tx, bool* fsys);
void var_declaration_list(int* ptx, int lev, int* pdx);
void function_declaration_list(int* ptx, int lev, int* pdx);
void statement_list(bool* fsys, int* ptx, int lev);
void interpret();
int base(int l, int* s, int b);


/* 主程序开始 */
int main()
{
    bool nxtlev[symnum];
    
    printf("Input sw file?   ");
    scanf("%s", fname);		/* 输入文件名 */
    
    if ((fin = fopen(fname, "r")) == NULL)
    {
        printf("Can't open the input file!\n");
        exit(1);
    }
    
    ch = fgetc(fin);
    if (ch == EOF)
    {
        printf("The input file is empty!\n");
        fclose(fin);
        exit(1);
    }
    rewind(fin);
    
    if ((foutput = fopen("/Users/zhaohaoying/Desktop/test/foutput.txt", "w")) == NULL)
    {
        printf("Can't open the output file!\n");
        exit(1);
    }
    
    if ((ftable = fopen("/Users/zhaohaoying/Desktop/test/ftable.txt", "w")) == NULL)
    {
        printf("Can't open ftable.txt file!\n");
        exit(1);
    }
    
    printf("List object codes?(Y/N)");	/* 是否输出虚拟机代码 */
    scanf("%s", fname);
    listswitch = (fname[0]=='y' || fname[0]=='Y');
    
    printf("List symbol table?(Y/N)");	/* 是否输出符号表 */
    scanf("%s", fname);
    tableswitch = (fname[0]=='y' || fname[0]=='Y');
    
    init();		/* 初始化 */
    err = 0;
    cc = ll = cx = 0;
    ch = ' ';
    
    getsym();
    
    addset(nxtlev, declbegsys, statbegsys, symnum);
    nxtlev[period] = true;
    program(0, 0, nxtlev);	/* 处理分程序 */
    
    if (sym != period)
    {
        error(9);
    }
    
    if (err == 0)
    {
        printf("\n===Parsing success!===\n");
        fprintf(foutput,"\n===Parsing success!===\n");
        
        if ((fcode = fopen("/Users/zhaohaoying/Desktop/test/fcode.txt", "w")) == NULL)
        {
            printf("Can't open fcode.txt file!\n");
            exit(1);
        }
        
        if ((fresult = fopen("/Users/zhaohaoying/Desktop/test/fresult.txt", "w")) == NULL)
        {
            printf("Can't open fresult.txt file!\n");
            exit(1);
        }
        
        listall();	 /* 输出所有代码 */
        fclose(fcode);
        
        interpret();	/* 调用解释执行程序 */
        fclose(fresult);
    }
    else
    {
        printf("\n%d errors in sw program!\n",err);
        fprintf(foutput,"\n%d errors in sw program!\n",err);
    }
    
    fclose(ftable);
    fclose(foutput);
    fclose(fin);
    
    return 0;
}



/*
 * 初始化
 */
void init()
{
    int i;
    
    /* 设置单字符符号 */
    for (i=0; i<=255; i++)
    {
        ssym[i] = nul;
    }
    ssym['+'] = plus;
    ssym['-'] = minus;
    ssym['*'] = times;
    ssym['/'] = slash;
    ssym['('] = lparen;
    ssym[')'] = rparen;
    ssym['='] = becomes;
    ssym[';'] = semicolon;
    ssym['{'] = lbrace;
    ssym['}'] = rbrace;
    ssym['$'] = period;
    
    /* 设置保留字名字,按照字母顺序，便于二分查找 */
    strcpy(&(word[0][0]), "call");
    strcpy(&(word[1][0]), "else");
    strcpy(&(word[2][0]), "for");
    strcpy(&(word[3][0]), "func");
    strcpy(&(word[4][0]), "if");
    strcpy(&(word[5][0]), "in");
    strcpy(&(word[6][0]), "print");
    strcpy(&(word[7][0]), "read");
    strcpy(&(word[8][0]), "var");
    strcpy(&(word[9][0]), "while");
    
    /* 设置保留字符号 */
    wsym[0] = callsym;
    wsym[1] = elsesym;
    wsym[2] = forsym;
    wsym[3] = funcsym;
    wsym[4] = ifsym;
    wsym[5] = insym;
    wsym[6] = printsym;
    wsym[7] = readsym;
    wsym[8] = varsym;
    wsym[9] = whilesym;
    
    /* 设置指令名称 */
    strcpy(&(mnemonic[lit][0]), "lit");
    strcpy(&(mnemonic[opr][0]), "opr");
    strcpy(&(mnemonic[lod][0]), "lod");
    strcpy(&(mnemonic[sto][0]), "sto");
    strcpy(&(mnemonic[cal][0]), "cal");
    strcpy(&(mnemonic[ini][0]), "int");
    strcpy(&(mnemonic[jmp][0]), "jmp");
    strcpy(&(mnemonic[jpc][0]), "jpc");
    
    /* 设置符号集 */
    for (i=0; i<symnum; i++)
    {
        declbegsys[i] = false;
        statbegsys[i] = false;
        facbegsys[i] = false;
    }
    
    /* 设置声明开始符号集 */
    declbegsys[ident] = true;
    declbegsys[funcsym] = true;
    
    /* 设置语句开始符号集 */
    statbegsys[forsym] = true;
    statbegsys[callsym] = true;
    statbegsys[readsym] = true;
    statbegsys[printsym] = true;
    statbegsys[ident] = true;
    statbegsys[ifsym] = true;
    statbegsys[whilesym] = true;
    statbegsys[callsym] = true;
    
    /* 设置因子开始符号集 */
    facbegsys[ident] = true;
    facbegsys[number] = true;
    facbegsys[lparen] = true;
    
}

/*
 *	出错处理，打印出错位置和错误编码
 */
void error(int n)
{
    char space[81];
    memset(space,32,81);
    
    space[cc-1]=0; /* 出错时当前符号已经读完，所以cc-1 */
    
    printf("**%s^%d\n", space, n);
    fprintf(foutput,"**%s^%d\n", space, n);
    
    err = err + 1;
    if (err > maxerr)
    {
        exit(1);
    }
}

/*
 * 测试当前符号是否合法
 *
 * 在语法分析程序的入口和出口处调用测试函数test，
 * 检查当前单词进入和退出该语法单位的合法性
 *
 * s1:	需要的单词集合
 * s2:	如果不是需要的单词，在某一出错状态时，
 *      可恢复语法分析继续正常工作的补充单词符号集合
 * n:  	错误号
 */
void test(bool* s1, bool* s2, int n)
{
    if (!inset(sym, s1))
    {
        error(n);
        /* 当检测不通过时，不停获取符号，直到它属于需要的集合或补救的集合 */
        while ((!inset(sym,s1)) && (!inset(sym,s2)))
        {
            getsym();
        }
    }
}

/*
 * 生成虚拟机代码
 *
 * x: instruction.f;
 * y: instruction.l;
 * z: instruction.a;
 */
void gen(enum fct x, int y, int z )
{
    if (cx >= cxmax)
    {
        printf("Program is too long!\n");	/* 生成的虚拟机代码程序过长 */
        exit(1);
    }
    if ( z >= amax)
    {
        printf("Displacement address is too big!\n");	/* 地址偏移越界 */
        exit(1);
    }
    code[cx].f = x;
    code[cx].l = y;
    code[cx].a = z;
    cx++;
}

/*
 * 输出目标代码清单
 */
void listcode(int cx0)
{
    int i;
    if (listswitch)
    {
        //printf("\n");
        for (i = cx0; i < cx; i++)
        {
            //printf("%d %s %d %d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
        }
    }
}

/*
 * 输出所有目标代码
 */
void listall()
{
    int i;
    if (listswitch)
    {
        for (i = 0; i < cx; i++)
        {
            //printf("%d %s %d %d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
            fprintf(fcode,"%d %s %d %d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
        }
    }
}



/*
 * 过滤空格，读取一个字符
 * 每次读一行，存入line缓冲区，line被getsym取空后再读一行
 * 被函数getsym调用
 */
void getch()
{
    if (cc == ll) /* 判断缓冲区中是否有字符，若无字符，则读入下一行字符到缓冲区中 */
    {
        if (feof(fin))
        {
            printf("Program is incomplete!\n");
            exit(1);
        }
        ll = 0;
        cc = 0;
        //printf("%d ", cx);
        fprintf(foutput,"%d ", cx);
        ch = ' ';
        while (ch != 13)
        {
            if (EOF == fscanf(fin,"%c", &ch))
            {
                line[ll] = 0;
                break;
            }
            //printf("%c", ch);
            fprintf(foutput, "%c", ch);
            line[ll] = ch;
            ll++;
        }
    }
    ch = line[cc];
    cc++;
}

/*
 * 词法分析，获取一个符号
 */
void getsym()
{
    int i,j,k;
    
    while (ch == ' ' || ch == 13 || ch == 9)	/* 过滤空格、换行和制表符 */
    {
        getch();
    }
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) /* 当前的单词是标识符或是保留字 */
    {
        k = 0;
        do {
            if(k < al)
            {
                a[k] = ch;
                k++;
            }
            getch();
        } while ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'));
        a[k] = 0;
        strcpy(id, a);
        i = 0;
        j = norw - 1;
        do {    /* 搜索当前单词是否为保留字，使用二分法查找 */
            k = (i + j) / 2;
            if (strcmp(id,word[k]) <= 0)
            {
                j = k - 1;
            }
            if (strcmp(id,word[k]) >= 0)
            {
                i = k + 1;
            }
        } while (i <= j);
        if (i-1 > j) /* 当前的单词是保留字 */
        {
            sym = wsym[k];
        }
        else /* 当前的单词是标识符 */
        {
            sym = ident;
        }
    }
    else
    {
        if (ch >= '0' && ch <= '9') /* 当前的单词是数字 */
        {
            k = 0;
            num = 0;
            sym = number;
            do {
                num = 10 * num + ch - '0';
                k++;
                getch();
            } while (ch >= '0' && ch <= '9'); /* 获取数字的值 */
            k--;
            if (k > nmax) /* 数字位数太多 */
            {
                error(30);
            }
        }
        else
        {
            if (ch == '=')		/* 检测等于符号 */
            {
                getch();
                if (ch == '=')
                {
                    sym = eql;
                    getch();
                }
                else
                {
                    sym = becomes;	/* 检测赋值符号 */
                }
            }
            else
            {
                if (ch == '<')		/* 检测小于或小于等于符号 */
                {
                    getch();
                    if (ch == '=')
                    {
                        sym = leq;
                        getch();
                    }
                    else
                    {
                        sym = lss;
                    }
                }
                else
                {
                    if (ch == '>')		/* 检测大于或大于等于符号 */
                    {
                        getch();
                        if (ch == '=')
                        {
                            sym = geq;
                            getch();
                        }
                        else
                        {
                            sym = gtr;
                        }
                    }
                    else
                    {
                        if (ch == '!')		/* 检测不等于符号 */
                        {
                            getch();
                            if (ch == '=')
                            {
                                sym = neq;
                                getch();
                            }
                            else
                            {
                                sym = gtr;
                            }
                        }
                        else
                        {
                            if (ch == '.')		/* 检测省略号符号 */
                            {
                                getch();
                                if (ch == '.') {
                                    getch();
                                    if(ch == '.') {
                                        sym = ellipsis;
                                        getch();
                                    }
                                    else {
                                        sym = nul;
                                    }
                                }
                                else
                                {
                                    sym = nul;
                                }
                            }
                            else
                            {
                                sym = ssym[ch];		/* 当符号不满足上述条件时，全部按照单字符符号处理 */
                                if(sym == period)
                                    return;
                                else getch();
                            }
                        }
                    }
                }
            }
        }
    }
}

/*
 * 用数组实现集合的集合运算
 */
int inset(int e, bool* s)
{
    return s[e];
}

int addset(bool* sr, bool* s1, bool* s2, int n)
{
    int i;
    for (i=0; i<n; i++)
    {
        sr[i] = s1[i]||s2[i];
    }
    return 0;
}

int subset(bool* sr, bool* s1, bool* s2, int n)
{
    int i;
    for (i=0; i<n; i++)
    {
        sr[i] = s1[i]&&(!s2[i]);
    }
    return 0;
}

int mulset(bool* sr, bool* s1, bool* s2, int n)
{
    int i;
    for (i=0; i<n; i++)
    {
        sr[i] = s1[i]&&s2[i];
    }
    return 0;
}


/*
 * 在符号表中加入一项
 *
 * k:      标识符的种类为var或func
 * ptx:    符号表尾指针的指针，为了可以改变符号表尾指针的值
 * lev:    标识符所在的层次
 * pdx:    dx为当前应分配的变量的相对地址，分配后要增加1
 *
 */
void enter(enum object k, int* ptx,	int lev, int* pdx)
{
    (*ptx)++;
    strcpy(table[(*ptx)].name, id); /* 符号表的name域记录标识符的名字 */
    table[(*ptx)].kind = k;
    switch (k)
    {
        case variable:	/* 变量 */
            table[(*ptx)].level = lev;
            table[(*ptx)].adr = (*pdx);
            (*pdx)++;
            break;
        case function:	/* 过程 */
            table[(*ptx)].level = lev;
            break;
    }
}

/*
 * 查找标识符在符号表中的位置，从tx开始倒序查找标识符
 * 找到则返回在符号表中的位置，否则返回0
 *
 * id:    要查找的名字
 * tx:    当前符号表尾指针
 */
int position(char* id, int tx)
{
    int i;
    strcpy(table[0].name, id);
    i = tx;
    while (strcmp(table[i].name, id) != 0)
    {
        i--;
    }
    return i;
}

/*
 * 变量声明处理
 */
void vardeclaration(int* ptx,int lev,int* pdx)
{
    if (sym == ident)
    {
        enter(variable, ptx, lev, pdx);	// 填写符号表
        getsym();
    }
    else
    {
        error(4);	/* var后面应是标识符 */
    }
}


/*
 * 项处理
 */
void term(bool* fsys, int* ptx, int lev)
{
    enum symbol mulop;	/* 用于保存乘除法符号 */
    bool nxtlev[symnum];
    
    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
    nxtlev[times] = true;
    nxtlev[slash] = true;
    factor(nxtlev, ptx, lev);	/* 处理因子 */
    while(sym == times || sym == slash)
    {
        mulop = sym;
        getsym();
        factor(nxtlev, ptx, lev);
        if(mulop == times)
        {
            gen(opr, 0, 4);	/* 生成乘法指令 */
        }
        else
        {
            gen(opr, 0, 5);	/* 生成除法指令 */
        }
    }
}

/*
 * 因子处理
 */
void factor(bool* fsys, int* ptx, int lev)
{
    int i;
    bool nxtlev[symnum];
    test(facbegsys, fsys, 24);	/* 检测因子的开始符号 */
    if(sym == ident)	/* 因子变量 */
    {
        i = position(id, *ptx);	/* 查找标识符在符号表中的位置 */
        if (i == 0)
        {
            error(11);	/* 标识符未声明 */
        }
        else
        {
            switch (table[i].kind)
            {
                case variable:	/* 标识符为变量 */
                    gen(lod, lev-table[i].level, table[i].adr);	/* 找到变量地址并将其值入栈 */
                    break;
                case function:	/* 标识符为过程 */
                    error(21);	/* 不能为过程 */
                    break;
            }
        }
        getsym();
    }
    else
    {
        if(sym == number)	/* 因子为数 */
        {
            if (num > amax)
            {
                error(31); /* 数越界 */
                num = 0;
            }
            gen(lit, 0, num);
            getsym();
        }
        else
        {
            if (sym == lparen)	/* 因子为表达式 */
            {
                getsym();
                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                nxtlev[rparen] = true;
                expression(nxtlev, ptx, lev);
                if (sym == rparen)
                {
                    getsym();
                }
                else
                {
                    error(22);	/* 缺少右括号 */
                }
            }
        }
    }
}

/*
 * 表达式处理
 */
void expression(bool* fsys, int* ptx, int lev)
{
    enum symbol addop;	/* 用于保存正负号 */
    bool nxtlev[symnum];
    
    if(sym == plus || sym == minus)	/* 表达式开头有正负号，此时当前表达式被看作一个正的或负的项 */
    {
        addop = sym;	/* 保存开头的正负号 */
        getsym();
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[plus] = true;
        nxtlev[minus] = true;
        term(nxtlev, ptx, lev);	/* 处理项 */
        if (addop == minus)
        {
            gen(opr,0,1);	/* 如果开头为负号生成取负指令 */
        }
    }
    else	/* 此时表达式被看作项的加减 */
    {
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[plus] = true;
        nxtlev[minus] = true;
        term(nxtlev, ptx, lev);	/* 处理项 */
    }
    while (sym == plus || sym == minus)
    {
        addop = sym;
        getsym();
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[plus] = true;
        nxtlev[minus] = true;
        term(nxtlev, ptx, lev);	/* 处理项 */
        if (addop == plus)
        {
            gen(opr, 0, 2);	/* 生成加法指令 */
        }
        else
        {
            gen(opr, 0, 3);	/* 生成减法指令 */
        }
    }
}

/*
 * 条件处理
 */
void condition(bool* fsys, int* ptx, int lev)
{
    enum symbol relop;
    bool nxtlev[symnum];
    /* 逻辑表达式处理 */
    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
    nxtlev[eql] = true;
    nxtlev[neq] = true;
    nxtlev[lss] = true;
    nxtlev[leq] = true;
    nxtlev[gtr] = true;
    nxtlev[geq] = true;
    expression(nxtlev, ptx, lev);
    if (sym != eql && sym != neq && sym != lss && sym != leq && sym != gtr && sym != geq)
    {
        error(20); /* 应该为关系运算符 */
    }
    else
    {
        relop = sym;
        getsym();
        expression(fsys, ptx, lev);
        switch (relop)
        {
            case eql:
                gen(opr, 0, 8);
                break;
            case neq:
                gen(opr, 0, 9);
                break;
            case lss:
                gen(opr, 0, 10);
                break;
            case geq:
                gen(opr, 0, 11);
                break;
            case gtr:
                gen(opr, 0, 12);
                break;
            case leq:
                gen(opr, 0, 13);
                break;
            default:
                break;
            }
        }
}

void if_stat(bool* fsys, int* ptx, int lev) {
    bool nxtlev[symnum];
    int cx1;
    int cx2;
    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
    nxtlev[elsesym] = true;
    nxtlev[lbrace] = true;	/* 后继符号为else或{ */
    condition(nxtlev, ptx, lev); /* 调用条件处理 */
    if (sym != lbrace) {
        error(36);  /* 缺少{ */
    }
    getsym();
    cx1 = cx;	/* 保存当前指令地址 */
    gen(jpc, 0, 0);	/* 生成条件跳转指令，跳转地址未知，暂时写0 */
    statement_list(nxtlev, ptx, lev); /* 调用语句列表处理 */
    if(sym != rbrace) {
        error(37); /* 缺少} */
    }
    else getsym();
    if(sym == elsesym) {    //有else语句
        getsym();
        cx2 = cx;   /* 保存当前指令地址 */
        gen(jmp,0,0); /* 生成无条件跳转指令，跳转地址未知，暂时写0 */
        code[cx1].a = cx; /* 经statement_list处理和添加无条件跳转语句后，cx为condition后statement语句执行完的位置,也是else语句开始的位置,它正是前面未定的跳转地址，此时进行回填 */
        if(sym != lbrace) {
            error(36);  /* 缺少{ */
        }
        getsym();
        statement_list(nxtlev, ptx, lev); /* 调用声明表处理 */
        code[cx2].a = cx; /* 经statement_list处理后，cx为else语句执行完的位置，它正是前面未定的跳转地址，此时进行回填 */
        if(sym == rbrace) {
            getsym();
        }
        else {
            error(37); /* 缺少} */
        }
        
    }
    else {        //没有else语句
        code[cx1].a = cx; /* 经statement_list处理后，cx为condition后statement语句执行完的位置，它正是前面未定的跳转地址，此时进行回填 */

    }
}

void while_stat(bool* fsys, int* ptx, int lev) {
    int cx1, cx2;
    bool nxtlev[symnum];
    cx1 = cx;	/* 保存判断条件操作的位置 */
    memcpy(nxtlev, fsys, sizeof(bool) * symnum);
    nxtlev[lbrace] = true;	/* 后继符号为{ */
    condition(nxtlev, ptx, lev);	/* 调用条件处理 */
    cx2 = cx;	/* 保存循环体的结束的下一个位置 */
    gen(jpc, 0, 0);	/* 生成条件跳转，但跳出循环的地址未知，标记为0等待回填 */
    if (sym == lbrace)
    {
        getsym();
    }
    else
    {
        error(36);	/* 缺少{ */
    }
    nxtlev[rbrace] = true;
    statement_list(fsys, ptx, lev);	/* 循环体 */
    if (sym == rbrace)
    {
        getsym();
    }
    else
    {
        error(37);	/* 缺少} */
    }
    gen(jmp, 0, cx1);	/* 生成条件跳转指令，跳转到前面判断条件操作的位置 */
    code[cx2].a = cx;	/* 回填跳出循环的地址 */
}

void read_stat(bool* fsys, int* ptx, int lev) {
    int i;
    if (sym != lparen)
    {
        error(34);	/* 格式错误，应是左括号 */
    }
    else
    {
        getsym();
        if (sym == ident)
        {
            i = position(id, *ptx);	/* 查找要读的变量 */
        }
        else
        {
            i = 0;
        }
        
        if (i == 0)
        {
            error(35);	/* read语句括号中的标识符应该是声明过的变量 */
        }
        else
        {
            gen(opr, 0, 16);	/* 生成输入指令，读取值到栈顶 */
            gen(sto, lev-table[i].level, table[i].adr);	/* 将栈顶内容送入变量单元中 */
        }
        getsym();
    }
    if(sym != rparen)
    {
        error(33);	/* 格式错误，应是右括号 */
        while (!inset(sym, fsys))	/* 出错补救，直到遇到上层函数的后继符号 */
        {
            getsym();
        }
    }
    else
    {
        getsym();
    }

}

void print_stat(bool* fsys, int* ptx, int lev) {
    bool nxtlev[symnum];
    if (sym == lparen)
    {
        getsym();
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[rparen] = true;
        expression(nxtlev, ptx, lev);	/* 调用表达式处理 */
        gen(opr, 0, 14);	/* 生成输出指令，输出栈顶的值 */
        gen(opr, 0, 15);	/* 生成换行指令 */
        if (sym != rparen)
        {
            error(33);	/* 格式错误，应是右括号 */
        }
        else
        {
            getsym();
        }
    }

}

void assign_stat(bool* fsys, int* ptx, int lev) {
    int i = position(id, *ptx);/* 查找标识符在符号表中的位置 */
    bool nxtlev[symnum];
    if(sym != becomes)
        error(13); /*缺少赋值符 */
    else getsym();
    if(i == 0) {
        error(11);	/* 标识符未声明 */
    }
    else if(table[i].kind != variable)
    {
        error(12);	/* 赋值语句中，赋值号左部标识符应该是变量 */
        i = 0;
    }
    else
    {
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        expression(nxtlev, ptx, lev);	/* 处理赋值符号右侧表达式 */
        if(i != 0)
        {
            /* expression将执行一系列指令，但最终结果将会保存在栈顶，执行sto命令完成赋值 */
            gen(sto, lev-table[i].level, table[i].adr);
        }
    }
}

void for_stat(bool* fsys, int* ptx, int lev) {
    int i;
    int cx1, cx2;
    bool nxtlev[symnum];
    if (sym == ident)
    {
        i = position(id, *ptx);
        if (i == 0) error(11);
        else
        {
            if (table[i].kind != variable)
            {
                error(12);
                i = 0;
            }
            else
            {
                getsym();
                if (sym != insym) error(38);	/*缺少 in */
                else getsym();
                
                expression(fsys, ptx, lev);
                gen(sto, lev-table[i].level, table[i].adr);
                
                if (sym != ellipsis) error(39);	/*缺少 ... */
                else getsym();
                
                cx1 = cx;
                gen(lod, lev-table[i].level, table[i].adr);
                expression(fsys, ptx, lev);
                
                gen(opr, 0, 13); /*for条件判断 */
                cx2 = cx; /*条件不满足时跳出点 */
                gen(jpc, 0, 0);
                
                if (sym == lbrace)
                    getsym();
                else error(36); /*缺少{ */
                
                memcpy(nxtlev, fsys, sizeof nxtlev);
                nxtlev[rbrace] = true;
                statement_list(nxtlev, ptx, lev);
                
                if (sym == rbrace)
                    getsym();
                else error(37); /*缺少} */
                
                gen(lod, lev-table[i].level, table[i].adr); /* 将循环变量放入栈顶 */
                gen(lit, 0, 1);	/* 循环变量步长 */
                gen(opr, 0, 2);	/* 循环变量加步长 */
                gen(sto, lev-table[i].level, table[i].adr); /* 保存循环变量 */
                gen(jmp, 0, cx1); /* 跳转到条件判断处 */
                code[cx2].a = cx; /* 回填不满足判断条件的出口 */
            }
        }
    }
}

void call_stat(bool* fsys, int* ptx, int lev) {
    int i;
    if (sym != ident)
    {
        error(14);	/* call后应为标识符 */
    }
    else
    {
        i = position(id, *ptx);
        if (i == 0)
        {
            error(11);	/* 过程名未找到 */
        }
        else
        {
            if (table[i].kind == function)
            {
                gen(cal, lev-table[i].level, table[i].adr);	/* 生成call指令 */
            }
            else
            {
                error(15);	/* call后标识符类型应为过程 */
            }
        }
        getsym();
    }
    if (sym != lparen)
    {
        error(23);	/* 缺少( */
    }
    else
        getsym();
    if (sym != rparen)
    {
        error(22);	/* 缺少) */
    }
    else
        getsym();
}

void program(int lev, int tx, bool* fsys) {
    int i;
    
    int dx;                 /* 记录数据分配的相对地址 */
    int tx0;                /* 保留初始tx */
    int cx0;                /* 保留初始cx */
    bool nxtlev[symnum];    /* 在下级函数的参数中，符号集合均为值参，但由于使用数组实现，
                             传递进来的是指针，为防止下级函数改变上级函数的集合，开辟新的空间
                             传递给下级函数*/
    dx = 3;                 /* 三个空间用于存放静态链SL、动态链DL和返回地址RA  */
    tx0 = tx;		        /* 记录本层标识符的初始位置 */
    table[tx].adr = cx;	    /* 记录当前层代码的开始位置 */
    gen(jmp, 0, 0);         /* 产生跳转指令，跳转位置未知暂时填0 */
    
    while(sym == varsym) {
        getsym();
        if (sym == ident)
        {
            enter(variable, &tx, lev, &dx);	// 填写符号表
            getsym();
        }
        else
        {
            error(4);	/* var后面应是标识符 */
        }
        if(sym != semicolon) {
            error(5); /* 漏掉了分号 */
        }
        getsym();
    }
    while(sym == funcsym) {
        getsym();
        if(sym == ident) {
            enter(function, &tx, lev, &dx);	/* 填写符号表 */
            getsym();
        }
        else
        {
            error(4);	/* func后应为标识符 */
        }
        int dx;                 /* 记录数据分配的相对地址 */
        int tx0;                /* 保留初始tx */
        int cx0;                /* 保留初始cx */
        bool nxtlev[symnum];    /* 在下级函数的参数中，符号集合均为值参，但由于使用数组实现，
                                 传递进来的是指针，为防止下级函数改变上级函数的集合，开辟新的空间
                                 传递给下级函数*/
        dx = 3;                 /* 三个空间用于存放静态链SL、动态链DL和返回地址RA  */
        tx0 = tx;		        /* 记录本层标识符的初始位置 */
        table[tx].adr = cx;	    /* 记录当前层代码的开始位置 */
        if(sym != lparen)
            error(23);   /*漏掉了( */
        getsym();
        if(sym != rparen)
            error(22);   /*漏掉了) */
        getsym();
        if(sym != lbrace)
            error(36);   /*漏掉了｛ */
        getsym();
        while(sym == varsym) {
            getsym();
            if (sym == ident)
            {
                enter(variable, &tx, lev+1, &dx);	// 填写符号表
                getsym();
            }
            else
            {
                error(4);	/* var后面应是标识符 */
            }
            if(sym != semicolon) {
                error(5); /* 漏掉了分号 */
            }
            getsym();
        }
        
        table[tx0].adr = cx;	        /* 记录当前过程代码地址 */
        table[tx0].size = dx;	        /* 声明部分中每增加一条声明都会给dx增加1，声明部分已经结束，dx就是当前过程数据的size */
        cx0 = cx;
        gen(ini, 0, dx);	            /* 生成指令，此指令执行时在数据栈中为被调用的过程开辟dx个单元的数据区 */
        
        memcpy(nxtlev, fsys, sizeof(bool) * symnum);
        nxtlev[rbrace] = true;
        statement_list(fsys, &tx, lev+1);
        if(sym != rbrace)
            error(37);   /*漏掉了} */
        getsym();
        gen(opr, 0, 0);	                    /* 每个过程出口都要使用的释放数据段指令 */
    }
    code[table[tx0].adr].a = cx;	/* 把前面生成的跳转语句的跳转位置改成当前位置 */
    table[tx0].adr = cx;	        /* 记录当前过程代码地址 */
    table[tx0].size = dx;	        /* 声明部分中每增加一条声明都会给dx增加1，声明部分已经结束，dx就是当前过程数据的size */
    cx0 = cx;
    gen(ini, 0, dx);	            /* 生成指令，此指令执行时在数据栈中为被调用的过程开辟dx个单元的数据区 */
    
    if (tableswitch)		/* 输出符号表 */
    {
        for (i = 1; i <= tx; i++)
        {
            switch (table[i].kind)
            {
                case variable:
                    //printf("    %d var   %s ", i, table[i].name);
                    //printf("lev=%d addr=%d\n", table[i].level, table[i].adr);
                    fprintf(ftable, "    %d var   %s ", i, table[i].name);
                    fprintf(ftable, "lev=%d addr=%d\n", table[i].level, table[i].adr);
                    break;
                case function:
                    //printf("    %d proc  %s ", i, table[i].name);
                    //printf("lev=%d addr=%d size=%d\n", table[i].level, table[i].adr, table[i].size);
                    fprintf(ftable,"    %d proc  %s ", i, table[i].name);
                    fprintf(ftable,"lev=%d addr=%d size=%d\n", table[i].level, table[i].adr, table[i].size);
                    break;
            }
        }
        //printf("\n");
        fprintf(ftable,"\n");
    }
    
    /* 语句后继符号为分号*/
    memcpy(nxtlev, fsys, sizeof(bool) * symnum);	/* 每个后继符号集合都包含上层后继符号集合，以便补救 */
    nxtlev[semicolon] = true;
    statement_list(nxtlev, &tx, lev);
    gen(opr, 0, 0);	                    /* 每个过程出口都要使用的释放数据段指令 */
    memset(nxtlev, 0, sizeof(bool) * symnum);	/* 分程序没有补救集合 */
    test(fsys, nxtlev, 8);            	/* 检测后继符号正确性 */
    listcode(cx0);                      /* 输出本分程序生成的代码 */
}

void statement_list(bool* fsys, int* ptx, int lev) {
    bool nxtlev[symnum];
    while(inset(sym, statbegsys)) {
        switch (sym) {
            case ifsym:
                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                nxtlev[lbrace] = true;
                nxtlev[rbrace] = true;
                nxtlev[elsesym] = true;
                getsym();
                if_stat(nxtlev, ptx, lev);
                break;
            case whilesym:
                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                nxtlev[lbrace] = true;
                nxtlev[rbrace] = true;
                getsym();
                while_stat(nxtlev, ptx, lev);
                break;
            case readsym:
                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                nxtlev[lparen] = true;
                nxtlev[rparen] = true;
                getsym();
                read_stat(nxtlev, ptx, lev);
                break;
            case ident:
                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                nxtlev[becomes] = true;
                getsym();
                assign_stat(nxtlev, ptx, lev);
                break;
            case printsym:
                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                nxtlev[lparen] = true;
                nxtlev[rparen] = true;
                getsym();
                print_stat(nxtlev, ptx, lev);
                break;
            case forsym:
                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                nxtlev[insym] = true;
                nxtlev[ellipsis] = true;
                nxtlev[lbrace] = true;
                nxtlev[rbrace] = true;
                getsym();
                for_stat(nxtlev, ptx, lev);
                break;
            case callsym:
                memcpy(nxtlev, fsys, sizeof(bool) * symnum);
                nxtlev[lparen] = true;
                nxtlev[rparen] = true;
                getsym();
                call_stat(nxtlev, ptx, lev);
            default:
                break;
        }
        if(sym != semicolon)
            error(5); /*缺少; */
        getsym();
    }
}

/*
 * 解释程序
 */
void interpret()
{
    int p = 0; /* 指令指针 */
    int b = 1; /* 指令基址 */
    int t = 0; /* 栈顶指针 */
    struct instruction i;	/* 存放当前指令 */
    int s[stacksize];	/* 栈 */
    
    //printf("Start sw\n");
    fprintf(fresult,"Start sw\n");
    s[0] = 0; /* s[0]不用 */
    s[1] = 0; /* 主程序的三个联系单元均置为0 */
    s[2] = 0;
    s[3] = 0;
    do {
        i = code[p];	/* 读当前指令 */
        p = p + 1;
        switch (i.f)
        {
            case lit:	/* 将常量a的值取到栈顶 */
                t = t + 1;
                s[t] = i.a;
                break;
            case opr:	/* 数学、逻辑运算 */
                switch (i.a)
            {
                case 0:  /* 函数调用结束后返回 */
                    t = b - 1;
                    p = s[t + 3];
                    b = s[t + 2];
                    break;
                case 1: /* 栈顶元素取反 */
                    s[t] = - s[t];
                    break;
                case 2: /* 次栈顶项加上栈顶项，退两个栈元素，相加值进栈 */
                    t = t - 1;
                    s[t] = s[t] + s[t + 1];
                    break;
                case 3:/* 次栈顶项减去栈顶项 */
                    t = t - 1;
                    s[t] = s[t] - s[t + 1];
                    break;
                case 4:/* 次栈顶项乘以栈顶项 */
                    t = t - 1;
                    s[t] = s[t] * s[t + 1];
                    break;
                case 5:/* 次栈顶项除以栈顶项 */
                    t = t - 1;
                    s[t] = s[t] / s[t + 1];
                    break;
                case 6:/* 栈顶元素的奇偶判断 */
                    s[t] = s[t] % 2;
                    break;
                case 8:/* 次栈顶项与栈顶项是否相等 */
                    t = t - 1;
                    s[t] = (s[t] == s[t + 1]);
                    break;
                case 9:/* 次栈顶项与栈顶项是否不等 */
                    t = t - 1;
                    s[t] = (s[t] != s[t + 1]);
                    break;
                case 10:/* 次栈顶项是否小于栈顶项 */
                    t = t - 1;
                    s[t] = (s[t] < s[t + 1]);
                    break;
                case 11:/* 次栈顶项是否大于等于栈顶项 */
                    t = t - 1;
                    s[t] = (s[t] >= s[t + 1]);
                    break;
                case 12:/* 次栈顶项是否大于栈顶项 */
                    t = t - 1;
                    s[t] = (s[t] > s[t + 1]);
                    break;
                case 13: /* 次栈顶项是否小于等于栈顶项 */
                    t = t - 1;
                    s[t] = (s[t] <= s[t + 1]);
                    break;
                case 14:/* 栈顶值输出 */
                    //printf("%d", s[t]);
                    fprintf(fresult, "%d", s[t]);
                    t = t - 1;
                    break;
                case 15:/* 输出换行符 */
                    //printf("\n");
                    fprintf(fresult,"\n");
                    break;
                case 16:/* 读入一个输入置于栈顶 */
                    t = t + 1;
                    printf("?");
                    fprintf(fresult, "?");
                    scanf("%d", &(s[t]));
                    fprintf(fresult, "%d\n", s[t]);
                    break;
            }
                break;
            case lod:	/* 取相对当前过程的数据基地址为a的内存的值到栈顶 */
                t = t + 1;
                s[t] = s[base(i.l,s,b) + i.a];
                break;
            case sto:	/* 栈顶的值存到相对当前过程的数据基地址为a的内存 */
                s[base(i.l, s, b) + i.a] = s[t];
                t = t - 1;
                break;
            case cal:	/* 调用子过程 */
                s[t + 1] = base(i.l, s, b);	/* 将父过程基地址入栈，即建立静态链 */
                s[t + 2] = b;	/* 将本过程基地址入栈，即建立动态链 */
                s[t + 3] = p;	/* 将当前指令指针入栈，即保存返回地址 */
                b = t + 1;	/* 改变基地址指针值为新过程的基地址 */
                p = i.a;	/* 跳转 */
                break;
            case ini:	/* 在数据栈中为被调用的过程开辟a个单元的数据区 */
                t = t + i.a;
                break;
            case jmp:	/* 直接跳转 */
                p = i.a;
                break;
            case jpc:	/* 条件跳转 */
                if (s[t] == 0)
                    p = i.a;
                t = t - 1;
                break;
        }
    } while (p != 0);
    fprintf(fresult,"End sw\n");
}

/* 通过过程基址求上l层过程的基址 */
int base(int l, int* s, int b)
{
    int b1;
    b1 = b;
    while (l > 0)
    {
        b1 = s[b1];
        l--;
    }
    return b1;
}

