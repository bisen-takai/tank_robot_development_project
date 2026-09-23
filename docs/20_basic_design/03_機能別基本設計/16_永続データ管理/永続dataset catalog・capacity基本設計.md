# Tank_Robot 永続データ一覧・容量基本設計

本書は、永続保存するデータの一覧と、各データに必要な保存容量を定める。保存・更新の単位として扱うデータのまとまりを「データ集合（dataset）」と呼ぶ。

データ集合ごとの規則・必要容量、`PERSIST_CRITICAL`の32 KiBに収まることの設計上の確認、および外付けSerial NORの完全性確認・認証方式を、基本設計として確定する。

**関連する設計**

[「永続データ管理基本設計」](永続データ管理基本設計.md)、[「所有者・操作端末管理基本設計」](../10_所有者・操作端末管理/所有者・操作端末管理基本設計.md)、[「設定管理基本設計」](../13_設定管理/設定管理基本設計.md)～[「時刻管理基本設計」](../20_時刻管理/時刻管理基本設計.md)、[「セキュリティ横断設計」](../../06_セキュリティ横断設計/Tank_Robotセキュリティ横断設計.md)。

**本書と関連文書の適用範囲**

1. **本書が定める事項**：永続データ集合の一覧・容量配分、PERSIST_CRITICALの32 KiBに収まることの設計上の確認、および外付けSerial NORの完全性確認・認証方式。
2. **関連文書が引き続き定める事項**：本書が明示的に補完する事項、または文書間で共通の規則として確定する事項以外は、本書で挙げる関連する基本設計の担当範囲と変更責任を維持する。
3. **本書を優先する場合**：上記の事項について本書で基本設計として確定した記述と、当該事項への対応前の旧記述が競合する範囲に限る。対応経緯と、比較基準の版がある場合の識別情報は、末尾の「作成経緯」に示す。
4. **本書を優先しない事項**：詳細設計へ残す内部実装、および補完対象外の状態、機能の動作、安全、セキュリティ、外部との通信・連携規則などは、それぞれを担当する基本設計に従う。

永続データ管理基本設計（B16）で定義済みの永続保存方式、各データを管理する機能の担当範囲、巻戻し・代替データへの切替えの原則は変更しない。`PERSIST_CRITICAL`の32 KiBと、外付けSerial NORの8 MiB以上という構成も維持する。

永続データ管理全体の方式は、引き続き永続データ管理基本設計で定める。本書は、同基本設計の第5.3節のデータ集合一覧、第4.2節・第6.1節の重要データ用領域の容量、第28.2節のE-03、第28.3節、第30.2節、およびセキュリティ横断設計（XSEC）の第26.2節・項21を補完する。

比較基準コミット以前の文書に次の記述が残る場合は、本書の補完対象について本書を優先する。

- データ集合ごとの一覧に記載する値を、詳細設計で新規に決定する記述。
- `PERSIST_CRITICAL`の32 KiBに収まるかどうかを、実機・実装評価で初めて判断する記述。
- 重要データの集合ごとの容量を、容量確認後に詳細設計で初めて決定する記述。
- 外付けSerial NORに保存するデータ集合ごとの完全性確認・認証方式を、詳細設計で新規に選択する記述。

本書で確定したデータ集合一覧、容量配分、完全性確認・認証方式の対応表を、詳細設計で決める事項へ戻さない。

---

## 1. 設計目的と詳細設計との分担

### 1.1 基本設計で固定するもの

本書では次を基本設計として固定する。

- データ集合を識別する名称（`datasetId`）
- データの意味や更新条件を管理する機能
- 保存の確定を要求する機能と、物理的な書込みを行う機能の担当範囲
- 保存先の区分
- データ本体（論理ペイロード）の容量上限
- 更新を全体として成立させる原子的更新の方式
- 完全性確認・認証方式
- 巻戻し・代替データへの切替えの規則
- 工場出荷状態への初期化時の扱い
- 起動上の重要度
- `PERSIST_CRITICAL`の各データ集合に物理的に予約する容量の上限
- `PERSIST_CRITICAL`の総量32 KiBに必要容量が収まることの設計上の確認
- 外付けSerial NORの各領域・データ集合に適用する完全性確認・認証方式

