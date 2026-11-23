# Nickey44A-PAD: 左右TPS43

右central／左peripheral。左右とも以下の配線・アドレスを使用する。

| 信号・設定 | 左手のTPS43 | 右手のTPS43 |
|---|---|---|
| SDA | P0.09 | P0.09 |
| SCL | P0.10 | P0.10 |
| RDY | P1.10 | P1.10 |
| NRST | P0.16 | P0.16 |
| I2Cアドレス | `0x74` | `0x74` |

各MCUに独立したI2Cバスがあるため、アドレスは同じでよい。
westのrevision、ZMK本体、Azoteqドライバーは変更しない。

## 取り付けの向き

TPS43の切り欠きの向きは、`boards/shields/nickey44a/nickey44a_r.overlay`
の先頭で左右別に選ぶ。

```c
#define RIGHT_PAD_MOUNT PAD_NOTCH_RIGHT
#define LEFT_PAD_MOUNT PAD_NOTCH_RIGHT
```

| 値 | 向き |
|---|---|
| `PAD_NOTCH_RIGHT` | 切り欠きが右。ビルドガイドの向き。作者が実測した補正（e3004b7）を使う |
| `PAD_NOTCH_LEFT` | 切り欠きが左。切り欠き右から180度回した向きで、FFCがねじれない。上下・左右とも逆になる |

作者によると、左のケースは切り欠き左だと改修が必要。右のケースは切り欠き左でも
収まるが、XIAOとぎりぎりになる。

左パッドの補正も右（central）側で行うため、向きを変えたときは右手側の
FWだけをビルドして書き込む。左手側のFWと`nickey44a_l.overlay`は
どちらの向きでも同じ。下のジェスチャの割り当てと方向も、どちらの向きでも同じ。

## ジェスチャの割り当て

現在の設定上の割り当て。左手・右手は、それぞれのTPS43を指す。
スクロールの方向は、ホストのナチュラルスクロールがオン（Mac、Windowsは
マウスのスクロール方向を逆にした状態）で、中身が指について動く向き。
左右とも切り欠き右で付ける。2026-11-21に、左右とも切り欠き右で付けた
実機で、この表の方向どおりに動くことを確認した。

| ジェスチャ・操作 | 左手のTPS43 | 右手のTPS43 |
|---|---|---|
| 1本指で移動 | - | カーソル移動 |
| 1本指でタップ | - | 左クリック |
| 1本指で長押し・保持 | 左ボタンを押したまま保持。指を離すと解除 | 長押しによる左ボタン保持なし |
| 2本指でタップ | 中クリック | 右クリック |
| 2本指で上下に動かす | 上へ：右のタブ／下へ：左のタブ | 縦スクロール |
| 2本指で左右に動かす | 右スワイプ：左の仮想デスクトップ／左スワイプ：右の仮想デスクトップ（iPadではなし） | 水平スクロール |
| 2本指でピンチ | - | 広げる：拡大／縮める：縮小（キーを段階的に送る） |
| 縦スクロール後に指を離す | 慣性処理なし | 慣性スクロール |
| Layer 1で1本指移動 | - | Snipe（移動量を通常の2/3に抑える） |
| Layer 1（Space長押し）で2本指左右スワイプ | 同上 | 右スワイプ：戻る／左スワイプ：進む |
| NUM（左親指のLANG2長押し）で2本指左右スワイプ | 同上 | アプリスイッチャー（全機器） |

ドラッグは、右手で対象へカーソルを移動 → 左手で長押しして保持 →
右手でカーソルを動かす → 左手の指を離してドロップ、の順で操作する。
左手の指を動かしてもカーソル座標には影響しない。

左手のタブ移動は、上へのスワイプで`Ctrl+Tab`、下へのスワイプで
`Ctrl+Shift+Tab`を送信する（リストを下へ送るのと同じ向き）。
ブラウザにフォーカスがある状態で使用する。1回のタッチにつき
最大1回切り替え、再度切り替えるには指を離してからスワイプする。

右手の2本指の左右は、通常は水平スクロールにする（2026-11-24から。
それまでは戻る／進むだった）。swipe processorに通常時のキー
（`left-keycode`／`right-keycode`）を設定しないと、上書きのレイヤーが
有効でない間は左右の動きをそのまま水平スクロールとして通す。向きは左手と
同じで、swipe processorの後ろで`tps43_scroll_direction`がREL_HWHEELを
反転する。タブ移動は左手だけで行う。

右手の戻る／進むは、Layer 1（AUTO_MOUSE中にSpaceを長押し）が有効な間の
左右スワイプで、Macでは`Cmd+[`／`Cmd+]`を送信する（Safari、Chrome、
Finderの戻る／進む。macOSはBrowser Back／Forwardのキーを無視する）。
向きはMacBookと同じで、指を右へで戻る。
WINのレイヤー（2。WindowsのプロファイルではJISでも有効）が有効な間は、
Browser Back／Forwardを送信する。
右のinput-listenerの`windows_layers`が、`tps43_touch_swipe`の代わりに
`tps43_touch_swipe_win`を使う。iPadはMacと同じキーなので、右手には
iPad用の設定はない。

