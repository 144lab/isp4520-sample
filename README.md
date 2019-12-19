# isp4520-sample

## ビルド環境 for Windows

- scoopのインストール
- scoop install make git go python
- pip3 install adafruit-nrfutil
- go get github.com/arduino/arduino-cli

これをダウンロード＆解凍してexeを％USERPROFILE％\scoop\apps\に移動。
https://github.com/arduino/arduino-cli/releases/download/0.6.0/arduino-cli_0.6.0_Windows_64bit.zip

```sh
arduino-cli version
```
にてバージョンが表示されることを確認。

```sh
arduino-cli config init
```

C:\Users\irieda\AppData\Local\Arduino15\arduino-cli.yaml を編集。

「board_manager: {}」を以下のように書き換える

```yaml
board_manager:
  additional_urls:
  - https://www.adafruit.com/package_adafruit_index.json
  - https://144lab.github.io/arduino_packages/package_144lab_index.json
```

保存したのち、

```sh
arduino-cli core update-index
arduino-cli core install 144lab:nrf52
arduino-cli lib update-index
arduino-cli lib install SX126x-Arduino
```

## LoRaTransmitterのビルド

```sh
git clone https://github.com/144lab/isp4520-sample.git
cd isp4520-sample
make REGION=REGION_EU868 build # EU版
make REGION=REGION_AS923 build # JP版
```

## LoRaTransmitterの書き込み
[ブートローダー:sirc_isp4520_bootloader-0.2.13-5-gc4cf262-dirty_s132_6.1.1.hex.zip](https://github.com/144lab/isp4520-sample/files/3982436/sirc_isp4520_bootloader-0.2.13-5-gc4cf262-dirty_s132_6.1.1.hex.zip)
これを解凍してJLinkを使って書き込む。

ターゲットをDFUモードにする（後述）
ターゲットのシリアル通信ポートをPORT=に指定して以下のコマンドを実行。
(実行後すぐにターゲットをリセットする)
```sh
make PORT=COM4 upload
```

## LoRaReceiverのビルド

```sh
make NAME=LoRaReceiver RX_CHANNEL=0 REGION=REGION_EU868 build # EU版 チャンネル0
make NAME=LoRaReceiver RX_CHANNEL=1 REGION=REGION_EU868 build # EU版 チャンネル1
make NAME=LoRaReceiver RX_CHANNEL=2 REGION=REGION_EU868 build # EU版 チャンネル2
make NAME=LoRaReceiver RX_CHANNEL=0 REGION=REGION_AS923 build # JP版 チャンネル0
make NAME=LoRaReceiver RX_CHANNEL=1 REGION=REGION_AS923 build # JP版 チャンネル1
make NAME=LoRaReceiver RX_CHANNEL=2 REGION=REGION_AS923 build # JP版 チャンネル2
```

## LoRaReceiverの書き込み
[ブートローダー:isp4520_bootloader-0.2.13-5-gc4cf262-dirty_s132_6.1.1.hex.zip](https://github.com/144lab/isp4520-sample/files/3932885/isp4520_bootloader-0.2.13-5-gc4cf262-dirty_s132_6.1.1.hex.zip)
これを解凍してJLinkを使って書き込む。

ターゲットをDFUモードにする（後述）
ターゲットのシリアル通信ポートをPORT=に指定して以下のコマンドを実行。
(実行後すぐにターゲットをリセットする)
```sh
make NAME=LoRaReceiver PORT=COM5 upload
```

## DFUモードにする方法

ブートローダーをビルドするときの設定で方法が決定する（ターゲットに依存）

- RESETして５秒間DFUモード（isp4520用）
- RESETして10秒間DFUモード(リレー付きPOC基板)

## ブートローダーの入手

https://github.com/144lab/isp4520-sample/issues/1 参照