### 1.2 詳細設計へ残すもの

次は、基本設計上の意味を変えずに実装へ対応付ける事項として、詳細設計へ残す。

- 名称で定めた`datasetId`に割り当てる列挙型の数値
- MRAMの物理アドレス、オフセット、リンカーセクション
- A/Bの各コピーを配置する具体的なアドレス
- 構造体・型定義と、各項目のバイト位置
- バイトの並び順（エンディアン）
- 256 byte以下の実際の配置境界・処理単位に合わせた最適化
- FSPのドライバAPI、排他区間、キュー、コールバック
- HMAC／AEADのAPI呼出しと鍵ハンドル
- 外付けNORの消去ブロック・セクタに対応する、各領域の具体的なオフセット

容量を収めるために、詳細設計だけでデータ本体の容量上限、コピー数、巻戻し・代替データへの切替えの規則、工場初期化時の処置、完全性確認・認証方式を変更してはならない。

---

## 2. データ集合一覧の共通規則

### 2.1 一覧の定義と固定範囲

初期製品のデータ集合一覧は、実行中に変更する汎用の表ではなく、設計・ビルド時に固定する一覧とする。

本書で定めるデータ集合名を、各データを識別するための基準とする。詳細設計で数値IDを割り当てても、名称が示すデータの意味は変更しない。

### 2.2 保存コピーの復旧と、使用するデータの切替え

永続データ管理基本設計（B16）の既存方針を維持する。同じ状態を表す保存コピーを復旧することと、PREVIOUS／FACTORYなどを現在使用するデータとして選ぶことを区別する。

- `SAME_PERSIST_REVISION_ONLY`：同じ状態を表す、冗長化された物理コピーの復旧だけを許す。
- `NO_OLDER_SEMANTIC_REVISION`：一度現在の状態として採用した新しい版を、破損だけを理由に古い版へ戻さない。
- `OWNER_DECIDES`：PREVIOUS／FACTORYなどを現在使用するデータにできるかどうかを、そのデータの管理機能が既存の取引規則に従って判断する。

`persistRevision`は、永続データ管理機能が管理する物理的な保存の確定世代である。各データの管理機能が定める、内容・状態の版とは別に扱う。

### 2.3 レコードの共通管理情報に予約する容量

`PERSIST_CRITICAL`の必要容量を設計上確認する際は、A/Bの物理レコード1個当たり、共通管理情報用として**128 byte**を見込む。

この128 byteには、少なくとも次を収める設計余裕を含む。

- データ集合・種別の識別情報
- `formatVersion`
- データ本体の長さ
- `persistRevision`
- フラグ
- ヘッダーのCRC32C
- データ本体のCRC32C、または完全性確認結果への参照
- 保存の確定を示すマーカー
- HMAC-SHA256タグ、または同等の32 byteの完全性確認用タグが必要なデータ集合のタグ領域
- 将来の互換性確保のために予約するヘッダー項目

RSIP Wrapped KeyやAEADの暗号文・タグなど、データ集合固有のバイナリデータの長さは、データ本体の容量へ計上する。共通管理情報の容量に含めて、データ本体の容量から除外しない。

A/B方式の1コピーに予約する容量は、次の式で算出する。

`copyReservation = ALIGN_UP(maxPayloadLength + 128, 256)`

A/B二面を合わせた物理的な予約容量は、次の式で算出する。

`datasetReservation = 2 × copyReservation`

256 byteの配置単位は、容量確認で余裕を見込むための論理的な予約単位である。詳細設計で実際の記憶媒体の処理単位に合わせて小さくしてもよいが、本書で定める総容量を超えてよい理由にはしない。

---

## 3. `PERSIST_CRITICAL`に保存するデータ集合の一覧

### 3.1 保存対象・容量と保護規則

データ管理機能は、データの意味や更新・復旧条件を定める。保存・参照経路は、記憶媒体への書込み処理や固定ブートからの読出し経路を示す。両者を分けて記載する。

以下の二つの表は、同じデータ集合識別子で対応付ける。
「通常保存」は、永続データ管理機能（B16）の本体アプリケーション側の永続保存処理を指す。「固定ブート最小経路」は、固定ブートで必要な最小限の保存・参照経路を指す。

