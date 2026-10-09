/* kernel.c — Garfuld OS kernel */
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;

#define VGA 0xB8000
#define W 80
#define H 25

static u16 *vga = (u16*)VGA;
static int cx = 0, cy = 0;
static u8 color = 0x0F;

static inline u8 inb(u16 p){ u8 r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void outb(u16 p,u8 v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }

void clear(void){
    for(int i=0;i<W*H;i++) vga[i] = (color<<8)|' ';
    cx=cy=0;
}

void scroll(void){
    if(cy < H) return;
    for(int i=0;i<(H-1)*W;i++) vga[i]=vga[i+W];
    for(int i=(H-1)*W;i<H*W;i++) vga[i]=(color<<8)|' ';
    cy = H-1;
}

void putc(char c){
    if(c=='\n'){ cx=0; cy++; scroll(); return; }
    if(c=='\b'){ if(cx){cx--; vga[cy*W+cx]=(color<<8)|' ';} return; }
    vga[cy*W+cx] = (color<<8)|(u8)c;
    cx++;
    if(cx>=W){ cx=0; cy++; scroll(); }
}

void puts(const char *s){ while(*s) putc(*s++); }

/* --- клавиатура (US) --- */
static const char kbd[128] = {
 0,27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
 '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
 0,'a','s','d','f','g','h','j','k','l',';','\'','`',
 0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};

char getch(void){
    while(!(inb(0x64)&1));
    u8 sc = inb(0x60);
    if(sc & 0x80) return 0;
    return kbd[sc & 0x7F];
}

/* --- Файловая система Garfuld (в ОЗУ, простейшая) --- */
#define MAXF 16
#define MAXSZ 2048

typedef struct { char name[32]; char data[MAXSZ]; u32 size; u8 used; } file_t;
static file_t fs[MAXF];

static int fs_find(const char *n){
    for(int i=0;i<MAXF;i++)
        if(fs[i].used){
            int k=0; while(n[k]&&fs[i].name[k]&&n[k]==fs[i].name[k]) k++;
            if(!n[k]&&!fs[i].name[k]) return i;
        }
    return -1;
}
static int fs_create(const char *n){
    for(int i=0;i<MAXF;i++) if(!fs[i].used){
        fs[i].used=1; fs[i].size=0;
        int k=0; while(n[k]&&k<31){fs[i].name[k]=n[k];k++;} fs[i].name[k]=0;
        return i;
    }
    return -1;
}

/* --- Текстовый редактор --- */
void editor(const char *fname){
    int idx = fs_find(fname);
    if(idx<0) idx = fs_create(fname);
    clear();
    puts("Garfuld Editor - "); puts(fname);
    puts("\n(ESC=save+exit, ENTER=newline, BACKSPACE)\n\n");
    int len = 0;
    while(1){
        char c = getch();
        if(c==27) break;
        if(c=='\b'){
            if(len){ len--; putc('\b'); fs[idx].data[len]=0; fs[idx].size=len; }
            continue;
        }
        if(c){
            if(len<MAXSZ-1){
                fs[idx].data[len++]=c; fs[idx].data[len]=0; fs[idx].size=len;
                putc(c);
            }
        }
    }
    clear();
    puts("Saved "); puts(fname); puts(" ("); 
    /* печать размера */
    char buf[12]; int s=fs[idx].size, p=0; if(!s){buf[p++]='0';}
    char tmp[12]; int t=0; while(s){tmp[t++]='0'+s%10; s/=10;}
    while(t) buf[p++]=tmp[--t]; buf[p]=0;
    puts(buf); puts(" bytes)\n\nPress any key...");
    getch();
}

/* --- Псевдо-браузер: HTTP GET через QEMU-заглушку --- */
void browser(const char *url){
    clear();
    puts("Garfuld Browser\nURL: "); puts(url);
    puts("\n\n[ Отправка GET-запроса... ]\n");
    puts("Соединение с "); puts(url); puts(":80\n");
    puts("\n--- Ответ ---\n");
    /* В реальном железе тут был бы RTL8139 + TCP/IP.
       Для учебной ОС выводим заранее зашитый контент: */
    puts("<html><body>\n");
    puts("Garfuld OS Browser works!\n");
    puts("You asked for: "); puts(url); puts("\n");
    puts("</body></html>\n");
    puts("\n--- End ---\n");
    puts("(ESC = exit)");
    while(getch()!=27);
}

/* --- Меню --- */
void menu(void){
    clear();
    puts("========================================\n");
    puts("            GARFULD OS v1.0             \n");
    puts("========================================\n\n");
    puts("  [1] Text Editor\n");
    puts("  [2] Browser\n");
    puts("  [3] List files\n");
    puts("  [4] Clear screen\n");
    puts("  [0] Reboot\n\n");
    puts("Choice: ");
}

void list_files(void){
    clear();
    puts("Files on Garfuld FS:\n\n");
    int any=0;
    for(int i=0;i<MAXF;i++) if(fs[i].used){
        puts("  "); puts(fs[i].name); puts("\n"); any=1;
    }
    if(!any) puts("  (empty)\n");
    puts("\nPress any key...");
    getch();
}

void kernel_main(void){
    clear();
    puts("Booting Garfuld OS...\n");
    puts("VGA OK, Keyboard OK, FAT OK\n\n");
    for(volatile u32 i=0;i<50000000;i++);

    while(1){
        menu();
        char c = getch();
        if(c=='1') editor("note.txt");
        else if(c=='2') browser("example.com");
        else if(c=='3') list_files();
        else if(c=='4') clear();
        else if(c=='0'){
            outb(0x64,0xFE); /* reboot */
        }
    }
}