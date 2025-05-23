#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// BLEの設定
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// LED設定
#define LED_PIN 21  // 内蔵LED

// シリアル通信用バッファ
char inputBuffer[2];  // 1文字 + NULL終端

// グローバル変数
BLEClient* pClient = NULL;
BLERemoteCharacteristic* pRemoteCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool doConnect = false;
BLEAddress* pServerAddress;
String lastReceivedData = "";  // 前回受信したデータを保存
unsigned long lastReceivedTime = 0;  // 前回データを受信した時刻

// LED制御用変数
unsigned long ledStartTime = 0;
bool ledOn = false;
const int LED_DURATION = 100;  // LED点灯時間（ms）

// 通知コールバック
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    // データを文字列として表示
    String receivedData = "";
    for (int i = 0; i < length; i++) {
        receivedData += (char)pData[i];
    }

    unsigned long currentTime = millis();
    // 前回と同じデータの場合、3秒以上経過しているかチェック
    if (receivedData == lastReceivedData) {
        if (currentTime - lastReceivedTime < 3000) {  // 3秒以内の場合
            return;  // 出力しない
        }
    }

    // データを出力し、現在の情報を保存
    Serial.println(receivedData);
    lastReceivedData = receivedData;
    lastReceivedTime = currentTime;

    // LED点滅
    digitalWrite(LED_PIN, HIGH);
    ledOn = true;
    ledStartTime = millis();
}

// 接続状態のコールバッククラス
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        deviceConnected = true;
    }

    void onDisconnect(BLEClient* pclient) {
        deviceConnected = false;
        // 切断時にLEDを消灯
        digitalWrite(LED_PIN, LOW);
        ledOn = false;
        lastReceivedData = "";  // 前回データをリセット
        lastReceivedTime = 0;   // 前回受信時刻をリセット
        Serial.println("サーバーから切断されました");
    }
};

// アドバタイズデバイスを見つけた時のコールバック
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        // スキャンしたデバイスの情報を表示
        Serial.printf("デバイス発見: %s ", advertisedDevice.toString().c_str());
        
        // RSSI（電波強度）を表示
        Serial.printf("(RSSI: %d)", advertisedDevice.getRSSI());
        
        // デバイス名があれば表示
        if (advertisedDevice.haveName()) {
            Serial.printf(" 名前: %s", advertisedDevice.getName().c_str());
        }
        Serial.println();

        // 対象のデバイスかチェック
        if (advertisedDevice.haveName() && 
            advertisedDevice.getName() == "Yonku Counter BLE Server") {
            BLEDevice::getScan()->stop();
            pServerAddress = new BLEAddress(advertisedDevice.getAddress());
            doConnect = true;
            Serial.println(">>> Yonku Counter BLE Serverが見つかりました! <<<");
        }
    }
};

// BLEサーバーへの接続
bool connectToServer() {
  Serial.print("接続先: ");
  Serial.println(pServerAddress->toString().c_str());
  
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  
  // サーバーに接続
  if (!pClient->connect(*pServerAddress)) {
    return false;
  }
  Serial.println("サーバーに接続しました");

  // サービスの取得
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) {
    Serial.println("サービスが見つかりません");
    pClient->disconnect();
    return false;
  }

  // キャラクタリスティックの取得
  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHARACTERISTIC_UUID));
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("キャラクタリスティックが見つかりません");
    pClient->disconnect();
    return false;
  }

  // 通知の登録
  if(pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }
  
  deviceConnected = true;
  return true;
}

void setup() {
  // シリアル通信の初期化（UTF-8エンコーディングを指定）
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // シリアルポートが準備できるまで待機
  }
  delay(1000);  // 安定化のための待機
  
  Serial.println("\n================");
  Serial.println("BLEクライアントを開始します...");
  
  // LED初期化
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  ledOn = false;  // LED状態を初期化
  Serial.println("LED初期化完了");

  // BLEデバイスの初期化
  BLEDevice::init("Yonku Counter BLE Client");  // わかりやすい名前を設定
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9); // 出力パワーを最大(+9dBm)に設定
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);     // アドバタイジングの出力も最大に
  Serial.println("BLEデバイス初期化完了");

  // スキャンオブジェクトの取得
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  Serial.println("BLEスキャン設定完了");
  
  // スキャン開始
  Serial.println("スキャンを開始します...");
  Serial.println("近くのBLEデバイスを探索中...");
  pBLEScan->start(5, false);
  Serial.println("================");
}

void loop() {
  unsigned long currentMillis = millis();

  // シリアル通信からの入力を確認
  if (Serial.available() > 0) {
    // 1文字読み取り
    inputBuffer[0] = Serial.read();
    inputBuffer[1] = '\0';  // NULL終端

    // 接続中かつ有効な1文字の場合のみ送信
    if (deviceConnected && isalpha(inputBuffer[0])) {
      // BLE経由で送信
      pRemoteCharacteristic->writeValue(inputBuffer);
      Serial.printf("送信: %c\n", inputBuffer[0]);
    }
    
    // バッファをクリア
    while(Serial.available()) {
      Serial.read();
    }
  }

  // 接続状態の変化を検出
  if (deviceConnected != oldDeviceConnected) {
    if (deviceConnected) {
      Serial.println("接続が確立されました");
    }
    oldDeviceConnected = deviceConnected;
  }

  // 接続が必要な場合
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("接続に成功しました");
    } else {
      Serial.println("接続に失敗しました");
      deviceConnected = false;
    }
    doConnect = false;
  }

  // 未接続状態での再スキャン制御
  static unsigned long lastScanTime = 0;
  const long scanInterval = 5000;  // スキャン間隔（5秒）
  
  if (!deviceConnected && (currentMillis - lastScanTime >= scanInterval)) {
    lastScanTime = currentMillis;
    Serial.println("\n-------------------");
    Serial.println("デバイスをスキャンしています...");
    Serial.println("近くのBLEデバイスを探索中...");
    BLEDevice::getScan()->start(5, false);  // 5秒間スキャン
  }

  // LED制御（確実に消灯するように修正）
  if (ledOn) {
    if (currentMillis - ledStartTime >= LED_DURATION) {
      digitalWrite(LED_PIN, LOW);
      ledOn = false;
    }
  } else if (digitalRead(LED_PIN) == HIGH) {
    // 予期しないLEDの点灯を防止
    digitalWrite(LED_PIN, LOW);
  }
  // delay(1);
}
