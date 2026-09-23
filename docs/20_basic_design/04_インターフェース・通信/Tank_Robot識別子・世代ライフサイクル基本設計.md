# Tank_Robot 識別子・世代ライフサイクル基本設計

## 目次

1. 基本原則
2. 横断ライフサイクル契約表
3. Power／Electrical Safety／SystemMonitor
4. Actuation Epoch
5. BLE connection／authentication／request
6. 所有者・操作端末管理のmanagement transactionとBLE requestの相関
7. 永続revision／boot instanceとの関係
8. 情報の提供側・利用側の規則
9. 詳細設計へ残す範囲
10. 検証条件
11. GR-04完了条件

---

## 1. 基本原則

**位置付けと目的**

- **位置付け**：`Tank_Robotインターフェース・通信基本設計.md` 第5章を補完する横断基本設計
- **目的**：複数機能で使用する識別子・世代の生成、更新、参照および無効化の共通規則を定める

本書は、Tank_Robotで使用する識別子、世代番号（generation）、版番号（revision）、有効な区間を区別する番号（epoch）および取引識別子（transaction ID）について、次の共通規則を定める。

- 発行・管理する機能
- 比較してよい範囲
- 未確立時の値
- 生成・更新条件
- 他機能へ公開する時点
- 起動・リセット時の扱い
- 上限到達時の扱い
- 複数の識別情報を組み合わせる条件
- 古い値との再一致を防ぐ規則

識別子が表す内容、更新を必要とする条件、取引の成立条件、および結果の内容は、各機能別基本設計で引き続き定める。
本書は、複数機能にまたがる古い結果の排除、操作失効、進行監視、IWDT更新、再送防止、または復旧時の照合に必要な、識別子の生成から無効化までの共通規則を定める。

既存基本設計に、本書で確定した対象について「具体型、0予約、初期化、周回処理、比較範囲またはID対応付けを詳細設計で決める」とした旧記述が残る場合、本書で定める識別子の生成・更新・無効化の規則に限り本書を優先する。

**対象文書**

次の文書を中心とする識別子・世代・取引の対応付けを対象とする。

- [`docs/20_basic_design/03_機能別基本設計/01_システム状態管理/Tank_Robotシステム状態管理基本設計.md`](../03_機能別基本設計/01_システム状態管理/Tank_Robotシステム状態管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/05_電源・省電力管理/電源・省電力管理基本設計.md`](../03_機能別基本設計/05_電源・省電力管理/電源・省電力管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/06_バッテリー・電気安全監視/バッテリー・電気安全監視基本設計.md`](../03_機能別基本設計/06_バッテリー・電気安全監視/バッテリー・電気安全監視基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/07_走行制御/走行制御基本設計.md`](../03_機能別基本設計/07_走行制御/走行制御基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/08_砲塔・砲身制御/砲塔・砲身制御基本設計.md`](../03_機能別基本設計/08_砲塔・砲身制御/砲塔・砲身制御基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/09_BLE接続・認証/BLE接続・認証基本設計.md`](../03_機能別基本設計/09_BLE接続・認証/BLE接続・認証基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/10_所有者・操作端末管理/所有者・操作端末管理基本設計.md`](../03_機能別基本設計/10_所有者・操作端末管理/所有者・操作端末管理基本設計.md)
- [`docs/20_basic_design/04_インターフェース・通信/Tank_Robotインターフェース・通信基本設計.md`](Tank_Robotインターフェース・通信基本設計.md)
- [`docs/20_basic_design/07_本体ソフトウェアアーキテクチャ/Tank_Robot本体ソフトアーキテクチャ基本設計.md`](../07_本体ソフトウェアアーキテクチャ/Tank_Robot本体ソフトアーキテクチャ基本設計.md)

**本書と関連文書の適用範囲**

