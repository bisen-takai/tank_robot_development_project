# Tank_Robot RTC継続性・時刻復旧契約基本設計

本書は、公開用システム要求仕様で追加されたRTCバックアップ電池を前提に、RTCの保持電源、保持継続を確認する情報、起動時判定、時刻の信頼を失った場合の正式な製品保守経路、および関連機能の担当分担を定める。

**関連する設計**

[「Tank_Robotシステム構成」](../../01_システム構成/Tank_Robotシステム構成.md)、[「電源・ハードウェア基本設計」](../../05_電源・ハードウェア/Tank_Robot電源・ハードウェア基本設計.md)、[「セキュリティ管理基本設計」](../17_セキュリティ管理/セキュリティ管理基本設計.md)、[「固定ブート・復旧基本設計」](../19_固定ブート・復旧/固定ブート・復旧基本設計.md)、[「時刻管理基本設計」](時刻管理基本設計.md)、[「ログ・診断基本設計」](../14_ログ・診断/ログ・診断基本設計.md)、[「永続データ管理基本設計」](../16_永続データ管理/永続データ管理基本設計.md)、[「状態表示・利用者通知基本設計」](../21_状態表示・利用者通知/状態表示・利用者通知基本設計.md)、[「性能・品質・検証基本設計」](../../08_性能・品質・検証/Tank_Robot性能・品質・検証基本設計.md)を中心とするRTCの保持継続確認・時刻復旧。

**本書と関連文書の適用範囲**

1. **本書が定める事項**：RTCの保持継続を確認する情報、RTCバックアップ電池、起動時判定、およびTIME_UNTRUSTEDからの時刻復旧経路。
2. **関連文書が引き続き定める事項**：本書が明示的に補完する事項、または文書間で共通の規則として確定する事項以外は、本書で挙げる関連する基本設計の担当範囲と変更責任を維持する。
3. **本書を優先する場合**：上記の事項について本書で基本設計として確定した記述と、当該事項への対応前の旧記述が競合する範囲に限る。対応経緯と、比較基準の版がある場合の識別情報は、末尾の「作成経緯」に示す。
4. **本書を優先しない事項**：詳細設計へ残す内部実装、および補完対象外の状態、機能の動作、安全、セキュリティ、外部との通信・連携規則などは、それぞれを担当する基本設計に従う。

時刻信頼状態、Trusted Time Record、時刻更新処理（Time Update transaction）、VPS時刻同期および`TIME_TRUSTED`判定は、引き続き時刻管理基本設計（B20）に従う。本書は、それらを成立させるために必要な、複数文書にまたがるRTCの保持継続を確認する情報（continuity evidence）と製品保守経路を補完する。

末尾の作成経緯に示す比較基準コミット以前の基本設計に、次の旧方針が残る場合、本書の補完対象については本書を優先する。

- 「専用CR2032等の第二電池を初期必須としない」
- 「main battery取り外しだけを理由としてRTC continuityを失う」
- 「RTC continuity検出方法を詳細設計で新規選択する」
- 「provisioning tool、maintenance tool、service buildまたは同等経路のいずれかを詳細設計で選択する」

本書で確定した方式を詳細設計で決める事項へ戻さない。

---

## 1. 上位要求と設計目的

### 1.1 追加された公開用要求

本書は、少なくとも次の公開用システム要求を設計入力とする。

- `SYS-PWRSAFE-001`：本体の通常動作に必要な電力を1つのバッテリーから供給する。
- `SYS-PWRSAFE-004`：RTC時刻保持専用バックアップ電池は本体通常動作・通信・アクチュエータ電源へ使用しない。
- `SYS-PWRSAFE-005`：主バッテリー取り外し状態でもRTCバックアップ電池でRTC保持電力を供給する。
- `SYS-PWRSAFE-006`：RTC保持電源切替えで時刻保持を意図せず失わず、逆流・誤接続等で損傷しない。
- `SYS-SUPPORT-020`：RTCバックアップ電池を交換可能な保守対象とする。
- `SYS-SUPPORT-021`：RTCバックアップ電池交換後にRTC保持状態および時刻信頼性を確認できる。
- `SYS-SUPPORT-022`：continuity／信頼性を確認できない場合は信頼済み時刻として使用せず、必要な復旧を実施できる。

