stm32f072上外挂了4颗MEMS Mic: ICS-43434

PA15/PA7/PB3控制其中一组:

PA15:WS
PA7:SD
PB3:SCK


PB12/PB15/PB10控制另外一组:

PB12:WS
PB15:SD
PB10:SCK

crystal:
stm32f072上的外部晶振频率是:12.288MHz,要使用外部晶振

请帮我实现如下功能:
1.验证两组麦克风都能工作
2.在stm32f072上实现usb转麦克风功能
3.将生成的固件转换成uf2格式,方便我刷写
4.尽量使用STM32F0xx_Hal实现



ref:
https://github.com/hananabilabd/Real-time-STM32-MEMS-MIC-Audio-Playback-DSP
https://andybrown.me.uk/2021/03/13/usb-microphone/
https://github.com/andysworkshop/usb-microphone
https://www.electronics-lab.com/i2s-usb-microphone-using-stm32-and-mems-microphone/
https://www.adafruit.com/product/3421
https://stackoverflow.com/questions/70048351/stm32-i2s-input-not-working-when-using-dma
https://community.st.com/s/question/0D53W00000PUoNXSA1/how-to-interface-i2s-mems-microphone-with-stm32f4-for-kws-application
https://www.google.com/search?q=stm32+i2s+microphone+example&sxsrf=ALiCzsb-Dh8oPGcwzjgZ3n2gQvHKsjDfFQ:1671105152358&ei=gAqbY6DAFZP7wAOOtb6IAg&start=10&sa=N&ved=2ahUKEwiggPTnx_v7AhWTPXAKHY6aDyEQ8tMDegQIAxAE&biw=2560&bih=1158&dpr=2
