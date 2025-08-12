# まえがき {-}

## このドキュメントは何 {-}

この本は、Arduino UNO R4 minima (以下 "Arduino R4") の低レベル(ベアメタル)プログラミング研究のため、手始めに
PWMをいじろうとして色々うんざりした経緯を語るドキュメントです。R4 Wifiは実験していません。

::: rmnote

## 愚痴またはポエム {-}

ていうかそもそも、HITACHIマイコンってハードル高くないですか？筆者自身はPIC・AVRを触ったことがあるけどH8・SHなどのHITACHI系には
ほとんど手を出してこなかった系の人間です[^hew-bad-name]。
PICは今で言うインフルエンサー的な方[^gokan]がいて、ラインナップごとに何度も本を書いてくれていたので入門しやすくて、
なおかつ秋月電子でいつでも何個でもDIPで手に入る状況でした。

一方HITACHI純正の評価ボードをどこからどのように手に入れればいいのかわからず、純正なんぞがあるのかもわからず、
ボードを作って売っているところから買って一から自力でノウハウを開拓する他ないように見えるので、全く食指が動きませんでした。いや、
わかるよ、_代理店_だろ？でも売ってくれるわけがないじゃない。

日立がルネになって１５年ほど経ちましたが、いまでもマイコンの評価ボードはメーカーサイトでも品切れ状態です。
そんなの誰が使うねん。まあ品種によるのだけど。


[^gokan]: 名前を伏せなくてもいいかもしれないけど、*G閑氏*のことです。

[^hew-bad-name]: 某月でバイトをしていたのでH8が毎週売れていくのを目にしていましたが、一方でネット上ではあまりいい話を聞きませんでした。
**HEWにうんざりしている**ツーイートばかりが目に入ってました。何がどううんざりさせるのかは不明です。

:::

## この本で解説する範囲 {-}

Arduino R4の2ピンを使って相補型PWMを1チャンネル出すまでを試します。
Arduino R4のMCUには、全部で8チャネルの汎用PWMタイマ（GPT: General PWM Timer）が内蔵されています。
カウンタのビット幅以外にチャネル間の機能の差はありません。16ビットのタイマでも32ビットレジスタを持っているふりをします。
Arduino R4の右側、D0-D13ピンは全部いずれかのチャンネルにつながっていて、タイマごとに2相の信号を出力できます。

ところで、ネットを探してみると、少ないながらも[^r4-minority]ArduinoR4でベアメタルプログラミングをしているブログ記事が見つかります。
いまやろうとしている相補PWMも、まさにそれを取り扱った記事がすでにあったりします。Arduinoライブラリの利用をあきらめて、
ハードウェアマニュアルを片手に頑張って実装したそうです。 なので、やや読みづらいソースコード[^r4-bad-name-convention]になっています。
つまり、どこのレジスタのどのビットをどのように操作するのか、というゴールはもうわかっています。この本では、ライブラリを使うことで、
可読性を上げたプログラムにすることを目標にします。


[^r4-minority]: 記事の少なさはR4が登場して2年？くらいしか経っていないからでしょう。
ライブラリの互換性が良くて、ベアメタルに手を出す必要がないんだと思います。

[^r4-bad-name-convention]: 読みづらいのは、ブログ主さんの問題ではなくて、
マイコンのレジスタフィールドの略称が一見では意味不明なところから来ていると思います。

## おことわり {-}

以下のマイコンボードで実験しました。リストにないものは触っていません。

- Arduino UNO R4 minima <https://docs.arduino.cc/hardware/uno-r4-minima/>
- FLINT ProMicro R4 <https://flint.works/p/flint-promicro-r4/>

コンパイラやライブラリの環境は以下のとおりです。

- Arduino IDE 2.3.6
    - 筆者はMac版・Windows版両方を使っていますが、このドキュメント内ではWindows版のキーアサインで説明します。
      なお、スクリーンショットはMac版になることがあります。
- arduino-cli 1.2.2
    - IDEも内部的に使っていますが、別個にインストールしています。設定ファイル類は共用するようです。
- arduino:renesas_uno 1.5.0
    - ボードマネージャからArduino UNO R4 Boardsをインストールします。
    - 実装コードの引用はGitHubリポジトリ<https://github.com/arduino/ArduinoCore-renesas>の`1.5.0`タグを
      ダウンロードして直接参照しています。

