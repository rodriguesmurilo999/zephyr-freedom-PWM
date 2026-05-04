#include <zephyr/kernel.h>                 // Funções básicas do Zephyr (ex: k_msleep, k_thread, etc.)
#include <zephyr/device.h>                 // API para obter e utilizar dispositivos do sistema
#include <zephyr/drivers/gpio.h>           // API para controle de pinos de entrada/saída (GPIO)
#include <pwm_z42.h>

#define INPUT_PORT  "gpio@400ff100"        // Porta E = GPIO_4 no seu .dts
#define INPUT_PINA   20                    // PTE20
#define INPUT_PINB   21                    // PTE21
#define TEMPO        1000                  // 1 segundo
// Define o valor do registrador MOD do TPM para configurar o período do PWM
#define TPM_MODULE   2000                  // Define a frequência do PWM fpwm = (TPM_CLK / (TPM_MODULE * PS))
// Valores de duty cycle correspondentes a diferentes larguras de pulso
uint16_t duty_Red    = TPM_MODULE*0.025;       
uint16_t duty_Cur    = TPM_MODULE*0.300; 

void _foward()
{
    //Motores girando para frente (PTB2 e PTB3) - Amarelo
    pwm_tpm_CnV(TPM2, 0, duty_Red); //Motor A
    pwm_tpm_CnV(TPM2, 1, duty_Red); //Motor B
    pwm_tpm_CnV(TPM1, 0, 2000);     //Motor A
    pwm_tpm_CnV(TPM1, 1, 2000);     //Motor B
}

void _left()
{
    //Motores curva para esquerda (PTB3) - Verde
    pwm_tpm_CnV(TPM2, 0, 2000);     //Motor A
    pwm_tpm_CnV(TPM2, 1, duty_Cur); //Motor B
    pwm_tpm_CnV(TPM1, 0, duty_Cur);     //Motor A
    pwm_tpm_CnV(TPM1, 1, 2000);     //Motor B
}

void _right()
{
    //Motores curva para esquerda (PTB2) - Vermelho
    pwm_tpm_CnV(TPM2, 0, duty_Cur); //Motor A
    pwm_tpm_CnV(TPM2, 1, 2000);     //Motor B
    pwm_tpm_CnV(TPM1, 0, 2000);     //Motor A
    pwm_tpm_CnV(TPM1, 1, duty_Cur);     //Motor B
}

int main()
{
    pwm_tpm_Init(TPM2, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);
    pwm_tpm_Init(TPM1, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);

    // Inicializa o canal 0 do TPM2 para gerar sinal PWM na porta GPIOB_18
    // - modo TPM_PWM_H (nível alto durante o pulso)
    pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, 18); // vermelho
    pwm_tpm_Ch_Init(TPM2, 1, TPM_PWM_H, GPIOB, 19); // Verde
    //Pinos dos motores (PTD1 e PTD2 para hor)
    pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, 2); //PTB2 
    pwm_tpm_Ch_Init(TPM2, 1, TPM_PWM_H, GPIOB, 3); //PTB3 
    //Pinos dos motores (PTC8 e PTC9 para anti-hor)
    pwm_tpm_Ch_Init(TPM1, 0, TPM_PWM_H, GPIOA, 12); //PTA12
    pwm_tpm_Ch_Init(TPM1, 1, TPM_PWM_H, GPIOA, 13); //PTA13

    const struct device *input_dev;
    int retA, retB, valA, valB;

    input_dev = device_get_binding(INPUT_PORT);
    if (!input_dev)
    {
        printk("Erro ao acessar porta %s\n", INPUT_PORT);
        return;
    }

    retA = gpio_pin_configure(input_dev, INPUT_PINA, GPIO_INPUT);
    retB = gpio_pin_configure(input_dev, INPUT_PINB, GPIO_INPUT);
    if(retA != 0)
    {
        printk("Erro ao configurar pino %d\n", INPUT_PINA);
        return;
    }
    if(retB != 0)
    {
        printk("Erro ao configurar pino %d\n", INPUT_PINB);
        return;
    }
    
    while (1)
    {
        valA = gpio_pin_get(input_dev, INPUT_PINA);
        valB = gpio_pin_get(input_dev, INPUT_PINB);
        if(valA == 0 && valB == 0 || valA == 1 && valB == 1)
        {
            _foward();
        }
        else if(valA == 0 && valB == 1)
        {
            _left();
        }
        else if(valA == 1 && valB == 0)
        {
            _right();
        }
    }
}