### 1.2 設計目的

初期製品では、通常のLiPo充電・交換のために本体から2S LiPoバッテリーを取り外しても、RTCバックアップ電池が正常である限りRTC calendarとcontinuity evidenceを保持し、次回起動直後から`TIME_TRUSTED_HOLDOVER`へ復帰可能な構成とする。

一方、バックアップ電池の存在だけを`TIME_TRUSTED`の根拠にはしない。RTC保持電源が一度でも失われた可能性、RTC source再初期化、保持証拠の破損・欠落、Trusted Time Recordとの矛盾等を検出または一意に否定できない場合は、安全側に`TIME_UNTRUSTED`とする。

---

## 2. RTC保持電源の基本構成

### 2.1 通常動作用電源との分離

本体の走行、サーボ、MCU通常動作、BLE、Wi-Fi、表示その他の通常動作用電源は、公開用要求どおり2S LiPoバッテリー系から供給する。

RTCバックアップ電池はRA8M2 RTC/VBATT保持専用とし、次へ給電しない。

- `PWR_CTRL`
- `PWR_SAFETY`
- `PWR_BLE`
- `PWR_WIFI`
- `PWR_DRIVE`
- `PWR_SERVO`
- `PWR_DISPLAY`
- その他の本体通常動作回路

RTCバックアップ電池だけが接続されている状態で、MCU Application、通信、表示またはアクチュエータを起動できる構成にしない。

### 2.2 バックアップ電池

初期製品は、交換可能な3 Vコイン形一次電池（CR2032クラス）をRTCバックアップ電池の基本方式とする。

基本設計で固定するのは、次である。

- nominal 3 Vクラスの一次電池であること
- RTC/VBATT保持専用であること
- 交換可能であること
- 主バッテリー側RTC保持電源との間で逆流させないこと
- 主バッテリー側から一次電池へ充電電流を流さないこと
- バックアップ電池から`PWR_AON`または本体通常電源へ逆給電しないこと

具体メーカー、製品型式、ホルダ、保護部品、ORing素子、抵抗・コンデンサ、PCB layoutは詳細設計で定める。

### 2.3 二つのRTC保持電源

RTC/VBATTは次の2系統から給電可能とする。

1. `PWR_AON`由来の主バッテリー側RTC保持電源
2. RTCバックアップ電池

通常は`PWR_AON`由来の保持電源を優先し、主バッテリー接続中にRTCバックアップ電池を不要に消費しない構成を基本とする。

`PWR_AON`由来保持電源が失われた場合は、RTCバックアップ電池がVBATT保持を継続する。切替え方式は、RTC/VBATTの許容電圧・保持条件を満たし、切替えそのものによってRTC calendarまたはbackup domainを初期化しないことを成立条件とする。

### 2.4 電流の配分の扱い

主バッテリー接続中かつ`PWR_CTRL` OFF時の主バッテリーの待機電流の配分は、既存PQV/H01の次の体系を維持する。

- システム総量：100 µA以下を評価目標
- `PWR_AON`：ハードウェア側への電流配分の目標は50 µA以下
- RTC/VBATT branch：5 µA以下を`PWR_AON`内数の評価目標

主バッテリー取り外し中にRTCバックアップ電池から供給する電流は、主バッテリー待機電流100 µA／`PWR_AON` 50 µAの算定対象外とする。

RTCバックアップ電池側については、RTC/VBATT消費、ORing/保護回路漏れ、PCB漏れおよび電池自己放電を含む保持寿命をH01/PQVで評価する。具体的な保証期間・交換周期は、採用電池・回路の実測結果に基づき確定し、詳細設計だけで製品運用値を決めない。

### 2.5 常時電圧監視

初期製品では、RTCバックアップ電池の電圧をADC等で常時測定し、残量を連続推定する機能を必須としない。

理由は、RTC保持専用電池に対して新しい常時監視回路・ADC入力・AON消費を追加せず、初期製品の複雑性を抑えるためである。

ただし、次は維持する。

- continuity evidenceによる時刻信頼判定
- 保守時の電池交換
- H01/PQVでの電池保持寿命評価
- continuity喪失・RTC異常時の利用者／保守通知