1. **本書が定める事項**：複数機能で使用する識別子・世代の発行元、比較範囲、生成・更新・公開時点、起動・リセット時および上限到達時の扱い。
2. **関連文書が引き続き定める事項**：本書が明示的に補完する事項、または文書間で共通の規則として確定する事項以外は、本書で挙げる関連する基本設計の担当範囲と変更責任を維持する。
3. **本書を優先する場合**：上記の事項について本書で基本設計として確定した記述と、当該事項への対応前の旧記述が競合する範囲に限る。対応経緯と、比較基準の版がある場合の識別情報は、末尾の「作成経緯」に示す。
4. **本書を優先しない事項**：詳細設計へ残す内部実装、および補完対象外の状態、機能の動作、安全、セキュリティ、外部との通信・連携規則などは、それぞれを担当する基本設計に従う。

### 1.1 発行・管理する機能または比較範囲が異なれば別IDとする

識別子を発行・管理する機能が異なる場合は、数値が偶然一致しても、同一の取引、世代またはセッションとして扱わない。
また、識別子を単独で比較してよい範囲も区別する。

例：

- BLE接続・認証の`requestId=10` と 所有者・操作端末管理の`managementTransactionId=10` は同一IDではない。
- 走行制御の`driveEpoch=5` と 砲塔・砲身制御の`actuationEpoch=5` は同一epochではない。
- システム状態管理の`stateGeneration=7` と 状態表示・利用者通知の`statusGeneration=7` は同一generationではない。

必要に応じて、識別子を発行・管理する機能、比較してよい範囲、および上位の処理を識別する情報を含む複合キーを用い、対応する処理や結果を照合する。

### 1.2 0の共通扱い

本書で0予約を定める整数ID／generation／epochは、0を次のいずれかにだけ使用する。

- 未確立
- 無効
- 該当なし
- 未認証／未相関

0を有効な新規IDへ割り当てない。
通信データの形式・送受信規則で0が「request不要」等の別意味として既に確定している場合は、その通信データの形式・送受信規則を優先し、非0が必須となる取引では0を拒否する。

### 1.3 現在の正式値として公開する時点

新しい世代番号（generation）または版番号（revision）は、関連する状態や状態情報一式（snapshot）が整合した一組として公開可能になった時点で、初めて現在の正式値とする。

次を禁止する。

- 世代番号だけを進め、以前のデータ内容を新しい世代の情報として公開すること。
- データ内容だけを更新し、以前の世代番号のまま現在の正式値として公開すること。
- 提供側機能で一部の処理を開始しただけで、機能側での更新が確定した世代番号として扱うこと。

Double Buffer + Generation、短いCritical Section、copy-on-publishなどの実装方式は詳細設計で選択できる。
ただし、上記の公開時点を変更しない。

### 1.4 同一起動内でのみ有効な値

同一起動内でのみ有効と定めた値は、その本体アプリケーション起動中だけ単独比較できる。
リセット後は、旧値との大小比較だけで新旧を判断しない。

起動をまたいでログ、診断、永続復旧または外部との対応付けへ使用する場合は、ログ・診断の`bootInstanceId`その他の起動単位識別情報と組み合わせる。

同一起動内でのみ有効な値をリセット後に同じ数値へ再初期化できることは、以前の起動で使用した値を現在値として再利用できることを意味しない。

### 1.5 上限到達時の扱い

古い値の排除、安全、セキュリティ、IWDTまたは取引結果の対応付けに使用する有限幅IDは、**同じ比較範囲内で数値を周回させ、古い値と再一致させない**。

次の値を安全に表現できない場合は、0または小さい値へ戻して通常処理を継続しない。
対象機能は、新しい取引、新しい操作実行条件、または新しいセッションの成立を禁止する。
そのうえで、現在の安全状態または最後に正常確定した状態を維持し、異常管理へ識別子・世代の上限到達として通知する。

同一起動内でのみ有効なIDは、必要な安全化と処理中取引の破棄を完了した後、制御された再起動によって新しい起動単位へ移行できる。
永続的な版番号（revision）は再起動によって巻き戻さず、その事項を定める基本設計の保守処置へ従う。

### 1.6 処理中の値との再衝突禁止

