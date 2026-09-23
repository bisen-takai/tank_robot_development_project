# Tank_Robot OTA更新基本設計

## 目次

1. 本書の目的
2. 設計方針と適用範囲
3. 責任分界と共通条件
4. OTAハードウェア・メモリ構成
5. ファームウェア識別・イメージ・manifest
6. OTA内部状態と永続管理情報
7. 更新情報確認と更新対象判定
8. OTA更新開始条件とシステム状態連携
9. 更新候補ファームウェア取得
10. 更新候補の検証とCANDIDATE確定
11. 直前正常ファームウェアのPREVIOUS退避
12. 本体アプリケーション領域書換え前の最終確認
13. MCUboot Overwriteを使用した更新反映
14. 更新後の一回限りの試行起動
15. 正常起動確認と更新成功確定
16. SecurityGen確定と巻戻し防止
17. 本体アプリケーション領域書換え前の失敗
18. 書換え開始後の失敗とPREVIOUS復旧
19. 電源断・リセット・処理中断時の復旧
20. 通常終了・緊急停止・異常・電源保護との連携
21. 設定データ・永続データ互換性
22. 有線復旧
23. 状態表示・ログ・診断・VPS結果送信
24. 実行時間・CPU・RAM・通信・固定資源方針
25. ビルド・署名・リリース構成管理
26. 検証方針と確認ケース
27. 主対象要求との対応
28. 設計判断・後続事項
29. 他文書へのフィードバック
30. 初版の自己確認結果と引継ぎ条件

---

## 1. 本書の目的

### 1.1 目的

本書の目的は、公開用システム要求仕様「15. OTA更新」を実現するため、Tank_Robot本体アプリケーションファームウェアを安全かつ復旧可能に更新する基本方式を、詳細設計へ展開可能な粒度まで具体化することである。

### 1.2 対象範囲

本書は、Tank_Robot本体アプリケーションファームウェアのOTA更新について、更新情報確認、更新候補取得、署名・完全性・互換性検証、直前正常ファームウェア退避、MCUbootを使用した本体アプリケーション領域への反映、更新後の一回限りの試行起動、正常起動確定、失敗時の直前正常ファームウェア復旧、電源断・リセット時の継続判断、有線復旧および関連機能との責任分界を具体化する。

通常のOTA更新対象は本体アプリケーションファームウェア一つだけとし、MCUbootを含む固定ブート・復旧機能、Wi-Fiモジュール、BLEモジュールその他の外部モジュールファームウェアは通常OTA更新対象としない。

OTA更新は安全機能そのものではない。更新処理の成否よりも、緊急停止、安全停止、電源保護、異常処理および起動可能なファームウェアの確保を優先する。

#### 基本設計と詳細設計の区分

本書で定めるOTA開始・中断・trial・復旧条件、通信再試行方針、時間上限、容量上限、image互換性、結果確定・報告条件その他、利用者・関連機能から見た更新結果または復旧可否を変える値・方式は、本書または参照する担当基本設計で定める事項とする。実機評価で変更が必要となった場合は詳細設計だけで変更せず、本書または値・方式を定める関連基本設計へ評価結果をフィードバックして正式値を更新する。物理address、erase/program単位、driver API、buffer配置、task、TLV IDその他、外部振る舞いを変更しない実装方法は詳細設計で具体化してよい。

### 1.3 上位文書・関連文書

本書の主対象要求は公開用システム要求仕様「15. OTA更新」とし、関連する公開用要求および完成済み基本設計を、責任分界、状態、停止、電源、VPS通信、設定、ログ、永続データおよびセキュリティの整合確認に使用する。

`20_要求仕様`および`30_コア要求仕様`は上位要求として使用しない。ただし、公開要求で未確定の設計事項について過去検討の背景を確認する参考資料として参照する場合がある。その場合でも、公開要求と矛盾する内容を本書へ復活させない。

関連基本設計との整合確認結果を本書へ反映する。後続基本設計で具体化された永続OTA state、SecurityGen、Product Descriptor／running firmware identity、Boot Interface、fixed boot起動判定、VPS request再送責任等が本書の外部振る舞いを具体化している場合は、その確定内容を本書でも使用する。

#### 1.3.1 上位文書

- [`docs/00_project_overview/01_製品目的・製品目標.md`](../../../00_project_overview/01_製品目的・製品目標.md)
- [`docs/10_requirements/15. OTA更新.md`](<../../../40_公開用システム要求仕様/15. OTA更新.md>)
- [`docs/20_basic_design/00_Tank_Robot 基本設計について.md`](<../../00_Tank_Robot 基本設計について.md>)
- [`docs/20_basic_design/01_システム構成/Tank_Robotシステム構成.md`](../../01_システム構成/Tank_Robotシステム構成.md)
- [`docs/20_basic_design/02_システムアーキテクチャ/Tank_Robotシステムアーキテクチャ.md`](../../02_システムアーキテクチャ/Tank_Robotシステムアーキテクチャ.md)

#### 1.3.2 関連するシステム要求仕様

- [`docs/10_requirements/01. 起動・終了・再起動.md`](<../../../40_公開用システム要求仕様/01. 起動・終了・再起動.md>)
- [`docs/10_requirements/02. システム状態管理.md`](<../../../40_公開用システム要求仕様/02. システム状態管理.md>)
- [`docs/10_requirements/03. 電源・省電力管理.md`](<../../../40_公開用システム要求仕様/03. 電源・省電力管理.md>)
- [`docs/10_requirements/09. 停止・緊急停止.md`](<../../../40_公開用システム要求仕様/09. 停止・緊急停止.md>)
- [`docs/10_requirements/10. バッテリー・電気安全.md`](<../../../40_公開用システム要求仕様/10. バッテリー・電気安全.md>)
- [`docs/10_requirements/12. VPS接続・デバイス認証.md`](<../../../40_公開用システム要求仕様/12. VPS接続・デバイス認証.md>)
- [`docs/10_requirements/13. 設定管理.md`](<../../../40_公開用システム要求仕様/13. 設定管理.md>)
- [`docs/10_requirements/14. ログ・診断.md`](<../../../40_公開用システム要求仕様/14. ログ・診断.md>)
- [`docs/10_requirements/17. 異常検出・復旧.md`](<../../../40_公開用システム要求仕様/17. 異常検出・復旧.md>)
- [`docs/10_requirements/18. 永続データ管理.md`](<../../../40_公開用システム要求仕様/18. 永続データ管理.md>)
- [`docs/10_requirements/20. セキュリティ管理.md`](<../../../40_公開用システム要求仕様/20. セキュリティ管理.md>)
- [`docs/10_requirements/21. 状態表示・利用者通知.md`](<../../../40_公開用システム要求仕様/21. 状態表示・利用者通知.md>)
- [`docs/10_requirements/22. 性能・品質・耐久性.md`](<../../../40_公開用システム要求仕様/22. 性能・品質・耐久性.md>)
- [`docs/10_requirements/23. 保守・サポート・製品ライフサイクル.md`](<../../../40_公開用システム要求仕様/23. 保守・サポート・製品ライフサイクル.md>)

#### 1.3.3 関連する基本設計

- [`docs/20_basic_design/03_機能別基本設計/01_システム状態管理/Tank_Robotシステム状態管理基本設計.md`](../01_システム状態管理/Tank_Robotシステム状態管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/02_起動・終了・再起動管理/起動・終了・再起動管理基本設計.md`](../02_起動・終了・再起動管理/起動・終了・再起動管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/03_停止・緊急停止管理/停止・緊急停止管理基本設計.md`](../03_停止・緊急停止管理/停止・緊急停止管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/04_異常管理/異常管理基本設計.md`](../04_異常管理/異常管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/05_電源・省電力管理/電源・省電力管理基本設計.md`](../05_電源・省電力管理/電源・省電力管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/06_バッテリー・電気安全監視/バッテリー・電気安全監視基本設計.md`](../06_バッテリー・電気安全監視/バッテリー・電気安全監視基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/12_VPS接続・デバイス認証/VPS接続・デバイス認証基本設計.md`](../12_VPS接続・デバイス認証/VPS接続・デバイス認証基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/13_設定管理/設定管理基本設計.md`](../13_設定管理/設定管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/14_ログ・診断/ログ・診断基本設計.md`](../14_ログ・診断/ログ・診断基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/16_永続データ管理/永続データ管理基本設計.md`](../16_永続データ管理/永続データ管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/17_セキュリティ管理/セキュリティ管理基本設計.md`](../17_セキュリティ管理/セキュリティ管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/18_製品情報管理/製品情報管理基本設計.md`](../18_製品情報管理/製品情報管理基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/19_固定ブート・復旧/固定ブート・復旧基本設計.md`](../19_固定ブート・復旧/固定ブート・復旧基本設計.md)
- [`docs/20_basic_design/03_機能別基本設計/21_状態表示・利用者通知/状態表示・利用者通知基本設計.md`](../21_状態表示・利用者通知/状態表示・利用者通知基本設計.md)
- [`docs/20_basic_design/04_インターフェース・通信/Tank_Robotインターフェース・通信基本設計.md`](../../04_インターフェース・通信/Tank_Robotインターフェース・通信基本設計.md)

### 1.4 外部技術資料の位置付け

実装成立性確認として、Renesas RA8M2/FSPのMCUboot Port、OSPI_B、RA8M2 Boot FirmwareおよびRenesas Flash Programmerの現行公開技術情報を参照する。

参照時点では、RA8M2向けFSP MCUboot PortはOverwrite方式およびOSPI_Bをsecondary storageとして使用する構成をサポートする。一方、OSPI_B external storageを使用するSwap方式は制約があるため、本書ではSwap rollbackを前提としない。

この技術情報は製品上位要求ではない。FSPまたはMCUbootの将来版で条件が変化した場合でも、本書の要求上の目的である「固定ブートを壊さない」「候補起動失敗時にPREVIOUSへ復旧する」を維持し、具体実装を再評価する。

### 1.5 用語

| 用語 | 本書での意味 |
| --- | --- |
| PRIMARY | RA8M2内蔵MRAMの、本体アプリケーションとして起動する領域 |
| CANDIDATE | 外付けSerial NOR上に保持する更新候補イメージ領域。MCUboot secondary slotとして使用する |
| PREVIOUS | PRIMARYを書き換える前に退避した直前正常ファームウェアの独立領域 |
| 固定ブート・復旧 | MCUbootとTank_Robot固有の起動・復旧ラッパを含む、通常OTAで書き換えない機能 |
| trial boot | 新候補をPRIMARYへ反映後、正常確認前に一回だけ実行する起動試行 |
| `otaTransactionId` | 一回の本体内OTA処理を識別するuint32識別子 |
| `otaOfferId` | VPS側で一回の配信・再配信意図を識別する128 bit識別子。同一FWを明示再配信する場合も新しい値を発行する |
| `SecurityGen` | セキュリティ巻戻し防止用のuint32世代 |
| `bootInterfaceVersion` | 固定ブート・復旧が正式に管理するfixed boot↔application Boot Handoff/Boot Service ABI版。imageは対応`bootInterfaceMin/Max`を署名保護metadataへ持つ |
| `INSTALL_REQUESTED` | CANDIDATE/PREVIOUS準備完了後、OTA機能の本体アプリケーション側がcontrolled reboot前に確定する、固定ブートへinstall開始を許可する非破壊状態 |
| `otaSettlementPending` | 更新後trialの正常起動確認・最終確定が完了しておらず、通常操作へ進ませない状態 |
| `TRIAL_BOOT_VALIDATED` | 候補FWが規定の正常起動条件を満たしたことを永続確定した段階 |
| `otaResultReportState` | local OTA resultをVPS result APIへ報告済みかを示す状態。local OTA resultそのものとは分離する |
| destructive phase | PRIMARYの消去・書換えを開始した後の段階 |

---

## 2. 設計方針と適用範囲

### 2.1 基本方針

OTA更新は次の原則で設計する。

