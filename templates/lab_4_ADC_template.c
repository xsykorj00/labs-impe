/*
 * ============================================================
 * IMP  –  Lab 4  (STUDENT TEMPLATE)
 * ADC continuous sampling, result shown on 7-segment display
 * ============================================================
 *
 * Student/author ID:
 * name and surname :
 * login at FIT VUT :
 *
 * ============================================================
 * Target : MCXC143VFT
 *
 * Analogue input
 * --------------
 * PTC0   RPOT_SLIDE (sliding potentiometer)   ADC0_SE14
 * PCR MUX = 0 (analogue / pin buffer disabled)
 *
 * Display wiring  (same as Lab 2)
 * --------------------------------
 * PTE18=SEG_A  PTE19=SEG_B  PTE20=SEG_C  PTE21=SEG_D
 * PTE24=SEG_E  PTE25=SEG_F  PTE29=SEG_G  PTE30=SEG_DP
 * PTA1=DIG1    PTA2=DIG2    PTA4=DIG3    PTC2=DIG4
 *
 * ADC0 configuration
 * ------------------
 * Channel   : SE14 (single-ended, PTC0)  ADCH = 14 = 0b01110
 * Resolution: 8-bit  →  result 0 … 255
 * Clock     : bus clock / 2 = 12 MHz     CFG1: ADICLK = 01
 * Sampling  : long sample time           CFG1: ADLSMP = 1
 * Averaging : 32 hardware samples        SC3:  AVGE=1, AVGS=11b
 * Mode      : continuous conversion      SC3:  ADCO = 1
 * Interrupt : conversion complete        SC1[0]: AIEN = 1
 * IRQ       : ADC0_IRQn (IRQ 15)
 *
 * Display behaviour
 * -----------------
 * 8-bit result (0–255) is scaled to 0–330 (representing 0.00–3.30 V):
 * DIG1 = hundreds digit  + decimal point always lit → X.XX format
 * DIG2 = tens digit
 * DIG3 = units digit
 * DIG4 = 'U' symbol (decorative)
 * Time-multiplexed at ~3 ms per digit ≈ 83 Hz refresh rate.
 * ============================================================
 */

#include "MCXC143.h"

/* ── Display constants ───────────────────────────────────────────────────── */

#define SEG_A   (1u << 18)
#define SEG_B   (1u << 19)
#define SEG_C   (1u << 20)
#define SEG_D   (1u << 21)
#define SEG_E   (1u << 24)
#define SEG_F   (1u << 25)
#define SEG_G   (1u << 29)
#define SEG_DP  (1u << 30)
#define ALL_SEGS  (SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G|SEG_DP)
#define PATTERN_U (SEG_B|SEG_C|SEG_D|SEG_E|SEG_F)

#define DIG1  (1u << 1)   /* PTA1 */
#define DIG2  (1u << 2)   /* PTA2 */
#define DIG3  (1u << 4)   /* PTA4 */
#define DIG4  (1u << 2)   /* PTC2 */

static const uint32_t DIGIT_PATTERN[10] = {
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,           /* 0 */
    SEG_B|SEG_C,                                   /* 1 */
    SEG_A|SEG_B|SEG_D|SEG_E|SEG_G,                 /* 2 */
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_G,                 /* 3 */
    SEG_B|SEG_C|SEG_F|SEG_G,                       /* 4 */
    SEG_A|SEG_C|SEG_D|SEG_F|SEG_G,                 /* 5 */
    SEG_A|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,           /* 6 */
    SEG_A|SEG_B|SEG_C,                             /* 7 */
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,     /* 8 */
    SEG_A|SEG_B|SEG_C|SEG_D|SEG_F|SEG_G,           /* 9 */
};

/* ── ADC result (written by ISR, read by main) ───────────────────────────── */

static volatile uint8_t adc_result = 0;

/* ── Display helpers ─────────────────────────────────────────────────────── */

