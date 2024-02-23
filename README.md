# micrec
Sample Code.
Record from the microphone and create an mp3 file.

マイク入力音声からMP3ファイルを作成するサンプルコード。

# 環境
- Linux
- ALSA
- LAME

# 雑多
## 必要なライブラリのインストール
```shell
sudo apt install libasound-dev
sudo apt install libmp3lame-dev
```

## ビルドと実行
```shell
make micrec
./micrec デバイス名 録音時間(秒)
```

## PCに接続されたマイクの一覧表示
```shell
arecord -L
```