1. 通常OTA対象は本体アプリケーションファームウェア一つだけとする。
2. 固定ブート・復旧機能を通常OTAで書き換えない。
3. 署名済み完全イメージを使用し、差分・圧縮イメージを初期製品で使用しない。
4. OTA開始はBLE接続待機状態かつアクチュエータ安全状態に限定する。
5. 所有者の追加承認を要求せず、VPS上の有効な対象更新と開始条件が成立すれば開始可能とする。
6. CANDIDATEを全量取得・検証し、PREVIOUSを退避・検証した後、`INSTALL_REQUESTED`を確定してcontrolled rebootし、固定ブート・復旧による再検証成功後にのみPRIMARYを書き換える。
7. RA8M2/FSPでサポートされるMCUboot Overwrite方式を基本とする。
8. MCUboot標準Swap rollbackへ依存せず、独立PREVIOUS領域を持つTank_Robot固有復旧方式を使用する。
9. 新FWのtrial bootは一回だけとし、candidateへbranchする直前に`trialAttempted=true`を永続確定する。`TRIAL_ARMED`が正常commit済みで`trialAttempted=false`なら、リセット後でも未実施の一回を実行できる状態として扱う。
10. `trialAttempted=true`のcandidateが`TRIAL_BOOT_VALIDATED`へ到達できなければ同candidateを再trialせずPREVIOUS復旧へ移行する。
11. candidateのSecurityGenが高い場合でも、trial bootの正常確認前に許可SecurityGenを上げない。
12. trial正常起動確認およびOTA最終確定が完了するまで`otaSettlementPending`を維持し、通常操作を許可しない。
13. OTA更新後も以前の走行・砲塔・砲身指令を自動再開しない。
14. 緊急停止、安全停止、電源保護、重大異常、強制電源遮断をOTAより優先する。
15. OTA更新中は低消費電力状態へ移行しない。
16. firmware取得中の通信断・request結果不明では現在の取得取引を失敗とし、途中resumeしない。新しいOTA取引ではbyte 0から再取得する。
17. VPS接続・デバイス認証は送信済みHTTP requestを通信層判断だけで暗黙再送しない。OTA用途の再要求可否は本書が所有し、初期製品のfirmware取得では通信断・timeout・結果不明となったRange requestを同じOTA取引内でapplication自動再送しない。
18. 同じ`otaOfferId`の失敗OTAを定期pollだけで自動再実行しない。一方、管理者が同一FWを新しいofferとして明示再発行することは可能とする。
19. OTA処理結果はVPS送信可否と独立して本体内で確定する。VPS結果送信状態はlocal resultと分離して管理する。
20. `malloc/free`を通常OTA処理で使用せず、通信・hash・manifest・状態管理を固定上限で行う。

### 2.2 初期製品で実装しない事項

公開要求の将来検討事項に従い、初期製品では次を実装しない。

- MCUbootまたは固定ブート・復旧機能自体のOTA更新
- BLE/Wi-FiモジュールFWのOTA更新
- 複数FW同時更新
- 差分更新
- 圧縮更新
- download resume
- VPSからのpush型OTA開始
- OTA実行中の管理者遠隔取消し
- 複雑な自動再試行・再配信
- 任意の旧バージョンへの通常ダウングレード
- firmware encryption
- 署名検証鍵のオンライン自動rotation
- OTA進捗率の細かな%表示

---

## 3. 責任分界と共通条件

### 3.1 管理主体

| 項目 | 管理・最終判断主体 |
| --- | --- |
| OTA処理上の状態、更新候補、直前のファームウェア、`recoveryEligible`、`INSTALL_REQUESTED`の発行条件、`TRIAL_BOOT_VALIDATED`、本体で確定したOTAの最終結果、`otaResultReportState` | OTA更新管理 |
| システム状態、OTA更新状態への遷移、通常操作可否 | システム状態管理 |
| アクチュエータ停止完了、緊急停止 | 停止・緊急停止管理 |
| OTA開始・継続の電気安全条件 | バッテリー・電気安全監視 |
| HTTPS、manifest/image/result通信、HTTP request結果 | VPS接続・デバイス認証 |
| 署名検証、SecurityGen、検証鍵 | セキュリティ |
| 物理保存、原子的更新、電源断整合 | 永続データ管理 |
| 設定schema・FACTORY互換性、設定永続取引完了状態 | 設定管理 |
| Device ID、Product Descriptor、running firmware identity | 製品情報管理 |
| 起動・再起動全体シーケンス | 起動・終了・再起動管理 |
| 起動前署名検証、install時の再検証、`PRIMARY_OVERWRITE_STARTED`、`TRIAL_ARMED`、`trialAttempted`、`RECOVERY_BOOT_PENDING`の確定、PRIMARY選択・書換え、trial branch、PREVIOUS復旧実行、Boot Handoff | 固定ブート・復旧 |
| OTA event記録、診断、event logのVPS送信 | ログ・診断 |
| OTA表示 | 状態表示・利用者通知 |
| OTA異常の影響度・異常停止判断 | 異常管理 |

OTA更新管理は他機能の安全判断を再実装しない。各機能から得た結果とOTA固有の状態を組み合わせ、OTA処理の開始・継続・中断・復旧必要性を判断する。

ログ・診断の未送信log管理はOTA event logの送信管理であり、VPS接続・デバイス認証の`/ota/result`へ送るOTA結果reportの正式な管理元ではない。OTA result APIの未送信／送信済み状態は本機能が所有する。

OTA stateについては、意味・遷移条件を定める機能と、当該段階で実際にcommitする実行主体を分ける。OTA更新はOTA transactionとlocal final resultの内容と確定条件の管理元である。Application停止中のboot-critical markerは固定ブート・復旧が確定し、永続データ管理は指定されたbytesを原子的に保存する。起動・終了・再起動管理/システム状態管理/異常管理は、それぞれ起動結果、システム状態、異常影響度を提供するが、OTA final resultを独自に確定しない。

### 3.2 安全処理優先

OTA更新中に安全関連事象が発生した場合、OTA更新の成功を守るために安全処理を遅らせない。

PRIMARY書換え中など、その瞬間に任意のsoftware処理を中断するとMRAM内容が不完全になる段階では、可能な場合は現在実行中の最小program/erase単位だけを完了し、`RECOVERY_REQUIRED`を保持して安全処理へ引き継ぐ。ただし強制電源遮断や危険な電源状態に対して、flash処理の完了を条件として遮断を待たせない。

---

## 4. OTAハードウェア・メモリ構成

### 4.1 RA8M2内蔵MRAM

初期製品のRA8M2内蔵MRAM 1 MiBについて、OTA観点の論理上限を次とする。

| 論理領域 | 設計上限 |
| --- | ---: |
| 固定ブート・復旧 executable＋trust領域 | 最大224 KiB |
| `PERSIST_CRITICAL` | 32 KiB |
| PRIMARY本体アプリケーションslot | 最大768 KiB |

768 KiBは署名付きMCUbootイメージとしてPRIMARYへ格納するslot上限であり、MCUboot header/trailer/alignmentを含む。実際のapplication データ本体の容量上限は詳細設計でこれより小さくなる。

固定ブート・復旧 executable＋trustの実ビルドが224 KiBへ収まらない場合、`PERSIST_CRITICAL` 32 KiBを侵食するのではなく、PRIMARY上限を縮小するかメモリ構成を基本設計へフィードバックする。

固定ブート・復旧、`PERSIST_CRITICAL`およびPRIMARYはlinker/memory mapで非重複領域として固定し、通常OTAの書込みAPIはPRIMARY範囲外を受け付けない。固定ブート領域および`PERSIST_CRITICAL`への誤erase/programをsoftware range checkおよび可能なhardware保護設定で防ぐ。

### 4.2 外付けSerial NOR

製品側の大容量不揮発メモリは、RA8M2のOSPI_B controllerから使用可能なSerial NOR Flashとし、OTA・ログ等を含むシステム要求から**8 MiB以上**を要求条件とする。

最終選定部品は、採用するFSP/MCUboot構成からsecondary storageとして実際に使用できることを確認する。RA8M2 hardwareが対応するprotocolであることだけをもって採用確定しない。

Octal SPIを製品必須とはしない。採用FSP/MCUbootで使用可能で、調達性・パッケージ・価格・書換え寿命を満たす場合はQuad SPI互換品を含めて選定可能とする。

開発・評価ではEK-RA8M2搭載の64 MiB Octo-SPI Flashを使用できるが、同一型式・同一BGA packageを量産製品へ必須としない。

### 4.3 外付けFlash上の論理領域

| 論理領域 | 最低論理割当 | 用途 |
| --- | ---: | --- |
| CANDIDATE / MCUboot secondary | 1 MiB | 新しい署名済みcandidate |
| PREVIOUS | 1 MiB | 直前正常FW退避 |
| OTA管理・補助領域 | 永続データ管理で決定 | OTA bulk管理、製品security補助data等 |
| その他 | 残り | CONFIG/Wi-Fi/log/予備等、永続データ管理の全体partition方針に従う |

ログ・診断の評価用トレースは64 KiB RAMのみであり、通常運用で外付けFlashへ不揮発保存しない。OTA設計からtrace用partitionを要求しない。

具体アドレス、erase sector境界、wear levelingおよび他用途との物理partitionは永続データ管理・ハードウェア詳細設計で決める。

CANDIDATEとPREVIOUSを同一領域として使い回さない。candidateの更新によってPREVIOUSを失わない。

---

## 5. ファームウェア識別・イメージ・manifest

### 5.1 firmware identity

初期製品では次を識別する。

| 情報 | 方式 |
| --- | --- |
| `firmwareId` | リリースイメージを識別する128 bit ID |
| SemVer | MAJOR.MINOR.PATCH。各成分uint16を基本 |
| BuildID | 128 bit build fingerprint |
| SecurityGen | uint32 |
| imageHash | SHA-256 256 bit |
| imageFormatVersion | MCUboot image format互換性識別 |

同じSemVerでもBuildIDまたはimageHashが異なる場合は異なるFWとして扱う。

BuildIDは、source commit、toolchain/FSP、build configuration、依存構成等のcanonical build manifestからSHA-256等で生成し、先頭128 bitを使用する方式を基本とする。完全な生成規則はbuild/release詳細設計で固定する。

実際に実行中のfirmware identityは製品情報管理がrunning signed image metadataから提供する。OTA更新は「正常確認済み現在のfirmware」「candidate」「previous」というOTA上の役割・確定状態を所有し、実行中image identityをOTA永続stateから推測して製品情報管理が提供する現在値を上書きしない。

### 5.2 manifest

VPSから取得するOTA manifestには少なくとも次を含める。

- `otaOfferId`
- target Device ID
- firmwareId
- SemVer
- BuildID
- SecurityGen
- imageFormatVersion
- product/model ID
- hardwareProfileId
- imageLength
- imageHash
- signature algorithm/profile information
- `bootInterfaceMin`
- `bootInterfaceMax`
- 対応設定schemaVersion範囲
- 対応永続データformat範囲
- image取得識別情報

`bootInterfaceMin/Max`は固定ブート・復旧のfixed boot↔application ABI互換性範囲を表し、現在fixed bootの`bootInterfaceVersion`が範囲内であることをOTA開始前およびboot時に確認する。fixed boot自身のrelease versionは診断・構成管理情報として保持できるが、それだけをBoot Interface互換性を判定する基準にしない。

`imageLength`はVPSから取得し検証する署名付きimage全体長を表し、PRIMARY 768 KiB slot boundary内へheader/trailer/alignmentを含めて格納可能であることを別途確認する。

`otaOfferId`はVPSが一回の配信意図ごとに発行する。同じfirmwareId/BuildID/imageHashを管理者が明示的に再配信する場合でも、新しい`otaOfferId`を発行する。

manifestは配信・取得判断のための情報であり、manifestだけを根拠にcandidateを信頼しない。candidate image内の署名保護対象metadataとmanifestの重要属性が一致することを検証する。

### 5.3 署名

公開セキュリティ要求およびセキュリティ管理に従い、firmware署名はECDSA P-256、hashはSHA-256を使用する。

署名保護対象には少なくともfirmware本体／image hash、対象製品、対象hardware、firmware identity、image format、SecurityGenを含める。固定ブート・復旧のBoot Interface互換性宣言も起動前に信頼して使用するため、`bootInterfaceMin/Max`を署名保護metadataに含める。

MCUboot標準header/TLVに不足する製品metadataはセキュリティ管理/固定ブート・復旧のprotected custom TLVまたは同等の署名保護metadataを使用する。具体TLV ID/layoutは詳細設計で固定してよいが、署名保護対象の意味を変更しない。