新しい値を発行する場合、現在処理中、再送待ち、結果保持中、取消し処理中または永続復旧中の、同じ比較範囲に属する値と再一致させない。

ID単体では比較範囲を一意に識別できない場合、本書で定める複合識別条件を使用する。

### 1.7 内部bit幅と通信・保存形式のbit幅

通信形式または永続保存形式で固定幅が確定している項目は、その幅を基本設計上の契約とする。
内部counterは、外部項目へ対応付けるときに切り詰めて、古い値と同じ値にしてはならない。

高頻度で更新する内部generationについて、本書でuint64を指定する場合、RA8M2上で64 bitの単一命令による原子的アクセスを要求するものではない。
snapshotとgenerationをDouble Buffer、短いCritical Sectionまたは同等方式で整合した一組として公開すればよい。

---

## 2. 横断ライフサイクル契約表

### 2.1 System State／Fault／Status

#### 基本属性

| 識別子 | 発行・管理する機能 | 幅・予約 | 比較してよい範囲 |
| --- | --- | --- | --- |
| `stateGeneration` | システム状態管理 | uint32、0予約 | 本体アプリケーションの同一起動内でのみ有効 |
| `transitionGeneration` | システム状態管理 | uint32、0予約 | 本体アプリケーションの同一起動内でのみ有効 |
| `stopStatusGeneration` | 停止・緊急停止管理 | 既存の停止・緊急停止管理の契約、0予約 | 本体アプリケーションの同一起動内でのみ有効 |
| `faultGeneration` | 異常管理 | 既存の異常管理の契約、0予約 | 本体アプリケーションの同一起動内でのみ有効 |
| `faultListGeneration` | 異常管理 | 既存の異常管理の契約、0予約 | 本体アプリケーションの同一起動内でのみ有効 |
| `statusGeneration` | 状態表示・利用者通知 | uint32、0予約 | 本体アプリケーションの同一起動内でのみ有効 |

#### 更新・公開・上限到達時の扱い

| 識別子 | 生成・更新条件 | 他機能へ公開する時点 | リセット・上限到達時の扱い |
| --- | --- | --- | --- |
| `stateGeneration` | 初期現在状態確立時を1とし、現在のシステム状態を新しい状態として正常確定した場合だけ次値へ進める | 新しい`currentSystemState`、operation availability等を含むシステム状態管理のsnapshotを一組として公開する時点 | リセット後は新しい起動で再初期化してよい。数値の周回は禁止する。次値を発行できない場合は新しい状態確定を正常扱いせず、安全側状態を維持し、異常管理へ通知した後に制御された再起動または保守へ移る |
| `transitionGeneration` | 新しい状態移行処理を開始するときだけ次値へ進める。同一遷移中は固定する | `transitionInProgress=true`、source、target、deadline contextを一組として公開する時点 | 状態移行を実行していないときは、値だけを有効な遷移とみなさない。数値の周回は禁止し、上限到達時は新しい通常遷移を開始しない |
| `stopStatusGeneration` | 現在の`StopStatusSnapshot`の意味内容を一組として更新するときだけ進める | `StopStatusSnapshot`を公開する時点 | インターフェース・通信基本設計の既存契約どおり、起動をまたぐ単純比較を禁止する |
| `faultGeneration` | 個々の異常（fault）の公開情報が変更された場合 | 個々の異常情報を公開する時点 | 起動をまたぐ単純比較を禁止し、数値の周回によって古い異常と再一致させない |
| `faultListGeneration` | 管理中の異常（active fault）の集合、または利用者へ公開する項目内容が変化した場合 | 管理中の異常一覧のsnapshotを公開する時点 | 起動をまたぐ単純比較を禁止する。詳細取得中に値が一致しなくなった場合はpage 0から再取得する |
| `statusGeneration` | `UserStatusModel`／`STATUS_SUMMARY`の通知内容が変化した場合 | 状態表示・利用者通知の現在summaryを公開する時点 | BLE接続・認証機能は再採番しない。数値の周回によって同一起動内の古いsummaryと再一致させない |