将来、実機評価または保守運用上必要と判定した場合は、バックアップ電池低下監視を基本設計へ追加する。

---

## 3. 責任分界

### 3.1 H01 電源・ハードウェア

電源・ハードウェア基本設計（H01）では、次を定める。

- `PWR_AON`由来のRTC保持電源と、RTCバックアップ電池の物理構成
- VBATTの許容電圧を満たすための条件
- 主バッテリー側とバックアップ電池側の優先順位・切替え方式
- 逆流の防止
- 一次電池への充電の防止
- バックアップ電池から通常電源への逆給電の防止
- 電池ホルダ、コネクタ、および電池交換のための構造
- 漏れ電流と保持寿命の条件を満たすハードウェア構成
- RTC保持の継続を確認する根拠情報を取得するために必要な、バックアップ電源で状態を保持する領域（backup domain）の保持条件

H01は、`TIME_TRUSTED`／`TIME_UNTRUSTED`の判定を担当しない。

### 3.2 B19 固定ブート・復旧

固定ブート・復旧機能（B19）は、起動直後にRTC保持の継続を確認するための根拠情報を、未加工の状態で取得する。
取得は、本体アプリケーションがRTCを再設定・再初期化する前に行う。

取得した情報は、起動時引継ぎ情報（Boot Handoff）を通じて時刻管理機能（B20）へ渡す。
固定ブート・復旧機能は、最終的に時刻を信頼してよいかを判断しない。

### 3.3 B20 時刻管理

時刻管理機能（B20）は、次の情報を用いて、RTC保持の継続と時刻の信頼状態を最終判断する。

- 起動時の根拠情報（`RTC_CONTINUITY_EVIDENCE`）
- Trusted Time Record
- Time Update Record
- RTCの読戻し結果

これらを照合し、`TIME_TRUSTED_HOLDOVER`、`TIME_UNTRUSTED`または`TIME_UNSET`として扱うかを判断するのは、時刻管理機能だけとする。

### 3.4 B17 セキュリティ

セキュリティ管理機能（B17）は、有線接続による製品保守用の時刻復旧について、許可条件と保護範囲を定める。

- 遠隔操作による保守への移行を禁止する。
- 本体への物理的な保守アクセスを必要とする。
- 通常運用（`NORMAL_OPERATION`）の一般診断UARTから、任意の時刻書換えを許可しない。
- 時刻管理機能の時刻更新処理（Time Update transaction）に必要な`K_SEC_STATE_MAC`処理を提供する。
- 許可された保守実行環境の外から、Trusted Time Recordを直接生成・改変できないようにする。

### 3.5 B16 永続データ管理

永続データ管理機能（B16）は、Trusted Time RecordとTime Update Recordの原子的な永続保存を実行する。
CRC、A/Bの複製、保存確定を示す印（commit marker）、および読戻しも担当する。

永続データ管理機能は、RTC保持が継続していたか、時刻を信頼してよいかを判断しない。

### 3.6 B14 ログ・診断

ログ・診断機能（B14）は、RTC保持の継続に関する判定、時刻の信頼喪失、バックアップ電池に関する保守、有線復旧の開始・成功・失敗などを記録する。
これらは、時刻品質の情報を含むイベントとして記録する。

### 3.7 B21 状態表示・利用者通知

状態表示・利用者通知機能（B21）は、時刻管理機能が提供する`TIME_UNTRUSTED`、有線保守が必要な状態、RTC異常などを、利用者向けの状態表示・通知へ変換する。

バックアップ電池残量の連続表示は初期必須としない。

---

## 4. `RTC_CONTINUITY_EVIDENCE`契約

### 4.1 目的

`RTC_CONTINUITY_EVIDENCE`は、前回の信頼時刻確立後も、RTCを信頼して利用できる状態が続いていたかを判断するための根拠情報である。
起動ごとに取得し、時刻管理機能（B20）が今回の初期化時に使用する。

確認するのは、RTCの日時情報（calendar）と、バックアップ電源で状態を保持する領域（backup domain）が、前回の信頼時刻確立から今回の初期化まで保持されていたかである。
**RTCの日時を読み出せることと、その時刻を継続して信頼できることは区別する。**

単一ビットの値やRTCの日時情報だけで、保持の継続を断定しない。

### 4.2 根拠情報の取得・提供主体