署名生成秘密鍵を本体、通常VPS実行環境または配布imageに含めない。

---

## 6. OTA内部状態と永続管理情報

### 6.1 内部状態

OTA更新管理は、システム状態とは別に、一連のOTA更新処理の進み具合を管理する。本体アプリケーションの停止中に固定ブート・復旧が確定する起動判断用の情報と復旧実行状態も、同じOTA更新処理へ対応付けて参照する。ただし、OTA更新が固定ブート・復旧の実行結果を推測し、処理を進めることはしない。

| 状態または起動判断用の情報 | 意味 | 内容と確定条件の管理元／確定主体 |
| --- | --- | --- |
| `IDLE` | OTA更新処理なし | OTA更新 |
| `INFO_READY` | 有効な更新情報を保持 | OTA更新 |
| `DOWNLOADING` | CANDIDATE取得中 | OTA更新 |
| `CANDIDATE_READY` | 更新候補の全量取得・検証が完了 | OTA更新 |
| `PREVIOUS_BACKUP` | 現PRIMARYをPREVIOUSへ退避・検証中 | OTA更新 |
| `INSTALL_READY` | 更新候補、直前正常版、および安全条件が揃った | OTA更新 |
| `INSTALL_REQUESTED` | 更新候補と直前正常版の準備完了後、管理された再起動の前に、固定ブート・復旧へインストール開始を許可した状態。PRIMARYの書換えはまだ開始していない | OTA更新 |
| `PRIMARY_OVERWRITE_STARTED` | PRIMARYの最初の消去・書込みの直前に確定する、既存内容の書換え開始を示す情報 | 固定ブート・復旧 |
| `TRIAL_ARMED` | 更新候補が試行起動可能な状態であることを示す情報。`trialAttempted=false`との組合せで、試行起動が未実施であることを表す | 固定ブート・復旧 |
| `trialAttempted` | 更新候補へ制御を移す直前に、試行起動一回を実施したものとして計上したことを示す情報 | 固定ブート・復旧 |
| `TRIAL_BOOT_VALIDATED` | 起動・終了・再起動管理による本体アプリケーションの起動正常完了と、システム状態管理によるBLE接続待機状態の確定を受け、更新候補の正常起動を永続確定済み | OTA更新 |
| `CONFIRM_COMMITTING` | SecurityGenおよび現在有効なファームウェアのメタデータを確定中 | OTA更新 |
| `CONFIRMED` | OTA成功確定 | OTA更新 |
| `RECOVERY_REQUIRED` | PREVIOUSへの復旧が必要 | OTA更新が意味を定める。固定ブート・復旧も、起動中に検出した条件について確定できる |
| `RECOVERY_PREPARING` | PREVIOUSを復旧用にsecondary領域へ準備中 | 固定ブート・復旧の復旧実行状態 |
| `RECOVERY_OVERWRITE` | PREVIOUSをPRIMARYへ反映中 | 固定ブート・復旧の復旧実行状態 |
| `RECOVERY_BOOT_PENDING` | 復元PRIMARYの検証完了後、本体アプリケーションの正常起動確認を待つ、起動判断用の確定情報 | 固定ブート・復旧 |
| `RECOVERY_BOOT` | OTA更新が管理する処理段階。復元したPREVIOUSの本体アプリケーションが正常に起動したかを確認中 | OTA更新 |
| `RECOVERY_SUCCEEDED` | 更新失敗・復旧成功確定 | OTA更新 |
| `FAILED_PREINSTALL` | PRIMARY書換え前にOTA失敗 | OTA更新 |
| `FAILED_RECOVERY` | PREVIOUS復旧不能。通常操作禁止 | OTA更新が意味を定める。本体アプリケーションを起動できないまま失敗が確定した場合は、固定ブート・復旧が規定済み条件に従って代理確定できる |

`internalPhase`は、OTA更新が管理する一連の更新処理の進み具合を表す。固定ブート・復旧が起動可否や起動先を判断するための、別個の正式な状態情報としては使用しない。

固定ブート・復旧は、`internalPhase`だけで起動先を決めない。上表の名前付きの起動判断用情報（boot-critical marker）、識別情報、SecurityGenおよび各レコードの完全性を使用する。

固定ブート・復旧の確定状態をOTA更新へ反映する場合も、Boot Handoffまたは確定済みの記録を根拠とする。同じ進捗をOTA更新が独自に再確定しない。

`RECOVERY_PREPARING`および`RECOVERY_OVERWRITE`は固定ブート・復旧の実行・診断状態であり、自動復旧を新たに許可する独立した識別情報ではない。リセット後に復旧を継続できるかは、固定ブート・復旧が、`RECOVERY_REQUIRED`、`RECOVERY_BOOT_PENDING`、イメージの識別情報、その他の名前付きの起動判断用情報と検証結果から再判定する。

### 6.2 永続管理情報

起動・復旧に必須となる小容量のOTA状態情報は、永続データ管理の`CRIT_OTA_BOOT`へ保存する。少なくとも次の情報を、必要に応じて電源断をまたいで保持する。

- otaStateFormatVersion
- otaTransactionId
- otaOfferId
- `internalPhase`（OTA更新が管理する処理段階。起動可否・起動先を判断するマーカーではない）
- OTA更新が管理する、正常確認済みの現在のファームウェアの識別情報
- 更新候補の識別情報・ハッシュ・長さ
- 直前正常版の識別情報・ハッシュ・長さ
- `recoveryEligible`（OTA更新が決定するPREVIOUS自動復旧許可）
- 更新候補の取得完了フラグ
- 直前正常版の検証完了フラグ
- `INSTALL_REQUESTED`
- `PRIMARY_OVERWRITE_STARTED`
- TRIAL_ARMED
- trialAttempted
- TRIAL_BOOT_VALIDATED
- otaSettlementPending
- `RECOVERY_REQUIRED`、固定ブート・復旧の復旧実行状態、`RECOVERY_BOOT_PENDING`
- 本体のOTA最終結果
- `otaResultReportState`
- 失敗した更新候補の再提示を抑制するための情報
- `BOOT_FAILURE_STATUS`や現在の起動試行等、固定ブート・復旧および起動・終了・再起動管理が起動判定に必要とする参照状態
- OTA開始時の単調時刻の基準、または期限管理に必要な情報

`INSTALL_REQUESTED`と`PRIMARY_OVERWRITE_STARTED`は、別々の確定状態として保持する。電源断・リセット後も、PRIMARYが未変更のインストール要求段階なのか、既存内容の書換えを開始した段階なのかを、一意に区別できるようにする。

固定ブート・復旧の起動可否・起動先の判断では、`PRIMARY_OVERWRITE_STARTED`、`TRIAL_ARMED`、`trialAttempted`、`TRIAL_BOOT_VALIDATED`、`RECOVERY_REQUIRED`、`RECOVERY_BOOT_PENDING`等の名前付きの起動判断用情報を基準とする。`internalPhase`とこれらの情報が矛盾する場合、`internalPhase`から安全側の値を推測せず、固定ブート・復旧の矛盾状態に対する処理へ移る。

`RECOVERY_BOOT_PENDING`は、固定ブート・復旧が復元したPRIMARYの検証後に確定する、起動判断用の情報である。`RECOVERY_BOOT`は、OTA更新が復元した本体アプリケーションの正常起動を確認している処理段階を表す。両者を同じフィールド、または同じ時点で確定する情報として扱わない。

`recoveryEligible`は、OTA更新がPREVIOUSの識別情報、現在のSecurityGen、およびOTA更新処理との対応に基づいて決定する、自動復旧の許可に関する情報である。固定ブート・復旧は、この情報を自動復旧の許可判断に使用する。それとは独立して、イメージの署名、識別情報、SecurityGen等の起動前検証を実施する。永続データ管理は、意味を再判定せずに保存する。

`otaResultReportState`は、少なくとも`NOT_READY`、`PENDING`、`SENT`を区別する。

| 状態 | 条件 |
| --- | --- |
| NOT_READY | 本体の最終結果を確定する前 |
| PENDING | 本体の最終結果を確定した後で、VPSの受領を確認する前 |
| SENT | VPSが同一の結果を正常に受領したことを、VPS接続・デバイス認証から確認した後だけ |

UNKNOWNをSENTへ読み替えない。

固定ブート・復旧および起動・終了・再起動管理が管理する`BOOT_FAILURE_STATUS`や起動試行の意味を、OTA更新が再定義しない。OTA更新は、一連の更新処理との対応付けに必要な範囲で参照する。

物理保存には、永続データ管理のA/Bレコード・CRC・確定マーカーの方式と、セキュリティ管理の状態保護用MACの方式を使用する。

### 6.3 commit pointの考え方

OTA成功は一つの書込みだけでは確定しない。

1. candidate trial bootの正常確認を`TRIAL_BOOT_VALIDATED`として原子的に保持する。
2. 必要なSecurityGen更新を確定する。
3. OTA更新の正常確認済み現在のfirmware role/metadataをcandidateへ更新する。
4. OTA final resultを`CONFIRMED`／SUCCESSとして永続確定し、`otaResultReportState=PENDING`とする。
5. `otaSettlementPending=false`を一貫した最終状態として公開する。

`TRIAL_BOOT_VALIDATED`以後に電源断してもPREVIOUSへrollbackしない。次回起動で成功確定処理を継続する。

製品情報管理のrunning firmware identityは実際に実行中のsigned image metadataから得るため、OTA更新の現在のfirmware role更新だけでrunning identityを書き換えるものではない。

---

## 7. 更新情報確認と更新対象判定

### 7.1 manifest取得

VPS接続・デバイス認証のOTA manifest APIを使用する。manifest transport上限は8 KiBとし、manifest受信だけではOTA更新開始としない。

### 7.2 対象判定

少なくとも次を確認する。

- target Device IDが製品情報管理の自機Device IDと一致
- firmware種別がTank_Robot本体アプリケーション
- product/model/hardwareProfileが製品情報管理のProduct Descriptorと一致
- imageLengthが768 KiB slot条件内
- 対応可能image format
- 現在fixed bootの`bootInterfaceVersion`がcandidateの`bootInterfaceMin/Max`内
- SecurityGenが現許可値以上
- 現在の正常確認済みFWと同一identityではない
- 同一`otaOfferId`のfailed-offer suppression対象ではない
- config/persistent compatibilityを確認可能

不一致は更新開始せず、現在正常FWを変更しない。

### 7.3 同一失敗offerの自動再実行禁止

一度のOTA取引が更新失敗または復旧成功で終了した場合、`otaOfferId`と対象firmware identityをfailed offerとして保持し、同一offerを通常の定期確認だけで自動再実行しない。

管理者・開発者が原因を確認したうえで同一firmwareを明示的に再配信する場合、VPSは新しい`otaOfferId`を発行できる。firmware binary、BuildIDまたはimageHashそのものを無意味に変更する必要はない。

新しいofferは新しいOTA取引として開始条件・署名・SecurityGen・互換性をすべて再確認する。これは失敗した同一offerの自動再試行ではない。

---

## 8. OTA更新開始条件とシステム状態連携

### 8.1 OTA-ready条件

OTA更新管理は次をすべて確認した場合だけ`otaReady=true`をシステム状態管理へ提供する。

1. 現在システム状態がBLE接続待機状態。
2. 停止・緊急停止管理のアクチュエータ停止完了条件成立。
3. 緊急停止要求不成立。
4. 異常管理から通常操作を禁止する未処置異常なし。
5. バッテリー・電気安全監視からOTA更新開始可能。
6. VPS接続・デバイス認証でWi-Fiおよび認証済みVPS通信利用可能。
7. 有効なmanifestあり。
8. CANDIDATE/PREVIOUS/`CRIT_OTA_BOOT`に必要な記憶領域使用可能。
9. 現在のPRIMARYが正常確認済みで、PREVIOUSへ退避可能。
10. 固定ブート・復旧機能とセキュリティ管理の検証鍵が使用可能。
11. config/persistent compatibilityを確認可能。
12. 設定管理で設定適用中ではなく、`CONFIG_COMMIT_PENDING`も存在しない。現在使用する設定と、巻戻し防止のための世代の下限値が一意に確定している。
13. 工場出荷状態への初期化、所有者管理情報の更新確定、その他の排他対象となる管理変更処理が実行中でない。