システム状態管理の`stateGeneration`／`transitionGeneration`について、具体的な保持型および数値周回時の処理を詳細設計へ委ねる旧記述より、本節のuint32・0予約・同一起動内でのみ有効・数値周回禁止を優先する。
詳細設計へ残すのは、C型宣言、原子的な公開方法、snapshot実装および通知方法である。

---

## 3. Power／Electrical Safety／SystemMonitor

### 3.1 電源・省電力管理のpower transaction identifiers

電源・省電力管理が所有する低消費電力移行識別子および通常のpower state change識別子は次を共通契約とする。

- uint32
- 0予約
- 本体アプリケーションの同一起動内でのみ有効
- 新しいlogical power transaction開始時に1回だけ採番
- 同一transactionのPREPARE、STOP、監視切替え、commit、rollback、resultで同じ親IDを維持
- 子機能のrequest IDで親IDを置換しない
- 正常完了、失敗、取消しまたはpreempt後に同じ値を別transactionへ再利用しない
- wrap禁止

次値を表現できない場合は、新しい低消費電力移行または通常power changeを開始せず、現在の安全な電力状態を維持して異常管理へ通知する。低消費電力移行中で枯渇を検出した場合はcommitへ進まず、可能な安全側通常状態へrollbackする。

### 3.2 バッテリー・電気安全監視のmeasurement generation

バッテリー・電気安全監視の高頻度計測で使用する`measurement generation`は次とする。

- **uint64**
- 0予約
- 本体アプリケーションの同一起動内でのみ有効
- 監視対象または派生計測値ごとにownerをバッテリー・電気安全監視とする
- 新しい有効サンプル集合または品質変化を反映したsnapshotをpublishする場合だけ進める
- 同じgenerationの再読出しを新しい測定、継続時間またはIWDT根拠にしない

1 ms周期の計測をuint32で循環させる方式は採用しない。uint64 generationをsnapshot fieldとして整合公開し、詳細設計で下位32 bitだけへ切詰めてconsumerへ提供しない。

### 3.3 バッテリー・電気安全監視/電源・省電力管理のsafety monitor generation

バッテリー・電気安全監視の通常監視／通電維持縮退監視でバッテリー・電気安全監視が発行する正常監視世代、および標準低消費電力監視で電源・省電力管理が発行してバッテリー・電気安全監視が応答へ返す低消費電力安全監視世代は、いずれも次を共通契約とする。

- uint64
- 0予約
- 本体アプリケーションの同一起動内でのみ有効
- `monitorMode`と対応付ける
- 低消費電力監視では親の電源・省電力管理の低消費電力移行識別子と組み合わせる
- 同一世代について一度成立した`LP_MON_COMPLETE`または正常監視結果を別周期の新しい完了として再利用しない
- old mode／old transitionの結果を現在のIWDT更新根拠へ使用しない

### 3.4 IWDT update transaction ID

電源・省電力管理が本体アプリケーション側IWDT ownerとなった後の1回のfeed判定／受付／実行／結果相関には、電源・省電力管理が所有する`IWDT更新取引識別子`を使用する。

- uint64
- 0予約
- 本体アプリケーションの同一起動内でのみ有効
- feed判定を開始するたび新しい値
- 更新元区分、バッテリー・電気安全監視のmonitor generation、本体ソフトアーキテクチャ基本設計 `monitorEvaluationGeneration`、必要な電源・省電力管理のtransition IDと対応付ける
- feed命令を実行したか、拒否したか、結果不明かを同じIDで返す
- 同じIDの重複結果を別feed実行として数えない
- wrapを通常運用で許可しない

### 3.5 SystemMonitor evaluation generation

GR-03で定めたfresh monitor evaluation evidenceの識別には、本体ソフトアーキテクチャ基本設計 runtime ownerの`monitorEvaluationGeneration`を使用する。