起動時の未加工の根拠情報は、固定ブート・復旧機能（B19）／プラットフォームの起動直後の処理（Platform early boot）が取得・提供する。

固定ブート・復旧機能は、本体アプリケーションによる通常のRTCドライバ初期化、RTCの計時源（RTC source）の再設定、または時刻設定より前に、少なくとも次の情報を取得する。

| 項目 | 意味 |
| --- | --- |
| `bootResetClass` | 電源投入、ソフトウェアリセット、IWDT、その他のリセット区分 |
| `backupRetentionMarkerValid` | バックアップ電源で状態を保持する領域の保持確認情報（marker）が正しいか |
| `rtcReadResult` | RTCの日時情報を読み出せるか |
| `rtcCalendarSnapshot` | 起動直後に取得したRTCの日時情報 |
| `rtcSourceContinuity` | RTCの計時源の保持が継続していると判断できるか、再初期化が必要か、または不明か |
| `backupPowerEvidence` | 起動時に取得可能なVBATTと保持領域の状態。未取得時は`UNKNOWN` |
| `evidenceCaptureResult` | 未加工の根拠情報一式を、矛盾なく取得できたか |

具体的なRA8M2のレジスタ名、FSP API、バックアップレジスタ番号、読出し順序、および排他制御を行う区間（Critical Section）は、詳細設計で定める。

### 4.3 backup retention marker

RTC backup domainには、前回`TIME_TRUSTED`が正常確立され、かつRTC保持条件が成立していたことを示す`RTC_CONTINUITY_MARKER`を保持する。

markerは少なくとも次の性質を持つ。

- backup domain電源が保持される限りresetをまたいで保持される。
- backup domain喪失時に有効値を維持したと仮定しない。
- magic値単独ではなく、反転値、CRCその他の単純破損を検出できる組で保持する。
- markerが不正・欠落・矛盾の場合、calendarがもっともらしくてもcontinuity confirmedにしない。
- markerの具体bit layout、register割当ては詳細設計で定める。

### 4.4 markerの成立・失効

`RTC_CONTINUITY_MARKER`は、次の場合だけ有効化または再有効化する。

- 製造provisioningまたは正式なwired time recoveryでB20 Time Update transactionが正常COMMITTEDとなり、Trusted Time RecordとRTC readbackが整合した後
- 既存TRUSTED状態のcontinuityを正常確認し、markerが既に正常である場合は再生成不要

次の場合はmarkerを新しいTRUSTED根拠として自動生成しない。

- RTC calendarがもっともらしいだけの場合
- VPS接続を試しただけの場合
- 起動時にmarkerが不正／欠落している場合
- RTC source再初期化を実施した場合
- Trusted Time Recordの現在のlogical revisionを確定できない場合

RTC sourceを意図的に再初期化する必要がある場合は、再初期化前にcontinuityを失効扱いとし、再初期化後のcalendarを自動でTRUSTEDへ昇格しない。

### 4.5 evidenceのpublish

B19はraw evidenceをBoot Handoffへ一回publishし、本体アプリケーション側で同一起動中の証拠として扱う。

ApplicationがRTCを設定・補正・source変更した後の値で、起動時raw evidenceを上書きしない。

Boot HandoffのC構造体、field packing、CRC、version番号等は詳細設計で定めるが、上記semantic fieldを欠落させない。

---

## 5. 起動・reset・電源イベント別判定

### 5.1 共通判定

B20は、少なくとも次を全て満たす場合だけcontinuityを`CONFIRMED`とする。

- 現在のlogical Trusted Time Recordまたは同revisionの正常冗長copyが成立
- Time Update RecordがNONE/COMMITTEDまたは一意に正常復旧済み
- `backupRetentionMarkerValid = true`
- RTC calendarが正常読出し可能
- RTC sourceを保持中に再初期化していないことが確認できる
- 起動時raw evidenceに矛盾・欠落がない
- RTC現在値がB20既存の後退許容条件を満たす

一つでも「喪失」が確認された場合はcontinuityを`LOST`とする。

証拠が取得不能、矛盾または一意に判断できない場合はcontinuityを`UNKNOWN`とする。

