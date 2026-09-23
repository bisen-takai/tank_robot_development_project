# Tank_Robot 外部API・管理Webセキュリティ契約基本設計

## 目次

1. 文書の位置付けと優先関係
2. 共通HTTPS・API version契約
3. Device API endpoint契約
4. POST／UNKNOWN／再送の共通規則
5. Server certificate受入れprofile
6. 管理Web－VPS セキュリティ契約
7. 管理Web－VPS API共通契約
8. 検証方針
9. 設計判断・詳細設計引継ぎ
10. GR-01対応範囲

---

## 1. 文書の位置付けと優先関係

### 1.1 目的

本書の目的は、次の外部契約を、関係するシステムが独立に詳細設計できる粒度まで固定することである。

- Tank_Robot本体－VPS間HTTPS API
- ログbatch送信とACK／再送／重複排除
- VPS UTC時刻取得
- サーバ証明書受入れprofile
- 管理Webアプリ－VPS間の管理者認証・セッション・CSRF・認可・監査

**位置付けと目的**

- **位置付け**：[「インターフェース・通信基本設計」](Tank_Robotインターフェース・通信基本設計.md)、[「VPS接続・デバイス認証基本設計」](../03_機能別基本設計/12_VPS接続・デバイス認証/VPS接続・デバイス認証基本設計.md)、[「ログ・診断基本設計」](../03_機能別基本設計/14_ログ・診断/ログ・診断基本設計.md)、[「時刻管理基本設計」](../03_機能別基本設計/20_時刻管理/時刻管理基本設計.md)、[「セキュリティ横断設計」](../06_セキュリティ横断設計/Tank_Robotセキュリティ横断設計.md)を補完するシステム間外部契約を定める基準文書
- **目的**：Tank_Robot本体－VPS間API、ログACK・冪等性、VPS時刻API、サーバ証明書profile、管理Web－VPS間の認証・セッション・CSRF・認可・監査の契約を基本設計として確定する

関連する設計事項は、引き続き次の文書に従う。

| 設計事項 | 定める文書 |
| --- | --- |
| VPS接続・デバイス認証の動作条件と処理 | VPS接続・デバイス認証基本設計 |
| ログの内容、診断動作、およびSENT/UNSENTの扱い | ログ・診断基本設計 |
| 時刻サンプルの採否、時刻補正、および時刻の信頼状態 | 時刻管理基本設計 |
| システム全体の信頼境界 | セキュリティ横断設計 |

本書は、上記の設計事項を定める担当を変更しない。Tank_Robot本体、VPSおよび管理Webアプリを個別に詳細設計・実装するために必要な、**外部通信形式・API・セキュリティの共通仕様**を一か所にまとめて定める。

本書で確定するendpoint、HTTP method、field名、型、単位、必須／任意、長さ上限、エラーの意味、ACK原子性、冪等性、管理者認証・セッション・CSRF・認可・監査、およびサーバ証明書受入れprofileは基本設計で定める事項とし、詳細設計で別方式へ変更しない。

既存文書に、本書で確定した事項を「詳細設計で固定する」「今後の管理Web/VPS基本設計で確定する」とする記述が残る場合、**当該外部契約に限って本書を優先する**。serializer/parser、C構造体、HTTP library mapping、DB table/index、controller名、password hashの具体algorithm/cost、session store実装、cookie名、CSRF tokenの具体符号化、framework middleware設定等は詳細設計へ残す。

**対象文書**

