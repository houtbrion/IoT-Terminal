# Xbee Arduino連携

Xbee Arduino連携は Arduino間連携と同じく
アプリケーションプロセッサとなるArduino (MEGA系を想定)
を電源制御回路から電源を投入してセンシング処理を
行わせて，その結果を電源制御回路に接続されている
Xbeeで外部に送信するものである．

## 概要
Arduino間連携との違いは，電源制御回路にMCUを用いず，
Xbeeで周期的にリレーを制御してアプリケーション
プロセッサに電力を供給している．

## プログラムについて
アプリケーションプロセッサのプログラムは
Arduino間連携とまったく同じであるため
そちらを利用すること．

## Xbeeの設定
Xbeeの設定は，スリープモードの4(もしくは5)で
アプリケーションプロセッサの処理が必ず
終わる程度の時間Xbeeが起き続ける設定に
する必要がある．



