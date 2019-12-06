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
```

保存したのち、

```sh
arduino-cli core update-index
arduino-cli core install adafruit:nrf52
arduino-cli lib update-index
arduino-cli lib install SPIMemory
arduino-cli lib install SX126x-Arduino
```

## ビルド

```sh
git clone https://github.com/144lab/isp4520-sample.git
cd isp4520-sample
make build
```

## 書き込み
ターゲットをDFUモードにする（後述）
ターゲットのシリアル通信ポートをPORT=に指定して以下のコマンドを実行。
```sh
make PORT=COM4 upload
```

## DFUモードにする方法

ブートローダーをビルドするときの設定で方法が決定する（ターゲットに依存）

- P0_12をLoにしたままRESET(Samaritaine)
- RESETして５秒間DFUモード（isp4520用）

## ブートローダーの入手

https://github.com/144lab/isp4520-sample/issues/1 参照
