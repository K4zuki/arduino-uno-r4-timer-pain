#include <FspTimer.h>
static FspTimer fsp_timer;

void setup() {
  fsp_timer.begin(TIMER_MODE_PWM,0,4, 1000.0 ,25.0);
  fsp_timer.open();

  R_PFS->PORT[3].PIN[1].PmnPFS_b.PMR = 0;  //D0を汎用入出力に設定
  R_PFS->PORT[3].PIN[2].PmnPFS_b.PMR = 0;  //D1を汎用入出力に設定
  R_PFS->PORT[3].PIN[1].PmnPFS_b.PSEL = 0x03; //D0をGPT端子として使用
  R_PFS->PORT[3].PIN[2].PmnPFS_b.PSEL = 0x03; //D1をGPT端子として使用
  R_PFS->PORT[3].PIN[1].PmnPFS_b.PMR = 1;  //D0を周辺機能用の端子に設定
  R_PFS->PORT[3].PIN[2].PmnPFS_b.PMR = 1;  //D1を周辺機能用の端子に設定

  R_GPT4->GTIOR_b.GTIOA = 0x06;  //GTIOCA端子機能選択
  R_GPT4->GTIOR_b.GTIOB = 0x19;  //GTIOCB端子機能選択
  R_GPT4->GTIOR_b.OAE = 1;  //GTIOCA端子出力許可
  R_GPT4->GTIOR_b.OBE = 1;  //GTIOCB端子出力許可

  fsp_timer.start();
}

void loop() {

}