static void delay_us(uint32_t us)
{
    volatile uint32_t i;
    for (i = 0; i < us * 48; i++) { __NOP(); }
}

static void display_off(void)
{
    GPIOE->PSOR = ALL_SEGS;        
    GPIOA->PSOR = DIG1|DIG2|DIG3; 
    GPIOC->PSOR = DIG4;            
}

static void show_digit_123(uint32_t dig_porta_bit, uint8_t value)
{
    display_off();
    GPIOE->PCOR = DIGIT_PATTERN[value];
    GPIOA->PCOR = dig_porta_bit;
}

static void show_pattern_4(uint32_t pattern)
{
    display_off();
    GPIOE->PCOR = pattern;
    GPIOC->PCOR = DIG4;            
}

/* ── ADC0 interrupt handler (IRQ 15) ─────────────────────────────────────── */

void ADC0_IRQHandler(void)
{
    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
     * TASK 1: Read the ADC Result Data Register.                                 *
     * Store the result from ADC0->R[0] into the global `adc_result` variable.    *
     * Note: Reading R[0] automatically clears the Conversion Complete (COCO)     *
     * flag and re-arms the continuous mode hardware.                             *
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
     
    // TODO: Read ADC result register here
    
}

/* ── Initialisation ──────────────────────────────────────────────────────── */

static void clock_init(void)
{
    SIM->COPC    = SIM_COPC_COPT(0);
    MCG->MC     |= MCG_MC_HIRCEN_MASK;
    MCG->C1      = (uint8_t)((MCG->C1 & ~MCG_C1_CLKS_MASK) | MCG_C1_CLKS(2));
    while ((MCG->S & MCG_S_CLKST_MASK) != MCG_S_CLKST(1)) { }
    SIM->CLKDIV1 = SIM_CLKDIV1_OUTDIV1(0) | SIM_CLKDIV1_OUTDIV4(1);
}

static void display_init(void)
{
    PORTE->PCR[18] = PORT_PCR_MUX(1); PORTE->PCR[19] = PORT_PCR_MUX(1);
    PORTE->PCR[20] = PORT_PCR_MUX(1); PORTE->PCR[21] = PORT_PCR_MUX(1);
    PORTE->PCR[24] = PORT_PCR_MUX(1); PORTE->PCR[25] = PORT_PCR_MUX(1);
    PORTE->PCR[29] = PORT_PCR_MUX(1); PORTE->PCR[30] = PORT_PCR_MUX(1);
    GPIOE->PDDR   |= ALL_SEGS;
    GPIOE->PSOR    = ALL_SEGS;

    PORTA->PCR[1]  = PORT_PCR_MUX(1); PORTA->PCR[2]  = PORT_PCR_MUX(1);
    PORTA->PCR[4]  = PORT_PCR_MUX(1);
    GPIOA->PDDR   |= DIG1|DIG2|DIG3;
    GPIOA->PSOR    = DIG1|DIG2|DIG3;

    PORTC->PCR[2]  = PORT_PCR_MUX(1);
    GPIOC->PDDR   |= DIG4;
    GPIOC->PSOR    = DIG4;
}

/*
 * configure ADC0 for 8-bit continuous single-ended conversion on SE14
 */
static void adc0_init(void)
{
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
     * TASK 2: Configure ADC Registers.                                           *
     * a) CFG1: ADICLK = 1 (bus clock/2), MODE = 0 (8-bit), ADLSMP = 1 (long time)
     * b) CFG2: Set to 0 to select 'a' side channels (SE14).                     *
     * c) SC3:  Enable ADCO (continuous), AVGE (averaging), AVGS = 3 (32 samples)*
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    // TODO: a) Configure CFG1 register
    

    // TODO: b) Configure CFG2 register
    

    // TODO: c) Configure SC3 register
    

    NVIC_ClearPendingIRQ(ADC0_IRQn);
    NVIC_EnableIRQ(ADC0_IRQn);

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
     * TASK 3: Trigger the first conversion.                                      *
     * Write SC1[0] LAST to select channel SE14 (ADCH=14) and enable interrupt    *
     * on completion (AIEN=1). This locks the config and starts the converter.    *
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    // TODO: Write to SC1[0] register
    
}

