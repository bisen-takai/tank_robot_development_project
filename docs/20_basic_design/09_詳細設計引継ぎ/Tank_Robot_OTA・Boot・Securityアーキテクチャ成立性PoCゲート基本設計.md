# Tank_Robot OTA・Boot・Securityアーキテクチャ成立性PoCゲート基本設計

## 目次

1. 基本方針
2. 技術検証（PoC）の管理項目
3. PoCで使用する評価基準の構成・版
4. 必須確認項目
5. PASS条件
6. PoC合格前の詳細設計範囲
7. 通常の`BD-EVAL-*`との分離
8. 必須証跡
9. FAIL時の扱い
10. 成立性確認に合格した後
11. 詳細設計への引継ぎ
12. GR-16完了条件

---

## 1. 基本方針

**位置付けと目的**

- **位置付け**：[「OTA更新基本設計」](../03_機能別基本設計/15_OTA更新/OTA更新基本設計.md)、[「セキュリティ管理基本設計」](../03_機能別基本設計/17_セキュリティ管理/セキュリティ管理基本設計.md)、[「固定ブート・復旧基本設計」](../03_機能別基本設計/19_固定ブート・復旧/固定ブート・復旧基本設計.md)を横断するPoCの合格条件を定める基準文書
- **目的**：採用済みOTA／Boot／Security方式が実装基盤上で動作することを確かめる試作検証（PoC）の合格条件を定める。通常の性能・容量・耐久評価とは分けて実施し、記憶領域の物理配置などの詳細設計を正式に確定する前に、この検証へ合格することを求める

OTA、Security、固定ブート・復旧の動作・状態、セキュリティ方針、および電源断からの復旧は、引き続きOTA更新基本設計、セキュリティ管理基本設計、固定ブート・復旧基本設計にそれぞれ従う。本書はこれらの担当を変更せず、**採用方式の成立性を確認し、後続の設計へ進めるかを判定する条件**だけを横断的に定める。

**対象文書**

- [`docs/20_basic_design/03_機能別基本設計/15_OTA更新/OTA更新基本設計.md`](../03_機能別基本設計/15_OTA更新/OTA更新基本設計.md) §13.1、§28.2 E-01
- [`docs/20_basic_design/03_機能別基本設計/17_セキュリティ管理/セキュリティ管理基本設計.md`](../03_機能別基本設計/17_セキュリティ管理/セキュリティ管理基本設計.md) §30.2関連
- [`docs/20_basic_design/03_機能別基本設計/19_固定ブート・復旧/固定ブート・復旧基本設計.md`](../03_機能別基本設計/19_固定ブート・復旧/固定ブート・復旧基本設計.md) §33.2 E-01、E-06、E-07、E-09、E-10

**本書と関連文書の適用範囲**

1. **本書が定める事項**：採用済みOTA・起動・セキュリティ方式の成立性を確認するPoCの合格条件と、合格前に正式確定してはならない詳細設計の範囲。
2. **関連文書が引き続き定める事項**：本書が明示的に補完する事項、または文書間で共通の規則として確定する事項以外は、本書で挙げる関連する基本設計の担当範囲と変更責任を維持する。
3. **本書を優先する場合**：上記の事項について本書で基本設計として確定した記述と、当該事項への対応前の旧記述が競合する範囲に限る。対応経緯と、比較基準の版がある場合の識別情報は、末尾の「作成経緯」に示す。
4. **本書を優先しない事項**：詳細設計へ残す内部実装、および補完対象外の状態、機能の動作、安全、セキュリティ、外部との通信・連携規則などは、それぞれを担当する基本設計に従う。

**基本方針**

初期製品では次の採用方式を維持する。

- RA8M2 CPU0 Cortex-M85でfixed bootを実行する。
- Renesas FSP／MCUbootのOverwrite方式を使用する。
- OSPI_B接続Serial NORをsecondary storageとして使用する。
- normal bootでは外付けsecondaryへ依存しないPrimary-only verifierでPRIMARYを検証する。
- firmware署名はECDSA P-256＋SHA-256を使用する。
- fixed bootはApplication Security task起動前にminimum security bootstrapを行い、SecurityGen、state MACおよびboot authorizationに必要なtrust情報を利用する。
- `PK_FW_VERIFY`、`K_SEC_STATE_MAC`、SecurityGenおよび署名保護metadataの意味はセキュリティ管理/固定ブート・復旧の契約を維持する。

これらは「未決定方式」ではない。PoCは、採用方式を自由に選び直すための比較試験ではなく、**採用方式が実装基盤上で成立することを確認し、後続工程へ進めるかを判定するための検証**である。

PoCがFAILした場合のみ、関連基本設計へ戻して代替アーキテクチャを検討する。詳細設計担当者が独自に方式を変更してはならない。

