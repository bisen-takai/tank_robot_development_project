# Tank_Robot BLE異常詳細取得通信契約基本設計

本書は、スマートフォンアプリが本体で現在管理中の異常（active fault）の詳細を、BLEで1ページ最大8件ずつ取得する方法を定める。
各ページでは、まずページ全体の情報を受け取り、その後に個々の異常データを受け取る。
取得中に異常一覧の世代が変わった場合は、古いページを破棄し、先頭のページ0から取得し直す。

**関連する設計**

[「異常管理基本設計」](../04_異常管理/異常管理基本設計.md)、[「状態表示・利用者通知基本設計」](../21_状態表示・利用者通知/状態表示・利用者通知基本設計.md)、[「BLE接続・認証基本設計」](BLE接続・認証基本設計.md)、[「インターフェース・通信基本設計」](../../04_インターフェース・通信/Tank_Robotインターフェース・通信基本設計.md)。

**本書と関連文書の適用範囲**

1. **本書が定める事項**：スマートフォンから異常詳細を取得するためのBLE通信形式、データ項目、権限、結果および再取得規則。
2. **関連文書が引き続き定める事項**：本書が明示的に補完する事項、または文書間で共通の規則として確定する事項以外は、本書で挙げる関連する基本設計の担当範囲と変更責任を維持する。
3. **本書を優先する場合**：上記の事項について本書で基本設計として確定した記述と、当該事項への対応前の旧記述が競合する範囲に限る。対応経緯と、比較基準の版がある場合の識別情報は、末尾の「作成経緯」に示す。
4. **本書を優先しない事項**：詳細設計へ残す内部実装、および補完対象外の状態、機能の動作、安全、セキュリティ、外部との通信・連携規則などは、それぞれを担当する基本設計に従う。

BLE接続・認証基本設計（B09）を補完し、プロトコル1.0で異常詳細を取得するためのメッセージ、データ項目、権限、結果および再取得規則を定める。

異常情報そのものの管理、利用者に示す内容の意味、BLE共通の通信形式、および世代を合わせ直す共通の連携規則は、引き続き各担当機能・文書で定める。
具体的な分担は第1.1節に示す。

状態表示・利用者通知基本設計（B21）に、`FAULT_DETAIL_PAGE`のバイナリ配置を詳細設計事項とする旧記述が残る場合は、**BLEで外部へ送るデータ配置に限って本書を優先する**。
通信形式への符号化・受信データの解析処理、内部C構造体、固定容量バッファ、キュー、および異常管理・状態表示機能の内部APIへの対応付けは、詳細設計へ残す。

---

## 1. 関連設計との分担と基本方針

### 1.1 情報の管理と通信仕様の分担

| 項目 | 管理・定義する機能または文書 |
| --- | --- |
| 管理中の異常一覧、`faultId`、`faultGeneration`、`faultListGeneration` | 異常管理機能（B04） |
| 利用者に示す異常詳細の意味、代表表示、利用者に求める対応の意味 | 状態表示・利用者通知機能（B21） |
| BLE共通ヘッダー、セッション、`requestId`、Characteristic、64 byte上限 | BLE接続・認証基本設計（B09） |
| 異常詳細を要求・応答するための通信形式と規則 | 本書。BLE接続・認証基本設計を補完する |
| 世代変更時の古いページの破棄と、ページ0からの再取得 | 状態表示・利用者通知基本設計（B21）とインターフェース・通信基本設計（I01）。本書では、それらの規則をBLE通信へ適用する |

### 1.2 基本方針

#### 取得の目的と権限

異常詳細の取得は読み取り専用とし、異常解除、復旧開始および状態変更を行わない。

OWNER／GENERALの両方の役割で取得可能とする。
所有者・操作端末管理機能（B10）の「主要状態参照／`STATUS_QUERY`」と同じ読み取り専用の権限区分を使用し、新しい管理権限は追加しない。

#### ページと応答の構成

1. 1ページは、状態表示・利用者通知基本設計で定めた最大8件とする。
2. BLE共通メッセージの最大64 byte、固定ヘッダー24 byte、ペイロード最大40 byteは変更しない。
3. 8件を一つのメッセージへ圧縮しない。ページ情報1通と、個々の異常データ最大8通を、**最大9回のIndication**で配送する。取得途中の世代変更に伴う終端通知と送信上限の例外は、第7.3節に従う。
4. 応答には`RELIABLE_TX`のIndicationを使用する。通常の`STATE_TX` Notificationで行う、複数の更新を最新情報へまとめる処理の対象には含めない。
5. 取得要求、ページ情報、およびすべての異常データを、同じ非0の`requestId`で対応付ける。