- [`docs/20_basic_design/04_インターフェース・通信/Tank_Robotインターフェース・通信基本設計.md`](Tank_Robotインターフェース・通信基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/12_VPS接続・デバイス認証/VPS接続・デバイス認証基本設計.md`](../03_機能別基本設計/12_VPS接続・デバイス認証/VPS接続・デバイス認証基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/14_ログ・診断/ログ・診断基本設計.md`](../03_機能別基本設計/14_ログ・診断/ログ・診断基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/20_時刻管理/時刻管理基本設計.md`](../03_機能別基本設計/20_時刻管理/時刻管理基本設計.md)
- [`docs/20_basic_design/06_セキュリティ横断設計/Tank_Robotセキュリティ横断設計.md`](../06_セキュリティ横断設計/Tank_Robotセキュリティ横断設計.md)

**本書と関連文書の適用範囲**

1. **本書が定める事項**：本体・VPS間の外部API、ログの受領確認・再送・重複排除、VPS時刻API、サーバ証明書の受入れ条件、および管理Webの認証・セッション・CSRF・認可・監査。
2. **関連文書が引き続き定める事項**：本書が明示的に補完する事項、または文書間で共通の規則として確定する事項以外は、本書で挙げる関連する基本設計の担当範囲と変更責任を維持する。
3. **本書を優先する場合**：上記の事項について本書で基本設計として確定した記述と、当該事項への対応前の旧記述が競合する範囲に限る。対応経緯と、比較基準の版がある場合の識別情報は、末尾の「作成経緯」に示す。
4. **本書を優先しない事項**：詳細設計へ残す内部実装、および補完対象外の状態、機能の動作、安全、セキュリティ、外部との通信・連携規則などは、それぞれを担当する基本設計に従う。

### 1.2 関連する基準文書

- [`docs/20_basic_design/04_インターフェース・通信/Tank_Robotインターフェース・通信基本設計.md`](Tank_Robotインターフェース・通信基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/12_VPS接続・デバイス認証/VPS接続・デバイス認証基本設計.md`](../03_機能別基本設計/12_VPS接続・デバイス認証/VPS接続・デバイス認証基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/13_設定管理/設定管理基本設計.md`](../03_機能別基本設計/13_設定管理/設定管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/14_ログ・診断/ログ・診断基本設計.md`](../03_機能別基本設計/14_ログ・診断/ログ・診断基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/15_OTA更新/OTA更新基本設計.md`](../03_機能別基本設計/15_OTA更新/OTA更新基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/17_セキュリティ管理/セキュリティ管理基本設計.md`](../03_機能別基本設計/17_セキュリティ管理/セキュリティ管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/20_時刻管理/時刻管理基本設計.md`](../03_機能別基本設計/20_時刻管理/時刻管理基本設計.md)
- [`docs/20_basic_design/06_セキュリティ横断設計/Tank_Robotセキュリティ横断設計.md`](../06_セキュリティ横断設計/Tank_Robotセキュリティ横断設計.md)

### 1.3 詳細設計との境界

基本設計で固定するもの：

- API versionと互換性規則
- endpointとHTTP method
- 必須項目、field type、単位、最大長、requiredness
- HTTP/API errorの意味
- request IDとdomain IDの相関
- state-changing POSTのUNKNOWN／retry／idempotency
- log ACKの原子性
- `/time` response schemaとserver timestamp生成契約
- サーバ証明書で許可するalgorithm・最小強度・chain profile
- 管理者identity、role、MFA、server-side session、session失効、CSRF、server-side authorization、高影響操作audit

詳細設計へ残すもの：

- JSON serializer/parser実装
- C/C++/C#等の構造体・DTO・class名
- HTTP library・ASP.NET Core等へのmapping
- route/controller/service class名
- DB table/index/column物理設計
- password hashの具体algorithmとcost parameter
- TOTP library、secret保存物理方式
- session storeのDB/cache方式
- cookie名・CSRF tokenの具体encoding
- middleware/filter配置
- audit table/index、検索UI

---

## 2. 共通HTTPS・API version契約

### 2.1 transport

Tank_Robot本体－VPS間はVPS接続・デバイス認証に従いTLS 1.3＋mTLS上のHTTP/1.1を使用する。

管理Web－VPS間もHTTPSを使用する。管理Web browserからTank_Robot本体へ直接BLE/Wi-Fi接続して操作しない。

### 2.2 API version

初期製品のdevice API major versionを`v1`とし、URI base pathを`/api/v1/`とする。

JSON共通field `protocolVersion`は初期値`"1.0"`とする。

- major不一致は互換性なしとして正常処理しない。
- 同一majorで将来minorを追加する場合、既存必須項目の意味・型・単位を変更しない。
- 同一majorで追加された未知のoptional fieldは、body上限内で安全に無視できる。
- 未知のrequired fieldを要求する変更、既存fieldの型変更、単位変更、意味変更はmajor version変更を必要とする。

### 2.3 共通header

Device API requestは少なくとも次を使用する。

| Header | 値・契約 |
| --- | --- |
| `Host` | VPS接続・デバイス認証の`vpsHostName` |
| `Content-Type` | JSON bodyでは`application/json; charset=utf-8`、OTA imageはbinary |
| `Content-Length` | VPS接続・デバイス認証の上限内の明示長 |
| `Connection` | HTTP/1.1 connection reuse方針に従う |
| `X-TankRobot-Protocol` | `1.0` |
| `X-Device-ID` | 製品情報管理のDevice ID |
| `X-Request-ID` | VPS接続・デバイス認証の`requestId`の10進文字列表現 |

`requestId`は本体内部ではuint64であり、JSONまたはHTTP headerで表現する場合はJavaScript等の整数精度差を避けるため、**1～20桁の符号なし10進文字列**とする。0は無効値である。

### 2.4 共通JSON response envelope

JSON responseは用途ごとのデータ本体に加え、少なくとも次を含む。

| field | wire type | 必須 | 上限・規則 |
| --- | --- | --- | --- |
| `protocolVersion` | string | 必須 | `"1.0"`、最大8 byte |
| `requestId` | string | 必須 | uint64の10進文字列、1～20 digit |
| `deviceId` | string | 必須 | 製品情報管理のDevice ID、最大40 byte |
| `result` | string enum | 必須 | 最大64 byte |
| `errorCode` | string enum | 必須 | 最大64 byte。成功時`NONE` |

requestの`requestId`またはDevice IDと一致しないresponseは正常応答として使用しない。

### 2.5 共通エラーの意味

初期製品で共通に使用するエラー分類を次のとおり定める。用途固有のエラーは、同じ意味の粒度で追加できる。ただし、同じコードを別の意味に再利用しない。

| errorCode | HTTPの代表 | 意味 |
| --- | ---: | --- |
| `NONE` | 2xx | errorなし |
| `BAD_REQUEST` | 400 | 構文、型、必須項目、値域等が不正 |
| `PROTOCOL_UNSUPPORTED` | 400 | protocol major/version非対応 |
| `DEVICE_UNAUTHORIZED` | 401 | device認証不成立 |
| `DEVICE_FORBIDDEN` | 403 | 認証済みだがdevice利用禁止・権限なし |
| `NOT_FOUND` | 404 | 対象device/artifact等が存在しない |
| `CONFLICT` | 409 | generation、transaction、idempotency内容等が競合 |
| `PAYLOAD_TOO_LARGE` | 413 | body等が上限超過 |
| `RATE_LIMITED` | 429 | 一時的な要求頻度制限 |
| `TEMPORARY_SERVER_ERROR` | 5xx | 一時的なVPS application障害 |
| `INTERNAL_ERROR` | 5xx | VPS内部異常。成功へ読み替えない |

HTTP statusと`errorCode`が矛盾するresponseを正常成功にしない。2xxであっても用途ごとのデータ本体の検証が完了するまでは、対象機能の処理成功としない。

### 2.6 field表現の共通規則

- JSON文字コードはUTF-8とする。
- uint16/uint32で53 bitを超えない数値はJSON integerを使用できる。
- uint64または96 bit以上の識別子はJSON numberへ載せず、正規化した文字列表現を使用する。
- `logBatchId`はuint64の10進文字列とする。
- ログ・診断の`logId`はログ・診断で定める`BBBBBBBBBBBBBBBB-SSSSSSSS`形式を使用する。
- `otaTransactionId`がuint64の場合は10進文字列とする。
- UTC absolute timeは`int64`のUnix epoch millisecondsとし、field名末尾を`Ms`とする。
- size/lengthは特記しない限りbyte単位とする。
- unknown enum値を既知成功値として扱わない。

---

## 3. Device API endpoint契約

### 3.1 endpoint一覧

VPS接続・デバイス認証で採用済みのendpointを外部契約として固定する。

| 用途 | Method | URI |
| --- | --- | --- |
| 設定取得 | GET | `/api/v1/device/{deviceId}/config` |
| ログ・診断送信 | POST | `/api/v1/device/{deviceId}/logs` |
| OTA manifest取得 | GET | `/api/v1/device/{deviceId}/ota/manifest` |
| OTA image取得 | GET | `/api/v1/device/{deviceId}/ota/image/{firmwareId}` |
| OTA結果送信 | POST | `/api/v1/device/{deviceId}/ota/result` |
| UTC時刻取得 | GET | `/api/v1/device/{deviceId}/time` |

redirectを自動追従せず、別hostへtrust boundaryを変更しない。

### 3.2 設定取得

設定取得requestには、少なくとも次のclient状態を送る。

| field/query | type | 必須 | 意味 |
| --- | --- | --- | --- |
| `currentGeneration` | uint32 integer | 必須 | 設定管理 現在のeffective generation。未設定は0 |
| `schemaVersion` | uint16 integer | 必須 | 本体が受入れ可能な設定schema |
| `firmwareId` | string | 必須 | 製品情報管理のrunning firmware identityに対応する識別子。最大64 byte |

正常response resultは少なくとも次を区別する。

- `NO_NEW_CONFIG`
- `CONFIG_AVAILABLE`

`CONFIG_AVAILABLE`では少なくとも次を必須とする。

| field | type | 上限・規則 |
| --- | --- | --- |
| `packageId` | string | 32 lowercase hex |
| `schemaVersion` | uint16 | 設定管理の定義 |
| `generation` | uint32 | 0は禁止 |
| `targetDeviceId` | string | 現在のDevice IDと一致 |
| `packageLength` | uint32 | byte。VPS接続・デバイス認証/設定管理の16 KiB上限以内 |
| `package` | JSON object | 設定管理で定める設定パッケージの内容とHMAC情報を含む |

VPS接続・デバイス認証は、通信と共通スキーマを検証した後に、完全なパッケージを設定管理へ渡す。HMAC、世代の下限値、値域、適用条件、および機能側での更新確定は、設定管理／セキュリティ管理が判断する。

### 3.3 ログ送信request

`POST /logs`は一つのimmutable application batchを表す。

request bodyは次を必須とする。

| field | type | 上限・規則 |
| --- | --- | --- |
| `protocolVersion` | string | `1.0` |
| `requestId` | string | 現在のHTTP attempt |
| `deviceId` | string | 現在のDevice ID |
| `logBatchId` | string | ログ・診断のuint64の10進文字列 |
| `eventCount` | uint16 | 1～64 |
| `events` | array | `eventCount`件。JSON全体16 KiB以下 |

各eventはログ・診断の意味を失わず、少なくとも次を送る。

| field | type | 規則 |
| --- | --- | --- |
| `logId` | string | ログ・診断の固定形式、必須 |
| `eventId` | uint32 | 必須 |
| `sourceFunctionId` | uint16 | 必須 |
| `category` | string enum | `OPERATION/FAULT/SECURITY` |
| `priority` | string enum | `IMPORTANT/NORMAL` |
| `bootInstanceId` | string | uint64 10進文字列 |
| `eventSequence` | uint32 | 必須 |
| `monotonicTimeUs` | string | uint64 10進文字列 |
| `utcEpochMs` | int64 integer | ログ・診断で無効時0 |
| `timeQuality` | string enum | 時刻管理/ログ・診断の`TIME_QUALITY_*` |
| `timeSyncGeneration` | uint32 | 未確立0可 |
| `configGeneration` | uint32 | 適用外0可 |
| `correlationId` | string | uint64 10進文字列。なしは`"0"` |
| `firmwareBuildFingerprint` | string | uint64を16桁lowercase hex |
| `payload` | string/object | eventごとのVPS表現。batch上限を超えない |

`logId`とイベントの意味は、ログ・診断が定める。VPS向けの通信データでは、別のログ識別子へ置き換えない。

### 3.4 ログACKの原子性

初期製品では**batch単位の全件ACK（all-or-nothing）**を採用し、partial ACKを採用しない。

VPSは一つの`logBatchId`について、次をすべて満たした場合だけ成功ACKを返す。

1. request envelopeが正常。
2. `eventCount`と`events`件数が一致。
3. 全eventの必須項目・型・上限が正常。
4. batch内`logId`が重複していない。
5. VPSが全eventを当該batchとして永続受領済み、または既に同じ内容で受領済みである。

成功responseは少なくとも次を含む。

| field | type | 規則 |
| --- | --- | --- |
| `result` | string | `LOG_BATCH_ACCEPTED` |
| `errorCode` | string | `NONE` |
| `logBatchId` | string | requestと同一 |
| `acceptedEventCount` | uint16 | requestの`eventCount`と完全一致 |

一部eventだけ保存できた場合、または一部だけ検証失敗した場合は成功ACKを返さない。ログ・診断はbatch内の一部だけをSENTへ変更しない。

### 3.5 ログ冪等性

ログ・診断が同じbatchを再送する場合、同じ`logBatchId`と同じ`logId`集合・同じevent内容を使用する。

VPSは次の規則を守る。

- 同じ`logBatchId`かつ同じ内容の再送：既存受領結果を利用し、重複eventを追加せず`LOG_BATCH_ACCEPTED`を返せる。
- 異なる`logBatchId`でも既受領の同一`logId`：同じeventとして重複登録しない。
- 同じ`logBatchId`で`logId`集合またはevent内容が異なる：`409 CONFLICT`とし、既存内容を書き換えない。
- 同じ`logId`で意味内容が異なる：`409 CONFLICT`とし、既存eventを書き換えない。

response確定前の切断・timeoutではVPS接続・デバイス認証はHTTP attemptをUNKNOWNとしてログ・診断へ返す。ログ・診断は当該batchをUNSENTのまま保持し、ログ・診断の再送policyに従う。VPS接続・デバイス認証がstate-changing POSTを通信層判断だけで暗黙再送しない。

### 3.6 OTA manifest

manifestの応答には、共通エンベロープに加え、OTA更新が定める次の情報を必須とする。

- target Device ID
- firmware ID
- SemVer
- BuildID
- SecurityGen
- image format/version
- target product/hardware
- image length
- image hash
- signature関連情報
- compatibility情報
- image取得情報

manifest JSON全体は8 KiB以下とする。具体fieldの内部互換性意味はOTA更新/セキュリティ管理/製品情報管理/固定ブート・復旧がそれぞれ定め、VPSは当該意味をwire上で欠落させない。

### 3.7 OTA image

OTA imageはbinaryとし、OTA更新が指定する`firmwareId`とRangeを使用する。

- 1 response chunkは最大16 KiB。
- HTTP Range `bytes=start-end`を使用する。
- successful partial responseはHTTP 206を基本とする。
- `Content-Range`、returned length、firmware ID、全体sizeがrequestと整合しなければ正常chunkとしない。
- response UNKNOWN時にVPS接続・デバイス認証が同一Rangeを暗黙再要求しない。

### 3.8 OTA結果POST

OTA結果requestは共通fieldに加え、少なくとも次を含む。

- `otaTransactionId`
- `firmwareId`
- local update result
- recovery result
- running firmware identity
- result確定に必要なOTA更新の定義metadata

同じ`otaTransactionId`の同内容再送をVPSはidempotentに扱う。同じtransaction IDで矛盾するlocal resultを受信した場合は`409 CONFLICT`とし、既存確定結果を暗黙上書きしない。

response UNKNOWN時、OTA更新はlocal final resultを書き換えず`otaResultReportState=PENDING`を維持し、再送する場合は同じOTA domain ID＋新しいHTTP `requestId`を使用する。

### 3.9 UTC時刻API

`GET /api/v1/device/{deviceId}/time`の正常responseは共通envelopeに加え、次を必須とする。

| field | wire type | 単位・規則 |
| --- | --- | --- |
| `serverUtcEpochMs` | int64 JSON integer | Unix epoch milliseconds / UTC |

正常resultを`TIME_SAMPLE`、`errorCode=NONE`とする。

VPSは`serverUtcEpochMs`を**response body送出直前**に取得し、そのresponseへ固定する。事前生成cache、前回request値、長時間前に取得した値を再利用しない。

VPS接続・デバイス認証はrequest送信直前とresponse受信完了の同一起動内の単調増加するタイムスタンプを時刻管理へ提供する。時刻管理がRTT、範囲、offset、large correction、Time Update transactionを判断する。`serverUtcEpochMs`単独でTIME_TRUSTEDを新規bootstrapしない。

---

## 4. POST／UNKNOWN／再送の共通規則

### 4.1 HTTP要求と機能側の一連の処理の識別

VPS接続・デバイス認証の`requestId`は、一回のHTTP要求の送信試行を識別する。ログ送信やOTA更新等の一連の処理を識別する`logBatchId`や`otaTransactionId`とは区別する。

POST送信後、正常な応答を確定する前に、応答期限の超過、TCP/TLS切断、応答本文の欠損等が発生した場合、その送信試行の結果をUNKNOWNとする。

### 4.2 暗黙再送禁止

VPS接続・デバイス認証は、状態を変更するPOSTを、通信処理の再試行として同じ本文で自動再送しない。

処理を管理する機能が再送を許可した場合だけ、次のすべてを満たす新しいHTTP要求を開始する。

- 同じ処理・重複実行防止の識別子を使用する。
- 要求本文の内容が、元の要求と同じである。
- 新しいHTTP `requestId`を使用する。

### 4.3 同じ識別子で内容が異なる場合

同じ処理識別子で内容が異なる要求をVPSが受信した場合、既存結果を上書きせず、`409 CONFLICT`を返す。

UNKNOWNを「未受信だった」と仮定して、処理識別子を新しい値へ勝手に置き換えない。

---

## 5. Server certificate受入れprofile

### 5.1 基本profile

VPS接続・デバイス認証/セキュリティ管理のTLS 1.3契約を維持し、VPS サーバ証明書は少なくとも次を満たすことを必要とする。

- X.509 v3。
- certificate validityを時刻管理の`TIME_TRUSTED`で検証できる。
- server authentication用途が許可されている。
- DNS SANにVPS接続・デバイス認証の`vpsHostName`を含み、hostname検証に成功する。
- 現在のACTIVE VPS trust anchorへchainが到達する。
- SHA-1、MD5等の弱いsignature hashを許可しない。
- RSA keyを使用する場合は2048 bit以上。
- EC keyを使用する場合はP-256相当以上のセキュリティ管理/VPS接続・デバイス認証実装で明示許可した曲線だけを使用する。
- certificate signatureはSHA-256以上を使用する。

### 5.2 初期許可algorithm

初期製品でサーバ証明書に許可する基本集合を次とする。

- ECDSA P-256 + SHA-256
- RSA-PSS 2048 bit以上 + SHA-256以上

上記より弱い方式へ自動downgradeしない。追加algorithmが必要になった場合は、interop都合だけで詳細設計へ追加せずVPS接続・デバイス認証/セキュリティ管理/本書を改訂する。

### 5.3 chain profile

VPSから提示するchainは、leaf＋最大2枚のintermediate certificateを基本上限とし、最終的に本体のACTIVE trust anchorへ到達することを必要とする。

- leafのSAN/用途/有効期間を検証する。
- intermediateのCA制約とsignature chainを検証する。
- unknown root、期限切れ、chain不成立、hostname mismatchをfail-openしない。
- 通常運用でverify-none、任意自己署名certificate受入れ、hostname検証無効を許可しない。

具体certificate DER bytes、trust anchor実体、library error code mappingは詳細設計・provisioning事項とする。

---

## 6. 管理Web－VPS セキュリティ契約

### 6.1 管理・保守通信の位置付け

管理WebとVPSは、Tank_Robotを管理・保守するための通信と処理を担当する。
本体のアクチュエータを直接操作するための通信経路としては使用しない。

管理Webから実行できる操作は、次の管理・保守用途に限定する。

- 設定の管理
- ログ・診断情報の閲覧
- ファームウェアおよびOTA更新の管理
- デバイスおよびセキュリティ保守用の管理情報の取扱い
- 監査記録の参照

走行、砲塔・砲身、通常停止、E-STOP解除その他の、本体アクチュエータを遠隔から直接操作するendpointは設けない。

### 6.2 管理者の識別情報と権限

初期製品は個人開発・単一管理者運用を前提とし、管理Webのroleは`ADMIN` 1種類とする。

- 匿名の管理操作を許可しない。
- 誰が操作したか追跡できない共通管理アカウントを採用しない。
- 管理者自身によるアカウントの自己登録は、初期製品では提供しない。
- VPSは、すべての管理APIで、サーバー側において現在認証済みの管理者IDと`ADMIN` roleを確認する。
- クライアント画面でボタンやメニューを隠すことだけを、操作権限の確認とはしない。
- 将来roleを追加する場合は、権限一覧（permission matrix）と高影響操作の権限を基本設計として改訂する。

管理者IDの確認と、要求された操作を実行してよいかの確認は分けて扱う。
画面に操作項目が表示されているかどうかに関係なく、VPS側で認証状態と権限を確認する。

### 6.3 管理者認証

初期製品では、管理者本人を確認するために次の2要素を両方必要とする。

1. 管理者ID＋password
2. TOTPによるMFA

passwordだけで管理セッションを成立させない。

passwordを平文または可逆暗号で保存しない。
salt付きの適応型password hashを使用し、具体的なalgorithmとcostは詳細設計で固定する。

MFA secretは、通常ログ、browser storage、管理画面へ平文のまま継続表示・保存しない。
MFAの初期登録およびresetは、高影響管理操作として監査記録の対象とする。

### 6.4 session方式

認証成功後は**server-side session**を採用する。

- browserには推測困難なopaque session identifierだけを保持させる。
- session identifierは暗号用途乱数で生成し、少なくとも256 bitのentropyを持つ。
- session cookieは`Secure`、`HttpOnly`、`SameSite=Strict`を必須とする。
- authentication bearer credentialを`localStorage`へ保存しない。
- login成功時および権限上重要な再認証時にsession IDをrotateし、session fixationを防ぐ。

初期session lifetimeを次とする。

- inactivity timeout：30分
- absolute lifetime：8時間

次でcurrent sessionを直ちに失効する。

- logout
- password変更・reset
- MFA reset
- account無効化
- security compromise対応で管理者session一括失効を実行した場合

具体session store、cleanup job、cookie名は詳細設計へ残す。

### 6.5 CSRF

cookie-based authenticated sessionを使用するため、state-changing management requestはCSRF防御を必須とする。

- GET/HEAD等のsafe methodで状態変更を行わない。
- POST/PUT/PATCH/DELETE等の状態変更requestでは、serverが発行し検証するCSRF tokenを必要とする。
- Origin/Host等のsame-origin条件も検証する。
- token欠損・不一致・期限切れをauthorization成功へ読み替えない。

CSRF tokenの具体生成・encoding・middlewareは詳細設計で決めてよいが、防御そのものを省略しない。

### 6.6 authorization

すべての管理操作はserver-side authorizationを行う。

初期製品の`ADMIN`は管理plane機能を利用できるが、次の制約はroleでもoverrideできない。

- unsigned firmwareを正式artifactとして配信しない。
- invalid HMAC configを本体へ適用成功扱いしない。
- VPS/管理Webからactuator direct commandを送らない。
- device-side Safety Gateをoverrideしない。
- firmware signing private keyをVPS runtimeへ置かない。

### 6.7 高影響管理操作

少なくとも次を高影響操作とする。

- config artifact/generationの登録・有効化
- firmware artifact登録・OTA campaign開始／停止／対象変更
- device利用状態の無効化・削除
- trust/credential maintenance metadata変更
- security recovery/maintenance result登録
- 管理者password変更/reset
- MFA登録/reset
- active admin sessionの一括失効

高影響操作は、authenticated `ADMIN`＋CSRF検証＋server-side authorizationをすべて満たした場合だけ実行する。

### 6.8 audit

高影響操作と主要な認証・セキュリティイベントは、サーバ側で監査する。

各監査レコードには、少なくとも次の情報を保持する。

- audit event type
- authenticated admin identity
- server側operation/request ID
- target device/artifact/config/firmware等の非秘密識別子
- action
- result success/failure/rejected
- failure category
- server UTC timestamp
- 変更対象にrevision/generationがある場合、そのbefore/after識別子

password、MFA secret、session ID全値、private key、config MAC key等のsecretをauditへ記録しない。

監査recordのDB table、index、検索画面等は詳細設計事項である。

### 6.9 login failure／session abuse

連続login failureや無効sessionの大量入力に対して、VPSはrate limit/backoffを適用し、CPU・DB・logを無制限消費しない。

具体回数・時間はVPS詳細設計・結合評価で固定できるが、rate limit無効化を初期運用前提にしない。

## 7. 管理Web－VPS API共通契約

### 7.1 API原則

管理Web APIの画面別endpoint名は後続管理Web/VPS基本設計で整理してよい。ただし、詳細設計前に次の外部意味契約を維持する。

- authentication APIとmanagement APIを分離する。
- state-changing APIはCSRF対象とする。
- 現在のadmin identity/roleをrequest bodyだけから信用せずserver sessionで管理する情報を判定の基準とする。
- request受付とdomain処理完了を分離する。
- 長いOTA/config処理は`accepted/in-progress/completed/failed`等を区別する。
- VPSへartifactを登録した状態とTank_Robot本体が取得・検証・適用・確定した状態を別field/stateとして保持する。
- error responseへsecret、stack trace、SQL、credentialを利用者向けに露出しない。

### 7.2 本体反映状態

管理Webで設定・OTA等を表示するとき、少なくとも次の意味を混同しない。

- VPSへ登録済み
- 対象deviceへ配信可能
- deviceが取得済み
- deviceが暗号・互換性検証済み
- deviceが適用／更新処理中
- deviceで正常確定済み
- deviceで失敗
- device result reportがUNKNOWN/PENDING

VPS registeredをdevice applied/updatedと表示しない。

---

## 8. 検証方針

### 8.1 Device API

| ID | 確認内容 |
| --- | --- |
| API-01 | protocol major不一致を正常処理しない |
| API-02 | requestId/deviceId不一致responseを拒否する |
| API-03 | 必須項目欠損、型不一致、上限超過を拒否する |
| API-04 | uint64 IDをJSON number精度へ依存せず文字列で往復できる |
| API-05 | 429/5xxの応答とTLS切断を区別して、処理を依頼した機能へ返す |
| API-06 | state-changing POSTのUNKNOWNをVPS接続・デバイス認証が暗黙再送しない |
| API-07 | 同じdomain ID＋同内容再送がidempotentになる |
| API-08 | 同じdomain ID＋異内容を409 CONFLICTとする |

### 8.2 Log ACK

| ID | 確認内容 |
| --- | --- |
| LOG-01 | 64 event/16 KiB上限が成立する |
| LOG-02 | batch全件正常時だけ`LOG_BATCH_ACCEPTED`を返す |
| LOG-03 | 1 eventでも不正・保存失敗ならpartial successを返さない |
| LOG-04 | ACK前はログ・診断が全件UNSENTを維持する |
| LOG-05 | UNKNOWN後の同一batch再送で重複eventを生成しない |
| LOG-06 | 同一logBatchId異内容を409 CONFLICTとする |
| LOG-07 | 同一logId異内容を409 CONFLICTとする |

### 8.3 Time API

| ID | 確認内容 |
| --- | --- |
| TIME-01 | `serverUtcEpochMs`がint64 epoch millisecondsとして取得できる |
| TIME-02 | response body送出直前にtimestampを取得する |
| TIME-03 | cacheした旧timestampを再利用しない |
| TIME-04 | request/response monotonic timestampを時刻管理へ渡せる |
| TIME-05 | TIME_UNTRUSTEDから本APIだけでtrust bootstrapしない |

### 8.4 Server certificate

| ID | 確認内容 |
| --- | --- |
| TLS-01 | SAN hostname mismatchを拒否する |
| TLS-02 | unknown root／期限切れchainを拒否する |
| TLS-03 | SHA-1/MD5等の弱い署名を拒否する |
| TLS-04 | RSA 2048未満を拒否する |
| TLS-05 | chain上限を超える提示を正常扱いしない |

### 8.5 Management Web

| ID | 確認内容 |
| --- | --- |
| WEB-01 | anonymousでmanagement APIを実行できない |
| WEB-02 | passwordだけではMFA未完了sessionをADMIN sessionとして成立させない |
| WEB-03 | authenticated session cookieがSecure/HttpOnly/SameSite=Strict |
| WEB-04 | inactivity 30分、absolute 8時間でsession失効する |
| WEB-05 | logout/password reset/MFA reset/account disableでsession失効する |
| WEB-06 | state-changing requestでCSRF token欠損・不一致を拒否する |
| WEB-07 | UI非表示だけでなくserver-side ADMIN authorizationを行う |
| WEB-08 | 高影響操作のadmin identity、target、result、timeをauditできる |
| WEB-09 | auditへpassword/MFA secret/session secretを記録しない |
| WEB-10 | 管理Web/VPSからremote actuator operation endpointが存在しない |

---

## 9. 設計判断・詳細設計引継ぎ

### 9.1 設計判断

| ID | 設計判断 |
| --- | --- |
| D-01 | Device APIを`/api/v1`、protocol `1.0`として固定する |
| D-02 | VPS接続・デバイス認証のHTTP `requestId`と、機能側の一連の処理の識別子を分離する。通信上のuint64値は10進文字列で扱う |
| D-03 | 共通response envelopeと共通error categoryを基本設計で固定する |
| D-04 | log ACKをbatch単位all-or-nothingとしpartial ACKを採用しない |
| D-05 | `logBatchId`＋`logId`によるidempotencyをVPS側契約とする |
| D-06 | POSTの結果がUNKNOWNでも、VPS接続・デバイス認証は暗黙に再送しない。処理を管理する機能の許可を受け、同じ処理識別子・同じ内容の要求本文・新しいrequestIdで再試行する |
| D-07 | `/time` responseを`serverUtcEpochMs` int64 epoch msとして固定し、response送出直前生成とする |
| D-08 | サーバ証明書許可algorithm・最小強度・chain profileを基本設計で固定する |
| D-09 | 初期Management Web roleを`ADMIN` 1種類とする |
| D-10 | 管理者認証をpassword＋TOTP MFAの2要素必須とする |
| D-11 | 管理Webをserver-side opaque sessionとし、Secure/HttpOnly/SameSite=Strict cookieを使用する |
| D-12 | 管理sessionをinactivity 30分、absolute 8時間とする |
| D-13 | state-changing management APIへCSRF防御を必須とする |
| D-14 | すべてのmanagement APIでserver-side authorizationを行う |
| D-15 | 高影響操作をadmin identity付きserver-side audit対象とする |
| D-16 | 管理Web/VPSをmanagement planeとしremote actuator operation endpointを設けない |

### 9.2 詳細設計事項

次は基本契約を変更しない範囲で詳細設計へ送る。

- JSON serializer/parserとDTO/C構造体
- HTTP client/server library mapping
- ASP.NET Core route/controller/service class名
- DB table/index/physical schema
- common error codeの内部enum値
- password hash具体algorithm/cost
- TOTP libraryとMFA secret物理保存方式
- server-side session store実装
- cookie名、CSRF token encoding
- middleware/filter配置
- audit table/index、検索UI
- certificate DER/PEM物理配置とlibrary error mapping
- rate limit/backoffの具体回数・時間

### 9.3 基本設計へ戻す変更

次を変更する場合は詳細設計内だけで処理せず、本書および関係する基準文書を改訂する。

- endpoint/method/API major
- 必須項目、型、単位、長さ上限
- log partial ACK採用
- idempotency keyまたはUNKNOWN意味
- `/time` timestamp生成意味
- サーバ証明書最小強度／chain profile
- ADMIN以外のrole追加
- MFA廃止・方式のsecurity level変更
- client-side bearer token方式への変更
- session timeout/security attribute変更
- CSRF防御廃止
- high-impact authorization/audit範囲変更
- remote actuator management endpoint追加

---

## 10. GR-01対応範囲

本書の追加により、GR-01で詳細設計へ先送りされていた事項を次のように基本設計へ引き上げた。

| GR-01対象 | 本書での対応 |
| --- | --- |
| endpoint/method | 3.1でVPS接続・デバイス認証の採用済み値を外部契約として固定 |
| 必須項目・型・単位・長さ・requiredness | 2章、3章で固定 |
| エラーの意味 | 2.5で共通の分類を固定 |
| HTTP要求と用途別処理の識別子 | 2.3、2.6、4章で分離規則を固定 |
| ログACKの原子性 | 3.4で全件受領または全件未受領に固定 |
| UNKNOWN時の扱い・再送・冪等性 | 3.5、4章で固定 |
| `/time` response | 3.9でwire schemaとtimestamp生成を固定 |
| サーバ証明書 profile | 5章で許可algorithm・最小強度・chainを固定 |
| 管理者identity/role | 6.2で単一`ADMIN`に固定 |
| 管理者authentication | 6.3でpassword＋TOTP MFA必須 |
| session／失効 | 6.4でserver-side session、30分/8時間、失効条件を固定 |
| CSRF | 6.5でstate-changing requestへ必須化 |
| authorization | 6.6～6.7でserver-side ADMIN確認を固定 |
| 高影響操作audit | 6.7～6.8で対象と必須audit semanticを固定 |

これにより、詳細設計担当者が製品挙動・外部相互運用性・セキュリティ方針を新たに選択する必要をなくし、詳細設計は実装への対応付けへ限定する。

---

**文書終端：GR-01対象の外部API・ACK・冪等性・時刻API・サーバ証明書・管理Web認証／session／CSRF／authorization／auditを、詳細設計へ展開可能な基本設計契約として確定する。**

---

## 作成経緯

基本設計粒度レビューのGR-01への対応として、本書の補完対象を基本設計で確定した。

参照したレビュー：[`50_基本設計/90_レビュー/04_Tank_Robot_基本設計粒度レビュー結果.md`](../90_レビュー/04_Tank_Robot_基本設計粒度レビュー結果.md)。