NUMのレイヤー（5。左親指のLANG2を長押し）が有効な間の右手の左右スワイプは、
全機器でアプリスイッチャーにする（2026-11-24から。それまではMac・Windowsで
仮想デスクトップ、iPadでアプリスイッチャーだった）。最初のスワイプで
`Tab`を送り、指を離すまで修飾キー（Mac・iPadは`Cmd`、Windowsは`Alt`）を
押したままにする。同じタッチで続けて動かすと、24ずつ動くたびに選択が1つ
動く（指を左へで`Tab`、右へで`Shift+Tab`）。指を離すと修飾キーを離し、
選んだアプリに切り替わる。Layer 1も有効なときは戻る／進むを優先する。
swipe processorの`second-override-layers`、`second-override-hold-keycode`、
`second-override-repeat-threshold`で設定する。

仮想デスクトップ移動は左手の2本指の左右で行う。Macでは`Ctrl+←`／`Ctrl+→`、
WINのレイヤーが有効な間は`Win+Ctrl+←`／`Win+Ctrl+→`を送信する。指を右へで
左のデスクトップ（MacBookの3本指と同じ向き）。iPadには仮想デスクトップが
ないので、IPADのレイヤーが有効な間は何もしない。左手には横スクロールはない。

スワイプ成立時のレイヤー状態で送信キーを選ぶ。1回のタッチでの発火状態は
レイヤーを切り替えても維持するため、指を離すまでは再発火しない。
スワイプしたタッチは、指を離すまで水平スクロールも出さない。

## イベント経路

```text
左TPS43 → 左input-splitの標準processors → BLE split (reg=0)
        → 右input-split proxy (reg=0) → 左専用input-listenerのorientation/tab swipe/desktop swipe（Mac用かWindows用、iPadはなし）/上下軸の破棄 → 共通HID
右TPS43 → 右input-listener → Snipe → temp layer/orientation/scroll方向/pinch/swipe（Mac用かWindows用かiPad用）/inertia → 共通HID
```

左のprocessor順序は次の通り。1〜4はBLE転送前（左手側のFW）、5〜7は
右central側で行う。左手側のFWは作者の設定（e3004b7）から変えていない。

| 順序 | Processor | 結果 |
|---|---|---|
| 1 | `zip_xy_scaler 0 1` | REL_X/Yを0化 |
| 2 | `left_swipe_axis_mapper` | 元のREL_HWHEEL（2本指の左右）をREL_DIALへ変換 |
| 3 | `left_wheel_to_hwheel` | REL_WHEEL（2本指の上下）をREL_HWHEELへ変換、値と符号は保持 |
| 4 | `left_middle_click_mapper` | BTN_1をBTN_2へ変換、DOWN/UPは保持 |
| 5 | `left_tps43_orientation` | 取り付けの向きに応じて符号をそろえる（右がDIAL正、下がHWHEEL正） |
| 6 | `left_tps43_tab_swipe` | REL_HWHEEL（上下）からタブ移動を判定。`pass-swipe-axis`で、REL_HWHEELも次へ通す |
| 7 | `left_tps43_desktop_swipe`（WINでは`_win`、IPADではなし） | REL_DIAL（左右）から仮想デスクトップ移動を判定し、この軸の出力を抑止。上下の動きも見て、斜めの動きで両方が反応しないようにする |
| 8 | `left_tps43_drop_tab_axis` | 通してきたREL_HWHEELをREL_MISCへ変換。listenerはREL_MISCとREL_DIALを無視するので、どちらの軸もスクロールにならない |

左TPS43の`invert-scroll-y`でWHEELの符号を反転してからHWHEELへ変換し、
左右スワイプは`invert-scroll-x`で符号を反転する。作者の実測では、
切り欠き右で右スワイプが負のREL_DIALになる。切り欠き左はこの逆で、
5の`left_tps43_orientation`が両方の向きを同じ符号にそろえる。
タブの判定閾値は8、上下方向の累積量が左右方向の総移動量の2倍以上の
場合にタブを切り替える。上（負）で`Ctrl+Tab`、下（正）で`Ctrl+Shift+Tab`。
仮想デスクトップも同じ閾値で、左右と上下を入れ替えて判定する。

右TPS43は、`tps43_orientation`でカーソルとスクロールの向きを補正した後、
`tps43_scroll_direction`で縦スクロールの符号を反転する。補正後は
カーソルと同じく指を下へで正になるため、ナチュラルスクロールのホストで
中身が指について動くよう、指を上へで正（上スクロール）にする。