/*
 * Configure TPM0 CH0/CH1/CH2 as edge-aligned PWM on PTD0/PTD1/PTD2 (RGB LED).
 * No overflow IRQ needed — main() writes CnV directly from adc_result.
 */
static void tpm0_init(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;

    SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_TPMSRC_MASK) | SIM_SOPT2_TPMSRC(1);

    PORTD->PCR[0] = PORT_PCR_MUX(4);   /* PTD0 → TPM0_CH0  RED   */
    PORTD->PCR[1] = PORT_PCR_MUX(4);   /* PTD1 → TPM0_CH1  GREEN */
    PORTD->PCR[2] = PORT_PCR_MUX(4);   /* PTD2 → TPM0_CH2  BLUE  */

    TPM0->SC  = 0;
    TPM0->CNT = 0;
    TPM0->MOD = 255;                    /* 8-bit resolution matches adc_result */

    TPM0->CONTROLS[0].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;
    TPM0->CONTROLS[0].CnV  = 0;

    TPM0->CONTROLS[1].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;
    TPM0->CONTROLS[1].CnV  = 0;

    TPM0->CONTROLS[2].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;
    TPM0->CONTROLS[2].CnV  = 0;

    /* Start: internal clock, /8 prescaler, no overflow IRQ */
    TPM0->SC = TPM_SC_CMOD(1) | TPM_SC_PS(3);
}

/* ── Main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    clock_init();

    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTC_MASK
               |  SIM_SCGC5_PORTE_MASK;

    PORTC->PCR[0] = PORT_PCR_MUX(0); /* Analogue input: disable GPIO buffer */

    display_init();
    adc0_init();
    tpm0_init();

    for (;;) {
        /* Standard C scaling logic is provided so you can focus on multiplexing */
        uint8_t  sample    = adc_result;                      
        uint32_t scaled    = ((uint32_t)sample * 330) / 255;  

        /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
         * TASK 4: Count the voltage that should be displayed                         *
         * Scaling: the 8-bit ADC range (0–255) maps linearly to 0–330,               *
         * representing 0.00 V–3.30 V (the potentiometer supply range).               *
         *   scaled = (sample × 330) / 255                                            *
         * Hint: for example 255 ~ 3.30 V or 127 ~ 1.65 V                             *
         * Cast to uint32_t prevents 8-bit overflow during multiplication.            *
         * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

        // TODO: replace the variable 'scaled' into formula for getting correct digit value
        uint8_t hundreds = (uint8_t)scaled;
        uint8_t tens     = (uint8_t)scaled;
        uint8_t units    = (uint8_t)scaled;

        /* Update LED brightness directly from raw ADC value */
        TPM0->CONTROLS[0].CnV = sample;   /* RED   */
        TPM0->CONTROLS[1].CnV = sample;   /* GREEN */
        TPM0->CONTROLS[2].CnV = sample;   /* BLUE  */

        /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
         * TASK 5: Display Time-Multiplexing                                          *
         * Show each digit sequentially, utilizing a delay_us(20) interval so         *
         * the human eye perceives a continuous image.                                *
         * - DIG1: 'hundreds' variable + SEG_DP. (Use GPIOE->PCOR & GPIOA->PCOR)      *
         *          Hint: see show_digit_123() function                               *
         * - DIG2: 'tens' variable. (Use the show_digit_123 helper)                   *
         * - DIG3: 'units' variable. (Use the show_digit_123 helper)                  *
         * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

        // TODO: Assign the value to the digits
        
        /* DIG1: hundreds + decimal point → X.XX format */
        
        
        /* DIG2: tens digit */
        
        
        /* DIG3: units digit */
        
        
        /* DIG4: decorative 'U' symbol */
        show_pattern_4(PATTERN_U);        
    }
}