`LOST`と`UNKNOWN`はいずれも`TIME_TRUSTED_HOLDOVER`へ昇格させず、B20既存方針に従って`TIME_UNTRUSTED`とする。

### 5.2 イベント別基本判定

| 事象 | 基本判定 |
| --- | --- |
| 初回製造前／Trusted Time Recordなし | `TIME_UNSET`。calendarだけでTRUSTEDにしない |
| 通常主電源OFF→ON、主バッテリー接続継続 | marker/RTC/record正常ならcontinuity confirmed |
| software reset | marker/RTC/record正常ならcontinuity confirmed |
| IWDT reset | marker/RTC/record正常ならcontinuity confirmed。reset理由だけでtrustを失効しない |
| main battery取り外し→再接続、RTC backup battery正常 | backup domain markerとRTCが保持されていればcontinuity confirmed |
| main battery取り外し中にRTC backup batteryも喪失 | continuity lost → `TIME_UNTRUSTED` |
| VCC brownout／main rail異常、VBATT保持成立 | evidenceが正常ならcontinuity confirmed。RTC source状態不明ならUNKNOWN |
| RTC source再初期化が必要 | continuity lost/unknown → `TIME_UNTRUSTED` |
| marker不正・欠落、calendarはもっともらしい | `TIME_UNTRUSTED` |
| recordとRTCが矛盾 | `TIME_UNTRUSTED` |
| evidence capture失敗 | `TIME_UNTRUSTED` |

### 5.3 main battery取り外し

main battery取り外し自体を、直接`TIME_UNTRUSTED`条件としない。

RTCバックアップ電池がVBATTを正常保持し、`RTC_CONTINUITY_MARKER`、RTC source、calendarおよびTrusted Time Recordの整合を次回起動で確認できれば、`TIME_TRUSTED_HOLDOVER`を継続できる。

### 5.4 backup battery交換

RTCバックアップ電池交換は、可能な限り次の保守条件で行う。

- 本体主電源はOFF
- 主バッテリーは接続したまま
- `PWR_AON`由来RTC保持電源が正常

この条件では、CR2032クラス電池を取り外しても`PWR_AON`がVBATT保持を継続し、RTC continuityを維持できる構成を基本とする。

主バッテリーを同時に取り外す必要がある、`PWR_AON`保持を保証できない、交換中にVBATT continuityが不明となった、または交換後evidenceが矛盾する場合は、`SYS-SUPPORT-022`に従い`TIME_UNTRUSTED`として正式なwired time recoveryを実施する。

---

## 6. `TIME_UNTRUSTED`時の製品挙動

### 6.1 ローカル操作

B20既存方針を維持し、時刻異常単独ではBLEによるローカル基本操作、安全停止、E-STOP、電源保護等を停止させない。

control／Safety timeoutはRTC絶対時刻に依存しない。

### 6.2 VPS

`TIME_UNTRUSTED`では通常VPS管理通信を開始しない。

既存VPS sessionがある状態で`TIME_TRUSTED`を失った場合は、B12既存契約に従い新規request受付を停止し、実行中requestを中断し、TLS/TCPを終了してsession情報を無効化する。

VPS `/time`、公開NTP、smartphone OS clock、BLE message、HTTP Dateまたはbuild timestampだけを用いて`TIME_UNTRUSTED`から`TIME_TRUSTED`へbootstrapしない。

### 6.3 ログ

`TIME_UNTRUSTED`中もB14の`bootInstanceId`、`eventSequence`、`monotonicTimeUs`でevent順序を維持する。

信頼できないRTC値を同期済みUTCとして保存しない。

---

## 7. 正式な製品保守用時刻復旧経路

### 7.1 一つの正式経路

初期製品で`TIME_UNTRUSTED`／`TIME_UNSET`から新しく`TIME_TRUSTED`を確立する正式な製品保守経路は、**本体アプリケーション側のAUTHORIZED_WIRED_TIME_RECOVERY**とする。

開発用SWD/JTAG、任意debug script、service buildへの一時書換え等を製品保守の正式経路としない。

製造provisioningは別途許可された初回信頼確立経路として維持する。

### 7.2 物理インターフェース

AUTHORIZED_WIRED_TIME_RECOVERYは、H01/B14の内部3.3 V保守用UARTを通信経路として使用できる。