システム状態管理が最終的にOTA更新状態への遷移を確定する。

システム状態管理がOTA更新状態への遷移を確定した時点を、本書のOTA全体時間計測開始点とする。公開用システム要求仕様「22. 性能・品質・耐久性」の`SYS-QUAL-035`の15分上限を満たすため、OTA管理は全体deadlineを管理する。

### 8.2 電気安全条件

バッテリー・電気安全監視の完成済み基本設計に従い、初版ではOTA更新開始可能条件を次とする。

- battery voltage 7.6 V以上が5秒継続
- battery温度10 ℃以上40 ℃以下
- 必須監視正常
- 警告なし
- 保護ラッチなし
- アクチュエータ電源停止

OTA中はバッテリー・電気安全監視が継続可能性と条件喪失を通知する。OTA管理が独自の電圧・温度しきい値を重複判定しない。

### 8.3 更新後起動確認未完了の操作禁止契約

candidate install後の再起動では、システム状態管理の既存契約に従い、更新後正常起動確認未完了の間は操作可能状態へ遷移しない。

OTA管理はcandidate起動開始から`CONFIRMED`まで`otaSettlementPending=true`を提供する。BLE接続待機状態へ到達したこと自体はtrial正常確認条件の一つであるが、それだけを理由として通常操作を開かない。

`TRIAL_BOOT_VALIDATED`の原子的保存、必要なSecurityGen/現在のOTA metadata/final result確定を終え、`CONFIRMED`となった後にだけ`otaSettlementPending=false`を提供する。

---

## 9. 更新候補ファームウェア取得

### 9.1 取得方式

VPS接続・デバイス認証で確定したHTTPS/HTTP Range方式を使用し、1回のRange requestを**16 KiB**とする。

image全体をRAMへ保持せず、順次CANDIDATEへ書き込む。

### 9.2 取得順序

1. CANDIDATEを新transaction用として無効化する。
2. otaTransactionIdを払い出す。
3. otaOfferIdとexpected firmware identity/imageLength/imageHashを保持する。
4. offset 0から16 KiB単位でRange要求する。
5. response offset/length/firmwareIdを確認する。
6. 受信dataを固定バッファからCANDIDATEへ書く。
7. 次offsetへ進む。
8. 全量受信後にCANDIDATE全体をreadbackする。
9. SHA-256、署名、metadataを検証する。
10. すべて正常時だけ`CANDIDATE_READY`とする。

最終chunkは16 KiB未満を許可する。

VPS接続・デバイス認証はTCP/TLS接続確立retryとHTTP requestを分離する。Range requestを送信した後のtimeout・切断・UNKNOWNをVPS接続・デバイス認証が暗黙再送しない。初期製品ではOTA更新も同じOTA transaction内で当該Range requestをapplication自動再送せず、firmware取得失敗として第9.3節へ移る。

HTTP 429/5xx等のretryable application結果をVPS接続・デバイス認証から受けた場合も、初期製品のOTA image取得では現在取引を失敗として終了する。短時間に同じchunkを自動連続再要求する複雑なretry policyを追加しない。

### 9.3 通信断・中断

通信断、timeout、UNKNOWN、HTTP retryable error、長さ不一致、Range不一致その他で取得を継続できない場合、CANDIDATEを`INCOMPLETE`として使用禁止にし、現在のOTA取引をdownload failureとして終了する。

初期製品では取得済みoffsetを永続resume情報として使用しない。後に新しいOTA取引を開始する場合はCANDIDATEを再初期化し、offset 0から取得する。

PRIMARYはこの段階では変更しない。

同一`otaOfferId`を定期pollだけで自動再実行しない第7.3節のpolicyを維持する。

---

## 10. 更新候補の検証とCANDIDATE確定

CANDIDATE全量取得後、少なくとも次を確認する。

- physical readback成功
- imageLength一致と768 KiB slot境界内
- SHA-256全体hash一致
- ECDSA P-256署名検証成功
- imageFormatVersion対応
- product/model一致
- hardwareProfile一致
- firmwareId/SemVer/BuildID一致
- SecurityGenが現許可値以上
- 現在fixed bootの`bootInterfaceVersion`がsigned `bootInterfaceMin/Max`内
- config schema互換
- persistent data format互換
- manifestと署名保護metadata一致

署名・SecurityGen検証はセキュリティ管理が最終判断し、製品／hardware identityは製品情報管理が提供する値と照合する。Boot Interface互換性は固定ブート・復旧の定義を使用する。OTA管理は結果を使用する。

一つでも失敗したcandidateは`CANDIDATE_READY`にしない。部分的に正常な領域だけを使用しない。

---

## 11. 直前正常ファームウェアのPREVIOUS退避

### 11.1 退避タイミング

CANDIDATE_READY後、PRIMARYを書き換える前に現在正常確認済みPRIMARYをPREVIOUSへコピーする。

### 11.2 PREVIOUS検証

退避後、少なくとも次を確認する。

- 全必要領域のreadback
- image長・format
- SHA-256
- 署名・完全性
- firmware identity
- SecurityGen
- 固定ブート・復旧のBoot Interface互換性
- product/model/hardwareProfile互換性

SecurityGenはこの時点の現許可値以上であることを確認する。

PREVIOUSの検証が失敗した場合、PRIMARYを書き換えない。

### 11.3 PREVIOUS保持

PREVIOUSはcandidate trialの正常起動確認が完了するまで絶対に上書き・消去しない。

candidateが最終確定した後も、次のOTAで新しいPREVIOUSを作成するまで物理的に保持できる。ただしcandidate確定により許可SecurityGenが上昇し、PREVIOUS SecurityGenが下回った場合は`recoveryEligible=false`として通常自動復旧に使用しない。

`recoveryEligible`の意味と更新条件はOTA更新が所有する。固定ブート・復旧はこのpolicyを独自に再計算せず、committed値を自動復旧許可条件として使用した上で、PREVIOUSの署名、identity、SecurityGenおよびBoot Interfaceを起動前に再検証する。

---

## 12. 本体アプリケーション領域書換え前の最終確認

OTA機能の本体アプリケーション側はCANDIDATE/PREVIOUS準備完了後、固定ブートへinstallを委譲するcontrolled rebootの直前に次を再確認する。

- system state OTA更新状態
- stop/safety condition維持
- バッテリー・電気安全監視のOTA継続可能条件
- candidate READY
- previous verified
- candidate SecurityGen有効
- 固定ブート・復旧のfixed boot/recovery使用可能およびBoot Interface互換
- storage/`CRIT_OTA_BOOT` metadata正常
- emergency stop/normal shutdown/higher-priority fault未発生
- 設定管理の`CONFIG_COMMIT_PENDING`その他の排他管理transaction未発生
- OTA全体deadlineまで、fixed bootでのPRIMARY overwrite、再起動、trial確認へ必要な設計余裕が残っている

初期基本値では、install要求を確定する前にOTA全体deadlineまで**5分以上**の残時間を必要とする。この残時間を確保できない場合はPRIMARYを書き換えず`FAILED_PREINSTALL`とする。

すべての条件が成立した場合、永続データ管理の`CRIT_OTA_BOOT`へ`INSTALL_REQUESTED`を原子的に保存する。このcommitが成功しない場合はcontrolled rebootを要求せず、PRIMARYを変更しない。

`INSTALL_REQUESTED`のcommit成功後、起動・終了・再起動管理へcontrolled rebootを要求する。この時点では`PRIMARY_OVERWRITE_STARTED`を成立させず、OTA機能の本体アプリケーション側からPRIMARYをerase/programしない。

controlled reboot後、固定ブート・復旧は固定ブート・復旧基本設計に従ってCANDIDATE、PREVIOUS、SecurityGen、Product Descriptor、Boot Interfaceおよびcritical stateを再検証する。再検証成功時だけdestructive phaseへ進み、失敗時はPRIMARYを変更せず旧正常PRIMARYを維持する。

---

## 13. MCUboot Overwriteを使用した更新反映

### 13.1 基本構成

FSPでサポートされるMCUboot Overwrite方式を採用し、外付けSerial NORのCANDIDATEをsecondary image source、内蔵MRAMのPRIMARYをprimary slotとして扱う。

固定ブート・復旧は通常OTAで書き換えない。

### 13.2 Tank_Robot固有復旧ラッパ

OSPI_B external storageを使用するMCUboot Swap rollbackへ依存しない。

固定ブート・復旧側が、OTA永続状態を読み、次のいずれを実行する。

- normal PRIMARY boot
- `INSTALL_REQUESTED`に基づくCANDIDATE overwrite install
- PREVIOUS recovery staging + overwrite
- wired recoveryへ移行

`INSTALL_REQUESTED`を検出した固定ブート・復旧は、CANDIDATE、PREVIOUS、SecurityGen、製品・hardware互換性、Boot Interfaceおよびcritical stateを再検証する。再検証に失敗した場合は`PRIMARY_OVERWRITE_STARTED`を成立させず、MCUboot install triggerを設定せず、PRIMARYを変更しない。

再検証が成功した場合、固定ブート・復旧がPRIMARYの最初のerase/program直前に`PRIMARY_OVERWRITE_STARTED`を`CRIT_OTA_BOOT`へ原子的にcommitする。当該commitが成功した後にだけMCUboot install triggerを設定し、Overwrite pathへ進む。

OSPI device初期化等、`boot_go()`前に必要な処理は固定ブート・復旧側の責任とする。

### 13.3 書込み後確認

Overwrite後、trial boot前に少なくともPRIMARYのreadback、image format、hash、署名、SecurityGen、Product DescriptorおよびBoot Interface互換性を固定ブート・復旧側で再確認する。

正常確認できないPRIMARYをtrial bootしない。PREVIOUS復旧へ移る。

---

## 14. 更新後の一回限りの試行起動

### 14.1 trial armと一回消費

candidate PRIMARYの書込み後検証に成功した後、固定ブート・復旧は`TRIAL_ARMED`、`trialAttempted=false`および`otaSettlementPending=true`を永続データ管理の`CRIT_OTA_BOOT`へ原子的に保存する。

`TRIAL_ARMED` commit済みで`trialAttempted=false`は「candidateがtrial-readyであり、許可された一回のtrialをまだ消費していない」状態を意味する。ここでreset/power-lossしても、次回起動で同candidateの未実施trialを一回開始できる。

candidateへbranchする**前**に、`trialAttempted=true`を原子的に永続確定する。当該確定に失敗した場合はcandidateへbranchせず、状態を一意に再評価してPREVIOUS復旧または有線復旧へ移る。

この順序により、branch直後にreset/power-lossしても次回起動時はcandidateを未試行と誤認しない。

`TRIAL_BOOT_VALIDATED`が存在しない状態で`trialAttempted=true`なら、同candidateをもう一度通常trialせずPREVIOUS復旧する。

### 14.2 trial中の操作禁止

trial boot中は通常起動処理を実行するが、正常起動確認とOTA settlementが完了するまで通常操作を許可しない。

以前のsession、走行指令、砲塔・砲身指令、PWM許可その他を復元しない。

システム状態管理がBLE接続待機状態まで遷移しても、`otaSettlementPending=true`の間は操作可能状態への遷移条件を満たしたものと扱わない。

### 14.3 正常起動確認条件

candidateが次を満たし、controlled reboot開始から**15秒以内**にBLE接続待機状態へ正常遷移した場合にtrial正常とする。

- 固定ブート・復旧のfixed boot署名・完全性・SecurityGen・Product Descriptor・Boot Interface検証成功
- 製品情報管理が提供する実running firmware identityが期待candidateと一致
- 起動・終了・再起動管理の通常起動完了
- 必須安全監視使用可能
- 設定管理が安全に使用可能なeffective configを確定
- 必須永続データを安全に解釈可能
- 通常操作禁止となる起動異常なし
- システム状態管理がBLE接続待機状態を確定

15秒は公開用システム要求仕様「22. 性能・品質・耐久性」の`SYS-QUAL-032`の再起動上限であり、OTA全体15分上限の内側に収める基本設計値とする。公開用システム要求仕様「22. 性能・品質・耐久性」の`SYS-QUAL-030`の主電源投入時5秒目標／10秒上限とは別の性能契約である。

---

## 15. 正常起動確認と更新成功確定

### 15.1 trial validationの永続確定