#### 管理機能・保存経路・容量

データ本体の容量上限と、冗長化・共通管理情報を含む物理的な予約容量は区別する。物理的な予約容量は第4.1節に示す。

| データ集合識別子 | データ管理機能 | 保存・参照経路 | データ本体の容量上限 | 原子的更新の方式 |
| --- | --- | --- | --- | --- |
| `CRIT_MIN_SAVE` | 起動・終了・再起動管理（B02） | 通常保存 | 1024 B | A/B |
| `CRIT_LIFECYCLE_RESET` | 所有者・操作端末管理（B10） | 通常保存。必要時は固定ブートから参照 | 256 B | A/B |
| `CRIT_OWNER_AUTH` | 所有者・操作端末管理（B10） | 通常保存 | 1024 B | A/B |
| `CRIT_ROLLBACK_FLOOR` | 各下限値の管理機能（B13／B17／B18／B19） | 通常保存。固定ブート最小経路からの読出し・書込み対象を含む | 256 B | A/B |
| `CRIT_WIFI_SECURITY` | セキュリティ管理（B17）。Wi-Fi設定の意味はWi-Fi接続（B11） | 通常保存 | 256 B | A/B |
| `CRIT_DEVICE_TRUST` | セキュリティ管理／VPS接続・デバイス認証（B17／B12） | 通常保存、および初期情報の書込み・有線保守の管理された経路 | 512 B | A/B |
| `CRIT_PRODUCT_DATA` | 製品情報管理（B18） | 通常保存／認可された保守経路 | 4096 B | A/B |
| `CRIT_TIME_TRUST_GROUP` | 時刻管理（B20） | 通常保存 | Trusted Time Record＋Time Update Recordの合計1024 B以下 | 各レコードをA/Bで保存＋時刻管理（B20）の取引 |
| `CRIT_OTA_BOOT` | OTA更新管理／固定ブート・復旧（B15／B19） | 通常保存＋固定ブート最小経路 | 512 B | A/B |
| `CRIT_FAULT_RESET_LOOP` | 異常管理／起動・終了・再起動管理（B04／B02） | 通常保存。固定ブートからの参照も可 | 256 B | A/B |
| `CRIT_BOOT_INSTANCE_JOURNAL` | ログ・診断（B14） | 永続データ管理（B16）／最小起動経路 | 記録項目は小容量。物理的な予約容量は2048 B固定 | 上限付きジャーナル（更新履歴） |
| `CRIT_CONFIG_COMMIT` | 設定管理（B13） | 通常保存 | 256 B | A/B |

#### 保護・復旧・工場初期化時の扱い

巻戻し・代替データへの切替えに使用する規則名は、第2.2節の定義に従う。データ集合固有の条件は、表の後の補足に示す。

| データ集合識別子 | 完全性・機密性の保護 | 巻戻し・代替データへの切替え | 工場初期化時の扱い | 起動上の重要度 |
| --- | --- | --- | --- | --- |
| `CRIT_MIN_SAVE` | CRC32C | `OWNER_DECIDES` | 保持／更新 | HIGH |
| `CRIT_LIFECYCLE_RESET` | CRC32C + `K_SEC_STATE_MAC` | `NO_OLDER_SEMANTIC_REVISION` | 初期化処理の完了まで保持 | HIGH |
| `CRIT_OWNER_AUTH` | CRC32C + `K_SEC_STATE_MAC` | `NO_OLDER_SEMANTIC_REVISION` | 所有者・端末の認証情報を工場出荷時の既定状態へ戻す | HIGH |
| `CRIT_ROLLBACK_FLOOR` | CRC32C + `K_SEC_STATE_MAC` | `NO_OLDER_SEMANTIC_REVISION` | 保持 | CRITICAL |
| `CRIT_WIFI_SECURITY` | CRC32C + `K_SEC_STATE_MAC` | `NO_OLDER_SEMANTIC_REVISION` | 鍵・ノンスの世代を切り替え、現在のWi-Fi秘密情報を失効 | HIGH |
| `CRIT_DEVICE_TRUST` | CRC32C + `K_SEC_STATE_MAC` | `NO_OLDER_SEMANTIC_REVISION` | 保持 | CRITICAL |
| `CRIT_PRODUCT_DATA` | CRC32C + `K_SEC_STATE_MAC` | `NO_OLDER_SEMANTIC_REVISION` | 保持 | CRITICAL |
| `CRIT_TIME_TRUST_GROUP` | CRC32C + `K_SEC_STATE_MAC` | `NO_OLDER_SEMANTIC_REVISION` | 保持 | HIGH |
| `CRIT_OTA_BOOT` | CRC32C + `K_SEC_STATE_MAC` | `OWNER_DECIDES` | 現在のファームウェアと最終確定処理の状態を保持。初期化だけで巻き戻さない | CRITICAL |
| `CRIT_FAULT_RESET_LOOP` | CRC32C + `K_SEC_STATE_MAC` | `OWNER_DECIDES` | データ管理機能の規則に従い、安全側へ初期化 | HIGH |
| `CRIT_BOOT_INSTANCE_JOURNAL` | CRC32C + `K_SEC_STATE_MAC` | 小さい過去値へ巻き戻さない | 保持 | MEDIUM |
| `CRIT_CONFIG_COMMIT` | CRC32C + `K_SEC_STATE_MAC` | `OWNER_DECIDES` | 設定管理（B13）の工場初期化・既定設定への復元取引に従う | HIGH |