通常保守UARTを参照専用の診断に使用する規則は維持する。NORMAL_OPERATION中の通常診断へ、一般的なRTC書込みコマンドを追加しない。

同じUARTを使用する場合も、参照専用の診断とAUTHORIZED_WIRED_TIME_RECOVERYの実行環境を論理的に分離する。

### 7.3 有線時刻復旧への移行・許可条件

初期製品の有線接続による時刻復旧（`AUTHORIZED_WIRED_TIME_RECOVERY`）では、少なくとも次のすべてを必要とする。

1. 筐体内部の保守用パッドまたはコネクタへ、物理的にアクセスしていること。
2. 電源投入またはリセット時に、物理操作による保守許可（service-enable）条件が成立していること。
3. セキュリティ管理機能（B17）が許可する、管理された有線保守の実行環境であること。
4. 通常のBLE・Wi-Fi・VPS操作では、上記の保守許可を成立させられないこと。

通常動作中のソフトウェア指令、BLE指令、VPS指令、または通常の利用者向け画面操作だけで、`AUTHORIZED_WIRED_TIME_RECOVERY`へ移行できないようにする。

保守許可に用いる具体的な端子、ストラップ設定、ボタン、操作手順、およびDLM/ALの条件は、電源・ハードウェア基本設計（H01）とセキュリティ管理基本設計（B17）の詳細設計で確定する。

### 7.4 保守実行中の安全状態

`AUTHORIZED_WIRED_TIME_RECOVERY`中は、少なくとも次を満たす。

- 走行・砲塔・砲身の通常操作を禁止する。
- アクチュエータ出力を安全停止状態に維持する。
- 通常VPS通信を開始しない。
- BLEの通常操作セッションを成立させない、または操作を受理しない。
- 時刻更新処理（Time Update transaction）と、必要な診断・ログだけを実行可能とする。

この保守実行環境を、新しい恒久的なシステム状態として追加する必要はない。
起動処理中に設ける、機能を制限した保守実行段階として実装できる。

### 7.5 保守ツールから受け取る情報

#### 保守ツールが提供する情報

保守ツールは、少なくとも次の情報を本体へ提供する。

- 信頼できる外部UTC時刻
- ツール・手順の識別情報、またはその版
- 対象のDevice IDを確認するための情報

#### 本体側で生成・確定する情報

保守ツールから、Trusted Time Recordのバイト列そのもの、MAC、版・世代（revision）、または内部の保存確定を示す印（commit marker）を直接書き込ませない。

本体側の時刻管理機能（B20）、セキュリティ管理機能（B17）、永続データ管理機能（B16）が、候補レコード、版・世代、MAC、および更新取引を生成・確定する。

通信パケットの区切り方、コマンドコード、符号化方式、およびPC用保守ツールの画面は、詳細設計で定める。

### 7.6 Time Updateの更新手順

正式な有線復旧でも、B20が定める既存のTime Updateの更新手順を変更せず使用する。

1. 入力UTC、Device ID、保守作業に関する情報を検証する。
2. B20が、新しい`timeTrustRevision`、新しい`timeSyncGeneration`、`updateTransactionId`を生成する。
3. Time Update RecordをPREPAREDとしてB16へ保存・確定する。
4. RTCへUTCを設定する。
5. RTCを読み戻して設定結果を確認する。
6. 候補のTrusted Time Recordを、B17の`K_SEC_STATE_MAC`で認証し、B16へ保存・確定する。
7. Time Update RecordをCOMMITTEDへ確定する。
8. RTC、Trusted Time Record、および版の関係を再確認する。
9. `RTC_CONTINUITY_MARKER`を正常に成立させる。
10. B20が`TIME_TRUSTED`を成立させる。
11. B14へ監査イベントを記録する。
12. 制御された手順で再起動し、通常起動時のRTC継続性の判定を再実施する。

### 7.7 中断・失敗

PREPARED前に失敗した場合はRTCを変更しない。

PREPARED後にリセット、電源喪失、ツールの切断等が発生した場合は、B20が定める既存の起動時Time Update復旧手順を使用する。

候補と更新前の状態のどちらを採用すべきか一意に証明できない場合は、`TIME_UNTRUSTED`を維持する。

保守ツール側が「成功」と表示したことだけを、TRUSTEDの根拠にしない。

