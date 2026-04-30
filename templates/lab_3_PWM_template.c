/*
 * ============================================================
 * IMP  –  Lab 3  (STUDENT TEMPLATE)
 * PWM via TPM0 timer  –  RGB LED breathing effect
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
 * PTD0   LED_R   TPM0_CH0   (ALT4)
 * PTD1   LED_G   TPM0_CH1   (ALT4)
 * PTD2   LED_B   TPM0_CH2   (ALT4)
 *
 * TPM0 setup
 * ----------
 * Clock source : IRC48M (48 MHz)  via SIM->SOPT2[TPMSRC = 1]
 * Prescaler    : /8               → TPM clock = 6 MHz
 * Modulo       : 255              → PWM period = 256 ticks ≈ 23.4 kHz
 * Channels 0-2 : edge-aligned PWM, high-true pulses
 * (output HIGH while counter < CnV → brightness = CnV / 255)
 * Overflow IRQ : TPM0_IRQn = IRQ 17
 *
 * Behaviour
 * ---------
 * 'brightness' counts 0 → 255 → 0 → 255 → … (smooth breathing).
 * All three colour channels track the same value → the LED breathes white.
 * The overflow ISR fires at ~23.4 kHz; it signals main() every 94 overflows
 * (~4 ms per step) → full fade in or out takes ~1 second.
 * ============================================================
 */

#include "MCXC143.h"

/* ── application state ───────────────────────────────────────────────────── */

#define PWM_MAX  255                   /* 8-bit resolution: duty 0 … 255     */

static volatile int brightness = 0;    /* current LED intensity              */
static volatile int direction  = 1;    /* +1 = brightening, −1 = dimming     */
static volatile int step_ready = 0;    /* set by ISR when a step is due      */

/* ── TPM0 overflow interrupt handler  (IRQ 17) ───────────────────────────── */

/*
 * Fires at the PWM carrier frequency (~23.4 kHz).
 * We count overflows and tell main() to advance by one brightness step
 * every 94 overflows ≈ every 4 ms.
 */
void TPM0_IRQHandler(void)
{
    static uint32_t overflow_count = 0;

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
     * TASK 1: Clear the timer overflow flag (TOF).                               *
     * Check the TPMx_SC register documentation. You must write a '1' to clear it.*
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    
    // TODO: Write code to clear the TOF flag here


    overflow_count++;
    if (overflow_count >= 94) {
        overflow_count = 0;
        step_ready = 1;                /* signal main() to take the next step */
    }
}

/* ── initialisation ──────────────────────────────────────────────────────── */

static void clock_init(void)
{
    SIM->COPC = SIM_COPC_COPT(0);
    MCG->MC |= MCG_MC_HIRCEN_MASK;
    MCG->C1  = (uint8_t)((MCG->C1 & ~MCG_C1_CLKS_MASK) | MCG_C1_CLKS(2));
    while ((MCG->S & MCG_S_CLKST_MASK) != MCG_S_CLKST(1)) { }
    SIM->CLKDIV1 = SIM_CLKDIV1_OUTDIV1(0) | SIM_CLKDIV1_OUTDIV4(1);
}

/*
 * Set up TPM0 to produce three synchronised edge-aligned PWM signals.
 */
static void tpm0_init(void)
{
    /* Enable clocks to PORTD and TPM0 */
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;    
    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;     

    /* IRC48M as TPM clock source (TPMSRC = 01) */
    SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_TPMSRC_MASK) | SIM_SOPT2_TPMSRC(1);

    /* Route TPM0 channel outputs to PTD0, PTD1, PTD2  (ALT4 = MUX 4) */
    PORTD->PCR[0] = PORT_PCR_MUX(4);       
    PORTD->PCR[1] = PORT_PCR_MUX(4);       
    PORTD->PCR[2] = PORT_PCR_MUX(4);       

    TPM0->SC  = 0;                         /* stop counter while setting up */
    TPM0->CNT = 0;                         /* reset counter                 */

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
     * TASK 2: Configure the PWM hardware registers.                              *
     * a) Set the TPM0 Modulo register (MOD) to PWM_MAX (8-bit resolution).      *
     * b) Configure channels 0, 1, and 2 for edge-aligned PWM, high-true pulses. *
     * (Set MSB=1, ELSB=1 in the CnSC register for each channel).             *
     * c) Set the initial duty cycle (CnV) to 0 for all three channels.          *
     * d) Start the counter by writing to the Status and Control (SC) register:  *
     * CMOD = 1 (internal clock), PS = 3 (prescaler /8), TOIE = 1 (interrupt).*
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    // TODO: a) Set modulo register
    

    // TODO: b & c) Configure channels 0, 1, and 2 (CnSC and CnV registers)
    

    // TODO: d) Start the counter (SC register)
    

    NVIC_ClearPendingIRQ(TPM0_IRQn);
    NVIC_EnableIRQ(TPM0_IRQn);
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    clock_init();
    tpm0_init();

    while (1) {
        if (step_ready) {
            step_ready = 0;

            brightness += direction;

            /* Smooth ping-pong fade logic */
            if (brightness >= PWM_MAX) { brightness = PWM_MAX; direction = -1; }
            if (brightness <= 0)       { brightness = 0;       direction = +1; }

            /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
             * TASK 3: Apply the new brightness.                                          *
             * Write the 'brightness' variable to the Channel Value (CnV) register        *
             * for all three TPM0 channels (0, 1, and 2) to update the duty cycle.        *
             * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

            // TODO: Write the new value to the compare registers (CnV)
            

        }
    }
}