---

## 2. 技術検証（PoC）の管理項目

GR-16では、OTA・Boot・Securityを横断して採用方式の成立性を確認する項目を、次の識別子で管理する。

```text
BD-POC-001 = OTA_BOOT_SECURITY_ARCHITECTURE_FEASIBILITY
```

状態は次のいずれかとする。

- `NOT_STARTED`
- `IN_PROGRESS`
- `PASS`
- `FAIL`

初期状態は`NOT_STARTED`とする。

`PASS`は、本書第4章の必須確認項目がすべてPASSし、再現可能な証跡が保存された場合だけ成立する。

`FAIL`は、一つ以上の必須確認項目について、**PoCで使用する評価基準の構成・版では基本設計で採用した方式を実現できない**ことが確認され、設定変更、APIへの対応付け、または実装不備の修正だけでは解消できない場合に成立する。

単なる実装不具合、設定不足、PoC用コードの欠陥を修正できる状態は、直ちに採用方式そのものの`FAIL`とはしない。
ただし、無期限に`IN_PROGRESS`へ留めず、採用方式が成立しない根拠が明確になった時点で`FAIL`とする。

---

## 3. PoCで使用する評価基準の構成・版

### 3.1 PoC実施時に構成・版を固定して記録する

基本設計では、FSP／MCUbootを将来にわたって特定のversionへ固定しない。
一方、PoC結果を再現できるようにするため、PoC開始時には、評価に使用する構成と版を一意に識別できる情報として次を記録する。

- RA8M2デバイス、評価ボードまたはPoC対象ハードウェアの版
- Renesas FSPのversion
- MCUbootのsource tag、commit、またはFSP同梱versionを一意に識別できる情報
- compiler／toolchainのversion
- 使用する外付けSerial NOR部品、または評価ボード搭載Flashの識別情報
- OSPI_B設定の主要条件
- MCUboot Overwrite設定の主要条件
- image header／trailer／protected metadataに関するPoC設定
- RSIP-E50D／Protected Mode設定の主要条件

これらの構成・版が記録されていないPoC結果は、`BD-POC-001 = PASS`の証跡として使用しない。

### 3.2 構成・版を変更した場合の再確認

PoCがPASSした後に、FSP、MCUboot、RSIP連携、OSPI driver、bootutil構成その他、採用方式の成立性へ影響し得る実装基盤を変更する場合は、影響する必須確認項目を再実施する。

Application機能だけの変更など、PoCの成立条件に影響しない変更では、リリースごとにPoC全体を再実施することを必須とはしない。

---

## 4. 必須確認項目

### 4.1 `POC-01` RA8M2 Overwrite＋OSPI_B secondary成立性

目的は、第3章で記録した評価基準の構成・版において、RA8M2 CPU0 fixed bootからOSPI_B secondaryを使用したMCUboot Overwriteの基本経路が成立することを確認することである。

最低成功条件は次とする。

1. signed candidate imageをOSPI_B secondaryへ配置できる。
2. fixed boot／MCUbootが当該candidateを認識できる。
3. PRIMARYへのOverwriteを実行できる。
4. Overwrite後のPRIMARY内容が期待imageと一致することを検証できる。
5. candidate image header／trailer／erase alignmentが、第3章で記録した評価基準の構成・版で破綻しない。
6. OSPI_B secondaryを通常Application boot sourceとせず、固定ブート・復旧のPrimary boot architectureを維持できる。

本PoCではOTA全体15分、60秒stage、100回耐久等の性能・耐久合格までは要求しない。それらは`BD-EVAL-007`／`BD-EVAL-010`側で評価する。

### 4.2 `POC-02` Primary-only verifier成立性

目的は、normal boot時にOSPI_B／secondaryを利用できない、または利用しない条件でも、PRIMARYのboot authorizationに必要な検証を行えることを確認することである。

最低成功条件は次とする。

1. PRIMARY imageを内蔵Flash側から検証できる。
2. image hashおよびECDSA P-256署名を検証できる。
3. protected metadataを検証対象として扱える。
4. product／model／hardware profile、SecurityGen、Boot Interface等、セキュリティ管理/製品情報管理/固定ブート・復旧がboot authorizationへ要求するpolicyを適用できる構成である。
5. OSPI_B device absent／unavailableを理由としてvalid PRIMARYの通常bootを不要に禁止しないarchitectureを維持できる。

PoC用の関数名、bootutil API、buffer構成等は詳細設計実験としてよい。

### 4.3 `POC-03` verifier判定一貫性

目的は、candidate install前後で使用するMCUboot系検証とPrimary-only verifierが、同じ署名済みimageおよび共通policyについて矛盾した許否を返さないことを確認することである。

最低限、次の代表caseを使用する。

