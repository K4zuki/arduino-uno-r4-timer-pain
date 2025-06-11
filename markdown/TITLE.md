# まえがき {-}

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