- uint64
- 0予約
- 本体アプリケーションの同一起動内でのみ有効
- SystemMonitorがcurrent phaseの必須監視集合を一回すべて評価し、freshness／deadline判定を完了してevidenceをpublishした場合だけ進める
- 一部Processだけの評価、前回結果の再読出しまたは空scanでは進めない
- Power Managementは前回成功feedに使用したgenerationと同じ値を次のfeed根拠へ再利用しない

`monitorEvaluationGeneration`のC構造体、atomic updateおよびscan実装は詳細設計事項だが、上記lifecycleは変更しない。

---

## 4. Actuation Epoch

### 4.1 走行制御の`driveEpoch`

走行制御の`driveEpoch`はBLE protocol 1.0の`actuationEpoch:uint32`へ搬送するため、次を基本設計契約とする。

- uint32
- 0 = 未確立／無効
- 本体アプリケーションの同一起動内でのみ有効
- 最初に有効となる走行操作の情報を1とする
- 走行制御だけが生成・更新する

旧走行指令を失効させる必要がある境界では、次の順序を守る。

1. 新しい通常走行指令の実行を禁止する。
2. `driveEpoch`を次値へ進める。
3. 旧command slot、未実行予約、継続操作情報の旧資格を破棄する。
4. 新しいepoch／validityをBLE接続・認証へ整合したcontextとしてpublishする。
5. 再許可する場合は、アプリが新しい`ACTUATION_CONTEXT`を取得した後の新規利用者入力だけを受付対象とする。

通常停止、安全停止、E-STOP、走行操作許可喪失、走行contextを無効化する設定変更その他走行制御が旧指令失効を必要と判断する境界で更新する。

wrapは許可しない。次値が必要なのに表現できない場合は走行操作を禁止したまま異常管理へ通知し、controlled reboot／保守へ移る。0または過去値へ戻して走行を再許可しない。

走行制御の「`driveEpoch`の具体型・0予約・初期化・wrap処理を詳細設計で決める」とした旧引継ぎ記述は、本節を優先する。

### 4.2 砲塔・砲身制御の`actuationEpoch`

初期製品の砲塔・砲身制御の`actuationEpoch`は、**砲塔旋回軸と砲身上下軸で共通する一つのuint32 epoch**とする。

理由は、BLE接続・認証のprotocol 1.0が砲塔・砲身操作のcommand execution contextとして単一の`actuationEpoch:uint32`を搬送しており、初期製品で軸別epochへ変更するとwire互換性を変更するためである。

lifecycleは次とする。

- uint32
- 0 = 未確立／無効
- 本体アプリケーションの同一起動内でのみ有効
- 砲塔・砲身制御だけが生成・更新する
- 最初の有効な砲塔・砲身操作contextを1とする
- 砲塔または砲身のどちらか一方でも、旧操作資格を失効させる必要がある境界で共通epochを進める
- epoch更新時は両軸の未実行通常指令、未反映PWM予約および開始確認のうち古い操作世代に属するものを失効させる

軸別内部operation IDを持つことは許容するが、BLE通信データ形式の共通epochを軸別IDで置換しない。

wrap時は走行制御と同様に新しい砲塔・砲身操作を許可せず、異常管理への通知後にcontrolled reboot／保守へ移る。

---

## 5. BLE connection／authentication／request

### 5.1 `connectionGeneration`

BLE接続・認証の`connectionGeneration`は次とする。

- uint32
- 0予約
- 本体アプリケーションの同一起動内でのみ有効
- 最初のBLE linkまたはBLE module利用可能contextを1とする
- 新しいBLE link確立、BLE module再初期化、または以前の接続を安全に継続できない復旧境界で次値へ進める
- 新値をpublishする前に旧connection-bound challenge、nonce、auth attempt、認証用の証明データの検証成功状態、temporary session key、fragment、受信途中／送信待ち状態および操作セッションを失効させる

wrapは許可しない。次の接続世代を表現できない場合、BLE接続・認証は新しい利用可能BLE link／認証セッションを成立させず、操作中なら安全停止連携を行い、異常管理への通知後にcontrolled reboot／保守へ移る。

### 5.2 `authAttemptId`