- 正常なsigned image：双方で許可。
- image body改ざん：双方で拒否。
- signature不正：双方で拒否。
- 対象product／model／hardware profile不一致：boot authorizationとして拒否。
- SecurityGenが現在の許可済みfloor未満：拒否。
- Boot Interface互換範囲外：拒否。

MCUboot library単体がTank_Robot固有policyを直接実装しない場合は、MCUboot verification結果＋Tank_Robot policy wrapperとPrimary-only verifier側policyを比較し、**最終boot authorization結果**が一致することを確認する。

具体test vector byte列やtest harnessは詳細設計・PoC実装で定める。

### 4.4 `POC-04` fixed boot minimum security bootstrap成立性

目的は、本体アプリケーション側PERSIST Process／Security task／TLS／BLE／Wi-Fi等へ依存せず、fixed bootがboot authorizationに必要な最小Security機能を利用できることを確認することである。

最低成功条件は次とする。

1. fixed boot contextで必要なRSIP-E50D／Protected Mode初期化を行える。
2. `PK_FW_VERIFY`を用いたfirmware signature verification pathを利用できる。
3. 永続データ管理のminimum access pathからSecurityGen／critical security stateを取得できる。
4. `K_SEC_STATE_MAC` Wrapped Keyまたはセキュリティ管理で規定した同等のboot用安全な鍵利用pathをApplication起動前に利用できる。
5. dataset-domain分離したstate MACを検証できる。
6. SecurityGen floor／critical stateを正常に検証できない場合、安全値を推測してApplicationへbranchしない。
7. full TLS、BLE認証、Wi-Fi credential処理等をfixed bootへ持ち込まず、minimum bootstrapとして分離できる。

---

## 5. PASS条件

`BD-POC-001 = PASS`とするには、次をすべて満たす。

- `POC-01 = PASS`
- `POC-02 = PASS`
- `POC-03 = PASS`
- `POC-04 = PASS`
- 第3章で定める、検証に使用した実装基盤の版・構成を記録済み。
- 第8章の最低証跡を保存済み。
- PASSのためにOTA更新/セキュリティ管理/固定ブート・復旧のセキュリティ、切戻し、試行起動、復旧の条件・処理規則を緩和していない。

性能値や容量値がまだ評価確定待ちであっても、採用方式の基本機能が成立することを確認できれば、PoCの判定をPASSとすることができる。

---

## 6. PoC合格前の詳細設計範囲

### 6.1 合格前に着手できる試作・検討

次は、PoCの実施・検討に必要な試作として、合格前から作成してよい。

- PoC用のFSPプロジェクト／MCUboot設定
- 仮の領域分割／リンカー設定案
- OSPI_Bのドライバ／コマンド／バッファの試験実装
- Primary-only verifierの試作
- RSIP minimum bootstrapの試作
- 試験用イメージ／鍵／テストベクトル
- 起動ログ／トレース取得のための実装
- PoC専用で、正式運用には使用しない構成

これらはPoCの証跡を得るための試験実装であり、正式に使用する実装の詳細設計を確定したものとは扱わない。

### 6.2 合格前に正式確定してはならない設計事項

次は、`BD-POC-001 = PASS`となる前に、正式運用で使用する実装の確定設計として扱わない。

- MRAM／OSPIの領域分割と物理アドレスの最終配置
- リンカースクリプト／ベクタの最終配置
- 正式に使用するMCUboot設定
- 正式に使用するPrimary-only verifierのAPI／bootutil構成
- OSPI_B secondaryの最終的な物理領域の対応付け／消去単位の前提
- 固定ブートの信頼領域の最終的な物理配置
- RSIP minimum bootstrapの正式な初期化手順
- `K_SEC_STATE_MAC`を起動前に使用するための、正式な鍵ハンドル／保存先の対応付け
- 更新反映／復旧の実装で、正式に使用する破壊的な書換え経路
- 正式に使用するDLM／Access Window／書込み保護と起動構成の最終的な組合せ

PoC用の仮配置、仮リンカー設定、試験ドライバは作成してよい。
ただし、PASSの証跡がない状態で、これらを正式な詳細設計として確定してはならない。

---

## 7. 通常の`BD-EVAL-*`との分離

GR-11で登録した次の評価groupは維持する。

- `BD-EVAL-007`：OTA更新のOTA時間、容量、throughput、stage deadline等。
- `BD-EVAL-008`：セキュリティ管理のSecurity運用、暗号処理時間、resource、provisioning等。
- `BD-EVAL-010`：固定ブート・復旧のboot／trial／rollback／recovery時間、容量、IWDT、power-cut、wired recovery等。

ただし次の問いは、上記`BD-EVAL-*`の通常評価ではなく`BD-POC-001`で管理する。