BLE接続待機状態への正常遷移を確認したcandidateは、まず`TRIAL_BOOT_VALIDATED`を原子的に永続保存する。

この保存完了をcandidateの起動正常確認pointとする。

`TRIAL_BOOT_VALIDATED`保存が完了するまでは`otaSettlementPending=true`を維持する。

### 15.2 final confirm

`TRIAL_BOOT_VALIDATED`後、次を順に行う。

1. candidate SecurityGenの確定要否をセキュリティ管理へ要求する。
2. 必要なら許可SecurityGenを更新し永続確定する。
3. OTA更新が所有する正常確認済み現在のfirmware role/identity metadataをcandidateへ更新する。
4. OTA final resultをSUCCESSとして永続確定し、`otaResultReportState=PENDING`とする。
5. `CONFIRMED`とする。
6. `otaSettlementPending=false`を提供可能な最終状態とする。
7. ログ・診断/状態表示・利用者通知へ結果を通知する。
8. VPS接続・デバイス認証へVPS result送信を要求する。

製品情報管理のrunning firmware identityは実際のsigned imageに基づく。第3項はOTA更新のOTA-confirmed-current状態を更新する意味である。

VPS送信失敗またはUNKNOWNはlocal OTA成功を取り消さない。

### 15.3 success後の動作

OTA成功後も自動的に操作可能状態へ移行しない。BLE接続待機状態を基本とし、通常操作は新しいBLE接続・認証・現在の安全条件および新しい有効指令によって開始する。

---

## 16. SecurityGen確定と巻戻し防止

### 16.1 trial前

candidate SecurityGenが現許可SecurityGenより大きくても、trial正常確認前は許可SecurityGenを更新しない。

これによりPREVIOUSをcandidate失敗時に復旧可能な状態に維持する。

### 16.2 trial後

`TRIAL_BOOT_VALIDATED`後にcandidate SecurityGenが現許可値より大きい場合だけ、セキュリティ管理の現在の許可済みSecurityGenを新値へ単調増加させる。

許可SecurityGenは通常OTA、工場初期化、設定変更またはPREVIOUS復旧によって小さくしない。

対象firmwareのSecurityGenが現在の許可済みSecurityGen未満なら、通常OTA・通常boot・PREVIOUS recoveryのいずれにも使用しない。

### 16.3 確定途中の電源断

`TRIAL_BOOT_VALIDATED`を先に永続化するため、SecurityGen更新途中またはOTA更新の現在のmetadata/final result確定途中にpower lossしてもcandidateを未確認と誤認してPREVIOUSへ戻さない。

次回起動ではcandidateを固定ブート・復旧で再検証して起動し、`otaSettlementPending=true`を維持したままfinal confirmの残処理を継続する。`CONFIRMED`まで完了してから通常操作可能条件へ引き渡す。

---

## 17. 本体アプリケーション領域書換え前の失敗

`PRIMARY_OVERWRITE_STARTED`が確定する前に次が発生した場合、PRIMARYを変更せず`FAILED_PREINSTALL`としてOTA取引を終了する。

- manifest不正
- download失敗
- candidate hash/署名/compatibility失敗
- PREVIOUS退避・検証失敗
- storage不足・異常
- start/continue power condition喪失
- OTA全体deadlineに対する残時間不足
- `INSTALL_REQUESTED` commit失敗
- controlled reboot後のfixed boot再検証失敗
- normal shutdown要求
- 上位安全異常

`INSTALL_REQUESTED`が既に確定していても、`PRIMARY_OVERWRITE_STARTED`が未確定ならPRIMARYは旧正常FWのままである。fixed boot再検証失敗時はBoot Handoff等でinstall失敗をapplicationへ引き継ぎ、OTA更新管理が`FAILED_PREINSTALL`として最終確定する。

PRIMARYを変更していないため、現在正常FWを継続使用する。

candidate途中dataをcomplete imageとして扱わず、後に新しいOTA取引を開始する場合はbyte 0から再取得する。

同一`otaOfferId`を定期pollだけで自動的にOTA処理全体として再実行しない。

---

## 18. 書換え開始後の失敗とPREVIOUS復旧

### 18.1 recovery-required条件

`PRIMARY_OVERWRITE_STARTED`以後、candidateが`TRIAL_BOOT_VALIDATED`になる前に次が発生した場合は、ただし第14.1節の「TRIAL_ARMED正常commit済みかつtrialAttempted=false」の未実施trial状態を除き、`RECOVERY_REQUIRED`とする。

- overwrite失敗
- PRIMARY検証失敗
- trial boot起動失敗
- trial boot timeout
- trialAttempted確定後のreset/power loss
- candidate起動中の正常確認不能
- OTA全体deadline超過によりcandidate正常確定を完了できない
- trial中に通常終了が要求され、candidate未確定のまま停止する場合

`TRIAL_BOOT_VALIDATED`後にdeadline超過等を検出した場合はPREVIOUSへrollbackしない。candidateのfinal confirmを完了し、性能要求違反を別途異常・ログとして扱う。

### 18.2 PREVIOUS復旧方式

PREVIOUS復旧は、次の順序と担当で実行する。

| 順序 | 主体 | 処理・確定点 |
| --- | --- | --- |
| 1 | 固定ブート・復旧 | OTA更新管理が確定した`recoveryEligible`および復旧の認可条件を確認し、PREVIOUSのメタデータ、ハッシュ、署名、SecurityGenおよびBoot Interfaceを再検証する |
| 2 | 固定ブート・復旧 | PREVIOUSをCANDIDATE／secondary領域へ復旧用に配置し、配置後のsecondary領域を再検証する |
| 3 | 固定ブート・復旧 | MCUboot Overwrite経路でPRIMARYへ反映する |
| 4 | 固定ブート・復旧 | 復元したPRIMARYを再検証する |
| 5 | 固定ブート・復旧 | 復元したファームウェアの識別情報とOTA取引を対応付け、`RECOVERY_BOOT_PENDING`を確定する。起動時引継ぎ情報（Boot Handoff）を作成し、復元したPREVIOUSの本体アプリケーションへ制御を移す |
| 6 | 起動・終了・再起動管理 | 本体アプリケーションの起動処理を実行し、起動正常完了または起動失敗を確定して提供する |
| 7 | システム状態管理 | 起動・終了・再起動管理の起動正常完了と他の状態条件から、BLE接続待機状態への遷移を確定する。`otaSettlementPending=true`の間は通常操作を許可しない |
| 8 | OTA更新管理 | 起動・終了・再起動管理とシステム状態管理の結果、および実行中ファームウェアの識別情報を確認する。`RECOVERY_BOOT`から`RECOVERY_SUCCEEDED`へ遷移し、本体のOTA最終結果を確定する |

上表の復旧手順では、固定ブート・復旧機能は、本体アプリケーションの通常起動完了、BLE接続待機状態または本体のOTA最終結果を確定しない。本体アプリケーションが動作できない場合の代理確定は、第18.3節に従う。

起動・終了・再起動管理機能とシステム状態管理機能も、OTA最終結果を確定しない。
OTA更新管理機能は、PREVIOUSの物理的な配置、PRIMARYの書換え、または起動先への制御移行を実行しない。

PREVIOUS原本は、OTA更新管理機能が復旧成功を確定するまで上書きしない。

### 18.3 復旧結果

#### 復旧成功時の結果確定

OTA更新管理機能は、次のすべてを確認した場合、本体のOTA最終結果を`FAILED_RECOVERED`として確定する。

- 起動・終了・再起動管理機能が、本体アプリケーションの起動正常完了を確定していること。
- システム状態管理機能が、BLE接続待機状態を確定していること。
- 実行中ファームウェアの識別情報が、期待するPREVIOUSの識別情報と一致すること。

更新候補のファームウェアを自動再試行しない。

#### 復旧失敗とする条件

次のいずれかに該当する場合は`FAILED_RECOVERY`とし、通常操作を許可しない。

- PREVIOUSを使用できない。
- 復旧のために書き換えたPRIMARYの検証に失敗した。
- 復元した本体アプリケーションを正常に起動できない。

#### 起動前の失敗記録と引継ぎ

固定ブート・復旧機能は、本体アプリケーションの起動前に検出した失敗を`BOOT_FAILURE_STATUS`へ記録する。
起動・終了・再起動管理機能と異常管理機能へ引き継げる場合は、その結果を提供する。

#### 本体アプリケーションが動作できない場合の代理確定

本体アプリケーションが動作できない最終的な復旧失敗では、固定ブート・復旧機能は、OTA更新管理機能が定めた`FAILED_RECOVERY`の条件とデータ形式に従う場合だけ、結果を代理で確定できる。
OTA結果に独自の解釈を追加しない。

起動失敗状態および有線復旧へ引き継ぐ。

#### 結果の保持とVPSへの報告

OTA更新管理機能による確定でも、上記の代理確定でも、本体のOTA最終結果を確定するときは`otaResultReportState=PENDING`とする。
復旧結果は、VPSへ送信できるかどうかにかかわらず保持する。

VPSの受領確認後に`SENT`を確定するのは、OTA更新管理機能だけとする。

---

## 19. 電源断・リセット・処理中断時の復旧

### 19.1 起動時判定表

| 永続段階 | 次回起動時の基本処理 |
| --- | --- |
| IDLE/INFO_READY | 現在のnormal FW起動 |
| DOWNLOADING | candidate不完全。破棄扱い、current起動 |
| CANDIDATE_READY | current起動。OTA全体を自動継続しない |
| PREVIOUS_BACKUP | previous完成確認不能ならcurrent起動、OTA失敗 |
| INSTALL_READY | current起動。再度自動installしない |
| `INSTALL_REQUESTED`かつ`PRIMARY_OVERWRITE_STARTED=false` | fixed bootがcandidate/PREVIOUS/critical stateを再検証。正常ならinstall開始、不成立ならPRIMARYを変更せず旧正常FWを起動可能とし、install失敗を引き継ぐ |
| `PRIMARY_OVERWRITE_STARTED=true`かつ`TRIAL_ARMED`未確定 | candidateがtrial-readyであることを証明できないためPREVIOUS復旧 |
| `TRIAL_ARMED=true`かつ`trialAttempted=false` | 公開用システム要求仕様「15. OTA更新」の`SYS-OTA-104`に従い、candidateの許可された一回のtrialを開始する。branch直前にtrialAttemptedをcommitする |
| `TRIAL_ARMED=true`かつ`trialAttempted=true`、未VALIDATED | 同candidateを再trialせずPREVIOUS復旧 |
| TRIAL_BOOT_VALIDATED | candidate起動、`otaSettlementPending=true`でfinal confirm継続 |
| CONFIRM_COMMITTING | candidate起動、`otaSettlementPending=true`でfinal confirm継続 |
| `RECOVERY_REQUIRED` / 固定ブート・復旧のrecovery execution status | 固定ブート・復旧が名前付きmarkerと検証結果からPREVIOUS復旧を継続 |
| `RECOVERY_BOOT_PENDING` | 固定ブート・復旧が復元PRIMARY identityを再検証し、再copyせずrecovery bootを継続 |
| `RECOVERY_BOOT` | 起動・終了・再起動管理/システム状態管理が復元Application起動・状態を確定し、OTA更新が復旧結果を確定 |
| `FAILED_RECOVERY` | normal operation禁止、有線復旧 |

`INSTALL_REQUESTED`だけが成立した段階はnon-destructiveであり、resetそのものをcandidate失敗またはPRIMARY破損として扱わない。fixed bootでの再検証結果に従ってinstall開始または旧正常PRIMARY起動を選択する。

`TRIAL_ARMED`が正常commit済みで`trialAttempted=false`の場合は「試行済み」ではない。電源断・reset後も一回のcandidate trialを開始し、branch前に`trialAttempted=true`をcommitする。これにより、公開用システム要求仕様「15. OTA更新」の`SYS-OTA-104`の「未実施なら正常起動確認を開始、実施済みならPREVIOUS復旧」を満たす。

一方、overwrite後にTRIAL_ARMEDまで正常commitできたか不明な状態ではtrial-readyを推測せずPREVIOUS復旧側へ倒す。

### 19.2 recovery中のpower loss

PREVIOUS原本を保持したまま復旧するため、recovery overwrite中のpower lossでは次回起動時にPREVIOUSから復旧処理をやり直せる。

