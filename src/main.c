#include <zephyr/kernel.h>             // Funções básicas do Zephyr (ex: k_msleep, k_thread, etc.)
#include <zephyr/device.h>             // API para obter e utilizar dispositivos do sistema
#include <zephyr/drivers/gpio.h>       // API para controle de pinos de entrada/saída (GPIO)
#include <pwm_z42.h>                   // Biblioteca personalizada com funções de controle do TPM (Timer/PWM Module)

#define TEMPO 2000 // 2 segundo
// Define o valor do registrador MOD do TPM para configurar o período do PWM
#define TPM_MODULE 1000         // Define a frequência do PWM fpwm = (TPM_CLK / (TPM_MODULE * PS))
// Valores de duty cycle correspondentes a diferentes larguras de pulso
uint16_t duty_Red  = TPM_MODULE*0.500;             

int main(void)
{
    pwm_tpm_Init(TPM2, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);

    // Inicializa o canal 0 do TPM2 para gerar sinal PWM na porta GPIOB_18
    // - modo TPM_PWM_H (nível alto durante o pulso)
    pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, 18); // vermelho
    pwm_tpm_Ch_Init(TPM2, 1, TPM_PWM_H, GPIOB, 19); // Verde
    //Pinos dos motores (PTB2 e PTB3)
    pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, 12); //PTB2 - pino 12
    pwm_tpm_Ch_Init(TPM2, 1, TPM_PWM_H, GPIOB, 13); //PTB3 - pino 13
    // Loop infinito
    while(1)
    {
        pwm_tpm_CnV(TPM2, 0, duty_Red);
        pwm_tpm_CnV(TPM2, 1 , 1000);
        k_msleep(TEMPO);
        pwm_tpm_CnV(TPM2, 0, 1000);
        pwm_tpm_CnV(TPM2, 1, duty_Red);
        k_msleep(TEMPO);
    }
    return 0;
}   