RA4M1のデータシート「ハードウェアマニュアル」(たぶん日立語)は **Rev.1.10 Sep 29, 2023**を参照しました。

\toc

# 先人の成果を調べてみる

*小倉 キャッスル 一馬* 氏のブログに筆者がやろうとしていることがほぼ全部書かれています。以下の記事と続編をたどると、
ArduinoのAPIや関数をほぼ使わず、レジスタの直接操作をして相補PWMにしています。

- [「任意のポートでPWM出力させたい」 (2023-11-14)](https://ameblo.jp/ogura-castle/entry-12828562536.html)
- [「任意のポートでPWM出力させるためのレジスタ制御」 (2023-11-15)](https://ameblo.jp/ogura-castle/entry-12828565434.html)

また、FspTimerの使い方は以下のブログページを参考にしました。

- [「Arduino UNO R4のFspTimerライブラリの使い方」 （2023-08-11 ）](https://qiita.com/yasuhiro-k/items/93efb640aa12f3db9086)
- [「Arduino UNO R4 でタイマー割り込みを使う」 （2024.10.21）](https://workshop.aaa-plaza.net/archives/1658)

キャッスル氏のコードをだいたい要約すると、

- IOピンのレジスタでタイマー機能ににコントロールを渡す
- タイマーのレジスタでピン出力変化タイミングを決める
- その他のタイマー設定をする
- タイマー起動（カウント開始、信号出力開始）

という手順を踏んでいます。

\newpage

::: rmnote
![Arduino UNO R4 Minima ピンアサイン一覧](images/r4-timers-pinout.png){width=150mm #fig:arduino-r4-pin-assignments}
:::

<div class="table" width="[0.15,0.15,0.15,0.05,0.15,0.15,0.15]">

Table: Arduino UNO R4 Minima ピンアサイン一覧 {#tbl:arduino-r4-pin-assignments}

| GPT channel |  RA4M1   | Arduino |   |   Arduino   |  RA4M1   | GPT channel |
|:-----------:|:--------:|:-------:|---|:-----------:|:--------:|:-----------:|
|      -      |    -     |    -    |   |   SCL 18    | **P100** |   **5B**    |
|      -      |    -     |    -    |   |   SDA 17    | **P101** |   **5A**    |
|      -      |    -     |    -    |   |   AREF 16   |   AREF   |      -      |
|      -      |    -     |    -    |   |   GND 15    |   GND    |      -      |
|      -      |    MD    |  1 NC   |   | D13/SCK 14  |   P111   |     3A      |
|      -      |    5V    | 2 IOREF |   | D12/MISO 13 |   P110   |     1B      |
|      -      |  /RESET  | 3 RESET |   | D11/MOSI 12 |   P109   |     1A      |
|      -      |    -     |  4 3V3  |   |  D10/CS 11  |   P112   |     3B      |
|      -      |    5V    |  5 5V   |   |    D9 10    |   P303   |     7B      |
|      -      |   GND    |  6 GND  |   |    D8 9     |   P304   |     7A      |
|      -      |   GND    |  7 GND  |   |     ---     |   ---    |     ---     |
|      -      |   VIN    |  8 VIN  |   |    D7 8     |   P107   |     0A      |
|     ---     |   ---    |   ---   |   |    D6 7     |   P106   |     0B      |
|      -      |   P014   |  9 A0   |   |    D5 6     |   P102   |     2B      |
|      -      |   P000   |  10 A1  |   |    D4 5     |   P103   |     2A      |
|      -      |   P001   |  11 A2  |   |    D3 4     |   P104   |     1B      |
|      -      |   P002   |  12 A3  |   |    D2 3     |   P105   |     1A      |
|   **5A**    | **P101** |  13 A4  |   |   D1/TX 2   |   P302   |     4A      |
|   **5B**    | **P100** |  14 A5  |   |   D0/RX 1   |   P301   |     4B      |

</div>

# PWM.h を読み解いてみる

## `PWM.h` &rarr; `pwm.h`

まず`PWM.h`をインクルードしてみます。多くの作例では、ここで定義されている`PwmOut`オブジェクトを使っています。
IDE上でCtrl+LMBを使いヘッダに飛ぶと、 \
`.../Arduino15/packages/arduino/hardware/renesas_uno/1.4.1/cores/arduino/pwm.h`
となっていました。この部分はターゲットボードごとに変わると思います。

実装ファイルは `.../Arduino15/packages/arduino/hardware/renesas_uno/1.4.1/cores/arduino/pwm.cpp`
です。ヘッダと同じディレクトリにあります。

中身を見ると、はじめの方で早速`Arduino.h` と `FspTimer.h` をインクルードしています。

[PWM.h (先頭部分抜粋)](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable to=8}

インスタンス宣言してコンストラクタを呼び出した時点では、ピン番号を内部に保持するだけで何もしません。
begin関数を呼び出すと、内部で初期設定が行われます。

## `PwmOut` クラス定義

[**`PwmOut`** クラス定義(ヘッダ)](arduino-core-renesas/cores/arduino/pwm.h){
.cpp .listingtable from=8 to=49 #lst:pwmout-class-definition-header}

privateメンバとして`FspTimer`オブジェクト`timer`が使われています。`timer`へのポインタを渡す`get_timer()`関数を通じてアクセスできます。
用意されている関数でカバーされていない設定を行うときは、`get_timer()`経由でオブジェクトを取得してFspTimerクラスの関数やメンバオブジェクトを操作します。

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=37 to=37 nocaption=true}

::: rmnote

\newpage

<div class="table" width="[0.1,0.15,0.25,0.35,0.3]">

Table: `PwmOut`クラスメンバ一覧 {#tbl:pwmout-class-members}

| Scope   | Type     | Return type               | Name                              | Purpose                                                  |
|---------|----------|---------------------------|-----------------------------------|----------------------------------------------------------|
| Public  | Function | `bool`{.cpp}              | `begin()`{.cpp}                   | Initialise and reserve pin and peripheral                |
|         |          | `void`{.cpp}              | `end()`{.cpp}                     | Release pin and peripheral for other usage               |
|         |          | `bool`{.cpp}              | `period(int ms)`{.cpp}            | Sets period in ms                                        |
|         |          | `bool`{.cpp}              | `pulseWidth(int ms)`{.cpp}        | Sets pulse width in ms                                   |
|         |          | `bool`{.cpp}              | `period_us(int us)`{.cpp}         | Sets period in &micro;s                                  |
|         |          | `bool`{.cpp}              | `pulseWidth_us(int us)`{.cpp}     | Sets pulse width in &micro;s                             |
|         |          | `bool`{.cpp}              | `period_raw(int period)`{.cpp}    | Sets period in raw register value                        |
|         |          | `bool`{.cpp}              | `pulseWidth_raw(int pulse)`{.cpp} | Sets pulse width in raw register value                   |
|         |          | `bool`{.cpp}              | `pulse_perc(float duty)`{.cpp}    | Sets duty cycle in percentage                            |
|         |          | `void`{.cpp}              | `suspend()`{.cpp}                 | Stops generating pulse from pin                          |
|         |          | `void`{.cpp}              | `resume()`{.cpp}                  | Restarts generating pulse from pin                       |
|         |          | `FspTimer *`{.cpp}        | `get_timer()`{.cpp}               | Returns pointer to `timer`{.cpp}                         |
| Private | Function | `bool`{.cpp}              | `cfg_pin(int max_index)`{.cpp}    | Sets up pin function and reserves from other peripherals |
|         | Variable | `int`{.cpp}               | `_pin`{.cpp}                      | Pin number                                               |
|         |          | `bool`{.cpp}              | `_enabled`{.cpp}                  | Status flag to enable timer                              |
|         |          | `bool`{.cpp}              | `_is_agt`{.cpp}                   | Flag to determine timer type                             |
|         |          | `TimerPWMChannel_t`{.cpp} | `_pwm_channel`{.cpp}              | PWM channel information either A or B                    |
|         |          | `uint8_t`{.cpp}           | `timer_channel`{.cpp}             | Timer channel number                                     |
|         |          | `FspTimer`{.cpp}          | `timer`{.cpp}                     | FspTimer object                                          |

</div>
:::

## `PwmOut::begin()`{.cpp}

IOとペリフェラルの確保と初期設定を行い、PWM信号の出力を[即時開始する]{.underline}関数です。
3種類の実装が用意されています（次節で詳しく見ます）。使用するピンによってAGTかGPTのタイマブロックを割り当て[^seems-no-agt]、
他のオブジェクトと衝突しないように予約します。
タイマブロックの種類はICレベルでピンごとに固有の割り当てが定義されています。アナログ用のピン(A0~A3)には割り当てがありません。
デジタルピン(D0~D13)とI2C兼用ピン(A4,A5)だけが対象です。

信号出力を開始するタイミングが制御を制御したいので、クラスを使わない別解を探したところ、
この関数内で使われているプライベート関数`cfg_pin()`と等価な操作をすればIOの設定をGPTに割り当てることができそうだとわかりました。

[^seems-no-agt]: ピンの機能割り当てを調べた限り、少なくともUNO R4では、AGTが割り当てられる可能性はなさそうです。

### 互換モード：490Hz、50％

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=13 to=14 nocaption=true}

引数なしの`begin()`は490Hz・50％デューティーに設定されます。R3との互換性のためと思われます。

[`PwmOut::begin()` (互換モード・`pwm.cpp`抜粋)](arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=40 to=59 #lst:pwm_cpp_compatible_mode}

内部で`timer.begin_pwm()`{.cpp}を呼び出しています。

### 周期・パルス幅を設定できるモード

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=15 to=24 nocaption=true}

`raw`に`false`を与えたときは、`period_width`と`pulse_width`はマイクロ秒単位になります。
`raw`を`true`にすると、比較レジスタに即値が取り込まれます。このとき`sd`の値によって実時間が変わってしまいます。

`sd`はタイマクロック回路に与える周波数を決めるための分周比を与えます。1/1（直結・48MHz）から1/1024（46.875kHz）までの10段階です。
タイマーはこの分周されたクロックのパルスをカウントしてPWMなどの機能を実現します。
`timer_source_div_t`型は`r_timer_api.h`で定義されていますが、上記の通り一部だけ有効です。

[`timer_source_div_t`型定義 (`r_timer_api.h`抜粋)](
arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/api/r_timer_api.h){
.cpp .listingtable from=130 to=145}

[`PwmOut::begin()`{.cpp} (周期・パルス幅指定モード・`pwm.cpp`抜粋)](arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=62 to=92 #lst:pwm_cpp_set_pulse_width}

デフォルトでは`raw = false`、`sd = TIMER_SOURCE_DIV_1` です。

### 周波数とデューティー比を設定するモード

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=25 to=25 nocaption=true}

[`PwmOut::begin()`{.cpp} (周波数・デューティー指定モード・`pwm.cpp`抜粋)]( arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=93 to=114 #lst:pwm_cpp_set_freq}

## `PwmOut::cfg_pin()`{.cpp} (プライベート関数)

[`PwmOut::cfg_pin()` (`pwm.cpp`抜粋)](arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=15 to=38 #lst:cfg-pin-whole-code}

::: rmnote

```{.cpp}
bool PwmOut::cfg_pin(int max_index) {
/* verify index are good */
if(_pin < 0 || _pin >= max_index) {
return false;
}
/* getting configuration from table */
auto pin_cgf = getPinCfgs(_pin, PIN_CFG_REQ_PWM);

/* verify configuration are good */
if(pin_cgf[0] == 0) {
return false;
}

timer_channel = GET_CHANNEL(pin_cgf[0]);

_is_agt = IS_PIN_AGT_PWM(pin_cgf[0]);

_pwm_channel = IS_PWM_ON_A(pin_cgf[0]) ? CHANNEL_A : CHANNEL_B;

/* actually configuring PIN function */
R_IOPORT_PinCfg(&g_ioport_ctrl, g_pin_cfg[_pin].pin, (uint32_t) (IOPORT_CFG_PERIPHERAL_PIN | (_is_agt ? IOPORT_PERIPHERAL_AGT : IOPORT_PERIPHERAL_GPT1)));
return true;

}
```

:::

## `PwmOut::suspend()`{.cpp}

内部で`timer.stop()`{.cpp}を呼んでいます。

## `PwmOut::resume()`{.cpp}

内部で`timer.start()`{.cpp}を呼んでいます。

## `PwmOut::end()`{.cpp}

内部で`timer.end()`{.cpp}を呼び、メモリを開放します。また、使用中フラグを`false`{.cpp}にします。

## `FspTimer PwmOut::*get_timer()`{.cpp}

# `FspTimer.h`

## `FspTimer::begin()`{.cpp}

\newpage

## `timer_cfg_t* FspTimer::get_cfg()`{.cpp}

## `GPTimer *gpt_timer;`{.cpp}

### `gpt_extended_cfg_t ext_cfg`{.cpp}

### `gpt_gtior_setting_t`

\newpage

# `r_timer_api.h`

## `timer_cfg_t`

[](arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/api/r_timer_api.h){
.cpp .listingtable from=165 to=189 nocaption=true}

::: rmnote

```cpp
/** User configuration structure, used in open function */
typedef struct st_timer_cfg
{
    timer_mode_t mode;                    ///< Select enumerated value from @ref timer_mode_t

    /* Period in raw timer counts.
     * @note For triangle wave PWM modes, enter the period of half the triangle wave, or half the desired period.
     */
    uint32_t           period_counts;     ///< Period in raw timer counts
    timer_source_div_t source_div;        ///< Source clock divider
    uint32_t           duty_cycle_counts; ///< Duty cycle in counts

    /** Select a channel corresponding to the channel number of the hardware. */
    uint8_t   channel;
    uint8_t   cycle_end_ipl;              ///< Cycle end interrupt priority
    IRQn_Type cycle_end_irq;              ///< Cycle end interrupt

    /** Callback provided when a timer ISR occurs.  Set to NULL for no CPU interrupt. */
    void (* p_callback)(timer_callback_args_t * p_args);

    /** Placeholder for user data.  Passed to the user callback in @ref timer_callback_args_t. */
    void const * p_context;
    void const * p_extend;             ///< Extension parameter for hardware specific settings.
} timer_cfg_t;
```

:::

\newpage

## `r_gpt.h`

### `gpt_extended_cfg_t`

[](arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/instances/r_gpt.h){
.cpp .listingtable from=366 to=398 nocaption=true}

::: rmnote

```cpp
/** GPT extension configures the output pins for GPT. */
typedef struct st_gpt_extended_cfg
{
    gpt_output_pin_t gtioca;           ///< DEPRECATED - Configuration for GPT I/O pin A
    gpt_output_pin_t gtiocb;           ///< DEPRECATED - Configuration for GPT I/O pin B
    gpt_source_t     start_source;     ///< Event sources that trigger the timer to start
    gpt_source_t     stop_source;      ///< Event sources that trigger the timer to stop
    gpt_source_t     clear_source;     ///< Event sources that trigger the timer to clear
    gpt_source_t     capture_a_source; ///< Event sources that trigger capture of GTIOCA
    gpt_source_t     capture_b_source; ///< Event sources that trigger capture of GTIOCB

    /** Event sources that trigger a single up count. If GPT_SOURCE_NONE is selected for both count_up_source
     * and count_down_source, then the timer count source is PCLK.  */
    gpt_source_t count_up_source;

    /** Event sources that trigger a single down count. If GPT_SOURCE_NONE is selected for both count_up_source
     * and count_down_source, then the timer count source is PCLK.  */
    gpt_source_t count_down_source;

    /* Debounce filter for GTIOCxA input signal pin (DEPRECATED). */
    gpt_capture_filter_t capture_filter_gtioca;

    /* Debounce filter for GTIOCxB input signal pin (DEPRECATED). */
    gpt_capture_filter_t capture_filter_gtiocb;

    uint8_t   capture_a_ipl;                      ///< Capture A interrupt priority
    uint8_t   capture_b_ipl;                      ///< Capture B interrupt priority
    IRQn_Type capture_a_irq;                      ///< Capture A interrupt
    IRQn_Type capture_b_irq;                      ///< Capture B interrupt
    gpt_extended_pwm_cfg_t const * p_pwm_cfg;     ///< Advanced PWM features, optional
    gpt_gtior_setting_t            gtior_setting; ///< Custom GTIOR settings used for configuring GTIOCxA and GTIOCxB pins.
} gpt_extended_cfg_t;
```

:::

### `gpt_gtior_setting_t`

[](arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/instances/r_gpt.h){
.cpp .listingtable from=158 to=190 nocaption=true}

### `p_extend`

## `r_ioport_api.h`

### `ioport_cfg_options_t`
