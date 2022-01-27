playshogiは2つのusiプログラム同士を対戦させます。

1. 513手に達した将棋は自動的に引き分け
2. 宣言勝ちも自動的に引き分け(通常は条件を満たしてから"win"を送りますが
   条件を満たす手を指した瞬間にplayshogiが判定します)。27点法
3. AobaZeroを使う場合、同時に複数の対戦を走らせ、プロセス間バッチを組むことで高速化できます。
4. 棋譜は標準出力に出ます。
5. 磯崎氏が作成された24手目までの互角定跡集を使って対戦できます。
   http://yaneuraou.yaneu.com/2016/08/24/%E8%87%AA%E5%B7%B1%E5%AF%BE%E5%B1%80%E7%94%A8%E3%81%AB%E4%BA%92%E8%A7%92%E3%81%AE%E5%B1%80%E9%9D%A2%E9%9B%86%E3%82%92%E5%85%AC%E9%96%8B%E3%81%97%E3%81%BE%E3%81%97%E3%81%9F/
   records2016_10818.sfen
   をカレントディレクトリに置いて下さい。
   定跡は起動ごとにランダムに選ばれます。



例:
AobaZero同士を対戦させる場合。800局。互角定跡集を400局使って先後交互に。"-0"が先手です。
bin/playshogi -rsbm 800 -0 "./bin/aobaz -p 100 -w ./weight/w1198.txt" -1 "./bin/aobaz -p 100 -w ./weight/w1198.txt" >> w1198_p100_vs_w1198_p100.csa

AobaZero(1手800playout)とKristallweizen(1手200kノード、1スレッド、定跡なし)を対戦させる場合。プロセス間バッチ利用。HALF利用。weightの指定はplayshogi、aobaz、同じものを指定してください(内部で時々GPUの計算とCPUの計算の一致を確認するため)。
bin/playshogi -rsbm 600 -B 7 -P 18 -U 0 -H 1 -c /bin/bash -W ./weight/w1198.txt -0 "./bin/aobaz -p 800 -e 0 -w ./weight/w1198.txt" -1 "~/Kristallweizen/yane483_nnue_avx2 usi , isready , setoption name BookMoves value 0 , setoption Threads value 1 , setoption NodesLimit value 200000" >> w1198_p800_vs_200k.csa

GPU 0 と GPU 1 を使ってw485とw450を800局対戦。定跡集は使わず。ランダム性としてノイズの追加と最初の30手は確率分布で選択。
bin/playshogi -rsm 800 -P 25 -U 0:1 -B 7:7 -H 1:1 -W w0485.txt:w0450.txt -0 "bin/aobaz -e 0 -p 800 -n -m 30 -w w0485.txt" -1 "bin/aobaz -e 1 -n -m 30 -p 800 -w w0450.txt"

GPU 0 のみを用いてw1650同士を対戦。片方は -msafe 30 で30手目まで乱数性を持たせる。
bin/playshogi -rsm 800 -P 25 -B 7 -U 0 -H 1 -c /bin/bash -W w1650.txt -0 "bin/aobaz -p 800 -msafe 30 -e 0 -w w1650.txt" -1 "bin/aobaz -p 800 -e 0 -w w1650.txt"

2枚落ち。先手が常に下手。"-d 1" は香落ち、以下、角(2)、飛(3)、2枚(4)、4枚(5)、6枚(6)
bin/playshogi -frsm 800 -d 4 -0 "bin/aobaz -p 400 -msafe 30 -w w1525.txt" -1 "bin/aobaz -p 10 -msafe 30 -w w1525.txt" >> 2mai_p400_vs_p10.csa

GPU 0 のみを用いて w70 と w62 を対戦。
bin/p