これはcandidateの再試行ではなく、安全な正常FWを再構築する復旧継続として扱う。

### 19.3 固定ブート保護

OTAのいずれの状態でも固定ブート・復旧領域をerase/program対象にしない。

---

## 20. 通常終了・緊急停止・異常・電源保護との連携

### 20.1 優先順位

次をOTAより優先する。

1. 強制電源遮断・危険電源保護
2. 緊急停止
3. 通常操作禁止異常への安全処理
4. 通常終了
5. OTA継続

### 20.2 normal shutdown

PRIMARY書換え前ならcandidateを無効化しOTA取引を終了して、通常終了へ移行可能結果を返す。

PRIMARY書換え開始後またはtrial未確定時は、現在の最小flash操作を安全に終了できる範囲で終了し、次回起動時に永続データ管理/固定ブート・復旧のcommitted stateからtrial未実施／試行済み／復旧必要を一意に判定できる状態を保持して通常終了へ引き継ぐ。

`TRIAL_ARMED=true`かつ`trialAttempted=false`が正常commit済みなら、単にshutdown/resetしたことだけで`RECOVERY_REQUIRED`へ書き換えず、第19章の未実施trial契約を維持する。trialAttempted後に未VALIDATEDで終了する場合はPREVIOUS復旧必要を保持する。

ただし電源条件が重要data書込み継続不能となった場合は、metadata完了を待つことよりバッテリー・電気安全監視/電源保護を優先する。次回起動で状態を一意に確認できない場合はPRIMARYが安全と推測せずPREVIOUS復旧／有線復旧側を選択する。

trial boot後にBLE接続待機へ到達していても`otaSettlementPending=true`の間に通常終了要求を受けた場合、candidate未VALIDATEDならtrialAttempted状態に応じて第19章へ従い、VALIDATED後ならcandidate確定処理を安全に完了可能な範囲で継続して終了へ引き継ぐ。

### 20.3 emergency stop

OTA中も本体側緊急停止入力を検出・処理できる。

OTA通信、hash、signature、OSPI/MRAM処理は緊急停止検出・出力無効化をマスクしない。

OTA中に受信した走行・砲塔・砲身操作をqueueへ保存せず、更新後に実行しない。

### 20.4 low-power

OTA更新状態または`otaSettlementPending=true`では低消費電力移行を開始しない。OTA settlement完了後、BLE接続待機状態から通常の未操作監視を新たに行う。

---

## 21. 設定データ・永続データ互換性

### 21.1 config互換

candidateは少なくとも次を満たすこと。

- 現在effectiveなconfig schemaVersionを解釈可能
- FACTORY schemaVersionを解釈可能
- 現在effective設定が不正な場合に設定管理のfallback規則を使用可能
- OTA開始時点で設定管理の`CONFIG_COMMIT_PENDING`が存在せず、設定永続確定取引が一意に完了している

candidateが現在設定を解釈できないことを理由に、trial起動中に不可逆な設定変換を行わない。

設定管理が代替設定としてPREVIOUS/FACTORYを使用している場合も、現在の設定の適用元、schemaVersion、`highestCommittedGeneration`の正当性を確認できる場合だけ、互換性判定に使用する。巻戻し防止のための世代の下限値を確認できない状態では、通常OTAを開始しない。

### 21.2 persistent data互換

初期製品では、candidateがtrial未確定の間、PREVIOUS firmwareによる復旧を不可能にする不可逆persistent format migrationを禁止する。

必要なデータ変換がある場合は次のいずれかであること。

- old/new双方が解釈可能な互換format
- additive changeで旧FWが既存必須dataを使用可能
- candidate確定後にのみ実行する不可逆migration

これを満たさないFWは通常OTA対象として受け付けない。

永続データ管理のcritical OTA/boot state自体をcandidate applicationが独自formatへ先行移行し、固定ブート・復旧のfixed bootが解釈不能になる設計を禁止する。boot/recoveryに必要なstate format変更はfixed boot互換性と一体で設計する。

### 21.3 rollbackとSecurityGen

PREVIOUS復旧はOTA失敗からの復旧であり、通常の旧版downgradeとは区別する。ただしSecurityGen条件は常に満たす必要がある。

---

## 22. 有線復旧

### 22.1 目的

固定ブート・復旧機能またはPREVIOUSによる自動復旧でも起動可能FWを確保できない場合、開発・保守者が本体へ直接接続して復旧できる手段を設ける。

### 22.2 基本方式

RA8M2内蔵Boot Firmwareのserial programming機能とRenesas Flash Programmerを使用する方式を基本とする。

製品基板には少なくとも次へアクセス可能なservice padまたは保守connectorを設ける方向とする。

- SCI serial programming用RX/TX
- MD/boot mode制御
- RESET
- GND
- 必要な基準電源/level条件

USB serial programmingを使用可能な構成も候補とするが、初期製品では専用USB回路を増やさずに済む2-wire SCIを優先候補とする。

端子、connector、level shifter、保守治具はハードウェア詳細設計で確定する。

製品化時のsecurity lifecycle、debug/programming protection設定によってBoot Firmware programming可否が変わり得るため、量産security設定を確定する前に「製品で必要な有線復旧を残しつつ、通常利用者からは使用できない」構成を検証する。

### 22.3 制限

有線復旧は通常利用者向け機能ではない。通常運用環境で不要なprogram/debug機能を利用可能なままにしないというセキュリティ要求に従い、物理アクセス・boot mode手順・製造/保守運用を制限する。

有線復旧後も署名済み正規FW、安全な設定、必要な製品情報の整合確認を行ってから通常運用へ戻す。

---

## 23. 状態表示・ログ・診断・VPS結果送信

### 23.1 状態表示へ提供する状態

少なくとも次を提供する。

- OTA更新中
- candidate取得中
- candidate検証中
- install準備中
- PRIMARY反映中
- 更新後再起動予定
- trial確認中
- OTA最終確定中
- 更新成功
- 更新失敗
- PREVIOUS復旧中
- 復旧成功
- 復旧失敗／有線保守必要

利用者向け本体表示の具体色・patternは状態表示・利用者通知基本設計に従う。

### 23.2 ログ

ログ・診断へOTA eventの意味、priority、`otaTransactionId`等の相関情報を提供する。`otaTransactionId`はuint32なので64 bit `correlationId`へzero-extendして使用できる。128 bit `otaOfferId`をcorrelationIdへ単純切詰めせず、必要な場合はevent payload／診断へ完全値または安全な参照を保持する。

正常なOTA lifecycle/progressは原則NORMAL eventとし、少なくとも次を記録する。

- manifest/otaOfferId取得結果
- candidate identity
- download開始/完了
- candidate検証正常結果
- PREVIOUS退避正常結果
- `INSTALL_REQUESTED`確定およびcontrolled reboot要求
- fixed boot再検証正常結果
- PRIMARY overwrite完了
- trial開始
- TRIAL_BOOT_VALIDATED確定
- OTA成功確定

少なくとも次はIMPORTANT eventとする。

- download／hash／署名／SecurityGen／互換性検証失敗
- PREVIOUS退避・検証失敗
- `PRIMARY_OVERWRITE_STARTED`確定（destructive boundaryの解析証跡）
- PRIMARY overwrite／検証失敗
- trialAttempted確定後のtrial失敗・timeout・reset
- PREVIOUS recovery開始・失敗・成功結果
- SecurityGen確定失敗
- OTA final confirm異常
- wired recovery必要
- power/reset中断からの異常復旧判定
- OTA deadline超過

各担当機能/ログ・診断がより強いpriorityを要求するeventはその指定に従う。正常progressを一律IMPORTANTとして重要ringを消費しない。

秘密鍵や署名生成鍵等をログへ含めない。

### 23.3 VPS結果送信

local final result確定後、VPS接続・デバイス認証のOTA result APIへ送信し、`otaResultReportState=PENDING`を維持する。

少なくとも次を送信対象とする。

- otaTransactionId
- otaOfferId
- firmwareId/SemVer/BuildID
- SUCCESS / FAILED_PREINSTALL / FAILED_RECOVERED / FAILED_RECOVERY
- failure stage/reason
- trial boot result
- recovery result
- local result確定時刻または発生順序

VPSが同一logical resultを正常受領したことをVPS接続・デバイス認証から確認した場合だけ`otaResultReportState=SENT`とする。HTTP request送信完了、TLS送信完了、response UNKNOWNだけではSENTにしない。

response確定前の切断やtimeoutではVPS接続・デバイス認証がUNKNOWNを返し、通信層判断だけでPOSTを暗黙再送しない。PENDING結果は次の認証済みVPS管理通信機会にOTA更新が再送要求できる。再送時は同じ`otaTransactionId`、`otaOfferId`、同じlogical final resultを使用し、新しいVPS接続・デバイス認証の`requestId`を使用する。VPS APIは同一OTA transaction結果を冪等に扱うことを要求する。

OTA result APIのPENDING/SENTはOTA更新が意味と遷移を所有し、永続データ管理が物理保存するOTA管理stateであり、ログ・診断のevent logのUNSENT/SENTとは別である。ログ・診断には同じ最終結果をeventとして通知し、そのeventはログ・診断自身のlog送信policyに従う。

VPS結果送信失敗だけを理由としてlocal OTA result、CONFIRMED、FAILED_RECOVERED等を変更したりfirmwareをrollbackしたりしない。

---

## 24. 実行時間・CPU・RAM・通信・固定資源方針

### 24.1 時間上限・目標

| 項目 | 基本値 |
| --- | ---: |
| manifest HTTP request | VPS接続・デバイス認証に従い30秒以内 |
| image取得全体 | 10分以内 |
| candidate full verify | 60秒以内 |
| PREVIOUS backup＋verify | 60秒以内 |
| PRIMARY overwrite＋verify | 60秒以内 |
| controlled reboot開始からtrial正常確認 | 15秒以内 |
| final confirm | 15秒以内を目標 |
| PREVIOUS復旧開始から正常起動 | 120秒以内 |
| OTA更新状態への遷移から更新後正常起動確認・最終確定 | 15分以内 |

公開用システム要求仕様「22. 性能・品質・耐久性」の`SYS-QUAL-035`の15分はhard upper boundとして扱う。各stageの個別timeoutの単純合計を使って15分を超える実行を許可しない。trial再起動は公開用システム要求仕様「22. 性能・品質・耐久性」の`SYS-QUAL-032`の再起動上限15秒にも従う。PREVIOUS復旧120秒は公開用システム要求仕様「22. 性能・品質・耐久性」の`SYS-QUAL-036`の暫定要求値である。

PRIMARY overwrite開始前は残時間5分未満ならdestructive phaseへ入らない。destructive phase開始後にdeadlineへ到達し、candidateがまだ`TRIAL_BOOT_VALIDATED`でなければ更新失敗としてPREVIOUS復旧へ移る。`TRIAL_BOOT_VALIDATED`後はrollbackせずfinal confirmを完了し、15分超過自体を性能異常として記録する。

上表の時間、5分残時間、768 KiB image上限、16 KiB Range、8 KiB manifest、1 MiB CANDIDATE/PREVIOUS等は、評価確定待ちを含むOTA更新または参照担当基本設計で管理する値である。実機評価で成立しない場合は詳細設計だけで変更せず、該当基本設計へ評価結果を反映する。

### 24.2 固定資源

| 資源 | 基本方式 |
| --- | --- |
| OTA transaction | 1件 |
| candidate | 1 image |
| previous | 1 image |
| OTA result pending | 1 logical final result |
| HTTP Range buffer | 16 KiB固定 |
| manifest buffer | 8 KiB以下 |
| hash/signature working area | library/FSP要求に合わせ固定確保 |
| queue | 無制限queueなし |
| dynamic allocation | 標準malloc/freeなし |

初期製品では複数OTA resultを無制限queueへ積まない。新しいOTA開始前に、既存PENDING resultをVPSへ報告可能なら先に報告する。報告できない場合でもlocal resultを消失させず、次のOTA開始可否はOTA state／永続容量／運用policyを再確認して決める。詳細設計でPENDING resultを無断上書きしない。

OSPI転送でDMAC等を使用可能な場合はCPU負荷低減に使用できるが、安全関連処理の優先度と競合しない構成を実機評価する。

### 24.3 watchdog

長時間hash、OSPI copy、MRAM write中もIWDT等のsystem監視を不必要に停止しない。