#### 世代の照合と再取得

1. スマートフォンアプリが指定した`expectedFaultListGeneration`と、異常管理機能が管理する現在の`faultListGeneration`が一致しない場合は、ページ内容を返さず`STALE_GENERATION`を返す。世代未取得時に0を指定する場合の扱いは、第3.3節に従う。
2. ページ取得中に世代が変化した場合は、その要求に対応する残りの異常データを送信せず、`STALE_GENERATION`として処理を終える。スマートフォンアプリは現在の世代を取得し、ページ0から取得し直す。
3. BLEリンクまたは操作セッションを失った場合、古いページの配送を新しいセッションへ引き継がない。

---

## 2. 使用Characteristicとmessage種別

### 2.1 Characteristic

既存B09 Characteristicを使用し、新Characteristicを追加しない。

| 方向 | Characteristic | Property | 用途 |
| --- | --- | --- | --- |
| スマートフォンアプリ → 本体 | `REQUEST_RX` | Write | `FAULT_DETAIL_QUERY` |
| 本体 → スマートフォンアプリ | `RELIABLE_TX` | Indicate | `FAULT_DETAIL_PAGE_INFO`、`FAULT_DETAIL_ITEM` |

`REQUEST_RX`および`RELIABLE_TX`の既存暗号化link／session条件を変更しない。fault detail取得は通常のアプリケーション層認証済みsessionだけで許可する。

### 2.2 messageType／subtype

B09のプロトコルのコード割当表へ、次のメッセージ種別／サブタイプを追加する。

- `FAULT_DETAIL_QUERY`
- `FAULT_DETAIL_PAGE_INFO`
- `FAULT_DETAIL_ITEM`

具体8 bit数値は既存未使用領域からB09の詳細なプロトコル定義表へ割り当てる。ただし、**上記3種の存在、方向、データ本体の項目配置、permissionおよび結果の意味は基本設計事項であり変更しない**。既存messageの数値を再利用して意味変更しない。

---

## 3. FAULT_DETAIL_QUERY

### 3.1 request条件

requestは次をすべて満たす場合だけdomain受付へ進める。

- encrypted BLE link
- authenticated application session
- OWNERまたはGENERALのACTIVE terminal
- 現在のsessionId／connectionGeneration
- 非0`requestId`
- payloadLength=8 byte
- reserved field=0

### 3.2 データ本体の項目配置

`FAULT_DETAIL_QUERY_V1`を8 byte固定とする。little-endianはB09 protocol 1.0の既存規則へ従う。

| Offset | field | 型 | 内容 |
| ---: | --- | --- | --- |
| 0 | `expectedFaultListGeneration` | uint32 | スマートフォンアプリが基準とするB04 generation。0は「generation未取得」を示し、pageIndex=0の場合だけ許可 |
| 4 | `pageIndex` | uint8 | 0始まり |
| 5 | `pageSize` | uint8 | protocol 1.0では8固定。8以外は`INVALID_REQUEST` |
| 6 | `reserved` | uint16 | 0固定 |

### 3.3 generation=0の扱い

初回取得またはgenerationを保持していないスマートフォンアプリは、`expectedFaultListGeneration=0`かつ`pageIndex=0`で要求できる。この場合、本体は現在のB04 generationをsnapshot開始generationとして採用し、responseで返す。

`expectedFaultListGeneration=0`でpageIndex>0は許可しない。2 page目以降はpage 0で取得したgenerationを明示する。

---

## 4. FAULT_DETAIL_PAGE_INFO

### 4.1 役割

`FAULT_DETAIL_PAGE_INFO`はrequest全体の結果および当該page metadataを返す最初のIndicationである。

正常時、後続する`FAULT_DETAIL_ITEM`の件数を`itemCount`で確定する。スマートフォンアプリは`itemCount`件すべてのitemを正常受領するまでpage取得完了としない。

### 4.2 データ本体の項目配置

`FAULT_DETAIL_PAGE_INFO_V1`を16 byte固定とする。

| Offset | field | 型 | 内容 |
| ---: | --- | --- | --- |
| 0 | `faultListGeneration` | uint32 | 当該pageのB04 generation |
| 4 | `pageIndex` | uint8 | requestと同じpage index |
| 5 | `itemCount` | uint8 | 0～8 |
| 6 | `hasMore` | uint8 | 0=false、1=true |
| 7 | `resultCode` | uint8 | 第6章 |
| 8 | `totalActiveFaultCount` | uint16 | 現在のactive fault件数。最大値超過時はprotocol error |
| 10 | `firstItemOrdinal` | uint16 | active fault snapshot内の0始まり先頭ordinal。正常時=`pageIndex*8` |
| 12 | `reserved` | uint32 | 0固定 |