### 7.8 終了条件

wired recovery成功後はcontrolled rebootを実施し、通常起動時に次を再確認する。

- `RTC_CONTINUITY_MARKER`
- RTC read
- RTC source continuity
- Trusted Time Recordの現在revision
- Time Update Record COMMITTED
- RTC後退条件

これらが成立した場合だけ通常VPS利用を再許可する。

---

## 8. DEVELOPMENT／EVALUATIONとの分離

### 8.1 開発用時刻設定

DEVELOPMENT/EVALUATIONでは、試験自動化、fault injection、SWD/JTAG等からRTC設定を行うことを許容できる。

ただし、これらは製品保守用の正式なTRUSTED確立経路ではない。

### 8.2 製品用credentialとの分離

DEVELOPMENT/EVALUATION toolによる直接RTC操作で、NORMAL_OPERATION製品のTrusted Time Recordを正規保守済みとして扱わない。

製品相当評価ではAUTHORIZED_WIRED_TIME_RECOVERYを使用して復旧成立性を検証する。

---

## 9. ログ・診断・通知

### 9.1 必須event

少なくとも次をB14へ記録する。

- RTC continuity confirmed
- RTC continuity lost
- RTC continuity unknown
- RTC retention marker invalid
- RTC source reinitialization required
- backup battery maintenance/replacement実施
- `TIME_UNTRUSTED`成立
- wired time recovery開始
- wired time recovery PREPARED
- wired time recovery成功
- wired time recovery失敗／中断
- `TIME_TRUSTED`再確立

### 9.2 event時刻

`TIME_UNTRUSTED`成立後のeventは、信頼済みUTCがない場合でも`bootInstanceId`＋`monotonicTimeUs`で順序を保持する。

### 9.3 利用者通知

B21へ少なくとも次を提供する。

- `TIME_UNTRUSTED`
- RTC continuity lost/unknown
- wired maintenance required
- RTC fault
- Time Update recovery failed

RTCバックアップ電池の残量warningは初期必須としない。

---

## 10. 評価・検証

### 10.1 電源切替え

- 主バッテリー接続中に`PWR_AON`側がRTC/VBATTを供給すること
- 主バッテリー取り外し時にbackup cellへ切替えてRTCを保持すること
- 主バッテリー再接続でRTC保持を失わないこと
- source間逆流がないこと
- CR2032クラス一次電池へ充電電流が流れないこと
- backup cellから`PWR_AON`／通常電源へ逆給電しないこと

### 10.2 continuity

- normal shutdown／power-on
- software reset
- IWDT reset
- main battery removal／reinsert with healthy backup cell
- backup cell removal with main battery/PWR_AON maintained
- main batteryとbackup cellの両方を喪失
- RTC source再初期化
- retention marker破損
- Trusted Time Record破損
- RTC値後退
- raw evidence一部取得不能
- raw evidence相互矛盾

### 10.3 backup battery交換

- 主電源OFF、主バッテリー接続、PWR_AON正常状態でbackup cell交換後もcontinuity confirmedになること
- main batteryも外した交換でcontinuity喪失した場合に`TIME_UNTRUSTED`となること
- 交換後の時刻信頼状態を確認できること

### 10.4 wired recovery

- remote経路からentry不可
- normal read-only maintenance contextからRTC write不可
- physical service-enable＋wired maintenanceでのみrecovery可能
- PREPARED前disconnect
- RTC設定直後reset
- Trusted Time Record commit前reset
- COMMITTED直後reset
- marker書込み失敗
- tool誤Device ID
- calendar範囲外入力
- recovery成功後rebootでTRUSTED再成立

### 10.5 寿命評価

H01/PQVは採用CR2032クラス電池について、少なくとも次を評価する。

- RTC/VBATT実消費
- ORing／保護回路漏れ
- PCB漏れ
- 温度影響
- 電池自己放電
- worst-case保持期間
- 交換周期候補

保持期間・交換周期を製品運用値として固定する場合は、評価結果をH01/PQVまたは関連保守基本設計へ反映する。

---

## 11. 詳細設計へ引き継ぐ範囲

次は、基本設計で確定した意味・安全側挙動を変更しない範囲で詳細設計へ引き継ぐ。