固定ブート・復旧/電源・省電力管理の3秒監視契約と整合し、watchdogを単に長大timeoutへ変更してOTA処理停止を隠さない。長処理はbounded chunk化し、正常進捗を確認できる点でのみIWDT更新を許可する。

---

## 25. ビルド・署名・リリース構成管理

### 25.1 固定する識別情報

リリース成果物は少なくとも次と対応付ける。

- source commit
- firmwareId
- SemVer
- BuildID
- 製品情報管理の`firmwareBuildFingerprint`
- SecurityGen
- FSP version
- MCUboot version/configuration
- compiler/toolchain version
- linker/memory map version
- productFamilyId/modelId/hardwareProfileId
- bootInterfaceMin/Max
- config schema support range
- persistent format support range
- image SHA-256
- signature public key generation/profile

製品情報管理のProduct Descriptorとapplication signed metadataは同一のbuild/release sourceから生成し、固定ブート・復旧のfixed boot用descriptorとの互換性をbuild時に検査可能とする。

### 25.2 MCUboot構成

選定時点でRenesas FSPがRA8M2に正式対応するMCUboot integrationを使用する。

MCUbootの特定version numberを本基本設計で永久固定しない。採用versionはリリース構成情報として識別し、変更時にboot/image互換性試験を行う。

fixed boot/application間の実行互換性は単なるMCUboot release version比較ではなく、固定ブート・復旧の`bootInterfaceVersion`とsigned `bootInterfaceMin/Max`で判定する。

### 25.3 署名環境

署名生成秘密鍵はofflineまたは通常VPS実行環境から分離された開発用署名環境で管理する。

本体に格納するのは検証に必要なtrust informationであり、署名生成秘密鍵ではない。

---

## 26. 検証方針と確認ケース

### 26.1 正常系

- 対象manifest取得からcandidate成功確定まで
- 768 KiB上限image
- 小さい実image（500 KiB程度を含む）
- 同一firmwareを新otaOfferIdで管理者が明示再配信するケース
- SecurityGen同値更新
- SecurityGen増加更新
- bootInterfaceMin/Max境界一致
- config schema互換
- persistent format互換
- local SUCCESS確定後にOTA result APIが一時送信不能となり、後続VPS機会で同一transaction resultを正常送信するケース

### 26.2 candidate取得・検証失敗

- HTTP timeout
- response UNKNOWN
- HTTP 429/5xx
- Range offset/length不一致
- 通信断
- imageLength不一致
- hash mismatch
- signature failure
- target Device ID mismatch
- product/hardware mismatch
- unsupported image format
- bootInterface範囲外
- SecurityGen低下
- candidate size超過
- 同一failed otaOfferIdの定期poll再検出
- 設定管理の`CONFIG_COMMIT_PENDING`中のOTA開始拒否

全ケースでPRIMARYが変化しないことを確認する。

### 26.3 PREVIOUS退避失敗

- OSPI read/write failure
- PREVIOUS hash mismatch
- PREVIOUS signature failure
- PREVIOUS SecurityGen不適合
- PREVIOUS Boot Interface不適合

PRIMARY overwriteが開始されないことを確認する。

### 26.4 destructive phase電源断

少なくとも次の境界でpower cut/resetを注入する。

- `INSTALL_REQUESTED`保存直後
- controlled reboot直後
- fixed boot再検証中
- `PRIMARY_OVERWRITE_STARTED`保存直後
- erase中
- program途中
- overwrite完了直後
- PRIMARY検証中
- TRIAL_ARMED保存直後
- trialAttempted確定直前
- trialAttempted確定直後
- trial branch直後
- trial起動途中
- BLE_WAIT到達直後
- TRIAL_BOOT_VALIDATED保存直前
- TRIAL_BOOT_VALIDATED保存直後
- SecurityGen確定中
- CONFIRMED保存直前

特に次を確認する。

- `INSTALL_REQUESTED`段階では旧PRIMARYを破損扱いにしない。
- `PRIMARY_OVERWRITE_STARTED`後でもTRIAL_ARMEDが正常commitされていなければPREVIOUS復旧へ移る。
- TRIAL_ARMED正常commit済みかつtrialAttempted=falseでresetした場合、公開用システム要求仕様「15. OTA更新」の`SYS-OTA-104`に従いcandidateを一回trialし、branch前にtrialAttemptedをcommitする。
- trialAttempted=trueかつ未VALIDATEDでresetした場合は同candidateを再trialせずPREVIOUS復旧へ移る。
- TRIAL_BOOT_VALIDATED後はcandidate settlementを継続しPREVIOUSへ戻さない。

期待するcandidate trial／candidate settlement／PREVIOUS復旧のいずれかが一意になることを確認する。

### 26.5 recovery

- candidate起動fail
- candidate watchdog/reset
- PREVIOUS stage中power loss
- recovery overwrite中power loss
- PREVIOUS正常復旧
- PREVIOUS破損
- fixed boot recovery不可
- wired recovery
- recovery result API UNKNOWN後の後続報告

### 26.6 安全競合

- OTA download中の緊急停止
- candidate verify中の緊急停止
- PRIMARY write中の緊急停止
- trial settlement中の緊急停止
- バッテリー・電気安全監視の電源条件喪失
- 危険低電圧
- 重大温度異常
- normal shutdown要求
- force power off

安全処理がOTA通信・ログ・hash完了待ちで遅延しないことを確認する。

### 26.7 性能・耐久

- 15分OTA上限
- stage個別timeoutが全体15分を超過させないこと
- controlled reboot 15秒上限
- 120秒recovery上限
- CPU/RAM peak
- 16 KiB Range通信負荷
- OSPI/MRAM書換え寿命
- 100回以上の正常更新・失敗・power cut組合せ
- boot/recovery metadataの書換え寿命

---

## 27. 主対象要求との対応

公開用システム要求仕様「15. OTA更新」の全42要求へ次の主対応を設定する。

| 公開用システム要求仕様「15. OTA更新」の要求 | 主対応章 |
| --- | --- |
| `SYS-OTA-001～003` | 2, 3, 4, 13 |
| `SYS-OTA-010～012` | 5, 7 |
| `SYS-OTA-020` | 8 |
| `SYS-OTA-030～031` | 9, 10 |
| `SYS-OTA-040` | 10, 21 |
| `SYS-OTA-050～052` | 5, 11, 16, 18 |
| `SYS-OTA-060～062` | 4, 10, 11, 12 |
| `SYS-OTA-070～074` | 12, 13, 14 |
| `SYS-OTA-080～084` | 14, 15, 16 |
| `SYS-OTA-090～096` | 17, 18, 19, 22 |
| `SYS-OTA-100～104` | 6, 9, 17～19 |
| `SYS-OTA-110` | 20 |
| `SYS-OTA-120～122` | 23 |

### 27.1 第15.15節具体化項目

| 項目 | 本書での扱い |
| --- | --- |
| MCUboot構成・起動確認 | 13～16、25 |
| app最大容量 | 4章：768 KiB |
| memory配置・媒体 | 4章。物理addressは詳細設計 |
| MCUboot image生成 | 5, 25。具体commandは詳細設計 |
| SemVer/BuildID | 5, 25 |
| write/erase/verify | 9～13。物理単位は詳細設計 |
| boot確認時間・recovery条件 | 14, 18, 19, 24 |
| 各stage timeout | 24 |
| 内部状態・失敗・結果・log | 6, 17～19, 23 |
| PREVIOUS保持・再利用 | 11, 16, 18, 19 |
| wired recovery | 22 |
| config/persist互換 | 8, 21 |
| download状態・途中data | 9, 19 |
| 内部interface | 3, 8, 20, 23 |

---

## 28. 設計判断・後続事項

### 28.1 設計判断

| ID   | 設計判断 |
| ---- | --- |
| D-01 | Renesas FSPでサポートされるMCUboot Overwrite＋OSPI_B secondaryを基本とし、Tank_Robot独立PREVIOUS復旧を重ねる |
| D-02 | PRIMARY署名付きimage slot上限を768 KiBとする |
| D-03 | 製品外付けmemoryを採用FSP/MCUbootから使用可能なRA8M2 OSPI_B接続Serial NOR 8 MiB以上とし、Octal/QSPI・型式はhardware設計で決定する |
| D-04 | 内蔵MRAMは、固定boot/recovery executable＋trust領域最大224 KiB、`PERSIST_CRITICAL` 32 KiB、PRIMARY application slot最大768 KiBの論理境界とする |
| D-05 | CANDIDATEとPREVIOUSを独立領域とする |
| D-06 | downloadは16 KiB Range、resumeなしとし、通信断・timeout・UNKNOWN・retryable HTTP errorでは当該OTA取引を失敗として後の新取引でbyte 0から再取得する |
| D-07 | ECDSA P-256＋SHA-256の署名済み完全imageのみ扱う |
| D-08 | trial bootは一回だけとし、TRIAL_ARMED正常commit済み＋trialAttempted=falseなら未実施trialを一回実行し、candidate branch前にtrialAttemptedを原子的確定する |
| D-09 | trialAttempted=trueかつ未VALIDATEDでは同candidateを再trialせずPREVIOUS復旧し、`TRIAL_BOOT_VALIDATED`をSecurityGen更新より先に永続確定する |
| D-10 | candidate未確定中はPREVIOUS互換性を壊す不可逆persistent migrationを禁止する |
| D-11 | 同一failed `otaOfferId`を定期pollだけで自動再実行しない。管理者の新offerによる同一FW明示再配信は許容する |
| D-12 | wired recoveryはRA8M2 Boot Firmware serial programming＋RFPを基本とする |
| D-13 | OTA中およびotaSettlementPending中はlow-powerへ移行せず、安全処理を最優先する |
| D-14 | local OTA resultとVPS result report stateを分離し、UNKNOWN時はPENDINGを維持する |
| D-15 | FSP/MCUboot versionはrelease構成として固定・追跡し、基本設計では永続的な特定versionへ固定しない |
| D-16 | `otaSettlementPending`をCONFIRMEDまで維持し、BLE接続待機へ到達した瞬間に通常操作を開かない |
| D-17 | OTA更新状態遷移から15分の全体deadlineを管理し、PRIMARY書換え前の残時間不足ではdestructive phaseへ入らない |
| D-18 | CANDIDATE/PREVIOUS準備完了後、OTA applicationがcontrolled reboot前にnon-destructive `INSTALL_REQUESTED`を原子的にcommitする |
| D-19 | `PRIMARY_OVERWRITE_STARTED`はfixed bootがfirst PRIMARY erase/program直前にcommitし、その成功後にMCUboot install triggerを設定する |
| D-20 | fixed boot/application互換性は固定ブート・復旧の`bootInterfaceVersion`とsigned `bootInterfaceMin/Max`で判定する |
| D-21 | 製品情報管理をDevice ID、Product Descriptor、running firmware identityの正式な管理元とし、OTA更新はOTA確定済み現在FW／candidate／previousの役割と確定状態を所有する |
| D-22 | 設定管理の`CONFIG_COMMIT_PENDING`その他の設定確定中は通常OTA開始しない |
| D-23 | OTA lifecycle正常progressはログ・診断のNORMALを基本とし、失敗・復旧・destructive boundary等をIMPORTANTとして重要ringを過剰消費しない |

### 28.2 評価事項

| ID | 評価事項 |
| --- | --- |
| E-01 | 選定FSP/MCUbootのRA8M2 Overwrite＋OSPI_B secondary成立性 |
| E-02 | fixed boot/recovery executable＋trustが224 KiB以内に収まること |
| E-03 | 768 KiB slotでheader/trailer/alignmentを含め成立すること |
| E-04 | 8 MiB以上Serial NOR候補の入手性、WSON/SOIC等の実装性、erase/program寿命 |
| E-05 | Quad/Octal protocolと選定FSP/MCUboot driverのsecondary storage成立性 |
| E-06 | candidate full verify 60秒以内 |
| E-07 | previous backup+verify 60秒以内 |
| E-08 | PRIMARY overwrite+verify 60秒以内 |
| E-09 | controlled reboot開始からtrial正常確認15秒以内、公開用システム要求仕様「22. 性能・品質・耐久性」のSYS-QUAL-032との整合 |
| E-10 | recovery120秒以内 |
| E-11 | OTA全体15分以内 |
| E-12 | power cut各境界で未実施trial／試行済み／settlement／復旧状態が一意であること |
| E-13 | OTA負荷中も停止・電源・監視性能を維持すること |
| E-14 | serial wired recovery治具・service pad実装性 |
| E-15 | 製品のセキュリティライフサイクル/programming protection設定下でも、認可された保守時だけwired recoveryを実行可能なこと |
| E-16 | OTA result POST UNKNOWN／再送時のVPS idempotencyとPENDING永続状態 |
| E-17 | 設定管理のCONFIG_COMMIT_PENDING・fallback状態とのOTA開始排他 |