### 4.3 active fault 0件

active faultが0件の場合も正常responseとし、pageIndex=0に対して次を返す。

- `resultCode=OK`
- `itemCount=0`
- `hasMore=0`
- `totalActiveFaultCount=0`

この場合、後続`FAULT_DETAIL_ITEM`は送信しない。

---

## 5. FAULT_DETAIL_ITEM

### 5.1 役割

1つの`FAULT_DETAIL_ITEM`はB21が利用者へ提示する1件のactive fault summaryを搬送する。固定24 byte payloadとし、B09共通header込み48 byteで64 byte上限内に収める。

### 5.2 データ本体の項目配置

`FAULT_DETAIL_ITEM_V1`を24 byte固定とする。

| Offset | field | 型 | 内容 |
| ---: | --- | --- | --- |
| 0 | `faultListGeneration` | uint32 | page infoと同じgeneration |
| 4 | `faultGeneration` | uint32 | B04個別fault generation |
| 8 | `faultId` | uint32 | B04 fault identifier |
| 12 | `sourceFunctionId` | uint16 | fault source。B14/I01の機能ID体系と対応 |
| 14 | `pageIndex` | uint8 | page infoと同じ |
| 15 | `itemIndex` | uint8 | 当該page内0～7、連続昇順 |
| 16 | `indicatorCode` | uint8 | B21が当該faultへ割り当てる利用者向け短縮表示code。該当なしは0 |
| 17 | `treatmentCode` | uint8 | 第5.3節のwire abstraction |
| 18 | `operationImpactCode` | uint8 | 第5.3節 |
| 19 | `recoveryCode` | uint8 | 第5.3節 |
| 20 | `userActionCode` | uint8 | 第5.3節 |
| 21 | `flags` | uint8 | 第5.4節 |
| 22 | `reserved` | uint16 | 0固定 |

### 5.3 利用者向けwire abstraction code

本fieldはB04内部enumを外部protocolへ直接露出するものではない。B21がB04の管理する異常情報の意味を利用者通知向けに変換した結果を、protocol 1.0の固定codeへ写像する。

#### treatmentCode

| 値 | 意味 |
| ---: | --- |
| 0 | `TREATMENT_UNKNOWN_OR_NONE` |
| 1 | `TREATMENT_MONITORING` |
| 2 | `TREATMENT_FUNCTION_LIMITED` |
| 3 | `TREATMENT_SAFE_STOP` |
| 4 | `TREATMENT_ABNORMAL_STOP` |
| 5 | `TREATMENT_EMERGENCY_STOP` |
| 6～255 | 予約 |

#### operationImpactCode

| 値 | 意味 |
| ---: | --- |
| 0 | `IMPACT_UNKNOWN` |
| 1 | `IMPACT_NONE` |
| 2 | `IMPACT_LIMITED` |
| 3 | `IMPACT_NORMAL_OPERATION_PROHIBITED` |
| 4～255 | 予約 |

#### recoveryCode

| 値 | 意味 |
| ---: | --- |
| 0 | `RECOVERY_UNKNOWN` |
| 1 | `RECOVERY_NOT_ALLOWED` |
| 2 | `RECOVERY_AUTOMATIC_IN_PROGRESS_OR_AVAILABLE` |
| 3 | `RECOVERY_USER_ACTION_ALLOWED` |
| 4 | `RECOVERY_RESTART_REQUIRED` |
| 5 | `RECOVERY_MAINTENANCE_REQUIRED` |
| 6～255 | 予約 |

#### userActionCode

| 値 | 意味 |
| ---: | --- |
| 0 | `ACTION_NONE` |
| 1 | `ACTION_WAIT` |
| 2 | `ACTION_RETRY` |
| 3 | `ACTION_RESTART` |
| 4 | `ACTION_CHECK_POWER_OR_BATTERY` |
| 5 | `ACTION_CHECK_CONNECTION_OR_COMMUNICATION` |
| 6 | `ACTION_CONTACT_OR_PERFORM_MAINTENANCE` |
| 7～255 | 予約 |

B04／B21でより詳細な内部分類を持つ場合でも、スマートフォンアプリに誤った強い復旧可能性を表示しない方向へ写像する。特にB04が`recoveryAllowed=false`の場合、`RECOVERY_USER_ACTION_ALLOWED`へ変換しない。

