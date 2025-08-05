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

だいたい要約すると、

- IOピンのレジスタでタイマー機能ににコントロールを渡す
- タイマのレジスタでピン出力変化タイミングを決める
- その他のタイマ設定をする

という手順を踏んでいます。

\newpage

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

## PwmOut クラス定義

[**PwmOut** クラス定義(ヘッダ)](arduino-core-renesas/cores/arduino/pwm.h){
.cpp .listingtable from=8 to=49 #lst:pwmout-class-definition-header}

privateメンバとして`FspTimer`オブジェクト`timer`が使われています。`timer`へのポインタを渡す`get_timer()`関数を通じてアクセスできます。
後で設定を上書きするときは、`get_timer()`経由でオブジェクトを取得してFspTimerクラスの関数やメンバオブジェクトを操作します。

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=37 to=37 nocaption=true}

\newpage

<div class="table" width="[0.1,0.15,0.25,0.35,0.3]">

Table: PwmOutクラスメンバ一覧 {#tbl:pwmout-class-members}

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

## PwmOut::begin()

ペリフェラルの確保と初期設定を行い、PWM信号の出力を[即時開始する]{.underline}関数です。
3種類の実装が用意されています。使用するピンによってAGTかGPTのタイマブロックを割り当て、外のオブジェクトと衝突しないように予約します。
タイマブロックの種類はICレベルでピンごとに固有の割り当てが定義されています。アナログ用のピンには割り当てがありません。デジタルピンだけで動作します。

### 互換モード：490Hz、50％

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=13 to=14 nocaption=true}

引数なしの`begin()`は490Hz・50％デューティーに設定されます。R3との互換性のためと思われます。

[`PwmOut::begin()` (互換モード・`pwm.cpp`抜粋)](
arduino-core-renesas/cores/arduino/pwm.cpp){
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

[`PwmOut::begin()` (周期・パルス幅指定モード・`pwm.cpp`抜粋)](
arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=62 to=92 #lst:pwm_cpp_set_pulse_width}

デフォルトでは`raw = false`、`sd = TIMER_SOURCE_DIV_1` です。

### 周波数とデューティー比を設定するモード

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=25 to=25 nocaption=true}

[`PwmOut::begin()` (周波数・デューティー指定モード・`pwm.cpp`抜粋)](
arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=61 to=92 #lst:pwm_cpp_set_freq}

## PwmOut::suspend()

内部で`timer.stop()`{.cpp}を呼んでいます。

## PwmOut::resume()

内部で`timer.start()`{.cpp}を呼んでいます。

## PwmOut::end()

内部で`timer.end()`{.cpp}を呼び、メモリを開放します。また、使用中フラグを`false`{.cpp}にします。

## FspTimer PwmOut::*get_timer()

# FspTimer.h

## FspTimer::begin()

\newpage

## `timer_cfg_t* FspTimer::get_cfg()`

\newpage

# r_timer_api.h

## timer_cfg_t

[](arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/api/r_timer_api.h){
.cpp .listingtable from=165 to=189 nocaption=true}

::: rmnote

> **Pagebreak(改ページ)挿入**
>
> `\newpage`を任意の場所に書いておくと、Luaフィルタ`docx-pagebreak-toc.lua`が処理して改ページします。
> Docx出力とLaTeX出力が対象です。PDF出力のときも動きますが、`--pdf-engine`の設定によってはうまく動かないかもしれません。

&darr;

:::

\newpage

## r_gpt.h

### gpt_extended_cfg_t

[](arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/instances/r_gpt.h){
.cpp .listingtable from=366 to=398 nocaption=true}

::: rmnote

> **番号なし見出し**
>
> レベル1~5の`.unnumbered`クラスが付与された見出しから番号付けを外します。Docx出力が対象です。
> 予め番号なし見出しスタイルを用意する必要があります。見出しスタイルの設定によって、
> 見出しの前で改ページするかどうかの挙動が変わります。

&darr;

:::

# 番号なし見出し1 {.unnumbered}

## 番号なし見出し2 {.unnumbered}

### 番号なし見出し3 {.unnumbered}

::: rmnote

### gpt_gtior_setting_t

[](arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/instances/r_gpt.h){
.cpp .listingtable from=158 to=190 nocaption=true}

### p_extend
