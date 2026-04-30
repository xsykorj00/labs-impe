/*
 * ============================================================
 * IMP  –  Lab 2  (TEACHER SOLUTION)
 * GPIO ports, interrupts, 7-segment LED display
 * ============================================================
 *
 * Target : MCXC143VFT
 *
 * Display wiring  (segments active LOW; digits via BSS84PW P-FETs)
 * ---------------------------------------------------------------
 * PTE18 = SEG_A    PTE19 = SEG_B    PTE20 = SEG_C    PTE21 = SEG_D
 * PTE24 = SEG_E    PTE25 = SEG_F    PTE29 = SEG_G    PTE30 = SEG_DP
 *
 * Digit-select pins (LOW = P-FET ON = anode powered):
 * PTA1 = DIG1_SEL   PTA2 = DIG2_SEL   PTA4 = DIG3_SEL
 * PTC2 = DIG4_SEL
 *
 * Buttons  (active LOW, pull-up, falling-edge interrupt – PORTC/D IRQ 31)
 * PTC1 = PB1 (UP)       PTC3 = PB2 (LEFT)     PTC4 = PB3 (CENTER)
 * PTC5 = PB4 (RIGHT)    PTC6 = PB5 (DOWN)
 *
 * ============================================================
 * Face states  –  [DIG1] [DIG2] [DIG3] [DIG4]
 * ============================================================
 * ┌──────────────────┬──────┬──────┬──────┬──────┐
 * │ State            │ DIG1 │ DIG2 │ DIG3 │ DIG4 │
 * ├──────────────────┼──────┼──────┼──────┼──────┤
 * │ FACE_DEFAULT     │  o   │  _   │  _   │  o   │
 * │ FACE_SURPRISED   │  O   │  _   │  _   │  O   │
 * │ FACE_ASLEEP      │  _   │  -   │  -   │  _   │
 * │ FACE_WINK_LEFT   │  ^   │  _   │  _   │  o   │
 * │ FACE_WINK_RIGHT  │  o   │  _   │  _   │  ^   │
 * │ FACE_HAPPY       │  U   │  _   │  _   │  U   │
 * └──────────────────┴──────┴──────┴──────┴──────┘
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
#define ALL_SEGS (SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G|SEG_DP)

/* Digit-select bit-masks */
#define DIG1  (1u << 1)   /* GPIOA PTA1 */
#define DIG2  (1u << 2)   /* GPIOA PTA2 */
#define DIG3  (1u << 4)   /* GPIOA PTA4 */
#define DIG4  (1u << 2)   /* GPIOC PTC2 */

/* Segment patterns for expressions */
#define PAT_o      (SEG_C | SEG_D | SEG_E | SEG_G)
#define PAT_O      (SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F)
#define PAT_U      (SEG_B | SEG_C | SEG_D | SEG_E | SEG_F)
#define PAT_LINE_D (SEG_D)
#define PAT_LINE_G (SEG_G)
#define PAT_WINK   (SEG_C | SEG_E | SEG_G)  /* ^ winking eye */

/* Welcome-animation letter patterns */
#define PATTERN_L  (SEG_D | SEG_E | SEG_F)
#define PATTERN_A  (SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G)
#define PATTERN_b  (SEG_C | SEG_D | SEG_E | SEG_F | SEG_G)
#define PATTERN_2  (SEG_A | SEG_B | SEG_D | SEG_E | SEG_G)

/* Face configurations */
typedef enum {
    FACE_DEFAULT,
    FACE_SURPRISED,
    FACE_ASLEEP,
    FACE_WINK_LEFT,
    FACE_WINK_RIGHT,
    FACE_HAPPY
} face_state_t;

/* ── Application state ───────────────────────────────────────────────────── */

static volatile uint32_t face[4];

/*
 * face_ticks: counts down once per full 4-digit refresh cycle in main().
 * The ISR sets it to FACE_TIMEOUT on each button press.
 * When it reaches 0, the display reverts to the default face.
 */
#define FACE_TIMEOUT  600u
static volatile uint32_t face_ticks = 0;

/* ── Utility functions ───────────────────────────────────────────────────── */

static void delay(uint32_t ticks)
{
    volatile uint32_t i;
    for (i = 0; i < ticks; i++) { __NOP(); }
}

static void display_off(void)
{
    GPIOE->PSOR = ALL_SEGS;
    GPIOA->PSOR = DIG1 | DIG2 | DIG3;
    GPIOC->PSOR = DIG4;
}

static void show_on_dig123(uint32_t dig_porta_bit, uint32_t pattern)
{
    display_off();
    GPIOE->PCOR = pattern;
    GPIOA->PCOR = dig_porta_bit;
}

static void show_on_dig4(uint32_t pattern)
{
    display_off();
    GPIOE->PCOR = pattern;
    GPIOC->PCOR = DIG4;
}

