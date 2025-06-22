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
- arduino-cli 1.2.2
    - IDEも内部的に使っていますが、別個にインストールしています。設定ファイル類は共用するようです。
- arduino:renesas_uno 1.4.1
    - ボードマネージャからArduino UNO R4 Boardsをインストールします。
    - 実装コードの引用はGitHubリポジトリ<https://github.com/arduino/ArduinoCore-renesas>の`1.4.1`タグを
      ダウンロードして直接参照しています。

\toc

# PWM.h を読み解いてみる

まず`PWM.h`をインクルードしてみます。多くの作例では、ここで定義されている`PwmOut`オブジェクトを使っています。
IDE上でCtrl+LMBを使いヘッダに飛ぶと、 \
`.../Arduino15/packages/arduino/hardware/renesas_uno/1.4.1/cores/arduino/pwm.h`
となっていました。この部分はターゲットボードごとに変わると思います。

実装ファイルは `.../Arduino15/packages/arduino/hardware/renesas_uno/1.4.1/cores/arduino/pwm.cpp`
です。ヘッダと同じディレクトリにあります。

中身を見ると、はじめの方で早速`Arduino.h` と `FspTimer.h` をインクルードしています。

[PWM.h (先頭部分抜粋)](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable to=8}

インスタンス宣言してコンストラクタを呼び出した時点では、ピンの予約などをするくらいで詳細は定まっていません。
begin関数を呼び出すと、内部で初期設定が行われます。

## PwmOut クラス

## PwmOut::begin()

3種類の実装が用意されています。

### 互換モード：490Hz、50％

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=13 to=14 nocaption=true}

引数なしの`begin()`は490Hz・50％デューティーに設定されます。R3との互換性のためと思われます。

[`PwmOut::begin()` (互換モード・`pwm.cpp`抜粋)](
arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=40 to=59 #lst:pwm_cpp_compatible_mode}

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

[`PwmOut::begin()` (周波数指定モード・`pwm.cpp`抜粋)](
arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=62 to=92 #lst:pwm_cpp_set_pulse_width}

デフォルトでは`raw = false`、`sd = TIMER_SOURCE_DIV_1` です。

### 周波数とデューティー比を設定するモード

[](arduino-core-renesas/cores/arduino/pwm.h){.cpp .listingtable from=25 to=25 nocaption=true}

[`PwmOut::begin()` (周波数指定モード・`pwm.cpp`抜粋)](
arduino-core-renesas/cores/arduino/pwm.cpp){
.cpp .listingtable from=61 to=92 #lst:pwm_cpp_set_freq}

## PwmOut::suspend()

## PwmOut::resume()

## PwmOut::end()

## pwm.cpp

# FspTimer.h

## start()

## stop()

## reset()

## end()

## r_gpt.h

## FspTimer.cpp

## `timer_cfg_t* get_cfg()`

::: rmnote

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
