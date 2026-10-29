#include <stdint.h>
#include <stdarg.h>

volatile uint64_t tohost __attribute__((section(".htif"), aligned(64))) = 0;
volatile uint64_t fromhost __attribute__((section(".htif"), aligned(64))) = 0;

void exit(int code) {
    tohost = ((uint64_t)code << 1) | 1;
    while (1);
}

void _exit(int code) {
    exit(code);
}

void *__dso_handle = 0;

extern char _end;

// Naked entry point: initialize Stack Pointer (sp) 2KB above _end in SRAM
__attribute__((section(".text.init"), naked))
void _start(void) {
    asm volatile (
        ".option push\n"
        ".option norelax\n"
        "la gp, __global_pointer$\n"
        ".option pop\n"
        "la sp, _end + 0x800\n"
        "call main\n"
        "tail exit\n"
    );
}

// Stubbed UART output (no-op to avoid unmapped memory access faults)
void uart_putchar(char c) {
    (void)c;
}

static void print_str(const char *s) {
    while (*s) {
        uart_putchar(*s++);
    }
}

static void print_hex(uint64_t val, int digits) {
    static const char hex_chars[] = "0123456789ABCDEF";
    for (int i = (digits - 1) * 4; i >= 0; i -= 4) {
        uart_putchar(hex_chars[(val >> i) & 0xF]);
    }
}

static void print_dec(uint64_t val) {
    char buf[20];
    int i = 0;
    if (val == 0) {
        uart_putchar('0');
        return;
    }
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i > 0) {
        uart_putchar(buf[--i]);
    }
}

int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++;
            int width = 0;
            int is_long = 0;

            while (*format >= '0' && *format <= '9') {
                width = width * 10 + (*format - '0');
                format++;
            }
            if (*format == 'l') {
                is_long = 1;
                format++;
            }

            switch (*format) {
                case 'd':
                case 'u': {
                    uint64_t val = is_long ? va_arg(args, unsigned long) : va_arg(args, unsigned int);
                    print_dec(val);
                    break;
                }
                case 'x':
                case 'X': {
                    uint64_t val = is_long ? va_arg(args, unsigned long) : va_arg(args, unsigned int);
                    print_hex(val, width ? width : 8);
                    break;
                }
                case 's': {
                    char *s = va_arg(args, char *);
                    print_str(s ? s : "(null)");
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    uart_putchar(c);
                    break;
                }
                case '%': {
                    uart_putchar('%');
                    break;
                }
                default:
                    uart_putchar('%');
                    uart_putchar(*format);
                    break;
            }
        } else {
            uart_putchar(*format);
        }
        format++;
    }

    va_end(args);
    return 0;
}