#### データ集合ごとの補足

**`CRIT_MIN_SAVE`**：巻戻し・代替データへの切替えは、起動・終了・再起動管理（B02）と異常管理（B04）が定める終了・復旧の意味に従う。

**`CRIT_LIFECYCLE_RESET`**：`RESET_PENDING`などを古い状態へ戻さない。工場初期化では、その初期化処理自身の情報として完了まで保持する。

**`CRIT_OWNER_AUTH`**：認証用データには、セキュリティ管理（B17）が定める保護形式（Wrapped形式を含む）を用いる。失効済みの認証情報を復活させない。

**`CRIT_WIFI_SECURITY`**：`K_WIFI_STORE`はWrapped Keyとする。古い`wifiCredentialRevision`へ戻さない。

**`CRIT_DEVICE_TRUST`**：秘密鍵はRSIP Wrapped Keyとする。

**`CRIT_PRODUCT_DATA`**：古いProduct Dataへの自動切替えを禁止する。

**`CRIT_TIME_TRUST_GROUP`**：状態を確認できない場合は`TIME_UNTRUSTED`とする。

**`CRIT_OTA_BOOT`**：状態遷移を決めるのは、OTA更新管理（B15）と固定ブート・復旧（B19）の試行起動・復旧取引だけとする。

**`CRIT_FAULT_RESET_LOOP`**：巻戻し・代替データへの切替えは、異常管理（B04）と起動・終了・再起動管理（B02）の自動復旧・リセット繰返し時の規則に従う。

**`CRIT_CONFIG_COMMIT`**：`CONFIG_COMMIT_PENDING`の取引に従い、`highestCommittedGeneration`を低下させない。

### 3.2 `CRIT_ROLLBACK_FLOOR`の内容

少なくとも次の、巻戻しによって意味が変わる値を収容可能とする。

- 現在の許可済み`SecurityGen`
- `highestCommittedGeneration`
- `highestCommittedProductDataRevision`
- 所有者情報・セキュリティ情報の失効管理に必要な単調増加の版のうち、所有者情報のデータ集合とは分離して保持する下限値
- 固定ブートがアプリケーション起動前に必要とする、巻戻し防止用の最小管理情報

`bootInstanceId`は書込み頻度とjournal性を分離するため`CRIT_BOOT_INSTANCE_JOURNAL`へ置く。

### 3.3 `CRIT_WIFI_SECURITY`と外付け`WIFI_STORE`の分離

`CRIT_WIFI_SECURITY`へ置くのは、B11/B17がrollback、key/nonce reuse防止へ必要とする小容量metadataである。

- `K_WIFI_STORE` Wrapped Keyまたはkey handle/ref
- `wifiNoncePrefix`／key epoch
- `wifiCredentialRevision`
- format/profile metadata

SSID、security mode、passphrase等を含むWi-Fi設定本体は外付け`WIFI_STORE`のAES-256-GCM protected blobとする。