E-01～E-17は採用方式・基本設計値の成立性確認である。結果により容量、時間上限、trial/recovery条件、通信retry方針、result report方針またはmemory構成を変更する場合は、詳細設計だけで変更せず本書および値・方式を定める関連基本設計へ反映する。

### 28.3 補足：外付けFlash選定

評価ボード上の64 MiB Octo-SPI品そのものを量産必須部品としない。

製品部品は次を満たせばよい。

- 8 MiB以上
- RA8M2 OSPI_Bに接続可能
- 採用FSP/MCUbootのsecondary storageとして実際に利用可能
- candidate/previous/log等のpartition要件を満たす
- 書換え寿命を満たす
- 継続調達可能
- 基板実装可能

個人試作・教材として手実装性を重視する場合、BGAよりWSON等を優先候補にできる。最終部品はhardware基本設計で決定する。

---

## 29. 他文書へのフィードバック

### F-01 システム構成・ハードウェア

外付け不揮発メモリの要求条件として、採用FSP/MCUbootから使用可能なRA8M2 OSPI_B接続Serial NOR Flash 8 MiB以上を維持し、EK-RA8M2搭載64 MiB BGA品を製品必須としない。

### F-02 永続データ管理

永続データ管理でCANDIDATE 1 MiB、PREVIOUS 1 MiB、`CRIT_OTA_BOOT`が具体化済みであり、本書へ反映した。

`CRIT_OTA_BOOT`へotaOfferId/otaTransactionId、expected firmware identity、`INSTALL_REQUESTED`、`PRIMARY_OVERWRITE_STARTED`、TRIAL_ARMED、trialAttempted、TRIAL_BOOT_VALIDATED、otaSettlementPending、RECOVERY_REQUIRED、recovery result、local OTA result、`otaResultReportState`等を一意に復旧可能な形で保持する。

`INSTALL_REQUESTED`と`PRIMARY_OVERWRITE_STARTED`は電源断後も別状態として識別可能にし、trialAttemptedとTRIAL_BOOT_VALIDATEDの組合せから公開用システム要求仕様「15. OTA更新」の`SYS-OTA-104`の未実施／試行済み判定を失わない。

### F-03 セキュリティ管理

セキュリティ管理のECDSA P-256＋SHA-256、`PK_FW_VERIFY`、SecurityGen commit順序、state MACを使用する。

firmware signed metadataへproduct/model/hardware、firmware identity、image format、SecurityGenに加えて固定ブート・復旧の`bootInterfaceMin/Max`を保護対象として含める。具体protected TLV ID/layoutは詳細設計で共通化する。

### F-04 起動・終了・再起動／固定ブート・復旧

CANDIDATE/PREVIOUS準備完了後にOTA applicationが`INSTALL_REQUESTED`をcommitしてcontrolled rebootを要求し、fixed bootが再検証後、first PRIMARY erase/program直前に`PRIMARY_OVERWRITE_STARTED`をcommitしてMCUboot install triggerを設定する責任分界を維持する。

公開用システム要求仕様「15. OTA更新」の`SYS-OTA-104`に従い、`TRIAL_ARMED=true`かつ`trialAttempted=false`は未実施trialとしてcandidateを一回起動し、branch前にtrialAttemptedをcommitする。`trialAttempted=true`かつ未VALIDATEDではPREVIOUS復旧する。

固定ブート・復旧基本設計第8章および第21.2節の起動可否・起動先の判断は、いずれもこの契約と整合済みである。trial未実施時の旧記述に関するフィードバックは反映済みとする。

`TRIAL_BOOT_VALIDATED`、`RECOVERY_REQUIRED`、recovery boot result、Boot Handoffの引継ぎ契約を維持する。

### F-05 システム状態管理

更新後起動時に`otaSettlementPending=true`の間はBLE接続待機状態に留め、通常操作可能条件を成立させない既存契約を、OTAのCONFIRMEDまでのsettlement契約と整合させる。

### F-06 VPS接続・デバイス認証／VPS API

VPS接続・デバイス認証のmanifest 8 KiB、16 KiB Range方式を維持する。

VPS接続・デバイス認証は送信済みHTTP requestを通信層判断だけで暗黙再送しない。OTA更新の初期製品ではRange requestがtimeout・切断・UNKNOWN・retryable HTTP errorとなった場合、現在のdownload transactionを失敗とし、同じOTA取引でapplication自動再送しない。後に新しいOTA取引を開始する場合はbyte 0から再取得する。

OTA result POSTは同じotaTransactionId/otaOfferId/final resultを冪等に扱えるAPIとし、VPS接続・デバイス認証のUNKNOWN時はOTA更新のPENDINGを維持して後続VPS機会に新しいrequestIdで再送できるようにする。

manifestへotaOfferIdとbootInterfaceMin/Maxを持ち、管理者の明示再配信では新しいotaOfferIdを発行する。

### F-07 設定管理

candidate trial中にPREVIOUS復旧を妨げる不可逆schema/persistent migrationを行わない互換性契約を維持する。

設定管理が`CONFIG_COMMIT_PENDING`の状態である場合、または巻戻し防止のための世代の下限値が不明な場合は、通常OTAの開始条件を満たさない。本書は、この確定済みの契約に従う。

### F-08 状態表示・ログ・診断

ログ・診断へOTA stage、trialAttempted、settlement、recovery、wired recovery必要状態、deadline超過等を提供する。

正常なmanifest/download/install/trial/success progressはNORMALを基本とし、失敗、復旧、`PRIMARY_OVERWRITE_STARTED`等のdestructive boundary、wired recovery必要、deadline超過等をIMPORTANTとする。ログ・診断で定めるpriorityを変更しない。

### F-09 保守・ハードウェア・セキュリティ設計

RA8M2 Boot Firmware serial programming用service pad/connector、保守治具、RFP手順、製品のセキュリティライフサイクルとの両立、通常利用者からのアクセス制限を具体化する。

### F-10 製品情報管理

製品情報管理をDevice ID、Product Descriptor、running firmware identityの正式な管理元として使用する。

OTA trial中も製品情報管理は実際に実行中のcandidate identityを返し、OTA更新は当該identityが期待candidateと一致することを正常起動確認へ使用する。OTA更新の「正常確認済み現在のfirmware」状態と製品情報管理のrunning identityを混同しない。

---

## 30. 初版の自己確認結果と引継ぎ条件

### 30.1 初版自己確認

初版作成後のレビューおよび後続基本設計との整合確認を含め、次を確認・修正した。

- 公開用システム要求仕様「15. OTA更新」の全42要求へ主対応章を設定した。
- 公開用システム要求仕様「15. OTA更新」第15.15節の14具体化事項へ基本方式または詳細設計責任を設定した。
- 固定ブート・復旧を通常OTA対象から除外した。
- external module firmwareをOTA対象へ追加していない。
- Renesas/FSPでサポートされるOverwrite方式を採用し、OSPI_B Swap rollbackを前提にしていない。
- Tank_Robot固有PREVIOUS復旧を、MCUboot標準機能と混同しない構成を維持した。
- CANDIDATEとPREVIOUSを独立領域とし、candidate取得・検証失敗だけでcurrent FWを失わない構成を維持した。
- 永続データ管理の`CRIT_OTA_BOOT`と固定ブート・復旧の起動可否・起動先の判断を反映し、`INSTALL_REQUESTED`を本体アプリケーション側の非破壊install要求、`PRIMARY_OVERWRITE_STARTED`をfixed boot側のdestructive markerとして分離した。
- 公開用システム要求仕様「15. OTA更新」の`SYS-OTA-104`に反していた「TRIAL_ARMED済み・trialAttempted=falseでもPREVIOUS復旧」を修正し、未実施なら一回trial、実施済み未VALIDATEDならPREVIOUS復旧とした。
- trial bootを一回だけとし、trialAttemptedをcandidate branch前に原子的確定する順序を維持した。
- BLE接続待機到達からOTA確定まで通常操作が開く隙間を作らないよう、`otaSettlementPending`をCONFIRMEDまで維持する契約を維持した。
- SecurityGenをtrial正常確認前に上げず、`TRIAL_BOOT_VALIDATED`を先に永続化する順序をセキュリティ管理と整合した。
- fixed boot/application互換性を固定ブート・復旧の`bootInterfaceVersion`とsigned `bootInterfaceMin/Max`で判定する方式へ整合した。
- 製品情報管理をDevice ID、Product Descriptor、running firmware identityの正式な管理元とし、OTA更新のOTA-confirmed-current状態と分離した。
- VPS接続・デバイス認証のHTTP request再送責任へ整合し、download時のtimeout/UNKNOWN等を同一OTA取引で暗黙再送せず、失敗後の新取引はbyte 0から取得する方式とした。
- OTA結果報告APIのPENDING/SENTを、ログ・診断のログ送信状態UNSENTと分離した。応答がUNKNOWNの場合は本体で確定した結果を維持し、後続のVPS通信機会に同一の結果を再送できる方式とした。
- failed target再試行方式は、新しい`otaOfferId`による管理者の明示再配信だけを新しいOTA取引として許容する既存方針を維持した。
- 公開用システム要求仕様「22. 性能・品質・耐久性」の`SYS-QUAL-035`の15分全体deadline、`SYS-QUAL-032`のcontrolled reboot 15秒上限、`SYS-QUAL-036`のPREVIOUS復旧120秒暫定要求を区別した。
- 設定管理が`CONFIG_COMMIT_PENDING`の状態である場合、または巻戻し防止のための世代の下限値が不明な場合は、通常OTAを開始しない契約を追加した。
- trial未確定中の不可逆persistent migrationを禁止し、boot/recovery critical state formatをcandidate applicationだけで先行変更しないよう補強した。
- safety/e-stop/power protectionをOTAより優先した。
- download resume、delta、compression、multi-image、bootloader OTA等の将来機能を初期製品へ追加していない。
- wired recoveryを独自追加bootloaderではなくRA8M2 Boot Firmware serial programmingへ分離し、製品のセキュリティライフサイクルとの成立性を評価事項へ残した。
- image上限768 KiB、manifest 8 KiB、Range 16 KiB、CANDIDATE/PREVIOUS各1 MiB、external Serial NOR 8 MiB以上の契約を維持した。
- ログ・診断/永続データ管理に合わせ、評価traceを外付けFlash用途から削除し、OTA正常progressを一律IMPORTANTにしないようpriority契約を修正した。
- 時間、容量、残時間、trial/recovery条件、通信retry方針等を評価確定待ちを含む基本設計で定める事項として明確化した。

### 30.2 初版完了判断

D-01～D-23は本基本設計で採用する方式として確定する。

E-01～E-17は採用方式・基本設計値の成立性・性能・部品選定を確認する実装・実機評価事項であり、初版基本設計を停止するユーザー判断待ち事項ではない。

F-01～F-10は関連基本設計・hardware設計・詳細設計へのフィードバックである。VPS接続・デバイス認証/設定管理/永続データ管理/セキュリティ管理/製品情報管理の後続契約は本書へ反映済みであり、固定ブート・復旧基本設計第8章および第21.2節のtrial未実施時契約も本書/公開用システム要求仕様「15. OTA更新」と整合済みである。

外付けSerial NORの具体型式・package、FSP/MCUboot具体version、OSPI pin/clock、MRAM/OSPI物理address、flash erase/program単位、protected TLV ID、RFP service connector詳細等は、基本方式を変更しない範囲で後続設計・評価に委ねる。

現時点で、OTA更新基本設計の初版完了を妨げるユーザー判断待ち事項は残していない。

---

**文書終端：全30章。基本方式は本書で確定し、詳細設計・実機評価事項および関連文書へのフィードバックは第28～30章に明記する。**
