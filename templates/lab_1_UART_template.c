/*
 * ============================================================
 * IMP  –  Lab 1  (STUDENT TEMPLATE)
 * UART communication, event detection by polling
 * ============================================================
 *
 * Student/author ID:
 * name and surname :
 * login at FIT VUT :
 *
 * ============================================================
 * Target : MCXC143VFT
 *
 * Pins used
 * ---------
 * PTE16   UART2 TX   (ALT3)
 * PTE17   UART2 RX   (ALT3)
 * PTB0    buzzer     (GPIO output, active HIGH → BC847C → piezo)
 *
 * Clock
 * -----
 * 48 MHz internal IRC (HIRC).
 * Bus clock = 24 MHz.
 * UART2 SBR = 24 000 000 / (16 × 115 200) = 13  →  115 384 Bd  (0.16 % err)
 * ============================================================
 */

#include "MCXC143.h"

/* ── Application data ────────────────────────────────────────────────────── */

#define LOGIN_LEN 8

static char correct_login[LOGIN_LEN + 1] = "xlogin00";   /* ← your FIT login */
static char entered_login[LOGIN_LEN + 1];
static char prompt[7] = "Login>";                       

/* ── Small reusable helpers ──────────────────────────────────────────────── */

/* Spin for 'ticks' iterations – used to pace the buzzer square wave. */
static void delay(int ticks)
{
    volatile int i;
    for (i = 0; i < ticks; i++) { }
}

/* Compare two strings of exactly n characters; return 1 if equal. */
static int str_equal(const char *strA, const char *strB, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (strA[i] != strB[i]) return 0;
    }
    return 1;
}

/* ── UART2 I/O ───────────────────────────────────────────────────────────── */

/* Block until the transmit buffer is empty, then send one character. */
static void uart_send_char(char ch)
{
    while (!(UART2->S1 & UART_S1_TDRE_MASK)) { }
    UART2->D = (uint8_t)ch;
}

static char uart_recv_char(void)
{
    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
     * TASK 2: Receive a character.                                               *
     * a) Block (busy-wait) until the Receive Data Register Full (RDRF)           *
     * flag in the UART2 Status Register 1 (S1) is set.                           *
     * b) Read and return the character from the UART2 Data Register (D).         *
     * Note: Reading D automatically clears the RDRF flag.                        *
     * Hint: you can figure out the behaviour based on the uart_send_char()       *
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
     
    // TODO: a) Wait for RDRF flag
    

    // TODO: b) Read and return from data register
    return 0; // by replacing this return statement
}

/* Send every character of a null-terminated string. */
static void uart_send_str(const char *str)
{
    while (*str) { uart_send_char(*str++); }
}

/* Move the terminal cursor to the next line. */
static void uart_newline(void)
{
    uart_send_char('\r');
    uart_send_char('\n');
}

/* ── Buzzer ──────────────────────────────────────────────────────────────── */

/* 500 cycles of a square wave on PTB0 → audible ~1 kHz tone via piezo. */
static void beep(void)
{
    int i;
    for (i = 0; i < 500; i++) {
        GPIOB->PSOR = (1u << 0);    /* PTB0 high */
        delay(500);
        GPIOB->PCOR = (1u << 0);    /* PTB0 low  */
        delay(500);
    }
}

/* ── Initialisation ──────────────────────────────────────────────────────── */

/*
 * Switch the MCU clock to the 48 MHz IRC.
 *
 * After reset the FLL runs at ~21 MHz.  Steps to reach 48 MHz IRC:
 * 1. Enable the 48 MHz high-speed IRC (HIRC) via MCG->MC.
 * 2. Select it by writing CLKS = 2 into MCG->C1.
 * 3. Wait for MCG->S[CLKST] = 1 to confirm the switch completed.
 * 4. Set OUTDIV1 = /1 (core 48 MHz) and OUTDIV4 = /2 (bus 24 MHz).
 */
static void clock_init(void)
{
    SIM->COPC = SIM_COPC_COPT(0);          

    MCG->MC |= MCG_MC_HIRCEN_MASK;         

    MCG->C1 = (uint8_t)(MCG->C1 & ~MCG_C1_CLKS_MASK) | MCG_C1_CLKS(0);
    while ((MCG->S & MCG_S_CLKST_MASK) != MCG_S_CLKST(0)) { }

    SIM->CLKDIV1 = SIM_CLKDIV1_OUTDIV1(0) | SIM_CLKDIV1_OUTDIV4(1);
}

/* Configure PTE16/17 for UART2 and PTB0 as the buzzer GPIO. */
static void pins_init(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;

    PORTE->PCR[16] = PORT_PCR_MUX(3);      /* PTE16 = UART2_TX  (ALT3) */
    PORTE->PCR[17] = PORT_PCR_MUX(3);      /* PTE17 = UART2_RX  (ALT3) */

    PORTB->PCR[0]  = PORT_PCR_MUX(1);      /* PTB0  = GPIO      (ALT1) */
    GPIOB->PDDR   |= (1u << 0);            
    GPIOB->PCOR    = (1u << 0);            
}

/*
 * UART2: 115 200 Bd, 8 data bits, no parity, 1 stop bit.
 */
static void uart2_init(void)
{
    SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;   

    UART2->C2 &= (uint8_t)~(UART_C2_TE_MASK | UART_C2_RE_MASK);

    UART2->BDH = 0x00;
    UART2->BDL = 0x0D;                     /* SBR = 13                  */
    UART2->C1  = 0x00;                     /* 8 data bits, no parity    */
    UART2->C3  = 0x00;                     

    (void)UART2->S1;
    UART2->D = 0x00;

    UART2->C2 |= (uint8_t)(UART_C2_TE_MASK | UART_C2_RE_MASK);
}

/* ── Main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    int i;

    clock_init();
    pins_init();
    uart2_init();

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
     * TASK 3: Complete the main login loop described below.                        *
     *                                                                            *
     * The program must repeat the following sequence until the user enters the   *
     * correct login string stored in correct_login[]:                            *
     *   1. Print the "Login>" prompt message (prompt) over UART.                 *
     *   2. Read exactly LOGIN_LEN characters one by one; echo each character     *
     *      back to the terminal immediately after receiving it.                  *
     *   3. Compare the entered string with correct_login using str_equal().      *
     *      - If they do NOT match: transmit "INCORRECT" followed by a new line   *
     *        and repeat from step 1.                                             *
     *      - If they match: exit the loop.                                       *
     * After a successful login, call beep() to confirm with an audible signal.   *
     * Embedded applications must never return from main(); end with while(1){}.  *
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    do {
        uart_send_str(prompt);

        // TODO: Create for loop that will store received chars into entered_login[]

        uart_newline();

        // TODO: Compare entered and correct strings and print INCORRECT if mismatched

    } while (!str_equal(entered_login, correct_login, LOGIN_LEN));

    beep();

    while (1) { }    
}