BLE接続・認証の通常アプリ認証および登録経路で用いる接続別認証試行IDは、各protocolで既に固定されたuint32 fieldを使用する。

共通lifecycleは次とする。

- uint32
- 0予約
- connection-local
- 各`connectionGeneration`で最初の試行を1とする
- 新しいchallenge／nonceを発行する新規認証試行ごとに進める
- timeout、失敗または成功後に同じ試行IDへ新しいchallengeを割り当てない
- 有効な認証試行の複合識別は`(connectionGeneration, authAttemptId)`とする

同一connection内で次値を表現できない場合は当該BLE linkを終了し、新しい`connectionGeneration`の接続から再開する。0へwrapしない。

### 5.3 `sessionId`

BLE接続・認証の`sessionId`は既存どおり64 bitの暗号用途乱数を使用し、0を未認証として予約する。

セッションの有効範囲は現在のBLE接続であり、内部相関では`(connectionGeneration, sessionId)`をsession identityとする。数値`sessionId`だけをboot全体または製品寿命全体の単調IDとして扱わない。

新しいセッション生成時に0は拒否し、現在connectionで既に使用中の値と一致する場合は再生成する。旧connectionのsessionは`connectionGeneration`変更時に必ず失効するため、数値一致だけで旧セッションを復活させない。

### 5.4 `commandSeq`

BLE接続・認証の既存契約を維持する。

- uint32
- session-local
- 1開始
- 0禁止
- wrap禁止
- 上限到達前にsessionを終了し再認証する

有効COMMANDの新旧判定は`(connectionGeneration, sessionId, commandSeq)`および必要な`authorizationRevision`／actuation epochを組み合わせる。

### 5.5 BLE `requestId`

BLE protocol 1.0の`requestId`はuint32 通信データの項目とし、domain transaction IDとは分離する。

- 0はunsolicited通知またはrequest correlation不要messageで使用可能
- responseを必要とする離散要求、fragment transaction、fault detail query等では非0を必須とする
- 認証済み通常要求のscopeは`(sessionId, requestId)`
- 未認証登録通信のscopeは`(connectionGeneration, requestId, registrationSubtype)`
- 同一logical BLE requestのexact retryは同じ`requestId`を使用する
- 同一scopeで処理中／結果保持中の`requestId`を異なる要求へ再利用しない

アプリ側で非0 `requestId`を次に採番できず、保持中IDとのaliasを避けられない場合はcurrent sessionで新規離散要求を開始せず、再認証により新しいsession scopeへ移る。

---

## 6. 所有者・操作端末管理のmanagement transactionとBLE requestの相関

### 6.1 `managementTransactionId`

所有者・操作端末管理の`managementTransactionId`は、所有者・端末管理domain上の一つの管理操作を識別するIDであり、BLE transport request IDではない。

初期製品では次を採用する。

- uint64
- 0予約
- 本体アプリケーションの同一起動内でのみ有効
- 所有者・操作端末管理だけが新規発行する
- 初回OWNER登録、GENERAL追加、OWNER再登録、端末削除／失効、工場初期化等の新しい管理transaction開始時に新規発行する
- 同じtransactionの計画的BLE接続切替え、proof前再試行、永続データ管理のcommit、セキュリティ管理のcredential処理および後処理では同じIDを維持する
- 完了、取消し、期限切れまたは中止後に別transactionへ再利用しない

ログまたはreset後の診断で一意に識別する必要がある場合は、`(bootInstanceId, managementTransactionId)`を使用する。通常の管理transactionをreset後へそのまま継続することを意味せず、永続commit後処理は管理元機能が永続化した`ownerDataRevision`等の情報に基づいて復旧する。

### 6.2 BLE接続・認証の`requestId`との関係

BLE接続・認証の`requestId`と所有者・操作端末管理の`managementTransactionId`は**等値を要求しない別ID**とする。

相関は次で管理する。

```text
(connectionGeneration, requestId)
            |
            +--> managementTransactionId
            +--> registrationSubtype / operation
            +--> authAttemptId（必要な場合）
```

