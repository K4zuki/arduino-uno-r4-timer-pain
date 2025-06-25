# まえがき {-}

## このドキュメントは何 {-}

この本は、Arduino UNO R4 minima (以下 "Arduino R4") の低レベルプログラミング研究のため、手始めに
PWMをいじろうとして色々うんざりした経緯を語るドキュメントです。R4 Wifiは実験していません。

Arduino R4の2ピンを使って相補型PWMを1チャンネル出すまでを試します。
同期した逆相の信号を設定するには最下層のレジスタアクセス実装まで深堀りしないとならないあたりが大変でした。

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
- arduino:renesas_uno 1.4.1
    - ボードマネージャからArduino UNO R4 Boardsをインストールします。
    - 実装コードの引用はGitHubリポジトリ<https://github.com/arduino/ArduinoCore-renesas>の`1.4.1`タグを
      ダウンロードして直接参照しています。

\toc

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

:::rmnote

``` cpp
    bool begin()
    void end();
    bool period(int ms);
    bool pulseWidth(int ms);
    bool period_us(int us);
    bool pulseWidth_us(int us);
    bool period_raw(int period);
    bool pulseWidth_raw(int pulse);
    bool pulse_perc(float duty);
    void suspend();
    void resume();
        FspTimer *get_timer() {return &timer;}
  private:
    bool cfg_pin(int max_index);
    int _pin;
    bool _enabled;
    bool _is_agt;
    TimerPWMChannel_t _pwm_channel;
    uint8_t timer_channel;  
    FspTimer timer;
```

:::

## PwmOut::begin()

3種類の実装が用意されています。使用するピンによってAGTかGPTのタイマブロックを割り当て、外のオブジェクトと衝突しないように予約します。
どの実装も呼び出すと同時に信号出力が開始されます。タイマブロックの種類はICレベルでピンごとに固有の割り当てが定義されています。

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

## pwm.cpp

# FspTimer.h

## FspTimer::start()

## FspTimer::stop()

## FspTimer::reset()

## FspTimer::end()

## r_timer_api.h

## r_gpt.h

## FspTimer.cpp

## `timer_cfg_t* get_cfg()`

## p_extend

::: rmnote

# 参照ファイル全文引用

## pwm.h

[pwm.h](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable #lst:pwm-h-whole-file}

\newpage

## pwm.cpp

[pwm.cpp](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable #lst:pwm-cpp-whole-file}

\newpage

## FspTimer.h

[FspTimer.h](arduino-core-renesas/cores/arduino/FspTimer.h){.cpp .listingtable #lst:fsptimer-h-whole-file}

\newpage

## FspTimer.cpp

[FspTimer.cpp](arduino-core-renesas/cores/arduino/FspTimer.cpp){.cpp .listingtable #lst:fsptimer-cpp-whole-file}

\newpage

## r_timer_api.h

[r_timer_api.h](
arduino-core-renesas/variants/MINIMA/includes/ra/fsp/inc/api/r_timer_api.h){
.cpp .listingtable #lst:r_timer_api-h-whole-file}

\newpage

> **このファイルは何**
>
> このファイルはPandockerがデフォルトで参照する原稿Markdownファイル`TITLE.md`のテンプレートです。
> **素のPandocでは実現できない機能の説明を含みます。**
>
> ------------------------
>
> **Pandoc的Divとrmnote**
>
> Pandocはコロン`:`3個ずつで囲まれた部分をDivとして扱います(fenced divs; <https://pandoc.org/MANUAL.html#divs-and-spans>)。
> 任意のclassやattributeを付与することができるので、
> フィルタのトリガやCSSで色設定をするなどの後処理に使えます。ちなみにこのDivはrmnoteクラスが付与されていて、
> `removalnote.lua`というLuaフィルタの処理対象です。メタデータの設定によって、すべてのrmnoteクラスDivの出力を
> 抑圧することができます。`config.yaml`を編集してください。
>
> **GitHubその他普通のレンダラでは三連コロンを解釈してくれないので、**
> **きれいなレンダリングを保つために前後に改行を入れておくことをおすすめします。**
>
> ---

> **TOC(目次)挿入**
>
> `\toc`を任意の場所に書いておくと、Luaフィルタ`docx-pagebreak-toc.lua`がその場所に目次を生成します。
> 現在のところ、Docx出力のみが対象です。目次の前は必ず改ページします。目次のあとは改ページしません。
> `toc-title`メタデータによって目次の見出しを変更できます。`config.yaml`を編集してください。

[](markdown/config.yaml){.listingtable from=18 to=20}

&darr;

:::

\toc

::: rmnote

> **Pagebreak(改ページ)挿入**
>
> `\newpage`を任意の場所に書いておくと、Luaフィルタ`docx-pagebreak-toc.lua`が処理して改ページします。
> Docx出力とLaTeX出力が対象です。PDF出力のときも動きますが、`--pdf-engine`の設定によってはうまく動かないかもしれません。

&darr;

:::

\newpage

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

> **下線**
>
> 任意のSpanに`underline`クラスを付与すると下線がつきます。Docx出力に加えLaTeX出力(*)が対象です。
>
> (*): LaTeX出力では`tex-underline.lua`が処理します。
>
> 例：`**下線**`
>
> ![QR](images/QRcode.png){#fig:qr-code width=120mm}

:::
