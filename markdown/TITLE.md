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
- Renesas FSP 4.0.0
    - FSPライブラリの実装を確かめるためにGitHubリポジトリ<https://github.com/renesas/fsp>から
      ソースコード一式をダウンロードしました。上記ボードと同じv4.0.0を参照しています。

RA4M1のデータシート「ハードウェアマニュアル」(たぶん日立語)は **Rev.1.10 Sep 29, 2023**を参照しました。

\toc

# 先人の成果を調べてみる

## お手本実装

*小倉 キャッスル 一馬* 氏のブログに筆者がやろうとしていることがほぼ全部書かれています。以下の記事と続編をたどると、
ArduinoのAPIや関数をほぼ使わず、レジスタの直接操作をして相補PWMにしています。

- [「任意のポートでPWM出力させたい」 (2023-11-14)](https://ameblo.jp/ogura-castle/entry-12828562536.html)
- [「任意のポートでPWM出力させるためのレジスタ制御」 (2023-11-15)](https://ameblo.jp/ogura-castle/entry-12828565434.html)

２つ目の記事に載っているコードを下に示します。1kHz、デューティー比25%の矩形波がD0/D1から出力されます。

[参考にしたコード](data/reference.cpp){.listingtable .cpp #lst:reference-code}

キャッスル氏のコードをだいたい要約すると、

- FspTimerライブラリを使ってタイマーペリフェラルを起動 (`begin()`{.cpp})
- タイマーの諸設定を行う (`open()`{.cpp})
- IOピンのレジスタでタイマー機能ににコントロールを渡す (`R_PFS->...`{.cpp})
- タイマーのレジスタでピン出力変化タイミングを決める (`R_GPT4->...`{.cpp})
- タイマー起動（カウント開始、信号出力開始）(`start()`{.cpp})

という手順を踏んでいます。
注意点として、このコード例では、AB相間のデッドタイムは設定されません。
また、デューティー比を変えるときに両方ともHになる期間が発生する可能性があります。

## 今回の計画

- キャッスル式をお手本にして、同じ結果を目指します。D0/D1ピンに、互いに逆相のノコギリ波PWM信号を出すことがゴールです。
  D0/D1にはGPT4が割り当てられています。[@tbl:arduino-r4-pin-assignments]を参考にしてください。
- 次に、デッドタイムつきで三角波PWMを同じく逆相で出します。A相のパルス幅を更新すると、
  B相はハードウェアが計算して自動的に更新してくれます。

また、FspTimerの使い方は以下のブログページを参考にしました。

- [「Arduino UNO R4のFspTimerライブラリの使い方」 （2023-08-11 ）](https://qiita.com/yasuhiro-k/items/93efb640aa12f3db9086)
- [「Arduino UNO R4 でタイマー割り込みを使う」 （2024.10.21）](https://workshop.aaa-plaza.net/archives/1658)

<div class="table" width="[0.15,0.15,0.15,0.05,0.15,0.15,0.15]">

Table: Arduino UNO R4 Minima ピンアサイン一覧 {#tbl:arduino-r4-pin-assignments}

| GPT channel |  RA4M1   | Arduino |   | Arduino  |  RA4M1   | GPT channel |
|:-----------:|:--------:|:-------:|---|:--------:|:--------:|:-----------:|
|      -      |    -     |    -    |   |   SCL    | **P100** |   **5B**    |
|      -      |    -     |    -    |   |   SDA    | **P101** |   **5A**    |
|      -      |    -     |    -    |   |   AREF   |   AREF   |      -      |
|      -      |    -     |    -    |   |   GND    |   GND    |      -      |
|      -      |    MD    |   NC    |   | D13/SCK  |   P111   |     3A      |
|      -      |    5V    |  IOREF  |   | D12/MISO |   P110   |     1B      |
|      -      |  /RESET  |  RESET  |   | D11/MOSI |   P109   |     1A      |
|      -      |    -     |   3V3   |   | D10/CS~  |   P112   |     3B      |
|      -      |    5V    |   5V    |   |   D9~    |   P303   |     7B      |
|      -      |   GND    |   GND   |   |   D8~    |   P304   |     7A      |
|      -      |   GND    |   GND   |   |   ---    |   ---    |     ---     |
|      -      |   VIN    |   VIN   |   |    D7    |   P107   |     0A      |
|     ---     |   ---    |   ---   |   |    D6    |   P106   |     0B      |
|      -      |   P014   |   A0    |   |   D5~    |   P102   |     2B      |
|      -      |   P000   |   A1    |   |   D4~    |   P103   |     2A      |
|      -      |   P001   |   A2    |   |    D3    |   P104   |     1B      |
|      -      |   P002   |   A3    |   |   D2~    |   P105   |     1A      |
|   **5A**    | **P101** |   A4    |   |  D1/TX   |   P302   |     4A      |
|   **5B**    | **P100** |   A5    |   |  D0/RX   |   P301   |     4B      |

</div>

# PWM.h を読み解いてみる

## `PWM.h` &rarr; `pwm.h`

まず`PWM.h`をインクルードしてみます。多くの作例では、ここで定義されている`PwmOut`オブジェクトを使っています。
IDE上でCtrl+LMBを使いヘッダに飛ぶと、 \
`.../Arduino15/packages/arduino/hardware/renesas_uno/1.5.0/cores/arduino/pwm.h`
となっていました。この部分はターゲットボードごとに変わると思います。実装ファイルはヘッダと同じディレクトリにある `.../pwm.cpp`です。

ヘッダの中身を見ると、はじめの方で早速`Arduino.h` と `FspTimer.h` をインクルードしています。

[PWM.h (先頭部分抜粋)](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable to=8}

インスタンス宣言してコンストラクタを呼び出した時点では、ピン番号を内部に保持するだけで何もしません。
begin関数を呼び出すと、内部で初期設定が行われます。

## `PwmOut` クラス定義

[`PwmOut` クラス定義(ヘッダ)](arduino-core-renesas/cores/arduino/pwm.h){
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
3種類の実装が用意されています。どの実装でも、ピンの制御をGPTのタイマブロックに割り当て[^seems-no-agt]、
対応するタイマーをノコギリ波PWMモードに初期化します。

タイマーブロックの種類はICレベルでピンごとに固有の割り当てが定義されています。アナログ用のピン(A0~A3)には割り当てがありません。
デジタルピン(D0~D13)とI2C兼用ピン(A4,A5)だけが対象です。

[^seems-no-agt]: ソースを見るとAGTとの切り分けが各所に入っていますが、少なくともUNO R4では、AGTが割り当てられる可能性はなさそうです。

::: rmnote
<!--

### 互換モード：490Hz、50％

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=13 to=14 nocaption=true}

引数なしの`begin()`は490Hz・50％デューティーに設定されます。R3との互換性のためと思われます。

[`PwmOut::begin()` (互換モード・`pwm.cpp`抜粋)](arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=40 to=59 #lst:pwm_cpp_compatible_mode}

内部で`timer.begin_pwm()`{.cpp}を呼び出しています。
`begin_pwm`の内部では、まず`timer.begin()`と`timer.add_pwm_extended_cfg()`に
よってPWMモード用の設定を用意して、`timer.enable_pwm_channel()`でIOピンを出力モードにし、
最終的に`timer.open()`と`timer.begin()`を呼び出して信号出力が始まります。

### 周期・パルス幅を設定できるモード

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=15 to=24 nocaption=true}

`raw`に`false`を与えたときは、`period_width`と`pulse_width`はマイクロ秒単位になります。
`raw`を`true`にすると、比較レジスタに即値が取り込まれます。このとき`sd`の値によって実時間が変わってしまいます。

`sd`はタイマクロック回路に与える周波数を決めるための分周比を与えます。1/1（直結・48MHz）から1/1024（46.875kHz）までの10段階が
用意されていますが、実際に受け入れられるのは1/4/16/64/256/1024の６種類です。
タイマーはこの分周されたクロックのパルスをカウントしてPWMなどの機能を実現します。
`timer_source_div_t`型は`r_timer_api.h`で定義されていますが、上記の通り一部だけ有効です。

[`timer_source_div_t`型定義 (`r_timer_api.h`抜粋)](
arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/api/r_timer_api.h){
.cpp .listingtable from=130 to=145}

[`PwmOut::begin()`{.cpp} (周期・パルス幅指定モード・`pwm.cpp`抜粋)](arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=62 to=92 #lst:pwm_cpp_set_pulse_width}

デフォルトでは`raw = false`、`sd = TIMER_SOURCE_DIV_1` です。

-->
:::

### 周波数とデューティー比を設定するモード

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=25 to=25 nocaption=true}

スイッチング周波数とデューティー比を指定するモードです。周波数はメインクロックまで指定できると思われますが、
筆者は実験していません。現実的には480kHzが上限と思われます[^duty-vs-clock]。

[^duty-vs-clock]: メインが48MHzなので、スイッチングが480kHzのときデューティー1％が1クロックになります。
スイッチングを1MHzにすると1周期が48クロック（デューティー1％あたり0.5クロックくらい）しかありません。

[`PwmOut::begin()`{.cpp} (周波数・デューティー指定モード・`pwm.cpp`抜粋)]( arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=93 to=114 #lst:pwm_cpp_set_freq}

## `PwmOut::cfg_pin()`{.cpp} (プライベート関数)

IOピンがPWMに使えるかを検査し、~~謎の~~FSPライブラリ関数`R_IOPORT_PinCfg()`を使って設定を行います。
PWMライブラリを使わずとも、この関数に適切な引数を渡せばIOピンは設定できそうです。

[`PwmOut::cfg_pin()` (`pwm.cpp`抜粋)](arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=15 to=38 #lst:cfg-pin-whole-code}

### `R_IOPORT_PinCfg()`{.cpp}

ルネサスの"FSPライブラリ"内にある関数です。お手本コードで直接レジスタ操作していた部分の置き換えに使えます。
[ヘルプページも存在します](https://renesas.github.io/fsp/group___i_o_p_o_r_t.html#gab518fc544fe2b59722e30bd0a28ef430)
が、Arduinoが使っているものとはバージョンが異なっている[^v4-0-0]ので、残念ながら助けにはなりません。
FSPのソースコードをリポジトリから入手して内容を見るのがベストです。

[^v4-0-0]: ヘッダ情報によると、FSPバージョン`v4.0.0`を使っています。

[FSPバージョン情報](arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/fsp_version.h){
.listingtable .cpp from=56 to=57}

この関数の引数一覧を[@tbl:r-ioport-pincfg-param-list]に示します。

どうやら最初の引数`p_ctrl`は互換性のためにあるらしく、`cfg_pin()`では共用の変数を使用しています。次の`pin`は
専用の数値型ですが、Arduino形式から変換したものを渡せばOKです。最後の`cfg`は、渡す値がそのままPmnPFSレジスタの構造に当てはまります。
`cfg_pin()`ではAGT・GPTの切り替え判定をするためややこしく見えますが、`IOPORT_CFG_PERIPHERAL_PIN`{.cpp}と
`IOPORT_PERIPHERAL_GPT1`{.cpp}の論理和を`uint32_t`{.cpp}にキャストして渡せば大丈夫です。

<div class="table" width="[0.13,0.3,0.2,0.37]">

Table: `R_IOPORT_PinCfg()`{.cpp} 引数一覧 {#tbl:r-ioport-pincfg-param-list}

| Parameter      | Type                          | Purpose                         | Note                                                                   |
|----------------|-------------------------------|---------------------------------|------------------------------------------------------------------------|
| `p_ctrl`{.cpp} | `ioport_ctrl_t * const`{.cpp} | Unused                          | 共用の固定値を使用                                                              |
| `pin`{.cpp}    | `bsp_io_port_pin_t`{.cpp}     | Pin identifier                  | `g_pin_cfg[_pin].pin`{.cpp}; `_pin`{.cpp}には`D13`などArduino形式のピン番号を指定できる |
| `cfg`{.cpp}    | `uint32_t`{.cpp}              | Directly update PmnPFS register | `(uint32_t) (IOPORT_CFG_PERIPHERAL_PIN｜IOPORT_PERIPHERAL_GPT1)`{.cpp}  |

</div>

::: rmnote
<!--

```cpp
IOPORT_CFG_PERIPHERAL_PIN        = 0x00010000  ///< Enables pin to operate as a peripheral pin

#define IOPORT_PRV_PFS_PSEL_OFFSET    (24)

/** Pin will function as an AGT peripheral pin */
IOPORT_PERIPHERAL_AGT = (0x01UL << IOPORT_PRV_PFS_PSEL_OFFSET),

/** Pin will function as a GPT peripheral pin */
IOPORT_PERIPHERAL_GPT1 = (0x03UL << IOPORT_PRV_PFS_PSEL_OFFSET),
```

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

## `PwmOut::suspend()`{.cpp}

内部で`timer.stop()`{.cpp}を呼んでいます。

## `PwmOut::resume()`{.cpp}

内部で`timer.start()`{.cpp}を呼んでいます。

## `PwmOut::end()`{.cpp}

内部で`timer.end()`{.cpp}を呼び、メモリを開放します。また、使用中フラグを`false`{.cpp}にします。

## `FspTimer PwmOut::*get_timer()`{.cpp}

-->
:::

# `FspTimer.h`

## `FspTimer::begin()`{.cpp}

::: rmnote

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

\newpage

## `r_gpt.h`

### `gpt_extended_cfg_t`

[](arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/instances/r_gpt.h){
.cpp .listingtable from=366 to=398 nocaption=true}

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

### `gpt_gtior_setting_t`

[](arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/instances/r_gpt.h){
.cpp .listingtable from=158 to=190 nocaption=true}

### `p_extend`

## `r_ioport_api.h`

### `ioport_cfg_options_t`

:::

# あとがき {-}

今回も前日印刷^TM^です[^genko]。主な遅延要因はバトルフィールド６のオープンベータとタルコフのワイプです。
BF3リメイクと言った感じでとても良かった。2週連続でベータ期間で、2回目はコミケと重なってしまうのであまりできないかも
しれないですが、タイムゾーンの差を活かして月曜日まで楽しもうと思います。

タルコフの進捗は、グランドゼロマップが嫌すぎて、プラパーの最初のタスクで詰まってます。メカニックとスキヤーも詰まってます。
かろうじてセラピは進んだけど、イエガー未開放奴です。
本編がつらいからってアリーナでしばらく遊んでたら勝手にレベル14まで進んでしまった。タスクは進んでないけど取引額
の条件は満たしているのでセラピッピとピスキはLL2になっているでしょう。
ハードコアワイプ初日のタスクなしモード、悪くないと思ったんですが、コミュニティは好きじゃないみたいですね。

Arduino R4のデバッガにtuboLinkII（現在は入手不可）を使おうとして一時沼りましたが、過去の自分がDropboxに残していた
ファームウェアのバックアップを書き込んで事なきを得ました。やっててよかったｴｪｪｪｪﾝﾍﾞｯﾄﾞ! まあMacとの相性が悪かったのかなー

[^genko]: このあとがきは木曜日朝7時頃に書かれました。本編はまだ書き終わっていません。

- ![](images/QRcode.png){width=80mm} &larr;原稿はこちらから