GENERAL追加でOWNER接続から新GENERAL接続へ計画的に切り替える場合、所有者・操作端末管理の`managementTransactionId`は同一値を維持できるが、`connectionGeneration`は更新し、新接続のBLE request／auth attemptは新しいconnection scopeで生成する。

旧接続の`requestId`、challenge、nonce、auth attempt、認証用の証明データの検証成功状態、temporary session keyまたはfragment状態を、新接続で同じ`managementTransactionId`だからという理由でcurrent transport stateへ昇格させない。

### 6.3 `managementTransactionId`のwrap

uint64上限到達を通常運用では想定しないが、wrapは許可しない。次値を表現できない場合は新規管理変更を開始せず、最後の正常commit済みowner dataを維持して異常管理および利用者通知へ保守必要状態を提供する。

所有者・操作端末管理ですでに確定している`ownerDataRevision`および`authorizationRevision`のwrap禁止・保守要求規則は変更しない。

---

## 7. 永続revision／boot instanceとの関係

### 7.1 永続revision

所有者・操作端末管理の`ownerDataRevision`、設定管理の`highestCommittedGeneration`、セキュリティ管理の`wifiCredentialRevision`／SecurityGen関連floor、製品情報管理の`productDataRevision`／`highestCommittedProductDataRevision`、時刻管理の`timeTrustRevision`その他の永続revisionは、その事項を定める基本設計で定める単調性・rollback floorに従う。

これらは同一起動内でのみ有効な世代と異なり、通常rebootだけを理由として1へ戻さない。破損、fallbackまたは古い保存用の複製発見だけを理由として小さいデータ内容の版をcurrentへ昇格させない。

### 7.2 `bootInstanceId`

ログ・診断の`bootInstanceId`は起動単位の横断相関に使用する既存uint64単調識別子である。本体アプリケーションの同一起動内でのみ有効なIDをログ・診断でboot越しに扱う場合は、必要に応じて`bootInstanceId`と組み合わせる。

同一起動内でのみ有効なIDを永続化して次bootのcurrent IDとして復元するために`bootInstanceId`を使用するものではない。

---

## 8. 情報の提供側・利用側の規則

### 8.1 情報を提供する機能

識別子・世代番号を発行・管理する機能は、新しい値を現在の正式値として公開するとき、少なくとも次の情報を、同じ状態情報一式または相互に対応付けられる処理結果に含める。

- 識別子・世代番号（identifier／generation）。
- 当該機能が管理する状態または処理結果。
- 有効性・品質（validity／quality）。
- 必要な上位の処理（parent transaction）の識別情報。
- 必要な適用範囲の識別情報（起動、接続、セッション、モードなどのscope identifier）。
- 必要な単調増加時刻（monotonic time）。

### 8.2 情報を利用する機能

情報を利用する機能は、次を守る。

- 有効な比較範囲が異なるIDを、数値の大小だけで比較しない。
- 同じ世代番号の状態情報一式を、新しい進捗を示す情報として扱わない。
- 結果に対応する要求・取引・適用範囲が現在のものと一致しない場合、その結果を現在の処理の完了判断に使用しない。
- `UNKNOWN`または対応する処理を特定できない結果を、成功と推測しない。
- 最新値を取得できないことを理由に、古い世代を現在の正式値へ戻さない。

### 8.3 リセット・有効範囲の変更時

リセットまたは識別子の有効な範囲（scope）の変更時に破棄する揮発性の情報には、ID値だけでなく、そのIDに対応する次の情報を含む。

- 処理中の要求・結果（pending request/result）。
- 再試行の管理情報（retry metadata）。
- 処理期限（deadline）。
- challenge／nonce／proofの状態。
- 分割データの受信・再構成状態（fragment/reassembly state）。
- 指令の保持領域（command slot）。
- 進行確認の情報（checkpoint/evidence）。
- 一時鍵および処理条件（temporary key/context）。

機能側で管理する一連の処理のうち、担当の基本設計が永続情報を使った復旧の対象として明示したものだけを復旧する。
復旧には、その処理の管理元機能が永続化した情報を使用する。