- CR2032クラス電池の具体メーカー／型式
- holder／connector
- ORing方式、diode／ideal diode／load switch等の具体部品
- resistor／capacitor
- RA8M2 VBATT／backup registerの具体pin・register
- `RTC_CONTINUITY_MARKER`のregister割当てとbit layout
- Boot Handoff C構造体
- RTC/FSP API呼出し順
- service-enable pin／strap／buttonの具体方式
- wired maintenance packet format／command code
- PC maintenance tool UI／script
- log event ID数値
- unit test vector

詳細設計だけで次を変更しない。

- RTC backup batteryを初期製品から削除すること
- backup batteryを本体通常動作電源へ使用すること
- main battery removalだけを理由としてcontinuity lostとすること
- evidence不明／矛盾時にTRUSTEDへ昇格すること
- B19以外が起動後のRTC変更済み値からraw continuity evidenceを捏造すること
- TIME_UNTRUSTEDからVPS／BLE／公開NTPだけで自動TRUSTED化すること
- product wired recoveryをservice buildの一時書換えだけへ置き換えること
- Time Update transactionを迂回してRTC／Trusted Time Recordを直接書換えること

---

## 12. 設計判断

| ID | 設計判断 |
| --- | --- |
| D-01 | 初期製品は交換可能な3 Vコイン形一次電池（CR2032クラス）をRTC backup batteryの基本方式とする |
| D-02 | backup batteryはRTC/VBATT保持専用とし、本体通常動作へ給電しない |
| D-03 | 主バッテリー接続中はPWR_AON側を優先し、backup battery消費を抑える |
| D-04 | source切替えでRTC continuityを失わない物理構成とする |
| D-05 | RTC backup batteryの常時ADC残量監視は初期必須としない |
| D-06 | B19/Platformの初期起動処理が、RTCの継続性を判断するための未加工の証拠情報を提供する |
| D-07 | B20が`RTC_CONTINUITY_EVIDENCE`＋Trusted Time Recordから最終trustを判定する |
| D-08 | backup retention markerをcontinuity evidenceの一部として使用する |
| D-09 | evidence欠落・矛盾・判定不能は`TIME_UNTRUSTED`とする |
| D-10 | main battery removalだけではtrustを失効せず、backup保持evidenceで判断する |
| D-11 | backup battery交換はmain battery/PWR_AON保持中を基本手順とする |
| D-12 | 初期製品の正式な製品保守時刻復旧経路をAUTHORIZED_WIRED_TIME_RECOVERYに一本化する |
| D-13 | normal maintenance UARTはread-onlyを維持し、time recoveryはphysical service-enable付きrestricted contextとする |
| D-14 | product time recoveryはB20 Time Update transactionを必ず通す |
| D-15 | recovery成功後はcontrolled rebootして通常起動continuity判定を再実施する |
| D-16 | DEVELOPMENT/EVALUATIONの直接RTC設定はproduct maintenance経路と区別する |

---

## 13. GR-05完了条件

GR-05は、次が基本設計で確定したことをもって完了とする。

- RTC backup batteryの採用と用途
- main battery／backup batteryのRTC保持責任
- RTCの継続性を判断するための未加工の証拠情報の提供元
- `RTC_CONTINUITY_EVIDENCE`のsemantic field
- reset／brownout／battery removal／battery replacement別の判定
- evidence欠落・矛盾時のfail-safe
- TIME_UNTRUSTED時のVPS禁止・local operation継続
- 一つの正式なproduct wired time recovery経路
- physical entry／authorization boundary
- Time Update transactionを用いた復旧
- 失敗・中断・監査・終了条件
- DEVELOPMENT/EVALUATION経路との分離

これらは詳細設計担当者が新規選択する事項ではない。

---

## 作成経緯

基本設計粒度レビューのGR-05への対応として、本書の補完対象を基本設計で確定した。

旧記述との比較基準コミット：`2f4f98c1f870bb64d677b3d037458cf812d3fa14`。これは指摘対応前の記述の適用範囲を確認するための履歴情報であり、本書の現行版を固定するSHAではない。現行版の識別はGit履歴に従う。

旧記述の文章上の整理は、後続の基本設計整合修正またはGR-12の旧引継ぎ記述整理時に実施してよい。
