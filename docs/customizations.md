# Nickey44A-PAD: このフォークのカスタマイズ

nixiy/zmk-config-nickey44a-pad をフォークし、作者の e3004b7（2026-10-19）の
上に加えた変更をまとめる。キーマップは tomkey のキーマップを元にしている。
変更ごとの詳しい理由は、各コミットのメッセージに書いてある。

## ビルドと書き込み

`scripts/build.sh` は、GitHub Actions と同じ ZMK v0.3（Zephyr 3.5）の
Dockerイメージを使って、手元でビルドする。`.uf2` とログは `build/` に出る。

```sh
scripts/build.sh nickey44a_r   # 右手
scripts/build.sh nickey44a_l   # 左手
scripts/build.sh               # build.yaml のすべて
```

書き込むときは、XIAOのリセットボタンを2回押すか、FUNCTIONレイヤーの
Bootキー（下の「Reset と Boot」）でブートモードにし、表示された
`XIAO-BOOT` に `.uf2` をコピーする。

`build.yaml` の `snippet` や `cmake-args` を消したときは、ビルド用の
Dockerボリュームにある古い設定が残るので、そのターゲットのビルド
ディレクトリを消してから作り直す。

```sh
docker run --rm -v nickey44a-zmk-ws:/ws zmkfirmware/zmk-build-arm:3.5 rm -rf /ws/build/nickey44a_r
```

ZMK Studioは使わないので、右手のビルドから外した（キーマップはgitで
管理する）。

右手がcentral。キーマップもトラックパッドの処理も右手で行うため、
キーマップやジェスチャを変えたときは右手だけ書き込めばよい。左手は、
左手の `.conf`・overlay や、左右両方に入る機能（ソフトオフなど）を
変えたときに書き込む。左手は2026-11-21（e8feeed）から、作者のFWではなく
このリポジトリのFWで動いている。

## キーマップ

tomkeyのレイヤー、コンボ、マクロ、behaviorを移植した。今のキーマップは
READMEの図（`keymap-drawer/nickey44a.svg`）で見られる。

### レイヤー

| 番号 | 名前 | 内容 |
|---|---|---|
| 0 | MAC | Mac用の基本レイヤー |
| 1 | MOUSE_SLOW | すべて透過。右パッドを遅くし、2本指の左右スワイプをタブ移動にする |
| 2 | WIN | Windows用。MACに重ねて、左外側の中と下をWinとCtrlにする |
| 3 | WIN_JIS | Windows（JIS）用。WINと同じ入れ替えと、JISで位置が違う記号キー |
| 4 | IPAD | iPadの目印。中身は空 |
| 5 | NUM | 数字と記号。左親指のLANG2を長押し |
| 6 | FUNCTION | Fキー、BTプロファイル、OSの切り替え、電池残量、電源オフ。`;` を長押し |
| 7 | MOUSE | マウス操作。右親指のLANG1を長押し |
| 8 | AUTO_MOUSE | 右パッドに触れると有効になる |

WIN、WIN_JIS、IPADは、つないでいる機器の種類を表す目印で、FUNCTIONの
BTプロファイルのキーで切り替わる（BT0〜2…Mac、ただしBT2はiPad、
BT3…Windows、BT4…Windows JIS）。NUMより下に置いて、NUMのキーが
WIN_JISのキーに負けないようにしている。

番号は右手のoverlay（スワイプの切り替え）と、`nickey44a_r.conf` の
LEDのレイヤーの色でも使っているので、変えるときは合わせて直す。

### Windows JISの記号