### 3.4 Device ID

Device ID文字列を`PERSIST_CRITICAL`へ重複保存しない。B18が管理するRA8M2 Unique ID由来の情報から、同じ入力には常に同じ値が得られる規則で生成する既存方針を維持する。

---

## 4. `PERSIST_CRITICAL`の必要容量と32 KiBへの収容確認

### 4.1 物理的な予約容量

| データ集合 | データ本体の最大容量 | 容量の算出方法 | 物理的な予約容量 |
| --- | ---: | --- | ---: |
| `CRIT_MIN_SAVE` | 1024 B | B16既存4 KiB reservation | 4096 B |
| `CRIT_LIFECYCLE_RESET` | 256 B | 2 × ALIGN256(256+128) | 1024 B |
| `CRIT_OWNER_AUTH` | 1024 B | 2 × ALIGN256(1024+128) | 2560 B |
| `CRIT_ROLLBACK_FLOOR` | 256 B | 2 × ALIGN256(256+128) | 1024 B |
| `CRIT_WIFI_SECURITY` | 256 B | 2 × ALIGN256(256+128) | 1024 B |
| `CRIT_DEVICE_TRUST` | 512 B | 2 × ALIGN256(512+128) | 1536 B |
| `CRIT_PRODUCT_DATA` | 4096 B | 2 × ALIGN256(4096+128) | 8704 B |
| `CRIT_TIME_TRUST_GROUP` | 合計1024 B以下 | 2 recordそれぞれA/B。split/alignment最悪値を予約 | 3072 B |
| `CRIT_OTA_BOOT` | 512 B | 2 × ALIGN256(512+128) | 1536 B |
| `CRIT_FAULT_RESET_LOOP` | 256 B | 2 × ALIGN256(256+128) | 1024 B |
| `CRIT_BOOT_INSTANCE_JOURNAL` | bounded entries | dedicated bounded journal | 2048 B |
| `CRIT_CONFIG_COMMIT` | 256 B | 2 × ALIGN256(256+128) | 1024 B |
| **合計予約** |  |  | **28672 B = 28 KiB** |
| **未割当reserve** |  |  | **4096 B = 4 KiB** |
| **`PERSIST_CRITICAL`総量** |  |  | **32768 B = 32 KiB** |

### 4.2 設計上の容量確認結果

`PERSIST_CRITICAL` 32 KiBに対し、現行基本設計の全critical logical datasetを保守的な256 byte alignmentと共通128 byte overheadを含めて計上しても、**28 KiBで収容可能**である。

**4 KiBを未割当reserveとして残す。**

したがってGR-06時点で、B18 Unit Product Data、B20 Trusted Time／Time Update、B17 trust/security metadata、B15/B19 OTA/boot/recovery metadataを含む32 KiB architectureは静的に成立する。

### 4.3 予備領域の使用条件

4 KiB reserveはruntimeの動的拡張領域ではない。

次の場合にのみ、基本設計を更新した上でdataset reservationへ割り当てる。

- 既存の定義元の基本設計で新しいcritical fieldが正式追加された
- 実装上必須のrecord overheadが128 byteを超えることが判明した
- MRAM program granularity等により256 byteより大きいreservation単位が必須となった
- 新しいrollback/security/safety critical datasetが追加された

詳細設計の都合だけでreserveを消費してcatalog外datasetを追加しない。

### 4.4 E-03の扱い

B16 §28.2 E-03およびB17側の同趣旨の「32 KiB capacity proof」は、**基本設計上の静的成立確認として本書で完了**とする。

後続評価では、容量そのものの採否判断ではなく次を確認する。

- linker/map上の実割当てが各reservation以内であること
- 実際のrecord header/tag/alignmentが本書budget以内であること
- MRAM write/readbackが選定FSP構成で成立すること
- program時間、CPU stall、interrupt影響、enduranceが既存deadlineを満たすこと

実装結果がreservationを超える場合、詳細設計だけでcopy削減、integrity削減、critical data外付け移動を行わず、B16基本設計へ戻す。

---

## 5. 外付けSerial NORの完全性確認・認証方式一覧

### 5.1 データ集合ごとの保護方式