static void set_face(face_state_t state)
{
    switch (state) {
        case FACE_DEFAULT:
            face[0] = PAT_o;      face[1] = PAT_LINE_D; face[2] = PAT_LINE_D; face[3] = PAT_o;
            face_ticks = 0;       /* default face never times out */
            break;
        case FACE_SURPRISED:
            face[0] = PAT_O;      face[1] = PAT_LINE_D; face[2] = PAT_LINE_D; face[3] = PAT_O;
            face_ticks = FACE_TIMEOUT;
            break;
        case FACE_ASLEEP:
            face[0] = PAT_LINE_D; face[1] = PAT_LINE_G; face[2] = PAT_LINE_G; face[3] = PAT_LINE_D;
            face_ticks = FACE_TIMEOUT;
            break;
        case FACE_WINK_LEFT:
            face[0] = PAT_WINK;   face[1] = PAT_LINE_D; face[2] = PAT_LINE_D; face[3] = PAT_o;
            face_ticks = FACE_TIMEOUT;
            break;
        case FACE_WINK_RIGHT:
            face[0] = PAT_o;      face[1] = PAT_LINE_D; face[2] = PAT_LINE_D; face[3] = PAT_WINK;
            face_ticks = FACE_TIMEOUT;
            break;
        case FACE_HAPPY:
            face[0] = PAT_U;      face[1] = PAT_LINE_D; face[2] = PAT_LINE_D; face[3] = PAT_U;
            face_ticks = FACE_TIMEOUT;
            break;
    }
}

/* ── Welcome animation  –  scrolls "LAb2" three times ───────────────────── */
static void welcome_animation(void)
{
    const uint32_t HOLD = 250000;
    int round;
    for (round = 0; round < 3; round++) {
        show_on_dig123(DIG1, PATTERN_L); delay(HOLD); display_off();
        show_on_dig123(DIG2, PATTERN_A); delay(HOLD); display_off();
        show_on_dig123(DIG3, PATTERN_b); delay(HOLD); display_off();
        show_on_dig4(PATTERN_2);         delay(HOLD); display_off();
    }
}

/* ── Interrupt handlers ──────────────────────────────────────────────────── */

void PORTC_PORTD_IRQHandler(void)
{
    if (PORTC->PCR[1] & PORT_PCR_ISF_MASK) {
        PORTC->PCR[1] |= PORT_PCR_ISF_MASK;  /* UP */
        set_face(FACE_SURPRISED);
    }
    if (PORTC->PCR[3] & PORT_PCR_ISF_MASK) {
        PORTC->PCR[3] |= PORT_PCR_ISF_MASK;  /* LEFT */
        set_face(FACE_WINK_LEFT);
    }
    if (PORTC->PCR[4] & PORT_PCR_ISF_MASK) {
        PORTC->PCR[4] |= PORT_PCR_ISF_MASK;  /* CENTER */
        set_face(FACE_HAPPY);
    }
    if (PORTC->PCR[5] & PORT_PCR_ISF_MASK) {
        PORTC->PCR[5] |= PORT_PCR_ISF_MASK;  /* RIGHT */
        set_face(FACE_WINK_RIGHT);
    }
    if (PORTC->PCR[6] & PORT_PCR_ISF_MASK) {
        PORTC->PCR[6] |= PORT_PCR_ISF_MASK;  /* DOWN */
        set_face(FACE_ASLEEP);
    }
}

/* ── Hardware initialisation ─────────────────────────────────────────────── */

static void clock_init(void)
{
    SIM->COPC = SIM_COPC_COPT(0);
    MCG->MC |= MCG_MC_HIRCEN_MASK;
    MCG->C1 = (uint8_t)(MCG->C1 & ~MCG_C1_CLKS_MASK) | MCG_C1_CLKS(0);
    while ((MCG->S & MCG_S_CLKST_MASK) != MCG_S_CLKST(0)) { }
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
    GPIOA->PDDR   |= DIG1 | DIG2 | DIG3;
    GPIOA->PSOR    = DIG1 | DIG2 | DIG3;

    PORTC->PCR[2]  = PORT_PCR_MUX(1);
    GPIOC->PDDR   |= DIG4;
    GPIOC->PSOR    = DIG4;
}

static void buttons_init(void)
{
    uint32_t btn_pcr = PORT_PCR_MUX(1)
                     | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK
                     | PORT_PCR_IRQC(0xA);

    PORTC->PCR[1] = btn_pcr;   /* UP     */
    PORTC->PCR[3] = btn_pcr;   /* LEFT   */
    PORTC->PCR[4] = btn_pcr;   /* CENTER */
    PORTC->PCR[5] = btn_pcr;   /* RIGHT  */
    PORTC->PCR[6] = btn_pcr;   /* DOWN   */

    NVIC_ClearPendingIRQ(PORTC_PORTD_IRQn);
    NVIC_EnableIRQ(PORTC_PORTD_IRQn);
}

static void system_init(void)
{
    clock_init();
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK
               |  SIM_SCGC5_PORTC_MASK | SIM_SCGC5_PORTE_MASK;
    display_init();
    buttons_init();
}

/* ── Main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    uint32_t current_face[4];

    system_init();
    set_face(FACE_DEFAULT);
    welcome_animation();

    for (;;) {
        /* Safely snapshot volatile ISR variables */
        __disable_irq();
        current_face[0] = face[0];
        current_face[1] = face[1];
        current_face[2] = face[2];
        current_face[3] = face[3];
        __enable_irq();

        /* Drive display (multiplexing) */
        show_on_dig123(DIG1, current_face[0]); delay(1000);
        show_on_dig123(DIG2, current_face[1]); delay(1000);
        show_on_dig123(DIG3, current_face[2]); delay(1000);
        show_on_dig4(current_face[3]);         delay(1000);

        /* Auto-reset countdown: revert to default face after FACE_TIMEOUT cycles */
        if (face_ticks > 0) {
            face_ticks--;
            if (face_ticks == 0) {
                set_face(FACE_DEFAULT);
            }
        }
    }
}