---

## 9. 詳細設計へ残す範囲

詳細設計で具体化してよい事項は次とする。

- C/C++のtypedef、struct、enum数値
- uint64のread/writeを整合させるCritical Section、Double Buffer、seqlock相当方式
- counter increment関数
- ID allocator API
- snapshot buffer配置
- atomic／memory barrier実装
- retained RAM／MRAM上のphysical layout
- mapping table／lookup table
- diagnostic表示形式
- 異常管理へ通知する内部event code

詳細設計だけで次を変更しない。

- owner
- scope
- 0予約
- 本書で固定したbit幅
- generation／IDのsemantic更新条件
- publish commit点
- wrap禁止
- 枯渇時の安全側処理
- 砲塔・砲身制御の砲塔・砲身共通epoch
- 所有者・操作端末管理の`managementTransactionId`とBLE接続・認証の`requestId`の分離
- バッテリー・電気安全監視の高頻度generationおよびSystemMonitor evidenceのuint64方針

---

## 10. 検証条件

詳細設計・実装・結合評価では少なくとも次を確認する。

1. 同一起動内でのみ有効な各世代が0を有効値として公開しない。
2. reset後、旧bootの遅延result／snapshotを新bootの現在値へ使用しない。
3. システム状態管理のstate／transition snapshotでpayloadとgenerationが混在しない。
4. バッテリー・電気安全監視の1 ms計測を長時間動作させてもgeneration aliasを生じない。
5. 同じバッテリー・電気安全監視のgeneration再読出しで継続時間やIWDT根拠を進めない。
6. GR-03の`monitorEvaluationGeneration`を前回IWDT feedから再利用できない。
7. 走行制御の停止・Safety境界で`driveEpoch`更新前の指令が実行されない。
8. 砲塔・砲身制御で砲塔または砲身の失効境界により共通`actuationEpoch`が更新され、両軸の旧通常操作資格が残らない。
9. `connectionGeneration`変更で旧challenge、auth attempt、session、fragment、request状態が破棄される。
10. `(connectionGeneration, authAttemptId)`が異なる認証試行を混同しない。
11. `commandSeq`／BLE `requestId`がscope内でwrapして旧値へ一致しない。
12. GENERAL追加の計画的接続切替えで`managementTransactionId`だけが継続し、旧BLE request／auth stateを引き継がない。
13. 所有者・操作端末管理の`managementTransactionId`とBLE接続・認証の`requestId`の数値一致／不一致のどちらでも正しいmappingで相関できる。
14. generation／ID枯渇をfault injectionした場合、0／小さい値へwrapせず新規処理を安全側に拒否できる。
15. persistent revisionがreboot、physical fallbackまたは破損を理由に巻き戻らない。

---

## 11. GR-04完了条件

本書により、GR-04で指摘された次の不足を基本設計として確定した。

- owner
- scope
- 未確立値
- bit幅が外部相互運用性または長時間動作に影響するIDの幅
- 生成／更新条件
- publish commit点
- boot/reset境界
- wrap／枯渇時の安全側処理
- in-flight旧値との再一致禁止
- 砲塔・砲身制御の砲塔・砲身共通`actuationEpoch`
- 所有者・操作端末管理の`managementTransactionId`とBLE `requestId`のmapping関係
- バッテリー・電気安全監視の高頻度generationのuint64化
- GR-03 SystemMonitor evidenceのgeneration lifecycle

既存文書に残る旧詳細設計分類は本契約を未決定へ戻さない。本書で確定したlifecycle事項を詳細設計事項へ戻さず、具体データ構造・API・atomic実装だけを詳細設計へ残す。

---

**文書終端：Tank_Robotの識別子・世代・epoch・transaction IDについて、owner、scope、更新、publish、reset、wrap、複合相関および旧値再利用禁止を横断契約として確定する。**

---

## 作成経緯

基本設計粒度レビューのGR-04への対応として、本書の補完対象を基本設計で確定した。
