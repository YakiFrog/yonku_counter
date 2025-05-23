# Yonku Counter

## 仕様
Arduinoから，PSDセンサー値とStart合図をUART通信経由で受け取り，BLEでパソコンと通信する．

## 詳細説明

### ハードウェア構成
- メインボード: Seeed XIAO ESP32S3
- 通信方式:
  - UART: 外部Arduino/センサーと通信
  - BLE: パソコンと通信

### 機能一覧
1. **UART入力処理**
   - Arduinoからのセンサーデータ（PSD）受信
   - Start信号の検出と処理
   - ボーレート: 115200bps
   - 接続ピン: RX=GPIO44, TX=GPIO43

2. **BLE通信**
   - デバイス名: "Yonku Counter BLE Server"
   - サービスUUID: "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
   - キャラクタリスティックUUID: "beb5483e-36e1-4688-b7f5-ea07361b26a8"
   - 送信データ: 
     - UARTから受信したセンサー値
     - 定期的なカウンター値
   - 受信データ: PC側からのコマンド（UART経由で転送）

3. **双方向通信機能**
   - UART→BLE: センサー値やステータス情報の転送
   - BLE→UART: 制御コマンドの転送

### データフォーマット
- PSDセンサーデータ: テキスト形式の数値
- Start信号: "START"というテキスト文字列
- BLEからの通信: テキスト形式のコマンド

### 使用方法

#### ハードウェア接続
1. **UART接続**
   - ESP32S3の GPIO44(RX) ↔ Arduino TX
   - ESP32S3の GPIO43(TX) ↔ Arduino RX
   - GNDを共通接続

2. **電源供給**
   - USB経由または外部電源

#### PCとの連携
1. BLEスキャナーで "Yonku Counter BLE Server" を検索
2. 接続後，自動的にデータ受信開始
3. コマンド送信可能（Arduino側に転送される）

#### デバッグ
- シリアルモニタ (115200bps) でログ確認可能
- 接続状態、データ送受信が表示される

## 開発環境
- PlatformIO
- フレームワーク: Arduino
- ボード: Seeed XIAO ESP32S3

## 注意事項
- BLE通信距離は環境により変動（最大約10m）
- 安定した電源供給が必要
- センサーデータの更新頻度はUARTの受信速度に依存