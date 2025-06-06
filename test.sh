#!/bin/sh

set -e

PIPE=pipe
CL_LOG=cl_output.log
SV_LOG=sv_output.log

# クリーンアップ
rm -f $PIPE $CL_LOG_LOG $SV_LOG

# sv をバックグラウンドで起動し、受信内容をログに保存
./sv > $SV_LOG 2>&1 &
SV_PID=$!

# cl をバックグラウンドで起動し、出力をログに保存
./cl > $CL_LOG 2>&1 &
CL_PID=$!

# パイプ作成まで待機
while [ ! -p "$PIPE" ]; do
  sleep 0.1
done

# テストメッセージをパイプに送信
TEST_MSG="integration test message"
echo "$TEST_MSG" > $PIPE

# 処理の完了を少し待つ
sleep 1

# プロセスを終了（必要なら強制停止）
kill $CL_PID 2>/dev/null || true
kill $SV_PID 2>/dev/null || true

# sv の出力にメッセージが届いているか確認
if grep -q "$TEST_MSG" $SV_LOG; then
  echo "cl + sv integration test: PASSED"
  exit 0
else
  echo "cl + sv integration test: FAILED"
  echo "==== cl_output.log ===="
  cat $CL_LOG
  echo "==== sv_output.log ===="
  cat $SV_LOG
  exit 1
fi
