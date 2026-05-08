#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <pwm_z42.h>

// Dispositivos GPIO
const struct device *dev_gpio_e = DEVICE_DT_GET(DT_NODELABEL(gpioe));
const struct device *dev_gpio_a = DEVICE_DT_GET(DT_NODELABEL(gpioa));

#define PIN_ECHO 20 // PTE20 (TPM1_CH0)
#define PIN_TRIG 12 // PTA12 (Trigger)
#define TPM_MODULE 2000

uint16_t duty_Red = TPM_MODULE * 0.400;

// Variáveis para o cálculo
volatile uint32_t t_subida = 0;
volatile uint32_t t_descida = 0;
volatile uint32_t pulso_ticks = 0;
volatile bool dado_pronto = false;
volatile bool esperando_subida = true; 

void tpm1_isr(void *arg) {
    // Limpa a flag de interrupção do canal 0 do TPM1
    TPM1->STATUS |= TPM_STATUS_CH0F_MASK;

    if (esperando_subida) {
        // Se estávamos esperando a subida, esta interrupção É a borda de subida
        t_subida = TPM1->CONTROLS[0].CnV;
        esperando_subida = false; // O próximo evento obrigatoriamente será a descida
    } else {
        // Se não estávamos esperando a subida, esta interrupção É a borda de descida
        t_descida = TPM1->CONTROLS[0].CnV;
        
        // Trata o estouro (overflow) do timer de 16 bits
        if (t_descida >= t_subida) {
            pulso_ticks = t_descida - t_subida;
        } else {
            pulso_ticks = (65535 - t_subida) + t_descida;
        }
        
        dado_pronto = true;       // Sinaliza ao main que a leitura terminou com sucesso
        esperando_subida = true;  // Reseta o estado para a próxima medição do sensor
    }
}

void _foward()
{
    pwm_tpm_CnV(TPM2, 0, 2000);
    pwm_tpm_CnV(TPM2, 1, 2000);
    pwm_tpm_CnV(TPM0, 2, duty_Red);
    pwm_tpm_CnV(TPM0, 3, duty_Red);
}

void _stop()
{
    pwm_tpm_CnV(TPM2, 0, 2000);
    pwm_tpm_CnV(TPM2, 1, 2000);
    pwm_tpm_CnV(TPM0, 2, 2000);
    pwm_tpm_CnV(TPM0, 3, 2000);
    printk("Pare de uma vez! Pare imediatamente!\n");
}

void main(void) {
    // 1. Inicializa GPIOs
    if (!device_is_ready(dev_gpio_e) || !device_is_ready(dev_gpio_a)) return;
    gpio_pin_configure(dev_gpio_a, PIN_TRIG, GPIO_OUTPUT_INACTIVE);
    
    // CORREÇÃO: Removida a linha gpio_pin_configure do PIN_ECHO para evitar conflito com o Timer

    // 2. Configura Interrupção e Timer para Input Capture
    IRQ_CONNECT(TPM1_IRQn, 1, tpm1_isr, NULL, 0);
    irq_enable(TPM1_IRQn);
    
    // Inicializa TPM1 para contar (Prescaler 128)
    pwm_tpm_Init(TPM1, TPM_PLLFLL, 65535, TPM_CLK, PS_128, EDGE_PWM);
    
    // CORREÇÃO: Mudado de TPM_INPUT_CAPTURE_RISING para TPM_INPUT_CAPTURE_BOTH
    pwm_tpm_Ch_Init(TPM1, 0, TPM_INPUT_CAPTURE_BOTH | TPM_CHANNEL_INTERRUPT, GPIOE, PIN_ECHO);

    // 3. Inicializa Motores no TPM2
    pwm_tpm_Init(TPM2, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);
    pwm_tpm_Init(TPM0, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);
    
    // Nota técnica: Como o TPM2 só tem 2 canais, as linhas abaixo reconfiguram os mesmos canais.
    // Os pinos GPIOB 2 e 3 deixarão de responder como PWM, apenas GPIOE 22 e 23 estarão ativos.
    pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, 2);  // IN 1 - Motor 1
    pwm_tpm_Ch_Init(TPM2, 1, TPM_PWM_H, GPIOB, 3);  // IN 3 - Motor 2
    pwm_tpm_Ch_Init(TPM0, 2, TPM_PWM_H, GPIOE, 29); // IN 2 - Motor 1 (Ativo)
    pwm_tpm_Ch_Init(TPM0, 3, TPM_PWM_H, GPIOE, 30); // IN 4 - Motor 2 (Ativo)

    while (1)
    {
        // 1. Força a sincronização lógica antes de disparar
        dado_pronto = false;
        esperando_subida = true;

        // 2. Dispara o Trigger do sensor
        gpio_pin_set(dev_gpio_a, PIN_TRIG, 1);
        k_busy_wait(10); // Aguarda exatamente os 10 microsegundos exigidos pelo HC-SR04
        gpio_pin_set(dev_gpio_a, PIN_TRIG, 0);

        // 3. Dá um tempo seguro para as interrupções acontecerem (o eco leva no máximo ~30ms)
        k_msleep(60); 

        // 4. Verifica se a interrupção preencheu os dados com sucesso
        if (dado_pronto) {
            // Conversão: com Prescaler 128 a 48MHz, cada tick do timer equivale a ~2.666us
            float distancia = (pulso_ticks * 2.666f) / 58.0f;
            int dist_inteira = (int)distancia;
            int dist_decimal = (int)((distancia - dist_inteira) * 10);
            
            printk("Distancia calculada: %d.%d cm\n", dist_inteira, dist_decimal);

            // Lógica de parada exata do carrinho
            if (distancia <= 25.0f && distancia > 2.0f) {
                _stop();
            } else {
                _foward();
            }
        } else {
            // Se cair aqui, o sensor não respondeu a tempo
            printk("Erro: Eco nao retornado pelo sensor.\n");
        }
    }
}