### 5.4 flags

| bit | 意味 |
| ---: | --- |
| 0 | `ACTIVE`。本契約では正常itemで1固定 |
| 1 | `USER_ACTION_REQUIRED` |
| 2 | `MAINTENANCE_REQUIRED` |
| 3 | `OPERATION_RESTRICTED` |
| 4 | `NORMAL_OPERATION_PROHIBITED` |
| 5～7 | 0固定予約 |

### 5.5 item順序

異常管理（B04）が提供する、現在管理中の異常のスナップショットについて、安定した通し番号順を使用する。ページ内の`itemIndex`は0から連続する昇順とする。同じ世代について再要求した場合、B04のスナップショットが同一である限り、同じpageIndex/itemIndexは同じfaultIdへ対応する。

---

## 6. resultCode

`FAULT_DETAIL_PAGE_INFO.resultCode`は次を使用する。

| 値 | code | 意味・後続動作 |
| ---: | --- | --- |
| 0 | `OK` | page正常。itemCount件のitemを後続送信 |
| 1 | `STALE_GENERATION` | expected generationと現在値が不一致。item送信なし。スマートフォンアプリはpage 0から再取得 |
| 2 | `PAGE_OUT_OF_RANGE` | 現在の集合に対してpageIndex範囲外。item送信なし |
| 3 | `UNAVAILABLE` | B04/B21の現在のsnapshotを正常取得不能。item送信なし |
| 4 | `INVALID_REQUEST` | length、pageSize、reserved、generation/page組合せ等が不正 |
| 5 | `NOT_AUTHORIZED` | 現在のauthenticated role/sessionで状態参照を許可できない |
| 6 | `BUSY` | 同時detail transaction上限により受付不能。後で新requestIdで再要求可能 |
| 7～255 | 予約 | 未知値を成功扱いしない |

`STALE_GENERATION`ではresponseの`faultListGeneration`へ現在のgenerationを返せる。スマートフォンアプリはその値を新しい基準候補として使用できるが、必ずpage 0から取得する。

---

## 7. transaction・資源上限・中断

### 7.1 同時transaction

認証済みの1 BLEセッションにつき、同時に実行する異常詳細取得処理は1件とする。

受付済みの異常詳細取得処理が完了していない間に、新しい`FAULT_DETAIL_QUERY`を受けても、現在の処理を暗黙に置き換えず`BUSY`を返す。スマートフォンアプリは、現在の処理が完了または失効した後、新しいrequestIdで要求する。

### 7.2 response上限

1 requestで送信するresponseは最大9 Indicationとする。

- PAGE_INFO：1件
- ITEM：0～8件

固定上限を超えるmessage生成、unbounded queue、malloc/freeを必要とする構成にしない。

### 7.3 generation変更

PAGE_INFO生成前にgeneration変更を検出した場合は`STALE_GENERATION`を返す。

PAGE_INFO送信後、ITEM配送中にB04 `faultListGeneration`変更を検出した場合、残りITEMを送らず、同じrequestIdの`FAULT_DETAIL_PAGE_INFO`を`STALE_GENERATION`として追加で1件返してtransactionを終了できる。この例外時の総Indication上限は10件とする。

スマートフォンアプリは一部受領済みITEMを現在の集合として使用せず破棄し、page 0から再取得する。

### 7.4 link/session喪失

次でin-flight fault detail transactionを破棄する。

- BLE切断
- `connectionGeneration`変更
- application session失効
- OWNER/GENERAL terminal validity喪失
- B09/B04/B21 unavailable

旧requestIdのresponseを新しいセッションへ継続送信しない。

### 7.5 timeout

fault detailはread-only参照であり、通信中断時にdomain rollbackは不要である。

Indication ACK待ちまたは内部snapshot取得がB09既存の管理・離散要求timeout上限を超えた場合、transactionを終了し、スマートフォンアプリは新requestIdで再要求する。detail取得失敗をSafety処理、異常管理、現在fault stateへ反映しない。

---

## 8. スマートフォンアプリ側の取得規則

### 8.1 ページの取得

1. 認証後、現在の`STATE_SNAPSHOT`から、`faultListGeneration`と管理中の異常の有無を確認する。
2. 異常の詳細が必要な場合は、`FAULT_DETAIL_QUERY(expectedGeneration, pageIndex=0, pageSize=8)`を送信する。
3. `PAGE_INFO=OK`を受領したら、同じ`requestId`、一覧の世代および`pageIndex`に対応する`itemCount`件の異常データを受け取る。すべて揃うまでは、そのページの取得を完了としない。
4. ページの取得が完了し、`hasMore=1`である場合は、同じ世代で次のページ（`pageIndex+1`）を要求する。