| 保存領域／データ集合 | データ管理機能 | 完全性確認／認証 | 巻戻し・過去データの再使用への対策 | 秘密保持 | 利用前の必須判定 |
| --- | --- | --- | --- | --- | --- |
| `OTA_CANDIDATE` | B15/B19 | イメージとメタデータをSHA-256およびECDSA P-256署名で保護 | `SecurityGen`、製品／ハードウェア／Boot Interface、試行起動の状態 | 不要 | B15/B19による署名・ハッシュ・互換性の検証成功後だけ使用 |
| `OTA_PREVIOUS` | B15/B19 | イメージとメタデータをSHA-256およびECDSA P-256署名で保護 | 現在の許可済み`SecurityGen`以下の禁止条件、復旧処理との対応 | 不要 | B19が定める復旧条件と署名検証が成立した後だけ使用 |
| `CONFIG_STORE` | B13 | `K_CONFIG_MAC`によるHMAC-SHA256と形式・スキーマの検証 | generationと`highestCommittedGeneration` | 原則不要 | B13によるHMAC、Device ID、スキーマ、generation、および設定内容の妥当性の検証 |
| `WIFI_STORE` | B11/B17 | AES-256-GCM AEAD | `wifiCredentialRevision`と鍵／nonceの世代。古い認証情報へ自動的に戻さない | 必須 | 現在のrevisionでAEAD認証・復号に成功した後だけ使用 |
| `IMPORTANT_LOG_RING` | B14 | レコードのCRC32Cと、確定後のレコードの不変性・保持状態を管理するメタデータ | logId／bootInstanceId／リング内の保持状態の管理 | 秘密情報を記録しない。追加暗号化は初期製品の必須条件ではない | CRC、レコードの長さ・種別、およびリングのメタデータの整合 |
| `NORMAL_LOG_RING` | B14 | レコードのCRC32Cと、確定後のレコードの不変性・保持状態を管理するメタデータ | logId／bootInstanceId／リング内の保持状態の管理 | 秘密情報を記録しない。追加暗号化は初期製品の必須条件ではない | CRC、レコードの長さ・種別、およびリングのメタデータの整合 |
| `LOG_META_JOURNAL` | B14 | CRC32Cと、処理範囲を限定したジャーナル／チェックポイントの整合 | 古い参照位置だけを根拠にSENTや現在状態を作り出さない | 不要 | ジャーナルの再構築規則とログレコードの照合 |
| `PRODUCT_SEC_AUX` | B12/B17/B18 | データ種別に応じた証明書・署名・証明書チェーンの検証。保存領域・レコードの物理破損はCRC等で検出 | 信頼情報の版と識別情報を、MRAMの重要メタデータと照合 | 原則としてPUBLIC情報／補助データ。秘密情報を置かない | データ管理機能／B17による検証成功後だけ利用 |
| `BULK_RESERVE` | なし | **未割当** | 未割当 | 未割当 | データ集合の一覧へ追加する前は、実行時のデータを書き込まない |

### 5.2 ログのセキュリティ上の位置付け

初期製品のlog ringはCRC32Cをbit破損／torn record検出に使用し、cryptographic MAC付き監査台帳を必須としない。

理由は、logを認証・認可・Safety Gate・rollback floorの正式な判断根拠として使用しないためである。物理攻撃者がSerial NORを任意改変できる場合のlog証拠性には限界があるが、その改変結果から権限・Safety状態を復活させない。

将来tamper-evident audit logを要求する場合は、詳細設計だけでHMACを追加せず、B14/XSEC基本設計へ戻してkey lifecycle・性能・容量を再評価する。

### 5.3 `PRODUCT_SEC_AUX`

`PRODUCT_SEC_AUX`はcertificate chain等の比較的大きい非秘密／security補助data用であり、raw private key、`K_CONFIG_MAC`、`K_SEC_STATE_MAC`、terminal key、Wi-Fi passphrase等の秘密情報を平文保存しない。

trust root級現在状態は`PERSIST_CRITICAL`のB17所有metadataと照合し、external object単独をtrust rootにしない。

---

## 6. 起動・工場初期化・セキュリティで維持する保存規則

### 6.1 起動時

fixed bootは、B19がApplication前に必要とするdatasetだけをB16 minimum access pathから読む。

