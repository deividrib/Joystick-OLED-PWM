# Projeto: Joystick + OLED + PWM

Este projeto tem como objetivo demonstrar o uso do conversor analógico-digital (ADC) do RP2040 para ler a posição de um joystick, controlar a intensidade de LEDs via PWM e exibir a posição do joystick em um display SSD1306 (128×64) por meio de um quadrado móvel. Além disso, há dois botões (um do joystick e um “Botão A”) que disparam interrupções para alterar comportamentos no sistema (ligar/desligar PWM, trocar borda do display, acionar LED verde, etc.).

## Funcionalidades Principais

1. **Leitura do Joystick (ADC)**
   - Leitura dos eixos X e Y por meio do ADC do RP2040 (GPIO26 e GPIO27).
   - Implementação de uma zona morta (dead zone) para evitar variações muito pequenas.

2. **Controle de LEDs via PWM**
   - LED Vermelho (GPIO13) controlado pela variação do eixo X.
   - LED Azul (GPIO12) controlado pela variação do eixo Y.
   - LED Verde (GPIO11) ligado/desligado pelo botão do joystick.

3. **Exibição em Display OLED SSD1306 (128×64)**
   - Exibe um quadrado 8×8 cuja posição é atualizada em função dos valores do joystick.
   - Possui duas opções de borda (sólida e pontilhada), alternadas pelo botão do joystick.
   - Exibe uma mensagem indicando o status do PWM (ON/OFF).

4. **Botões e Interrupções**
   - Botão do Joystick (GPIO22): alterna o LED verde e a borda do display.
   - Botão A (GPIO5): ativa ou desativa os LEDs controlados por PWM.
   - Tratamento de debounce via software.

## Organização do Código

- **`Joystick-OLED-PWM.c`**  
  Arquivo principal contendo o `main()` e toda a lógica de:
  - Configuração de GPIOs.
  - Leitura de ADC.
  - Setup do display SSD1306 via I2C.
  - Funções de interrupção para os botões.
  - Loop principal atualizando PWM e desenhando no display.

- **`ssd1306.c` e `ssd1306.h`**  
  Implementação de funções para inicializar e desenhar no display SSD1306.

- **`font.h`**  
  Fonte 8×8 utilizada para desenhar caracteres e bitmaps no display.

## Hardware Utilizado

- **RP2040** (Raspberry Pi Pico ou placa similar)
- **Display OLED SSD1306** (128×64, conectado via I2C)
- **Joystick** (pinos ligados ao ADC do RP2040, além de um botão digital)
- **LED RGB** (pinos de cor conectados às GPIOs 11, 12 e 13)
- **Botão A** (GPIO5)

## Demonstração em Vídeo

[Insira aqui o link para o vídeo de demonstração](https://exemplo.com/video)

## Autor

**Deividson Ribeiro Silva**  
[devidrs27@gmail.com](mailto:devidrs27@gmail.com)