JIS配列のWindowsでは、同じキーでも出る記号が違う（`@` や `[` など）。
NUMの記号のうち位置が違う11個と、コンボの `` ` `` と `=` は、
WIN_JISが有効なときにJIS用のキーを送る。以前はNUMの写しのNUM_JISを
別に持っていたが、レイヤーで送るキーを選ぶ仕組み（下の「MOUSEレイヤーの
Mac / Windows切り替え」と同じ）にまとめた。NUMを直せば両方に効く。

### tomkeyから動かしたキー

Nickey44A-PADには、tomkeyにある内側の列と最下段がなく、代わりに外側の列が
ある。Bの右（30）とNの左（31）はトラックパッドの下でスイッチがないため
`&none` にしてあり、使えるキーは42個。

| キー | Nickey44A-PADでの位置 |
|---|---|
| Opt / Ctrl / Cmd | 左外側の上 / 中 / 下 |
| `;`（長押しでFUNCTION） | 右外側の上（Pの右、11） |
| `-`（長押しでShift） | 右親指の外側（43） |
| `'` | 右ホーム段の小指（Lの右、22） |
| Del | 右下段の小指（36） |
| F2 / F7 | 右外側の中 / 下（23 / 37） |
| Ctrl+Space | コンボ F + G |
| `` ` `` | コンボ J + K |

- tomkeyのSCROLLレイヤーとエンコーダーは削除した。スクロールは
  両パッドの2本指で行う。

### MOUSEレイヤーのMac / Windows / iPad切り替え

MOUSEレイヤー（右親指のLANG1を長押し）のキーのうち、機器によって
ショートカットが違うものは、どれでも同じ操作になるように、WIN・
WIN_JIS・IPADのどれが有効かで送るキーを変える。

| キー | 操作 | Mac | Windows | iPad |
|---|---|---|---|---|
| W / R | 文書の先頭 / 末尾 | `Cmd+↑` / `Cmd+↓` | `Ctrl+Home` / `Ctrl+End` | Macと同じ |
| C / V | 左 / 右の仮想デスクトップ | `Ctrl+←` / `Ctrl+→` | `Win+Ctrl+←` / `Win+Ctrl+→` | ホーム画面（`Cmd+H`） / 直前のアプリ（`Cmd+Tab`） |
| Q / A | ページの先頭 / 末尾 | `Home` / `End` | 同じ | 同じ |
| U / I | 左 / 右のタブ | `Ctrl+Shift+Tab` / `Ctrl+Tab` | 同じ | 同じ |

コンボのコメントアウト（D + F）も、Macでは `Cmd+/`、Windowsでは
`Ctrl+/` を送る。

Q / Aは、ブラウザではどちらでもページの先頭 / 末尾に移る。文字の入力中は、
Windowsでは行頭 / 行末への移動になる。

切り替えは、mod-morphのレイヤー版として書いた `zmk,behavior-layer-morph`
（`src/behavior_layer_morph.c`）で行う。レイヤーは増やしていない。

### Reset と Boot

FUNCTIONレイヤーのResetとBootは、押したキーがある側にだけ効く（ZMKの
仕様）。左右それぞれに置いてある。

| | Reset（再起動） | Boot（ブートモード） |
|---|---|---|
| 左手に効く | R | T |
| 右手に効く | M | N |

右手のResetは、F10の隣（右外側の中）から押し間違えにくいMへ移した。
FUNCTIONを押す `;`（11）と同じ右手だけで押せる。

### キーマップの編集

`config/nickey44a.keymap` を直接編集する。編集したら、keymap-drawerで
図を作り直す。

```sh
uvx --from keymap-drawer keymap -c keymap_drawer.config.yaml parse -z config/nickey44a.keymap -o keymap-drawer/nickey44a.yaml
uvx --from keymap-drawer keymap -c keymap_drawer.config.yaml draw -j config/nickey44a.json keymap-drawer/nickey44a.yaml -o keymap-drawer/nickey44a.svg
```

独自のbehaviorを図でどう表示するかは、`keymap_drawer.config.yaml` の
`raw_binding_map` で決める。

## トラックパッド

ジェスチャの一覧と処理の流れは [dual-tps43.md](dual-tps43.md) にある。
このフォークで変えたのは次の点。

- 左右とも、ビルドガイドどおり切り欠きを右にして取り付ける。向きは
  `boards/shields/nickey44a/nickey44a_r.overlay` の先頭にある
  `RIGHT_PAD_MOUNT` と `LEFT_PAD_MOUNT` で切り替えられる。左パッドの補正も
  右手で行うので、向きを変えたときは右手だけ書き込めばよい。
- スクロールは、ホストのナチュラルスクロールをオンにしたとき（Windowsでは
  マウスのスクロール方向を逆にしたとき）に、中身が指について動く向き。
- 右パッドに触れると、AUTO_MOUSEに800 ms入る。その間は、右親指のLANG1と
  Enterが左クリック、BackSpaceが右クリック、Spaceの長押しがMOUSE_SLOWになる。
- 左パッドは、2本指の左右で水平スクロール、上下でタブ移動（指を上へで
  右のタブ）。長押しで左ボタンを押したままにでき、ドラッグに使う。

右パッドの2本指の左右スワイプは、レイヤーによって送るキーが変わる。

| そのときの状態 | Mac | Windows（WIN・WIN_JIS） | iPad |
|---|---|---|---|
| 通常 | 戻る / 進む（`Cmd+[` / `Cmd+]`） | Browser Back / Forward | Macと同じ |
| MOUSE_SLOW（パッドに触れてSpaceを長押し） | タブ移動（`Ctrl+Shift+Tab` / `Ctrl+Tab`） | Macと同じ | Macと同じ |
| NUM（LANG2を長押し） | 仮想デスクトップ（`Ctrl+←` / `Ctrl+→`） | 仮想デスクトップ（`Win+Ctrl+←` / `Win+Ctrl+→`） | アプリスイッチャー（下） |

iPadのアプリスイッチャー：LANG2を押しながら2本指で左右に動かすと、
`Cmd+Tab` でアプリスイッチャーが開き、指を置いている間は `Cmd` を
押したままにする。そのまま左右に動かすと選択が1つずつ動き（指を左へで右、
右へで左）、指を離すと選んだアプリに切り替わる。`Cmd+Tab` を1回押すだけ
だと直前のアプリとの行き来になるが、これならもっと前のアプリにも行ける。

向きはMacBookと同じで、指を右へで「戻る」、指を左へで右のタブ・右の
デスクトップに移る。macOSはBrowser Back / Forwardのキーを無視するので、
Macでは `Cmd+[` / `Cmd+]` を送る。

2つ目のレイヤーでも送るキーを変えられるように、タッチスワイプの
モジュール（`modules/zmk-input-processor-touch-swipe`）に
`second-override-layers` を追加した。iPadのアプリスイッチャーのために、
キーを押したままにして続けて送る `second-override-hold-keycode` と
`second-override-repeat-threshold` も追加した。

## 電源

### 電池残量の表示

`;`（FUNCTION）を押したままQを押すと、右手のLEDに、右手、左手の順で
電池残量を2秒ずつ色で表示する。

| 色 | 残量 |
|---|---|
| 緑 | 60%以上 |
| 黄 | 30〜59% |
| 赤 | 30%未満 |
| マゼンタ | 取得できない（左手とつながっていないときなど） |

- 電源を入れたときに光る色も、同じ基準。
- 左手のLEDにも、同時に左手の残量が出る。
- `;` を押したままだと、2つの表示の間にFUNCTIONレイヤーの色（水色）が
  挟まる。Qを押したら `;` も離すと見やすい。
- 残量は電池の電圧から推定していて、単4のNi-MH電池向けに合わせてある
  （`boards/shields/nickey44a/nickey44a.dtsi`）。Ni-MHは使っている間の
  電圧の変化が小さいので、目安として見る。
- しきい値は `config/nickey44a_r.conf` と `config/nickey44a_l.conf` の
  `CONFIG_RGBLED_WIDGET_BATTERY_LEVEL_*` で、左右同じ値にしておく。

### ソフトオフ

- **切る**：`;`（FUNCTION）を押したまま、Qの左隣（Opt、位置0）を
  1秒以上押してから離す。左右とも切れ、LEDも消える。
- **起こす**：左右それぞれの電源スイッチを、切ってから入れ直す。
  キーを押しても起きない。
- XIAOのリセットボタンでも起きる。ただしXIAOの小さな部品で、2回続けて
  押すとブートモードに入るので、ふだんは電源スイッチを使う。

最初はQの左隣とPの右のキーで起きるようにしていた。持ち運び用のケースの
中で押されて起きてしまうため、キーでは起きないようにした。ソフトオフ中は
チップが止まっていて、ダブルタップや組み合わせを判定できない。

ZMK標準の `&soft_off` は、左手を押した瞬間に切る。キーが左手にあると、
左手が先に切れて「離した」が右手に届かない。そこで、左右とも1秒押して
離したときに切れる `soft_off_hold`（ノード名 `so_hold`）を定義した。
ノード名は、左右の間で送れる8文字以内にしてある。

LEDは、そのときのレイヤーの色で光ったまま残っていた。ウィジェットは
自動スリープのときにしかLEDを消さず、チップは電源オフ中も出力ピンの
状態を保つためである。電源を落とす直前にLEDを消す処理を
`src/soft_off_leds.c` に足した。

### 自動スリープ

作者の設定のまま。2時間操作しないと深いスリープに入り、キーを押すと起きる。

### ソフトオフと電源スイッチの使い分け

- 電源スイッチで切ると、電池が回路から切り離されて電池は減らない。
  ソフトオフでも、チップ、電池電圧の測定回路、電源回路、パッドの待機で
  わずかに電流が流れる（実測はしていない）。何日も使わないときは
  電源スイッチで切る。
- 起こすには電源スイッチを使うので、切るのもスイッチで十分。ソフトオフは、
  キー操作1回で左右とも切りたいときに使う。
- BTプロファイルなどの設定は、変えてから1分後に保存される。それより前に
  切ると、その変更は保存されない。どちらの切り方でも同じで、設定が
  壊れることはない。

## 実機での確認

2026-11-21に確認した。

- トラックパッドの向きとジェスチャ（左右とも切り欠き右）。
- 左パッドの長押しによる左ボタンの保持と解除。
- ソフトオフで左右とも切れること。ソフトオフ中に右手のLEDが消えていること。
- 電池残量の表示。

まだ確認していないこと。

- NUMを押したままのスワイプで、仮想デスクトップを移れること。
- iPadモード（BT2）：アプリスイッチャー、MOUSEのC / V、LEDが消えたままのこと。
- Windows JISで、NUMの記号とコンボの `` ` `` `=` が正しく出ること。