- 現在の許可済み`SecurityGen`
- OTA/trial/recovery state
- Product/compatibilityに必要なcritical metadata
- reset/failure判断に必要なbounded state

外付けSerial NORの全partition scanを正常boot条件にしない。

### 6.2 工場出荷状態への初期化

工場出荷状態への初期化では、物理領域全体を消去せず、論理データ集合ごとの方針に従って処理する。

- 所有者・端末の認証情報：削除／工場出荷値へ戻す
- Wi-Fiの秘密情報：削除し、`K_WIFI_STORE`とnonceの世代を更新する
- 製品・デバイスの信頼情報：保持する
- 巻戻し防止の下限値：保持する
- Product Data：保持する
- Trusted Time：保持する
- ファームウェアの信頼情報／SecurityGen：保持する
- 未完了の初期化処理の管理情報：完了まで保持する

### 6.3 古い状態への巻戻しを防ぐデータ

少なくとも次はCRC正常だけで古い版のデータを現在の有効データとして扱わない。

- SecurityGen
- 所有者情報・失効状態
- highestCommittedGeneration
- Wi-Fi credential revision/key epoch
- highestCommittedProductDataRevision
- Product Data
- Trusted Time／Time Update
- OTA・試行起動・復旧の状態のうち、担当機能が単調な変更または一方向の遷移を要求するもの

---

## 7. 検証契約

GR-06後の検証では少なくとも次を確認する。

1. build map上の`PERSIST_CRITICAL`総使用量が32 KiBを超えない。
2. 各データ集合が、本書で定める物理的な予約容量を超えない。
3. A/B copyを一方ずつ破損させ、同じ状態・版を保持する正常な複製だけを使って保存データを復旧できる。
4. 機能側で新しい内容への更新を確定した後に古いA/Bレコードを注入しても`NO_OLDER_SEMANTIC_REVISION`対象が復活しない。
5. 工場出荷状態への初期化時に、DELETE/ROTATE/RETAINの区分がデータ一覧の規定どおりとなる。
6. 外付けNORの各領域で、本書の対応表に定めた完全性検証・認証の失敗を検出し、利用不可とする。
7. `BULK_RESERVE`を未割当のまま通常runtimeが使用しない。
8. 4 KiB critical reserveを実装都合の未登録datasetが侵食していない。

---

## 8. 詳細設計への引継ぎ

詳細設計担当者は、本書のデータ一覧を基に、データ集合IDの数値、物理オフセット、構造体、ドライバ呼出しを具体化する。

詳細設計だけで次を変更しない。

- `PERSIST_CRITICAL` 32 KiB総量
- 28 KiB予約＋4 KiB reserveのarchitecture budget
- 保存対象データ本体の容量上限
- A/B／journal方式
- データ集合の管理機能
- rollback/fallback policy
- factory reset action
- integrity/authentication方式
- external NOR matrix

これらの変更が必要な場合は、B16または該当事項を定める基本設計へ戻して変更理由、capacity、Safety/Security影響を再確認する。

---

## 9. GR-06完了条件

本書により、GR-06で要求された次を基本設計として確定した。

- 論理データ集合の一覧
- 管理する機能、保存先、原子性、完全性、巻戻し・代替データの使用規則、工場出荷状態への初期化時の扱い、起動に必須かどうか
- `PERSIST_CRITICAL`の32 KiBへ収容できることの静的な容量確認
- 未割当ての予約領域4 KiB
- 外付けNORのデータ集合ごとの完全性検証・認証方式の対応表
- 容量確認を実機評価ではなく、アーキテクチャの成立性確認として扱う工程上の区分

したがって、詳細設計担当者がdataset policyや32 KiB成立可否を新規判断する必要はない。

---

## 作成経緯

基本設計粒度レビューのGR-06への対応として、本書の補完対象を基本設計で確定した。

旧記述との比較基準コミット：`28b3ee2d505d53aefaedfc601669b75e92a104ad`。これは指摘対応前の記述の適用範囲を確認するための履歴情報であり、本書の現行版を固定するSHAではない。現行版の識別はGit履歴に従う。

旧引継ぎ記述の文章整理は、レビュー指摘GR-12への対応で実施してよい。