- Overwrite＋OSPI_B secondaryという採用architectureが実装可能か。
- Primary-only verifierという分離が実装可能か。
- install側とnormal boot側の最終authorizationを同じpolicyへ収束できるか。
- fixed bootからminimum security bootstrapを実行可能か。

一方、次はPoC PASS後も`BD-EVAL-*`で継続評価する。

- candidate verify／backup／overwrite／recoveryの時間上限。
- OTA全体15分。
- normal boot 2秒等の性能budget。
- fixed boot 224 KiB、PRIMARY 768 KiB、alignmentの最終成立性。
- crypto処理時間／RAM／stack。
- IWDTとerase/program/hash処理の両立。
- power-cut、反復、耐久、書換え寿命。
- wired recovery運用・DLMの最終成立性。

PoCのPASSを通常評価のPASSへ読み替えず、通常評価のPASSをアーキテクチャ成立性PoCのPASSの代用にもしない。

---

## 8. 必須証跡

各必須確認項目について、少なくとも次を記録する。

- `BD-POC-001`および確認項目ID
- 実施日
- 評価に使用した構成・版（version／commit／toolchain／対象ハードウェア）
- 使用した設定の識別情報
- 試験ケースと期待結果
- 実結果
- PASS／FAIL判定
- ビルドログ、または再現に必要なビルド情報
- 起動／検証ログ
- 使用したimage hash／署名テストベクトルの識別情報
- 既知の制約／未解決事項

証跡は、将来の詳細設計・評価資料から参照できる形で保存する。

実験が一度成功したという口頭確認だけでは、`BD-POC-001 = PASS`にしない。

---

## 9. FAIL時の扱い

いずれかの必須確認項目で採用方式が成立せず、`BD-POC-001 = FAIL`となった場合は、次を行う。

1. 正式運用用の領域分割、リンカー設定、起動方式、信頼情報配置を確定する作業を停止する。
2. 成立しなかった条件、評価に使用した構成・版、および再現方法を記録する。
3. OTA更新、セキュリティ管理、固定ブート・復旧、および影響する永続データ管理／電源・ハードウェア基本設計／セキュリティ横断設計へ結果を戻し、基本設計を見直す。
4. 詳細設計だけでSecurity、rollback floor、trial one-shot、power-loss recovery、wired recovery trust boundaryの条件を緩和しない。
5. 代替アーキテクチャが複数ある場合は、ユーザー判断を受けて基本設計を改訂する。
6. 改訂後のアーキテクチャについて、成立性確認の対象と必須確認項目を定義し直してPoCを再実施する。

代替アーキテクチャを本書であらかじめ固定しない。
FAIL時は、原因と実装可能な選択肢を整理し、基本設計での判断へ戻す。

---

## 10. 成立性確認に合格した後

`BD-POC-001 = PASS`となった後は、第3章で記録した評価基準の構成・版を参照しながら、正式運用用の詳細設計を具体化してよい。

ただし、PoCのPASSは、次の評価まで完了したことを意味しない。

- 性能／Deadline
- 容量／RAM／stack
- 電源断耐久
- 正式運用時のセキュリティライフサイクル／DLM設定
- OTA／復旧を含むシステムテスト全体

これらは、OTA更新、セキュリティ管理、固定ブート・復旧、懸案事項一覧`BD-EVAL-*`、および後続の試験設計に従って別途確認する。

---

## 11. 詳細設計への引継ぎ

詳細設計では、PoC PASS後に次を具体化する。

- FSP／MCUboot final configuration。
- OSPI_B command／driver mapping。
- buffer／work area。
- exact partition／linker／vector。
- Primary-only verifierの具体API／bootutil mapping。
- RSIP API／key handle／initialization sequence。
- test／production keyの環境分離。
- boot/recovery logging／diagnostic implementation。

これらの具体化でOTA更新/セキュリティ管理/固定ブート・復旧の採用方式、Security policy、boot authorization、trial／rollback／recovery semanticsを変更しない。

---

## 12. GR-16完了条件

GR-16レビュー指摘への文書修正としては、本書により次を確定した時点で対応済みとする。

- アーキテクチャの成立性を確認するPoCを通常評価から分離した。
- `BD-POC-001`と4つの必須確認項目を定義した。
- PoCで使用する評価基準の構成・版の記録条件を定義した。
- PASS条件、FAIL時に基本設計へ戻す条件、ユーザー判断が必要となる条件を定義した。
- PASS前に正式確定してはならない、正式運用用の詳細設計項目を定義した。
- `BD-EVAL-007/008/010`との責任分界を定義した。

PoCそのものの実施・PASSは、後続の詳細設計を正式に確定してよいかを判断する工程上の条件であり、GR-16の文書修正完了条件とは分けて扱う。

---

## 作成経緯

基本設計粒度レビューのGR-16への対応として、本書の補完対象を基本設計で確定した。