### 8.2 未完了・再取得が必要な場合

| 条件 | アプリの扱い |
| --- | --- |
| `itemCount`件が揃わない、Indicationがタイムアウトした、またはセッションが切断された | 当該ページを未完了として破棄する |
| `STALE_GENERATION`を受領した、`STATE_SNAPSHOT`で一覧の世代変更を確認した、または再接続・再認証が発生した | 取得途中のページをすべて破棄し、ページ0から再取得する |
| `PAGE_OUT_OF_RANGE`を受領した | 空ページとして正常扱いしない。一覧の世代とページ番号の計算を再確認する |

### 8.3 異常への操作との区別

異常詳細の取得結果だけを理由として、本体の異常解除、処理の再試行、再起動などを行わない。
これらは別の明示的な管理操作として扱い、その操作に必要な権限と安全上の実行条件を使用する。

---

## 9. 詳細設計へ引き継ぐ事項

次は本契約を変更しない実装事項として詳細設計へ残す。

- 3 message/subtypeの具体8 bit code割当て
- serializer/parser関数名
- B04 snapshot／B21 mappingのC API
- fixed transaction context構造体
- RELIABLE_TX Indication enqueue実装
- internal timeout timer／callback
- B21内部enumから第5.3節wire codeへのswitch/table実装
- unit test vector

次は詳細設計で変更しない。

- page最大8件
- request payload 8 byte
- PAGE_INFO 16 byte
- ITEM 24 byte
- 最大9 Indication（generation途中変更時のみ終端STALEを含み最大10）
- OWNER/GENERAL read-only permission
- requestId相関
- expected／現在generation照合
- stale時page 0再取得
- link/session境界で旧transaction破棄

---

## 10. 検証項目

| ID | 確認内容 |
| --- | --- |
| FDT-01 | OWNER/GENERAL認証済みsessionからpage 0を取得できる |
| FDT-02 | 未認証セッションから取得できない |
| FDT-03 | active fault 0件でOK、itemCount=0、hasMore=0となる |
| FDT-04 | 1～8件で1 pageに収まる |
| FDT-05 | 9件以上でhasMore=1となり次pageを取得できる |
| FDT-06 | expected generation不一致でSTALE_GENERATIONとなりitemを返さない |
| FDT-07 | item配送中generation変更で部分pageをcurrentとして採用せずpage 0から収束する |
| FDT-08 | PAGE_OUT_OF_RANGEを正常空pageへ読み替えない |
| FDT-09 | itemCount件未満で通信断した場合pageを未完了として破棄する |
| FDT-10 | 再接続後に旧requestId responseを継続しない |
| FDT-11 | 24 byte ITEM＋24 byte共通headerが48 byteで64 byte上限内 |
| FDT-12 | 8件pageでもfixed resource上限内で処理できる |
| FDT-13 | B04 recoveryAllowed=falseをスマートフォンアプリ側でretry可能へ誤変換しない |
| FDT-14 | detail取得失敗がSafety処理／異常管理機能が管理するfault情報を変更しない |

---

## 11. GR-02完了条件

本書により、GR-02で不足していた次を基本設計として固定した。

- fault detail request/response message
- page index／page size
- expected/現在の`faultListGeneration`
- item count／hasMore／total count
- 1件分の通信データに含める項目と、その意味の固定定義
- OWNER／GENERAL permission
- 結果・エラーの意味
- 64 byte制約下のbounded multi-message layout
- generation変更時のpartial page破棄とpage 0再取得
- requestId/session/connection相関
- serializer／buffer／queue等だけを詳細設計へ残す境界

したがって、詳細設計担当者がfault detailの外部BLE互換性や利用者再取得挙動を新規選択する必要はない。

---

**文書終端：本書はBLE接続・認証基本設計（B09）のプロトコル1.0を補完し、異常詳細を取得する通信形式と規則を定める。異常管理機能（B04）による異常情報の管理、および状態表示・利用者通知機能（B21）による利用者向け情報の意味の定義は変更しない。**

---

## 作成経緯

基本設計粒度レビューのGR-02への対応として、本書の補完対象を基本設計で確定した。

参照したレビュー：[`50_基本設計/90_レビュー/04_Tank_Robot_基本設計粒度レビュー結果.md`](../../90_レビュー/04_Tank_Robot_基本設計粒度レビュー結果.md)。