BTN_0は変換しない。左はsingle-tapを有効にせず、press-and-hold、
two-finger-tap、scrollのみ有効。現在の設定値は`hold-time = <1>`。
標準scalerはイベントを破棄せず値を0にするため、X/YのBLE通知件数は
減らない。左右スワイプの値とtouch、sync情報もそのまま転送される。

右の実測補正はXY交換と両軸反転を含む。左は上下操作をドライバーの
WHEELとして扱うため、右のコントローラー設定からswitch-xyを省き、
invert-xは維持する。左の取り付けの向きは、右central側の
`left_tps43_orientation`で補正する。

右TPS43はpress-and-holdを無効にし、Layer 1で左右スワイプを戻る／進むへ切り替える（2026-11-24以前はタブ移動）。
通常レイヤーのスワイプ、タップ、スクロール、inertia、orientation、
Layer 1 Snipe、感度、電源管理、RST/RDYは維持。
固定ZMKのHIDはボタン押下をカウントするため、右の移動イベントは左の
BTN_0保持を解除しない。左保持中の右タップも、タップ終了後に左の押下が残る。

## 自動確認

左右のFWを通常のbuild.yaml設定でビルドしてから実行する。
Zephyr付属のpython-devicetreeを使い、生成されたDevicetreeと.configを検証する。

```powershell
python scripts/check_dual_tps43.py `
  --zephyr-base <Zephyrのパス> `
  --left-build <左buildディレクトリ> `
  --right-build <右buildディレクトリ>
```

確認対象はピン、RSTのActive HIGH、GPIO/pinctrl競合、gesture設定、
split reg/device/listener経路、central/peripheral、バッテリー・sleepの
有効設定とStudioの無効設定、processor順序。生成されたprocessorの設定値からイベント変換を
モデル化し、負値を含むX/Y抑止、スワイプ軸と水平スクロール軸の分離、
タブ移動のキー設定、WHEEL変換、BTN_1変換、BTN_0とsync保持、
左保持中の右タップを確認する。

この検証は実際のC処理・BLE通信・TPS43のgesture判定・HIDタイミングを
実行するものではない。実機動作の合格を意味しない。

2026-10-06の確認結果：左右TPS43ドライバーとinput-splitを含むFWビルド成功。
左の標準scaler/code mapperもコンパイル成功。上記自動確認はすべてPASS。
Devicetree/bindingエラーなし。非推奨設定、peripheralのZMK_USB依存、
右ZMKのcombo/event_manager配列境界についてビルド警告あり。

2026-10-07の確認結果：左手のタブ移動を追加した左右FWのビルド成功。
スワイプ軸と水平スクロール軸の分離、タブ移動のキー設定を含む自動確認は
すべてPASS。実機でのスワイプ方向とブラウザの動作確認は未実施。

Layer 1の右手タブ移動追加後も、右FWのビルドと自動確認はPASS。
通常時の進む／戻るとLayer 1のタブ移動のキー設定を検証済み。

## 実機確認（未実施）

左右それぞれ新しいUF2を書き込み、通常のBLE split接続後に確認する。
既存のペアリング情報はまず維持する。settings_resetは通常の更新では不要。

ジェスチャの動作は冒頭の割り当て表と照合する。加えて、次の点を確認する。

- 左手で保持したまま右手でドラッグでき、左手の指を離すとドロップできる。
  左手の指を少し動かしても座標に影響せず、右手を止めても保持が継続する。
- 左手で保持中に右手でタップしても、左手の指を離すまで保持が残る。
- 左手の上スワイプで右のタブ、下スワイプで左のタブへ移動し、1回のタッチで
  複数回切り替わらない。左右の操作でタブが切り替わらない。
- 左手の2本指の左右で仮想デスクトップを移り、上下の操作でデスクトップが
  切り替わらない。iPadでは左右で何も起きない。
- 右手の2本指の左右で水平スクロールし、向きが左手と同じ。Layer 1では右手の
  右スワイプで戻る、左スワイプで進む。Layer 1のSnipeと縦スクロールも動作する。
- 左右のキー入力と両側のバッテリー取得が従来通りに動作する。
- 左右同時操作を繰り返しても、クラッシュ・ボタン固着・入力欠落がない。
- 両側のDeep Sleep後にキーで復帰すると、splitが再接続し、両TPS43とキーを操作できる。

必要ならホストのHIDイベント表示でBTN_0 DOWN/UP、BTN_2 DOWN/UP、
水平wheel、左由来の非ゼロX/Yがないことを確認する。左操作のみでは
BTN_1がホストへ出ないことも確認する。

Deep Sleepでは起動時の既存RST処理を利用する。新たなsleep hookはない。
保持中のBLE切断・peripheral再起動でUPが届かなかった場合の状態復旧は、
この設定では追加していない。接続断を伴うケースは通常ドラッグと分けて確認する。
